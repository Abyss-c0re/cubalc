/* CubalC flat nestq dispatcher */
#include "lang/cubalc_lang_ops_core_flat_nestq.h"
#include "lang/cubalc_lang_ops_core_flat_nest_pick.h"
#include "lang/cubalc_lang_ops_core_flat_nest_kv.h"
#include "lang/cubalc_lang_ops_core_flat_nest_agg.h"

int cubalc_lang_ops_core_try_flat_nestq(VM *vm, Lex *L) {
  int r;
  r = cubalc_lang_ops_core_try_flat_nest_pick(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_flat_nest_kv(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_flat_nest_agg(vm, L); if (r) return r;
  return 0;
}
