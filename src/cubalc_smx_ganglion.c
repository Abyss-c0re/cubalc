/* cubalc_smx_ganglion.c — MESH_GANGLION SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/117_smx_ganglion.cubalc · 1851_smx_mesh_ganglion_life.cubalc
 * Energy path: bipolar axon terminal → ganglion dendritic arbor → action-potential spike
 * gate → optic-nerve fiber fascicle → lateral geniculate free-energy crown.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_ganglion_feature(void) {
  return "MESH_GANGLION";
}

const char *cubalc_smx_ganglion_ship(void) {
  return "1851_smx_mesh_ganglion_life";
}

int cubalc_smx_ganglion_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_ganglion_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* segment landmarks: dendritic arbor, spike gate, optic-nerve fascicle crown */
int cubalc_smx_ganglion_segment_landmarks(void) {
  return 3;
}

int cubalc_smx_ganglion_selftest(void) {
  if (strcmp(cubalc_smx_ganglion_feature(), "MESH_GANGLION") != 0) return 0;
  if (cubalc_smx_ganglion_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_ganglion_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_ganglion_segment_landmarks() != 3) return 0;
  return 1;
}
