/* CubalC real-time EEG → State Matrix streamer (standalone, no Grok TUI).
 *
 * Usage:
 *   eeg_matrix_stream --demo [--hz 10]
 *   eeg_matrix_stream --file path.csv --ch 8 --win 32
 *   eeg_matrix_stream --loop --file /tmp/eeg.fifo
 *
 * CSV: one sample per line, comma/space separated channels.
 * Packs windows into 64-bit State Matrices; optional harmony over ring.
 * Status: CUBALC_STATE/eeg_matrix_status.json
 */
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "cubalc.h"
#include "cubalc_eeg.h"
#include "cubalc_hw.h"
#include "cubalc_algocube.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

static volatile int g_stop = 0;
static void on_sig(int s) { (void)s; g_stop = 1; }

static long mono_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long)ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

static void write_status(const char *path, const cubalc_matrix *m, int n_ch,
                         long seq, float unity, int digit, long ms) {
  FILE *f = fopen(path, "wb");
  if (!f) return;
  char bits[CUBALC_ATOM_BITS + 1];
  cubalc_eeg_matrix_bits(m, bits, sizeof bits);
  time_t now = time(NULL);
  char iso[40];
  struct tm tm;
  gmtime_r(&now, &tm);
  strftime(iso, sizeof iso, "%Y-%m-%dT%H:%M:%SZ", &tm);
  fprintf(f,
    "{\n"
    "  \"schema\": \"cube.eeg.matrix.stream.v1\",\n"
    "  \"engine\": \"cubalc_eeg\",\n"
    "  \"standalone\": true,\n"
    "  \"grok_tui\": false,\n"
    "  \"ts\": \"%s\",\n"
    "  \"ok\": true,\n"
    "  \"n_ch\": %d,\n"
    "  \"seq\": %ld,\n"
    "  \"set\": %u,\n"
    "  \"digit\": %d,\n"
    "  \"unity\": %.6f,\n"
    "  \"backend\": \"%s\",\n"
    "  \"solve_ms\": %ld,\n"
    "  \"bits\": \"%s\",\n"
    "  \"law\": \"eeg_window→state_matrix;harmony=mean_unity\"\n"
    "}\n",
    iso, n_ch, seq, m ? (unsigned)m->set : 0u, digit, unity,
    cubalc_hw_backend(), ms, bits);
  fclose(f);
  /* plate mirror */
  FILE *p = fopen("/opt/nexuscore/plates/EEG_MATRIX_STATUS.json", "wb");
  if (p) {
    fprintf(p,
      "{\"schema\":\"cube.eeg.matrix.stream.v1\",\"seq\":%ld,\"digit\":%d,"
      "\"unity\":%.4f,\"backend\":\"%s\",\"bits\":\"%s\"}\n",
      seq, digit, unity, cubalc_hw_backend(), bits);
    fclose(p);
  }
}

int main(int argc, char **argv) {
  int demo = 0, loop = 0, n_ch = 8, win = 32;
  double hz = 10.0;
  const char *file = NULL;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--demo")) demo = 1;
    else if (!strcmp(argv[i], "--loop")) loop = 1;
    else if (!strcmp(argv[i], "--file") && i + 1 < argc) file = argv[++i];
    else if (!strcmp(argv[i], "--ch") && i + 1 < argc) n_ch = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--win") && i + 1 < argc) win = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--hz") && i + 1 < argc) hz = atof(argv[++i]);
    else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
      fprintf(stderr,
        "usage: %s [--demo|--file PATH] [--ch N] [--win N] [--hz F] [--loop]\n"
        "  CubalC EEG → State Matrix real-time packer (standalone).\n",
        argv[0]);
      return 0;
    }
  }
  if (!demo && !file) demo = 1;
  if (n_ch < 1) n_ch = 1;
  if (n_ch > CUBALC_EEG_MAX_CH) n_ch = CUBALC_EEG_MAX_CH;
  if (win < 1) win = 16;
  if (hz <= 0) hz = 10.0;

  cubalc_hw_init(0);
  signal(SIGINT, on_sig);
  signal(SIGTERM, on_sig);

  const char *state = getenv("CUBALC_STATE");
  if (!state || !state[0]) {
    if (access("/opt/nexuscore/lab/state", W_OK) == 0)
      state = "/opt/nexuscore/lab/state";
    else
      state = "state";
  }
  char status_path[512];
  snprintf(status_path, sizeof status_path, "%s/eeg_matrix_status.json", state);

  cubalc_eeg_frame fr;
  cubalc_eeg_frame_init(&fr, n_ch, CUBALC_EEG_DEF_SCALE);
  cubalc_matrix mats[32];
  int n_mats = 0;
  long seq = 0;
  long seed = 1;

  printf("eeg-matrix-stream ch=%d win=%d hz=%.1f backend=%s demo=%d\n",
         n_ch, win, hz, cubalc_hw_backend(), demo);

  do {
    cubalc_matrix m;
    long t0 = mono_ms();
    int packed = 0;

    if (demo) {
      cubalc_eeg_demo_frame(&fr, n_ch, seed++);
      if (cubalc_eeg_pack_matrix(&fr, &m) == 0) packed = 1;
    } else {
      FILE *fp = fopen(file, "rb");
      if (!fp) {
        fprintf(stderr, "eeg-matrix-stream: cannot open %s\n", file);
        if (!loop) return 1;
        usleep(200000);
        continue;
      }
      float ch[CUBALC_EEG_MAX_CH];
      int lines = 0;
      while (lines < win * 4 && !g_stop) {
        int rc = cubalc_eeg_read_csv_sample(fp, ch, n_ch);
        if (rc < 0) break;
        if (rc > 0) continue;
        int pr = cubalc_eeg_window_push(&fr, ch, n_ch, win, 0, &m);
        lines++;
        if (pr == 1) { packed = 1; break; }
      }
      if (!packed && fr.n_samp > 0) {
        if (cubalc_eeg_window_push(&fr, ch, n_ch, win, 1, &m) == 1)
          packed = 1;
      }
      fclose(fp);
    }

    if (packed) {
      seq++;
      int digit = cubalc_algocube_digit(&m);
      if (n_mats < 32) mats[n_mats++] = m;
      else {
        memmove(mats, mats + 1, 31 * sizeof(cubalc_matrix));
        mats[31] = m;
      }
      float unity = 1.f;
      if (n_mats >= 2)
        unity = cubalc_hw_harmony_unity(mats, n_mats);
      long ms = mono_ms() - t0;
      write_status(status_path, &m, n_ch, seq, unity, digit, ms);
      char bits[CUBALC_ATOM_BITS + 1];
      cubalc_eeg_matrix_bits(&m, bits, sizeof bits);
      printf("eeg seq=%ld digit=%d set=%u unity=%.4f bits=%.16s… ms=%ld\n",
             seq, digit, (unsigned)m.set, unity, bits, ms);
    }

    if (!loop) break;
    double period = 1.0 / hz;
    if (period < 0.02) period = 0.02;
    long us = (long)(period * 1e6);
    for (long left = us; left > 0 && !g_stop; ) {
      long chunk = left > 50000 ? 50000 : left;
      usleep((useconds_t)chunk);
      left -= chunk;
    }
  } while (loop && !g_stop);

  return 0;
}
