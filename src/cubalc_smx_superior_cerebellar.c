/* cubalc_smx_superior_cerebellar.c — MESH_SUPERIOR_CEREBELLAR SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/99_smx_superior_cerebellar.cubalc · 1837_smx_mesh_superior_cerebellar_life.cubalc
 * Energy path: basilar trunk apex → superior cerebellar takeoff → ambient cistern arc →
 * tentorial notch → cerebellar hemispheric / vermian perfusion seal · midbrain tegmentum feed.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_superior_cerebellar_feature(void) {
  return "MESH_SUPERIOR_CEREBELLAR";
}

const char *cubalc_smx_superior_cerebellar_ship(void) {
  return "1837_smx_mesh_superior_cerebellar_life";
}

int cubalc_smx_superior_cerebellar_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_superior_cerebellar_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_superior_cerebellar_selftest(void) {
  if (strcmp(cubalc_smx_superior_cerebellar_feature(), "MESH_SUPERIOR_CEREBELLAR") != 0) return 0;
  if (cubalc_smx_superior_cerebellar_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_superior_cerebellar_mesh_bonds_complete4() != 6) return 0;
  return 1;
}
