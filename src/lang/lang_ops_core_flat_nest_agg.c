/* nest_agg dispatcher */
#include "lang/cubalc_lang_ops_core_flat_nest_agg.h"
#include "lang/cubalc_lang_ops_core_flat_nest_sum.h"
#include "lang/cubalc_lang_ops_core_flat_nest_thr.h"
#include "lang/cubalc_lang_ops_core_flat_nest_top.h"
#include "lang/cubalc_lang_ops_core_flat_nest_sort.h"
int cubalc_lang_ops_core_try_flat_nest_agg(VM *vm, Lex *L) {
  int r;
  r = cubalc_lang_ops_core_try_flat_nest_sum(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_flat_nest_thr(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_flat_nest_top(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_flat_nest_sort(vm, L); if (r) return r;
  return 0;
}
