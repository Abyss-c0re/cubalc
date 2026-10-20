/* nest_sort dispatcher — sortobj + merge only (sortp/sortin deferred broken) */
#include "lang/cubalc_lang_ops_core_flat_nest_sort.h"
#include "lang/cubalc_lang_ops_core_flat_nest_sortobj.h"
#include "lang/cubalc_lang_ops_core_flat_nest_merge.h"
int cubalc_lang_ops_core_try_flat_nest_sort(VM *vm, Lex *L) {
  int r;
  r = cubalc_lang_ops_core_try_flat_nest_sortobj(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_flat_nest_merge(vm, L); if (r) return r;
  return 0;
}
