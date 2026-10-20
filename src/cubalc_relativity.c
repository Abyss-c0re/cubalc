/* CubalC special relativity — Minkowski + Lorentz boosts. */
#include "cubalc_relativity.h"
#include <math.h>
#include <string.h>

int cubalc_rel_from_v(double v_mps, cubalc_lorentz *out) {
  double beta, g2;
  if (!out) return -1;
  if (fabs(v_mps) >= CUBALC_C_MPS) return -1; /* physical |v| < c */
  beta = v_mps / CUBALC_C_MPS;
  g2 = 1.0 - beta * beta;
  if (g2 <= 0.0) return -1;
  out->v_mps = v_mps;
  out->beta = beta;
  out->gamma = 1.0 / sqrt(g2);
  return 0;
}

double cubalc_rel_time_dilate(double dtau, const cubalc_lorentz *L) {
  if (!L) return dtau;
  return L->gamma * dtau;
}

double cubalc_rel_length_contract(double L0, const cubalc_lorentz *L) {
  if (!L || L->gamma <= 0.0) return L0;
  return L0 / L->gamma;
}

double cubalc_rel_rapidity(const cubalc_lorentz *L) {
  if (!L) return 0.0;
  if (fabs(L->beta) >= 1.0) return 0.0;
  return 0.5 * log((1.0 + L->beta) / (1.0 - L->beta)); /* artanh β */
}

double cubalc_rel_doppler(const cubalc_lorentz *L, int receding) {
  double b;
  if (!L) return 1.0;
  b = L->beta;
  if (fabs(b) >= 1.0) return 0.0;
  if (receding)
    return sqrt((1.0 - b) / (1.0 + b));
  return sqrt((1.0 + b) / (1.0 - b));
}

double cubalc_rel_interval2(const cubalc_event5 *a, const cubalc_event5 *b) {
  double dt, dx, dy, dz;
  if (!a || !b) return 0.0;
  dt = b->t - a->t;
  dx = b->x - a->x;
  dy = b->y - a->y;
  dz = b->z - a->z;
  /* mostly-plus: ds² = −c²dt² + dx² + dy² + dz² */
  return -(CUBALC_C_MPS * CUBALC_C_MPS) * dt * dt + dx * dx + dy * dy + dz * dz;
}

int cubalc_rel_interval_class(double ds2, double eps) {
  if (eps <= 0.0) eps = 1e-6;
  if (ds2 < -eps) return 1;  /* timelike */
  if (ds2 > eps) return -1;  /* spacelike */
  return 0;                  /* null */
}

void cubalc_rel_boost_x(const cubalc_event5 *in, const cubalc_lorentz *L,
                        cubalc_event5 *out) {
  double g, b, ct, x;
  if (!in || !L || !out) return;
  g = L->gamma;
  b = L->beta;
  ct = CUBALC_C_MPS * in->t;
  x = in->x;
  out->t = (g * (ct - b * x)) / CUBALC_C_MPS;
  out->x = g * (x - b * ct);
  out->y = in->y;
  out->z = in->z;
  out->seed = in->seed;
}

double cubalc_rel_proper_time(double dt_lab, const cubalc_lorentz *L) {
  if (!L || L->gamma <= 0.0) return dt_lab;
  return dt_lab / L->gamma; /* Δτ = Δt/γ */
}

uint64_t cubalc_rel_to_matrix_word(const cubalc_lorentz *L, double ds2) {
  uint64_t h = 0x2E1A71ULL;
  uint64_t bits;
  if (L) {
    memcpy(&bits, &L->gamma, sizeof bits);
    h ^= bits;
    memcpy(&bits, &L->beta, sizeof bits);
    h ^= bits << 1;
  }
  memcpy(&bits, &ds2, sizeof bits);
  h ^= bits + 0x9E3779B97F4A7C15ULL;
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdULL;
  h ^= h >> 33;
  return h;
}

