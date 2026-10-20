/* CubalC lang — lang_ops_dual_unary32.c
 * DUAL imm-32 unary/shift/abs/zeros/metrics: DNOT32N…DPARITY32N (G peel).
 * Mesh vote G_split_dual_cell · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_unary32.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_dual_try_unary32(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-1 dual-stack imm 32-bit field unary+rotate: DNOT32N · DROL32N · DROR32N
   * (word ladder of DNOT16N/DROL16N/DROR16N; dual of SNOT32N; start dual 32n unary) */
  if (kw(&L->cur,"DNOT32N")||kw(&L->cur,"S2NOT32N")||kw(&L->cur,"STACK2NOT32N")||
      kw(&L->cur,"PAIRNOT32N")||kw(&L->cur,"DINV32N")||kw(&L->cur,"DINVERT32N")||
      kw(&L->cur,"DNOTDWN")){
    /* a b + n → word n of each = ~w & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~((ma >> sh) & 0xFFFFFFFFul)) & 0xFFFFFFFFul;
    unsigned long wb = (~((mb >> sh) & 0xFFFFFFFFul)) & 0xFFFFFFFFul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROL32N")||kw(&L->cur,"S2ROL32N")||kw(&L->cur,"STACK2ROL32N")||
      kw(&L->cur,"PAIRROL32N")||kw(&L->cur,"DROTL32N")||kw(&L->cur,"DROLDWN")||
      kw(&L->cur,"DROTDWL")){
    /* a b + k n → rotl32 word n of each by k&31; n clamped 0..1 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 31;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long ra = (kk == 0) ? va : (((va << (unsigned)kk) | (va >> (unsigned)(32 - kk))) & 0xFFFFFFFFul);
    unsigned long rb = (kk == 0) ? vb : (((vb << (unsigned)kk) | (vb >> (unsigned)(32 - kk))) & 0xFFFFFFFFul);
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (ra << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (rb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROR32N")||kw(&L->cur,"S2ROR32N")||kw(&L->cur,"STACK2ROR32N")||
      kw(&L->cur,"PAIRROR32N")||kw(&L->cur,"DROTR32N")||kw(&L->cur,"DRORDWN")||
      kw(&L->cur,"DROTDWR")){
    /* a b + k n → rotr32 word n of each by k&31; n clamped 0..1 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 31;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long ra = (kk == 0) ? va : (((va >> (unsigned)kk) | (va << (unsigned)(32 - kk))) & 0xFFFFFFFFul);
    unsigned long rb = (kk == 0) ? vb : (((vb >> (unsigned)kk) | (vb << (unsigned)(32 - kk))) & 0xFFFFFFFFul);
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (ra << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (rb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack imm 32-bit field shift: DSHL32N · DSHR32N · DSAR32N
   * (word ladder of DSHL16N/DSHR16N/DSAR16N; dual of SSHL32N after DNOT32N) */
  if (kw(&L->cur,"DSHL32N")||kw(&L->cur,"S2SHL32N")||kw(&L->cur,"STACK2SHL32N")||
      kw(&L->cur,"PAIRSHL32N")||kw(&L->cur,"DLSH32N")||kw(&L->cur,"DSHLDWN")||
      kw(&L->cur,"DSHIFTDWL")){
    /* a b + k n → word n of each = (uint32)<<k (k>=32 → 0); n clamped 0..1 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long wa = (kk >= 32) ? 0ul : ((va << (unsigned)kk) & 0xFFFFFFFFul);
    unsigned long wb = (kk >= 32) ? 0ul : ((vb << (unsigned)kk) & 0xFFFFFFFFul);
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSHR32N")||kw(&L->cur,"S2SHR32N")||kw(&L->cur,"STACK2SHR32N")||
      kw(&L->cur,"PAIRSHR32N")||kw(&L->cur,"DLSHR32N")||kw(&L->cur,"DSHRDWN")||
      kw(&L->cur,"DSHIFTDWR")){
    /* a b + k n → word n of each = (uint32)>>k logical (k>=32 → 0); n clamped 0..1 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long wa = (kk >= 32) ? 0ul : (va >> (unsigned)kk);
    unsigned long wb = (kk >= 32) ? 0ul : (vb >> (unsigned)kk);
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSAR32N")||kw(&L->cur,"S2SAR32N")||kw(&L->cur,"STACK2SAR32N")||
      kw(&L->cur,"PAIRSAR32N")||kw(&L->cur,"DASHR32N")||kw(&L->cur,"DSARDWN")||
      kw(&L->cur,"DSARSHIFTDW")){
    /* a b + k n → word n of each = (int32)>>k arithmetic (k>=32 → all sign); n clamped 0..1 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFFFFFul);
    long vb = (long)((mb >> sh) & 0xFFFFFFFFul);
    if (va & 0x80000000L) va -= 4294967296L;
    if (vb & 0x80000000L) vb -= 4294967296L;
    long sa, sb;
    if (kk >= 32){ sa = (va < 0) ? -1L : 0L; sb = (vb < 0) ? -1L : 0L; }
    else { sa = va >> kk; sb = vb >> kk; }
    unsigned long wa = (unsigned long)sa & 0xFFFFFFFFul;
    unsigned long wb = (unsigned long)sb & 0xFFFFFFFFul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 32-bit field abs+extend: DABS32N · DSEXT32N · DZEXT32N
   * (word ladder of DABS16N/DSEXT16N/DZEXT16N; dual of SABS32N after DSHL32N) */
  if (kw(&L->cur,"DABS32N")||kw(&L->cur,"S2ABS32N")||kw(&L->cur,"STACK2ABS32N")||
      kw(&L->cur,"PAIRABS32N")||kw(&L->cur,"DIABS32N")||kw(&L->cur,"DABSDWN")||
      kw(&L->cur,"DDWABSN")){
    /* a b + n → word n of each = abs(int32); min -2147483648 → 0x80000000; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFFFFFul);
    long vb = (long)((mb >> sh) & 0xFFFFFFFFul);
    if (va & 0x80000000L) va -= 4294967296L;
    if (vb & 0x80000000L) vb -= 4294967296L;
    long ara = (va < 0) ? -va : va;
    long arb = (vb < 0) ? -vb : vb;
    /* min int32 -2147483648 → +2147483648 does not fit uint32; keep 0x80000000 */
    unsigned long nva = (unsigned long)(ara & 0xFFFFFFFFul);
    unsigned long nvb = (unsigned long)(arb & 0xFFFFFFFFul);
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (nva << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (nvb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSEXT32N")||kw(&L->cur,"S2SEXT32N")||kw(&L->cur,"STACK2SEXT32N")||
      kw(&L->cur,"PAIRSEXT32N")||kw(&L->cur,"DSIGNEXT32N")||kw(&L->cur,"DSEXTDWN")||
      kw(&L->cur,"DDWSEXTN")){
    /* a b + n → each cell = sign-extend word n to full width; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFFFFFul);
    long vb = (long)((mb >> sh) & 0xFFFFFFFFul);
    if (va & 0x80000000L) va -= 4294967296L;
    if (vb & 0x80000000L) vb -= 4294967296L;
    vm->stack[vm->sp - 2] = va;
    vm->stack[vm->sp - 1] = vb;
    var_set_num(vm,"LAST_N",vb); vm->last_n=vb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DZEXT32N")||kw(&L->cur,"S2ZEXT32N")||kw(&L->cur,"STACK2ZEXT32N")||
      kw(&L->cur,"PAIRZEXT32N")||kw(&L->cur,"DZEROEXT32N")||kw(&L->cur,"DZEXTDWN")||
      kw(&L->cur,"DDWZEXTN")){
    /* a b + n → each cell = zero-extend word n; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long xa = (long)((ma >> sh) & 0xFFFFFFFFul);
    long yb = (long)((mb >> sh) & 0xFFFFFFFFul);
    vm->stack[vm->sp - 2] = xa;
    vm->stack[vm->sp - 1] = yb;
    var_set_num(vm,"LAST_N",yb); vm->last_n=yb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 32-bit field zeros+signed: DCLZ32N · DCTZ32N · DNEG32N
   * (word ladder of DCLZ16N/DCTZ16N/DNEG16N; dual of SCLZ32N after DABS32N) */
  if (kw(&L->cur,"DCLZ32N")||kw(&L->cur,"S2CLZ32N")||kw(&L->cur,"STACK2CLZ32N")||
      kw(&L->cur,"PAIRCLZ32N")||kw(&L->cur,"DNLZ32N")||kw(&L->cur,"DCLZDWN")||
      kw(&L->cur,"DDWCLZN")){
    /* a b + n → word n of each = clz32(w); 0 → 32; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned int wa = (unsigned int)((ma >> sh) & 0xFFFFFFFFul);
    unsigned int wb = (unsigned int)((mb >> sh) & 0xFFFFFFFFul);
    unsigned long cza = 0, czb = 0;
    if (wa == 0) cza = 32;
    else { for (int b = 31; b >= 0; b--){ if (wa & (1u << (unsigned)b)) break; cza++; } }
    if (wb == 0) czb = 32;
    else { for (int b = 31; b >= 0; b--){ if (wb & (1u << (unsigned)b)) break; czb++; } }
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | ((cza & 0xFFFFFFFFul) << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | ((czb & 0xFFFFFFFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCTZ32N")||kw(&L->cur,"S2CTZ32N")||kw(&L->cur,"STACK2CTZ32N")||
      kw(&L->cur,"PAIRCTZ32N")||kw(&L->cur,"DNTZ32N")||kw(&L->cur,"DCTZDWN")||
      kw(&L->cur,"DDWCTZN")){
    /* a b + n → word n of each = ctz32(w); 0 → 32; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned int wa = (unsigned int)((ma >> sh) & 0xFFFFFFFFul);
    unsigned int wb = (unsigned int)((mb >> sh) & 0xFFFFFFFFul);
    unsigned long tza = 0, tzb = 0;
    if (wa == 0) tza = 32;
    else { while ((wa & 1u) == 0){ tza++; wa >>= 1; } }
    if (wb == 0) tzb = 32;
    else { while ((wb & 1u) == 0){ tzb++; wb >>= 1; } }
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | ((tza & 0xFFFFFFFFul) << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | ((tzb & 0xFFFFFFFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNEG32N")||kw(&L->cur,"S2NEG32N")||kw(&L->cur,"STACK2NEG32N")||
      kw(&L->cur,"PAIRNEG32N")||kw(&L->cur,"DINEG32N")||kw(&L->cur,"DNEGDWN")||
      kw(&L->cur,"DDWNEGN")){
    /* a b + n → word n of each = -(int32) as uint32; min int32 stays 0x80000000; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFFFFFul);
    long vb = (long)((mb >> sh) & 0xFFFFFFFFul);
    if (va & 0x80000000L) va -= 4294967296L;
    if (vb & 0x80000000L) vb -= 4294967296L;
    long na = (va == -2147483648L) ? -2147483648L : -va;
    long nb = (vb == -2147483648L) ? -2147483648L : -vb;
    unsigned long nva = (unsigned long)(na & 0xFFFFFFFFul);
    unsigned long nvb = (unsigned long)(nb & 0xFFFFFFFFul);
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (nva << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (nvb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack imm 32-bit field metrics: DBITREV32N · DPOPCNT32N · DPARITY32N
   * (word ladder of DBITREV16N/DPOPCNT16N/DPARITY16N; dual of SBITREV32N after DCLZ32N) */
  if (kw(&L->cur,"DBITREV32N")||kw(&L->cur,"S2BITREV32N")||kw(&L->cur,"STACK2BITREV32N")||
      kw(&L->cur,"PAIRBITREV32N")||kw(&L->cur,"DBREV32N")||kw(&L->cur,"DREV32N")||
      kw(&L->cur,"DBITREVDWN")){
    /* a b + n → word n of each = bitrev32(w); n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long ra = 0, rb = 0, ta = va, tb = vb;
    for (int b = 0; b < 32; b++){
      ra = (ra << 1) | (ta & 1u); ta >>= 1;
      rb = (rb << 1) | (tb & 1u); tb >>= 1;
    }
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | ((ra & 0xFFFFFFFFul) << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | ((rb & 0xFFFFFFFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPOPCNT32N")||kw(&L->cur,"S2POPCNT32N")||kw(&L->cur,"STACK2POPCNT32N")||
      kw(&L->cur,"PAIRPOPCNT32N")||kw(&L->cur,"DPCNT32N")||kw(&L->cur,"DPOPDWN")||
      kw(&L->cur,"DDWPOPN")){
    /* a b + n → word n of each = popcount(w); n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long pa = 0, pb = 0;
    while (va){ pa += (va & 1u); va >>= 1; }
    while (vb){ pb += (vb & 1u); vb >>= 1; }
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | ((pa & 0xFFFFFFFFul) << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | ((pb & 0xFFFFFFFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPARITY32N")||kw(&L->cur,"S2PARITY32N")||kw(&L->cur,"STACK2PARITY32N")||
      kw(&L->cur,"PAIRPARITY32N")||kw(&L->cur,"DXORRED32N")||kw(&L->cur,"DPARITYDWN")||
      kw(&L->cur,"DDWPARN")){
    /* a b + n → word n of each = xor-reduce(w) in low bit; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long pra = 0, prb = 0;
    while (va){ pra ^= (va & 1u); va >>= 1; }
    while (vb){ prb ^= (vb & 1u); vb >>= 1; }
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (pra << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (prb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
