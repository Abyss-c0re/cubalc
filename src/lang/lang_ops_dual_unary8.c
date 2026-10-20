/* CubalC lang — lang_ops_dual_unary8.c
 * DUAL imm-8 unary/shift/metrics/zeros/abs: DNOT8N…DZEXT8N (G peel).
 * Mesh vote G_split_dual_cell · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_unary8.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_dual_try_unary8(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-1 dual-stack imm 8-bit field unary+rotate: DNOT8N · DROL8N · DROR8N
   * (byte-field ladder of DNOT4N/DROL4N/DROR4N; dual 8n unary plane after dual 4n complete) */
  if (kw(&L->cur,"DNOT8N")||kw(&L->cur,"S2NOT8N")||kw(&L->cur,"STACK2NOT8N")||
      kw(&L->cur,"PAIRNOT8N")||kw(&L->cur,"DINV8N")||kw(&L->cur,"DINVERT8N")||
      kw(&L->cur,"DNOTBYTEN")){
    /* a b + n → byte n of each = ~byte & 0xFF; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~((ma >> sh) & 0xFFul)) & 0xFFul;
    unsigned long wb = (~((mb >> sh) & 0xFFul)) & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROL8N")||kw(&L->cur,"S2ROL8N")||kw(&L->cur,"STACK2ROL8N")||
      kw(&L->cur,"PAIRROL8N")||kw(&L->cur,"DROTL8N")||kw(&L->cur,"DROLBYTEN")||
      kw(&L->cur,"DROTBYTEL")){
    /* a b + k n → rotl8 byte n of each by k&7; n clamped 0..7 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (kk == 0) ? va : (((va << (unsigned)kk) | (va >> (unsigned)(8 - kk))) & 0xFFul);
    unsigned long wb = (kk == 0) ? vb : (((vb << (unsigned)kk) | (vb >> (unsigned)(8 - kk))) & 0xFFul);
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROR8N")||kw(&L->cur,"S2ROR8N")||kw(&L->cur,"STACK2ROR8N")||
      kw(&L->cur,"PAIRROR8N")||kw(&L->cur,"DROTR8N")||kw(&L->cur,"DRORBYTEN")||
      kw(&L->cur,"DROTBYTER")){
    /* a b + k n → rotr8 byte n of each by k&7; n clamped 0..7 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (kk == 0) ? va : (((va >> (unsigned)kk) | (va << (unsigned)(8 - kk))) & 0xFFul);
    unsigned long wb = (kk == 0) ? vb : (((vb >> (unsigned)kk) | (vb << (unsigned)(8 - kk))) & 0xFFul);
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 8-bit field shift: DSHL8N · DSHR8N · DSAR8N
   * (byte ladder of DSHL4N/DSHR4N/DSAR4N; dual 8n shift plane after DNOT8N/DROL8N) */
  if (kw(&L->cur,"DSHL8N")||kw(&L->cur,"S2SHL8N")||kw(&L->cur,"STACK2SHL8N")||
      kw(&L->cur,"PAIRSHL8N")||kw(&L->cur,"DLSH8N")||kw(&L->cur,"DSHLBYTEN")||
      kw(&L->cur,"DSHIFTBYTEL")){
    /* a b + k n → byte n of each = (uint8)<<k (k>=8 → 0); n clamped 0..7 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (kk >= 8) ? 0ul : ((va << (unsigned)kk) & 0xFFul);
    unsigned long wb = (kk >= 8) ? 0ul : ((vb << (unsigned)kk) & 0xFFul);
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSHR8N")||kw(&L->cur,"S2SHR8N")||kw(&L->cur,"STACK2SHR8N")||
      kw(&L->cur,"PAIRSHR8N")||kw(&L->cur,"DLSHR8N")||kw(&L->cur,"DSHRBYTEN")||
      kw(&L->cur,"DSHIFTBYTER")){
    /* a b + k n → byte n of each = (uint8)>>k logical (k>=8 → 0); n clamped 0..7 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (kk >= 8) ? 0ul : (va >> (unsigned)kk);
    unsigned long wb = (kk >= 8) ? 0ul : (vb >> (unsigned)kk);
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSAR8N")||kw(&L->cur,"S2SAR8N")||kw(&L->cur,"STACK2SAR8N")||
      kw(&L->cur,"PAIRSAR8N")||kw(&L->cur,"DASHR8N")||kw(&L->cur,"DSARBYTEN")||
      kw(&L->cur,"DSARSHIFTBYTE")){
    /* a b + k n → byte n of each = (int8)>>k arithmetic (k>=8 → all sign); n clamped 0..7 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul);
    long vb = (long)((mb >> sh) & 0xFFul);
    if (va & 0x80) va -= 256;
    if (vb & 0x80) vb -= 256;
    long sa, sb;
    if (kk >= 8){ sa = (va < 0) ? -1L : 0L; sb = (vb < 0) ? -1L : 0L; }
    else { sa = va >> kk; sb = vb >> kk; }
    unsigned long wa = (unsigned long)sa & 0xFFul;
    unsigned long wb = (unsigned long)sb & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 8-bit field metrics: DBITREV8N · DPOPCNT8N · DPARITY8N
   * (byte ladder of DBITREV4N/DPOPCNT4N/DPARITY4N; dual 8n metrics after DSHL8N) */
  if (kw(&L->cur,"DBITREV8N")||kw(&L->cur,"S2BITREV8N")||kw(&L->cur,"STACK2BITREV8N")||
      kw(&L->cur,"PAIRBITREV8N")||kw(&L->cur,"DBREV8N")||kw(&L->cur,"DREV8N")||
      kw(&L->cur,"DBITREVBYTEN")){
    /* a b + n → byte n of each = bitrev8(byte); n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long ra = 0, rb = 0, ta = va, tb = vb;
    for (int b = 0; b < 8; b++){
      ra = (ra << 1) | (ta & 1u); ta >>= 1;
      rb = (rb << 1) | (tb & 1u); tb >>= 1;
    }
    long x = (long)((ma & ~(0xFFul << sh)) | (ra << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (rb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPOPCNT8N")||kw(&L->cur,"S2POPCNT8N")||kw(&L->cur,"STACK2POPCNT8N")||
      kw(&L->cur,"PAIRPOPCNT8N")||kw(&L->cur,"DPCNT8N")||kw(&L->cur,"DPOPBYTEN")||
      kw(&L->cur,"DBYTEPOPN")){
    /* a b + n → byte n of each = popcount(byte); n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long pa = 0, pb = 0;
    while (va){ pa += (va & 1u); va >>= 1; }
    while (vb){ pb += (vb & 1u); vb >>= 1; }
    long x = (long)((ma & ~(0xFFul << sh)) | ((pa & 0xFFul) << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | ((pb & 0xFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPARITY8N")||kw(&L->cur,"S2PARITY8N")||kw(&L->cur,"STACK2PARITY8N")||
      kw(&L->cur,"PAIRPARITY8N")||kw(&L->cur,"DXORRED8N")||kw(&L->cur,"DPARITYBYTEN")||
      kw(&L->cur,"DBYTEPARN")){
    /* a b + n → byte n of each = xor-reduce(byte) in low bit; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long pra = 0, prb = 0;
    while (va){ pra ^= (va & 1u); va >>= 1; }
    while (vb){ prb ^= (vb & 1u); vb >>= 1; }
    long x = (long)((ma & ~(0xFFul << sh)) | (pra << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (prb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm 8-bit field zeros+signed: DCLZ8N · DCTZ8N · DNEG8N
   * (byte ladder of DCLZ4N/DCTZ4N/DNEG4N; dual 8n zeros+negate after DPARITY8N) */
  if (kw(&L->cur,"DCLZ8N")||kw(&L->cur,"S2CLZ8N")||kw(&L->cur,"STACK2CLZ8N")||
      kw(&L->cur,"PAIRCLZ8N")||kw(&L->cur,"DNLZ8N")||kw(&L->cur,"DCLZBYTEN")||
      kw(&L->cur,"DBYTECLZN")){
    /* a b + n → byte n of each = clz8(byte); 0 → 8; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned int wa = (unsigned int)((ma >> sh) & 0xFFul);
    unsigned int wb = (unsigned int)((mb >> sh) & 0xFFul);
    unsigned long cza = 0, czb = 0;
    if (wa == 0) cza = 8;
    else { for (int b = 7; b >= 0; b--){ if (wa & (1u << (unsigned)b)) break; cza++; } }
    if (wb == 0) czb = 8;
    else { for (int b = 7; b >= 0; b--){ if (wb & (1u << (unsigned)b)) break; czb++; } }
    long x = (long)((ma & ~(0xFFul << sh)) | ((cza & 0xFFul) << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | ((czb & 0xFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCTZ8N")||kw(&L->cur,"S2CTZ8N")||kw(&L->cur,"STACK2CTZ8N")||
      kw(&L->cur,"PAIRCTZ8N")||kw(&L->cur,"DNTZ8N")||kw(&L->cur,"DCTZBYTEN")||
      kw(&L->cur,"DBYTECTZN")){
    /* a b + n → byte n of each = ctz8(byte); 0 → 8; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned int wa = (unsigned int)((ma >> sh) & 0xFFul);
    unsigned int wb = (unsigned int)((mb >> sh) & 0xFFul);
    unsigned long tza = 0, tzb = 0;
    if (wa == 0) tza = 8;
    else { while ((wa & 1u) == 0){ tza++; wa >>= 1; } }
    if (wb == 0) tzb = 8;
    else { while ((wb & 1u) == 0){ tzb++; wb >>= 1; } }
    long x = (long)((ma & ~(0xFFul << sh)) | ((tza & 0xFFul) << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | ((tzb & 0xFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNEG8N")||kw(&L->cur,"S2NEG8N")||kw(&L->cur,"STACK2NEG8N")||
      kw(&L->cur,"PAIRNEG8N")||kw(&L->cur,"DINEG8N")||kw(&L->cur,"DNEGBYTEN")||
      kw(&L->cur,"DBYTENEGN")){
    /* a b + n → byte n of each = -(int8) as uint8; min int8 -128 stays 0x80; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul);
    long vb = (long)((mb >> sh) & 0xFFul);
    if (va & 0x80) va -= 256;
    if (vb & 0x80) vb -= 256;
    long na = (va == -128L) ? -128L : -va;
    long nb = (vb == -128L) ? -128L : -vb;
    unsigned long nva = (unsigned long)(na & 0xFF);
    unsigned long nvb = (unsigned long)(nb & 0xFF);
    long x = (long)((ma & ~(0xFFul << sh)) | (nva << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (nvb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack imm 8-bit field abs+extend: DABS8N · DSEXT8N · DZEXT8N
   * (byte ladder of DABS4N/DSEXT4N/DZEXT4N; complete dual 8n unary plane after DNEG8N) */
  if (kw(&L->cur,"DABS8N")||kw(&L->cur,"S2ABS8N")||kw(&L->cur,"STACK2ABS8N")||
      kw(&L->cur,"PAIRABS8N")||kw(&L->cur,"DIABS8N")||kw(&L->cur,"DABSBYTEN")||
      kw(&L->cur,"DBYTEABSN")){
    /* a b + n → byte n of each = abs(int8); min -128 → 0x80; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul);
    long vb = (long)((mb >> sh) & 0xFFul);
    if (va & 0x80) va -= 256;
    if (vb & 0x80) vb -= 256;
    long ara = (va < 0) ? -va : va;
    long arb = (vb < 0) ? -vb : vb;
    /* min int8 -128 → +128 does not fit uint8; keep 0x80 (same as SABS4N min path) */
    unsigned long nva = (unsigned long)(ara & 0xFF);
    unsigned long nvb = (unsigned long)(arb & 0xFF);
    long x = (long)((ma & ~(0xFFul << sh)) | (nva << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (nvb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSEXT8N")||kw(&L->cur,"S2SEXT8N")||kw(&L->cur,"STACK2SEXT8N")||
      kw(&L->cur,"PAIRSEXT8N")||kw(&L->cur,"DSIGNEXT8N")||kw(&L->cur,"DSEXTBYTEN")||
      kw(&L->cur,"DBYTESEXTN")){
    /* a b + n → each cell = sign-extend byte n to full width; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul);
    long vb = (long)((mb >> sh) & 0xFFul);
    if (va & 0x80) va -= 256;
    if (vb & 0x80) vb -= 256;
    vm->stack[vm->sp - 2] = va;
    vm->stack[vm->sp - 1] = vb;
    var_set_num(vm,"LAST_N",vb); vm->last_n=vb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DZEXT8N")||kw(&L->cur,"S2ZEXT8N")||kw(&L->cur,"STACK2ZEXT8N")||
      kw(&L->cur,"PAIRZEXT8N")||kw(&L->cur,"DZEROEXT8N")||kw(&L->cur,"DZEXTBYTEN")||
      kw(&L->cur,"DBYTEZEXTN")){
    /* a b + n → each cell = zero-extend byte n; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long xa = (long)((ma >> sh) & 0xFFul);
    long yb = (long)((mb >> sh) & 0xFFul);
    vm->stack[vm->sp - 2] = xa;
    vm->stack[vm->sp - 1] = yb;
    var_set_num(vm,"LAST_N",yb); vm->last_n=yb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
