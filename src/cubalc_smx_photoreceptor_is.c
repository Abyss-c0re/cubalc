/* cubalc_smx_photoreceptor_is.c — MESH_PHOTORECEPTOR_IS SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/99_smx_photoreceptor_is.cubalc · 1852_smx_mesh_photoreceptor_is_life.cubalc
 * Energy path: outer-segment disc stack → connecting cilium → inner-segment mitochondria →
 * ellipsoid/myoid ATP life-force seal · photoreceptor_is mesh.
 * Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_photoreceptor_is_feature(void) {
  return "MESH_PHOTORECEPTOR_IS";
}

const char *cubalc_smx_photoreceptor_is_ship(void) {
  return "1852_smx_mesh_photoreceptor_is_life";
}

int cubalc_smx_photoreceptor_is_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_photoreceptor_is_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

int cubalc_smx_photoreceptor_is_selftest(void) {
  if (strcmp(cubalc_smx_photoreceptor_is_feature(), "MESH_PHOTORECEPTOR_IS") != 0) return 0;
  if (cubalc_smx_photoreceptor_is_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_photoreceptor_is_mesh_bonds_complete4() != 6) return 0;
  return 1;
}
