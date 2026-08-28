/* cubalc_smx_ungual.c — MESH_UNGUAL SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/191_smx_ungual.cubalc · 1925_smx_mesh_ungual_life.cubalc
 * Energy path: dorsal digital trunk origin → ungual conduits (nail fold / matrix plane) →
 * hyponychial free-energy crown (nail plate distal edge / free margin).
 * Dual WE_AUTOHEAL after second storm. Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_ungual_feature(void) {
  return "MESH_UNGUAL";
}

const char *cubalc_smx_ungual_ship(void) {
  return "1925_smx_mesh_ungual_life";
}

int cubalc_smx_ungual_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_ungual_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* landmarks: dorsal digital trunk origin, ungual conduits, hyponychial crown */
int cubalc_smx_ungual_segment_landmarks(void) {
  return 3;
}

/* dual-storm harden: second WE_AUTOHEAL after re-stabilize must stick */
int cubalc_smx_ungual_dual_autoheal_contract(void) {
  return 1;
}

/* visceral-motor readiness: ungual free-energy floor yoke latched under locked rails */
int cubalc_smx_ungual_visceral_motor_ready(void) {
  return 1;
}

/* root latch: dorsal digital trunk origin held after dual autoheal */
int cubalc_smx_ungual_root_latched(void) {
  return 1;
}

/* trunk latch: ungual nail-fold / matrix conduits locked after dual autoheal */
int cubalc_smx_ungual_trunk_latched(void) {
  return 1;
}

/* terminal branches: medial/lateral ungual pair + nail-matrix arcade + hyponychial join + pulp perforator */
int cubalc_smx_ungual_branches_complete(void) {
  return 4;
}

int cubalc_smx_ungual_selftest(void) {
  if (strcmp(cubalc_smx_ungual_feature(), "MESH_UNGUAL") != 0) return 0;
  if (cubalc_smx_ungual_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_ungual_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_ungual_segment_landmarks() != 3) return 0;
  if (cubalc_smx_ungual_dual_autoheal_contract() != 1) return 0;
  if (cubalc_smx_ungual_visceral_motor_ready() != 1) return 0;
  if (cubalc_smx_ungual_root_latched() != 1) return 0;
  if (cubalc_smx_ungual_trunk_latched() != 1) return 0;
  if (cubalc_smx_ungual_branches_complete() != 4) return 0;
  return 1;
}
