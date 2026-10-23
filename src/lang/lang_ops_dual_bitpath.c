/* CubalC lang — lang_ops_dual_bitpath.c
 * DUAL bitpath: DPEXT·DPDEP·DZIP·DUNZIP·DBEXT·DBDEP (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_bitpath.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_bitpath(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-3 dual-stack parallel extract/deposit: DPEXT · DPDEP (BMI2-style) */
  if (kw(&L->cur,"DPEXT")||kw(&L->cur,"2PEXT")||kw(&L->cur,"S2PEXT")||
      kw(&L->cur,"STACK2PEXT")||kw(&L->cur,"PAIRPEXT")||kw(&L->cur,"2PEXTRACT")||
      kw(&L->cur,"DPEXTRACT")||
      kw(&L->cur,"DPDEP")||kw(&L->cur,"2PDEP")||kw(&L->cur,"S2PDEP")||
      kw(&L->cur,"STACK2PDEP")||kw(&L->cur,"PAIRPDEP")||kw(&L->cur,"2PDEPOSIT")||
      kw(&L->cur,"DPDEPOSIT")){
    /* a b ma mb → f(a,ma) f(b,mb)
     * PEXT: gather bits of src under mask ones into low bits
     * PDEP: scatter low bits of src into mask ones positions */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_dep = (strcmp(op,"DPDEP")==0 || strcmp(op,"2PDEP")==0 || strcmp(op,"S2PDEP")==0 ||
                  strcmp(op,"STACK2PDEP")==0 || strcmp(op,"PAIRPDEP")==0 ||
                  strcmp(op,"2PDEPOSIT")==0 || strcmp(op,"DPDEPOSIT")==0);
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long long mb = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long ma = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long sb = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long sa = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long ra = 0, rb = 0;
    if (is_dep){
      unsigned long long ba = 1, bb = 1;
      for (int i = 0; i < 64; i++){
        if ((ma >> i) & 1ull){
          if (sa & ba) ra |= (1ull << i);
          ba <<= 1;
        }
        if ((mb >> i) & 1ull){
          if (sb & bb) rb |= (1ull << i);
          bb <<= 1;
        }
      }
    } else {
      unsigned long long ka = 0, kb = 0;
      for (int i = 0; i < 64; i++){
        if ((ma >> i) & 1ull){
          if ((sa >> i) & 1ull) ra |= (1ull << ka);
          ka++;
        }
        if ((mb >> i) & 1ull){
          if ((sb >> i) & 1ull) rb |= (1ull << kb);
          kb++;
        }
      }
    }
    long x = (long)ra, y = (long)rb;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack Morton zip/unzip: DZIP · DUNZIP */
  if (kw(&L->cur,"DZIP")||kw(&L->cur,"2ZIP")||kw(&L->cur,"S2ZIP")||
      kw(&L->cur,"STACK2ZIP")||kw(&L->cur,"PAIRZIP")||kw(&L->cur,"2MORTON")||
      kw(&L->cur,"DMORTON")){
    /* a b c d → zip(a,c) zip(b,d); low 32 bits of each lane interleaved
     * zip(x,y): bit i of x → bit 2i; bit i of y → bit 2i+1 */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long long d = (unsigned long long)(unsigned int)vm->stack[--vm->sp];
    unsigned long long c = (unsigned long long)(unsigned int)vm->stack[--vm->sp];
    unsigned long long b = (unsigned long long)(unsigned int)vm->stack[--vm->sp];
    unsigned long long a = (unsigned long long)(unsigned int)vm->stack[--vm->sp];
    unsigned long long rx = 0, ry = 0;
    for (int i = 0; i < 32; i++){
      if ((a >> i) & 1ull) rx |= (1ull << (2 * i));
      if ((c >> i) & 1ull) rx |= (1ull << (2 * i + 1));
      if ((b >> i) & 1ull) ry |= (1ull << (2 * i));
      if ((d >> i) & 1ull) ry |= (1ull << (2 * i + 1));
    }
    long x = (long)rx, y = (long)ry;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DUNZIP")||kw(&L->cur,"2UNZIP")||kw(&L->cur,"S2UNZIP")||
      kw(&L->cur,"STACK2UNZIP")||kw(&L->cur,"PAIRUNZIP")||kw(&L->cur,"2DEMORTON")||
      kw(&L->cur,"DDEMORTON")){
    /* a b → even(a) even(b) odd(a) odd(b)
     * even = bits at even positions packed low; odd = bits at odd positions
     * stack ends with TOS = odd(b) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long long zb = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long za = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long ea = 0, oa = 0, eb = 0, ob = 0;
    for (int i = 0; i < 32; i++){
      if ((za >> (2 * i)) & 1ull) ea |= (1ull << i);
      if ((za >> (2 * i + 1)) & 1ull) oa |= (1ull << i);
      if ((zb >> (2 * i)) & 1ull) eb |= (1ull << i);
      if ((zb >> (2 * i + 1)) & 1ull) ob |= (1ull << i);
    }
    vm->stack[vm->sp++] = (long)ea;
    vm->stack[vm->sp++] = (long)eb;
    vm->stack[vm->sp++] = (long)oa;
    vm->stack[vm->sp++] = (long)ob;
    var_set_num(vm,"LAST_N",(long)ob); vm->last_n=(long)ob;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack field extract/deposit: DBEXT · DBDEP */
  if (kw(&L->cur,"DBEXT")||kw(&L->cur,"2BEXT")||kw(&L->cur,"S2BEXT")||
      kw(&L->cur,"STACK2BEXT")||kw(&L->cur,"PAIRBEXT")||kw(&L->cur,"2BITEXT")||
      kw(&L->cur,"DBITEXT")){
    /* a b pos width → extract width bits at pos from each (pos/width shared) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long width = vm->stack[--vm->sp];
    long pos = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (pos < 0) pos = 0;
    if (pos > 62){
      vm->stack[vm->sp++] = 0;
      vm->stack[vm->sp++] = 0;
      var_set_num(vm,"LAST_N",0); vm->last_n=0;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    if (width < 1) width = 0;
    if (width > 63 - pos) width = 63 - pos;
    long x = 0, y = 0;
    if (width > 0){
      unsigned long mask = (width >= 63) ? ~0ul : ((1ul << (unsigned)width) - 1ul);
      x = (long)(((unsigned long)a >> (unsigned)pos) & mask);
      y = (long)(((unsigned long)b >> (unsigned)pos) & mask);
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DBDEP")||kw(&L->cur,"2BDEP")||kw(&L->cur,"S2BDEP")||
      kw(&L->cur,"STACK2BDEP")||kw(&L->cur,"PAIRBDEP")||kw(&L->cur,"2BITDEP")||
      kw(&L->cur,"DBITDEP")){
    /* a b fa fb pos → deposit low 8 bits of fa/fb into a/b at pos (SBDEP dual) */
    lex_next(L);
    if (vm->sp < 5){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long pos = vm->stack[--vm->sp];
    long fb = vm->stack[--vm->sp];
    long fa = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long width = 8;
    if (pos < 0) pos = 0;
    if (pos > 62){
      vm->stack[vm->sp++] = a;
      vm->stack[vm->sp++] = b;
      var_set_num(vm,"LAST_N",b); vm->last_n=b;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    if (width > 63 - pos) width = 63 - pos;
    long x = a, y = b;
    if (width > 0){
      unsigned long mask = (width >= 63) ? ~0ul : ((1ul << (unsigned)width) - 1ul);
      unsigned long ba = (unsigned long)a;
      unsigned long bb = (unsigned long)b;
      unsigned long faa = (unsigned long)fa & mask;
      unsigned long fbb = (unsigned long)fb & mask;
      ba = (ba & ~(mask << (unsigned)pos)) | (faa << (unsigned)pos);
      bb = (bb & ~(mask << (unsigned)pos)) | (fbb << (unsigned)pos);
      x = (long)ba; y = (long)bb;
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
