#define _POSIX_C_SOURCE 200809L
#include "cubalc_cubechain.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

/* Fast cube hash (FNV-1a avalanche ×4 lanes) — machine-friendly, C-speed.
 * Not a bank; CubeChain integrity for Matrix State storage. */
void cubalc_cube_hash(const void *data, size_t n, uint8_t out[CUBALC_HASH_LEN]) {
  const uint8_t *p = (const uint8_t *)data;
  uint64_t h[4] = {
    0xcbf29ce484222325ULL, 0x100000001b3ULL,
    0x84222325cbf29ce4ULL, 0x9e3779b97f4a7c15ULL
  };
  for (size_t i = 0; i < n; i++) {
    h[i & 3] ^= p[i];
    h[i & 3] *= 0x100000001b3ULL;
    h[(i + 1) & 3] ^= h[i & 3] >> 17;
  }
  for (int k = 0; k < 4; k++) {
    h[k] ^= h[(k + 1) & 3] << 13;
    h[k] ^= h[(k + 3) & 3] >> 7;
    h[k] *= 0x9e3779b97f4a7c15ULL;
  }
  for (int k = 0; k < 4; k++) {
    uint64_t v = h[k];
    for (int b = 0; b < 8; b++)
      out[k * 8 + b] = (uint8_t)(v >> (b * 8));
  }
}

static void hex32(const uint8_t h[CUBALC_HASH_LEN], char *out, size_t n) {
  static const char *x = "0123456789abcdef";
  size_t i, o = 0;
  for (i = 0; i < CUBALC_HASH_LEN && o + 2 < n; i++) {
    out[o++] = x[h[i] >> 4];
    out[o++] = x[h[i] & 15];
  }
  if (o < n) out[o] = 0;
}

static void default_dir(char *buf, size_t n) {
  const char *e = getenv("CUBALC_STATE");
  if (e && e[0]) {
    snprintf(buf, n, "%s", e);
    return;
  }
  /* relative state next to cwd — portable; lab may export CUBALC_STATE */
  snprintf(buf, n, "state");
}

int cubalc_cchn_open(cubalc_cchn *cc, const char *dir_or_null) {
  if (!cc) return -1;
  memset(cc, 0, sizeof(*cc));
  char dir[400];
  if (dir_or_null && dir_or_null[0])
    snprintf(dir, sizeof dir, "%s", dir_or_null);
  else
    default_dir(dir, sizeof dir);
  mkdir(dir, 0755);
  snprintf(cc->path, sizeof cc->path, "%s/cubechain.cchn", dir);
  cc->hold_flash = CUBALC_HOLD_FLASH;
  /* load tip if exists */
  FILE *f = fopen(cc->path, "rb");
  if (!f) {
    /* empty chain genesis hash = zeros */
    cc->tip_seq = 0;
    memset(cc->tip_hash, 0, CUBALC_HASH_LEN);
    cc->open = 1;
    return 0;
  }
  uint8_t tip[CUBALC_HASH_LEN];
  memset(tip, 0, sizeof tip);
  uint32_t seq = 0;
  for (;;) {
    cubalc_cchn_block hdr;
    size_t nr = fread(&hdr, 1, sizeof hdr, f);
    if (nr == 0) break;
    if (nr != sizeof hdr || hdr.magic != CUBALC_CCHN_MAGIC) {
      fclose(f);
      return -2;
    }
    size_t mbytes = (hdr.n_bits + 7) / 8;
    if (mbytes > 256) mbytes = 256;
    uint8_t body[512];
    size_t body_n = mbytes + hdr.payload_len;
    if (body_n > sizeof body) {
      fseek(f, (long)body_n, SEEK_CUR);
      seq = hdr.seq;
      continue;
    }
    if (fread(body, 1, body_n, f) != body_n) break;
    /* recompute block hash = hash(hdr without prev? full hdr+body) */
    {
      uint8_t raw[sizeof(hdr) + 512];
      memcpy(raw, &hdr, sizeof hdr);
      memcpy(raw + sizeof hdr, body, body_n);
      cubalc_cube_hash(raw, sizeof hdr + body_n, tip);
    }
    seq = hdr.seq;
  }
  fclose(f);
  cc->tip_seq = seq;
  memcpy(cc->tip_hash, tip, CUBALC_HASH_LEN);
  cc->open = 1;
  return 0;
}

