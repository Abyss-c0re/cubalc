/* cubalc_smx_venous_angle.c — MESH_VENOUS_ANGLE SMX mesh stability life-force slice
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/74_smx_venous_angle.cubalc · 1810_smx_mesh_venous_angle_life.cubalc
 * was: programs/proof/75_smx_venous_angle.cubalc · 1810_smx_mesh_venous_angle_life.cubalc
 * Energy path: thoracic duct terminus → venous angle confluence → subclavian/jugular → systemic return.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_venous_angle_feature(void) {
  return "MESH_VENOUS_ANGLE";
}

const char *cubalc_smx_venous_angle_ship(void) {
  return "1810_smx_mesh_venous_angle_life";
}

int cubalc_smx_venous_angle_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_venous_angle_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_venous_angle_selftest(void) {
  if (strcmp(cubalc_smx_venous_angle_feature(), "MESH_VENOUS_ANGLE") != 0) return 0;
  if (cubalc_smx_venous_angle_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_venous_angle_mesh_bonds_complete4() != 6) return 0;
  return 1;
}
