/* CubalC CLI argmap — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_argmap.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cubalc_cmd_argmap(int argc, char **argv) {
  /* Usability: CLI dual of ARGMAP — raw argv i=value bag without .cubalc.
   *   cubalc argmap -- --verbose file.txt
   * Schema cubalc.argmap.v1 · twin of flagmap for full argv. */
  cubalc_run_result rr, rr_n;
  int ok, ai, narg = 0;
  long n = 0;
  char envn[32];
  char esc[CUBALC_HOST_STR_MAX];
  size_t e = 0, k;
  static const char src_bag[] = "ARGMAP\nPRINT LAST\nPASS\n";
  static const char src_n[] = "ARGMAP\nPRINT LAST_N\nPASS\n";
  for (ai = 2; ai < argc; ai++) {
    if (!strcmp(argv[ai], "--")) {
      ai++;
      break;
    }
  }
  if (ai < argc) {
    for (; ai < argc && narg < 32; ai++, narg++) {
      snprintf(envn, sizeof envn, "CUBALC_ARG%d", narg);
      setenv(envn, argv[ai], 1);
    }
    snprintf(envn, sizeof envn, "%d", narg);
    setenv("CUBALC_ARGC", envn, 1);
  }
  memset(&rr, 0, sizeof rr);
  (void)cubalc_run_source(src_bag, sizeof src_bag - 1, "<cli-argmap>", &rr, NULL);
  ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
  memset(&rr_n, 0, sizeof rr_n);
  (void)cubalc_run_source(src_n, sizeof src_n - 1, "<cli-argmap-n>", &rr_n, NULL);
  if (rr_n.last_print[0])
    n = strtol(rr_n.last_print, NULL, 10);
  for (k = 0; rr.last_print[k] && e + 2 < sizeof esc; k++) {
    char c = rr.last_print[k];
    if (c == '"' || c == '\\') {
      esc[e++] = '\\';
      esc[e++] = c;
    } else if (c == '\n') {
      esc[e++] = '\\';
      esc[e++] = 'n';
    } else if ((unsigned char)c < 32)
      esc[e++] = ' ';
    else
      esc[e++] = c;
  }
  esc[e] = 0;
  printf("{\"schema\":\"cubalc.argmap.v1\",\"ok\":%s,\"cmd\":\"argmap\","
         "\"n\":%ld,\"live_args\":%d,\"version\":\"%s\","
         "\"note\":\"CLI dual of ARGMAP · i=value bag for CUBALC_ARGn\","
         "\"bag\":\"%s\"}\n",
         ok ? "true" : "false", n, narg, CUBALC_LANG_VERSION, esc);
  return ok ? 0 : 1;
}
