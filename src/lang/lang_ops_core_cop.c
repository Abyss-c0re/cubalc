/* CubalC lang — lang_ops_core_cop.c
 * SCIENCE · CUBE · PLUG · NEST · COMPILE · IMPULSE · FLOW (core carve).
 */
#include "lang/cubalc_lang_ops_core_cop.h"
#include "cubalc_law.h"
#include "cubalc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cubalc_lang_ops_core_try_cop(VM *vm, Lex *L) {
  /* SCIENCE [LOAD] — inject pure-science constants (public domain; language design) */
  if (kw(&L->cur,"SCIENCE")||kw(&L->cur,"PURESCIENCE")){
    lex_next(L);
    if (kw(&L->cur,"LOAD")||kw(&L->cur,"CONST")||kw(&L->cur,"CONSTANTS")) lex_next(L);
    var_set_num(vm, "PI100", CUBALC_SCI_PI100);
    var_set_num(vm, "E100", CUBALC_SCI_E100);
    var_set_num(vm, "G_EARTH", CUBALC_SCI_G_EARTH10);
    var_set_num(vm, "C_LIGHT", CUBALC_SCI_C_LIGHT);
    var_set_num(vm, "ATM_KPA", CUBALC_SCI_ATM_KPA);
    var_set_num(vm, "WATER_K", CUBALC_SCI_WATER_K);
    var_set_num(vm, "H2O_BP", CUBALC_SCI_H2O_BP_C);
    var_set_num(vm, "R_GAS", CUBALC_SCI_R_J);
    var_set_num(vm, "NA_ORDER", CUBALC_SCI_AVOGADRO_E23);
    var_set_num(vm, "EARTH_R", CUBALC_SCI_EARTH_R_KM);
    var_set_num(vm, "AU_KM", CUBALC_SCI_AU_KM);
    var_set_num(vm, "YEAR_D", CUBALC_SCI_YEAR_D);
    var_set_num(vm, "MOON_D", CUBALC_SCI_MOON_D);
    var_set_num(vm, "SOLAR_C", CUBALC_SCI_SOLAR_C);
    var_set_num(vm, "ATM_O2", CUBALC_SCI_ATM_O2_PCT);
    var_set_num(vm, "ATM_N2", CUBALC_SCI_ATM_N2_PCT);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CUBE")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail_at(vm,L,"CUBE needs id — CUBE body"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    char role[48]; snprintf(role,sizeof role,"%s",id); int proton=1;
    char of_class[48]; of_class[0]=0;
    /* COP: CUBE name OF ClassName [ROLE …] [PROTON …]
     * Instantiates OOP class fields on the cube unit (reusable cube type). */
    if (kw(&L->cur,"OF") || kw(&L->cur,"AS") || kw(&L->cur,"TYPE") ||
        kw(&L->cur,"CLASS")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT) { fail_at(vm,L,"CUBE OF needs ClassName — CUBE u OF Ticket"); return -1; }
      snprintf(of_class, sizeof of_class, "%s", L->cur.text);
      lex_next(L);
    }
    while (L->cur.kind==TK_IDENT){
      if (kw(&L->cur,"ROLE")){ lex_next(L); if (L->cur.kind==TK_IDENT){ snprintf(role,sizeof role,"%s",L->cur.text); lex_next(L);} }
      else if (kw(&L->cur,"PROTON")){ lex_next(L); if (L->cur.kind==TK_NUM){ proton=L->cur.num?1:0; lex_next(L);} }
      else break;
    }
    /* Prefer class default role when OF and ROLE not explicitly set to custom */
    if (of_class[0]) {
      int ci;
      for (ci = 0; ci < vm->n_classes; ci++) {
        if (strcmp(vm->classes[ci].name, of_class) == 0) {
          if (strcmp(role, id) == 0 && vm->classes[ci].role[0])
            snprintf(role, sizeof role, "%s", vm->classes[ci].role);
          break;
        }
      }
      if (ci >= vm->n_classes) {
        snprintf(vm->err, sizeof vm->err, "CUBE OF unknown CLASS %s", of_class);
        fail(vm, vm->err); return -1;
      }
    }
    place_cube(vm,id,role,proton);
    /* Bind OOP instance to cube id (same name) for SEND/GETF/SETF */
    if (of_class[0]) {
      int ci = -1, fi;
      ClassDef *cd;
      ObjInst *ob;
      for (ci = 0; ci < vm->n_classes; ci++)
        if (strcmp(vm->classes[ci].name, of_class) == 0) break;
      cd = &vm->classes[ci];
      /* re-NEW or attach */
      for (fi = 0; fi < vm->n_objs; fi++) {
        if (vm->objs[fi].live && strcmp(vm->objs[fi].name, id) == 0) {
          snprintf(vm->err, sizeof vm->err, "CUBE OF object name clash %s", id);
          fail(vm, vm->err); return -1;
        }
      }
      if (vm->n_objs >= CUBALC_MAX_OBJS) { fail(vm, "too many objects"); return -1; }
      ob = &vm->objs[vm->n_objs++];
      memset(ob, 0, sizeof *ob);
      snprintf(ob->name, sizeof ob->name, "%s", id);
      ob->class_idx = ci;
      ob->live = 1;
      ob->cube_idx = find_cube(vm, id);
      for (fi = 0; fi < cd->n_fields; fi++) {
        FieldDef *fd = &cd->fields[fi];
        if (fd->has_def && fd->is_str) {
          snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", fd->def_str);
          ob->fis_str[fi] = 1;
        } else {
          ob->fnum[fi] = fd->has_def ? fd->def_num : 0;
          ob->fis_str[fi] = 0;
        }
      }
      var_set_str(vm, id, of_class);
      var_set_str(vm, "CLASS", of_class);
      var_set_str(vm, "OBJECT", id);
    }
    bump(vm); return 1;
  }
  /* PLUG — only cubes; pluggable I/O wire (matrix-compatible) */
  if (kw(&L->cur,"PLUG")||kw(&L->cur,"WIRE")||kw(&L->cur,"IO_PLUG")){
    lex_next(L);
    if (kw(&L->cur,"RING")){ lex_next(L); do_ring(vm); bump(vm); return 1; }
    if (L->cur.kind!=TK_IDENT){ fail_at(vm,L,"PLUG needs two units — PLUG a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail_at(vm,L,"PLUG needs second unit — PLUG a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    do_plug(vm,a,b); bump(vm); return 1;
  }
  /* UNPLUG a b — detach pluggable I/O */
  if (kw(&L->cur,"UNPLUG")||kw(&L->cur,"DETACH")||kw(&L->cur,"DISCONNECT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNPLUG a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNPLUG a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    do_unplug(vm,a,b); bump(vm); return 1;
  }
  /* REVERSE a b — reverse pluggable I/O direction (IN↔OUT) */
  if (kw(&L->cur,"REVERSE")||kw(&L->cur,"REV")||kw(&L->cur,"FLIP_IO")||kw(&L->cur,"IOR")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"REVERSE a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"REVERSE a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    do_reverse(vm,a,b); bump(vm); return 1;
  }
  /* IO cube IN|OUT [face] — declare pluggable port direction on a cube */
  if (kw(&L->cur,"IO")||kw(&L->cur,"PORT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"IO cube IN|OUT [face]"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int is_out = 1;
    if (kw(&L->cur,"IN")||kw(&L->cur,"RECV")||kw(&L->cur,"RX")){ is_out=0; lex_next(L); }
    else if (kw(&L->cur,"OUT")||kw(&L->cur,"EMIT")||kw(&L->cur,"TX")||kw(&L->cur,"SEND")){ is_out=1; lex_next(L); }
    int face = 0;
    if (L->cur.kind==TK_NUM){ face=(int)L->cur.num; lex_next(L); }
    do_io(vm, id, face, is_out); bump(vm); return 1;
  }
  /* NEST parent child — cubes may nest */
  if (kw(&L->cur,"NEST")||kw(&L->cur,"INSIDE")||kw(&L->cur,"CONTAIN")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"NEST parent child"); return -1; }
    char p[48]; snprintf(p,sizeof p,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"NEST parent child"); return -1; }
    char c[48]; snprintf(c,sizeof c,"%s",L->cur.text); lex_next(L);
    do_nest(vm, p, c); bump(vm); return 1;
  }
  /* UNNEST child — detach from parent */
  if (kw(&L->cur,"UNNEST")||kw(&L->cur,"EJECT")||kw(&L->cur,"DETACH_NEST")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNNEST child"); return -1; }
    char c[48]; snprintf(c,sizeof c,"%s",L->cur.text); lex_next(L);
    do_unnest(vm, c); bump(vm); return 1;
  }
  /* COMPILE cube | COMPILE ALL — each cube → matrix; no flow → no compile */
  if (kw(&L->cur,"COMPILE")||kw(&L->cur,"TOMATRIX")||kw(&L->cur,"MATERIALIZE")){
    lex_next(L);
    if (kw(&L->cur,"ALL")||kw(&L->cur,"CHAIN")||kw(&L->cur,"WORLD")){
      lex_next(L);
      do_compile_all(vm); bump(vm); return 1;
    }
    if (L->cur.kind!=TK_IDENT){ fail(vm,"COMPILE cube|ALL"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    do_compile_cube(vm, id); bump(vm); return 1;
  }
  if (kw(&L->cur,"IMPULSE")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"IMPULSE"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int p=1; if (L->cur.kind==TK_NUM){ p=L->cur.num?1:0; lex_next(L); }
    cubalc_chain_impulse(&vm->ch,id,(uint8_t)p); bump(vm); return 1;
  }
  /* FLOW [DIR|IO] n — free-flow energy; DIR respects OUT→IN only (pluggable I/O).
   * TICK is reserved for COP engine world-step (METHOD tick on objects) in ops_flow. */
  if (kw(&L->cur,"FLOW")||kw(&L->cur,"ENERGY_TICK")||kw(&L->cur,"BOARD_TICK")){
    lex_next(L);
    int directed = 0;
    if (kw(&L->cur,"DIR")||kw(&L->cur,"DIRECTED")||kw(&L->cur,"IO")||kw(&L->cur,"OUT")){
      directed = 1; lex_next(L);
    }
    int n=8;
    if (L->cur.kind==TK_NUM){ n=(int)L->cur.num; lex_next(L); }
    if (n < 1) n = 1;
    if (n > 1000) n = 1000;
    ensure_world(vm);
    if (directed) {
      for (int i = 0; i < n; i++) cubalc_chain_flow_directed(&vm->ch);
    } else {
      do_flow(vm, n);
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"OS_ASPECTS")||kw(&L->cur,"SPAWN_OS")){
    lex_next(L); ensure_world(vm); cubalc_chain_os_aspects(&vm->ch); bump(vm); return 1;
  }
  /* HASFORM|NEEDFORM name — soft 0|1 / fail-fast if human-plane form is in HELP catalog.
   * Usability: capability probe without version floor or unknown-form crash.
   * Dual of REQUIRE FORM · multi-word: HASFORM "SYS ENV". */
  return 0;
}
