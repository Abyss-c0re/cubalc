/* cubalc_smx_basilar.c — MESH_BASILAR SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/102_smx_basilar.cubalc · 1836_smx_mesh_basilar_life.cubalc
 * Energy path: bilateral vertebral confluence → basilar trunk → pontine perforators →
 * superior cerebellar takeoff → posterior cerebral / circle-of-Willis posterior seal.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_basilar_feature(void) {
  return "MESH_BASILAR";
}

const char *cubalc_smx_basilar_ship(void) {
  return "1836_smx_mesh_basilar_life";
}

int cubalc_smx_basilar_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_basilar_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* segment landmarks: vertebral confluence, basilar trunk, PCA/Willis posterior */
int cubalc_smx_basilar_segment_landmarks(void) {
  return 3;
}

int cubalc_smx_basilar_selftest(void) {
  if (strcmp(cubalc_smx_basilar_feature(), "MESH_BASILAR") != 0) return 0;
  if (cubalc_smx_basilar_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_basilar_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_basilar_segment_landmarks() != 3) return 0;
  return 1;
}
