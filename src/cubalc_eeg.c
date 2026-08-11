#define _POSIX_C_SOURCE 200809L
#include "cubalc_eeg.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

void cubalc_eeg_frame_clear(cubalc_eeg_frame *f) {
  if (!f) return;
  memset(f, 0, sizeof *f);
  f->scale_uv = CUBALC_EEG_DEF_SCALE;
  f->n_ch = CUBALC_EEG_DEF_CH;
}

void cubalc_eeg_frame_init(cubalc_eeg_frame *f, int n_ch, float scale_uv) {
  cubalc_eeg_frame_clear(f);
  if (n_ch < 1) n_ch = 1;
  if (n_ch > CUBALC_EEG_MAX_CH) n_ch = CUBALC_EEG_MAX_CH;
  f->n_ch = n_ch;
  if (scale_uv > 0.f) f->scale_uv = scale_uv;
}

int cubalc_eeg_frame_set(cubalc_eeg_frame *f, int ch, int samp, float uv) {
  if (!f || ch < 0 || ch >= f->n_ch || samp < 0 || samp >= CUBALC_EEG_MAX_WIN)
    return -1;
  f->samples[ch * CUBALC_EEG_MAX_WIN + samp] = uv;
  if (samp + 1 > f->n_samp) f->n_samp = samp + 1;
  return 0;
}

/* 8 feature bits for one channel window */
static uint8_t pack_ch8(const float *w, int n, float scale) {
  if (n < 1) return 0;
  if (scale < 1e-6f) scale = CUBALC_EEG_DEF_SCALE;
  double sum = 0.0, sum2 = 0.0, absmax = 0.0;
  int zc = 0;
  float prev = w[0];
  for (int i = 0; i < n; i++) {
    float v = w[i];
    sum += (double)v;
    sum2 += (double)v * (double)v;
    double a = fabs((double)v);
    if (a > absmax) absmax = a;
    if (i > 0) {
      if ((prev >= 0.f && v < 0.f) || (prev < 0.f && v >= 0.f)) zc++;
      prev = v;
    }
  }
  double mean = sum / (double)n;
  double rms = sqrt(sum2 / (double)n);
  float rise = w[n - 1] - w[0];
  double var = (sum2 / (double)n) - mean * mean;
  if (var < 0.0) var = 0.0;

  uint8_t b = 0;
  if (mean >= 0.0)            b |= 1u << 0; /* polarity */
  if (absmax > (double)scale) b |= 1u << 1; /* energy mid */
  if (absmax > 2.0 * (double)scale) b |= 1u << 2; /* energy high */
  if (zc * 4 >= n)            b |= 1u << 3; /* zero-cross dense */
  if (rms > 0.5 * (double)scale) b |= 1u << 4; /* rms active */
  if (rise > 0.f)             b |= 1u << 5; /* rising */
  if (var < 0.05 * (double)scale * (double)scale) b |= 1u << 6; /* flat */
  if (absmax > 5.0 * (double)scale) b |= 1u << 7; /* clip/high */
  return b;
}

/* 4 feature bits for dense channel packing (16 ch): polarity·mid·high·zc */
static uint8_t pack_ch4(const float *w, int n, float scale) {
  return (uint8_t)(pack_ch8(w, n, scale) & 0x0Fu);
}

