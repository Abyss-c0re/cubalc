/* CubalC self-evolve — pure C · braincube solves · algocube optimizes.
 * No Python. Language improvements emitted as .cubalc + genome binary.
 */
#ifndef CUBALC_EVOLVE_H
#define CUBALC_EVOLVE_H

#include "cubalc.h"
#include "cubalc_algocube.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CUBALC_EVOLVE_N        8
#define CUBALC_EVOLVE_GENOME   32
#define CUBALC_EVOLVE_CELLS    (CUBALC_EVOLVE_N * CUBALC_EVOLVE_N * CUBALC_EVOLVE_N)
/* Cube cadence: deep algocube solve/optimize every 6.6 minutes */
#define CUBALC_ALGO_DEEP_SEC   396.0   /* 6.6 * 60 */
#define CUBALC_ALGO_DEEP_TRIALS 320

typedef struct cubalc_evolve_mind {
  int    n;
  uint8_t cells[CUBALC_EVOLVE_CELLS];
  uint8_t genome[CUBALC_EVOLVE_GENOME];   /* algocube blueprint digits 0–9 */
  uint8_t best_genome[CUBALC_EVOLVE_GENOME];
  int    gen;
  uint32_t seed;
  double energy;
  int    races, wins, losses, streak;
  int    last_path, last_i, last_o, last_plug, last_winner;
  int    face_mode, path_variants, natural_ticks;
  float  difficulty;
  float  best_fitness;   /* algocube fitness 0..1 */
  float  last_fitness;
  int    algo_solves;    /* algocube problems solved this gen */
  int    algo_fails;
  char   status[96];
} cubalc_evolve_mind;

typedef struct cubalc_evolve_result {
  int ok;
  int gen, races, wins, losses;
  float difficulty, fitness, unity;
  double energy;
  int winner, path, digit;
  int algo_solves, algo_fails;
  char status[96];
  char program[256];
} cubalc_evolve_result;

/* Load/save mind under CUBALC_STATE/evolve/ (or given root). */
int cubalc_evolve_load(cubalc_evolve_mind *m, const char *state_dir);
int cubalc_evolve_save(const cubalc_evolve_mind *m, const char *state_dir);

/* One cycle: braincube race + algocube solve/optimize + emit .cubalc + run. */
int cubalc_evolve_cycle(cubalc_evolve_mind *m, const char *state_dir,
                        const char *prog_dir, cubalc_evolve_result *out);

/* Deep search: multi-strategy algocube solve/optimize (call every 6.6 min). */
int cubalc_algocube_deep_optimize(cubalc_evolve_mind *m, const char *state_dir,
                                  int trials, double *fit_before, double *fit_after);

/* Constant loop. hz<=0 → default 5. max_cycles==0 → forever.
 * Always runs a deep algocube pass every CUBALC_ALGO_DEEP_SEC (6.6 min). */
int cubalc_evolve_loop(double hz, int max_cycles,
                       const char *state_dir, const char *prog_dir);

/* CLI helpers used by cubalc_main */
int cubalc_cmd_evolve(int argc, char **argv);

#ifdef __cplusplus
}
#endif
#endif
