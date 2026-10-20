/* CubalC quantum gate-model statevector — classical sim + QASM export. */
#include "cubalc_quantum.h"
#include "cubalc_dim5.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static cubalc_cplx c_add(cubalc_cplx a, cubalc_cplx b) {
  cubalc_cplx r = {a.re + b.re, a.im + b.im};
  return r;
}
static cubalc_cplx c_mul(cubalc_cplx a, cubalc_cplx b) {
  cubalc_cplx r = {a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re};
  return r;
}
void cubalc_q_init(cubalc_qstate *qs, int n_qubits, uint64_t seed) {
  if (!qs) return;
  if (n_qubits < 1) n_qubits = 1;
  if (n_qubits > CUBALC_Q_MAX_QUBITS) n_qubits = CUBALC_Q_MAX_QUBITS;
  memset(qs, 0, sizeof(*qs));
  qs->n_qubits = n_qubits;
  qs->n_amps = 1 << n_qubits;
  qs->seed = seed ? seed : 1ULL;
  cubalc_q_reset_zero(qs);
}

void cubalc_q_reset_zero(cubalc_qstate *qs) {
  int i;
  if (!qs) return;
  for (i = 0; i < qs->n_amps; i++) {
    qs->amp[i].re = 0.0;
    qs->amp[i].im = 0.0;
  }
  qs->amp[0].re = 1.0; /* |0…0⟩ */
  qs->last_meas = 0;
}

static int apply_1q(cubalc_qstate *qs, int target,
                    cubalc_cplx u00, cubalc_cplx u01,
                    cubalc_cplx u10, cubalc_cplx u11) {
  int mask, i;
  if (!qs || target < 0 || target >= qs->n_qubits) return -1;
  mask = 1 << target;
  for (i = 0; i < qs->n_amps; i++) {
    if (i & mask) continue; /* pair once via i and i|mask */
    {
      int j = i | mask;
      cubalc_cplx a0 = qs->amp[i];
      cubalc_cplx a1 = qs->amp[j];
      qs->amp[i] = c_add(c_mul(u00, a0), c_mul(u01, a1));
      qs->amp[j] = c_add(c_mul(u10, a0), c_mul(u11, a1));
    }
  }
  return 0;
}

int cubalc_q_h(cubalc_qstate *qs, int target) {
  const double s = 1.0 / sqrt(2.0);
  cubalc_cplx u00 = {s, 0}, u01 = {s, 0}, u10 = {s, 0}, u11 = {-s, 0};
  return apply_1q(qs, target, u00, u01, u10, u11);
}

int cubalc_q_x(cubalc_qstate *qs, int target) {
  cubalc_cplx z = {0, 0}, o = {1, 0};
  return apply_1q(qs, target, z, o, o, z);
}

int cubalc_q_y(cubalc_qstate *qs, int target) {
  /* Y = [[0,-i],[i,0]] */
  cubalc_cplx z = {0, 0}, mi = {0, -1}, pi = {0, 1};
  return apply_1q(qs, target, z, mi, pi, z);
}

int cubalc_q_z(cubalc_qstate *qs, int target) {
  cubalc_cplx o = {1, 0}, z = {0, 0}, m = {-1, 0};
  return apply_1q(qs, target, o, z, z, m);
}

int cubalc_q_s(cubalc_qstate *qs, int target) {
  cubalc_cplx o = {1, 0}, z = {0, 0}, i = {0, 1};
  return apply_1q(qs, target, o, z, z, i);
}

int cubalc_q_sdg(cubalc_qstate *qs, int target) {
  cubalc_cplx o = {1, 0}, z = {0, 0}, mi = {0, -1};
  return apply_1q(qs, target, o, z, z, mi);
}

int cubalc_q_t(cubalc_qstate *qs, int target) {
  cubalc_cplx o = {1, 0}, z = {0, 0};
  cubalc_cplx tt = {cos(M_PI / 4.0), sin(M_PI / 4.0)};
  return apply_1q(qs, target, o, z, z, tt);
}

int cubalc_q_tdg(cubalc_qstate *qs, int target) {
  cubalc_cplx o = {1, 0}, z = {0, 0};
  cubalc_cplx td = {cos(-M_PI / 4.0), sin(-M_PI / 4.0)};
  return apply_1q(qs, target, o, z, z, td);
}

int cubalc_q_rx(cubalc_qstate *qs, int target, double theta) {
  double c = cos(theta / 2.0), s = sin(theta / 2.0);
  cubalc_cplx u00 = {c, 0}, u01 = {0, -s}, u10 = {0, -s}, u11 = {c, 0};
  return apply_1q(qs, target, u00, u01, u10, u11);
}

