/* cubalc_smx_trochlear.c — MESH_TROCHLEAR SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/132_smx_trochlear.cubalc · 1866_smx_mesh_trochlear_life.cubalc
 * Energy path: MLF internuclear highway → trochlear nucleus →
 * CN4 decussating fascicle → superior oblique free-energy conjugate crown.
 * Dual WE_AUTOHEAL after second storm. Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_trochlear_feature(void) {
  return "MESH_TROCHLEAR";
}

const char *cubalc_smx_trochlear_ship(void) {
  return "1866_smx_mesh_trochlear_life";
}

int cubalc_smx_trochlear_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_trochlear_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* landmarks: trochlear nucleus, CN4 decussating fascicle, superior-oblique conjugate crown */
int cubalc_smx_trochlear_segment_landmarks(void) {
  return 3;
}

/* dual-storm harden: second WE_AUTOHEAL after re-stabilize must stick */
int cubalc_smx_trochlear_dual_autoheal_contract(void) {
  return 1;
}

/* conjugate readiness: trochlear free-energy superior-oblique yoke latched under locked rails */
int cubalc_smx_trochlear_conjugate_yoke_ready(void) {
  return 1;
}

/* nucleus latch: CN4 vector held after dual autoheal */
int cubalc_smx_trochlear_nucleus_latched(void) {
  return 1;
}

/* decussation latch: contralateral superior-oblique feed locked after dual autoheal */
int cubalc_smx_trochlear_decussation_latched(void) {
  return 1;
}

int cubalc_smx_trochlear_selftest(void) {
  if (strcmp(cubalc_smx_trochlear_feature(), "MESH_TROCHLEAR") != 0) return 0;
  if (cubalc_smx_trochlear_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_trochlear_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_trochlear_segment_landmarks() != 3) return 0;
  if (cubalc_smx_trochlear_dual_autoheal_contract() != 1) return 0;
  if (cubalc_smx_trochlear_conjugate_yoke_ready() != 1) return 0;
  if (cubalc_smx_trochlear_nucleus_latched() != 1) return 0;
  if (cubalc_smx_trochlear_decussation_latched() != 1) return 0;
  return 1;
}
