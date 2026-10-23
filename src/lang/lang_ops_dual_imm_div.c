/* CubalC lang — lang_ops_dual_imm_div.c
 * DUAL imm div: DDIVN·DMODN·DDIVCEILN·DDIVFLOORN·DSUBFROMN·DDIVFROMN·DMODFROMN·DUDIVFROMN·DUMODFROMN (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_div.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_div(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-7 dual-stack immediate div/mod: DDIVN · DMODN (dual of SDIVN/SMODN; complete imm ALU) */
  if (kw(&L->cur,"DDIVN")||kw(&L->cur,"2DIVN")||kw(&L->cur,"S2DIVN")||
      kw(&L->cur,"STACK2DIVN")||kw(&L->cur,"PAIRDIVN")||kw(&L->cur,"DDIVIMM")||
      kw(&L->cur,"2DIVIMM")||kw(&L->cur,"PAIRDIVIMM")||kw(&L->cur,"DQUOTN")){
    /* a b + n → (a/n) (b/n); n==0 → 0,0 soft */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (n == 0) ? 0 : (a / n);
    long y = (n == 0) ? 0 : (b / n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMODN")||kw(&L->cur,"2MODN")||kw(&L->cur,"S2MODN")||
      kw(&L->cur,"STACK2MODN")||kw(&L->cur,"PAIRMODN")||kw(&L->cur,"DMODIMM")||
      kw(&L->cur,"2MODIMM")||kw(&L->cur,"PAIRMODIMM")||kw(&L->cur,"DREMN")){
    /* a b + n → (a%n) (b%n); n==0 → 0,0 soft */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (n == 0) ? 0 : (a % n);
    long y = (n == 0) ? 0 : (b % n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm ceil/floor div: DDIVCEILN · DDIVFLOORN (dual of SDIVCEILN plane) */
  if (kw(&L->cur,"DDIVCEILN")||kw(&L->cur,"2DIVCEILN")||kw(&L->cur,"S2DIVCEILN")||
      kw(&L->cur,"STACK2DIVCEILN")||kw(&L->cur,"PAIRDIVCEILN")||kw(&L->cur,"DCEILDIVN")||
      kw(&L->cur,"2CEILDIVN")||kw(&L->cur,"PAIRCEILDIVN")||kw(&L->cur,"DDIVCEILIMM")){
    /* a b + n → ceil(a/n) ceil(b/n); n==0 → 0,0 soft */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (n != 0){
      if (a >= 0 && n > 0) x = (a + n - 1) / n;
      else if (a <= 0 && n < 0){ long aa = -a, nn = -n; x = (aa + nn - 1) / nn; }
      else x = a / n;
      if (b >= 0 && n > 0) y = (b + n - 1) / n;
      else if (b <= 0 && n < 0){ long bb = -b, nn = -n; y = (bb + nn - 1) / nn; }
      else y = b / n;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DDIVFLOORN")||kw(&L->cur,"2DIVFLOORN")||kw(&L->cur,"S2DIVFLOORN")||
      kw(&L->cur,"STACK2DIVFLOORN")||kw(&L->cur,"PAIRDIVFLOORN")||kw(&L->cur,"DFLOORDIVN")||
      kw(&L->cur,"2FLOORDIVN")||kw(&L->cur,"PAIRFLOORDIVN")||kw(&L->cur,"DDIVFLOORIMM")){
    /* a b + n → floor(a/n) floor(b/n); n==0 → 0,0 soft */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (n != 0){
      long qx = a / n, rx = a % n;
      if (rx != 0 && ((a < 0) != (n < 0))) qx--;
      x = qx;
      long qy = b / n, ry = b % n;
      if (ry != 0 && ((b < 0) != (n < 0))) qy--;
      y = qy;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack imm reverse ALU: DSUBFROMN · DDIVFROMN · DMODFROMN (dual of SSUBFROMN plane) */
  if (kw(&L->cur,"DSUBFROMN")||kw(&L->cur,"S2SUBFROMN")||kw(&L->cur,"STACK2SUBFROMN")||
      kw(&L->cur,"PAIRSUBFROMN")||kw(&L->cur,"DRSUBN")||kw(&L->cur,"PAIRRSUBN")||
      kw(&L->cur,"DNSUBN")||kw(&L->cur,"DSUBFROMIMM")){
    /* a b + n → (n-a) (n-b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = n - a;
    long y = n - b;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DDIVFROMN")||kw(&L->cur,"S2DIVFROMN")||kw(&L->cur,"STACK2DIVFROMN")||
      kw(&L->cur,"PAIRDIVFROMN")||kw(&L->cur,"DRDIVN")||kw(&L->cur,"PAIRRDIVN")||
      kw(&L->cur,"DDIVFROMIMM")){
    /* a b + n → (a==0?0:n/a) (b==0?0:n/b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a == 0) ? 0 : (n / a);
    long y = (b == 0) ? 0 : (n / b);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMODFROMN")||kw(&L->cur,"S2MODFROMN")||kw(&L->cur,"STACK2MODFROMN")||
      kw(&L->cur,"PAIRMODFROMN")||kw(&L->cur,"DRMODN")||kw(&L->cur,"PAIRRMODN")||
      kw(&L->cur,"DREMFROMN")||kw(&L->cur,"PAIRREMFROMN")||kw(&L->cur,"DMODFROMIMM")){
    /* a b + n → (a==0?0:n%a) (b==0?0:n%b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a == 0) ? 0 : (n % a);
    long y = (b == 0) ? 0 : (n % b);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 dual-stack imm reverse unsigned: DUDIVFROMN · DUMODFROMN (dual of SUDIVFROMN plane) */
  if (kw(&L->cur,"DUDIVFROMN")||kw(&L->cur,"S2UDIVFROMN")||kw(&L->cur,"STACK2UDIVFROMN")||
      kw(&L->cur,"PAIRUDIVFROMN")||kw(&L->cur,"DRUDIVN")||kw(&L->cur,"PAIRRUDIVN")||
      kw(&L->cur,"DUDIVFROMIMM")||kw(&L->cur,"NUDIVFROMN")){
    /* a b + n → unsigned (a==0?0:n/a) (b==0?0:n/b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (a != 0) x = (long)((unsigned long)n / (unsigned long)a);
    if (b != 0) y = (long)((unsigned long)n / (unsigned long)b);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DUMODFROMN")||kw(&L->cur,"S2UMODFROMN")||kw(&L->cur,"STACK2UMODFROMN")||
      kw(&L->cur,"PAIRUMODFROMN")||kw(&L->cur,"DRUMODN")||kw(&L->cur,"PAIRRUMODN")||
      kw(&L->cur,"DUMODFROMIMM")||kw(&L->cur,"DUREMFROMN")||kw(&L->cur,"PAIRUREMFROMN")){
    /* a b + n → unsigned (a==0?0:n%a) (b==0?0:n%b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (a != 0) x = (long)((unsigned long)n % (unsigned long)a);
    if (b != 0) y = (long)((unsigned long)n % (unsigned long)b);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
