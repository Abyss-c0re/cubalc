/* CubalC lang — lang_ops_bit.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_bit(VM *vm, Lex *L){
  /* plane ops_bit: L22809-25535 */
  /* digit-3 stack bitwise ALU: SAND SOR SXOR SNOT SSHL SSHR */
  if (kw(&L->cur,"SAND")||kw(&L->cur,"STACKAND")||kw(&L->cur,"BANDST")||
      kw(&L->cur,"SOR")||kw(&L->cur,"STACKOR")||kw(&L->cur,"BORST")||
      kw(&L->cur,"SXOR")||kw(&L->cur,"STACKXOR")||kw(&L->cur,"BXORST")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (strcmp(op,"SAND")==0 || strcmp(op,"STACKAND")==0 || strcmp(op,"BANDST")==0)
      r = a & b;
    else if (strcmp(op,"SOR")==0 || strcmp(op,"STACKOR")==0 || strcmp(op,"BORST")==0)
      r = a | b;
    else if (strcmp(op,"SXOR")==0 || strcmp(op,"STACKXOR")==0 || strcmp(op,"BXORST")==0)
      r = a ^ b;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNOT")||kw(&L->cur,"STACKNOT")||kw(&L->cur,"BNOTST")||kw(&L->cur,"SINVERT")){
    /* bitwise invert TOS */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = ~vm->stack[vm->sp - 1];
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHL")||kw(&L->cur,"STACKSHL")||kw(&L->cur,"SLSHL")||
      kw(&L->cur,"SSHR")||kw(&L->cur,"STACKSHR")||kw(&L->cur,"SLSHR")){
    /* a b → a<<b or a>>b (logical-ish on unsigned cast for SHR) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (b < 0) b = 0;
    if (b > 63) b = 63;
    if (strcmp(op,"SSHL")==0 || strcmp(op,"STACKSHL")==0 || strcmp(op,"SLSHL")==0)
      r = (long)((unsigned long)a << (unsigned)b);
    else
      r = (long)((unsigned long)a >> (unsigned)b);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3/1 stack bit metrics: SPOPCNT SCLZ SCTZ SFFS SFLS SCLO SCTO SBWIDTH */
  if (kw(&L->cur,"SPOPCNT")||kw(&L->cur,"SPCOUNT")||kw(&L->cur,"STACKPOPCNT")||
      kw(&L->cur,"SPOPCOUNT")||kw(&L->cur,"SPCNT")||
      kw(&L->cur,"SCLZ")||kw(&L->cur,"STACKCLZ")||kw(&L->cur,"SNLZ")||
      kw(&L->cur,"SCTZ")||kw(&L->cur,"STACKCTZ")||kw(&L->cur,"SNTZ")||
      kw(&L->cur,"SFFS")||kw(&L->cur,"SFINDFS")||kw(&L->cur,"STACKFFS")||
      kw(&L->cur,"SFLS")||kw(&L->cur,"SMSB")||kw(&L->cur,"STACKFLS")||
      kw(&L->cur,"SCLO")||kw(&L->cur,"STACKCLO")||
      kw(&L->cur,"SCTO")||kw(&L->cur,"STACKCTO")||
      kw(&L->cur,"SBWIDTH")||kw(&L->cur,"SBITWIDTH")||kw(&L->cur,"STACKBWIDTH")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    unsigned long u = (unsigned long)a;
    long r = 0;
    if (strcmp(op,"SPOPCNT")==0 || strcmp(op,"SPCOUNT")==0 ||
        strcmp(op,"STACKPOPCNT")==0 || strcmp(op,"SPOPCOUNT")==0 ||
        strcmp(op,"SPCNT")==0){
      while (u){ r += (long)(u & 1ul); u >>= 1; }
    } else if (strcmp(op,"SCLZ")==0 || strcmp(op,"STACKCLZ")==0 || strcmp(op,"SNLZ")==0){
      if (u == 0) r = 64;
      else {
        for (int i = 63; i >= 0; i--){
          if (u & (1ul << i)) break;
          r++;
        }
      }
    } else if (strcmp(op,"SCTZ")==0 || strcmp(op,"STACKCTZ")==0 || strcmp(op,"SNTZ")==0){
      if (u == 0) r = 64;
      else {
        while ((u & 1ul) == 0){ r++; u >>= 1; }
      }
    } else if (strcmp(op,"SFFS")==0 || strcmp(op,"SFINDFS")==0 || strcmp(op,"STACKFFS")==0){
      if (u == 0) r = 0;
      else {
        r = 1;
        while ((u & 1ul) == 0){ r++; u >>= 1; }
      }
    } else if (strcmp(op,"SFLS")==0 || strcmp(op,"SMSB")==0 || strcmp(op,"STACKFLS")==0){
      if (u == 0) r = 0;
      else {
        for (int i = 63; i >= 0; i--){
          if (u & (1ul << (unsigned)i)){ r = i + 1; break; }
        }
      }
    } else if (strcmp(op,"SCLO")==0 || strcmp(op,"STACKCLO")==0){
      for (int i = 63; i >= 0; i--){
        if ((u & (1ul << (unsigned)i)) == 0) break;
        r++;
      }
    } else if (strcmp(op,"SCTO")==0 || strcmp(op,"STACKCTO")==0){
      while (u & 1ul){ r++; u >>= 1; if (r >= 64) break; }
    } else {
      /* SBWIDTH / SBITWIDTH / STACKBWIDTH */
      if (u == 0) r = 0;
      else {
        for (int i = 63; i >= 0; i--){
          if (u & (1ul << (unsigned)i)){ r = i + 1; break; }
        }
      }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROL")||kw(&L->cur,"STACKROL")||kw(&L->cur,"SROTL")||
      kw(&L->cur,"SROR")||kw(&L->cur,"STACKROR")||kw(&L->cur,"SROTR")||
      kw(&L->cur,"SSAR")||kw(&L->cur,"STACKSAR")||kw(&L->cur,"SASHR")){
    /* a k → rotate/arithmetic-shift a by k (k mod 64; k<0 → 0) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (k < 0) k = 0;
    unsigned uk = (unsigned)(k & 63);
    if (strcmp(op,"SSAR")==0 || strcmp(op,"STACKSAR")==0 || strcmp(op,"SASHR")==0){
      /* arithmetic right shift (sign-preserving) */
      if (k > 63) k = 63;
      r = a >> k;
    } else if (strcmp(op,"SROL")==0 || strcmp(op,"STACKROL")==0 || strcmp(op,"SROTL")==0){
      unsigned long u = (unsigned long)a;
      if (uk == 0) r = a;
      else r = (long)((u << uk) | (u >> (64u - uk)));
    } else {
      /* SROR / STACKROR / SROTR */
      unsigned long u = (unsigned long)a;
      if (uk == 0) r = a;
      else r = (long)((u >> uk) | (u << (64u - uk)));
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack bitfield data path: SBTEST SSETB SCLRB SFLPB SBEXT SBDEP */
  if (kw(&L->cur,"SBTEST")||kw(&L->cur,"SBITT")||kw(&L->cur,"STACKBIT")||
      kw(&L->cur,"SBTST")||kw(&L->cur,"TESTBIT")||
      kw(&L->cur,"SSETB")||kw(&L->cur,"SSETBIT")||kw(&L->cur,"STACKSETB")||
      kw(&L->cur,"SCLRB")||kw(&L->cur,"SCLRBIT")||kw(&L->cur,"STACKCLRB")||
      kw(&L->cur,"SFLPB")||kw(&L->cur,"SFLIPB")||kw(&L->cur,"STGLB")||
      kw(&L->cur,"STGLBIT")||kw(&L->cur,"STACKFLIPB")){
    /* a k → test/set/clear/flip bit k of a (k clamped 0..63) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (k < 0) k = 0;
    if (k > 63) k = 63;
    unsigned long uk = (unsigned long)k;
    unsigned long bit = 1ul << uk;
    unsigned long u = (unsigned long)a;
    long r = 0;
    if (strcmp(op,"SBTEST")==0 || strcmp(op,"SBITT")==0 || strcmp(op,"STACKBIT")==0 ||
        strcmp(op,"SBTST")==0 || strcmp(op,"TESTBIT")==0)
      r = (u & bit) ? 1 : 0;
    else if (strcmp(op,"SSETB")==0 || strcmp(op,"SSETBIT")==0 || strcmp(op,"STACKSETB")==0)
      r = (long)(u | bit);
    else if (strcmp(op,"SCLRB")==0 || strcmp(op,"SCLRBIT")==0 || strcmp(op,"STACKCLRB")==0)
      r = (long)(u & ~bit);
    else
      /* SFLPB / SFLIPB / STGLB / STGLBIT / STACKFLIPB */
      r = (long)(u ^ bit);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBEXT")||kw(&L->cur,"STACKBEXT")||kw(&L->cur,"SEXTR")||
      kw(&L->cur,"SBITEXT")){
    /* a pos width → extract width bits at pos (dual of BEXT) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long width = vm->stack[--vm->sp];
    long pos = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (pos < 0) pos = 0;
    if (pos > 62){ vm->stack[vm->sp++] = 0; var_set_num(vm,"LAST_N",0); vm->last_n=0;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (width < 1) width = 0;
    if (width > 63 - pos) width = 63 - pos;
    long r = 0;
    if (width > 0){
      unsigned long mask = (width >= 63) ? ~0ul : ((1ul << (unsigned)width) - 1ul);
      r = (long)(((unsigned long)a >> (unsigned)pos) & mask);
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBDEP")||kw(&L->cur,"STACKBDEP")||kw(&L->cur,"SDEP")||
      kw(&L->cur,"SBITDEP")){
    /* base field pos → deposit low 8 bits of field at pos (dual of BDEP) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long pos = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long width = 8;
    if (pos < 0) pos = 0;
    if (pos > 62){
      vm->stack[vm->sp++] = a;
      var_set_num(vm,"LAST_N",a); vm->last_n=a;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    if (width > 63 - pos) width = 63 - pos;
    long r = a;
    if (width > 0){
      unsigned long mask = (width >= 63) ? ~0ul : ((1ul << (unsigned)width) - 1ul);
      unsigned long base = (unsigned long)a;
      unsigned long f = (unsigned long)field & mask;
      base = (base & ~(mask << (unsigned)pos)) | (f << (unsigned)pos);
      r = (long)base;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0/9 stack immediate bitwise mask: SANDI · SORI · SXORI
   * NOTE: SANDN/SORN/SXORN stay stack-stack (a&~b / a|~b / dual of SNAND family). */
  if (kw(&L->cur,"SANDI")||kw(&L->cur,"ANDIMM")||kw(&L->cur,"STACKANDI")||
      kw(&L->cur,"ANDI")||kw(&L->cur,"SANDIMM")){
    /* SANDI n — TOS &= n */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1] & n;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORI")||kw(&L->cur,"ORIMM")||kw(&L->cur,"STACKORI")||
      kw(&L->cur,"ORI")||kw(&L->cur,"SORIMM")){
    /* SORI n — TOS |= n */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1] | n;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORI")||kw(&L->cur,"XORIMM")||kw(&L->cur,"STACKXORI")||
      kw(&L->cur,"XORI")||kw(&L->cur,"SXORIMM")){
    /* SXORI n — TOS ^= n */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1] ^ n;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack immediate inverted bitwise: SNANDI · SNORI · SXNORI (complete after SANDI/SORI/SXORI) */
  if (kw(&L->cur,"SNANDI")||kw(&L->cur,"NANDI")||kw(&L->cur,"STACKNANDI")||
      kw(&L->cur,"SNANDIMM")||kw(&L->cur,"NANDIMM")){
    /* SNANDI n — TOS = ~(TOS & n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = ~(vm->stack[vm->sp - 1] & n);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORI")||kw(&L->cur,"NORI")||kw(&L->cur,"STACKNORI")||
      kw(&L->cur,"SNORIMM")||kw(&L->cur,"NORIMM")){
    /* SNORI n — TOS = ~(TOS | n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = ~(vm->stack[vm->sp - 1] | n);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORI")||kw(&L->cur,"XNORI")||kw(&L->cur,"STACKXNORI")||
      kw(&L->cur,"SXNORIMM")||kw(&L->cur,"XNORIMM")||kw(&L->cur,"SEQUIVI")){
    /* SXNORI n — TOS = ~(TOS ^ n)  (bitwise equivalence with constant) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = ~(vm->stack[vm->sp - 1] ^ n);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack immediate ANDN plane: SANDNI · SORNI · SXORNI (TOS op ~imm after SANDI/SORI/SXORI + SNANDI) */
  if (kw(&L->cur,"SANDNI")||kw(&L->cur,"STACKANDNI")||kw(&L->cur,"ANDNI")||
      kw(&L->cur,"SBICI")||kw(&L->cur,"BICIMM")||kw(&L->cur,"SANDNOTI")||
      kw(&L->cur,"ANDNOTI")){
    /* SANDNI n — TOS &= ~n  (clear bits set in n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1] & ~n;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORNI")||kw(&L->cur,"STACKORNI")||kw(&L->cur,"ORNI")||
      kw(&L->cur,"SORNOTI")||kw(&L->cur,"ORNOTI")||kw(&L->cur,"SORNIMM")){
    /* SORNI n — TOS |= ~n */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1] | ~n;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORNI")||kw(&L->cur,"STACKXORNI")||kw(&L->cur,"XORNI")||
      kw(&L->cur,"SXORNOTI")||kw(&L->cur,"XORNOTI")||kw(&L->cur,"SXORNOTIMM")){
    /* SXORNI n — TOS ^= ~n  (equiv SXNORI / ~(TOS ^ n)) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1] ^ ~n;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm inverted ANDN plane: SNANDNI · SNORNI · SXNORNI (after SANDNI + SNANDI) */
  if (kw(&L->cur,"SNANDNI")||kw(&L->cur,"STACKNANDNI")||kw(&L->cur,"NANDNI")||
      kw(&L->cur,"SINVERTANDNI")||kw(&L->cur,"NANDNOTI")||kw(&L->cur,"SNANDNOTI")){
    /* SNANDNI n — TOS = ~(TOS & ~n)  (= ~TOS | n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = ~(vm->stack[vm->sp - 1] & ~n);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORNI")||kw(&L->cur,"STACKNORNI")||kw(&L->cur,"NORNI")||
      kw(&L->cur,"SINVERTORNI")||kw(&L->cur,"NORNOTI")||kw(&L->cur,"SNORNOTI")){
    /* SNORNI n — TOS = ~(TOS | ~n)  (= ~TOS & n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = ~(vm->stack[vm->sp - 1] | ~n);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORNI")||kw(&L->cur,"STACKXNORNI")||kw(&L->cur,"XNORNI")||
      kw(&L->cur,"SEQUIVNI")||kw(&L->cur,"XNORNOTI")||kw(&L->cur,"SXNORNOTI")){
    /* SXNORNI n — TOS = ~(TOS ^ ~n)  (equiv SXORI / TOS ^ n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = ~(vm->stack[vm->sp - 1] ^ ~n);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack low-n field reverse/rotate: SBREVN · SROLBN · SRORBN (dual of DBREVN/DROLBN/DRORBN) */
  if (kw(&L->cur,"SBREVN")||kw(&L->cur,"STACKBREVN")||kw(&L->cur,"BREVNS")||
      kw(&L->cur,"SREVLOWN")||kw(&L->cur,"SBITREVN")||kw(&L->cur,"STACKREVLOWN")){
    /* SBREVN n — reverse low n bits of TOS; high kept; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 1];
    long x = a;
    if (n > 0 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      unsigned long ra = 0;
      for (long i = 0; i < n; i++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      x = (long)(((unsigned long)a & ~m) | (ra & m));
    } else if (n >= 64){
      unsigned long la = (unsigned long)a;
      unsigned long ra = 0;
      for (int i = 0; i < 64; i++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      x = (long)ra;
    }
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROLBN")||kw(&L->cur,"STACKROLBN")||kw(&L->cur,"ROLBNS")||
      kw(&L->cur,"SROTLBN")||kw(&L->cur,"SLOWROLN")||kw(&L->cur,"STACKLOWROLN")){
    /* SROLBN n — rotate left by 1 within low n bits of TOS; high kept */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 1];
    long x = a;
    if (n >= 2 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      la = ((la << 1) | (la >> (unsigned)(n - 1))) & m;
      x = (long)(((unsigned long)a & ~m) | la);
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      x = (long)((ua << 1) | (ua >> 63));
    }
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORBN")||kw(&L->cur,"STACKRORBN")||kw(&L->cur,"RORBNS")||
      kw(&L->cur,"SROTRBN")||kw(&L->cur,"SLOWRORN")||kw(&L->cur,"STACKLOWRORN")){
    /* SRORBN n — rotate right by 1 within low n bits of TOS; high kept */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 1];
    long x = a;
    if (n >= 2 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      la = ((la >> 1) | (la << (unsigned)(n - 1))) & m;
      x = (long)(((unsigned long)a & ~m) | la);
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      x = (long)((ua >> 1) | (ua << 63));
    }
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack high-n field reverse/rotate: SBREVHN · SROLHN · SRORHN (dual of DBREVHN; complete SBREVN plane) */
  if (kw(&L->cur,"SBREVHN")||kw(&L->cur,"STACKBREVHN")||kw(&L->cur,"BREVHNS")||
      kw(&L->cur,"SREVHIGHN")||kw(&L->cur,"SBITREVHN")||kw(&L->cur,"STACKREVHIGHN")){
    /* SBREVHN n — reverse high n bits of TOS; low kept; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 1];
    long x = a;
    if (n > 0 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long ra = 0;
      for (long i = 0; i < n; i++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      x = (long)(((unsigned long)a & ~m) | ((ra << sh) & m));
    } else if (n >= 64){
      unsigned long la = (unsigned long)a;
      unsigned long ra = 0;
      for (int i = 0; i < 64; i++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      x = (long)ra;
    }
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROLHN")||kw(&L->cur,"STACKROLHN")||kw(&L->cur,"ROLHNS")||
      kw(&L->cur,"SROTLHN")||kw(&L->cur,"SHIGHROLN")||kw(&L->cur,"STACKHIGHROLN")){
    /* SROLHN n — rotate left by 1 within high n bits of TOS; low kept */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 1];
    long x = a;
    if (n >= 2 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long fm = (1ul << (unsigned)n) - 1ul;
      la = ((la << 1) | (la >> (unsigned)(n - 1))) & fm;
      x = (long)(((unsigned long)a & ~m) | ((la << sh) & m));
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      x = (long)((ua << 1) | (ua >> 63));
    }
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORHN")||kw(&L->cur,"STACKRORHN")||kw(&L->cur,"RORHNS")||
      kw(&L->cur,"SROTRHN")||kw(&L->cur,"SHIGHRORN")||kw(&L->cur,"STACKHIGHRORN")){
    /* SRORHN n — rotate right by 1 within high n bits of TOS; low kept */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 1];
    long x = a;
    if (n >= 2 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long fm = (1ul << (unsigned)n) - 1ul;
      la = ((la >> 1) | (la << (unsigned)(n - 1))) & fm;
      x = (long)(((unsigned long)a & ~m) | ((la << sh) & m));
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      x = (long)((ua >> 1) | (ua << 63));
    }
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack immediate bitfield: SSETBN SCLRBN SFLIPBN SBTESTN + SSHLN SSHRN */
  if (kw(&L->cur,"SSETBN")||kw(&L->cur,"SETBN")||kw(&L->cur,"SSETBITN")||
      kw(&L->cur,"STACKSETBN")){
    /* SSETBN n — TOS |= (1<<n); n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    unsigned long u = (unsigned long)vm->stack[vm->sp - 1];
    u |= (1ul << (unsigned)n);
    long v = (long)u;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRBN")||kw(&L->cur,"CLRBN")||kw(&L->cur,"SCLRBITN")||
      kw(&L->cur,"STACKCLRBN")||kw(&L->cur,"SCLRBIMM")){
    /* SCLRBN n — TOS &= ~(1<<n); n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    unsigned long u = (unsigned long)vm->stack[vm->sp - 1];
    u &= ~(1ul << (unsigned)n);
    long v = (long)u;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SFLIPBN")||kw(&L->cur,"FLIPBN")||kw(&L->cur,"STGLBN")||
      kw(&L->cur,"SFLPBN")||kw(&L->cur,"STACKFLIPBN")||kw(&L->cur,"STGLBITN")){
    /* SFLIPBN n — TOS ^= (1<<n); n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    unsigned long u = (unsigned long)vm->stack[vm->sp - 1];
    u ^= (1ul << (unsigned)n);
    long v = (long)u;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBTESTN")||kw(&L->cur,"TESTBITN")||kw(&L->cur,"SBITN")||
      kw(&L->cur,"SBTSTN")||kw(&L->cur,"STACKBITN")||kw(&L->cur,"SBITTESTN")){
    /* SBTESTN n — replace TOS with bit n (0/1); n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    unsigned long u = (unsigned long)vm->stack[vm->sp - 1];
    long v = (u & (1ul << (unsigned)n)) ? 1 : 0;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack-imm field duals: SBEXTN · SBDEPN · SMASKN (after SBEXT/SBDEP stack) */
  if (kw(&L->cur,"SBEXTN")||kw(&L->cur,"EXTN")||kw(&L->cur,"SEXTRN")||
      kw(&L->cur,"STACKBEXTN")||kw(&L->cur,"SBITEXTN")||kw(&L->cur,"BEXTN")){
    /* SBEXTN pos width — extract width bits at pos from TOS */
    lex_next(L);
    long pos = parse_expr(vm,L);
    long width = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (pos < 0) pos = 0;
    if (pos > 62){
      vm->stack[vm->sp - 1] = 0;
      var_set_num(vm,"LAST_N",0); vm->last_n=0;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    if (width < 1) width = 0;
    if (width > 63 - pos) width = 63 - pos;
    long r = 0;
    if (width > 0){
      unsigned long mask = (width >= 63) ? ~0ul : ((1ul << (unsigned)width) - 1ul);
      r = (long)(((unsigned long)vm->stack[vm->sp - 1] >> (unsigned)pos) & mask);
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBDEPN")||kw(&L->cur,"DEPN")||kw(&L->cur,"SDEPN")||
      kw(&L->cur,"STACKBDEPN")||kw(&L->cur,"SBITDEPN")||kw(&L->cur,"BDEPN")){
    /* SBDEPN field pos — deposit low 8 bits of field into TOS at bit pos */
    lex_next(L);
    long field = parse_expr(vm,L);
    long pos = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long width = 8;
    if (pos < 0) pos = 0;
    if (pos > 62){
      var_set_num(vm,"LAST_N",a); vm->last_n=a;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    if (width > 63 - pos) width = 63 - pos;
    long r = a;
    if (width > 0){
      unsigned long mask = (width >= 63) ? ~0ul : ((1ul << (unsigned)width) - 1ul);
      unsigned long base = (unsigned long)a;
      unsigned long f = (unsigned long)field & mask;
      base = (base & ~(mask << (unsigned)pos)) | (f << (unsigned)pos);
      r = (long)base;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMASKN")||kw(&L->cur,"MASKN")||kw(&L->cur,"BITSMSK")||
      kw(&L->cur,"STACKMASKN")||kw(&L->cur,"LOWMASKN")||kw(&L->cur,"ONESN")){
    /* SMASKN n — TOS = low-n-bit mask (1<<n)-1; n clamped 0..64 (64 → all ones) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){
      /* allow push form if stack empty: grow if room */
      if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
      vm->sp++; /* will write below */
    }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long v = (long)m;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack low-n mask plane: SANDMN · SORMN · SXORMN (dual of DANDMN/DORMN/DXORMN; energy bit-fill) */
  if (kw(&L->cur,"SANDMN")||kw(&L->cur,"STACKANDMN")||kw(&L->cur,"SKEEPLN")||
      kw(&L->cur,"SLOWANDN")||kw(&L->cur,"ANDMN")||kw(&L->cur,"KEEPLN")){
    /* SANDMN n — TOS &= low-n mask; keep low n bits; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long x = (long)((unsigned long)vm->stack[vm->sp - 1] & m);
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORMN")||kw(&L->cur,"STACKORMN")||kw(&L->cur,"SSETLN")||
      kw(&L->cur,"SLOWORN")||kw(&L->cur,"ORMN")||kw(&L->cur,"SETLN")){
    /* SORMN n — TOS |= low-n mask; set low n bits; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long x = (long)((unsigned long)vm->stack[vm->sp - 1] | m);
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORMN")||kw(&L->cur,"STACKXORMN")||kw(&L->cur,"SFLIPLN")||
      kw(&L->cur,"SLOWXORN")||kw(&L->cur,"XORMN")||kw(&L->cur,"FLIPLN")){
    /* SXORMN n — TOS ^= low-n mask; toggle low n bits; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long x = (long)((unsigned long)vm->stack[vm->sp - 1] ^ m);
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack inverted low-n mask: SNANDMN · SNORMN · SXNORMN (dual of DNANDMN/DNORMN/DXNORMN; invert SANDMN plane) */
  if (kw(&L->cur,"SNANDMN")||kw(&L->cur,"STACKNANDMN")||kw(&L->cur,"SLOWNANDN")||
      kw(&L->cur,"SNANDMASKN")||kw(&L->cur,"NANDMN")||kw(&L->cur,"MASKNANDN")){
    /* SNANDMN n — TOS = ~(TOS & low-n mask); n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long x = (long)~((unsigned long)vm->stack[vm->sp - 1] & m);
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORMN")||kw(&L->cur,"STACKNORMN")||kw(&L->cur,"SLOWNORN")||
      kw(&L->cur,"SNORMASKN")||kw(&L->cur,"NORMN")||kw(&L->cur,"MASKNORN")){
    /* SNORMN n — TOS = ~(TOS | low-n mask); n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long x = (long)~((unsigned long)vm->stack[vm->sp - 1] | m);
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORMN")||kw(&L->cur,"STACKXNORMN")||kw(&L->cur,"SLOWXNORN")||
      kw(&L->cur,"SXNORMASKN")||kw(&L->cur,"XNORMN")||kw(&L->cur,"MASKXNORN")||
      kw(&L->cur,"SEQUIVMN")||kw(&L->cur,"EQUIVMN")){
    /* SXNORMN n — TOS = ~(TOS ^ low-n mask); n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long x = (long)~((unsigned long)vm->stack[vm->sp - 1] ^ m);
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack high-n mask plane: SANDHN · SORHN · SXORHN (dual of DANDHN/DORHN/DXORHN; complete SANDMN) */
  if (kw(&L->cur,"SANDHN")||kw(&L->cur,"STACKANDHN")||kw(&L->cur,"SKEEPHN")||
      kw(&L->cur,"SHIGHANDN")||kw(&L->cur,"ANDHN")||kw(&L->cur,"KEEPHN")){
    /* SANDHN n — TOS &= high-n mask; keep high n bits; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long x = (long)((unsigned long)vm->stack[vm->sp - 1] & m);
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORHN")||kw(&L->cur,"STACKORHN")||kw(&L->cur,"SSETHN")||
      kw(&L->cur,"SHIGHORN")||kw(&L->cur,"ORHN")||kw(&L->cur,"SETHN")){
    /* SORHN n — TOS |= high-n mask; set high n bits; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long x = (long)((unsigned long)vm->stack[vm->sp - 1] | m);
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORHN")||kw(&L->cur,"STACKXORHN")||kw(&L->cur,"SFLIPHN")||
      kw(&L->cur,"SHIGHXORN")||kw(&L->cur,"XORHN")||kw(&L->cur,"FLIPHN")){
    /* SXORHN n — TOS ^= high-n mask; toggle high n bits; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long x = (long)((unsigned long)vm->stack[vm->sp - 1] ^ m);
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack inverted high-n mask: SNANDHN · SNORHN · SXNORHN (dual of DNANDHN/DNORHN/DXNORHN; complete SNANDMN plane) */
  if (kw(&L->cur,"SNANDHN")||kw(&L->cur,"STACKNANDHN")||kw(&L->cur,"SHIGHNANDN")||
      kw(&L->cur,"SNANDMASKH")||kw(&L->cur,"NANDHN")||kw(&L->cur,"MASKNANDH")){
    /* SNANDHN n — TOS = ~(TOS & high-n mask); n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long x = (long)~((unsigned long)vm->stack[vm->sp - 1] & m);
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORHN")||kw(&L->cur,"STACKNORHN")||kw(&L->cur,"SHIGHNORN")||
      kw(&L->cur,"SNORMASKH")||kw(&L->cur,"NORHN")||kw(&L->cur,"MASKNORH")){
    /* SNORHN n — TOS = ~(TOS | high-n mask); n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long x = (long)~((unsigned long)vm->stack[vm->sp - 1] | m);
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORHN")||kw(&L->cur,"STACKXNORHN")||kw(&L->cur,"SHIGHXNORN")||
      kw(&L->cur,"SXNORMASKH")||kw(&L->cur,"XNORHN")||kw(&L->cur,"MASKXNORH")||
      kw(&L->cur,"SEQUIVHN")||kw(&L->cur,"EQUIVHN")){
    /* SXNORHN n — TOS = ~(TOS ^ high-n mask); n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long x = (long)~((unsigned long)vm->stack[vm->sp - 1] ^ m);
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack low-n bitfield metrics: SPOPMN · SANYMN · SALLMN (dual of DPOPMN/DANYMN/DALLMN) */
  if (kw(&L->cur,"SPOPMN")||kw(&L->cur,"STACKPOPMN")||kw(&L->cur,"SONESMN")||
      kw(&L->cur,"SLOWPOPN")||kw(&L->cur,"SPCNTMN")||kw(&L->cur,"POPMN")){
    /* SPOPMN n — TOS = popcount(TOS & low-n mask); n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 1] & m;
    long x = 0;
    while (ua){ x += (long)(ua & 1ul); ua >>= 1; }
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SANYMN")||kw(&L->cur,"STACKANYMN")||kw(&L->cur,"SLOWANYN")||
      kw(&L->cur,"STESTANYN")||kw(&L->cur,"ANYMN")){
    /* SANYMN n — TOS = ((TOS & low-n mask) != 0) ? 1 : 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long x = (((unsigned long)vm->stack[vm->sp - 1] & m) != 0) ? 1 : 0;
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SALLMN")||kw(&L->cur,"STACKALLMN")||kw(&L->cur,"SLOWALLN")||
      kw(&L->cur,"STESTALLN")||kw(&L->cur,"ALLMN")){
    /* SALLMN n — TOS = ((TOS & low-n mask) == mask) ? 1 : 0; n=0 → 1 vacuous */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long x = (((unsigned long)vm->stack[vm->sp - 1] & m) == m) ? 1 : 0;
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack high-n bitfield metrics: SPOPHN · SANYHN · SALLHN (dual of DPOPHN; complete SPOPMN) */
  if (kw(&L->cur,"SPOPHN")||kw(&L->cur,"STACKPOPHN")||kw(&L->cur,"SONESHN")||
      kw(&L->cur,"SHIGHPOPN")||kw(&L->cur,"SPCNTHN")||kw(&L->cur,"POPHN")){
    /* SPOPHN n — TOS = popcount(TOS & high-n mask); n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 1] & m;
    long x = 0;
    while (ua){ x += (long)(ua & 1ul); ua >>= 1; }
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SANYHN")||kw(&L->cur,"STACKANYHN")||kw(&L->cur,"SHIGHANYN")||
      kw(&L->cur,"STESTANYHN")||kw(&L->cur,"ANYHN")){
    /* SANYHN n — TOS = ((TOS & high-n mask) != 0) ? 1 : 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long x = (((unsigned long)vm->stack[vm->sp - 1] & m) != 0) ? 1 : 0;
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SALLHN")||kw(&L->cur,"STACKALLHN")||kw(&L->cur,"SHIGHALLN")||
      kw(&L->cur,"STESTALLHN")||kw(&L->cur,"ALLHN")){
    /* SALLHN n — TOS = ((TOS & high-n mask) == mask) ? 1 : 0; n=0 → 1 vacuous */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long x = (((unsigned long)vm->stack[vm->sp - 1] & m) == m) ? 1 : 0;
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack clear + high-mask: SCLRMN · SCLRHN · SHMASKN (dual of DCLRLN/DCLRHN/DHMASKN) */
  if (kw(&L->cur,"SCLRMN")||kw(&L->cur,"STACKCLRMN")||kw(&L->cur,"SCLEARLN")||
      kw(&L->cur,"SZAPLN")||kw(&L->cur,"SLOWCLRN")||kw(&L->cur,"CLRLN")){
    /* SCLRMN n — TOS &= ~low-n mask; clear low n bits; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long x = (long)((unsigned long)vm->stack[vm->sp - 1] & ~m);
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRHN")||kw(&L->cur,"STACKCLRHN")||kw(&L->cur,"SCLEARHN")||
      kw(&L->cur,"SZAPHN")||kw(&L->cur,"SHIGHCLRN")||kw(&L->cur,"CLRHN")){
    /* SCLRHN n — TOS &= ~high-n mask; clear high n bits; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long x = (long)((unsigned long)vm->stack[vm->sp - 1] & ~m);
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHMASKN")||kw(&L->cur,"STACKHMASKN")||kw(&L->cur,"SHIMASKN")||
      kw(&L->cur,"SHIGHMASKN")||kw(&L->cur,"HMASKN")){
    /* SHMASKN n — TOS = high-n-bit mask; n clamped 0..64 (allows push if empty) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){
      if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
      vm->sp++;
    }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long v = (long)m;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHLN")||kw(&L->cur,"SHLN")||kw(&L->cur,"STACKSHLN")||
      kw(&L->cur,"SLSHLN")||kw(&L->cur,"SSHLIMM")){
    /* SSHLN n — TOS <<= n (n clamped 0..63) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long v = (long)((unsigned long)vm->stack[vm->sp - 1] << (unsigned)n);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHRN")||kw(&L->cur,"SHRN")||kw(&L->cur,"STACKSHRN")||
      kw(&L->cur,"SLSHRN")||kw(&L->cur,"SSHRIMM")){
    /* SSHRN n — logical TOS >>= n (n clamped 0..63) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long v = (long)((unsigned long)vm->stack[vm->sp - 1] >> (unsigned)n);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack immediate rotate + arithmetic shift (complete after SSHLN/SSHRN) */
  if (kw(&L->cur,"SROLN")||kw(&L->cur,"ROLN")||kw(&L->cur,"STACKROLN")||
      kw(&L->cur,"SROTLN")||kw(&L->cur,"SROLIMM")){
    /* SROLN n — rotate left TOS by n mod 64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    unsigned uk = (unsigned)(n & 63);
    unsigned long u = (unsigned long)vm->stack[vm->sp - 1];
    long v = (uk == 0) ? (long)u : (long)((u << uk) | (u >> (64u - uk)));
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORN")||kw(&L->cur,"RORN")||kw(&L->cur,"STACKRORN")||
      kw(&L->cur,"SROTRN")||kw(&L->cur,"SRORIMM")){
    /* SRORN n — rotate right TOS by n mod 64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    unsigned uk = (unsigned)(n & 63);
    unsigned long u = (unsigned long)vm->stack[vm->sp - 1];
    long v = (uk == 0) ? (long)u : (long)((u >> uk) | (u << (64u - uk)));
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSARN")||kw(&L->cur,"SARN")||kw(&L->cur,"SASHRN")||
      kw(&L->cur,"STACKSARN")||kw(&L->cur,"SSARIMM")||kw(&L->cur,"ASHRN")){
    /* SSARN n — arithmetic (sign-preserving) TOS >>= n; n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long v = vm->stack[vm->sp - 1] >> n;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack compare + min/max: predicate 0/1 or ordered select */
  if (kw(&L->cur,"SMIN")||kw(&L->cur,"SMAX")||
      kw(&L->cur,"STACKMIN")||kw(&L->cur,"STACKMAX")||
      kw(&L->cur,"SEQ")||kw(&L->cur,"SNE")||kw(&L->cur,"SLT")||
      kw(&L->cur,"SGT")||kw(&L->cur,"SLE")||kw(&L->cur,"SGE")||
      kw(&L->cur,"STACKEQ")||kw(&L->cur,"STACKNE")||kw(&L->cur,"STACKLT")||
      kw(&L->cur,"STACKGT")||kw(&L->cur,"STACKLE")||kw(&L->cur,"STACKGE")||
      kw(&L->cur,"CMPEQ")||kw(&L->cur,"CMPNE")||kw(&L->cur,"CMPLT")||
      kw(&L->cur,"CMPGT")||kw(&L->cur,"CMPLE")||kw(&L->cur,"CMPGE")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (strcmp(op,"SMIN")==0 || strcmp(op,"STACKMIN")==0) r = a < b ? a : b;
    else if (strcmp(op,"SMAX")==0 || strcmp(op,"STACKMAX")==0) r = a > b ? a : b;
    else if (strcmp(op,"SEQ")==0 || strcmp(op,"STACKEQ")==0 || strcmp(op,"CMPEQ")==0)
      r = (a == b) ? 1 : 0;
    else if (strcmp(op,"SNE")==0 || strcmp(op,"STACKNE")==0 || strcmp(op,"CMPNE")==0)
      r = (a != b) ? 1 : 0;
    else if (strcmp(op,"SLT")==0 || strcmp(op,"STACKLT")==0 || strcmp(op,"CMPLT")==0)
      r = (a < b) ? 1 : 0;
    else if (strcmp(op,"SGT")==0 || strcmp(op,"STACKGT")==0 || strcmp(op,"CMPGT")==0)
      r = (a > b) ? 1 : 0;
    else if (strcmp(op,"SLE")==0 || strcmp(op,"STACKLE")==0 || strcmp(op,"CMPLE")==0)
      r = (a <= b) ? 1 : 0;
    else if (strcmp(op,"SGE")==0 || strcmp(op,"STACKGE")==0 || strcmp(op,"CMPGE")==0)
      r = (a >= b) ? 1 : 0;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack immediate compare + min/max-with-constant */
  if (kw(&L->cur,"SEQN")||kw(&L->cur,"EQN")||kw(&L->cur,"CMPEQN")||
      kw(&L->cur,"STACKEQN")||kw(&L->cur,"SEQIMM")){
    /* SEQN n — TOS = (TOS == n) ? 1 : 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = (vm->stack[vm->sp - 1] == n) ? 1 : 0;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNEN")||kw(&L->cur,"NEN")||kw(&L->cur,"CMPNEN")||
      kw(&L->cur,"STACKNEN")||kw(&L->cur,"SNEIMM")){
    /* SNEN n — TOS = (TOS != n) ? 1 : 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = (vm->stack[vm->sp - 1] != n) ? 1 : 0;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLTN")||kw(&L->cur,"LTN")||kw(&L->cur,"CMPLTN")||
      kw(&L->cur,"STACKLTN")||kw(&L->cur,"SLTIMM")){
    /* SLTN n — TOS = (TOS < n) ? 1 : 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = (vm->stack[vm->sp - 1] < n) ? 1 : 0;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGTN")||kw(&L->cur,"GTN")||kw(&L->cur,"CMPGTN")||
      kw(&L->cur,"STACKGTN")||kw(&L->cur,"SGTIMM")){
    /* SGTN n — TOS = (TOS > n) ? 1 : 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = (vm->stack[vm->sp - 1] > n) ? 1 : 0;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLENN")||kw(&L->cur,"LENN")||kw(&L->cur,"CMPLENN")||
      kw(&L->cur,"STACKLENN")||kw(&L->cur,"SLEIMM")||kw(&L->cur,"SLEQN")){
    /* SLENN n — TOS = (TOS <= n) ? 1 : 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = (vm->stack[vm->sp - 1] <= n) ? 1 : 0;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGENN")||kw(&L->cur,"GENN")||kw(&L->cur,"CMPGENN")||
      kw(&L->cur,"STACKGENN")||kw(&L->cur,"SGEIMM")||kw(&L->cur,"SGEQN")){
    /* SGENN n — TOS = (TOS >= n) ? 1 : 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = (vm->stack[vm->sp - 1] >= n) ? 1 : 0;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack imm unsigned compare: SULTN · SUGTN · SULEN · SUGEN (imm dual of SULT plane) */
  if (kw(&L->cur,"SULTN")||kw(&L->cur,"ULTN")||kw(&L->cur,"STACKULTN")||
      kw(&L->cur,"SULTIMM")||kw(&L->cur,"CMPULTN")){
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long un = (unsigned long)n;
    long v = (ua < un) ? 1 : 0;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUGTN")||kw(&L->cur,"UGTN")||kw(&L->cur,"STACKUGTN")||
      kw(&L->cur,"SUGTIMM")||kw(&L->cur,"CMPUGTN")){
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long un = (unsigned long)n;
    long v = (ua > un) ? 1 : 0;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SULEN")||kw(&L->cur,"ULEN")||kw(&L->cur,"STACKULEN")||
      kw(&L->cur,"SULEIMM")||kw(&L->cur,"SULEQN")||kw(&L->cur,"CMPULEN")){
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long un = (unsigned long)n;
    long v = (ua <= un) ? 1 : 0;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUGEN")||kw(&L->cur,"UGEN")||kw(&L->cur,"STACKUGEN")||
      kw(&L->cur,"SUGEIMM")||kw(&L->cur,"SUGEQN")||kw(&L->cur,"CMPUGEN")){
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long un = (unsigned long)n;
    long v = (ua >= un) ? 1 : 0;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMINN")||kw(&L->cur,"MINN")||kw(&L->cur,"STACKMINN")||
      kw(&L->cur,"SMINIMM")){
    /* SMINN n — TOS = min(TOS, n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long v = a < n ? a : n;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMAXN")||kw(&L->cur,"MAXN")||kw(&L->cur,"STACKMAXN")||
      kw(&L->cur,"SMAXIMM")){
    /* SMAXN n — TOS = max(TOS, n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long v = a > n ? a : n;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm unsigned min/max: SUMINN · SUMAXN (imm dual of SMINN/SMAXN for unsigned) */
  if (kw(&L->cur,"SUMINN")||kw(&L->cur,"UMINN")||kw(&L->cur,"STACKUMINN")||
      kw(&L->cur,"SUMINIMM")||kw(&L->cur,"UMINIMM")){
    /* SUMINN n — TOS = unsigned min(TOS, n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long un = (unsigned long)n;
    long v = (long)(ua < un ? ua : un);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMAXN")||kw(&L->cur,"UMAXN")||kw(&L->cur,"STACKUMAXN")||
      kw(&L->cur,"SUMAXIMM")||kw(&L->cur,"UMAXIMM")){
    /* SUMAXN n — TOS = unsigned max(TOS, n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long un = (unsigned long)n;
    long v = (long)(ua > un ? ua : un);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack select/within/clamp + zero-tests/sign; digit-6 adds S0LE/S0GE */
  if (kw(&L->cur,"SZ")||kw(&L->cur,"S0EQ")||kw(&L->cur,"STACK0EQ")||kw(&L->cur,"S0=")||
      kw(&L->cur,"SNZ")||kw(&L->cur,"S0NE")||kw(&L->cur,"STACK0NE")||kw(&L->cur,"S0<>")||
      kw(&L->cur,"S0LT")||kw(&L->cur,"STACK0LT")||kw(&L->cur,"S0<")||
      kw(&L->cur,"S0GT")||kw(&L->cur,"STACK0GT")||kw(&L->cur,"S0>")||
      kw(&L->cur,"S0LE")||kw(&L->cur,"STACK0LE")||kw(&L->cur,"S0<=")||
      kw(&L->cur,"SNONPOS")||kw(&L->cur,"S0LEQ")||
      kw(&L->cur,"S0GE")||kw(&L->cur,"STACK0GE")||kw(&L->cur,"S0>=")||
      kw(&L->cur,"SNONNEG")||kw(&L->cur,"S0GEQ")||
      kw(&L->cur,"SSIGN")||kw(&L->cur,"STACKSIGN")||kw(&L->cur,"SGN")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SZ")==0 || strcmp(op,"S0EQ")==0 || strcmp(op,"STACK0EQ")==0 || strcmp(op,"S0=")==0)
      r = (a == 0) ? 1 : 0;
    else if (strcmp(op,"SNZ")==0 || strcmp(op,"S0NE")==0 || strcmp(op,"STACK0NE")==0 || strcmp(op,"S0<>")==0)
      r = (a != 0) ? 1 : 0;
    else if (strcmp(op,"S0LT")==0 || strcmp(op,"STACK0LT")==0 || strcmp(op,"S0<")==0)
      r = (a < 0) ? 1 : 0;
    else if (strcmp(op,"S0GT")==0 || strcmp(op,"STACK0GT")==0 || strcmp(op,"S0>")==0)
      r = (a > 0) ? 1 : 0;
    else if (strcmp(op,"S0LE")==0 || strcmp(op,"STACK0LE")==0 || strcmp(op,"S0<=")==0 ||
             strcmp(op,"SNONPOS")==0 || strcmp(op,"S0LEQ")==0)
      r = (a <= 0) ? 1 : 0;
    else if (strcmp(op,"S0GE")==0 || strcmp(op,"STACK0GE")==0 || strcmp(op,"S0>=")==0 ||
             strcmp(op,"SNONNEG")==0 || strcmp(op,"S0GEQ")==0)
      r = (a >= 0) ? 1 : 0;
    else {
      /* SSIGN / STACKSIGN / SGN */
      r = (a > 0) ? 1 : ((a < 0) ? -1 : 0);
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEL")||kw(&L->cur,"STACKSEL")||kw(&L->cur,"SSELECT")||kw(&L->cur,"STACKSELECT")){
    /* f t cond → (cond ? t : f)  — cond on TOS */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long cond = vm->stack[--vm->sp];
    long t = vm->stack[--vm->sp];
    long f = vm->stack[--vm->sp];
    long r = cond ? t : f;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SWITHIN")||kw(&L->cur,"WITHIN")||kw(&L->cur,"STACKWITHIN")){
    /* n lo hi → 1 if lo <= n < hi else 0 (Forth WITHIN) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    long n = vm->stack[--vm->sp];
    long r = (n >= lo && n < hi) ? 1 : 0;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLAMP")||kw(&L->cur,"STACKCLAMP")||kw(&L->cur,"SCLMP")){
    /* n lo hi → clamp n into [lo,hi] (if lo>hi swap) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    long n = vm->stack[--vm->sp];
    if (lo > hi){ long t=lo; lo=hi; hi=t; }
    long r = n;
    if (r < lo) r = lo;
    if (r > hi) r = hi;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack immediate clamp/range: SCLAMPN · SBETWEENN · SWITHINN (imm dual of SCLAMP/SBETWEEN/SWITHIN; pair of DCLAMPN) */
  if (kw(&L->cur,"SCLAMPN")||kw(&L->cur,"STACKCLAMPN")||kw(&L->cur,"CLAMPN")||
      kw(&L->cur,"SCLAMPIMM")||kw(&L->cur,"SBOUNDN")||kw(&L->cur,"BOUNDN")){
    /* SCLAMPN lo hi — TOS = clamp(TOS,[lo,hi]); swap lo/hi if needed */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (lo > hi){ long t=lo; lo=hi; hi=t; }
    long r = vm->stack[vm->sp - 1];
    if (r < lo) r = lo;
    if (r > hi) r = hi;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBETWEENN")||kw(&L->cur,"SINRANGEN")||kw(&L->cur,"STACKBETWEENN")||
      kw(&L->cur,"BETWEENN")||kw(&L->cur,"INRANGEN")||kw(&L->cur,"SINRANGEIMM")){
    /* SBETWEENN lo hi — TOS = 1 if TOS in [lo,hi] inclusive; swap lo/hi if needed */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (lo > hi){ long t=lo; lo=hi; hi=t; }
    long n = vm->stack[vm->sp - 1];
    long r = (n >= lo && n <= hi) ? 1 : 0;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SWITHINN")||kw(&L->cur,"STACKWITHINN")||kw(&L->cur,"WITHINN")||
      kw(&L->cur,"SWITHINIMM")||kw(&L->cur,"SINTERVALN")){
    /* SWITHINN lo hi — TOS = 1 if lo <= TOS < hi (Forth WITHIN); swap not applied (hi exclusive) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[vm->sp - 1];
    long r = (n >= lo && n < hi) ? 1 : 0;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 select/clamp stack: SBETWEEN SINRANGE SMEDIAN */
  if (kw(&L->cur,"SBETWEEN")||kw(&L->cur,"SINRANGE")||kw(&L->cur,"STACKBETWEEN")||
      kw(&L->cur,"STACKINRANGE")){
    /* n lo hi → 1 if n in [lo,hi] inclusive (swap lo/hi if needed) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    long n = vm->stack[--vm->sp];
    if (lo > hi){ long t=lo; lo=hi; hi=t; }
    long r = (n >= lo && n <= hi) ? 1 : 0;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMEDIAN")||kw(&L->cur,"SMID3")||kw(&L->cur,"STACKMEDIAN")){
    /* a b c → median of three */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long z = vm->stack[--vm->sp];
    long y = vm->stack[--vm->sp];
    long x = vm->stack[--vm->sp];
    if (x > y){ long t=x; x=y; y=t; }
    if (y > z){ long t=y; y=z; z=t; }
    if (x > y){ long t=x; x=y; y=t; }
    long r = y;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1/3 clamp/bound stack: SSATADD SSATSUB SSATMUL SSATDIV SWMOD SCLIP8 SCLIP16 SBOUND */
  if (kw(&L->cur,"SSATADD")||kw(&L->cur,"SSATSUB")||kw(&L->cur,"SSATMUL")||
      kw(&L->cur,"SSATDIV")||kw(&L->cur,"STACKSATDIV")||kw(&L->cur,"SATDIVST")||
      kw(&L->cur,"STACKSATADD")||kw(&L->cur,"STACKSATSUB")||kw(&L->cur,"STACKSATMUL")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    int is_add = (strcmp(op,"SSATADD")==0 || strcmp(op,"STACKSATADD")==0);
    int is_sub = (strcmp(op,"SSATSUB")==0 || strcmp(op,"STACKSATSUB")==0);
    int is_div = (strcmp(op,"SSATDIV")==0 || strcmp(op,"STACKSATDIV")==0 ||
                  strcmp(op,"SATDIVST")==0);
    if (is_add){
      if (b > 0 && a > LONG_MAX - b) r = LONG_MAX;
      else if (b < 0 && a < LONG_MIN - b) r = LONG_MIN;
      else r = a + b;
    } else if (is_sub){
      if (b > 0 && a < LONG_MIN + b) r = LONG_MIN;
      else if (b < 0 && a > LONG_MAX + b) r = LONG_MAX;
      else r = a - b;
    } else if (is_div){
      /* SSATDIV — trunc-toward-zero; /0 → 0; LONG_MIN/-1 → LONG_MAX */
      if (b == 0) r = 0;
      else if (a == LONG_MIN && b == -1) r = LONG_MAX;
      else r = a / b;
    } else {
      /* SSATMUL / STACKSATMUL */
      if (a == 0 || b == 0) r = 0;
      else {
        __int128 p = (__int128)a * (__int128)b;
        if (p > (__int128)LONG_MAX) r = LONG_MAX;
        else if (p < (__int128)LONG_MIN) r = LONG_MIN;
        else r = (long)p;
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack shared-divisor sat div: DSATDIVN already imm; stack form SSATDIV completes +/−/* plane */
  /* digit-1 stack imm saturating ALU: SSATADDN · SSATSUBN · SSATMULN (imm dual of SSATADD plane) */
  if (kw(&L->cur,"SSATADDN")||kw(&L->cur,"STACKSATADDN")||kw(&L->cur,"SATADDN")||
      kw(&L->cur,"SSATADDIMM")||kw(&L->cur,"SADDSATN")){
    /* SSATADDN n — TOS = sat(TOS + n) to LONG_MIN..LONG_MAX */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (n > 0 && a > LONG_MAX - n) r = LONG_MAX;
    else if (n < 0 && a < LONG_MIN - n) r = LONG_MIN;
    else r = a + n;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATSUBN")||kw(&L->cur,"STACKSATSUBN")||kw(&L->cur,"SATSUBN")||
      kw(&L->cur,"SSATSUBIMM")||kw(&L->cur,"SSUBSATN")){
    /* SSATSUBN n — TOS = sat(TOS - n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (n > 0 && a < LONG_MIN + n) r = LONG_MIN;
    else if (n < 0 && a > LONG_MAX + n) r = LONG_MAX;
    else r = a - n;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATMULN")||kw(&L->cur,"STACKSATMULN")||kw(&L->cur,"SATMULN")||
      kw(&L->cur,"SSATMULIMM")||kw(&L->cur,"SMULSATN")){
    /* SSATMULN n — TOS = sat(TOS * n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (a == 0 || n == 0) r = 0;
    else {
      __int128 p = (__int128)a * (__int128)n;
      if (p > (__int128)LONG_MAX) r = LONG_MAX;
      else if (p < (__int128)LONG_MIN) r = LONG_MIN;
      else r = (long)p;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack imm sat div: SSATDIVN (completes sat imm ALU plane) */
  if (kw(&L->cur,"SSATDIVN")||kw(&L->cur,"STACKSATDIVN")||kw(&L->cur,"SATDIVN")||
      kw(&L->cur,"SSATDIVIMM")||kw(&L->cur,"SDIVSATN")){
    /* SSATDIVN n — TOS = sat(TOS / n); n==0 → 0; LONG_MIN/-1 → LONG_MAX */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (n == 0) r = 0;
    else if (a == LONG_MIN && n == -1) r = LONG_MAX;
    else r = a / n;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack imm reverse sat ALU: SSATSUBFROMN · SSATDIVFROMN (sat dual of SSUBFROMN/SDIVFROMN) */
  if (kw(&L->cur,"SSATSUBFROMN")||kw(&L->cur,"STACKSATSUBFROMN")||kw(&L->cur,"SATSUBFROMN")||
      kw(&L->cur,"SSRSUBN")||kw(&L->cur,"SSATRSUBN")||kw(&L->cur,"SSATSUBFROMIMM")||
      kw(&L->cur,"NSATSUBN")){
    /* SSATSUBFROMN n — TOS = sat(n - TOS) to LONG_MIN..LONG_MAX */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (a > 0 && n < LONG_MIN + a) r = LONG_MIN;
    else if (a < 0 && n > LONG_MAX + a) r = LONG_MAX;
    else r = n - a;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATDIVFROMN")||kw(&L->cur,"STACKSATDIVFROMN")||kw(&L->cur,"SATDIVFROMN")||
      kw(&L->cur,"SSRDIVN")||kw(&L->cur,"SSATRDIVN")||kw(&L->cur,"SSATDIVFROMIMM")||
      kw(&L->cur,"NSATDIVN")){
    /* SSATDIVFROMN n — TOS = sat(n / TOS); TOS==0 → 0; LONG_MIN/-1 → LONG_MAX */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (a == 0) r = 0;
    else if (n == LONG_MIN && a == -1) r = LONG_MAX;
    else r = n / a;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SWMOD")||kw(&L->cur,"SWRAPMOD")||kw(&L->cur,"STACKWRAPMOD")){
    /* n m → n mod m in [0,m); m<=0 → 0 */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long n = vm->stack[--vm->sp];
    long r = 0;
    if (m > 0){ r = n % m; if (r < 0) r += m; }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack imm energy wrap: SWMODN · SWRAPN (imm dual of SWMOD) */
  if (kw(&L->cur,"SWMODN")||kw(&L->cur,"SWRAPN")||kw(&L->cur,"STACKWRAPN")||
      kw(&L->cur,"SWRAPMODN")||kw(&L->cur,"SWMODIMM")||kw(&L->cur,"SWRAPIMM")||
      kw(&L->cur,"WRAPN")||kw(&L->cur,"WMODN")){
    /* SWMODN m — TOS = wrap(TOS, m) in [0,m); m<=0 → 0 */
    lex_next(L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[vm->sp - 1];
    long r = 0;
    if (m > 0){ r = n % m; if (r < 0) r += m; }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIP8")||kw(&L->cur,"SCLIP16")||
      kw(&L->cur,"STACKCLIP8")||kw(&L->cur,"STACKCLIP16")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = a;
    int is16 = (strcmp(op,"SCLIP16")==0 || strcmp(op,"STACKCLIP16")==0);
    long hi = is16 ? 65535L : 255L;
    if (r < 0) r = 0;
    if (r > hi) r = hi;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack signed clip word path: SCLIPS4 · SCLIPS8 · SCLIPS16
   * (signed dual of SCLIP8/SCLIP16; complete stack clamp 4/8/16 plane) */
  if (kw(&L->cur,"SCLIPS4")||kw(&L->cur,"CLIPS4")||kw(&L->cur,"SSCLIP4")||
      kw(&L->cur,"STACKSCLIPS4")||kw(&L->cur,"SCLIPSN")||kw(&L->cur,"STACKCLIPS4")){
    /* TOS = clamp to signed 4-bit [-8,7] */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long r = vm->stack[vm->sp - 1];
    if (r < -8L) r = -8L;
    if (r > 7L) r = 7L;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS8")||kw(&L->cur,"CLIPS8")||kw(&L->cur,"SSCLIP8")||
      kw(&L->cur,"STACKSCLIPS8")||kw(&L->cur,"SCLIPSB")||kw(&L->cur,"STACKCLIPS8")){
    /* TOS = clamp to signed 8-bit [-128,127] */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long r = vm->stack[vm->sp - 1];
    if (r < -128L) r = -128L;
    if (r > 127L) r = 127L;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS16")||kw(&L->cur,"CLIPS16")||kw(&L->cur,"SSCLIP16")||
      kw(&L->cur,"STACKSCLIPS16")||kw(&L->cur,"SCLIPSW")||kw(&L->cur,"STACKCLIPS16")){
    /* TOS = clamp to signed 16-bit [-32768,32767] */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long r = vm->stack[vm->sp - 1];
    if (r < -32768L) r = -32768L;
    if (r > 32767L) r = 32767L;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack clip complete 4/32 u+s: SCLIP4 · SCLIP32 · SCLIPS32
   * (complete unsigned stack clip 4/8/16/32 with prior SCLIP8/16; signed 32 dual of SCLIPS4/8/16) */
  if (kw(&L->cur,"SCLIP4")||kw(&L->cur,"CLIP4")||kw(&L->cur,"UCLIP4")||
      kw(&L->cur,"STACKCLIP4")||kw(&L->cur,"SCLIPN")||kw(&L->cur,"UCLIPN")){
    /* TOS = clamp to unsigned nibble [0,15] */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long r = vm->stack[vm->sp - 1];
    if (r < 0) r = 0;
    if (r > 15) r = 15;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIP32")||kw(&L->cur,"CLIP32")||kw(&L->cur,"UCLIP32")||
      kw(&L->cur,"STACKCLIP32")||kw(&L->cur,"SCLIPL")||kw(&L->cur,"SCLIPD")||
      kw(&L->cur,"UCLIPL")){
    /* TOS = clamp to unsigned 32-bit [0,0xFFFFFFFF] */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long r = vm->stack[vm->sp - 1];
    if (r < 0) r = 0;
    if ((unsigned long)r > 0xFFFFFFFFul) r = (long)0xFFFFFFFFul;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS32")||kw(&L->cur,"CLIPS32")||kw(&L->cur,"SSCLIP32")||
      kw(&L->cur,"STACKSCLIPS32")||kw(&L->cur,"SCLIPSL")||kw(&L->cur,"SCLIPSD")||
      kw(&L->cur,"STACKCLIPS32")){
    /* TOS = clamp to signed 32-bit [INT32_MIN,INT32_MAX] */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long r = vm->stack[vm->sp - 1];
    if (r < (-2147483647L - 1)) r = (-2147483647L - 1);
    if (r > 2147483647L) r = 2147483647L;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBOUND")||kw(&L->cur,"STACKBOUND")){
    /* alias of SCLAMP: n lo hi → clamp */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    long n = vm->stack[--vm->sp];
    if (lo > hi){ long t=lo; lo=hi; hi=t; }
    long r = n;
    if (r < lo) r = lo;
    if (r > hi) r = hi;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 bitwise logic stack: SNAND SNOR SXNOR SANDN SORN SBSEL */
  if (kw(&L->cur,"SNAND")||kw(&L->cur,"STACKNAND")||
      kw(&L->cur,"SNOR")||kw(&L->cur,"STACKNOR")||
      kw(&L->cur,"SXNOR")||kw(&L->cur,"STACKXNOR")||kw(&L->cur,"SXNORB")||
      kw(&L->cur,"SANDN")||kw(&L->cur,"SANDNOT")||kw(&L->cur,"SBIC")||kw(&L->cur,"STACKANDN")||
      kw(&L->cur,"SORN")||kw(&L->cur,"SORNOT")||kw(&L->cur,"STACKORN")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (strcmp(op,"SNAND")==0 || strcmp(op,"STACKNAND")==0) r = ~(a & b);
    else if (strcmp(op,"SNOR")==0 || strcmp(op,"STACKNOR")==0) r = ~(a | b);
    else if (strcmp(op,"SXNOR")==0 || strcmp(op,"STACKXNOR")==0 || strcmp(op,"SXNORB")==0)
      r = ~(a ^ b);
    else if (strcmp(op,"SANDN")==0 || strcmp(op,"SANDNOT")==0 || strcmp(op,"SBIC")==0 ||
             strcmp(op,"STACKANDN")==0)
      r = a & ~b;
    else
      r = a | ~b; /* SORN / SORNOT / STACKORN */
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBSEL")||kw(&L->cur,"SBITSEL")||kw(&L->cur,"SBLEND")||kw(&L->cur,"STACKBSEL")){
    /* mask a b → (a & mask) | (b & ~mask)  — mask on bottom of trio */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long m = vm->stack[--vm->sp];
    unsigned long um = (unsigned long)m;
    long r = (long)(((unsigned long)a & um) | ((unsigned long)b & ~um));
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 multiword stack: SADDC SSUBB SDIVMOD (cin/bin from CARRY/BORROW) */
  if (kw(&L->cur,"SADDC")||kw(&L->cur,"SADC")||kw(&L->cur,"STACKADDC")||
      kw(&L->cur,"SSUBB")||kw(&L->cur,"SSBB")||kw(&L->cur,"STACKSUBB")){
    /* SADDC: a b → sum  using CARRY as cin, then write new CARRY
       SSUBB: a b → diff using BORROW (or CARRY) as bin */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    int is_sub = (strcmp(op,"SSUBB")==0 || strcmp(op,"SSBB")==0 ||
                  strcmp(op,"STACKSUBB")==0);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long cin = 0;
    {
      Var *vc = var_get(vm, is_sub ? "BORROW" : "CARRY", 0);
      if (!vc && is_sub) vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    unsigned long ua = (unsigned long)a;
    unsigned long ub = (unsigned long)b;
    unsigned long uc = (unsigned long)cin;
    long r;
    if (!is_sub){
      unsigned long s = ua + ub;
      int c1 = (s < ua) ? 1 : 0;
      unsigned long sum = s + uc;
      int c2 = (sum < s) ? 1 : 0;
      int carry = c1 | c2;
      r = (long)sum;
      var_set_num(vm,"CARRY",carry); var_set_num(vm,"CY",carry);
    } else {
      int b1 = (ua < ub) ? 1 : 0;
      unsigned long d = ua - ub;
      int b2 = (d < uc) ? 1 : 0;
      unsigned long diff = d - uc;
      int borrow = b1 | b2;
      r = (long)diff;
      var_set_num(vm,"BORROW",borrow); var_set_num(vm,"BW",borrow);
      var_set_num(vm,"CARRY",borrow);
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack multiword shift-through-CARRY: SSHLC · SSHRC (cin from CARRY; dual of DSHLCC) */
  if (kw(&L->cur,"SSHLC")||kw(&L->cur,"STACKSHLC")||kw(&L->cur,"SHLCY")||
      kw(&L->cur,"SSHLCF")||kw(&L->cur,"SHLCF")||kw(&L->cur,"SHLCST")){
    /* SSHLC — TOS = (TOS<<1)|cin(CARRY); CARRY = old MSB */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long cin = 0;
    {
      Var *vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    unsigned long ua = (unsigned long)a;
    unsigned long msb = 1ul << (sizeof(unsigned long) * 8 - 1);
    int cout = (ua & msb) ? 1 : 0;
    long r = (long)((ua << 1) | (cin ? 1ul : 0ul));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"CARRY",cout); var_set_num(vm,"CY",cout);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHRC")||kw(&L->cur,"STACKSHRC")||kw(&L->cur,"SHRCY")||
      kw(&L->cur,"SSHRCF")||kw(&L->cur,"SHRCF")||kw(&L->cur,"SHRCST")){
    /* SSHRC — TOS = (TOS>>1)|(cin<<MSB); cin=CARRY; CARRY = old LSB */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long cin = 0;
    {
      Var *vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    unsigned long ua = (unsigned long)a;
    unsigned long msb = 1ul << (sizeof(unsigned long) * 8 - 1);
    int cout = (ua & 1ul) ? 1 : 0;
    long r = (long)((ua >> 1) | (cin ? msb : 0ul));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"CARRY",cout); var_set_num(vm,"CY",cout);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack multiword imm rotate-through-CARRY: SSHLCN · SSHRCN (imm dual of SSHLC/SSHRC) */
  if (kw(&L->cur,"SSHLCN")||kw(&L->cur,"STACKSHLCN")||kw(&L->cur,"SHLCYN")||
      kw(&L->cur,"SSHLCFN")||kw(&L->cur,"SHLCFN")||kw(&L->cur,"SHLCIMM")||
      kw(&L->cur,"SRCLN")||kw(&L->cur,"RCLN")){
    /* SSHLCN n — rotate-through-carry left n times (n clamped 0..64); cin/cout via CARRY */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long msb = 1ul << (sizeof(unsigned long) * 8 - 1);
    long cin = 0;
    {
      Var *vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    for (long i = 0; i < n; i++){
      int cout = (ua & msb) ? 1 : 0;
      ua = (ua << 1) | (cin ? 1ul : 0ul);
      cin = cout;
    }
    long r = (long)ua;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"CARRY",cin); var_set_num(vm,"CY",cin);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHRCN")||kw(&L->cur,"STACKSHRCN")||kw(&L->cur,"SHRCYN")||
      kw(&L->cur,"SSHRCFN")||kw(&L->cur,"SHRCFN")||kw(&L->cur,"SHRCIMM")||
      kw(&L->cur,"SRCRN")||kw(&L->cur,"RCRN")){
    /* SSHRCN n — rotate-through-carry right n times (n clamped 0..64) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long msb = 1ul << (sizeof(unsigned long) * 8 - 1);
    long cin = 0;
    {
      Var *vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    for (long i = 0; i < n; i++){
      int cout = (ua & 1ul) ? 1 : 0;
      ua = (ua >> 1) | (cin ? msb : 0ul);
      cin = cout;
    }
    long r = (long)ua;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"CARRY",cin); var_set_num(vm,"CY",cin);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack multiword negate-via-complement+cin: SNEGC (stack dual of DNEGC2 path) */
  if (kw(&L->cur,"SNEGC")||kw(&L->cur,"SCOMADC")||kw(&L->cur,"SNEGADC")||
      kw(&L->cur,"STACKNEGC")||kw(&L->cur,"SNEGCST")||kw(&L->cur,"SNOTADC")){
    /* SNEGC — TOS = (~TOS) + cin(CARRY); CARRY = cout (unsigned wrap) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long cin = 0;
    {
      Var *vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    unsigned long ta = ~(unsigned long)a;
    unsigned long uin = cin ? 1ul : 0ul;
    unsigned long sa = ta + uin;
    int cout = (uin && sa < ta) ? 1 : 0;
    long r = (long)sa;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"CARRY",cout); var_set_num(vm,"CY",cout);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm cin negate: SNEGCN n — cin from n (0/1); dual of shared-CARRY form */
  if (kw(&L->cur,"SNEGCN")||kw(&L->cur,"SCOMADCN")||kw(&L->cur,"SNEGADCIMM")||
      kw(&L->cur,"STACKNEGCN")||kw(&L->cur,"SNEGCIMM")||kw(&L->cur,"NEGCN")){
    /* SNEGCN n — TOS = (~TOS) + (n?1:0); CARRY = cout */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    unsigned long ta = ~(unsigned long)a;
    unsigned long uin = n ? 1ul : 0ul;
    unsigned long sa = ta + uin;
    int cout = (uin && sa < ta) ? 1 : 0;
    long r = (long)sa;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"CARRY",cout); var_set_num(vm,"CY",cout);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 multiword imm: SADDCN · SSUBBN (imm dual of SADDC/SSUBB) */
  if (kw(&L->cur,"SADDCN")||kw(&L->cur,"STACKADDCN")||
      kw(&L->cur,"ADDCN")||kw(&L->cur,"SADCIMM")||kw(&L->cur,"SADDCIMM")||
      kw(&L->cur,"SADCIN")){
    /* SADDCN n — TOS = TOS + n + cin(CARRY); update CARRY/CY */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long cin = 0;
    {
      Var *vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    unsigned long ua = (unsigned long)a;
    unsigned long ub = (unsigned long)n;
    unsigned long uc = (unsigned long)cin;
    unsigned long s = ua + ub;
    int c1 = (s < ua) ? 1 : 0;
    unsigned long sum = s + uc;
    int c2 = (sum < s) ? 1 : 0;
    int carry = c1 | c2;
    long r = (long)sum;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"CARRY",carry); var_set_num(vm,"CY",carry);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUBBN")||kw(&L->cur,"SSBBN")||kw(&L->cur,"STACKSUBBN")||
      kw(&L->cur,"SUBBN")||kw(&L->cur,"SSBBIMM")||kw(&L->cur,"SSUBBIMM")||
      kw(&L->cur,"SSUBBIN")){
    /* SSUBBN n — TOS = TOS - n - bin(BORROW|CARRY); update BORROW/BW/CARRY */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long cin = 0;
    {
      Var *vc = var_get(vm, "BORROW", 0);
      if (!vc) vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    unsigned long ua = (unsigned long)a;
    unsigned long ub = (unsigned long)n;
    unsigned long uc = (unsigned long)cin;
    int b1 = (ua < ub) ? 1 : 0;
    unsigned long d = ua - ub;
    int b2 = (d < uc) ? 1 : 0;
    unsigned long diff = d - uc;
    int borrow = b1 | b2;
    long r = (long)diff;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"BORROW",borrow); var_set_num(vm,"BW",borrow);
    var_set_num(vm,"CARRY",borrow);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDIVMOD")||kw(&L->cur,"SDIVREM")||kw(&L->cur,"STACKDIVMOD")){
    /* a b → rem quot  (TOS=quot, under=rem); sets QUOT REM */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (b == 0){
      var_set_num(vm,"QUOT",0); var_set_num(vm,"REM",0);
      var_set_num(vm,"OK",0);
      vm->stack[vm->sp++] = 0;
      vm->stack[vm->sp++] = 0;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",0); vm->last_n=0;
      bump(vm); return 1;
    }
    long q = a / b;
    long r = a % b;
    var_set_num(vm,"QUOT",q); var_set_num(vm,"REM",r);
    vm->stack[vm->sp++] = r;
    vm->stack[vm->sp++] = q;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",q); vm->last_n=q;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0/2 muldiv + byteswap stack: SUDIV SUMOD SMULHI SUMULHI SBSWAP16 SBSWAP64 */
  if (kw(&L->cur,"SUDIV")||kw(&L->cur,"SUDIVIDE")||kw(&L->cur,"STACKUDIV")||
      kw(&L->cur,"SUMOD")||kw(&L->cur,"SUREM")||kw(&L->cur,"STACKUMOD")||
      kw(&L->cur,"SMULHI")||kw(&L->cur,"SMULH")||kw(&L->cur,"STACKMULHI")||
      kw(&L->cur,"SUMULHI")||kw(&L->cur,"SUMULH")||kw(&L->cur,"STACKUMULHI")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (strcmp(op,"SUDIV")==0 || strcmp(op,"SUDIVIDE")==0 || strcmp(op,"STACKUDIV")==0){
      if (b != 0) r = (long)((unsigned long)a / (unsigned long)b);
    } else if (strcmp(op,"SUMOD")==0 || strcmp(op,"SUREM")==0 || strcmp(op,"STACKUMOD")==0){
      if (b != 0) r = (long)((unsigned long)a % (unsigned long)b);
    } else if (strcmp(op,"SMULHI")==0 || strcmp(op,"SMULH")==0 || strcmp(op,"STACKMULHI")==0){
      __int128 p = (__int128)a * (__int128)b;
      r = (long)(p >> 64);
    } else {
      unsigned __int128 p =
          (unsigned __int128)(unsigned long)a *
          (unsigned __int128)(unsigned long)b;
      r = (long)(p >> 64);
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBSWAP16")||kw(&L->cur,"STACKBSWAP16")||
      kw(&L->cur,"SBSWAP64")||kw(&L->cur,"STACKBSWAP64")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (strcmp(op,"SBSWAP16")==0 || strcmp(op,"STACKBSWAP16")==0){
      unsigned int w = (unsigned int)a & 0xFFFFu;
      w = ((w & 0x00FFu) << 8) | ((w & 0xFF00u) >> 8);
      r = (long)w;
    } else {
      unsigned long w = (unsigned long)a;
      w = ((w & 0x00000000000000FFul) << 56) | ((w & 0x000000000000FF00ul) << 40) |
          ((w & 0x0000000000FF0000ul) << 24) | ((w & 0x00000000FF000000ul) << 8) |
          ((w & 0x000000FF00000000ul) >> 8) | ((w & 0x0000FF0000000000ul) >> 24) |
          ((w & 0x00FF000000000000ul) >> 40) | ((w & 0xFF00000000000000ul) >> 56);
      r = (long)w;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 rotate/shift-extend stack: SROTL8/16 SSEXT32 SZEXT32 SSHL32 SSHR32 */
  if (kw(&L->cur,"SROTL8")||kw(&L->cur,"SROL8")||kw(&L->cur,"SROTR8")||kw(&L->cur,"SROR8")||
      kw(&L->cur,"SROTL16")||kw(&L->cur,"SROL16")||kw(&L->cur,"SROTR16")||kw(&L->cur,"SROR16")||
      kw(&L->cur,"SSHL32")||kw(&L->cur,"SSHR32")||
      kw(&L->cur,"STACKROTL8")||kw(&L->cur,"STACKROTR8")||
      kw(&L->cur,"STACKROTL16")||kw(&L->cur,"STACKROTR16")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    if (strcmp(op,"SROTL8")==0 || strcmp(op,"SROL8")==0 || strcmp(op,"STACKROTL8")==0){
      unsigned int w = (unsigned int)a & 0xFFu;
      kk &= 7;
      r = kk ? (long)(((w << kk) | (w >> (8 - kk))) & 0xFFu) : (long)w;
    } else if (strcmp(op,"SROTR8")==0 || strcmp(op,"SROR8")==0 || strcmp(op,"STACKROTR8")==0){
      unsigned int w = (unsigned int)a & 0xFFu;
      kk &= 7;
      r = kk ? (long)(((w >> kk) | (w << (8 - kk))) & 0xFFu) : (long)w;
    } else if (strcmp(op,"SROTL16")==0 || strcmp(op,"SROL16")==0 || strcmp(op,"STACKROTL16")==0){
      unsigned int w = (unsigned int)a & 0xFFFFu;
      kk &= 15;
      r = kk ? (long)(((w << kk) | (w >> (16 - kk))) & 0xFFFFu) : (long)w;
    } else if (strcmp(op,"SROTR16")==0 || strcmp(op,"SROR16")==0 || strcmp(op,"STACKROTR16")==0){
      unsigned int w = (unsigned int)a & 0xFFFFu;
      kk &= 15;
      r = kk ? (long)(((w >> kk) | (w << (16 - kk))) & 0xFFFFu) : (long)w;
    } else if (strcmp(op,"SSHL32")==0){
      unsigned int w = (unsigned int)a;
      if (kk >= 32) r = 0;
      else r = (long)(w << kk);
    } else {
      /* SSHR32 */
      unsigned int w = (unsigned int)a;
      if (kk >= 32) r = 0;
      else r = (long)(w >> kk);
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack word path 32: SSAR32 · SROTL32 · SROTR32
   * (arith SHR32 dual of SSHR32; rotate32 dual of SROTL8/16 plane) */
  if (kw(&L->cur,"SSAR32")||kw(&L->cur,"SASHR32")||kw(&L->cur,"STACKSAR32")||
      kw(&L->cur,"SSARL")||kw(&L->cur,"ASHR32")){
    /* a k → arithmetic right shift low 32 of a by k (k>=32 → all sign) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    int kk = (int)k;
    if (kk < 0) kk = 0;
    long va = (long)(int)(unsigned int)a; /* sign-extend low 32 to long */
    long r;
    if (kk >= 32) r = (va < 0) ? -1L : 0L;
    else r = va >> kk;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROTL32")||kw(&L->cur,"SROL32")||kw(&L->cur,"STACKROTL32")||
      kw(&L->cur,"SROTR32")||kw(&L->cur,"SROR32")||kw(&L->cur,"STACKROTR32")){
    /* a k → rotate left/right within low 32 bits */
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *q=op;*q;q++) if (*q>='a'&&*q<='z') *q=(char)(*q-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned int w = (unsigned int)a;
    kk &= 31;
    long r;
    int is_left = (strcmp(op,"SROTL32")==0 || strcmp(op,"SROL32")==0 ||
                   strcmp(op,"STACKROTL32")==0);
    if (kk == 0) r = (long)w;
    else if (is_left) r = (long)(((w << kk) | (w >> (32 - kk))));
    else r = (long)(((w >> kk) | (w << (32 - kk))));
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT32")||kw(&L->cur,"SSEXTL")||kw(&L->cur,"STACKSEXT32")||
      kw(&L->cur,"SZEXT32")||kw(&L->cur,"SZEXTL")||kw(&L->cur,"STACKZEXT32")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (strcmp(op,"SSEXT32")==0 || strcmp(op,"SSEXTL")==0 || strcmp(op,"STACKSEXT32")==0)
      r = (long)(int)(unsigned int)a;
    else
      r = (long)((unsigned long)a & 0xFFFFFFFFul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 boolean logic stack: SLAND SLOR SLXOR SLNOT SIMP SEQZ SNEZ */
  if (kw(&L->cur,"SLAND")||kw(&L->cur,"STACKLAND")||
      kw(&L->cur,"SLOR")||kw(&L->cur,"STACKLOR")||
      kw(&L->cur,"SLXOR")||kw(&L->cur,"STACKLXOR")||
      kw(&L->cur,"SIMP")||kw(&L->cur,"SIMPLY")||kw(&L->cur,"STACKIMPLY")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (strcmp(op,"SLAND")==0 || strcmp(op,"STACKLAND")==0) r = (a && b) ? 1 : 0;
    else if (strcmp(op,"SLOR")==0 || strcmp(op,"STACKLOR")==0) r = (a || b) ? 1 : 0;
    else if (strcmp(op,"SLXOR")==0 || strcmp(op,"STACKLXOR")==0)
      r = ((a != 0) ^ (b != 0)) ? 1 : 0;
    else
      r = (!a || b) ? 1 : 0; /* SIMP / SIMPLY / STACKIMPLY */
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLNOT")||kw(&L->cur,"STACKLNOT")||
      kw(&L->cur,"SEQZ")||kw(&L->cur,"SISZERO")||kw(&L->cur,"STACKEQZ")||
      kw(&L->cur,"SNEZ")||kw(&L->cur,"SISNZ")||kw(&L->cur,"STACKNEZ")||
      kw(&L->cur,"SNONZERO")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (strcmp(op,"SLNOT")==0 || strcmp(op,"STACKLNOT")==0) r = a ? 0 : 1;
    else if (strcmp(op,"SEQZ")==0 || strcmp(op,"SISZERO")==0 || strcmp(op,"STACKEQZ")==0)
      r = a ? 0 : 1;
    else
      r = a ? 1 : 0; /* SNEZ / SISNZ / STACKNEZ / SNONZERO */
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 shift/rotate/cmp stack: SCMP SUCMP SULT SUGT SULE SUGE SROTL64 SROTR64 */
  if (kw(&L->cur,"SCMP")||kw(&L->cur,"SICMP")||kw(&L->cur,"SCMP3")||kw(&L->cur,"STACKCMP")||
      kw(&L->cur,"SUCMP")||kw(&L->cur,"SUCMP3")||kw(&L->cur,"STACKUCMP")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    int is_u = (strcmp(op,"SUCMP")==0 || strcmp(op,"SUCMP3")==0 || strcmp(op,"STACKUCMP")==0);
    if (is_u){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      if (ua < ub) r = -1;
      else if (ua > ub) r = 1;
      else r = 0;
    } else {
      if (a < b) r = -1;
      else if (a > b) r = 1;
      else r = 0;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SULT")||kw(&L->cur,"SUGT")||kw(&L->cur,"SULE")||kw(&L->cur,"SUGE")||
      kw(&L->cur,"STACKULT")||kw(&L->cur,"STACKUGT")||kw(&L->cur,"STACKULE")||kw(&L->cur,"STACKUGE")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    long r = 0;
    if (strcmp(op,"SULT")==0 || strcmp(op,"STACKULT")==0) r = (ua < ub) ? 1 : 0;
    else if (strcmp(op,"SUGT")==0 || strcmp(op,"STACKUGT")==0) r = (ua > ub) ? 1 : 0;
    else if (strcmp(op,"SULE")==0 || strcmp(op,"STACKULE")==0) r = (ua <= ub) ? 1 : 0;
    else r = (ua >= ub) ? 1 : 0;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROTL64")||kw(&L->cur,"SROL64")||kw(&L->cur,"STACKROTL64")||
      kw(&L->cur,"SROTR64")||kw(&L->cur,"SROR64")||kw(&L->cur,"STACKROTR64")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    unsigned long w = (unsigned long)a;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 63;
    long r;
    int is_l = (strcmp(op,"SROTL64")==0 || strcmp(op,"SROL64")==0 || strcmp(op,"STACKROTL64")==0);
    if (kk == 0) r = (long)w;
    else if (is_l) r = (long)((w << kk) | (w >> (64 - kk)));
    else r = (long)((w >> kk) | (w << (64 - kk)));
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 overflow predicates stack: SADDOVF SSUBOVF SMULOVF */
  if (kw(&L->cur,"SADDOVF")||kw(&L->cur,"SADDOVER")||kw(&L->cur,"STACKADDOVF")||
      kw(&L->cur,"SSUBOVF")||kw(&L->cur,"SSUBOVER")||kw(&L->cur,"STACKSUBOVF")||
      kw(&L->cur,"SMULOVF")||kw(&L->cur,"SMULOVER")||kw(&L->cur,"STACKMULOVF")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    int is_add = (strcmp(op,"SADDOVF")==0 || strcmp(op,"SADDOVER")==0 ||
                  strcmp(op,"STACKADDOVF")==0);
    int is_sub = (strcmp(op,"SSUBOVF")==0 || strcmp(op,"SSUBOVER")==0 ||
                  strcmp(op,"STACKSUBOVF")==0);
    if (is_add){
      if (b > 0 && a > LONG_MAX - b) r = 1;
      else if (b < 0 && a < LONG_MIN - b) r = 1;
    } else if (is_sub){
      if (b > 0 && a < LONG_MIN + b) r = 1;
      else if (b < 0 && a > LONG_MAX + b) r = 1;
    } else {
      /* SMULOVF / SMULOVER / STACKMULOVF */
      if (a != 0 && b != 0){
        __int128 p = (__int128)a * (__int128)b;
        if (p > (__int128)LONG_MAX || p < (__int128)LONG_MIN) r = 1;
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm overflow predicates: SADDOVFN · SSUBOVFN · SMULOVFN (TOS op n → 0/1) */
  if (kw(&L->cur,"SADDOVFN")||kw(&L->cur,"STACKADDOVFN")||kw(&L->cur,"ADDOVFN")||
      kw(&L->cur,"SADDOVERIMM")||kw(&L->cur,"SADDOVFIMM")){
    /* SADDOVFN n — TOS = 1 if TOS+n signed overflow, else 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (n > 0 && a > LONG_MAX - n) r = 1;
    else if (n < 0 && a < LONG_MIN - n) r = 1;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUBOVFN")||kw(&L->cur,"STACKSUBOVFN")||kw(&L->cur,"SUBOVFN")||
      kw(&L->cur,"SSUBOVERIMM")||kw(&L->cur,"SSUBOVFIMM")){
    /* SSUBOVFN n — TOS = 1 if TOS-n signed overflow, else 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (n > 0 && a < LONG_MIN + n) r = 1;
    else if (n < 0 && a > LONG_MAX + n) r = 1;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMULOVFN")||kw(&L->cur,"STACKMULOVFN")||kw(&L->cur,"MULOVFN")||
      kw(&L->cur,"SMULOVERIMM")||kw(&L->cur,"SMULOVFIMM")){
    /* SMULOVFN n — TOS = 1 if TOS*n signed overflow, else 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (a != 0 && n != 0){
      __int128 p = (__int128)a * (__int128)n;
      if (p > (__int128)LONG_MAX || p < (__int128)LONG_MIN) r = 1;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack fold/reduce: SSUM SPROD SFAND SFOR SFXOR FOLDMIN FOLDMAX
   * SMEAN SCOUNTNZ — optional trailing n = fold only top n; omit n → whole stack.
   * empty identities: sum/or/xor/countnz=0, prod=1, and=-1; min/max/mean empty → OK=0. */
  if (kw(&L->cur,"SSUM")||kw(&L->cur,"STACKSUM")||kw(&L->cur,"SFOLDADD")||
      kw(&L->cur,"SPROD")||kw(&L->cur,"STACKPROD")||kw(&L->cur,"SFOLDMUL")||
      kw(&L->cur,"SFAND")||kw(&L->cur,"FOLDAND")||kw(&L->cur,"SFOLDAND")||
      kw(&L->cur,"SFOR")||kw(&L->cur,"FOLDOR")||kw(&L->cur,"SFOLDOR")||
      kw(&L->cur,"SFXOR")||kw(&L->cur,"FOLDXOR")||kw(&L->cur,"SFOLDXOR")||
      kw(&L->cur,"FOLDMIN")||kw(&L->cur,"SFOLDMIN")||
      kw(&L->cur,"FOLDMAX")||kw(&L->cur,"SFOLDMAX")||
      kw(&L->cur,"SMEAN")||kw(&L->cur,"SFOLDAVG")||kw(&L->cur,"STACKMEAN")||
      kw(&L->cur,"SAVGALL")||
      kw(&L->cur,"SCOUNTNZ")||kw(&L->cur,"SCNTNZ")||kw(&L->cur,"SFOLDCOUNTNZ")||
      kw(&L->cur,"STACKCOUNTNZ")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    int have_n = 0;
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE") &&
         !kw(&L->cur,"PUSH") && !kw(&L->cur,"POP") && !kw(&L->cur,"CLEARSTACK") &&
         !kw(&L->cur,"PEEK") && !kw(&L->cur,"DROP") && !kw(&L->cur,"DUP"))){
      n = parse_expr(vm,L);
      have_n = 1;
    }
    if (!have_n) n = (long)vm->sp;
    if (n < 0) n = 0;
    if (have_n && n > (long)vm->sp){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    int is_min = (strcmp(op,"FOLDMIN")==0 || strcmp(op,"SFOLDMIN")==0);
    int is_max = (strcmp(op,"FOLDMAX")==0 || strcmp(op,"SFOLDMAX")==0);
    int is_sum = (strcmp(op,"SSUM")==0 || strcmp(op,"STACKSUM")==0 || strcmp(op,"SFOLDADD")==0);
    int is_prod = (strcmp(op,"SPROD")==0 || strcmp(op,"STACKPROD")==0 || strcmp(op,"SFOLDMUL")==0);
    int is_and = (strcmp(op,"SFAND")==0 || strcmp(op,"FOLDAND")==0 || strcmp(op,"SFOLDAND")==0);
    int is_or = (strcmp(op,"SFOR")==0 || strcmp(op,"FOLDOR")==0 || strcmp(op,"SFOLDOR")==0);
    int is_xor = (strcmp(op,"SFXOR")==0 || strcmp(op,"FOLDXOR")==0 || strcmp(op,"SFOLDXOR")==0);
    int is_mean = (strcmp(op,"SMEAN")==0 || strcmp(op,"SFOLDAVG")==0 ||
                   strcmp(op,"STACKMEAN")==0 || strcmp(op,"SAVGALL")==0);
    int is_cntnz = (strcmp(op,"SCOUNTNZ")==0 || strcmp(op,"SCNTNZ")==0 ||
                    strcmp(op,"SFOLDCOUNTNZ")==0 || strcmp(op,"STACKCOUNTNZ")==0);
    long r = 0;
    if (n == 0){
      if (is_min || is_max || is_mean){ var_set_num(vm,"OK",0); bump(vm); return 1; }
      if (is_sum || is_or || is_xor || is_cntnz) r = 0;
      else if (is_prod) r = 1;
      else if (is_and) r = -1;
      else r = 0;
      if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
      vm->stack[vm->sp++] = r;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
      var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    /* left-fold top n items (bottom of window first) */
    int base = vm->sp - (int)n;
    if (is_mean){
      long acc = 0;
      for (int i = 0; i < (int)n; i++) acc += vm->stack[base + i];
      r = acc / n;
    } else if (is_cntnz){
      long c = 0;
      for (int i = 0; i < (int)n; i++)
        if (vm->stack[base + i] != 0) c++;
      r = c;
    } else {
      r = vm->stack[base];
      for (int i = 1; i < (int)n; i++){
        long v = vm->stack[base + i];
        if (is_sum) r = r + v;
        else if (is_prod) r = r * v;
        else if (is_and) r = r & v;
        else if (is_or) r = r | v;
        else if (is_xor) r = r ^ v;
        else if (is_min) r = (r < v) ? r : v;
        else if (is_max) r = (r > v) ? r : v;
      }
    }
    vm->sp = base;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack↔cell bridge: TOCELL / FROMCELL */
  if (kw(&L->cur,"TOCELL")||kw(&L->cur,"STACKTOCELL")||kw(&L->cur,">CELL")){
    /* TOCELL dst [n] — pop n values into cells[dst..dst+n-1] (TOS → highest index) */
    lex_next(L);
    long dst = parse_expr(vm,L);
    long n = 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 1) n = 1;
    if (dst < 0) dst = 0;
    if (dst >= CUBALC_CELL_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (dst + n > CUBALC_CELL_N) n = CUBALC_CELL_N - dst;
    if (vm->sp < n){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    /* pop n times: first pop goes to dst+n-1 (TOS last index) */
    for (long i = n - 1; i >= 0; i--){
      long v = vm->stack[--vm->sp];
      vm->cells[(int)(dst + i)] = v;
    }
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"FROMCELL")||kw(&L->cur,"CELLTOSTACK")||kw(&L->cur,"CELL>")||
      kw(&L->cur,"PUSHCELL")){
    /* FROMCELL src [n] — push cells[src..src+n-1] onto stack (src first, then up) */
    lex_next(L);
    long src = parse_expr(vm,L);
    long n = 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 1) n = 1;
    if (src < 0) src = 0;
    if (src >= CUBALC_CELL_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (src + n > CUBALC_CELL_N) n = CUBALC_CELL_N - src;
    if (vm->sp + n > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long last = 0;
    for (long i = 0; i < n; i++){
      last = vm->cells[(int)(src + i)];
      vm->stack[vm->sp++] = last;
    }
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"FILLCELL")||kw(&L->cur,"CELLFILL")||kw(&L->cur,"FILL")){
    /* FILLCELL lo hi val — fill cell[lo..hi] with val */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long val = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i = lo; i <= hi; i++) vm->cells[(int)i] = val;
    var_set_num(vm,"LAST_N",val); vm->last_n=val;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack duals of cell memory: SCOPYCELL · SMOVECELL · SREVCELL · SROTCELL */
  if (kw(&L->cur,"SCOPYCELL")||kw(&L->cur,"SCELLCOPY")||kw(&L->cur,"STACKCOPYCELL")||
      kw(&L->cur,"SCOPYC")||kw(&L->cur,"SCMOVE")){
    /* src dst n (stack) — overlap-safe copy n cells src.. → dst.. */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long dst = vm->stack[--vm->sp];
    long src = vm->stack[--vm->sp];
    if (n < 0) n = 0;
    if (src < 0) src = 0;
    if (dst < 0) dst = 0;
    if (src >= CUBALC_CELL_N || dst >= CUBALC_CELL_N || n == 0){
      var_set_num(vm,"OK", n==0 ? 1 : 0);
      var_set_num(vm,"LAST_N",0); vm->last_n=0;
      var_set_num(vm,"SP",vm->sp);
      bump(vm); return 1;
    }
    if (src + n > CUBALC_CELL_N) n = CUBALC_CELL_N - src;
    if (dst + n > CUBALC_CELL_N) n = CUBALC_CELL_N - dst;
    if (n > 0){
      long tmp[CUBALC_CELL_N];
      for (long i=0;i<n;i++) tmp[i] = vm->cells[(int)(src+i)];
      for (long i=0;i<n;i++) vm->cells[(int)(dst+i)] = tmp[i];
    }
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMOVECELL")||kw(&L->cur,"SCELLMOVE")||kw(&L->cur,"STACKMOVECELL")||
      kw(&L->cur,"SMOVEC")){
    /* src dst n (stack) — copy then zero non-overlapping source */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long dst = vm->stack[--vm->sp];
    long src = vm->stack[--vm->sp];
    if (n < 0) n = 0;
    if (src < 0) src = 0;
    if (dst < 0) dst = 0;
    if (src >= CUBALC_CELL_N || dst >= CUBALC_CELL_N || n == 0){
      var_set_num(vm,"OK", n==0 ? 1 : 0);
      var_set_num(vm,"LAST_N",0); vm->last_n=0;
      var_set_num(vm,"SP",vm->sp);
      bump(vm); return 1;
    }
    if (src + n > CUBALC_CELL_N) n = CUBALC_CELL_N - src;
    if (dst + n > CUBALC_CELL_N) n = CUBALC_CELL_N - dst;
    if (n > 0){
      long tmp[CUBALC_CELL_N];
      for (long i=0;i<n;i++) tmp[i] = vm->cells[(int)(src+i)];
      for (long i=0;i<n;i++) vm->cells[(int)(dst+i)] = tmp[i];
      for (long i=0;i<n;i++){
        long si = src + i;
        if (si < dst || si >= dst + n)
          vm->cells[(int)si] = 0;
      }
    }
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SREVCELL")||kw(&L->cur,"SCELLREV")||kw(&L->cur,"STACKREVCELL")||
      kw(&L->cur,"SREVC")){
    /* lo hi (stack) — reverse cell[lo..hi] in place */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long i = lo, j = hi;
    while (i < j){
      long t = vm->cells[(int)i];
      vm->cells[(int)i] = vm->cells[(int)j];
      vm->cells[(int)j] = t;
      i++; j--;
    }
    long cnt = hi - lo + 1;
    var_set_num(vm,"LAST_N",cnt); vm->last_n=cnt;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROTCELL")||kw(&L->cur,"SCELLROT")||kw(&L->cur,"STACKROTCELL")||
      kw(&L->cur,"SROTC")){
    /* lo hi k (stack) — rotate cell[lo..hi] left by k (k<0 = right) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long n = hi - lo + 1;
    if (n > 0){
      long kk = (n == 0) ? 0 : (k % n);
      if (kk < 0) kk += n;
      if (kk){
        long tmp[CUBALC_CELL_N];
        for (long i=0;i<n;i++) tmp[i] = vm->cells[(int)(lo+i)];
        for (long i=0;i<n;i++)
          vm->cells[(int)(lo+i)] = tmp[(i + kk) % n];
      }
    }
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 data/memory plane: block copy/move + search + reverse */
  if (kw(&L->cur,"COPYCELL")||kw(&L->cur,"CELLCOPY")||kw(&L->cur,"CMOVE")){
    /* COPYCELL src dst n — copy n cells src.. → dst.. (overlap-safe) */
    lex_next(L);
    long src = parse_expr(vm,L);
    long dst = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (src < 0) src = 0;
    if (dst < 0) dst = 0;
    if (src >= CUBALC_CELL_N || dst >= CUBALC_CELL_N || n == 0){
      var_set_num(vm,"OK", n==0 ? 1 : 0);
      var_set_num(vm,"LAST_N",0); vm->last_n=0;
      bump(vm); return 1;
    }
    if (src + n > CUBALC_CELL_N) n = CUBALC_CELL_N - src;
    if (dst + n > CUBALC_CELL_N) n = CUBALC_CELL_N - dst;
    if (n > 0){
      long tmp[CUBALC_CELL_N];
      for (long i=0;i<n;i++) tmp[i] = vm->cells[(int)(src+i)];
      for (long i=0;i<n;i++) vm->cells[(int)(dst+i)] = tmp[i];
    }
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"MOVECELL")||kw(&L->cur,"CELLMOVE")){
    /* MOVECELL src dst n — copy then zero source (non-overlapping src clear) */
    lex_next(L);
    long src = parse_expr(vm,L);
    long dst = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (src < 0) src = 0;
    if (dst < 0) dst = 0;
    if (src >= CUBALC_CELL_N || dst >= CUBALC_CELL_N || n == 0){
      var_set_num(vm,"OK", n==0 ? 1 : 0);
      var_set_num(vm,"LAST_N",0); vm->last_n=0;
      bump(vm); return 1;
    }
    if (src + n > CUBALC_CELL_N) n = CUBALC_CELL_N - src;
    if (dst + n > CUBALC_CELL_N) n = CUBALC_CELL_N - dst;
    if (n > 0){
      long tmp[CUBALC_CELL_N];
      for (long i=0;i<n;i++) tmp[i] = vm->cells[(int)(src+i)];
      for (long i=0;i<n;i++) vm->cells[(int)(dst+i)] = tmp[i];
      /* clear source cells that are not inside the destination range */
      for (long i=0;i<n;i++){
        long si = src + i;
        if (si < dst || si >= dst + n)
          vm->cells[(int)si] = 0;
      }
    }
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"FINDCELL")||kw(&L->cur,"CELLFIND")||kw(&L->cur,"INDEXCELL")){
    /* FINDCELL val [lo [hi]] — first index of val, or -1; OK=found */
    lex_next(L);
    long val = parse_expr(vm,L);
    long lo = 0, hi = CUBALC_CELL_N - 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE"))){
      lo = parse_expr(vm,L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        hi = parse_expr(vm,L);
      else hi = CUBALC_CELL_N - 1;
    }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long found = -1;
    for (long i=lo;i<=hi;i++){
      if (vm->cells[(int)i] == val){ found = i; break; }
    }
    var_set_num(vm,"LAST_N",found); vm->last_n=found;
    var_set_num(vm,"OK", found >= 0 ? 1 : 0);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"COUNTCELL")||kw(&L->cur,"CELLCOUNT")||kw(&L->cur,"COUNTVAL")){
    /* COUNTCELL val [lo [hi]] — how many cells equal val */
    lex_next(L);
    long val = parse_expr(vm,L);
    long lo = 0, hi = CUBALC_CELL_N - 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE"))){
      lo = parse_expr(vm,L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        hi = parse_expr(vm,L);
      else hi = CUBALC_CELL_N - 1;
    }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long cnt = 0;
    for (long i=lo;i<=hi;i++)
      if (vm->cells[(int)i] == val) cnt++;
    var_set_num(vm,"LAST_N",cnt); vm->last_n=cnt;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
