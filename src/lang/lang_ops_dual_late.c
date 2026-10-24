/* CubalC DUAL late imm_h·imm_b·imm_n width-family meta-try — B_dual_peel.
 * Zero bloat. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_late.h"
#include "lang/cubalc_lang_ops_dual_imm_h.h"
#include "lang/cubalc_lang_ops_dual_imm_b.h"
#include "lang/cubalc_lang_ops_dual_imm_n.h"

int cubalc_lang_ops_dual_try_late(VM *vm, Lex *L) {
  {
    int r = cubalc_lang_ops_dual_try_imm_h(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_b(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_n(vm, L);
    if (r != 0) return r;
  }
  return 0;
}
