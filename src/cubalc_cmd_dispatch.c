/* CubalC CLI master try-dispatch — F_main_carve from cubalc_main.c.
 * corehive·agentcli·discover·libgate·util. Returns exit if handled, -1 else.
 * Zero bloat. Cube is SoT.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_dispatch.h"
#include "cubalc_cmd_corehive_dispatch.h"
#include "cubalc_cmd_agentcli_dispatch.h"
#include "cubalc_cmd_discover_dispatch.h"
#include "cubalc_cmd_libgate_dispatch.h"
#include "cubalc_cmd_util_dispatch.h"

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
