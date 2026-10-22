/* CubalC lang — lang_ops_cell_matpath.c
 * CELL matrix path: PEXTBITS/PDEPBITS · ZIPBITS/UNZIPBITS (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_matpath.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_matpath(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-8 matrix data-path: PEXTBITS/PDEPBITS + ZIPBITS/UNZIPBITS
   * Matrix duals of stack SPEXT/SPDEP/SZIP/SUNZIP. */
  if (kw(&L->cur,"PEXTBITS")||kw(&L->cur,"GATHERBITS")||kw(&L->cur,"MPEXT")||
      kw(&L->cur,"BITPEXT")||kw(&L->cur,"EXTRACTMASK")){
    /* PEXTBITS cube mask → LAST_N = parallel extract under mask ones */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"PEXTBITS cube mask"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long maskv = parse_expr(vm,L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n > 64) n = 64;
    unsigned long long mask = (unsigned long long)maskv;
    unsigned long long res = 0;
    unsigned long long k = 0;
    for (int i=0;i<n;i++){
      if ((mask >> i) & 1ull){
        if (cubalc_matrix_get(m, i)) res |= (1ull << k);
        k++;
        if (k >= 63) break; /* keep signed long portable */
      }
    }
    long r = (long)res;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"PDEPBITS")||kw(&L->cur,"SCATTERBITS")||kw(&L->cur,"MPDEP")||
      kw(&L->cur,"BITPDEP")||kw(&L->cur,"DEPOSITMASK")){
    /* PDEPBITS cube mask val — deposit low bits of val into mask positions */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"PDEPBITS cube mask val"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long maskv = parse_expr(vm,L);
    long valv = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"PDEPBITS missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    if (n > 64) n = 64;
    unsigned long long mask = (unsigned long long)maskv;
    unsigned long long src = (unsigned long long)valv;
    unsigned long long bb = 1;
    int written = 0;
    for (int i=0;i<n;i++){
      if ((mask >> i) & 1ull){
        int on = (src & bb) ? 1 : 0;
        cubalc_matrix_set(m, i, on);
        bb <<= 1;
        written++;
      }
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",written); vm->last_n=written;
    var_set_num(vm,"DIGIT",vm->ch.cubes[ix].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"ZIPBITS")||kw(&L->cur,"INTERLEAVEBITS")||kw(&L->cur,"MORTONBITS")||
      kw(&L->cur,"MZIP")){
    /* ZIPBITS dst a b — dst[2i]=a[i], dst[2i+1]=b[i] for i in 0..31 */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ZIPBITS dst a b"); return -1; }
    char dst[48]; snprintf(dst,sizeof dst,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ZIPBITS dst a b"); return -1; }
    char aid[48]; snprintf(aid,sizeof aid,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ZIPBITS dst a b"); return -1; }
    char bid[48]; snprintf(bid,sizeof bid,"%s",L->cur.text); lex_next(L);
    ensure_world(vm);
    int id=find_cube(vm,dst); if (id<0){ place_cube(vm,dst,dst,1); id=find_cube(vm,dst); }
    int ia=find_cube(vm,aid); if (ia<0){ place_cube(vm,aid,aid,1); ia=find_cube(vm,aid); }
    int ib=find_cube(vm,bid); if (ib<0){ place_cube(vm,bid,bid,1); ib=find_cube(vm,bid); }
    if (id<0||ia<0||ib<0){ fail(vm,"ZIPBITS cube missing"); return -1; }
    cubalc_matrix *md = &vm->ch.cubes[id].atom.matrix;
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    if (md->n < CUBALC_ATOM_BITS) md->n = (uint16_t)CUBALC_ATOM_BITS;
    int half = CUBALC_ATOM_BITS / 2;
    if (half > 32) half = 32;
    for (int i=0;i<half;i++){
      int ba = cubalc_matrix_get(ma, i) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, i) ? 1 : 0;
      int di = 2*i;
      if (di < CUBALC_ATOM_BITS) cubalc_matrix_set(md, di, ba);
      if (di+1 < CUBALC_ATOM_BITS) cubalc_matrix_set(md, di+1, bb);
    }
    /* clear any remaining high bits past 2*half if atom wider */
    for (int i=2*half;i<CUBALC_ATOM_BITS;i++) cubalc_matrix_set(md, i, 0);
    vm->ch.cubes[id].atom.digit_lock = 0;
    vm->ch.cubes[id].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[id].atom.matrix);
    vm->ch.cubes[id].flowed = 1;
    long ones = cubalc_matrix_popcount(md);
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",ones); vm->last_n=ones;
    var_set_num(vm,"DIGIT",vm->ch.cubes[id].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"UNZIPBITS")||kw(&L->cur,"DEINTERLEAVEBITS")||kw(&L->cur,"MUNZIP")||
      kw(&L->cur,"UNMORTONBITS")){
    /* UNZIPBITS even odd src — even gets even indices, odd gets odd indices */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNZIPBITS even odd src"); return -1; }
    char eid[48]; snprintf(eid,sizeof eid,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNZIPBITS even odd src"); return -1; }
    char oid[48]; snprintf(oid,sizeof oid,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNZIPBITS even odd src"); return -1; }
    char sid[48]; snprintf(sid,sizeof sid,"%s",L->cur.text); lex_next(L);
    ensure_world(vm);
    int ie=find_cube(vm,eid); if (ie<0){ place_cube(vm,eid,eid,1); ie=find_cube(vm,eid); }
    int io=find_cube(vm,oid); if (io<0){ place_cube(vm,oid,oid,1); io=find_cube(vm,oid); }
    int is=find_cube(vm,sid); if (is<0){ place_cube(vm,sid,sid,1); is=find_cube(vm,sid); }
    if (ie<0||io<0||is<0){ fail(vm,"UNZIPBITS cube missing"); return -1; }
    cubalc_matrix *me = &vm->ch.cubes[ie].atom.matrix;
    cubalc_matrix *mo = &vm->ch.cubes[io].atom.matrix;
    cubalc_matrix *ms = &vm->ch.cubes[is].atom.matrix;
    if (me->n < CUBALC_ATOM_BITS) me->n = (uint16_t)CUBALC_ATOM_BITS;
    if (mo->n < CUBALC_ATOM_BITS) mo->n = (uint16_t)CUBALC_ATOM_BITS;
    int half = CUBALC_ATOM_BITS / 2;
    if (half > 32) half = 32;
    for (int i=0;i<CUBALC_ATOM_BITS;i++){
      cubalc_matrix_set(me, i, 0);
      cubalc_matrix_set(mo, i, 0);
    }
    for (int i=0;i<half;i++){
      int be = cubalc_matrix_get(ms, 2*i) ? 1 : 0;
      int bo = cubalc_matrix_get(ms, 2*i+1) ? 1 : 0;
      cubalc_matrix_set(me, i, be);
      cubalc_matrix_set(mo, i, bo);
    }
    for (int t=0;t<2;t++){
      int ix = t ? io : ie;
      vm->ch.cubes[ix].atom.digit_lock = 0;
      vm->ch.cubes[ix].atom.digit =
        (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
      vm->ch.cubes[ix].flowed = 1;
    }
    long ones = cubalc_matrix_popcount(me) + cubalc_matrix_popcount(mo);
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",ones); vm->last_n=ones;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  return 0;
}
