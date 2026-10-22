/* CubalC lang — lang_ops_cell_matrel.c
 * CELL matrix relations: EQBITS/NEBITS/SUBSETBITS/SUPERSETBITS/DISJOINTBITS (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_matrel.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_matrel(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-5 COP matrix relations: EQBITS/NEBITS/SUBSETBITS/SUPERSETBITS/DISJOINTBITS */
  if (kw(&L->cur,"EQBITS")||kw(&L->cur,"SAMEBITS")||kw(&L->cur,"EQUALBITS")||
      kw(&L->cur,"NEBITS")||kw(&L->cur,"NEQBITS")||kw(&L->cur,"DIFFERSBITS")||
      kw(&L->cur,"SUBSETBITS")||kw(&L->cur,"CONTAINEDBITS")||kw(&L->cur,"ISSUBSET")||
      kw(&L->cur,"SUPERSETBITS")||kw(&L->cur,"COVERSBITS")||kw(&L->cur,"ISSUPERSET")||
      kw(&L->cur,"DISJOINTBITS")||kw(&L->cur,"NODJBITS")||kw(&L->cur,"INTERSECT0")||
      kw(&L->cur,"OVERLAPBITS")||kw(&L->cur,"INTERSECTBITS")){
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"EQBITS a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"EQBITS a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    int ia=find_cube(vm,a), ib=find_cube(vm,b);
    if (ia<0 || ib<0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1;
    }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    int na = ma->n > 0 ? ma->n : CUBALC_ATOM_BITS;
    int nb = mb->n > 0 ? mb->n : CUBALC_ATOM_BITS;
    if (na > CUBALC_ATOM_BITS) na = CUBALC_ATOM_BITS;
    if (nb > CUBALC_ATOM_BITS) nb = CUBALC_ATOM_BITS;
    int nn = na > nb ? na : nb;
    if (nn < 1) nn = CUBALC_ATOM_BITS;
    int eq = 1, subset = 1, superset = 1, overlap = 0;
    for (int i=0;i<nn;i++){
      int ba = cubalc_matrix_get(ma, i) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, i) ? 1 : 0;
      if (ba != bb) eq = 0;
      if (ba && !bb) subset = 0;   /* a bit not in b */
      if (bb && !ba) superset = 0; /* b bit not in a */
      if (ba && bb) overlap = 1;
    }
    long r = 0;
    if (strcmp(op,"EQBITS")==0 || strcmp(op,"SAMEBITS")==0 || strcmp(op,"EQUALBITS")==0)
      r = eq ? 1 : 0;
    else if (strcmp(op,"NEBITS")==0 || strcmp(op,"NEQBITS")==0 || strcmp(op,"DIFFERSBITS")==0)
      r = eq ? 0 : 1;
    else if (strcmp(op,"SUBSETBITS")==0 || strcmp(op,"CONTAINEDBITS")==0 ||
             strcmp(op,"ISSUBSET")==0)
      r = subset ? 1 : 0;
    else if (strcmp(op,"SUPERSETBITS")==0 || strcmp(op,"COVERSBITS")==0 ||
             strcmp(op,"ISSUPERSET")==0)
      r = superset ? 1 : 0;
    else if (strcmp(op,"DISJOINTBITS")==0 || strcmp(op,"NODJBITS")==0 ||
             strcmp(op,"INTERSECT0")==0)
      r = overlap ? 0 : 1;
    else
      /* OVERLAPBITS / INTERSECTBITS */
      r = overlap ? 1 : 0;
    var_set_num(vm, "LAST_N", r); vm->last_n = r;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  return 0;
}
