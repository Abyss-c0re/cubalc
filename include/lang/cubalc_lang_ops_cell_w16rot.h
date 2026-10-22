/* CubalC lang — CELL width16 rot/bitrev + bswap64 peel (G_cell_margin). */
#ifndef CUBALC_LANG_OPS_CELL_W16ROT_H
#define CUBALC_LANG_OPS_CELL_W16ROT_H
#include "lang/cubalc_lang_internal.h"
#ifdef __cplusplus
extern "C" {
#endif
int cubalc_lang_ops_cell_try_w16rot(VM *vm, Lex *L);
#ifdef __cplusplus
}
#endif
#endif
