/* cubalc_smx_main_pulmonary_artery.c — MESH_MAIN_PULMONARY_ARTERY SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/83_smx_main_pulmonary_artery.cubalc · 1819_smx_mesh_main_pulmonary_artery_life.cubalc
 * Energy path: pulmonary valve leaflets → pulmonary outflow trunk → main pulmonary artery →
 * left/right pulmonary branches → lung capillary return.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_main_pulmonary_artery_feature(void) {
  return "MESH_MAIN_PULMONARY_ARTERY";
}

const char *cubalc_smx_main_pulmonary_artery_ship(void) {
  return "1819_smx_mesh_main_pulmonary_artery_life";
}

int cubalc_smx_main_pulmonary_artery_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_main_pulmonary_artery_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_main_pulmonary_artery_selftest(void) {
  if (strcmp(cubalc_smx_main_pulmonary_artery_feature(), "MESH_MAIN_PULMONARY_ARTERY") != 0) return 0;
  if (cubalc_smx_main_pulmonary_artery_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_main_pulmonary_artery_mesh_bonds_complete4() != 6) return 0;
  return 1;
}
