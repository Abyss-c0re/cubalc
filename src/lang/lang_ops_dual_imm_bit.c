/* CubalC lang — lang_ops_dual_imm_bit.c
 * DUAL imm bitwise: DANDI·DORI·DXORI·DNANDI…DXNORNFROMN (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_bit.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_bit(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-0 dual-stack immediate bitwise mask: DANDI · DORI · DXORI (dual of SANDI/SORI/SXORI) */
  if (kw(&L->cur,"DANDI")||kw(&L->cur,"2ANDI")||kw(&L->cur,"S2ANDI")||
      kw(&L->cur,"STACK2ANDI")||kw(&L->cur,"PAIRANDI")||kw(&L->cur,"DANDIMM")||
      kw(&L->cur,"2ANDIMM")||kw(&L->cur,"PAIRANDIMM")){
    /* a b + n → (a&n) (b&n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a & n;
    long y = b & n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DORI")||kw(&L->cur,"2ORI")||kw(&L->cur,"S2ORI")||
      kw(&L->cur,"STACK2ORI")||kw(&L->cur,"PAIRORI")||kw(&L->cur,"DORIMM")||
      kw(&L->cur,"2ORIMM")||kw(&L->cur,"PAIRORIMM")){
    /* a b + n → (a|n) (b|n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a | n;
    long y = b | n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXORI")||kw(&L->cur,"2XORI")||kw(&L->cur,"S2XORI")||
      kw(&L->cur,"STACK2XORI")||kw(&L->cur,"PAIRXORI")||kw(&L->cur,"DXORIMM")||
      kw(&L->cur,"2XORIMM")||kw(&L->cur,"PAIRXORIMM")){
    /* a b + n → (a^n) (b^n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a ^ n;
    long y = b ^ n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack immediate inverted bitwise: DNANDI · DNORI · DXNORI (dual of SNANDI/SNORI/SXNORI) */
  if (kw(&L->cur,"DNANDI")||kw(&L->cur,"2NANDI")||kw(&L->cur,"S2NANDI")||
      kw(&L->cur,"STACK2NANDI")||kw(&L->cur,"PAIRNANDI")||kw(&L->cur,"DNANDIMM")||
      kw(&L->cur,"2NANDIMM")||kw(&L->cur,"PAIRNANDIMM")){
    /* a b + n → ~(a&n) ~(b&n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(a & n);
    long y = ~(b & n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNORI")||kw(&L->cur,"2NORI")||kw(&L->cur,"S2NORI")||
      kw(&L->cur,"STACK2NORI")||kw(&L->cur,"PAIRNORI")||kw(&L->cur,"DNORIMM")||
      kw(&L->cur,"2NORIMM")||kw(&L->cur,"PAIRNORIMM")){
    /* a b + n → ~(a|n) ~(b|n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(a | n);
    long y = ~(b | n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNORI")||kw(&L->cur,"2XNORI")||kw(&L->cur,"S2XNORI")||
      kw(&L->cur,"STACK2XNORI")||kw(&L->cur,"PAIRXNORI")||kw(&L->cur,"DXNORIMM")||
      kw(&L->cur,"2XNORIMM")||kw(&L->cur,"PAIRXNORIMM")||kw(&L->cur,"DEQUIVI")||
      kw(&L->cur,"2EQUIVI")){
    /* a b + n → ~(a^n) ~(b^n)  (bitwise equivalence with constant) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(a ^ n);
    long y = ~(b ^ n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 dual-stack immediate ANDN plane: DANDNI · DORNI · DXORNI (dual of SANDNI/SORNI/SXORNI) */
  if (kw(&L->cur,"DANDNI")||kw(&L->cur,"2ANDNI")||kw(&L->cur,"S2ANDNI")||
      kw(&L->cur,"STACK2ANDNI")||kw(&L->cur,"PAIRANDNI")||kw(&L->cur,"DBICI")||
      kw(&L->cur,"DBICIMM")||kw(&L->cur,"DANDNOTI")||kw(&L->cur,"2ANDNOTI")||
      kw(&L->cur,"PAIRANDNOTI")){
    /* a b + n → (a&~n) (b&~n)  clear bits set in n on both tops */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a & ~n;
    long y = b & ~n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DORNI")||kw(&L->cur,"2ORNI")||kw(&L->cur,"S2ORNI")||
      kw(&L->cur,"STACK2ORNI")||kw(&L->cur,"PAIRORNI")||kw(&L->cur,"DORNOTI")||
      kw(&L->cur,"2ORNOTI")||kw(&L->cur,"PAIRORNOTI")){
    /* a b + n → (a|~n) (b|~n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a | ~n;
    long y = b | ~n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXORNI")||kw(&L->cur,"2XORNI")||kw(&L->cur,"S2XORNI")||
      kw(&L->cur,"STACK2XORNI")||kw(&L->cur,"PAIRXORNI")||kw(&L->cur,"DXORNOTI")||
      kw(&L->cur,"2XORNOTI")||kw(&L->cur,"PAIRXORNOTI")){
    /* a b + n → (a^~n) (b^~n)  (equiv DXNORI / ~(a^n) ~(b^n)) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a ^ ~n;
    long y = b ^ ~n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm inverted ANDN plane: DNANDNI · DNORNI · DXNORNI (dual of SNANDNI) */
  if (kw(&L->cur,"DNANDNI")||kw(&L->cur,"S2NANDNI")||kw(&L->cur,"STACK2NANDNI")||
      kw(&L->cur,"PAIRNANDNI")||kw(&L->cur,"DINVERTANDNI")||kw(&L->cur,"DNANDNOTI")||
      kw(&L->cur,"PAIRNANDNOTI")){
    /* a b + n → ~(a&~n) ~(b&~n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(a & ~n);
    long y = ~(b & ~n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNORNI")||kw(&L->cur,"S2NORNI")||kw(&L->cur,"STACK2NORNI")||
      kw(&L->cur,"PAIRNORNI")||kw(&L->cur,"DINVERTORNI")||kw(&L->cur,"DNORNOTI")||
      kw(&L->cur,"PAIRNORNOTI")){
    /* a b + n → ~(a|~n) ~(b|~n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(a | ~n);
    long y = ~(b | ~n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNORNI")||kw(&L->cur,"S2XNORNI")||kw(&L->cur,"STACK2XNORNI")||
      kw(&L->cur,"PAIRXNORNI")||kw(&L->cur,"DEQUIVNI")||kw(&L->cur,"DXNORNOTI")||
      kw(&L->cur,"PAIRXNORNOTI")||kw(&L->cur,"PAIRSEQUIVNI")){
    /* a b + n → ~(a^~n) ~(b^~n)  (equiv DXORI / a^n , b^n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(a ^ ~n);
    long y = ~(b ^ ~n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack reverse imm ANDN: DANDNFROMN · DORNFROMN · DXORNFROMN
   * (a b + n → (n op ~a) (n op ~b); dual of SANDNFROMN after DANDNI plane) */
  if (kw(&L->cur,"DANDNFROMN")||kw(&L->cur,"2ANDNFROMN")||kw(&L->cur,"S2ANDNFROMN")||
      kw(&L->cur,"STACK2ANDNFROMN")||kw(&L->cur,"PAIRANDNFROMN")||kw(&L->cur,"DBICFROMN")||
      kw(&L->cur,"2BICFROMN")||kw(&L->cur,"PAIRBICFROMN")||kw(&L->cur,"DANDNFROMIMM")||
      kw(&L->cur,"DRANDNFROMN")||kw(&L->cur,"PAIRANDNOTFROMN")){
    /* a b + n → (n & ~a) (n & ~b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = n & ~a;
    long y = n & ~b;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DORNFROMN")||kw(&L->cur,"2ORNFROMN")||kw(&L->cur,"S2ORNFROMN")||
      kw(&L->cur,"STACK2ORNFROMN")||kw(&L->cur,"PAIRORNFROMN")||kw(&L->cur,"DORNOTFROMN")||
      kw(&L->cur,"2ORNOTFROMN")||kw(&L->cur,"PAIRORNOTFROMN")||kw(&L->cur,"DORNFROMIMM")||
      kw(&L->cur,"DRORNFROMN")){
    /* a b + n → (n | ~a) (n | ~b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = n | ~a;
    long y = n | ~b;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXORNFROMN")||kw(&L->cur,"2XORNFROMN")||kw(&L->cur,"S2XORNFROMN")||
      kw(&L->cur,"STACK2XORNFROMN")||kw(&L->cur,"PAIRXORNFROMN")||kw(&L->cur,"DXORNOTFROMN")||
      kw(&L->cur,"2XORNOTFROMN")||kw(&L->cur,"PAIRXORNOTFROMN")||kw(&L->cur,"DXORNFROMIMM")||
      kw(&L->cur,"DRXORNFROMN")){
    /* a b + n → (n ^ ~a) (n ^ ~b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = n ^ ~a;
    long y = n ^ ~b;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack reverse imm inverted ANDN: DNANDNFROMN · DNORNFROMN · DXNORNFROMN
   * (a b + n → ~(n op ~a) ~(n op ~b); dual of SNANDNFROMN after DANDNFROMN plane) */
  if (kw(&L->cur,"DNANDNFROMN")||kw(&L->cur,"2NANDNFROMN")||kw(&L->cur,"S2NANDNFROMN")||
      kw(&L->cur,"STACK2NANDNFROMN")||kw(&L->cur,"PAIRNANDNFROMN")||kw(&L->cur,"DINVERTANDNFROMN")||
      kw(&L->cur,"DNANDNFROMIMM")||kw(&L->cur,"DRNANDNFROMN")||kw(&L->cur,"PAIRNANDNOTFROMN")){
    /* a b + n → ~(n & ~a) ~(n & ~b)  (= (~n|a) (~n|b)) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(n & ~a);
    long y = ~(n & ~b);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNORNFROMN")||kw(&L->cur,"2NORNFROMN")||kw(&L->cur,"S2NORNFROMN")||
      kw(&L->cur,"STACK2NORNFROMN")||kw(&L->cur,"PAIRNORNFROMN")||kw(&L->cur,"DINVERTORNFROMN")||
      kw(&L->cur,"DNORNFROMIMM")||kw(&L->cur,"DRNORNFROMN")||kw(&L->cur,"PAIRNORNOTFROMN")){
    /* a b + n → ~(n | ~a) ~(n | ~b)  (= (~n&a) (~n&b)) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(n | ~a);
    long y = ~(n | ~b);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNORNFROMN")||kw(&L->cur,"2XNORNFROMN")||kw(&L->cur,"S2XNORNFROMN")||
      kw(&L->cur,"STACK2XNORNFROMN")||kw(&L->cur,"PAIRXNORNFROMN")||kw(&L->cur,"DEQUIVNFROMN")||
      kw(&L->cur,"DXNORNFROMIMM")||kw(&L->cur,"DRXNORNFROMN")||kw(&L->cur,"PAIRXNORNOTFROMN")){
    /* a b + n → ~(n ^ ~a) ~(n ^ ~b)  (equiv (n^a) (n^b)) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(n ^ ~a);
    long y = ~(n ^ ~b);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
