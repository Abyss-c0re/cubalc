/* CubalC science loop — 5D · quantum · relativity · chaos coordinator. */
#include "cubalc_science_loop.h"
#include "cubalc.h"
#include "cubalc_law.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void word_to_matrix(uint64_t w, cubalc_matrix *m) {
  int i;
  cubalc_matrix_clear(m);
  m->n = (uint16_t)CUBALC_ATOM_BITS;
  for (i = 0; i < CUBALC_ATOM_BITS; i++) {
    if ((w >> i) & 1ULL) cubalc_matrix_set(m, i, 1);
  }
}

int cubalc_science_loop_run(cubalc_science_loop *S, uint64_t seed, int n_loops,
                            int q_qubits, double v_frac_c, int chaos_steps) {
  int loop;
  cubalc_event5 e0, e1;
  cubalc_matrix mat;
  if (!S) return -1;
  memset(S, 0, sizeof(*S));
  if (n_loops < 1) n_loops = 1;
  if (n_loops > 64) n_loops = 64;
  if (q_qubits < 1) q_qubits = 2;
  if (q_qubits > CUBALC_Q_MAX_QUBITS) q_qubits = CUBALC_Q_MAX_QUBITS;
  if (v_frac_c <= 0.0 || v_frac_c >= 1.0) v_frac_c = 0.5;
  if (chaos_steps < 50) chaos_steps = 200;
  if (!seed) seed = ((uint64_t)time(NULL) << 16) ^ 0x5C1E7CEULL;

  cubalc_dim5_from_seed(seed, &S->event);
  e0 = S->event;

  if (cubalc_rel_from_v(v_frac_c * CUBALC_C_MPS, &S->boost) != 0) {
    S->ok = 0;
    return -1;
  }

  for (loop = 0; loop < n_loops; loop++) {
    /* --- quantum: seed-bound Bell/prep --- */
    cubalc_q_init(&S->quantum, q_qubits, S->event.seed ^ (uint64_t)loop);
    cubalc_q_h(&S->quantum, 0);
    if (q_qubits > 1) cubalc_q_cnot(&S->quantum, 0, 1);
    /* seed tilts second qubit phase (basis loop into 5th dim) */
    cubalc_q_rz(&S->quantum, 0,
                (double)(S->event.seed & 0xffff) / 65536.0 * 6.283185307179586);
    /* pre-collapse Shannon / participation (loop polish ↔ pack B residual) */
    {
      int qi;
      double H = 0.0, sum_p2 = 0.0;
      for (qi = 0; qi < S->quantum.n_amps; qi++) {
        double p = cubalc_q_prob(&S->quantum, qi);
        if (p > 1e-300) H -= p * (log(p) / log(2.0));
        sum_p2 += p * p;
      }
      S->q_entropy_bits = H;
      S->q_participation = (sum_p2 > 1e-300) ? (1.0 / sum_p2) : 0.0;
    }
    cubalc_q_measure_all(&S->quantum);

    /* --- relativity: boost event, interval self-check --- */
    cubalc_rel_boost_x(&S->event, &S->boost, &e1);
    S->ds2 = cubalc_rel_interval2(&e0, &e1);
    /* proper time advances coordinate t via dilated lab tick */
    S->event = e1;
    S->event.t += cubalc_rel_proper_time(0.01, &S->boost);
    /* seed absorbs measurement (quantum → 5th dim loop) */
    S->event.seed ^= ((uint64_t)S->quantum.last_meas << 17) ^
                     cubalc_q_to_matrix_word(&S->quantum);

    /* --- chaos: Lorenz from seed IC --- */
    cubalc_chaos_lorenz_init(&S->chaos, S->event.seed, 0.01);
    cubalc_chaos_lorenz_run(&S->chaos, chaos_steps);
    if (loop == n_loops - 1)
      S->lyapunov = cubalc_chaos_lyapunov(&S->chaos, 400, 1e-8);
    /* chaos state folds into spatial coords (viz + loop) */
    S->event.x = S->chaos.x * 0.1;
    S->event.y = S->chaos.y * 0.1;
    S->event.z = S->chaos.z * 0.1;
    S->event.seed = cubalc_dim5_fold_seed(&S->event) ^
                    cubalc_chaos_to_matrix_word(&S->chaos) ^
                    cubalc_rel_to_matrix_word(&S->boost, S->ds2);

    S->loops_done = loop + 1;
  }

  S->seed_next = S->event.seed;
  S->matrix_word = cubalc_dim5_to_matrix_word(&S->event) ^
                   cubalc_q_to_matrix_word(&S->quantum) ^
                   cubalc_chaos_to_matrix_word(&S->chaos) ^
                   cubalc_rel_to_matrix_word(&S->boost, S->ds2);

  word_to_matrix(S->matrix_word, &mat);
  cubalc_viz_from_matrix(&mat, &S->viz, (uint32_t)S->loops_done, 1.0f,
                         (int)(S->quantum.last_meas % 10), 0, "science_loop",
                         "cpu:science");
  S->ok = 1;
  return 0;
}

