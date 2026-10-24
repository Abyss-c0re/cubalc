/* CubalC DUAL imm 16-bit h* family meta-try — B_dual_peel from dual.c.
 * hbit·harith·hdiv·hcmp·hord·hsord·hsmm·hsclamp·hpred·h0ord. Zero bloat. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_imm_h.h"
#include "lang/cubalc_lang_ops_dual_imm_hbit.h"
#include "lang/cubalc_lang_ops_dual_imm_harith.h"
#include "lang/cubalc_lang_ops_dual_imm_hdiv.h"
#include "lang/cubalc_lang_ops_dual_imm_hcmp.h"
#include "lang/cubalc_lang_ops_dual_imm_hord.h"
#include "lang/cubalc_lang_ops_dual_imm_hsord.h"
#include "lang/cubalc_lang_ops_dual_imm_hsmm.h"
#include "lang/cubalc_lang_ops_dual_imm_hsclamp.h"
#include "lang/cubalc_lang_ops_dual_imm_hpred.h"
#include "lang/cubalc_lang_ops_dual_imm_h0ord.h"

int cubalc_lang_ops_dual_try_imm_h(VM *vm, Lex *L) {
  {
    int r = cubalc_lang_ops_dual_try_imm_hbit(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_harith(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_hdiv(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_hcmp(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_hord(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_hsord(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_hsmm(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_hsclamp(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_hpred(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_h0ord(vm, L);
    if (r != 0) return r;
  }
  return 0;
}
