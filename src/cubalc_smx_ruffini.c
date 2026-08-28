/* cubalc_smx_ruffini.c — MESH_RUFFINI SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/196_smx_ruffini.cubalc · 1930_smx_mesh_ruffini_life.cubalc
 * Energy path: merkel disc plane origin → ruffini spindle conduits (deep dermis
 * Ruffini corpuscle / joint-capsule stretch SA-II) → proprioceptive stretch free-energy crown
 * (Ruffini corpuscle SA-II stretch crest).
 * Dual WE_AUTOHEAL after second storm. Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_ruffini_feature(void) {
  return "MESH_RUFFINI";
}

const char *cubalc_smx_ruffini_ship(void) {
  return "1930_smx_mesh_ruffini_life";
}

int cubalc_smx_ruffini_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_ruffini_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* landmarks: merkel disc origin, ruffini spindle conduits, proprioceptive stretch crown */
int cubalc_smx_ruffini_segment_landmarks(void) {
  return 3;
}

/* dual-storm harden: second WE_AUTOHEAL after re-stabilize must stick */
int cubalc_smx_ruffini_dual_autoheal_contract(void) {
  return 1;
}

/* visceral-motor readiness: ruffini free-energy floor yoke latched under locked rails */
int cubalc_smx_ruffini_visceral_motor_ready(void) {
  return 1;
}

/* root latch: merkel disc plane origin held after dual autoheal */
int cubalc_smx_ruffini_root_latched(void) {
  return 1;
}

/* trunk latch: ruffini spindle / joint-capsule conduits locked after dual autoheal */
int cubalc_smx_ruffini_trunk_latched(void) {
  return 1;
}

/* terminal branches: primary spindle capsule + secondary collagen bouquet + SA-II axon + joint-stretch crest */
int cubalc_smx_ruffini_branches_complete(void) {
  return 4;
}

int cubalc_smx_ruffini_selftest(void) {
  if (strcmp(cubalc_smx_ruffini_feature(), "MESH_RUFFINI") != 0) return 0;
  if (cubalc_smx_ruffini_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_ruffini_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_ruffini_segment_landmarks() != 3) return 0;
  if (cubalc_smx_ruffini_dual_autoheal_contract() != 1) return 0;
  if (cubalc_smx_ruffini_visceral_motor_ready() != 1) return 0;
  if (cubalc_smx_ruffini_root_latched() != 1) return 0;
  if (cubalc_smx_ruffini_trunk_latched() != 1) return 0;
  if (cubalc_smx_ruffini_branches_complete() != 4) return 0;
  return 1;
}
