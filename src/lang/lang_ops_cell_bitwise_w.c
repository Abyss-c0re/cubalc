/* CubalC lang — lang_ops_cell_bitwise_w.c
 * CELL width 8/16 bitwise + NOT8/16/32: AND8CELL…NOT32CELL (G peel).
 * Mesh vote G_split_cell · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_bitwise_w.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_bitwise_w(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-1 cell fixed-width 8 bitwise: AND8CELL · OR8CELL · XOR8CELL
   * (byte dual of AND32/OR32/XOR32 after inverted32 plane; complete 8/32 bitwise ladder start) */
  if (kw(&L->cur,"AND8CELL")||kw(&L->cur,"CELLAND8")||kw(&L->cur,"BAND8CELL")||
      kw(&L->cur,"RANGEAND8")||kw(&L->cur,"AND8RANGE")||kw(&L->cur,"UAND8CELL")){
    /* AND8CELL lo hi mask — cells[i] = low8(cells[i]) & low8(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFFu) & m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"OR8CELL")||kw(&L->cur,"CELLOR8")||kw(&L->cur,"BOR8CELL")||
      kw(&L->cur,"RANGEOR8")||kw(&L->cur,"OR8RANGE")||kw(&L->cur,"UOR8CELL")){
    /* OR8CELL lo hi mask — cells[i] = low8(cells[i]) | low8(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFFu) | m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XOR8CELL")||kw(&L->cur,"CELLXOR8")||kw(&L->cur,"BXOR8CELL")||
      kw(&L->cur,"RANGEXOR8")||kw(&L->cur,"XOR8RANGE")||kw(&L->cur,"UXOR8CELL")){
    /* XOR8CELL lo hi mask — cells[i] = low8(cells[i]) ^ low8(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFFu) ^ m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 cell fixed-width 8 inverted bitwise: NAND8CELL · NOR8CELL · XNOR8CELL
   * (inverted dual of AND8/OR8/XOR8; result low8 only — complete 8 cell logic foundation) */
  if (kw(&L->cur,"NAND8CELL")||kw(&L->cur,"CELLNAND8")||kw(&L->cur,"BNAND8CELL")||
      kw(&L->cur,"RANGENAND8")||kw(&L->cur,"NAND8RANGE")||kw(&L->cur,"UNAND8CELL")){
    /* NAND8CELL lo hi mask — cells[i] = ~(low8 & low8(mask)) & 0xFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFFu) & m)) & 0xFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NOR8CELL")||kw(&L->cur,"CELLNOR8")||kw(&L->cur,"BNOR8CELL")||
      kw(&L->cur,"RANGENOR8")||kw(&L->cur,"NOR8RANGE")||kw(&L->cur,"UNOR8CELL")){
    /* NOR8CELL lo hi mask — cells[i] = ~(low8 | low8(mask)) & 0xFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFFu) | m)) & 0xFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XNOR8CELL")||kw(&L->cur,"CELLXNOR8")||kw(&L->cur,"BXNOR8CELL")||
      kw(&L->cur,"RANGEXNOR8")||kw(&L->cur,"XNOR8RANGE")||kw(&L->cur,"NXOR8CELL")||
      kw(&L->cur,"UXNOR8CELL")){
    /* XNOR8CELL lo hi mask — cells[i] = ~(low8 ^ low8(mask)) & 0xFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFFu) ^ m)) & 0xFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell fixed-width 16 bitwise: AND16CELL · OR16CELL · XOR16CELL
   * (word dual of AND8/OR8/XOR8 after inverted8 plane; complete 8/16/32 bitwise ladder) */
  if (kw(&L->cur,"AND16CELL")||kw(&L->cur,"CELLAND16")||kw(&L->cur,"BAND16CELL")||
      kw(&L->cur,"RANGEAND16")||kw(&L->cur,"AND16RANGE")||kw(&L->cur,"UAND16CELL")){
    /* AND16CELL lo hi mask — cells[i] = low16(cells[i]) & low16(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFFFFu) & m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"OR16CELL")||kw(&L->cur,"CELLOR16")||kw(&L->cur,"BOR16CELL")||
      kw(&L->cur,"RANGEOR16")||kw(&L->cur,"OR16RANGE")||kw(&L->cur,"UOR16CELL")){
    /* OR16CELL lo hi mask — cells[i] = low16(cells[i]) | low16(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFFFFu) | m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XOR16CELL")||kw(&L->cur,"CELLXOR16")||kw(&L->cur,"BXOR16CELL")||
      kw(&L->cur,"RANGEXOR16")||kw(&L->cur,"XOR16RANGE")||kw(&L->cur,"UXOR16CELL")){
    /* XOR16CELL lo hi mask — cells[i] = low16(cells[i]) ^ low16(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFFFFu) ^ m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell fixed-width 16 inverted bitwise: NAND16CELL · NOR16CELL · XNOR16CELL
   * (inverted dual of AND16/OR16/XOR16; result low16 only — complete 8/16/32 inverted ladder) */
  if (kw(&L->cur,"NAND16CELL")||kw(&L->cur,"CELLNAND16")||kw(&L->cur,"BNAND16CELL")||
      kw(&L->cur,"RANGENAND16")||kw(&L->cur,"NAND16RANGE")||kw(&L->cur,"UNAND16CELL")){
    /* NAND16CELL lo hi mask — cells[i] = ~(low16 & low16(mask)) & 0xFFFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFFFFu) & m)) & 0xFFFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NOR16CELL")||kw(&L->cur,"CELLNOR16")||kw(&L->cur,"BNOR16CELL")||
      kw(&L->cur,"RANGENOR16")||kw(&L->cur,"NOR16RANGE")||kw(&L->cur,"UNOR16CELL")){
    /* NOR16CELL lo hi mask — cells[i] = ~(low16 | low16(mask)) & 0xFFFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFFFFu) | m)) & 0xFFFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XNOR16CELL")||kw(&L->cur,"CELLXNOR16")||kw(&L->cur,"BXNOR16CELL")||
      kw(&L->cur,"RANGEXNOR16")||kw(&L->cur,"XNOR16RANGE")||kw(&L->cur,"NXOR16CELL")||
      kw(&L->cur,"UXNOR16CELL")){
    /* XNOR16CELL lo hi mask — cells[i] = ~(low16 ^ low16(mask)) & 0xFFFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFFFFu) ^ m)) & 0xFFFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 cell fixed-width unary NOT: NOT8CELL · NOT16CELL · NOT32CELL
   * (width dual of NOTCELL; complete 8/16/32 bitwise unary after inverted ladder) */
  if (kw(&L->cur,"NOT8CELL")||kw(&L->cur,"CELLNOT8")||kw(&L->cur,"BNOT8CELL")||
      kw(&L->cur,"RANGENOT8")||kw(&L->cur,"NOT8RANGE")||kw(&L->cur,"INV8CELL")){
    /* NOT8CELL lo hi — cells[i] = ~low8 & 0xFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~((unsigned int)vm->cells[(int)i] & 0xFFu)) & 0xFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NOT16CELL")||kw(&L->cur,"CELLNOT16")||kw(&L->cur,"BNOT16CELL")||
      kw(&L->cur,"RANGENOT16")||kw(&L->cur,"NOT16RANGE")||kw(&L->cur,"INV16CELL")){
    /* NOT16CELL lo hi — cells[i] = ~low16 & 0xFFFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~((unsigned int)vm->cells[(int)i] & 0xFFFFu)) & 0xFFFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NOT32CELL")||kw(&L->cur,"CELLNOT32")||kw(&L->cur,"BNOT32CELL")||
      kw(&L->cur,"RANGENOT32")||kw(&L->cur,"NOT32RANGE")||kw(&L->cur,"INV32CELL")){
    /* NOT32CELL lo hi — cells[i] = ~low32 as u32 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(~(unsigned int)vm->cells[(int)i]);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
