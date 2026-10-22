/* CubalC lang — lang_ops_cell_matbits.c
 * CELL matrix bit plane: ROTBITS·SHLBITS/SHRBITS·SETBIT·COUNTBITS·GETBIT (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_matbits.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_matbits(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* ROTBITS cube k — rotate State Matrix bits left by k (digit-6 matrix flow) */
  if (kw(&L->cur,"ROTBITS")||kw(&L->cur,"ROLBITS")||kw(&L->cur,"SHIFTBITS")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ROTBITS cube k"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long k = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"ROTBITS missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n < 1) n = CUBALC_ATOM_BITS;
    if (k < 0){
      /* right rotate = left by n - (|k|%n) */
      long kk = (-k) % n;
      k = kk ? (n - kk) : 0;
    } else {
      k = k % n;
    }
    if (k){
      uint8_t tmp[(CUBALC_ATOM_BITS + 7) / 8];
      memset(tmp, 0, sizeof tmp);
      for (int i=0;i<n;i++){
        int src = cubalc_matrix_get(m, i);
        int dst = (int)((i + k) % n);
        if (src) tmp[dst >> 3] |= (uint8_t)(1u << (dst & 7));
      }
      cubalc_matrix_clear(m);
      m->n = (uint16_t)n;
      for (int i=0;i<n;i++){
        int on = (tmp[i >> 3] >> (i & 7)) & 1;
        if (on) cubalc_matrix_set(m, i, 1);
      }
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    var_set_num(vm, "SET", cubalc_matrix_popcount(m));
    var_set_num(vm, "DIGIT", vm->ch.cubes[ix].atom.digit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-3 COP matrix logical shift: SHLBITS/SHRBITS cube k (zero-fill, not rotate) */
  if (kw(&L->cur,"SHLBITS")||kw(&L->cur,"LSHBITS")||kw(&L->cur,"LSHIFTBITS")||
      kw(&L->cur,"SHRBITS")||kw(&L->cur,"RSHBITS")||kw(&L->cur,"RSHIFTBITS")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SHLBITS cube k"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long k = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"SHLBITS missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n < 1) n = CUBALC_ATOM_BITS;
    int left = (strcmp(op,"SHLBITS")==0 || strcmp(op,"LSHBITS")==0 ||
                strcmp(op,"LSHIFTBITS")==0);
    if (k < 0){
      /* negative shift flips direction */
      left = !left;
      k = -k;
    }
    if (k >= n){
      cubalc_matrix_clear(m);
      m->n = (uint16_t)n;
    } else if (k > 0){
      uint8_t tmp[(CUBALC_ATOM_BITS + 7) / 8];
      memset(tmp, 0, sizeof tmp);
      for (int i=0;i<n;i++){
        if (!cubalc_matrix_get(m, i)) continue;
        int dst = left ? i + (int)k : i - (int)k;
        if (dst < 0 || dst >= n) continue; /* drop overflow */
        tmp[dst >> 3] |= (uint8_t)(1u << (dst & 7));
      }
      cubalc_matrix_clear(m);
      m->n = (uint16_t)n;
      for (int i=0;i<n;i++){
        int on = (tmp[i >> 3] >> (i & 7)) & 1;
        if (on) cubalc_matrix_set(m, i, 1);
      }
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
  if (kw(&L->cur,"SETBIT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SETBIT cube i on"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int bit=(int)parse_expr(vm,L);
    int on=1;
    /* optional on/off expr (default 1) */
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
        L->cur.kind==TK_MINUS){
      on = parse_expr(vm,L) ? 1 : 0;
    }
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"SETBIT unknown cube"); return -1; }
    cubalc_matrix_set(&vm->ch.cubes[ix].atom.matrix, bit, on);
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    var_set_num(vm, "LAST_N", on ? 1 : 0); vm->last_n = on ? 1 : 0;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-0/5 COP matrix query: COUNTBITS/ONES/ZEROS cube · GETBIT cube i */
  if (kw(&L->cur,"COUNTBITS")||kw(&L->cur,"ONES")||kw(&L->cur,"POPBITS")||
      kw(&L->cur,"BITCOUNT")||kw(&L->cur,"ZEROS")||kw(&L->cur,"ZEROCOUNT")||
      kw(&L->cur,"NZERO")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"COUNTBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long ones = cubalc_matrix_popcount(m);
    long r = ones;
    if (strcmp(op,"ZEROS")==0 || strcmp(op,"ZEROCOUNT")==0 || strcmp(op,"NZERO")==0)
      r = (long)n - ones;
    var_set_num(vm, "SET", ones);
    var_set_num(vm, "LAST_N", r); vm->last_n = r;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"GETBIT")||kw(&L->cur,"BITAT")||kw(&L->cur,"MATBIT")||
      kw(&L->cur,"CUBEBIT")||kw(&L->cur,"READBIT")){
    /* note: TESTBIT is stack bitfield — use GETBIT for matrix */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"GETBIT cube i"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int bit = (int)parse_expr(vm,L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    long v = cubalc_matrix_get(&vm->ch.cubes[ix].atom.matrix, bit) ? 1 : 0;
    var_set_num(vm, "LAST_N", v); vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  return 0;
}
