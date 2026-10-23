/* CubalC lang — lang_ops_dual_pair.c
 * DUAL pair ALU: DADD·DSUB·DMUL·DDIV·DMOD·DMIN·DMAX (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_pair.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_pair(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* plane ops_dual: L12580-20288 */
  /* digit-7 dual-stack pair ALU: DADD · DSUB · DMUL (vector pairs a b + c d) */
  if (kw(&L->cur,"DADD")||kw(&L->cur,"2ADD")||kw(&L->cur,"S2ADD")||
      kw(&L->cur,"STACK2ADD")||kw(&L->cur,"PAIRADD")){
    /* a b c d → (a+c) (b+d) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a + c;
    long y = b + d;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSUB")||kw(&L->cur,"2SUB")||kw(&L->cur,"S2SUB")||
      kw(&L->cur,"STACK2SUB")||kw(&L->cur,"PAIRSUB")){
    /* a b c d → (a-c) (b-d) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a - c;
    long y = b - d;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMUL")||kw(&L->cur,"2MUL")||kw(&L->cur,"S2MUL")||
      kw(&L->cur,"STACK2MUL")||kw(&L->cur,"PAIRMUL")){
    /* a b c d → (a*c) (b*d) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a * c;
    long y = b * d;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack pair ALU ext: DDIV · DMOD · DMIN · DMAX */
  if (kw(&L->cur,"DDIV")||kw(&L->cur,"2DIV")||kw(&L->cur,"S2DIV")||
      kw(&L->cur,"STACK2DIV")||kw(&L->cur,"PAIRDIV")){
    /* a b c d → (a/c) (b/d); divisor 0 → 0 */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = c ? (a / c) : 0;
    long y = d ? (b / d) : 0;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack div modes: DDIVCEIL · DDIVFLOOR (pair ceildiv/floordiv) */
  if (kw(&L->cur,"DDIVCEIL")||kw(&L->cur,"2DIVCEIL")||kw(&L->cur,"S2DIVCEIL")||
      kw(&L->cur,"STACK2DIVCEIL")||kw(&L->cur,"PAIRDIVCEIL")||kw(&L->cur,"2CEILDIV")||
      kw(&L->cur,"DCEILDIV")||
      kw(&L->cur,"DDIVFLOOR")||kw(&L->cur,"2DIVFLOOR")||kw(&L->cur,"S2DIVFLOOR")||
      kw(&L->cur,"STACK2DIVFLOOR")||kw(&L->cur,"PAIRDIVFLOOR")||kw(&L->cur,"2FLOORDIV")||
      kw(&L->cur,"DFLOORDIV")){
    /* a b c d → f(a,c) f(b,d); divisor 0 → 0
     * CEIL: toward +∞ for same-sign positive path; mixed/neg uses trunc toward zero for ceil when signs differ (match SDIVCEIL)
     * FLOOR: toward −∞ (adjust when rem and signs differ) */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_ceil = (strcmp(op,"DDIVCEIL")==0 || strcmp(op,"2DIVCEIL")==0 ||
                   strcmp(op,"S2DIVCEIL")==0 || strcmp(op,"STACK2DIVCEIL")==0 ||
                   strcmp(op,"PAIRDIVCEIL")==0 || strcmp(op,"2CEILDIV")==0 ||
                   strcmp(op,"DCEILDIV")==0);
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = 0, y = 0;
    if (is_ceil){
      if (c == 0) x = 0;
      else if (a >= 0 && c > 0) x = (a + c - 1) / c;
      else if (a <= 0 && c < 0){
        long aa = -a, cc = -c;
        x = (aa + cc - 1) / cc;
      } else x = a / c;
      if (d == 0) y = 0;
      else if (b >= 0 && d > 0) y = (b + d - 1) / d;
      else if (b <= 0 && d < 0){
        long bb = -b, dd = -d;
        y = (bb + dd - 1) / dd;
      } else y = b / d;
    } else {
      if (c == 0) x = 0;
      else {
        long q = a / c, rem = a % c;
        if (rem != 0 && ((a < 0) != (c < 0))) q--;
        x = q;
      }
      if (d == 0) y = 0;
      else {
        long q = b / d, rem = b % d;
        if (rem != 0 && ((b < 0) != (d < 0))) q--;
        y = q;
      }
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMOD")||kw(&L->cur,"2MOD")||kw(&L->cur,"S2MOD")||
      kw(&L->cur,"STACK2MOD")||kw(&L->cur,"PAIRMOD")||kw(&L->cur,"DREM")){
    /* a b c d → (a%c) (b%d); divisor 0 → 0 */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = c ? (a % c) : 0;
    long y = d ? (b % d) : 0;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMIN")||kw(&L->cur,"2MIN")||kw(&L->cur,"S2MIN")||
      kw(&L->cur,"STACK2MIN")||kw(&L->cur,"PAIRMIN")){
    /* a b c d → min(a,c) min(b,d) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a < c ? a : c;
    long y = b < d ? b : d;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMAX")||kw(&L->cur,"2MAX")||kw(&L->cur,"S2MAX")||
      kw(&L->cur,"STACK2MAX")||kw(&L->cur,"PAIRMAX")){
    /* a b c d → max(a,c) max(b,d) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a > c ? a : c;
    long y = b > d ? b : d;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
