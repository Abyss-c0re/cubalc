/* CubalC CLI firstarg/lastarg — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_endarg.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cubalc_cmd_endarg(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "firstarg";
  /* Usability: CLI dual of FIRSTARG/LASTARG — end peels without .cubalc.
   *   cubalc firstarg -- --verbose file.txt
   *   cubalc lastarg -- a b c
   * Schema cubalc.endarg.v1 · which=first|last · value + hit. */
  cubalc_run_result rr;
  int ok, ai, narg = 0, want_last = 0;
  long hit = 0;
  char envn[32];
  char esc_val[CUBALC_HOST_STR_MAX];
  size_t e = 0, k;
  static const char src_first[] = "FIRSTARG\nPRINT LAST\nPASS\n";
  static const char src_last[] = "LASTARG\nPRINT LAST\nPASS\n";
  want_last = (strcmp(cmd, "lastarg") == 0 || strcmp(cmd, "tailarg") == 0 ||
               strcmp(cmd, "argvlast") == 0);
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
  if (want_last)
    (void)cubalc_run_source(src_last, sizeof src_last - 1, "<cli-lastarg>", &rr, NULL);
  else
    (void)cubalc_run_source(src_first, sizeof src_first - 1, "<cli-firstarg>", &rr, NULL);
  ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
  hit = (rr.last_print[0] != 0) ? 1 : 0;
  for (k = 0; rr.last_print[k] && e + 2 < sizeof esc_val; k++) {
    char c = rr.last_print[k];
    if (c == '"' || c == '\\') {
      esc_val[e++] = '\\';
      esc_val[e++] = c;
    } else if (c == '\n') {
      esc_val[e++] = '\\';
      esc_val[e++] = 'n';
    } else if ((unsigned char)c < 32)
      esc_val[e++] = ' ';
    else
      esc_val[e++] = c;
  }
  esc_val[e] = 0;
  printf("{\"schema\":\"cubalc.endarg.v1\",\"ok\":%s,\"cmd\":\"%s\","
         "\"which\":\"%s\",\"hit\":%s,\"value\":\"%s\",\"live_args\":%d,"
         "\"version\":\"%s\","
         "\"note\":\"CLI dual of FIRSTARG/LASTARG · raw argv end peels\"}\n",
         ok ? "true" : "false", want_last ? "lastarg" : "firstarg",
         want_last ? "last" : "first", hit ? "true" : "false", esc_val, narg,
         CUBALC_LANG_VERSION);
  return ok ? 0 : 1;
}