int cubalc_science_loop_write_json(const cubalc_science_loop *S, FILE *out) {
  cubalc_chaos_attractor A;
  double rap, drec, dapp;
  if (!S || !out) return -1;
  memset(&A, 0, sizeof A);
  cubalc_chaos_attractor_from(&S->chaos, S->lyapunov, &A);
  rap = cubalc_rel_rapidity(&S->boost);
  drec = cubalc_rel_doppler(&S->boost, 1);
  dapp = cubalc_rel_doppler(&S->boost, 0);
  fprintf(out,
          "{\"schema\":\"cubalc.science_loop.v1\",\"ok\":%s,"
          "\"loops\":%d,\"seed_in_derived\":true,"
          "\"seed_next\":\"%016llx\",\"matrix_word\":\"%016llx\","
          "\"ds2\":%.17g,\"lyapunov\":%.17g,\"chaotic\":%s,"
          "\"quantum_meas\":%d,\"q_norm2\":%.17g,"
          "\"q_entropy_bits\":%.17g,\"q_participation\":%.17g,"
          "\"boost_gamma\":%.17g,\"boost_beta\":%.17g,"
          "\"rapidity\":%.17g,\"doppler_rec\":%.17g,\"doppler_app\":%.17g,"
          "\"lorenz\":{\"x\":%.17g,\"y\":%.17g,\"z\":%.17g,\"steps\":%d},"
          "\"attractor\":{\"path_n\":%d,\"rms_r\":%.17g,"
          "\"lobe_balance\":%.17g,\"x_cross\":%d,\"z_cross\":%d,"
          "\"span\":{\"x\":%.17g,\"y\":%.17g,\"z\":%.17g}},"
          "\"event5\":{\"x\":%.17g,\"y\":%.17g,\"z\":%.17g,\"t\":%.17g,"
          "\"seed\":\"%016llx\"},"
          "\"axes\":[\"x\",\"y\",\"z\",\"t\",\"seed\"],"
          "\"wire\":[\"seed→quantum\",\"quantum→seed\","
          "\"seed→chaos\",\"chaos→xyz\",\"boost→t\","
          "\"fold→matrix→viz→seed_next\"],"
          "\"modules\":[\"quantum\",\"dim5\",\"relativity\",\"chaos\"],"
          "\"law\":\"pure_science;cube_is_sot;free_energy;modular;quartet\","
          "\"version\":\"%s\"}\n",
          S->ok ? "true" : "false", S->loops_done,
          (unsigned long long)S->seed_next,
          (unsigned long long)S->matrix_word, S->ds2, S->lyapunov,
          A.chaotic ? "true" : "false",
          S->quantum.last_meas, cubalc_q_norm2(&S->quantum),
          S->q_entropy_bits, S->q_participation, S->boost.gamma,
          S->boost.beta, rap, drec, dapp,
          S->chaos.x, S->chaos.y, S->chaos.z, S->chaos.steps,
          A.path_n, A.rms_r, A.lobe_balance, A.x_cross, A.z_cross,
          A.span_x, A.span_y, A.span_z,
          S->event.x, S->event.y, S->event.z, S->event.t,
          (unsigned long long)S->event.seed, CUBALC_LANG_VERSION);
  return 0;
}

