/* pneuma dispatcher */
#include "lang/cubalc_lang_ops_smx_pneuma.h"
#include "lang/cubalc_lang_ops_smx_pneuma_a.h"
#include "lang/cubalc_lang_ops_smx_pneuma_b.h"
int cubalc_lang_ops_smx_try_pneuma(VM *vm, Lex *L) {
  int r;
  r = cubalc_lang_ops_smx_try_pneuma_a(vm, L); if (r) return r;
  r = cubalc_lang_ops_smx_try_pneuma_b(vm, L); if (r) return r;
  return 0;
}
