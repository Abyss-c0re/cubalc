/* CubalC CLI showcase — F_main peel from main · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_showcase.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

static void state_dir(char *buf, size_t n) {
  const char *e = getenv("CUBALC_STATE");
  if (e && e[0]) { snprintf(buf, n, "%s", e); return; }
  snprintf(buf, n, "state");
}

static void ensure_dir(const char *d) { mkdir(d, 0755); }

/* ── CubalC Showcase — multi-act COP demonstration ── */
static void showcase_root(char *out, size_t n) {
  const char *e = getenv("CUBALC_ROOT");
  if (e && e[0]) { snprintf(out, n, "%s", e); return; }
  char self[512];
  ssize_t m = readlink("/proc/self/exe", self, sizeof self - 1);
  if (m > 0) {
    self[m] = 0;
    /* .../out/cubalc → parent of out */
    char *slash = strrchr(self, '/');
    if (slash) {
      *slash = 0; /* strip binary name */
      slash = strrchr(self, '/');
      if (slash && strcmp(slash, "/out") == 0) {
        *slash = 0;
        snprintf(out, n, "%s", self);
        return;
      }
      /* if run from PATH install, fall back to cwd */
    }
  }
  if (!getcwd(out, n)) snprintf(out, n, ".");
}

static int run_one(const char *root, const char *label, const char *rel,
                   int *pass, int *fail, int *asserts_ok, int *asserts_fail,
                   int *cubes_max) {
  char path[768];
  snprintf(path, sizeof path, "%s/%s", root, rel);
  cubalc_run_result rr;
  memset(&rr, 0, sizeof rr);
  int rc = cubalc_run_file(path, &rr, NULL);
  int ok = (rc == 0 && rr.ok);
  if (ok) {
    (*pass)++;
    printf("  ✓ %-28s asserts=%d n=%d unity=%.3f\n",
           label, rr.asserts_ok, rr.n_cubes, rr.unity);
  } else {
    (*fail)++;
    printf("  ✗ %-28s err=%s asserts_fail=%d\n",
           label, rr.err[0] ? rr.err : "fail", rr.asserts_fail);
  }
  *asserts_ok += rr.asserts_ok;
  *asserts_fail += rr.asserts_fail;
  if (rr.n_cubes > *cubes_max) *cubes_max = rr.n_cubes;
  return ok ? 0 : 1;
}

static void print_cube_art(void) {
  puts("");
  puts("              ┌─────────────────┐");
  puts("             ╱                 ╱│");
  puts("            ╱   C U B a l C   ╱ │");
  puts("           ╱   1.8.0-nest     ╱  │");
  puts("          ┌─────────────────┐   │");
  puts("          │  ■ ■ ■ ■ ■ ■ ■  │   │");
  puts("          │  ■ COP / flow ■  │   │");
  puts("          │  ■ nest→matrix■  │   │");
  puts("          │  ■ no flow? no■  │  ╱");
  puts("          │  ■ compile    ■  │ ╱");
  puts("          │  ■ ■ ■ ■ ■ ■ ■  │╱");
  puts("          └─────────────────┘");
  puts("     bits flow · nest folds · matrix is the unit");
  puts("");
}

