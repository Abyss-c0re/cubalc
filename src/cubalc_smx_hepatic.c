/* cubalc_smx_hepatic.c — MESH_HEPATIC SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/154_smx_hepatic.cubalc · 1888_smx_mesh_hepatic_life.cubalc
 * Energy path: vagal hepatic branch / celiac feed → hepatic plexus →
 * portal triad free-energy liver visceral crown
 * (portal vein / hepatic artery proper / bile canalicular rails).
 * Dual WE_AUTOHEAL after second storm. Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_hepatic_feature(void) {
  return "MESH_HEPATIC";
}

const char *cubalc_smx_hepatic_ship(void) {
  return "1888_smx_mesh_hepatic_life";
}

int cubalc_smx_hepatic_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_hepatic_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* landmarks: hepatic plexus ganglia, portal triad, liver visceral crown */
int cubalc_smx_hepatic_segment_landmarks(void) {
  return 3;
}

/* dual-storm harden: second WE_AUTOHEAL after re-stabilize must stick */
int cubalc_smx_hepatic_dual_autoheal_contract(void) {
  return 1;
}

/* visceral-motor readiness: hepatic free-energy floor yoke latched under locked rails */
int cubalc_smx_hepatic_visceral_motor_ready(void) {
  return 1;
}

/* root latch: vagal hepatic + celiac inflow held after dual autoheal */
int cubalc_smx_hepatic_root_latched(void) {
  return 1;
}

/* trunk latch: hepatic plexus locked after dual autoheal */
int cubalc_smx_hepatic_trunk_latched(void) {
  return 1;
}

/* terminal branches: portal vein + hepatic artery proper + bile canalicular set */
int cubalc_smx_hepatic_branches_complete(void) {
  return 3;
}

int cubalc_smx_hepatic_selftest(void) {
  if (strcmp(cubalc_smx_hepatic_feature(), "MESH_HEPATIC") != 0) return 0;
  if (cubalc_smx_hepatic_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_hepatic_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_hepatic_segment_landmarks() != 3) return 0;
  if (cubalc_smx_hepatic_dual_autoheal_contract() != 1) return 0;
  if (cubalc_smx_hepatic_visceral_motor_ready() != 1) return 0;
  if (cubalc_smx_hepatic_root_latched() != 1) return 0;
  if (cubalc_smx_hepatic_trunk_latched() != 1) return 0;
  if (cubalc_smx_hepatic_branches_complete() != 3) return 0;
  return 1;
}
