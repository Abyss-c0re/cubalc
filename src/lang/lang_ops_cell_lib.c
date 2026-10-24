/* CubalC CELL lib* family meta-try — B_cell_peel from cell.c.
 * libcat·libpick·libage·libread·libmeta. Zero bloat. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_lib.h"
#include "lang/cubalc_lang_ops_cell_libcat.h"
#include "lang/cubalc_lang_ops_cell_libpick.h"
#include "lang/cubalc_lang_ops_cell_libage.h"
#include "lang/cubalc_lang_ops_cell_libread.h"
#include "lang/cubalc_lang_ops_cell_libmeta.h"

int cubalc_lang_ops_cell_try_lib(VM *vm, Lex *L) {
  {
    int r = cubalc_lang_ops_cell_try_libcat(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_libpick(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_libage(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_libread(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_libmeta(vm, L);
    if (r != 0) return r;
  }
  return 0;
}
