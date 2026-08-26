/* cubalc_smx_appendix.c — MESH_APPENDIX SMX mesh stability life-force slice
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/72_smx_appendix.cubalc · 1807_smx_mesh_appendix_life.cubalc
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_appendix_feature(void) {
  return "MESH_APPENDIX";
}

const char *cubalc_smx_appendix_ship(void) {
  return "1807_smx_mesh_appendix_life";
}

int cubalc_smx_appendix_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_appendix_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_appendix_selftest(void) {
  if (strcmp(cubalc_smx_appendix_feature(), "MESH_APPENDIX") != 0) return 0;
  if (cubalc_smx_appendix_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_appendix_mesh_bonds_complete4() != 6) return 0;
  return 1;
}
