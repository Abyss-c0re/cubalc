/* CubalC lang — lang_ops_math_stack_sci.c
 * MATH stack science duals: SAVG SPCT SHYP SHAM SDIST · SMASK SISDIV
 * Peel from math residual (B_math_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_math_stack_sci.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cubalc_lang_ops_math_try_stack_sci(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-8 stack science/math duals: SAVG SPCT SHYP SHAM SDIST */
  if (kw(&L->cur,"SAVG")||kw(&L->cur,"STACKAVG")||
      kw(&L->cur,"SPCT")||kw(&L->cur,"STACKPCT")||kw(&L->cur,"SPERCENT")||
      kw(&L->cur,"SHYP")||kw(&L->cur,"STACKHYP")||kw(&L->cur,"SHYPOT")||
      kw(&L->cur,"SHAM")||kw(&L->cur,"SHAMMING")||kw(&L->cur,"STACKHAMMING")||
      kw(&L->cur,"SDIST")||kw(&L->cur,"SABSDIFF")||kw(&L->cur,"STACKDIST")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (strcmp(op,"SAVG")==0 || strcmp(op,"STACKAVG")==0)
      r = (a + b) / 2;
    else if (strcmp(op,"SPCT")==0 || strcmp(op,"STACKPCT")==0 || strcmp(op,"SPERCENT")==0)
      r = b ? (a * 100 / b) : 0;
    else if (strcmp(op,"SHYP")==0 || strcmp(op,"STACKHYP")==0 || strcmp(op,"SHYPOT")==0){
      long s = a * a + b * b;
      if (s < 0) r = 0;
      else {
        long t = 0;
        while ((t + 1) * (t + 1) <= s) t++;
        r = t;
      }
    } else if (strcmp(op,"SHAM")==0 || strcmp(op,"SHAMMING")==0 ||
               strcmp(op,"STACKHAMMING")==0){
      unsigned long u = (unsigned long)(a ^ b);
      int n = 0;
      while (u){ n += (int)(u & 1u); u >>= 1; }
      r = (long)n;
    } else {
      /* SDIST / SABSDIFF / STACKDIST — |a-b| */
      long d = a - b;
      r = d < 0 ? -d : d;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 foundation stack: SMASK SISDIV */
  if (kw(&L->cur,"SMASK")||kw(&L->cur,"SBITMASK")||kw(&L->cur,"STACKMASK")){
    /* n → low n bits set */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (a <= 0) r = 0;
    else if (a >= 63) r = (long)~0ul;
    else r = (long)((1ul << (unsigned)a) - 1ul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SISDIV")||kw(&L->cur,"SDIVISIBLE")||kw(&L->cur,"STACKISDIV")){
    /* a b → 1 if b!=0 and a multiple of b */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = (b != 0 && (a % b) == 0) ? 1 : 0;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
