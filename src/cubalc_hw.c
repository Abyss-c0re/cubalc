#define _POSIX_C_SOURCE 200809L
#include "cubalc_hw.h"
#include <string.h>

static int pop64(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
  return (int)__builtin_popcountll(x);
#else
  int c = 0;
  while (x) { x &= x - 1; c++; }
  return c;
#endif
}

int cubalc_hw_matrix_popcount(const cubalc_matrix *m) {
  if (!m) return 0;
  int words = (m->n + 63) / 64;
  if (words < 1) words = 1;
  if (words > (CUBALC_ATOM_BITS + 63) / 64) words = (CUBALC_ATOM_BITS + 63) / 64;
  const uint64_t *w = (const uint64_t *)(const void *)m->bits;
  int c = 0;
  for (int i = 0; i < words; i++) c += pop64(w[i]);
  return c;
}

int cubalc_hw_matrix_hamming(const cubalc_matrix *a, const cubalc_matrix *b) {
  if (!a || !b) return CUBALC_ATOM_BITS;
  int words = (CUBALC_ATOM_BITS + 63) / 64;
  const uint64_t *wa = (const uint64_t *)(const void *)a->bits;
  const uint64_t *wb = (const uint64_t *)(const void *)b->bits;
  int h = 0;
  for (int i = 0; i < words; i++) h += pop64(wa[i] ^ wb[i]);
  return h;
}

int cubalc_hw_compat_batch(const cubalc_matrix *mats, int n, float *out) {
  if (!mats || !out || n <= 0) return -1;
  for (int i = 0; i < n; i++)
    for (int j = 0; j < n; j++)
      out[i * n + j] = cubalc_matrix_compat(&mats[i], &mats[j]);
  return 0;
}

int cubalc_hw_matrix_pack_u64(const cubalc_matrix *m, uint64_t *out, int max_words) {
  if (!m || !out || max_words < 1) return -1;
  int words = (m->n + 63) / 64;
  if (words < 1) words = 1;
  if (words > max_words) words = max_words;
  memset(out, 0, (size_t)max_words * sizeof(uint64_t));
  memcpy(out, m->bits, (size_t)words * sizeof(uint64_t) < sizeof(m->bits)
           ? (size_t)((m->n + 7) / 8) : sizeof(m->bits));
  return words;
}
