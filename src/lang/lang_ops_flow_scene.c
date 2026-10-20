/* CubalC lang — lang_ops_flow_scene.c
 * SPAWN · ENTITY · SCENE · TICK engine plane (A3 peel).
 * Mesh vote A3_carve_lang_ops_flow · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_flow_scene.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include <stdio.h>
#include <string.h>

int cubalc_lang_ops_flow_try_scene(VM *vm, Lex *L) {
  /* Game/engine plane. Returns 1 handled, 0 not this plane, -1 fail.
   */
  if (kw(&L->cur, "SPAWN") || kw(&L->cur, "SPAWNENTITY") ||
      kw(&L->cur, "SPAWN_UNIT")) {
    char cname[48], oname[48];
    ClassDef *cd;
    int ci, proton = 1;
    char role[48];
    lex_next(L);
    if (L->cur.kind != TK_IDENT) { fail(vm, "SPAWN ClassName"); return -1; }
    snprintf(cname, sizeof cname, "%s", L->cur.text);
    lex_next(L);
    if (oop_read_name(vm, L, oname, sizeof oname, "SPAWN name") < 0)
      return -1;
    ci = oop_find_class_idx(vm, cname);
    if (ci < 0) {
      char sug[48], ebuf[160], base[96];
      oop_suggest_class(vm, cname, sug, sizeof sug);
      snprintf(base, sizeof base, "SPAWN unknown CLASS %s", cname);
      oop_err_suggest(ebuf, sizeof ebuf, base, sug);
      fail(vm, ebuf); return -1;
    }
    cd = &vm->classes[ci];
    snprintf(role, sizeof role, "%s", cd->role[0] ? cd->role : "body");
    while (L->cur.kind == TK_IDENT) {
      if (kw(&L->cur, "ROLE")) {
        lex_next(L);
        if (L->cur.kind == TK_IDENT) {
          snprintf(role, sizeof role, "%s", L->cur.text);
          lex_next(L);
        }
      } else if (kw(&L->cur, "PROTON")) {
        lex_next(L);
        if (L->cur.kind == TK_NUM) {
          proton = L->cur.num ? 1 : 0;
          lex_next(L);
        }
      } else break;
    }
    place_cube(vm, oname, role, proton);
    if (oop_new_instance(vm, cname, oname, find_cube(vm, oname), L, 1, 0) < 0)
      return -1;
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "ENTITY", oname);
    bump(vm);
    return 1;
  }

  /* ENTITY name OF Class â alias CUBE OF for game engines (also handled in core) */
  if (kw(&L->cur, "ENTITY") || kw(&L->cur, "ACTOR") || kw(&L->cur, "UNIT")) {
    char id[48], of_class[48], role[48];
    int proton = 1;
    ClassDef *cd;
    int ci;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) { fail(vm, "ENTITY id"); return -1; }
    snprintf(id, sizeof id, "%s", L->cur.text);
    lex_next(L);
    of_class[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "AS") || kw(&L->cur, "CLASS") ||
        kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT) { fail(vm, "ENTITY OF Class"); return -1; }
      snprintf(of_class, sizeof of_class, "%s", L->cur.text);
      lex_next(L);
    }
    snprintf(role, sizeof role, "%s", id);
    while (L->cur.kind == TK_IDENT) {
      if (kw(&L->cur, "ROLE")) {
        lex_next(L);
        if (L->cur.kind == TK_IDENT) {
          snprintf(role, sizeof role, "%s", L->cur.text);
          lex_next(L);
        }
      } else if (kw(&L->cur, "PROTON")) {
        lex_next(L);
        if (L->cur.kind == TK_NUM) {
          proton = L->cur.num ? 1 : 0;
          lex_next(L);
        }
      } else break;
    }
    if (!of_class[0]) { fail(vm, "ENTITY needs OF ClassName"); return -1; }
    ci = oop_find_class_idx(vm, of_class);
    if (ci < 0) {
      {
        char sug[48], ebuf[160], base[96];
        oop_suggest_class(vm, of_class, sug, sizeof sug);
        snprintf(base, sizeof base, "ENTITY OF unknown CLASS %s", of_class);
        oop_err_suggest(ebuf, sizeof ebuf, base, sug);
        fail(vm, ebuf); return -1;
      }
    }
    cd = &vm->classes[ci];
    if (strcmp(role, id) == 0 && cd->role[0])
      snprintf(role, sizeof role, "%s", cd->role);
    place_cube(vm, id, role, proton);
    if (oop_new_instance(vm, of_class, id, find_cube(vm, id), L, 1, 0) < 0)
      return -1;
    var_set_str(vm, "ENTITY", id);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SCENE name â tag current scene for engine plates */
  if (kw(&L->cur, "SCENE") || kw(&L->cur, "LEVEL") || kw(&L->cur, "WORLD")) {
    lex_next(L);
    if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(vm->scene, sizeof vm->scene, "%s", L->cur.text);
      lex_next(L);
    } else {
      vm->scene[0] = 0;
    }
    var_set_str(vm, "SCENE", vm->scene);
    var_set_str(vm, "LAST", vm->scene);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* TICK [n] â SEND tick/update to every live object, then optional FLOW n */
  if (kw(&L->cur, "TICK") || kw(&L->cur, "UPDATE") || kw(&L->cur, "FRAME") ||
      kw(&L->cur, "STEPWORLD")) {
    long nflow = 0;
    int i, ntick = 0;
    lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
        L->cur.kind == TK_MINUS ||
        (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
         strcmp(L->cur.text, "LAST") != 0)) {
      /* only parse if looks numeric */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS)
        nflow = parse_expr(vm, L);
      else {
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && !v->is_str) {
          nflow = parse_expr(vm, L);
        }
      }
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      MethodDef *md;
      if (!ob->live) continue;
      cd = &vm->classes[ob->class_idx];
      md = oop_find_method(cd, "tick");
      if (!md) md = oop_find_method(cd, "update");
      if (!md) md = oop_find_method(cd, "frame");
      if (!md) continue;
      var_set_num(vm, "NARGS", 0);
      if (oop_run_method(vm, ob, md) < 0) return -1;
      ntick++;
    }
    if (nflow > 0) do_flow(vm, (int)nflow);
    var_set_num(vm, "TICK_N", ntick);
    var_set_num(vm, "LAST_N", ntick);
    vm->last_n = ntick;
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  return 0;
}
