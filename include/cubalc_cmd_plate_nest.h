/* CubalC — CLI plate nest ops (B_cmd_plate_nest). */
#ifndef CUBALC_CMD_PLATE_NEST_H
#define CUBALC_CMD_PLATE_NEST_H
int cubalc_cmd_plate_try_nest(const char *op, const char *path, char *plate,
                              int *file_hit, int argc, char **argv, int *ai_io);
#endif
