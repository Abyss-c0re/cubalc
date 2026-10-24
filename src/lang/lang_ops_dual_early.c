/* CubalC DUAL early pack·clip·field32·unary*·pair meta-try — B_dual_peel.
 * Zero bloat. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_early.h"
#include "lang/cubalc_lang_ops_dual_pack.h"
#include "lang/cubalc_lang_ops_dual_clip.h"
#include "lang/cubalc_lang_ops_dual_field32.h"
#include "lang/cubalc_lang_ops_dual_unary32.h"
#include "lang/cubalc_lang_ops_dual_unary16.h"
#include "lang/cubalc_lang_ops_dual_unary8.h"
#include "lang/cubalc_lang_ops_dual_unary4.h"
#include "lang/cubalc_lang_ops_dual_pair.h"

int cubalc_lang_ops_dual_try_early(VM *vm, Lex *L) {
  {
    int r = cubalc_lang_ops_dual_try_pack(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_clip(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_field32(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_unary32(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_unary16(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_unary8(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_unary4(vm, L);
    if (r != 0) return r;
  }
  {
    int r = cubalc_lang_ops_dual_try_pair(vm, L);
    if (r != 0) return r;
  }
  return 0;
}
