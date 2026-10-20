/* SMX cascade dispatcher — water · land · cosmos soft planes */
#include "lang/cubalc_lang_ops_smx_cascade.h"
#include "lang/cubalc_lang_ops_smx_water.h"
#include "lang/cubalc_lang_ops_smx_land.h"
#include "lang/cubalc_lang_ops_smx_cosmos.h"
int cubalc_lang_ops_smx_try_cascade(VM *vm, Lex *L) {
  int r;
  r = cubalc_lang_ops_smx_try_water(vm, L); if (r) return r;
  r = cubalc_lang_ops_smx_try_land(vm, L); if (r) return r;
  r = cubalc_lang_ops_smx_try_cosmos(vm, L); if (r) return r;
  return 0;
}
