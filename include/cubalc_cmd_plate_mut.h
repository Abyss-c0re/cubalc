/* CubalC — CLI plate mut core get/set (B_cmd_plate_core). */
#ifndef CUBALC_CMD_PLATE_MUT_H
#define CUBALC_CMD_PLATE_MUT_H
int cubalc_cmd_plate_try_mut(const char *op, const char *path, char *plate,
                             int *file_hit, int argc, char **argv, int *ai_io);
#endif