int cubalc_science_loop_write_json_path(const cubalc_science_loop *S,
                                        const char *path) {
  FILE *f = fopen(path, "w");
  if (!f) return -1;
  int rc = cubalc_science_loop_write_json(S, f);
  fclose(f);
  return rc < 0 ? -1 : 0;
}

static void usage(FILE *f) {
  fprintf(f,
          "usage: cubalc science <sub> [opts]\n"
          "  loop [--seed N] [--loops N] [--qubits N] [--beta F] [--chaos N]\n"
          "  quantum [--qubits N] [--seed N] [--qasm]\n"
          "  relativity|lightcone [--beta F] [--seed N] [--viz] [--viz-n N]\n"
          "  chaos|attractor [--seed N] [--steps N] [--attractor]\n"
          "  dim5|mesh [--seed N] [--mesh N]  # 5D event or seed mesh plate\n"
          "  pack|bundle [--seed N] [--out DIR]  # science surface plate (F)\n"
          "  prove   # self-check science gates\n");
}

static uint64_t parse_u64(const char *s, uint64_t def) {
  char *end = NULL;
  unsigned long long v;
  if (!s || !*s) return def;
  v = strtoull(s, &end, 0);
  if (end == s) return def;
  return (uint64_t)v;
}

static int science_prove(void) {
  cubalc_science_loop S;
  cubalc_lorentz L;
  cubalc_qstate qs;
  double n2, ly;
  int fails = 0;

  /* relativity: γ(0)=1, γ→∞ as β→1, |v|<c enforced */
  if (cubalc_rel_from_v(0.0, &L) != 0 || fabs(L.gamma - 1.0) > 1e-12) fails++;
  if (cubalc_rel_from_v(0.6 * CUBALC_C_MPS, &L) != 0) fails++;
  if (fabs(L.gamma - 1.25) > 1e-9) fails++; /* γ(0.6)=1.25 exact */
  if (cubalc_rel_from_v(CUBALC_C_MPS, &L) == 0) fails++; /* must reject */

  /* quantum: H|0⟩ norm + Bell measure domain */
  cubalc_q_init(&qs, 2, 42);
  cubalc_q_h(&qs, 0);
  cubalc_q_cnot(&qs, 0, 1);
  n2 = cubalc_q_norm2(&qs);
  if (fabs(n2 - 1.0) > 1e-9) fails++;
  cubalc_q_measure_all(&qs);
  if (qs.last_meas < 0 || qs.last_meas > 3) fails++;

  /* chaos: Lorenz runs finite */
  {
    cubalc_lorenz C;
    cubalc_chaos_lorenz_init(&C, 7, 0.01);
    cubalc_chaos_lorenz_run(&C, 100);
    if (C.steps != 100) fails++;
    ly = cubalc_chaos_lyapunov(&C, 200, 1e-8);
    if (!isfinite(ly)) fails++;
  }

  /* full loop */
  if (cubalc_science_loop_run(&S, 0x5C1E7CE1ULL, 3, 2, 0.5, 150) != 0) fails++;
  if (!S.ok || S.loops_done != 3) fails++;

  printf("{\"schema\":\"cubalc.science_prove.v1\",\"ok\":%s,\"fails\":%d,"
         "\"gamma06\":%.17g,\"q_norm\":%.17g,\"lyapunov\":%.17g,"
         "\"matrix_word\":\"%016llx\",\"law\":\"pure_science_gates\"}\n",
         fails == 0 ? "true" : "false", fails, 1.25, n2, S.lyapunov,
         (unsigned long long)S.matrix_word);
  return fails ? 1 : 0;
}

