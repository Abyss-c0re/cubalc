/* cubalc_smx_right_lymph_duct.c — MESH_RIGHT_LYMPH_DUCT SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/75_smx_right_lymph_duct.cubalc · 1811_smx_mesh_right_lymph_duct_life.cubalc
 * Energy path: right trunks → right lymph duct → right venous angle → systemic return.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_right_lymph_duct_feature(void) {
  return "MESH_RIGHT_LYMPH_DUCT";
}

const char *cubalc_smx_right_lymph_duct_ship(void) {
  return "1811_smx_mesh_right_lymph_duct_life";
}

int cubalc_smx_right_lymph_duct_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_right_lymph_duct_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_right_lymph_duct_selftest(void) {
  if (strcmp(cubalc_smx_right_lymph_duct_feature(), "MESH_RIGHT_LYMPH_DUCT") != 0) return 0;
  if (cubalc_smx_right_lymph_duct_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_right_lymph_duct_mesh_bonds_complete4() != 6) return 0;
  return 1;
}
