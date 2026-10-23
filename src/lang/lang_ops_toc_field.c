/* CubalC lang — lang_ops_toc_field.c
 * TOC field plane: BYTE · WORD/16 · NIB get/set/clr (imm + stack dual)
 * Peel from toc (B_toc_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_toc_field.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_toc_try_field(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-1 imm byte field TOC: SBYTETOCN · SSETBYTETOCN · SCLRBYTETOCN
   * (imm dual of SBYTEN/SSETBYTEN/SCLRBYTEN into cell; complete bitfield TOC with LE bytes) */
  if (kw(&L->cur,"SBYTETOCN")||kw(&L->cur,"SGETBYTETOCN")||kw(&L->cur,"STACKBYTETOCN")||
      kw(&L->cur,"SBYTETOCIMM")||kw(&L->cur,"BYTETOCN")||kw(&L->cur,"SCELLBYTEN")||
      kw(&L->cur,"GETBYTETOCN")){
    /* i + n → cells[i] = LE byte n of cells[i]; n clamped 0..7; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 8)) & 0xFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETBYTETOCN")||kw(&L->cur,"SSETBYTOCIMM")||kw(&L->cur,"STACKSETBYTETOCN")||
      kw(&L->cur,"SETBYTETOCN")||kw(&L->cur,"SCELLSETBYTEN")||kw(&L->cur,"PUTBYTETOCN")||
      kw(&L->cur,"SSETBYATN")){
    /* i + field + n → deposit low 8 bits of field into LE byte n of cells[i]; leave result */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long shift = (unsigned long)(n * 8);
    long r = (long)((base & ~(0xFFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRBYTETOCN")||kw(&L->cur,"SCLRBYTOCIMM")||kw(&L->cur,"STACKCLRBYTETOCN")||
      kw(&L->cur,"CLRBYTETOCN")||kw(&L->cur,"SCELLCLRBYTEN")||kw(&L->cur,"ZAPBYTETOCN")||
      kw(&L->cur,"SCLRBYATN")){
    /* i + n → clear LE byte n of cells[i]; n clamped 0..7; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 8);
    long r = (long)(base & ~(0xFFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack dual byte field TOC: SBYTETOC · SSETBYTETOC · SCLRBYTETOC
   * (stack dual of SBYTETOCN/SSETBYTETOCN/SCLRBYTETOCN; LE bytes from stack) */
  if (kw(&L->cur,"SBYTETOC")||kw(&L->cur,"SGETBYTETOC")||kw(&L->cur,"STACKBYTETOC")||
      kw(&L->cur,"BYTETOC")||kw(&L->cur,"SCELLBYTES")||kw(&L->cur,"GETBYTETOC")||
      kw(&L->cur,"SBYTEAT")){
    /* i n → cells[i] = LE byte n of cells[i]; n clamped 0..7; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 8)) & 0xFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETBYTETOC")||kw(&L->cur,"STACKSETBYTETOC")||kw(&L->cur,"SETBYTETOC")||
      kw(&L->cur,"SCELLSETBYTES")||kw(&L->cur,"PUTBYTETOC")||kw(&L->cur,"SSETBYAT")||
      kw(&L->cur,"SDEPOSITBYTE")){
    /* i field n → deposit low 8 bits of field into LE byte n of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long shift = (unsigned long)(n * 8);
    long r = (long)((base & ~(0xFFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRBYTETOC")||kw(&L->cur,"STACKCLRBYTETOC")||kw(&L->cur,"CLRBYTETOC")||
      kw(&L->cur,"SCELLCLRBYTES")||kw(&L->cur,"ZAPBYTETOC")||kw(&L->cur,"SCLRBYAT")||
      kw(&L->cur,"SZAPBYTE")){
    /* i n → clear LE byte n of cells[i]; n clamped 0..7; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 8);
    long r = (long)(base & ~(0xFFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 imm halfword field TOC: SWORDTOCN · SSET16TOCN · SCLR16TOCN
   * (imm dual of SWORDN/SSET16N/SCLR16N into cell; complete byte TOC ladder with 16-bit) */
  if (kw(&L->cur,"SWORDTOCN")||kw(&L->cur,"SGET16TOCN")||kw(&L->cur,"STACKWORDTOCN")||
      kw(&L->cur,"SWORDTOCIMM")||kw(&L->cur,"WORDTOCN")||kw(&L->cur,"SCELLWORDN")||
      kw(&L->cur,"SHALFTOCN")||kw(&L->cur,"GET16TOCN")){
    /* i + n → cells[i] = LE 16-bit halfword n of cells[i]; n clamped 0..3; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 16)) & 0xFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSET16TOCN")||kw(&L->cur,"SSET16TOCIMM")||kw(&L->cur,"STACKSET16TOCN")||
      kw(&L->cur,"SET16TOCN")||kw(&L->cur,"SCELLSET16N")||kw(&L->cur,"SSETWORDTOCN")||
      kw(&L->cur,"PUT16TOCN")||kw(&L->cur,"SSETHALFTOCN")){
    /* i + field + n → deposit low 16 bits of field into LE halfword n of cells[i] */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long shift = (unsigned long)(n * 16);
    long r = (long)((base & ~(0xFFFFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLR16TOCN")||kw(&L->cur,"SCLR16TOCIMM")||kw(&L->cur,"STACKCLR16TOCN")||
      kw(&L->cur,"CLR16TOCN")||kw(&L->cur,"SCELLCLR16N")||kw(&L->cur,"SCLRWORDTOCN")||
      kw(&L->cur,"ZAP16TOCN")||kw(&L->cur,"SCLRHAFTOCN")){
    /* i + n → clear LE halfword n of cells[i]; n clamped 0..3; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 16);
    long r = (long)(base & ~(0xFFFFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack dual halfword field TOC: SWORDTOC · SSET16TOC · SCLR16TOC
   * (stack dual of SWORDTOCN/SSET16TOCN/SCLR16TOCN; LE 16-bit after SBYTETOC plane) */
  if (kw(&L->cur,"SWORDTOC")||kw(&L->cur,"SGET16TOC")||kw(&L->cur,"STACKWORDTOC")||
      kw(&L->cur,"WORDTOC")||kw(&L->cur,"SCELLWORDS")||kw(&L->cur,"SHALFWORDTOC")||
      kw(&L->cur,"GET16TOC")||kw(&L->cur,"SWORDAT")){
    /* i n → cells[i] = LE 16-bit halfword n of cells[i]; n clamped 0..3; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 16)) & 0xFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSET16TOC")||kw(&L->cur,"STACKSET16TOC")||kw(&L->cur,"SET16TOC")||
      kw(&L->cur,"SCELLSET16S")||kw(&L->cur,"SSETWORDTOC")||kw(&L->cur,"PUT16TOC")||
      kw(&L->cur,"SSETHALFTOC")||kw(&L->cur,"SSET16AT")){
    /* i field n → deposit low 16 bits of field into LE halfword n of cells[i] */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long shift = (unsigned long)(n * 16);
    long r = (long)((base & ~(0xFFFFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLR16TOC")||kw(&L->cur,"STACKCLR16TOC")||kw(&L->cur,"CLR16TOC")||
      kw(&L->cur,"SCELLCLR16S")||kw(&L->cur,"SCLRWORDTOC")||kw(&L->cur,"ZAP16TOC")||
      kw(&L->cur,"SCLRHAFTOC")||kw(&L->cur,"SCLR16AT")){
    /* i n → clear LE halfword n of cells[i]; n clamped 0..3; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 16);
    long r = (long)(base & ~(0xFFFFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 imm ceil/floor div TOC: SDIVCEILTOCN · SDIVFLOORTOCN
   * (imm dual of SDIVCEILN/SDIVFLOORN into cell; complete SDIVTOCN plane with rounding modes) */
  if (kw(&L->cur,"SDIVCEILTOCN")||kw(&L->cur,"SCEILDIVTOCN")||kw(&L->cur,"STACKDIVCEILTOCN")||
      kw(&L->cur,"SDIVCEILTOCIMM")||kw(&L->cur,"CEILDIVTOCN")||kw(&L->cur,"SCELLDIVCEILN")||
      kw(&L->cur,"CEILTOCN")){
    /* i + n → cells[i] = ceil(cells[i]/n); n==0 → 0 soft; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (n == 0) r = 0;
    else if (a >= 0 && n > 0) r = (a + n - 1) / n;
    else if (a <= 0 && n < 0){
      long aa = -a, nn = -n;
      r = (aa + nn - 1) / nn;
    } else r = a / n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDIVFLOORTOCN")||kw(&L->cur,"SFLOORDIVTOCN")||kw(&L->cur,"STACKDIVFLOORTOCN")||
      kw(&L->cur,"SDIVFLOORTOCIMM")||kw(&L->cur,"FLOORDIVTOCN")||kw(&L->cur,"SCELLDIVFLOORN")||
      kw(&L->cur,"FLOORTOCN")){
    /* i + n → cells[i] = floor(cells[i]/n); n==0 → 0 soft; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (n == 0) r = 0;
    else {
      long q = a / n, rem = a % n;
      if (rem != 0 && ((a < 0) != (n < 0))) q--;
      r = q;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 imm nibble field TOC: SNIBTOCN · SSETNIBTOCN · SCLRNIBTOCN
   * (imm dual of SNIBN/SSETNIBN/SCLRNIBN into cell; complete field ladder 4/8/16) */
  if (kw(&L->cur,"SNIBTOCN")||kw(&L->cur,"SGETNIBTOCN")||kw(&L->cur,"STACKNIBTOCN")||
      kw(&L->cur,"SNIBTOCIMM")||kw(&L->cur,"NIBTOCN")||kw(&L->cur,"SCELLNIBN")||
      kw(&L->cur,"GETNIBTOCN")||kw(&L->cur,"SNIBBLETOCN")){
    /* i + n → cells[i] = LE nibble n of cells[i]; n clamped 0..15; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 4)) & 0xFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETNIBTOCN")||kw(&L->cur,"SSETNIBTOCIMM")||kw(&L->cur,"STACKSETNIBTOCN")||
      kw(&L->cur,"SETNIBTOCN")||kw(&L->cur,"SCELLSETNIBN")||kw(&L->cur,"PUTNIBTOCN")||
      kw(&L->cur,"SSETNIBATN")){
    /* i + field + n → deposit low 4 bits of field into LE nibble n of cells[i] */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long shift = (unsigned long)(n * 4);
    long r = (long)((base & ~(0xFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRNIBTOCN")||kw(&L->cur,"SCLRNIBTOCIMM")||kw(&L->cur,"STACKCLRNIBTOCN")||
      kw(&L->cur,"CLRNIBTOCN")||kw(&L->cur,"SCELLCLRNIBN")||kw(&L->cur,"ZAPNIBTOCN")||
      kw(&L->cur,"SCLRNIBATN")){
    /* i + n → clear LE nibble n of cells[i]; n clamped 0..15; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 4);
    long r = (long)(base & ~(0xFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack dual nibble field TOC: SNIBTOC · SSETNIBTOC · SCLRNIBTOC
   * (stack dual of SNIBTOCN/SSETNIBTOCN/SCLRNIBTOCN; foundation completes 4/8/16/32 stack ladder) */
  if (kw(&L->cur,"SNIBTOC")||kw(&L->cur,"SGETNIBTOC")||kw(&L->cur,"STACKNIBTOC")||
      kw(&L->cur,"NIBTOC")||kw(&L->cur,"SCELLNIBS")||kw(&L->cur,"GETNIBTOC")||
      kw(&L->cur,"SNIBBLETOC")||kw(&L->cur,"SNIBAT")){
    /* i n → cells[i] = LE nibble n of cells[i]; n clamped 0..15; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 4)) & 0xFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETNIBTOC")||kw(&L->cur,"STACKSETNIBTOC")||kw(&L->cur,"SETNIBTOC")||
      kw(&L->cur,"SCELLSETNIBS")||kw(&L->cur,"PUTNIBTOC")||kw(&L->cur,"SSETNIBAT")||
      kw(&L->cur,"SDEPOSITNIB")){
    /* i field n → deposit low 4 bits of field into LE nibble n of cells[i] */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long shift = (unsigned long)(n * 4);
    long r = (long)((base & ~(0xFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRNIBTOC")||kw(&L->cur,"STACKCLRNIBTOC")||kw(&L->cur,"CLRNIBTOC")||
      kw(&L->cur,"SCELLCLRNIBS")||kw(&L->cur,"ZAPNIBTOC")||kw(&L->cur,"SCLRNIBAT")||
      kw(&L->cur,"SZAPNIB")){
    /* i n → clear LE nibble n of cells[i]; n clamped 0..15; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 4);
    long r = (long)(base & ~(0xFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
