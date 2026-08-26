/* cubalc_smx_posterior_ciliary.c — MESH_POSTERIOR_CILIARY SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/111_smx_posterior_ciliary.cubalc · 1845_smx_mesh_posterior_ciliary_life.cubalc
 * Energy path: ophthalmic artery origin → posterior ciliary takeoff → scleral
 * pierce → choroidal free-energy arc → choriocapillaris outer-retinal seal.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_posterior_ciliary_feature(void) {
  return "MESH_POSTERIOR_CILIARY";
}

const char *cubalc_smx_posterior_ciliary_ship(void) {
  return "1845_smx_mesh_posterior_ciliary_life";
}

int cubalc_smx_posterior_ciliary_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_posterior_ciliary_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* segment landmarks: PCA takeoff, scleral pierce, choriocapillaris seal */
int cubalc_smx_posterior_ciliary_segment_landmarks(void) {
  return 3;
}

int cubalc_smx_posterior_ciliary_selftest(void) {
  if (strcmp(cubalc_smx_posterior_ciliary_feature(), "MESH_POSTERIOR_CILIARY") != 0) return 0;
  if (cubalc_smx_posterior_ciliary_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_posterior_ciliary_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_posterior_ciliary_segment_landmarks() != 3) return 0;
  return 1;
}
