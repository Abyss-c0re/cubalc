/* CubalC matrix harmonic solver — standalone process.
 * Independent of Grok Build TUI. Scalable CPU multi-thread + OpenCL GPU.
 * Law: State Matrix SoT · unity = 1 − hamming/ATOM_BITS · consensus = majority.
 *
 * Usage:
 *   matrix_harmonic_solver --once
 *   matrix_harmonic_solver --loop --hz 0.5
 *   matrix_harmonic_solver --bench --n 16
 *
 * Env:
 *   CUBALC_STATE          state dir (default: ./state or /opt/nexuscore/lab/state)
 *   MATRIX_SOLVER_STATUS  status JSON path
 *   MATRIX_SOLVER_HZ      default loop rate
 */
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "cubalc.h"
#include "cubalc_hw.h"
#include "cubalc_algocube.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

static volatile int g_stop = 0;
static void on_sig(int s) { (void)s; g_stop = 1; }

static long mono_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long)ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

static void matrix_from_seed(cubalc_matrix *m, uint64_t seed) {
  cubalc_matrix_clear(m);
  m->n = CUBALC_ATOM_BITS;
  for (int i = 0; i < CUBALC_ATOM_BITS; i++) {
    seed = seed * 6364136223846793005ULL + 1ULL;
    if (seed & (1ULL << 63))
      cubalc_matrix_set(m, i, 1);
  }
}

/* Load up to max matrices from live/fleet observe JSON if present; else synthetic. */
static int load_mats(cubalc_matrix *mats, int max, const char *state_dir) {
  char path[512];
  /* Prefer nanobot observe plate (real peers) */
  snprintf(path, sizeof path, "%s/nanobot_observe.json", state_dir);
  FILE *f = fopen(path, "rb");
  if (!f) {
    snprintf(path, sizeof path, "/opt/nexuscore/lab/state/nanobot_observe.json");
    f = fopen(path, "rb");
  }
  int n = 0;
  if (f) {
    char *buf = NULL;
    long sz;
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz > 0 && sz < 8 * 1024 * 1024) {
      buf = (char *)malloc((size_t)sz + 1);
      if (buf && fread(buf, 1, (size_t)sz, f) == (size_t)sz) {
        buf[sz] = 0;
        /* Extract "matrix" or "bits" hex/binary fields if present — light scan */
        char *p = buf;
        while (n < max && (p = strstr(p, "\"bits\"")) != NULL) {
          p += 6;
          while (*p && *p != '"' && *p != '0' && *p != '1') p++;
          if (*p == '"') p++;
          if (*p == '0' || *p == '1') {
            cubalc_matrix_clear(&mats[n]);
            cubalc_matrix_from_ascii(&mats[n], p, CUBALC_ATOM_BITS);
            if (mats[n].n > 0) n++;
          }
        }
      }
      free(buf);
    }
    fclose(f);
  }
  if (n >= 2) return n;

  /* Synthetic bank mirroring lean atlas mesh (6 peers) for continuous solve */
  int want = max < 6 ? max : 6;
  for (int i = 0; i < want; i++)
    matrix_from_seed(&mats[i], 0xC0BEA160ULL ^ (uint64_t)(i + 1) * 0x9E3779B97F4A7C15ULL);
  return want;
}

static int write_status(const char *path, int n, float unity, int digit,
                        const cubalc_matrix *consensus, long ms, int ok) {
  FILE *f = fopen(path, "wb");
  if (!f) return -1;
  unsigned set = consensus ? (unsigned)consensus->set : 0;
  time_t now = time(NULL);
  char iso[40];
  struct tm tm;
  gmtime_r(&now, &tm);
  strftime(iso, sizeof iso, "%Y-%m-%dT%H:%M:%SZ", &tm);
  fprintf(f,
    "{\n"
    "  \"schema\": \"cube.matrix.harmonic.solver.v1\",\n"
    "  \"engine\": \"cubalc_hw\",\n"
    "  \"standalone\": true,\n"
    "  \"grok_tui\": false,\n"
    "  \"ts\": \"%s\",\n"
    "  \"ok\": %s,\n"
    "  \"n\": %d,\n"
    "  \"unity\": %.6f,\n"
    "  \"digit\": %d,\n"
    "  \"consensus_set\": %u,\n"
    "  \"backend\": \"%s\",\n"
    "  \"workers\": %d,\n"
    "  \"gpu_ok\": %s,\n"
    "  \"solve_ms\": %ld,\n"
    "  \"law\": \"unity=1-hamming/ATOM_BITS;consensus=majority;cpu+gpu_scale\"\n"
    "}\n",
    iso,
    ok ? "true" : "false",
    n, unity, digit, set,
    cubalc_hw_backend(),
    cubalc_hw_workers(),
    cubalc_hw_gpu_ok() ? "true" : "false",
    ms);
  fclose(f);
  return 0;
}

