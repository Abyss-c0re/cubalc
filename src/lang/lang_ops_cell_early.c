/* CubalC CELL early width/mid + inc/lib meta-try — B_cell_peel.
 * nibble4…rng·mid·inc. Zero bloat. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_early.h"
#include "lang/cubalc_lang_ops_cell_nibble4.h"
#include "lang/cubalc_lang_ops_cell_width8.h"
#include "lang/cubalc_lang_ops_cell_bitwise_w.h"
#include "lang/cubalc_lang_ops_cell_w16rot.h"
#include "lang/cubalc_lang_ops_cell_shift32.h"
#include "lang/cubalc_lang_ops_cell_rot32.h"
#include "lang/cubalc_lang_ops_cell_metrics.h"
#include "lang/cubalc_lang_ops_cell_rng.h"
#include "lang/cubalc_lang_ops_cell_mid.h"
#include "lang/cubalc_lang_ops_cell_inc.h"

int cubalc_lang_ops_cell_try_early(VM *vm, Lex *L) {
  {
    int r = cubalc_lang_ops_cell_try_nibble4(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_width8(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_bitwise_w(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_w16rot(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_shift32(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_rot32(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_metrics(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_rng(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_cell_try_mid(vm, L);
    if (r != 0) return r;
  }
  /* inc runtime·include*·preload·lib after mid — same order as prior cell.c */
  {
    int r = cubalc_lang_ops_cell_try_inc(vm, L);
    if (r != 0) return r;
  }
  return 0;
}
