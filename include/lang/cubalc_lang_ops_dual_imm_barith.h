/* CubalC lang — cubalc_lang_ops_dual_imm_barith.h
 * DUAL imm 8-bit field arith: DADD8N·DSUB8N·DMUL8N (B_dual_peel).
 */
#ifndef CUBALC_LANG_OPS_DUAL_IMM_BARITH_H
#define CUBALC_LANG_OPS_DUAL_IMM_BARITH_H
#include "lang/cubalc_lang_internal.h"
int cubalc_lang_ops_dual_try_imm_barith(VM *vm, Lex *L);
#endif
