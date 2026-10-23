/* CubalC lang — lang_ops_math_field16n.c
 * MATH 16-bit field ladder: SADD16N…S0GE16N · SABS/SNOT/SHL · metrics
 * Peel from math residual (B_math_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_math_field16n.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_math_try_field16n(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-3 stack imm 16-bit field arith merge: SADD16N · SSUB16N · SMUL16N
   * (halfword dual of SADD8N/SSUB8N/SMUL8N; wrap uint16 ALU foundation after SAND16N plane) */
  if (kw(&L->cur,"SADD16N")||kw(&L->cur,"STACKADD16N")||kw(&L->cur,"ADD16N")||
      kw(&L->cur,"SADDWORDN")||kw(&L->cur,"SADD16IMM")||kw(&L->cur,"SINC16N")){
    /* SADD16N field n — halfword n of TOS = (hw + field) & 0xFFFF; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (((base >> sh) & 0xFFFFul) + f) & 0xFFFFul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUB16N")||kw(&L->cur,"STACKSUB16N")||kw(&L->cur,"SUB16N")||
      kw(&L->cur,"SSUBWORDN")||kw(&L->cur,"SSUB16IMM")||kw(&L->cur,"SDEC16N")){
    /* SSUB16N field n — halfword n of TOS = (hw - field) & 0xFFFF; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (((base >> sh) & 0xFFFFul) - f) & 0xFFFFul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMUL16N")||kw(&L->cur,"STACKMUL16N")||kw(&L->cur,"MUL16N")||
      kw(&L->cur,"SMULWORDN")||kw(&L->cur,"SMUL16IMM")||kw(&L->cur,"STIMES16N")){
    /* SMUL16N field n — halfword n of TOS = (hw * field) & 0xFFFF; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (((base >> sh) & 0xFFFFul) * f) & 0xFFFFul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 16-bit field div/mod/min: SDIV16N · SMOD16N · SMIN16N
   * (complete uint16 field ALU after SADD16N/SSUB16N/SMUL16N; field 0 → div/mod 0) */
  if (kw(&L->cur,"SDIV16N")||kw(&L->cur,"STACKDIV16N")||kw(&L->cur,"DIV16N")||
      kw(&L->cur,"SDIVWORDN")||kw(&L->cur,"SDIV16IMM")||kw(&L->cur,"SQUO16N")){
    /* SDIV16N field n — halfword n of TOS = hw / field (field 0 → 0); n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long v = (base >> sh) & 0xFFFFul;
    unsigned long w = (f == 0) ? 0ul : (v / f);
    long r = (long)((base & ~(0xFFFFul << sh)) | ((w & 0xFFFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMOD16N")||kw(&L->cur,"STACKMOD16N")||kw(&L->cur,"MOD16N")||
      kw(&L->cur,"SMODWORDN")||kw(&L->cur,"SMOD16IMM")||kw(&L->cur,"SREM16N")){
    /* SMOD16N field n — halfword n of TOS = hw % field (field 0 → 0); n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long v = (base >> sh) & 0xFFFFul;
    unsigned long w = (f == 0) ? 0ul : (v % f);
    long r = (long)((base & ~(0xFFFFul << sh)) | ((w & 0xFFFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMIN16N")||kw(&L->cur,"STACKMIN16N")||kw(&L->cur,"MIN16N")||
      kw(&L->cur,"SMINWORDN")||kw(&L->cur,"SMIN16IMM")||kw(&L->cur,"SLE16N")){
    /* SMIN16N field n — halfword n of TOS = min(hw, field); n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long v = (base >> sh) & 0xFFFFul;
    unsigned long w = (v < f) ? v : f;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 16-bit field max+eq: SMAX16N · SEQ16N · SNE16N
   * (complete min/max + equality compare plane after SMIN16N; bool writes 0/1 into halfword) */
  if (kw(&L->cur,"SMAX16N")||kw(&L->cur,"STACKMAX16N")||kw(&L->cur,"MAX16N")||
      kw(&L->cur,"SMAXWORDN")||kw(&L->cur,"SMAX16IMM")||kw(&L->cur,"SGE16N")){
    /* SMAX16N field n — halfword n of TOS = max(hw, field); n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long v = (base >> sh) & 0xFFFFul;
    unsigned long w = (v > f) ? v : f;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SEQ16N")||kw(&L->cur,"STACKEQ16N")||kw(&L->cur,"EQ16N")||
      kw(&L->cur,"SEQWORDN")||kw(&L->cur,"SEQ16IMM")||kw(&L->cur,"SCMPEQ16N")){
    /* SEQ16N field n — halfword n of TOS = (hw == field) ? 1 : 0; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long v = (base >> sh) & 0xFFFFul;
    unsigned long w = (v == f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNE16N")||kw(&L->cur,"STACKNE16N")||kw(&L->cur,"NE16N")||
      kw(&L->cur,"SNEWORDN")||kw(&L->cur,"SNE16IMM")||kw(&L->cur,"SCMPNE16N")){
    /* SNE16N field n — halfword n of TOS = (hw != field) ? 1 : 0; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long v = (base >> sh) & 0xFFFFul;
    unsigned long w = (v != f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm 16-bit field ordered-cmp: SLT16N · SGT16N · SLTE16N
   * (halfword ladder of SLT32N/SGT32N/SLTE32N; unsigned predicates after SEQ16N/SNE16N) */
  if (kw(&L->cur,"SLT16N")||kw(&L->cur,"STACKLT16N")||kw(&L->cur,"LT16N")||
      kw(&L->cur,"SCMPLT16N")||kw(&L->cur,"SLTWORDN")||kw(&L->cur,"SLT16IMM")){
    /* SLT16N field n — halfword n of TOS = (hw < field) ? 1 : 0; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long v = (base >> sh) & 0xFFFFul;
    unsigned long w = (v < f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGT16N")||kw(&L->cur,"STACKGT16N")||kw(&L->cur,"GT16N")||
      kw(&L->cur,"SCMPGT16N")||kw(&L->cur,"SGTWORDN")||kw(&L->cur,"SGT16IMM")){
    /* SGT16N field n — halfword n of TOS = (hw > field) ? 1 : 0; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long v = (base >> sh) & 0xFFFFul;
    unsigned long w = (v > f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLTE16N")||kw(&L->cur,"STACKLTE16N")||kw(&L->cur,"LTE16N")||
      kw(&L->cur,"SCMPLE16N")||kw(&L->cur,"SLEQ16N")||kw(&L->cur,"SLTEWORDN")||
      kw(&L->cur,"SLTE16IMM")){
    /* SLTE16N field n — halfword n of TOS = (hw <= field) ? 1 : 0; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long v = (base >> sh) & 0xFFFFul;
    unsigned long w = (v <= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack imm 16-bit field GTE: SGTE16N (ladder after SLTE16N) */
  if (kw(&L->cur,"SGTE16N")||kw(&L->cur,"STACKGTE16N")||kw(&L->cur,"GTE16N")||
      kw(&L->cur,"SCMPGE16N")||kw(&L->cur,"SGEQ16N")||kw(&L->cur,"SGTEWORDN")||
      kw(&L->cur,"SGTE16IMM")){
    /* SGTE16N field n — halfword n of TOS = (hw >= field) ? 1 : 0; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long v = (base >> sh) & 0xFFFFul;
    unsigned long w = (v >= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm 16-bit field signed ordered-cmp: SLTS16N · SGTS16N · SLTES16N · SGTES16N
   * (halfword ladder of SLTS8N plane; signed int16 after unsigned SGTE16N) */
  if (kw(&L->cur,"SLTS16N")||kw(&L->cur,"STACKLTS16N")||kw(&L->cur,"LTS16N")||
      kw(&L->cur,"SCMPLTS16N")||kw(&L->cur,"SLTSSIGN16N")||kw(&L->cur,"SLTS16IMM")){
    /* SLTS16N field n — halfword n = (int16(hw) < int16(field)) ? 1 : 0; n 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long v = (long)((base >> sh) & 0xFFFFul);
    if (v & 0x8000) v -= 65536;
    long f = (long)((unsigned long)field & 0xFFFFul);
    if (f & 0x8000) f -= 65536;
    unsigned long w = (v < f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGTS16N")||kw(&L->cur,"STACKGTS16N")||kw(&L->cur,"GTS16N")||
      kw(&L->cur,"SCMPGTS16N")||kw(&L->cur,"SGTSSIGN16N")||kw(&L->cur,"SGTS16IMM")){
    /* SGTS16N field n — halfword n = (int16(hw) > int16(field)) ? 1 : 0; n 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long v = (long)((base >> sh) & 0xFFFFul);
    if (v & 0x8000) v -= 65536;
    long f = (long)((unsigned long)field & 0xFFFFul);
    if (f & 0x8000) f -= 65536;
    unsigned long w = (v > f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLTES16N")||kw(&L->cur,"STACKLTES16N")||kw(&L->cur,"LTES16N")||
      kw(&L->cur,"SCMPLES16N")||kw(&L->cur,"SLEQS16N")||kw(&L->cur,"SLTES16IMM")){
    /* SLTES16N field n — halfword n = (int16(hw) <= int16(field)) ? 1 : 0; n 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long v = (long)((base >> sh) & 0xFFFFul);
    if (v & 0x8000) v -= 65536;
    long f = (long)((unsigned long)field & 0xFFFFul);
    if (f & 0x8000) f -= 65536;
    unsigned long w = (v <= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGTES16N")||kw(&L->cur,"STACKGTES16N")||kw(&L->cur,"GTES16N")||
      kw(&L->cur,"SCMPGES16N")||kw(&L->cur,"SGEQS16N")||kw(&L->cur,"SGTES16IMM")){
    /* SGTES16N field n — halfword n = (int16(hw) >= int16(field)) ? 1 : 0; n 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long v = (long)((base >> sh) & 0xFFFFul);
    if (v & 0x8000) v -= 65536;
    long f = (long)((unsigned long)field & 0xFFFFul);
    if (f & 0x8000) f -= 65536;
    unsigned long w = (v >= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack imm 16-bit field signed min/max: SMINS16N · SMAXS16N
   * (halfword ladder of SMINS32N plane; signed select after SLTS16N) */
  if (kw(&L->cur,"SMINS16N")||kw(&L->cur,"STACKMINS16N")||kw(&L->cur,"MINS16N")||
      kw(&L->cur,"SMINSIGN16N")||kw(&L->cur,"SMINS16IMM")||kw(&L->cur,"SMINSI16N")){
    /* SMINS16N field n — halfword n = min_signed(int16(hw), int16(field)); n 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long v = (long)((base >> sh) & 0xFFFFul);
    if (v & 0x8000) v -= 65536;
    long f = (long)((unsigned long)field & 0xFFFFul);
    if (f & 0x8000) f -= 65536;
    long m = (v < f) ? v : f;
    unsigned long w = (unsigned long)m & 0xFFFFul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMAXS16N")||kw(&L->cur,"STACKMAXS16N")||kw(&L->cur,"MAXS16N")||
      kw(&L->cur,"SMAXSIGN16N")||kw(&L->cur,"SMAXS16IMM")||kw(&L->cur,"SMAXSI16N")){
    /* SMAXS16N field n — halfword n = max_signed(int16(hw), int16(field)); n 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long v = (long)((base >> sh) & 0xFFFFul);
    if (v & 0x8000) v -= 65536;
    long f = (long)((unsigned long)field & 0xFFFFul);
    if (f & 0x8000) f -= 65536;
    long m = (v > f) ? v : f;
    unsigned long w = (unsigned long)m & 0xFFFFul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 16-bit field signed clamp+between: SCLAMPS16N · SBETWEENS16N
   * (halfword ladder of SCLAMPS32N plane after SMINS16N/SMAXS16N) */
  if (kw(&L->cur,"SCLAMPS16N")||kw(&L->cur,"STACKCLAMPS16N")||kw(&L->cur,"CLAMPS16N")||
      kw(&L->cur,"SCLAMPSIGN16N")||kw(&L->cur,"SCLAMPS16IMM")||kw(&L->cur,"SBOUNDS16N")){
    /* SCLAMPS16N lo hi n — halfword n = clamp_signed(int16(hw),[lo,hi]); n 0..3 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long slo = (long)((unsigned long)lo & 0xFFFFul); if (slo & 0x8000) slo -= 65536;
    long shi = (long)((unsigned long)hi & 0xFFFFul); if (shi & 0x8000) shi -= 65536;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long v = (long)((base >> sh) & 0xFFFFul); if (v & 0x8000) v -= 65536;
    if (v < slo) v = slo;
    if (v > shi) v = shi;
    unsigned long w = (unsigned long)v & 0xFFFFul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBETWEENS16N")||kw(&L->cur,"STACKBETWEENS16N")||kw(&L->cur,"BETWEENS16N")||
      kw(&L->cur,"SINRANGES16N")||kw(&L->cur,"SBETWEENS16IMM")||kw(&L->cur,"SINRANGESIGN16N")){
    /* SBETWEENS16N lo hi n — halfword n = (lo <= int16(hw) <= hi) ? 1 : 0; n 0..3 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long slo = (long)((unsigned long)lo & 0xFFFFul); if (slo & 0x8000) slo -= 65536;
    long shi = (long)((unsigned long)hi & 0xFFFFul); if (shi & 0x8000) shi -= 65536;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long v = (long)((base >> sh) & 0xFFFFul); if (v & 0x8000) v -= 65536;
    unsigned long w = (v >= slo && v <= shi) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 16-bit field signed WITHIN: SWITHINS16N (halfword ladder of SWITHINS32N) */
  if (kw(&L->cur,"SWITHINS16N")||kw(&L->cur,"STACKWITHINS16N")||kw(&L->cur,"WITHINS16N")||
      kw(&L->cur,"SWITHINSIGN16N")||kw(&L->cur,"SWITHINS16IMM")||kw(&L->cur,"SINTERVALS16N")){
    /* SWITHINS16N lo hi n — halfword n = (lo <= int16(hw) < hi) ? 1 : 0; n 0..3; no swap */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long slo = (long)((unsigned long)lo & 0xFFFFul); if (slo & 0x8000) slo -= 65536;
    long shi = (long)((unsigned long)hi & 0xFFFFul); if (shi & 0x8000) shi -= 65536;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long v = (long)((base >> sh) & 0xFFFFul); if (v & 0x8000) v -= 65536;
    unsigned long w = (v >= slo && v < shi) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm 16-bit field signed signum: SSIGN16N (halfword ladder; field dual of SSIGN) */
  if (kw(&L->cur,"SSIGN16N")||kw(&L->cur,"STACKSIGN16N")||kw(&L->cur,"SIGN16N")||
      kw(&L->cur,"SSIGNUM16N")||kw(&L->cur,"SGN16N")||kw(&L->cur,"SSIGN16IMM")){
    /* SSIGN16N n — halfword n = sgn(int16): +1 / 0 / 0xFFFF(-1); n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long v = (long)((base >> sh) & 0xFFFFul); if (v & 0x8000) v -= 65536;
    unsigned long w = (v > 0) ? 1ul : ((v < 0) ? 0xFFFFul : 0ul);
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm 16-bit field zero-eq: S0EQ16N (halfword ladder of S0EQ32N; field dual of S0EQ/SZ) */
  if (kw(&L->cur,"S0EQ16N")||kw(&L->cur,"STACK0EQ16N")||kw(&L->cur,"SZ16N")||
      kw(&L->cur,"SEQZ16N")||kw(&L->cur,"SISZERO16N")||kw(&L->cur,"S0EQ16IMM")){
    /* S0EQ16N n — halfword n = (hw == 0) ? 1 : 0; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long v = (base >> sh) & 0xFFFFul;
    unsigned long w = (v == 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack imm 16-bit field nonzero: S0NE16N (halfword ladder of S0NE32N; field dual of S0NE/SNZ) */
  if (kw(&L->cur,"S0NE16N")||kw(&L->cur,"STACK0NE16N")||kw(&L->cur,"SNZ16N")||
      kw(&L->cur,"SNEZ16N")||kw(&L->cur,"SISNZ16N")||kw(&L->cur,"S0NE16IMM")){
    /* S0NE16N n — halfword n = (hw != 0) ? 1 : 0; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long v = (base >> sh) & 0xFFFFul;
    unsigned long w = (v != 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack imm 16-bit field signed LT0: S0LT16N (halfword ladder of S0LT32N; field dual of S0LT) */
  if (kw(&L->cur,"S0LT16N")||kw(&L->cur,"STACK0LT16N")||kw(&L->cur,"SLT016N")||
      kw(&L->cur,"SNEGZ16N")||kw(&L->cur,"SISNEG16N")||kw(&L->cur,"S0LT16IMM")){
    /* S0LT16N n — halfword n = (int16(hw) < 0) ? 1 : 0; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long v = (long)((base >> sh) & 0xFFFFul); if (v & 0x8000) v -= 65536;
    unsigned long w = (v < 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm 16-bit field signed GT0: S0GT16N (halfword ladder of S0GT32N; field dual of S0GT) */
  if (kw(&L->cur,"S0GT16N")||kw(&L->cur,"STACK0GT16N")||kw(&L->cur,"SGT016N")||
      kw(&L->cur,"SPOSZ16N")||kw(&L->cur,"SISPOS16N")||kw(&L->cur,"S0GT16IMM")){
    /* S0GT16N n — halfword n = (int16(hw) > 0) ? 1 : 0; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long v = (long)((base >> sh) & 0xFFFFul); if (v & 0x8000) v -= 65536;
    unsigned long w = (v > 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm 16-bit field signed LE0: S0LE16N (halfword ladder of S0LE32N; field dual of S0LE) */
  if (kw(&L->cur,"S0LE16N")||kw(&L->cur,"STACK0LE16N")||kw(&L->cur,"SLE016N")||
      kw(&L->cur,"SNONPOS16N")||kw(&L->cur,"SISNONPOS16N")||kw(&L->cur,"S0LE16IMM")){
    /* S0LE16N n — halfword n = (int16(hw) <= 0) ? 1 : 0; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long v = (long)((base >> sh) & 0xFFFFul); if (v & 0x8000) v -= 65536;
    unsigned long w = (v <= 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack imm 16-bit field signed GE0: S0GE16N (halfword ladder of S0GE32N; field dual of S0GE) */
  if (kw(&L->cur,"S0GE16N")||kw(&L->cur,"STACK0GE16N")||kw(&L->cur,"SGE016N")||
      kw(&L->cur,"SNONNEG16N")||kw(&L->cur,"SISNONNEG16N")||kw(&L->cur,"S0GE16IMM")){
    /* S0GE16N n — halfword n = (int16(hw) >= 0) ? 1 : 0; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long v = (long)((base >> sh) & 0xFFFFul); if (v & 0x8000) v -= 65536;
    unsigned long w = (v >= 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm 16-bit field abs+extend: SABS16N · SSEXT16N · SZEXT16N
   * (halfword ladder of SABS8N/SSEXT8N/SZEXT8N; signed extract after SNE16N ALU plane) */
  if (kw(&L->cur,"SABS16N")||kw(&L->cur,"STACKABS16N")||kw(&L->cur,"ABS16N")||
      kw(&L->cur,"SIABS16N")||kw(&L->cur,"SABSWORDN")||kw(&L->cur,"SWORDABSN")){
    /* SABS16N n — halfword n of TOS = abs(int16); min int16 -32768 → 0x8000; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long va = (long)((base >> sh) & 0xFFFFul);
    if (va & 0x8000) va -= 65536;
    long ar = (va < 0) ? -va : va;
    /* min int16 -32768 → +32768 does not fit uint16; keep 0x8000 (same as SABS8N min path) */
    unsigned long nv = (unsigned long)(ar & 0xFFFF);
    long r = (long)((base & ~(0xFFFFul << sh)) | (nv << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT16N")||kw(&L->cur,"STACKSEXT16N")||kw(&L->cur,"SEXT16N")||
      kw(&L->cur,"SSIGNEXT16N")||kw(&L->cur,"SSEXTWORDN")||kw(&L->cur,"SWORDSEXTN")){
    /* SSEXT16N n — TOS = sign-extend halfword n of TOS to full width; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long va = (long)((base >> sh) & 0xFFFFul);
    if (va & 0x8000) va -= 65536; /* sign-extend int16 */
    long r = va;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT16N")||kw(&L->cur,"STACKZEXT16N")||kw(&L->cur,"ZEXT16N")||
      kw(&L->cur,"SZEROEXT16N")||kw(&L->cur,"SZEXTWORDN")||kw(&L->cur,"SWORDZEXTN")){
    /* SZEXT16N n — TOS = zero-extend halfword n of TOS; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long r = (long)((base >> sh) & 0xFFFFul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack imm 16-bit field unary+rotate: SNOT16N · SROL16N · SROR16N
   * (halfword ladder of SNOT8N/SROL8N/SROR8N; stack dual of DNOT16N after SZEXT16N) */
  if (kw(&L->cur,"SNOT16N")||kw(&L->cur,"STACKNOT16N")||kw(&L->cur,"NOT16N")||
      kw(&L->cur,"SNOTWORDN")||kw(&L->cur,"SINV16N")||kw(&L->cur,"SINVERT16N")){
    /* SNOT16N n — halfword n of TOS = ~hw & 0xFFFF; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (~((base >> sh) & 0xFFFFul)) & 0xFFFFul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROL16N")||kw(&L->cur,"STACKROL16N")||kw(&L->cur,"ROL16N")||
      kw(&L->cur,"SROTL16N")||kw(&L->cur,"SROLWORDN")||kw(&L->cur,"SROTWORDL")){
    /* SROL16N k n — rotl16 halfword n of TOS by k&15; n clamped 0..3 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (base >> sh) & 0xFFFFul;
    unsigned long rot = (kk == 0) ? w : (((w << (unsigned)kk) | (w >> (unsigned)(16 - kk))) & 0xFFFFul);
    long r = (long)((base & ~(0xFFFFul << sh)) | (rot << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROR16N")||kw(&L->cur,"STACKROR16N")||kw(&L->cur,"ROR16N")||
      kw(&L->cur,"SROTR16N")||kw(&L->cur,"SRORWORDN")||kw(&L->cur,"SROTWORDR")){
    /* SROR16N k n — rotr16 halfword n of TOS by k&15; n clamped 0..3 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (base >> sh) & 0xFFFFul;
    unsigned long rot = (kk == 0) ? w : (((w >> (unsigned)kk) | (w << (unsigned)(16 - kk))) & 0xFFFFul);
    long r = (long)((base & ~(0xFFFFul << sh)) | (rot << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack imm 16-bit field shift: SSHL16N · SSHR16N · SSAR16N
   * (halfword ladder of SSHL8N/SSHR8N/SSAR8N; stack dual of DSHL16N after SNOT16N/SROL16N) */
  if (kw(&L->cur,"SSHL16N")||kw(&L->cur,"STACKSHL16N")||kw(&L->cur,"SHL16N")||
      kw(&L->cur,"SLSH16N")||kw(&L->cur,"SSHLWORDN")||kw(&L->cur,"SSHIFTWORDL")){
    /* SSHL16N k n — halfword n of TOS = (uint16)<<k (k>=16 → 0); n clamped 0..3 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (base >> sh) & 0xFFFFul;
    unsigned long nw = (kk >= 16) ? 0ul : ((w << (unsigned)kk) & 0xFFFFul);
    long r = (long)((base & ~(0xFFFFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHR16N")||kw(&L->cur,"STACKSHR16N")||kw(&L->cur,"SHR16N")||
      kw(&L->cur,"SLSHR16N")||kw(&L->cur,"SSHRWORDN")||kw(&L->cur,"SSHIFTWORDR")){
    /* SSHR16N k n — halfword n of TOS = (uint16)>>k logical (k>=16 → 0); n clamped 0..3 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (base >> sh) & 0xFFFFul;
    unsigned long nw = (kk >= 16) ? 0ul : (w >> (unsigned)kk);
    long r = (long)((base & ~(0xFFFFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSAR16N")||kw(&L->cur,"STACKSAR16N")||kw(&L->cur,"SAR16N")||
      kw(&L->cur,"SASHR16N")||kw(&L->cur,"SSARWORDN")||kw(&L->cur,"SSARSHIFTWORD")){
    /* SSAR16N k n — halfword n of TOS = (int16)>>k arithmetic (k>=16 → all sign); n clamped 0..3 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long va = (long)((base >> sh) & 0xFFFFul);
    if (va & 0x8000) va -= 65536; /* sign-extend int16 */
    long shifted;
    if (kk >= 16) shifted = (va < 0) ? -1L : 0L;
    else shifted = va >> kk;
    unsigned long nw = (unsigned long)shifted & 0xFFFFul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack imm 16-bit field metrics: SBITREV16N · SPOPCNT16N · SPARITY16N
   * (halfword ladder of SBITREV8N/SPOPCNT8N/SPARITY8N; metrics after SSHL16N plane) */
  if (kw(&L->cur,"SBITREV16N")||kw(&L->cur,"STACKBITREV16N")||kw(&L->cur,"BITREV16N")||
      kw(&L->cur,"SBREV16N")||kw(&L->cur,"SREV16N")||kw(&L->cur,"SBITREVWORDN")){
    /* SBITREV16N n — halfword n of TOS = bitrev16(hw); n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (base >> sh) & 0xFFFFul;
    unsigned long rv = 0;
    for (int b = 0; b < 16; b++){
      rv = (rv << 1) | (w & 1u);
      w >>= 1;
    }
    long r = (long)((base & ~(0xFFFFul << sh)) | ((rv & 0xFFFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOPCNT16N")||kw(&L->cur,"STACKPOPCNT16N")||kw(&L->cur,"POPCNT16N")||
      kw(&L->cur,"SPCNT16N")||kw(&L->cur,"SPOPWORDN")||kw(&L->cur,"SWORDPOPN")){
    /* SPOPCNT16N n — halfword n of TOS = popcount(hw); n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (base >> sh) & 0xFFFFul;
    unsigned long pc = 0;
    while (w){ pc += (w & 1u); w >>= 1; }
    long r = (long)((base & ~(0xFFFFul << sh)) | ((pc & 0xFFFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPARITY16N")||kw(&L->cur,"STACKPARITY16N")||kw(&L->cur,"PARITY16N")||
      kw(&L->cur,"SXORRED16N")||kw(&L->cur,"SPARITYWORDN")||kw(&L->cur,"SWORDPARN")){
    /* SPARITY16N n — halfword n of TOS = xor-reduce(hw) in low bit; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (base >> sh) & 0xFFFFul;
    unsigned long pr = 0;
    while (w){ pr ^= (w & 1u); w >>= 1; }
    long r = (long)((base & ~(0xFFFFul << sh)) | (pr << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack imm 16-bit field zeros+signed: SCLZ16N · SCTZ16N · SNEG16N
   * (halfword ladder of SCLZ8N/SCTZ8N/SNEG8N; complete stack 16n unary after SPARITY16N) */
  if (kw(&L->cur,"SCLZ16N")||kw(&L->cur,"STACKCLZ16N")||kw(&L->cur,"CLZ16N")||
      kw(&L->cur,"SNLZ16N")||kw(&L->cur,"SCLZWORDN")||kw(&L->cur,"SWORDCLZN")){
    /* SCLZ16N n — halfword n of TOS = clz16(hw); 0 → 16; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    unsigned int w = (unsigned int)((base >> sh) & 0xFFFFul);
    unsigned long cz = 0;
    if (w == 0) cz = 16;
    else {
      for (int b = 15; b >= 0; b--){
        if (w & (1u << (unsigned)b)) break;
        cz++;
      }
    }
    long r = (long)((base & ~(0xFFFFul << sh)) | ((cz & 0xFFFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCTZ16N")||kw(&L->cur,"STACKCTZ16N")||kw(&L->cur,"CTZ16N")||
      kw(&L->cur,"SNTZ16N")||kw(&L->cur,"SCTZWORDN")||kw(&L->cur,"SWORDCTZN")){
    /* SCTZ16N n — halfword n of TOS = ctz16(hw); 0 → 16; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    unsigned int w = (unsigned int)((base >> sh) & 0xFFFFul);
    unsigned long tz = 0;
    if (w == 0) tz = 16;
    else {
      while ((w & 1u) == 0){ tz++; w >>= 1; }
    }
    long r = (long)((base & ~(0xFFFFul << sh)) | ((tz & 0xFFFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNEG16N")||kw(&L->cur,"STACKNEG16N")||kw(&L->cur,"NEG16N")||
      kw(&L->cur,"SINEG16N")||kw(&L->cur,"SNEGWORDN")||kw(&L->cur,"SWORDNEGN")){
    /* SNEG16N n — halfword n of TOS = -(int16) as uint16; min int16 -32768 stays 0x8000; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 16);
    long va = (long)((base >> sh) & 0xFFFFul);
    if (va & 0x8000) va -= 65536;
    long nr = (va == -32768L) ? -32768L : -va;
    unsigned long nv = (unsigned long)(nr & 0xFFFF);
    long r = (long)((base & ~(0xFFFFul << sh)) | (nv << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
