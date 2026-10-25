/* CubalC — CLI plate sync fill/merge/eq (B_cmd_plate_sync). */
#ifndef CUBALC_CMD_PLATE_SYNC_H
#define CUBALC_CMD_PLATE_SYNC_H
int cubalc_cmd_plate_try_sync(const char *op, const char *path, char *plate,
                              int *file_hit, int argc, char **argv, int *ai_io,
                              int fill_strict);
#endif
