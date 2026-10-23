/* CubalC lang — lang_ops_toc_alu.c
 * TOC ALU plane: SNAND/NOR/XNOR TOCN · SANDN/ORN/XORN TOCN
 *                SDIV/SMOD/SMIN/SMAX TOCN · unsigned DIV/MOD/MIN/MAX imm+stack
 * Peel from toc (B_toc_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_toc_alu.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_toc_try_alu(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-0 foundation imm inverted bitwise TOC: SNANDTOCN · SNORTOCN · SXNORTOCN
   * (imm dual of SNANDTOC/SNORTOC/SXNORTOC after SANDTOCN plane) */
  if (kw(&L->cur,"SNANDTOCN")||kw(&L->cur,"SNANDTOCIMM")||kw(&L->cur,"STACKNANDTOCN")||
      kw(&L->cur,"SNANDATN")||kw(&L->cur,"NANDTOCN")||kw(&L->cur,"SCELLNANDN")||
      kw(&L->cur,"BNANDTOCN")){
    /* i + n → cells[i] = ~(cells[i] & n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(vm->cells[(int)i] & n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORTOCN")||kw(&L->cur,"SNORTOCIMM")||kw(&L->cur,"STACKNORTOCN")||
      kw(&L->cur,"SNORATN")||kw(&L->cur,"NORTOCN")||kw(&L->cur,"SCELLNORN")||
      kw(&L->cur,"BNORTOCN")){
    /* i + n → cells[i] = ~(cells[i] | n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(vm->cells[(int)i] | n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORTOCN")||kw(&L->cur,"SXNORTOCIMM")||kw(&L->cur,"STACKXNORTOCN")||
      kw(&L->cur,"SXNORATN")||kw(&L->cur,"XNORTOCN")||kw(&L->cur,"SCELLXNORN")||
      kw(&L->cur,"BXNORTOCN")||kw(&L->cur,"SEQUIVTOCN")){
    /* i + n → cells[i] = ~(cells[i] ^ n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(vm->cells[(int)i] ^ n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 imm ANDN-plane TOC: SANDNTOCN · SORNTOCN · SXORNTOCN
   * (imm dual of SANDNTOC/SORNTOC/SXORNTOC after SNANDTOCN; BIC/ORNOT/XORNOT into cell) */
  if (kw(&L->cur,"SANDNTOCN")||kw(&L->cur,"SANDNTOCIMM")||kw(&L->cur,"STACKANDNTOCN")||
      kw(&L->cur,"SANDNATN")||kw(&L->cur,"ANDNTOCN")||kw(&L->cur,"SBICTOCN")||
      kw(&L->cur,"BICTOCN")||kw(&L->cur,"SCELLANDNN")){
    /* i + n → cells[i] &= ~n, leave result (clear bits set in n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] & ~n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORNTOCN")||kw(&L->cur,"SORNTOCIMM")||kw(&L->cur,"STACKORNTOCN")||
      kw(&L->cur,"SORNATN")||kw(&L->cur,"ORNTOCN")||kw(&L->cur,"SORNOTTOCN")||
      kw(&L->cur,"SCELLORNN")){
    /* i + n → cells[i] |= ~n, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] | ~n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORNTOCN")||kw(&L->cur,"SXORNTOCIMM")||kw(&L->cur,"STACKXORNTOCN")||
      kw(&L->cur,"SXORNATN")||kw(&L->cur,"XORNTOCN")||kw(&L->cur,"SXORNOTTOCN")||
      kw(&L->cur,"SCELLXORNN")){
    /* i + n → cells[i] ^= ~n, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] ^ ~n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 imm accumulate bound TOC: SDIVTOCN · SMODTOCN · SMINTOCN · SMAXTOCN
   * (complete SADDTOCN/SMULTOCN with / % min max; imm dual of SDIVTOC/SMODTOC/SMINTOC/SMAXTOC) */
  if (kw(&L->cur,"SDIVTOCN")||kw(&L->cur,"SDIVTOCIMM")||kw(&L->cur,"STACKDIVTOCN")||
      kw(&L->cur,"SDIVATN")||kw(&L->cur,"DIVTOCN")||kw(&L->cur,"SCELLDIVN")||
      kw(&L->cur,"QUOTTOCN")){
    /* i + n → cells[i]/=n (n==0 → 0 soft), leave quotient */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = n ? (vm->cells[(int)i] / n) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODTOCN")||kw(&L->cur,"SMODTOCIMM")||kw(&L->cur,"STACKMODTOCN")||
      kw(&L->cur,"SMODATN")||kw(&L->cur,"MODTOCN")||kw(&L->cur,"SCELLMODN")||
      kw(&L->cur,"REMTOCN")){
    /* i + n → cells[i]%=n (n==0 → 0 soft), leave remainder */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = n ? (vm->cells[(int)i] % n) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMINTOCN")||kw(&L->cur,"SMINTOCIMM")||kw(&L->cur,"STACKMINTOCN")||
      kw(&L->cur,"SMINATN")||kw(&L->cur,"MINTOCN")||kw(&L->cur,"SCELLMINN")){
    /* i + n → cells[i]=min(cells[i],n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = (c < n) ? c : n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMAXTOCN")||kw(&L->cur,"SMAXTOCIMM")||kw(&L->cur,"STACKMAXTOCN")||
      kw(&L->cur,"SMAXATN")||kw(&L->cur,"MAXTOCN")||kw(&L->cur,"SCELLMAXN")){
    /* i + n → cells[i]=max(cells[i],n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = (c > n) ? c : n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 foundation forward unsigned imm TOC: SUDIVTOCN · SUMODTOCN · SUMINTOCN · SUMAXTOCN
   * (unsigned peer of SDIVTOCN/SMODTOCN/SMINTOCN/SMAXTOCN; imm dual of SUDIVN/SUMINN after reverse unsigned TOC) */
  if (kw(&L->cur,"SUDIVTOCN")||kw(&L->cur,"SUDIVTOCIMM")||kw(&L->cur,"STACKUDIVTOCN")||
      kw(&L->cur,"SUDIVATN")||kw(&L->cur,"UDIVTOCN")||kw(&L->cur,"SCELLUDIVN")||
      kw(&L->cur,"UQUOTTOCN")){
    /* i + n → cells[i] = (u)cells[i] / (u)n; n==0 → 0 soft; leave quotient */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = 0;
    if (n != 0) r = (long)((unsigned long)vm->cells[(int)i] / (unsigned long)n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMODTOCN")||kw(&L->cur,"SUMODTOCIMM")||kw(&L->cur,"STACKUMODTOCN")||
      kw(&L->cur,"SUMODATN")||kw(&L->cur,"UMODTOCN")||kw(&L->cur,"SCELLUMODN")||
      kw(&L->cur,"UREMTOCN")||kw(&L->cur,"SUREMTOCN")){
    /* i + n → cells[i] = (u)cells[i] % (u)n; n==0 → 0 soft; leave remainder */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = 0;
    if (n != 0) r = (long)((unsigned long)vm->cells[(int)i] % (unsigned long)n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMINTOCN")||kw(&L->cur,"SUMINTOCIMM")||kw(&L->cur,"STACKUMINTOCN")||
      kw(&L->cur,"SUMINATN")||kw(&L->cur,"UMINTOCN")||kw(&L->cur,"SCELLUMINN")||
      kw(&L->cur,"UMINTOCN")){
    /* i + n → cells[i]=umin(cells[i],n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    unsigned long un = (unsigned long)n;
    long r = (long)((c < un) ? c : un);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMAXTOCN")||kw(&L->cur,"SUMAXTOCIMM")||kw(&L->cur,"STACKUMAXTOCN")||
      kw(&L->cur,"SUMAXATN")||kw(&L->cur,"UMAXTOCN")||kw(&L->cur,"SCELLUMAXN")||
      kw(&L->cur,"UMAXTOCN")){
    /* i + n → cells[i]=umax(cells[i],n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    unsigned long un = (unsigned long)n;
    long r = (long)((c > un) ? c : un);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 foundation stack↔cell unsigned TOC: SUDIVTOC · SUMODTOC · SUMINTOC · SUMAXTOC
   * (stack dual of SUDIVTOCN/SUMODTOCN/SUMINTOCN/SUMAXTOCN; unsigned peer of SDIVTOC/SMODTOC/SMINTOC/SMAXTOC) */
  if (kw(&L->cur,"SUDIVTOC")||kw(&L->cur,"SCELLUDIV")||kw(&L->cur,"STACKUDIVTOC")||
      kw(&L->cur,"SUDIVTOCELL")||kw(&L->cur,"UDIVTOC")||kw(&L->cur,"SUQUOTTOC")){
    /* i v → cells[i] = (u)cells[i] / (u)v; v==0 → 0 soft; leave quotient */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = 0;
    if (v != 0) r = (long)((unsigned long)vm->cells[(int)i] / (unsigned long)v);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMODTOC")||kw(&L->cur,"SCELLUMOD")||kw(&L->cur,"STACKUMODTOC")||
      kw(&L->cur,"SUMODTOCELL")||kw(&L->cur,"UMODTOC")||kw(&L->cur,"SUREMTOC")||
      kw(&L->cur,"UREMTOC")){
    /* i v → cells[i] = (u)cells[i] % (u)v; v==0 → 0 soft; leave remainder */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = 0;
    if (v != 0) r = (long)((unsigned long)vm->cells[(int)i] % (unsigned long)v);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMINTOC")||kw(&L->cur,"SCELLUMIN")||kw(&L->cur,"STACKUMINTOC")||
      kw(&L->cur,"SUMINTOCELL")||kw(&L->cur,"UMINTOC")||kw(&L->cur,"SUMINTO")){
    /* i v → cells[i]=umin(cells[i],v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    unsigned long uv = (unsigned long)v;
    long r = (long)((c < uv) ? c : uv);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMAXTOC")||kw(&L->cur,"SCELLUMAX")||kw(&L->cur,"STACKUMAXTOC")||
      kw(&L->cur,"SUMAXTOCELL")||kw(&L->cur,"UMAXTOC")||kw(&L->cur,"SUMAXTO")){
    /* i v → cells[i]=umax(cells[i],v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    unsigned long uv = (unsigned long)v;
    long r = (long)((c > uv) ? c : uv);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  return 0;
}
