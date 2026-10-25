/* CubalC CLI master try-dispatch + main entry — F_main_carve.
 * corehive·agentcli·discover·libgate·util + run_alias + unknown.
 * Zero bloat. Cube is SoT.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_dispatch.h"
#include "cubalc_cmd_run_alias.h"
#include "cubalc_cmd_corehive_dispatch.h"
#include "cubalc_cmd_agentcli_dispatch.h"
#include "cubalc_cmd_discover_dispatch.h"
#include "cubalc_cmd_libgate_dispatch.h"
#include "cubalc_cmd_util_dispatch.h"
#include <stdio.h>

int cubalc_cmd_try_dispatch(const char *cmd, int argc, char **argv) {
  {
    int r = cubalc_cmd_try_corehive_dispatch(cmd, argc, argv);
    if (r >= 0) return r;
  }
  {
    int r = cubalc_cmd_try_agentcli_dispatch(cmd, argc, argv);
    if (r >= 0) return r;
  }
  {
    int r = cubalc_cmd_try_discover_dispatch(argc, argv);
    if (r >= 0) return r;
  }
  {
    int r = cubalc_cmd_try_libgate_dispatch(argc, argv);
    if (r >= 0) return r;
  }
  {
    int r = cubalc_cmd_try_util_dispatch(cmd, argc, argv);
    if (r >= 0) return r;
  }
  return -1;
}

/* main entry: resolve -e/-I/-L/-R alias → try_dispatch → unknown */
int cubalc_cmd_run_main(int argc, char **argv) {
  const char *cmd = argc > 1 ? argv[1] : "genesis";
  cmd = cubalc_cmd_resolve_run_alias(cmd);
  {
    int rc = cubalc_cmd_try_dispatch(cmd, argc, argv);
    if (rc >= 0) return rc;
  }
  fprintf(stderr, "unknown cmd %s — try help\n", cmd ? cmd : "");
  return 2;
}
