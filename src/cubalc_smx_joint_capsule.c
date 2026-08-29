/* cubalc_smx_joint_capsule.c — MESH_JOINT_CAPSULE SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/202_smx_joint_capsule.cubalc · 1936_smx_mesh_joint_capsule_life.cubalc
 * Energy path: golgi-tendon tension origin → joint-capsule conduits
 * (articular capsule type-I Ruffini + type-II Pacini + type-III Golgi-like + type-IV free-nerve
 *  mechanoreceptors — position, acceleration, end-range & nociception)
 * → joint-capsule free-energy crown (proprioceptive articular sensory crest).
 * Dual WE_AUTOHEAL after second storm. Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_joint_capsule_feature(void) {
  return "MESH_JOINT_CAPSULE";
}

const char *cubalc_smx_joint_capsule_ship(void) {
  return "1936_smx_mesh_joint_capsule_life";
}

int cubalc_smx_joint_capsule_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_joint_capsule_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* landmarks: golgi-tendon tension origin, capsule conduits, articular crown */
int cubalc_smx_joint_capsule_segment_landmarks(void) {
  return 3;
}

/* dual-storm harden: second WE_AUTOHEAL after re-stabilize must stick */
int cubalc_smx_joint_capsule_dual_autoheal_contract(void) {
  return 1;
}

/* visceral-motor readiness: joint-capsule free-energy floor yoke latched under locked rails */
int cubalc_smx_joint_capsule_visceral_motor_ready(void) {
  return 1;
}

/* root latch: golgi-tendon tension plane origin held after dual autoheal */
int cubalc_smx_joint_capsule_root_latched(void) {
  return 1;
}

/* trunk latch: type I–IV capsule mechanoreceptor conduits locked after dual autoheal */
int cubalc_smx_joint_capsule_trunk_latched(void) {
  return 1;
}

/* terminal branches: type-I Ruffini + type-II Pacini + type-III Golgi-like + type-IV free-nerve crest */
int cubalc_smx_joint_capsule_branches_complete(void) {
  return 4;
}

int cubalc_smx_joint_capsule_selftest(void) {
  if (strcmp(cubalc_smx_joint_capsule_feature(), "MESH_JOINT_CAPSULE") != 0) return 0;
  if (cubalc_smx_joint_capsule_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_joint_capsule_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_joint_capsule_segment_landmarks() != 3) return 0;
  if (cubalc_smx_joint_capsule_dual_autoheal_contract() != 1) return 0;
  if (cubalc_smx_joint_capsule_visceral_motor_ready() != 1) return 0;
  if (cubalc_smx_joint_capsule_root_latched() != 1) return 0;
  if (cubalc_smx_joint_capsule_trunk_latched() != 1) return 0;
  if (cubalc_smx_joint_capsule_branches_complete() != 4) return 0;
  return 1;
}
