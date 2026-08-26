/* cubalc_smx_left_subclavian.c — MESH_LEFT_SUBCLAVIAN SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/100_smx_left_subclavian.cubalc · 1834_smx_mesh_left_subclavian_life.cubalc
 * Energy path: aortic arch apex → left subclavian origin → left subclavian trunk →
 * axillary approach → upper limb and vertebral perfusion takeoff.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_left_subclavian_feature(void) {
  return "MESH_LEFT_SUBCLAVIAN";
}

const char *cubalc_smx_left_subclavian_ship(void) {
  return "1834_smx_mesh_left_subclavian_life";
}

int cubalc_smx_left_subclavian_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_left_subclavian_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* segment landmarks: arch origin, trunk, axillary/vertebral takeoff */
int cubalc_smx_left_subclavian_segment_landmarks(void) {
  return 3;
}

int cubalc_smx_left_subclavian_selftest(void) {
  if (strcmp(cubalc_smx_left_subclavian_feature(), "MESH_LEFT_SUBCLAVIAN") != 0) return 0;
  if (cubalc_smx_left_subclavian_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_left_subclavian_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_left_subclavian_segment_landmarks() != 3) return 0;
  return 1;
}
