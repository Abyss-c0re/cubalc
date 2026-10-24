/* CubalC CLI init|plate|pathver|which|search|help —
 * F_main_carve try-dispatch peel from cubalc_main.c.
 * Returns exit code if handled, -1 if not this family. Zero bloat. Cube is SoT.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_util_dispatch.h"
#include "cubalc_cmd_init.h"
#include "cubalc_cmd_plate.h"
#include "cubalc_cmd_pathver.h"
#include "cubalc_cmd_which.h"
#include "cubalc_cmd_search.h"
#include "cubalc_cmd_help.h"
#include <string.h>

int cubalc_cmd_try_util_dispatch(const char *cmd, int argc, char **argv) {
  if (!cmd || !cmd[0]) return -1;

  if (strcmp(cmd, "init") == 0 || strcmp(cmd, "new") == 0 ||
      strcmp(cmd, "scaffold") == 0 || strcmp(cmd, "create") == 0)
    return cubalc_cmd_init(argc, argv);

  if (strcmp(cmd, "plate") == 0 || strcmp(cmd, "jsonplate") == 0 ||
      strcmp(cmd, "platefile") == 0 || strcmp(cmd, "agent-plate") == 0)
    return cubalc_cmd_plate(argc, argv);

  if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0 ||
      strcmp(cmd, "-V") == 0 || strcmp(cmd, "ver") == 0 ||
      strcmp(cmd, "paths") == 0 || strcmp(cmd, "path") == 0 ||
      strcmp(cmd, "where") == 0 || strcmp(cmd, "layout") == 0)
    return cubalc_cmd_pathver(argc, argv);

  if (strcmp(cmd, "which") == 0 || strcmp(cmd, "locate") == 0 ||
      strcmp(cmd, "resolve") == 0)
    return cubalc_cmd_which(argc, argv);

  if (strcmp(cmd, "search") == 0 || strcmp(cmd, "find") == 0 ||
      strcmp(cmd, "query") == 0 || strcmp(cmd, "look") == 0)
    return cubalc_cmd_search(argc, argv);

  if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0)
    return cubalc_cmd_help(argc, argv);

  return -1;
}
