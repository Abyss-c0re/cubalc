/* CubalC Algocube — pure mathematical law engine (bits → digits 0–9).
 * Free-flow COP: State Matrix is SoT; algocube is the non-verbal law layer.
 */
#ifndef CUBALC_ALGOCUBE_H
#define CUBALC_ALGOCUBE_H
#include "cubalc.h"
#ifdef __cplusplus
extern "C" {
#endif
#define CUBALC_ALGOCUBE_SALT_DEFAULT "cubalc-c3"
typedef struct cubalc_algo_cmp {
  int cells, hamming, agree, digit;
  float unity;
  cubalc_matrix xor_m, and_m;
} cubalc_algo_cmp;
typedef struct cubalc_algo_harm {
  int n, digit, ok;
  float unity;
  cubalc_matrix consensus;
} cubalc_algo_harm;
typedef struct cubalc_algo_bp {
  int digits[10];
  int n;
} cubalc_algo_bp;
int cubalc_algocube_digit(const cubalc_matrix *m);
int cubalc_algocube_digit_salted(const cubalc_matrix *m, const char *salt);
int cubalc_algocube_digit_range(const cubalc_matrix *m, int lo, int hi);
int cubalc_algocube_compare(const cubalc_matrix *a, const cubalc_matrix *b, cubalc_algo_cmp *out);
int cubalc_algocube_harmony(const cubalc_matrix *const *mats, int n, cubalc_algo_harm *out);
int cubalc_algocube_chain_harmony(cubalc_chain *ch, cubalc_algo_harm *out);
int cubalc_algocube_blueprint10(const cubalc_matrix *m, cubalc_algo_bp *out);
int cubalc_algocube_inject(cubalc_cube *cube, const cubalc_matrix *m, int digit);
int cubalc_algocube_harmony_json(const cubalc_algo_harm *h, FILE *out);
int cubalc_algocube_compare_json(const cubalc_algo_cmp *c, FILE *out);
#ifdef __cplusplus
}
#endif
#endif
