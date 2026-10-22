/* CubalC lang — lang_ops_cell_matlogic.c
 * CELL matrix logic: CLEAR/NOT/COPY/AND/OR/XOR/NAND/XNOR/NOR/ANDN/ORN (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_matlogic.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_matlogic(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane, -1 fail. */
  /* COP matrix algebra (digit-5/7): CLEAR|NOT|COPY|AND|OR|XOR|NAND|XNOR|NOR|ANDN|ORN */
  if (kw(&L->cur,"CLEARBITS")||kw(&L->cur,"ZEROBITS")||kw(&L->cur,"CLRBITS")||
      kw(&L->cur,"NOTBITS")||kw(&L->cur,"INVERTBITS")||kw(&L->cur,"FLIPBITS")||
      kw(&L->cur,"COPYBITS")||kw(&L->cur,"CLONEBITS")||
      kw(&L->cur,"ANDBITS")||kw(&L->cur,"ORBITS")||kw(&L->cur,"XORBITS")||
      kw(&L->cur,"NANDBITS")||kw(&L->cur,"FILLBITS")||
      kw(&L->cur,"XNORBITS")||kw(&L->cur,"EQVBITS")||kw(&L->cur,"NXORBITS")||
      kw(&L->cur,"NORBITS")||kw(&L->cur,"ANDNBITS")||kw(&L->cur,"ORNBITS")||
      kw(&L->cur,"BICBITS")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op; *p; p++) if (*p>='a'&&*p<='z') *p = (char)(*p - 'a' + 'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"matrix op needs cube id"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    char b[48]={0};
    if (L->cur.kind==TK_IDENT){ snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L); }
    ensure_world(vm);
    int ia=find_cube(vm,a);
    if (ia<0){ place_cube(vm,a,a,1); ia=find_cube(vm,a); }
    if (ia<0){ fail(vm,"matrix op cube missing"); return -1; }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    int n = ma->n > 0 ? ma->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;

    if (strcmp(op,"CLEARBITS")==0 || strcmp(op,"ZEROBITS")==0 || strcmp(op,"CLRBITS")==0){
      cubalc_matrix_clear(ma);
      ma->n = (uint16_t)CUBALC_ATOM_BITS;
    } else if (strcmp(op,"FILLBITS")==0){
      int on = 1;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS)
        on = parse_expr(vm,L) ? 1 : 0;
      else if (b[0] && (b[0]=='0' || b[0]=='1') && b[1]==0) on = b[0]=='1';
      cubalc_matrix_clear(ma);
      ma->n = (uint16_t)CUBALC_ATOM_BITS;
      for (int i=0;i<CUBALC_ATOM_BITS;i++) cubalc_matrix_set(ma, i, on);
    } else if (strcmp(op,"NOTBITS")==0 || strcmp(op,"INVERTBITS")==0 || strcmp(op,"FLIPBITS")==0){
      if (ma->n == 0) ma->n = (uint16_t)CUBALC_ATOM_BITS;
      n = ma->n;
      for (int i=0;i<n;i++)
        cubalc_matrix_set(ma, i, cubalc_matrix_get(ma, i) ? 0 : 1);
    } else {
      /* binary: dst op= src  (COPYBITS dst src | ANDBITS dst src | …) */
      if (!b[0]){ fail(vm,"matrix op needs two cubes"); return -1; }
      int ib=find_cube(vm,b);
      if (ib<0){ place_cube(vm,b,b,1); ib=find_cube(vm,b); }
      if (ib<0){ fail(vm,"matrix op src missing"); return -1; }
      cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
      int nb = mb->n > 0 ? mb->n : CUBALC_ATOM_BITS;
      if (nb > CUBALC_ATOM_BITS) nb = CUBALC_ATOM_BITS;
      int nn = n > nb ? n : nb;
      if (nn < 1) nn = CUBALC_ATOM_BITS;
      if (strcmp(op,"COPYBITS")==0 || strcmp(op,"CLONEBITS")==0){
        *ma = *mb;
      } else if (strcmp(op,"ANDBITS")==0){
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, cubalc_matrix_get(ma,i) & cubalc_matrix_get(mb,i));
      } else if (strcmp(op,"ORBITS")==0){
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, cubalc_matrix_get(ma,i) | cubalc_matrix_get(mb,i));
      } else if (strcmp(op,"XORBITS")==0){
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, cubalc_matrix_get(ma,i) ^ cubalc_matrix_get(mb,i));
      } else if (strcmp(op,"NANDBITS")==0){
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, (cubalc_matrix_get(ma,i) & cubalc_matrix_get(mb,i)) ? 0 : 1);
      } else if (strcmp(op,"XNORBITS")==0 || strcmp(op,"EQVBITS")==0 ||
                 strcmp(op,"NXORBITS")==0){
        /* equivalence: ~(a ^ b) */
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, (cubalc_matrix_get(ma,i) ^ cubalc_matrix_get(mb,i)) ? 0 : 1);
      } else if (strcmp(op,"NORBITS")==0){
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, (cubalc_matrix_get(ma,i) | cubalc_matrix_get(mb,i)) ? 0 : 1);
      } else if (strcmp(op,"ANDNBITS")==0 || strcmp(op,"BICBITS")==0){
        /* a & ~b (bit clear / and-not) */
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, cubalc_matrix_get(ma,i) & (cubalc_matrix_get(mb,i) ? 0 : 1));
      } else if (strcmp(op,"ORNBITS")==0){
        /* a | ~b */
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, cubalc_matrix_get(ma,i) | (cubalc_matrix_get(mb,i) ? 0 : 1));
      } else {
        fail(vm,"unknown matrix op"); return -1;
      }
    }
    vm->ch.cubes[ia].atom.digit_lock = 0;
    vm->ch.cubes[ia].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ia].atom.matrix);
    vm->ch.cubes[ia].flowed = 1;
    var_set_num(vm, "SET", cubalc_matrix_popcount(&vm->ch.cubes[ia].atom.matrix));
    var_set_num(vm, "DIGIT", vm->ch.cubes[ia].atom.digit);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# %s %s set=%ld digit=%u\n",
                           op, a, (long)cubalc_matrix_popcount(ma),
                           vm->ch.cubes[ia].atom.digit);
    bump(vm); return 1;
  }
  return 0;
}
