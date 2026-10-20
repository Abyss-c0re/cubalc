/* CubalC lang — SYS/HOST dispatch plane.
 * Split from lang_ops_core (mesh vote A).
 */
#ifndef CUBALC_LANG_OPS_CORE_SYS_H
#define CUBALC_LANG_OPS_CORE_SYS_H
#include "lang/cubalc_lang_internal.h"
#ifdef __cplusplus
extern "C" {
#endif
/* 1 handled, 0 not SYS, -1 fail */
int cubalc_lang_ops_core_try_sys(VM *vm, Lex *L);
#ifdef __cplusplus
}
#endif
#endif