int cubalc_rel_write_json(const cubalc_lorentz *L, const cubalc_event5 *e,
                          double ds2, FILE *out) {
  int cls;
  double rap, drec, dapp, tau, g, inv_g;
  if (!out) return -1;
  cls = cubalc_rel_interval_class(ds2, 1e-3);
  g = L ? L->gamma : 1.0;
  inv_g = (g > 0.0) ? (1.0 / g) : 1.0;
  rap = cubalc_rel_rapidity(L);
  drec = cubalc_rel_doppler(L, 1);
  dapp = cubalc_rel_doppler(L, 0);
  /* proper time of event lab-t from t=0 on this boost: τ = t/γ */
  tau = cubalc_rel_proper_time(e ? e->t : 0.0, L);
  return fprintf(out,
                 "{\"schema\":\"cubalc.relativity.v1\",\"ok\":true,"
                 "\"c_mps\":%.17g,\"beta\":%.17g,\"gamma\":%.17g,"
                 "\"v_mps\":%.17g,\"ds2\":%.17g,"
                 "\"interval\":\"%s\","
                 "\"rapidity\":%.17g,\"doppler_rec\":%.17g,\"doppler_app\":%.17g,"
                 "\"dilate_ratio\":%.17g,\"contract_ratio\":%.17g,"
                 "\"proper_time\":%.17g,"
                 "\"event\":{\"x\":%.17g,\"y\":%.17g,\"z\":%.17g,\"t\":%.17g,"
                 "\"seed\":\"%016llx\"},"
                 "\"matrix_word\":\"%016llx\","
                 "\"law\":\"lorentz_minkowski;rapidity_doppler;einstein_1905\"}\n",
                 CUBALC_C_MPS, L ? L->beta : 0.0, g,
                 L ? L->v_mps : 0.0, ds2,
                 cls > 0 ? "timelike" : (cls < 0 ? "spacelike" : "null"),
                 rap, drec, dapp, g, inv_g, tau,
                 e ? e->x : 0.0, e ? e->y : 0.0, e ? e->z : 0.0,
                 e ? e->t : 0.0, e ? (unsigned long long)e->seed : 0ULL,
                 (unsigned long long)cubalc_rel_to_matrix_word(L, ds2));
}

int cubalc_rel_write_json_path(const cubalc_lorentz *L, const cubalc_event5 *e,
                               double ds2, const char *path) {
  FILE *f = fopen(path, "w");
  if (!f) return -1;
  int rc = cubalc_rel_write_json(L, e, ds2, f);
  fclose(f);
  return rc < 0 ? -1 : 0;
}

int cubalc_rel_viz_build(const cubalc_lorentz *L, const cubalc_event5 *origin,
                         int n, double dt_lab, cubalc_rel_viz *out) {
  int i;
  cubalc_event5 o, e, ep;
  double ds2 = 0.0;
  if (!L || !out) return -1;
  if (n < 2) n = 2;
  if (n > CUBALC_REL_VIZ_MAX) n = CUBALC_REL_VIZ_MAX;
  if (dt_lab <= 0.0) dt_lab = 0.1;
  memset(out, 0, sizeof *out);
  out->n = n;
  out->beta = L->beta;
  out->gamma = L->gamma;
  out->c_mps = CUBALC_C_MPS;
  out->dilate_ratio = L->gamma;
  out->contract_ratio = (L->gamma > 0.0) ? (1.0 / L->gamma) : 1.0;
  out->rapidity = cubalc_rel_rapidity(L);
  out->doppler_rec = cubalc_rel_doppler(L, 1);
  out->doppler_app = cubalc_rel_doppler(L, 0);
  out->proper_time = 0.0;
  if (origin)
    o = *origin;
  else {
    memset(&o, 0, sizeof o);
    o.seed = 1;
  }
  for (i = 0; i < n; i++) {
    double ct, ax;
    e = o;
    e.t = o.t + dt_lab * (double)i;
    /* moving twin worldline: x = v t from origin */
    if (i > 0)
      e.x = o.x + L->v_mps * (e.t - o.t);
    else
      e.x = o.x;
    cubalc_rel_boost_x(&e, L, &ep);
    out->ct[i] = CUBALC_C_MPS * e.t;
    out->x[i] = e.x;
    out->ct_p[i] = CUBALC_C_MPS * ep.t;
    out->x_p[i] = ep.x;
    /* proper time from origin along this inertial worldline: τ = t/γ */
    out->tau[i] = cubalc_rel_proper_time(e.t - o.t, L);
    if (i == n - 1) {
      out->proper_time = out->tau[i];
      ds2 = cubalc_rel_interval2(&o, &e);
    }
    /* causal: |x - x0| <= c|t - t0|  (on or inside future/past light cone) */
    ct = fabs(out->ct[i] - CUBALC_C_MPS * o.t);
    ax = fabs(out->x[i] - o.x);
    out->inside_cone[i] = (ax <= ct + 1e-9 * (ct + 1.0)) ? 1 : 0;
  }
  out->matrix_word = cubalc_rel_to_matrix_word(L, ds2);
  return 0;
}

