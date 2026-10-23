/* CubalC lang — lang_ops_math_field8n.c
 * MATH 8-bit field ladder: SADD8N…S0GE8N · abs/unary/shift/metrics
 * Peel from math residual (B_math_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_math_field8n.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_math_try_field8n(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-0 stack imm 8-bit field arith merge: SADD8N · SSUB8N · SMUL8N
   * (byte-field dual of SADD4N/SSUB4N/SMUL4N; wrap uint8 ALU foundation after SAND8N plane) */
  if (kw(&L->cur,"SADD8N")||kw(&L->cur,"STACKADD8N")||kw(&L->cur,"ADD8N")||
      kw(&L->cur,"SADDBYTEN")||kw(&L->cur,"SADD8IMM")||kw(&L->cur,"SINC8N")){
    /* SADD8N field n — byte n of TOS = (byte + field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (((base >> sh) & 0xFFul) + f) & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUB8N")||kw(&L->cur,"STACKSUB8N")||kw(&L->cur,"SUB8N")||
      kw(&L->cur,"SSUBBYTEN")||kw(&L->cur,"SSUB8IMM")||kw(&L->cur,"SDEC8N")){
    /* SSUB8N field n — byte n of TOS = (byte - field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (((base >> sh) & 0xFFul) - f) & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMUL8N")||kw(&L->cur,"STACKMUL8N")||kw(&L->cur,"MUL8N")||
      kw(&L->cur,"SMULBYTEN")||kw(&L->cur,"SMUL8IMM")||kw(&L->cur,"STIMES8N")){
    /* SMUL8N field n — byte n of TOS = (byte * field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (((base >> sh) & 0xFFul) * f) & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack imm 8-bit field div/mod/min: SDIV8N · SMOD8N · SMIN8N
   * (complete uint8 field ALU after SADD8N/SSUB8N/SMUL8N; field 0 → div/mod 0) */
  if (kw(&L->cur,"SDIV8N")||kw(&L->cur,"STACKDIV8N")||kw(&L->cur,"DIV8N")||
      kw(&L->cur,"SDIVBYTEN")||kw(&L->cur,"SDIV8IMM")||kw(&L->cur,"SQUO8N")){
    /* SDIV8N field n — byte n of TOS = byte / field (field 0 → 0); n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (f == 0) ? 0ul : (v / f);
    long r = (long)((base & ~(0xFFul << sh)) | ((w & 0xFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMOD8N")||kw(&L->cur,"STACKMOD8N")||kw(&L->cur,"MOD8N")||
      kw(&L->cur,"SMODBYTEN")||kw(&L->cur,"SMOD8IMM")||kw(&L->cur,"SREM8N")){
    /* SMOD8N field n — byte n of TOS = byte % field (field 0 → 0); n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (f == 0) ? 0ul : (v % f);
    long r = (long)((base & ~(0xFFul << sh)) | ((w & 0xFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMIN8N")||kw(&L->cur,"STACKMIN8N")||kw(&L->cur,"MIN8N")||
      kw(&L->cur,"SMINBYTEN")||kw(&L->cur,"SMIN8IMM")||kw(&L->cur,"SLE8N")){
    /* SMIN8N field n — byte n of TOS = min(byte, field); n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (v < f) ? v : f;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack imm 8-bit field max+eq: SMAX8N · SEQ8N · SNE8N
   * (complete min/max + equality compare plane after SMIN8N; bool writes 0/1 into byte) */
  if (kw(&L->cur,"SMAX8N")||kw(&L->cur,"STACKMAX8N")||kw(&L->cur,"MAX8N")||
      kw(&L->cur,"SMAXBYTEN")||kw(&L->cur,"SMAX8IMM")||kw(&L->cur,"SGE8N")){
    /* SMAX8N field n — byte n of TOS = max(byte, field); n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (v > f) ? v : f;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SEQ8N")||kw(&L->cur,"STACKEQ8N")||kw(&L->cur,"EQ8N")||
      kw(&L->cur,"SEQBYTEN")||kw(&L->cur,"SEQ8IMM")||kw(&L->cur,"SCMPEQ8N")){
    /* SEQ8N field n — byte n of TOS = (byte == field) ? 1 : 0; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (v == f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNE8N")||kw(&L->cur,"STACKNE8N")||kw(&L->cur,"NE8N")||
      kw(&L->cur,"SNEBYTEN")||kw(&L->cur,"SNE8IMM")||kw(&L->cur,"SCMPNE8N")){
    /* SNE8N field n — byte n of TOS = (byte != field) ? 1 : 0; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (v != f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm 8-bit field ordered-cmp: SLT8N · SGT8N · SLTE8N
   * (byte ladder of SLT16N/SGT16N/SLTE16N; unsigned predicates after SEQ8N/SNE8N) */
  if (kw(&L->cur,"SLT8N")||kw(&L->cur,"STACKLT8N")||kw(&L->cur,"LT8N")||
      kw(&L->cur,"SCMPLT8N")||kw(&L->cur,"SLTBYTEN")||kw(&L->cur,"SLT8IMM")){
    /* SLT8N field n — byte n of TOS = (b < field) ? 1 : 0; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (v < f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGT8N")||kw(&L->cur,"STACKGT8N")||kw(&L->cur,"GT8N")||
      kw(&L->cur,"SCMPGT8N")||kw(&L->cur,"SGTBYTEN")||kw(&L->cur,"SGT8IMM")){
    /* SGT8N field n — byte n of TOS = (b > field) ? 1 : 0; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (v > f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLTE8N")||kw(&L->cur,"STACKLTE8N")||kw(&L->cur,"LTE8N")||
      kw(&L->cur,"SCMPLE8N")||kw(&L->cur,"SLEQ8N")||kw(&L->cur,"SLTEBYTEN")||
      kw(&L->cur,"SLTE8IMM")){
    /* SLTE8N field n — byte n of TOS = (b <= field) ? 1 : 0; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (v <= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack imm 8-bit field GTE: SGTE8N (ladder after SLTE8N) */
  if (kw(&L->cur,"SGTE8N")||kw(&L->cur,"STACKGTE8N")||kw(&L->cur,"GTE8N")||
      kw(&L->cur,"SCMPGE8N")||kw(&L->cur,"SGEQ8N")||kw(&L->cur,"SGTEBYTEN")||
      kw(&L->cur,"SGTE8IMM")){
    /* SGTE8N field n — byte n of TOS = (b >= field) ? 1 : 0; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (v >= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm 8-bit field signed ordered-cmp: SLTS8N · SGTS8N · SLTES8N · SGTES8N
   * (byte ladder of SLTS4N plane; signed int8 after unsigned SLT/SGT/SLTE/SGTE8N) */
  if (kw(&L->cur,"SLTS8N")||kw(&L->cur,"STACKLTS8N")||kw(&L->cur,"LTS8N")||
      kw(&L->cur,"SCMPLTS8N")||kw(&L->cur,"SLTSSIGN8N")||kw(&L->cur,"SLTS8IMM")){
    /* SLTS8N field n — byte n = (int8(b) < int8(field)) ? 1 : 0; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long v = (long)((base >> sh) & 0xFFul);
    if (v & 0x80) v -= 256;
    long f = (long)((unsigned long)field & 0xFFul);
    if (f & 0x80) f -= 256;
    unsigned long w = (v < f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGTS8N")||kw(&L->cur,"STACKGTS8N")||kw(&L->cur,"GTS8N")||
      kw(&L->cur,"SCMPGTS8N")||kw(&L->cur,"SGTSSIGN8N")||kw(&L->cur,"SGTS8IMM")){
    /* SGTS8N field n — byte n = (int8(b) > int8(field)) ? 1 : 0; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long v = (long)((base >> sh) & 0xFFul);
    if (v & 0x80) v -= 256;
    long f = (long)((unsigned long)field & 0xFFul);
    if (f & 0x80) f -= 256;
    unsigned long w = (v > f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLTES8N")||kw(&L->cur,"STACKLTES8N")||kw(&L->cur,"LTES8N")||
      kw(&L->cur,"SCMPLES8N")||kw(&L->cur,"SLEQS8N")||kw(&L->cur,"SLTES8IMM")){
    /* SLTES8N field n — byte n = (int8(b) <= int8(field)) ? 1 : 0; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long v = (long)((base >> sh) & 0xFFul);
    if (v & 0x80) v -= 256;
    long f = (long)((unsigned long)field & 0xFFul);
    if (f & 0x80) f -= 256;
    unsigned long w = (v <= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGTES8N")||kw(&L->cur,"STACKGTES8N")||kw(&L->cur,"GTES8N")||
      kw(&L->cur,"SCMPGES8N")||kw(&L->cur,"SGEQS8N")||kw(&L->cur,"SGTES8IMM")){
    /* SGTES8N field n — byte n = (int8(b) >= int8(field)) ? 1 : 0; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long v = (long)((base >> sh) & 0xFFul);
    if (v & 0x80) v -= 256;
    long f = (long)((unsigned long)field & 0xFFul);
    if (f & 0x80) f -= 256;
    unsigned long w = (v >= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack imm 8-bit field signed min/max: SMINS8N · SMAXS8N
   * (byte ladder of SMINS16N plane; signed select after SLTS8N) */
  if (kw(&L->cur,"SMINS8N")||kw(&L->cur,"STACKMINS8N")||kw(&L->cur,"MINS8N")||
      kw(&L->cur,"SMINSIGN8N")||kw(&L->cur,"SMINS8IMM")||kw(&L->cur,"SMINSI8N")){
    /* SMINS8N field n — byte n = min_signed(int8(b), int8(field)); n 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long v = (long)((base >> sh) & 0xFFul);
    if (v & 0x80) v -= 256;
    long f = (long)((unsigned long)field & 0xFFul);
    if (f & 0x80) f -= 256;
    long m = (v < f) ? v : f;
    unsigned long w = (unsigned long)m & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMAXS8N")||kw(&L->cur,"STACKMAXS8N")||kw(&L->cur,"MAXS8N")||
      kw(&L->cur,"SMAXSIGN8N")||kw(&L->cur,"SMAXS8IMM")||kw(&L->cur,"SMAXSI8N")){
    /* SMAXS8N field n — byte n = max_signed(int8(b), int8(field)); n 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long v = (long)((base >> sh) & 0xFFul);
    if (v & 0x80) v -= 256;
    long f = (long)((unsigned long)field & 0xFFul);
    if (f & 0x80) f -= 256;
    long m = (v > f) ? v : f;
    unsigned long w = (unsigned long)m & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack imm 8-bit field signed clamp+between: SCLAMPS8N · SBETWEENS8N
   * (byte ladder of SCLAMPS16N plane after SMINS8N/SMAXS8N) */
  if (kw(&L->cur,"SCLAMPS8N")||kw(&L->cur,"STACKCLAMPS8N")||kw(&L->cur,"CLAMPS8N")||
      kw(&L->cur,"SCLAMPSIGN8N")||kw(&L->cur,"SCLAMPS8IMM")||kw(&L->cur,"SBOUNDS8N")){
    /* SCLAMPS8N lo hi n — byte n = clamp_signed(int8(b),[lo,hi]); n 0..7 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long slo = (long)((unsigned long)lo & 0xFFul); if (slo & 0x80) slo -= 256;
    long shi = (long)((unsigned long)hi & 0xFFul); if (shi & 0x80) shi -= 256;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long v = (long)((base >> sh) & 0xFFul); if (v & 0x80) v -= 256;
    if (v < slo) v = slo;
    if (v > shi) v = shi;
    unsigned long w = (unsigned long)v & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBETWEENS8N")||kw(&L->cur,"STACKBETWEENS8N")||kw(&L->cur,"BETWEENS8N")||
      kw(&L->cur,"SINRANGES8N")||kw(&L->cur,"SBETWEENS8IMM")||kw(&L->cur,"SINRANGESIGN8N")){
    /* SBETWEENS8N lo hi n — byte n = (lo <= int8(b) <= hi) ? 1 : 0; n 0..7 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long slo = (long)((unsigned long)lo & 0xFFul); if (slo & 0x80) slo -= 256;
    long shi = (long)((unsigned long)hi & 0xFFul); if (shi & 0x80) shi -= 256;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long v = (long)((base >> sh) & 0xFFul); if (v & 0x80) v -= 256;
    unsigned long w = (v >= slo && v <= shi) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack imm 8-bit field signed WITHIN: SWITHINS8N (byte ladder of SWITHINS16N) */
  if (kw(&L->cur,"SWITHINS8N")||kw(&L->cur,"STACKWITHINS8N")||kw(&L->cur,"WITHINS8N")||
      kw(&L->cur,"SWITHINSIGN8N")||kw(&L->cur,"SWITHINS8IMM")||kw(&L->cur,"SINTERVALS8N")){
    /* SWITHINS8N lo hi n — byte n = (lo <= int8(b) < hi) ? 1 : 0; n 0..7; no swap */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long slo = (long)((unsigned long)lo & 0xFFul); if (slo & 0x80) slo -= 256;
    long shi = (long)((unsigned long)hi & 0xFFul); if (shi & 0x80) shi -= 256;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long v = (long)((base >> sh) & 0xFFul); if (v & 0x80) v -= 256;
    unsigned long w = (v >= slo && v < shi) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 8-bit field signed signum: SSIGN8N (byte ladder of SSIGN16N) */
  if (kw(&L->cur,"SSIGN8N")||kw(&L->cur,"STACKSIGN8N")||kw(&L->cur,"SIGN8N")||
      kw(&L->cur,"SSIGNUM8N")||kw(&L->cur,"SGN8N")||kw(&L->cur,"SSIGN8IMM")){
    /* SSIGN8N n — byte n = sgn(int8): +1 / 0 / 0xFF(-1); n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long v = (long)((base >> sh) & 0xFFul); if (v & 0x80) v -= 256;
    unsigned long w = (v > 0) ? 1ul : ((v < 0) ? 0xFFul : 0ul);
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 8-bit field zero-eq: S0EQ8N (byte ladder of S0EQ16N) */
  if (kw(&L->cur,"S0EQ8N")||kw(&L->cur,"STACK0EQ8N")||kw(&L->cur,"SZ8N")||
      kw(&L->cur,"SEQZ8N")||kw(&L->cur,"SISZERO8N")||kw(&L->cur,"S0EQ8IMM")){
    /* S0EQ8N n — byte n = (b == 0) ? 1 : 0; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (v == 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack imm 8-bit field nonzero: S0NE8N (byte ladder of S0NE16N) */
  if (kw(&L->cur,"S0NE8N")||kw(&L->cur,"STACK0NE8N")||kw(&L->cur,"SNZ8N")||
      kw(&L->cur,"SNEZ8N")||kw(&L->cur,"SISNZ8N")||kw(&L->cur,"S0NE8IMM")){
    /* S0NE8N n — byte n = (b != 0) ? 1 : 0; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (v != 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 8-bit field signed LT0: S0LT8N (byte ladder of S0LT16N) */
  if (kw(&L->cur,"S0LT8N")||kw(&L->cur,"STACK0LT8N")||kw(&L->cur,"SLT08N")||
      kw(&L->cur,"SNEGZ8N")||kw(&L->cur,"SISNEG8N")||kw(&L->cur,"S0LT8IMM")){
    /* S0LT8N n — byte n = (int8(b) < 0) ? 1 : 0; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long v = (long)((base >> sh) & 0xFFul); if (v & 0x80) v -= 256;
    unsigned long w = (v < 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm 8-bit field signed GT0: S0GT8N (byte ladder of S0GT16N; completes field signed GT0) */
  if (kw(&L->cur,"S0GT8N")||kw(&L->cur,"STACK0GT8N")||kw(&L->cur,"SGT08N")||
      kw(&L->cur,"SPOSZ8N")||kw(&L->cur,"SISPOS8N")||kw(&L->cur,"S0GT8IMM")){
    /* S0GT8N n — byte n = (int8(b) > 0) ? 1 : 0; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long v = (long)((base >> sh) & 0xFFul); if (v & 0x80) v -= 256;
    unsigned long w = (v > 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm 8-bit field signed LE0: S0LE8N (byte ladder of S0LE16N; completes field signed LE0 all widths) */
  if (kw(&L->cur,"S0LE8N")||kw(&L->cur,"STACK0LE8N")||kw(&L->cur,"SLE08N")||
      kw(&L->cur,"SNONPOS8N")||kw(&L->cur,"SISNONPOS8N")||kw(&L->cur,"S0LE8IMM")){
    /* S0LE8N n — byte n = (int8(b) <= 0) ? 1 : 0; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long v = (long)((base >> sh) & 0xFFul); if (v & 0x80) v -= 256;
    unsigned long w = (v <= 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack imm 8-bit field signed GE0: S0GE8N (byte ladder of S0GE16N; completes field signed GE0 all widths) */
  if (kw(&L->cur,"S0GE8N")||kw(&L->cur,"STACK0GE8N")||kw(&L->cur,"SGE08N")||
      kw(&L->cur,"SNONNEG8N")||kw(&L->cur,"SISNONNEG8N")||kw(&L->cur,"S0GE8IMM")){
    /* S0GE8N n — byte n = (int8(b) >= 0) ? 1 : 0; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long v = (long)((base >> sh) & 0xFFul); if (v & 0x80) v -= 256;
    unsigned long w = (v >= 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack imm 8-bit field abs+extend: SABS8N · SSEXT8N · SZEXT8N
   * (byte ladder of SABS4N/SSEXT4N/SZEXT4N; stack dual of DABS8N/DSEXT8N/DZEXT8N after SNE8N) */
  if (kw(&L->cur,"SABS8N")||kw(&L->cur,"STACKABS8N")||kw(&L->cur,"ABS8N")||
      kw(&L->cur,"SIABS8N")||kw(&L->cur,"SABSBYTEN")||kw(&L->cur,"SBYTEABSN")){
    /* SABS8N n — byte n of TOS = abs(int8); min int8 -128 → 0x80; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long va = (long)((base >> sh) & 0xFFul);
    if (va & 0x80) va -= 256;
    long ar = (va < 0) ? -va : va;
    /* min int8 -128 → +128 does not fit uint8; keep 0x80 (same as DABS8N/SABS4N) */
    unsigned long nv = (unsigned long)(ar & 0xFF);
    long r = (long)((base & ~(0xFFul << sh)) | (nv << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT8N")||kw(&L->cur,"STACKSEXT8N")||kw(&L->cur,"SEXT8N")||
      kw(&L->cur,"SSIGNEXT8N")||kw(&L->cur,"SSEXTBYTEN")||kw(&L->cur,"SBYTESEXTN")){
    /* SSEXT8N n — TOS = sign-extend byte n of TOS to full width; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long va = (long)((base >> sh) & 0xFFul);
    if (va & 0x80) va -= 256; /* sign-extend int8 */
    long r = va;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT8N")||kw(&L->cur,"STACKZEXT8N")||kw(&L->cur,"ZEXT8N")||
      kw(&L->cur,"SZEROEXT8N")||kw(&L->cur,"SZEXTBYTEN")||kw(&L->cur,"SBYTEZEXTN")){
    /* SZEXT8N n — TOS = zero-extend byte n of TOS; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long r = (long)((base >> sh) & 0xFFul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm 8-bit field metrics: SBITREV8N · SPOPCNT8N · SPARITY8N
   * (byte ladder of SBITREV4N/SPOPCNT4N/SPARITY4N; stack dual of DBITREV8N plane after SZEXT8N) */
  if (kw(&L->cur,"SBITREV8N")||kw(&L->cur,"STACKBITREV8N")||kw(&L->cur,"BITREV8N")||
      kw(&L->cur,"SBREV8N")||kw(&L->cur,"SREV8N")||kw(&L->cur,"SBITREVBYTEN")){
    /* SBITREV8N n — byte n of TOS = bitrev8(byte); n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (base >> sh) & 0xFFul;
    unsigned long rv = 0;
    for (int b = 0; b < 8; b++){
      rv = (rv << 1) | (w & 1u);
      w >>= 1;
    }
    long r = (long)((base & ~(0xFFul << sh)) | (rv << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOPCNT8N")||kw(&L->cur,"STACKPOPCNT8N")||kw(&L->cur,"POPCNT8N")||
      kw(&L->cur,"SPCNT8N")||kw(&L->cur,"SPOPBYTEN")||kw(&L->cur,"SBYTEPOPN")){
    /* SPOPCNT8N n — byte n of TOS = popcount(byte); n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (base >> sh) & 0xFFul;
    unsigned long pc = 0;
    while (w){ pc += (w & 1u); w >>= 1; }
    long r = (long)((base & ~(0xFFul << sh)) | ((pc & 0xFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPARITY8N")||kw(&L->cur,"STACKPARITY8N")||kw(&L->cur,"PARITY8N")||
      kw(&L->cur,"SXORRED8N")||kw(&L->cur,"SPARITYBYTEN")||kw(&L->cur,"SBYTEPARN")){
    /* SPARITY8N n — byte n of TOS = xor-reduce(byte) in low bit; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (base >> sh) & 0xFFul;
    unsigned long pr = 0;
    while (w){ pr ^= (w & 1u); w >>= 1; }
    long r = (long)((base & ~(0xFFul << sh)) | (pr << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm 8-bit field zeros+signed: SCLZ8N · SCTZ8N · SNEG8N
   * (byte ladder of SCLZ4N/SCTZ4N/SNEG4N; stack dual of DCLZ8N plane after SPARITY8N) */
  if (kw(&L->cur,"SCLZ8N")||kw(&L->cur,"STACKCLZ8N")||kw(&L->cur,"CLZ8N")||
      kw(&L->cur,"SNLZ8N")||kw(&L->cur,"SCLZBYTEN")||kw(&L->cur,"SBYTECLZN")){
    /* SCLZ8N n — byte n of TOS = clz8(byte); 0 → 8; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    unsigned int w = (unsigned int)((base >> sh) & 0xFFul);
    unsigned long cz = 0;
    if (w == 0) cz = 8;
    else {
      for (int b = 7; b >= 0; b--){
        if (w & (1u << (unsigned)b)) break;
        cz++;
      }
    }
    long r = (long)((base & ~(0xFFul << sh)) | ((cz & 0xFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCTZ8N")||kw(&L->cur,"STACKCTZ8N")||kw(&L->cur,"CTZ8N")||
      kw(&L->cur,"SNTZ8N")||kw(&L->cur,"SCTZBYTEN")||kw(&L->cur,"SBYTECTZN")){
    /* SCTZ8N n — byte n of TOS = ctz8(byte); 0 → 8; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    unsigned int w = (unsigned int)((base >> sh) & 0xFFul);
    unsigned long tz = 0;
    if (w == 0) tz = 8;
    else {
      while ((w & 1u) == 0){ tz++; w >>= 1; }
    }
    long r = (long)((base & ~(0xFFul << sh)) | ((tz & 0xFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNEG8N")||kw(&L->cur,"STACKNEG8N")||kw(&L->cur,"NEG8N")||
      kw(&L->cur,"SINEG8N")||kw(&L->cur,"SNEGBYTEN")||kw(&L->cur,"SBYTENEGN")){
    /* SNEG8N n — byte n of TOS = -(int8) as uint8; min int8 -128 stays 0x80; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long va = (long)((base >> sh) & 0xFFul);
    if (va & 0x80) va -= 256;
    long nr = (va == -128L) ? -128L : -va;
    unsigned long nv = (unsigned long)(nr & 0xFF);
    long r = (long)((base & ~(0xFFul << sh)) | (nv << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 8-bit field unary+rotate: SNOT8N · SROL8N · SROR8N
   * (byte ladder of SNOT4N/SROL4N/SROR4N; stack dual of DNOT8N plane; close stack 8n unary hole) */
  if (kw(&L->cur,"SNOT8N")||kw(&L->cur,"STACKNOT8N")||kw(&L->cur,"NOT8N")||
      kw(&L->cur,"SNOTBYTEN")||kw(&L->cur,"SINV8N")||kw(&L->cur,"SINVERT8N")){
    /* SNOT8N n — byte n of TOS = ~byte & 0xFF; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (~((base >> sh) & 0xFFul)) & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROL8N")||kw(&L->cur,"STACKROL8N")||kw(&L->cur,"ROL8N")||
      kw(&L->cur,"SROTL8N")||kw(&L->cur,"SROLBYTEN")||kw(&L->cur,"SROTBYTEL")){
    /* SROL8N k n — rotl8 byte n of TOS by k&7; n clamped 0..7 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (base >> sh) & 0xFFul;
    unsigned long rot = (kk == 0) ? w : (((w << (unsigned)kk) | (w >> (unsigned)(8 - kk))) & 0xFFul);
    long r = (long)((base & ~(0xFFul << sh)) | (rot << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROR8N")||kw(&L->cur,"STACKROR8N")||kw(&L->cur,"ROR8N")||
      kw(&L->cur,"SROTR8N")||kw(&L->cur,"SRORBYTEN")||kw(&L->cur,"SROTBYTER")){
    /* SROR8N k n — rotr8 byte n of TOS by k&7; n clamped 0..7 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (base >> sh) & 0xFFul;
    unsigned long rot = (kk == 0) ? w : (((w >> (unsigned)kk) | (w << (unsigned)(8 - kk))) & 0xFFul);
    long r = (long)((base & ~(0xFFul << sh)) | (rot << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm 8-bit field shift: SSHL8N · SSHR8N · SSAR8N
   * (byte ladder of SSHL4N/SSHR4N/SSAR4N; stack dual of DSHL8N; complete stack 8n unary plane) */
  if (kw(&L->cur,"SSHL8N")||kw(&L->cur,"STACKSHL8N")||kw(&L->cur,"SHL8N")||
      kw(&L->cur,"SLSH8N")||kw(&L->cur,"SSHLBYTEN")||kw(&L->cur,"SSHIFTBYTEL")){
    /* SSHL8N k n — byte n of TOS = (uint8)<<k (k>=8 → 0); n clamped 0..7 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (base >> sh) & 0xFFul;
    unsigned long nw = (kk >= 8) ? 0ul : ((w << (unsigned)kk) & 0xFFul);
    long r = (long)((base & ~(0xFFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHR8N")||kw(&L->cur,"STACKSHR8N")||kw(&L->cur,"SHR8N")||
      kw(&L->cur,"SLSHR8N")||kw(&L->cur,"SSHRBYTEN")||kw(&L->cur,"SSHIFTBYTER")){
    /* SSHR8N k n — byte n of TOS = (uint8)>>k logical (k>=8 → 0); n clamped 0..7 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (base >> sh) & 0xFFul;
    unsigned long nw = (kk >= 8) ? 0ul : (w >> (unsigned)kk);
    long r = (long)((base & ~(0xFFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSAR8N")||kw(&L->cur,"STACKSAR8N")||kw(&L->cur,"SAR8N")||
      kw(&L->cur,"SASHR8N")||kw(&L->cur,"SSARBYTEN")||kw(&L->cur,"SSARSHIFTBYTE")){
    /* SSAR8N k n — byte n of TOS = (int8)>>k arithmetic (k>=8 → all sign); n clamped 0..7 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 8);
    long va = (long)((base >> sh) & 0xFFul);
    if (va & 0x80) va -= 256; /* sign-extend int8 */
    long shifted;
    if (kk >= 8) shifted = (va < 0) ? -1L : 0L;
    else shifted = va >> kk;
    unsigned long nw = (unsigned long)shifted & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
