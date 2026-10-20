/* CubalC lang — lang_ops_cell_width8.c
 * CELL fixed-width 8 ALU: SHL8CELL…CLIP8CELL (G peel).
 * Mesh vote G_split_cell · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_width8.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_width8(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-1 cell fixed-width 8 shift ALU: SHL8CELL · SHR8CELL · SAR8CELL
   * (byte-width dual of SHL16/SHR16/SAR16 after 16-metrics plane; complete 8/16/32 shift ladder) */
  if (kw(&L->cur,"SHL8CELL")||kw(&L->cur,"LSH8CELL")||kw(&L->cur,"BSHL8CELL")||
      kw(&L->cur,"RANGESHL8")||kw(&L->cur,"SHL8RANGE")||kw(&L->cur,"CELLLSH8")){
    /* SHL8CELL lo hi k — cells[i] = (uint8)cells[i] << k (k>=8 → 0) */
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
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = (kk >= 8) ? 0L : (long)((w << kk) & 0xFFu);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHR8CELL")||kw(&L->cur,"LSHR8CELL")||kw(&L->cur,"BSHR8CELL")||
      kw(&L->cur,"RANGESHR8")||kw(&L->cur,"SHR8RANGE")||kw(&L->cur,"CELLLSHR8")){
    /* SHR8CELL lo hi k — cells[i] = (uint8)cells[i] >> k logical (k>=8 → 0) */
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
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = (kk >= 8) ? 0L : (long)(w >> kk);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SAR8CELL")||kw(&L->cur,"ASHR8CELL")||kw(&L->cur,"BSAR8CELL")||
      kw(&L->cur,"RANGESAR8")||kw(&L->cur,"SAR8RANGE")||kw(&L->cur,"CELLASHR8")){
    /* SAR8CELL lo hi k — arithmetic right shift low8 as int8 (k>=8 → all sign) */
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
      long va = (long)(signed char)(unsigned char)(vm->cells[(int)i] & 0xFFu);
      long r;
      if (kk >= 8) r = (va < 0) ? -1L : 0L;
      else r = va >> kk;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 cell fixed-width 8 rotate/bitrev: ROL8CELL · ROR8CELL · BITREV8CELL
   * (byte dual of ROL16/ROR16/BITREV16 after SHL8 plane; complete 8/16/32 rotate+bitrev ladder) */
  if (kw(&L->cur,"ROL8CELL")||kw(&L->cur,"ROTL8CELL")||kw(&L->cur,"BROL8CELL")||
      kw(&L->cur,"RANGEROL8")||kw(&L->cur,"ROL8RANGE")||kw(&L->cur,"CELLROTL8")){
    /* ROL8CELL lo hi k — cells[i] = rotl8(low8 cells[i], k&7) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 7;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = (kk == 0) ? (long)w : (long)((((w << kk) | (w >> (8 - kk))) & 0xFFu));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ROR8CELL")||kw(&L->cur,"ROTR8CELL")||kw(&L->cur,"BROR8CELL")||
      kw(&L->cur,"RANGEROR8")||kw(&L->cur,"ROR8RANGE")||kw(&L->cur,"CELLROTR8")){
    /* ROR8CELL lo hi k — cells[i] = rotr8(low8 cells[i], k&7) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 7;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = (kk == 0) ? (long)w : (long)((((w >> kk) | (w << (8 - kk))) & 0xFFu));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"BITREV8CELL")||kw(&L->cur,"BREV8CELL")||kw(&L->cur,"REV8CELL")||
      kw(&L->cur,"RANGEBITREV8")||kw(&L->cur,"BITREV8RANGE")||kw(&L->cur,"CELLBITREV8R")){
    /* BITREV8CELL lo hi — cells[i] = bitrev8(low8 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      unsigned int rv = 0;
      for (int b = 0; b < 8; b++){
        rv = (rv << 1) | (w & 1u);
        w >>= 1;
      }
      vm->cells[(int)i] = (long)rv;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 cell fixed-width 8 signed/metrics: SEXT8CELL · ABS8CELL · POPCNT8CELL
   * (byte dual of SEXT16/ABS16/POPCNT16 after ROL8 plane; complete signed8 + energy count) */
  if (kw(&L->cur,"SEXT8CELL")||kw(&L->cur,"SIGNEXT8CELL")||kw(&L->cur,"BSEXT8CELL")||
      kw(&L->cur,"RANGESEXT8")||kw(&L->cur,"SEXT8RANGE")||kw(&L->cur,"CELLSEXTB")){
    /* SEXT8CELL lo hi — cells[i] = (long)(int8)low8 cells[i] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = (long)(signed char)(unsigned char)(vm->cells[(int)i] & 0xFFu);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ABS8CELL")||kw(&L->cur,"CELLABS8")||kw(&L->cur,"BABS8CELL")||
      kw(&L->cur,"RANGEABS8")||kw(&L->cur,"ABS8RANGE")||kw(&L->cur,"IABS8CELL")){
    /* ABS8CELL lo hi — cells[i] = abs((int8)low8); min int8 stays 0x80 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)(signed char)(unsigned char)(vm->cells[(int)i] & 0xFFu);
      long r;
      if (va == (long)(signed char)0x80) r = 0x80L; /* keep min int8 */
      else if (va < 0) r = -va;
      else r = va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"POPCNT8CELL")||kw(&L->cur,"PCNT8CELL")||kw(&L->cur,"BPOPCNT8CELL")||
      kw(&L->cur,"RANGEPOPCNT8")||kw(&L->cur,"POPCNT8RANGE")||kw(&L->cur,"CELLPOPCNT8")){
    /* POPCNT8CELL lo hi — cells[i] = popcount(low8 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = 0;
      while (w){ r += (long)(w & 1u); w >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 cell fixed-width 8 metrics foundation: CLZ8CELL · CTZ8CELL · PARITY8CELL
   * (byte dual of CLZ16/CTZ16 + parity after POPCNT8; complete 8-bit metrics foundation) */
  if (kw(&L->cur,"CLZ8CELL")||kw(&L->cur,"NLZ8CELL")||kw(&L->cur,"BCLZ8CELL")||
      kw(&L->cur,"RANGECLZ8")||kw(&L->cur,"CLZ8RANGE")||kw(&L->cur,"CELLCLZ8")){
    /* CLZ8CELL lo hi — cells[i] = clz8(low8); 0 → 8 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = 0;
      if (w == 0) r = 8;
      else {
        for (int b = 7; b >= 0; b--){
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
  if (kw(&L->cur,"CTZ8CELL")||kw(&L->cur,"NTZ8CELL")||kw(&L->cur,"BCTZ8CELL")||
      kw(&L->cur,"RANGECTZ8")||kw(&L->cur,"CTZ8RANGE")||kw(&L->cur,"CELLCTZ8")){
    /* CTZ8CELL lo hi — cells[i] = ctz8(low8); 0 → 8 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = 0;
      if (w == 0) r = 8;
      else {
        while ((w & 1u) == 0){ r++; w >>= 1; }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"PARITY8CELL")||kw(&L->cur,"CELLPARITY8")||kw(&L->cur,"BPARITY8CELL")||
      kw(&L->cur,"RANGEPARITY8")||kw(&L->cur,"PARITY8RANGE")||kw(&L->cur,"XORRED8CELL")){
    /* PARITY8CELL lo hi — cells[i] = xor-reduce of low8 bits (parity) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = 0;
      while (w){ r ^= (long)(w & 1u); w >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 cell fixed-width 8 math: NEG8CELL · ZEXT8CELL · CLIP8CELL
   * (byte dual of signed/zext/clip after CLZ8 metrics; complete 8-bit arithmetic foundation) */
  if (kw(&L->cur,"NEG8CELL")||kw(&L->cur,"CELLNEG8")||kw(&L->cur,"BNEG8CELL")||
      kw(&L->cur,"RANGENEG8")||kw(&L->cur,"NEG8RANGE")||kw(&L->cur,"INEG8CELL")){
    /* NEG8CELL lo hi — cells[i] = -(int8)low8; min int8 0x80 stays 0x80 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)(signed char)(unsigned char)(vm->cells[(int)i] & 0xFFu);
      long r;
      if (va == (long)(signed char)0x80) r = 0x80L;
      else r = -va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ZEXT8CELL")||kw(&L->cur,"ZEROEXT8CELL")||kw(&L->cur,"BZEXT8CELL")||
      kw(&L->cur,"RANGEZEXT8")||kw(&L->cur,"ZEXT8RANGE")||kw(&L->cur,"CELLZEXTB")){
    /* ZEXT8CELL lo hi — cells[i] = low8 only (clear high bits) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((unsigned long)vm->cells[(int)i] & 0xFFul);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLIP8CELL")||kw(&L->cur,"CELLCLIP8")||kw(&L->cur,"BCLIP8CELL")||
      kw(&L->cur,"RANGECLIP8")||kw(&L->cur,"CLIP8RANGE")||kw(&L->cur,"UCLIP8CELL")){
    /* CLIP8CELL lo hi — clamp cells[i] to unsigned 8-bit [0,255] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < 0) r = 0;
      if (r > 255) r = 255;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