void cubalc_cchn_close(cubalc_cchn *cc) {
  if (cc) cc->open = 0;
}

static int append_raw(cubalc_cchn *cc, cubalc_cchn_block *hdr,
                      const uint8_t *mbytes, size_t mlen,
                      const uint8_t *payload, uint32_t plen) {
  if (!cc || !cc->open || !hdr) return -1;
  hdr->magic = CUBALC_CCHN_MAGIC;
  hdr->proto = CUBALC_CCHN_PROTO;
  hdr->seq = cc->tip_seq + 1;
  memcpy(hdr->prev_hash, cc->tip_hash, CUBALC_HASH_LEN);
  hdr->payload_len = plen;
  FILE *f = fopen(cc->path, "ab");
  if (!f) return -1;
  if (fwrite(hdr, 1, sizeof *hdr, f) != sizeof *hdr) { fclose(f); return -1; }
  if (mlen && fwrite(mbytes, 1, mlen, f) != mlen) { fclose(f); return -1; }
  if (plen && payload && fwrite(payload, 1, plen, f) != plen) { fclose(f); return -1; }
  fclose(f);
  /* tip hash */
  {
    size_t total = sizeof *hdr + mlen + plen;
    uint8_t *raw = malloc(total);
    if (!raw) return -1;
    memcpy(raw, hdr, sizeof *hdr);
    if (mlen) memcpy(raw + sizeof *hdr, mbytes, mlen);
    if (plen && payload) memcpy(raw + sizeof *hdr + mlen, payload, plen);
    cubalc_cube_hash(raw, total, cc->tip_hash);
    free(raw);
  }
  cc->tip_seq = hdr->seq;
  return 0;
}

int cubalc_cchn_append_cube(cubalc_cchn *cc, const cubalc_cube *cube,
                            uint16_t flags, const uint8_t *payload, uint32_t plen) {
  if (!cc || !cube) return -1;
  cubalc_cchn_block hdr;
  memset(&hdr, 0, sizeof hdr);
  hdr.flags = flags | (CUBALC_HOLD_FLASH ? 1u : 0u);
  strncpy(hdr.cube_id, cube->id, CUBALC_ID_LEN - 1);
  hdr.proton = cube->atom.proton;
  hdr.digit = cube->atom.digit;
  hdr.set = cube->atom.matrix.set;
  hdr.n_bits = cube->atom.matrix.n ? cube->atom.matrix.n : CUBALC_ATOM_BITS;
  hdr.energy_q8 = (uint8_t)(cube->atom.energy * 255.f);
  size_t mlen = (hdr.n_bits + 7) / 8;
  cubalc_cube_hash(cube->atom.matrix.bits, mlen, hdr.matrix_hash);
  return append_raw(cc, &hdr, cube->atom.matrix.bits, mlen, payload, plen);
}

int cubalc_cchn_append_chain(cubalc_cchn *cc, const cubalc_chain *ch, uint16_t flags) {
  if (!cc || !ch) return -1;
  /* genesis matrix as first block if empty */
  if (cc->tip_seq == 0) {
    cubalc_cube gen;
    memset(&gen, 0, sizeof gen);
    strncpy(gen.id, "genesis", CUBALC_ID_LEN - 1);
    gen.atom.matrix = ch->initial;
    gen.atom.proton = 1;
    gen.atom.alive = 1;
    gen.atom.energy = 1.f;
    gen.atom.digit = cubalc_algocube_digit(&ch->initial);
    if (cubalc_cchn_append_cube(cc, &gen, flags | 2u, NULL, 0) != 0) return -1;
  }
  for (int i = 0; i < ch->n_cubes; i++) {
    if (cubalc_cchn_append_cube(cc, &ch->cubes[i], flags, NULL, 0) != 0)
      return -1;
  }
  return 0;
}

