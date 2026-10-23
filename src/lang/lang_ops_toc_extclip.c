/* CubalC lang — lang_ops_toc_extclip.c
 * TOC extend/clip plane: SSEXT/SZEXT · SCLIP/SCLIPS (stack + imm duals)
 * Peel from toc (B_toc_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_toc_extclip.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_toc_try_extclip(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-8 stack dual sign/zero extend TOC: SSEXTTOC · SZEXTTOC · SSEXT8TOC
   * (stack dual of SSEXT/SZEXT/SSEXT8 into cell; width from stack after field ladder) */
  if (kw(&L->cur,"SSEXTTOC")||kw(&L->cur,"SEXTTOC")||kw(&L->cur,"SSIGNEXTTOC")||
      kw(&L->cur,"STACKSEXTTOC")||kw(&L->cur,"SCELLSEXT")||kw(&L->cur,"SIGNEXTTOC")||
      kw(&L->cur,"SSEXTAT")){
    /* i w → cells[i] = sign-extend low w bits of cells[i]; w 0..63; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long w = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (w <= 0) r = 0;
    else if (w >= 63) r = a;
    else {
      unsigned long mask = (1ul << (unsigned)w) - 1ul;
      unsigned long v = (unsigned long)a & mask;
      unsigned long sign = 1ul << (unsigned)(w - 1);
      if (v & sign) v |= ~mask;
      r = (long)v;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXTTOC")||kw(&L->cur,"ZEXTTOC")||kw(&L->cur,"SZEROEXTTOC")||
      kw(&L->cur,"STACKZEXTTOC")||kw(&L->cur,"SCELLZEXT")||kw(&L->cur,"ZEROEXTTOC")||
      kw(&L->cur,"SZEXTAT")){
    /* i w → cells[i] = zero-extend low w bits of cells[i]; w 0..63; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long w = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (w <= 0) r = 0;
    else if (w >= 63) r = a;
    else {
      unsigned long mask = (1ul << (unsigned)w) - 1ul;
      r = (long)((unsigned long)a & mask);
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT8TOC")||kw(&L->cur,"SEXT8TOC")||kw(&L->cur,"SSEXTBTOC")||
      kw(&L->cur,"STACKSEXT8TOC")||kw(&L->cur,"SCELLSEXT8")||kw(&L->cur,"SIGNEXT8TOC")||
      kw(&L->cur,"SSEXT8AT")){
    /* i → cells[i] = sign-extend low 8 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = a & 0xFFL;
    if (r & 0x80L) r |= ~0xFFL;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 imm dual sign/zero extend TOC: SSEXTTOCN · SZEXTTOCN · SSEXT16TOCN
   * (imm dual of SSEXTTOC/SZEXTTOC; fixed 16-bit path after SSEXT8TOC plane) */
  if (kw(&L->cur,"SSEXTTOCN")||kw(&L->cur,"SEXTTOCN")||kw(&L->cur,"SSIGNEXTTOCN")||
      kw(&L->cur,"SSEXTTOCIMM")||kw(&L->cur,"STACKSEXTTOCN")||kw(&L->cur,"SCELLSEXTN")||
      kw(&L->cur,"SIGNEXTTOCN")){
    /* i + w → cells[i] = sign-extend low w bits of cells[i]; w 0..63; leave result */
    lex_next(L);
    long w = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (w <= 0) r = 0;
    else if (w >= 63) r = a;
    else {
      unsigned long mask = (1ul << (unsigned)w) - 1ul;
      unsigned long v = (unsigned long)a & mask;
      unsigned long sign = 1ul << (unsigned)(w - 1);
      if (v & sign) v |= ~mask;
      r = (long)v;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXTTOCN")||kw(&L->cur,"ZEXTTOCN")||kw(&L->cur,"SZEROEXTTOCN")||
      kw(&L->cur,"SZEXTTOCIMM")||kw(&L->cur,"STACKZEXTTOCN")||kw(&L->cur,"SCELLZEXTN")||
      kw(&L->cur,"ZEROEXTTOCN")){
    /* i + w → cells[i] = zero-extend low w bits of cells[i]; w 0..63; leave result */
    lex_next(L);
    long w = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (w <= 0) r = 0;
    else if (w >= 63) r = a;
    else {
      unsigned long mask = (1ul << (unsigned)w) - 1ul;
      r = (long)((unsigned long)a & mask);
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT16TOCN")||kw(&L->cur,"SEXT16TOCN")||kw(&L->cur,"SSEXTWTOCN")||
      kw(&L->cur,"STACKSEXT16TOCN")||kw(&L->cur,"SCELLSEXT16N")||kw(&L->cur,"SIGNEXT16TOCN")||
      kw(&L->cur,"SSEXT16TOCIMM")){
    /* i → cells[i] = sign-extend low 16 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = a & 0xFFFFL;
    if (r & 0x8000L) r |= ~0xFFFFL;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack dual fixed-width extend TOC: SSEXT16TOC · SSEXT32TOC · SZEXT8TOC
   * (complete 8/16/32 fixed extend plane after SSEXT8TOC/SSEXT16TOCN) */
  if (kw(&L->cur,"SSEXT16TOC")||kw(&L->cur,"SEXT16TOC")||kw(&L->cur,"SSEXTWTOC")||
      kw(&L->cur,"STACKSEXT16TOC")||kw(&L->cur,"SCELLSEXT16")||kw(&L->cur,"SIGNEXT16TOC")||
      kw(&L->cur,"SSEXT16AT")){
    /* i → cells[i] = sign-extend low 16 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = a & 0xFFFFL;
    if (r & 0x8000L) r |= ~0xFFFFL;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT32TOC")||kw(&L->cur,"SEXT32TOC")||kw(&L->cur,"SSEXTLTOC")||
      kw(&L->cur,"STACKSEXT32TOC")||kw(&L->cur,"SCELLSEXT32")||kw(&L->cur,"SIGNEXT32TOC")||
      kw(&L->cur,"SSEXT32AT")||kw(&L->cur,"SSEXTDTOC")){
    /* i → cells[i] = sign-extend low 32 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = (long)(int)(unsigned int)a;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT8TOC")||kw(&L->cur,"ZEXT8TOC")||kw(&L->cur,"SZEXTBTOC")||
      kw(&L->cur,"STACKZEXT8TOC")||kw(&L->cur,"SCELLZEXT8")||kw(&L->cur,"ZEROEXT8TOC")||
      kw(&L->cur,"SZEXT8AT")){
    /* i → cells[i] = zero-extend low 8 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack dual fixed zext + nibble sext TOC: SZEXT16TOC · SZEXT32TOC · SSEXT4TOC
   * (complete zext 8/16/32 plane after SZEXT8TOC; nibble sext closes 4-bit extend) */
  if (kw(&L->cur,"SZEXT16TOC")||kw(&L->cur,"ZEXT16TOC")||kw(&L->cur,"SZEXTWTOC")||
      kw(&L->cur,"STACKZEXT16TOC")||kw(&L->cur,"SCELLZEXT16")||kw(&L->cur,"ZEROEXT16TOC")||
      kw(&L->cur,"SZEXT16AT")){
    /* i → cells[i] = zero-extend low 16 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT32TOC")||kw(&L->cur,"ZEXT32TOC")||kw(&L->cur,"SZEXTLTOC")||
      kw(&L->cur,"STACKZEXT32TOC")||kw(&L->cur,"SCELLZEXT32")||kw(&L->cur,"ZEROEXT32TOC")||
      kw(&L->cur,"SZEXT32AT")||kw(&L->cur,"SZEXTDTOC")){
    /* i → cells[i] = zero-extend low 32 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFFFFFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT4TOC")||kw(&L->cur,"SEXT4TOC")||kw(&L->cur,"SSEXTNTOC")||
      kw(&L->cur,"STACKSEXT4TOC")||kw(&L->cur,"SCELLSEXT4")||kw(&L->cur,"SIGNEXT4TOC")||
      kw(&L->cur,"SSEXT4AT")||kw(&L->cur,"SNIBSEXTTOC")){
    /* i → cells[i] = sign-extend low 4 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = a & 0xFL;
    if (r & 0x8L) r |= ~0xFL;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 imm dual fixed nibble/byte extend TOC: SSEXT8TOCN · SSEXT4TOCN · SZEXT4TOCN
   * (imm dual of SSEXT8TOC/SSEXT4TOC; nibble zext closes 4-bit zext plane) */
  if (kw(&L->cur,"SSEXT8TOCN")||kw(&L->cur,"SEXT8TOCN")||kw(&L->cur,"SSEXTBTOCN")||
      kw(&L->cur,"STACKSEXT8TOCN")||kw(&L->cur,"SCELLSEXT8N")||kw(&L->cur,"SIGNEXT8TOCN")||
      kw(&L->cur,"SSEXT8TOCIMM")){
    /* i → cells[i] = sign-extend low 8 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = a & 0xFFL;
    if (r & 0x80L) r |= ~0xFFL;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT4TOCN")||kw(&L->cur,"SEXT4TOCN")||kw(&L->cur,"SSEXTNTOCN")||
      kw(&L->cur,"STACKSEXT4TOCN")||kw(&L->cur,"SCELLSEXT4N")||kw(&L->cur,"SIGNEXT4TOCN")||
      kw(&L->cur,"SSEXT4TOCIMM")||kw(&L->cur,"SNIBSEXTTOCN")){
    /* i → cells[i] = sign-extend low 4 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = a & 0xFL;
    if (r & 0x8L) r |= ~0xFL;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT4TOCN")||kw(&L->cur,"ZEXT4TOCN")||kw(&L->cur,"SZEXTNTOCN")||
      kw(&L->cur,"STACKZEXT4TOCN")||kw(&L->cur,"SCELLZEXT4N")||kw(&L->cur,"ZEROEXT4TOCN")||
      kw(&L->cur,"SZEXT4TOCIMM")||kw(&L->cur,"SNIBZEXTTOCN")){
    /* i → cells[i] = zero-extend low 4 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack/imm zext dual ladder: SZEXT4TOC · SZEXT8TOCN · SZEXT16TOCN
   * (stack dual of SZEXT4TOCN; imm dual of SZEXT8TOC/SZEXT16TOC after nibble plane) */
  if (kw(&L->cur,"SZEXT4TOC")||kw(&L->cur,"ZEXT4TOC")||kw(&L->cur,"SZEXTNTOC")||
      kw(&L->cur,"STACKZEXT4TOC")||kw(&L->cur,"SCELLZEXT4")||kw(&L->cur,"ZEROEXT4TOC")||
      kw(&L->cur,"SZEXT4AT")||kw(&L->cur,"SNIBZEXTTOC")){
    /* i → cells[i] = zero-extend low 4 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT8TOCN")||kw(&L->cur,"ZEXT8TOCN")||kw(&L->cur,"SZEXTBTOCN")||
      kw(&L->cur,"STACKZEXT8TOCN")||kw(&L->cur,"SCELLZEXT8N")||kw(&L->cur,"ZEROEXT8TOCN")||
      kw(&L->cur,"SZEXT8TOCIMM")){
    /* i → cells[i] = zero-extend low 8 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT16TOCN")||kw(&L->cur,"ZEXT16TOCN")||kw(&L->cur,"SZEXTWTOCN")||
      kw(&L->cur,"STACKZEXT16TOCN")||kw(&L->cur,"SCELLZEXT16N")||kw(&L->cur,"ZEROEXT16TOCN")||
      kw(&L->cur,"SZEXT16TOCIMM")){
    /* i → cells[i] = zero-extend low 16 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 imm dual 32-bit extend + clip8 TOC: SZEXT32TOCN · SSEXT32TOCN · SCLIP8TOC
   * (complete zext/sext imm 4/8/16/32 ladder; clip dual of CLIP8 into cell) */
  if (kw(&L->cur,"SZEXT32TOCN")||kw(&L->cur,"ZEXT32TOCN")||kw(&L->cur,"SZEXTLTOCN")||
      kw(&L->cur,"STACKZEXT32TOCN")||kw(&L->cur,"SCELLZEXT32N")||kw(&L->cur,"ZEROEXT32TOCN")||
      kw(&L->cur,"SZEXT32TOCIMM")||kw(&L->cur,"SZEXTDTOCN")){
    /* i → cells[i] = zero-extend low 32 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFFFFFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT32TOCN")||kw(&L->cur,"SEXT32TOCN")||kw(&L->cur,"SSEXTLTOCN")||
      kw(&L->cur,"STACKSEXT32TOCN")||kw(&L->cur,"SCELLSEXT32N")||kw(&L->cur,"SIGNEXT32TOCN")||
      kw(&L->cur,"SSEXT32TOCIMM")||kw(&L->cur,"SSEXTDTOCN")){
    /* i → cells[i] = sign-extend low 32 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = (long)(int)(unsigned int)a;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIP8TOC")||kw(&L->cur,"CLIP8TOC")||kw(&L->cur,"SCLIPBTOC")||
      kw(&L->cur,"STACKCLIP8TOC")||kw(&L->cur,"SCELLCLIP8")||kw(&L->cur,"UCLIP8TOC")||
      kw(&L->cur,"SCLIP8AT")){
    /* i → cells[i] = clamp cells[i] to unsigned 8-bit [0,255]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = a;
    if (r < 0) r = 0;
    if (r > 255) r = 255;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack dual fixed clip TOC: SCLIP4TOC · SCLIP16TOC · SCLIP32TOC
   * (complete unsigned clip 4/8/16/32 plane after SCLIP8TOC; dual of CLIP4/16/32) */
  if (kw(&L->cur,"SCLIP4TOC")||kw(&L->cur,"CLIP4TOC")||kw(&L->cur,"SCLIPNTOC")||
      kw(&L->cur,"STACKCLIP4TOC")||kw(&L->cur,"SCELLCLIP4")||kw(&L->cur,"UCLIP4TOC")||
      kw(&L->cur,"SCLIP4AT")){
    /* i → cells[i] = clamp cells[i] to unsigned nibble [0,15]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < 0) r = 0;
    if (r > 15) r = 15;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIP16TOC")||kw(&L->cur,"CLIP16TOC")||kw(&L->cur,"SCLIPWTOC")||
      kw(&L->cur,"STACKCLIP16TOC")||kw(&L->cur,"SCELLCLIP16")||kw(&L->cur,"UCLIP16TOC")||
      kw(&L->cur,"SCLIP16AT")){
    /* i → cells[i] = clamp cells[i] to unsigned 16-bit [0,65535]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < 0) r = 0;
    if (r > 65535L) r = 65535L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIP32TOC")||kw(&L->cur,"CLIP32TOC")||kw(&L->cur,"SCLIPLTOC")||
      kw(&L->cur,"STACKCLIP32TOC")||kw(&L->cur,"SCELLCLIP32")||kw(&L->cur,"UCLIP32TOC")||
      kw(&L->cur,"SCLIP32AT")||kw(&L->cur,"SCLIPDTOC")){
    /* i → cells[i] = clamp cells[i] to unsigned 32-bit [0,0xFFFFFFFF]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < 0) r = 0;
    if ((unsigned long)r > 0xFFFFFFFFul) r = (long)0xFFFFFFFFul;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 imm dual fixed clip TOC: SCLIP4TOCN · SCLIP8TOCN · SCLIP16TOCN
   * (imm dual of SCLIP4TOC/SCLIP8TOC/SCLIP16TOC; keep i on stack) */
  if (kw(&L->cur,"SCLIP4TOCN")||kw(&L->cur,"CLIP4TOCN")||kw(&L->cur,"SCLIPNTOCN")||
      kw(&L->cur,"STACKCLIP4TOCN")||kw(&L->cur,"SCELLCLIP4N")||kw(&L->cur,"UCLIP4TOCN")||
      kw(&L->cur,"SCLIP4TOCIMM")){
    /* i → cells[i] = clamp to [0,15]; leave result on stack (i replaced) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < 0) r = 0;
    if (r > 15) r = 15;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIP8TOCN")||kw(&L->cur,"CLIP8TOCN")||kw(&L->cur,"SCLIPBTOCN")||
      kw(&L->cur,"STACKCLIP8TOCN")||kw(&L->cur,"SCELLCLIP8N")||kw(&L->cur,"UCLIP8TOCN")||
      kw(&L->cur,"SCLIP8TOCIMM")){
    /* i → cells[i] = clamp to [0,255]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < 0) r = 0;
    if (r > 255) r = 255;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIP16TOCN")||kw(&L->cur,"CLIP16TOCN")||kw(&L->cur,"SCLIPWTOCN")||
      kw(&L->cur,"STACKCLIP16TOCN")||kw(&L->cur,"SCELLCLIP16N")||kw(&L->cur,"UCLIP16TOCN")||
      kw(&L->cur,"SCLIP16TOCIMM")){
    /* i → cells[i] = clamp to [0,65535]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < 0) r = 0;
    if (r > 65535L) r = 65535L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 imm clip32 + signed clip TOC: SCLIP32TOCN · SCLIPS8TOC · SCLIPS16TOC
   * (complete uclip imm 4/8/16/32; signed clamp duals of CLIP into cell) */
  if (kw(&L->cur,"SCLIP32TOCN")||kw(&L->cur,"CLIP32TOCN")||kw(&L->cur,"SCLIPLTOCN")||
      kw(&L->cur,"STACKCLIP32TOCN")||kw(&L->cur,"SCELLCLIP32N")||kw(&L->cur,"UCLIP32TOCN")||
      kw(&L->cur,"SCLIP32TOCIMM")||kw(&L->cur,"SCLIPDTOCN")){
    /* i → cells[i] = clamp to [0,0xFFFFFFFF]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < 0) r = 0;
    if ((unsigned long)r > 0xFFFFFFFFul) r = (long)0xFFFFFFFFul;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS8TOC")||kw(&L->cur,"CLIPS8TOC")||kw(&L->cur,"SSCLIP8TOC")||
      kw(&L->cur,"STACKSCLIPS8TOC")||kw(&L->cur,"SCELLSCLIP8")||kw(&L->cur,"SCLIPBSTO")||
      kw(&L->cur,"SCLIPS8AT")){
    /* i → cells[i] = clamp to signed 8-bit [-128,127]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < -128L) r = -128L;
    if (r > 127L) r = 127L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS16TOC")||kw(&L->cur,"CLIPS16TOC")||kw(&L->cur,"SSCLIP16TOC")||
      kw(&L->cur,"STACKSCLIPS16TOC")||kw(&L->cur,"SCELLSCLIP16")||kw(&L->cur,"SCLIPWSTO")||
      kw(&L->cur,"SCLIPS16AT")){
    /* i → cells[i] = clamp to signed 16-bit [-32768,32767]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < -32768L) r = -32768L;
    if (r > 32767L) r = 32767L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 signed clip 4/32 + imm signed8 TOC: SCLIPS4TOC · SCLIPS32TOC · SCLIPS8TOCN
   * (complete signed clip 4/8/16/32 plane; imm dual of SCLIPS8TOC) */
  if (kw(&L->cur,"SCLIPS4TOC")||kw(&L->cur,"CLIPS4TOC")||kw(&L->cur,"SSCLIP4TOC")||
      kw(&L->cur,"STACKSCLIPS4TOC")||kw(&L->cur,"SCELLSCLIP4")||kw(&L->cur,"SCLIPNSTO")||
      kw(&L->cur,"SCLIPS4AT")){
    /* i → cells[i] = clamp to signed 4-bit [-8,7]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < -8L) r = -8L;
    if (r > 7L) r = 7L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS32TOC")||kw(&L->cur,"CLIPS32TOC")||kw(&L->cur,"SSCLIP32TOC")||
      kw(&L->cur,"STACKSCLIPS32TOC")||kw(&L->cur,"SCELLSCLIP32")||kw(&L->cur,"SCLIPLSTO")||
      kw(&L->cur,"SCLIPS32AT")||kw(&L->cur,"SCLIPSDTOC")){
    /* i → cells[i] = clamp to signed 32-bit [INT32_MIN,INT32_MAX]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < (-2147483647L - 1)) r = (-2147483647L - 1);
    if (r > 2147483647L) r = 2147483647L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS8TOCN")||kw(&L->cur,"CLIPS8TOCN")||kw(&L->cur,"SSCLIP8TOCN")||
      kw(&L->cur,"STACKSCLIPS8TOCN")||kw(&L->cur,"SCELLSCLIP8N")||kw(&L->cur,"SCLIPBSTON")||
      kw(&L->cur,"SCLIPS8TOCIMM")){
    /* i → cells[i] = clamp to signed 8-bit [-128,127]; leave result (i replaced) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < -128L) r = -128L;
    if (r > 127L) r = 127L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 imm dual signed clip TOC: SCLIPS4TOCN · SCLIPS16TOCN · SCLIPS32TOCN
   * (imm dual of SCLIPS4/16/32TOC; complete signed clip imm plane with SCLIPS8TOCN) */
  if (kw(&L->cur,"SCLIPS4TOCN")||kw(&L->cur,"CLIPS4TOCN")||kw(&L->cur,"SSCLIP4TOCN")||
      kw(&L->cur,"STACKSCLIPS4TOCN")||kw(&L->cur,"SCELLSCLIP4N")||kw(&L->cur,"SCLIPNSTON")||
      kw(&L->cur,"SCLIPS4TOCIMM")){
    /* i → cells[i] = clamp to signed 4-bit [-8,7]; leave result (i replaced) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < -8L) r = -8L;
    if (r > 7L) r = 7L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS16TOCN")||kw(&L->cur,"CLIPS16TOCN")||kw(&L->cur,"SSCLIP16TOCN")||
      kw(&L->cur,"STACKSCLIPS16TOCN")||kw(&L->cur,"SCELLSCLIP16N")||kw(&L->cur,"SCLIPWSTON")||
      kw(&L->cur,"SCLIPS16TOCIMM")){
    /* i → cells[i] = clamp to signed 16-bit [-32768,32767]; leave result (i replaced) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < -32768L) r = -32768L;
    if (r > 32767L) r = 32767L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS32TOCN")||kw(&L->cur,"CLIPS32TOCN")||kw(&L->cur,"SSCLIP32TOCN")||
      kw(&L->cur,"STACKSCLIPS32TOCN")||kw(&L->cur,"SCELLSCLIP32N")||kw(&L->cur,"SCLIPLSTON")||
      kw(&L->cur,"SCLIPS32TOCIMM")||kw(&L->cur,"SCLIPSDTOCN")){
    /* i → cells[i] = clamp to signed 32-bit [INT32_MIN,INT32_MAX]; leave result (i replaced) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < (-2147483647L - 1)) r = (-2147483647L - 1);
    if (r > 2147483647L) r = 2147483647L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