int cubalc_q_ry(cubalc_qstate *qs, int target, double theta) {
  double c = cos(theta / 2.0), s = sin(theta / 2.0);
  cubalc_cplx u00 = {c, 0}, u01 = {-s, 0}, u10 = {s, 0}, u11 = {c, 0};
  return apply_1q(qs, target, u00, u01, u10, u11);
}

int cubalc_q_rz(cubalc_qstate *qs, int target, double theta) {
  cubalc_cplx u00 = {cos(-theta / 2.0), sin(-theta / 2.0)};
  cubalc_cplx u11 = {cos(theta / 2.0), sin(theta / 2.0)};
  cubalc_cplx z = {0, 0};
  return apply_1q(qs, target, u00, z, z, u11);
}

int cubalc_q_cnot(cubalc_qstate *qs, int control, int target) {
  int cmask, tmask, i;
  if (!qs || control < 0 || target < 0 || control >= qs->n_qubits ||
      target >= qs->n_qubits || control == target)
    return -1;
  cmask = 1 << control;
  tmask = 1 << target;
  for (i = 0; i < qs->n_amps; i++) {
    if ((i & cmask) && !(i & tmask)) {
      int j = i | tmask;
      cubalc_cplx tmp = qs->amp[i];
      qs->amp[i] = qs->amp[j];
      qs->amp[j] = tmp;
    }
  }
  return 0;
}

int cubalc_q_cz(cubalc_qstate *qs, int control, int target) {
  int cmask, tmask, i;
  if (!qs || control < 0 || target < 0 || control >= qs->n_qubits ||
      target >= qs->n_qubits || control == target)
    return -1;
  cmask = 1 << control;
  tmask = 1 << target;
  for (i = 0; i < qs->n_amps; i++) {
    if ((i & cmask) && (i & tmask)) {
      qs->amp[i].re = -qs->amp[i].re;
      qs->amp[i].im = -qs->amp[i].im;
    }
  }
  return 0;
}

int cubalc_q_swap(cubalc_qstate *qs, int a, int b) {
  int am, bm, i;
  if (!qs || a < 0 || b < 0 || a >= qs->n_qubits || b >= qs->n_qubits || a == b)
    return -1;
  am = 1 << a;
  bm = 1 << b;
  for (i = 0; i < qs->n_amps; i++) {
    int has_a = (i & am) != 0;
    int has_b = (i & bm) != 0;
    if (has_a == has_b) continue; /* 00 and 11 fixed; swap 01 <-> 10 once */
    if (has_a && !has_b) {
      int j = (i & ~am) | bm;
      if (i < j) {
        cubalc_cplx tmp = qs->amp[i];
        qs->amp[i] = qs->amp[j];
        qs->amp[j] = tmp;
      }
    }
  }
  return 0;
}

double cubalc_q_prob(const cubalc_qstate *qs, int k) {
  if (!qs || k < 0 || k >= qs->n_amps) return 0.0;
  return qs->amp[k].re * qs->amp[k].re + qs->amp[k].im * qs->amp[k].im;
}

double cubalc_q_norm2(const cubalc_qstate *qs) {
  double n = 0.0;
  int i;
  if (!qs) return 0.0;
  for (i = 0; i < qs->n_amps; i++) n += cubalc_q_prob(qs, i);
  return n;
}

int cubalc_q_measure_all(cubalc_qstate *qs) {
  double r, acc = 0.0;
  int k, chosen = 0;
  uint64_t st;
  if (!qs) return -1;
  st = qs->seed ^ (uint64_t)(qs->shots_done + 1) * 0x9E3779B97F4A7C15ULL;
  r = cubalc_dim5_unit(&st);
  for (k = 0; k < qs->n_amps; k++) {
    acc += cubalc_q_prob(qs, k);
    if (r <= acc) {
      chosen = k;
      break;
    }
    chosen = k;
  }
  /* collapse */
  for (k = 0; k < qs->n_amps; k++) {
    qs->amp[k].re = 0.0;
    qs->amp[k].im = 0.0;
  }
  qs->amp[chosen].re = 1.0;
  qs->last_meas = chosen;
  qs->shots_done++;
  qs->seed = cubalc_dim5_splitmix64(&st);
  return chosen;
}


