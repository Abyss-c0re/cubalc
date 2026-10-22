/* CubalC lang — lang_ops_cell_matfind.c
 * CELL matrix find/run: MAXRUN·ZRUNS·FINDZERO/LASTZERO (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_matfind.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_matfind(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-9 COP metrics ext: MAXRUN · ZRUNS · FINDZERO/LASTZERO */
  if (kw(&L->cur,"MAXRUN")||kw(&L->cur,"LONGRUN")||kw(&L->cur,"LONGESTONES")||
      kw(&L->cur,"MAXONESRUN")||kw(&L->cur,"LONGEST1")){
    /* MAXRUN cube → length of longest contiguous 1-run (0 if no ones) */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MAXRUN cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long best = 0, cur = 0;
    for (int i=0;i<n;i++){
      if (cubalc_matrix_get(m, i)){ cur++; if (cur > best) best = cur; }
      else cur = 0;
    }
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"LAST_N",best); vm->last_n=best;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"ZRUNS")||kw(&L->cur,"ZERORUNS")||kw(&L->cur,"OFFRUNS")||
      kw(&L->cur,"GROUPZEROS")||kw(&L->cur,"ZEROGROUPS")){
    /* ZRUNS cube → number of maximal contiguous 0-runs */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ZRUNS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long runs = 0;
    int prev = 1; /* treat edge as "ones" so leading zeros start a run */
    for (int i=0;i<n;i++){
      int on = cubalc_matrix_get(m, i) ? 1 : 0;
      if (!on && prev) runs++;
      prev = on;
    }
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"LAST_N",runs); vm->last_n=runs;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"FINDZERO")||kw(&L->cur,"FIRSTZERO")||kw(&L->cur,"FIRST0")||
      kw(&L->cur,"FFZBIT")||kw(&L->cur,"LASTZERO")||kw(&L->cur,"LAST0")||
      kw(&L->cur,"FLZBIT")){
    /* FINDZERO cube → index of first 0, or -1 if all ones
     * LASTZERO cube → index of last 0, or -1 if all ones */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FINDZERO cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long ones = cubalc_matrix_popcount(m);
    long r = -1;
    int is_last = (strcmp(op,"LASTZERO")==0 || strcmp(op,"LAST0")==0 ||
                   strcmp(op,"FLZBIT")==0);
    if (is_last){
      for (int i=n-1;i>=0;i--){
        if (!cubalc_matrix_get(m, i)){ r = i; break; }
      }
    } else {
      for (int i=0;i<n;i++){
        if (!cubalc_matrix_get(m, i)){ r = i; break; }
      }
    }
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"MAJBITS")||kw(&L->cur,"MAJORITYBITS")||kw(&L->cur,"THRESHBITS")||
      kw(&L->cur,"VOTEBITS")||kw(&L->cur,"ONESGE")){
    /* MAJBITS cube [k] → LAST_N=1 if popcount >= k; default k = n/2+1 (strict majority)
     * THRESHBITS same with required k (if omitted, same majority default). */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MAJBITS cube [k]"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int have_k = 0;
    long kthr = 0;
    /* optional k: number / (expr) / unary minus only — bare IDENT is next statement */
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS){
      kthr = parse_expr(vm,L);
      have_k = 1;
    }
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n < 1) n = CUBALC_ATOM_BITS;
    long ones = cubalc_matrix_popcount(m);
    if (!have_k){
      kthr = (long)n / 2 + 1; /* strict majority */
    }
    if (kthr < 0) kthr = 0;
    long hit = (ones >= kthr) ? 1 : 0;
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",hit); vm->last_n=hit;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"GRAYBITS")||kw(&L->cur,"TOGRAY")||kw(&L->cur,"BIN2GRAY")||
      kw(&L->cur,"UNGRAYBITS")||kw(&L->cur,"FROMGRAY")||kw(&L->cur,"GRAY2BIN")||
      kw(&L->cur,"DEGRAYBITS")){
    /* GRAYBITS cube — binary→Gray: g = n^(n>>1); UNGRAYBITS Gray→binary */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int to_gray = (strcmp(op,"GRAYBITS")==0 || strcmp(op,"TOGRAY")==0 ||
                   strcmp(op,"BIN2GRAY")==0);
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"GRAYBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"GRAYBITS missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    if (n > 64) n = 64;
    unsigned long long u = 0;
    for (int i=0;i<n && i<64;i++){
      if (cubalc_matrix_get(m, i)) u |= (1ULL << i);
    }
    unsigned long long r;
    if (to_gray){
      r = u ^ (u >> 1);
    } else {
      /* Gray → binary: successive XOR of higher bits */
      r = u;
      for (int s=1;s<64;s<<=1) r ^= (r >> s);
    }
    for (int i=0;i<n && i<64;i++){
      cubalc_matrix_set(m, i, (r >> i) & 1ULL ? 1 : 0);
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",(long)r); vm->last_n=(long)r;
    var_set_num(vm,"DIGIT",vm->ch.cubes[ix].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  return 0;
}
