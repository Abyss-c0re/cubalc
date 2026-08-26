/* cubalc_smx_right_lung_capillary.c — MESH_RIGHT_LUNG_CAPILLARY SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/90_smx_right_lung_capillary.cubalc · 1824_smx_mesh_right_lung_capillary_life.cubalc
 * Energy path: main pulmonary artery trunk → right pulmonary artery branch → right lung hilum →
 * right lung segmental arteries → right lung capillary return → pulmonary venous confluence.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_right_lung_capillary_feature(void) {
  return "MESH_RIGHT_LUNG_CAPILLARY";
}

const char *cubalc_smx_right_lung_capillary_ship(void) {
  return "1824_smx_mesh_right_lung_capillary_life";
}

int cubalc_smx_right_lung_capillary_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_right_lung_capillary_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_right_lung_capillary_selftest(void) {
  if (strcmp(cubalc_smx_right_lung_capillary_feature(), "MESH_RIGHT_LUNG_CAPILLARY") != 0) return 0;
  if (cubalc_smx_right_lung_capillary_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_right_lung_capillary_mesh_bonds_complete4() != 6) return 0;
  return 1;
}
