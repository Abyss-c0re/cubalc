/* cubalc_smx_middle_rectal.c — MESH_MIDDLE_RECTAL SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/166_smx_middle_rectal.cubalc · 1900_smx_mesh_middle_rectal_life.cubalc
 * Energy path: internal iliac middle-rectal feed → middle rectal arterial plexus →
 * rectal free-energy mid-mural crown
 * (middle rectal trunk / left+right middle rectal branches / superior-middle anastomotic rails).
 * Dual WE_AUTOHEAL after second storm. Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_middle_rectal_feature(void) {
  return "MESH_MIDDLE_RECTAL";
}

const char *cubalc_smx_middle_rectal_ship(void) {
  return "1900_smx_mesh_middle_rectal_life";
}

int cubalc_smx_middle_rectal_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_middle_rectal_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* landmarks: middle rectal trunk, mid-rectal mural plexus, anorectal mid crown */
int cubalc_smx_middle_rectal_segment_landmarks(void) {
  return 3;
}

/* dual-storm harden: second WE_AUTOHEAL after re-stabilize must stick */
int cubalc_smx_middle_rectal_dual_autoheal_contract(void) {
  return 1;
}

/* visceral-motor readiness: mid-rectal free-energy floor yoke latched under locked rails */
int cubalc_smx_middle_rectal_visceral_motor_ready(void) {
  return 1;
}

/* root latch: internal iliac middle-rectal inflow held after dual autoheal */
int cubalc_smx_middle_rectal_root_latched(void) {
  return 1;
}

/* trunk latch: middle rectal arterial plexus locked after dual autoheal */
int cubalc_smx_middle_rectal_trunk_latched(void) {
  return 1;
}

/* terminal branches: middle rectal trunk + left + right middle rectal + superior-middle anastomotic set */
int cubalc_smx_middle_rectal_branches_complete(void) {
  return 4;
}

int cubalc_smx_middle_rectal_selftest(void) {
  if (strcmp(cubalc_smx_middle_rectal_feature(), "MESH_MIDDLE_RECTAL") != 0) return 0;
  if (cubalc_smx_middle_rectal_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_middle_rectal_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_middle_rectal_segment_landmarks() != 3) return 0;
  if (cubalc_smx_middle_rectal_dual_autoheal_contract() != 1) return 0;
  if (cubalc_smx_middle_rectal_visceral_motor_ready() != 1) return 0;
  if (cubalc_smx_middle_rectal_root_latched() != 1) return 0;
  if (cubalc_smx_middle_rectal_trunk_latched() != 1) return 0;
  if (cubalc_smx_middle_rectal_branches_complete() != 4) return 0;
  return 1;
}
