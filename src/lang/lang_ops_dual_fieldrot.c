/* CubalC lang — lang_ops_dual_fieldrot.c
 * DUAL field reverse/rotate: DBREVN·DROLBN·DRORBN·DBREVHN·DROLHN·DRORHN (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_fieldrot.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_fieldrot(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-5 dual-stack low-n field reverse/rotate: DBREVN · DROLBN · DRORBN */
  if (kw(&L->cur,"DBREVN")||kw(&L->cur,"2BREVN")||kw(&L->cur,"S2BREVN")||
      kw(&L->cur,"STACK2BREVN")||kw(&L->cur,"PAIRBREVN")||kw(&L->cur,"DREVLOWN")||
      kw(&L->cur,"2REVLOWN")||kw(&L->cur,"DBITREVN")||kw(&L->cur,"2BITREVN")){
    /* a b + n → reverse low n bits of each; high bits kept; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a, y = b;
    if (n > 0 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m, lb = (unsigned long)b & m;
      unsigned long ra = 0, rb = 0;
      for (long i = 0; i < n; i++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
        rb = (rb << 1) | (lb & 1ul); lb >>= 1;
      }
      x = (long)(((unsigned long)a & ~m) | (ra & m));
      y = (long)(((unsigned long)b & ~m) | (rb & m));
    } else if (n >= 64){
      /* full reverse 64 */
      unsigned long la = (unsigned long)a, lb = (unsigned long)b;
      unsigned long ra = 0, rb = 0;
      for (int i = 0; i < 64; i++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
        rb = (rb << 1) | (lb & 1ul); lb >>= 1;
      }
      x = (long)ra; y = (long)rb;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROLBN")||kw(&L->cur,"2ROLBN")||kw(&L->cur,"S2ROLBN")||
      kw(&L->cur,"STACK2ROLBN")||kw(&L->cur,"PAIRROLBN")||kw(&L->cur,"DROTLBN")||
      kw(&L->cur,"2ROTLBN")||kw(&L->cur,"DLOWROLN")||kw(&L->cur,"2LOWROLN")){
    /* a b + n → rotate left by 1 within low n bits; high kept; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a, y = b;
    if (n >= 2 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m, lb = (unsigned long)b & m;
      la = ((la << 1) | (la >> (unsigned)(n - 1))) & m;
      lb = ((lb << 1) | (lb >> (unsigned)(n - 1))) & m;
      x = (long)(((unsigned long)a & ~m) | la);
      y = (long)(((unsigned long)b & ~m) | lb);
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      x = (long)((ua << 1) | (ua >> 63));
      y = (long)((ub << 1) | (ub >> 63));
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DRORBN")||kw(&L->cur,"2RORBN")||kw(&L->cur,"S2RORBN")||
      kw(&L->cur,"STACK2RORBN")||kw(&L->cur,"PAIRRORBN")||kw(&L->cur,"DROTRBN")||
      kw(&L->cur,"2ROTRBN")||kw(&L->cur,"DLOWRORN")||kw(&L->cur,"2LOWRORN")){
    /* a b + n → rotate right by 1 within low n bits; high kept; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a, y = b;
    if (n >= 2 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m, lb = (unsigned long)b & m;
      la = ((la >> 1) | (la << (unsigned)(n - 1))) & m;
      lb = ((lb >> 1) | (lb << (unsigned)(n - 1))) & m;
      x = (long)(((unsigned long)a & ~m) | la);
      y = (long)(((unsigned long)b & ~m) | lb);
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      x = (long)((ua >> 1) | (ua << 63));
      y = (long)((ub >> 1) | (ub << 63));
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack high-n field reverse/rotate: DBREVHN · DROLHN · DRORHN */
  if (kw(&L->cur,"DBREVHN")||kw(&L->cur,"2BREVHN")||kw(&L->cur,"S2BREVHN")||
      kw(&L->cur,"STACK2BREVHN")||kw(&L->cur,"PAIRBREVHN")||kw(&L->cur,"DREVHIGHN")||
      kw(&L->cur,"2REVHIGHN")||kw(&L->cur,"DBITREVHN")||kw(&L->cur,"2BITREVHN")){
    /* a b + n → reverse high n bits of each; low bits kept; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a, y = b;
    if (n > 0 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long lb = ((unsigned long)b & m) >> sh;
      unsigned long ra = 0, rb = 0;
      for (long i = 0; i < n; i++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
        rb = (rb << 1) | (lb & 1ul); lb >>= 1;
      }
      x = (long)(((unsigned long)a & ~m) | ((ra << sh) & m));
      y = (long)(((unsigned long)b & ~m) | ((rb << sh) & m));
    } else if (n >= 64){
      unsigned long la = (unsigned long)a, lb = (unsigned long)b;
      unsigned long ra = 0, rb = 0;
      for (int i = 0; i < 64; i++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
        rb = (rb << 1) | (lb & 1ul); lb >>= 1;
      }
      x = (long)ra; y = (long)rb;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROLHN")||kw(&L->cur,"2ROLHN")||kw(&L->cur,"S2ROLHN")||
      kw(&L->cur,"STACK2ROLHN")||kw(&L->cur,"PAIRROLHN")||kw(&L->cur,"DROTLHN")||
      kw(&L->cur,"2ROTLHN")||kw(&L->cur,"DHIGHROLN")||kw(&L->cur,"2HIGHROLN")){
    /* a b + n → rotate left by 1 within high n bits; low kept; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a, y = b;
    if (n >= 2 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long lb = ((unsigned long)b & m) >> sh;
      unsigned long fm = (1ul << (unsigned)n) - 1ul;
      la = ((la << 1) | (la >> (unsigned)(n - 1))) & fm;
      lb = ((lb << 1) | (lb >> (unsigned)(n - 1))) & fm;
      x = (long)(((unsigned long)a & ~m) | ((la << sh) & m));
      y = (long)(((unsigned long)b & ~m) | ((lb << sh) & m));
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      x = (long)((ua << 1) | (ua >> 63));
      y = (long)((ub << 1) | (ub >> 63));
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DRORHN")||kw(&L->cur,"2RORHN")||kw(&L->cur,"S2RORHN")||
      kw(&L->cur,"STACK2RORHN")||kw(&L->cur,"PAIRRORHN")||kw(&L->cur,"DROTRHN")||
      kw(&L->cur,"2ROTRHN")||kw(&L->cur,"DHIGHRORN")||kw(&L->cur,"2HIGHRORN")){
    /* a b + n → rotate right by 1 within high n bits; low kept; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a, y = b;
    if (n >= 2 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long lb = ((unsigned long)b & m) >> sh;
      unsigned long fm = (1ul << (unsigned)n) - 1ul;
      la = ((la >> 1) | (la << (unsigned)(n - 1))) & fm;
      lb = ((lb >> 1) | (lb << (unsigned)(n - 1))) & fm;
      x = (long)(((unsigned long)a & ~m) | ((la << sh) & m));
      y = (long)(((unsigned long)b & ~m) | ((lb << sh) & m));
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      x = (long)((ua >> 1) | (ua << 63));
      y = (long)((ub >> 1) | (ub << 63));
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
