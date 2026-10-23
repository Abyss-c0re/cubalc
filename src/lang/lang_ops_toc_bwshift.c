/* CubalC lang — lang_ops_toc_bwshift.c
 * TOC bw/shift plane: SAND/OR/XOR · SNAND/NOR/XNOR · SANDN/ORN/XORN
 *                     SSHL/SHR/SAR · SROL/ROR (stack + imm)
 * Peel from toc (B_toc_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_toc_bwshift.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_toc_try_bwshift(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-1 stack↔cell bitwise accumulate: SANDTOC · SORTOC · SXORTOC
   * (single-index dual of range SANDCELL; names avoid SCELLAND/SANDCELL) */
  if (kw(&L->cur,"SANDTOC")||kw(&L->cur,"SANDTOCELL")||kw(&L->cur,"STACKANDTOC")||
      kw(&L->cur,"BANDTOC")||kw(&L->cur,"SANDAT")){
    /* i v → cells[i] &= v, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] & v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORTOC")||kw(&L->cur,"SORTOCELL")||kw(&L->cur,"STACKORTOC")||
      kw(&L->cur,"BORTOC")||kw(&L->cur,"SORAT")){
    /* i v → cells[i] |= v, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] | v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORTOC")||kw(&L->cur,"SXORTOCELL")||kw(&L->cur,"STACKXORTOC")||
      kw(&L->cur,"BXORTOC")||kw(&L->cur,"SXORAT")){
    /* i v → cells[i] ^= v, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] ^ v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 energy inverted bitwise TOC: SNANDTOC · SNORTOC · SXNORTOC (after SAND/OR/XOR TOC) */
  if (kw(&L->cur,"SNANDTOC")||kw(&L->cur,"SNANDTOCELL")||kw(&L->cur,"STACKNANDTOC")||
      kw(&L->cur,"BNANDTOC")||kw(&L->cur,"SNANDAT")){
    /* i v → cells[i] = ~(cells[i] & v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(vm->cells[(int)i] & v);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORTOC")||kw(&L->cur,"SNORTOCELL")||kw(&L->cur,"STACKNORTOC")||
      kw(&L->cur,"BNORTOC")||kw(&L->cur,"SNORAT")){
    /* i v → cells[i] = ~(cells[i] | v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(vm->cells[(int)i] | v);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORTOC")||kw(&L->cur,"SXNORTOCELL")||kw(&L->cur,"STACKXNORTOC")||
      kw(&L->cur,"BXNORTOC")||kw(&L->cur,"SEQUIVTOC")||kw(&L->cur,"SXNORAT")){
    /* i v → cells[i] = ~(cells[i] ^ v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(vm->cells[(int)i] ^ v);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 energy ANDN plane TOC: SANDNTOC · SORNTOC · SXORNTOC (cells[i] op ~v after SNAND/NOR/XNOR TOC) */
  if (kw(&L->cur,"SANDNTOC")||kw(&L->cur,"SANDNTOCELL")||kw(&L->cur,"STACKANDNTOC")||
      kw(&L->cur,"SBICTOC")||kw(&L->cur,"SANDNOTTOC")||kw(&L->cur,"SANDNAT")||
      kw(&L->cur,"BICTOC")){
    /* i v → cells[i] &= ~v, leave result (clear bits set in v) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] & ~v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORNTOC")||kw(&L->cur,"SORNTOCELL")||kw(&L->cur,"STACKORNTOC")||
      kw(&L->cur,"SORNOTTOC")||kw(&L->cur,"SORNAT")||kw(&L->cur,"ORNOTTOC")){
    /* i v → cells[i] |= ~v, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] | ~v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORNTOC")||kw(&L->cur,"SXORNTOCELL")||kw(&L->cur,"STACKXORNTOC")||
      kw(&L->cur,"SXORNOTTOC")||kw(&L->cur,"SXORNAT")||kw(&L->cur,"XORNOTTOC")){
    /* i v → cells[i] ^= ~v, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] ^ ~v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack↔cell shift foundation: SSHLTOC · SSHRTOC · SSARTOC (after SSHL/SSHR/SSAR stack + bitwise TOC) */
  if (kw(&L->cur,"SSHLTOC")||kw(&L->cur,"SSHLTOCELL")||kw(&L->cur,"STACKSHLTOC")||
      kw(&L->cur,"SHLTOC")||kw(&L->cur,"SCELLSHL")||kw(&L->cur,"SSHLAT")){
    /* i n → cells[i] <<= n (n clamped 0..63), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long r = (long)((unsigned long)vm->cells[(int)i] << (unsigned)n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHRTOC")||kw(&L->cur,"SSHRTOCELL")||kw(&L->cur,"STACKSHRTOC")||
      kw(&L->cur,"SHRTOC")||kw(&L->cur,"SCELLSHR")||kw(&L->cur,"SSHRAT")){
    /* i n → cells[i] = logical cells[i] >> n (n clamped 0..63), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long r = (long)((unsigned long)vm->cells[(int)i] >> (unsigned)n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSARTOC")||kw(&L->cur,"SSARTOCELL")||kw(&L->cur,"STACKSARTOC")||
      kw(&L->cur,"SARTOC")||kw(&L->cur,"SCELLSAR")||kw(&L->cur,"SASHRTOC")||
      kw(&L->cur,"SSARAT")||kw(&L->cur,"ASHRTOC")){
    /* i n → cells[i] = arithmetic cells[i] >> n (sign-preserving; n clamped 0..63), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long r = vm->cells[(int)i] >> n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack foundation imm shift TOC: SSHLTOCN · SSHRTOCN · SSARTOCN
   * (imm dual of SSHLTOC/SSHRTOC/SSARTOC; peer of SSHLN/SSHRN/SSARN into cell) */
  if (kw(&L->cur,"SSHLTOCN")||kw(&L->cur,"SSHLTOCIMM")||kw(&L->cur,"STACKSHLTOCN")||
      kw(&L->cur,"SSHLATN")||kw(&L->cur,"SHLTOCN")||kw(&L->cur,"SCELLSHLN")){
    /* i + n → cells[i] <<= n (n clamped 0..63), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long r = (long)((unsigned long)vm->cells[(int)i] << (unsigned)n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHRTOCN")||kw(&L->cur,"SSHRTOCIMM")||kw(&L->cur,"STACKSHRTOCN")||
      kw(&L->cur,"SSHRATN")||kw(&L->cur,"SHRTOCN")||kw(&L->cur,"SCELLSHRN")){
    /* i + n → cells[i] logical >>= n (n clamped 0..63), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long r = (long)((unsigned long)vm->cells[(int)i] >> (unsigned)n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSARTOCN")||kw(&L->cur,"SSARTOCIMM")||kw(&L->cur,"STACKSARTOCN")||
      kw(&L->cur,"SSARATN")||kw(&L->cur,"SARTOCN")||kw(&L->cur,"SCELLSARN")||
      kw(&L->cur,"SASHRTOCN")||kw(&L->cur,"ASHRTOCN")){
    /* i + n → cells[i] arithmetic >>= n (n clamped 0..63), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long r = vm->cells[(int)i] >> n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack↔cell rotate dual: SROLTOC · SRORTOC (complete shift/rotate TOC after SSHL/SHR/SAR TOC) */
  if (kw(&L->cur,"SROLTOC")||kw(&L->cur,"SROLTOCELL")||kw(&L->cur,"STACKROLTOC")||
      kw(&L->cur,"ROLTOC")||kw(&L->cur,"SCELLROL")||kw(&L->cur,"SROTLTOC")||
      kw(&L->cur,"SROLAT")){
    /* i n → cells[i] = rotl(cells[i], n mod 64), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    unsigned uk = (unsigned)(n & 63);
    unsigned long u = (unsigned long)vm->cells[(int)i];
    long r = (uk == 0) ? (long)u : (long)((u << uk) | (u >> (64u - uk)));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORTOC")||kw(&L->cur,"SRORTOCELL")||kw(&L->cur,"STACKRORTOC")||
      kw(&L->cur,"RORTOC")||kw(&L->cur,"SCELLROR")||kw(&L->cur,"SROTRTOC")||
      kw(&L->cur,"SRORAT")){
    /* i n → cells[i] = rotr(cells[i], n mod 64), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    unsigned uk = (unsigned)(n & 63);
    unsigned long u = (unsigned long)vm->cells[(int)i];
    long r = (uk == 0) ? (long)u : (long)((u >> uk) | (u << (64u - uk)));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 imm rotate TOC: SROLTOCN · SRORTOCN
   * (imm dual of SROLTOC/SRORTOC; complete shift/rotate TOCN after SSHLTOCN plane) */
  if (kw(&L->cur,"SROLTOCN")||kw(&L->cur,"SROLTOCIMM")||kw(&L->cur,"STACKROLTOCN")||
      kw(&L->cur,"SROLATN")||kw(&L->cur,"ROLTOCN")||kw(&L->cur,"SCELLROLN")||
      kw(&L->cur,"SROTLTOCN")){
    /* i + n → cells[i] = rotl(cells[i], n mod 64), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    unsigned uk = (unsigned)(n & 63);
    unsigned long u = (unsigned long)vm->cells[(int)i];
    long r = (uk == 0) ? (long)u : (long)((u << uk) | (u >> (64u - uk)));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORTOCN")||kw(&L->cur,"SRORTOCIMM")||kw(&L->cur,"STACKRORTOCN")||
      kw(&L->cur,"SRORATN")||kw(&L->cur,"RORTOCN")||kw(&L->cur,"SCELLRORN")||
      kw(&L->cur,"SROTRTOCN")){
    /* i + n → cells[i] = rotr(cells[i], n mod 64), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    unsigned uk = (unsigned)(n & 63);
    unsigned long u = (unsigned long)vm->cells[(int)i];
    long r = (uk == 0) ? (long)u : (long)((u >> uk) | (u << (64u - uk)));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  return 0;
}
