/* CubalC lang — lang_ops_math.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_math_stack_sci.h"
#include "lang/cubalc_lang_ops_math_nbit.h"
#include "lang/cubalc_lang_ops_math_numthy.h"
#include "lang/cubalc_lang_ops_math_bitpath.h"
#include "lang/cubalc_lang_ops_math_unary.h"
#include "lang/cubalc_lang_ops_math_mod.h"
#include "lang/cubalc_lang_ops_math_field16n.h"
#include "lang/cubalc_lang_ops_math_field8n.h"
#include "lang/cubalc_lang_ops_math_fieldacc.h"
#include "lang/cubalc_lang_ops_math_field4n.h"
#include "lang/cubalc_lang_ops_math_field32n.h"
#include <limits.h>

int cubalc_lang_ops_math(VM *vm, Lex *L){
  /* plane ops_math: L20289-22808 */
  /* Number theory plane → lang_ops_math_numthy.c (B_math_peel) */
  {
    int nt = cubalc_lang_ops_math_try_numthy(vm, L);
    if (nt != 0) return nt;
  }

  /* MATH stack unary → lang_ops_math_unary.c (B_math_peel) */
  {
    int un = cubalc_lang_ops_math_try_unary(vm, L);
    if (un != 0) return un;
  }

  /* Modular plane → lang_ops_math_mod.c (B_math_peel) */
  {
    int md = cubalc_lang_ops_math_try_mod(vm, L);
    if (md != 0) return md;
  }

  /* Field access/merge → lang_ops_math_fieldacc.c (B_math_peel) */
  {
    int fa = cubalc_lang_ops_math_try_fieldacc(vm, L);
    if (fa != 0) return fa;
  }

  /* 4-bit field ladder → lang_ops_math_field4n.c (B_math_peel) */
  {
    int f4 = cubalc_lang_ops_math_try_field4n(vm, L);
    if (f4 != 0) return f4;
  }

  /* 8-bit field ladder → lang_ops_math_field8n.c (B_math_peel) */
  {
    int f8 = cubalc_lang_ops_math_try_field8n(vm, L);
    if (f8 != 0) return f8;
  }

  /* 16-bit field ladder → lang_ops_math_field16n.c (B_math_peel) */
  {
    int f16 = cubalc_lang_ops_math_try_field16n(vm, L);
    if (f16 != 0) return f16;
  }

  /* 32-bit field ladder → lang_ops_math_field32n.c (B_math_peel) */
  {
    int f32 = cubalc_lang_ops_math_try_field32n(vm, L);
    if (f32 != 0) return f32;
  }

  /* MATH bitpath/nibble → lang_ops_math_bitpath.c (B_math_peel) */
  {
    int bp = cubalc_lang_ops_math_try_bitpath(vm, L);
    if (bp != 0) return bp;
  }

  /* Stack science duals → lang_ops_math_stack_sci.c (B_math_peel) */
  {
    int ss = cubalc_lang_ops_math_try_stack_sci(vm, L);
    if (ss != 0) return ss;
  }

  /* N-suffix bit/align plane → lang_ops_math_nbit.c (B_math_peel) */
  {
    int nb = cubalc_lang_ops_math_try_nbit(vm, L);
    if (nb != 0) return nb;
  }

  return 0;
}
