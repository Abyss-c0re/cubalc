/* CubalC DUAL imm 4-bit n* family meta-try — B_dual_peel from dual.c.
 * narith·ndiv·ncmp·nord·nsord·nsmm·nsclamp·npred·nbit. Zero bloat. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_imm_n.h"
#include "lang/cubalc_lang_ops_dual_imm_narith.h"
#include "lang/cubalc_lang_ops_dual_imm_ndiv.h"
#include "lang/cubalc_lang_ops_dual_imm_ncmp.h"
#include "lang/cubalc_lang_ops_dual_imm_nord.h"
#include "lang/cubalc_lang_ops_dual_imm_nsord.h"
#include "lang/cubalc_lang_ops_dual_imm_nsmm.h"
#include "lang/cubalc_lang_ops_dual_imm_nsclamp.h"
#include "lang/cubalc_lang_ops_dual_imm_npred.h"
#include "lang/cubalc_lang_ops_dual_imm_nbit.h"

int cubalc_lang_ops_dual_try_imm_n(VM *vm, Lex *L) {
  {
    int r = cubalc_lang_ops_dual_try_imm_narith(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_ndiv(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_ncmp(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_nord(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_nsord(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_nsmm(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_nsclamp(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_npred(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_nbit(vm, L);
    if (r != 0) return r;
  }
  return 0;
}
