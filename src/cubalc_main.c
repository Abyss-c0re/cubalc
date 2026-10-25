#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_dispatch.h"

/* CubalC machine — CBLC · smx · cubechain · hold=1
 * main entry → cubalc_cmd_run_main (F_main_carve). Cube is SoT. */

int main(int argc, char **argv) {
  return cubalc_cmd_run_main(argc, argv);
}
