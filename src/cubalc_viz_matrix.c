#define _POSIX_C_SOURCE 200809L
#include "cubalc_viz_matrix.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

static int64_t mono_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static uint64_t pack_word(const cubalc_matrix *m) {
  if (!m) return 0;
  uint64_t w = 0;
  int nbytes = (CUBALC_ATOM_BITS + 7) / 8;
  if (nbytes > (int)sizeof(w)) nbytes = (int)sizeof(w);
  memcpy(&w, m->bits, (size_t)nbytes);
  return w;
}

void cubalc_viz_frame_clear(cubalc_viz_frame *f) {
  if (!f) return;
  memset(f, 0, sizeof *f);
  f->magic = CUBALC_VIZ_MAGIC;
  f->version = CUBALC_VIZ_VERSION;
  f->n_bits = CUBALC_VIZ_BITS;
}

int cubalc_viz_from_matrix(const cubalc_matrix *m, cubalc_viz_frame *out,
                           uint32_t seq, float unity, int digit, int n_ch,
                           const char *source, const char *backend) {
  if (!out) return -1;
  cubalc_viz_frame_clear(out);
  out->seq = seq;
  out->unity = unity;
  out->digit = (uint16_t)(digit < 0 ? 0 : (digit > 9 ? digit % 10 : digit));
  out->t_ms = mono_ms();
  out->n_ch = (uint16_t)(n_ch < 0 ? 0 : (n_ch > 16 ? 16 : n_ch));
  if (source && source[0])
    snprintf(out->source, sizeof out->source, "%s", source);
  else
    snprintf(out->source, sizeof out->source, "matrix");
  if (backend && backend[0])
    snprintf(out->backend, sizeof out->backend, "%s", backend);
  else
    snprintf(out->backend, sizeof out->backend, "cpu");

  out->word = pack_word(m);
  out->set = 0;
  int feats = 8;
  if (out->n_ch > 8) feats = 4;
  if (out->n_ch < 1) feats = 0;

  for (int i = 0; i < CUBALC_VIZ_BITS; i++) {
    int on = m ? cubalc_matrix_get(m, i) : 0;
    out->bits[i] = on ? 1 : 0;
    if (on) out->set++;
    out->cells[i].on = out->bits[i];
    out->cells[i].heat = on ? 1.f : 0.f;
    if (feats > 0 && out->n_ch > 0) {
      out->cells[i].ch = (uint8_t)(i / feats);
      out->cells[i].feat = (uint8_t)(i % feats);
      if (out->cells[i].ch >= out->n_ch) {
        out->cells[i].ch = 0;
        out->cells[i].feat = (uint8_t)i;
      }
    } else {
      out->cells[i].ch = 0;
      out->cells[i].feat = (uint8_t)i;
    }
  }
  if (m && m->set > 0 && out->set == 0)
    out->set = m->set; /* prefer tracked set if bit walk empty */
  return 0;
}

void cubalc_viz_heat_blend(cubalc_viz_frame *cur, const cubalc_viz_frame *prev,
                           float decay) {
  if (!cur) return;
  if (decay < 0.f) decay = 0.f;
  if (decay > 0.99f) decay = 0.99f;
  if (!prev || prev->magic != CUBALC_VIZ_MAGIC) return;
  for (int i = 0; i < CUBALC_VIZ_BITS; i++) {
    float h = prev->cells[i].heat * decay;
    if (cur->bits[i]) h = 1.f;
    else if (h < 0.02f) h = 0.f;
    cur->cells[i].heat = h;
  }
}

int cubalc_viz_write_bin(const cubalc_viz_frame *f, FILE *out) {
  if (!f || !out) return -1;
  return fwrite(f, sizeof *f, 1, out) == 1 ? 0 : -1;
}

int cubalc_viz_read_bin(cubalc_viz_frame *f, FILE *in) {
  if (!f || !in) return -1;
  if (fread(f, sizeof *f, 1, in) != 1) return -1;
  if (f->magic != CUBALC_VIZ_MAGIC) return -1;
  return 0;
}

int cubalc_viz_write_bin_path(const cubalc_viz_frame *f, const char *path) {
  if (!path) return -1;
  FILE *fp = fopen(path, "wb");
  if (!fp) return -1;
  int rc = cubalc_viz_write_bin(f, fp);
  fclose(fp);
  return rc;
}

int cubalc_viz_read_bin_path(cubalc_viz_frame *f, const char *path) {
  if (!path) return -1;
  FILE *fp = fopen(path, "rb");
  if (!fp) return -1;
  int rc = cubalc_viz_read_bin(f, fp);
  fclose(fp);
  return rc;
}

static int json_escape(const char *s, char *out, size_t cap) {
  if (!out || cap < 1) return -1;
  size_t j = 0;
  out[0] = 0;
  if (!s) return 0;
  for (const unsigned char *p = (const unsigned char *)s; *p && j + 2 < cap; p++) {
    if (*p == '"' || *p == '\\') {
      if (j + 3 >= cap) break;
      out[j++] = '\\';
      out[j++] = (char)*p;
    } else if (*p < 32) {
      /* skip control */
    } else {
      out[j++] = (char)*p;
    }
  }
  out[j] = 0;
  return 0;
}

