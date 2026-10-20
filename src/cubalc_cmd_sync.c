/* CubalC CLI sync + cubechain — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_sync.h"
#include "cubalc.h"
#include "cubalc_cubechain.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

static void state_dir(char *buf, size_t n) {
  const char *e = getenv("CUBALC_STATE");
  if (e && e[0]) { snprintf(buf, n, "%s", e); return; }
  snprintf(buf, n, "state");
}

int cubalc_cmd_sync(const char *plate) {
  static const char src[] =
    "[hold]\n"
    "[creed \"C3\"]\n"
    "[genesis]\n"
    "[sync]\n"
    "[~4]\n"
    "?\n";
  cubalc_run_result rr;
  char full[2048];
  if (plate && plate[0]) {
    snprintf(full, sizeof full,
      "[hold]\n[creed \"C3\"]\n"
      "[genesis \"%s\"]\n[sync]\n[~4]\n?\n", plate);
    cubalc_run_source(full, strlen(full), "<sync>", &rr, stdout);
  } else {
    cubalc_run_source(src, sizeof src - 1, "<sync>", &rr, stdout);
  }
  printf("{\"ok\":%s,\"cmd\":\"sync\",\"n\":%d,\"unity\":%.3f,"
         "\"version\":\"%s\",\"hive\":\"WE\",\"err\":\"%s\"}\n",
         rr.ok ? "true" : "false", rr.n_cubes, rr.unity,
         CUBALC_LANG_VERSION, rr.err);
  return rr.ok ? 0 : 1;
}

int cubalc_cmd_cubechain(const char *sub) {
  char dir[512];
  state_dir(dir, sizeof dir);
  char path[560];
  snprintf(path, sizeof path, "%s/cubechain.cchn", dir);
  if (!sub || strcmp(sub, "tip") == 0) {
    cubalc_cchn cc;
    if (cubalc_cchn_open(&cc, dir) != 0) {
      puts("{\"ok\":false,\"error\":\"open\"}");
      return 1;
    }
    cubalc_cchn_tip_json(&cc, stdout);
    cubalc_cchn_close(&cc);
    return 0;
  }
  if (strcmp(sub, "verify") == 0) {
    char err[80];
    int n = cubalc_cchn_verify(path, err, sizeof err);
    if (n < 0)
      printf("{\"ok\":false,\"error\":\"%s\",\"path\":\"%s\"}\n", err, path);
    else
      printf("{\"ok\":true,\"cmd\":\"cubechain-verify\",\"blocks\":%d,\"path\":\"%s\","
             "\"matrix_is_key\":true}\n", n, path);
    return n < 0 ? 2 : 0;
  }
  puts("{\"ok\":false,\"error\":\"use tip|verify\"}");
  return 2;
}
