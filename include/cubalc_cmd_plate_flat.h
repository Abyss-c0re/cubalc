/* CubalC — CLI plate flat/leaf ops (B_cmd_plate_peel). */
#ifndef CUBALC_CMD_PLATE_FLAT_H
#define CUBALC_CMD_PLATE_FLAT_H
int cubalc_cmd_plate_try_flat(const char *op, const char *path, char *plate,
                              int *file_hit, int argc, char **argv, int *ai_io);
#endif
