/* CubalC — flat_query dispatcher */
#include "lang/cubalc_lang_ops_core_flat_query.h"
#include "lang/cubalc_lang_ops_core_flat_has.h"
#include "lang/cubalc_lang_ops_core_flat_get.h"
#include "lang/cubalc_lang_ops_core_flat_stats.h"
#include "lang/cubalc_lang_ops_core_flat_nestq.h"
#include "lang/cubalc_lang_ops_core_flat_bag.h"
#include "lang/cubalc_lang_ops_core_bag_push.h"

int cubalc_lang_ops_core_try_flat_query(VM *vm, Lex *L) {
  int r;
  r = cubalc_lang_ops_core_try_flat_has(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_flat_get(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_flat_stats(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_flat_nestq(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_bag_push(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_flat_bag(vm, L); if (r) return r;
  return 0;
}
