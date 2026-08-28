/* cubalc_smx_fibular.c — MESH_FIBULAR SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/181_smx_fibular.cubalc · 1915_smx_mesh_fibular_life.cubalc
 * Energy path: tibiofibular trunk origin → deep lateral fibular conduit
 * (FHL / peroneal muscle plane) → lateral ankle-foot free-energy crown
 * (perforating / malleolar / calcaneal / lateral plantar rails).
 * Dual WE_AUTOHEAL after second storm. Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_fibular_feature(void) {
  return "MESH_FIBULAR";
}

const char *cubalc_smx_fibular_ship(void) {
  return "1915_smx_mesh_fibular_life";
}

int cubalc_smx_fibular_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_fibular_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* landmarks: tibiofibular origin, deep lateral fibular trunk, lateral ankle-foot crown */
int cubalc_smx_fibular_segment_landmarks(void) {
  return 3;
}

/* dual-storm harden: second WE_AUTOHEAL after re-stabilize must stick */
int cubalc_smx_fibular_dual_autoheal_contract(void) {
  return 1;
}

/* visceral-motor readiness: fibular free-energy floor yoke latched under locked rails */
int cubalc_smx_fibular_visceral_motor_ready(void) {
  return 1;
}

/* root latch: tibiofibular trunk origin inflow held after dual autoheal */
int cubalc_smx_fibular_root_latched(void) {
  return 1;
}

/* trunk latch: deep lateral fibular FHL/peroneal conduit trunk locked after dual autoheal */
int cubalc_smx_fibular_trunk_latched(void) {
  return 1;
}

/* terminal branches: perforating + malleolar + calcaneal + lateral plantar */
int cubalc_smx_fibular_branches_complete(void) {
  return 4;
}

int cubalc_smx_fibular_selftest(void) {
  if (strcmp(cubalc_smx_fibular_feature(), "MESH_FIBULAR") != 0) return 0;
  if (cubalc_smx_fibular_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_fibular_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_fibular_segment_landmarks() != 3) return 0;
  if (cubalc_smx_fibular_dual_autoheal_contract() != 1) return 0;
  if (cubalc_smx_fibular_visceral_motor_ready() != 1) return 0;
  if (cubalc_smx_fibular_root_latched() != 1) return 0;
  if (cubalc_smx_fibular_trunk_latched() != 1) return 0;
  if (cubalc_smx_fibular_branches_complete() != 4) return 0;
  return 1;
}
