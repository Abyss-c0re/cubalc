/* CubalC below C — hardware-shaped matrix ops.
 * State Matrix packs as u64 lanes; CPU SIMD-friendly; GPU can swallow the same blob.
 * HOLD_FLASH. Binary talk. No fashion formats required at the wire.
 */
#ifndef CUBALC_HW_H
#define CUBALC_HW_H
#include "cubalc.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* popcount / hamming on packed matrix bits — uses compiler HW popcount when available */
int cubalc_hw_matrix_popcount(const cubalc_matrix *m);
int cubalc_hw_matrix_hamming(const cubalc_matrix *a, const cubalc_matrix *b);
/* batch compat for N cubes into NxN upper triangle (CPU; GPU can replace later) */
int cubalc_hw_compat_batch(const cubalc_matrix *mats, int n, float *out_n_by_n);
/* export matrix bank as contiguous u64 words for GPU upload */
int cubalc_hw_matrix_pack_u64(const cubalc_matrix *m, uint64_t *out, int max_words);
#ifdef __cplusplus
}
#endif
#endif
