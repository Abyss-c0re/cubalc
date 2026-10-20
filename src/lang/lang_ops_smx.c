/* CubalC lang — lang_ops_smx.c thin dispatcher */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_smx_core.h"
#include "lang/cubalc_lang_ops_smx_cascade.h"
int cubalc_lang_ops_smx(VM *vm, Lex *L) {
  int r;
  r = cubalc_lang_ops_smx_try_core(vm, L); if (r) return r;
  r = cubalc_lang_ops_smx_try_cascade(vm, L); if (r) return r;
  return 0;
}
