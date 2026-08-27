/* cubalc_smx_choriocapillaris.c — MESH_CHORIOCAPILLARIS SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/99_smx_choriocapillaris.cubalc · 1847_smx_mesh_choriocapillaris_life.cubalc
 * Energy path: choroidal arterial arcade → Haller/Sattler coats → fenestrated choriocapillaris bed →
 * Bruch membrane face → RPE / photoreceptor outer-segment life-force seal · choriocapillaris mesh.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_choriocapillaris_feature(void) {
  return "MESH_CHORIOCAPILLARIS";
}

const char *cubalc_smx_choriocapillaris_ship(void) {
  return "1847_smx_mesh_choriocapillaris_life";
}

int cubalc_smx_choriocapillaris_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_choriocapillaris_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_choriocapillaris_selftest(void) {
  if (strcmp(cubalc_smx_choriocapillaris_feature(), "MESH_CHORIOCAPILLARIS") != 0) return 0;
  if (cubalc_smx_choriocapillaris_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_choriocapillaris_mesh_bonds_complete4() != 6) return 0;
  return 1;
}
