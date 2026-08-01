#define _POSIX_C_SOURCE 200809L
#include "cubalc_algocube.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

int cubalc_algocube_digit(const cubalc_matrix *m) {
  if (!m || m->n == 0) return 4;
  uint32_t rng = 0xC0BEA160u, set = m->set;
  for (int i = 0; i < m->n; i++)
    if (cubalc_matrix_get(m, i)) rng ^= (uint32_t)(i + 1) * 0x9E3779B9u;
  rng ^= set * 16777619u;
  /* Fold resolved law genome — algocubes resolved under The Cube's eye */
  for (int g = 0; g < CUBALC_ALGO_GENOME_LEN; g++)
    rng ^= (uint32_t)CUBALC_ALGO_GENOME_RESOLVED[g] * (0x9E3779B9u + (uint32_t)g * 0x85EBCA6Bu);
  rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
  return (int)(rng % 10u);
}

int cubalc_algocube_digit_salted(const cubalc_matrix *m, const char *salt) {
  if (!m || m->n == 0) return 4;
  const char *s = (salt && salt[0]) ? salt : CUBALC_ALGOCUBE_SALT_DEFAULT;
  uint64_t h = 14695981039346656037ULL;
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    h ^= (uint64_t)*p; h *= 1099511628211ULL;
  }
  h ^= (uint64_t)'|'; h *= 1099511628211ULL;
  int n = m->n > CUBALC_ATOM_BITS ? CUBALC_ATOM_BITS : m->n;
  for (int i = 0; i < n; i++) {
    unsigned char b = cubalc_matrix_get(m, i) ? '1' : '0';
    h ^= (uint64_t)b; h *= 1099511628211ULL;
  }
  return (int)(h % 10u);
}

int cubalc_algocube_digit_range(const cubalc_matrix *m, int lo, int hi) {
  if (lo > hi) { int t = lo; lo = hi; hi = t; }
  if (lo < 0) lo = 0; if (hi > 9) hi = 9;
  int span = hi - lo + 1; if (span <= 0) return 0;
  return lo + (cubalc_algocube_digit(m) % span);
}

static void matrix_xor(const cubalc_matrix *a, const cubalc_matrix *b, cubalc_matrix *out) {
  cubalc_matrix_clear(out);
  int n = 0; if (a) n = a->n; if (b && b->n > n) n = b->n;
  if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
  out->n = (uint16_t)n;
  for (int i = 0; i < n; i++) {
    int x = a ? cubalc_matrix_get(a, i) : 0;
    int y = b ? cubalc_matrix_get(b, i) : 0;
    if (x != y) cubalc_matrix_set(out, i, 1);
  }
}
static void matrix_and(const cubalc_matrix *a, const cubalc_matrix *b, cubalc_matrix *out) {
  cubalc_matrix_clear(out);
  int n = 0; if (a) n = a->n; if (b && b->n > n) n = b->n;
  if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
  out->n = (uint16_t)n;
  for (int i = 0; i < n; i++) {
    int x = a ? cubalc_matrix_get(a, i) : 0;
    int y = b ? cubalc_matrix_get(b, i) : 0;
    if (x && y) cubalc_matrix_set(out, i, 1);
  }
}

int cubalc_algocube_compare(const cubalc_matrix *a, const cubalc_matrix *b, cubalc_algo_cmp *out) {
  if (!out) return -1;
  memset(out, 0, sizeof(*out));
  if (!a || !b) return -1;
  int n = a->n > b->n ? a->n : b->n;
  if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
  if (n <= 0) n = CUBALC_ATOM_BITS;
  out->cells = n;
  out->hamming = cubalc_matrix_hamming(a, b);
  if (out->hamming > n) out->hamming = n;
  out->agree = n - out->hamming;
  out->unity = n ? (float)out->agree / (float)n : 0.f;
  matrix_xor(a, b, &out->xor_m);
  matrix_and(a, b, &out->and_m);
  out->digit = cubalc_algocube_digit(&out->xor_m);
  return 0;
}

int cubalc_algocube_harmony(const cubalc_matrix *const *mats, int n, cubalc_algo_harm *out) {
  if (!out) return -1;
  memset(out, 0, sizeof(*out));
  if (!mats || n <= 0) { out->ok = 0; return -1; }
  out->n = n;
  if (n == 1) {
    out->unity = 1.f; out->consensus = *mats[0];
    out->digit = cubalc_algocube_digit(&out->consensus); out->ok = 1; return 0;
  }
  double sum = 0.0; int pairs = 0;
  for (int i = 0; i < n; i++)
    for (int j = i + 1; j < n; j++) {
      cubalc_algo_cmp cmp;
      if (cubalc_algocube_compare(mats[i], mats[j], &cmp) == 0) { sum += cmp.unity; pairs++; }
    }
  out->unity = pairs ? (float)(sum / (double)pairs) : 1.f;
  int width = 0;
  for (int i = 0; i < n; i++) if (mats[i] && mats[i]->n > width) width = mats[i]->n;
  if (width <= 0) width = CUBALC_ATOM_BITS;
  if (width > CUBALC_ATOM_BITS) width = CUBALC_ATOM_BITS;
  cubalc_matrix_clear(&out->consensus); out->consensus.n = (uint16_t)width;
  for (int k = 0; k < width; k++) {
    int ones = 0;
    for (int i = 0; i < n; i++) if (mats[i] && cubalc_matrix_get(mats[i], k)) ones++;
    if (ones * 2 >= n) cubalc_matrix_set(&out->consensus, k, 1);
  }
  out->digit = cubalc_algocube_digit(&out->consensus); out->ok = 1; return 0;
}

