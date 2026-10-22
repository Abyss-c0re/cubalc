/* CubalC lang — lang_ops_cell_matmetric.c
 * CELL matrix metrics: DOTBITS·CLZ/CTZ·ORPOP·JACCARD·MAJ/THRESH·GRAY (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_matmetric.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_matmetric(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-9 COP metrics: DOTBITS · MAJBITS/THRESHBITS · GRAYBITS/UNGRAYBITS */
  if (kw(&L->cur,"DOTBITS")||kw(&L->cur,"ANDPOP")||kw(&L->cur,"ANDCOUNT")||
      kw(&L->cur,"BITDOT")||kw(&L->cur,"OVERLAPCOUNT")){
    /* DOTBITS a b → LAST_N = popcount(a AND b) */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DOTBITS a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DOTBITS a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    int ia=find_cube(vm,a), ib=find_cube(vm,b);
    if (ia<0 || ib<0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1;
    }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    int na = ma->n > 0 ? ma->n : CUBALC_ATOM_BITS;
    int nb = mb->n > 0 ? mb->n : CUBALC_ATOM_BITS;
    if (na > CUBALC_ATOM_BITS) na = CUBALC_ATOM_BITS;
    if (nb > CUBALC_ATOM_BITS) nb = CUBALC_ATOM_BITS;
    int nn = na > nb ? na : nb;
    if (nn < 1) nn = CUBALC_ATOM_BITS;
    long d = 0;
    for (int k=0;k<nn;k++){
      int ba = cubalc_matrix_get(ma, k) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, k) ? 1 : 0;
      if (ba && bb) d++;
    }
    var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"SET",d);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-9 COP metrics: CLZBITS/CTZBITS · ORPOP/UNIONPOP · JACCARD */
  if (kw(&L->cur,"CLZBITS")||kw(&L->cur,"NLZBITS")||kw(&L->cur,"LEADINGZEROS")||
      kw(&L->cur,"CTZBITS")||kw(&L->cur,"NTZBITS")||kw(&L->cur,"TRAILINGZEROS")){
    /* CLZBITS cube → consecutive zeros from high end; CTZBITS from low end.
     * All-zero → n. Empty/missing → OK=0 LAST_N=-1. */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_clz = (strcmp(op,"CLZBITS")==0 || strcmp(op,"NLZBITS")==0 ||
                  strcmp(op,"LEADINGZEROS")==0);
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"CLZBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n < 1) n = CUBALC_ATOM_BITS;
    long r = 0;
    if (is_clz){
      for (int i=n-1;i>=0;i--){
        if (cubalc_matrix_get(m, i)) break;
        r++;
      }
    } else {
      for (int i=0;i<n;i++){
        if (cubalc_matrix_get(m, i)) break;
        r++;
      }
    }
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"ORPOP")||kw(&L->cur,"UNIONPOP")||kw(&L->cur,"ORCOUNT")||
      kw(&L->cur,"BITUNION")||kw(&L->cur,"UNIONBITS")){
    /* ORPOP a b → popcount(a OR b) */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ORPOP a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ORPOP a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    int ia=find_cube(vm,a), ib=find_cube(vm,b);
    if (ia<0 || ib<0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1;
    }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    int na = ma->n > 0 ? ma->n : CUBALC_ATOM_BITS;
    int nb = mb->n > 0 ? mb->n : CUBALC_ATOM_BITS;
    if (na > CUBALC_ATOM_BITS) na = CUBALC_ATOM_BITS;
    if (nb > CUBALC_ATOM_BITS) nb = CUBALC_ATOM_BITS;
    int nn = na > nb ? na : nb;
    if (nn < 1) nn = CUBALC_ATOM_BITS;
    long d = 0;
    for (int k=0;k<nn;k++){
      int ba = cubalc_matrix_get(ma, k) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, k) ? 1 : 0;
      if (ba || bb) d++;
    }
    var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"SET",d);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"JACCARD")||kw(&L->cur,"JACCARD100")||kw(&L->cur,"SIMBITS")||
      kw(&L->cur,"BITJACCARD")||kw(&L->cur,"OVERLAPRATIO")){
    /* JACCARD a b → 100 * |A∩B| / |A∪B| (integer percent); empty union → 0 */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"JACCARD a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"JACCARD a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    int ia=find_cube(vm,a), ib=find_cube(vm,b);
    if (ia<0 || ib<0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1;
    }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    int na = ma->n > 0 ? ma->n : CUBALC_ATOM_BITS;
    int nb = mb->n > 0 ? mb->n : CUBALC_ATOM_BITS;
    if (na > CUBALC_ATOM_BITS) na = CUBALC_ATOM_BITS;
    if (nb > CUBALC_ATOM_BITS) nb = CUBALC_ATOM_BITS;
    int nn = na > nb ? na : nb;
    if (nn < 1) nn = CUBALC_ATOM_BITS;
    long inter = 0, uni = 0;
    for (int k=0;k<nn;k++){
      int ba = cubalc_matrix_get(ma, k) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, k) ? 1 : 0;
      if (ba && bb) inter++;
      if (ba || bb) uni++;
    }
    long r = 0;
    if (uni > 0) r = (inter * 100) / uni;
    var_set_num(vm,"SET",inter);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  return 0;
}
