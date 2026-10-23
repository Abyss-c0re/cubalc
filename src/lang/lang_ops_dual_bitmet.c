/* CubalC lang — lang_ops_dual_bitmet.c
 * DUAL bit metrics: DFFS·DFLS·DBWIDTH·DBHSI·DCEILPOW2·DCLO·DCTO·DISPOW2 (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_bitmet.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_bitmet(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-7 dual-stack bit-position metrics: DFFS · DFLS · DBWIDTH (unary pair) */
  if (kw(&L->cur,"DFFS")||kw(&L->cur,"2FFS")||kw(&L->cur,"S2FFS")||
      kw(&L->cur,"STACK2FFS")||kw(&L->cur,"PAIRFFS")||kw(&L->cur,"2FINDLS")||
      kw(&L->cur,"DFINDFS")||
      kw(&L->cur,"DFLS")||kw(&L->cur,"2FLS")||kw(&L->cur,"S2FLS")||
      kw(&L->cur,"STACK2FLS")||kw(&L->cur,"PAIRFLS")||kw(&L->cur,"2MSB")||
      kw(&L->cur,"DMSB")||
      kw(&L->cur,"DBWIDTH")||kw(&L->cur,"2BWIDTH")||kw(&L->cur,"S2BWIDTH")||
      kw(&L->cur,"STACK2BWIDTH")||kw(&L->cur,"PAIRBWIDTH")||kw(&L->cur,"2BITWIDTH")||
      kw(&L->cur,"DBITWIDTH")){
    /* a b → metric(a) metric(b)
     * FFS: 1-based index of lowest 1-bit (0 if zero)
     * FLS: 1-based index of highest 1-bit (0 if zero)
     * BWIDTH: minimal bits to represent unsigned (0 if zero) = FLS */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_ffs = (strcmp(op,"DFFS")==0 || strcmp(op,"2FFS")==0 || strcmp(op,"S2FFS")==0 ||
                  strcmp(op,"STACK2FFS")==0 || strcmp(op,"PAIRFFS")==0 ||
                  strcmp(op,"2FINDLS")==0 || strcmp(op,"DFINDFS")==0);
    int is_fls = (strcmp(op,"DFLS")==0 || strcmp(op,"2FLS")==0 || strcmp(op,"S2FLS")==0 ||
                  strcmp(op,"STACK2FLS")==0 || strcmp(op,"PAIRFLS")==0 ||
                  strcmp(op,"2MSB")==0 || strcmp(op,"DMSB")==0);
    /* else DBWIDTH / BITWIDTH */
    long x = 0, y = 0;
    if (is_ffs){
      if (a != 0){
        unsigned long ua = (unsigned long)a; x = 1;
        while ((ua & 1ul) == 0){ x++; ua >>= 1; }
      }
      if (b != 0){
        unsigned long ub = (unsigned long)b; y = 1;
        while ((ub & 1ul) == 0){ y++; ub >>= 1; }
      }
    } else {
      /* FLS and BWIDTH share highest-bit position; both return 0 for zero */
      if (a != 0){
        unsigned long ua = (unsigned long)a;
        for (int i = 63; i >= 0; i--){
          if (ua & (1ul << (unsigned)i)){ x = (long)(i + 1); break; }
        }
      }
      if (b != 0){
        unsigned long ub = (unsigned long)b;
        for (int i = 63; i >= 0; i--){
          if (ub & (1ul << (unsigned)i)){ y = (long)(i + 1); break; }
        }
      }
      (void)is_fls; /* FLS == BWIDTH numeric result for unsigned words */
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack bit/power metrics: DBHSI · DCEILPOW2 */
  if (kw(&L->cur,"DBHSI")||kw(&L->cur,"2BHSI")||kw(&L->cur,"S2BHSI")||
      kw(&L->cur,"STACK2BHSI")||kw(&L->cur,"PAIRBHSI")||kw(&L->cur,"2HIBIT")||
      kw(&L->cur,"DHIBIT")||
      kw(&L->cur,"DCEILPOW2")||kw(&L->cur,"2CEILPOW2")||kw(&L->cur,"S2CEILPOW2")||
      kw(&L->cur,"STACK2CEILPOW2")||kw(&L->cur,"PAIRCEILPOW2")||kw(&L->cur,"2NEXTPOW2")||
      kw(&L->cur,"DNEXTPOW2")||kw(&L->cur,"2CPOW2")||kw(&L->cur,"DCPOW2")){
    /* a b → f(a) f(b)
     * BHSI/HIBIT: isolate highest set bit (value with only that bit); 0 → 0
     * CEILPOW2/NEXTPOW2: smallest power of 2 ≥ n; n<=0 → 0; overflow → 0 */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_bhsi = (strcmp(op,"DBHSI")==0 || strcmp(op,"2BHSI")==0 || strcmp(op,"S2BHSI")==0 ||
                   strcmp(op,"STACK2BHSI")==0 || strcmp(op,"PAIRBHSI")==0 ||
                   strcmp(op,"2HIBIT")==0 || strcmp(op,"DHIBIT")==0);
    long x = 0, y = 0;
    if (is_bhsi){
      if (a != 0){
        unsigned long ua = (unsigned long)a;
        for (int i = 63; i >= 0; i--){
          if (ua & (1ul << (unsigned)i)){ x = (long)(1ul << (unsigned)i); break; }
        }
      }
      if (b != 0){
        unsigned long ub = (unsigned long)b;
        for (int i = 63; i >= 0; i--){
          if (ub & (1ul << (unsigned)i)){ y = (long)(1ul << (unsigned)i); break; }
        }
      }
    } else {
      /* CEILPOW2 */
      if (a > 0){
        if (a == 1) x = 1;
        else {
          unsigned long u = (unsigned long)a;
          if ((u & (u - 1ul)) == 0ul) x = a;
          else if (a <= (1L << 62)){
            x = 1;
            while (x < a){
              if (x > (1L << 61)){ x = 0; break; }
              x <<= 1;
            }
          }
        }
      }
      if (b > 0){
        if (b == 1) y = 1;
        else {
          unsigned long u = (unsigned long)b;
          if ((u & (u - 1ul)) == 0ul) y = b;
          else if (b <= (1L << 62)){
            y = 1;
            while (y < b){
              if (y > (1L << 61)){ y = 0; break; }
              y <<= 1;
            }
          }
        }
      }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack ones-metrics + power-of-2: DCLO · DCTO · DISPOW2 */
  if (kw(&L->cur,"DCLO")||kw(&L->cur,"2CLO")||kw(&L->cur,"S2CLO")||
      kw(&L->cur,"STACK2CLO")||kw(&L->cur,"PAIRCLO")||
      kw(&L->cur,"DCTO")||kw(&L->cur,"2CTO")||kw(&L->cur,"S2CTO")||
      kw(&L->cur,"STACK2CTO")||kw(&L->cur,"PAIRCTO")||
      kw(&L->cur,"DISPOW2")||kw(&L->cur,"2ISPOW2")||kw(&L->cur,"S2ISPOW2")||
      kw(&L->cur,"STACK2ISPOW2")||kw(&L->cur,"PAIRISPOW2")||kw(&L->cur,"2POW2P")||
      kw(&L->cur,"DPOW2P")){
    /* a b → metric(a) metric(b)
     * CLO: count leading ones in 64-bit word
     * CTO: count trailing ones
     * ISPOW2: 1 if exactly one bit set and value > 0 */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_clo = (strcmp(op,"DCLO")==0 || strcmp(op,"2CLO")==0 || strcmp(op,"S2CLO")==0 ||
                  strcmp(op,"STACK2CLO")==0 || strcmp(op,"PAIRCLO")==0);
    int is_cto = (strcmp(op,"DCTO")==0 || strcmp(op,"2CTO")==0 || strcmp(op,"S2CTO")==0 ||
                  strcmp(op,"STACK2CTO")==0 || strcmp(op,"PAIRCTO")==0);
    long x = 0, y = 0;
    if (is_clo){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      for (int i = 63; i >= 0; i--){
        if ((ua & (1ul << (unsigned)i)) == 0) break;
        x++;
      }
      for (int i = 63; i >= 0; i--){
        if ((ub & (1ul << (unsigned)i)) == 0) break;
        y++;
      }
    } else if (is_cto){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      while (ua & 1ul){ x++; ua >>= 1; if (x >= 64) break; }
      while (ub & 1ul){ y++; ub >>= 1; if (y >= 64) break; }
    } else {
      /* ISPOW2 / POW2P */
      if (a > 0){
        unsigned long ua = (unsigned long)a;
        x = ((ua & (ua - 1ul)) == 0ul) ? 1 : 0;
      }
      if (b > 0){
        unsigned long ub = (unsigned long)b;
        y = ((ub & (ub - 1ul)) == 0ul) ? 1 : 0;
      }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