int cubalc_eeg_pack_matrix(const cubalc_eeg_frame *f, cubalc_matrix *out) {
  if (!f || !out || f->n_ch < 1) return -1;
  cubalc_matrix_clear(out);
  out->n = CUBALC_ATOM_BITS;
  int n_samp = f->n_samp > 0 ? f->n_samp : 1;
  if (n_samp > CUBALC_EEG_MAX_WIN) n_samp = CUBALC_EEG_MAX_WIN;
  float scale = f->scale_uv > 0.f ? f->scale_uv : CUBALC_EEG_DEF_SCALE;

  if (f->n_ch <= 8) {
    /* 8 bits × up to 8 channels = 64 */
    for (int c = 0; c < f->n_ch && c < 8; c++) {
      const float *w = &f->samples[c * CUBALC_EEG_MAX_WIN];
      uint8_t bits = pack_ch8(w, n_samp, scale);
      for (int b = 0; b < 8; b++) {
        int idx = c * 8 + b;
        if (idx >= CUBALC_ATOM_BITS) break;
        if (bits & (1u << b))
          cubalc_matrix_set(out, idx, 1);
      }
    }
  } else {
    /* 4 bits × up to 16 channels = 64 */
    for (int c = 0; c < f->n_ch && c < 16; c++) {
      const float *w = &f->samples[c * CUBALC_EEG_MAX_WIN];
      uint8_t bits = pack_ch4(w, n_samp, scale) & 0x0Fu;
      for (int b = 0; b < 4; b++) {
        int idx = c * 4 + b;
        if (idx >= CUBALC_ATOM_BITS) break;
        if (bits & (1u << b))
          cubalc_matrix_set(out, idx, 1);
      }
    }
  }
  return 0;
}

int cubalc_eeg_pack_samples(const float *samples, int n_ch, int n_samp,
                            float scale_uv, cubalc_matrix *out) {
  if (!samples || !out || n_ch < 1 || n_samp < 1) return -1;
  cubalc_eeg_frame f;
  cubalc_eeg_frame_init(&f, n_ch, scale_uv);
  if (n_samp > CUBALC_EEG_MAX_WIN) n_samp = CUBALC_EEG_MAX_WIN;
  if (n_ch > CUBALC_EEG_MAX_CH) n_ch = CUBALC_EEG_MAX_CH;
  f.n_samp = n_samp;
  /* samples interleaved by sample: samples[i*n_ch + c] */
  for (int i = 0; i < n_samp; i++)
    for (int c = 0; c < n_ch; c++)
      f.samples[c * CUBALC_EEG_MAX_WIN + i] = samples[i * n_ch + c];
  return cubalc_eeg_pack_matrix(&f, out);
}

int cubalc_eeg_pack_csv_line(const char *line, int n_ch, float scale_uv,
                             cubalc_matrix *out) {
  if (!line || !out) return -1;
  if (n_ch < 1) n_ch = CUBALC_EEG_DEF_CH;
  if (n_ch > CUBALC_EEG_MAX_CH) n_ch = CUBALC_EEG_MAX_CH;
  float ch[CUBALC_EEG_MAX_CH];
  memset(ch, 0, sizeof ch);
  int got = 0;
  const char *p = line;
  while (*p && got < n_ch) {
    while (*p && (isspace((unsigned char)*p) || *p == ',' || *p == ';')) p++;
    if (!*p) break;
    char *end = NULL;
    float v = strtof(p, &end);
    if (end == p) break;
    ch[got++] = v;
    p = end;
  }
  if (got < 1) return -1;
  /* single sample frame */
  cubalc_eeg_frame f;
  cubalc_eeg_frame_init(&f, got, scale_uv);
  f.n_samp = 1;
  for (int c = 0; c < got; c++)
    f.samples[c * CUBALC_EEG_MAX_WIN] = ch[c];
  return cubalc_eeg_pack_matrix(&f, out);
}

void cubalc_eeg_matrix_bits(const cubalc_matrix *m, char *out, int out_max) {
  if (!out || out_max < 2) return;
  int n = CUBALC_ATOM_BITS;
  if (out_max - 1 < n) n = out_max - 1;
  for (int i = 0; i < n; i++)
    out[i] = (m && cubalc_matrix_get(m, i)) ? '1' : '0';
  out[n] = 0;
}

