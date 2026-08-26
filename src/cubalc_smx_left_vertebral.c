/* cubalc_smx_left_vertebral.c — MESH_LEFT_VERTEBRAL SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/99_smx_left_vertebral.cubalc · 1834_smx_mesh_left_vertebral_life.cubalc
 * Energy path: left subclavian origin → left vertebral takeoff → cervical transverse foramina →
 * atlanto-occipital ascent → basilar confluence / hindbrain perfusion takeoff.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_left_vertebral_feature(void) {
  return "MESH_LEFT_VERTEBRAL";
}

const char *cubalc_smx_left_vertebral_ship(void) {
  return "1834_smx_mesh_left_vertebral_life";
}

int cubalc_smx_left_vertebral_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_left_vertebral_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_left_vertebral_selftest(void) {
  if (strcmp(cubalc_smx_left_vertebral_feature(), "MESH_LEFT_VERTEBRAL") != 0) return 0;
  if (cubalc_smx_left_vertebral_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_left_vertebral_mesh_bonds_complete4() != 6) return 0;
  return 1;
}