int cubalc_cchn_verify(const char *path, char *err, size_t err_n) {
  if (err && err_n) err[0] = 0;
  FILE *f = fopen(path, "rb");
  if (!f) {
    if (err) snprintf(err, err_n, "open_fail");
    return -1;
  }
  uint8_t expect_prev[CUBALC_HASH_LEN];
  memset(expect_prev, 0, sizeof expect_prev);
  uint32_t n = 0;
  for (;;) {
    cubalc_cchn_block hdr;
    size_t nr = fread(&hdr, 1, sizeof hdr, f);
    if (nr == 0) break;
    if (nr != sizeof hdr || hdr.magic != CUBALC_CCHN_MAGIC) {
      if (err) snprintf(err, err_n, "bad_magic_at_%u", n);
      fclose(f);
      return -2;
    }
    if (memcmp(hdr.prev_hash, expect_prev, CUBALC_HASH_LEN) != 0) {
      if (err) snprintf(err, err_n, "prev_hash_break_seq_%u", hdr.seq);
      fclose(f);
      return -3;
    }
    size_t mlen = (hdr.n_bits + 7) / 8;
    uint8_t body[1024];
    size_t body_n = mlen + hdr.payload_len;
    if (body_n > sizeof body) {
      if (err) snprintf(err, err_n, "block_too_big");
      fclose(f);
      return -4;
    }
    if (fread(body, 1, body_n, f) != body_n) {
      if (err) snprintf(err, err_n, "truncated");
      fclose(f);
      return -5;
    }
    uint8_t mh[CUBALC_HASH_LEN];
    cubalc_cube_hash(body, mlen, mh);
    if (memcmp(mh, hdr.matrix_hash, CUBALC_HASH_LEN) != 0) {
      if (err) snprintf(err, err_n, "matrix_hash_mismatch_seq_%u", hdr.seq);
      fclose(f);
      return -6;
    }
    uint8_t raw[sizeof(hdr) + 1024];
    memcpy(raw, &hdr, sizeof hdr);
    memcpy(raw + sizeof hdr, body, body_n);
    cubalc_cube_hash(raw, sizeof hdr + body_n, expect_prev);
    n++;
  }
  fclose(f);
  return (int)n;
}

int cubalc_cchn_tip_json(const cubalc_cchn *cc, FILE *out) {
  if (!cc || !out) return -1;
  char hx[CUBALC_HASH_LEN * 2 + 1];
  hex32(cc->tip_hash, hx, sizeof hx);
  fprintf(out,
    "{\"ok\":true,\"store\":\"cubechain\",\"schema\":\"cubalc.cubechain.v1\","
    "\"path\":\"%s\",\"tip_seq\":%u,\"tip_hash\":\"%s\","
    "\"hold_flash\":%u,\"share\":\"%s\",\"matrix_is_key\":true,"
    "\"talk\":\"binary_CBLC\",\"paradigm\":\"Cube-Oriented\","
    "\"not\":\"corporate_blockchain\",\"creed\":\"%s\"}\n",
    cc->path, (unsigned)cc->tip_seq, hx, (unsigned)cc->hold_flash,
    CUBALC_SHARE, CUBALC_CREED);
  return 0;
}

int cubalc_cchn_stat(const char *path, uint32_t *n_blocks, uint32_t *tip_seq) {
  char err[64];
  int n = cubalc_cchn_verify(path, err, sizeof err);
  if (n < 0) return n;
  if (n_blocks) *n_blocks = (uint32_t)n;
  if (tip_seq) {
    cubalc_cchn cc;
    if (cubalc_cchn_open(&cc, NULL) == 0) {
      *tip_seq = cc.tip_seq;
      cubalc_cchn_close(&cc);
    } else *tip_seq = (uint32_t)n;
  }
  return n;
}