int cubalc_rel_viz_write_json(const cubalc_rel_viz *v, FILE *out) {
  int i, cone_ok = 1;
  if (!v || !out) return -1;
  for (i = 0; i < v->n; i++)
    if (!v->inside_cone[i]) cone_ok = 0;
  fprintf(out,
          "{\"schema\":\"cubalc.relativity_viz.v1\",\"ok\":true,"
          "\"n\":%d,\"beta\":%.17g,\"gamma\":%.17g,\"c_mps\":%.17g,"
          "\"dilate_ratio\":%.17g,\"contract_ratio\":%.17g,"
          "\"rapidity\":%.17g,\"doppler_rec\":%.17g,\"doppler_app\":%.17g,"
          "\"proper_time\":%.17g,\"worldline_causal\":%s,"
          "\"matrix_word\":\"%016llx\","
          "\"law\":\"light_cone;lorentz_worldline;rapidity_doppler;einstein_1905\","
          "\"lab\":[",
          v->n, v->beta, v->gamma, v->c_mps, v->dilate_ratio, v->contract_ratio,
          v->rapidity, v->doppler_rec, v->doppler_app, v->proper_time,
          cone_ok ? "true" : "false",
          (unsigned long long)v->matrix_word);
  for (i = 0; i < v->n; i++) {
    if (i) fputc(',', out);
    fprintf(out,
            "{\"i\":%d,\"ct\":%.17g,\"x\":%.17g,\"tau\":%.17g,"
            "\"inside_cone\":%d}",
            i, v->ct[i], v->x[i], v->tau[i], v->inside_cone[i]);
  }
  fputs("],\"boosted\":[", out);
  for (i = 0; i < v->n; i++) {
    if (i) fputc(',', out);
    fprintf(out, "{\"i\":%d,\"ct\":%.17g,\"x\":%.17g}", i, v->ct_p[i], v->x_p[i]);
  }
  fputs("],\"null_cone\":{", out);
  if (v->n > 0) {
    double ct = v->ct[v->n - 1];
    fprintf(out, "\"ct\":%.17g,\"x_plus\":%.17g,\"x_minus\":%.17g", ct, ct, -ct);
  } else {
    fputs("\"ct\":0,\"x_plus\":0,\"x_minus\":0", out);
  }
  fputs("}}\n", out);
  return 0;
}

int cubalc_rel_viz_write_json_path(const cubalc_rel_viz *v, const char *path) {
  FILE *f = fopen(path, "w");
  if (!f) return -1;
  int rc = cubalc_rel_viz_write_json(v, f);
  fclose(f);
  return rc < 0 ? -1 : 0;
}
