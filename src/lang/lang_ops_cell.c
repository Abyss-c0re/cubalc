/* CubalC lang — lang_ops_cell.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_cell_early.h"
#include "lang/cubalc_lang_ops_cell_inc.h"

int cubalc_lang_ops_cell(VM *vm, Lex *L){
  /* early nibble4…rng + mid energy·mat·energyflow → cell_early (B_cell_peel) */
  {
    int er = cubalc_lang_ops_cell_try_early(vm, L);
    if (er != 0) return er;
  }
  /* runtime·include*·preload + lib* → cell_inc meta-try (B_cell_peel) */
  {
    int inc = cubalc_lang_ops_cell_try_inc(vm, L);
    if (inc != 0) return inc;
  }

  return 0;
}
