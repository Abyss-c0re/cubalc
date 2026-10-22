/* CubalC lang — lang_ops_dual_unary4.c
 * DUAL imm-4 unary/shift/metrics/zeros/abs: DNOT4N…DZEXT4N (G peel).
 * Mesh vote G_dual_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_unary4.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_dual_try_unary4(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-1 dual-stack imm 4-bit field unary+rotate: DNOT4N · DROL4N · DROR4N
   * (dual of SNOT4N/SROL4N/SROR4N; nibble-field unary plane on top two cells after dual 4n bitwise) */
  if (kw(&L->cur,"DNOT4N")||kw(&L->cur,"S2NOT4N")||kw(&L->cur,"STACK2NOT4N")||
      kw(&L->cur,"PAIRNOT4N")||kw(&L->cur,"DINV4N")||kw(&L->cur,"DINVERT4N")||
      kw(&L->cur,"DNOTNIBN")){
    /* a b + n → nibble n of each = ~nibble & 0xF; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~((ma >> sh) & 0xFul)) & 0xFul;
    unsigned long wb = (~((mb >> sh) & 0xFul)) & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROL4N")||kw(&L->cur,"S2ROL4N")||kw(&L->cur,"STACK2ROL4N")||
      kw(&L->cur,"PAIRROL4N")||kw(&L->cur,"DROTL4N")||kw(&L->cur,"DROLNIBN")||
      kw(&L->cur,"DROTNIBL")){
    /* a b + k n → rotl4 nibble n of each by k&3; n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 3;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (kk == 0) ? va : (((va << (unsigned)kk) | (va >> (unsigned)(4 - kk))) & 0xFul);
    unsigned long wb = (kk == 0) ? vb : (((vb << (unsigned)kk) | (vb >> (unsigned)(4 - kk))) & 0xFul);
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROR4N")||kw(&L->cur,"S2ROR4N")||kw(&L->cur,"STACK2ROR4N")||
      kw(&L->cur,"PAIRROR4N")||kw(&L->cur,"DROTR4N")||kw(&L->cur,"DRORNIBN")||
      kw(&L->cur,"DROTNIBR")){
    /* a b + k n → rotr4 nibble n of each by k&3; n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 3;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (kk == 0) ? va : (((va >> (unsigned)kk) | (va << (unsigned)(4 - kk))) & 0xFul);
    unsigned long wb = (kk == 0) ? vb : (((vb >> (unsigned)kk) | (vb << (unsigned)(4 - kk))) & 0xFul);
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack imm 4-bit field shift: DSHL4N · DSHR4N · DSAR4N
   * (dual of SSHL4N/SSHR4N/SSAR4N; nibble-field shift plane on top two cells after DNOT4N/DROL4N) */
  if (kw(&L->cur,"DSHL4N")||kw(&L->cur,"S2SHL4N")||kw(&L->cur,"STACK2SHL4N")||
      kw(&L->cur,"PAIRSHL4N")||kw(&L->cur,"DLSH4N")||kw(&L->cur,"DSHLNIBN")||
      kw(&L->cur,"DSHIFTNIBL")){
    /* a b + k n → nibble n of each = (uint4)<<k (k>=4 → 0); n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (kk >= 4) ? 0ul : ((va << (unsigned)kk) & 0xFul);
    unsigned long wb = (kk >= 4) ? 0ul : ((vb << (unsigned)kk) & 0xFul);
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSHR4N")||kw(&L->cur,"S2SHR4N")||kw(&L->cur,"STACK2SHR4N")||
      kw(&L->cur,"PAIRSHR4N")||kw(&L->cur,"DLSHR4N")||kw(&L->cur,"DSHRNIBN")||
      kw(&L->cur,"DSHIFTNIBR")){
    /* a b + k n → nibble n of each = (uint4)>>k logical (k>=4 → 0); n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (kk >= 4) ? 0ul : (va >> (unsigned)kk);
    unsigned long wb = (kk >= 4) ? 0ul : (vb >> (unsigned)kk);
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSAR4N")||kw(&L->cur,"S2SAR4N")||kw(&L->cur,"STACK2SAR4N")||
      kw(&L->cur,"PAIRSAR4N")||kw(&L->cur,"DASHR4N")||kw(&L->cur,"DSARNIBN")||
      kw(&L->cur,"DSARSHIFTNIB")){
    /* a b + k n → nibble n of each = (int4)>>k arithmetic (k>=4 → all sign); n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul);
    long vb = (long)((mb >> sh) & 0xFul);
    if (va & 0x8) va -= 16;
    if (vb & 0x8) vb -= 16;
    long sa, sb;
    if (kk >= 4){ sa = (va < 0) ? -1L : 0L; sb = (vb < 0) ? -1L : 0L; }
    else { sa = va >> kk; sb = vb >> kk; }
    unsigned long wa = (unsigned long)sa & 0xFul;
    unsigned long wb = (unsigned long)sb & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 4-bit field metrics: DBITREV4N · DPOPCNT4N · DPARITY4N
   * (dual of SBITREV4N/SPOPCNT4N/SPARITY4N; nibble metrics plane on top two after DSHL4N) */
  if (kw(&L->cur,"DBITREV4N")||kw(&L->cur,"S2BITREV4N")||kw(&L->cur,"STACK2BITREV4N")||
      kw(&L->cur,"PAIRBITREV4N")||kw(&L->cur,"DBREV4N")||kw(&L->cur,"DREV4N")||
      kw(&L->cur,"DBITREVNIBN")){
    /* a b + n → nibble n of each = bitrev4(nibble); n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long ra = 0, rb = 0, ta = va, tb = vb;
    for (int b = 0; b < 4; b++){
      ra = (ra << 1) | (ta & 1u); ta >>= 1;
      rb = (rb << 1) | (tb & 1u); tb >>= 1;
    }
    long x = (long)((ma & ~(0xFul << sh)) | (ra << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (rb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPOPCNT4N")||kw(&L->cur,"S2POPCNT4N")||kw(&L->cur,"STACK2POPCNT4N")||
      kw(&L->cur,"PAIRPOPCNT4N")||kw(&L->cur,"DPCNT4N")||kw(&L->cur,"DPOPNIBN")||
      kw(&L->cur,"DNIBPOPN")){
    /* a b + n → nibble n of each = popcount(nibble); n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long pa = 0, pb = 0;
    while (va){ pa += (va & 1u); va >>= 1; }
    while (vb){ pb += (vb & 1u); vb >>= 1; }
    long x = (long)((ma & ~(0xFul << sh)) | (pa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (pb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPARITY4N")||kw(&L->cur,"S2PARITY4N")||kw(&L->cur,"STACK2PARITY4N")||
      kw(&L->cur,"PAIRPARITY4N")||kw(&L->cur,"DXORRED4N")||kw(&L->cur,"DPARITYNIBN")||
      kw(&L->cur,"DNIBPARN")){
    /* a b + n → nibble n of each = xor-reduce(nibble) in low bit; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long pra = 0, prb = 0;
    while (va){ pra ^= (va & 1u); va >>= 1; }
    while (vb){ prb ^= (vb & 1u); vb >>= 1; }
    long x = (long)((ma & ~(0xFul << sh)) | (pra << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (prb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack imm 4-bit field zeros+signed: DCLZ4N · DCTZ4N · DNEG4N
   * (dual of SCLZ4N/SCTZ4N/SNEG4N; nibble zeros+negate plane on top two after DPARITY4N) */
  if (kw(&L->cur,"DCLZ4N")||kw(&L->cur,"S2CLZ4N")||kw(&L->cur,"STACK2CLZ4N")||
      kw(&L->cur,"PAIRCLZ4N")||kw(&L->cur,"DNLZ4N")||kw(&L->cur,"DCLZNIBN")||
      kw(&L->cur,"DNIBCLZN")){
    /* a b + n → nibble n of each = clz4(nibble); 0 → 4; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned int wa = (unsigned int)((ma >> sh) & 0xFul);
    unsigned int wb = (unsigned int)((mb >> sh) & 0xFul);
    unsigned long cza = 0, czb = 0;
    if (wa == 0) cza = 4;
    else { for (int b = 3; b >= 0; b--){ if (wa & (1u << (unsigned)b)) break; cza++; } }
    if (wb == 0) czb = 4;
    else { for (int b = 3; b >= 0; b--){ if (wb & (1u << (unsigned)b)) break; czb++; } }
    long x = (long)((ma & ~(0xFul << sh)) | (cza << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (czb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCTZ4N")||kw(&L->cur,"S2CTZ4N")||kw(&L->cur,"STACK2CTZ4N")||
      kw(&L->cur,"PAIRCTZ4N")||kw(&L->cur,"DNTZ4N")||kw(&L->cur,"DCTZNIBN")||
      kw(&L->cur,"DNIBCTZN")){
    /* a b + n → nibble n of each = ctz4(nibble); 0 → 4; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned int wa = (unsigned int)((ma >> sh) & 0xFul);
    unsigned int wb = (unsigned int)((mb >> sh) & 0xFul);
    unsigned long tza = 0, tzb = 0;
    if (wa == 0) tza = 4;
    else { while ((wa & 1u) == 0){ tza++; wa >>= 1; } }
    if (wb == 0) tzb = 4;
    else { while ((wb & 1u) == 0){ tzb++; wb >>= 1; } }
    long x = (long)((ma & ~(0xFul << sh)) | (tza << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (tzb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNEG4N")||kw(&L->cur,"S2NEG4N")||kw(&L->cur,"STACK2NEG4N")||
      kw(&L->cur,"PAIRNEG4N")||kw(&L->cur,"DINEG4N")||kw(&L->cur,"DNEGNIBN")||
      kw(&L->cur,"DNIBNEGN")){
    /* a b + n → nibble n of each = -(int4) as uint4; min int4 -8 stays 0x8; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul);
    long vb = (long)((mb >> sh) & 0xFul);
    if (va & 0x8) va -= 16;
    if (vb & 0x8) vb -= 16;
    long na = (va == -8L) ? -8L : -va;
    long nb = (vb == -8L) ? -8L : -vb;
    unsigned long nva = (unsigned long)(na & 0xF);
    unsigned long nvb = (unsigned long)(nb & 0xF);
    long x = (long)((ma & ~(0xFul << sh)) | (nva << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (nvb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack imm 4-bit field abs+extend: DABS4N · DSEXT4N · DZEXT4N
   * (dual of SABS4N/SSEXT4N/SZEXT4N; complete dual nibble unary plane after DNEG4N) */
  if (kw(&L->cur,"DABS4N")||kw(&L->cur,"S2ABS4N")||kw(&L->cur,"STACK2ABS4N")||
      kw(&L->cur,"PAIRABS4N")||kw(&L->cur,"DIABS4N")||kw(&L->cur,"DABSNIBN")||
      kw(&L->cur,"DNIBABSN")){
    /* a b + n → nibble n of each = abs(int4); min -8 → 0x8; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul);
    long vb = (long)((mb >> sh) & 0xFul);
    if (va & 0x8) va -= 16;
    if (vb & 0x8) vb -= 16;
    long ara = (va < 0) ? -va : va;
    long arb = (vb < 0) ? -vb : vb;
    unsigned long nva = (unsigned long)(ara & 0xF);
    unsigned long nvb = (unsigned long)(arb & 0xF);
    long x = (long)((ma & ~(0xFul << sh)) | (nva << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (nvb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSEXT4N")||kw(&L->cur,"S2SEXT4N")||kw(&L->cur,"STACK2SEXT4N")||
      kw(&L->cur,"PAIRSEXT4N")||kw(&L->cur,"DSIGNEXT4N")||kw(&L->cur,"DSEXTNIBN")||
      kw(&L->cur,"DNIBSEXTN")){
    /* a b + n → each cell = sign-extend nibble n to full width; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul);
    long vb = (long)((mb >> sh) & 0xFul);
    if (va & 0x8) va -= 16;
    if (vb & 0x8) vb -= 16;
    vm->stack[vm->sp - 2] = va;
    vm->stack[vm->sp - 1] = vb;
    var_set_num(vm,"LAST_N",vb); vm->last_n=vb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DZEXT4N")||kw(&L->cur,"S2ZEXT4N")||kw(&L->cur,"STACK2ZEXT4N")||
      kw(&L->cur,"PAIRZEXT4N")||kw(&L->cur,"DZEROEXT4N")||kw(&L->cur,"DZEXTNIBN")||
      kw(&L->cur,"DNIBZEXTN")){
    /* a b + n → each cell = zero-extend nibble n (low4 only); n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long xa = (long)((ma >> sh) & 0xFul);
    long yb = (long)((mb >> sh) & 0xFul);
    vm->stack[vm->sp - 2] = xa;
    vm->stack[vm->sp - 1] = yb;
    var_set_num(vm,"LAST_N",yb); vm->last_n=yb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
