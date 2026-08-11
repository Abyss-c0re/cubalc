/* CubalC below C — hardware-shaped matrix ops for harmonic sync.
 * State Matrix packs as u64 lanes; CPU multi-thread always;
 * OpenCL GPU when built with USE_OPENCL=1 and platforms present.
 * HOLD_FLASH · Binary talk · No fashion formats on the wire.
 */
#ifndef CUBALC_HW_H
#define CUBALC_HW_H
#include "cubalc.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Backend probe / scale */
void        cubalc_hw_init(int n_workers); /* n<=0 → auto min(nproc,32) */
int         cubalc_hw_workers(void);
int         cubalc_hw_gpu_ok(void);        /* 1 if OpenCL path live */
const char *cubalc_hw_backend(void);       /* "cpu:N" or "cpu:N+gpu" */

/* popcount / hamming on packed matrix bits — HW popcount when available */
int   cubalc_hw_matrix_popcount(const cubalc_matrix *m);
int   cubalc_hw_matrix_hamming(const cubalc_matrix *a, const cubalc_matrix *b);
/* Unity law: 1 − hamming / ATOM_BITS  (raw harmonic measure) */
float cubalc_hw_matrix_unity(const cubalc_matrix *a, const cubalc_matrix *b);

/* batch compat NxN (CPU multi-thread; GPU when large & available) */
int cubalc_hw_compat_batch(const cubalc_matrix *mats, int n, float *out_n_by_n);

/* Mean pairwise unity (i < j). Scalable CPU threads; OpenCL for large n. */
float cubalc_hw_harmony_unity(const cubalc_matrix *mats, int n);

/* Full harmonic solve: mean unity + majority-vote consensus matrix.
 * mats may be non-contiguous (pointer bank). Returns 0 on success. */
int cubalc_hw_harmony_solve(const cubalc_matrix *const *mats, int n,
                            float *out_unity, cubalc_matrix *out_consensus);

/* export matrix as contiguous u64 words for GPU upload */
int cubalc_hw_matrix_pack_u64(const cubalc_matrix *m, uint64_t *out, int max_words);
/* pack bank of n matrices → n words (ATOM_BITS≤64) for GPU harmony */
int cubalc_hw_pack_bank(const cubalc_matrix *mats, int n, uint64_t *out_words);

#ifdef __cplusplus
}
#endif
#endif
