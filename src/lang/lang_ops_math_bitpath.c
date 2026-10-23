/* CubalC lang — lang_ops_math_bitpath.c
 * MATH bitpath: pack/nibble · SBSWAP/PEXT · SSEXT
 * Peel from math residual (B_math_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_math_bitpath.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_math_try_bitpath(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-8 pack8/nibble + set nibble: SPACK8 SPACKNIB SSETNIB */
  if (kw(&L->cur,"SPACK8")||kw(&L->cur,"SPACKB")||kw(&L->cur,"STACKPACK8")||
      kw(&L->cur,"SPACKNIB")||kw(&L->cur,"SPACK4")||kw(&L->cur,"STACKPACKNIB")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long lo = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long r;
    if (strcmp(op,"SPACKNIB")==0 || strcmp(op,"SPACK4")==0 || strcmp(op,"STACKPACKNIB")==0)
      r = (long)((((unsigned int)hi & 0xFu) << 4) | ((unsigned int)lo & 0xFu));
    else
      r = (long)((((unsigned int)hi & 0xFFu) << 8) | ((unsigned int)lo & 0xFFu));
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETNIB")||kw(&L->cur,"SSETNIBBLE")||kw(&L->cur,"STACKSETNIB")){
    /* a field i → deposit 4-bit field at nibble index i */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i > 15) i = 15;
    unsigned long base = (unsigned long)a;
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long shift = (unsigned long)(i * 4);
    long r = (long)((base & ~(0xFul << shift)) | (f << shift));
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 word data path: SBSWAP SBITREV SPARITY SNIBBLE */
  if (kw(&L->cur,"SBSWAP")||kw(&L->cur,"SBSWAP32")||kw(&L->cur,"STACKBSWAP")||
      kw(&L->cur,"SBITREV")||kw(&L->cur,"SREVBITS")||kw(&L->cur,"STACKBITREV")||
      kw(&L->cur,"SPARITY")||kw(&L->cur,"STACKPARITY")||kw(&L->cur,"SPAR")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SBSWAP")==0 || strcmp(op,"SBSWAP32")==0 || strcmp(op,"STACKBSWAP")==0){
      unsigned int w = (unsigned int)a;
      w = ((w & 0x000000FFu) << 24) | ((w & 0x0000FF00u) << 8) |
          ((w & 0x00FF0000u) >> 8) | ((w & 0xFF000000u) >> 24);
      r = (long)w;
    } else if (strcmp(op,"SBITREV")==0 || strcmp(op,"SREVBITS")==0 ||
               strcmp(op,"STACKBITREV")==0){
      unsigned int w = (unsigned int)a;
      unsigned int rv = 0;
      for (int i = 0; i < 32; i++){
        rv = (rv << 1) | (w & 1u);
        w >>= 1;
      }
      r = (long)rv;
    } else {
      /* SPARITY / STACKPARITY / SPAR */
      unsigned long u = (unsigned long)a;
      int n = 0;
      while (u){ n ^= (int)(u & 1u); u >>= 1; }
      r = (long)n;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNIB")||kw(&L->cur,"SNIBBLE")||kw(&L->cur,"STACKNIBBLE")||
      kw(&L->cur,"SGETNIB")){
    /* a i → i-th little-endian nibble of a (i clamped 0..15) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i > 15) i = 15;
    long r = (long)(((unsigned long)a >> (unsigned)(i * 4)) & 0xFul);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm nibble field: SNIBN · SSETNIBN · SCLRNIBN (imm dual of SNIB/SSETNIB; control-word plane) */
  if (kw(&L->cur,"SNIBN")||kw(&L->cur,"STACKNIBN")||kw(&L->cur,"SGETNIBN")||
      kw(&L->cur,"NIBN")||kw(&L->cur,"GETNIBN")||kw(&L->cur,"SNIBBLEN")){
    /* SNIBN n — TOS = little-endian nibble n of TOS; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    long r = (long)(((unsigned long)vm->stack[vm->sp - 1] >> (unsigned)(n * 4)) & 0xFul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETNIBN")||kw(&L->cur,"STACKSETNIBN")||kw(&L->cur,"SSETNIBIMM")||
      kw(&L->cur,"SETNIBN")||kw(&L->cur,"PUTNIBN")){
    /* SSETNIBN field n — deposit low 4 bits of field into nibble n of TOS; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long shift = (unsigned long)(n * 4);
    long r = (long)((base & ~(0xFul << shift)) | (f << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRNIBN")||kw(&L->cur,"STACKCLRNIBN")||kw(&L->cur,"SCLRNIBIMM")||
      kw(&L->cur,"CLRNIBN")||kw(&L->cur,"ZAPNIBN")){
    /* SCLRNIBN n — clear little-endian nibble n of TOS; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long shift = (unsigned long)(n * 4);
    long r = (long)(base & ~(0xFul << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 pack32 + PEXT/PDEP + interleave (universal bit data-path) */
  if (kw(&L->cur,"SPACK32")||kw(&L->cur,"SPACKW")||kw(&L->cur,"STACKPACK32")){
    /* hi lo → (hi<<32)|lo  (32-bit halves → 64-bit word) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long lo = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    unsigned long long h = (unsigned long long)(unsigned int)hi;
    unsigned long long l = (unsigned long long)(unsigned int)lo;
    long r = (long)((h << 32) | l);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHI32")||kw(&L->cur,"SHIWORD")||kw(&L->cur,"STACKHI32")||
      kw(&L->cur,"SLO32")||kw(&L->cur,"SLOWORD")||kw(&L->cur,"STACKLO32")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long long u = (unsigned long long)vm->stack[--vm->sp];
    long r;
    if (strcmp(op,"SHI32")==0 || strcmp(op,"SHIWORD")==0 || strcmp(op,"STACKHI32")==0)
      r = (long)(unsigned int)(u >> 32);
    else
      r = (long)(unsigned int)(u & 0xFFFFFFFFu);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPEXT")||kw(&L->cur,"SPEXTRACT")||kw(&L->cur,"STACKPEXT")||
      kw(&L->cur,"SPEX")||kw(&L->cur,"SPDEP")||kw(&L->cur,"SPDEPOSIT")||
      kw(&L->cur,"STACKPDEP")||kw(&L->cur,"SPDP")){
    /* SPEXT: src mask → parallel extract bits of src under mask (BMI2 PEXT)
     * SPDEP: src mask → deposit low bits of src into mask positions (BMI2 PDEP) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_dep = (strcmp(op,"SPDEP")==0 || strcmp(op,"SPDEPOSIT")==0 ||
                  strcmp(op,"STACKPDEP")==0 || strcmp(op,"SPDP")==0);
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long long mask = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long src  = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long res = 0;
    if (is_dep){
      unsigned long long bb = 1;
      for (int i=0;i<64;i++){
        if ((mask >> i) & 1ull){
          if (src & bb) res |= (1ull << i);
          bb <<= 1;
        }
      }
    } else {
      unsigned long long k = 0;
      for (int i=0;i<64;i++){
        if ((mask >> i) & 1ull){
          if ((src >> i) & 1ull) res |= (1ull << k);
          k++;
        }
      }
    }
    long r = (long)res;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZIP")||kw(&L->cur,"SINTERLEAVE")||kw(&L->cur,"STACKZIP")||
      kw(&L->cur,"SZIPBITS")||kw(&L->cur,"SMORTON")){
    /* a b → interleave low 32 bits: ... b1 a1 b0 a0 (Morton / zip) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long long b = (unsigned long long)(unsigned int)vm->stack[--vm->sp];
    unsigned long long a = (unsigned long long)(unsigned int)vm->stack[--vm->sp];
    unsigned long long r = 0;
    for (int i=0;i<32;i++){
      if ((a >> i) & 1ull) r |= (1ull << (2*i));
      if ((b >> i) & 1ull) r |= (1ull << (2*i + 1));
    }
    long out = (long)r;
    vm->stack[vm->sp++] = out;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",out); vm->last_n=out;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUNZIP")||kw(&L->cur,"SDEINTERLEAVE")||kw(&L->cur,"STACKUNZIP")||
      kw(&L->cur,"SUNZIPBITS")){
    /* z → push even bits (lo), odd bits (hi) as two stack values: lo hi (TOS=hi)
     * convention: after SUNZIP, TOS=odd/hi half, NOS=even/lo half */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 1 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long long z = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long even = 0, odd = 0;
    for (int i=0;i<32;i++){
      if ((z >> (2*i)) & 1ull) even |= (1ull << i);
      if ((z >> (2*i + 1)) & 1ull) odd |= (1ull << i);
    }
    vm->stack[vm->sp++] = (long)even;
    vm->stack[vm->sp++] = (long)odd;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",(long)odd); vm->last_n=(long)odd;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 sign/zero extend: SSEXT SZEXT SSEXT8 SSEXT16 */
  if (kw(&L->cur,"SSEXT8")||kw(&L->cur,"SSEXTB")||kw(&L->cur,"STACKSEXT8")||
      kw(&L->cur,"SSEXT16")||kw(&L->cur,"SSEXTW")||kw(&L->cur,"STACKSEXT16")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (strcmp(op,"SSEXT16")==0 || strcmp(op,"SSEXTW")==0 || strcmp(op,"STACKSEXT16")==0){
      r = a & 0xFFFFL;
      if (r & 0x8000L) r |= ~0xFFFFL;
    } else {
      r = a & 0xFFL;
      if (r & 0x80L) r |= ~0xFFL;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT")||kw(&L->cur,"STACKSEXT")||kw(&L->cur,"SSIGNEXT")||
      kw(&L->cur,"SZEXT")||kw(&L->cur,"STACKZEXT")||kw(&L->cur,"SZEROEXT")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long w = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    int is_z = (strcmp(op,"SZEXT")==0 || strcmp(op,"STACKZEXT")==0 ||
                strcmp(op,"SZEROEXT")==0);
    if (w <= 0) r = 0;
    else if (w >= 63) r = a;
    else {
      unsigned long mask = (1ul << (unsigned)w) - 1ul;
      unsigned long v = (unsigned long)a & mask;
      if (!is_z){
        unsigned long sign = 1ul << (unsigned)(w - 1);
        if (v & sign) v |= ~mask;
      }
      r = (long)v;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SALIGN")||kw(&L->cur,"SROUNDUP")||kw(&L->cur,"STACKALIGN")||
      kw(&L->cur,"SALIGNDN")||kw(&L->cur,"SROUNDDN")||kw(&L->cur,"STACKALIGNDN")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long al = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = a;
    int is_dn = (strcmp(op,"SALIGNDN")==0 || strcmp(op,"SROUNDDN")==0 ||
                 strcmp(op,"STACKALIGNDN")==0);
    if (al > 0){
      long q = a / al, rem = a % al;
      if (rem != 0){
        if (is_dn) r = (a > 0) ? q * al : (q - 1) * al;
        else r = (a > 0) ? (q + 1) * al : q * al;
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPACK16")||kw(&L->cur,"SPACK")||kw(&L->cur,"STACKPACK")||
      kw(&L->cur,"STACKPACK16")){
    /* hi lo → (hi<<16)|lo  (16-bit halves) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long lo = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    unsigned int h = (unsigned int)hi & 0xFFFFu;
    unsigned int l = (unsigned int)lo & 0xFFFFu;
    long r = (long)((h << 16) | l);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  return 0;
}
