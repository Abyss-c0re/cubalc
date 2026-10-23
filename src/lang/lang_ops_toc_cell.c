/* CubalC lang — lang_ops_toc_cell.c
 * TOC cell plane: CELLSET/GET + stack↔cell accumulate SADDTOC..SMAXTOC
 * Peel from toc (B_toc_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_toc_cell.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_toc_try_cell(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* ---- digit-1 data plane: cells + stack ---- */
  if (kw(&L->cur,"CELLSET")||kw(&L->cur,"SLOTSET")||kw(&L->cur,"STORE")||
      kw(&L->cur,"POKE")||kw(&L->cur,"PUTCELL")||kw(&L->cur,"SETCELL")){
    lex_next(L);
    /* SETCELL(i,v) paren form — expr-style mutator as statement */
    if (L->cur.kind==TK_LPAREN){
      lex_next(L);
      long i = parse_expr(vm,L);
      long v = 0;
      if (L->cur.kind==TK_COMMA){ lex_next(L); v = parse_expr(vm,L); }
      if (L->cur.kind==TK_RPAREN) lex_next(L);
      if (i < 0) i = 0;
      if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
      vm->cells[(int)i] = v;
      var_set_num(vm, "LAST_N", v);
      vm->last_n = v;
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    long i = parse_expr(vm,L);
    long v = 0;
    if (L->cur.kind==TK_EQ){ lex_next(L); v = parse_expr(vm,L); }
    else if (L->cur.kind!=TK_NL && L->cur.kind!=TK_EOF)
      v = parse_expr(vm,L);
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    vm->cells[(int)i] = v;
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CELLGET")||kw(&L->cur,"SLOTGET")||kw(&L->cur,"LOAD")){
    lex_next(L);
    long i = parse_expr(vm,L);
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i];
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SWAPCELL")||kw(&L->cur,"CELLSWAP")||kw(&L->cur,"SWAPSLOT")){
    lex_next(L);
    long i = parse_expr(vm,L);
    long j = parse_expr(vm,L);
    if (i < 0) i = 0; if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (j < 0) j = 0; if (j >= CUBALC_CELL_N) j = CUBALC_CELL_N - 1;
    long t = vm->cells[(int)i];
    vm->cells[(int)i] = vm->cells[(int)j];
    vm->cells[(int)j] = t;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-1 stack↔cell: SSETCELL SINCCELL SDECCELL SXCHGCELL */
  if (kw(&L->cur,"SSETCELL")||kw(&L->cur,"SSTORE")||kw(&L->cur,"SPUTCELL")||
      kw(&L->cur,"STACKSETCELL")||kw(&L->cur,"SPOKE")){
    /* i v → cells[i]=v, leave v */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    vm->cells[(int)i] = v;
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SINCCELL")||kw(&L->cur,"SINCELL")||kw(&L->cur,"STACKINCCELL")||
      kw(&L->cur,"SDECCELL")||kw(&L->cur,"SDECELL")||kw(&L->cur,"STACKDECCELL")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    int is_dec = (strcmp(op,"SDECCELL")==0 || strcmp(op,"SDECELL")==0 ||
                  strcmp(op,"STACKDECCELL")==0);
    if (is_dec) vm->cells[(int)i] -= 1;
    else vm->cells[(int)i] += 1;
    long r = vm->cells[(int)i];
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXCHGCELL")||kw(&L->cur,"SEXCHCELL")||kw(&L->cur,"SSWAPC")||
      kw(&L->cur,"STACKXCHGCELL")){
    /* i v → swap cells[i] with v, leave previous cell value */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long old = vm->cells[(int)i];
    vm->cells[(int)i] = v;
    vm->stack[vm->sp++] = old;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",old); vm->last_n=old;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack↔cell fetch: SGETCELL (complement of SSETCELL) */
  if (kw(&L->cur,"SGETCELL")||kw(&L->cur,"SLOAD")||kw(&L->cur,"SFETCH")||
      kw(&L->cur,"STACKGETCELL")||kw(&L->cur,"SPEEKCELL")){
    /* i → cells[i]  (stack index → cell value) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i];
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack↔cell accumulate: SADDTOC (i v → cells[i]+=v leave sum) */
  if (kw(&L->cur,"SADDTOC")||kw(&L->cur,"SCELLADD")||kw(&L->cur,"SACCUMCELL")||
      kw(&L->cur,"STACKADDCELL")||kw(&L->cur,"SADDTOCELL")){
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] + v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack↔cell accumulate ext: SSUBTOC · SMULTOC · SDIVTOC */
  if (kw(&L->cur,"SSUBTOC")||kw(&L->cur,"SCELLSUB")||kw(&L->cur,"SSUBFROMCELL")||
      kw(&L->cur,"STACKSUBCELL")||kw(&L->cur,"SSUBCELL")){
    /* i v → cells[i]-=v, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] - v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMULTOC")||kw(&L->cur,"SCELLMUL")||kw(&L->cur,"SMULCELL")||
      kw(&L->cur,"STACKMULCELL")||kw(&L->cur,"SMULTOCELL")){
    /* i v → cells[i]*=v, leave product */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] * v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDIVTOC")||kw(&L->cur,"SCELLDIV")||kw(&L->cur,"SDIVCELL")||
      kw(&L->cur,"STACKDIVCELL")||kw(&L->cur,"SDIVTOCELL")){
    /* i v → cells[i]/=v (0 if v==0), leave quotient */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = v ? (vm->cells[(int)i] / v) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack↔cell accumulate bound: SMODTOC · SMINTOC · SMAXTOC */
  if (kw(&L->cur,"SMODTOC")||kw(&L->cur,"SCELLMOD")||kw(&L->cur,"SMODCELL")||
      kw(&L->cur,"STACKMODCELL")||kw(&L->cur,"SMODTOCELL")){
    /* i v → cells[i]%=v (0 if v==0), leave remainder */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = v ? (vm->cells[(int)i] % v) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMINTOC")||kw(&L->cur,"SCELLMIN")||kw(&L->cur,"SMINCELL")||
      kw(&L->cur,"STACKMINCELL")||kw(&L->cur,"SMINTOCELL")){
    /* i v → cells[i]=min(cells[i],v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = (c < v) ? c : v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMAXTOC")||kw(&L->cur,"SCELLMAX")||kw(&L->cur,"SMAXCELL")||
      kw(&L->cur,"STACKMAXCELL")||kw(&L->cur,"SMAXTOCELL")){
    /* i v → cells[i]=max(cells[i],v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = (c > v) ? c : v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
