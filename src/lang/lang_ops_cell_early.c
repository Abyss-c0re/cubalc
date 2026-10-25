/* CubalC CELL early width stack + mid energy/mat meta-try — B_cell_peel.
 * nibble4·width8·bitwise_w·w16rot·shift32·rot32·metrics·rng·mid.
 * Zero bloat. Cube is SoT.
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
  /* mid energy·mat·energyflow after rng — same order as prior cell.c top-level */
  {
    int r = cubalc_lang_ops_cell_try_mid(vm, L);
    if (r != 0) return r;
  }
  return 0;
}
