/* CubalC lang — lang_ops_cell.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_cell_early.h"

int cubalc_lang_ops_cell(VM *vm, Lex *L){
  /* early nibble4…rng·mid + inc/lib → cell_early meta-try (B_cell_peel) */
  {
    int er = cubalc_lang_ops_cell_try_early(vm, L);
    if (er != 0) return er;
  }
  return 0;
}
