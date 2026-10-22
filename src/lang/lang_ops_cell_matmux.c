/* CubalC lang — lang_ops_cell_matmux.c
 * CELL matrix mux/match: MUXBITS/BLENDBITS · MATCHBITS (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_matmux.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_matmux(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-5 COP matrix mux/match: MUXBITS/BLENDBITS dst a b mask · MATCHBITS a b mask */
  if (kw(&L->cur,"MUXBITS")||kw(&L->cur,"BLENDBITS")||kw(&L->cur,"SELECTBITS")||
      kw(&L->cur,"BITSMUX")||kw(&L->cur,"MERGEBITS")){
    /* MUXBITS dst a b mask: dst[i] = mask[i] ? a[i] : b[i] */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MUXBITS dst a b mask"); return -1; }
    char dst[48]; snprintf(dst,sizeof dst,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MUXBITS dst a b mask"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MUXBITS dst a b mask"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MUXBITS dst a b mask"); return -1; }
    char mk[48]; snprintf(mk,sizeof mk,"%s",L->cur.text); lex_next(L);
    ensure_world(vm);
    int id=find_cube(vm,dst); if (id<0){ place_cube(vm,dst,dst,1); id=find_cube(vm,dst); }
    int ia=find_cube(vm,a); if (ia<0){ place_cube(vm,a,a,1); ia=find_cube(vm,a); }
    int ib=find_cube(vm,b); if (ib<0){ place_cube(vm,b,b,1); ib=find_cube(vm,b); }
    int im=find_cube(vm,mk); if (im<0){ place_cube(vm,mk,mk,1); im=find_cube(vm,mk); }
    if (id<0||ia<0||ib<0||im<0){ fail(vm,"MUXBITS cube missing"); return -1; }
    cubalc_matrix *md = &vm->ch.cubes[id].atom.matrix;
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    cubalc_matrix *mm = &vm->ch.cubes[im].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    cubalc_matrix_clear(md);
    md->n = (uint16_t)n;
    long ones = 0;
    for (int i=0;i<n;i++){
      int on = cubalc_matrix_get(mm, i)
                 ? (cubalc_matrix_get(ma, i) ? 1 : 0)
                 : (cubalc_matrix_get(mb, i) ? 1 : 0);
      if (on){ cubalc_matrix_set(md, i, 1); ones++; }
    }
    vm->ch.cubes[id].atom.digit_lock = 0;
    vm->ch.cubes[id].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[id].atom.matrix);
    vm->ch.cubes[id].flowed = 1;
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",ones); vm->last_n=ones;
    var_set_num(vm,"DIGIT",vm->ch.cubes[id].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"MATCHBITS")||kw(&L->cur,"EQMASK")||kw(&L->cur,"MASKED_EQ")||
      kw(&L->cur,"EQUNDER")||kw(&L->cur,"BITSMATCH")){
    /* MATCHBITS a b mask → 1 if a and b agree on all mask=1 positions */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MATCHBITS a b mask"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MATCHBITS a b mask"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MATCHBITS a b mask"); return -1; }
    char mk[48]; snprintf(mk,sizeof mk,"%s",L->cur.text); lex_next(L);
    int ia=find_cube(vm,a), ib=find_cube(vm,b), im=find_cube(vm,mk);
    if (ia<0 || ib<0 || im<0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1;
    }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    cubalc_matrix *mm = &vm->ch.cubes[im].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    int ok = 1;
    long checked = 0;
    for (int i=0;i<n;i++){
      if (!cubalc_matrix_get(mm, i)) continue;
      checked++;
      int ba = cubalc_matrix_get(ma, i) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, i) ? 1 : 0;
      if (ba != bb){ ok = 0; break; }
    }
    var_set_num(vm,"SET",checked);
    var_set_num(vm,"LAST_N",ok ? 1 : 0); vm->last_n = ok ? 1 : 0;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  return 0;
}
