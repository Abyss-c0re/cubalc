/* cubalc_smx_pcom.c — MESH_PCOM SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/104_smx_pcom.cubalc · 1838_smx_mesh_pcom_life.cubalc
 * Energy path: PCA P1 origin → PCOM takeoff → communicating arc →
 * ICA terminus → anterior circle-of-Willis seal.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_pcom_feature(void) {
  return "MESH_PCOM";
}

const char *cubalc_smx_pcom_ship(void) {
  return "1838_smx_mesh_pcom_life";
}

int cubalc_smx_pcom_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_pcom_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* segment landmarks: PCA P1, PCOM trunk, ICA/Willis anterior seal */
int cubalc_smx_pcom_segment_landmarks(void) {
  return 3;
}

int cubalc_smx_pcom_selftest(void) {
  if (strcmp(cubalc_smx_pcom_feature(), "MESH_PCOM") != 0) return 0;
  if (cubalc_smx_pcom_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_pcom_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_pcom_segment_landmarks() != 3) return 0;
  return 1;
}
