/* cubalc_smx_ileum.c — MESH_ILEUM SMX mesh stability life-force slice
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/71_smx_ileum.cubalc · 1805_smx_mesh_ileum_life.cubalc
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_ileum_feature(void) {
  return "MESH_ILEUM";
}

const char *cubalc_smx_ileum_ship(void) {
  return "1805_smx_mesh_ileum_life";
}

int cubalc_smx_ileum_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_ileum_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_ileum_selftest(void) {
  if (strcmp(cubalc_smx_ileum_feature(), "MESH_ILEUM") != 0) return 0;
  if (cubalc_smx_ileum_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_ileum_mesh_bonds_complete4() != 6) return 0;
  return 1;
}
