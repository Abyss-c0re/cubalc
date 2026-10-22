/* CubalC lang — CELL width16 rot/bitrev + bswap64 peel (G_cell_margin).
 * BSWAP64…SLT/SMUX · SANY/SEQRANGES · SFINDCELL/SFINDLAST/SFIRSTNZ/SLASTNZ (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_w16rot.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_w16rot(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  if (kw(&L->cur,"BSWAP64CELL")||kw(&L->cur,"CELLBSWAP64")||kw(&L->cur,"BBSWAP64CELL")||
      kw(&L->cur,"RANGEBSWAP64")||kw(&L->cur,"BSWAP64RANGE")){
    /* BSWAP64CELL lo hi — cells[i] = bswap64(cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned long w = (unsigned long)vm->cells[(int)i];
      w = ((w & 0x00000000000000FFul) << 56) | ((w & 0x000000000000FF00ul) << 40) |
          ((w & 0x0000000000FF0000ul) << 24) | ((w & 0x00000000FF000000ul) << 8) |
          ((w & 0x000000FF00000000ul) >> 8) | ((w & 0x0000FF0000000000ul) >> 24) |
          ((w & 0x00FF000000000000ul) >> 40) | ((w & 0xFF00000000000000ul) >> 56);
      vm->cells[(int)i] = (long)w;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"BITREV16CELL")||kw(&L->cur,"BREV16CELL")||kw(&L->cur,"REV16CELL")||
      kw(&L->cur,"RANGEBITREV16")||kw(&L->cur,"BITREV16RANGE")||kw(&L->cur,"CELLBITREV16R")){
    /* BITREV16CELL lo hi — cells[i] = bitrev16(low16 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      unsigned int rv = 0;
      for (int b = 0; b < 16; b++){
        rv = (rv << 1) | (w & 1u);
        w >>= 1;
      }
      vm->cells[(int)i] = (long)rv;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ROL16CELL")||kw(&L->cur,"ROTL16CELL")||kw(&L->cur,"BROL16CELL")||
      kw(&L->cur,"RANGEROL16")||kw(&L->cur,"ROL16RANGE")||kw(&L->cur,"CELLROTL16")){
    /* ROL16CELL lo hi k — cells[i] = rotl16(low16 cells[i], k&15) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 15;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      long r = (kk == 0) ? (long)w : (long)((((w << kk) | (w >> (16 - kk))) & 0xFFFFu));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 cell fixed-width 16 shift/rotate peel (ROR16·SHL16·SHR16) — G_cell_margin */
  /* digit-1 cell fixed-width 16 shift/rotate: ROR16CELL · SHL16CELL · SHR16CELL
   * (complete 16-bit rotate + logical shift plane after ROL16CELL; dual of SROTR16/SSHL path) */
  if (kw(&L->cur,"ROR16CELL")||kw(&L->cur,"ROTR16CELL")||kw(&L->cur,"BROR16CELL")||
      kw(&L->cur,"RANGEROR16")||kw(&L->cur,"ROR16RANGE")||kw(&L->cur,"CELLROTR16")){
    /* ROR16CELL lo hi k — cells[i] = rotr16(low16 cells[i], k&15) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 15;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      long r = (kk == 0) ? (long)w : (long)((((w >> kk) | (w << (16 - kk))) & 0xFFFFu));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHL16CELL")||kw(&L->cur,"LSH16CELL")||kw(&L->cur,"BSHL16CELL")||
      kw(&L->cur,"RANGESHL16")||kw(&L->cur,"SHL16RANGE")||kw(&L->cur,"CELLLSH16")){
    /* SHL16CELL lo hi k — cells[i] = (uint16)cells[i] << k (k>=16 → 0) */
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
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      long r = (kk >= 16) ? 0L : (long)((w << kk) & 0xFFFFu);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHR16CELL")||kw(&L->cur,"LSHR16CELL")||kw(&L->cur,"BSHR16CELL")||
      kw(&L->cur,"RANGESHR16")||kw(&L->cur,"SHR16RANGE")||kw(&L->cur,"CELLLSHR16")){
    /* SHR16CELL lo hi k — cells[i] = (uint16)cells[i] >> k logical (k>=16 → 0) */
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
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      long r = (kk >= 16) ? 0L : (long)(w >> kk);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 cell signed16 peel (SAR16·SEXT16·ABS16) — G_cell_margin */
  /* digit-2 cell fixed-width 16 signed/bitfield: SAR16CELL · SEXT16CELL · ABS16CELL
   * (complete arithmetic shift + sign-extend + abs16 after logical SHL16/SHR16 plane) */
  if (kw(&L->cur,"SAR16CELL")||kw(&L->cur,"ASHR16CELL")||kw(&L->cur,"BSAR16CELL")||
      kw(&L->cur,"RANGESAR16")||kw(&L->cur,"SAR16RANGE")||kw(&L->cur,"CELLASHR16")){
    /* SAR16CELL lo hi k — arithmetic right shift low16 as int16 (k>=16 → all sign) */
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
      long va = (long)(short)(unsigned short)(vm->cells[(int)i] & 0xFFFFu);
      long r;
      if (kk >= 16) r = (va < 0) ? -1L : 0L;
      else r = va >> kk;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SEXT16CELL")||kw(&L->cur,"SIGNEXT16CELL")||kw(&L->cur,"BSEXT16CELL")||
      kw(&L->cur,"RANGESEXT16")||kw(&L->cur,"SEXT16RANGE")||kw(&L->cur,"CELLSEXTW")){
    /* SEXT16CELL lo hi — cells[i] = (long)(int16)low16 cells[i] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = (long)(short)(unsigned short)(vm->cells[(int)i] & 0xFFFFu);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ABS16CELL")||kw(&L->cur,"CELLABS16")||kw(&L->cur,"BABS16CELL")||
      kw(&L->cur,"RANGEABS16")||kw(&L->cur,"ABS16RANGE")||kw(&L->cur,"IABS16CELL")){
    /* ABS16CELL lo hi — cells[i] = abs((int16)low16); min int16 stays 0x8000 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)(short)(unsigned short)(vm->cells[(int)i] & 0xFFFFu);
      long r;
      if (va == (long)(short)0x8000) r = 0x8000L; /* keep min int16 */
      else if (va < 0) r = -va;
      else r = va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell metrics16 peel (POPCNT16·CLZ16·CTZ16) — G_cell_margin */
  /* digit-9 cell fixed-width 16 bit-metrics: POPCNT16CELL · CLZ16CELL · CTZ16CELL
   * (width-16 dual of POPCNTCELL/CLZCELL/CTZCELL after SAR16/SEXT16/ABS16 plane) */
  if (kw(&L->cur,"POPCNT16CELL")||kw(&L->cur,"PCNT16CELL")||kw(&L->cur,"BPOPCNT16CELL")||
      kw(&L->cur,"RANGEPOPCNT16")||kw(&L->cur,"POPCNT16RANGE")||kw(&L->cur,"CELLPOPCNT16")){
    /* POPCNT16CELL lo hi — cells[i] = popcount(low16 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      long r = 0;
      while (w){ r += (long)(w & 1u); w >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLZ16CELL")||kw(&L->cur,"NLZ16CELL")||kw(&L->cur,"BCLZ16CELL")||
      kw(&L->cur,"RANGECLZ16")||kw(&L->cur,"CLZ16RANGE")||kw(&L->cur,"CELLCLZ16")){
    /* CLZ16CELL lo hi — cells[i] = clz16(low16); 0 → 16 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      long r = 0;
      if (w == 0) r = 16;
      else {
        for (int b = 15; b >= 0; b--){
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
  if (kw(&L->cur,"CTZ16CELL")||kw(&L->cur,"NTZ16CELL")||kw(&L->cur,"BCTZ16CELL")||
      kw(&L->cur,"RANGECTZ16")||kw(&L->cur,"CTZ16RANGE")||kw(&L->cur,"CELLCTZ16")){
    /* CTZ16CELL lo hi — cells[i] = ctz16(low16); 0 → 16 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      long r = 0;
      if (w == 0) r = 16;
      else {
        while ((w & 1u) == 0){ r++; w >>= 1; }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 cell fixed-width 16 math peel (NEG16·ZEXT16·CLIP16) — G_cell_margin */
  /* digit-7 cell fixed-width 16 math: NEG16CELL · ZEXT16CELL · CLIP16CELL
   * (word dual of NEG8/ZEXT8/CLIP8 after SAR16/SEXT16/ABS16 plane; complete 16-bit math) */
  if (kw(&L->cur,"NEG16CELL")||kw(&L->cur,"CELLNEG16")||kw(&L->cur,"BNEG16CELL")||
      kw(&L->cur,"RANGENEG16")||kw(&L->cur,"NEG16RANGE")||kw(&L->cur,"INEG16CELL")){
    /* NEG16CELL lo hi — cells[i] = -(int16)low16; min int16 0x8000 stays 0x8000 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)(short)(unsigned short)(vm->cells[(int)i] & 0xFFFFu);
      long r;
      if (va == (long)(short)0x8000) r = 0x8000L;
      else r = -va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ZEXT16CELL")||kw(&L->cur,"ZEROEXT16CELL")||kw(&L->cur,"BZEXT16CELL")||
      kw(&L->cur,"RANGEZEXT16")||kw(&L->cur,"ZEXT16RANGE")||kw(&L->cur,"CELLZEXTW")){
    /* ZEXT16CELL lo hi — cells[i] = low16 only (clear high bits) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((unsigned long)vm->cells[(int)i] & 0xFFFFul);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLIP16CELL")||kw(&L->cur,"CELLCLIP16")||kw(&L->cur,"BCLIP16CELL")||
      kw(&L->cur,"RANGECLIP16")||kw(&L->cur,"CLIP16RANGE")||kw(&L->cur,"UCLIP16CELL")){
    /* CLIP16CELL lo hi — clamp cells[i] to unsigned 16-bit [0,65535] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < 0) r = 0;
      if (r > 65535L) r = 65535L;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 cell signed-clip peel (CLIPS8·CLIPS16·CLIPS32) — G_cell_margin */
  /* digit-3 cell signed-clip dual ladder: CLIPS8CELL · CLIPS16CELL · CLIPS32CELL
   * (signed dual of CLIP8/CLIP16 after SCLIPS*TOC; complete 8/16/32 signed clamp plane) */
  if (kw(&L->cur,"CLIPS8CELL")||kw(&L->cur,"SCLIPS8CELL")||kw(&L->cur,"BCLIPS8CELL")||
      kw(&L->cur,"RANGECLIPS8")||kw(&L->cur,"CLIPS8RANGE")||kw(&L->cur,"SCLIP8CELL")){
    /* CLIPS8CELL lo hi — clamp cells[i] to signed 8-bit [-128,127] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < -128L) r = -128L;
      if (r > 127L) r = 127L;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLIPS16CELL")||kw(&L->cur,"SCLIPS16CELL")||kw(&L->cur,"BCLIPS16CELL")||
      kw(&L->cur,"RANGECLIPS16")||kw(&L->cur,"CLIPS16RANGE")||kw(&L->cur,"SCLIP16CELL")){
    /* CLIPS16CELL lo hi — clamp cells[i] to signed 16-bit [-32768,32767] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < -32768L) r = -32768L;
      if (r > 32767L) r = 32767L;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLIPS32CELL")||kw(&L->cur,"SCLIPS32CELL")||kw(&L->cur,"BCLIPS32CELL")||
      kw(&L->cur,"RANGECLIPS32")||kw(&L->cur,"CLIPS32RANGE")||kw(&L->cur,"SCLIP32CELL")){
    /* CLIPS32CELL lo hi — clamp cells[i] to signed 32-bit [-2147483648,2147483647] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < -2147483648L) r = -2147483648L;
      if (r > 2147483647L) r = 2147483647L;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 cell fixed-width 32 math peel (NEG32·ZEXT32·CLIP32) — G_cell_margin */
  /* digit-3 cell fixed-width 32 math dual ladder: NEG32CELL · ZEXT32CELL · CLIP32CELL
   * (dword dual of NEG16/ZEXT16/CLIP16 after CLIPS32 plane; complete 8/16/32 math) */
  if (kw(&L->cur,"NEG32CELL")||kw(&L->cur,"CELLNEG32")||kw(&L->cur,"BNEG32CELL")||
      kw(&L->cur,"RANGENEG32")||kw(&L->cur,"NEG32RANGE")||kw(&L->cur,"INEG32CELL")){
    /* NEG32CELL lo hi — cells[i] = -(int32)low32; min int32 stays 0x80000000 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)(int)(unsigned int)vm->cells[(int)i];
      long r;
      if (va == (long)(int)0x80000000) r = (long)(int)0x80000000;
      else r = -va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ZEXT32CELL")||kw(&L->cur,"ZEROEXT32CELL")||kw(&L->cur,"BZEXT32CELL")||
      kw(&L->cur,"RANGEZEXT32")||kw(&L->cur,"ZEXT32RANGE")||kw(&L->cur,"CELLZEXTL")){
    /* ZEXT32CELL lo hi — cells[i] = low32 only (clear high bits) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((unsigned long)vm->cells[(int)i] & 0xFFFFFFFFul);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLIP32CELL")||kw(&L->cur,"CELLCLIP32")||kw(&L->cur,"BCLIP32CELL")||
      kw(&L->cur,"RANGECLIP32")||kw(&L->cur,"CLIP32RANGE")||kw(&L->cur,"UCLIP32CELL")){
    /* CLIP32CELL lo hi — clamp cells[i] to unsigned 32-bit [0,4294967295] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < 0) r = 0;
      if (r > 4294967295L) r = 4294967295L;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 cell fixed-width 32 signed/metrics peel (SEXT32·ABS32·POPCNT32) — G_cell_margin */
  /* digit-4 cell fixed-width 32 signed/metrics: SEXT32CELL · ABS32CELL · POPCNT32CELL
   * (dword dual of SEXT8/ABS8/POPCNT8 after NEG32/ZEXT32/CLIP32; complete 8/16/32 signed+pop) */
  if (kw(&L->cur,"SEXT32CELL")||kw(&L->cur,"SIGNEXT32CELL")||kw(&L->cur,"BSEXT32CELL")||
      kw(&L->cur,"RANGESEXT32")||kw(&L->cur,"SEXT32RANGE")||kw(&L->cur,"CELLSEXTL")){
    /* SEXT32CELL lo hi — cells[i] = (long)(int32)low32 cells[i] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = (long)(int)(unsigned int)vm->cells[(int)i];
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ABS32CELL")||kw(&L->cur,"CELLABS32")||kw(&L->cur,"BABS32CELL")||
      kw(&L->cur,"RANGEABS32")||kw(&L->cur,"ABS32RANGE")||kw(&L->cur,"IABS32CELL")){
    /* ABS32CELL lo hi — cells[i] = abs((int32)low32); min int32 stays 0x80000000 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)(int)(unsigned int)vm->cells[(int)i];
      long r;
      if (va == (long)(int)0x80000000) r = (long)(int)0x80000000; /* keep min int32 */
      else if (va < 0) r = -va;
      else r = va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"POPCNT32CELL")||kw(&L->cur,"PCNT32CELL")||kw(&L->cur,"BPOPCNT32CELL")||
      kw(&L->cur,"RANGEPOPCNT32")||kw(&L->cur,"POPCNT32RANGE")||kw(&L->cur,"CELLPOPCNT32")){
    /* POPCNT32CELL lo hi — cells[i] = popcount(low32 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      long r = 0;
      while (w){ r += (long)(w & 1u); w >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 cell fixed-width 32 metrics peel (CLZ32·CTZ32·PARITY32) — G_cell_margin */
  /* digit-5 cell fixed-width 32 metrics foundation: CLZ32CELL · CTZ32CELL · PARITY32CELL
   * (dword dual of CLZ8/CTZ8/PARITY8 after POPCNT32; complete 8/16/32 metrics ladder) */
  if (kw(&L->cur,"CLZ32CELL")||kw(&L->cur,"NLZ32CELL")||kw(&L->cur,"BCLZ32CELL")||
      kw(&L->cur,"RANGECLZ32")||kw(&L->cur,"CLZ32RANGE")||kw(&L->cur,"CELLCLZ32")){
    /* CLZ32CELL lo hi — cells[i] = clz32(low32); 0 → 32 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      long r = 0;
      if (w == 0) r = 32;
      else {
        for (int b = 31; b >= 0; b--){
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
  if (kw(&L->cur,"CTZ32CELL")||kw(&L->cur,"NTZ32CELL")||kw(&L->cur,"BCTZ32CELL")||
      kw(&L->cur,"RANGECTZ32")||kw(&L->cur,"CTZ32RANGE")||kw(&L->cur,"CELLCTZ32")){
    /* CTZ32CELL lo hi — cells[i] = ctz32(low32); 0 → 32 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      long r = 0;
      if (w == 0) r = 32;
      else {
        while ((w & 1u) == 0){ r++; w >>= 1; }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"PARITY32CELL")||kw(&L->cur,"CELLPARITY32")||kw(&L->cur,"BPARITY32CELL")||
      kw(&L->cur,"RANGEPARITY32")||kw(&L->cur,"PARITY32RANGE")||kw(&L->cur,"XORRED32CELL")){
    /* PARITY32CELL lo hi — cells[i] = xor-reduce of low32 bits (parity) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      long r = 0;
      while (w){ r ^= (long)(w & 1u); w >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 cell fixed-width 32 bitwise peel (AND32·OR32·XOR32) — G_cell_margin */
  /* digit-3 cell fixed-width 32 bitwise: AND32CELL · OR32CELL · XOR32CELL
   * (dword dual of ANDCELL/ORCELL/XORCELL; result low32 only after 32 metrics plane) */
  if (kw(&L->cur,"AND32CELL")||kw(&L->cur,"CELLAND32")||kw(&L->cur,"BAND32CELL")||
      kw(&L->cur,"RANGEAND32")||kw(&L->cur,"AND32RANGE")||kw(&L->cur,"UAND32CELL")){
    /* AND32CELL lo hi mask — cells[i] = low32(cells[i]) & low32(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((unsigned int)vm->cells[(int)i] & m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"OR32CELL")||kw(&L->cur,"CELLOR32")||kw(&L->cur,"BOR32CELL")||
      kw(&L->cur,"RANGEOR32")||kw(&L->cur,"OR32RANGE")||kw(&L->cur,"UOR32CELL")){
    /* OR32CELL lo hi mask — cells[i] = low32(cells[i]) | low32(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((unsigned int)vm->cells[(int)i] | m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XOR32CELL")||kw(&L->cur,"CELLXOR32")||kw(&L->cur,"BXOR32CELL")||
      kw(&L->cur,"RANGEXOR32")||kw(&L->cur,"XOR32RANGE")||kw(&L->cur,"UXOR32CELL")){
    /* XOR32CELL lo hi mask — cells[i] = low32(cells[i]) ^ low32(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((unsigned int)vm->cells[(int)i] ^ m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 cell fixed-width 32 inverted bitwise peel (NAND32·NOR32·XNOR32) — G_cell_margin */
  /* digit-4 cell fixed-width 32 inverted bitwise: NAND32CELL · NOR32CELL · XNOR32CELL
   * (inverted dual of AND32/OR32/XOR32; result low32 only — complete 32 cell logic plane) */
  if (kw(&L->cur,"NAND32CELL")||kw(&L->cur,"CELLNAND32")||kw(&L->cur,"BNAND32CELL")||
      kw(&L->cur,"RANGENAND32")||kw(&L->cur,"NAND32RANGE")||kw(&L->cur,"UNAND32CELL")){
    /* NAND32CELL lo hi mask — cells[i] = ~(low32 & low32(mask)) & 0xFFFFFFFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(~((unsigned int)vm->cells[(int)i] & m));
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NOR32CELL")||kw(&L->cur,"CELLNOR32")||kw(&L->cur,"BNOR32CELL")||
      kw(&L->cur,"RANGENOR32")||kw(&L->cur,"NOR32RANGE")||kw(&L->cur,"UNOR32CELL")){
    /* NOR32CELL lo hi mask — cells[i] = ~(low32 | low32(mask)) & 0xFFFFFFFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(~((unsigned int)vm->cells[(int)i] | m));
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XNOR32CELL")||kw(&L->cur,"CELLXNOR32")||kw(&L->cur,"BXNOR32CELL")||
      kw(&L->cur,"RANGEXNOR32")||kw(&L->cur,"XNOR32RANGE")||kw(&L->cur,"NXOR32CELL")||
      kw(&L->cur,"UXNOR32CELL")){
    /* XNOR32CELL lo hi mask — cells[i] = ~(low32 ^ low32(mask)) & 0xFFFFFFFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(~((unsigned int)vm->cells[(int)i] ^ m));
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* cell NOT/EQ/NE peel (NOTCELL·EQCELL·NECELL) — G_cell_margin */
  if (kw(&L->cur,"NOTCELL")||kw(&L->cur,"CELLNOT")||kw(&L->cur,"BNOTCELL")||kw(&L->cur,"INVCELL")){
    /* NOTCELL lo hi — bitwise NOT (~) each cell in range */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] = ~vm->cells[(int)i];
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"EQCELL")||kw(&L->cur,"CELLEQ")||kw(&L->cur,"CMPEQCELL")){
    /* EQCELL lo hi val — set cell to 1 if == val else 0 (predicate mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long val = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long hits = 0;
    for (long i=lo;i<=hi;i++){
      long eq = (vm->cells[(int)i] == val) ? 1 : 0;
      vm->cells[(int)i] = eq;
      hits += eq;
    }
    var_set_num(vm,"LAST_N",hits); vm->last_n=hits;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NECELL")||kw(&L->cur,"CELLNE")||kw(&L->cur,"CMPNECELL")||
      kw(&L->cur,"NOTEQCELL")){
    /* NECELL lo hi val — set cell to 1 if != val else 0 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long val = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long hits = 0;
    for (long i=lo;i<=hi;i++){
      long ne = (vm->cells[(int)i] != val) ? 1 : 0;
      vm->cells[(int)i] = ne;
      hits += ne;
    }
    var_set_num(vm,"LAST_N",hits); vm->last_n=hits;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* cell relational+mux peel (LT/GT/LE/GE · MUXCELL) — G_cell_margin */
  /* digit-5 cell relational predicates: LTCELL GTCELL LECELL GECELL (complete EQ/NE plane) */
  if (kw(&L->cur,"LTCELL")||kw(&L->cur,"CELLLT")||kw(&L->cur,"CMPLTCELL")||
      kw(&L->cur,"GTCELL")||kw(&L->cur,"CELLGT")||kw(&L->cur,"CMPGTCELL")||
      kw(&L->cur,"LECELL")||kw(&L->cur,"CELLLE")||kw(&L->cur,"CMPLECELL")||
      kw(&L->cur,"GECELL")||kw(&L->cur,"CELLGE")||kw(&L->cur,"CMPGECELL")){
    /* LTCELL/GTCELL/LECELL/GECELL lo hi val — 0/1 mask vs val; LAST_N = hits */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long val = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int mode = 0; /* 0=lt 1=gt 2=le 3=ge */
    if (strcmp(op,"GTCELL")==0 || strcmp(op,"CELLGT")==0 || strcmp(op,"CMPGTCELL")==0) mode = 1;
    else if (strcmp(op,"LECELL")==0 || strcmp(op,"CELLLE")==0 || strcmp(op,"CMPLECELL")==0) mode = 2;
    else if (strcmp(op,"GECELL")==0 || strcmp(op,"CELLGE")==0 || strcmp(op,"CMPGECELL")==0) mode = 3;
    long hits = 0;
    for (long i=lo;i<=hi;i++){
      long c = vm->cells[(int)i];
      long bit = 0;
      if (mode==0) bit = (c < val) ? 1 : 0;
      else if (mode==1) bit = (c > val) ? 1 : 0;
      else if (mode==2) bit = (c <= val) ? 1 : 0;
      else bit = (c >= val) ? 1 : 0;
      vm->cells[(int)i] = bit;
      hits += bit;
    }
    var_set_num(vm,"LAST_N",hits); vm->last_n=hits;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 cell mux: MUXCELL dst_lo a_lo b_lo mask_lo n
   * dst[i] = mask[i] ? a[i] : b[i]  for i in 0..n-1 (overlap-safe via temp) */
  if (kw(&L->cur,"MUXCELL")||kw(&L->cur,"CELLMUX")||kw(&L->cur,"SELECTCELL")||
      kw(&L->cur,"BLENDCELL")||kw(&L->cur,"CELLBLEND")){
    lex_next(L);
    long dlo = parse_expr(vm,L);
    long alo = parse_expr(vm,L);
    long blo = parse_expr(vm,L);
    long mlo = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n > CUBALC_CELL_N) n = CUBALC_CELL_N;
    long tmp[CUBALC_CELL_N];
    long wrote = 0;
    for (long i=0;i<n;i++){
      long mi = mlo + i, ai = alo + i, bi = blo + i;
      long av = (ai >= 0 && ai < CUBALC_CELL_N) ? vm->cells[(int)ai] : 0;
      long bv = (bi >= 0 && bi < CUBALC_CELL_N) ? vm->cells[(int)bi] : 0;
      long mv = (mi >= 0 && mi < CUBALC_CELL_N) ? vm->cells[(int)mi] : 0;
      tmp[i] = mv ? av : bv;
    }
    for (long i=0;i<n;i++){
      long di = dlo + i;
      if (di >= 0 && di < CUBALC_CELL_N){ vm->cells[(int)di] = tmp[i]; wrote++; }
    }
    var_set_num(vm,"LAST_N",wrote); vm->last_n=wrote;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* cell range predicates peel (ANY/ALL/NONE/NZCOUNT/EQRANGE) — G_cell_margin */
  /* digit-5 cell range predicates: ANYCELL ALLCELL NONECELL NZCOUNT EQRANGE */
  if (kw(&L->cur,"ANYCELL")||kw(&L->cur,"CELLANY")||kw(&L->cur,"ANYNZ")||
      kw(&L->cur,"ALLCELL")||kw(&L->cur,"CELLALL")||kw(&L->cur,"ALLNZ")||
      kw(&L->cur,"NONECELL")||kw(&L->cur,"CELLNONE")||kw(&L->cur,"NONENZ")||
      kw(&L->cur,"NZCOUNT")||kw(&L->cur,"COUNTNZCELL")||kw(&L->cur,"CELLNZCOUNT")||
      kw(&L->cur,"CNZCELL")){
    /* ANY/ALL/NONE: nonzero tests; NZCOUNT: count of nonzero cells in [lo,hi] */
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long nz = 0, n = hi - lo + 1;
    for (long i=lo;i<=hi;i++) if (vm->cells[(int)i] != 0) nz++;
    long r = nz;
    if (strcmp(op,"ANYCELL")==0 || strcmp(op,"CELLANY")==0 || strcmp(op,"ANYNZ")==0)
      r = (nz > 0) ? 1 : 0;
    else if (strcmp(op,"ALLCELL")==0 || strcmp(op,"CELLALL")==0 || strcmp(op,"ALLNZ")==0)
      r = (n > 0 && nz == n) ? 1 : 0;
    else if (strcmp(op,"NONECELL")==0 || strcmp(op,"CELLNONE")==0 || strcmp(op,"NONENZ")==0)
      r = (nz == 0) ? 1 : 0;
    /* else NZCOUNT / COUNTNZCELL / CELLNZCOUNT / CNZCELL → nz */
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SET",nz);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"EQRANGE")||kw(&L->cur,"RANGEEQ")||kw(&L->cur,"CELLEQRANGE")||
      kw(&L->cur,"EQCELLS")||kw(&L->cur,"SAMECELLS")){
    /* EQRANGE a_lo b_lo n — 1 if cells[a+i]==cells[b+i] for i in 0..n-1 */
    lex_next(L);
    long alo = parse_expr(vm,L);
    long blo = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n > CUBALC_CELL_N) n = CUBALC_CELL_N;
    int ok = 1;
    long checked = 0;
    for (long i=0;i<n;i++){
      long ai = alo + i, bi = blo + i;
      long av = (ai >= 0 && ai < CUBALC_CELL_N) ? vm->cells[(int)ai] : 0;
      long bv = (bi >= 0 && bi < CUBALC_CELL_N) ? vm->cells[(int)bi] : 0;
      checked++;
      if (av != bv){ ok = 0; break; }
    }
    long r = ok ? 1 : 0;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SET",checked);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell-logic stack duals: SANDCELL SORCELL SXORCELL SNOTCELL SEQCELL SNECELL */
  if (kw(&L->cur,"SANDCELL")||kw(&L->cur,"SCELLAND")||kw(&L->cur,"STACKANDCELL")||
      kw(&L->cur,"SANDC")){
    /* lo hi mask (stack) — AND each cell with mask */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long mask = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] &= mask;
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORCELL")||kw(&L->cur,"SCELLOR")||kw(&L->cur,"STACKORCELL")||
      kw(&L->cur,"SORC")){
    /* lo hi mask (stack) — OR each cell with mask */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long mask = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] |= mask;
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORCELL")||kw(&L->cur,"SCELLXOR")||kw(&L->cur,"STACKXORCELL")||
      kw(&L->cur,"SXORC")){
    /* lo hi mask (stack) — XOR each cell with mask */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long mask = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] ^= mask;
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell-logic stack duals remain: SNOTCELL SEQCELL SNECELL */
  if (kw(&L->cur,"SNOTCELL")||kw(&L->cur,"SCELLNOT")||kw(&L->cur,"STACKNOTCELL")||
      kw(&L->cur,"SINVCELL")||kw(&L->cur,"SNOTC")){
    /* lo hi (stack) — bitwise NOT each cell */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] = ~vm->cells[(int)i];
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCELLEQ")||kw(&L->cur,"STACKEQCELL")||kw(&L->cur,"SEQC")||
      kw(&L->cur,"SEQCELLS")||kw(&L->cur,"CMPEQCELLS")){
    /* lo hi val (stack) — predicate mask == val; LAST_N = hit count
     * NOTE: not SEQCELL — that alias is IOTA/RANGECELL */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long val = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long hits = 0;
    for (long i=lo;i<=hi;i++){
      long eq = (vm->cells[(int)i] == val) ? 1 : 0;
      vm->cells[(int)i] = eq;
      hits += eq;
    }
    var_set_num(vm,"LAST_N",hits); vm->last_n=hits;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNECELL")||kw(&L->cur,"SCELLNE")||kw(&L->cur,"STACKNECELL")||
      kw(&L->cur,"SNEC")){
    /* lo hi val (stack) — predicate mask != val; LAST_N = hit count */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long val = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long hits = 0;
    for (long i=lo;i<=hi;i++){
      long ne = (vm->cells[(int)i] != val) ? 1 : 0;
      vm->cells[(int)i] = ne;
      hits += ne;
    }
    var_set_num(vm,"LAST_N",hits); vm->last_n=hits;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack duals of cell relational: SLTCELL SGTCELL SLECELL SGECELL */
  if (kw(&L->cur,"SLTCELL")||kw(&L->cur,"SCELLLT")||kw(&L->cur,"STACKLTCELL")||
      kw(&L->cur,"SLTC")||
      kw(&L->cur,"SGTCELL")||kw(&L->cur,"SCELLGT")||kw(&L->cur,"STACKGTCELL")||
      kw(&L->cur,"SGTC")||
      kw(&L->cur,"SLECELL")||kw(&L->cur,"SCELLLE")||kw(&L->cur,"STACKLECELL")||
      kw(&L->cur,"SLEC")||
      kw(&L->cur,"SGECELL")||kw(&L->cur,"SCELLGE")||kw(&L->cur,"STACKGECELL")||
      kw(&L->cur,"SGEC")){
    /* lo hi val (stack) — relational 0/1 mask; LAST_N = hits */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long val = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int mode = 0;
    if (strcmp(op,"SGTCELL")==0 || strcmp(op,"SCELLGT")==0 ||
        strcmp(op,"STACKGTCELL")==0 || strcmp(op,"SGTC")==0) mode = 1;
    else if (strcmp(op,"SLECELL")==0 || strcmp(op,"SCELLLE")==0 ||
             strcmp(op,"STACKLECELL")==0 || strcmp(op,"SLEC")==0) mode = 2;
    else if (strcmp(op,"SGECELL")==0 || strcmp(op,"SCELLGE")==0 ||
             strcmp(op,"STACKGECELL")==0 || strcmp(op,"SGEC")==0) mode = 3;
    long hits = 0;
    for (long i=lo;i<=hi;i++){
      long c = vm->cells[(int)i];
      long bit = 0;
      if (mode==0) bit = (c < val) ? 1 : 0;
      else if (mode==1) bit = (c > val) ? 1 : 0;
      else if (mode==2) bit = (c <= val) ? 1 : 0;
      else bit = (c >= val) ? 1 : 0;
      vm->cells[(int)i] = bit;
      hits += bit;
    }
    var_set_num(vm,"LAST_N",hits); vm->last_n=hits;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 SMUXCELL — stack: dlo alo blo mlo n → mux cell plane */
  if (kw(&L->cur,"SMUXCELL")||kw(&L->cur,"SCELLMUX")||kw(&L->cur,"STACKMUXCELL")||
      kw(&L->cur,"SMUXC")||kw(&L->cur,"SSELECTCELL")||kw(&L->cur,"SBLENDCELL")){
    lex_next(L);
    if (vm->sp < 5){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long mlo = vm->stack[--vm->sp];
    long blo = vm->stack[--vm->sp];
    long alo = vm->stack[--vm->sp];
    long dlo = vm->stack[--vm->sp];
    if (n < 0) n = 0;
    if (n > CUBALC_CELL_N) n = CUBALC_CELL_N;
    long tmp[CUBALC_CELL_N];
    long wrote = 0;
    for (long i=0;i<n;i++){
      long mi = mlo + i, ai = alo + i, bi = blo + i;
      long av = (ai >= 0 && ai < CUBALC_CELL_N) ? vm->cells[(int)ai] : 0;
      long bv = (bi >= 0 && bi < CUBALC_CELL_N) ? vm->cells[(int)bi] : 0;
      long mv = (mi >= 0 && mi < CUBALC_CELL_N) ? vm->cells[(int)mi] : 0;
      tmp[i] = mv ? av : bv;
    }
    for (long i=0;i<n;i++){
      long di = dlo + i;
      if (di >= 0 && di < CUBALC_CELL_N){ vm->cells[(int)di] = tmp[i]; wrote++; }
    }
    var_set_num(vm,"LAST_N",wrote); vm->last_n=wrote;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack duals: SANYCELL SALLCELL SNONECELL SNZCOUNT SEQRANGES */
  if (kw(&L->cur,"SANYCELL")||kw(&L->cur,"SCELLANY")||kw(&L->cur,"STACKANYCELL")||
      kw(&L->cur,"SANYC")||
      kw(&L->cur,"SALLCELL")||kw(&L->cur,"SCELLALL")||kw(&L->cur,"STACKALLCELL")||
      kw(&L->cur,"SALLC")||
      kw(&L->cur,"SNONECELL")||kw(&L->cur,"SCELLNONE")||kw(&L->cur,"STACKNONECELL")||
      kw(&L->cur,"SNONEC")||
      kw(&L->cur,"SNZCOUNT")||kw(&L->cur,"SCELLNZCOUNT")||kw(&L->cur,"STACKNZCOUNT")||
      kw(&L->cur,"SNZC")){
    /* lo hi (stack) — cell-range nonzero predicates / count
     * NOTE: not SCOUNTNZ — that is stack-fold over data stack */
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long nz = 0, n = hi - lo + 1;
    for (long i=lo;i<=hi;i++) if (vm->cells[(int)i] != 0) nz++;
    long r = nz;
    if (strcmp(op,"SANYCELL")==0 || strcmp(op,"SCELLANY")==0 ||
        strcmp(op,"STACKANYCELL")==0 || strcmp(op,"SANYC")==0)
      r = (nz > 0) ? 1 : 0;
    else if (strcmp(op,"SALLCELL")==0 || strcmp(op,"SCELLALL")==0 ||
             strcmp(op,"STACKALLCELL")==0 || strcmp(op,"SALLC")==0)
      r = (n > 0 && nz == n) ? 1 : 0;
    else if (strcmp(op,"SNONECELL")==0 || strcmp(op,"SCELLNONE")==0 ||
             strcmp(op,"STACKNONECELL")==0 || strcmp(op,"SNONEC")==0)
      r = (nz == 0) ? 1 : 0;
    /* else SNZCOUNT */
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SEQRANGES")||kw(&L->cur,"SEQRANG")||kw(&L->cur,"STACKEQRANGE")||
      kw(&L->cur,"SCELLEQRANGE")||kw(&L->cur,"SEQCRANGE")){
    /* alo blo n (stack) → 1 if equal pairwise */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long blo = vm->stack[--vm->sp];
    long alo = vm->stack[--vm->sp];
    if (n < 0) n = 0;
    if (n > CUBALC_CELL_N) n = CUBALC_CELL_N;
    int ok = 1;
    for (long i=0;i<n;i++){
      long ai = alo + i, bi = blo + i;
      long av = (ai >= 0 && ai < CUBALC_CELL_N) ? vm->cells[(int)ai] : 0;
      long bv = (bi >= 0 && bi < CUBALC_CELL_N) ? vm->cells[(int)bi] : 0;
      if (av != bv){ ok = 0; break; }
    }
    long r = ok ? 1 : 0;
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack cell search: SFINDCELL · SFINDLAST · SFIRSTNZ · SLASTNZ */
  if (kw(&L->cur,"SFINDCELL")||kw(&L->cur,"SCELLFIND")||kw(&L->cur,"STACKFINDCELL")||
      kw(&L->cur,"SFINDC")){
    /* lo hi val (stack) → first index of val or -1 */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long val = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long found = -1;
    for (long i=lo;i<=hi;i++){
      if (vm->cells[(int)i] == val){ found = i; break; }
    }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = found;
    var_set_num(vm,"LAST_N",found); vm->last_n=found;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK", found >= 0 ? 1 : 0);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SFINDLAST")||kw(&L->cur,"SRFINDCELL")||kw(&L->cur,"STACKFINDLAST")||
      kw(&L->cur,"SFINDL")||kw(&L->cur,"SLASTFIND")){
    /* lo hi val (stack) → last index of val or -1 */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long val = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long found = -1;
    for (long i=hi;i>=lo;i--){
      if (vm->cells[(int)i] == val){ found = i; break; }
    }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = found;
    var_set_num(vm,"LAST_N",found); vm->last_n=found;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK", found >= 0 ? 1 : 0);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SFIRSTNZ")||kw(&L->cur,"STACKFIRSTNZ")||kw(&L->cur,"SFNZ")||
      kw(&L->cur,"SLASTNZ")||kw(&L->cur,"STACKLASTNZ")||kw(&L->cur,"SLNZ")){
    /* lo hi (stack) → first/last nonzero index or -1 */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int want_last = (strcmp(op,"SLASTNZ")==0 || strcmp(op,"STACKLASTNZ")==0 ||
                     strcmp(op,"SLNZ")==0);
    long found = -1;
    if (want_last){
      for (long i=hi;i>=lo;i--){
        if (vm->cells[(int)i] != 0){ found = i; break; }
      }
    } else {
      for (long i=lo;i<=hi;i++){
        if (vm->cells[(int)i] != 0){ found = i; break; }
      }
    }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = found;
    var_set_num(vm,"LAST_N",found); vm->last_n=found;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK", found >= 0 ? 1 : 0);
    bump(vm); return 1;
  }
  /* digit-9 cell-fold stack duals: SSCANCELL · SDIFFCELL · SSHIFTCELL · SCLAMPCELL */
  if (kw(&L->cur,"SSCANCELL")||kw(&L->cur,"SPREFIXSUM")||kw(&L->cur,"SCUMSUM")||
      kw(&L->cur,"STACKSCANCELL")||kw(&L->cur,"SSCANC")){
    /* lo hi (stack) — in-place inclusive prefix sum */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long run = 0;
    for (long i=lo;i<=hi;i++){
      run += vm->cells[(int)i];
      vm->cells[(int)i] = run;
    }
    var_set_num(vm,"LAST_N",run); vm->last_n=run;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDIFFCELL")||kw(&L->cur,"SCELLDIFF")||kw(&L->cur,"SDELTACELL")||
      kw(&L->cur,"STACKDIFFCELL")||kw(&L->cur,"SDIFFC")){
    /* lo hi (stack) — adjacent differences (inverse of SSCANCELL) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long prev = vm->cells[(int)lo];
    for (long i = lo + 1; i <= hi; i++){
      long v = vm->cells[(int)i];
      vm->cells[(int)i] = v - prev;
      prev = v;
    }
    long last = (hi >= lo) ? vm->cells[(int)hi] : 0;
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHIFTCELL")||kw(&L->cur,"SCELLSHIFT")||kw(&L->cur,"STACKSHIFTCELL")||
      kw(&L->cur,"SSHIFTC")){
    /* lo hi k (stack) — shift left by k (k>0) or right by |k|; zero-fill */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long n = hi - lo + 1;
    if (n > 0 && k != 0){
      long tmp[CUBALC_CELL_N];
      for (long i=0;i<n;i++) tmp[i] = 0;
      if (k > 0){
        if (k > n) k = n;
        for (long i=0;i<n-k;i++) tmp[i] = vm->cells[(int)(lo+i+k)];
      } else {
        long kk = -k;
        if (kk > n) kk = n;
        for (long i=kk;i<n;i++) tmp[i] = vm->cells[(int)(lo+i-kk)];
      }
      for (long i=0;i<n;i++) vm->cells[(int)(lo+i)] = tmp[i];
    }
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLAMPCELL")||kw(&L->cur,"SCELLCLAMP")||kw(&L->cur,"SBOUNDCELL")||
      kw(&L->cur,"STACKCLAMPCELL")||kw(&L->cur,"SCLAMPC")){
    /* lo hi mn mx (stack) — clamp each cell into [mn,mx] */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long mx = vm->stack[--vm->sp];
    long mn = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (mx < mn){ long t=mn; mn=mx; mx=t; }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long v = vm->cells[(int)i];
      if (v < mn) v = mn;
      if (v > mx) v = mx;
      vm->cells[(int)i] = v;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-9 cell fold arith: SUBCELL/DIVCELL/MODCELL + SCANCELL + CLAMPCELL */
  if (kw(&L->cur,"SUBCELL")||kw(&L->cur,"CELLSUB")||kw(&L->cur,"SUBFROMCELL")){
    /* SUBCELL lo hi delta — subtract delta from each cell in range */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long delta = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] -= delta;
    var_set_num(vm,"LAST_N",delta); vm->last_n=delta;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DIVCELL")||kw(&L->cur,"CELLDIV")||kw(&L->cur,"SCALEDIVCELL")){
    /* DIVCELL lo hi k — integer divide each cell by k (k==0 → leave unchanged) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    if (k != 0){
      for (long i=lo;i<=hi;i++) vm->cells[(int)i] /= k;
    }
    var_set_num(vm,"LAST_N",k); vm->last_n=k;
    var_set_num(vm,"OK", k != 0 ? 1 : 0); bump(vm); return 1;
  }
  if (kw(&L->cur,"MODCELL")||kw(&L->cur,"CELLMOD")||kw(&L->cur,"REMCELL")){
    /* MODCELL lo hi k — cell %= k (k==0 → no-op) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    if (k != 0){
      for (long i=lo;i<=hi;i++) vm->cells[(int)i] %= k;
    }
    var_set_num(vm,"LAST_N",k); vm->last_n=k;
    var_set_num(vm,"OK", k != 0 ? 1 : 0); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCANCELL")||kw(&L->cur,"PREFIXSUM")||kw(&L->cur,"CUMSUM")||
      kw(&L->cur,"CELLSCAN")||kw(&L->cur,"RUNNINGSUM")){
    /* SCANCELL lo hi — in-place inclusive prefix sum */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long run = 0;
    for (long i=lo;i<=hi;i++){
      run += vm->cells[(int)i];
      vm->cells[(int)i] = run;
    }
    var_set_num(vm,"LAST_N",run); vm->last_n=run;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLAMPCELL")||kw(&L->cur,"CELLCLAMP")||kw(&L->cur,"BOUNDCELL")){
    /* CLAMPCELL lo hi min max — clamp each cell into [min,max] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mn = parse_expr(vm,L);
    long mx = parse_expr(vm,L);
    if (mx < mn){ long t=mn; mn=mx; mx=t; }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long v = vm->cells[(int)i];
      if (v < mn) v = mn;
      if (v > mx) v = mx;
      vm->cells[(int)i] = v;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DIFFCELL")||kw(&L->cur,"CELLDIFF")||kw(&L->cur,"DELTACELL")||
      kw(&L->cur,"ADJDIFF")){
    /* DIFFCELL lo hi — in-place adjacent differences (inverse of SCANCELL);
     * cells[lo] kept; cells[i] = cells[i]-prev for i>lo */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long prev = vm->cells[(int)lo];
    for (long i = lo + 1; i <= hi; i++){
      long v = vm->cells[(int)i];
      vm->cells[(int)i] = v - prev;
      prev = v;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N", n > 0 ? vm->cells[(int)hi] : 0); vm->last_n = n > 0 ? vm->cells[(int)hi] : 0;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-9 cell logic plane: ANDCELL ORCELL XORCELL NOTCELL EQCELL */
  if (kw(&L->cur,"ANDCELL")||kw(&L->cur,"CELLAND")||kw(&L->cur,"BANDCELL")){
    /* ANDCELL lo hi mask — bitwise AND each cell in range with mask */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] &= mask;
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ORCELL")||kw(&L->cur,"CELLOR")||kw(&L->cur,"BORCELL")){
    /* ORCELL lo hi mask — bitwise OR each cell in range with mask */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] |= mask;
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XORCELL")||kw(&L->cur,"CELLXOR")||kw(&L->cur,"BXORCELL")){
    /* XORCELL lo hi mask — bitwise XOR each cell in range with mask */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] ^= mask;
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell inverted logic: NANDCELL · NORCELL · XNORCELL
   * (complete AND/OR/XOR cell plane with inverted duals) */
  if (kw(&L->cur,"NANDCELL")||kw(&L->cur,"CELLNAND")||kw(&L->cur,"BNANDCELL")){
    /* NANDCELL lo hi mask — cells[i] = ~(cells[i] & mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] = ~(vm->cells[(int)i] & mask);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NORCELL")||kw(&L->cur,"CELLNOR")||kw(&L->cur,"BNORCELL")){
    /* NORCELL lo hi mask — cells[i] = ~(cells[i] | mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] = ~(vm->cells[(int)i] | mask);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XNORCELL")||kw(&L->cur,"CELLXNOR")||kw(&L->cur,"BXNORCELL")||
      kw(&L->cur,"NXORCELL")||kw(&L->cur,"CELLEQX")){
    /* XNORCELL lo hi mask — cells[i] = ~(cells[i] ^ mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] = ~(vm->cells[(int)i] ^ mask);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* Peeled from lang_ops_cell.c (G_cell_margin mesh vote) — ADDCELL·MULCELL·IOTA·SORTCELL. */
  /* digit-9 cell fold plane: ADDCELL MULCELL IOTA SORTCELL */
  if (kw(&L->cur,"ADDCELL")||kw(&L->cur,"CELLADD")||kw(&L->cur,"ADDTOCELL")){
    /* ADDCELL lo hi delta — add delta to each cell in range */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long delta = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] += delta;
    var_set_num(vm,"LAST_N",delta); vm->last_n=delta;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"MULCELL")||kw(&L->cur,"CELLMUL")||kw(&L->cur,"SCALECELL")){
    /* MULCELL lo hi k — multiply each cell in range by k */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] *= k;
    var_set_num(vm,"LAST_N",k); vm->last_n=k;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"IOTA")||kw(&L->cur,"CELLIOTA")||kw(&L->cur,"SEQCELL")||kw(&L->cur,"RANGECELL")){
    /* IOTA lo hi [start [step]] — fill cell[lo..hi] with arithmetic sequence */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long start = 0, step = 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE"))){
      start = parse_expr(vm,L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        step = parse_expr(vm,L);
    }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long v = start;
    for (long i=lo;i<=hi;i++){
      vm->cells[(int)i] = v;
      v += step;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORTCELL")||kw(&L->cur,"CELLSORT")||kw(&L->cur,"ISORTCELL")){
    /* SORTCELL lo hi [ASC|DESC|dir] — insertion sort; default ASC
     * Note: bare -1 after hi would parse as hi-1; use DESC or ( -1 ). */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long dir = 1;
    if (kw(&L->cur,"DESC")||kw(&L->cur,"DOWN")||kw(&L->cur,"REV")||kw(&L->cur,"REVERSE")){
      lex_next(L); dir = -1;
    } else if (kw(&L->cur,"ASC")||kw(&L->cur,"UP")){
      lex_next(L); dir = 1;
    } else if (L->cur.kind==TK_NUM){
      dir = L->cur.num; if (dir == 0) dir = -1;
      lex_next(L);
    } else if (L->cur.kind==TK_LPAREN){
      dir = parse_expr(vm,L);
    }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i = lo + 1; i <= hi; i++){
      long key = vm->cells[(int)i];
      long j = i - 1;
      if (dir >= 0){
        while (j >= lo && vm->cells[(int)j] > key){
          vm->cells[(int)(j+1)] = vm->cells[(int)j];
          j--;
        }
      } else {
        while (j >= lo && vm->cells[(int)j] < key){
          vm->cells[(int)(j+1)] = vm->cells[(int)j];
          j--;
        }
      }
      vm->cells[(int)(j+1)] = key;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* Peeled from lang_ops_cell.c (G_cell_margin mesh vote) — MINIDX·MAXIDX·ROTCELL·SHIFTCELL. */
  /* digit-9 cell fold ext: MINIDX/MAXIDX + ROTCELL/SHIFTCELL */
  if (kw(&L->cur,"MINIDX")||kw(&L->cur,"ARGMIN")||kw(&L->cur,"CELLMINI")||
      kw(&L->cur,"MAXIDX")||kw(&L->cur,"ARGMAX")||kw(&L->cur,"CELLMAXI")){
    /* MINIDX/MAXIDX [lo [hi]] — index of min/max in range (first on ties) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int want_max = (strcmp(op,"MAXIDX")==0 || strcmp(op,"ARGMAX")==0 ||
                    strcmp(op,"CELLMAXI")==0);
    lex_next(L);
    long lo = 0, hi = CUBALC_CELL_N - 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE"))){
      lo = parse_expr(vm,L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        hi = parse_expr(vm,L);
      else hi = lo;
    }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long best_i = lo;
    long best_v = vm->cells[(int)lo];
    for (long i = lo + 1; i <= hi; i++){
      long v = vm->cells[(int)i];
      if (want_max){ if (v > best_v){ best_v = v; best_i = i; } }
      else { if (v < best_v){ best_v = v; best_i = i; } }
    }
    var_set_num(vm,"LAST_N",best_i); vm->last_n=best_i;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ROTCELL")||kw(&L->cur,"CELLROT")||kw(&L->cur,"ROTATECELL")){
    /* ROTCELL lo hi k — rotate cell[lo..hi] left by k (k<0 = right) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long n = hi - lo + 1;
    if (n > 0){
      long kk = k % n;
      if (kk < 0) kk += n;
      if (kk){
        long tmp[CUBALC_CELL_N];
        for (long i=0;i<n;i++) tmp[i] = vm->cells[(int)(lo+i)];
        for (long i=0;i<n;i++)
          vm->cells[(int)(lo+i)] = tmp[(i + kk) % n];
      }
    }
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHIFTCELL")||kw(&L->cur,"CELLSHIFT")){
    /* SHIFTCELL lo hi k — shift left by k (k>0) or right by |k| (k<0); zero-fill */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long n = hi - lo + 1;
    if (n > 0 && k != 0){
      long tmp[CUBALC_CELL_N];
      for (long i=0;i<n;i++) tmp[i] = 0;
      if (k > 0){
        if (k > n) k = n;
        for (long i=0;i<n-k;i++) tmp[i] = vm->cells[(int)(lo+i+k)];
      } else {
        long kk = -k;
        if (kk > n) kk = n;
        for (long i=kk;i<n;i++) tmp[i] = vm->cells[(int)(lo+i-kk)];
      }
      for (long i=0;i<n;i++) vm->cells[(int)(lo+i)] = tmp[i];
    }
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* Peeled from lang_ops_cell.c (G_cell_margin mesh vote) — FINDLASTCELL·FIRSTNZ·LASTNZ·REVCELL. */
  /* digit-5 cell search ext: FINDLASTCELL · FIRSTNZ · LASTNZ */
  if (kw(&L->cur,"FINDLASTCELL")||kw(&L->cur,"RFINDCELL")||kw(&L->cur,"CELLLASTFIND")||
      kw(&L->cur,"RINDEXCELL")||kw(&L->cur,"LASTFINDCELL")){
    /* FINDLASTCELL val [lo [hi]] — last index of val, or -1; OK=found */
    lex_next(L);
    long val = parse_expr(vm,L);
    long lo = 0, hi = CUBALC_CELL_N - 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE"))){
      lo = parse_expr(vm,L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        hi = parse_expr(vm,L);
      else hi = CUBALC_CELL_N - 1;
    }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long found = -1;
    for (long i=hi;i>=lo;i--){
      if (vm->cells[(int)i] == val){ found = i; break; }
    }
    var_set_num(vm,"LAST_N",found); vm->last_n=found;
    var_set_num(vm,"OK", found >= 0 ? 1 : 0);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"FIRSTNZ")||kw(&L->cur,"FIRSTNONZERO")||kw(&L->cur,"CELLFIRSTNZ")||
      kw(&L->cur,"FINDNZ")||
      kw(&L->cur,"LASTNZ")||kw(&L->cur,"LASTNONZERO")||kw(&L->cur,"CELLLASTNZ")||
      kw(&L->cur,"RFINDNZ")){
    /* FIRSTNZ/LASTNZ [lo [hi]] — index of first/last nonzero, or -1 */
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    long lo = 0, hi = CUBALC_CELL_N - 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE"))){
      lo = parse_expr(vm,L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        hi = parse_expr(vm,L);
      else hi = CUBALC_CELL_N - 1;
    }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int want_last = (strcmp(op,"LASTNZ")==0 || strcmp(op,"LASTNONZERO")==0 ||
                     strcmp(op,"CELLLASTNZ")==0 || strcmp(op,"RFINDNZ")==0);
    long found = -1;
    if (want_last){
      for (long i=hi;i>=lo;i--){
        if (vm->cells[(int)i] != 0){ found = i; break; }
      }
    } else {
      for (long i=lo;i<=hi;i++){
        if (vm->cells[(int)i] != 0){ found = i; break; }
      }
    }
    var_set_num(vm,"LAST_N",found); vm->last_n=found;
    var_set_num(vm,"OK", found >= 0 ? 1 : 0);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"REVCELL")||kw(&L->cur,"CELLREV")||kw(&L->cur,"REVERSECELLS")){
    /* REVCELL lo hi — reverse cell[lo..hi] in place */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long i = lo, j = hi;
    while (i < j){
      long t = vm->cells[(int)i];
      vm->cells[(int)i] = vm->cells[(int)j];
      vm->cells[(int)j] = t;
      i++; j--;
    }
    var_set_num(vm,"LAST_N", hi - lo + 1); vm->last_n = hi - lo + 1;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  return 0;
}