static int solve_once(const char *state_dir, const char *status_path, int n_force) {
  cubalc_matrix mats[CUBALC_MAX_CUBES];
  int n = n_force > 0 ? n_force : 0;
  if (n_force > 0) {
    if (n > CUBALC_MAX_CUBES) n = CUBALC_MAX_CUBES;
    for (int i = 0; i < n; i++)
      matrix_from_seed(&mats[i], 0xC0BEA160ULL ^ (uint64_t)(i + 1) * 0x9E3779B97F4A7C15ULL);
  } else {
    n = load_mats(mats, CUBALC_MAX_CUBES, state_dir);
  }
  const cubalc_matrix *ptrs[CUBALC_MAX_CUBES];
  for (int i = 0; i < n; i++) ptrs[i] = &mats[i];

  long t0 = mono_ms();
  cubalc_algo_harm h;
  int rc = cubalc_algocube_harmony(ptrs, n, &h);
  long ms = mono_ms() - t0;

  printf("matrix-harmonic-solver n=%d unity=%.4f digit=%d backend=%s gpu=%d ms=%ld ok=%d\n",
         n, h.unity, h.digit, cubalc_hw_backend(), cubalc_hw_gpu_ok(), ms, h.ok && rc == 0);

  write_status(status_path, n, h.unity, h.digit, &h.consensus, ms, h.ok && rc == 0);

  /* Mirror plate for observers */
  char plate[512];
  snprintf(plate, sizeof plate, "/opt/nexuscore/plates/MATRIX_HARMONY_STATUS.json");
  write_status(plate, n, h.unity, h.digit, &h.consensus, ms, h.ok && rc == 0);

  /* CubalC state harmony.json if state dir writable */
  char harm[512];
  snprintf(harm, sizeof harm, "%s/harmony.json", state_dir);
  FILE *hf = fopen(harm, "wb");
  if (hf) {
    cubalc_algocube_harmony_json(&h, hf);
    fclose(hf);
  }
  return (h.ok && rc == 0) ? 0 : 1;
}

static void usage(const char *argv0) {
  fprintf(stderr,
    "usage: %s [--once|--loop] [--hz F] [--bench --n N] [--workers N]\n"
    "  CubalC native matrix harmonic solver (CPU multi-thread + OpenCL GPU).\n"
    "  Standalone — no Grok TUI required.\n",
    argv0);
}

int main(int argc, char **argv) {
  int loop = 0, bench = 0, n_force = 0, workers = 0;
  double hz = 0.5;
  const char *env_hz = getenv("MATRIX_SOLVER_HZ");
  if (env_hz && env_hz[0]) hz = atof(env_hz);
  if (hz <= 0) hz = 0.5;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--once")) loop = 0;
    else if (!strcmp(argv[i], "--loop")) loop = 1;
    else if (!strcmp(argv[i], "--bench")) bench = 1;
    else if (!strcmp(argv[i], "--hz") && i + 1 < argc) hz = atof(argv[++i]);
    else if (!strcmp(argv[i], "--n") && i + 1 < argc) n_force = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--workers") && i + 1 < argc) workers = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
      usage(argv[0]);
      return 0;
    }
  }

  cubalc_hw_init(workers);

  const char *state = getenv("CUBALC_STATE");
  if (!state || !state[0]) {
    if (access("/opt/nexuscore/lab/state", W_OK) == 0)
      state = "/opt/nexuscore/lab/state";
    else
      state = "state";
  }
  const char *status = getenv("MATRIX_SOLVER_STATUS");
  char status_buf[512];
  if (!status || !status[0]) {
    snprintf(status_buf, sizeof status_buf, "%s/matrix_harmony_status.json", state);
    status = status_buf;
  }

  signal(SIGINT, on_sig);
  signal(SIGTERM, on_sig);

  if (bench) {
    if (n_force < 2) n_force = 16;
    return solve_once(state, status, n_force);
  }

  if (!loop)
    return solve_once(state, status, n_force);

  printf("matrix-harmonic-solver loop hz=%.3f backend=%s (standalone, no Grok)\n",
         hz, cubalc_hw_backend());
  while (!g_stop) {
    solve_once(state, status, n_force);
    double period = 1.0 / hz;
    if (period < 0.2) period = 0.2;
    if (period > 3600) period = 3600;
    long us = (long)(period * 1e6);
    /* interruptible sleep */
    for (long left = us; left > 0 && !g_stop; ) {
      long chunk = left > 200000 ? 200000 : left;
      usleep((useconds_t)chunk);
      left -= chunk;
    }
  }
  return 0;
}
