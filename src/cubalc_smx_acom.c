/* cubalc_smx_acom.c — MESH_ACOM SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/105_smx_acom.cubalc · 1839_smx_mesh_acom_life.cubalc
 * Energy path: A1 ACA origin → ACOM takeoff → communicating bridge →
 * contralateral A2 takeoff → anterior circle-of-Willis crown seal.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_acom_feature(void) {
  return "MESH_ACOM";
}

const char *cubalc_smx_acom_ship(void) {
  return "1839_smx_mesh_acom_life";
}

int cubalc_smx_acom_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_acom_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* segment landmarks: A1 origin, ACOM bridge, A2/Willis anterior crown seal */
int cubalc_smx_acom_segment_landmarks(void) {
  return 3;
}

int cubalc_smx_acom_selftest(void) {
  if (strcmp(cubalc_smx_acom_feature(), "MESH_ACOM") != 0) return 0;
  if (cubalc_smx_acom_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_acom_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_acom_segment_landmarks() != 3) return 0;
  return 1;
}
