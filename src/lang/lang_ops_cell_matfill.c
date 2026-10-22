/* CubalC lang — lang_ops_cell_matfill.c
 * CELL matrix fill: FILLRANGE/CLEARRANGE/FLIPRANGE/COUNTRANGE (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_matfill.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_matfill(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-3 COP matrix range: FILLRANGE/CLEARRANGE/FLIPRANGE/COUNTRANGE cube lo hi [val] */
  if (kw(&L->cur,"FILLRANGE")||kw(&L->cur,"SETRANGE")||kw(&L->cur,"CLEARRANGE")||
      kw(&L->cur,"CLRRANGE")||kw(&L->cur,"ZERORANGE")||kw(&L->cur,"FLIPRANGE")||
      kw(&L->cur,"NOTRANGE")||kw(&L->cur,"INVERTRANGE")||kw(&L->cur,"COUNTRANGE")||
      kw(&L->cur,"ONESRANGE")||kw(&L->cur,"POPRANGE")){
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FILLRANGE cube lo hi"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int lo = (int)parse_expr(vm,L);
    int hi = (int)parse_expr(vm,L);
    int is_count = (strcmp(op,"COUNTRANGE")==0 || strcmp(op,"ONESRANGE")==0 ||
                    strcmp(op,"POPRANGE")==0);
    int is_clear = (strcmp(op,"CLEARRANGE")==0 || strcmp(op,"CLRRANGE")==0 ||
                    strcmp(op,"ZERORANGE")==0);
    int is_flip  = (strcmp(op,"FLIPRANGE")==0 || strcmp(op,"NOTRANGE")==0 ||
                    strcmp(op,"INVERTRANGE")==0);
    int val = 1;
    if (!is_count && !is_clear && !is_flip){
      /* FILLRANGE/SETRANGE optional val (default 1) */
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
          L->cur.kind==TK_MINUS)
        val = parse_expr(vm,L) ? 1 : 0;
    }
    if (is_clear) val = 0;
    if (is_count){
      int ix=find_cube(vm,id);
      if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
      cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
      int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
      if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
      if (lo < 0) lo = 0;
      if (hi < lo){ int t=lo; lo=hi; hi=t; }
      if (lo >= n){ var_set_num(vm,"LAST_N",0); vm->last_n=0; var_set_num(vm,"OK",1); bump(vm); return 1; }
      if (hi >= n) hi = n - 1;
      long c = 0;
      for (int i=lo;i<=hi;i++) if (cubalc_matrix_get(m, i)) c++;
      var_set_num(vm,"LAST_N",c); vm->last_n=c;
      var_set_num(vm,"SET",cubalc_matrix_popcount(m));
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"FILLRANGE missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    if (lo < 0) lo = 0;
    if (hi < lo){ int t=lo; lo=hi; hi=t; }
    if (lo >= n){ var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (hi >= n) hi = n - 1;
    for (int i=lo;i<=hi;i++){
      if (is_flip){
        int on = cubalc_matrix_get(m, i) ? 0 : 1;
        cubalc_matrix_set(m, i, on);
      } else {
        cubalc_matrix_set(m, i, val);
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
  return 0;
}
