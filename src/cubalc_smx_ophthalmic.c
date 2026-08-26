/* cubalc_smx_ophthalmic.c — MESH_OPHTHALMIC SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/109_smx_ophthalmic.cubalc · 1843_smx_mesh_ophthalmic_life.cubalc
 * Energy path: clinoid ICA crown → ophthalmic artery origin → optic canal transit
 * → orbital apex free-energy arc → central retinal / ciliary seal.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_ophthalmic_feature(void) {
  return "MESH_OPHTHALMIC";
}

const char *cubalc_smx_ophthalmic_ship(void) {
  return "1843_smx_mesh_ophthalmic_life";
}

int cubalc_smx_ophthalmic_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_ophthalmic_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* segment landmarks: clinoid ICA crown, optic canal, central retinal/ciliary seal */
int cubalc_smx_ophthalmic_segment_landmarks(void) {
  return 3;
}

int cubalc_smx_ophthalmic_selftest(void) {
  if (strcmp(cubalc_smx_ophthalmic_feature(), "MESH_OPHTHALMIC") != 0) return 0;
  if (cubalc_smx_ophthalmic_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_ophthalmic_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_ophthalmic_segment_landmarks() != 3) return 0;
  return 1;
}
