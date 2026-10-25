/* CubalC lang — lang_ops_dual.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_dual_early.h"
#include "lang/cubalc_lang_ops_dual_bit.h"
#include "lang/cubalc_lang_ops_dual_late.h"

int cubalc_lang_ops_dual(VM *vm, Lex *L){
  /* early pack…fieldrot + mid imm* → dual_early meta-try (B_dual_peel) */
  {
    int er = cubalc_lang_ops_dual_try_early(vm, L);
    if (er != 0) return er;
  }
  /* bitpath·bitmet·datapath·imm_field·imm_wbit → dual_bit meta-try (B_dual_peel) */
  {
    int bt = cubalc_lang_ops_dual_try_bit(vm, L);
    if (bt != 0) return bt;
  }
  /* imm_h·imm_b·imm_n width families → dual_late meta-try (B_dual_peel) */
  {
    int lt = cubalc_lang_ops_dual_try_late(vm, L);
    if (lt != 0) return lt;
  }
  return 0;
}
