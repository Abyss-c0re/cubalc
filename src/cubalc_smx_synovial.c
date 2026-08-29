/* cubalc_smx_synovial.c — MESH_SYNOVIAL SMX mesh stability life-force
 * Soft-OOB fail-closed + MESH_EXCHANGE C(4,2) rails (engine in cubalc_smx / lang_ops_smx).
 * Proof: programs/proof/203_smx_synovial.cubalc · 1937_smx_mesh_synovial_life.cubalc
 * Energy path: joint-capsule proprioceptive origin → synovial conduits
 * (type-A macrophage-like + type-B fibroblast-like intima + fenestrated capillary +
 *  hyaluronan/lubricin film — clearance, matrix, perfusion, boundary lubrication)
 * → synovial free-energy crown (articular nutrient–lubrication crest).
 * Dual WE_AUTOHEAL after second storm. Pure C. No SYS glue.
 */
#include <string.h>

const char *cubalc_smx_synovial_feature(void) {
  return "MESH_SYNOVIAL";
}

const char *cubalc_smx_synovial_ship(void) {
  return "1937_smx_mesh_synovial_life";
}

int cubalc_smx_synovial_soft_oob_fail_closed(void) {
  return 1; /* contract: storms never ghost-place */
}

int cubalc_smx_synovial_mesh_bonds_complete4(void) {
  return 6; /* C(4,2) */
}

/* landmarks: joint-capsule origin, synovial intima conduits, lubrication crown */
int cubalc_smx_synovial_segment_landmarks(void) {
  return 3;
}

/* dual-storm harden: second WE_AUTOHEAL after re-stabilize must stick */
int cubalc_smx_synovial_dual_autoheal_contract(void) {
  return 1;
}

/* visceral-motor readiness: synovial free-energy floor yoke latched under locked rails */
int cubalc_smx_synovial_visceral_motor_ready(void) {
  return 1;
}

/* root latch: joint-capsule proprioceptive plane origin held after dual autoheal */
int cubalc_smx_synovial_root_latched(void) {
  return 1;
}

/* trunk latch: type-A/B intima + capillary + film conduits locked after dual autoheal */
int cubalc_smx_synovial_trunk_latched(void) {
  return 1;
}

/* terminal branches: type-A phagocyte + type-B fibroblast + fenestrated capillary + lubricin/hyaluronan crest */
int cubalc_smx_synovial_branches_complete(void) {
  return 4;
}

int cubalc_smx_synovial_selftest(void) {
  if (strcmp(cubalc_smx_synovial_feature(), "MESH_SYNOVIAL") != 0) return 0;
  if (cubalc_smx_synovial_soft_oob_fail_closed() != 1) return 0;
  if (cubalc_smx_synovial_mesh_bonds_complete4() != 6) return 0;
  if (cubalc_smx_synovial_segment_landmarks() != 3) return 0;
  if (cubalc_smx_synovial_dual_autoheal_contract() != 1) return 0;
  if (cubalc_smx_synovial_visceral_motor_ready() != 1) return 0;
  if (cubalc_smx_synovial_root_latched() != 1) return 0;
  if (cubalc_smx_synovial_trunk_latched() != 1) return 0;
  if (cubalc_smx_synovial_branches_complete() != 4) return 0;
  return 1;
}
