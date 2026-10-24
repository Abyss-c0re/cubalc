/* CubalC CLI top-level -e|--expr|-I|-L|-R → run —
 * F_main_carve peel from cubalc_main.c. Zero bloat. Cube is SoT.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_run_alias.h"
#include <string.h>

const char *cubalc_cmd_resolve_run_alias(const char *cmd) {
  if (!cmd || !cmd[0]) return cmd;
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
    return "run";
  return cmd;
}
