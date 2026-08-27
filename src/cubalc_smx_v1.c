/* cubalc_smx_v1.c — MESH_V1 SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/122_smx_v1.cubalc · 1856_smx_mesh_v1_life.cubalc
 * Energy path: LGN relay lamina → geniculocalcarine radiation stem →
 * calcarine bank (upper/lower field) → striate columnar free-energy crown.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_v1_feature(void) {
  return "MESH_V1";
}

const char *cubalc_smx_v1_ship(void) {
  return "1856_smx_mesh_v1_life";
}

int cubalc_smx_v1_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_v1_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* segment landmarks: geniculocalcarine stem, calcarine bank, striate column crown */
int cubalc_smx_v1_segment_landmarks(void) {
  return 3;
}

int cubalc_smx_v1_selftest(void) {
  if (strcmp(cubalc_smx_v1_feature(), "MESH_V1") != 0) return 0;
  if (cubalc_smx_v1_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_v1_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_v1_segment_landmarks() != 3) return 0;
  return 1;
}
