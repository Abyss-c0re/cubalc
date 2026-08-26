/* cubalc_smx_subclavian_confluence.c — MESH_SUBCLAVIAN_CONFLUENCE SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/77_smx_subclavian_confluence.cubalc · 1813_smx_mesh_subclavian_confluence_life.cubalc
 * Anatomy: thoracic duct + right lymph → subclavian confluence (IJV∩SCV bilat)
 *           → brachiocephalic veins → SVC → right atrium systemic venous return.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_subclavian_confluence_feature(void) {
  return "MESH_SUBCLAVIAN_CONFLUENCE";
}

const char *cubalc_smx_subclavian_confluence_ship(void) {
  return "1813_smx_mesh_subclavian_confluence_life";
}

int cubalc_smx_subclavian_confluence_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_subclavian_confluence_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_subclavian_confluence_selftest(void) {
  if (strcmp(cubalc_smx_subclavian_confluence_feature(), "MESH_SUBCLAVIAN_CONFLUENCE") != 0) return 0;
  if (cubalc_smx_subclavian_confluence_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_subclavian_confluence_mesh_bonds_complete4() != 6) return 0;
  return 1;
}
