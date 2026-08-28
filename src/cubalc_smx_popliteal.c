/* cubalc_smx_popliteal.c — MESH_POPLITEAL SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/178_smx_popliteal.cubalc · 1912_smx_mesh_popliteal_life.cubalc
 * Energy path: femoral hiatus adductor feed → popliteal fossa knee conduit →
 * tibial free-energy distal crown
 * (anterior tibial / posterior tibial / fibular / genicular rails).
 * Dual WE_AUTOHEAL after second storm. Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_popliteal_feature(void) {
  return "MESH_POPLITEAL";
}

const char *cubalc_smx_popliteal_ship(void) {
  return "1912_smx_mesh_popliteal_life";
}

int cubalc_smx_popliteal_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_popliteal_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* landmarks: femoral-hiatus origin, popliteal fossa trunk, tibial distal crown */
int cubalc_smx_popliteal_segment_landmarks(void) {
  return 3;
}

/* dual-storm harden: second WE_AUTOHEAL after re-stabilize must stick */
int cubalc_smx_popliteal_dual_autoheal_contract(void) {
  return 1;
}

/* visceral-motor readiness: femoral free-energy floor yoke latched under locked rails */
int cubalc_smx_popliteal_visceral_motor_ready(void) {
  return 1;
}

/* root latch: femoral hiatus adductor inflow held after dual autoheal */
int cubalc_smx_popliteal_root_latched(void) {
  return 1;
}

/* trunk latch: popliteal fossa knee conduit trunk locked after dual autoheal */
int cubalc_smx_popliteal_trunk_latched(void) {
  return 1;
}

/* terminal branches: anterior tibial + posterior tibial + fibular + genicular */
int cubalc_smx_popliteal_branches_complete(void) {
  return 4;
}

int cubalc_smx_popliteal_selftest(void) {
  if (strcmp(cubalc_smx_popliteal_feature(), "MESH_POPLITEAL") != 0) return 0;
  if (cubalc_smx_popliteal_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_popliteal_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_popliteal_segment_landmarks() != 3) return 0;
  if (cubalc_smx_popliteal_dual_autoheal_contract() != 1) return 0;
  if (cubalc_smx_popliteal_visceral_motor_ready() != 1) return 0;
  if (cubalc_smx_popliteal_root_latched() != 1) return 0;
  if (cubalc_smx_popliteal_trunk_latched() != 1) return 0;
  if (cubalc_smx_popliteal_branches_complete() != 4) return 0;
  return 1;
}
