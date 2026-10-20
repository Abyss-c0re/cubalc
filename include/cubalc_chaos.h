/* CubalC chaos — Lorenz 1963 attractor (RK4), Lyapunov probe.
 * Science: deterministic chaos, sensitive dependence on ICs.
 * Seed (5th dim) sets initial conditions; loops wire back to matrix.
 * Modular plane.
 */
#ifndef CUBALC_CHAOS_H
#define CUBALC_CHAOS_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Classic Lorenz parameters */
#define CUBALC_LORENZ_SIGMA 10.0
#define CUBALC_LORENZ_RHO   28.0
#define CUBALC_LORENZ_BETA  (8.0 / 3.0)

typedef struct cubalc_lorenz {
  double x, y, z;
  double sigma, rho, beta;
  double dt;
  uint64_t seed;
  int steps;
  /* trajectory ring for viz (downsampled) */
  double path_x[64];
  double path_y[64];
  double path_z[64];
  int path_n;
} cubalc_lorenz;

void cubalc_chaos_lorenz_init(cubalc_lorenz *L, uint64_t seed, double dt);
void cubalc_chaos_lorenz_step(cubalc_lorenz *L);
void cubalc_chaos_lorenz_run(cubalc_lorenz *L, int n_steps);

/* Finite-time Lyapunov estimate: diverge twin trajectory */
double cubalc_chaos_lyapunov(const cubalc_lorenz *base, int n_steps,
                             double delta0);

int cubalc_chaos_write_json(const cubalc_lorenz *L, double lyap, FILE *out);
int cubalc_chaos_write_json_path(const cubalc_lorenz *L, double lyap,
                                 const char *path);

/* Attractor plate (D_chaos_attractor): bbox, lobe counts, z-crossings. */
typedef struct cubalc_chaos_attractor {
  int path_n;
  double xmin, xmax, ymin, ymax, zmin, zmax;
  double span_x, span_y, span_z; /* bbox edge lengths */
  double cx, cy, cz;     /* path centroid */
  double rms_r;          /* sqrt(mean(x²+y²+z²)) */
  double lobe_balance;   /* lobe_pos / (pos+neg) in [0,1] */
  int lobe_pos, lobe_neg; /* samples with x>0 / x<0 */
  int z_cross;            /* sign flips of (z - rho+1 approx mid) */
  int x_cross;            /* Poincaré: x=0 plane crossings (lobe hops) */
  int chaotic;            /* 1 if lyapunov > 0 (sensitive IC) */
  double lyapunov;
  uint64_t seed;
  uint64_t matrix_word;
} cubalc_chaos_attractor;

int cubalc_chaos_attractor_from(const cubalc_lorenz *L, double lyap,
                                cubalc_chaos_attractor *out);
int cubalc_chaos_attractor_write_json(const cubalc_chaos_attractor *A, FILE *out);
int cubalc_chaos_attractor_write_json_path(const cubalc_chaos_attractor *A,
                                           const char *path);

/* Fold Lorenz state → matrix word */
uint64_t cubalc_chaos_to_matrix_word(const cubalc_lorenz *L);

#ifdef __cplusplus
}
#endif
#endif /* CUBALC_CHAOS_H */
