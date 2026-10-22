/* CubalC CLI cliinfo — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_cliinfo.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cubalc_cmd_cliinfo(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "cliinfo";
  (void)cmd;
  /* Usability: full CLI surface plate (dual of CLIINFO).
   * Forwards optional trailing args after -- as CUBALC_ARGn for live inspect.
   *   cubalc cliinfo -- --verbose --out x a.txt
   * Schema cubalc.cli.v1 · agents skip LISTFLAGS+FLAGMAP+RESTARGS glue. */
  cubalc_run_result rr;
  int ok, ai, narg = 0;
  char envn[32];
  static const char src[] = "CLIINFO\nPASS\n";
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
  (void)cubalc_run_source(src, sizeof src - 1, "<cli-cliinfo>", &rr, NULL);
  ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
  if (rr.last_print[0] && strstr(rr.last_print, "cubalc.cli.v1")) {
    printf("%s\n", rr.last_print);
  } else if (rr.last_print[0]) {
    printf("%s\n", rr.last_print);
  } else {
    printf("{\"schema\":\"cubalc.cli.v1\",\"ok\":%s,\"cmd\":\"cliinfo\","
           "\"version\":\"%s\",\"note\":\"CLI surface · dual of CLIINFO\","
           "\"err\":\"%s\"}\n",
           ok ? "true" : "false", CUBALC_LANG_VERSION,
           ok ? "" : (rr.err[0] ? rr.err : "cliinfo fail"));
  }
  return ok ? 0 : 1;
}
