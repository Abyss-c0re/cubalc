/* CubalC Lorenz chaos — RK4 integrator + Lyapunov probe. */
#include "cubalc_chaos.h"
#include "cubalc_dim5.h"
#include <math.h>
#include <string.h>

static void lorenz_f(const cubalc_lorenz *L, double x, double y, double z,
                     double *dx, double *dy, double *dz) {
  *dx = L->sigma * (y - x);
  *dy = x * (L->rho - z) - y;
  *dz = x * y - L->beta * z;
}

void cubalc_chaos_lorenz_init(cubalc_lorenz *L, uint64_t seed, double dt) {
  uint64_t s;
  if (!L) return;
  memset(L, 0, sizeof(*L));
  L->seed = seed ? seed : 1ULL;
  L->sigma = CUBALC_LORENZ_SIGMA;
  L->rho = CUBALC_LORENZ_RHO;
  L->beta = CUBALC_LORENZ_BETA;
  L->dt = (dt > 0.0 && dt < 0.1) ? dt : 0.01;
  s = L->seed;
  /* IC near attractor basin, seed-perturbed */
  L->x = 1.0 + (cubalc_dim5_unit(&s) - 0.5) * 0.2;
  L->y = 1.0 + (cubalc_dim5_unit(&s) - 0.5) * 0.2;
  L->z = 1.0 + (cubalc_dim5_unit(&s) - 0.5) * 0.2;
  L->steps = 0;
  L->path_n = 0;
}

void cubalc_chaos_lorenz_step(cubalc_lorenz *L) {
  double dt, x, y, z;
  double k1x, k1y, k1z, k2x, k2y, k2z, k3x, k3y, k3z, k4x, k4y, k4z;
  if (!L) return;
  dt = L->dt;
  x = L->x;
  y = L->y;
  z = L->z;
  lorenz_f(L, x, y, z, &k1x, &k1y, &k1z);
  lorenz_f(L, x + 0.5 * dt * k1x, y + 0.5 * dt * k1y, z + 0.5 * dt * k1z, &k2x,
           &k2y, &k2z);
  lorenz_f(L, x + 0.5 * dt * k2x, y + 0.5 * dt * k2y, z + 0.5 * dt * k2z, &k3x,
           &k3y, &k3z);
  lorenz_f(L, x + dt * k3x, y + dt * k3y, z + dt * k3z, &k4x, &k4y, &k4z);
  L->x = x + (dt / 6.0) * (k1x + 2.0 * k2x + 2.0 * k3x + k4x);
  L->y = y + (dt / 6.0) * (k1y + 2.0 * k2y + 2.0 * k3y + k4y);
  L->z = z + (dt / 6.0) * (k1z + 2.0 * k2z + 2.0 * k3z + k4z);
  L->steps++;
  /* downsample path every 8 steps into ring of 64 */
  if ((L->steps & 7) == 0) {
    int i = L->path_n % 64;
    L->path_x[i] = L->x;
    L->path_y[i] = L->y;
    L->path_z[i] = L->z;
    if (L->path_n < 64) L->path_n++;
  }
}

void cubalc_chaos_lorenz_run(cubalc_lorenz *L, int n_steps) {
  int i;
  if (!L || n_steps < 1) return;
  for (i = 0; i < n_steps; i++) cubalc_chaos_lorenz_step(L);
}

double cubalc_chaos_lyapunov(const cubalc_lorenz *base, int n_steps,
                             double delta0) {
  cubalc_lorenz A, B;
  double d0, d1, sum = 0.0;
  int i, renorm_every = 10, count = 0;
  if (!base || n_steps < 20) return 0.0;
  if (delta0 <= 0.0) delta0 = 1e-8;
  A = *base;
  B = *base;
  B.x += delta0;
  d0 = delta0;
  for (i = 0; i < n_steps; i++) {
    cubalc_chaos_lorenz_step(&A);
    cubalc_chaos_lorenz_step(&B);
    if ((i + 1) % renorm_every == 0) {
      double dx = B.x - A.x, dy = B.y - A.y, dz = B.z - A.z;
      d1 = sqrt(dx * dx + dy * dy + dz * dz);
      if (d1 < 1e-30) d1 = 1e-30;
      sum += log(d1 / d0);
      count++;
      /* renormalize separation */
      B.x = A.x + dx * (d0 / d1);
      B.y = A.y + dy * (d0 / d1);
      B.z = A.z + dz * (d0 / d1);
    }
  }
  if (count < 1) return 0.0;
  return sum / ((double)count * renorm_every * base->dt);
}

