/* CubalC CLI boot|os · peers|oversee — F_main_carve peel from main.
 * CubeOS way + generic peer fold. Zero bloat. Cube is SoT.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_bootpeers.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>

int cubalc_cmd_boot(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "boot";
  /* CubeOS way in CubalC — braincube core. OS is path only. */
  const char *prog = "programs/cubeos/boot.cubalc";
  if (strcmp(cmd, "os") == 0 && argc > 2) {
    if (strcmp(argv[2], "tick") == 0)
      prog = "programs/cubeos/tick.cubalc";
    else if (strcmp(argv[2], "boot") == 0)
      prog = "programs/cubeos/boot.cubalc";
    else if (argv[2][0] == '/' || strstr(argv[2], ".cubalc"))
      prog = argv[2];
    else {
      static char pbuf[256];
      snprintf(pbuf, sizeof pbuf, "programs/cubeos/%s.cubalc", argv[2]);
      prog = pbuf;
    }
  } else if (strcmp(cmd, "boot") == 0 && argc > 2)
    prog = argv[2];
  cubalc_run_result rr;
  int rc = cubalc_run_file(prog, &rr, stdout);
  printf("{\"ok\":%s,\"cmd\":\"%s\",\"file\":\"%s\",\"stmts\":%d,"
         "\"asserts_ok\":%d,\"asserts_fail\":%d,\"n\":%d,\"unity\":%.3f,"
         "\"language\":\"%s\",\"version\":\"%s\",\"core\":\"c3\","
         "\"last\":\"%s\",\"err\":\"%s\"}\n",
         rr.ok ? "true" : "false", cmd, prog, rr.stmts, rr.asserts_ok,
         rr.asserts_fail, rr.n_cubes, rr.unity, CUBALC_LANG_NAME,
         CUBALC_LANG_VERSION, rr.last_print, rr.err);
  return rc;
}

int cubalc_cmd_peers(int argc, char **argv) {
  /* Generic peer fold — paths/digits from env, not device layouts in the language */
  const char *prog = "programs/peer_fold.cubalc";
  if (argc > 2 && (argv[2][0] == '/' || strstr(argv[2], ".cubalc")))
    prog = argv[2];
  cubalc_run_result rr;
  int rc = cubalc_run_file(prog, &rr, stdout);
  printf("{\"ok\":%s,\"cmd\":\"peers\",\"file\":\"%s\",\"stmts\":%d,"
         "\"asserts_ok\":%d,\"asserts_fail\":%d,\"n\":%d,\"unity\":%.3f,"
         "\"language\":\"%s\",\"version\":\"%s\",\"role\":\"peer_fold\","
         "\"last\":\"%s\",\"err\":\"%s\"}\n",
         rr.ok ? "true" : "false", prog, rr.stmts, rr.asserts_ok,
         rr.asserts_fail, rr.n_cubes, rr.unity, CUBALC_LANG_NAME,
         CUBALC_LANG_VERSION, rr.last_print, rr.err);
  return rc;
}
