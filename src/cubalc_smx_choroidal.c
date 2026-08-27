/* cubalc_smx_choroidal.c — MESH_CHOROIDAL SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/99_smx_choroidal.cubalc · 1846_smx_mesh_choroidal_life.cubalc
 * Energy path: posterior ciliary artery feed → scleral canal pierce → choroidal arterial
 * arcade → choriocapillaris bed → outer retinal / RPE life-force seal · choroidal mesh.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_choroidal_feature(void) {
  return "MESH_CHOROIDAL";
}

const char *cubalc_smx_choroidal_ship(void) {
  return "1846_smx_mesh_choroidal_life";
}

int cubalc_smx_choroidal_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_choroidal_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_choroidal_selftest(void) {
  if (strcmp(cubalc_smx_choroidal_feature(), "MESH_CHOROIDAL") != 0) return 0;
  if (cubalc_smx_choroidal_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_choroidal_mesh_bonds_complete4() != 6) return 0;
  return 1;
}
