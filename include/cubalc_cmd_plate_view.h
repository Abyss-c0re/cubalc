/* CubalC — CLI plate view show/keys/pick/sum (B_cmd_plate_view). */
#ifndef CUBALC_CMD_PLATE_VIEW_H
#define CUBALC_CMD_PLATE_VIEW_H
int cubalc_cmd_plate_try_view(const char *op, const char *path, char *plate,
                              int *file_hit, int argc, char **argv, int *ai_io);
#endif
