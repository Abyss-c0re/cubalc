/* CubalC CELL inc* family meta-try — B_cell_peel from cell.c.
 * runtime·include_query·preload·include. Zero bloat. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_inc.h"
#include "lang/cubalc_lang_ops_cell_runtime.h"
#include "lang/cubalc_lang_ops_cell_include_query.h"
#include "lang/cubalc_lang_ops_cell_preload.h"
#include "lang/cubalc_lang_ops_cell_include.h"

int cubalc_lang_ops_cell_try_inc(VM *vm, Lex *L) {
  {
    int r = cubalc_lang_ops_cell_try_runtime(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_include_query(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_preload(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_include(vm, L);
    if (r != 0) return r;
  }
  return 0;
}
