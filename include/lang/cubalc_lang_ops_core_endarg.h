#ifndef CUBALC_LANG_OPS_CORE_ENDARG_H
#define CUBALC_LANG_OPS_CORE_ENDARG_H
#include "lang/cubalc_lang_internal.h"
#ifdef __cplusplus
extern "C" {
#endif
/* FIRSTARG/LASTARG/NTHARG — raw CUBALC_ARGi peels (rescued post-thin). */
int cubalc_lang_ops_core_try_endarg(VM *vm, Lex *L);
#ifdef __cplusplus
}
#endif
#endif
