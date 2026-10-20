/* CubalC lang — ASYNC/AWAIT/PARALLEL ops plane.
 * Split from lang_ops_core (mesh vote A).
 */
#ifndef CUBALC_LANG_OPS_CORE_ASYNC_H
#define CUBALC_LANG_OPS_CORE_ASYNC_H

#include "lang/cubalc_lang_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 1 = handled, 0 = not this plane, -1 = fail */
int cubalc_lang_ops_core_try_async(VM *vm, Lex *L);

#ifdef __cplusplus
}
#endif
#endif
