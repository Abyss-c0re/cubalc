/* CubalC CLI top-level -e/-I/-L → run alias — F_main_carve.
 * Returns "run" if argv[1] is a run-path alias, else cmd unchanged.
 * Zero bloat. Cube is SoT.
 */
#ifndef CUBALC_CMD_RUN_ALIAS_H
#define CUBALC_CMD_RUN_ALIAS_H
const char *cubalc_cmd_resolve_run_alias(const char *cmd);
#endif
