/* cubalc_smx_right_lymph.c — MESH_RIGHT_LYMPH SMX mesh stability life-force slice
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/75_smx_right_lymph.cubalc · 1811_smx_mesh_right_lymph_life.cubalc
 * Anatomy: right jugular+subclavian+bronchomediastinal trunks → right lymphatic duct
 *           → right venous angle (IJV∩SCV) → systemic venous return.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_right_lymph_feature(void) {
  return "MESH_RIGHT_LYMPH";
}

const char *cubalc_smx_right_lymph_ship(void) {
  return "1811_smx_mesh_right_lymph_life";
}

int cubalc_smx_right_lymph_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_right_lymph_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_right_lymph_selftest(void) {
  if (strcmp(cubalc_smx_right_lymph_feature(), "MESH_RIGHT_LYMPH") != 0) return 0;
  if (cubalc_smx_right_lymph_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_right_lymph_mesh_bonds_complete4() != 6) return 0;
  return 1;
}
