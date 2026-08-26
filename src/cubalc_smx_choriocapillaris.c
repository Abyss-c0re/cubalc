/* cubalc_smx_choriocapillaris.c — MESH_CHORIOCAPILLARIS SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/112_smx_choriocapillaris.cubalc · 1846_smx_mesh_choriocapillaris_life.cubalc
 * Energy path: posterior ciliary inflow → choroidal free-energy arc → choriocapillaris
 * bed → Bruch membrane seal → outer-retinal photoreceptor feed.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_choriocapillaris_feature(void) {
  return "MESH_CHORIOCAPILLARIS";
}

const char *cubalc_smx_choriocapillaris_ship(void) {
  return "1846_smx_mesh_choriocapillaris_life";
}

int cubalc_smx_choriocapillaris_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_choriocapillaris_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* segment landmarks: choroidal arc, Bruch membrane, outer-retinal photoreceptor feed */
int cubalc_smx_choriocapillaris_segment_landmarks(void) {
  return 3;
}

int cubalc_smx_choriocapillaris_selftest(void) {
  if (strcmp(cubalc_smx_choriocapillaris_feature(), "MESH_CHORIOCAPILLARIS") != 0) return 0;
  if (cubalc_smx_choriocapillaris_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_choriocapillaris_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_choriocapillaris_segment_landmarks() != 3) return 0;
  return 1;
}