int cubalc_viz_write_json_buf(const cubalc_viz_frame *f, char *buf, size_t cap) {
  if (!f || !buf || cap < 64) return -1;
  char bits[CUBALC_VIZ_BITS + 1];
  for (int i = 0; i < CUBALC_VIZ_BITS; i++)
    bits[i] = f->bits[i] ? '1' : '0';
  bits[CUBALC_VIZ_BITS] = 0;
  char src[64], back[64];
  json_escape(f->source, src, sizeof src);
  json_escape(f->backend, back, sizeof back);

  /* heat as compact array of 0-100 ints */
  char heat[CUBALC_VIZ_BITS * 4 + 8];
  size_t hi = 0;
  heat[hi++] = '[';
  for (int i = 0; i < CUBALC_VIZ_BITS; i++) {
    int h = (int)(f->cells[i].heat * 100.f + 0.5f);
    if (h < 0) h = 0;
    if (h > 100) h = 100;
    int n = snprintf(heat + hi, sizeof heat - hi, "%s%d", i ? "," : "", h);
    if (n < 0 || (size_t)n >= sizeof heat - hi) break;
    hi += (size_t)n;
  }
  if (hi + 1 < sizeof heat) { heat[hi++] = ']'; heat[hi] = 0; }

  int n = snprintf(buf, cap,
    "{\"schema\":\"cube.viz.matrix.v1\",\"magic\":\"CVZM\",\"version\":%u,"
    "\"n_bits\":%u,\"n_ch\":%u,\"digit\":%u,\"set\":%u,\"seq\":%u,"
    "\"t_ms\":%lld,\"unity\":%.6f,\"source\":\"%s\",\"backend\":\"%s\","
    "\"word\":\"0x%016llx\",\"bits\":\"%s\",\"heat\":%s,"
    "\"law\":\"state_matrix_soT;raw_c_viz;realtime_stream\"}",
    (unsigned)f->version, (unsigned)f->n_bits, (unsigned)f->n_ch,
    (unsigned)f->digit, (unsigned)f->set, (unsigned)f->seq,
    (long long)f->t_ms, f->unity, src, back,
    (unsigned long long)f->word, bits, heat);
  if (n < 0 || (size_t)n >= cap) return -1;
  return n;
}

int cubalc_viz_write_json(const cubalc_viz_frame *f, FILE *out) {
  if (!out) return -1;
  char buf[4096];
  if (cubalc_viz_write_json_buf(f, buf, sizeof buf) < 0) return -1;
  return fputs(buf, out) >= 0 && fputc('\n', out) != EOF ? 0 : -1;
}

int cubalc_viz_write_json_path(const cubalc_viz_frame *f, const char *path) {
  if (!path) return -1;
  FILE *fp = fopen(path, "wb");
  if (!fp) return -1;
  int rc = cubalc_viz_write_json(f, fp);
  fclose(fp);
  return rc;
}

void cubalc_viz_ring_init(cubalc_viz_ring *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

void cubalc_viz_ring_push(cubalc_viz_ring *r, const cubalc_viz_frame *f) {
  if (!r || !f) return;
  r->frames[r->head] = *f;
  r->head = (r->head + 1) % CUBALC_VIZ_RING;
  if (r->n < CUBALC_VIZ_RING) r->n++;
}

int cubalc_viz_ring_get(const cubalc_viz_ring *r, int age, cubalc_viz_frame *out) {
  if (!r || !out || r->n < 1 || age < 0 || age >= r->n) return -1;
  int idx = r->head - 1 - age;
  while (idx < 0) idx += CUBALC_VIZ_RING;
  idx %= CUBALC_VIZ_RING;
  *out = r->frames[idx];
  return 0;
}

int cubalc_viz_ring_latest(const cubalc_viz_ring *r, cubalc_viz_frame *out) {
  return cubalc_viz_ring_get(r, 0, out);
}

int cubalc_viz_ring_write_json(const cubalc_viz_ring *r, int limit, FILE *out) {
  if (!r || !out) return -1;
  int n = r->n;
  if (limit > 0 && limit < n) n = limit;
  fputc('[', out);
  for (int age = n - 1; age >= 0; age--) {
    cubalc_viz_frame f;
    if (cubalc_viz_ring_get(r, age, &f) != 0) continue;
    if (age < n - 1) fputc(',', out);
    char buf[4096];
    if (cubalc_viz_write_json_buf(&f, buf, sizeof buf) < 0) continue;
    fputs(buf, out);
  }
  fputs("]\n", out);
  return 0;
}

int cubalc_viz_ring_write_json_path(const cubalc_viz_ring *r, int limit,
                                    const char *path) {
  if (!path) return -1;
  FILE *fp = fopen(path, "wb");
  if (!fp) return -1;
  int rc = cubalc_viz_ring_write_json(r, limit, fp);
  fclose(fp);
  return rc;
}

int cubalc_viz_rgba8(const cubalc_viz_frame *f, int grid_w, int grid_h,
                     uint8_t *rgba, int pitch) {
  if (!f || !rgba || grid_w < 1 || grid_h < 1) return -1;
  if (pitch <= 0) pitch = grid_w * 4;
  for (int y = 0; y < grid_h; y++) {
    for (int x = 0; x < grid_w; x++) {
      int i = y * grid_w + x;
      uint8_t *p = rgba + y * pitch + x * 4;
      if (i >= CUBALC_VIZ_BITS) {
        p[0] = 6; p[1] = 10; p[2] = 8; p[3] = 255;
        continue;
      }
      float h = f->cells[i].heat;
      if (f->bits[i]) {
        /* live bit: cyan-green */
        p[0] = (uint8_t)(40 + 40 * h);
        p[1] = (uint8_t)(180 + 60 * h);
        p[2] = (uint8_t)(140 + 40 * h);
        p[3] = 255;
      } else if (h > 0.02f) {
        /* heat trail */
        p[0] = (uint8_t)(20 + 60 * h);
        p[1] = (uint8_t)(40 + 80 * h);
        p[2] = (uint8_t)(30 + 50 * h);
        p[3] = 255;
      } else {
        p[0] = 8; p[1] = 12; p[2] = 10; p[3] = 255;
      }
    }
  }
  return 0;
}
