/* CubalC lang — lang_ops_dual.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_dual_early.h"

int cubalc_lang_ops_dual(VM *vm, Lex *L){
  /* early pack…mid + bit/late → dual_early meta-try (B_dual_peel) */
  {
    int er = cubalc_lang_ops_dual_try_early(vm, L);
    if (er != 0) return er;
  }
  return 0;
}
