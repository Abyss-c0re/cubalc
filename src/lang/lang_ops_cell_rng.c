/* CubalC lang — lang_ops_cell_rng.c
 * CELL seeded RNG plane: RAND·SEED·SRAND·SHUFFLE·RANDBITS (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_rng.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int cubalc_lang_ops_cell_try_rng(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* RAND [max] — seeded RNG (CUBALC_SEED); default range 0..9 */
  if (kw(&L->cur,"RAND")||kw(&L->cur,"RND")||kw(&L->cur,"IRAND")){
    lex_next(L);
    long m = 10;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      m = parse_expr(vm,L);
    if (m < 1) m = 1;
    uint32_t x = vm->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    long v = (long)(x % (uint32_t)m);
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-6 SEED n — set PRNG state (0 → 1); GETSEED / RNG reports state */
  if (kw(&L->cur,"SEED")||kw(&L->cur,"SETSEED")){
    lex_next(L);
    long s = parse_expr(vm,L);
    uint32_t u = (uint32_t)s;
    if (!u) u = 1;
    vm->rng = u;
    var_set_num(vm, "LAST_N", (long)u);
    vm->last_n = (long)u;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"GETSEED")||kw(&L->cur,"SHOWSEED")){
    /* statement form: report current rng without advancing */
    lex_next(L);
    long v = (long)vm->rng;
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-6 stack RNG: SSEED SRAND SGETSEED */
  if (kw(&L->cur,"SSEED")||kw(&L->cur,"STACKSEED")){
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long s = vm->stack[--vm->sp];
    uint32_t u = (uint32_t)s;
    if (!u) u = 1;
    vm->rng = u;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",(long)u); vm->last_n=(long)u;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGETSEED")||kw(&L->cur,"STACKGETSEED")||kw(&L->cur,"SRNG")){
    lex_next(L);
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = (long)vm->rng;
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRAND")||kw(&L->cur,"STACKRAND")||kw(&L->cur,"SRND")){
    /* max → rand in [0, max); max<=0 treated as 10 */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    if (m < 1) m = 10;
    uint32_t x = vm->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    long v = (long)(x % (uint32_t)m);
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack/imm RNG dual forms: SRANDI/SRANDN · SSEEDI/SSEEDN (Cube aliases retained) */
  if (kw(&L->cur,"SRANDI")||kw(&L->cur,"SRANDN")||kw(&L->cur,"RANDIMM")||
      kw(&L->cur,"STACKRANDI")||kw(&L->cur,"RANDN")||kw(&L->cur,"SRANDIMM")||
      kw(&L->cur,"STACKRANDN")||kw(&L->cur,"SRNGN")){
    /* SRANDI/SRANDN n — push rand in [0,n); n<=0 → 10 */
    lex_next(L);
    long m = parse_expr(vm,L);
    if (m < 1) m = 10;
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    uint32_t x = vm->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    long v = (long)(x % (uint32_t)m);
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEEDI")||kw(&L->cur,"SSEEDN")||kw(&L->cur,"SEEDIMM")||
      kw(&L->cur,"STACKSEEDI")||kw(&L->cur,"SEEDN")||kw(&L->cur,"SSEEDIMM")){
    /* SSEEDI/SSEEDN n — set PRNG seed to n (0→1); push seed used */
    lex_next(L);
    long s = parse_expr(vm,L);
    uint32_t u = (uint32_t)s;
    if (!u) u = 1;
    vm->rng = u;
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = (long)u;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",(long)u); vm->last_n=(long)u;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack-imm range RNG: SRANDRANGEN lo hi — push uniform [lo,hi] (dual of SRANDRANGE) */
  if (kw(&L->cur,"SRANDRANGEN")||kw(&L->cur,"RANDRANGEN")||kw(&L->cur,"SRNGX")||
      kw(&L->cur,"STACKRANDRANGEN")||kw(&L->cur,"SRANDINN")){
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long span = hi - lo + 1;
    if (span < 1) span = 1;
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    uint32_t x = vm->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    long v = lo + (long)(x % (uint32_t)span);
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 RNG range + shuffle + random matrix bits */
  if (kw(&L->cur,"RANDRANGE")||kw(&L->cur,"RANDIN")||kw(&L->cur,"RANDBETWEEN")){
    /* RANDRANGE lo hi — uniform integer in [lo,hi] inclusive */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long span = hi - lo + 1;
    if (span < 1) span = 1;
    uint32_t x = vm->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    long v = lo + (long)(x % (uint32_t)span);
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRANDRANGE")||kw(&L->cur,"STACKRANDRANGE")||kw(&L->cur,"SRANDIN")){
    /* lo hi → push rand in [lo,hi] */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long span = hi - lo + 1;
    if (span < 1) span = 1;
    uint32_t x = vm->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    long v = lo + (long)(x % (uint32_t)span);
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHUFFLECELL")||kw(&L->cur,"CELLSHUFFLE")||kw(&L->cur,"SHUFFLE")){
    /* SHUFFLECELL lo hi — Fisher–Yates using seeded RNG */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i = hi; i > lo; i--){
      uint32_t x = vm->rng;
      x ^= x << 13; x ^= x >> 17; x ^= x << 5;
      if (!x) x = 1;
      vm->rng = x;
      long j = lo + (long)(x % (uint32_t)(i - lo + 1));
      long tmp = vm->cells[(int)i];
      vm->cells[(int)i] = vm->cells[(int)j];
      vm->cells[(int)j] = tmp;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"PICKCELL")||kw(&L->cur,"CELLPICK")||kw(&L->cur,"RANDCELL")){
    /* PICKCELL lo hi — LAST_N = value at random index; IT = index */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long span = hi - lo + 1;
    if (span < 1) span = 1;
    uint32_t x = vm->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    long idx = lo + (long)(x % (uint32_t)span);
    long v = vm->cells[(int)idx];
    long *it = var_slot(vm,"IT",1); if (it) *it = idx;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"RANDBITS")||kw(&L->cur,"RANDOMBITS")||kw(&L->cur,"RANDMATRIX")||
      kw(&L->cur,"FILLRAND")){
    /* RANDBITS cube [pct] — randomize matrix; optional density 0..100 (default 50) */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"RANDBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long pct = 50;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      pct = parse_expr(vm,L);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"RANDBITS missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    cubalc_matrix_clear(m);
    m->n = (uint16_t)n;
    long ones = 0;
    for (int i=0;i<n;i++){
      uint32_t x = vm->rng;
      x ^= x << 13; x ^= x >> 17; x ^= x << 5;
      if (!x) x = 1;
      vm->rng = x;
      int on = ((long)(x % 100u) < pct) ? 1 : 0;
      if (on){ cubalc_matrix_set(m, i, 1); ones++; }
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",ones); vm->last_n=ones;
    var_set_num(vm,"DIGIT",vm->ch.cubes[ix].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  return 0;
}