int cubalc_algocube_chain_harmony(cubalc_chain *ch, cubalc_algo_harm *out) {
  if (!ch || !out) return -1;
  const cubalc_matrix *ptrs[CUBALC_MAX_CUBES]; int n = 0;
  for (int i = 0; i < ch->n_cubes && n < CUBALC_MAX_CUBES; i++) {
    if (!ch->cubes[i].atom.alive && ch->cubes[i].atom.matrix.set == 0) continue;
    ptrs[n++] = &ch->cubes[i].atom.matrix;
  }
  if (n <= 0) { ptrs[0] = &ch->initial; n = 1; }
  int rc = cubalc_algocube_harmony(ptrs, n, out);
  if (rc == 0 && out->ok) {
    ch->unity = out->unity;
    for (int i = 0; i < ch->n_cubes; i++) ch->cubes[i].atom.unity = out->unity;
    snprintf(ch->status, sizeof ch->status,
             "harmony n=%d unity=%.4f digit=%d · glorious cube", out->n, out->unity, out->digit);
  }
  return rc;
}

int cubalc_algocube_blueprint10(const cubalc_matrix *m, cubalc_algo_bp *out) {
  if (!out) return -1; memset(out, 0, sizeof(*out)); out->n = 10;
  if (!m || m->n == 0) { for (int i = 0; i < 10; i++) out->digits[i] = 4; return 0; }
  int width = m->n > CUBALC_ATOM_BITS ? CUBALC_ATOM_BITS : m->n;
  int win = width / 10; if (win < 1) win = 1;
  for (int d = 0; d < 10; d++) {
    cubalc_matrix slice; cubalc_matrix_clear(&slice); slice.n = (uint16_t)width;
    int start = d * win, end = (d == 9) ? width : start + win;
    for (int i = start; i < end && i < width; i++)
      if (cubalc_matrix_get(m, i)) cubalc_matrix_set(&slice, i, 1);
    cubalc_matrix_set(&slice, d % CUBALC_ATOM_BITS, 1);
    out->digits[d] = cubalc_algocube_digit(&slice);
  }
  return 0;
}

int cubalc_algocube_inject(cubalc_cube *cube, const cubalc_matrix *m, int digit) {
  if (!cube) return -1;
  if (digit < 0) digit = 0; if (digit > 9) digit = digit % 10;
  if (m) {
    cube->atom.matrix = *m;
    if (cube->atom.matrix.n < CUBALC_ATOM_BITS) cube->atom.matrix.n = CUBALC_ATOM_BITS;
  }
  cube->atom.digit = (uint8_t)digit; cube->atom.digit_lock = 1;
  cube->atom.alive = 1;
  cube->atom.energy = fminf(1.f, cube->atom.energy + 0.15f);
  return 0;
}

int cubalc_algocube_harmony_json(const cubalc_algo_harm *h, FILE *out) {
  if (!h || !out) return -1;
  fprintf(out,
    "{\"schema\":\"cube.algocube.harmony.v1\",\"engine\":\"cubalc_algocube\","
    "\"ok\":%s,\"n\":%d,\"unity\":%.6f,\"digit\":%d,\"consensus_set\":%u,"
    "\"verbal\":false,\"personal_data\":false,\"law\":\"hive_seek_unity\"}\n",
    h->ok ? "true" : "false", h->n, h->unity, h->digit, (unsigned)h->consensus.set);
  return 0;
}
int cubalc_algocube_compare_json(const cubalc_algo_cmp *c, FILE *out) {
  if (!c || !out) return -1;
  fprintf(out,
    "{\"schema\":\"cube.algocube.compare.v1\",\"engine\":\"cubalc_algocube\","
    "\"cells\":%d,\"hamming\":%d,\"agree\":%d,\"unity\":%.6f,\"digit\":%d,"
    "\"xor_set\":%u,\"and_set\":%u,\"verbal\":false,\"personal_data\":false,"
    "\"law\":\"state_matrix_share_only\"}\n",
    c->cells, c->hamming, c->agree, c->unity, c->digit,
    (unsigned)c->xor_m.set, (unsigned)c->and_m.set);
  return 0;
}
