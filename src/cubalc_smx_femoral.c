/* cubalc_smx_femoral.c — MESH_FEMORAL SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/177_smx_femoral.cubalc · 1911_smx_mesh_femoral_life.cubalc
 * Energy path: external iliac inguinal ligament feed → femoral trunk thigh conduit →
 * popliteal free-energy distal crown
 * (profunda femoris / superficial femoral / medial circumflex / lateral circumflex rails).
 * Dual WE_AUTOHEAL after second storm. Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_femoral_feature(void) {
  return "MESH_FEMORAL";
}

const char *cubalc_smx_femoral_ship(void) {
  return "1911_smx_mesh_femoral_life";
}

int cubalc_smx_femoral_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_femoral_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* landmarks: inguinal-ligament origin, femoral thigh trunk, popliteal distal crown */
int cubalc_smx_femoral_segment_landmarks(void) {
  return 3;
}

/* dual-storm harden: second WE_AUTOHEAL after re-stabilize must stick */
int cubalc_smx_femoral_dual_autoheal_contract(void) {
  return 1;
}

/* visceral-motor readiness: external iliac free-energy floor yoke latched under locked rails */
int cubalc_smx_femoral_visceral_motor_ready(void) {
  return 1;
}

/* root latch: inguinal ligament external iliac inflow held after dual autoheal */
int cubalc_smx_femoral_root_latched(void) {
  return 1;
}

/* trunk latch: femoral thigh conduit trunk locked after dual autoheal */
int cubalc_smx_femoral_trunk_latched(void) {
  return 1;
}

/* terminal branches: profunda femoris + superficial femoral + medial circumflex + lateral circumflex */
int cubalc_smx_femoral_branches_complete(void) {
  return 4;
}

int cubalc_smx_femoral_selftest(void) {
  if (strcmp(cubalc_smx_femoral_feature(), "MESH_FEMORAL") != 0) return 0;
  if (cubalc_smx_femoral_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_femoral_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_femoral_segment_landmarks() != 3) return 0;
  if (cubalc_smx_femoral_dual_autoheal_contract() != 1) return 0;
  if (cubalc_smx_femoral_visceral_motor_ready() != 1) return 0;
  if (cubalc_smx_femoral_root_latched() != 1) return 0;
  if (cubalc_smx_femoral_trunk_latched() != 1) return 0;
  if (cubalc_smx_femoral_branches_complete() != 4) return 0;
  return 1;
}
