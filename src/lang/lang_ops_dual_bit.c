/* CubalC DUAL bit/field meta-try — B_dual_peel from dual.c.
 * bitpath·bitmet·datapath·imm_field·imm_wbit. Zero bloat. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_bit.h"
#include "lang/cubalc_lang_ops_dual_bitpath.h"
#include "lang/cubalc_lang_ops_dual_bitmet.h"
#include "lang/cubalc_lang_ops_dual_datapath.h"
#include "lang/cubalc_lang_ops_dual_imm_field.h"
#include "lang/cubalc_lang_ops_dual_imm_wbit.h"

int cubalc_lang_ops_dual_try_bit(VM *vm, Lex *L) {
  {
    int r = cubalc_lang_ops_dual_try_bitpath(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_bitmet(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_datapath(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_field(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_imm_wbit(vm, L);
    if (r != 0) return r;
  }
  return 0;
}
