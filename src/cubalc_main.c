#define _POSIX_C_SOURCE 200809L
#include "cubalc.h"
#include "cubalc_smx.h"
#include "cubalc_cubechain.h"
#include "cubalc_lang.h"
#include "cubalc_cmd_run_alias.h"
#include "cubalc_cmd_dispatch.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* CubalC machine — CBLC · smx · cubechain · hold=1 */

/* state_dir/paths/publish/store_chain/load_genesis → cubalc_cmd_genesis (already peeled)
 * lib_count_recipe_meta → cubalc_cmd_contracts (already peeled) */

int main(int argc, char **argv) {
  const char *cmd = argc > 1 ? argv[1] : "genesis";
  /* -e/-I/-L/-R top-level → run (F_main_carve run_alias) */
  cmd = cubalc_cmd_resolve_run_alias(cmd);
  /* corehive·agentcli·discover·libgate·util → try_dispatch (F_main_carve) */
  {
    int rc = cubalc_cmd_try_dispatch(cmd, argc, argv);
    if (rc >= 0) return rc;
  }
  fprintf(stderr, "unknown cmd %s — try help\n", cmd);
  return 2;
}
