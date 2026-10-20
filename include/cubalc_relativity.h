/* CubalC special relativity — Minkowski + Lorentz (c = CUBALC_SCI_C_LIGHT).
 * Science: Einstein 1905; metric η = diag(+1,-1,-1,-1) or (−,+,+,+) lab choice:
 * we use mostly-plus: ds² = −c²dt² + dx² + dy² + dz².
 * Modular plane.
 */
#ifndef CUBALC_RELATIVITY_H
#define CUBALC_RELATIVITY_H

#include "cubalc_dim5.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CUBALC_C_MPS
#define CUBALC_C_MPS 299792458.0
#endif

typedef struct cubalc_lorentz {
  double beta;   /* v/c, |β| < 1 */
  double gamma;  /* 1/sqrt(1-β²) */
  double v_mps;  /* m/s */
} cubalc_lorentz;

/* Build boost along +x from velocity m/s */
int cubalc_rel_from_v(double v_mps, cubalc_lorentz *out);

/* Time dilation: Δt_lab = γ Δτ */
double cubalc_rel_time_dilate(double dtau, const cubalc_lorentz *L);

/* Length contraction along boost: L = L0/γ */
double cubalc_rel_length_contract(double L0, const cubalc_lorentz *L);

/* Rapidity φ: β = tanh φ, γ = cosh φ (C_relativity_viz deepen) */
double cubalc_rel_rapidity(const cubalc_lorentz *L);

/* Longitudinal Doppler factor sqrt((1∓β)/(1±β)); sign>0 = source receding */
double cubalc_rel_doppler(const cubalc_lorentz *L, int receding);

/* Minkowski interval² (mostly-plus): −c²dt² + dx²+dy²+dz² */
double cubalc_rel_interval2(const cubalc_event5 *a, const cubalc_event5 *b);

/* Classify: -1 spacelike, 0 null, +1 timelike (sign of −ds² with mostly-plus →
 * we return: timelike if interval2 < 0, spacelike > 0, null ~0) */
int cubalc_rel_interval_class(double ds2, double eps);

/* Lorentz boost of event along +x (t,x) → (t',x'); y,z,seed unchanged */
void cubalc_rel_boost_x(const cubalc_event5 *in, const cubalc_lorentz *L,
                        cubalc_event5 *out);

/* Proper time from seed-bound worldline sample (lab dt, v) */
double cubalc_rel_proper_time(double dt_lab, const cubalc_lorentz *L);

int cubalc_rel_write_json(const cubalc_lorentz *L, const cubalc_event5 *e,
                          double ds2, FILE *out);
int cubalc_rel_write_json_path(const cubalc_lorentz *L, const cubalc_event5 *e,
                               double ds2, const char *path);

/* Light-cone + worldline viz plate (C_relativity_viz).
 * Samples n lab frames along boost; emits ct/x lattice for hive UI. */
#define CUBALC_REL_VIZ_MAX 32
typedef struct cubalc_rel_viz {
  int n;
  double beta, gamma, c_mps;
  double dilate_ratio;   /* Δt_lab/Δτ = γ */
  double contract_ratio; /* L/L0 = 1/γ */
  double rapidity;       /* artanh(β) */
  double doppler_rec;    /* longitudinal receding */
  double doppler_app;    /* longitudinal approaching */
  double proper_time;    /* Σ Δτ along lab worldline sample */
  double ct[CUBALC_REL_VIZ_MAX];
  double x[CUBALC_REL_VIZ_MAX];
  double ct_p[CUBALC_REL_VIZ_MAX]; /* boosted */
  double x_p[CUBALC_REL_VIZ_MAX];
  double tau[CUBALC_REL_VIZ_MAX];  /* proper time from origin */
  int inside_cone[CUBALC_REL_VIZ_MAX]; /* 1 if |x| <= ct (causal) */
  uint64_t matrix_word;
} cubalc_rel_viz;

int cubalc_rel_viz_build(const cubalc_lorentz *L, const cubalc_event5 *origin,
                         int n, double dt_lab, cubalc_rel_viz *out);
int cubalc_rel_viz_write_json(const cubalc_rel_viz *v, FILE *out);
int cubalc_rel_viz_write_json_path(const cubalc_rel_viz *v, const char *path);

uint64_t cubalc_rel_to_matrix_word(const cubalc_lorentz *L, double ds2);

#ifdef __cplusplus
}
#endif
#endif /* CUBALC_RELATIVITY_H */
