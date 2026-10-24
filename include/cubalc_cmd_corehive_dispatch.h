/* CubalC CLI core-hive family — F_main_carve try-dispatch.
 * cmd is main-resolved subcommand (handles -e → run). Returns exit or -1.
 * Zero bloat. Cube is SoT.
 */
#ifndef CUBALC_CMD_COREHIVE_DISPATCH_H
#define CUBALC_CMD_COREHIVE_DISPATCH_H
int cubalc_cmd_try_corehive_dispatch(const char *cmd, int argc, char **argv);
#endif