void cubalc_eeg_demo_frame(cubalc_eeg_frame *f, int n_ch, long seed) {
  if (!f) return;
  if (n_ch < 1) n_ch = CUBALC_EEG_DEF_CH;
  cubalc_eeg_frame_init(f, n_ch, CUBALC_EEG_DEF_SCALE);
  int win = 32;
  f->n_samp = win;
  uint32_t rng = (uint32_t)(seed ? seed : 0xC0BEA160u);
  for (int c = 0; c < f->n_ch; c++) {
    /* ~10 Hz alpha-ish + channel phase */
    double phase = (double)c * 0.7 + (double)(rng & 0xff) * 0.01;
    for (int i = 0; i < win; i++) {
      rng = rng * 1664525u + 1013904223u;
      double t = (double)i / 32.0;
      double sig = 30.0 * sin(2.0 * 3.141592653589793 * 10.0 * t + phase)
                 + 8.0 * sin(2.0 * 3.141592653589793 * 20.0 * t)
                 + ((int)(rng % 21) - 10) * 0.5; /* noise */
      f->samples[c * CUBALC_EEG_MAX_WIN + i] = (float)sig;
    }
  }
}

int cubalc_eeg_read_csv_sample(FILE *fp, float *ch_out, int n_ch) {
  if (!fp || !ch_out || n_ch < 1) return -1;
  char line[4096];
  if (!fgets(line, sizeof line, fp)) return -1;
  /* skip comments / blank */
  char *p = line;
  while (*p && isspace((unsigned char)*p)) p++;
  if (!*p || *p == '#' || *p == '/') return 1;
  /* NDJSON light path: {"ch":[1,2,3]} */
  if (*p == '{') {
    char *br = strchr(p, '[');
    if (!br) return 1;
    p = br + 1;
  }
  int got = 0;
  while (*p && got < n_ch) {
    while (*p && (isspace((unsigned char)*p) || *p == ',' || *p == ';' || *p == '[' || *p == ']')) p++;
    if (!*p || *p == '}') break;
    char *end = NULL;
    float v = strtof(p, &end);
    if (end == p) break;
    ch_out[got++] = v;
    p = end;
  }
  if (got < 1) return 1;
  while (got < n_ch) ch_out[got++] = 0.f;
  return 0;
}

int cubalc_eeg_window_push(cubalc_eeg_frame *f, const float *ch, int n_ch,
                           int win_target, int force, cubalc_matrix *out) {
  if (!f || !ch || !out) return -1;
  if (n_ch < 1) n_ch = f->n_ch > 0 ? f->n_ch : CUBALC_EEG_DEF_CH;
  if (f->n_ch < 1) f->n_ch = n_ch;
  if (win_target < 1) win_target = 16;
  if (win_target > CUBALC_EEG_MAX_WIN) win_target = CUBALC_EEG_MAX_WIN;
  int s = f->n_samp;
  if (s >= CUBALC_EEG_MAX_WIN) {
    /* slide window left by 1 */
    for (int c = 0; c < f->n_ch; c++) {
      memmove(&f->samples[c * CUBALC_EEG_MAX_WIN],
              &f->samples[c * CUBALC_EEG_MAX_WIN + 1],
              (size_t)(CUBALC_EEG_MAX_WIN - 1) * sizeof(float));
    }
    s = CUBALC_EEG_MAX_WIN - 1;
    f->n_samp = s;
  }
  int use_ch = n_ch < f->n_ch ? n_ch : f->n_ch;
  for (int c = 0; c < use_ch; c++)
    f->samples[c * CUBALC_EEG_MAX_WIN + s] = ch[c];
  f->n_samp = s + 1;
  if (f->n_samp >= win_target || force) {
    if (cubalc_eeg_pack_matrix(f, out) != 0) return -1;
    return 1;
  }
  return 0;
}

int cubalc_eeg_status_json(const cubalc_matrix *m, int n_ch, long seq,
                           const char *backend, FILE *out) {
  if (!out) return -1;
  char bits[CUBALC_ATOM_BITS + 1];
  cubalc_eeg_matrix_bits(m, bits, sizeof bits);
  unsigned set = m ? (unsigned)m->set : 0;
  fprintf(out,
    "{\"schema\":\"cube.eeg.matrix.v1\",\"engine\":\"cubalc_eeg\","
    "\"n_ch\":%d,\"seq\":%ld,\"set\":%u,\"bits\":\"%s\","
    "\"backend\":\"%s\",\"law\":\"eeg→state_matrix_atom\"}\n",
    n_ch, seq, set, bits, backend ? backend : "cpu");
  return 0;
}
