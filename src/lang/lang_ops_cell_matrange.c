/* CubalC lang — lang_ops_cell_matrange.c
 * CELL matrix range/parity: PARITYBITS·COPYRANGE·SWAPRANGE·ROTRANGE/SHLRANGE (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_matrange.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_matrange(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-7 COP matrix: PARITYBITS · COPYRANGE · SWAPRANGE (local reverse) */
  if (kw(&L->cur,"PARITYBITS")||kw(&L->cur,"XORREDUCE")||kw(&L->cur,"BITPARITY")||
      kw(&L->cur,"PARBIT")){
    /* PARITYBITS cube → LAST_N = XOR of all bits (0/1) */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"PARITYBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long p = 0;
    long ones = 0;
    for (int i=0;i<n;i++){
      if (cubalc_matrix_get(m, i)){ p ^= 1; ones++; }
    }
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",p); vm->last_n=p;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"COPYRANGE")||kw(&L->cur,"BITCOPY")||kw(&L->cur,"MOVEBITS")||
      kw(&L->cur,"XFERBITS")){
    /* COPYRANGE dst doff src soff n — copy n bits src[soff..] → dst[doff..] */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"COPYRANGE dst doff src soff n"); return -1; }
    char dst[48]; snprintf(dst,sizeof dst,"%s",L->cur.text); lex_next(L);
    long doff = parse_expr(vm,L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"COPYRANGE dst doff src soff n"); return -1; }
    char srcid[48]; snprintf(srcid,sizeof srcid,"%s",L->cur.text); lex_next(L);
    long soff = parse_expr(vm,L);
    long cnt = parse_expr(vm,L);
    ensure_world(vm);
    int id=find_cube(vm,dst); if (id<0){ place_cube(vm,dst,dst,1); id=find_cube(vm,dst); }
    int is=find_cube(vm,srcid); if (is<0){ place_cube(vm,srcid,srcid,1); is=find_cube(vm,srcid); }
    if (id<0||is<0){ fail(vm,"COPYRANGE cube missing"); return -1; }
    cubalc_matrix *md = &vm->ch.cubes[id].atom.matrix;
    cubalc_matrix *ms = &vm->ch.cubes[is].atom.matrix;
    if (md->n < CUBALC_ATOM_BITS) md->n = (uint16_t)CUBALC_ATOM_BITS;
    if (doff < 0) doff = 0;
    if (soff < 0) soff = 0;
    if (cnt < 0) cnt = 0;
    long copied = 0;
    for (long k=0;k<cnt;k++){
      long di = doff + k;
      long si = soff + k;
      if (di < 0 || di >= CUBALC_ATOM_BITS) break;
      if (si < 0 || si >= CUBALC_ATOM_BITS){
        cubalc_matrix_set(md, (int)di, 0);
      } else {
        int on = cubalc_matrix_get(ms, (int)si) ? 1 : 0;
        cubalc_matrix_set(md, (int)di, on);
      }
      copied++;
    }
    vm->ch.cubes[id].atom.digit_lock = 0;
    vm->ch.cubes[id].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[id].atom.matrix);
    vm->ch.cubes[id].flowed = 1;
    var_set_num(vm,"SET",cubalc_matrix_popcount(md));
    var_set_num(vm,"LAST_N",copied); vm->last_n=copied;
    var_set_num(vm,"DIGIT",vm->ch.cubes[id].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SWAPRANGE")||kw(&L->cur,"REVERSERANGE")||kw(&L->cur,"REVRANGE")||
      kw(&L->cur,"MIRRORRANGE")){
    /* SWAPRANGE cube lo hi — reverse bits in [lo..hi] inclusive */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SWAPRANGE cube lo hi"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int lo = (int)parse_expr(vm,L);
    int hi = (int)parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"SWAPRANGE missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    if (lo < 0) lo = 0;
    if (hi < lo){ int t=lo; lo=hi; hi=t; }
    if (lo >= n){ var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (hi >= n) hi = n - 1;
    for (int i=lo, j=hi; i<j; i++, j--){
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
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",ones); vm->last_n=ones;
    var_set_num(vm,"DIGIT",vm->ch.cubes[ix].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-1 data: ROTRANGE/RORRANGE/SHLRANGE/SHRRANGE cube lo hi k
   * Local rotate or logical shift of bits in [lo..hi] (outside range untouched).
   * SHL: index i → i+k (zero-fill low); SHR: i → i-k; ROL wraps; ROR wraps. */
  if (kw(&L->cur,"ROTRANGE")||kw(&L->cur,"ROLRANGE")||kw(&L->cur,"ROTATERANGE")||
      kw(&L->cur,"RORRANGE")||kw(&L->cur,"ROTRRANGE")||
      kw(&L->cur,"SHLRANGE")||kw(&L->cur,"LSHRANGE")||kw(&L->cur,"SHIFTRANGE")||
      kw(&L->cur,"SHRRANGE")||kw(&L->cur,"RSHRANGE")){
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ROTRANGE cube lo hi k"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int lo = (int)parse_expr(vm,L);
    int hi = (int)parse_expr(vm,L);
    long k = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"ROTRANGE missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    if (lo < 0) lo = 0;
    if (hi < lo){ int t=lo; lo=hi; hi=t; }
    if (lo >= n){ var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (hi >= n) hi = n - 1;
    int len = hi - lo + 1;
    if (len < 1){ var_set_num(vm,"OK",1); bump(vm); return 1; }
    int is_ror = (strcmp(op,"RORRANGE")==0 || strcmp(op,"ROTRRANGE")==0);
    int is_rol = (strcmp(op,"ROTRANGE")==0 || strcmp(op,"ROLRANGE")==0 ||
                  strcmp(op,"ROTATERANGE")==0);
    int is_shr = (strcmp(op,"SHRRANGE")==0 || strcmp(op,"RSHRANGE")==0);
    int is_shl = (strcmp(op,"SHLRANGE")==0 || strcmp(op,"LSHRANGE")==0 ||
                  strcmp(op,"SHIFTRANGE")==0);
    if (k < 0){
      k = -k;
      if (is_rol){ is_rol=0; is_ror=1; }
      else if (is_ror){ is_ror=0; is_rol=1; }
      else if (is_shl){ is_shl=0; is_shr=1; }
      else if (is_shr){ is_shr=0; is_shl=1; }
    }
    uint8_t oldb[CUBALC_ATOM_BITS];
    uint8_t newb[CUBALC_ATOM_BITS];
    memset(oldb, 0, sizeof oldb);
    memset(newb, 0, sizeof newb);
    for (int i=0;i<len;i++) oldb[i] = cubalc_matrix_get(m, lo + i) ? 1 : 0;
    if (k == 0){
      memcpy(newb, oldb, (size_t)len);
    } else if (is_rol || is_ror){
      /* left-by-k: bit i → (i+k)%len  ⇒  new[i] = old[(i-k+len)%len] */
      int kk = (int)(k % (long)len);
      if (is_ror) kk = (len - kk) % len;
      for (int i=0;i<len;i++){
        int si = i - kk;
        while (si < 0) si += len;
        si %= len;
        newb[i] = oldb[si];
      }
    } else {
      int kk = (int)k;
      if (kk >= len){
        /* all zeros */
      } else if (is_shl){
        for (int i=0;i<len;i++){
          int si = i - kk;
          if (si >= 0) newb[i] = oldb[si];
        }
      } else { /* SHR */
        for (int i=0;i<len;i++){
          int si = i + kk;
          if (si < len) newb[i] = oldb[si];
        }
      }
    }
    for (int i=0;i<len;i++) cubalc_matrix_set(m, lo + i, newb[i] ? 1 : 0);
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",ones); vm->last_n=ones;
    var_set_num(vm,"DIGIT",vm->ch.cubes[ix].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  return 0;
}
