/* cosmos dispatcher */
#include "lang/cubalc_lang_ops_smx_cosmos.h"
#include "lang/cubalc_lang_ops_smx_galaxy.h"
#include "lang/cubalc_lang_ops_smx_zodiac.h"
#include "lang/cubalc_lang_ops_smx_pneuma.h"
int cubalc_lang_ops_smx_try_cosmos(VM *vm, Lex *L) {
  int r;
  r = cubalc_lang_ops_smx_try_galaxy(vm, L); if (r) return r;
  r = cubalc_lang_ops_smx_try_zodiac(vm, L); if (r) return r;
  r = cubalc_lang_ops_smx_try_pneuma(vm, L); if (r) return r;
  return 0;
}
