/* CubalC lang — lang_ops_toc.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_toc_cell.h"
#include "lang/cubalc_lang_ops_toc_imm.h"
#include "lang/cubalc_lang_ops_toc_field.h"
#include "lang/cubalc_lang_ops_toc_extclip.h"
#include "lang/cubalc_lang_ops_toc_mask.h"
#include "lang/cubalc_lang_ops_toc_bitops.h"
#include "lang/cubalc_lang_ops_toc_alu.h"
#include "lang/cubalc_lang_ops_toc_mod.h"
#include "lang/cubalc_lang_ops_toc_revsat.h"
#include "lang/cubalc_lang_ops_toc_bwshift.h"
#include "lang/cubalc_lang_ops_toc_unary.h"
#include "lang/cubalc_lang_ops_toc_metrics.h"
#include "lang/cubalc_lang_ops_toc_cmp.h"

int cubalc_lang_ops_toc(VM *vm, Lex *L){
  /* plane ops_toc — sequential thin dispatchers */
  {
    int tc = cubalc_lang_ops_toc_try_cell(vm, L);
    if (tc != 0) return tc;
  }
  {
    int ti = cubalc_lang_ops_toc_try_imm(vm, L);
    if (ti != 0) return ti;
  }
  {
    int tf = cubalc_lang_ops_toc_try_field(vm, L);
    if (tf != 0) return tf;
  }
  {
    int te = cubalc_lang_ops_toc_try_extclip(vm, L);
    if (te != 0) return te;
  }
  {
    int tm = cubalc_lang_ops_toc_try_mask(vm, L);
    if (tm != 0) return tm;
  }
  {
    int tb = cubalc_lang_ops_toc_try_bitops(vm, L);
    if (tb != 0) return tb;
  }
  {
    int ta = cubalc_lang_ops_toc_try_alu(vm, L);
    if (ta != 0) return ta;
  }
  {
    int td = cubalc_lang_ops_toc_try_mod(vm, L);
    if (td != 0) return td;
  }
  {
    int tr = cubalc_lang_ops_toc_try_revsat(vm, L);
    if (tr != 0) return tr;
  }
  {
    int tw = cubalc_lang_ops_toc_try_bwshift(vm, L);
    if (tw != 0) return tw;
  }
  {
    int tu = cubalc_lang_ops_toc_try_unary(vm, L);

  /* metrics/width/range → lang_ops_toc_metrics.c (B_toc_peel) */
  {
    int tm = cubalc_lang_ops_toc_try_metrics(vm, L);
    if (tm != 0) return tm;
  }
    if (tu != 0) return tu;
  }
  {
    int tp = cubalc_lang_ops_toc_try_cmp(vm, L);
    if (tp != 0) return tp;
  }

  return 0;
}
