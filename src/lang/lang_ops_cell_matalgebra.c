/* CubalC lang — lang_ops_cell_matalgebra.c
 * CELL range algebra: ANDRANGE/ORRANGE/XORRANGE (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_matalgebra.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_matalgebra(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-3 COP range algebra: ANDRANGE/ORRANGE/XORRANGE a b lo hi
   * In-place: a[i] = a[i] OP b[i] for i in [lo..hi]; outside untouched.
   * ANDREDUCE/ORREDUCE cube lo hi → LAST_N fold of bits in range. */
  if (kw(&L->cur,"ANDRANGE")||kw(&L->cur,"ORRANGE")||kw(&L->cur,"XORRANGE")||
      kw(&L->cur,"NANDRANGE")||kw(&L->cur,"NORRANGE")||kw(&L->cur,"XNORRANGE")||
      kw(&L->cur,"ANDNRANGE")||kw(&L->cur,"ORNRANGE")){
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ANDRANGE a b lo hi"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ANDRANGE a b lo hi"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    int lo = (int)parse_expr(vm,L);
    int hi = (int)parse_expr(vm,L);
    ensure_world(vm);
    int ia=find_cube(vm,a); if (ia<0){ place_cube(vm,a,a,1); ia=find_cube(vm,a); }
    int ib=find_cube(vm,b); if (ib<0){ place_cube(vm,b,b,1); ib=find_cube(vm,b); }
    if (ia<0||ib<0){ fail(vm,"ANDRANGE cube missing"); return -1; }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (ma->n < (uint16_t)n) ma->n = (uint16_t)n;
    if (lo < 0) lo = 0;
    if (hi < lo){ int t=lo; lo=hi; hi=t; }
    if (lo >= n){ var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (hi >= n) hi = n - 1;
    for (int i=lo;i<=hi;i++){
      int ba = cubalc_matrix_get(ma, i) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, i) ? 1 : 0;
      int r = ba;
      if (strcmp(op,"ANDRANGE")==0) r = ba & bb;
      else if (strcmp(op,"ORRANGE")==0) r = ba | bb;
      else if (strcmp(op,"XORRANGE")==0) r = ba ^ bb;
      else if (strcmp(op,"NANDRANGE")==0) r = (ba & bb) ? 0 : 1;
      else if (strcmp(op,"NORRANGE")==0) r = (ba | bb) ? 0 : 1;
      else if (strcmp(op,"XNORRANGE")==0) r = (ba ^ bb) ? 0 : 1;
      else if (strcmp(op,"ANDNRANGE")==0) r = ba & (bb ? 0 : 1);
      else if (strcmp(op,"ORNRANGE")==0) r = ba | (bb ? 0 : 1);
      cubalc_matrix_set(ma, i, r ? 1 : 0);
    }
    vm->ch.cubes[ia].atom.digit_lock = 0;
    vm->ch.cubes[ia].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ia].atom.matrix);
    vm->ch.cubes[ia].flowed = 1;
    long ones = cubalc_matrix_popcount(ma);
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",ones); vm->last_n=ones;
    var_set_num(vm,"DIGIT",vm->ch.cubes[ia].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"ANDREDUCE")||kw(&L->cur,"ORREDUCE")||kw(&L->cur,"ALLRANGE")||
      kw(&L->cur,"ANYRANGE")||kw(&L->cur,"ANDFOLD")||kw(&L->cur,"ORFOLD")){
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_and = (strcmp(op,"ANDREDUCE")==0 || strcmp(op,"ALLRANGE")==0 ||
                  strcmp(op,"ANDFOLD")==0);
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ANDREDUCE cube lo hi"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int lo = (int)parse_expr(vm,L);
    int hi = (int)parse_expr(vm,L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (lo < 0) lo = 0;
    if (hi < lo){ int t=lo; lo=hi; hi=t; }
    if (lo >= n){
      long empty = is_and ? 1 : 0;
      var_set_num(vm,"LAST_N",empty); vm->last_n=empty;
      var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    if (hi >= n) hi = n - 1;
    long r;
    if (is_and){
      r = 1;
      for (int i=lo;i<=hi;i++){
        if (!cubalc_matrix_get(m, i)){ r = 0; break; }
      }
    } else {
      r = 0;
      for (int i=lo;i<=hi;i++){
        if (cubalc_matrix_get(m, i)){ r = 1; break; }
      }
    }
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  return 0;
}
