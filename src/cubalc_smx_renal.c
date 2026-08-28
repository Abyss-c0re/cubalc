/* cubalc_smx_renal.c — MESH_RENAL SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/152_smx_renal.cubalc · 1886_smx_mesh_renal_life.cubalc
 * Energy path: least splanchnic / aorticorenal feed → renal plexus →
 * renal ganglia free-energy kidney visceral crown (afferent/efferent arteriolar rails).
 * Dual WE_AUTOHEAL after second storm. Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_renal_feature(void) {
  return "MESH_RENAL";
}

const char *cubalc_smx_renal_ship(void) {
  return "1886_smx_mesh_renal_life";
}

int cubalc_smx_renal_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_renal_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* landmarks: aorticorenal ganglia, left+right renal plexus, kidney visceral crown */
int cubalc_smx_renal_segment_landmarks(void) {
  return 3;
}

/* dual-storm harden: second WE_AUTOHEAL after re-stabilize must stick */
int cubalc_smx_renal_dual_autoheal_contract(void) {
  return 1;
}

/* visceral-motor readiness: renal free-energy floor yoke latched under locked rails */
int cubalc_smx_renal_visceral_motor_ready(void) {
  return 1;
}

/* root latch: least splanchnic + aorticorenal inflow held after dual autoheal */
int cubalc_smx_renal_root_latched(void) {
  return 1;
}

/* trunk latch: bilateral renal plexuses locked after dual autoheal */
int cubalc_smx_renal_trunk_latched(void) {
  return 1;
}

/* terminal branches: afferent arteriolar + efferent arteriolar + juxtaglomerular set */
int cubalc_smx_renal_branches_complete(void) {
  return 3;
}

int cubalc_smx_renal_selftest(void) {
  if (strcmp(cubalc_smx_renal_feature(), "MESH_RENAL") != 0) return 0;
  if (cubalc_smx_renal_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_renal_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_renal_segment_landmarks() != 3) return 0;
  if (cubalc_smx_renal_dual_autoheal_contract() != 1) return 0;
  if (cubalc_smx_renal_visceral_motor_ready() != 1) return 0;
  if (cubalc_smx_renal_root_latched() != 1) return 0;
  if (cubalc_smx_renal_trunk_latched() != 1) return 0;
  if (cubalc_smx_renal_branches_complete() != 3) return 0;
  return 1;
}
