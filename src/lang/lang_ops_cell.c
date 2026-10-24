/* CubalC lang — lang_ops_cell.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_cell_early.h"
#include "lang/cubalc_lang_ops_cell_mid.h"
#include "lang/cubalc_lang_ops_cell_inc.h"
#include "lang/cubalc_lang_ops_cell_lib.h"

int cubalc_lang_ops_cell(VM *vm, Lex *L){
  /* early nibble4…rng width stack → cell_early meta-try (B_cell_peel) */
  {
    int er = cubalc_lang_ops_cell_try_early(vm, L);
    if (er != 0) return er;
  }
  /* energy·mat·energyflow → cell_mid meta-try (B_cell_peel) */
  {
    int md = cubalc_lang_ops_cell_try_mid(vm, L);
    if (md != 0) return md;
  }
  /* runtime·include_query·preload·include → cell_inc meta-try (B_cell_peel) */
  {
    int inc = cubalc_lang_ops_cell_try_inc(vm, L);
    if (inc != 0) return inc;
  }
  /* lib* family libcat·libpick·libage·libread·libmeta → cell_lib meta-try (B_cell_peel) */
  {
    int lib = cubalc_lang_ops_cell_try_lib(vm, L);
    if (lib != 0) return lib;
  }

  return 0;
}