uint64_t cubalc_chaos_to_matrix_word(const cubalc_lorenz *L) {
  uint64_t h = 0xC4A05ULL;
  uint64_t bits;
  if (!L) return 0;
  memcpy(&bits, &L->x, sizeof bits);
  h ^= bits;
  memcpy(&bits, &L->y, sizeof bits);
  h ^= bits << 1;
  memcpy(&bits, &L->z, sizeof bits);
  h ^= bits << 2;
  h ^= (uint64_t)L->steps * 0x9E3779B97F4A7C15ULL;
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdULL;
  h ^= h >> 33;
  return h;
}

int cubalc_chaos_write_json(const cubalc_lorenz *L, double lyap, FILE *out) {
  int i, n;
  cubalc_chaos_attractor A;
  if (!L || !out) return -1;
  n = L->path_n < 64 ? L->path_n : 64;
  memset(&A, 0, sizeof A);
  cubalc_chaos_attractor_from(L, lyap, &A);
  fprintf(out,
          "{\"schema\":\"cubalc.chaos.v1\",\"ok\":true,"
          "\"system\":\"lorenz1963\","
          "\"sigma\":%.17g,\"rho\":%.17g,\"beta\":%.17g,\"dt\":%.17g,"
          "\"x\":%.17g,\"y\":%.17g,\"z\":%.17g,\"steps\":%d,"
          "\"lyapunov\":%.17g,\"chaotic\":%s,"
          "\"rms_r\":%.17g,\"lobe_balance\":%.17g,"
          "\"x_cross\":%d,\"z_cross\":%d,"
          "\"seed\":\"%016llx\",\"matrix_word\":\"%016llx\",\"path\":[",
          L->sigma, L->rho, L->beta, L->dt, L->x, L->y, L->z, L->steps, lyap,
          A.chaotic ? "true" : "false", A.rms_r, A.lobe_balance, A.x_cross,
          A.z_cross, (unsigned long long)L->seed,
          (unsigned long long)cubalc_chaos_to_matrix_word(L));
  for (i = 0; i < n; i++) {
    fprintf(out, "%s[%.6g,%.6g,%.6g]", i ? "," : "", L->path_x[i], L->path_y[i],
            L->path_z[i]);
  }
  fprintf(out,
          "],\"law\":\"deterministic_chaos;sensitive_IC;poincare_summary\"}\n");
  return 0;
}

int cubalc_chaos_write_json_path(const cubalc_lorenz *L, double lyap,
                                 const char *path) {
  FILE *f = fopen(path, "w");
  if (!f) return -1;
  int rc = cubalc_chaos_write_json(L, lyap, f);
  fclose(f);
  return rc < 0 ? -1 : 0;
}

