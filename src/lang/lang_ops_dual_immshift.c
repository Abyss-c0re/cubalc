/* CubalC lang — lang_ops_dual_immshift.c
 * DUAL imm rotate/shift: DROLN·DRORN·DSHLN·DSHRN·DSARN (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_immshift.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_immshift(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-3 dual-stack immediate rotate: DROLN · DRORN (dual of SROLN/SRORN) */
  if (kw(&L->cur,"DROLN")||kw(&L->cur,"2ROLN")||kw(&L->cur,"S2ROLN")||
      kw(&L->cur,"STACK2ROLN")||kw(&L->cur,"PAIRROLN")||kw(&L->cur,"DROTLN")||
      kw(&L->cur,"2ROTLN")||kw(&L->cur,"PAIRROTLN")||kw(&L->cur,"DROLIMM")){
    /* a b + n → rotl(a,n) rotl(b,n); n mod 64; n<0 → 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    unsigned uk = (unsigned)(n & 63);
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    long x = (uk == 0) ? (long)ua : (long)((ua << uk) | (ua >> (64u - uk)));
    long y = (uk == 0) ? (long)ub : (long)((ub << uk) | (ub >> (64u - uk)));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DRORN")||kw(&L->cur,"2RORN")||kw(&L->cur,"S2RORN")||
      kw(&L->cur,"STACK2RORN")||kw(&L->cur,"PAIRRORN")||kw(&L->cur,"DROTRN")||
      kw(&L->cur,"2ROTRN")||kw(&L->cur,"PAIRROTRN")||kw(&L->cur,"DRORIMM")){
    /* a b + n → rotr(a,n) rotr(b,n); n mod 64; n<0 → 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    unsigned uk = (unsigned)(n & 63);
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    long x = (uk == 0) ? (long)ua : (long)((ua >> uk) | (ua << (64u - uk)));
    long y = (uk == 0) ? (long)ub : (long)((ub >> uk) | (ub << (64u - uk)));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack immediate shift: DSHLN · DSHRN · DSARN (dual of SSHLN/SSHRN/SSARN) */
  if (kw(&L->cur,"DSHLN")||kw(&L->cur,"2SHLN")||kw(&L->cur,"S2SHLN")||
      kw(&L->cur,"STACK2SHLN")||kw(&L->cur,"PAIRSHLN")||kw(&L->cur,"DSHLIMM")||
      kw(&L->cur,"2SHLIMM")||kw(&L->cur,"PAIRSHLIMM")){
    /* a b + n → (a<<n) (b<<n); n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)((unsigned long)a << (unsigned)n);
    long y = (long)((unsigned long)b << (unsigned)n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSHRN")||kw(&L->cur,"2SHRN")||kw(&L->cur,"S2SHRN")||
      kw(&L->cur,"STACK2SHRN")||kw(&L->cur,"PAIRSHRN")||kw(&L->cur,"DSHRIMM")||
      kw(&L->cur,"2SHRIMM")||kw(&L->cur,"PAIRSHRIMM")){
    /* a b + n → logical (a>>n) (b>>n); n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)((unsigned long)a >> (unsigned)n);
    long y = (long)((unsigned long)b >> (unsigned)n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSARN")||kw(&L->cur,"2SARN")||kw(&L->cur,"S2SARN")||
      kw(&L->cur,"STACK2SARN")||kw(&L->cur,"PAIRSARN")||kw(&L->cur,"DASHRN")||
      kw(&L->cur,"2ASHRN")||kw(&L->cur,"DSARIMM")||kw(&L->cur,"PAIRASHRN")){
    /* a b + n → arithmetic (a>>n) (b>>n); n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a >> n;
    long y = b >> n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
