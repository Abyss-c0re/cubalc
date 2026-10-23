/* CubalC lang — lang_ops_toc_bitops.c
 * TOC bitops: SGET32 field · BREV/ROL/ROR reverse-rotate (imm + stack dual)
 * Peel from toc (B_toc_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_toc_bitops.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_toc_try_bitops(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-7 imm 32-bit field TOC: SGET32TOCN · SSET32TOCN · SCLR32TOCN
   * (imm dual of SGET32N/SSET32N/SCLR32N into cell; complete 4/8/16/32 field ladder) */
  if (kw(&L->cur,"SGET32TOCN")||kw(&L->cur,"SWORD32TOCN")||kw(&L->cur,"STACKGET32TOCN")||
      kw(&L->cur,"SGET32TOCIMM")||kw(&L->cur,"GET32TOCN")||kw(&L->cur,"SCELLGET32N")||
      kw(&L->cur,"SDWORDTOCN")||kw(&L->cur,"WORD32TOCN")){
    /* i + n → cells[i] = LE 32-bit word n of cells[i]; n clamped 0..1; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 32)) & 0xFFFFFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSET32TOCN")||kw(&L->cur,"SSET32TOCIMM")||kw(&L->cur,"STACKSET32TOCN")||
      kw(&L->cur,"SET32TOCN")||kw(&L->cur,"SCELLSET32N")||kw(&L->cur,"SSETDWORDTOCN")||
      kw(&L->cur,"PUT32TOCN")||kw(&L->cur,"SSETW32TOCN")){
    /* i + field + n → deposit low 32 bits of field into LE word n of cells[i] */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long shift = (unsigned long)(n * 32);
    long r = (long)((base & ~(0xFFFFFFFFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLR32TOCN")||kw(&L->cur,"SCLR32TOCIMM")||kw(&L->cur,"STACKCLR32TOCN")||
      kw(&L->cur,"CLR32TOCN")||kw(&L->cur,"SCELLCLR32N")||kw(&L->cur,"SCLRDWORDTOCN")||
      kw(&L->cur,"ZAP32TOCN")||kw(&L->cur,"SCLRW32TOCN")){
    /* i + n → clear LE 32-bit word n of cells[i]; n clamped 0..1; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 32);
    long r = (long)(base & ~(0xFFFFFFFFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack dual 32-bit field TOC: SGET32TOC · SSET32TOC · SCLR32TOC
   * (stack dual of SGET32TOCN/SSET32TOCN/SCLR32TOCN; foundation complete field ladder) */
  if (kw(&L->cur,"SGET32TOC")||kw(&L->cur,"SWORD32TOC")||kw(&L->cur,"STACKGET32TOC")||
      kw(&L->cur,"GET32TOC")||kw(&L->cur,"SCELLGET32S")||kw(&L->cur,"SDWORDTOC")||
      kw(&L->cur,"WORD32TOC")||kw(&L->cur,"SGET32AT")){
    /* i n → cells[i] = LE 32-bit word n of cells[i]; n clamped 0..1; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 32)) & 0xFFFFFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSET32TOC")||kw(&L->cur,"STACKSET32TOC")||kw(&L->cur,"SET32TOC")||
      kw(&L->cur,"SCELLSET32S")||kw(&L->cur,"SSETDWORDTOC")||kw(&L->cur,"PUT32TOC")||
      kw(&L->cur,"SSETW32TOC")||kw(&L->cur,"SSET32AT")){
    /* i field n → deposit low 32 bits of field into LE word n of cells[i] */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long shift = (unsigned long)(n * 32);
    long r = (long)((base & ~(0xFFFFFFFFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLR32TOC")||kw(&L->cur,"STACKCLR32TOC")||kw(&L->cur,"CLR32TOC")||
      kw(&L->cur,"SCELLCLR32S")||kw(&L->cur,"SCLRDWORDTOC")||kw(&L->cur,"ZAP32TOC")||
      kw(&L->cur,"SCLRW32TOC")||kw(&L->cur,"SCLR32AT")){
    /* i n → clear LE 32-bit word n of cells[i]; n clamped 0..1; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 32);
    long r = (long)(base & ~(0xFFFFFFFFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 imm low-n reverse/rotate TOC: SBREVTOCN · SROLBTOCN · SRORBTOCN
   * (imm dual of SBREVN/SROLBN/SRORBN into cell after SCLRMNTOCN plane) */
  if (kw(&L->cur,"SBREVTOCN")||kw(&L->cur,"SBREVTOCIMM")||kw(&L->cur,"STACKBREVTOCN")||
      kw(&L->cur,"SREVLOWTOCN")||kw(&L->cur,"BREVTOCN")||kw(&L->cur,"SCELLBREVN")||
      kw(&L->cur,"SBITREVTOCN")||kw(&L->cur,"BREVNTOCN")){
    /* i + n → reverse low n bits of cells[i]; high kept; n 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n > 0 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      unsigned long ra = 0;
      for (long k = 0; k < n; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)(((unsigned long)a & ~m) | (ra & m));
    } else if (n >= 64){
      unsigned long la = (unsigned long)a;
      unsigned long ra = 0;
      for (int k = 0; k < 64; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)ra;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROLBTOCN")||kw(&L->cur,"SROLBTOCIMM")||kw(&L->cur,"STACKROLBTOCN")||
      kw(&L->cur,"SROTLBTOCN")||kw(&L->cur,"ROLBTOCN")||kw(&L->cur,"SCELLROLBN")||
      kw(&L->cur,"SLOWROLTOCN")||kw(&L->cur,"ROLBNTOCN")){
    /* i + n → rotate left by 1 within low n bits of cells[i]; high kept; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      la = ((la << 1) | (la >> (unsigned)(n - 1))) & m;
      r = (long)(((unsigned long)a & ~m) | la);
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua << 1) | (ua >> 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORBTOCN")||kw(&L->cur,"SRORBTOCIMM")||kw(&L->cur,"STACKRORBTOCN")||
      kw(&L->cur,"SROTRBTOCN")||kw(&L->cur,"RORBTOCN")||kw(&L->cur,"SCELLRORBN")||
      kw(&L->cur,"SLOWRORTOCN")||kw(&L->cur,"RORBNTOCN")){
    /* i + n → rotate right by 1 within low n bits of cells[i]; high kept; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      la = ((la >> 1) | (la << (unsigned)(n - 1))) & m;
      r = (long)(((unsigned long)a & ~m) | la);
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua >> 1) | (ua << 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 imm high-n reverse/rotate TOC: SBREVHNTOCN · SROLHNTOCN · SRORHNTOCN
   * (imm dual of SBREVHN/SROLHN/SRORHN into cell; high dual of SBREVTOCN plane) */
  if (kw(&L->cur,"SBREVHNTOCN")||kw(&L->cur,"SBREVHNTOCIMM")||kw(&L->cur,"STACKBREVHNTOCN")||
      kw(&L->cur,"SREVHIGHTOCN")||kw(&L->cur,"BREVHNTOCN")||kw(&L->cur,"SCELLBREVHN")||
      kw(&L->cur,"SBITREVHNTOCN")||kw(&L->cur,"BREVHNTOC")){
    /* i + n → reverse high n bits of cells[i]; low kept; n 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n > 0 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long ra = 0;
      for (long k = 0; k < n; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)(((unsigned long)a & ~m) | ((ra << sh) & m));
    } else if (n >= 64){
      unsigned long la = (unsigned long)a;
      unsigned long ra = 0;
      for (int k = 0; k < 64; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)ra;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROLHNTOCN")||kw(&L->cur,"SROLHNTOCIMM")||kw(&L->cur,"STACKROLHNTOCN")||
      kw(&L->cur,"SROTLHNTOCN")||kw(&L->cur,"ROLHNTOCN")||kw(&L->cur,"SCELLROLHN")||
      kw(&L->cur,"SHIGHROLTOCN")||kw(&L->cur,"ROLHNTOC")){
    /* i + n → rotate left by 1 within high n bits of cells[i]; low kept; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long fm = (1ul << (unsigned)n) - 1ul;
      la = ((la << 1) | (la >> (unsigned)(n - 1))) & fm;
      r = (long)(((unsigned long)a & ~m) | ((la << sh) & m));
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua << 1) | (ua >> 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORHNTOCN")||kw(&L->cur,"SRORHNTOCIMM")||kw(&L->cur,"STACKRORHNTOCN")||
      kw(&L->cur,"SROTRHNTOCN")||kw(&L->cur,"RORHNTOCN")||kw(&L->cur,"SCELLRORHN")||
      kw(&L->cur,"SHIGHRORTOCN")||kw(&L->cur,"RORHNTOC")){
    /* i + n → rotate right by 1 within high n bits of cells[i]; low kept; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long fm = (1ul << (unsigned)n) - 1ul;
      la = ((la >> 1) | (la << (unsigned)(n - 1))) & fm;
      r = (long)(((unsigned long)a & ~m) | ((la << sh) & m));
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua >> 1) | (ua << 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 imm high-n metric TOC: SPOPHNTOCN · SANYHNTOCN · SALLHNTOCN
   * (imm dual of SPOPHN/SANYHN/SALLHN into cell; high dual of SPOPMNTOCN plane) */
  if (kw(&L->cur,"SPOPHNTOCN")||kw(&L->cur,"SPOPHNTOCIMM")||kw(&L->cur,"STACKPOPHNTOCN")||
      kw(&L->cur,"SONESHNTOCN")||kw(&L->cur,"POPHNTOCN")||kw(&L->cur,"SCELLPOPHN")||
      kw(&L->cur,"SHIGHPOPTOCN")||kw(&L->cur,"SPCNTHNTOCN")){
    /* i + n → cells[i] = popcount(cells[i] & high-n mask); n 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    unsigned long ua = (unsigned long)vm->cells[(int)i] & m;
    long r = 0;
    while (ua){ r += (long)(ua & 1ul); ua >>= 1; }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SANYHNTOCN")||kw(&L->cur,"SANYHNTOCIMM")||kw(&L->cur,"STACKANYHNTOCN")||
      kw(&L->cur,"SHIGHANYTOCN")||kw(&L->cur,"ANYHNTOCN")||kw(&L->cur,"SCELLANYHN")||
      kw(&L->cur,"STESTANYHNTOCN")){
    /* i + n → cells[i] = ((cells[i] & high-n mask) != 0) ? 1 : 0; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (((unsigned long)vm->cells[(int)i] & m) != 0) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SALLHNTOCN")||kw(&L->cur,"SALLHNTOCIMM")||kw(&L->cur,"STACKALLHNTOCN")||
      kw(&L->cur,"SHIGHALLTOCN")||kw(&L->cur,"ALLHNTOCN")||kw(&L->cur,"SCELLALLHN")||
      kw(&L->cur,"STESTALLHNTOCN")){
    /* i + n → cells[i] = ((cells[i] & high-n mask) == mask) ? 1 : 0; n=0 → 1 vacuous */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (((unsigned long)vm->cells[(int)i] & m) == m) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack dual low-n metric TOC: SPOPMNTOC · SANYMNTOC · SALLMNTOC
   * (stack dual of SPOPMNTOCN/SANYMNTOCN/SALLMNTOCN; i n from stack → metric into cell) */
  if (kw(&L->cur,"SPOPMNTOC")||kw(&L->cur,"STACKPOPMNTOC")||kw(&L->cur,"SONESMNTOC")||
      kw(&L->cur,"POPMNTOC")||kw(&L->cur,"SCELLPOPMNS")||kw(&L->cur,"SLOWPOPTOC")||
      kw(&L->cur,"SPCNTMNTOC")||kw(&L->cur,"SPOPMNAT")){
    /* i n → cells[i] = popcount(cells[i] & low-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    unsigned long ua = (unsigned long)vm->cells[(int)i] & m;
    long r = 0;
    while (ua){ r += (long)(ua & 1ul); ua >>= 1; }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SANYMNTOC")||kw(&L->cur,"STACKANYMNTOC")||kw(&L->cur,"SLOWANYTOC")||
      kw(&L->cur,"ANYMNTOC")||kw(&L->cur,"SCELLANYMNS")||kw(&L->cur,"STESTANYTOC")||
      kw(&L->cur,"SANYMNAT")){
    /* i n → cells[i] = ((cells[i] & low-n mask) != 0) ? 1 : 0; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (((unsigned long)vm->cells[(int)i] & m) != 0) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SALLMNTOC")||kw(&L->cur,"STACKALLMNTOC")||kw(&L->cur,"SLOWALLTOC")||
      kw(&L->cur,"ALLMNTOC")||kw(&L->cur,"SCELLALLMNS")||kw(&L->cur,"STESTALLTOC")||
      kw(&L->cur,"SALLMNAT")){
    /* i n → cells[i] = ((cells[i] & low-n mask) == mask) ? 1 : 0; n=0 → 1 vacuous */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (((unsigned long)vm->cells[(int)i] & m) == m) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack dual high-n metric TOC: SPOPHNTOC · SANYHNTOC · SALLHNTOC
   * (stack dual of SPOPHNTOCN/SANYHNTOCN/SALLHNTOCN; high dual of SPOPMNTOC plane) */
  if (kw(&L->cur,"SPOPHNTOC")||kw(&L->cur,"STACKPOPHNTOC")||kw(&L->cur,"SONESHNTOC")||
      kw(&L->cur,"POPHNTOC")||kw(&L->cur,"SCELLPOPHNS")||kw(&L->cur,"SHIGHPOPTOC")||
      kw(&L->cur,"SPCNTHNTOC")||kw(&L->cur,"SPOPHNAT")){
    /* i n → cells[i] = popcount(cells[i] & high-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    unsigned long ua = (unsigned long)vm->cells[(int)i] & m;
    long r = 0;
    while (ua){ r += (long)(ua & 1ul); ua >>= 1; }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SANYHNTOC")||kw(&L->cur,"STACKANYHNTOC")||kw(&L->cur,"SHIGHANYTOC")||
      kw(&L->cur,"ANYHNTOC")||kw(&L->cur,"SCELLANYHNS")||kw(&L->cur,"STESTANYHNTOC")||
      kw(&L->cur,"SANYHNAT")){
    /* i n → cells[i] = ((cells[i] & high-n mask) != 0) ? 1 : 0; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (((unsigned long)vm->cells[(int)i] & m) != 0) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SALLHNTOC")||kw(&L->cur,"STACKALLHNTOC")||kw(&L->cur,"SHIGHALLTOC")||
      kw(&L->cur,"ALLHNTOC")||kw(&L->cur,"SCELLALLHNS")||kw(&L->cur,"STESTALLHNTOC")||
      kw(&L->cur,"SALLHNAT")){
    /* i n → cells[i] = ((cells[i] & high-n mask) == mask) ? 1 : 0; n=0 → 1 vacuous */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (((unsigned long)vm->cells[(int)i] & m) == m) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack dual low-n mask TOC: SANDMNTOC · SORMNTOC · SXORMNTOC
   * (stack dual of SANDMNTOCN/SORMNTOCN/SXORMNTOCN; i n from stack → mask op into cell) */
  if (kw(&L->cur,"SANDMNTOC")||kw(&L->cur,"STACKANDMNTOC")||kw(&L->cur,"SKEEPLNTOC")||
      kw(&L->cur,"ANDMNTOC")||kw(&L->cur,"SCELLANDMNS")||kw(&L->cur,"SLOWANDTOC")||
      kw(&L->cur,"KEEPLNTOC")||kw(&L->cur,"SANDMNAT")){
    /* i n → cells[i] &= low-n mask; keep low n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (long)((unsigned long)vm->cells[(int)i] & m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORMNTOC")||kw(&L->cur,"STACKORMNTOC")||kw(&L->cur,"SSETLNTOC")||
      kw(&L->cur,"ORMNTOC")||kw(&L->cur,"SCELLORMNS")||kw(&L->cur,"SLOWORTOC")||
      kw(&L->cur,"SETLNTOC")||kw(&L->cur,"SORMNAT")){
    /* i n → cells[i] |= low-n mask; set low n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (long)((unsigned long)vm->cells[(int)i] | m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORMNTOC")||kw(&L->cur,"STACKXORMNTOC")||kw(&L->cur,"SFLIPLNTOC")||
      kw(&L->cur,"XORMNTOC")||kw(&L->cur,"SCELLXORMNS")||kw(&L->cur,"SLOWXORTOC")||
      kw(&L->cur,"FLIPLNTOC")||kw(&L->cur,"SXORMNAT")){
    /* i n → cells[i] ^= low-n mask; toggle low n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (long)((unsigned long)vm->cells[(int)i] ^ m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack dual inverted low-n mask TOC: SNANDMNTOC · SNORMNTOC · SXNORMNTOC
   * (stack dual of SNANDMNTOCN/SNORMNTOCN/SXNORMNTOCN; i n from stack after SANDMNTOC plane) */
  if (kw(&L->cur,"SNANDMNTOC")||kw(&L->cur,"STACKNANDMNTOC")||kw(&L->cur,"SLOWNANDTOC")||
      kw(&L->cur,"NANDMNTOC")||kw(&L->cur,"SCELLNANDMNS")||kw(&L->cur,"SNANDMASKTOC")||
      kw(&L->cur,"MASKNANDTOC")||kw(&L->cur,"SNANDMNAT")){
    /* i n → cells[i] = ~(cells[i] & low-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (long)~((unsigned long)vm->cells[(int)i] & m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORMNTOC")||kw(&L->cur,"STACKNORMNTOC")||kw(&L->cur,"SLOWNORTOC")||
      kw(&L->cur,"NORMNTOC")||kw(&L->cur,"SCELLNORMNS")||kw(&L->cur,"SNORMASKTOC")||
      kw(&L->cur,"MASKNORTOC")||kw(&L->cur,"SNORMNAT")){
    /* i n → cells[i] = ~(cells[i] | low-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (long)~((unsigned long)vm->cells[(int)i] | m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORMNTOC")||kw(&L->cur,"STACKXNORMNTOC")||kw(&L->cur,"SLOWXNORTOC")||
      kw(&L->cur,"XNORMNTOC")||kw(&L->cur,"SCELLXNORMNS")||kw(&L->cur,"SXNORMASKTOC")||
      kw(&L->cur,"MASKXNORTOC")||kw(&L->cur,"SEQUIVMNTOC")||kw(&L->cur,"EQUIVMNTOC")||
      kw(&L->cur,"SXNORMNAT")){
    /* i n → cells[i] = ~(cells[i] ^ low-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (long)~((unsigned long)vm->cells[(int)i] ^ m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack dual high-n mask TOC: SANDHNTOC · SORHNTOC · SXORHNTOC
   * (stack dual of SANDHNTOCN/SORHNTOCN/SXORHNTOCN; high-n dual of SANDMNTOC plane) */
  if (kw(&L->cur,"SANDHNTOC")||kw(&L->cur,"STACKANDHNTOC")||kw(&L->cur,"SKEEPHNTOC")||
      kw(&L->cur,"ANDHNTOC")||kw(&L->cur,"SCELLANDHNS")||kw(&L->cur,"SHIGHANDTOC")||
      kw(&L->cur,"KEEPHNTOC")||kw(&L->cur,"SANDHNAT")){
    /* i n → cells[i] &= high-n mask; keep high n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)((unsigned long)vm->cells[(int)i] & m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORHNTOC")||kw(&L->cur,"STACKORHNTOC")||kw(&L->cur,"SSETHNTOC")||
      kw(&L->cur,"ORHNTOC")||kw(&L->cur,"SCELLORHNS")||kw(&L->cur,"SHIGHORTOC")||
      kw(&L->cur,"SETHNTOC")||kw(&L->cur,"SORHNAT")){
    /* i n → cells[i] |= high-n mask; set high n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)((unsigned long)vm->cells[(int)i] | m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORHNTOC")||kw(&L->cur,"STACKXORHNTOC")||kw(&L->cur,"SFLIPHNTOC")||
      kw(&L->cur,"XORHNTOC")||kw(&L->cur,"SCELLXORHNS")||kw(&L->cur,"SHIGHXORTOC")||
      kw(&L->cur,"FLIPHNTOC")||kw(&L->cur,"SXORHNAT")){
    /* i n → cells[i] ^= high-n mask; toggle high n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)((unsigned long)vm->cells[(int)i] ^ m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack dual inverted high-n mask TOC: SNANDHNTOC · SNORHNTOC · SXNORHNTOC
   * (stack dual of SNANDHNTOCN/SNORHNTOCN/SXNORHNTOCN; high dual of SNANDMNTOC after SANDHNTOC) */
  if (kw(&L->cur,"SNANDHNTOC")||kw(&L->cur,"STACKNANDHNTOC")||kw(&L->cur,"SHIGHNANDTOC")||
      kw(&L->cur,"NANDHNTOC")||kw(&L->cur,"SCELLNANDHNS")||kw(&L->cur,"SNANDMASKHTOC")||
      kw(&L->cur,"MASKNANDHTOC")||kw(&L->cur,"SNANDHNAT")){
    /* i n → cells[i] = ~(cells[i] & high-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)~((unsigned long)vm->cells[(int)i] & m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORHNTOC")||kw(&L->cur,"STACKNORHNTOC")||kw(&L->cur,"SHIGHNORTOC")||
      kw(&L->cur,"NORHNTOC")||kw(&L->cur,"SCELLNORHNS")||kw(&L->cur,"SNORMASKHTOC")||
      kw(&L->cur,"MASKNORHTOC")||kw(&L->cur,"SNORHNAT")){
    /* i n → cells[i] = ~(cells[i] | high-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)~((unsigned long)vm->cells[(int)i] | m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORHNTOC")||kw(&L->cur,"STACKXNORHNTOC")||kw(&L->cur,"SHIGHXNORTOC")||
      kw(&L->cur,"XNORHNTOC")||kw(&L->cur,"SCELLXNORHNS")||kw(&L->cur,"SXNORMASKHTOC")||
      kw(&L->cur,"MASKXNORHTOC")||kw(&L->cur,"SEQUIVHNTOC")||kw(&L->cur,"EQUIVHNTOC")||
      kw(&L->cur,"SXNORHNAT")){
    /* i n → cells[i] = ~(cells[i] ^ high-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)~((unsigned long)vm->cells[(int)i] ^ m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack dual clear + high-mask TOC: SCLRMNTOC · SCLRHNTOC · SHMASKTOC
   * (stack dual of SCLRMNTOCN/SCLRHNTOCN/SHMASKTOCN after inverted high-n plane) */
  if (kw(&L->cur,"SCLRMNTOC")||kw(&L->cur,"STACKCLRMNTOC")||kw(&L->cur,"SCLEARLNTOC")||
      kw(&L->cur,"CLRMNTOC")||kw(&L->cur,"SCELLCLRMNS")||kw(&L->cur,"SLOWCLRTOC")||
      kw(&L->cur,"CLEARLNTOC")||kw(&L->cur,"SCLRMNAT")){
    /* i n → cells[i] &= ~low-n mask; clear low n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (long)((unsigned long)vm->cells[(int)i] & ~m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRHNTOC")||kw(&L->cur,"STACKCLRHNTOC")||kw(&L->cur,"SCLEARHNTOC")||
      kw(&L->cur,"CLRHNTOC")||kw(&L->cur,"SCELLCLRHNS")||kw(&L->cur,"SHIGHCLRTOC")||
      kw(&L->cur,"CLEARHNTOC")||kw(&L->cur,"SCLRHNAT")){
    /* i n → cells[i] &= ~high-n mask; clear high n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)((unsigned long)vm->cells[(int)i] & ~m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHMASKTOC")||kw(&L->cur,"STACKHMASKTOC")||kw(&L->cur,"SHIMASKTOC")||
      kw(&L->cur,"HMASKTOC")||kw(&L->cur,"SCELLHMASKS")||kw(&L->cur,"SHIGHMASKTOC")||
      kw(&L->cur,"SETHMASKTOC")||kw(&L->cur,"SHMASKAT")){
    /* i n → cells[i] = high-n-bit mask; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)m;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack dual low-n reverse/rotate TOC: SBREVTOC · SROLBTOC · SRORBTOC
   * (stack dual of SBREVTOCN/SROLBTOCN/SRORBTOCN after SCLRMNTOC plane) */
  if (kw(&L->cur,"SBREVTOC")||kw(&L->cur,"STACKBREVTOC")||kw(&L->cur,"SREVLOWTOC")||
      kw(&L->cur,"BREVTOC")||kw(&L->cur,"SCELLBREVNS")||kw(&L->cur,"SBITREVTOC")||
      kw(&L->cur,"BREVNTOC")||kw(&L->cur,"SBREVAT")){
    /* i n → reverse low n bits of cells[i]; high kept; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n > 0 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      unsigned long ra = 0;
      for (long k = 0; k < n; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)(((unsigned long)a & ~m) | (ra & m));
    } else if (n >= 64){
      unsigned long la = (unsigned long)a;
      unsigned long ra = 0;
      for (int k = 0; k < 64; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)ra;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROLBTOC")||kw(&L->cur,"STACKROLBTOC")||kw(&L->cur,"SROTLBTOC")||
      kw(&L->cur,"ROLBTOC")||kw(&L->cur,"SCELLROLBNS")||kw(&L->cur,"SLOWROLTOC")||
      kw(&L->cur,"ROLBNTOC")||kw(&L->cur,"SROLBAT")){
    /* i n → rotate left by 1 within low n bits of cells[i]; high kept; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      la = ((la << 1) | (la >> (unsigned)(n - 1))) & m;
      r = (long)(((unsigned long)a & ~m) | la);
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua << 1) | (ua >> 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORBTOC")||kw(&L->cur,"STACKRORBTOC")||kw(&L->cur,"SROTRBTOC")||
      kw(&L->cur,"RORBTOC")||kw(&L->cur,"SCELLRORBNS")||kw(&L->cur,"SLOWRORTOC")||
      kw(&L->cur,"RORBNTOC")||kw(&L->cur,"SRORBAT")){
    /* i n → rotate right by 1 within low n bits of cells[i]; high kept; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      la = ((la >> 1) | (la << (unsigned)(n - 1))) & m;
      r = (long)(((unsigned long)a & ~m) | la);
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua >> 1) | (ua << 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack dual high-n reverse/rotate TOC: SBREVHNTOC · SROLHNTOC · SRORHNTOC
   * (stack dual of SBREVHNTOCN/SROLHNTOCN/SRORHNTOCN; high dual of SBREVTOC plane) */
  if (kw(&L->cur,"SBREVHNTOC")||kw(&L->cur,"STACKBREVHNTOC")||kw(&L->cur,"SREVHIGHTOC")||
      kw(&L->cur,"BREVHNAT")||kw(&L->cur,"SCELLBREVHNS")||kw(&L->cur,"SBITREVHNTOC")||
      kw(&L->cur,"BREVHNTOCS")||kw(&L->cur,"SHIGHBREVTOC")){
    /* i n → reverse high n bits of cells[i]; low kept; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n > 0 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long ra = 0;
      for (long k = 0; k < n; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)(((unsigned long)a & ~m) | ((ra << sh) & m));
    } else if (n >= 64){
      unsigned long la = (unsigned long)a;
      unsigned long ra = 0;
      for (int k = 0; k < 64; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)ra;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROLHNTOC")||kw(&L->cur,"STACKROLHNTOC")||kw(&L->cur,"SROTLHNTOC")||
      kw(&L->cur,"ROLHNAT")||kw(&L->cur,"SCELLROLHNS")||kw(&L->cur,"SHIGHROLTOC")||
      kw(&L->cur,"ROLHNTOCS")||kw(&L->cur,"SROTLHNTOCS")){
    /* i n → rotate left by 1 within high n bits of cells[i]; low kept; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long fm = (1ul << (unsigned)n) - 1ul;
      la = ((la << 1) | (la >> (unsigned)(n - 1))) & fm;
      r = (long)(((unsigned long)a & ~m) | ((la << sh) & m));
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua << 1) | (ua >> 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORHNTOC")||kw(&L->cur,"STACKRORHNTOC")||kw(&L->cur,"SROTRHNTOC")||
      kw(&L->cur,"RORHNAT")||kw(&L->cur,"SCELLRORHNS")||kw(&L->cur,"SHIGHRORTOC")||
      kw(&L->cur,"RORHNTOCS")||kw(&L->cur,"SROTRHNTOCS")){
    /* i n → rotate right by 1 within high n bits of cells[i]; low kept; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long fm = (1ul << (unsigned)n) - 1ul;
      la = ((la >> 1) | (la << (unsigned)(n - 1))) & fm;
      r = (long)(((unsigned long)a & ~m) | ((la << sh) & m));
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua >> 1) | (ua << 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