int cubalc_q_apply_gates_csv(cubalc_qstate *qs, const char *gates_csv) {
  const char *p;
  if (!qs) return -1;
  if (!gates_csv || !gates_csv[0]) return 0;
  p = gates_csv;
  while (*p) {
    char g[16];
    int a = 0, b = 0, n = 0, rc = 0;
    double th = 0.0;
    if (sscanf(p, "%15[^,:]:%d:%d%n", g, &a, &b, &n) >= 3 && n > 0 &&
        (strcmp(g, "cnot") == 0 || strcmp(g, "cx") == 0 ||
         strcmp(g, "cz") == 0 || strcmp(g, "swap") == 0)) {
      if (strcmp(g, "cz") == 0) rc = cubalc_q_cz(qs, a, b);
      else if (strcmp(g, "swap") == 0) rc = cubalc_q_swap(qs, a, b);
      else rc = cubalc_q_cnot(qs, a, b);
      if (rc != 0) return -1;
      p += n;
    } else if (sscanf(p, "%15[^,:]:%d:%lf%n", g, &a, &th, &n) >= 3 && n > 0 &&
               (strcmp(g, "rz") == 0 || strcmp(g, "rx") == 0 ||
                strcmp(g, "ry") == 0)) {
      if (strcmp(g, "rz") == 0) rc = cubalc_q_rz(qs, a, th);
      else if (strcmp(g, "ry") == 0) rc = cubalc_q_ry(qs, a, th);
      else rc = cubalc_q_rx(qs, a, th);
      if (rc != 0) return -1;
      p += n;
    } else if (sscanf(p, "%15[^,:]:%d%n", g, &a, &n) >= 2) {
      if (strcmp(g, "h") == 0) rc = cubalc_q_h(qs, a);
      else if (strcmp(g, "x") == 0) rc = cubalc_q_x(qs, a);
      else if (strcmp(g, "y") == 0) rc = cubalc_q_y(qs, a);
      else if (strcmp(g, "z") == 0) rc = cubalc_q_z(qs, a);
      else if (strcmp(g, "s") == 0) rc = cubalc_q_s(qs, a);
      else if (strcmp(g, "t") == 0) rc = cubalc_q_t(qs, a);
      else if (strcmp(g, "sdg") == 0) rc = cubalc_q_sdg(qs, a);
      else if (strcmp(g, "tdg") == 0) rc = cubalc_q_tdg(qs, a);
      else if (strcmp(g, "barrier") == 0) rc = 0; /* no-op on statevector */
      else return -1;
      if (rc != 0) return -1;
      p += n;
    } else if (sscanf(p, "%15[^,:]%n", g, &n) >= 1 && strcmp(g, "barrier") == 0) {
      p += n;
    } else {
      return -1;
    }
    if (*p == ',') p++;
  }
  return 0;
}

