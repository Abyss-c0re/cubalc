/* CubalC CLI master try-dispatch + main entry — F_main_carve.
 * Zero bloat. Cube is SoT.
 */
#ifndef CUBALC_CMD_DISPATCH_H
#define CUBALC_CMD_DISPATCH_H
int cubalc_cmd_try_dispatch(const char *cmd, int argc, char **argv);
int cubalc_cmd_run_main(int argc, char **argv);
#endif
