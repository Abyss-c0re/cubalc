/* CubalC lang — lang_ops_dual.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_dual_early.h"
#include "lang/cubalc_lang_ops_dual_bit.h"

int cubalc_lang_ops_dual(VM *vm, Lex *L){
  /* early pack…fieldrot + mid imm* → dual_early meta-try (B_dual_peel) */
  {
    int er = cubalc_lang_ops_dual_try_early(vm, L);
    if (er != 0) return er;
  }
  /* bitpath…imm_wbit + late imm_h/b/n → dual_bit meta-try (B_dual_peel) */
  {
    int bt = cubalc_lang_ops_dual_try_bit(vm, L);
    if (bt != 0) return bt;
  }
  return 0;
}
