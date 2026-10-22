/* CubalC lang — lang_ops_cell_energy.c
 * CELL energy plane: ENERGYGET·SET·ADD·SUB·XFER·CLAMP·SWAP·SHARE +
 * fleet TOTAL/AVG/MIN/MAX + SEFLOW/SEFLOWN (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_energy.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

int cubalc_lang_ops_cell_try_energy(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  if (kw(&L->cur,"ENERGYGET")||kw(&L->cur,"GETENERGY")||kw(&L->cur,"READENERGY")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYGET cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    long ev = (long)lround(vm->ch.cubes[ix].atom.energy * 100.0);
    var_set_num(vm,"ENERGY",ev);
    var_set_num(vm,"LAST_N",ev); vm->last_n=ev;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* ENERGYSET cube n · ENERGYADD cube n — energy plane 0..100 (digit-6).
   * PULSE cube n stays that energy add. PULSE intensity hz ms is a schedule
   * step and starts with a number, so this form lets it through. */
  if (kw(&L->cur,"ENERGYSET")||kw(&L->cur,"SETENERGY")||
      kw(&L->cur,"ENERGYADD")||kw(&L->cur,"ADDENERGY")||kw(&L->cur,"PULSE")){
    if (kw(&L->cur,"PULSE")) {
      Lex saved = *L;
      int cube;
      lex_next(L);
      cube = (L->cur.kind == TK_IDENT);
      *L = saved;
      if (!cube) return 0;
    }
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGY* cube n"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long n = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"ENERGY missing cube"); return -1; }
    float e = vm->ch.cubes[ix].atom.energy;
    if (strcmp(op,"ENERGYSET")==0 || strcmp(op,"SETENERGY")==0){
      e = (float)n / 100.f;
    } else {
      e += (float)n / 100.f;
    }
    if (e < 0.f) e = 0.f;
    if (e > 1.f) e = 1.f;
    vm->ch.cubes[ix].atom.energy = e;
    vm->ch.cubes[ix].flowed = 1;
    long ev = (long)lround(e * 100.0);
    var_set_num(vm, "ENERGY", ev);
    var_set_num(vm, "LAST_N", ev);
    vm->last_n = ev;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-6 ENERGYSUB cube n — drain energy plane (complete set/add/sub) */
  if (kw(&L->cur,"ENERGYSUB")||kw(&L->cur,"SUBENERGY")||kw(&L->cur,"DRAIN")||
      kw(&L->cur,"SENRN")||kw(&L->cur,"ENERGYDRAIN")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYSUB cube n"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long n = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"ENERGYSUB missing cube"); return -1; }
    float e = vm->ch.cubes[ix].atom.energy;
    e -= (float)n / 100.f;
    if (e < 0.f) e = 0.f;
    if (e > 1.f) e = 1.f;
    vm->ch.cubes[ix].atom.energy = e;
    vm->ch.cubes[ix].flowed = 1;
    long ev = (long)lround(e * 100.0);
    var_set_num(vm, "ENERGY", ev);
    var_set_num(vm, "LAST_N", ev);
    vm->last_n = ev;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-6 stack ENERGYFLOW: SEFLOW / SFLWN — hops from TOS (or imm form SEFLOWN) */
  if (kw(&L->cur,"SEFLOW")||kw(&L->cur,"SENERGYFLOW")||kw(&L->cur,"STACKENERGYFLOW")||
      kw(&L->cur,"SFLW")){
    /* pop hops → ENERGYFLOW n */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    if (n < 1) n = 1;
    if (n > 64) n = 64;
    for (long i = 0; i < n; i++) do_flow(vm, 1);
    long e = 0;
    for (int i = 0; i < vm->ch.n_cubes; i++)
      e += (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
    long *se = var_slot(vm, "ENERGY", 1); if (se) *se = e;
    var_set_num(vm, "ENERGY", e);
    var_set_num(vm, "LAST_N", e); vm->last_n = e;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SEFLOWN")||kw(&L->cur,"SFLWN")||kw(&L->cur,"EFLOWN")||
      kw(&L->cur,"ENERGYFLOWN")){
    /* SEFLOWN n — ENERGYFLOW with immediate hops (alias plane) */
    lex_next(L);
    long n = parse_expr(vm,L);
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
    bump(vm); return 1;
  }
  /* digit-6 ENERGYXFER src dst n — move energy units (0..100 scale) src→dst, clamp both */
  if (kw(&L->cur,"ENERGYXFER")||kw(&L->cur,"XFERENERGY")||kw(&L->cur,"SENRX")||
      kw(&L->cur,"TRANSFERENERGY")||kw(&L->cur,"ENERGYMOVE")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYXFER src dst n"); return -1; }
    char src[48]; snprintf(src,sizeof src,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYXFER dst"); return -1; }
    char dst[48]; snprintf(dst,sizeof dst,"%s",L->cur.text); lex_next(L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    ensure_world(vm);
    int is = find_cube(vm,src);
    int id = find_cube(vm,dst);
    if (is<0){ place_cube(vm,src,src,1); is=find_cube(vm,src); }
    if (id<0){ place_cube(vm,dst,dst,1); id=find_cube(vm,dst); }
    if (is<0 || id<0){ fail(vm,"ENERGYXFER missing"); return -1; }
    float es = vm->ch.cubes[is].atom.energy;
    float ed = vm->ch.cubes[id].atom.energy;
    float amt = (float)n / 100.f;
    if (amt > es) amt = es;
    es -= amt;
    ed += amt;
    if (es < 0.f) es = 0.f;
    if (es > 1.f) es = 1.f;
    if (ed < 0.f) ed = 0.f;
    if (ed > 1.f) ed = 1.f;
    vm->ch.cubes[is].atom.energy = es;
    vm->ch.cubes[id].atom.energy = ed;
    vm->ch.cubes[is].flowed = 1;
    vm->ch.cubes[id].flowed = 1;
    long moved = (long)lround(amt * 100.0);
    long *se = var_slot(vm, "ENERGY", 1);
    if (se) *se = (long)lround(ed * 100.0);
    var_set_num(vm, "ENERGY", (long)lround(ed * 100.0));
    var_set_num(vm, "LAST_N", moved); vm->last_n = moved;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-6 ENERGYCLAMP id lo hi — clamp energy plane to [lo,hi] (0..100 scale) */
  if (kw(&L->cur,"ENERGYCLAMP")||kw(&L->cur,"CLAMPENERGY")||kw(&L->cur,"SFLWX")||
      kw(&L->cur,"ENERGYBOUND")||kw(&L->cur,"BOUNDENERGY")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYCLAMP id lo hi"); return -1; }
    char idn[48]; snprintf(idn,sizeof idn,"%s",L->cur.text); lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo > hi){ long t=lo; lo=hi; hi=t; }
    if (lo < 0) lo = 0;
    if (hi > 100) hi = 100;
    ensure_world(vm);
    int ix = find_cube(vm,idn);
    if (ix<0){ place_cube(vm,idn,idn,1); ix=find_cube(vm,idn); }
    if (ix<0){ fail(vm,"ENERGYCLAMP missing"); return -1; }
    long ev = (long)lround(vm->ch.cubes[ix].atom.energy * 100.0);
    if (ev < lo) ev = lo;
    if (ev > hi) ev = hi;
    vm->ch.cubes[ix].atom.energy = (float)ev / 100.f;
    vm->ch.cubes[ix].flowed = 1;
    var_set_num(vm, "ENERGY", ev);
    var_set_num(vm, "LAST_N", ev); vm->last_n = ev;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-6 ENERGYSWAP a b — exchange energy planes */
  if (kw(&L->cur,"ENERGYSWAP")||kw(&L->cur,"SWAPENERGY")||kw(&L->cur,"ESWAP")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYSWAP a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYSWAP a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    ensure_world(vm);
    int ia = find_cube(vm,a); if (ia<0){ place_cube(vm,a,a,1); ia=find_cube(vm,a); }
    int ib = find_cube(vm,b); if (ib<0){ place_cube(vm,b,b,1); ib=find_cube(vm,b); }
    if (ia<0 || ib<0){ fail(vm,"ENERGYSWAP missing"); return -1; }
    float ea = vm->ch.cubes[ia].atom.energy;
    float eb = vm->ch.cubes[ib].atom.energy;
    vm->ch.cubes[ia].atom.energy = eb;
    vm->ch.cubes[ib].atom.energy = ea;
    vm->ch.cubes[ia].flowed = 1;
    vm->ch.cubes[ib].flowed = 1;
    long ev = (long)lround(eb * 100.0); /* energy now on a */
    var_set_num(vm,"ENERGY",ev);
    var_set_num(vm,"LAST_N",ev); vm->last_n=ev;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 ENERGYSHARE a b — equalize energy (mean of both, clamp 0..1) */
  if (kw(&L->cur,"ENERGYSHARE")||kw(&L->cur,"SHAREENERGY")||kw(&L->cur,"EQUALIZEENERGY")||
      kw(&L->cur,"ENERGYEQZ")||kw(&L->cur,"ESHAR")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYSHARE a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYSHARE a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    ensure_world(vm);
    int ia = find_cube(vm,a); if (ia<0){ place_cube(vm,a,a,1); ia=find_cube(vm,a); }
    int ib = find_cube(vm,b); if (ib<0){ place_cube(vm,b,b,1); ib=find_cube(vm,b); }
    if (ia<0 || ib<0){ fail(vm,"ENERGYSHARE missing"); return -1; }
    float mean = 0.5f * (vm->ch.cubes[ia].atom.energy + vm->ch.cubes[ib].atom.energy);
    if (mean < 0.f) mean = 0.f;
    if (mean > 1.f) mean = 1.f;
    vm->ch.cubes[ia].atom.energy = mean;
    vm->ch.cubes[ib].atom.energy = mean;
    vm->ch.cubes[ia].flowed = 1;
    vm->ch.cubes[ib].flowed = 1;
    long ev = (long)lround(mean * 100.0);
    var_set_num(vm,"ENERGY",ev);
    var_set_num(vm,"LAST_N",ev); vm->last_n=ev;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 fleet energy metrics: ENERGYTOTAL ENERGYAVG ENERGYMIN ENERGYMAX */
  if (kw(&L->cur,"ENERGYTOTAL")||kw(&L->cur,"ENERGYSUM")||kw(&L->cur,"SUMENERGY")||
      kw(&L->cur,"TOTALENERGY")||
      kw(&L->cur,"ENERGYAVG")||kw(&L->cur,"AVGENERGY")||kw(&L->cur,"MEANENERGY")||
      kw(&L->cur,"ENERGYMEAN")||
      kw(&L->cur,"ENERGYMIN")||kw(&L->cur,"MINENERGY")||
      kw(&L->cur,"ENERGYMAX")||kw(&L->cur,"MAXENERGY")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    ensure_world(vm);
    int nc = vm->ch.n_cubes;
    if (nc < 1){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1;
    }
    long sum = 0, mn = 101, mx = -1;
    for (int i=0;i<nc;i++){
      long ev = (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
      sum += ev;
      if (ev < mn) mn = ev;
      if (ev > mx) mx = ev;
    }
    long r = sum;
    if (strcmp(op,"ENERGYAVG")==0 || strcmp(op,"AVGENERGY")==0 ||
        strcmp(op,"MEANENERGY")==0 || strcmp(op,"ENERGYMEAN")==0)
      r = sum / nc;
    else if (strcmp(op,"ENERGYMIN")==0 || strcmp(op,"MINENERGY")==0)
      r = mn;
    else if (strcmp(op,"ENERGYMAX")==0 || strcmp(op,"MAXENERGY")==0)
      r = mx;
    /* else TOTAL/SUM */
    var_set_num(vm,"ENERGY",r);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SET",(long)nc);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack fleet energy metrics: SENERGYTOTAL SENERGYAVG SENERGYMIN SENERGYMAX */
  if (kw(&L->cur,"SENERGYTOTAL")||kw(&L->cur,"SENERGYSUM")||kw(&L->cur,"STACKENERGYTOTAL")||
      kw(&L->cur,"SETOTAL")||
      kw(&L->cur,"SENERGYAVG")||kw(&L->cur,"SENERGYMEAN")||kw(&L->cur,"STACKENERGYAVG")||
      kw(&L->cur,"SEAVG")||
      kw(&L->cur,"SENERGYMIN")||kw(&L->cur,"STACKENERGYMIN")||kw(&L->cur,"SEMIN")||
      kw(&L->cur,"SENERGYMAX")||kw(&L->cur,"STACKENERGYMAX")||kw(&L->cur,"SEMAX")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    ensure_world(vm);
    int nc = vm->ch.n_cubes;
    if (nc < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long sum = 0, mn = 101, mx = -1;
    for (int i=0;i<nc;i++){
      long ev = (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
      sum += ev;
      if (ev < mn) mn = ev;
      if (ev > mx) mx = ev;
    }
    long r = sum;
    if (strcmp(op,"SENERGYAVG")==0 || strcmp(op,"SENERGYMEAN")==0 ||
        strcmp(op,"STACKENERGYAVG")==0 || strcmp(op,"SEAVG")==0)
      r = sum / nc;
    else if (strcmp(op,"SENERGYMIN")==0 || strcmp(op,"STACKENERGYMIN")==0 ||
             strcmp(op,"SEMIN")==0)
      r = mn;
    else if (strcmp(op,"SENERGYMAX")==0 || strcmp(op,"STACKENERGYMAX")==0 ||
             strcmp(op,"SEMAX")==0)
      r = mx;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"ENERGY",r);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
