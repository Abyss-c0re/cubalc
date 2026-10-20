/* CubalC quantum gate-model simulator — classical statevector.
 * Usable offline; export OpenQASM 2.0 for real QPU backends.
 * Science: unitary gates, Born measurement, no hidden variables fudge.
 * Modular plane — keep out of lang_ops monoliths.
 */
#ifndef CUBALC_QUANTUM_H
#define CUBALC_QUANTUM_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CUBALC_Q_MAX_QUBITS 8 /* 2^8 = 256 amplitudes — lean */

typedef struct cubalc_cplx {
  double re, im;
} cubalc_cplx;

typedef struct cubalc_qstate {
  int n_qubits;
  int n_amps; /* 2^n */
  cubalc_cplx amp[1u << CUBALC_Q_MAX_QUBITS];
  uint64_t seed; /* dimension seed binds measurement RNG */
  int last_meas; /* last measured bitstring */
  int shots_done;
} cubalc_qstate;

void cubalc_q_init(cubalc_qstate *qs, int n_qubits, uint64_t seed);
void cubalc_q_reset_zero(cubalc_qstate *qs);

/* Single-qubit gates on target qubit (0 = LSB) */
int cubalc_q_h(cubalc_qstate *qs, int target);
int cubalc_q_x(cubalc_qstate *qs, int target);
int cubalc_q_y(cubalc_qstate *qs, int target);
int cubalc_q_z(cubalc_qstate *qs, int target);
int cubalc_q_s(cubalc_qstate *qs, int target);
int cubalc_q_sdg(cubalc_qstate *qs, int target);
int cubalc_q_t(cubalc_qstate *qs, int target);
int cubalc_q_tdg(cubalc_qstate *qs, int target);
int cubalc_q_rx(cubalc_qstate *qs, int target, double theta);
int cubalc_q_ry(cubalc_qstate *qs, int target, double theta);
int cubalc_q_rz(cubalc_qstate *qs, int target, double theta);

/* Two-qubit */
int cubalc_q_cnot(cubalc_qstate *qs, int control, int target);
int cubalc_q_cz(cubalc_qstate *qs, int control, int target);
int cubalc_q_swap(cubalc_qstate *qs, int a, int b);

/* Apply gates_csv to statevector (same dialect as cubalc_q_write_qasm).
 * barrier is a no-op. Returns 0 ok, -1 on parse/range error. */
int cubalc_q_apply_gates_csv(cubalc_qstate *qs, const char *gates_csv);

/* Born-rule projective measure all qubits → classical bitstring */
int cubalc_q_measure_all(cubalc_qstate *qs);

/* Probability of basis state k */
double cubalc_q_prob(const cubalc_qstate *qs, int k);

/* Norm check (should be ~1) */
double cubalc_q_norm2(const cubalc_qstate *qs);

/* Export OpenQASM 2.0 for external QPU
 * (h/x/y/z/s/t/sdg/tdg/cx/cz/swap/barrier + rz/rx/ry θ). */
int cubalc_q_write_qasm(int n_qubits, const char *gates_csv, FILE *out);

/* JSON plate */
int cubalc_q_write_json(const cubalc_qstate *qs, FILE *out);
int cubalc_q_write_json_path(const cubalc_qstate *qs, const char *path);

/* Fold amplitudes → matrix word (phase/magnitude hash) */
uint64_t cubalc_q_to_matrix_word(const cubalc_qstate *qs);

#ifdef __cplusplus
}
#endif
#endif /* CUBALC_QUANTUM_H */
