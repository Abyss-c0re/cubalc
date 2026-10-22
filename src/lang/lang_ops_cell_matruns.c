/* CubalC lang — lang_ops_cell_matruns.c
 * CELL matrix run metrics: TRANSBITS·RUNSBITS·MASKPOP (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_matruns.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_matruns(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-9 COP metrics: TRANSBITS · RUNSBITS · MASKPOP */
  if (kw(&L->cur,"TRANSBITS")||kw(&L->cur,"BITTRANS")||kw(&L->cur,"EDGESBITS")||
      kw(&L->cur,"TRANSITIONS")||kw(&L->cur,"BITEDGES")){
    /* TRANSBITS cube → count of adjacent bit flips (bit[i] XOR bit[i+1]) */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"TRANSBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long t = 0;
    for (int i=0;i+1<n;i++){
      int a = cubalc_matrix_get(m, i) ? 1 : 0;
      int b = cubalc_matrix_get(m, i+1) ? 1 : 0;
      if (a != b) t++;
    }
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"LAST_N",t); vm->last_n=t;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"RUNSBITS")||kw(&L->cur,"ONERUNS")||kw(&L->cur,"BITRUNS")||
      kw(&L->cur,"RUNCOUNT")||kw(&L->cur,"GROUPONES")){
    /* RUNSBITS cube → number of maximal contiguous 1-runs */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"RUNSBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long runs = 0;
    int prev = 0;
    for (int i=0;i<n;i++){
      int on = cubalc_matrix_get(m, i) ? 1 : 0;
      if (on && !prev) runs++;
      prev = on;
    }
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"LAST_N",runs); vm->last_n=runs;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"MASKPOP")||kw(&L->cur,"POPMASK")||kw(&L->cur,"ANDMASKPOP")||
      kw(&L->cur,"WEIGHTMASK")||kw(&L->cur,"MASKONES")){
    /* MASKPOP cube mask → popcount of cube bits under mask ones (mask is integer word) */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MASKPOP cube mask"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long maskv = parse_expr(vm,L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n > 64) n = 64;
    unsigned long long mask = (unsigned long long)maskv;
    long c = 0;
    for (int i=0;i<n;i++){
      if (((mask >> i) & 1ull) && cubalc_matrix_get(m, i)) c++;
    }
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"LAST_N",c); vm->last_n=c;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  return 0;
}
