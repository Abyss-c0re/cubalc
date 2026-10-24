/* CubalC CLI agent/cli surface — F_main_carve try-dispatch.
 * cmd is main-resolved. Returns exit code if handled, -1 if not. Zero bloat. Cube is SoT.
 */
#ifndef CUBALC_CMD_AGENTCLI_DISPATCH_H
#define CUBALC_CMD_AGENTCLI_DISPATCH_H
int cubalc_cmd_try_agentcli_dispatch(const char *cmd, int argc, char **argv);
#endif
