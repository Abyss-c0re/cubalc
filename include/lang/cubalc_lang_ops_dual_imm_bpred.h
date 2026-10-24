/* CubalC lang — cubalc_lang_ops_dual_imm_bpred.h
 * DUAL imm 8-bit sign/zero preds: DSIGN8N·D0EQ8N·D0NE8N (B_dual_peel).
 */
#ifndef CUBALC_LANG_OPS_DUAL_IMM_BPRED_H
#define CUBALC_LANG_OPS_DUAL_IMM_BPRED_H
#include "lang/cubalc_lang_internal.h"
int cubalc_lang_ops_dual_try_imm_bpred(VM *vm, Lex *L);
#endif