int cubalc_q_write_qasm(int n_qubits, const char *gates_csv, FILE *out) {
  if (!out) return -1;
  if (n_qubits < 1) n_qubits = 1;
  if (n_qubits > CUBALC_Q_MAX_QUBITS) n_qubits = CUBALC_Q_MAX_QUBITS;
  fprintf(out, "OPENQASM 2.0;\ninclude \"qelib1.inc\";\n");
  fprintf(out, "qreg q[%d];\ncreg c[%d];\n", n_qubits, n_qubits);
  if (gates_csv && gates_csv[0]) {
    /* gates_csv: "h:0,cnot:0:1,cz:0:1,swap:0:1,ry:0:0.5,rz:0:1.57,rx:1:0.5,barrier:0,y:1" */
    const char *p = gates_csv;
    while (*p) {
      char g[16];
      int a = 0, b = 0, n = 0;
      double th = 0.0;
      /* two-qubit int form first: cnot/cx/cz/swap */
      if (sscanf(p, "%15[^,:]:%d:%d%n", g, &a, &b, &n) >= 3 && n > 0 &&
          (strcmp(g, "cnot") == 0 || strcmp(g, "cx") == 0 ||
           strcmp(g, "cz") == 0 || strcmp(g, "swap") == 0)) {
        if (strcmp(g, "cz") == 0)
          fprintf(out, "cz q[%d],q[%d];\n", a, b);
        else if (strcmp(g, "swap") == 0)
          fprintf(out, "swap q[%d],q[%d];\n", a, b);
        else
          fprintf(out, "cx q[%d],q[%d];\n", a, b);
        p += n;
      } else if (sscanf(p, "%15[^,:]:%d:%lf%n", g, &a, &th, &n) >= 3 && n > 0 &&
                 (strcmp(g, "rz") == 0 || strcmp(g, "rx") == 0 ||
                  strcmp(g, "ry") == 0)) {
        /* rotation with theta (radians) — OpenQASM 2.0 */
        if (strcmp(g, "rz") == 0)
          fprintf(out, "rz(%.17g) q[%d];\n", th, a);
        else if (strcmp(g, "ry") == 0)
          fprintf(out, "ry(%.17g) q[%d];\n", th, a);
        else
          fprintf(out, "rx(%.17g) q[%d];\n", th, a);
        p += n;
      } else if (sscanf(p, "%15[^,:]:%d%n", g, &a, &n) >= 2) {
        if (strcmp(g, "h") == 0) fprintf(out, "h q[%d];\n", a);
        else if (strcmp(g, "x") == 0) fprintf(out, "x q[%d];\n", a);
        else if (strcmp(g, "y") == 0) fprintf(out, "y q[%d];\n", a);
        else if (strcmp(g, "z") == 0) fprintf(out, "z q[%d];\n", a);
        else if (strcmp(g, "s") == 0) fprintf(out, "s q[%d];\n", a);
        else if (strcmp(g, "t") == 0) fprintf(out, "t q[%d];\n", a);
        else if (strcmp(g, "sdg") == 0) fprintf(out, "sdg q[%d];\n", a);
        else if (strcmp(g, "tdg") == 0) fprintf(out, "tdg q[%d];\n", a);
        else if (strcmp(g, "barrier") == 0) fprintf(out, "barrier q[%d];\n", a);
        p += n;
      } else if (sscanf(p, "%15[^,:]%n", g, &n) >= 1 &&
                 strcmp(g, "barrier") == 0) {
        fprintf(out, "barrier q;\n");
        p += n;
      } else {
        break;
      }
      if (*p == ',') p++;
    }
  } else {
    /* default depth: H·CX·RY·CZ·RZ (mesh vote B_deepen_quantum_qasm) */
    fprintf(out, "h q[0];\n");
    if (n_qubits > 1) {
      fprintf(out, "cx q[0],q[1];\n");
      fprintf(out, "ry(0.52359877559829882) q[1];\n");
      fprintf(out, "cz q[0],q[1];\n");
    }
    fprintf(out, "rz(0.78539816339744828) q[0];\n");
  }
  fprintf(out, "measure q -> c;\n");
  return 0;
}

uint64_t cubalc_q_to_matrix_word(const cubalc_qstate *qs) {
  uint64_t h;
  int i;
  if (!qs) return 0;
  h = 0x51A47A7AULL ^ (uint64_t)qs->n_qubits ^ ((uint64_t)qs->last_meas << 8);
  for (i = 0; i < qs->n_amps && i < 64; i++) {
    double p = cubalc_q_prob(qs, i);
    uint64_t bits;
    memcpy(&bits, &p, sizeof bits);
    h ^= bits + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
  }
  return h;
}

int cubalc_q_write_json(const cubalc_qstate *qs, FILE *out) {
  int i, top = 0;
  double best = -1.0, H = 0.0, sum_p2 = 0.0, pr;
  if (!qs || !out) return -1;
  for (i = 0; i < qs->n_amps; i++) {
    double p = cubalc_q_prob(qs, i);
    if (p > best) {
      best = p;
      top = i;
    }
    /* Shannon entropy (bits) + participation ratio of Born probs */
    if (p > 1e-300) H -= p * (log(p) / log(2.0));
    sum_p2 += p * p;
  }
  pr = (sum_p2 > 1e-300) ? (1.0 / sum_p2) : 0.0;
  return fprintf(out,
                 "{\"schema\":\"cubalc.quantum.v1\",\"ok\":true,"
                 "\"n_qubits\":%d,\"n_amps\":%d,\"norm2\":%.17g,"
                 "\"last_meas\":%d,\"shots\":%d,\"top_basis\":%d,"
                 "\"top_prob\":%.17g,\"entropy_bits\":%.17g,"
                 "\"participation\":%.17g,\"purity_diag\":%.17g,"
                 "\"seed\":\"%016llx\",\"matrix_word\":\"%016llx\","
                 "\"export\":\"openqasm2\",\"backend\":\"statevector\","
                 "\"law\":\"unitary_born;shannon_entropy;qpu_exportable\"}\n",
                 qs->n_qubits, qs->n_amps, cubalc_q_norm2(qs), qs->last_meas,
                 qs->shots_done, top, best, H, pr, sum_p2,
                 (unsigned long long)qs->seed,
                 (unsigned long long)cubalc_q_to_matrix_word(qs));
}

int cubalc_q_write_json_path(const cubalc_qstate *qs, const char *path) {
  FILE *f = fopen(path, "w");
  if (!f) return -1;
  int rc = cubalc_q_write_json(qs, f);
  fclose(f);
  return rc < 0 ? -1 : 0;
}
