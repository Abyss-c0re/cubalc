/* CubalC lang — lang_ops_math_field32n.c
 * MATH stack imm 32-bit field ladder: unary·shift·arith·cmp·signed preds
 * Peel from math residual (B_math_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_math_field32n.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_math_try_field32n(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-1 stack imm 32-bit field unary+rotate: SNOT32N · SROL32N · SROR32N
   * (word ladder of SNOT16N/SROL16N/SROR16N; start stack 32n unary after SNAND32N plane) */
  if (kw(&L->cur,"SNOT32N")||kw(&L->cur,"STACKNOT32N")||kw(&L->cur,"NOT32N")||
      kw(&L->cur,"SNOTDWN")||kw(&L->cur,"SINV32N")||kw(&L->cur,"SINVERT32N")){
    /* SNOT32N n — word n of TOS = ~w & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (~((base >> sh) & 0xFFFFFFFFul)) & 0xFFFFFFFFul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROL32N")||kw(&L->cur,"STACKROL32N")||kw(&L->cur,"ROL32N")||
      kw(&L->cur,"SROTL32N")||kw(&L->cur,"SROLDWN")||kw(&L->cur,"SROTDWL")){
    /* SROL32N k n — rotl32 word n of TOS by k&31; n clamped 0..1 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 31;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (base >> sh) & 0xFFFFFFFFul;
    unsigned long rot = (kk == 0) ? w : (((w << (unsigned)kk) | (w >> (unsigned)(32 - kk))) & 0xFFFFFFFFul);
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (rot << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROR32N")||kw(&L->cur,"STACKROR32N")||kw(&L->cur,"ROR32N")||
      kw(&L->cur,"SROTR32N")||kw(&L->cur,"SRORDWN")||kw(&L->cur,"SROTDWR")){
    /* SROR32N k n — rotr32 word n of TOS by k&31; n clamped 0..1 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 31;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (base >> sh) & 0xFFFFFFFFul;
    unsigned long rot = (kk == 0) ? w : (((w >> (unsigned)kk) | (w << (unsigned)(32 - kk))) & 0xFFFFFFFFul);
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (rot << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm 32-bit field shift: SSHL32N · SSHR32N · SSAR32N
   * (word ladder of SSHL16N/SSHR16N/SSAR16N; stack 32n shift after SNOT32N/SROL32N) */
  if (kw(&L->cur,"SSHL32N")||kw(&L->cur,"STACKSHL32N")||kw(&L->cur,"SHL32N")||
      kw(&L->cur,"SLSH32N")||kw(&L->cur,"SSHLDWN")||kw(&L->cur,"SSHIFTDWL")){
    /* SSHL32N k n — word n of TOS = (uint32)<<k (k>=32 → 0); n clamped 0..1 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (base >> sh) & 0xFFFFFFFFul;
    unsigned long nw = (kk >= 32) ? 0ul : ((w << (unsigned)kk) & 0xFFFFFFFFul);
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHR32N")||kw(&L->cur,"STACKSHR32N")||kw(&L->cur,"SHR32N")||
      kw(&L->cur,"SLSHR32N")||kw(&L->cur,"SSHRDWN")||kw(&L->cur,"SSHIFTDWR")){
    /* SSHR32N k n — word n of TOS = (uint32)>>k logical (k>=32 → 0); n clamped 0..1 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (base >> sh) & 0xFFFFFFFFul;
    unsigned long nw = (kk >= 32) ? 0ul : (w >> (unsigned)kk);
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSAR32N")||kw(&L->cur,"STACKSAR32N")||kw(&L->cur,"SAR32N")||
      kw(&L->cur,"SASHR32N")||kw(&L->cur,"SSARDWN")||kw(&L->cur,"SSARSHIFTDW")){
    /* SSAR32N k n — word n of TOS = (int32)>>k arithmetic (k>=32 → all sign); n clamped 0..1 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long va = (long)((base >> sh) & 0xFFFFFFFFul);
    if (va & 0x80000000L) va -= 4294967296L; /* sign-extend int32 */
    long shifted;
    if (kk >= 32) shifted = (va < 0) ? -1L : 0L;
    else shifted = va >> kk;
    unsigned long nw = (unsigned long)shifted & 0xFFFFFFFFul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack imm 32-bit field zeros+signed: SCLZ32N · SCTZ32N · SNEG32N
   * (word ladder of SCLZ16N/SCTZ16N/SNEG16N; complete stack 32n unary after SSHL32N) */
  if (kw(&L->cur,"SCLZ32N")||kw(&L->cur,"STACKCLZ32N")||kw(&L->cur,"CLZ32N")||
      kw(&L->cur,"SNLZ32N")||kw(&L->cur,"SCLZDWN")||kw(&L->cur,"SDWCLZN")){
    /* SCLZ32N n — word n of TOS = clz32(w); 0 → 32; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    unsigned int w = (unsigned int)((base >> sh) & 0xFFFFFFFFul);
    unsigned long cz = 0;
    if (w == 0) cz = 32;
    else {
      for (int b = 31; b >= 0; b--){
        if (w & (1u << (unsigned)b)) break;
        cz++;
      }
    }
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | ((cz & 0xFFFFFFFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCTZ32N")||kw(&L->cur,"STACKCTZ32N")||kw(&L->cur,"CTZ32N")||
      kw(&L->cur,"SNTZ32N")||kw(&L->cur,"SCTZDWN")||kw(&L->cur,"SDWCTZN")){
    /* SCTZ32N n — word n of TOS = ctz32(w); 0 → 32; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    unsigned int w = (unsigned int)((base >> sh) & 0xFFFFFFFFul);
    unsigned long tz = 0;
    if (w == 0) tz = 32;
    else {
      while ((w & 1u) == 0){ tz++; w >>= 1; }
    }
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | ((tz & 0xFFFFFFFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNEG32N")||kw(&L->cur,"STACKNEG32N")||kw(&L->cur,"NEG32N")||
      kw(&L->cur,"SINEG32N")||kw(&L->cur,"SNEGDWN")||kw(&L->cur,"SDWNEGN")){
    /* SNEG32N n — word n of TOS = -(int32) as uint32; min int32 stays 0x80000000; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long va = (long)((base >> sh) & 0xFFFFFFFFul);
    if (va & 0x80000000L) va -= 4294967296L;
    long nr = (va == -2147483648L) ? -2147483648L : -va;
    unsigned long nv = (unsigned long)(nr & 0xFFFFFFFFul);
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (nv << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm 32-bit field abs+extend: SABS32N · SSEXT32N · SZEXT32N
   * (word ladder of SABS16N/SSEXT16N/SZEXT16N; signed extract after SNEG32N plane) */
  if (kw(&L->cur,"SABS32N")||kw(&L->cur,"STACKABS32N")||kw(&L->cur,"ABS32N")||
      kw(&L->cur,"SIABS32N")||kw(&L->cur,"SABSDWN")||kw(&L->cur,"SDWABSN")){
    /* SABS32N n — word n of TOS = abs(int32); min int32 stays 0x80000000; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long va = (long)((base >> sh) & 0xFFFFFFFFul);
    if (va & 0x80000000L) va -= 4294967296L;
    long ar = (va < 0) ? -va : va;
    /* min int32 -2147483648 → +2147483648 does not fit uint32; keep 0x80000000 */
    unsigned long nv = (unsigned long)(ar & 0xFFFFFFFFul);
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (nv << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT32N")||kw(&L->cur,"STACKSEXT32N")||kw(&L->cur,"SEXT32N")||
      kw(&L->cur,"SSIGNEXT32N")||kw(&L->cur,"SSEXTDWN")||kw(&L->cur,"SDWSEXTN")){
    /* SSEXT32N n — TOS = sign-extend word n of TOS to full width; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long va = (long)((base >> sh) & 0xFFFFFFFFul);
    if (va & 0x80000000L) va -= 4294967296L; /* sign-extend int32 */
    long r = va;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT32N")||kw(&L->cur,"STACKZEXT32N")||kw(&L->cur,"ZEXT32N")||
      kw(&L->cur,"SZEROEXT32N")||kw(&L->cur,"SZEXTDWN")||kw(&L->cur,"SDWZEXTN")){
    /* SZEXT32N n — TOS = zero-extend word n of TOS; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long r = (long)((base >> sh) & 0xFFFFFFFFul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack imm 32-bit field metrics: SBITREV32N · SPOPCNT32N · SPARITY32N
   * (word ladder of SBITREV16N/SPOPCNT16N/SPARITY16N; metrics after SABS32N plane) */
  if (kw(&L->cur,"SBITREV32N")||kw(&L->cur,"STACKBITREV32N")||kw(&L->cur,"BITREV32N")||
      kw(&L->cur,"SBREV32N")||kw(&L->cur,"SREV32N")||kw(&L->cur,"SBITREVDWN")){
    /* SBITREV32N n — word n of TOS = bitrev32(w); n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (base >> sh) & 0xFFFFFFFFul;
    unsigned long rv = 0;
    for (int b = 0; b < 32; b++){
      rv = (rv << 1) | (w & 1u);
      w >>= 1;
    }
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | ((rv & 0xFFFFFFFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOPCNT32N")||kw(&L->cur,"STACKPOPCNT32N")||kw(&L->cur,"POPCNT32N")||
      kw(&L->cur,"SPCNT32N")||kw(&L->cur,"SPOPDWN")||kw(&L->cur,"SDWPOPN")){
    /* SPOPCNT32N n — word n of TOS = popcount(w); n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (base >> sh) & 0xFFFFFFFFul;
    unsigned long pc = 0;
    while (w){ pc += (w & 1u); w >>= 1; }
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | ((pc & 0xFFFFFFFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPARITY32N")||kw(&L->cur,"STACKPARITY32N")||kw(&L->cur,"PARITY32N")||
      kw(&L->cur,"SXORRED32N")||kw(&L->cur,"SPARITYDWN")||kw(&L->cur,"SDWPARN")){
    /* SPARITY32N n — word n of TOS = xor-reduce(w) in low bit; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (base >> sh) & 0xFFFFFFFFul;
    unsigned long pr = 0;
    while (w){ pr ^= (w & 1u); w >>= 1; }
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (pr << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack imm 32-bit field arith merge: SADD32N · SSUB32N · SMUL32N
   * (word ladder of SADD16N/SSUB16N/SMUL16N; wrap uint32 ALU after SPARITY32N metrics) */
  if (kw(&L->cur,"SADD32N")||kw(&L->cur,"STACKADD32N")||kw(&L->cur,"ADD32N")||
      kw(&L->cur,"SADDDWN")||kw(&L->cur,"SADD32IMM")||kw(&L->cur,"SINC32N")){
    /* SADD32N field n — word n of TOS = (w + field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (((base >> sh) & 0xFFFFFFFFul) + f) & 0xFFFFFFFFul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUB32N")||kw(&L->cur,"STACKSUB32N")||kw(&L->cur,"SUB32N")||
      kw(&L->cur,"SSUBDWN")||kw(&L->cur,"SSUB32IMM")||kw(&L->cur,"SDEC32N")){
    /* SSUB32N field n — word n of TOS = (w - field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (((base >> sh) & 0xFFFFFFFFul) - f) & 0xFFFFFFFFul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMUL32N")||kw(&L->cur,"STACKMUL32N")||kw(&L->cur,"MUL32N")||
      kw(&L->cur,"SMULDWN")||kw(&L->cur,"SMUL32IMM")||kw(&L->cur,"STIMES32N")){
    /* SMUL32N field n — word n of TOS = (w * field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (((base >> sh) & 0xFFFFFFFFul) * f) & 0xFFFFFFFFul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 32-bit field div/mod/min: SDIV32N · SMOD32N · SMIN32N
   * (word ladder of SDIV16N/SMOD16N/SMIN16N; complete uint32 field ALU after SMUL32N) */
  if (kw(&L->cur,"SDIV32N")||kw(&L->cur,"STACKDIV32N")||kw(&L->cur,"DIV32N")||
      kw(&L->cur,"SDIVDWN")||kw(&L->cur,"SDIV32IMM")||kw(&L->cur,"SQUO32N")){
    /* SDIV32N field n — word n of TOS = w / field (field 0 → 0); n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long v = (base >> sh) & 0xFFFFFFFFul;
    unsigned long w = (f == 0) ? 0ul : (v / f);
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | ((w & 0xFFFFFFFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMOD32N")||kw(&L->cur,"STACKMOD32N")||kw(&L->cur,"MOD32N")||
      kw(&L->cur,"SMODDWN")||kw(&L->cur,"SMOD32IMM")||kw(&L->cur,"SREM32N")){
    /* SMOD32N field n — word n of TOS = w % field (field 0 → 0); n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long v = (base >> sh) & 0xFFFFFFFFul;
    unsigned long w = (f == 0) ? 0ul : (v % f);
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | ((w & 0xFFFFFFFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMIN32N")||kw(&L->cur,"STACKMIN32N")||kw(&L->cur,"MIN32N")||
      kw(&L->cur,"SMINDWN")||kw(&L->cur,"SMIN32IMM")||kw(&L->cur,"SLE32N")){
    /* SMIN32N field n — word n of TOS = min(w, field) unsigned; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long v = (base >> sh) & 0xFFFFFFFFul;
    unsigned long w = (v < f) ? v : f;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack imm 32-bit field max+eq: SMAX32N · SEQ32N · SNE32N
   * (word ladder of SMAX16N/SEQ16N/SNE16N; complete min/max + equality after SMIN32N) */
  if (kw(&L->cur,"SMAX32N")||kw(&L->cur,"STACKMAX32N")||kw(&L->cur,"MAX32N")||
      kw(&L->cur,"SMAXDWN")||kw(&L->cur,"SMAX32IMM")||kw(&L->cur,"SGE32N")){
    /* SMAX32N field n — word n of TOS = max(w, field) unsigned; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long v = (base >> sh) & 0xFFFFFFFFul;
    unsigned long w = (v > f) ? v : f;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SEQ32N")||kw(&L->cur,"STACKEQ32N")||kw(&L->cur,"EQ32N")||
      kw(&L->cur,"SEQDWN")||kw(&L->cur,"SEQ32IMM")||kw(&L->cur,"SCMPEQ32N")){
    /* SEQ32N field n — word n of TOS = (w == field) ? 1 : 0; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long v = (base >> sh) & 0xFFFFFFFFul;
    unsigned long w = (v == f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNE32N")||kw(&L->cur,"STACKNE32N")||kw(&L->cur,"NE32N")||
      kw(&L->cur,"SNEDWN")||kw(&L->cur,"SNE32IMM")||kw(&L->cur,"SCMPNE32N")){
    /* SNE32N field n — word n of TOS = (w != field) ? 1 : 0; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long v = (base >> sh) & 0xFFFFFFFFul;
    unsigned long w = (v != f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack imm 32-bit field ordered-cmp: SLT32N · SGT32N · SLTE32N
   * (unsigned predicates after SEQ32N/SNE32N; complete stack 32n field compare plane) */
  if (kw(&L->cur,"SLT32N")||kw(&L->cur,"STACKLT32N")||kw(&L->cur,"LT32N")||
      kw(&L->cur,"SCMPLT32N")||kw(&L->cur,"SLTDWN")||kw(&L->cur,"SLT32IMM")){
    /* SLT32N field n — word n of TOS = (w < field) ? 1 : 0; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long v = (base >> sh) & 0xFFFFFFFFul;
    unsigned long w = (v < f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGT32N")||kw(&L->cur,"STACKGT32N")||kw(&L->cur,"GT32N")||
      kw(&L->cur,"SCMPGT32N")||kw(&L->cur,"SGTDWN")||kw(&L->cur,"SGT32IMM")){
    /* SGT32N field n — word n of TOS = (w > field) ? 1 : 0; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long v = (base >> sh) & 0xFFFFFFFFul;
    unsigned long w = (v > f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLTE32N")||kw(&L->cur,"STACKLTE32N")||kw(&L->cur,"LTE32N")||
      kw(&L->cur,"SCMPLE32N")||kw(&L->cur,"SLTEDWN")||kw(&L->cur,"SLEQ32N")||
      kw(&L->cur,"SLTE32IMM")){
    /* SLTE32N field n — word n of TOS = (w <= field) ? 1 : 0; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long v = (base >> sh) & 0xFFFFFFFFul;
    unsigned long w = (v <= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack imm field GTE: SGTE32N (complete ordered plane after SLTE32N) */
  if (kw(&L->cur,"SGTE32N")||kw(&L->cur,"STACKGTE32N")||kw(&L->cur,"GTE32N")||
      kw(&L->cur,"SCMPGE32N")||kw(&L->cur,"SGEQ32N")||kw(&L->cur,"SGTEDWN")||
      kw(&L->cur,"SGTE32IMM")){
    /* SGTE32N field n — word n of TOS = (w >= field) ? 1 : 0; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long v = (base >> sh) & 0xFFFFFFFFul;
    unsigned long w = (v >= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack imm 32-bit field signed ordered-cmp: SLTS32N · SGTS32N · SLTES32N · SGTES32N
   * (word ladder of SLTS16N plane; signed int32 after unsigned SGTE32N) */
  if (kw(&L->cur,"SLTS32N")||kw(&L->cur,"STACKLTS32N")||kw(&L->cur,"LTS32N")||
      kw(&L->cur,"SCMPLTS32N")||kw(&L->cur,"SLTSSIGN32N")||kw(&L->cur,"SLTS32IMM")){
    /* SLTS32N field n — word n = (int32(w) < int32(field)) ? 1 : 0; n 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long v = (long)(int)((unsigned int)((base >> sh) & 0xFFFFFFFFul));
    long f = (long)(int)((unsigned int)((unsigned long)field & 0xFFFFFFFFul));
    unsigned long w = (v < f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGTS32N")||kw(&L->cur,"STACKGTS32N")||kw(&L->cur,"GTS32N")||
      kw(&L->cur,"SCMPGTS32N")||kw(&L->cur,"SGTSSIGN32N")||kw(&L->cur,"SGTS32IMM")){
    /* SGTS32N field n — word n = (int32(w) > int32(field)) ? 1 : 0; n 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long v = (long)(int)((unsigned int)((base >> sh) & 0xFFFFFFFFul));
    long f = (long)(int)((unsigned int)((unsigned long)field & 0xFFFFFFFFul));
    unsigned long w = (v > f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLTES32N")||kw(&L->cur,"STACKLTES32N")||kw(&L->cur,"LTES32N")||
      kw(&L->cur,"SCMPLES32N")||kw(&L->cur,"SLEQS32N")||kw(&L->cur,"SLTES32IMM")){
    /* SLTES32N field n — word n = (int32(w) <= int32(field)) ? 1 : 0; n 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long v = (long)(int)((unsigned int)((base >> sh) & 0xFFFFFFFFul));
    long f = (long)(int)((unsigned int)((unsigned long)field & 0xFFFFFFFFul));
    unsigned long w = (v <= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGTES32N")||kw(&L->cur,"STACKGTES32N")||kw(&L->cur,"GTES32N")||
      kw(&L->cur,"SCMPGES32N")||kw(&L->cur,"SGEQS32N")||kw(&L->cur,"SGTES32IMM")){
    /* SGTES32N field n — word n = (int32(w) >= int32(field)) ? 1 : 0; n 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long v = (long)(int)((unsigned int)((base >> sh) & 0xFFFFFFFFul));
    long f = (long)(int)((unsigned int)((unsigned long)field & 0xFFFFFFFFul));
    unsigned long w = (v >= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack imm 32-bit field signed min/max: SMINS32N · SMAXS32N
   * (signed select after SLTS32N plane; dual of unsigned SMIN32N/SMAX32N) */
  if (kw(&L->cur,"SMINS32N")||kw(&L->cur,"STACKMINS32N")||kw(&L->cur,"MINS32N")||
      kw(&L->cur,"SMINSIGN32N")||kw(&L->cur,"SMINS32IMM")||kw(&L->cur,"SMINSI32N")){
    /* SMINS32N field n — word n = min_signed(int32(w), int32(field)); n 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long v = (long)(int)((unsigned int)((base >> sh) & 0xFFFFFFFFul));
    long f = (long)(int)((unsigned int)((unsigned long)field & 0xFFFFFFFFul));
    long m = (v < f) ? v : f;
    unsigned long w = (unsigned long)(unsigned int)m;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMAXS32N")||kw(&L->cur,"STACKMAXS32N")||kw(&L->cur,"MAXS32N")||
      kw(&L->cur,"SMAXSIGN32N")||kw(&L->cur,"SMAXS32IMM")||kw(&L->cur,"SMAXSI32N")){
    /* SMAXS32N field n — word n = max_signed(int32(w), int32(field)); n 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long v = (long)(int)((unsigned int)((base >> sh) & 0xFFFFFFFFul));
    long f = (long)(int)((unsigned int)((unsigned long)field & 0xFFFFFFFFul));
    long m = (v > f) ? v : f;
    unsigned long w = (unsigned long)(unsigned int)m;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm 32-bit field signed clamp+between: SCLAMPS32N · SBETWEENS32N
   * (signed range plane after SMINS32N/SMAXS32N; field dual of SCLAMPN/SBETWEENN) */
  if (kw(&L->cur,"SCLAMPS32N")||kw(&L->cur,"STACKCLAMPS32N")||kw(&L->cur,"CLAMPS32N")||
      kw(&L->cur,"SCLAMPSIGN32N")||kw(&L->cur,"SCLAMPS32IMM")||kw(&L->cur,"SBOUNDS32N")){
    /* SCLAMPS32N lo hi n — word n = clamp_signed(int32(w),[lo,hi]); swap lo/hi if needed; n 0..1 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    long slo = (long)(int)((unsigned int)((unsigned long)lo & 0xFFFFFFFFul));
    long shi = (long)(int)((unsigned int)((unsigned long)hi & 0xFFFFFFFFul));
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long v = (long)(int)((unsigned int)((base >> sh) & 0xFFFFFFFFul));
    if (v < slo) v = slo;
    if (v > shi) v = shi;
    unsigned long w = (unsigned long)(unsigned int)v;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBETWEENS32N")||kw(&L->cur,"STACKBETWEENS32N")||kw(&L->cur,"BETWEENS32N")||
      kw(&L->cur,"SINRANGES32N")||kw(&L->cur,"SBETWEENS32IMM")||kw(&L->cur,"SINRANGESIGN32N")){
    /* SBETWEENS32N lo hi n — word n = (lo <= int32(w) <= hi) ? 1 : 0; swap lo/hi; n 0..1 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    long slo = (long)(int)((unsigned int)((unsigned long)lo & 0xFFFFFFFFul));
    long shi = (long)(int)((unsigned int)((unsigned long)hi & 0xFFFFFFFFul));
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long v = (long)(int)((unsigned int)((base >> sh) & 0xFFFFFFFFul));
    unsigned long w = (v >= slo && v <= shi) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 32-bit field signed WITHIN (hi exclusive): SWITHINS32N
   * (Forth WITHIN dual of SWITHINN; no lo/hi swap; after SBETWEENS32N inclusive) */
  if (kw(&L->cur,"SWITHINS32N")||kw(&L->cur,"STACKWITHINS32N")||kw(&L->cur,"WITHINS32N")||
      kw(&L->cur,"SWITHINSIGN32N")||kw(&L->cur,"SWITHINS32IMM")||kw(&L->cur,"SINTERVALS32N")){
    /* SWITHINS32N lo hi n — word n = (lo <= int32(w) < hi) ? 1 : 0; n 0..1; no swap */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    long slo = (long)(int)((unsigned int)((unsigned long)lo & 0xFFFFFFFFul));
    long shi = (long)(int)((unsigned int)((unsigned long)hi & 0xFFFFFFFFul));
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long v = (long)(int)((unsigned int)((base >> sh) & 0xFFFFFFFFul));
    unsigned long w = (v >= slo && v < shi) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm 32-bit field signed signum: SSIGN32N
   * (word ladder of SSIGN16N; field dual of SSIGN after signed WITHIN plane) */
  if (kw(&L->cur,"SSIGN32N")||kw(&L->cur,"STACKSIGN32N")||kw(&L->cur,"SIGN32N")||
      kw(&L->cur,"SSIGNUM32N")||kw(&L->cur,"SGN32N")||kw(&L->cur,"SSIGN32IMM")){
    /* SSIGN32N n — word n = sgn(int32): +1 / 0 / 0xFFFFFFFF(-1); n 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long v = (long)(int)((unsigned int)((base >> sh) & 0xFFFFFFFFul));
    unsigned long w = (v > 0) ? 1ul : ((v < 0) ? 0xFFFFFFFFul : 0ul);
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm 32-bit field zero-eq: S0EQ32N
   * (word ladder of S0EQ16N; field dual of S0EQ/SZ after SSIGN plane) */
  if (kw(&L->cur,"S0EQ32N")||kw(&L->cur,"STACK0EQ32N")||kw(&L->cur,"SZ32N")||
      kw(&L->cur,"SEQZ32N")||kw(&L->cur,"SISZERO32N")||kw(&L->cur,"S0EQ32IMM")){
    /* S0EQ32N n — word n = (w == 0) ? 1 : 0; n 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long v = (base >> sh) & 0xFFFFFFFFul;
    unsigned long w = (v == 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack imm 32-bit field nonzero: S0NE32N
   * (word ladder of S0NE16N; field dual of S0NE/SNZ after S0EQ plane) */
  if (kw(&L->cur,"S0NE32N")||kw(&L->cur,"STACK0NE32N")||kw(&L->cur,"SNZ32N")||
      kw(&L->cur,"SNEZ32N")||kw(&L->cur,"SISNZ32N")||kw(&L->cur,"S0NE32IMM")){
    /* S0NE32N n — word n = (w != 0) ? 1 : 0; n 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long v = (base >> sh) & 0xFFFFFFFFul;
    unsigned long w = (v != 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack imm 32-bit field signed LT0: S0LT32N
   * (word ladder of S0LT16N; field dual of S0LT after S0EQ/S0NE plane) */
  if (kw(&L->cur,"S0LT32N")||kw(&L->cur,"STACK0LT32N")||kw(&L->cur,"SLT032N")||
      kw(&L->cur,"SNEGZ32N")||kw(&L->cur,"SISNEG32N")||kw(&L->cur,"S0LT32IMM")){
    /* S0LT32N n — word n = (int32(w) < 0) ? 1 : 0; n 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long v = (long)(int)((unsigned int)((base >> sh) & 0xFFFFFFFFul));
    unsigned long w = (v < 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm 32-bit field signed GT0: S0GT32N
   * (word ladder of S0GT16N; field dual of S0GT after S0LT plane) */
  if (kw(&L->cur,"S0GT32N")||kw(&L->cur,"STACK0GT32N")||kw(&L->cur,"SGT032N")||
      kw(&L->cur,"SPOSZ32N")||kw(&L->cur,"SISPOS32N")||kw(&L->cur,"S0GT32IMM")){
    /* S0GT32N n — word n = (int32(w) > 0) ? 1 : 0; n 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long v = (long)(int)((unsigned int)((base >> sh) & 0xFFFFFFFFul));
    unsigned long w = (v > 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm 32-bit field signed LE0: S0LE32N
   * (word ladder of S0LE16N; field dual of S0LE after S0GT plane) */
  if (kw(&L->cur,"S0LE32N")||kw(&L->cur,"STACK0LE32N")||kw(&L->cur,"SLE032N")||
      kw(&L->cur,"SNONPOS32N")||kw(&L->cur,"SISNONPOS32N")||kw(&L->cur,"S0LE32IMM")){
    /* S0LE32N n — word n = (int32(w) <= 0) ? 1 : 0; n 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long v = (long)(int)((unsigned int)((base >> sh) & 0xFFFFFFFFul));
    unsigned long w = (v <= 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack imm 32-bit field signed GE0: S0GE32N
   * (word ladder of S0GE16N; field dual of S0GE after S0LE plane open) */
  if (kw(&L->cur,"S0GE32N")||kw(&L->cur,"STACK0GE32N")||kw(&L->cur,"SGE032N")||
      kw(&L->cur,"SNONNEG32N")||kw(&L->cur,"SISNONNEG32N")||kw(&L->cur,"S0GE32IMM")){
    /* S0GE32N n — word n = (int32(w) >= 0) ? 1 : 0; n 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 32);
    long v = (long)(int)((unsigned int)((base >> sh) & 0xFFFFFFFFul));
    unsigned long w = (v >= 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
