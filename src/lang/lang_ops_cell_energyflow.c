/* CubalC lang — lang_ops_cell_energyflow.c
 * CELL energy/flow: COMPARE·HARMONY·RESOLVE·ENERGYFLOW·FLOWI (G peel).
 * Mesh vote G_cell_margin · free energy must flow · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_energyflow.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

int cubalc_lang_ops_cell_try_energyflow(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* COMPARE cube_a cube_b */
  if (kw(&L->cur,"COMPARE")||kw(&L->cur,"UNITE")||kw(&L->cur,"HAMMING")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"COMPARE a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"COMPARE a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    long u=do_compare(vm,a,b);
    if (vm->res) snprintf(vm->res->last_print,sizeof vm->res->last_print,
                          "compare %s~%s unity=%ld", a, b, u);
    bump(vm); return 1;
  }
  /* HARMONY [target] */
  if (kw(&L->cur,"HARMONY")||kw(&L->cur,"HIVEMIND")||kw(&L->cur,"CONSENSUS")){
    lex_next(L);
    char tid[48]={0};
    if (L->cur.kind==TK_IDENT && !kw(&L->cur,"END") && !kw(&L->cur,"ELSE")
        && !kw(&L->cur,"LET") && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"PRINT")){
      snprintf(tid,sizeof tid,"%s",L->cur.text); lex_next(L);
    }
    do_harmony(vm, tid[0]?tid:NULL);
    bump(vm); return 1;
  }
  /* RESOLVE [target] — harmony + decide + energy pulse (algocubes resolved) */
  if (kw(&L->cur,"RESOLVE")||kw(&L->cur,"ALGORESOLVE")||kw(&L->cur,"SETTLE")){
    lex_next(L);
    char tid[48]={0};
    if (L->cur.kind==TK_IDENT && !kw(&L->cur,"END") && !kw(&L->cur,"ELSE")
        && !kw(&L->cur,"LET") && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"PRINT")){
      snprintf(tid,sizeof tid,"%s",L->cur.text); lex_next(L);
    }
    do_resolve(vm, tid[0]?tid:NULL);
    bump(vm); return 1;
  }
  /* ENERGYFLOW n — multi-hop free-flow; energy must flow */
  if (kw(&L->cur,"ENERGYFLOW")||kw(&L->cur,"EFLOW")||kw(&L->cur,"PULSEFLOW")){
    lex_next(L);
    long n = 4;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS)
      n = parse_expr(vm, L);
    if (n < 1) n = 1;
    if (n > 64) n = 64;
    for (long i = 0; i < n; i++) do_flow(vm, 1);
    long e = 0;
    for (int i = 0; i < vm->ch.n_cubes; i++)
      e += (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
    long *se = var_slot(vm, "ENERGY", 1); if (se) *se = e;
    var_set_num(vm, "ENERGY", e);
    var_set_num(vm, "LAST_N", e); vm->last_n = e;
    var_set_num(vm, "OK", 1);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print,
                          "energyflow n=%ld e=%ld u=%.2f", n, e, vm->ch.unity);
    bump(vm); return 1;
  }
  /* digit-6 energy/flow immediate dual forms: ENERGYFLOWI · FLOWI/FLOWN */
  if (kw(&L->cur,"ENERGYFLOWI")||kw(&L->cur,"EFLOWI")||
      kw(&L->cur,"PULSEFLOWI")||kw(&L->cur,"PULSEFLOWN")){
    /* ENERGYFLOWI n — required imm multi-hop free-flow (ENERGYFLOWN via SEFLOWN Cube path) */
    lex_next(L);
    long n = parse_expr(vm, L);
    if (n < 1) n = 1;
    if (n > 64) n = 64;
    for (long i = 0; i < n; i++) do_flow(vm, 1);
    long e = 0;
    for (int i = 0; i < vm->ch.n_cubes; i++)
      e += (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
    long *se = var_slot(vm, "ENERGY", 1); if (se) *se = e;
    var_set_num(vm, "ENERGY", e);
    var_set_num(vm, "LAST_N", e); vm->last_n = e;
    var_set_num(vm, "OK", 1);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print,
                          "energyflown n=%ld e=%ld u=%.2f", n, e, vm->ch.unity);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"FLOWI")||kw(&L->cur,"FLOWN")||kw(&L->cur,"TICKI")||kw(&L->cur,"TICKN")){
    /* FLOWI/FLOWN n — required imm free-flow hops */
    lex_next(L);
    long n = parse_expr(vm, L);
    if (n < 1) n = 1;
    if (n > 1000) n = 1000;
    ensure_world(vm);
    do_flow(vm, (int)n);
    long e = 0;
    for (int i = 0; i < vm->ch.n_cubes; i++)
      e += (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
    var_set_num(vm, "ENERGY", e);
    var_set_num(vm, "LAST_N", e); vm->last_n = e;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  return 0;
}
