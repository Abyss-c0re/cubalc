/* CubalC lang — lang_ops_cell_nibble4.c
 * CELL fixed-width 4 nibble plane: AND4CELL…CLIPS4CELL (G peel).
 * Mesh vote G_split_cell · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_nibble4.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_nibble4(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-4 cell fixed-width 4 nibble bitwise: AND4CELL · OR4CELL · XOR4CELL
   * (nibble dual of AND8/OR8/XOR8 after NOT width plane; complete 4/8/16/32 bitwise base) */
  if (kw(&L->cur,"AND4CELL")||kw(&L->cur,"CELLAND4")||kw(&L->cur,"BAND4CELL")||
      kw(&L->cur,"RANGEAND4")||kw(&L->cur,"AND4RANGE")||kw(&L->cur,"UAND4CELL")||
      kw(&L->cur,"ANDNIBCELL")){
    /* AND4CELL lo hi mask — cells[i] = low4(cells[i]) & low4(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFu) & m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"OR4CELL")||kw(&L->cur,"CELLOR4")||kw(&L->cur,"BOR4CELL")||
      kw(&L->cur,"RANGEOR4")||kw(&L->cur,"OR4RANGE")||kw(&L->cur,"UOR4CELL")||
      kw(&L->cur,"ORNIBCELL")){
    /* OR4CELL lo hi mask — cells[i] = low4(cells[i]) | low4(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFu) | m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XOR4CELL")||kw(&L->cur,"CELLXOR4")||kw(&L->cur,"BXOR4CELL")||
      kw(&L->cur,"RANGEXOR4")||kw(&L->cur,"XOR4RANGE")||kw(&L->cur,"UXOR4CELL")||
      kw(&L->cur,"XORNIBCELL")){
    /* XOR4CELL lo hi mask — cells[i] = low4(cells[i]) ^ low4(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFu) ^ m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 cell fixed-width 4 nibble inverted bitwise: NAND4CELL · NOR4CELL · XNOR4CELL
   * (inverted dual of AND4/OR4/XOR4; result low4 only — complete 4/8/16/32 inverted ladder) */
  if (kw(&L->cur,"NAND4CELL")||kw(&L->cur,"CELLNAND4")||kw(&L->cur,"BNAND4CELL")||
      kw(&L->cur,"RANGENAND4")||kw(&L->cur,"NAND4RANGE")||kw(&L->cur,"UNAND4CELL")||
      kw(&L->cur,"NANDNIBCELL")){
    /* NAND4CELL lo hi mask — cells[i] = ~(low4 & low4(mask)) & 0xF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFu) & m)) & 0xFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NOR4CELL")||kw(&L->cur,"CELLNOR4")||kw(&L->cur,"BNOR4CELL")||
      kw(&L->cur,"RANGENOR4")||kw(&L->cur,"NOR4RANGE")||kw(&L->cur,"UNOR4CELL")||
      kw(&L->cur,"NORNIBCELL")){
    /* NOR4CELL lo hi mask — cells[i] = ~(low4 | low4(mask)) & 0xF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFu) | m)) & 0xFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XNOR4CELL")||kw(&L->cur,"CELLXNOR4")||kw(&L->cur,"BXNOR4CELL")||
      kw(&L->cur,"RANGEXNOR4")||kw(&L->cur,"XNOR4RANGE")||kw(&L->cur,"NXOR4CELL")||
      kw(&L->cur,"UXNOR4CELL")||kw(&L->cur,"XNORNIBCELL")){
    /* XNOR4CELL lo hi mask — cells[i] = ~(low4 ^ low4(mask)) & 0xF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFu) ^ m)) & 0xFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell fixed-width 4 nibble shift ALU: SHL4CELL · SHR4CELL · SAR4CELL
   * (nibble dual of SHL8/SHR8/SAR8 after inverted nibble plane; complete 4/8/16/32 shift ladder) */
  if (kw(&L->cur,"SHL4CELL")||kw(&L->cur,"LSH4CELL")||kw(&L->cur,"BSHL4CELL")||
      kw(&L->cur,"RANGESHL4")||kw(&L->cur,"SHL4RANGE")||kw(&L->cur,"CELLLSH4")||
      kw(&L->cur,"SHLNIBCELL")){
    /* SHL4CELL lo hi k — cells[i] = (uint4)cells[i] << k (k>=4 → 0) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = (kk >= 4) ? 0L : (long)((w << kk) & 0xFu);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHR4CELL")||kw(&L->cur,"LSHR4CELL")||kw(&L->cur,"BSHR4CELL")||
      kw(&L->cur,"RANGESHR4")||kw(&L->cur,"SHR4RANGE")||kw(&L->cur,"CELLLSHR4")||
      kw(&L->cur,"SHRNIBCELL")){
    /* SHR4CELL lo hi k — cells[i] = (uint4)cells[i] >> k logical (k>=4 → 0) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = (kk >= 4) ? 0L : (long)(w >> kk);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SAR4CELL")||kw(&L->cur,"ASHR4CELL")||kw(&L->cur,"BSAR4CELL")||
      kw(&L->cur,"RANGESAR4")||kw(&L->cur,"SAR4RANGE")||kw(&L->cur,"CELLASHR4")||
      kw(&L->cur,"SARNIBCELL")){
    /* SAR4CELL lo hi k — arithmetic right shift low4 as int4 (k>=4 → all sign) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      long va = (long)((unsigned int)vm->cells[(int)i] & 0xFu);
      if (va & 0x8) va -= 16; /* sign-extend nibble → int4 [-8..7] */
      long r;
      if (kk >= 4) r = (va < 0) ? -1L : 0L;
      else r = va >> kk;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 cell fixed-width 4 nibble signed/metrics: SEXT4CELL · ABS4CELL · POPCNT4CELL
   * (nibble dual of SEXT8/ABS8/POPCNT8 after SAR4 plane; complete 4/8/16/32 signed+pop) */
  if (kw(&L->cur,"SEXT4CELL")||kw(&L->cur,"SIGNEXT4CELL")||kw(&L->cur,"BSEXT4CELL")||
      kw(&L->cur,"RANGESEXT4")||kw(&L->cur,"SEXT4RANGE")||kw(&L->cur,"CELLSEXTNIB")||
      kw(&L->cur,"SEXTNIBCELL")){
    /* SEXT4CELL lo hi — cells[i] = (long)(int4)low4 cells[i] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)((unsigned int)vm->cells[(int)i] & 0xFu);
      if (va & 0x8) va -= 16;
      vm->cells[(int)i] = va;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ABS4CELL")||kw(&L->cur,"CELLABS4")||kw(&L->cur,"BABS4CELL")||
      kw(&L->cur,"RANGEABS4")||kw(&L->cur,"ABS4RANGE")||kw(&L->cur,"IABS4CELL")||
      kw(&L->cur,"ABSNIBCELL")){
    /* ABS4CELL lo hi — cells[i] = abs((int4)low4); min int4 -8 stays 8 (0x8) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)((unsigned int)vm->cells[(int)i] & 0xFu);
      if (va & 0x8) va -= 16;
      long r;
      if (va == -8L) r = 8L; /* keep min int4 magnitude as 0x8 */
      else if (va < 0) r = -va;
      else r = va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"POPCNT4CELL")||kw(&L->cur,"PCNT4CELL")||kw(&L->cur,"BPOPCNT4CELL")||
      kw(&L->cur,"RANGEPOPCNT4")||kw(&L->cur,"POPCNT4RANGE")||kw(&L->cur,"CELLPOPCNT4")||
      kw(&L->cur,"POPCNTNIBCELL")){
    /* POPCNT4CELL lo hi — cells[i] = popcount(low4 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = 0;
      while (w){ r += (long)(w & 1u); w >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 cell fixed-width 4 nibble rotate/bitrev: ROL4CELL · ROR4CELL · BITREV4CELL
   * (nibble dual of ROL8/ROR8/BITREV8 after SEXT4/ABS4/POPCNT4; complete 4/8/16/32 rotate ladder) */
  if (kw(&L->cur,"ROL4CELL")||kw(&L->cur,"ROTL4CELL")||kw(&L->cur,"BROL4CELL")||
      kw(&L->cur,"RANGEROL4")||kw(&L->cur,"ROL4RANGE")||kw(&L->cur,"CELLROTL4")||
      kw(&L->cur,"ROLNIBCELL")){
    /* ROL4CELL lo hi k — cells[i] = rotl4(low4 cells[i], k&3) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 3;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = (kk == 0) ? (long)w : (long)((((w << kk) | (w >> (4 - kk))) & 0xFu));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ROR4CELL")||kw(&L->cur,"ROTR4CELL")||kw(&L->cur,"BROR4CELL")||
      kw(&L->cur,"RANGEROR4")||kw(&L->cur,"ROR4RANGE")||kw(&L->cur,"CELLROTR4")||
      kw(&L->cur,"RORNIBCELL")){
    /* ROR4CELL lo hi k — cells[i] = rotr4(low4 cells[i], k&3) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 3;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = (kk == 0) ? (long)w : (long)((((w >> kk) | (w << (4 - kk))) & 0xFu));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"BITREV4CELL")||kw(&L->cur,"BREV4CELL")||kw(&L->cur,"REV4CELL")||
      kw(&L->cur,"RANGEBITREV4")||kw(&L->cur,"BITREV4RANGE")||kw(&L->cur,"CELLBITREV4")||
      kw(&L->cur,"BITREVNIBCELL")){
    /* BITREV4CELL lo hi — cells[i] = bitrev4(low4 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      unsigned int rv = 0;
      for (int b = 0; b < 4; b++){
        rv = (rv << 1) | (w & 1u);
        w >>= 1;
      }
      vm->cells[(int)i] = (long)rv;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 cell fixed-width 4 nibble math: NEG4CELL · ZEXT4CELL · CLIP4CELL
   * (nibble dual of NEG8/ZEXT8/CLIP8 after ROL4 plane; complete 4/8/16/32 arithmetic foundation) */
  if (kw(&L->cur,"NEG4CELL")||kw(&L->cur,"CELLNEG4")||kw(&L->cur,"BNEG4CELL")||
      kw(&L->cur,"RANGENEG4")||kw(&L->cur,"NEG4RANGE")||kw(&L->cur,"INEG4CELL")||
      kw(&L->cur,"NEGNIBCELL")){
    /* NEG4CELL lo hi — cells[i] = -(int4)low4; min int4 -8 stays -8 (0x8 as -8 after sext) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)((unsigned int)vm->cells[(int)i] & 0xFu);
      if (va & 0x8) va -= 16;
      long r;
      if (va == -8L) r = -8L; /* keep min int4 */
      else r = -va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ZEXT4CELL")||kw(&L->cur,"ZEROEXT4CELL")||kw(&L->cur,"BZEXT4CELL")||
      kw(&L->cur,"RANGEZEXT4")||kw(&L->cur,"ZEXT4RANGE")||kw(&L->cur,"CELLZEXTNIB")||
      kw(&L->cur,"ZEXTNIBCELL")){
    /* ZEXT4CELL lo hi — cells[i] = low4 only (clear high bits) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((unsigned long)vm->cells[(int)i] & 0xFul);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLIP4CELL")||kw(&L->cur,"CELLCLIP4")||kw(&L->cur,"BCLIP4CELL")||
      kw(&L->cur,"RANGECLIP4")||kw(&L->cur,"CLIP4RANGE")||kw(&L->cur,"UCLIP4CELL")||
      kw(&L->cur,"CLIPNIBCELL")){
    /* CLIP4CELL lo hi — clamp cells[i] to unsigned 4-bit [0,15] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < 0) r = 0;
      if (r > 15) r = 15;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 cell fixed-width 4 nibble metrics foundation: CLZ4CELL · CTZ4CELL · PARITY4CELL
   * (nibble dual of CLZ8/CTZ8/PARITY8 after NEG4/ZEXT4/CLIP4; complete 4/8/16/32 metrics) */
  if (kw(&L->cur,"CLZ4CELL")||kw(&L->cur,"NLZ4CELL")||kw(&L->cur,"BCLZ4CELL")||
      kw(&L->cur,"RANGECLZ4")||kw(&L->cur,"CLZ4RANGE")||kw(&L->cur,"CELLCLZ4")||
      kw(&L->cur,"CLZNIBCELL")){
    /* CLZ4CELL lo hi — cells[i] = clz4(low4); 0 → 4 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = 0;
      if (w == 0) r = 4;
      else {
        for (int b = 3; b >= 0; b--){
          if (w & (1u << (unsigned)b)) break;
          r++;
        }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CTZ4CELL")||kw(&L->cur,"NTZ4CELL")||kw(&L->cur,"BCTZ4CELL")||
      kw(&L->cur,"RANGECTZ4")||kw(&L->cur,"CTZ4RANGE")||kw(&L->cur,"CELLCTZ4")||
      kw(&L->cur,"CTZNIBCELL")){
    /* CTZ4CELL lo hi — cells[i] = ctz4(low4); 0 → 4 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = 0;
      if (w == 0) r = 4;
      else {
        while ((w & 1u) == 0){ r++; w >>= 1; }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"PARITY4CELL")||kw(&L->cur,"CELLPARITY4")||kw(&L->cur,"BPARITY4CELL")||
      kw(&L->cur,"RANGEPARITY4")||kw(&L->cur,"PARITY4RANGE")||kw(&L->cur,"XORRED4CELL")||
      kw(&L->cur,"PARITYNIBCELL")){
    /* PARITY4CELL lo hi — cells[i] = xor-reduce of low4 bits (parity) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = 0;
      while (w){ r ^= (long)(w & 1u); w >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 cell fixed-width 4 nibble unary/signed: NOT4CELL · CLIPS4CELL
   * (nibble dual of NOT8 + CLIPS8 after metrics plane; complete 4/8/16/32 unary+sclip) */
  if (kw(&L->cur,"NOT4CELL")||kw(&L->cur,"CELLNOT4")||kw(&L->cur,"BNOT4CELL")||
      kw(&L->cur,"RANGENOT4")||kw(&L->cur,"NOT4RANGE")||kw(&L->cur,"INV4CELL")||
      kw(&L->cur,"NOTNIBCELL")){
    /* NOT4CELL lo hi — cells[i] = ~low4 & 0xF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~((unsigned int)vm->cells[(int)i] & 0xFu)) & 0xFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLIPS4CELL")||kw(&L->cur,"SCLIPS4CELL")||kw(&L->cur,"BCLIPS4CELL")||
      kw(&L->cur,"RANGECLIPS4")||kw(&L->cur,"CLIPS4RANGE")||kw(&L->cur,"SCLIP4CELL")||
      kw(&L->cur,"CLIPSNIBCELL")){
    /* CLIPS4CELL lo hi — clamp cells[i] to signed 4-bit [-8,7] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < -8L) r = -8L;
      if (r > 7L) r = 7L;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
