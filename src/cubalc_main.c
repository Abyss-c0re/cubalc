#define _POSIX_C_SOURCE 200809L
#include "cubalc.h"
#include "cubalc_smx.h"
#include "cubalc_cubechain.h"
#include "cubalc_lang.h"
#include "cubalc_cmd_corehive_dispatch.h"
#include "cubalc_cmd_agentcli_dispatch.h"
#include "cubalc_cmd_discover_dispatch.h"
#include "cubalc_cmd_libgate_dispatch.h"
#include "cubalc_cmd_util_dispatch.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* CubalC machine — CBLC · smx · cubechain · hold=1 */

/* state_dir/paths/publish/store_chain/load_genesis → cubalc_cmd_genesis (already peeled)
 * lib_count_recipe_meta → cubalc_cmd_contracts (already peeled) */

int main(int argc, char **argv) {
  const char *cmd = argc > 1 ? argv[1] : "genesis";
  /* Usability: cubalc -e CODE ≡ cubalc run -e CODE (no subcommand required).
   * Also -I/--include/-L top-level → run so agents skip the subcommand. */
  if (strcmp(cmd, "-e") == 0 || strcmp(cmd, "--expr") == 0 ||
      strcmp(cmd, "--code") == 0 || strcmp(cmd, "-c") == 0 ||
      strcmp(cmd, "-I") == 0 || strcmp(cmd, "--include") == 0 ||
      strcmp(cmd, "--preload") == 0 ||
      strncmp(cmd, "--include=", 10) == 0 ||
      strncmp(cmd, "--preload=", 10) == 0 ||
      strcmp(cmd, "-L") == 0 || strcmp(cmd, "--include-path") == 0 ||
      strcmp(cmd, "--lib-path") == 0 ||
      strncmp(cmd, "--include-path=", 15) == 0 ||
      strncmp(cmd, "--lib-path=", 11) == 0 ||
      strcmp(cmd, "-R") == 0 || strcmp(cmd, "--require-version") == 0 ||
      strncmp(cmd, "--require-version=", 18) == 0)
    cmd = "run";
  /* genesis…evolve core-hive → cubalc_cmd_try_corehive_dispatch (F_main_carve) */
  {
    int crc = cubalc_cmd_try_corehive_dispatch(cmd, argc, argv);
    if (crc >= 0) return crc;
  }
  /* cliinfo…runsnip agent/cli → cubalc_cmd_try_agentcli_dispatch (F_main_carve) */
  {
    int arc = cubalc_cmd_try_agentcli_dispatch(cmd, argc, argv);
    if (arc >= 0) return arc;
  }
  /* discover·open·topics·matchtopics·topichint·guide·err* → cubalc_cmd_try_discover_dispatch (F_main_carve) */
  {
    int drc = cubalc_cmd_try_discover_dispatch(argc, argv);
    if (drc >= 0) return drc;
  }
  /* libs·recipe·flag·env → cubalc_cmd_try_libgate_dispatch (F_main_carve) */
  {
    int lrc = cubalc_cmd_try_libgate_dispatch(argc, argv);
    if (lrc >= 0) return lrc;
  }
  /* init·plate·pathver·which·search·help → cubalc_cmd_try_util_dispatch (F_main_carve) */
  {
    int urc = cubalc_cmd_try_util_dispatch(cmd, argc, argv);
    if (urc >= 0) return urc;
  }
  fprintf(stderr, "unknown cmd %s — try help\n", cmd);
  return 2;
}
