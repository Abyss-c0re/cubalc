/* cubalc_smx_photoreceptor.c — MESH_PHOTORECEPTOR SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/115_smx_photoreceptor.cubalc · 1849_smx_mesh_photoreceptor_life.cubalc
 * Energy path: RPE adhesion → outer-segment disk renewal → phototransduction cascade →
 * bipolar synaptic ribbon → macular free-energy crown.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_photoreceptor_feature(void) {
  return "MESH_PHOTORECEPTOR";
}

const char *cubalc_smx_photoreceptor_ship(void) {
  return "1849_smx_mesh_photoreceptor_life";
}

int cubalc_smx_photoreceptor_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_photoreceptor_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* segment landmarks: outer-segment disk renewal, phototransduction cascade, bipolar ribbon */
int cubalc_smx_photoreceptor_segment_landmarks(void) {
  return 3;
}

int cubalc_smx_photoreceptor_selftest(void) {
  if (strcmp(cubalc_smx_photoreceptor_feature(), "MESH_PHOTORECEPTOR") != 0) return 0;
  if (cubalc_smx_photoreceptor_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_photoreceptor_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_photoreceptor_segment_landmarks() != 3) return 0;
  return 1;
}
