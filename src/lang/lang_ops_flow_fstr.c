/* fstr dispatcher */
#include "lang/cubalc_lang_ops_flow_fstr.h"
#include "lang/cubalc_lang_ops_flow_fstr_a.h"
#include "lang/cubalc_lang_ops_flow_fstr_b.h"
int cubalc_lang_ops_flow_try_fstr(VM *vm, Lex *L) {
  int r;
  r = cubalc_lang_ops_flow_try_fstr_a(vm, L); if (r) return r;
  r = cubalc_lang_ops_flow_try_fstr_b(vm, L); if (r) return r;
  return 0;
}
