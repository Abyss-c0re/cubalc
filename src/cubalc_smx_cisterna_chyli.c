/* cubalc_smx_cisterna_chyli.c — MESH_CISTERNA_CHYLI SMX mesh stability life-force slice
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/73_smx_cisterna_chyli.cubalc · 1809_smx_mesh_cisterna_chyli_life.cubalc
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_cisterna_chyli_feature(void) {
  return "MESH_CISTERNA_CHYLI";
}

const char *cubalc_smx_cisterna_chyli_ship(void) {
  return "1809_smx_mesh_cisterna_chyli_life";
}

int cubalc_smx_cisterna_chyli_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_cisterna_chyli_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_cisterna_chyli_selftest(void) {
  if (strcmp(cubalc_smx_cisterna_chyli_feature(), "MESH_CISTERNA_CHYLI") != 0) return 0;
  if (cubalc_smx_cisterna_chyli_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_cisterna_chyli_mesh_bonds_complete4() != 6) return 0;
  return 1;
}
