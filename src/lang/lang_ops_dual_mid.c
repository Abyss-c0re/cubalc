/* CubalC DUAL mid imm* meta-try — B_dual_peel from dual.c.
 * immshift·imm_alu·imm_sat·imm_div·imm_bit·imm_range·imm_cmp.
 * Zero bloat. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_mid.h"
#include "lang/cubalc_lang_ops_dual_immshift.h"
#include "lang/cubalc_lang_ops_dual_imm_alu.h"
#include "lang/cubalc_lang_ops_dual_imm_sat.h"
#include "lang/cubalc_lang_ops_dual_imm_div.h"
#include "lang/cubalc_lang_ops_dual_imm_bit.h"
#include "lang/cubalc_lang_ops_dual_imm_range.h"
#include "lang/cubalc_lang_ops_dual_imm_cmp.h"

int cubalc_lang_ops_dual_try_mid(VM *vm, Lex *L) {
  {
    int r = cubalc_lang_ops_dual_try_immshift(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_alu(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_sat(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_div(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_bit(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_range(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_cmp(vm, L);
    if (r != 0) return r;
  }
  return 0;
}
