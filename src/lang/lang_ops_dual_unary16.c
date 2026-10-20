/* CubalC lang — lang_ops_dual_unary16.c
 * DUAL imm-16 abs/unary/shift/metrics/zeros: DABS16N…DNEG16N (G peel).
 * Mesh vote G_split_dual_cell · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_unary16.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_dual_try_unary16(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-5 dual-stack imm 16-bit field abs+extend: DABS16N · DSEXT16N · DZEXT16N
   * (halfword ladder of DABS8N/DSEXT8N/DZEXT8N; dual of SABS16N/SSEXT16N/SZEXT16N) */
  if (kw(&L->cur,"DABS16N")||kw(&L->cur,"S2ABS16N")||kw(&L->cur,"STACK2ABS16N")||
      kw(&L->cur,"PAIRABS16N")||kw(&L->cur,"DIABS16N")||kw(&L->cur,"DABSWORDN")||
      kw(&L->cur,"DWORDABSN")){
    /* a b + n → halfword n of each = abs(int16); min -32768 → 0x8000; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul);
    long vb = (long)((mb >> sh) & 0xFFFFul);
    if (va & 0x8000) va -= 65536;
    if (vb & 0x8000) vb -= 65536;
    long ara = (va < 0) ? -va : va;
    long arb = (vb < 0) ? -vb : vb;
    /* min int16 -32768 → +32768 does not fit uint16; keep 0x8000 (same as SABS16N) */
    unsigned long nva = (unsigned long)(ara & 0xFFFF);
    unsigned long nvb = (unsigned long)(arb & 0xFFFF);
    long x = (long)((ma & ~(0xFFFFul << sh)) | (nva << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (nvb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSEXT16N")||kw(&L->cur,"S2SEXT16N")||kw(&L->cur,"STACK2SEXT16N")||
      kw(&L->cur,"PAIRSEXT16N")||kw(&L->cur,"DSIGNEXT16N")||kw(&L->cur,"DSEXTWORDN")||
      kw(&L->cur,"DWORDSEXTN")){
    /* a b + n → each cell = sign-extend halfword n to full width; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul);
    long vb = (long)((mb >> sh) & 0xFFFFul);
    if (va & 0x8000) va -= 65536;
    if (vb & 0x8000) vb -= 65536;
    vm->stack[vm->sp - 2] = va;
    vm->stack[vm->sp - 1] = vb;
    var_set_num(vm,"LAST_N",vb); vm->last_n=vb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DZEXT16N")||kw(&L->cur,"S2ZEXT16N")||kw(&L->cur,"STACK2ZEXT16N")||
      kw(&L->cur,"PAIRZEXT16N")||kw(&L->cur,"DZEROEXT16N")||kw(&L->cur,"DZEXTWORDN")||
      kw(&L->cur,"DWORDZEXTN")){
    /* a b + n → each cell = zero-extend halfword n; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long xa = (long)((ma >> sh) & 0xFFFFul);
    long yb = (long)((mb >> sh) & 0xFFFFul);
    vm->stack[vm->sp - 2] = xa;
    vm->stack[vm->sp - 1] = yb;
    var_set_num(vm,"LAST_N",yb); vm->last_n=yb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 16-bit field unary+rotate: DNOT16N · DROL16N · DROR16N
   * (halfword ladder of DNOT8N/DROL8N/DROR8N; start dual 16n unary plane after DZEXT16N) */
  if (kw(&L->cur,"DNOT16N")||kw(&L->cur,"S2NOT16N")||kw(&L->cur,"STACK2NOT16N")||
      kw(&L->cur,"PAIRNOT16N")||kw(&L->cur,"DINV16N")||kw(&L->cur,"DINVERT16N")||
      kw(&L->cur,"DNOTWORDN")){
    /* a b + n → halfword n of each = ~hw & 0xFFFF; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~((ma >> sh) & 0xFFFFul)) & 0xFFFFul;
    unsigned long wb = (~((mb >> sh) & 0xFFFFul)) & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROL16N")||kw(&L->cur,"S2ROL16N")||kw(&L->cur,"STACK2ROL16N")||
      kw(&L->cur,"PAIRROL16N")||kw(&L->cur,"DROTL16N")||kw(&L->cur,"DROLWORDN")||
      kw(&L->cur,"DROTWORDL")){
    /* a b + k n → rotl16 halfword n of each by k&15; n clamped 0..3 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 15;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (kk == 0) ? va : (((va << (unsigned)kk) | (va >> (unsigned)(16 - kk))) & 0xFFFFul);
    unsigned long wb = (kk == 0) ? vb : (((vb << (unsigned)kk) | (vb >> (unsigned)(16 - kk))) & 0xFFFFul);
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROR16N")||kw(&L->cur,"S2ROR16N")||kw(&L->cur,"STACK2ROR16N")||
      kw(&L->cur,"PAIRROR16N")||kw(&L->cur,"DROTR16N")||kw(&L->cur,"DRORWORDN")||
      kw(&L->cur,"DROTWORDR")){
    /* a b + k n → rotr16 halfword n of each by k&15; n clamped 0..3 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 15;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (kk == 0) ? va : (((va >> (unsigned)kk) | (va << (unsigned)(16 - kk))) & 0xFFFFul);
    unsigned long wb = (kk == 0) ? vb : (((vb >> (unsigned)kk) | (vb << (unsigned)(16 - kk))) & 0xFFFFul);
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack imm 16-bit field shift: DSHL16N · DSHR16N · DSAR16N
   * (halfword ladder of DSHL8N/DSHR8N/DSAR8N; dual 16n shift after DNOT16N/DROL16N) */
  if (kw(&L->cur,"DSHL16N")||kw(&L->cur,"S2SHL16N")||kw(&L->cur,"STACK2SHL16N")||
      kw(&L->cur,"PAIRSHL16N")||kw(&L->cur,"DLSH16N")||kw(&L->cur,"DSHLWORDN")||
      kw(&L->cur,"DSHIFTWORDL")){
    /* a b + k n → halfword n of each = (uint16)<<k (k>=16 → 0); n clamped 0..3 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (kk >= 16) ? 0ul : ((va << (unsigned)kk) & 0xFFFFul);
    unsigned long wb = (kk >= 16) ? 0ul : ((vb << (unsigned)kk) & 0xFFFFul);
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSHR16N")||kw(&L->cur,"S2SHR16N")||kw(&L->cur,"STACK2SHR16N")||
      kw(&L->cur,"PAIRSHR16N")||kw(&L->cur,"DLSHR16N")||kw(&L->cur,"DSHRWORDN")||
      kw(&L->cur,"DSHIFTWORDR")){
    /* a b + k n → halfword n of each = (uint16)>>k logical (k>=16 → 0); n clamped 0..3 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (kk >= 16) ? 0ul : (va >> (unsigned)kk);
    unsigned long wb = (kk >= 16) ? 0ul : (vb >> (unsigned)kk);
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSAR16N")||kw(&L->cur,"S2SAR16N")||kw(&L->cur,"STACK2SAR16N")||
      kw(&L->cur,"PAIRSAR16N")||kw(&L->cur,"DASHR16N")||kw(&L->cur,"DSARWORDN")||
      kw(&L->cur,"DSARSHIFTWORD")){
    /* a b + k n → halfword n of each = (int16)>>k arithmetic (k>=16 → all sign); n clamped 0..3 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul);
    long vb = (long)((mb >> sh) & 0xFFFFul);
    if (va & 0x8000) va -= 65536;
    if (vb & 0x8000) vb -= 65536;
    long sa, sb;
    if (kk >= 16){ sa = (va < 0) ? -1L : 0L; sb = (vb < 0) ? -1L : 0L; }
    else { sa = va >> kk; sb = vb >> kk; }
    unsigned long wa = (unsigned long)sa & 0xFFFFul;
    unsigned long wb = (unsigned long)sb & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack imm 16-bit field metrics: DBITREV16N · DPOPCNT16N · DPARITY16N
   * (halfword ladder of DBITREV8N/DPOPCNT8N/DPARITY8N; dual of SBITREV16N after DSHL16N) */
  if (kw(&L->cur,"DBITREV16N")||kw(&L->cur,"S2BITREV16N")||kw(&L->cur,"STACK2BITREV16N")||
      kw(&L->cur,"PAIRBITREV16N")||kw(&L->cur,"DBREV16N")||kw(&L->cur,"DREV16N")||
      kw(&L->cur,"DBITREVWORDN")){
    /* a b + n → halfword n of each = bitrev16(hw); n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long ra = 0, rb = 0, ta = va, tb = vb;
    for (int b = 0; b < 16; b++){
      ra = (ra << 1) | (ta & 1u); ta >>= 1;
      rb = (rb << 1) | (tb & 1u); tb >>= 1;
    }
    long x = (long)((ma & ~(0xFFFFul << sh)) | ((ra & 0xFFFFul) << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | ((rb & 0xFFFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPOPCNT16N")||kw(&L->cur,"S2POPCNT16N")||kw(&L->cur,"STACK2POPCNT16N")||
      kw(&L->cur,"PAIRPOPCNT16N")||kw(&L->cur,"DPCNT16N")||kw(&L->cur,"DPOPWORDN")||
      kw(&L->cur,"DWORDPOPN")){
    /* a b + n → halfword n of each = popcount(hw); n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long pa = 0, pb = 0;
    while (va){ pa += (va & 1u); va >>= 1; }
    while (vb){ pb += (vb & 1u); vb >>= 1; }
    long x = (long)((ma & ~(0xFFFFul << sh)) | ((pa & 0xFFFFul) << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | ((pb & 0xFFFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPARITY16N")||kw(&L->cur,"S2PARITY16N")||kw(&L->cur,"STACK2PARITY16N")||
      kw(&L->cur,"PAIRPARITY16N")||kw(&L->cur,"DXORRED16N")||kw(&L->cur,"DPARITYWORDN")||
      kw(&L->cur,"DWORDPARN")){
    /* a b + n → halfword n of each = xor-reduce(hw) in low bit; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long pra = 0, prb = 0;
    while (va){ pra ^= (va & 1u); va >>= 1; }
    while (vb){ prb ^= (vb & 1u); vb >>= 1; }
    long x = (long)((ma & ~(0xFFFFul << sh)) | (pra << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (prb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 16-bit field zeros+signed: DCLZ16N · DCTZ16N · DNEG16N
   * (halfword ladder of DCLZ8N/DCTZ8N/DNEG8N; dual of SCLZ16N after DPARITY16N) */
  if (kw(&L->cur,"DCLZ16N")||kw(&L->cur,"S2CLZ16N")||kw(&L->cur,"STACK2CLZ16N")||
      kw(&L->cur,"PAIRCLZ16N")||kw(&L->cur,"DNLZ16N")||kw(&L->cur,"DCLZWORDN")||
      kw(&L->cur,"DWORDCLZN")){
    /* a b + n → halfword n of each = clz16(hw); 0 → 16; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned int wa = (unsigned int)((ma >> sh) & 0xFFFFul);
    unsigned int wb = (unsigned int)((mb >> sh) & 0xFFFFul);
    unsigned long cza = 0, czb = 0;
    if (wa == 0) cza = 16;
    else { for (int b = 15; b >= 0; b--){ if (wa & (1u << (unsigned)b)) break; cza++; } }
    if (wb == 0) czb = 16;
    else { for (int b = 15; b >= 0; b--){ if (wb & (1u << (unsigned)b)) break; czb++; } }
    long x = (long)((ma & ~(0xFFFFul << sh)) | ((cza & 0xFFFFul) << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | ((czb & 0xFFFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCTZ16N")||kw(&L->cur,"S2CTZ16N")||kw(&L->cur,"STACK2CTZ16N")||
      kw(&L->cur,"PAIRCTZ16N")||kw(&L->cur,"DNTZ16N")||kw(&L->cur,"DCTZWORDN")||
      kw(&L->cur,"DWORDCTZN")){
    /* a b + n → halfword n of each = ctz16(hw); 0 → 16; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned int wa = (unsigned int)((ma >> sh) & 0xFFFFul);
    unsigned int wb = (unsigned int)((mb >> sh) & 0xFFFFul);
    unsigned long tza = 0, tzb = 0;
    if (wa == 0) tza = 16;
    else { while ((wa & 1u) == 0){ tza++; wa >>= 1; } }
    if (wb == 0) tzb = 16;
    else { while ((wb & 1u) == 0){ tzb++; wb >>= 1; } }
    long x = (long)((ma & ~(0xFFFFul << sh)) | ((tza & 0xFFFFul) << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | ((tzb & 0xFFFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNEG16N")||kw(&L->cur,"S2NEG16N")||kw(&L->cur,"STACK2NEG16N")||
      kw(&L->cur,"PAIRNEG16N")||kw(&L->cur,"DINEG16N")||kw(&L->cur,"DNEGWORDN")||
      kw(&L->cur,"DWORDNEGN")){
    /* a b + n → halfword n of each = -(int16) as uint16; min int16 -32768 stays 0x8000; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul);
    long vb = (long)((mb >> sh) & 0xFFFFul);
    if (va & 0x8000) va -= 65536;
    if (vb & 0x8000) vb -= 65536;
    long na = (va == -32768L) ? -32768L : -va;
    long nb = (vb == -32768L) ? -32768L : -vb;
    unsigned long nva = (unsigned long)(na & 0xFFFF);
    unsigned long nvb = (unsigned long)(nb & 0xFFFF);
    long x = (long)((ma & ~(0xFFFFul << sh)) | (nva << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (nvb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
