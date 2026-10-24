/* CubalC CELL mat* family meta-try — B_cell_peel from cell.c.
 * matbits·matscan·matmetric·matruns·matfind·matrange·matword·matpath·
 * matfill·matalgebra·matrel·matmux·matbrain·matlogic. Zero bloat. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_mat.h"
#include "lang/cubalc_lang_ops_cell_matbits.h"
#include "lang/cubalc_lang_ops_cell_matscan.h"
#include "lang/cubalc_lang_ops_cell_matmetric.h"
#include "lang/cubalc_lang_ops_cell_matruns.h"
#include "lang/cubalc_lang_ops_cell_matfind.h"
#include "lang/cubalc_lang_ops_cell_matrange.h"
#include "lang/cubalc_lang_ops_cell_matword.h"
#include "lang/cubalc_lang_ops_cell_matpath.h"
#include "lang/cubalc_lang_ops_cell_matfill.h"
#include "lang/cubalc_lang_ops_cell_matalgebra.h"
#include "lang/cubalc_lang_ops_cell_matrel.h"
#include "lang/cubalc_lang_ops_cell_matmux.h"
#include "lang/cubalc_lang_ops_cell_matbrain.h"
#include "lang/cubalc_lang_ops_cell_matlogic.h"

int cubalc_lang_ops_cell_try_mat(VM *vm, Lex *L) {
  {
    int r = cubalc_lang_ops_cell_try_matbits(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_matscan(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_matmetric(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_matruns(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_matfind(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_matrange(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_matword(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_matpath(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_matfill(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_matalgebra(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_matrel(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_matmux(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_matbrain(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_matlogic(vm, L);
    if (r != 0) return r;
  }
  return 0;
}
