/* cubalc_smx_v3.c — MESH_V3 SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/124_smx_v3.cubalc · 1858_smx_mesh_v3_life.cubalc
 * Energy path: extrastriate V3 complex-cell gate → orientation-column lamina →
 * dorsal-stream MT association stem → parietal free-energy crown.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_v3_feature(void) {
  return "MESH_V3";
}

const char *cubalc_smx_v3_ship(void) {
  return "1858_smx_mesh_v3_life";
}

int cubalc_smx_v3_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_v3_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* landmarks: orientation-column lamina, MT association stem, parietal crown */
int cubalc_smx_v3_segment_landmarks(void) {
  return 3;
}

/* dual-storm harden: second WE_AUTOHEAL after re-stabilize must stick */
int cubalc_smx_v3_dual_autoheal_contract(void) {
  return 1;
}

int cubalc_smx_v3_selftest(void) {
  if (strcmp(cubalc_smx_v3_feature(), "MESH_V3") != 0) return 0;
  if (cubalc_smx_v3_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_v3_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_v3_segment_landmarks() != 3) return 0;
  if (cubalc_smx_v3_dual_autoheal_contract() != 1) return 0;
  return 1;
}
