/* CubalC lang — lang_ops_cell_matword.c
 * CELL matrix↔word: WORDFROM/WORDTO · EXTRACTBITS/DEPOSITBITS (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_matword.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_matword(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-8 COP matrix↔word bridge: WORDFROM/WORDTO · EXTRACTBITS/DEPOSITBITS */
  if (kw(&L->cur,"WORDFROM")||kw(&L->cur,"MAT2WORD")||kw(&L->cur,"BITS2WORD")||
      kw(&L->cur,"BITS2N")||kw(&L->cur,"LOADWORD")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"WORDFROM cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    /* pack bits 0..min(63,n-1) into unsigned 64-bit value (signed long storage) */
    int w = n < 64 ? n : 64;
    unsigned long long u = 0;
    for (int i=0;i<w;i++){
      if (cubalc_matrix_get(m, i)) u |= (1ULL << i);
    }
    long r = (long)u;
    var_set_num(vm, "LAST_N", r); vm->last_n = r;
    var_set_num(vm, "SET", cubalc_matrix_popcount(m));
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"WORDTO")||kw(&L->cur,"WORD2MAT")||kw(&L->cur,"N2BITS")||
      kw(&L->cur,"STOREWORD")||kw(&L->cur,"WORD2BITS")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"WORDTO cube n"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long v = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"WORDTO missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    unsigned long long u = (unsigned long long)v;
    for (int i=0;i<64 && i<n;i++){
      cubalc_matrix_set(m, i, (u >> i) & 1ULL ? 1 : 0);
    }
    /* leave bits >=64 untouched if any; atom is typically 64 */
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm, "SET", ones);
    var_set_num(vm, "LAST_N", v); vm->last_n = v;
    var_set_num(vm, "DIGIT", vm->ch.cubes[ix].atom.digit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"EXTRACTBITS")||kw(&L->cur,"GETBITS")||kw(&L->cur,"SLICEBITS")||
      kw(&L->cur,"BITFIELD")||kw(&L->cur,"FIELDGET")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"EXTRACTBITS cube lo hi"); return -1; }
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
    if (lo >= n){ var_set_num(vm,"LAST_N",0); vm->last_n=0; var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (hi >= n) hi = n - 1;
    /* width capped at 63 to keep signed long portable; full 64 via WORDFROM */
    int width = hi - lo + 1;
    if (width > 63) width = 63;
    unsigned long long u = 0;
    for (int i=0;i<width;i++){
      if (cubalc_matrix_get(m, lo + i)) u |= (1ULL << i);
    }
    long r = (long)u;
    var_set_num(vm, "LAST_N", r); vm->last_n = r;
    var_set_num(vm, "SET", cubalc_matrix_popcount(m));
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"DEPOSITBITS")||kw(&L->cur,"PUTBITS")||kw(&L->cur,"SETBITS")||
      kw(&L->cur,"FIELDSET")||kw(&L->cur,"INJECTBITS")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DEPOSITBITS cube lo hi val"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int lo = (int)parse_expr(vm,L);
    int hi = (int)parse_expr(vm,L);
    long val = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"DEPOSITBITS missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    if (lo < 0) lo = 0;
    if (hi < lo){ int t=lo; lo=hi; hi=t; }
    if (lo >= n){ var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (hi >= n) hi = n - 1;
    int width = hi - lo + 1;
    if (width > 63) width = 63;
    unsigned long long u = (unsigned long long)val;
    for (int i=0;i<width;i++){
      cubalc_matrix_set(m, lo + i, (u >> i) & 1ULL ? 1 : 0);
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm, "SET", ones);
    var_set_num(vm, "LAST_N", val); vm->last_n = val;
    var_set_num(vm, "DIGIT", vm->ch.cubes[ix].atom.digit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  return 0;
}
