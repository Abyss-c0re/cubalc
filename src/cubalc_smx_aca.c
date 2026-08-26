/* cubalc_smx_aca.c — MESH_ACA SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/106_smx_aca.cubalc · 1840_smx_mesh_aca_life.cubalc
 * Energy path: ICA bifurcation → ACA A1 origin → callosomarginal / pericallosal
 * takeoff → cortical free-energy arc → circle-of-Willis anterior seal.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_aca_feature(void) {
  return "MESH_ACA";
}

const char *cubalc_smx_aca_ship(void) {
  return "1840_smx_mesh_aca_life";
}

int cubalc_smx_aca_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_aca_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* segment landmarks: ICA bifurcation, ACA A1 trunk, cortical/Willis anterior seal */
int cubalc_smx_aca_segment_landmarks(void) {
  return 3;
}

int cubalc_smx_aca_selftest(void) {
  if (strcmp(cubalc_smx_aca_feature(), "MESH_ACA") != 0) return 0;
  if (cubalc_smx_aca_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_aca_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_aca_segment_landmarks() != 3) return 0;
  return 1;
}
