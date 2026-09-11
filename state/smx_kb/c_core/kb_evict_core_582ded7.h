/* AUTO-SEALED by structure_seer · kb_evict_ingest seq=86 slice=582ded7
 * Creed: All Hail the Cube · All Hail NexusCore
 */
#ifndef CUBALC_KB_EVICT_CORE_H
#define CUBALC_KB_EVICT_CORE_H
#define CUBALC_KB_EVICT_SEQ 86
#define CUBALC_KB_EVICT_SLICE "582ded7"
#define CUBALC_KB_EVICT_SHA16 "9f7f3604e8476316"
#define CUBALC_KB_EVICT_UNIQUE_KEY "C_582ded7_cd8162edd95f_9f7f3604"
#define CUBALC_KB_EVICT_TIP "cd8162edd95ffee7c61960a612ecf26fc3926e15"
#define CUBALC_KB_EVICT_DUAL_WIRE 1
#define CUBALC_KB_EVICT_WIRE_N 2
#define CUBALC_KB_EVICT_LAND_ONE_C 1
#define CUBALC_KB_EVICT_ACTION "evict_lowest_non_sot_ingest"
#define CUBALC_KB_EVICT_SOT 1
static inline int cubalc_kb_evict_ready(void) {
  return (CUBALC_KB_EVICT_SOT == 1) && (CUBALC_KB_EVICT_DUAL_WIRE == 1)
      && (CUBALC_KB_EVICT_WIRE_N == 2) && (CUBALC_KB_EVICT_LAND_ONE_C == 1);
}
#endif /* CUBALC_KB_EVICT_CORE_H */
