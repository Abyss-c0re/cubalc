/* CubalC DUAL imm 8-bit b* family meta-try — B_dual_peel from dual.c.
 * barith·bdiv·bcmp·bord·bsord·bsmm·bsclamp·bpred·b0ord·bbit. Zero bloat. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_imm_b.h"
#include "lang/cubalc_lang_ops_dual_imm_barith.h"
#include "lang/cubalc_lang_ops_dual_imm_bdiv.h"
#include "lang/cubalc_lang_ops_dual_imm_bcmp.h"
#include "lang/cubalc_lang_ops_dual_imm_bord.h"
#include "lang/cubalc_lang_ops_dual_imm_bsord.h"
#include "lang/cubalc_lang_ops_dual_imm_bsmm.h"
#include "lang/cubalc_lang_ops_dual_imm_bsclamp.h"
#include "lang/cubalc_lang_ops_dual_imm_bpred.h"
#include "lang/cubalc_lang_ops_dual_imm_b0ord.h"
#include "lang/cubalc_lang_ops_dual_imm_bbit.h"

int cubalc_lang_ops_dual_try_imm_b(VM *vm, Lex *L) {
  {
    int r = cubalc_lang_ops_dual_try_imm_barith(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_bdiv(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_bcmp(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_bord(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_bsord(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_bsmm(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_bsclamp(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_bpred(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_b0ord(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_bbit(vm, L);
    if (r != 0) return r;
  }
  return 0;
}
