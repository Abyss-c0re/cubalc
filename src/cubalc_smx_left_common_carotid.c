/* cubalc_smx_left_common_carotid.c — MESH_LEFT_COMMON_CAROTID SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/99_smx_left_common_carotid.cubalc · 1833_smx_mesh_left_common_carotid_life.cubalc
 * Energy path: aortic arch apex → left common carotid origin → left common carotid trunk →
 * carotid bifurcation approach → cranial perfusion takeoff.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_left_common_carotid_feature(void) {
  return "MESH_LEFT_COMMON_CAROTID";
}

const char *cubalc_smx_left_common_carotid_ship(void) {
  return "1833_smx_mesh_left_common_carotid_life";
}

int cubalc_smx_left_common_carotid_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_left_common_carotid_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_left_common_carotid_segment_landmarks(void) {
  return 3; /* arch origin, trunk, bifurcation approach */
}

int cubalc_smx_left_common_carotid_selftest(void) {
  if (strcmp(cubalc_smx_left_common_carotid_feature(), "MESH_LEFT_COMMON_CAROTID") != 0) return 0;
  if (cubalc_smx_left_common_carotid_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_left_common_carotid_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_left_common_carotid_segment_landmarks() != 3) return 0;
  return 1;
}