int cubalc_cmd_showcase(void) {
  char root[512];
  showcase_root(root, sizeof root);
  print_cube_art();
  printf("CubalC %s — Showcase\n", CUBALC_LANG_VERSION);
  printf("paradigm=%s · creed=%s · hold_flash=%d · share=%s\n",
         CUBALC_LANG_PARADIGM, CUBALC_CREED, CUBALC_HOLD_FLASH, CUBALC_SHARE);
  printf("root=%s\n\n", root);

  int pass = 0, fail = 0, aok = 0, afail = 0, cubes_max = 0;
  struct timespec ts0, ts1;
  clock_gettime(CLOCK_MONOTONIC, &ts0);

  puts("── Act I · Language completeness ──");
  run_one(root, "arithmetic", "programs/proof/01_arithmetic.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);
  run_one(root, "branching", "programs/proof/03_branch.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);

  puts("\n── Act II · COP programming ──");
  run_one(root, "COP matrix", "programs/proof/02_cop_matrix.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);
  run_one(root, "decide / algocube", "programs/proof/06_decide.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);
  run_one(root, "I/O reverse", "programs/proof/11_cube_io_reverse.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);
  run_one(root, "nest + compile", "programs/proof/12_nest_compile.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);

  puts("\n── Act III · Hive geometry ──");
  run_one(root, "harmony", "programs/proof/09_algocube_harmony.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);

  puts("\n── Act IV · Prophecy free-flow ──");
  run_one(root, "free_flow_prophecy", "programs/free_flow_prophecy.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);
  run_one(root, "prophecy_of_the_cube", "programs/prophecy_of_the_cube.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);

  puts("\n── Act V · Glorious Symphony ──");
  run_one(root, "glorious_symphony", "programs/showcase/glorious_symphony.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);

  puts("\n── Act VI · Binary law plane ──");
  {
    int rc = system(NULL); /* check shell available - avoid; call directly */
    (void)rc;
    /* smx-selftest via re-entry: run as child would be heavy; call cmd path */
  }
  /* inline smx using existing command function - re-exec self is cleaner */
  {
    char *argv_smx[] = { "cubalc", "smx-selftest", NULL };
    /* can't easily call main; spawn self */
    char self[512];
    ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
    if (n > 0) {
      self[n] = 0;
      char cmd[640];
      snprintf(cmd, sizeof cmd, "'%s' smx-selftest >/dev/null 2>&1", self);
      int rc = system(cmd);
      if (rc == 0) {
        pass++;
        printf("  ✓ %-28s proto=SMX2 anti_replay\n", "smx-selftest");
      } else {
        fail++;
        printf("  ✗ %-28s rc=%d\n", "smx-selftest", rc);
      }
    }
  }
  {
    char self[512];
    ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
    if (n > 0) {
      self[n] = 0;
      char cmd[640];
      snprintf(cmd, sizeof cmd, "'%s' law >/dev/null 2>&1", self);
      int rc = system(cmd);
      if (rc == 0) {
        pass++;
        printf("  ✓ %-28s 15 laws\n", "law plate");
      } else {
        fail++;
        printf("  ✗ %-28s\n", "law plate");
      }
    }
  }

  clock_gettime(CLOCK_MONOTONIC, &ts1);
  double ms = (ts1.tv_sec - ts0.tv_sec) * 1000.0 +
              (ts1.tv_nsec - ts0.tv_nsec) / 1e6;

  puts("\n════════════════════════════════════════");
  printf("  SHOWCASE SCORE  pass=%d  fail=%d\n", pass, fail);
  printf("  asserts_ok=%d  asserts_fail=%d  max_units=%d\n", aok, afail, cubes_max);
  printf("  wall_ms=%.1f  version=%s  paradigm=%s\n", ms, CUBALC_LANG_VERSION,
         CUBALC_LANG_PARADIGM);
  puts("════════════════════════════════════════");
  if (fail == 0) {
    puts("  ★ FULL SHOWCASE MANIFESTED ★");
    puts("  Showcase operational.");
  } else {
    puts("  ⚠ showcase incomplete — inspect asserts");
  }
  puts("");

  /* write plate */
  char dir[512], path[512];
  state_dir(dir, sizeof dir);
  ensure_dir(dir);
  snprintf(path, sizeof path, "%s/SHOWCASE_MANIFEST.json", dir);
  FILE *f = fopen(path, "w");
  if (f) {
    fprintf(f,
      "{\"schema\":\"cube.showcase.v1\",\"ok\":%s,\"version\":\"%s\","
      "\"paradigm\":\"%s\",\"pass\":%d,\"fail\":%d,\"asserts_ok\":%d,"
      "\"asserts_fail\":%d,\"max_n\":%d,\"wall_ms\":%.1f,"
      "\"token\":\"%s\",\"engine\":\"C\","
      "\"creed\":\"bits flow · digits judge · unity binds the hive\"}\n",
      fail == 0 ? "true" : "false", CUBALC_LANG_VERSION, CUBALC_LANG_PARADIGM,
      pass, fail, aok, afail, cubes_max, ms,
      fail == 0 ? "GLORIOUS_CUBE_SHOWCASE" : "SHOWCASE_PARTIAL");
    fclose(f);
    printf("  plate → %s\n", path);
  }

  printf("{\"ok\":%s,\"cmd\":\"showcase\",\"version\":\"%s\",\"pass\":%d,"
         "\"fail\":%d,\"asserts_ok\":%d,\"asserts_fail\":%d,\"max_n\":%d,"
         "\"wall_ms\":%.1f,\"language\":\"CubalC\"}\n",
         fail == 0 ? "true" : "false", CUBALC_LANG_VERSION, pass, fail,
         aok, afail, cubes_max, ms);
  return fail == 0 ? 0 : 1;
}