int cubalc_cmd_science(int argc, char **argv) {
  const char *sub;
  uint64_t seed = 0;
  int i, loops = 3, qubits = 2, chaos_steps = 300, want_qasm = 0, mesh_n = 0,
         want_viz = 0, viz_n = 8, want_attractor = 0;
  double beta = 0.5;
  const char *state_dir = "state";
  /* default QASM depth plate (B_deepen): H·CX·RY·CZ·RZ */
  const char *gates_csv = "h:0,cnot:0:1,ry:1:0.52359877559829882,cz:0:1,rz:0:0.78539816339744828";

  if (argc < 3) {
    usage(stderr);
    return 2;
  }
  sub = argv[2];
  for (i = 3; i < argc; i++) {
    if ((!strcmp(argv[i], "--seed") || !strcmp(argv[i], "-s")) && i + 1 < argc)
      seed = parse_u64(argv[++i], 0);
    else if ((!strcmp(argv[i], "--loops") || !strcmp(argv[i], "-n")) &&
             i + 1 < argc)
      loops = (int)parse_u64(argv[++i], 3);
    else if ((!strcmp(argv[i], "--qubits") || !strcmp(argv[i], "-q")) &&
             i + 1 < argc)
      qubits = (int)parse_u64(argv[++i], 2);
    else if ((!strcmp(argv[i], "--beta") || !strcmp(argv[i], "-b")) &&
             i + 1 < argc)
      beta = atof(argv[++i]);
    else if ((!strcmp(argv[i], "--chaos") || !strcmp(argv[i], "--steps")) &&
             i + 1 < argc)
      chaos_steps = (int)parse_u64(argv[++i], 300);
    else if (!strcmp(argv[i], "--qasm"))
      want_qasm = 1;
    else if (!strcmp(argv[i], "--viz") || !strcmp(argv[i], "--lightcone"))
      want_viz = 1;
    else if (!strcmp(argv[i], "--attractor") || !strcmp(argv[i], "--butterfly"))
      want_attractor = 1;
    else if ((!strcmp(argv[i], "--viz-n") || !strcmp(argv[i], "--samples")) &&
             i + 1 < argc)
      viz_n = (int)parse_u64(argv[++i], 8);
    else if ((!strcmp(argv[i], "--gates") || !strcmp(argv[i], "--gate")) &&
             i + 1 < argc)
      gates_csv = argv[++i];
    else if ((!strcmp(argv[i], "--mesh") || !strcmp(argv[i], "--nodes")) &&
             i + 1 < argc)
      mesh_n = (int)parse_u64(argv[++i], 8);
    else if ((!strcmp(argv[i], "--out") || !strcmp(argv[i], "-o")) &&
             i + 1 < argc)
      state_dir = argv[++i];
  }

  if (!strcmp(sub, "prove") || !strcmp(sub, "selftest") ||
      !strcmp(sub, "test")) {
    return science_prove();
  }

  /* F_main_cli_extract adjacent: one-shot science surface pack (no main thrash). */
  if (!strcmp(sub, "pack") || !strcmp(sub, "bundle") || !strcmp(sub, "surface")) {
    cubalc_qstate qs;
    cubalc_lorentz Lr;
    cubalc_event5 e0;
    cubalc_dim5_mesh M;
    cubalc_rel_viz V;
    cubalc_lorenz C;
    cubalc_chaos_attractor A;
    double ly, ds2, q_H, q_sum_p2, q_pr;
    uint64_t mw = 0;
    char path[512];
    int prc;
    FILE *pf;
    uint64_t s0 = seed ? seed : 42ULL;
    prc = science_prove();
    cubalc_q_init(&qs, qubits > 0 ? qubits : 2, s0);
    cubalc_q_h(&qs, 0);
    if (qubits > 1 || 1) cubalc_q_cnot(&qs, 0, 1);
    /* B residual: Shannon of pre-collapse Bell before measure */
    {
      int qi;
      q_H = 0.0;
      q_sum_p2 = 0.0;
      for (qi = 0; qi < qs.n_amps; qi++) {
        double p = cubalc_q_prob(&qs, qi);
        if (p > 1e-300) q_H -= p * (log(p) / log(2.0));
        q_sum_p2 += p * p;
      }
      q_pr = (q_sum_p2 > 1e-300) ? (1.0 / q_sum_p2) : 0.0;
    }
    cubalc_q_measure_all(&qs);
    cubalc_dim5_from_seed(s0, &e0);
    cubalc_dim5_mesh_build(s0, mesh_n > 0 ? mesh_n : 6, &M);
    cubalc_rel_from_v(beta * CUBALC_C_MPS, &Lr);
    cubalc_rel_viz_build(&Lr, &e0, viz_n > 1 ? viz_n : 6, 0.1, &V);
    {
      cubalc_event5 eb;
      cubalc_rel_boost_x(&e0, &Lr, &eb);
      ds2 = cubalc_rel_interval2(&e0, &eb);
      (void)ds2;
    }
    cubalc_chaos_lorenz_init(&C, s0, 0.01);
    cubalc_chaos_lorenz_run(&C, chaos_steps > 50 ? chaos_steps : 300);
    ly = cubalc_chaos_lyapunov(&C, 200, 1e-8);
    cubalc_chaos_attractor_from(&C, ly, &A);
    mw = cubalc_q_to_matrix_word(&qs) ^ M.mesh_word ^ V.matrix_word ^ A.matrix_word;
    /* pack_surface_sync: B/E/C/D residual fields on pack plate */
    printf("{\"schema\":\"cubalc.science_pack.v1\",\"ok\":%s,"
           "\"prove_ok\":%s,\"seed\":\"%016llx\","
           "\"modules\":[\"quantum\",\"dim5_mesh\",\"relativity_viz\","
           "\"chaos_attractor\",\"prove\"],"
           "\"q_norm\":%.17g,\"q_entropy_bits\":%.17g,\"q_participation\":%.17g,"
           "\"mesh_word\":\"%016llx\","
           "\"mesh_n\":%d,\"mesh_span_xyz\":%.17g,\"mesh_edge_n\":%d,"
           "\"mesh_rms_r\":%.17g,\"mesh_centroid\":{\"x\":%.17g,\"y\":%.17g,\"z\":%.17g},"
           "\"rel_gamma\":%.17g,\"rel_rapidity\":%.17g,"
           "\"doppler_rec\":%.17g,\"doppler_app\":%.17g,"
           "\"rel_proper_time\":%.17g,\"worldline_causal\":%s,"
           "\"lyapunov\":%.17g,\"chaotic\":%s,"
           "\"rms_r\":%.17g,\"x_cross\":%d,\"z_cross\":%d,"
           "\"lobe_pos\":%d,\"lobe_neg\":%d,\"lobe_balance\":%.17g,"
           "\"matrix_word\":\"%016llx\","
           "\"cli\":[\"science prove\",\"science quantum --qasm\","
           "\"science mesh --mesh N\",\"science relativity --viz\","
           "\"science attractor\",\"science pack\",\"science loop\"],"
           "\"law\":\"pure_science_pack;quartet_sync;B_E_C_D;cube_is_sot;zero_bloat\"}\n",
           prc == 0 ? "true" : "false", prc == 0 ? "true" : "false",
           (unsigned long long)s0, cubalc_q_norm2(&qs), q_H, q_pr,
           (unsigned long long)M.mesh_word, M.n, M.span_xyz, M.edge_n,
           M.rms_r, M.cx, M.cy, M.cz,
           Lr.gamma, V.rapidity, V.doppler_rec, V.doppler_app, V.proper_time,
           (V.n > 0 && V.inside_cone[V.n - 1]) ? "true" : "false",
           ly, A.chaotic ? "true" : "false", A.rms_r, A.x_cross, A.z_cross,
           A.lobe_pos, A.lobe_neg, A.lobe_balance,
           (unsigned long long)mw);
    snprintf(path, sizeof path, "%s/science_pack.json", state_dir);
    pf = fopen(path, "w");
    if (pf) {
      fprintf(pf,
              "{\"schema\":\"cubalc.science_pack.v1\",\"ok\":%s,"
              "\"prove_ok\":%s,\"seed\":\"%016llx\","
              "\"q_norm\":%.17g,\"q_entropy_bits\":%.17g,\"q_participation\":%.17g,"
              "\"mesh_n\":%d,\"mesh_word\":\"%016llx\","
              "\"mesh_span_xyz\":%.17g,\"mesh_edge_n\":%d,\"mesh_rms_r\":%.17g,"
              "\"mesh_centroid\":{\"x\":%.17g,\"y\":%.17g,\"z\":%.17g},"
              "\"rel_gamma\":%.17g,\"rel_beta\":%.17g,\"rel_rapidity\":%.17g,"
              "\"doppler_rec\":%.17g,\"doppler_app\":%.17g,\"rel_proper_time\":%.17g,"
              "\"lyapunov\":%.17g,\"chaotic\":%s,\"attractor_path_n\":%d,"
              "\"rms_r\":%.17g,\"x_cross\":%d,\"z_cross\":%d,"
              "\"lobe_pos\":%d,\"lobe_neg\":%d,\"lobe_balance\":%.17g,"
              "\"matrix_word\":\"%016llx\","
              "\"law\":\"pure_science_pack;quartet_sync;B_E_C_D;cube_is_sot\"}\n",
              prc == 0 ? "true" : "false", prc == 0 ? "true" : "false",
              (unsigned long long)s0, cubalc_q_norm2(&qs), q_H, q_pr, M.n,
              (unsigned long long)M.mesh_word, M.span_xyz, M.edge_n, M.rms_r,
              M.cx, M.cy, M.cz, Lr.gamma, Lr.beta, V.rapidity, V.doppler_rec,
              V.doppler_app, V.proper_time, ly,
              A.chaotic ? "true" : "false", A.path_n, A.rms_r, A.x_cross,
              A.z_cross, A.lobe_pos, A.lobe_neg, A.lobe_balance,
              (unsigned long long)mw);
      fclose(pf);
    }
    return prc == 0 ? 0 : 1;
  }

  if (!strcmp(sub, "loop") || !strcmp(sub, "wire") || !strcmp(sub, "all")) {
    cubalc_science_loop S;
    char path[512], vizpath[512];
    if (cubalc_science_loop_run(&S, seed, loops, qubits, beta, chaos_steps) !=
        0) {
      printf("{\"ok\":false,\"err\":\"science_loop_failed\"}\n");
      return 1;
    }
    cubalc_science_loop_write_json(&S, stdout);
    snprintf(path, sizeof path, "%s/science_loop.json", state_dir);
    cubalc_science_loop_write_json_path(&S, path);
    snprintf(path, sizeof path, "%s/science_dim5.json", state_dir);
    cubalc_dim5_write_json_path(&S.event, path);
    snprintf(path, sizeof path, "%s/science_quantum.json", state_dir);
    cubalc_q_write_json_path(&S.quantum, path);
    snprintf(path, sizeof path, "%s/science_relativity.json", state_dir);
    cubalc_rel_write_json_path(&S.boost, &S.event, S.ds2, path);
    snprintf(path, sizeof path, "%s/science_chaos.json", state_dir);
    cubalc_chaos_write_json_path(&S.chaos, S.lyapunov, path);
    snprintf(vizpath, sizeof vizpath, "%s/science_viz_frame.json", state_dir);
    cubalc_viz_write_json_path(&S.viz, vizpath);
    return 0;
  }

  if (!strcmp(sub, "quantum") || !strcmp(sub, "q")) {
    cubalc_qstate qs;
    cubalc_q_init(&qs, qubits, seed ? seed : 1ULL);
    if (gates_csv && gates_csv[0]) {
      /* B_deepen_quantum_qasm: CSV drives both statevector + QASM */
      if (cubalc_q_apply_gates_csv(&qs, gates_csv) != 0) {
        printf("{\"ok\":false,\"err\":\"q_gates_csv_failed\"}\n");
        return 1;
      }
    } else {
      cubalc_q_h(&qs, 0);
      if (qubits > 1) cubalc_q_cnot(&qs, 0, 1);
    }
    {
      double n2 = cubalc_q_norm2(&qs);
      if (n2 < 0.999 || n2 > 1.001) {
        printf("{\"ok\":false,\"err\":\"q_norm_failed\",\"norm2\":%.17g}\n", n2);
        return 1;
      }
    }
    /* plate first (Shannon/PR of unitary Born probs), then collapse */
    cubalc_q_write_json(&qs, stdout);
    cubalc_q_measure_all(&qs);
    if (want_qasm) {
      printf("---- OPENQASM2 ----\n");
      cubalc_q_write_qasm(qubits, gates_csv, stdout);
    }
    return 0;
  }

  if (!strcmp(sub, "relativity") || !strcmp(sub, "rel") ||
      !strcmp(sub, "einstein") || !strcmp(sub, "lightcone")) {
    cubalc_lorentz L;
    cubalc_event5 a, b;
    cubalc_rel_viz V;
    double ds2;
    char path[512];
    cubalc_dim5_from_seed(seed ? seed : 1ULL, &a);
    if (cubalc_rel_from_v(beta * CUBALC_C_MPS, &L) != 0) {
      printf("{\"ok\":false,\"err\":\"|v| must be < c\"}\n");
      return 1;
    }
    cubalc_rel_boost_x(&a, &L, &b);
    ds2 = cubalc_rel_interval2(&a, &b);
    if (want_viz || !strcmp(sub, "lightcone")) {
      if (viz_n < 2) viz_n = 8;
      if (cubalc_rel_viz_build(&L, &a, viz_n, 0.1, &V) != 0) {
        printf("{\"ok\":false,\"err\":\"rel_viz_failed\"}\n");
        return 1;
      }
      cubalc_rel_viz_write_json(&V, stdout);
      snprintf(path, sizeof path, "%s/science_relativity_viz.json", state_dir);
      cubalc_rel_viz_write_json_path(&V, path);
      return 0;
    }
    cubalc_rel_write_json(&L, &b, ds2, stdout);
    return 0;
  }

  if (!strcmp(sub, "chaos") || !strcmp(sub, "lorenz") ||
      !strcmp(sub, "attractor")) {
    cubalc_lorenz C;
    cubalc_chaos_attractor A;
    double ly;
    char path[512];
    cubalc_chaos_lorenz_init(&C, seed ? seed : 1ULL, 0.01);
    cubalc_chaos_lorenz_run(&C, chaos_steps);
    ly = cubalc_chaos_lyapunov(&C, 400, 1e-8);
    if (want_attractor || !strcmp(sub, "attractor")) {
      if (cubalc_chaos_attractor_from(&C, ly, &A) != 0) {
        printf("{\"ok\":false,\"err\":\"chaos_attractor_failed\"}\n");
        return 1;
      }
      cubalc_chaos_attractor_write_json(&A, stdout);
      snprintf(path, sizeof path, "%s/science_chaos_attractor.json", state_dir);
      cubalc_chaos_attractor_write_json_path(&A, path);
      return 0;
    }
    cubalc_chaos_write_json(&C, ly, stdout);
    return 0;
  }

  if (!strcmp(sub, "dim5") || !strcmp(sub, "5d") || !strcmp(sub, "seed") ||
      !strcmp(sub, "mesh")) {
    cubalc_event5 e;
    cubalc_dim5_mesh M;
    char path[512];
    if (mesh_n > 0 || !strcmp(sub, "mesh")) {
      if (mesh_n < 1) mesh_n = 8;
      if (cubalc_dim5_mesh_build(seed ? seed : 1ULL, mesh_n, &M) != 0) {
        printf("{\"ok\":false,\"err\":\"dim5_mesh_failed\"}\n");
        return 1;
      }
      cubalc_dim5_mesh_write_json(&M, stdout);
      snprintf(path, sizeof path, "%s/science_dim5_mesh.json", state_dir);
      cubalc_dim5_mesh_write_json_path(&M, path);
      return 0;
    }
    cubalc_dim5_from_seed(seed, &e);
    cubalc_dim5_write_json(&e, stdout);
    return 0;
  }

  usage(stderr);
  return 2;
}