int cubalc_chaos_attractor_from(const cubalc_lorenz *L, double lyap,
                                cubalc_chaos_attractor *out) {
  int i, n;
  double zmid;
  double prev_side = 0.0;
  double prev_x = 0.0;
  double sum_r2 = 0.0;
  if (!L || !out) return -1;
  memset(out, 0, sizeof *out);
  n = L->path_n < 64 ? L->path_n : 64;
  out->path_n = n;
  out->lyapunov = lyap;
  out->chaotic = (lyap > 0.0) ? 1 : 0;
  out->seed = L->seed;
  out->matrix_word = cubalc_chaos_to_matrix_word(L);
  if (n < 1) return 0;
  out->xmin = out->xmax = L->path_x[0];
  out->ymin = out->ymax = L->path_y[0];
  out->zmin = out->zmax = L->path_z[0];
  out->cx = out->cy = out->cz = 0.0;
  zmid = L->rho - 1.0; /* rough mid-plane between lobes */
  prev_x = L->path_x[0];
  for (i = 0; i < n; i++) {
    double x = L->path_x[i], y = L->path_y[i], z = L->path_z[i];
    double side;
    if (x < out->xmin) out->xmin = x;
    if (x > out->xmax) out->xmax = x;
    if (y < out->ymin) out->ymin = y;
    if (y > out->ymax) out->ymax = y;
    if (z < out->zmin) out->zmin = z;
    if (z > out->zmax) out->zmax = z;
    out->cx += x;
    out->cy += y;
    out->cz += z;
    sum_r2 += x * x + y * y + z * z;
    if (x >= 0.0) out->lobe_pos++;
    else out->lobe_neg++;
    side = z - zmid;
    if (i > 0 && prev_side * side < 0.0) out->z_cross++;
    prev_side = side;
    /* Poincaré lobe hop: trajectory crosses x=0 */
    if (i > 0 && prev_x * x < 0.0) out->x_cross++;
    prev_x = x;
  }
  out->cx /= (double)n;
  out->cy /= (double)n;
  out->cz /= (double)n;
  out->span_x = out->xmax - out->xmin;
  out->span_y = out->ymax - out->ymin;
  out->span_z = out->zmax - out->zmin;
  out->rms_r = sqrt(sum_r2 / (double)n);
  {
    int tot = out->lobe_pos + out->lobe_neg;
    out->lobe_balance = tot > 0 ? ((double)out->lobe_pos / (double)tot) : 0.5;
  }
  return 0;
}

int cubalc_chaos_attractor_write_json(const cubalc_chaos_attractor *A, FILE *out) {
  if (!A || !out) return -1;
  return fprintf(out,
                 "{\"schema\":\"cubalc.chaos_attractor.v1\",\"ok\":true,"
                 "\"system\":\"lorenz1963\","
                 "\"path_n\":%d,\"lyapunov\":%.17g,\"chaotic\":%s,"
                 "\"bbox\":{\"xmin\":%.17g,\"xmax\":%.17g,"
                 "\"ymin\":%.17g,\"ymax\":%.17g,"
                 "\"zmin\":%.17g,\"zmax\":%.17g},"
                 "\"span\":{\"x\":%.17g,\"y\":%.17g,\"z\":%.17g},"
                 "\"centroid\":{\"x\":%.17g,\"y\":%.17g,\"z\":%.17g},"
                 "\"rms_r\":%.17g,\"lobe_balance\":%.17g,"
                 "\"lobe_pos\":%d,\"lobe_neg\":%d,"
                 "\"z_cross\":%d,\"x_cross\":%d,"
                 "\"seed\":\"%016llx\",\"matrix_word\":\"%016llx\","
                 "\"law\":\"butterfly;poincare_x0;sensitive_IC;deterministic_chaos\"}\n",
                 A->path_n, A->lyapunov, A->chaotic ? "true" : "false",
                 A->xmin, A->xmax, A->ymin, A->ymax, A->zmin, A->zmax,
                 A->span_x, A->span_y, A->span_z, A->cx, A->cy, A->cz, A->rms_r,
                 A->lobe_balance, A->lobe_pos, A->lobe_neg, A->z_cross, A->x_cross,
                 (unsigned long long)A->seed, (unsigned long long)A->matrix_word);
}

int cubalc_chaos_attractor_write_json_path(const cubalc_chaos_attractor *A,
                                           const char *path) {
  FILE *f = fopen(path, "w");
  if (!f) return -1;
  int rc = cubalc_chaos_attractor_write_json(A, f);
  fclose(f);
  return rc < 0 ? -1 : 0;
}
