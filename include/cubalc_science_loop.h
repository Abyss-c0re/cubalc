/* CubalC science loop — wires 5D · quantum · relativity · chaos.
 * Free energy cycle: seed → IC → evolve → measure → fold → seed'.
 * Modular coordinator (CLI + plates + viz). No lang monolith growth.
 */
#ifndef CUBALC_SCIENCE_LOOP_H
#define CUBALC_SCIENCE_LOOP_H

#include "cubalc_chaos.h"
#include "cubalc_dim5.h"
#include "cubalc_quantum.h"
#include "cubalc_relativity.h"
#include "cubalc_viz_matrix.h"
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cubalc_science_loop {
  cubalc_event5 event;
  cubalc_qstate quantum;
  cubalc_lorentz boost;
  cubalc_lorenz chaos;
  cubalc_viz_frame viz;
  uint64_t matrix_word;
  uint64_t seed_next;
  double ds2;
  double lyapunov;
  double q_entropy_bits;   /* Shannon of last pre-collapse Born probs */
  double q_participation;  /* 1/Σp² last pre-collapse */
  int loops_done;
  int ok;
} cubalc_science_loop;

/* One full cycle from seed (or 0 → time-based) */
int cubalc_science_loop_run(cubalc_science_loop *S, uint64_t seed, int n_loops,
                            int q_qubits, double v_frac_c, int chaos_steps);

/* Write combined SoT plate */
int cubalc_science_loop_write_json(const cubalc_science_loop *S, FILE *out);
int cubalc_science_loop_write_json_path(const cubalc_science_loop *S,
                                        const char *path);

/* CLI entry: cubalc science …  (exported for main dispatch) */
int cubalc_cmd_science(int argc, char **argv);

#ifdef __cplusplus
}
#endif
#endif /* CUBALC_SCIENCE_LOOP_H */
