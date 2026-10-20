/* CubalC lang — lang_ops_dual_field32.c
 * DUAL imm-32 field plane: DADD32N…D0GE32N (G peel from dual).
 * Mesh vote G_split_dual_cell · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_field32.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_dual_try_field32(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-8 dual-stack imm 32-bit field arith: DADD32N · DSUB32N · DMUL32N
   * (word ladder of DADD16N/DSUB16N/DMUL16N; dual of SADD32N after DPARITY32N) */
  if (kw(&L->cur,"DADD32N")||kw(&L->cur,"S2ADD32N")||kw(&L->cur,"STACK2ADD32N")||
      kw(&L->cur,"PAIRADD32N")||kw(&L->cur,"DADDDIMM")||kw(&L->cur,"DINC32N")||
      kw(&L->cur,"DADDDWN")){
    /* a b + field n → word n of each = (w + field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (((ma >> sh) & 0xFFFFFFFFul) + f) & 0xFFFFFFFFul;
    unsigned long wb = (((mb >> sh) & 0xFFFFFFFFul) + f) & 0xFFFFFFFFul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSUB32N")||kw(&L->cur,"S2SUB32N")||kw(&L->cur,"STACK2SUB32N")||
      kw(&L->cur,"PAIRSUB32N")||kw(&L->cur,"DSUBDIMM")||kw(&L->cur,"DDEC32N")||
      kw(&L->cur,"DSUBDWN")){
    /* a b + field n → word n of each = (w - field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (((ma >> sh) & 0xFFFFFFFFul) - f) & 0xFFFFFFFFul;
    unsigned long wb = (((mb >> sh) & 0xFFFFFFFFul) - f) & 0xFFFFFFFFul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMUL32N")||kw(&L->cur,"S2MUL32N")||kw(&L->cur,"STACK2MUL32N")||
      kw(&L->cur,"PAIRMUL32N")||kw(&L->cur,"DMULDIMM")||kw(&L->cur,"DTIMES32N")||
      kw(&L->cur,"DMULDWN")){
    /* a b + field n → word n of each = (w * field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (((ma >> sh) & 0xFFFFFFFFul) * f) & 0xFFFFFFFFul;
    unsigned long wb = (((mb >> sh) & 0xFFFFFFFFul) * f) & 0xFFFFFFFFul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 32-bit field div/mod/min: DDIV32N · DMOD32N · DMIN32N
   * (word ladder of DDIV16N/DMOD16N/DMIN16N; dual of SDIV32N after DMUL32N) */
  if (kw(&L->cur,"DDIV32N")||kw(&L->cur,"S2DIV32N")||kw(&L->cur,"STACK2DIV32N")||
      kw(&L->cur,"PAIRDIV32N")||kw(&L->cur,"DDIVDIMM")||kw(&L->cur,"DQUO32N")||
      kw(&L->cur,"DDIVDWN")){
    /* a b + field n → word n of each = w / field (field 0 → 0); n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long wa = (f == 0) ? 0ul : (va / f);
    unsigned long wb = (f == 0) ? 0ul : (vb / f);
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | ((wa & 0xFFFFFFFFul) << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | ((wb & 0xFFFFFFFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMOD32N")||kw(&L->cur,"S2MOD32N")||kw(&L->cur,"STACK2MOD32N")||
      kw(&L->cur,"PAIRMOD32N")||kw(&L->cur,"DMODDIMM")||kw(&L->cur,"DREM32N")||
      kw(&L->cur,"DMODDWN")){
    /* a b + field n → word n of each = w % field (field 0 → 0); n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long wa = (f == 0) ? 0ul : (va % f);
    unsigned long wb = (f == 0) ? 0ul : (vb % f);
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | ((wa & 0xFFFFFFFFul) << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | ((wb & 0xFFFFFFFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMIN32N")||kw(&L->cur,"S2MIN32N")||kw(&L->cur,"STACK2MIN32N")||
      kw(&L->cur,"PAIRMIN32N")||kw(&L->cur,"DMINDIMM")||kw(&L->cur,"DLE32N")||
      kw(&L->cur,"DMINDWN")){
    /* a b + field n → word n of each = min(w, field) unsigned; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long wa = (va < f) ? va : f;
    unsigned long wb = (vb < f) ? vb : f;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack imm 32-bit field max+eq: DMAX32N · DEQ32N · DNE32N
   * (word ladder of DMAX16N/DEQ16N/DNE16N; dual of SMAX32N after DMIN32N) */
  if (kw(&L->cur,"DMAX32N")||kw(&L->cur,"S2MAX32N")||kw(&L->cur,"STACK2MAX32N")||
      kw(&L->cur,"PAIRMAX32N")||kw(&L->cur,"DMAXDIMM")||kw(&L->cur,"DGE32N")||
      kw(&L->cur,"DMAXDWN")){
    /* a b + field n → word n of each = max(w, field) unsigned; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long wa = (va > f) ? va : f;
    unsigned long wb = (vb > f) ? vb : f;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DEQ32N")||kw(&L->cur,"S2EQ32N")||kw(&L->cur,"STACK2EQ32N")||
      kw(&L->cur,"PAIREQ32N")||kw(&L->cur,"DEQDIMM")||kw(&L->cur,"DCMPEQ32N")||
      kw(&L->cur,"DEQDWN")){
    /* a b + field n → word n of each = (w == field) ? 1 : 0; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long wa = (va == f) ? 1ul : 0ul;
    unsigned long wb = (vb == f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNE32N")||kw(&L->cur,"S2NE32N")||kw(&L->cur,"STACK2NE32N")||
      kw(&L->cur,"PAIRNE32N")||kw(&L->cur,"DNEDIMM")||kw(&L->cur,"DCMPNE32N")||
      kw(&L->cur,"DNEDWN")){
    /* a b + field n → word n of each = (w != field) ? 1 : 0; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long wa = (va != f) ? 1ul : 0ul;
    unsigned long wb = (vb != f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 dual-stack imm 32-bit field ordered-cmp: DLT32N · DGT32N · DLTE32N
   * (unsigned predicates after DEQ32N/DNE32N; dual of SLT32N/SGT32N/SLTE32N) */
  if (kw(&L->cur,"DLT32N")||kw(&L->cur,"S2LT32N")||kw(&L->cur,"STACK2LT32N")||
      kw(&L->cur,"PAIRLT32N")||kw(&L->cur,"DCMPLT32N")||kw(&L->cur,"DLTDIMM")||
      kw(&L->cur,"DLTDWN")){
    /* a b + field n → word n of each = (w < field) ? 1 : 0; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long wa = (va < f) ? 1ul : 0ul;
    unsigned long wb = (vb < f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGT32N")||kw(&L->cur,"S2GT32N")||kw(&L->cur,"STACK2GT32N")||
      kw(&L->cur,"PAIRGT32N")||kw(&L->cur,"DCMPGT32N")||kw(&L->cur,"DGTDIMM")||
      kw(&L->cur,"DGTDWN")){
    /* a b + field n → word n of each = (w > field) ? 1 : 0; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long wa = (va > f) ? 1ul : 0ul;
    unsigned long wb = (vb > f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLTE32N")||kw(&L->cur,"S2LTE32N")||kw(&L->cur,"STACK2LTE32N")||
      kw(&L->cur,"PAIRLTE32N")||kw(&L->cur,"DCMPLE32N")||kw(&L->cur,"DLEQ32N")||
      kw(&L->cur,"DLTEDIMM")||kw(&L->cur,"DLTEDWN")){
    /* a b + field n → word n of each = (w <= field) ? 1 : 0; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long wa = (va <= f) ? 1ul : 0ul;
    unsigned long wb = (vb <= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 32-bit field GTE: DGTE32N (dual of SGTE32N after DLTE32N) */
  if (kw(&L->cur,"DGTE32N")||kw(&L->cur,"S2GTE32N")||kw(&L->cur,"STACK2GTE32N")||
      kw(&L->cur,"PAIRGTE32N")||kw(&L->cur,"DCMPGE32N")||kw(&L->cur,"DGEQ32N")||
      kw(&L->cur,"DGTEDIMM")||kw(&L->cur,"DGTEDWN")){
    /* a b + field n → word n of each = (w >= field) ? 1 : 0; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFFFFFul;
    unsigned long wa = (va >= f) ? 1ul : 0ul;
    unsigned long wb = (vb >= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 32-bit field signed ordered-cmp: DLTS32N · DGTS32N · DLTES32N · DGTES32N
   * (dual of SLTS32N/SGTS32N/SLTES32N/SGTES32N; signed int32 plane on top two) */
  if (kw(&L->cur,"DLTS32N")||kw(&L->cur,"S2LTS32N")||kw(&L->cur,"STACK2LTS32N")||
      kw(&L->cur,"PAIRLTS32N")||kw(&L->cur,"DCMPLTS32N")||kw(&L->cur,"DLTSSIGN32N")||
      kw(&L->cur,"DLTS32IMM")){
    /* a b + field n → word n of each = (int32(w) < int32(field)) ? 1 : 0; n 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    long f = (long)(int)((unsigned int)((unsigned long)field & 0xFFFFFFFFul));
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)(int)((unsigned int)((ma >> sh) & 0xFFFFFFFFul));
    long vb = (long)(int)((unsigned int)((mb >> sh) & 0xFFFFFFFFul));
    unsigned long wa = (va < f) ? 1ul : 0ul;
    unsigned long wb = (vb < f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTS32N")||kw(&L->cur,"S2GTS32N")||kw(&L->cur,"STACK2GTS32N")||
      kw(&L->cur,"PAIRGTS32N")||kw(&L->cur,"DCMPGTS32N")||kw(&L->cur,"DGTSSIGN32N")||
      kw(&L->cur,"DGTS32IMM")){
    /* a b + field n → word n of each = (int32(w) > int32(field)) ? 1 : 0; n 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    long f = (long)(int)((unsigned int)((unsigned long)field & 0xFFFFFFFFul));
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)(int)((unsigned int)((ma >> sh) & 0xFFFFFFFFul));
    long vb = (long)(int)((unsigned int)((mb >> sh) & 0xFFFFFFFFul));
    unsigned long wa = (va > f) ? 1ul : 0ul;
    unsigned long wb = (vb > f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLTES32N")||kw(&L->cur,"S2LTES32N")||kw(&L->cur,"STACK2LTES32N")||
      kw(&L->cur,"PAIRLTES32N")||kw(&L->cur,"DCMPLES32N")||kw(&L->cur,"DLEQS32N")||
      kw(&L->cur,"DLTES32IMM")){
    /* a b + field n → word n of each = (int32(w) <= int32(field)) ? 1 : 0; n 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    long f = (long)(int)((unsigned int)((unsigned long)field & 0xFFFFFFFFul));
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)(int)((unsigned int)((ma >> sh) & 0xFFFFFFFFul));
    long vb = (long)(int)((unsigned int)((mb >> sh) & 0xFFFFFFFFul));
    unsigned long wa = (va <= f) ? 1ul : 0ul;
    unsigned long wb = (vb <= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTES32N")||kw(&L->cur,"S2GTES32N")||kw(&L->cur,"STACK2GTES32N")||
      kw(&L->cur,"PAIRGTES32N")||kw(&L->cur,"DCMPGES32N")||kw(&L->cur,"DGEQS32N")||
      kw(&L->cur,"DGTES32IMM")){
    /* a b + field n → word n of each = (int32(w) >= int32(field)) ? 1 : 0; n 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    long f = (long)(int)((unsigned int)((unsigned long)field & 0xFFFFFFFFul));
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)(int)((unsigned int)((ma >> sh) & 0xFFFFFFFFul));
    long vb = (long)(int)((unsigned int)((mb >> sh) & 0xFFFFFFFFul));
    unsigned long wa = (va >= f) ? 1ul : 0ul;
    unsigned long wb = (vb >= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack imm 32-bit field signed min/max: DMINS32N · DMAXS32N
   * (dual of SMINS32N/SMAXS32N; signed select after DLTS32N plane) */
  if (kw(&L->cur,"DMINS32N")||kw(&L->cur,"S2MINS32N")||kw(&L->cur,"STACK2MINS32N")||
      kw(&L->cur,"PAIRMINS32N")||kw(&L->cur,"DMINSIGN32N")||kw(&L->cur,"DMINS32IMM")||
      kw(&L->cur,"DMINSI32N")){
    /* a b + field n → word n of each = min_signed(int32(w), int32(field)); n 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    long f = (long)(int)((unsigned int)((unsigned long)field & 0xFFFFFFFFul));
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)(int)((unsigned int)((ma >> sh) & 0xFFFFFFFFul));
    long vb = (long)(int)((unsigned int)((mb >> sh) & 0xFFFFFFFFul));
    long ma_m = (va < f) ? va : f;
    long mb_m = (vb < f) ? vb : f;
    unsigned long wa = (unsigned long)(unsigned int)ma_m;
    unsigned long wb = (unsigned long)(unsigned int)mb_m;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMAXS32N")||kw(&L->cur,"S2MAXS32N")||kw(&L->cur,"STACK2MAXS32N")||
      kw(&L->cur,"PAIRMAXS32N")||kw(&L->cur,"DMAXSIGN32N")||kw(&L->cur,"DMAXS32IMM")||
      kw(&L->cur,"DMAXSI32N")){
    /* a b + field n → word n of each = max_signed(int32(w), int32(field)); n 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    long f = (long)(int)((unsigned int)((unsigned long)field & 0xFFFFFFFFul));
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)(int)((unsigned int)((ma >> sh) & 0xFFFFFFFFul));
    long vb = (long)(int)((unsigned int)((mb >> sh) & 0xFFFFFFFFul));
    long ma_m = (va > f) ? va : f;
    long mb_m = (vb > f) ? vb : f;
    unsigned long wa = (unsigned long)(unsigned int)ma_m;
    unsigned long wb = (unsigned long)(unsigned int)mb_m;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 dual-stack imm 32-bit field signed clamp+between: DCLAMPS32N · DBETWEENS32N
   * (dual of SCLAMPS32N/SBETWEENS32N; signed range after DMINS32N/DMAXS32N) */
  if (kw(&L->cur,"DCLAMPS32N")||kw(&L->cur,"S2CLAMPS32N")||kw(&L->cur,"STACK2CLAMPS32N")||
      kw(&L->cur,"PAIRCLAMPS32N")||kw(&L->cur,"DCLAMPSIGN32N")||kw(&L->cur,"DCLAMPS32IMM")||
      kw(&L->cur,"DBOUNDS32N")){
    /* a b + lo hi n → word n of each = clamp_signed(int32(w),[lo,hi]); n 0..1 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    long slo = (long)(int)((unsigned int)((unsigned long)lo & 0xFFFFFFFFul));
    long shi = (long)(int)((unsigned int)((unsigned long)hi & 0xFFFFFFFFul));
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)(int)((unsigned int)((ma >> sh) & 0xFFFFFFFFul));
    long vb = (long)(int)((unsigned int)((mb >> sh) & 0xFFFFFFFFul));
    if (va < slo) va = slo; if (va > shi) va = shi;
    if (vb < slo) vb = slo; if (vb > shi) vb = shi;
    unsigned long wa = (unsigned long)(unsigned int)va;
    unsigned long wb = (unsigned long)(unsigned int)vb;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DBETWEENS32N")||kw(&L->cur,"S2BETWEENS32N")||kw(&L->cur,"STACK2BETWEENS32N")||
      kw(&L->cur,"PAIRBETWEENS32N")||kw(&L->cur,"DINRANGES32N")||kw(&L->cur,"DBETWEENS32IMM")||
      kw(&L->cur,"DINRANGESIGN32N")){
    /* a b + lo hi n → word n of each = (lo <= int32(w) <= hi) ? 1 : 0; n 0..1 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    long slo = (long)(int)((unsigned int)((unsigned long)lo & 0xFFFFFFFFul));
    long shi = (long)(int)((unsigned int)((unsigned long)hi & 0xFFFFFFFFul));
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)(int)((unsigned int)((ma >> sh) & 0xFFFFFFFFul));
    long vb = (long)(int)((unsigned int)((mb >> sh) & 0xFFFFFFFFul));
    unsigned long wa = (va >= slo && va <= shi) ? 1ul : 0ul;
    unsigned long wb = (vb >= slo && vb <= shi) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 32-bit field signed WITHIN: DWITHINS32N (dual of SWITHINS32N; hi exclusive) */
  if (kw(&L->cur,"DWITHINS32N")||kw(&L->cur,"S2WITHINS32N")||kw(&L->cur,"STACK2WITHINS32N")||
      kw(&L->cur,"PAIRWITHINS32N")||kw(&L->cur,"DWITHINSIGN32N")||kw(&L->cur,"DWITHINS32IMM")||
      kw(&L->cur,"DINTERVALS32N")){
    /* a b + lo hi n → word n of each = (lo <= int32(w) < hi) ? 1 : 0; n 0..1; no swap */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    long slo = (long)(int)((unsigned int)((unsigned long)lo & 0xFFFFFFFFul));
    long shi = (long)(int)((unsigned int)((unsigned long)hi & 0xFFFFFFFFul));
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)(int)((unsigned int)((ma >> sh) & 0xFFFFFFFFul));
    long vb = (long)(int)((unsigned int)((mb >> sh) & 0xFFFFFFFFul));
    unsigned long wa = (va >= slo && va < shi) ? 1ul : 0ul;
    unsigned long wb = (vb >= slo && vb < shi) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 32-bit field signed signum: DSIGN32N (dual of SSIGN32N) */
  if (kw(&L->cur,"DSIGN32N")||kw(&L->cur,"S2SIGN32N")||kw(&L->cur,"STACK2SIGN32N")||
      kw(&L->cur,"PAIRSIGN32N")||kw(&L->cur,"DSIGNUM32N")||kw(&L->cur,"DSGN32N")||
      kw(&L->cur,"DSIGN32IMM")){
    /* a b + n → word n of each = sgn(int32): +1 / 0 / 0xFFFFFFFF; n 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)(int)((unsigned int)((ma >> sh) & 0xFFFFFFFFul));
    long vb = (long)(int)((unsigned int)((mb >> sh) & 0xFFFFFFFFul));
    unsigned long wa = (va > 0) ? 1ul : ((va < 0) ? 0xFFFFFFFFul : 0ul);
    unsigned long wb = (vb > 0) ? 1ul : ((vb < 0) ? 0xFFFFFFFFul : 0ul);
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack imm 32-bit field zero-eq: D0EQ32N (dual of S0EQ32N) */
  if (kw(&L->cur,"D0EQ32N")||kw(&L->cur,"S20EQ32N")||kw(&L->cur,"STACK20EQ32N")||
      kw(&L->cur,"PAIR0EQ32N")||kw(&L->cur,"DZ32N")||kw(&L->cur,"DEQZ32N")||
      kw(&L->cur,"D0EQ32IMM")){
    /* a b + n → word n of each = (w == 0) ? 1 : 0; n 0..1 */
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
    unsigned long wa = (va == 0) ? 1ul : 0ul;
    unsigned long wb = (vb == 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 32-bit field nonzero: D0NE32N (dual of S0NE32N) */
  if (kw(&L->cur,"D0NE32N")||kw(&L->cur,"S20NE32N")||kw(&L->cur,"STACK20NE32N")||
      kw(&L->cur,"PAIR0NE32N")||kw(&L->cur,"DNZ32N")||kw(&L->cur,"DNEZ32N")||
      kw(&L->cur,"D0NE32IMM")){
    /* a b + n → word n of each = (w != 0) ? 1 : 0; n 0..1 */
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
    unsigned long wa = (va != 0) ? 1ul : 0ul;
    unsigned long wb = (vb != 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack imm 32-bit field signed LT0: D0LT32N (dual of S0LT32N) */
  if (kw(&L->cur,"D0LT32N")||kw(&L->cur,"S20LT32N")||kw(&L->cur,"STACK20LT32N")||
      kw(&L->cur,"PAIR0LT32N")||kw(&L->cur,"DLT032N")||kw(&L->cur,"DNEGZ32N")||
      kw(&L->cur,"D0LT32IMM")){
    /* a b + n → word n of each = (int32(w) < 0) ? 1 : 0; n 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)(int)((unsigned int)((ma >> sh) & 0xFFFFFFFFul));
    long vb = (long)(int)((unsigned int)((mb >> sh) & 0xFFFFFFFFul));
    unsigned long wa = (va < 0) ? 1ul : 0ul;
    unsigned long wb = (vb < 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 dual-stack imm 32-bit field signed GT0: D0GT32N (dual of S0GT32N) */
  if (kw(&L->cur,"D0GT32N")||kw(&L->cur,"S20GT32N")||kw(&L->cur,"STACK20GT32N")||
      kw(&L->cur,"PAIR0GT32N")||kw(&L->cur,"DGT032N")||kw(&L->cur,"DPOSZ32N")||
      kw(&L->cur,"D0GT32IMM")){
    /* a b + n → word n of each = (int32(w) > 0) ? 1 : 0; n 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)(int)((unsigned int)((ma >> sh) & 0xFFFFFFFFul));
    long vb = (long)(int)((unsigned int)((mb >> sh) & 0xFFFFFFFFul));
    unsigned long wa = (va > 0) ? 1ul : 0ul;
    unsigned long wb = (vb > 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack imm 32-bit field signed LE0: D0LE32N (dual of S0LE32N) */
  if (kw(&L->cur,"D0LE32N")||kw(&L->cur,"S20LE32N")||kw(&L->cur,"STACK20LE32N")||
      kw(&L->cur,"PAIR0LE32N")||kw(&L->cur,"DLE032N")||kw(&L->cur,"DNONPOS32N")||
      kw(&L->cur,"D0LE32IMM")){
    /* a b + n → word n of each = (int32(w) <= 0) ? 1 : 0; n 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)(int)((unsigned int)((ma >> sh) & 0xFFFFFFFFul));
    long vb = (long)(int)((unsigned int)((mb >> sh) & 0xFFFFFFFFul));
    unsigned long wa = (va <= 0) ? 1ul : 0ul;
    unsigned long wb = (vb <= 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 32-bit field signed GE0: D0GE32N (dual of S0GE32N) */
  if (kw(&L->cur,"D0GE32N")||kw(&L->cur,"S20GE32N")||kw(&L->cur,"STACK20GE32N")||
      kw(&L->cur,"PAIR0GE32N")||kw(&L->cur,"DGE032N")||kw(&L->cur,"DNONNEG32N")||
      kw(&L->cur,"D0GE32IMM")){
    /* a b + n → word n of each = (int32(w) >= 0) ? 1 : 0; n 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)(int)((unsigned int)((ma >> sh) & 0xFFFFFFFFul));
    long vb = (long)(int)((unsigned int)((mb >> sh) & 0xFFFFFFFFul));
    unsigned long wa = (va >= 0) ? 1ul : 0ul;
    unsigned long wb = (vb >= 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
