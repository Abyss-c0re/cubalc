/* CubalC CLI genesis|impulse|tick|cubes|smx*|law|science|protect|sync|boot|
 * translate|compile|run|peers|showcase|evolve — F_main_carve try-dispatch.
 * Returns exit code if handled, -1 if not this family. Zero bloat. Cube is SoT.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_corehive_dispatch.h"
#include "cubalc_cmd_genesis.h"
#include "cubalc_cmd_sync.h"
#include "cubalc_cmd_smx_selftest.h"
#include "cubalc_cmd_smx_exchange.h"
#include "cubalc_cmd_smx_bus.h"
#include "cubalc_cmd_law.h"
#include "cubalc_cmd_protect.h"
#include "cubalc_cmd_bootpeers.h"
#include "cubalc_cmd_translate.h"
#include "cubalc_cmd_compile.h"
#include "cubalc_cmd_run.h"
#include "cubalc_cmd_showcase.h"
#include "cubalc_evolve.h"
#include "cubalc_science_loop.h"
#include <string.h>
#include <stdlib.h>

int cubalc_cmd_try_corehive_dispatch(const char *cmd, int argc, char **argv) {
  if (!cmd || !cmd[0]) return -1;

  if (strcmp(cmd, "genesis") == 0)
    return cubalc_cmd_genesis(argc > 2 ? argv[2] :
      "NEXUS_COORD v1 | from=BlackCube | type=heartbeat | hold_flash=1 |");

  if (strcmp(cmd, "impulse") == 0)
    return cubalc_cmd_impulse(argc > 2 ? argv[2] : "cube-create",
                       argc > 3 ? atoi(argv[3]) : 1);

  if (strcmp(cmd, "tick") == 0 || strcmp(cmd, "flow") == 0)
    return cubalc_cmd_flow(argc > 2 ? atoi(argv[2]) : 8,
                    argc > 3 ? atoi(argv[3]) : 50);

  if (strcmp(cmd, "cubes") == 0 || strcmp(cmd, "show") == 0 ||
      strcmp(cmd, "lego") == 0)
    return cubalc_cmd_cubes();

  if (strcmp(cmd, "cubechain") == 0 || strcmp(cmd, "store") == 0)
    return cubalc_cmd_cubechain(argc > 2 ? argv[2] : "tip");

  if (strcmp(cmd, "smx-selftest") == 0 || strcmp(cmd, "smx") == 0)
    return cubalc_cmd_smx_selftest();

  if (strcmp(cmd, "smx-exchange") == 0)
    return cubalc_cmd_smx_exchange(argc, argv);

  if (strcmp(cmd, "smx-bus") == 0 || strcmp(cmd, "smx-nohttp") == 0 ||
      strcmp(cmd, "smx-net") == 0)
    return cubalc_cmd_smx_bus(argc, argv);

  if (strcmp(cmd, "law") == 0 || strcmp(cmd, "manifest") == 0)
    return cubalc_cmd_law_manifest();

  if (strcmp(cmd, "science") == 0 || strcmp(cmd, "sci") == 0 ||
      strcmp(cmd, "pure-science") == 0)
    return cubalc_cmd_science(argc, argv);

  if (strcmp(cmd, "protect") == 0 || strcmp(cmd, "core-protect") == 0 ||
      strcmp(cmd, "core-guard") == 0 || strcmp(cmd, "guard") == 0)
    return cubalc_cmd_protect(argc, argv);

  if (strcmp(cmd, "sync") == 0 || strcmp(cmd, "hive") == 0)
    return cubalc_cmd_sync(argc > 2 ? argv[2] : NULL);

  if (strcmp(cmd, "boot") == 0 || strcmp(cmd, "os") == 0)
    return cubalc_cmd_boot(argc, argv);

  if (strcmp(cmd, "translate") == 0 || strcmp(cmd, "from") == 0 ||
      strcmp(cmd, "decide") == 0)
    return cubalc_cmd_translate(argc, argv);

  if (strcmp(cmd, "compile") == 0 || strcmp(cmd, "jit") == 0 ||
      strcmp(cmd, "cflow") == 0 || strcmp(cmd, "disasm") == 0)
    return cubalc_cmd_compile(argc, argv);

  if (strcmp(cmd, "run") == 0 || strcmp(cmd, "eval") == 0)
    return cubalc_cmd_run(argc, argv);

  if (strcmp(cmd, "peers") == 0 || strcmp(cmd, "oversee") == 0)
    return cubalc_cmd_peers(argc, argv);

  if (strcmp(cmd, "showcase") == 0 || strcmp(cmd, "demo") == 0 ||
      strcmp(cmd, "symphony") == 0)
    return cubalc_cmd_showcase();

  if (strcmp(cmd, "evolve") == 0 || strcmp(cmd, "evolve-loop") == 0 ||
      strcmp(cmd, "self-evolve") == 0)
    return cubalc_cmd_evolve(argc - 1, argv + 1);

  return -1;
}
