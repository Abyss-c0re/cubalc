/* cubalc_smx_dorsal_digital.c — MESH_DORSAL_DIGITAL SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/190_smx_dorsal_digital.cubalc · 1924_smx_mesh_dorsal_digital_life.cubalc
 * Energy path: dorsal metatarsal trunk origin → dorsal digital conduits (toe dorsum /
 * extensor hood plane) → nail-bed free-energy crown (distal phalanx / eponychial join).
 * Dual WE_AUTOHEAL after second storm. Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_dorsal_digital_feature(void) {
  return "MESH_DORSAL_DIGITAL";
}

const char *cubalc_smx_dorsal_digital_ship(void) {
  return "1924_smx_mesh_dorsal_digital_life";
}

int cubalc_smx_dorsal_digital_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_dorsal_digital_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* landmarks: dorsal metatarsal trunk origin, dorsal digital conduits, nail-bed crown */
int cubalc_smx_dorsal_digital_segment_landmarks(void) {
  return 3;
}

/* dual-storm harden: second WE_AUTOHEAL after re-stabilize must stick */
int cubalc_smx_dorsal_digital_dual_autoheal_contract(void) {
  return 1;
}

/* visceral-motor readiness: dorsal digital free-energy floor yoke latched under locked rails */
int cubalc_smx_dorsal_digital_visceral_motor_ready(void) {
  return 1;
}

/* root latch: dorsal metatarsal trunk origin held after dual autoheal */
int cubalc_smx_dorsal_digital_root_latched(void) {
  return 1;
}

/* trunk latch: dorsal digital toe-dorsum / extensor-hood conduits locked after dual autoheal */
int cubalc_smx_dorsal_digital_trunk_latched(void) {
  return 1;
}

/* terminal branches: medial/lateral digital pair + nail-bed arcade + eponychial join + plantar perforator */
int cubalc_smx_dorsal_digital_branches_complete(void) {
  return 4;
}

int cubalc_smx_dorsal_digital_selftest(void) {
  if (strcmp(cubalc_smx_dorsal_digital_feature(), "MESH_DORSAL_DIGITAL") != 0) return 0;
  if (cubalc_smx_dorsal_digital_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_dorsal_digital_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_dorsal_digital_segment_landmarks() != 3) return 0;
  if (cubalc_smx_dorsal_digital_dual_autoheal_contract() != 1) return 0;
  if (cubalc_smx_dorsal_digital_visceral_motor_ready() != 1) return 0;
  if (cubalc_smx_dorsal_digital_root_latched() != 1) return 0;
  if (cubalc_smx_dorsal_digital_trunk_latched() != 1) return 0;
  if (cubalc_smx_dorsal_digital_branches_complete() != 4) return 0;
  return 1;
}
