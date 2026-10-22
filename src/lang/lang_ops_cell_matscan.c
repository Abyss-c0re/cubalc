/* CubalC lang — lang_ops_cell_matscan.c
 * CELL matrix scan/reorder/distance: FINDONE·REVBITS·SWAPBIT·DIFFBITS (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_matscan.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_matscan(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-3 COP matrix scan: FINDONE/LASTONE cube · ANYBITS/ALLBITS cube */
  if (kw(&L->cur,"FINDONE")||kw(&L->cur,"FIRSTBIT")||kw(&L->cur,"FIRSTONE")||
      kw(&L->cur,"FFSBIT")||kw(&L->cur,"LASTONE")||kw(&L->cur,"LASTBIT")||
      kw(&L->cur,"FLSBIT")||kw(&L->cur,"ANYBITS")||kw(&L->cur,"ALLBITS")||
      kw(&L->cur,"NONEBITS")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FINDONE cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long ones = cubalc_matrix_popcount(m);
    long r = -1;
    if (strcmp(op,"ANYBITS")==0){
      r = ones > 0 ? 1 : 0;
    } else if (strcmp(op,"ALLBITS")==0){
      r = (ones >= n && n > 0) ? 1 : 0;
    } else if (strcmp(op,"NONEBITS")==0){
      r = ones == 0 ? 1 : 0;
    } else if (strcmp(op,"LASTONE")==0 || strcmp(op,"LASTBIT")==0 ||
               strcmp(op,"FLSBIT")==0){
      r = -1;
      for (int i=n-1;i>=0;i--){
        if (cubalc_matrix_get(m, i)){ r = i; break; }
      }
    } else {
      /* FINDONE / FIRSTBIT / FIRSTONE / FFSBIT */
      r = -1;
      for (int i=0;i<n;i++){
        if (cubalc_matrix_get(m, i)){ r = i; break; }
      }
    }
    var_set_num(vm, "SET", ones);
    var_set_num(vm, "LAST_N", r); vm->last_n = r;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-7 COP matrix reorder: REVBITS cube · SWAPBIT cube i j */
  if (kw(&L->cur,"REVBITS")||kw(&L->cur,"REVERSEBITS")||kw(&L->cur,"BITREVM")||
      kw(&L->cur,"MIRRORBITS")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"REVBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"REVBITS missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n < 1) n = CUBALC_ATOM_BITS;
    if (n > 1){
      uint8_t tmp[(CUBALC_ATOM_BITS + 7) / 8];
      memset(tmp, 0, sizeof tmp);
      for (int i=0;i<n;i++){
        if (!cubalc_matrix_get(m, i)) continue;
        int dst = n - 1 - i;
        tmp[dst >> 3] |= (uint8_t)(1u << (dst & 7));
      }
      cubalc_matrix_clear(m);
      m->n = (uint16_t)n;
      for (int i=0;i<n;i++){
        int on = (tmp[i >> 3] >> (i & 7)) & 1;
        if (on) cubalc_matrix_set(m, i, 1);
      }
    } else {
      m->n = (uint16_t)n;
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm, "SET", ones);
    var_set_num(vm, "LAST_N", ones); vm->last_n = ones;
    var_set_num(vm, "DIGIT", vm->ch.cubes[ix].atom.digit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SWAPBIT")||kw(&L->cur,"XCHGBIT")||kw(&L->cur,"EXCHBIT")||
      kw(&L->cur,"SWBIT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SWAPBIT cube i j"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int i = (int)parse_expr(vm,L);
    int j = (int)parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"SWAPBIT missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n < 1) n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    if (i >= 0 && j >= 0 && i < n && j < n && i != j){
      int bi = cubalc_matrix_get(m, i) ? 1 : 0;
      int bj = cubalc_matrix_get(m, j) ? 1 : 0;
      cubalc_matrix_set(m, i, bj);
      cubalc_matrix_set(m, j, bi);
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm, "SET", ones);
    var_set_num(vm, "LAST_N", ones); vm->last_n = ones;
    var_set_num(vm, "DIGIT", vm->ch.cubes[ix].atom.digit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-7 COP matrix distance: DIFFBITS/HAMBITS a b → LAST_N = popcount(a XOR b) */
  if (kw(&L->cur,"DIFFBITS")||kw(&L->cur,"HAMBITS")||kw(&L->cur,"HAMMINGBITS")||
      kw(&L->cur,"BITDIFF")||kw(&L->cur,"XORDIST")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DIFFBITS a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DIFFBITS a b"); return -1; }
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
      if (ba != bb) d++;
    }
    var_set_num(vm, "LAST_N", d); vm->last_n = d;
    var_set_num(vm, "SET", d);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  return 0;
}
