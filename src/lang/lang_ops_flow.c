/* CubalC lang — lang_ops_flow.c (COP/flow · pure C · cube is SoT)
 * OOP plane: CLASS/FIELD/METHOD/NEW/SEND/GETF/SETF/ISOF
 * Engine plane: SCENE/ENTITY/SPAWN/TICK — game loop rides FLOW
 * Advanced vs C++: composition by PLUG, State Matrix SoT, flow-before-compile
 */
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static ClassDef *oop_find_class(VM *vm, const char *name){
  int i;
  for (i = 0; i < vm->n_classes; i++)
    if (strcmp(vm->classes[i].name, name) == 0) return &vm->classes[i];
  return NULL;
}
static int oop_find_class_idx(VM *vm, const char *name){
  int i;
  for (i = 0; i < vm->n_classes; i++)
    if (strcmp(vm->classes[i].name, name) == 0) return i;
  return -1;
}
static ObjInst *oop_find_obj(VM *vm, const char *name){
  int i;
  for (i = 0; i < vm->n_objs; i++)
    if (vm->objs[i].live && strcmp(vm->objs[i].name, name) == 0)
      return &vm->objs[i];
  return NULL;
}
static int oop_field_idx(ClassDef *cd, const char *fname){
  int i;
  for (i = 0; i < cd->n_fields; i++)
    if (strcmp(cd->fields[i].name, fname) == 0) return i;
  return -1;
}
static MethodDef *oop_find_method(ClassDef *cd, const char *mname){
  int i;
  for (i = 0; i < cd->n_methods; i++)
    if (strcmp(cd->methods[i].name, mname) == 0) return &cd->methods[i];
  return NULL;
}
static int oop_resolve_obj_name(VM *vm, Lex *L, char *out, size_t outn){
  if (L->cur.kind == TK_STR) {
    if (!L->cur.text[0]) return -1;
    snprintf(out, outn, "%s", L->cur.text);
    lex_next(L);
    return 0;
  }
  if (L->cur.kind != TK_IDENT) return -1;
  if (strcasecmp(L->cur.text, "THIS") == 0 || strcasecmp(L->cur.text, "SELF") == 0) {
    if (!vm->this_obj[0]) return -1;
    snprintf(out, outn, "%s", vm->this_obj);
    lex_next(L);
    return 0;
  }
  /* LAST / string-var as object name when value is a *live* object.
   * NEW stores var(name)=Class as string — do NOT expand that pollution;
   * only expand when sval names a live instance (EACH OBJ bind, LET peer=…). */
  if (strcmp(L->cur.text, "LAST") == 0) {
    if (vm->last_str[0] && oop_find_obj(vm, vm->last_str)) {
      snprintf(out, outn, "%s", vm->last_str);
      lex_next(L);
      return 0;
    }
  } else {
    Var *sv = var_get(vm, L->cur.text, 0);
    if (sv && sv->is_str && sv->sval[0] && oop_find_obj(vm, sv->sval)) {
      snprintf(out, outn, "%s", sv->sval);
      lex_next(L);
      return 0;
    }
  }
  snprintf(out, outn, "%s", L->cur.text);
  lex_next(L);
  return 0;
}
static int oop_stmt_kw(Lex *L){
  return L->cur.kind == TK_IDENT &&
         (kw(&L->cur, "END") || kw(&L->cur, "LET") || kw(&L->cur, "ASSERT") ||
          kw(&L->cur, "PRINT") || kw(&L->cur, "IF") || kw(&L->cur, "FOR") ||
          kw(&L->cur, "WHILE") || kw(&L->cur, "CALL") || kw(&L->cur, "SEND") ||
          kw(&L->cur, "RET") || kw(&L->cur, "RETURN") || kw(&L->cur, "CUBE") ||
          kw(&L->cur, "SYS") || kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") ||
          kw(&L->cur, "GETF") || kw(&L->cur, "SETF") || kw(&L->cur, "METHOD") ||
          kw(&L->cur, "FIELD") || kw(&L->cur, "ENTITY") || kw(&L->cur, "SPAWN") ||
          kw(&L->cur, "TICK") || kw(&L->cur, "SCENE") || kw(&L->cur, "FLOW") ||
          kw(&L->cur, "PLUG") || kw(&L->cur, "TYPE"));
}
static int oop_bind_args(VM *vm, Lex *L, char params[][32], int n_params){
  int ai = 0;
  while (ai < 8 &&
         (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN)) {
    char an[16];
    snprintf(an, sizeof an, "ARG%d", ai);
    if (L->cur.kind == TK_STR) {
      var_set_str(vm, an, L->cur.text);
      if (ai < n_params && params[ai][0])
        var_set_str(vm, params[ai], L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      /* String formals: bind string vars / LAST by value (not strlen via parse_expr). */
      if (strcmp(L->cur.text, "LAST") == 0) {
        var_set_str(vm, an, vm->last_str);
        if (ai < n_params && params[ai][0])
          var_set_str(vm, params[ai], vm->last_str);
        lex_next(L);
      } else {
        Var *sv = var_get(vm, L->cur.text, 0);
        if (sv && sv->is_str) {
          var_set_str(vm, an, sv->sval);
          if (ai < n_params && params[ai][0])
            var_set_str(vm, params[ai], sv->sval);
          lex_next(L);
        } else {
          long v = parse_expr(vm, L);
          var_set_num(vm, an, v);
          if (ai < n_params && params[ai][0])
            var_set_num(vm, params[ai], v);
        }
      }
    } else {
      if (oop_stmt_kw(L)) break;
      {
        long v = parse_expr(vm, L);
        var_set_num(vm, an, v);
        if (ai < n_params && params[ai][0])
          var_set_num(vm, params[ai], v);
      }
    }
    ai++;
  }
  var_set_num(vm, "NARGS", ai);
  return ai;
}
/* Resolve instance name: bare token, string literal, or string-var value.
 * NEW sets var(name)=Class as string — so bare re-NEW of a free slot must NOT
 * expand that pollution. Prefer literal when an obj slot (live or dead) already
 * uses this exact name (pool recycle). Else expand string-var for dynamic slots
 * (LET slot="dyn1"; NEW Cell slot). */
static int oop_read_name(VM *vm, Lex *L, char *out, size_t outn, const char *why){
  if (L->cur.kind == TK_STR) {
    if (!L->cur.text[0]) { fail(vm, why); return -1; }
    snprintf(out, outn, "%s", L->cur.text);
    lex_next(L);
    return 0;
  }
  if (L->cur.kind != TK_IDENT) { fail(vm, why); return -1; }
  if (strcmp(L->cur.text, "LAST") == 0) {
    if (!vm->last_str[0]) { fail(vm, why); return -1; }
    snprintf(out, outn, "%s", vm->last_str);
    lex_next(L);
    return 0;
  }
  {
    int i;
    for (i = 0; i < vm->n_objs; i++) {
      if (strcmp(vm->objs[i].name, L->cur.text) == 0) {
        snprintf(out, outn, "%s", L->cur.text);
        lex_next(L);
        return 0;
      }
    }
  }
  {
    Var *sv = var_get(vm, L->cur.text, 0);
    if (sv && sv->is_str && sv->sval[0]) {
      snprintf(out, outn, "%s", sv->sval);
      lex_next(L);
      return 0;
    }
  }
  /* bare identifier = literal object name token */
  snprintf(out, outn, "%s", L->cur.text);
  lex_next(L);
  return 0;
}
static int oop_run_method(VM *vm, ObjInst *ob, MethodDef *md){
  ClassDef *cd;
  char save_this[48];
  if (!ob || !md || ob->class_idx < 0 || ob->class_idx >= vm->n_classes)
    return -1;
  cd = &vm->classes[ob->class_idx];
  snprintf(save_this, sizeof save_this, "%s", vm->this_obj);
  snprintf(vm->this_obj, sizeof vm->this_obj, "%s", ob->name);
  var_set_str(vm, "THIS", ob->name);
  var_set_str(vm, "SELF", ob->name);
  var_set_str(vm, "CLASS", cd->name);
  var_set_num(vm, "CALLED", 1);
  vm->return_fn = 0;
  {
    Lex fl;
    lex_init(&fl, md->body, md->len);
    if (exec_stmts_until(vm, &fl, "END", NULL) < 0) {
      snprintf(vm->this_obj, sizeof vm->this_obj, "%s", save_this);
      return -1;
    }
  }
  vm->return_fn = 0;
  snprintf(vm->this_obj, sizeof vm->this_obj, "%s", save_this);
  if (save_this[0]) {
    var_set_str(vm, "THIS", save_this);
    var_set_str(vm, "SELF", save_this);
  }
  return 0;
}
static int oop_new_instance(VM *vm, const char *cname, const char *oname,
                            int cube_idx, Lex *L, int bind_ctor){
  int ci = oop_find_class_idx(vm, cname);
  ClassDef *cd;
  ObjInst *ob;
  int fi, i, reuse = -1;
  MethodDef *initm;
  if (ci < 0) {
    snprintf(vm->err, sizeof vm->err, "unknown CLASS %s", cname);
    fail(vm, vm->err);
    return -1;
  }
  if (oop_find_obj(vm, oname)) {
    snprintf(vm->err, sizeof vm->err, "object redefine %s", oname);
    fail(vm, vm->err);
    return -1;
  }
  /* Reuse DESTROY'd slot with same name (life_engine free slots). */
  for (i = 0; i < vm->n_objs; i++) {
    if (!vm->objs[i].live && strcmp(vm->objs[i].name, oname) == 0) {
      reuse = i;
      break;
    }
  }
  if (reuse < 0 && vm->n_objs >= CUBALC_MAX_OBJS) {
    /* pack: reuse any dead slot */
    for (i = 0; i < vm->n_objs; i++) {
      if (!vm->objs[i].live) { reuse = i; break; }
    }
  }
  if (reuse < 0 && vm->n_objs >= CUBALC_MAX_OBJS) {
    fail(vm, "too many objects");
    return -1;
  }
  cd = &vm->classes[ci];
  if (reuse >= 0)
    ob = &vm->objs[reuse];
  else
    ob = &vm->objs[vm->n_objs++];
  memset(ob, 0, sizeof *ob);
  snprintf(ob->name, sizeof ob->name, "%s", oname);
  ob->class_idx = ci;
  ob->live = 1;
  ob->cube_idx = cube_idx;
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
  var_set_str(vm, oname, cname);
  var_set_str(vm, "OBJECT", oname);
  var_set_str(vm, "CLASS", cname);
  if (!bind_ctor || !L) return 0;
  initm = oop_find_method(cd, "init");
  if (!initm) initm = oop_find_method(cd, "construct");
  if (!initm) initm = oop_find_method(cd, "spawn");
  if (!initm) initm = oop_find_method(cd, "new");
  if (initm) {
    oop_bind_args(vm, L, initm->params, initm->n_params);
    if (oop_run_method(vm, ob, initm) < 0) return -1;
  } else {
    while (L->cur.kind == TK_NUM || L->cur.kind == TK_STR ||
           L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
           (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
      if (L->cur.kind == TK_STR) lex_next(L);
      else (void)parse_expr(vm, L);
    }
  }
  return 0;
}

int cubalc_lang_ops_flow(VM *vm, Lex *L){
  /* ---- OOP: CLASS / TYPE … FIELD … METHOD … END ---- */
  if (kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
    lex_next(L);
    if (L->cur.kind != TK_IDENT) { fail(vm, "CLASS name"); return -1; }
    char cname[48];
    snprintf(cname, sizeof cname, "%s", L->cur.text);
    lex_next(L);
    skip_nl(L);
    if (vm->n_classes >= CUBALC_MAX_CLASSES) {
      fail(vm, "too many CLASS"); return -1;
    }
    if (oop_find_class(vm, cname)) {
      snprintf(vm->err, sizeof vm->err, "CLASS redefine %s", cname);
      fail(vm, vm->err); return -1;
    }
    {
      ClassDef *cd = &vm->classes[vm->n_classes++];
      memset(cd, 0, sizeof *cd);
      snprintf(cd->name, sizeof cd->name, "%s", cname);
      snprintf(cd->role, sizeof cd->role, "body");
      for (;;) {
        skip_nl(L);
        if (L->cur.kind == TK_EOF) { fail(vm, "CLASS without END"); return -1; }
        if (kw(&L->cur, "END")) { lex_next(L); break; }
        if (kw(&L->cur, "FIELD") || kw(&L->cur, "VAR") || kw(&L->cur, "PROP") ||
            kw(&L->cur, "MEMBER") || kw(&L->cur, "COMPONENT") ||
            kw(&L->cur, "ATTR")) {
          lex_next(L);
          if (L->cur.kind != TK_IDENT) { fail(vm, "FIELD name"); return -1; }
          if (cd->n_fields >= CUBALC_MAX_FIELDS) {
            fail(vm, "too many FIELD"); return -1;
          }
          {
            FieldDef *fd = &cd->fields[cd->n_fields++];
            memset(fd, 0, sizeof *fd);
            snprintf(fd->name, sizeof fd->name, "%s", L->cur.text);
            lex_next(L);
            if (L->cur.kind == TK_EQ) lex_next(L);
            if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                L->cur.kind == TK_LPAREN) {
              fd->def_num = parse_expr(vm, L);
              fd->has_def = 1;
              fd->is_str = 0;
            } else if (L->cur.kind == TK_STR) {
              snprintf(fd->def_str, sizeof fd->def_str, "%s", L->cur.text);
              fd->is_str = 1;
              fd->has_def = 1;
              lex_next(L);
            } else if (L->cur.kind == TK_IDENT && !kw(&L->cur, "FIELD") &&
                       !kw(&L->cur, "METHOD") && !kw(&L->cur, "END") &&
                       !kw(&L->cur, "ROLE") && !kw(&L->cur, "COMPONENT") &&
                       !kw(&L->cur, "ATTR")) {
              long v = parse_expr(vm, L);
              fd->def_num = v;
              fd->has_def = 1;
              fd->is_str = 0;
            }
          }
          continue;
        }
        if (kw(&L->cur, "ROLE")) {
          lex_next(L);
          if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
            snprintf(cd->role, sizeof cd->role, "%s", L->cur.text);
            lex_next(L);
          }
          continue;
        }
        if (kw(&L->cur, "METHOD") || kw(&L->cur, "FN") || kw(&L->cur, "FUNC") ||
            kw(&L->cur, "DEF") || kw(&L->cur, "UPDATE") || kw(&L->cur, "HANDLER")) {
          lex_next(L);
          if (L->cur.kind != TK_IDENT) { fail(vm, "METHOD name"); return -1; }
          if (cd->n_methods >= CUBALC_MAX_METHODS) {
            fail(vm, "too many METHOD"); return -1;
          }
          {
            MethodDef *md = &cd->methods[cd->n_methods++];
            size_t b0, b1;
            int depth = 1;
            memset(md, 0, sizeof *md);
            snprintf(md->name, sizeof md->name, "%s", L->cur.text);
            lex_next(L);
            while (md->n_params < 8 && L->cur.kind == TK_IDENT &&
                   !kw(&L->cur, "END") && !kw(&L->cur, "THEN")) {
              snprintf(md->params[md->n_params], sizeof md->params[0], "%s",
                       L->cur.text);
              md->n_params++;
              lex_next(L);
            }
            skip_nl(L);
            b0 = L->tok_off;
            while (L->cur.kind != TK_EOF) {
              if (block_scan_step(L, &depth, 0)) break;
            }
            if (depth != 0) { fail(vm, "METHOD without END"); return -1; }
            b1 = L->tok_off;
            if (b1 < b0) b1 = b0;
            md->body = L->s + b0;
            md->len = b1 - b0;
            if (kw(&L->cur, "END")) lex_next(L);
          }
          continue;
        }
        snprintf(vm->err, sizeof vm->err, "CLASS %s unknown form '%s'", cname,
                 L->cur.text[0] ? L->cur.text : "?");
        fail(vm, vm->err);
        return -1;
      }
      var_set_str(vm, "CLASS", cname);
      var_set_num(vm, "NFIELDS", cd->n_fields);
      var_set_num(vm, "NMETHODS", cd->n_methods);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# CLASS %s fields=%d methods=%d\n", cname,
                cd->n_fields, cd->n_methods);
    }
    bump(vm);
    return 1;
  }

  /* NEW ClassName instance [ctor args…]
   * instance may be bare token, "str", or string-var value (dynamic slot names). */
  if (kw(&L->cur, "NEW") || kw(&L->cur, "MAKE") || kw(&L->cur, "CREATE")) {
    char cname[48], oname[48];
    lex_next(L);
    if (L->cur.kind != TK_IDENT) { fail(vm, "NEW ClassName"); return -1; }
    snprintf(cname, sizeof cname, "%s", L->cur.text);
    lex_next(L);
    if (oop_read_name(vm, L, oname, sizeof oname, "NEW instance name") < 0)
      return -1;
    if (oop_new_instance(vm, cname, oname, -1, L, 1) < 0) return -1;
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# NEW %s %s\n", cname, oname);
    bump(vm);
    return 1;
  }

  /* SPAWN ClassName name [args] — game: NEW + optional cube place */
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
      snprintf(vm->err, sizeof vm->err, "SPAWN unknown CLASS %s", cname);
      fail(vm, vm->err); return -1;
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
    if (oop_new_instance(vm, cname, oname, find_cube(vm, oname), L, 1) < 0)
      return -1;
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "ENTITY", oname);
    bump(vm);
    return 1;
  }

  /* ENTITY name OF Class — alias CUBE OF for game engines (also handled in core) */
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
      snprintf(vm->err, sizeof vm->err, "ENTITY OF unknown CLASS %s", of_class);
      fail(vm, vm->err); return -1;
    }
    cd = &vm->classes[ci];
    if (strcmp(role, id) == 0 && cd->role[0])
      snprintf(role, sizeof role, "%s", cd->role);
    place_cube(vm, id, role, proton);
    if (oop_new_instance(vm, of_class, id, find_cube(vm, id), L, 1) < 0)
      return -1;
    var_set_str(vm, "ENTITY", id);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SCENE name — tag current scene for engine plates */
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

  /* TICK [n] — SEND tick/update to every live object, then optional FLOW n */
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

  /* SEND obj method [args]
   * TRYSEND|SENDSOFT|SEND SOFT — soft miss OK=0 for unknown obj/method.
   * Method may be IDENT, "string", or string-var (LISTMETHODS walk).
   * Bare SEND still fatal. Usability: agent dispatch after HASMETHOD. */
  if (kw(&L->cur, "SEND") || kw(&L->cur, "CALLMETHOD") || kw(&L->cur, "INVOKE") ||
      kw(&L->cur, "DOMETHOD") || kw(&L->cur, "MSG") || kw(&L->cur, "EMIT") ||
      kw(&L->cur, "TRYSEND") || kw(&L->cur, "SENDSOFT") ||
      kw(&L->cur, "SOFTSEND") || kw(&L->cur, "TRYCALLMETHOD") ||
      kw(&L->cur, "TRYINVOKE")) {
    char oname[48], mname[48], op[24];
    ObjInst *ob;
    ClassDef *cd;
    MethodDef *md;
    int soft = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "TRYSEND") == 0 || strcmp(op, "SENDSOFT") == 0 ||
        strcmp(op, "SOFTSEND") == 0 || strcmp(op, "TRYCALLMETHOD") == 0 ||
        strcmp(op, "TRYINVOKE") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "SEND object"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      vv = var_get(vm, id, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else
        snprintf(mname, sizeof mname, "%s", id);
    } else {
      fail(vm, "SEND method"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "SEND unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      /* drain optional args so next form stays aligned */
      while (L->cur.kind == TK_NUM || L->cur.kind == TK_STR ||
             L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
        if (L->cur.kind == TK_STR) lex_next(L);
        else (void)parse_expr(vm, L);
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "SEND_N", 0);
      var_set_num(vm, "TRYSEND_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "SEND: unknown object");
      var_set_str(vm, "ERR", "SEND: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    md = oop_find_method(cd, mname);
    if (!md) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "SEND unknown METHOD %s.%s", cd->name,
                 mname);
        fail(vm, vm->err); return -1;
      }
      while (L->cur.kind == TK_NUM || L->cur.kind == TK_STR ||
             L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
        if (L->cur.kind == TK_STR) lex_next(L);
        else (void)parse_expr(vm, L);
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "SEND_N", 0);
      var_set_num(vm, "TRYSEND_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "SEND: unknown method");
      var_set_str(vm, "ERR", "SEND: unknown method");
      bump(vm);
      return 1;
    }
    oop_bind_args(vm, L, md->params, md->n_params);
    if (oop_run_method(vm, ob, md) < 0) return -1;
    var_set_num(vm, "SEND_N", 1);
    var_set_num(vm, "TRYSEND_N", 1);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SENDALL|BROADCAST [Class] method [args]
   * — invoke method on every live object (optional class filter).
   * Skips objects missing the method (soft). LAST_N/SENDALL_N = call count.
   * Usability: fleet tick without EACH OBJ + SEND glue (beyond TICK). */
  if (kw(&L->cur, "SENDALL") || kw(&L->cur, "BROADCAST") ||
      kw(&L->cur, "INVOKEALL") || kw(&L->cur, "CALLALL") ||
      kw(&L->cur, "MAPSEND") || kw(&L->cur, "FORALLSEND")) {
    char filt[48], mname[48], tok1[48];
    int has_filt = 0, i, n = 0, n_skip = 0, advanced = 0;
    Lex args_start;
    lex_next(L);
    filt[0] = 0;
    mname[0] = 0;
    tok1[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "SENDALL [Class] method [args]"); return -1;
    }
    snprintf(tok1, sizeof tok1, "%s", L->cur.text);
    lex_next(L);
    /* Class method when tok1 names a known CLASS and next is method */
    if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
        oop_find_class(vm, tok1)) {
      snprintf(filt, sizeof filt, "%s", tok1);
      has_filt = 1;
      if (L->cur.kind == TK_STR) {
        snprintf(mname, sizeof mname, "%s", L->cur.text);
        lex_next(L);
      } else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(mname, sizeof mname, "%s", vv->sval);
        else
          snprintf(mname, sizeof mname, "%s", L->cur.text);
        lex_next(L);
      }
    } else {
      Var *vv = var_get(vm, tok1, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else
        snprintf(mname, sizeof mname, "%s", tok1);
    }
    args_start = *L;
    for (i = 0; i < vm->n_objs && !vm->fatal && !vm->halt; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      MethodDef *md;
      Lex bl;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      md = oop_find_method(cd, mname);
      if (!md) { n_skip++; continue; }
      bl = args_start;
      oop_bind_args(vm, &bl, md->params, md->n_params);
      *L = bl;
      advanced = 1;
      if (oop_run_method(vm, ob, md) < 0) return -1;
      n++;
    }
    if (!advanced) {
      Lex bl = args_start;
      char dummy[8][32];
      memset(dummy, 0, sizeof dummy);
      oop_bind_args(vm, &bl, dummy, 0);
      *L = bl;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "SENDALL_N", n);
    var_set_num(vm, "BROADCAST_N", n);
    var_set_num(vm, "SENDALL_SKIP", n_skip);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SENDWHERE|INVOKEWHERE [Class] field value method [args]
   * — call method on every live object whose field equals value.
   * Soft always; missing method → SENDWHERE_SKIP. LAST_N = call count.
   * Usability: WHEREOBJ + SENDALL one-shot · no EACH+GETF+IF+SEND glue. */
  if (kw(&L->cur, "SENDWHERE") || kw(&L->cur, "INVOKEWHERE") ||
      kw(&L->cur, "CALLWHERE") || kw(&L->cur, "BROADCASTWHERE") ||
      kw(&L->cur, "MAPSENDWHERE") || kw(&L->cur, "FORWHERE")) {
    char filt[48], fname[48], tok1[48], mname[48], sval[512];
    int has_filt = 0, is_str = 0, i, n = 0, n_skip = 0, advanced = 0;
    long nval = 0;
    Lex args_start;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    mname[0] = 0;
    sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SENDWHERE OF Class field value method"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "SENDWHERE [Class] field value method"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SENDWHERE [Class] field value method"); return -1;
      }
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(sval, sizeof sval, "%s", L->cur.text);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(sval, sizeof sval, "%s", vm->last_str);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      /* method name may follow immediately if value is bare ident — only
       * treat as string-var when next token can still be method. Prefer num
       * var via parse_expr if not pure string. */
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(sval, sizeof sval, "%s", sv->sval);
        is_str = 1;
        lex_next(L);
      } else {
        nval = parse_expr(vm, L);
        is_str = 0;
      }
    } else {
      nval = parse_expr(vm, L);
      is_str = 0;
    }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "SENDWHERE [Class] field value method"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else
        snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    }
    args_start = *L;
    for (i = 0; i < vm->n_objs && !vm->fatal && !vm->halt; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      MethodDef *md;
      Lex bl;
      int fi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) {
        if (is_str)
          hit = (strcmp(ob->fstr[fi], sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", nval);
          hit = (strcmp(ob->fstr[fi], nb) == 0);
        }
      } else {
        if (is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[fi]);
          hit = (strcmp(nb, sval) == 0);
        } else {
          hit = (ob->fnum[fi] == nval);
        }
      }
      if (!hit) continue;
      md = oop_find_method(cd, mname);
      if (!md) { n_skip++; continue; }
      bl = args_start;
      oop_bind_args(vm, &bl, md->params, md->n_params);
      *L = bl;
      advanced = 1;
      if (oop_run_method(vm, ob, md) < 0) return -1;
      n++;
    }
    if (!advanced) {
      Lex bl = args_start;
      char dummy[8][32];
      memset(dummy, 0, sizeof dummy);
      oop_bind_args(vm, &bl, dummy, 0);
      *L = bl;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "SENDWHERE_N", n);
    var_set_num(vm, "INVOKEWHERE_N", n);
    var_set_num(vm, "SENDWHERE_SKIP", n_skip);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "METHOD", mname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SENDWHEREGE|SENDWHERELE|SENDWHEREGT|SENDWHERELT [Class] field value method [args]
   * INVOKEWHEREGE|CALLWHEREGE|SENDATLEAST|SENDABOVE|SENDBELOW aliases.
   * — call method on every live object whose numeric field meets threshold.
   * Soft always; missing method / string fields → SENDWHEREGE_SKIP.
   * Usability: WHEREGE + SENDALL one-shot · tick high-energy cells / drain low
   * counters without EACH+GETF+IF+SEND. Completes threshold triad with
   * WHEREGE + DELETEWHEREGE. */
  if (kw(&L->cur, "SENDWHEREGE") || kw(&L->cur, "SENDWHEREGTE") ||
      kw(&L->cur, "INVOKEWHEREGE") || kw(&L->cur, "CALLWHEREGE") ||
      kw(&L->cur, "SENDATLEAST") || kw(&L->cur, "MAPSENDWHEREGE") ||
      kw(&L->cur, "SENDWHEREGT") || kw(&L->cur, "INVOKEWHEREGT") ||
      kw(&L->cur, "CALLWHEREGT") || kw(&L->cur, "SENDABOVE") ||
      kw(&L->cur, "INVOKEABOVE") ||
      kw(&L->cur, "SENDWHERELE") || kw(&L->cur, "SENDWHERELTE") ||
      kw(&L->cur, "INVOKEWHERELE") || kw(&L->cur, "CALLWHERELE") ||
      kw(&L->cur, "SENDATMOST") ||
      kw(&L->cur, "SENDWHERELT") || kw(&L->cur, "INVOKEWHERELT") ||
      kw(&L->cur, "CALLWHERELT") || kw(&L->cur, "SENDBELOW") ||
      kw(&L->cur, "INVOKEBELOW") ||
      kw(&L->cur, "THRESHSEND") || kw(&L->cur, "SENDTHRESH")) {
    char filt[48], fname[48], tok1[48], mname[48], op[32];
    int has_filt = 0, mode = 0, i, n = 0, n_skip = 0, advanced = 0;
    long thresh = 0;
    Lex args_start;
    /* mode: 0=GE 1=GT 2=LE 3=LT */
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "SENDWHEREGT") == 0 || strcmp(op, "INVOKEWHEREGT") == 0 ||
        strcmp(op, "CALLWHEREGT") == 0 || strcmp(op, "SENDABOVE") == 0 ||
        strcmp(op, "INVOKEABOVE") == 0)
      mode = 1;
    else if (strcmp(op, "SENDWHERELE") == 0 || strcmp(op, "SENDWHERELTE") == 0 ||
             strcmp(op, "INVOKEWHERELE") == 0 || strcmp(op, "CALLWHERELE") == 0 ||
             strcmp(op, "SENDATMOST") == 0)
      mode = 2;
    else if (strcmp(op, "SENDWHERELT") == 0 || strcmp(op, "INVOKEWHERELT") == 0 ||
             strcmp(op, "CALLWHERELT") == 0 || strcmp(op, "SENDBELOW") == 0 ||
             strcmp(op, "INVOKEBELOW") == 0)
      mode = 3;
    else
      mode = 0; /* GE / ATLEAST / THRESHSEND */
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    mname[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SENDWHEREGE OF Class field value method"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "SENDWHEREGE [Class] field value method"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SENDWHEREGE [Class] field value method"); return -1;
      }
    }
    if (kw(&L->cur, "GE") || kw(&L->cur, "GTE") || kw(&L->cur, "GT") ||
        kw(&L->cur, "LE") || kw(&L->cur, "LTE") || kw(&L->cur, "LT") ||
        kw(&L->cur, "MIN") || kw(&L->cur, "MAX"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      thresh = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      thresh = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        thresh = atol(sv->sval);
      else
        thresh = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      thresh = parse_expr(vm, L);
    }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "SENDWHEREGE [Class] field value method"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else
        snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    }
    args_start = *L;
    for (i = 0; i < vm->n_objs && !vm->fatal && !vm->halt; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      MethodDef *md;
      Lex bl;
      int fi, hit = 0;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (mode == 0) hit = (v >= thresh);
      else if (mode == 1) hit = (v > thresh);
      else if (mode == 2) hit = (v <= thresh);
      else hit = (v < thresh);
      if (!hit) continue;
      md = oop_find_method(cd, mname);
      if (!md) { n_skip++; continue; }
      bl = args_start;
      oop_bind_args(vm, &bl, md->params, md->n_params);
      *L = bl;
      advanced = 1;
      if (oop_run_method(vm, ob, md) < 0) return -1;
      n++;
    }
    if (!advanced) {
      Lex bl = args_start;
      char dummy[8][32];
      memset(dummy, 0, sizeof dummy);
      oop_bind_args(vm, &bl, dummy, 0);
      *L = bl;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "SENDWHEREGE_N", n);
    var_set_num(vm, "SENDWHERELE_N", n);
    var_set_num(vm, "SENDWHEREGT_N", n);
    var_set_num(vm, "SENDWHERELT_N", n);
    var_set_num(vm, "INVOKEWHEREGE_N", n);
    var_set_num(vm, "SENDATLEAST_N", n);
    var_set_num(vm, "SENDATMOST_N", n);
    var_set_num(vm, "SENDABOVE_N", n);
    var_set_num(vm, "SENDBELOW_N", n);
    var_set_num(vm, "SENDWHEREGE_SKIP", n_skip);
    var_set_num(vm, "SENDWHEREGE_THRESH", thresh);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "METHOD", mname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SENDBETWEEN|INVOKEBETWEEN|CALLBETWEEN|SENDBAND [Class] field lo [TO] hi method [args]
   * — call method on every live obj with lo <= numeric field <= hi.
   * Soft always; missing method / string fields → SENDBETWEEN_SKIP.
   * Completes BETWEEN triad: WHEREBETWEEN · DELETEBETWEEN · SENDBETWEEN.
   * Usability: band tick/drain without dual SENDWHEREGE+SENDWHERELE. */
  if (kw(&L->cur, "SENDBETWEEN") || kw(&L->cur, "INVOKEBETWEEN") ||
      kw(&L->cur, "CALLBETWEEN") || kw(&L->cur, "SENDBAND") ||
      kw(&L->cur, "MAPSENDBETWEEN") || kw(&L->cur, "FORBETWEEN") ||
      kw(&L->cur, "BROADCASTBETWEEN") || kw(&L->cur, "SENDINRANGE")) {
    char filt[48], fname[48], tok1[48], mname[48];
    int has_filt = 0, i, n = 0, n_skip = 0, advanced = 0;
    long lo = 0, hi = 0, t;
    Lex args_start;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    mname[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SENDBETWEEN OF Class field lo hi method"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "SENDBETWEEN [Class] field lo hi method"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SENDBETWEEN [Class] field lo hi method"); return -1;
      }
    }
    if (kw(&L->cur, "IN") || kw(&L->cur, "BETWEEN") || kw(&L->cur, "RANGE"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      lo = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      lo = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        lo = atol(sv->sval);
      else
        lo = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      lo = parse_expr(vm, L);
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "AND") || kw(&L->cur, "THRU") ||
        kw(&L->cur, "THROUGH") || kw(&L->cur, "UNTIL") ||
        (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "..") == 0))
      lex_next(L);
    else if (L->cur.kind == TK_COMMA)
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      hi = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      hi = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        hi = atol(sv->sval);
      else
        hi = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      hi = parse_expr(vm, L);
    }
    if (lo > hi) { t = lo; lo = hi; hi = t; }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "SENDBETWEEN [Class] field lo hi method"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else
        snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    }
    args_start = *L;
    for (i = 0; i < vm->n_objs && !vm->fatal && !vm->halt; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      MethodDef *md;
      Lex bl;
      int fi;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (v < lo || v > hi) continue;
      md = oop_find_method(cd, mname);
      if (!md) { n_skip++; continue; }
      bl = args_start;
      oop_bind_args(vm, &bl, md->params, md->n_params);
      *L = bl;
      advanced = 1;
      if (oop_run_method(vm, ob, md) < 0) return -1;
      n++;
    }
    if (!advanced) {
      Lex bl = args_start;
      char dummy[8][32];
      memset(dummy, 0, sizeof dummy);
      oop_bind_args(vm, &bl, dummy, 0);
      *L = bl;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "SENDBETWEEN_N", n);
    var_set_num(vm, "INVOKEBETWEEN_N", n);
    var_set_num(vm, "CALLBETWEEN_N", n);
    var_set_num(vm, "SENDBAND_N", n);
    var_set_num(vm, "SENDINRANGE_N", n);
    var_set_num(vm, "SENDBETWEEN_SKIP", n_skip);
    var_set_num(vm, "SENDBETWEEN_LO", lo);
    var_set_num(vm, "SENDBETWEEN_HI", hi);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "METHOD", mname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* GETFALL|COLLECTF|MAPGETF [Class] field [AS KV|WITH NAMES]
   * — collect field values from every live object (optional class filter).
   * Soft always; objects missing the field are skipped (GETFALL_SKIP).
   * LAST = newline bag of values (or name:value with AS KV); LAST_N/GETFALL_N = count.
   * Usability: fleet field harvest without EACH OBJ + GETF + PUSH glue (SUM/AVG/FREQ ready). */
  if (kw(&L->cur, "GETFALL") || kw(&L->cur, "COLLECTF") ||
      kw(&L->cur, "MAPGETF") || kw(&L->cur, "GETF_ALL") ||
      kw(&L->cur, "FIELDALL") || kw(&L->cur, "COLLECTFIELD") ||
      kw(&L->cur, "PLUCKALL") || kw(&L->cur, "MAPFIELD")) {
    char filt[48], fname[48], tok1[48], bag[4096];
    int has_filt = 0, as_kv = 0, i, n = 0, n_skip = 0;
    size_t o = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    bag[0] = 0;
    /* OF|CLASS|TYPE Class field */
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "GETFALL OF Class field"); return -1;
      }
      if (L->cur.kind == TK_STR)
        snprintf(filt, sizeof filt, "%s", L->cur.text);
      else
        snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      /* Class field when tok1 names a known CLASS and next is field */
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "AS") && !kw(&L->cur, "WITH") && !kw(&L->cur, "KV") &&
          !kw(&L->cur, "NAMES") && !kw(&L->cur, "ASNAMES")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        /* bare field (or class-less); resolve string-var */
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] && L->cur.kind != TK_STR &&
            !(L->cur.kind == TK_IDENT && oop_find_class(vm, tok1)))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else if (tok1[0] == 0) {
          fail(vm, "GETFALL [Class] field"); return -1;
        } else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "GETFALL [Class] field"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT &&
                 !kw(&L->cur, "AS") && !kw(&L->cur, "WITH") &&
                 !kw(&L->cur, "KV") && !kw(&L->cur, "NAMES")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "GETFALL [Class] field"); return -1;
      }
    }
    /* optional AS KV | WITH NAMES | KV — name:value bag for LOOKUP */
    if (kw(&L->cur, "AS") || kw(&L->cur, "WITH")) {
      lex_next(L);
      if (kw(&L->cur, "KV") || kw(&L->cur, "NAMES") || kw(&L->cur, "NAME") ||
          kw(&L->cur, "PAIRS") || kw(&L->cur, "MAP") || kw(&L->cur, "OBJECTS")) {
        as_kv = 1;
        lex_next(L);
      }
    } else if (kw(&L->cur, "KV") || kw(&L->cur, "NAMES") ||
               kw(&L->cur, "ASNAMES") || kw(&L->cur, "WITHNAMES") ||
               kw(&L->cur, "PAIRS")) {
      as_kv = 1;
      lex_next(L);
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      char line[256];
      size_t ln;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (as_kv) {
        if (ob->fis_str[fi])
          snprintf(line, sizeof line, "%s:%s", ob->name, ob->fstr[fi]);
        else
          snprintf(line, sizeof line, "%s:%ld", ob->name, ob->fnum[fi]);
      } else {
        if (ob->fis_str[fi])
          snprintf(line, sizeof line, "%s", ob->fstr[fi]);
        else
          snprintf(line, sizeof line, "%ld", ob->fnum[fi]);
      }
      ln = strlen(line);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, line, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "GETFALL", bag);
    var_set_str(vm, "COLLECTF", bag);
    var_set_str(vm, "MAPGETF", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "GETFALL_N", n);
    var_set_num(vm, "COLLECTF_N", n);
    var_set_num(vm, "MAPGETF_N", n);
    var_set_num(vm, "GETFALL_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0])
      var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SETFALL|MAPSETF|PUTFALL [Class] field value
   * — write same field value on every live object (optional class filter).
   * Soft always; objects missing the field are skipped (SETFALL_SKIP).
   * LAST_N/SETFALL_N = write count. Value: string | LAST | string-var | expr.
   * Usability: fleet field stamp without EACH OBJ + SETF glue (dual of GETFALL). */
  if (kw(&L->cur, "SETFALL") || kw(&L->cur, "MAPSETF") ||
      kw(&L->cur, "PUTFALL") || kw(&L->cur, "SETF_ALL") ||
      kw(&L->cur, "WRITEFALL") || kw(&L->cur, "FIELDSETALL") ||
      kw(&L->cur, "BULKSETF") || kw(&L->cur, "SETFIELDALL")) {
    char filt[48], fname[48], tok1[48], sval[512];
    int has_filt = 0, is_str = 0, i, n = 0, n_skip = 0;
    long nval = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    sval[0] = 0;
    /* OF|CLASS|TYPE Class field value */
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SETFALL OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      /* Class field value when tok1 is known CLASS and next looks like field */
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "SETFALL [Class] field value"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SETFALL [Class] field value"); return -1;
      }
    }
    if (L->cur.kind == TK_EQ) lex_next(L);
    /* parse value once, apply to all matching objs */
    if (L->cur.kind == TK_STR) {
      snprintf(sval, sizeof sval, "%s", L->cur.text);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(sval, sizeof sval, "%s", vm->last_str);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(sval, sizeof sval, "%s", sv->sval);
        is_str = 1;
        lex_next(L);
      } else {
        nval = parse_expr(vm, L);
        is_str = 0;
      }
    } else {
      nval = parse_expr(vm, L);
      is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (is_str) {
        snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", sval);
        ob->fis_str[fi] = 1;
      } else {
        ob->fnum[fi] = nval;
        ob->fis_str[fi] = 0;
      }
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "SETFALL_N", n);
    var_set_num(vm, "MAPSETF_N", n);
    var_set_num(vm, "PUTFALL_N", n);
    var_set_num(vm, "SETFALL_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0])
      var_set_str(vm, "CLASS", filt);
    if (is_str) {
      var_set_str(vm, "LAST", sval);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", sval);
    } else {
      char nb[24];
      snprintf(nb, sizeof nb, "%ld", nval);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* COPYF|COPYFALL|COPYFIELD|MAPCOPYF [Class] src dst
   * — copy src field → dst field on every live object (optional class).
   * Preserves num/str kind per object. Soft skip if either field missing.
   * LAST_N = copy count. TO sugar: COPYF Cell energy TO prev.
   * Usability: snapshot fields before tick/mutation without EACH+GETF+SETF. */
  if (kw(&L->cur, "COPYF") || kw(&L->cur, "COPYFALL") ||
      kw(&L->cur, "COPYFIELD") || kw(&L->cur, "MAPCOPYF") ||
      kw(&L->cur, "COPYFIELDALL") || kw(&L->cur, "DUPFIELD") ||
      kw(&L->cur, "CLONEFIELD") || kw(&L->cur, "FIELDCOPY") ||
      kw(&L->cur, "SNAPSHOTF") || kw(&L->cur, "BACKUPF")) {
    char filt[48], srcf[48], dstf[48], tok1[48];
    int has_filt = 0, i, n = 0, n_skip = 0;
    lex_next(L);
    filt[0] = 0;
    srcf[0] = 0;
    dstf[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "COPYF OF Class src dst"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      /* Class src dst when tok1 is known CLASS */
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "TO") && !kw(&L->cur, "INTO") &&
          !kw(&L->cur, "AS") && !oop_stmt_kw(L)) {
        /* peek: Class src [TO] dst needs a third token; if only one more field name
         * without third, tok1 might be src. Prefer class when known + next is field. */
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(srcf, sizeof srcf, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(srcf, sizeof srcf, "%s", vv->sval);
          else
            snprintf(srcf, sizeof srcf, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(srcf, sizeof srcf, "%s", vv->sval);
        else
          snprintf(srcf, sizeof srcf, "%s", tok1);
      }
    } else {
      fail(vm, "COPYF [Class] src dst"); return -1;
    }
    if (!srcf[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(srcf, sizeof srcf, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "TO") && !kw(&L->cur, "INTO") && !kw(&L->cur, "AS")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(srcf, sizeof srcf, "%s", vv->sval);
        else
          snprintf(srcf, sizeof srcf, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "COPYF [Class] src dst"); return -1;
      }
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "INTO") || kw(&L->cur, "AS") ||
        kw(&L->cur, "->") || kw(&L->cur, "=")) {
      lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(dstf, sizeof dstf, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(dstf, sizeof dstf, "%s", vv->sval);
      else
        snprintf(dstf, sizeof dstf, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "COPYF [Class] src dst"); return -1;
    }
    if (!srcf[0] || !dstf[0]) {
      fail(vm, "COPYF [Class] src dst"); return -1;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int si, di;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      si = oop_field_idx(cd, srcf);
      di = oop_field_idx(cd, dstf);
      if (si < 0 || di < 0) { n_skip++; continue; }
      if (ob->fis_str[si]) {
        snprintf(ob->fstr[di], sizeof ob->fstr[di], "%s", ob->fstr[si]);
        ob->fis_str[di] = 1;
      } else {
        ob->fnum[di] = ob->fnum[si];
        ob->fis_str[di] = 0;
      }
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "COPYF_N", n);
    var_set_num(vm, "COPYFALL_N", n);
    var_set_num(vm, "MAPCOPYF_N", n);
    var_set_num(vm, "COPYF_SKIP", n_skip);
    var_set_str(vm, "FIELD", srcf);
    var_set_str(vm, "SRC", srcf);
    var_set_str(vm, "DST", dstf);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SWAPF|SWAPFALL|SWAPFIELD|EXCHANGEF [Class] a b
   * — exchange fields a and b on every live object (optional class).
   * Preserves num/str kinds per field. Soft skip if either missing.
   * WITH/AND sugar: SWAPF Cell energy WITH age.
   * Usability: rotate dual buffers without third temp field + EACH+GETF+SETF. */
  if (kw(&L->cur, "SWAPF") || kw(&L->cur, "SWAPFALL") ||
      kw(&L->cur, "SWAPFIELD") || kw(&L->cur, "EXCHANGEF") ||
      kw(&L->cur, "SWAPFIELDS") || kw(&L->cur, "XCHGF") ||
      kw(&L->cur, "FLIPFIELDS") || kw(&L->cur, "FIELDSWAP") ||
      kw(&L->cur, "SWAPF_ALL") || kw(&L->cur, "EXCHFALL")) {
    char filt[48], af[48], bf[48], tok1[48];
    int has_filt = 0, i, n = 0, n_skip = 0;
    lex_next(L);
    filt[0] = 0;
    af[0] = 0;
    bf[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SWAPF OF Class a b"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WITH") && !kw(&L->cur, "AND") &&
          !kw(&L->cur, "TO") && !oop_stmt_kw(L)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(af, sizeof af, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(af, sizeof af, "%s", vv->sval);
          else
            snprintf(af, sizeof af, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(af, sizeof af, "%s", vv->sval);
        else
          snprintf(af, sizeof af, "%s", tok1);
      }
    } else {
      fail(vm, "SWAPF [Class] a b"); return -1;
    }
    if (!af[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(af, sizeof af, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WITH") && !kw(&L->cur, "AND") &&
                 !kw(&L->cur, "TO")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(af, sizeof af, "%s", vv->sval);
        else
          snprintf(af, sizeof af, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SWAPF [Class] a b"); return -1;
      }
    }
    if (kw(&L->cur, "WITH") || kw(&L->cur, "AND") || kw(&L->cur, "TO") ||
        kw(&L->cur, "AND") || kw(&L->cur, ",")) {
      lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(bf, sizeof bf, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(bf, sizeof bf, "%s", vv->sval);
      else
        snprintf(bf, sizeof bf, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "SWAPF [Class] a b"); return -1;
    }
    if (!af[0] || !bf[0]) {
      fail(vm, "SWAPF [Class] a b"); return -1;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int ia, ib;
      long tnum;
      char tstr[128];
      int t_is_str;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      ia = oop_field_idx(cd, af);
      ib = oop_field_idx(cd, bf);
      if (ia < 0 || ib < 0) { n_skip++; continue; }
      /* swap including kind bits */
      t_is_str = ob->fis_str[ia];
      tnum = ob->fnum[ia];
      snprintf(tstr, sizeof tstr, "%s", ob->fstr[ia]);
      if (ob->fis_str[ib]) {
        snprintf(ob->fstr[ia], sizeof ob->fstr[ia], "%s", ob->fstr[ib]);
        ob->fis_str[ia] = 1;
      } else {
        ob->fnum[ia] = ob->fnum[ib];
        ob->fis_str[ia] = 0;
      }
      if (t_is_str) {
        snprintf(ob->fstr[ib], sizeof ob->fstr[ib], "%s", tstr);
        ob->fis_str[ib] = 1;
      } else {
        ob->fnum[ib] = tnum;
        ob->fis_str[ib] = 0;
      }
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "SWAPF_N", n);
    var_set_num(vm, "SWAPFALL_N", n);
    var_set_num(vm, "EXCHANGEF_N", n);
    var_set_num(vm, "SWAPF_SKIP", n_skip);
    var_set_str(vm, "FIELD", af);
    var_set_str(vm, "SRC", af);
    var_set_str(vm, "DST", bf);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* INCFALL|ADDFALL|DECFALL [Class] field [delta]
   * — add delta (default +1; DECFALL default −1) to numeric field on every live obj.
   * Soft always; missing or string fields skipped (INCFALL_SKIP). LAST_N = update count.
   * Usability: fleet counter/age bump without EACH OBJ + GETF + arith + SETF glue. */
  if (kw(&L->cur, "INCFALL") || kw(&L->cur, "ADDFALL") ||
      kw(&L->cur, "DECFALL") || kw(&L->cur, "SUBFALL") ||
      kw(&L->cur, "INCALL") || kw(&L->cur, "ADDFIELDALL") ||
      kw(&L->cur, "BUMPALL") || kw(&L->cur, "INCFIELDALL")) {
    char filt[48], fname[48], tok1[48], op[24];
    int has_filt = 0, i, n = 0, n_skip = 0, dec_mode = 0, have_delta = 0;
    long delta = 1;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "DECFALL") == 0 || strcmp(op, "SUBFALL") == 0) {
      dec_mode = 1;
      delta = -1;
    }
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "INCFALL OF Class field [delta]"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
          !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
          !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
        /* Class field [delta] — next is field if not a bare numeric-only line */
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "INCFALL [Class] field [delta]"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "INCFALL [Class] field [delta]"); return -1;
      }
    }
    /* optional delta: BY n | = n | bare expr (not stmt kw).
     * DECFALL/SUBFALL: magnitude is subtracted (DECFALL f 5 → −5). */
    if (kw(&L->cur, "BY") || kw(&L->cur, "PLUS") || kw(&L->cur, "DELTA")) {
      lex_next(L);
      delta = parse_expr(vm, L);
      have_delta = 1;
    } else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      delta = parse_expr(vm, L);
      have_delta = 1;
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN ||
               (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
                !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
                !kw(&L->cur, "END") && !kw(&L->cur, "NEW") &&
                !kw(&L->cur, "GETF") && !kw(&L->cur, "SETF") &&
                !kw(&L->cur, "INCFALL") && !kw(&L->cur, "ADDFALL") &&
                !kw(&L->cur, "DECFALL") && !kw(&L->cur, "CLASS") &&
                !kw(&L->cur, "WHEREOBJ") && !kw(&L->cur, "SENDWHERE") &&
                !kw(&L->cur, "DELETEWHERE") && !kw(&L->cur, "SETFALL") &&
                !kw(&L->cur, "GETFALL") && !kw(&L->cur, "SENDALL") &&
                !kw(&L->cur, "LISTOBJS") && !kw(&L->cur, "HASOBJ"))) {
      delta = parse_expr(vm, L);
      have_delta = 1;
    }
    if (dec_mode && have_delta) {
      if (delta < 0) delta = -delta; /* normalize then flip */
      delta = -delta;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; } /* numeric only */
      ob->fnum[fi] += delta;
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "INCFALL_N", n);
    var_set_num(vm, "ADDFALL_N", n);
    var_set_num(vm, "DECFALL_N", n);
    var_set_num(vm, "INCFALL_SKIP", n_skip);
    var_set_num(vm, "INCFALL_DELTA", delta);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0])
      var_set_str(vm, "CLASS", filt);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MULFALL|SCALEFALL|MULF|TIMESFALL [Class] field factor
   * — multiply every live object's numeric field by factor (integer).
   * Soft always; missing/string fields skipped. LAST_N = update count.
   * Usability: scale scores/energy (double, zero, decay) without EACH+GETF+*+SETF.
   * Complements INCFALL (add) with product update. */
  if (kw(&L->cur, "MULFALL") || kw(&L->cur, "SCALEFALL") ||
      kw(&L->cur, "MULF") || kw(&L->cur, "TIMESFALL") ||
      kw(&L->cur, "MULALL") || kw(&L->cur, "SCALEF") ||
      kw(&L->cur, "MULTFALL") || kw(&L->cur, "PRODUCTFALL") ||
      kw(&L->cur, "SCALEFIELDALL") || kw(&L->cur, "MULFIELDALL")) {
    char filt[48], fname[48], tok1[48];
    int has_filt = 0, i, n = 0, n_skip = 0;
    long factor = 1;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "MULFALL OF Class field factor"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
          !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
          !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "MULFALL [Class] field factor"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "MULFALL [Class] field factor"); return -1;
      }
    }
    /* factor: BY n | * n | = n | bare expr (required for scale) */
    if (kw(&L->cur, "BY") || kw(&L->cur, "TIMES") || kw(&L->cur, "FACTOR") ||
        kw(&L->cur, "MUL") || kw(&L->cur, "SCALE"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ)
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      factor = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      factor = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW") &&
               !kw(&L->cur, "GETF") && !kw(&L->cur, "SETF") &&
               !kw(&L->cur, "INCFALL") && !kw(&L->cur, "MULFALL") &&
               !kw(&L->cur, "CLAMPFALL") && !kw(&L->cur, "CLASS")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        factor = atol(sv->sval);
      else
        factor = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      factor = parse_expr(vm, L);
    } else {
      fail(vm, "MULFALL [Class] field factor"); return -1;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      ob->fnum[fi] *= factor;
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "MULFALL_N", n);
    var_set_num(vm, "SCALEFALL_N", n);
    var_set_num(vm, "MULF_N", n);
    var_set_num(vm, "TIMESFALL_N", n);
    var_set_num(vm, "MULFALL_SKIP", n_skip);
    var_set_num(vm, "MULFALL_FACTOR", factor);
    var_set_num(vm, "SCALEFALL_FACTOR", factor);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* CLAMPFALL|CLAMPF|BOUNDALL [Class] field lo [TO] hi
   * — clamp every live object's numeric field into [lo,hi] (auto-swap lo/hi).
   * Soft always; string/missing fields skipped. LAST_N = objs touched (any clamp
   * or already-in-range still counts as visited with field present).
   * CLAMPFALL_CHANGED = how many values actually moved.
   * Usability: bound energy/retries/scores without EACH+GETF+IF+SETF glue. */
  if (kw(&L->cur, "CLAMPFALL") || kw(&L->cur, "CLAMPF") ||
      kw(&L->cur, "BOUNDALL") || kw(&L->cur, "CLAMPFIELD") ||
      kw(&L->cur, "CLIPFALL") || kw(&L->cur, "CLIPF") ||
      kw(&L->cur, "LIMITFALL") || kw(&L->cur, "BOUNDFALL") ||
      kw(&L->cur, "SATFALL") || kw(&L->cur, "SATF")) {
    char filt[48], fname[48], tok1[48];
    int has_filt = 0, i, n = 0, n_skip = 0, n_chg = 0;
    long lo = 0, hi = 0, t;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "CLAMPFALL OF Class field lo hi"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "CLAMPFALL [Class] field lo hi"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "CLAMPFALL [Class] field lo hi"); return -1;
      }
    }
    /* optional glue before lo */
    if (kw(&L->cur, "IN") || kw(&L->cur, "BETWEEN") || kw(&L->cur, "RANGE"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      lo = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      lo = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        lo = atol(sv->sval);
      else
        lo = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      lo = parse_expr(vm, L);
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "AND") || kw(&L->cur, "THRU") ||
        kw(&L->cur, "THROUGH") || kw(&L->cur, "UNTIL") ||
        (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "..") == 0))
      lex_next(L);
    else if (L->cur.kind == TK_COMMA)
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      hi = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      hi = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        hi = atol(sv->sval);
      else
        hi = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      hi = parse_expr(vm, L);
    }
    if (lo > hi) { t = lo; lo = hi; hi = t; }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      long v, nv;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      nv = v;
      if (nv < lo) nv = lo;
      if (nv > hi) nv = hi;
      if (nv != v) {
        ob->fnum[fi] = nv;
        n_chg++;
      }
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "CLAMPFALL_N", n);
    var_set_num(vm, "CLAMPF_N", n);
    var_set_num(vm, "CLIPFALL_N", n);
    var_set_num(vm, "BOUNDALL_N", n);
    var_set_num(vm, "CLAMPFALL_SKIP", n_skip);
    var_set_num(vm, "CLAMPFALL_CHANGED", n_chg);
    var_set_num(vm, "CLAMPFALL_LO", lo);
    var_set_num(vm, "CLAMPFALL_HI", hi);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MAXOBJ|HIGHEST|ARGMAXF [Class] field
   * MINOBJ|LOWEST|ARGMINF [Class] field
   * — name of first live object with max/min numeric field (optional class).
   * LAST=name, LAST_N=field value, OBJECT=name. Soft empty → OK=0 LAST="".
   * Usability: pick extreme without GETFALL + ARGMAX + NTH / ZIP glue. */
  if (kw(&L->cur, "MAXOBJ") || kw(&L->cur, "HIGHEST") ||
      kw(&L->cur, "ARGMAXF") || kw(&L->cur, "TOPOBJ") ||
      kw(&L->cur, "BESTOBJ") || kw(&L->cur, "MAXINST") ||
      kw(&L->cur, "MINOBJ") || kw(&L->cur, "LOWEST") ||
      kw(&L->cur, "ARGMINF") || kw(&L->cur, "BOTTOMOBJ") ||
      kw(&L->cur, "WORSTOBJ") || kw(&L->cur, "MININST")) {
    char filt[48], fname[48], tok1[48], op[24], best_name[48];
    int has_filt = 0, want_min = 0, i, n = 0, found = 0;
    long best = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "MINOBJ") == 0 || strcmp(op, "LOWEST") == 0 ||
        strcmp(op, "ARGMINF") == 0 || strcmp(op, "BOTTOMOBJ") == 0 ||
        strcmp(op, "WORSTOBJ") == 0 || strcmp(op, "MININST") == 0)
      want_min = 1;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    best_name[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        /* MAXOBJ BY field — no class; next is field */
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "MAXOBJ OF Class field"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "MAXOBJ [Class] field"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "MAXOBJ [Class] field"); return -1;
      }
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) continue;
      if (ob->fis_str[fi]) continue; /* numeric field only */
      v = ob->fnum[fi];
      n++;
      if (!found || (want_min ? (v < best) : (v > best))) {
        best = v;
        snprintf(best_name, sizeof best_name, "%s", ob->name);
        found = 1;
      }
    }
    if (!found) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_str(vm, "MAXOBJ", "");
      var_set_str(vm, "MINOBJ", "");
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "MAXOBJ_N", 0);
      var_set_num(vm, "MINOBJ_N", 0);
      var_set_num(vm, "MAXOBJ_VAL", 0);
      var_set_str(vm, "FIELD", fname);
      if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "MAXOBJ: no matching objects");
      var_set_str(vm, "ERR", "MAXOBJ: no matching objects");
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", best_name);
    var_set_str(vm, "MAXOBJ", best_name);
    var_set_str(vm, "MINOBJ", best_name);
    var_set_str(vm, "OBJECT", best_name);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", best_name);
    var_set_num(vm, "LAST_N", best);
    vm->last_n = best;
    var_set_num(vm, "MAXOBJ_N", 1);
    var_set_num(vm, "MINOBJ_N", 1);
    var_set_num(vm, "MAXOBJ_VAL", best);
    var_set_num(vm, "MINOBJ_VAL", best);
    var_set_num(vm, "NOBJS", n);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SORTBYF|SORTOBJS|ORDERBYF [Class] field [ASC|DESC]
   * — bag of live object names ordered by numeric field (stable first-seen on ties).
   * Soft always; string/missing fields skipped. LAST_N = count. Default ASC.
   * Usability: ranked fleet without GETFALL+ZIP+SORTN+KEYS glue. */
  if (kw(&L->cur, "SORTBYF") || kw(&L->cur, "SORTOBJS") ||
      kw(&L->cur, "ORDERBYF") || kw(&L->cur, "RANKBYF") ||
      kw(&L->cur, "ORDEROBJS") || kw(&L->cur, "SORTBYFIELD") ||
      kw(&L->cur, "OBJSSORT") || kw(&L->cur, "RANKOBJS")) {
    char filt[48], fname[48], tok1[48], bag[4096];
    int has_filt = 0, desc = 0, i, n = 0, n_skip = 0;
    size_t o = 0;
    typedef struct { char name[48]; long v; int idx; } SortByFRow;
    SortByFRow rows[CUBALC_MAX_OBJS];
    SortByFRow key;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "SORTBYF OF Class field [ASC|DESC]"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
          !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "SORTBYF [Class] field [ASC|DESC]"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SORTBYF [Class] field [ASC|DESC]"); return -1;
      }
    }
    if (kw(&L->cur, "DESC") || kw(&L->cur, "DOWN") || kw(&L->cur, "REV") ||
        kw(&L->cur, "REVERSE") || kw(&L->cur, "HIGHFIRST")) {
      desc = 1;
      lex_next(L);
    } else if (kw(&L->cur, "ASC") || kw(&L->cur, "UP") ||
               kw(&L->cur, "LOWFIRST")) {
      desc = 0;
      lex_next(L);
    }
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      snprintf(rows[n].name, sizeof rows[n].name, "%s", ob->name);
      rows[n].v = ob->fnum[fi];
      rows[n].idx = n;
      n++;
    }
    /* stable-ish insertion sort (n<=64) */
    {
      int a, b;
      for (a = 1; a < n; a++) {
        key = rows[a];
        b = a - 1;
        while (b >= 0) {
          int less;
          if (desc)
            less = (key.v > rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          else
            less = (key.v < rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          if (!less) break;
          rows[b + 1] = rows[b];
          b--;
        }
        rows[b + 1] = key;
      }
    }
    for (i = 0; i < n; i++) {
      size_t ln = strlen(rows[i].name);
      if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, rows[i].name, ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "SORTBYF", bag);
    var_set_str(vm, "SORTOBJS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "SORTBYF_N", n);
    var_set_num(vm, "SORTOBJS_N", n);
    var_set_num(vm, "SORTBYF_SKIP", n_skip);
    var_set_num(vm, "SORTBYF_DESC", desc);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SUMF|SUMFALL|TOTALF [Class] field
   * AVGF|AVGFALL|MEANF [Class] field
   * — sum or integer mean of numeric field over live objects (optional class).
   * LAST_N = result; SUMF_N/AVGF_N = sample count; soft empty → 0.
   * Usability: fleet health without GETFALL + SYS SUM/AVG glue. */
  if (kw(&L->cur, "SUMF") || kw(&L->cur, "SUMFALL") ||
      kw(&L->cur, "TOTALF") || kw(&L->cur, "FIELDSUM") ||
      kw(&L->cur, "SUMFIELD") || kw(&L->cur, "OBJSSUM") ||
      kw(&L->cur, "AVGF") || kw(&L->cur, "AVGFALL") ||
      kw(&L->cur, "MEANF") || kw(&L->cur, "FIELDAVG") ||
      kw(&L->cur, "AVGFIELD") || kw(&L->cur, "OBJSAVG")) {
    char filt[48], fname[48], tok1[48], op[24];
    int has_filt = 0, want_avg = 0, i, n = 0, n_skip = 0;
    long sum = 0, outv = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "AVGF") == 0 || strcmp(op, "AVGFALL") == 0 ||
        strcmp(op, "MEANF") == 0 || strcmp(op, "FIELDAVG") == 0 ||
        strcmp(op, "AVGFIELD") == 0 || strcmp(op, "OBJSAVG") == 0)
      want_avg = 1;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "SUMF OF Class field"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "SUMF [Class] field"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SUMF [Class] field"); return -1;
      }
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      sum += ob->fnum[fi];
      n++;
    }
    if (want_avg)
      outv = (n > 0) ? (sum / n) : 0;
    else
      outv = sum;
    var_set_num(vm, "LAST_N", outv);
    vm->last_n = outv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", outv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "SUMF_N", n);
    var_set_num(vm, "AVGF_N", n);
    var_set_num(vm, "SUMFALL_N", n);
    var_set_num(vm, "SUMF_SKIP", n_skip);
    var_set_num(vm, "SUMF_SUM", sum);
    var_set_num(vm, "AVGF_SUM", sum);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MEDIANF|P50F|MIDF|MEDIANFALL [Class] field
   * — integer median of numeric field over live objects (optional class).
   * Sort ascending; odd → middle; even → lower mid (matches SYS MEDIAN).
   * LAST_N = median; MEDIANF_N = sample count; soft empty → 0.
   * Usability: robust fleet mid vs AVGF outliers without GETFALL+SYS MEDIAN. */
  if (kw(&L->cur, "MEDIANF") || kw(&L->cur, "P50F") ||
      kw(&L->cur, "MIDF") || kw(&L->cur, "MEDIANFALL") ||
      kw(&L->cur, "FIELDMEDIAN") || kw(&L->cur, "OBJSMEDIAN") ||
      kw(&L->cur, "MEDF") || kw(&L->cur, "MIDFALL")) {
    char filt[48], fname[48], tok1[48];
    long vals[CUBALC_MAX_OBJS];
    int has_filt = 0, i, j, n = 0, n_skip = 0;
    long outv = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "MEDIANF OF Class field"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "MEDIANF [Class] field"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "MEDIANF [Class] field"); return -1;
      }
    }
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      vals[n++] = ob->fnum[fi];
    }
    /* insertion sort ascending */
    for (i = 1; i < n; i++) {
      long key = vals[i];
      j = i - 1;
      while (j >= 0 && vals[j] > key) {
        vals[j + 1] = vals[j];
        j--;
      }
      vals[j + 1] = key;
    }
    if (n == 0)
      outv = 0;
    else if (n & 1)
      outv = vals[n / 2];
    else
      outv = vals[n / 2 - 1]; /* lower mid for even — match SYS MEDIAN */
    var_set_num(vm, "LAST_N", outv);
    vm->last_n = outv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", outv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "MEDIANF", outv);
    var_set_num(vm, "MEDIANF_N", n);
    var_set_num(vm, "P50F_N", n);
    var_set_num(vm, "MIDF_N", n);
    var_set_num(vm, "MEDIANFALL_N", n);
    var_set_num(vm, "MEDIANF_SKIP", n_skip);
    var_set_num(vm, "MEDIANF_VAL", outv);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* FREQF|HISTF|COUNTF|FREQFALL [Class] field
   * — frequency histogram of field values over live objects (num or str).
   * LAST = "val:count" bag (first-seen order, sep ":"); LAST_N = distinct keys.
   * FREQF_TOTAL = sample count; soft empty → empty bag / 0.
   * Usability: fleet status rollups without GETFALL+SYS FREQ / EACH+KVINC. */
  if (kw(&L->cur, "FREQF") || kw(&L->cur, "HISTF") ||
      kw(&L->cur, "COUNTF") || kw(&L->cur, "FREQFALL") ||
      kw(&L->cur, "HISTFALL") || kw(&L->cur, "FIELDFREQ") ||
      kw(&L->cur, "OBJSFREQ") || kw(&L->cur, "FREQFIELD") ||
      kw(&L->cur, "HISTFIELD") || kw(&L->cur, "VALUEFREQ") ||
      kw(&L->cur, "ROLLUPF")) {
    char filt[48], fname[48], tok1[48];
    char keys[64][128];
    long counts[64];
    char out[CUBALC_HOST_STR_MAX];
    int has_filt = 0, i, k, nk = 0, n_skip = 0, total = 0;
    size_t olen = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    out[0] = 0;
    memset(counts, 0, sizeof counts);
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "FREQF OF Class field"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "FREQF [Class] field"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "FREQF [Class] field"); return -1;
      }
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      char field[128];
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi])
        snprintf(field, sizeof field, "%s", ob->fstr[fi]);
      else
        snprintf(field, sizeof field, "%ld", ob->fnum[fi]);
      total++;
      for (k = 0; k < nk; k++) {
        if (strcmp(keys[k], field) == 0) {
          counts[k]++;
          break;
        }
      }
      if (k == nk && nk < 64) {
        snprintf(keys[nk], sizeof keys[0], "%s", field);
        counts[nk] = 1;
        nk++;
      }
    }
    for (k = 0; k < nk; k++) {
      char line[160];
      int n;
      n = snprintf(line, sizeof line, "%s:%ld", keys[k], counts[k]);
      if (n < 0) n = 0;
      if ((size_t)n >= sizeof line) n = (int)sizeof line - 1;
      if (k > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
      if (olen + (size_t)n < sizeof out) {
        memcpy(out + olen, line, (size_t)n);
        olen += (size_t)n;
      } else if (olen < sizeof out - 1) {
        size_t t = sizeof out - 1 - olen;
        memcpy(out + olen, line, t);
        olen += t;
      }
      out[olen] = 0;
    }
    var_set_str(vm, "LAST", out);
    var_set_str(vm, "FREQF", out);
    var_set_str(vm, "HISTF", out);
    var_set_str(vm, "FREQFALL", out);
    var_set_str(vm, "FREQ", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = nk;
    var_set_num(vm, "LAST_N", nk);
    var_set_num(vm, "FREQF_N", nk);
    var_set_num(vm, "HISTF_N", nk);
    var_set_num(vm, "COUNTF_N", nk);
    var_set_num(vm, "FREQFALL_N", nk);
    var_set_num(vm, "FREQF_TOTAL", total);
    var_set_num(vm, "HISTF_TOTAL", total);
    var_set_num(vm, "FREQF_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MODEF|TOPF|MODEFALL|DOMINANTF [Class] field
   * — most frequent field value over live objects (num or str).
   * LAST = mode value (first-seen on ties); LAST_N = mode count.
   * MODEF_TOTAL = samples; MODEF_DISTINCT = unique keys; soft empty → "".
   * Usability: dominant status/energy without FREQF+SYS TOPKEY glue. */
  if (kw(&L->cur, "MODEF") || kw(&L->cur, "TOPF") ||
      kw(&L->cur, "MODEFALL") || kw(&L->cur, "DOMINANTF") ||
      kw(&L->cur, "MOSTF") || kw(&L->cur, "COMMONF") ||
      kw(&L->cur, "FIELDMODE") || kw(&L->cur, "OBJSMODE") ||
      kw(&L->cur, "TOPVALUE") || kw(&L->cur, "MODEFIELD")) {
    char filt[48], fname[48], tok1[48];
    char keys[64][128];
    long counts[64];
    char best[128];
    int has_filt = 0, i, k, nk = 0, n_skip = 0, total = 0, best_i = -1;
    long best_c = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    best[0] = 0;
    memset(counts, 0, sizeof counts);
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "MODEF OF Class field"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "MODEF [Class] field"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "MODEF [Class] field"); return -1;
      }
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      char field[128];
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi])
        snprintf(field, sizeof field, "%s", ob->fstr[fi]);
      else
        snprintf(field, sizeof field, "%ld", ob->fnum[fi]);
      total++;
      for (k = 0; k < nk; k++) {
        if (strcmp(keys[k], field) == 0) {
          counts[k]++;
          break;
        }
      }
      if (k == nk && nk < 64) {
        snprintf(keys[nk], sizeof keys[0], "%s", field);
        counts[nk] = 1;
        nk++;
      }
    }
    for (k = 0; k < nk; k++) {
      if (best_i < 0 || counts[k] > best_c) {
        best_c = counts[k];
        best_i = k;
      }
    }
    if (best_i >= 0)
      snprintf(best, sizeof best, "%s", keys[best_i]);
    var_set_str(vm, "LAST", best);
    var_set_str(vm, "MODEF", best);
    var_set_str(vm, "TOPF", best);
    var_set_str(vm, "MODEFALL", best);
    var_set_str(vm, "DOMINANTF", best);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", best);
    vm->last_n = best_c;
    var_set_num(vm, "LAST_N", best_c);
    var_set_num(vm, "MODEF_COUNT", best_c);
    var_set_num(vm, "TOPF_COUNT", best_c);
    var_set_num(vm, "MODEF_N", total);
    var_set_num(vm, "MODEFALL_N", total);
    var_set_num(vm, "TOPF_N", total);
    var_set_num(vm, "MODEF_TOTAL", total);
    var_set_num(vm, "MODEF_DISTINCT", nk);
    var_set_num(vm, "MODEF_SKIP", n_skip);
    /* numeric peel when mode key is integer text */
    {
      char *end = 0;
      long nv;
      if (best[0]) {
        nv = strtol(best, &end, 10);
        if (end && end != best && *end == 0)
          var_set_num(vm, "MODEF_NUM", nv);
        else
          var_set_num(vm, "MODEF_NUM", 0);
      } else {
        var_set_num(vm, "MODEF_NUM", 0);
      }
    }
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* UNIQUF|DISTINCTF|UNIQUEFALL|UNIQF [Class] field
   * — unique field values over live objects as newline bag (first-seen order).
   * LAST = bag; LAST_N = distinct count; UNIQUF_TOTAL = samples.
   * Num or str fields. Soft empty → "" / 0.
   * Usability: "what statuses exist?" without FREQF+KEYS or EACH+HASLINE. */
  if (kw(&L->cur, "UNIQUF") || kw(&L->cur, "DISTINCTF") ||
      kw(&L->cur, "UNIQUEFALL") || kw(&L->cur, "UNIQF") ||
      kw(&L->cur, "UNIQUEF") || kw(&L->cur, "FIELDUNIQ") ||
      kw(&L->cur, "OBJSUNIQ") || kw(&L->cur, "DISTINCTFALL") ||
      kw(&L->cur, "UNIQVALUES") || kw(&L->cur, "UNIQUEVALUES")) {
    char filt[48], fname[48], tok1[48];
    char keys[64][128];
    char out[CUBALC_HOST_STR_MAX];
    int has_filt = 0, i, k, nk = 0, n_skip = 0, total = 0;
    size_t olen = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    out[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "UNIQUF OF Class field"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "UNIQUF [Class] field"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "UNIQUF [Class] field"); return -1;
      }
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      char field[128];
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi])
        snprintf(field, sizeof field, "%s", ob->fstr[fi]);
      else
        snprintf(field, sizeof field, "%ld", ob->fnum[fi]);
      total++;
      for (k = 0; k < nk; k++) {
        if (strcmp(keys[k], field) == 0)
          break;
      }
      if (k == nk && nk < 64) {
        snprintf(keys[nk], sizeof keys[0], "%s", field);
        nk++;
      }
    }
    for (k = 0; k < nk; k++) {
      size_t ln = strlen(keys[k]);
      if (k > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
      if (olen + ln < sizeof out) {
        memcpy(out + olen, keys[k], ln);
        olen += ln;
      } else if (olen < sizeof out - 1) {
        size_t t = sizeof out - 1 - olen;
        memcpy(out + olen, keys[k], t);
        olen += t;
      }
      out[olen] = 0;
    }
    var_set_str(vm, "LAST", out);
    var_set_str(vm, "UNIQUF", out);
    var_set_str(vm, "DISTINCTF", out);
    var_set_str(vm, "UNIQUEFALL", out);
    var_set_str(vm, "UNIQF", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = nk;
    var_set_num(vm, "LAST_N", nk);
    var_set_num(vm, "UNIQUF_N", nk);
    var_set_num(vm, "DISTINCTF_N", nk);
    var_set_num(vm, "UNIQUEFALL_N", nk);
    var_set_num(vm, "UNIQF_N", nk);
    var_set_num(vm, "UNIQUF_TOTAL", total);
    var_set_num(vm, "DISTINCTF_TOTAL", total);
    var_set_num(vm, "UNIQUF_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* WHEREGE|WHEREGT|WHERELE|WHERELT [Class] field value
   * — bag of live object names where numeric field meets threshold.
   * Soft always; string/missing fields skipped. LAST_N = count.
   * Usability: range/threshold select beyond WHEREOBJ equality · no EACH+GETF+IF. */
  if (kw(&L->cur, "WHEREGE") || kw(&L->cur, "WHEREGTE") ||
      kw(&L->cur, "ATLEAST") || kw(&L->cur, "MINWHERE") ||
      kw(&L->cur, "WHEREGT") || kw(&L->cur, "ABOVE") ||
      kw(&L->cur, "WHERELE") || kw(&L->cur, "WHERELTE") ||
      kw(&L->cur, "ATMOST") || kw(&L->cur, "MAXWHERE") ||
      kw(&L->cur, "WHERELT") || kw(&L->cur, "BELOW") ||
      kw(&L->cur, "THRESHOBJS") || kw(&L->cur, "FILTERGE") ||
      kw(&L->cur, "FILTERLE")) {
    char filt[48], fname[48], tok1[48], bag[4096], op[24];
    int has_filt = 0, mode = 0, i, n = 0, n_skip = 0;
    long thresh = 0;
    size_t o = 0;
    /* mode: 0=GE 1=GT 2=LE 3=LT */
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "WHEREGT") == 0 || strcmp(op, "ABOVE") == 0)
      mode = 1;
    else if (strcmp(op, "WHERELE") == 0 || strcmp(op, "WHERELTE") == 0 ||
             strcmp(op, "ATMOST") == 0 || strcmp(op, "MAXWHERE") == 0 ||
             strcmp(op, "FILTERLE") == 0)
      mode = 2;
    else if (strcmp(op, "WHERELT") == 0 || strcmp(op, "BELOW") == 0)
      mode = 3;
    else
      mode = 0; /* GE / ATLEAST / THRESHOBJS / FILTERGE */
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "WHEREGE OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "WHEREGE [Class] field value"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "WHEREGE [Class] field value"); return -1;
      }
    }
    if (kw(&L->cur, "GE") || kw(&L->cur, "GTE") || kw(&L->cur, "GT") ||
        kw(&L->cur, "LE") || kw(&L->cur, "LTE") || kw(&L->cur, "LT") ||
        kw(&L->cur, "MIN") || kw(&L->cur, "MAX"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      thresh = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      thresh = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        thresh = atol(sv->sval);
      else
        thresh = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      thresh = parse_expr(vm, L);
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      size_t ln;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (mode == 0) hit = (v >= thresh);
      else if (mode == 1) hit = (v > thresh);
      else if (mode == 2) hit = (v <= thresh);
      else hit = (v < thresh);
      if (!hit) continue;
      ln = strlen(ob->name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, ob->name, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "WHEREGE", bag);
    var_set_str(vm, "WHERELE", bag);
    var_set_str(vm, "WHEREGT", bag);
    var_set_str(vm, "WHERELT", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "WHEREGE_N", n);
    var_set_num(vm, "WHERELE_N", n);
    var_set_num(vm, "WHEREGT_N", n);
    var_set_num(vm, "WHERELT_N", n);
    var_set_num(vm, "WHEREGE_SKIP", n_skip);
    var_set_num(vm, "WHEREGE_THRESH", thresh);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* WHEREBETWEEN|WHERERANGE|INRANGEOBJS [Class] field lo [TO|..] hi
   * COUNTBETWEEN|COUNTRANGE — count only (no bag).
   * HASBETWEEN|ANYBETWEEN — soft 0|1 + first name (early-exit).
   * Closed interval: lo <= numeric field <= hi (swaps if lo>hi).
   * Usability: band/SLO select without dual WHEREGE+WHERELE glue. */
  if (kw(&L->cur, "WHEREBETWEEN") || kw(&L->cur, "WHERERANGE") ||
      kw(&L->cur, "INRANGEOBJS") || kw(&L->cur, "BETWEENOBJS") ||
      kw(&L->cur, "RANGEOBJS") || kw(&L->cur, "WHERERANGEOF") ||
      kw(&L->cur, "COUNTBETWEEN") || kw(&L->cur, "COUNTBAND") ||
      kw(&L->cur, "COUNTINRANGE") || kw(&L->cur, "NBETWEEN") ||
      kw(&L->cur, "TALLYBETWEEN") || kw(&L->cur, "TALLYRANGE") ||
      kw(&L->cur, "HASBETWEEN") || kw(&L->cur, "ANYBETWEEN") ||
      kw(&L->cur, "INRANGE") || kw(&L->cur, "HASRANGE") ||
      kw(&L->cur, "ANYRANGE")) {
    char filt[48], fname[48], tok1[48], bag[4096], found[48], op[32];
    int has_filt = 0, mode = 0; /* 0=bag 1=count 2=has */
    int i, n = 0, n_skip = 0;
    long lo = 0, hi = 0, t;
    size_t o = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "COUNTBETWEEN") == 0 || strcmp(op, "COUNTBAND") == 0 ||
        strcmp(op, "COUNTINRANGE") == 0 || strcmp(op, "NBETWEEN") == 0 ||
        strcmp(op, "TALLYBETWEEN") == 0 || strcmp(op, "TALLYRANGE") == 0)
      mode = 1;
    else if (strcmp(op, "HASBETWEEN") == 0 || strcmp(op, "ANYBETWEEN") == 0 ||
             strcmp(op, "INRANGE") == 0 || strcmp(op, "HASRANGE") == 0 ||
             strcmp(op, "ANYRANGE") == 0)
      mode = 2;
    else
      mode = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    bag[0] = 0;
    found[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "WHEREBETWEEN OF Class field lo hi"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "WHEREBETWEEN [Class] field lo hi"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "WHEREBETWEEN [Class] field lo hi"); return -1;
      }
    }
    /* optional IN | BETWEEN before lo */
    if (kw(&L->cur, "IN") || kw(&L->cur, "BETWEEN") || kw(&L->cur, "RANGE"))
      lex_next(L);
    /* lo */
    if (L->cur.kind == TK_STR) {
      lo = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      lo = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        lo = atol(sv->sval);
      else
        lo = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      lo = parse_expr(vm, L);
    }
    /* optional TO | .. | - | AND | , between lo and hi */
    if (kw(&L->cur, "TO") || kw(&L->cur, "AND") || kw(&L->cur, "THRU") ||
        kw(&L->cur, "THROUGH") || kw(&L->cur, "UNTIL") ||
        (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "..") == 0))
      lex_next(L);
    else if (L->cur.kind == TK_COMMA)
      lex_next(L);
    /* hi */
    if (L->cur.kind == TK_STR) {
      hi = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      hi = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        hi = atol(sv->sval);
      else
        hi = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      hi = parse_expr(vm, L);
    }
    if (lo > hi) { t = lo; lo = hi; hi = t; }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      long v;
      size_t ln;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (v < lo || v > hi) continue;
      if (mode == 2) {
        /* has: first hit */
        snprintf(found, sizeof found, "%s", ob->name);
        n = 1;
        break;
      }
      if (mode == 0) {
        ln = strlen(ob->name);
        if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) {
          memcpy(bag + o, ob->name, ln);
          o += ln;
        }
        bag[o] = 0;
      }
      n++;
    }
    if (mode == 2) {
      var_set_str(vm, "LAST", found);
      var_set_str(vm, "HASBETWEEN", found);
      var_set_str(vm, "ANYBETWEEN", found);
      var_set_str(vm, "FIRST", found);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", found);
      vm->last_n = n ? 1 : 0;
      var_set_num(vm, "LAST_N", n ? 1 : 0);
      var_set_num(vm, "HASBETWEEN_N", n ? 1 : 0);
      var_set_num(vm, "ANYBETWEEN_N", n ? 1 : 0);
      var_set_num(vm, "INRANGE_N", n ? 1 : 0);
    } else if (mode == 1) {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "COUNTBETWEEN", n);
      var_set_num(vm, "COUNTBETWEEN_N", n);
      var_set_num(vm, "COUNTBAND_N", n);
      var_set_num(vm, "COUNTINRANGE_N", n);
      var_set_num(vm, "NBETWEEN_N", n);
    } else {
      var_set_str(vm, "LAST", bag);
      var_set_str(vm, "WHEREBETWEEN", bag);
      var_set_str(vm, "WHERERANGE", bag);
      var_set_str(vm, "INRANGEOBJS", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "WHEREBETWEEN_N", n);
      var_set_num(vm, "WHERERANGE_N", n);
      var_set_num(vm, "INRANGEOBJS_N", n);
    }
    var_set_num(vm, "WHEREBETWEEN_SKIP", n_skip);
    var_set_num(vm, "WHEREBETWEEN_LO", lo);
    var_set_num(vm, "WHEREBETWEEN_HI", hi);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* WHEREOBJ|FILTEROBJS|KEEPOBJS [Class] field value
   * FINDOBJ|FIRSTOBJ — first match only (LAST=name, LAST_N 0|1).
   * Soft always. Equality on num or string fields.
   * Usability: select live objects by field without EACH OBJ + GETF + IF glue. */
  if (kw(&L->cur, "WHEREOBJ") || kw(&L->cur, "FILTEROBJS") ||
      kw(&L->cur, "KEEPOBJS") || kw(&L->cur, "SELECTOBJS") ||
      kw(&L->cur, "OBJSWHERE") || kw(&L->cur, "MATCHOBJS") ||
      kw(&L->cur, "FINDOBJ") || kw(&L->cur, "FIRSTOBJ") ||
      kw(&L->cur, "FINDINST") || kw(&L->cur, "OBJFIND") ||
      kw(&L->cur, "FINDWHERE")) {
    char filt[48], fname[48], tok1[48], sval[512], bag[4096], op[24];
    int has_filt = 0, is_str = 0, first_only = 0, i, n = 0, n_skip = 0;
    long nval = 0;
    size_t o = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "FINDOBJ") == 0 || strcmp(op, "FIRSTOBJ") == 0 ||
        strcmp(op, "FINDINST") == 0 || strcmp(op, "OBJFIND") == 0 ||
        strcmp(op, "FINDWHERE") == 0)
      first_only = 1;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    sval[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "WHEREOBJ OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "WHEREOBJ [Class] field value"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "WHEREOBJ [Class] field value"); return -1;
      }
    }
    /* optional == | EQ | IS before value */
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS") ||
        (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "==") == 0)) {
      lex_next(L);
    } else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L); /* == as two EQ tokens if any */
    }
    if (L->cur.kind == TK_STR) {
      snprintf(sval, sizeof sval, "%s", L->cur.text);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(sval, sizeof sval, "%s", vm->last_str);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(sval, sizeof sval, "%s", sv->sval);
        is_str = 1;
        lex_next(L);
      } else {
        nval = parse_expr(vm, L);
        is_str = 0;
      }
    } else {
      nval = parse_expr(vm, L);
      is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      size_t ln;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) {
        if (is_str)
          hit = (strcmp(ob->fstr[fi], sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", nval);
          hit = (strcmp(ob->fstr[fi], nb) == 0);
        }
      } else {
        if (is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[fi]);
          hit = (strcmp(nb, sval) == 0);
        } else {
          hit = (ob->fnum[fi] == nval);
        }
      }
      if (!hit) continue;
      if (first_only) {
        var_set_str(vm, "LAST", ob->name);
        var_set_str(vm, "FINDOBJ", ob->name);
        var_set_str(vm, "WHEREOBJ", ob->name);
        var_set_str(vm, "OBJECT", ob->name);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->name);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_num(vm, "FINDOBJ_N", 1);
        var_set_num(vm, "WHEREOBJ_N", 1);
        var_set_num(vm, "WHEREOBJ_SKIP", n_skip);
        var_set_str(vm, "FIELD", fname);
        if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
        var_set_num(vm, "OK", 1);
        bump(vm);
        return 1;
      }
      ln = strlen(ob->name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, ob->name, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    if (first_only) {
      /* no match */
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_str(vm, "FINDOBJ", "");
      var_set_str(vm, "WHEREOBJ", "");
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "FINDOBJ_N", 0);
      var_set_num(vm, "WHEREOBJ_N", 0);
      var_set_num(vm, "WHEREOBJ_SKIP", n_skip);
      var_set_str(vm, "FIELD", fname);
      if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "WHEREOBJ", bag);
    var_set_str(vm, "FILTEROBJS", bag);
    var_set_str(vm, "KEEPOBJS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "WHEREOBJ_N", n);
    var_set_num(vm, "FILTEROBJS_N", n);
    var_set_num(vm, "KEEPOBJS_N", n);
    var_set_num(vm, "WHEREOBJ_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* COUNTOBJ|NOBJS|COUNTOBJS [Class]
   * — count live objects (optional class filter). LAST_N = count; LAST = decimal.
   * Usability: fleet size without LISTOBJS bag materialize. */
  if (kw(&L->cur, "COUNTOBJ") || kw(&L->cur, "COUNTOBJS") ||
      kw(&L->cur, "NUMOBJS") || kw(&L->cur, "OBJCOUNT") ||
      kw(&L->cur, "COUNTINST") || kw(&L->cur, "NINST") ||
      kw(&L->cur, "NOBJS")) {
    /* NOBJS as stmt = count-only; LISTOBJS also sets NOBJS after listing. */
    char filt[48];
    int has_filt = 0, i, n = 0;
    lex_next(L);
    filt[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "COUNTOBJ [Class]"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "SYS") && !kw(&L->cur, "PRINT") &&
               !kw(&L->cur, "CUBE") && !kw(&L->cur, "END") &&
               !kw(&L->cur, "NEW") && !kw(&L->cur, "CLASS")) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_STR) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      n++;
    }
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "COUNTOBJ", n);
    var_set_num(vm, "COUNTOBJ_N", n);
    var_set_num(vm, "NOBJS", n);
    var_set_num(vm, "NUMOBJS", n);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* COUNTWHERE [Class] field value
   * — count live objs with field==value (num or string). Soft; LAST_N=count.
   * Usability: fleet tally without WHEREOBJ bag · IF/gates for pool size. */
  if (kw(&L->cur, "COUNTWHERE") || kw(&L->cur, "COUNTMATCH") ||
      kw(&L->cur, "COUNTEQ") || kw(&L->cur, "NWHERE") ||
      kw(&L->cur, "HOWMANY") || kw(&L->cur, "TALLYWHERE")) {
    char filt[48], fname[48], tok1[48], sval[512];
    int has_filt = 0, is_str = 0, i, n = 0, n_skip = 0;
    long nval = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "COUNTWHERE OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "COUNTWHERE [Class] field value"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "COUNTWHERE [Class] field value"); return -1;
      }
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(sval, sizeof sval, "%s", L->cur.text);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(sval, sizeof sval, "%s", vm->last_str);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(sval, sizeof sval, "%s", sv->sval);
        is_str = 1;
        lex_next(L);
      } else {
        nval = parse_expr(vm, L);
        is_str = 0;
      }
    } else {
      nval = parse_expr(vm, L);
      is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) {
        if (is_str)
          hit = (strcmp(ob->fstr[fi], sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", nval);
          hit = (strcmp(ob->fstr[fi], nb) == 0);
        }
      } else {
        if (is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[fi]);
          hit = (strcmp(nb, sval) == 0);
        } else {
          hit = (ob->fnum[fi] == nval);
        }
      }
      if (hit) n++;
    }
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "COUNTWHERE", n);
    var_set_num(vm, "COUNTWHERE_N", n);
    var_set_num(vm, "COUNTMATCH_N", n);
    var_set_num(vm, "HOWMANY_N", n);
    var_set_num(vm, "COUNTWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* COUNTWHEREGE|COUNTWHERELE|COUNTWHEREGT|COUNTWHERELT [Class] field value
   * COUNTATLEAST|COUNTATMOST|COUNTABOVE|COUNTBELOW aliases.
   * — count live objs whose numeric field meets threshold. Soft; LAST_N=count.
   * Usability: SLO / capacity gates without WHEREGE bag materialize. */
  if (kw(&L->cur, "COUNTWHEREGE") || kw(&L->cur, "COUNTWHEREGTE") ||
      kw(&L->cur, "COUNTATLEAST") || kw(&L->cur, "NGE") ||
      kw(&L->cur, "COUNTWHEREGT") || kw(&L->cur, "COUNTABOVE") ||
      kw(&L->cur, "NGT") ||
      kw(&L->cur, "COUNTWHERELE") || kw(&L->cur, "COUNTWHERELTE") ||
      kw(&L->cur, "COUNTATMOST") || kw(&L->cur, "NLE") ||
      kw(&L->cur, "COUNTWHERELT") || kw(&L->cur, "COUNTBELOW") ||
      kw(&L->cur, "NLT") ||
      kw(&L->cur, "THRESHCOUNT") || kw(&L->cur, "COUNTTHRESH")) {
    char filt[48], fname[48], tok1[48], op[32];
    int has_filt = 0, mode = 0, i, n = 0, n_skip = 0;
    long thresh = 0;
    /* mode: 0=GE 1=GT 2=LE 3=LT */
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "COUNTWHEREGT") == 0 || strcmp(op, "COUNTABOVE") == 0 ||
        strcmp(op, "NGT") == 0)
      mode = 1;
    else if (strcmp(op, "COUNTWHERELE") == 0 ||
             strcmp(op, "COUNTWHERELTE") == 0 ||
             strcmp(op, "COUNTATMOST") == 0 || strcmp(op, "NLE") == 0)
      mode = 2;
    else if (strcmp(op, "COUNTWHERELT") == 0 || strcmp(op, "COUNTBELOW") == 0 ||
             strcmp(op, "NLT") == 0)
      mode = 3;
    else
      mode = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "COUNTWHEREGE OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "COUNTWHEREGE [Class] field value"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "COUNTWHEREGE [Class] field value"); return -1;
      }
    }
    if (kw(&L->cur, "GE") || kw(&L->cur, "GTE") || kw(&L->cur, "GT") ||
        kw(&L->cur, "LE") || kw(&L->cur, "LTE") || kw(&L->cur, "LT") ||
        kw(&L->cur, "MIN") || kw(&L->cur, "MAX"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      thresh = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      thresh = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        thresh = atol(sv->sval);
      else
        thresh = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      thresh = parse_expr(vm, L);
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (mode == 0) hit = (v >= thresh);
      else if (mode == 1) hit = (v > thresh);
      else if (mode == 2) hit = (v <= thresh);
      else hit = (v < thresh);
      if (hit) n++;
    }
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "COUNTWHEREGE", n);
    var_set_num(vm, "COUNTWHEREGE_N", n);
    var_set_num(vm, "COUNTWHERELE_N", n);
    var_set_num(vm, "COUNTWHEREGT_N", n);
    var_set_num(vm, "COUNTWHERELT_N", n);
    var_set_num(vm, "COUNTATLEAST_N", n);
    var_set_num(vm, "COUNTATMOST_N", n);
    var_set_num(vm, "COUNTABOVE_N", n);
    var_set_num(vm, "COUNTBELOW_N", n);
    var_set_num(vm, "COUNTWHEREGE_SKIP", n_skip);
    var_set_num(vm, "COUNTWHEREGE_THRESH", thresh);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASWHERE|ANYWHERE|ANYOBJWHERE [Class] field value
   * — soft 0|1 existence probe: any live obj with field==value?
   * Early-exit on first hit. LAST_N 0|1; optional FIRST name in LAST/HASWHERE.
   * Usability: IF gates without COUNTWHERE compare · mirrors HASOBJ/HASFIELD. */
  if (kw(&L->cur, "HASWHERE") || kw(&L->cur, "ANYWHERE") ||
      kw(&L->cur, "ANYOBJWHERE") || kw(&L->cur, "EXISTSWHERE") ||
      kw(&L->cur, "SOMEWHERE") || kw(&L->cur, "HASMATCH") ||
      kw(&L->cur, "ANYMATCH")) {
    char filt[48], fname[48], tok1[48], sval[512], found[48];
    int has_filt = 0, is_str = 0, i, hit_any = 0, n_skip = 0;
    long nval = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    sval[0] = 0;
    found[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "HASWHERE OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "HASWHERE [Class] field value"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "HASWHERE [Class] field value"); return -1;
      }
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(sval, sizeof sval, "%s", L->cur.text);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(sval, sizeof sval, "%s", vm->last_str);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(sval, sizeof sval, "%s", sv->sval);
        is_str = 1;
        lex_next(L);
      } else {
        nval = parse_expr(vm, L);
        is_str = 0;
      }
    } else {
      nval = parse_expr(vm, L);
      is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) {
        if (is_str)
          hit = (strcmp(ob->fstr[fi], sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", nval);
          hit = (strcmp(ob->fstr[fi], nb) == 0);
        }
      } else {
        if (is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[fi]);
          hit = (strcmp(nb, sval) == 0);
        } else {
          hit = (ob->fnum[fi] == nval);
        }
      }
      if (hit) {
        hit_any = 1;
        snprintf(found, sizeof found, "%s", ob->name);
        break;
      }
    }
    var_set_str(vm, "LAST", found);
    var_set_str(vm, "HASWHERE", found);
    var_set_str(vm, "ANYWHERE", found);
    var_set_str(vm, "FIRST", found);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", found);
    vm->last_n = hit_any ? 1 : 0;
    var_set_num(vm, "LAST_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASWHERE_N", hit_any ? 1 : 0);
    var_set_num(vm, "ANYWHERE_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASMATCH_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASWHEREGE|HASWHERELE|HASWHEREGT|HASWHERELT [Class] field value
   * ANYATLEAST|ANYABOVE|ANYBELOW|HASATLEAST aliases.
   * — soft 0|1: any live obj whose numeric field meets threshold?
   * Early-exit; LAST = first matching name. Completes WHERE/COUNT/HAS triad. */
  if (kw(&L->cur, "HASWHEREGE") || kw(&L->cur, "HASWHEREGTE") ||
      kw(&L->cur, "HASATLEAST") || kw(&L->cur, "ANYATLEAST") ||
      kw(&L->cur, "ANYWHEREGE") ||
      kw(&L->cur, "HASWHEREGT") || kw(&L->cur, "HASABOVE") ||
      kw(&L->cur, "ANYABOVE") || kw(&L->cur, "ANYWHEREGT") ||
      kw(&L->cur, "HASWHERELE") || kw(&L->cur, "HASWHERELTE") ||
      kw(&L->cur, "HASATMOST") || kw(&L->cur, "ANYATMOST") ||
      kw(&L->cur, "ANYWHERELE") ||
      kw(&L->cur, "HASWHERELT") || kw(&L->cur, "HASBELOW") ||
      kw(&L->cur, "ANYBELOW") || kw(&L->cur, "ANYWHERELT") ||
      kw(&L->cur, "THRESHHAS") || kw(&L->cur, "HASTHRESH")) {
    char filt[48], fname[48], tok1[48], found[48], op[32];
    int has_filt = 0, mode = 0, i, hit_any = 0, n_skip = 0;
    long thresh = 0;
    /* mode: 0=GE 1=GT 2=LE 3=LT */
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "HASWHEREGT") == 0 || strcmp(op, "HASABOVE") == 0 ||
        strcmp(op, "ANYABOVE") == 0 || strcmp(op, "ANYWHEREGT") == 0)
      mode = 1;
    else if (strcmp(op, "HASWHERELE") == 0 || strcmp(op, "HASWHERELTE") == 0 ||
             strcmp(op, "HASATMOST") == 0 || strcmp(op, "ANYATMOST") == 0 ||
             strcmp(op, "ANYWHERELE") == 0)
      mode = 2;
    else if (strcmp(op, "HASWHERELT") == 0 || strcmp(op, "HASBELOW") == 0 ||
             strcmp(op, "ANYBELOW") == 0 || strcmp(op, "ANYWHERELT") == 0)
      mode = 3;
    else
      mode = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    found[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "HASWHEREGE OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "HASWHEREGE [Class] field value"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "HASWHEREGE [Class] field value"); return -1;
      }
    }
    if (kw(&L->cur, "GE") || kw(&L->cur, "GTE") || kw(&L->cur, "GT") ||
        kw(&L->cur, "LE") || kw(&L->cur, "LTE") || kw(&L->cur, "LT") ||
        kw(&L->cur, "MIN") || kw(&L->cur, "MAX"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      thresh = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      thresh = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        thresh = atol(sv->sval);
      else
        thresh = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      thresh = parse_expr(vm, L);
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (mode == 0) hit = (v >= thresh);
      else if (mode == 1) hit = (v > thresh);
      else if (mode == 2) hit = (v <= thresh);
      else hit = (v < thresh);
      if (hit) {
        hit_any = 1;
        snprintf(found, sizeof found, "%s", ob->name);
        break;
      }
    }
    var_set_str(vm, "LAST", found);
    var_set_str(vm, "HASWHEREGE", found);
    var_set_str(vm, "HASWHERELE", found);
    var_set_str(vm, "HASATLEAST", found);
    var_set_str(vm, "FIRST", found);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", found);
    vm->last_n = hit_any ? 1 : 0;
    var_set_num(vm, "LAST_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASWHEREGE_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASWHERELE_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASWHEREGT_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASWHERELT_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASATLEAST_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASATMOST_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASABOVE_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASBELOW_N", hit_any ? 1 : 0);
    var_set_num(vm, "ANYATLEAST_N", hit_any ? 1 : 0);
    var_set_num(vm, "ANYBELOW_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASWHEREGE_SKIP", n_skip);
    var_set_num(vm, "HASWHEREGE_THRESH", thresh);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* DELETEWHERE|FREEWHERE|PURGEWHERE [Class] field value
   * — free every live object whose field equals value (optional class).
   * Soft always; LAST_N/DELETEWHERE_N = count freed. Optional bag of names freed.
   * Usability: WHEREOBJ + DELETEALL one-shot · pool GC without EACH+GETF+IF+DELETEOBJ. */
  if (kw(&L->cur, "DELETEWHERE") || kw(&L->cur, "FREEWHERE") ||
      kw(&L->cur, "PURGEWHERE") || kw(&L->cur, "DROPWHERE") ||
      kw(&L->cur, "KILLWHERE") || kw(&L->cur, "DELOBJWHERE") ||
      kw(&L->cur, "REMOVEWHERE") || kw(&L->cur, "CLEARWHERE")) {
    char filt[48], fname[48], tok1[48], sval[512], bag[4096];
    int has_filt = 0, is_str = 0, i, n = 0, n_skip = 0;
    long nval = 0;
    size_t o = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    sval[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "DELETEWHERE OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "DELETEWHERE [Class] field value"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "DELETEWHERE [Class] field value"); return -1;
      }
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(sval, sizeof sval, "%s", L->cur.text);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(sval, sizeof sval, "%s", vm->last_str);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(sval, sizeof sval, "%s", sv->sval);
        is_str = 1;
        lex_next(L);
      } else {
        nval = parse_expr(vm, L);
        is_str = 0;
      }
    } else {
      nval = parse_expr(vm, L);
      is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      size_t ln;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) {
        if (is_str)
          hit = (strcmp(ob->fstr[fi], sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", nval);
          hit = (strcmp(ob->fstr[fi], nb) == 0);
        }
      } else {
        if (is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[fi]);
          hit = (strcmp(nb, sval) == 0);
        } else {
          hit = (ob->fnum[fi] == nval);
        }
      }
      if (!hit) continue;
      ln = strlen(ob->name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, ob->name, ln);
        o += ln;
      }
      bag[o] = 0;
      ob->live = 0;
      if (strcmp(vm->this_obj, ob->name) == 0)
        vm->this_obj[0] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "DELETEWHERE", bag);
    var_set_str(vm, "FREEWHERE", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "DELETEWHERE_N", n);
    var_set_num(vm, "FREEWHERE_N", n);
    var_set_num(vm, "PURGEWHERE_N", n);
    var_set_num(vm, "DELETEWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* DELETEWHEREGE|DELETEWHERELE|DELETEWHEREGT|DELETEWHERELT [Class] field value
   * FREEWHEREGE|FREEWHERELE|…|PURGEBELOW|FREEABOVE aliases.
   * — free every live object whose numeric field meets threshold.
   * Soft always; string/missing fields skipped. LAST_N = count freed.
   * Usability: WHEREGE + DELETEWHERE one-shot · pool GC by range without
   * EACH+GETF+IF+DELETEOBJ (dead cells energy<=0, expired retries>=max). */
  if (kw(&L->cur, "DELETEWHEREGE") || kw(&L->cur, "DELETEWHEREGTE") ||
      kw(&L->cur, "FREEWHEREGE") || kw(&L->cur, "PURGEWHEREGE") ||
      kw(&L->cur, "KILLWHEREGE") || kw(&L->cur, "DROPWHEREGE") ||
      kw(&L->cur, "FREEATLEAST") || kw(&L->cur, "PURGEATLEAST") ||
      kw(&L->cur, "DELETEWHEREGT") || kw(&L->cur, "FREEWHEREGT") ||
      kw(&L->cur, "PURGEWHEREGT") || kw(&L->cur, "FREEABOVE") ||
      kw(&L->cur, "PURGEABOVE") || kw(&L->cur, "KILLABOVE") ||
      kw(&L->cur, "DELETEWHERELE") || kw(&L->cur, "DELETEWHERELTE") ||
      kw(&L->cur, "FREEWHERELE") || kw(&L->cur, "PURGEWHERELE") ||
      kw(&L->cur, "KILLWHERELE") || kw(&L->cur, "DROPWHERELE") ||
      kw(&L->cur, "FREEATMOST") || kw(&L->cur, "PURGEATMOST") ||
      kw(&L->cur, "DELETEWHERELT") || kw(&L->cur, "FREEWHERELT") ||
      kw(&L->cur, "PURGEWHERELT") || kw(&L->cur, "FREEBELOW") ||
      kw(&L->cur, "PURGEBELOW") || kw(&L->cur, "KILLBELOW") ||
      kw(&L->cur, "THRESHFREE") || kw(&L->cur, "FREETHRESH")) {
    char filt[48], fname[48], tok1[48], bag[4096], op[32];
    int has_filt = 0, mode = 0, i, n = 0, n_skip = 0;
    long thresh = 0;
    size_t o = 0;
    /* mode: 0=GE 1=GT 2=LE 3=LT */
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "DELETEWHEREGT") == 0 || strcmp(op, "FREEWHEREGT") == 0 ||
        strcmp(op, "PURGEWHEREGT") == 0 || strcmp(op, "FREEABOVE") == 0 ||
        strcmp(op, "PURGEABOVE") == 0 || strcmp(op, "KILLABOVE") == 0)
      mode = 1;
    else if (strcmp(op, "DELETEWHERELE") == 0 ||
             strcmp(op, "DELETEWHERELTE") == 0 ||
             strcmp(op, "FREEWHERELE") == 0 || strcmp(op, "PURGEWHERELE") == 0 ||
             strcmp(op, "KILLWHERELE") == 0 || strcmp(op, "DROPWHERELE") == 0 ||
             strcmp(op, "FREEATMOST") == 0 || strcmp(op, "PURGEATMOST") == 0)
      mode = 2;
    else if (strcmp(op, "DELETEWHERELT") == 0 || strcmp(op, "FREEWHERELT") == 0 ||
             strcmp(op, "PURGEWHERELT") == 0 || strcmp(op, "FREEBELOW") == 0 ||
             strcmp(op, "PURGEBELOW") == 0 || strcmp(op, "KILLBELOW") == 0)
      mode = 3;
    else
      mode = 0; /* GE / ATLEAST / THRESHFREE */
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "DELETEWHEREGE OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "DELETEWHEREGE [Class] field value"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "DELETEWHEREGE [Class] field value"); return -1;
      }
    }
    if (kw(&L->cur, "GE") || kw(&L->cur, "GTE") || kw(&L->cur, "GT") ||
        kw(&L->cur, "LE") || kw(&L->cur, "LTE") || kw(&L->cur, "LT") ||
        kw(&L->cur, "MIN") || kw(&L->cur, "MAX"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      thresh = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      thresh = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        thresh = atol(sv->sval);
      else
        thresh = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      thresh = parse_expr(vm, L);
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      size_t ln;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (mode == 0) hit = (v >= thresh);
      else if (mode == 1) hit = (v > thresh);
      else if (mode == 2) hit = (v <= thresh);
      else hit = (v < thresh);
      if (!hit) continue;
      ln = strlen(ob->name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, ob->name, ln);
        o += ln;
      }
      bag[o] = 0;
      ob->live = 0;
      if (strcmp(vm->this_obj, ob->name) == 0)
        vm->this_obj[0] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "DELETEWHEREGE", bag);
    var_set_str(vm, "DELETEWHERELE", bag);
    var_set_str(vm, "DELETEWHEREGT", bag);
    var_set_str(vm, "DELETEWHERELT", bag);
    var_set_str(vm, "FREEWHEREGE", bag);
    var_set_str(vm, "FREEWHERELE", bag);
    var_set_str(vm, "FREEBELOW", bag);
    var_set_str(vm, "FREEABOVE", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "DELETEWHEREGE_N", n);
    var_set_num(vm, "DELETEWHERELE_N", n);
    var_set_num(vm, "DELETEWHEREGT_N", n);
    var_set_num(vm, "DELETEWHERELT_N", n);
    var_set_num(vm, "FREEWHEREGE_N", n);
    var_set_num(vm, "FREEWHERELE_N", n);
    var_set_num(vm, "FREEBELOW_N", n);
    var_set_num(vm, "FREEABOVE_N", n);
    var_set_num(vm, "PURGEBELOW_N", n);
    var_set_num(vm, "PURGEABOVE_N", n);
    var_set_num(vm, "PURGEWHEREGE_N", n);
    var_set_num(vm, "PURGEWHERELE_N", n);
    var_set_num(vm, "DELETEWHEREGE_SKIP", n_skip);
    var_set_num(vm, "DELETEWHEREGE_THRESH", thresh);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* DELETEBETWEEN|FREEBETWEEN|PURGEBETWEEN|FREEBAND [Class] field lo [TO] hi
   * — free live objs with lo <= numeric field <= hi (auto-swap lo/hi).
   * Soft always; LAST = bag of freed names; LAST_N = count.
   * Usability: WHEREBETWEEN + DELETEWHERE one-shot · band GC without
   * dual DELETEWHEREGE+DELETEWHERELE or EACH+GETF+IF+DELETEOBJ. */
  if (kw(&L->cur, "DELETEBETWEEN") || kw(&L->cur, "FREEBETWEEN") ||
      kw(&L->cur, "PURGEBETWEEN") || kw(&L->cur, "DROPBETWEEN") ||
      kw(&L->cur, "KILLBETWEEN") || kw(&L->cur, "FREEBAND") ||
      kw(&L->cur, "PURGEBAND") || kw(&L->cur, "DELETEBAND") ||
      kw(&L->cur, "FREERANGEOBJS") || kw(&L->cur, "PURGERANGEOBJS")) {
    char filt[48], fname[48], tok1[48], bag[4096];
    int has_filt = 0, i, n = 0, n_skip = 0;
    long lo = 0, hi = 0, t;
    size_t o = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "DELETEBETWEEN OF Class field lo hi"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "DELETEBETWEEN [Class] field lo hi"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "DELETEBETWEEN [Class] field lo hi"); return -1;
      }
    }
    if (kw(&L->cur, "IN") || kw(&L->cur, "BETWEEN") || kw(&L->cur, "RANGE"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      lo = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      lo = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        lo = atol(sv->sval);
      else
        lo = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      lo = parse_expr(vm, L);
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "AND") || kw(&L->cur, "THRU") ||
        kw(&L->cur, "THROUGH") || kw(&L->cur, "UNTIL") ||
        (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "..") == 0))
      lex_next(L);
    else if (L->cur.kind == TK_COMMA)
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      hi = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      hi = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        hi = atol(sv->sval);
      else
        hi = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      hi = parse_expr(vm, L);
    }
    if (lo > hi) { t = lo; lo = hi; hi = t; }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      long v;
      size_t ln;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (v < lo || v > hi) continue;
      ln = strlen(ob->name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, ob->name, ln);
        o += ln;
      }
      bag[o] = 0;
      ob->live = 0;
      if (strcmp(vm->this_obj, ob->name) == 0)
        vm->this_obj[0] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "DELETEBETWEEN", bag);
    var_set_str(vm, "FREEBETWEEN", bag);
    var_set_str(vm, "PURGEBETWEEN", bag);
    var_set_str(vm, "FREEBAND", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "DELETEBETWEEN_N", n);
    var_set_num(vm, "FREEBETWEEN_N", n);
    var_set_num(vm, "PURGEBETWEEN_N", n);
    var_set_num(vm, "FREEBAND_N", n);
    var_set_num(vm, "DELETEBAND_N", n);
    var_set_num(vm, "DELETEBETWEEN_SKIP", n_skip);
    var_set_num(vm, "DELETEBETWEEN_LO", lo);
    var_set_num(vm, "DELETEBETWEEN_HI", hi);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* GETF obj field [OR|DEFAULT fallback]
   * TRYGETF|GETFSOFT|GETF SOFT — soft miss OK=0 (no fatal) without fallback.
   * GETF … OR val — like SYS ENV/LOOKUP: miss → LAST=fallback, OK=1, GETF_OR=1.
   * Bare GETF still fatal on unknown object/field (strict).
   * Field may be IDENT or "string" (dynamic LISTFIELDS walk).
   * Usability: agent probes without HASFIELD+IF glue. */
  if (kw(&L->cur, "GETF") || kw(&L->cur, "GETFIELD") || kw(&L->cur, "FIELDGET") ||
      kw(&L->cur, "READF") || kw(&L->cur, "TRYGETF") || kw(&L->cur, "GETFSOFT") ||
      kw(&L->cur, "SOFTGETF") || kw(&L->cur, "TRYGETFIELD")) {
    char oname[48], fname[48], fb[256];
    char op[24];
    ObjInst *ob;
    ClassDef *cd;
    int fi;
    int soft = 0;      /* soft miss without fatal */
    int have_fb = 0;   /* OR fallback present */
    int used_or = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "TRYGETF") == 0 || strcmp(op, "GETFSOFT") == 0 ||
        strcmp(op, "SOFTGETF") == 0 || strcmp(op, "TRYGETFIELD") == 0)
      soft = 1;
    lex_next(L);
    /* optional SOFT keyword: GETF SOFT obj field */
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "GETF object"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      /* IDENT field name; if named LAST and no such field, use last_str
       * (LISTFIELDS / EACH LINE walk). If IDENT is a string var, use sval. */
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      vv = var_get(vm, id, 0);
      if (vv && vv->is_str && vv->sval[0]) {
        snprintf(fname, sizeof fname, "%s", vv->sval);
      } else {
        snprintf(fname, sizeof fname, "%s", id);
      }
    } else {
      fail(vm, "GETF field"); return -1;
    }
    /* optional OR|DEFAULT|ELSE|FALLBACK value */
    fb[0] = 0;
    if (kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") || kw(&L->cur, "ELSE") ||
        kw(&L->cur, "FALLBACK")) {
      soft = 1; /* OR implies soft miss */
      lex_next(L);
      if (L->cur.kind == TK_STR) {
        snprintf(fb, sizeof fb, "%s", L->cur.text);
        lex_next(L);
        have_fb = 1;
      } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                 L->cur.kind == TK_LPAREN || L->cur.kind == TK_IDENT) {
        long v = parse_expr(vm, L);
        snprintf(fb, sizeof fb, "%ld", v);
        have_fb = 1;
      } else {
        fail(vm, "GETF field OR fallback"); return -1;
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "GETF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      if (have_fb) {
        var_set_str(vm, "LAST", fb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
        {
          char *end = NULL;
          long vn = strtol(fb, &end, 10);
          if (end && *end == 0 && fb[0]) {
            var_set_num(vm, "LAST_N", vn);
            vm->last_n = vn;
          } else {
            var_set_num(vm, "LAST_N", (long)strlen(fb));
            vm->last_n = (long)strlen(fb);
          }
        }
        used_or = 1;
        var_set_num(vm, "OK", 1);
      } else {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "GETF: unknown object");
        var_set_str(vm, "ERR", "GETF: unknown object");
      }
      var_set_num(vm, "GETF_N", 0);
      var_set_num(vm, "GETF_OR", used_or);
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    /* If field named LAST missing but last_str holds a field name, try that
     * (dynamic walk after LISTFIELDS + EACH LINE without SETF glue). */
    if (fi < 0 && strcmp(fname, "LAST") == 0 && vm->last_str[0]) {
      char alt[48];
      snprintf(alt, sizeof alt, "%s", vm->last_str);
      /* last_str may be multi-line; take first line only */
      {
        char *nl = strchr(alt, '\n');
        if (nl) *nl = 0;
      }
      fi = oop_field_idx(cd, alt);
      if (fi >= 0)
        snprintf(fname, sizeof fname, "%s", alt);
    }
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "GETF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      if (have_fb) {
        var_set_str(vm, "LAST", fb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
        {
          char *end = NULL;
          long vn = strtol(fb, &end, 10);
          if (end && *end == 0 && fb[0]) {
            var_set_num(vm, "LAST_N", vn);
            vm->last_n = vn;
          } else {
            var_set_num(vm, "LAST_N", (long)strlen(fb));
            vm->last_n = (long)strlen(fb);
          }
        }
        used_or = 1;
        var_set_num(vm, "OK", 1);
      } else {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "GETF: unknown field");
        var_set_str(vm, "ERR", "GETF: unknown field");
      }
      var_set_num(vm, "GETF_N", 0);
      var_set_num(vm, "GETF_OR", used_or);
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi]) {
      var_set_str(vm, "LAST", ob->fstr[fi]);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->fstr[fi]);
      var_set_num(vm, "LAST_N", (long)strlen(ob->fstr[fi]));
      vm->last_n = (long)strlen(ob->fstr[fi]);
    } else {
      char nb[32];
      var_set_num(vm, "LAST_N", ob->fnum[fi]);
      vm->last_n = ob->fnum[fi];
      snprintf(nb, sizeof nb, "%ld", ob->fnum[fi]);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "GETF_N", 1);
    var_set_num(vm, "GETF_OR", 0);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SETF obj field value
   * TRYSETF|SETFSOFT|SETF SOFT — soft miss OK=0 (no fatal) for unknown obj/field.
   * Bare SETF still fatal (strict). Field may be IDENT, "string", or string-var.
   * Usability: agent writes after HASFIELD without dual IF; mirrors TRYGETF. */
  if (kw(&L->cur, "SETF") || kw(&L->cur, "SETFIELD") || kw(&L->cur, "FIELDSET") ||
      kw(&L->cur, "PUTF") || kw(&L->cur, "WRITEF") || kw(&L->cur, "TRYSETF") ||
      kw(&L->cur, "SETFSOFT") || kw(&L->cur, "SOFTSETF") ||
      kw(&L->cur, "TRYSETFIELD")) {
    char oname[48], fname[48], op[24];
    ObjInst *ob;
    ClassDef *cd;
    int fi;
    int soft = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "TRYSETF") == 0 || strcmp(op, "SETFSOFT") == 0 ||
        strcmp(op, "SOFTSETF") == 0 || strcmp(op, "TRYSETFIELD") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "SETF object"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      vv = var_get(vm, id, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(fname, sizeof fname, "%s", vv->sval);
      else
        snprintf(fname, sizeof fname, "%s", id);
    } else {
      fail(vm, "SETF field"); return -1;
    }
    if (L->cur.kind == TK_EQ) lex_next(L);
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "SETF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      /* consume value so parse stays aligned */
      if (L->cur.kind == TK_STR) lex_next(L);
      else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0)
        lex_next(L);
      else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN || L->cur.kind == TK_IDENT)
        (void)parse_expr(vm, L);
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "SETF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "SETF: unknown object");
      var_set_str(vm, "ERR", "SETF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0 && strcmp(fname, "LAST") == 0 && vm->last_str[0]) {
      char alt[48];
      char *nl;
      snprintf(alt, sizeof alt, "%s", vm->last_str);
      nl = strchr(alt, '\n');
      if (nl) *nl = 0;
      fi = oop_field_idx(cd, alt);
      if (fi >= 0) snprintf(fname, sizeof fname, "%s", alt);
    }
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "SETF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      if (L->cur.kind == TK_STR) lex_next(L);
      else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0)
        lex_next(L);
      else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN || L->cur.kind == TK_IDENT)
        (void)parse_expr(vm, L);
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "SETF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "SETF: unknown field");
      var_set_str(vm, "ERR", "SETF: unknown field");
      bump(vm);
      return 1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", L->cur.text);
      ob->fis_str[fi] = 1;
      var_set_str(vm, "LAST", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", vm->last_str);
      ob->fis_str[fi] = 1;
      var_set_str(vm, "LAST", vm->last_str);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", sv->sval);
        ob->fis_str[fi] = 1;
        var_set_str(vm, "LAST", sv->sval);
        lex_next(L);
      } else {
        long v = parse_expr(vm, L);
        ob->fnum[fi] = v;
        ob->fis_str[fi] = 0;
        var_set_num(vm, "LAST_N", v);
        vm->last_n = v;
      }
    } else {
      long v = parse_expr(vm, L);
      ob->fnum[fi] = v;
      ob->fis_str[fi] = 0;
      var_set_num(vm, "LAST_N", v);
      vm->last_n = v;
    }
    var_set_num(vm, "SETF_N", 1);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* ISOF obj ClassName */
  if (kw(&L->cur, "ISOF") || kw(&L->cur, "ISINSTANCE") || kw(&L->cur, "ISA") ||
      kw(&L->cur, "INSTANCEOF")) {
    char oname[48], cname[48];
    ObjInst *ob;
    int hit = 0;
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "ISOF object"); return -1;
    }
    if (L->cur.kind != TK_IDENT) { fail(vm, "ISOF ClassName"); return -1; }
    snprintf(cname, sizeof cname, "%s", L->cur.text);
    lex_next(L);
    ob = oop_find_obj(vm, oname);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
      hit = (strcmp(vm->classes[ob->class_idx].name, cname) == 0) ? 1 : 0;
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* CLASSNAME obj */
  if (kw(&L->cur, "CLASSNAME") || kw(&L->cur, "TYPEOF_OBJ") ||
      kw(&L->cur, "OBJCLASS")) {
    char oname[48];
    ObjInst *ob;
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "CLASSNAME object"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    {
      ClassDef *cd = &vm->classes[ob->class_idx];
      var_set_str(vm, "LAST", cd->name);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", cd->name);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LISTCLASSES|CLASSES — newline bag of defined class names.
   * LAST = bag; LAST_N / NCLASSES = count. Agent discovery after INCLUDE. */
  if (kw(&L->cur, "LISTCLASSES") || kw(&L->cur, "CLASSES") ||
      kw(&L->cur, "CLASSLIST") || kw(&L->cur, "LISTCLASS")) {
    char bag[2048];
    size_t o = 0;
    int i, n = 0;
    lex_next(L);
    bag[0] = 0;
    for (i = 0; i < vm->n_classes; i++) {
      size_t ln = strlen(vm->classes[i].name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, vm->classes[i].name, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTCLASSES", bag);
    var_set_str(vm, "CLASSES", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "NCLASSES", n);
    var_set_num(vm, "LISTCLASSES_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LISTOBJS|OBJECTS [ClassName] — live object names (optional class filter).
   * LAST = newline bag; LAST_N / NOBJS = count. Optional: only that class. */
  if (kw(&L->cur, "LISTOBJS") || kw(&L->cur, "OBJECTS") ||
      kw(&L->cur, "OBJLIST") || kw(&L->cur, "LISTOBJECTS") ||
      kw(&L->cur, "LISTINST")) {
    char bag[2048], filt[48];
    size_t o = 0;
    int i, n = 0, has_filt = 0;
    lex_next(L);
    filt[0] = 0;
    if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
        !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") && !kw(&L->cur, "SYS") &&
        !kw(&L->cur, "PRINT") && !kw(&L->cur, "CUBE") && !kw(&L->cur, "END")) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_STR) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    }
    bag[0] = 0;
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      size_t ln;
      if (!ob->live) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      ln = strlen(ob->name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, ob->name, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTOBJS", bag);
    var_set_str(vm, "OBJECTS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "NOBJS", n);
    var_set_num(vm, "LISTOBJS_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASMETHOD obj|Class method — LAST_N 1|0 soft probe before SEND.
   * First arg object (live) or ClassName. Usability: agent IF without fatal. */
  if (kw(&L->cur, "HASMETHOD") || kw(&L->cur, "HASMETH") ||
      kw(&L->cur, "CANCALL") || kw(&L->cur, "RESPONDS") ||
      kw(&L->cur, "RESPONDSTO") || kw(&L->cur, "METHOD?")) {
    char a[48], mname[48];
    ClassDef *cd = NULL;
    ObjInst *ob;
    MethodDef *md;
    int hit = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "HASMETHOD obj|Class method"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    } else {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "HASMETHOD method"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    }
    ob = oop_find_obj(vm, a);
    if (ob)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    if (cd) {
      md = oop_find_method(cd, mname);
      if (md) hit = 1;
    }
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASMETHOD_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LISTMETHODS Class|obj — newline bag of method names on class/object.
   * LAST_N / NMETHODS = count. Soft empty if unknown. */
  if (kw(&L->cur, "LISTMETHODS") || kw(&L->cur, "METHODS") ||
      kw(&L->cur, "METHODLIST") || kw(&L->cur, "LISTMETH")) {
    char a[48], bag[2048];
    ClassDef *cd = NULL;
    ObjInst *ob;
    size_t o = 0;
    int i, n = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "LISTMETHODS Class|obj"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    } else {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    }
    ob = oop_find_obj(vm, a);
    if (ob)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    bag[0] = 0;
    if (cd) {
      for (i = 0; i < cd->n_methods; i++) {
        size_t ln = strlen(cd->methods[i].name);
        if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) {
          memcpy(bag + o, cd->methods[i].name, ln);
          o += ln;
        }
        bag[o] = 0;
        n++;
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTMETHODS", bag);
    var_set_str(vm, "METHODS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "NMETHODS", n);
    var_set_num(vm, "LISTMETHODS_N", n);
    var_set_num(vm, "OK", cd ? 1 : 0);
    if (!cd) {
      var_set_str(vm, "LAST_ERR", "LISTMETHODS: unknown class/obj");
      var_set_str(vm, "ERR", "LISTMETHODS: unknown class/obj");
    }
    bump(vm);
    return 1;
  }

  /* HASOBJ|LIVES|ISOBJ|ALIVEOBJ name — soft probe if live object exists.
   * LAST_N 1|0. Usability: IF before SEND/GETF without fatal. */
  if (kw(&L->cur, "HASOBJ") || kw(&L->cur, "LIVES") || kw(&L->cur, "ISOBJ") ||
      kw(&L->cur, "ALIVEOBJ") || kw(&L->cur, "OBJ?") || kw(&L->cur, "HASOBJECT") ||
      kw(&L->cur, "OBJEXISTS") || kw(&L->cur, "EXISTS_OBJ")) {
    char oname[48];
    ObjInst *ob;
    int hit = 0;
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "HASOBJ name"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    hit = ob ? 1 : 0;
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASOBJ_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* DELETEOBJ|FREEOBJ|DELOBJ|DISPOSE name — mark live object dead (free slot).
   * Note: DESTROY is cube DECONSTRUCT (ops_cell) — not this form.
   * Soft OK=0 if missing. NEW reuses same name. Usability: pool recycle. */
  if (kw(&L->cur, "DELETEOBJ") || kw(&L->cur, "FREEOBJ") ||
      kw(&L->cur, "DELOBJ") || kw(&L->cur, "RELEASE") ||
      kw(&L->cur, "KILLOBJ") || kw(&L->cur, "DISPOSE") ||
      kw(&L->cur, "UNOBJ") || kw(&L->cur, "OBJDEL") ||
      kw(&L->cur, "DEL_OBJ") || kw(&L->cur, "FREESLOT")) {
    char oname[48];
    ObjInst *ob;
    lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(oname, sizeof oname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      if (strcmp(L->cur.text, "LAST") == 0) {
        snprintf(oname, sizeof oname, "%s", vm->last_str);
        lex_next(L);
      } else {
        snprintf(oname, sizeof oname, "%s", L->cur.text);
        lex_next(L);
      }
    } else {
      fail(vm, "DELETEOBJ name"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "DELETEOBJ: unknown object");
      var_set_str(vm, "ERR", "DELETEOBJ: unknown object");
      bump(vm);
      return 1;
    }
    ob->live = 0;
    /* clear this if receiver destroyed */
    if (strcmp(vm->this_obj, oname) == 0)
      vm->this_obj[0] = 0;
    var_set_str(vm, "LAST", oname);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", oname);
    var_set_num(vm, "LAST_N", 1);
    vm->last_n = 1;
    var_set_num(vm, "DELETEOBJ_N", 1);
    var_set_num(vm, "FREEOBJ_N", 1);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* DELETEALL|FREEALL|CLEAROBJS [Class] — free every live object (optional class).
   * Soft always; LAST_N/DELETEALL_N = count freed. Complements DELETEOBJ + SENDALL.
   * Usability: pool wipe without EACH OBJ + DELETEOBJ glue. */
  if (kw(&L->cur, "DELETEALL") || kw(&L->cur, "FREEALL") ||
      kw(&L->cur, "CLEAROBJS") || kw(&L->cur, "CLEAROBJECTS") ||
      kw(&L->cur, "PURGEOBJS") || kw(&L->cur, "KILLALL") ||
      kw(&L->cur, "DELOBJALL") || kw(&L->cur, "OBJCLEAR")) {
    char filt[48];
    int has_filt = 0, i, n = 0;
    lex_next(L);
    filt[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_STR) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "SYS") && !kw(&L->cur, "PRINT") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "CUBE") &&
               !kw(&L->cur, "NEW") && !kw(&L->cur, "OF")) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      ob->live = 0;
      if (strcmp(vm->this_obj, ob->name) == 0)
        vm->this_obj[0] = 0;
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "DELETEALL_N", n);
    var_set_num(vm, "FREEALL_N", n);
    var_set_num(vm, "CLEAROBJS_N", n);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* CLONEOBJ|COPYOBJ|DUPLOBJ src dst — shallow copy live object fields into
   * new instance of same class. Does not re-run init (snapshot clone).
   * Soft miss src OK=0; dst live already → fatal redefine.
   * Usability: template/prototype + pool spawn without field-by-field SETF. */
  if (kw(&L->cur, "CLONEOBJ") || kw(&L->cur, "COPYOBJ") ||
      kw(&L->cur, "DUPLOBJ") || kw(&L->cur, "CLONE") ||
      kw(&L->cur, "OBJCLONE") || kw(&L->cur, "OBJCOPY") ||
      kw(&L->cur, "DUP_OBJ") || kw(&L->cur, "COPY_OBJ")) {
    char sname[48], dname[48];
    ObjInst *src, *dst;
    ClassDef *cd;
    int fi;
    lex_next(L);
    if (oop_read_name(vm, L, sname, sizeof sname, "CLONEOBJ src") < 0)
      return -1;
    if (kw(&L->cur, "AS") || kw(&L->cur, "TO") || kw(&L->cur, "INTO"))
      lex_next(L);
    if (oop_read_name(vm, L, dname, sizeof dname, "CLONEOBJ dst") < 0)
      return -1;
    src = oop_find_obj(vm, sname);
    if (!src) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "CLONEOBJ_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CLONEOBJ: unknown source");
      var_set_str(vm, "ERR", "CLONEOBJ: unknown source");
      bump(vm);
      return 1;
    }
    if (src->class_idx < 0 || src->class_idx >= vm->n_classes) {
      fail(vm, "CLONEOBJ bad class"); return -1;
    }
    cd = &vm->classes[src->class_idx];
    /* allocate dst without ctor (field defaults then overwrite from src) */
    if (oop_new_instance(vm, cd->name, dname, -1, NULL, 0) < 0)
      return -1;
    dst = oop_find_obj(vm, dname);
    if (!dst) {
      fail(vm, "CLONEOBJ alloc fail"); return -1;
    }
    for (fi = 0; fi < cd->n_fields && fi < CUBALC_MAX_FIELDS; fi++) {
      dst->fis_str[fi] = src->fis_str[fi];
      if (src->fis_str[fi])
        snprintf(dst->fstr[fi], sizeof dst->fstr[fi], "%s", src->fstr[fi]);
      else
        dst->fnum[fi] = src->fnum[fi];
    }
    /* pure object clone: not bound to src cube */
    dst->cube_idx = -1;
    var_set_str(vm, "LAST", dname);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", dname);
    var_set_str(vm, "OBJECT", dname);
    var_set_str(vm, "CLASS", cd->name);
    var_set_str(vm, "CLONEOBJ", dname);
    var_set_str(vm, "CLONE_SRC", sname);
    var_set_num(vm, "LAST_N", cd->n_fields);
    vm->last_n = cd->n_fields;
    var_set_num(vm, "CLONEOBJ_N", 1);
    var_set_num(vm, "NFIELDS", cd->n_fields);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* RENAMEOBJ|MOVEOBJ|RENOBJ old [AS|TO] new — rename live object slot in place.
   * Soft miss old OK=0. Live destination → fatal redefine. Dead destination
   * name cleared so pool recycle stays unique. Updates THIS if receiver.
   * Usability: promote temp/proto names after CLONE without re-NEW. */
  if (kw(&L->cur, "RENAMEOBJ") || kw(&L->cur, "MOVEOBJ") ||
      kw(&L->cur, "RENOBJ") || kw(&L->cur, "OBJRENAME") ||
      kw(&L->cur, "RENAME_OBJ") || kw(&L->cur, "MVOBJ") ||
      kw(&L->cur, "OBJMOVE") || kw(&L->cur, "ALIASOBJ")) {
    char oname[48], nname[48], oldn[48];
    ObjInst *ob;
    ClassDef *cd;
    int i;
    lex_next(L);
    if (oop_read_name(vm, L, oname, sizeof oname, "RENAMEOBJ old") < 0)
      return -1;
    if (kw(&L->cur, "AS") || kw(&L->cur, "TO") || kw(&L->cur, "INTO"))
      lex_next(L);
    if (oop_read_name(vm, L, nname, sizeof nname, "RENAMEOBJ new") < 0)
      return -1;
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "RENAMEOBJ_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "RENAMEOBJ: unknown object");
      var_set_str(vm, "ERR", "RENAMEOBJ: unknown object");
      bump(vm);
      return 1;
    }
    /* no-op same name */
    if (strcmp(oname, nname) == 0) {
      var_set_str(vm, "LAST", nname);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nname);
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_num(vm, "RENAMEOBJ_N", 1);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    /* destination live already? */
    if (oop_find_obj(vm, nname)) {
      snprintf(vm->err, sizeof vm->err, "RENAMEOBJ destination live %s", nname);
      fail(vm, vm->err);
      return -1;
    }
    /* clear dead slots that hold destination name (pool uniqueness) */
    for (i = 0; i < vm->n_objs; i++) {
      if (&vm->objs[i] == ob) continue;
      if (!vm->objs[i].live && strcmp(vm->objs[i].name, nname) == 0)
        vm->objs[i].name[0] = 0;
    }
    snprintf(oldn, sizeof oldn, "%s", ob->name);
    snprintf(ob->name, sizeof ob->name, "%s", nname);
    if (strcmp(vm->this_obj, oldn) == 0) {
      snprintf(vm->this_obj, sizeof vm->this_obj, "%s", nname);
      var_set_str(vm, "THIS", nname);
      var_set_str(vm, "SELF", nname);
    }
    if (ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
      cd = &vm->classes[ob->class_idx];
      var_set_str(vm, nname, cd->name);
      var_set_str(vm, "CLASS", cd->name);
    }
    var_set_str(vm, "LAST", nname);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", nname);
    var_set_str(vm, "OBJECT", nname);
    var_set_str(vm, "RENAMEOBJ", nname);
    var_set_str(vm, "RENAME_FROM", oldn);
    var_set_num(vm, "LAST_N", 1);
    vm->last_n = 1;
    var_set_num(vm, "RENAMEOBJ_N", 1);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASFIELD obj|Class field — soft 0|1 probe before GETF/SETF. */
  if (kw(&L->cur, "HASFIELD") || kw(&L->cur, "HASF") ||
      kw(&L->cur, "FIELD?") || kw(&L->cur, "HASFILD") ||
      kw(&L->cur, "HAS_FIELD")) {
    char a[48], fname[48];
    ClassDef *cd = NULL;
    ObjInst *ob;
    int hit = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "HASFIELD obj|Class field"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text); lex_next(L);
    } else {
      snprintf(a, sizeof a, "%s", L->cur.text); lex_next(L);
    }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "HASFIELD field"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(fname, sizeof fname, "%s", L->cur.text); lex_next(L);
    } else {
      snprintf(fname, sizeof fname, "%s", L->cur.text); lex_next(L);
    }
    ob = oop_find_obj(vm, a);
    if (ob) cd = &vm->classes[ob->class_idx];
    else cd = oop_find_class(vm, a);
    if (cd && oop_field_idx(cd, fname) >= 0) hit = 1;
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASFIELD_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LISTFIELDS Class|obj — newline bag of field names. */
  if (kw(&L->cur, "LISTFIELDS") || kw(&L->cur, "FIELDS") ||
      kw(&L->cur, "FIELDLIST") || kw(&L->cur, "LISTF")) {
    char a[48], bag[2048];
    ClassDef *cd = NULL;
    ObjInst *ob;
    size_t o = 0;
    int i, n = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "LISTFIELDS Class|obj"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text); lex_next(L);
    } else {
      snprintf(a, sizeof a, "%s", L->cur.text); lex_next(L);
    }
    ob = oop_find_obj(vm, a);
    if (ob) cd = &vm->classes[ob->class_idx];
    else cd = oop_find_class(vm, a);
    bag[0] = 0;
    if (cd) {
      for (i = 0; i < cd->n_fields; i++) {
        size_t ln = strlen(cd->fields[i].name);
        if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) {
          memcpy(bag + o, cd->fields[i].name, ln);
          o += ln;
        }
        bag[o] = 0;
        n++;
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTFIELDS", bag);
    var_set_str(vm, "FIELDS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "NFIELDS", n);
    var_set_num(vm, "LISTFIELDS_N", n);
    var_set_num(vm, "OK", cd ? 1 : 0);
    if (!cd) {
      var_set_str(vm, "LAST_ERR", "LISTFIELDS: unknown class/obj");
      var_set_str(vm, "ERR", "LISTFIELDS: unknown class/obj");
    }
    bump(vm);
    return 1;
  }

  /* DUMPOBJ|INSPECT|OBJDUMP|DUMPF obj — field:value bag of live object state.
   * Complements LISTFIELDS (names only): one-shot snapshot for agents without
   * EACH+GETF. LAST = newline bag field:val (LOOKUP/KVGET ready); LAST_N =
   * NFIELDS count. Soft OK=0 if missing. Optional JSON flag → cubalc.obj.v1. */
  if (kw(&L->cur, "DUMPOBJ") || kw(&L->cur, "INSPECT") ||
      kw(&L->cur, "OBJDUMP") || kw(&L->cur, "DUMPF") ||
      kw(&L->cur, "INSPECTOBJ") || kw(&L->cur, "OBJINSPECT") ||
      kw(&L->cur, "SHOWOBJ") || kw(&L->cur, "DUMPFIELDS")) {
    char oname[48], bag[4096];
    ObjInst *ob;
    ClassDef *cd;
    size_t o = 0;
    int i, n = 0, as_json = 0;
    lex_next(L);
    /* optional JSON|ASJSON before or after name */
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(oname, sizeof oname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      if (strcmp(L->cur.text, "LAST") == 0) {
        snprintf(oname, sizeof oname, "%s", vm->last_str);
        lex_next(L);
      } else if (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
                 kw(&L->cur, "TOJSON") || kw(&L->cur, "J")) {
        as_json = 1;
        lex_next(L);
        if (L->cur.kind == TK_STR) {
          snprintf(oname, sizeof oname, "%s", L->cur.text);
          lex_next(L);
        } else if (L->cur.kind == TK_IDENT) {
          if (strcmp(L->cur.text, "LAST") == 0)
            snprintf(oname, sizeof oname, "%s", vm->last_str);
          else
            snprintf(oname, sizeof oname, "%s", L->cur.text);
          lex_next(L);
        } else {
          fail(vm, "DUMPOBJ name"); return -1;
        }
      } else {
        snprintf(oname, sizeof oname, "%s", L->cur.text);
        lex_next(L);
      }
    } else {
      fail(vm, "DUMPOBJ name"); return -1;
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "NFIELDS", 0);
      var_set_num(vm, "DUMPOBJ_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "DUMPOBJ: unknown object");
      var_set_str(vm, "ERR", "DUMPOBJ: unknown object");
      bump(vm);
      return 1;
    }
    if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) {
      fail(vm, "DUMPOBJ bad class"); return -1;
    }
    cd = &vm->classes[ob->class_idx];
    bag[0] = 0;
    if (as_json) {
      /* compact cubalc.obj.v1 — fields as object; numbers bare, strings quoted */
      o = (size_t)snprintf(bag, sizeof bag,
                           "{\"schema\":\"cubalc.obj.v1\",\"name\":\"%s\","
                           "\"class\":\"%s\",\"n\":%d,\"fields\":{",
                           oname, cd->name, cd->n_fields);
      for (i = 0; i < cd->n_fields && o + 8 < sizeof bag; i++) {
        FieldDef *fd = &cd->fields[i];
        char vb[160];
        size_t need;
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = ',';
        if (ob->fis_str[i]) {
          /* escape " \ and control minimally for plate safety */
          size_t vi, vo = 0;
          vb[vo++] = '"';
          for (vi = 0; ob->fstr[i][vi] && vo + 3 < sizeof vb; vi++) {
            char c = ob->fstr[i][vi];
            if (c == '"' || c == '\\') { vb[vo++] = '\\'; vb[vo++] = c; }
            else if ((unsigned char)c < 0x20) { /* skip controls */ }
            else vb[vo++] = c;
          }
          vb[vo++] = '"';
          vb[vo] = 0;
        } else {
          snprintf(vb, sizeof vb, "%ld", ob->fnum[i]);
        }
        need = strlen(fd->name) + 3 + strlen(vb);
        if (o + need >= sizeof bag) break;
        o += (size_t)snprintf(bag + o, sizeof bag - o, "\"%s\":%s", fd->name,
                              vb);
        n++;
      }
      if (o + 3 < sizeof bag) {
        bag[o++] = '}';
        bag[o++] = '}';
        bag[o] = 0;
      }
    } else {
      /* field:value bag — LOOKUP/KVGET/TOPKEY ready */
      for (i = 0; i < cd->n_fields; i++) {
        FieldDef *fd = &cd->fields[i];
        char line[192];
        size_t ln;
        if (ob->fis_str[i])
          snprintf(line, sizeof line, "%s:%s", fd->name, ob->fstr[i]);
        else
          snprintf(line, sizeof line, "%s:%ld", fd->name, ob->fnum[i]);
        ln = strlen(line);
        if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) {
          memcpy(bag + o, line, ln);
          o += ln;
        }
        bag[o] = 0;
        n++;
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "DUMPOBJ", bag);
    var_set_str(vm, "INSPECT", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "NFIELDS", n);
    var_set_num(vm, "DUMPOBJ_N", n);
    var_set_str(vm, "OBJECT", oname);
    var_set_str(vm, "CLASS", cd->name);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* plane ops_flow */
  if (kw(&L->cur,"FN")||kw(&L->cur,"FUNC")||kw(&L->cur,"FUNCTION")||kw(&L->cur,"DEF")){
    char fname[48];
    char params[8][32];
    int n_params = 0;
    size_t b0, b1, blen;
    int depth = 1;
    FnDef *fn;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FN name"); return -1; }
    snprintf(fname,sizeof fname,"%s",L->cur.text); lex_next(L);
    memset(params, 0, sizeof params);
    while (n_params < 8 && L->cur.kind == TK_IDENT &&
           !kw(&L->cur, "END") && !kw(&L->cur, "THEN")) {
      snprintf(params[n_params], sizeof params[0], "%s", L->cur.text);
      n_params++;
      lex_next(L);
    }
    skip_nl(L);
    b0 = L->tok_off;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FN without END"); return -1; }
    b1 = L->tok_off;
    if (b1 < b0) b1 = b0;
    blen = b1 - b0;
    if (vm->n_fns >= CUBALC_MAX_FNS){ fail(vm,"too many FN"); return -1; }
    fn = &vm->fns[vm->n_fns++];
    memset(fn, 0, sizeof *fn);
    snprintf(fn->name, sizeof fn->name, "%s", fname);
    fn->body = L->s + b0;
    fn->len = blen;
    fn->n_params = n_params;
    {
      int pi;
      for (pi = 0; pi < n_params; pi++)
        snprintf(fn->params[pi], sizeof fn->params[0], "%s", params[pi]);
    }
    if (kw(&L->cur,"END")) lex_next(L);
    if (vm->trace) fprintf(vm->trace, "# FN %s params=%d len=%zu\n", fname, n_params, blen);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CALL")||kw(&L->cur,"RUNFN")||kw(&L->cur,"DO")||
      kw(&L->cur,"CALLIF")||kw(&L->cur,"CALLNZ")||kw(&L->cur,"CALLZ")||
      kw(&L->cur,"CALLWHEN")||kw(&L->cur,"CALLUNLESS")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    char fname[48];
    int mode = 0;
    long cond = 1;
    int do_call = 1;
    FnDef *fn=NULL;
    int i;
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (strcmp(op,"CALLIF")==0 || strcmp(op,"CALLNZ")==0 || strcmp(op,"CALLWHEN")==0) mode = 1;
    else if (strcmp(op,"CALLZ")==0 || strcmp(op,"CALLUNLESS")==0) mode = 2;
    if (mode) cond = parse_expr(vm, L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"CALL name"); return -1; }
    snprintf(fname,sizeof fname,"%s",L->cur.text); lex_next(L);
    if (mode == 1) do_call = (cond != 0);
    else if (mode == 2) do_call = (cond == 0);
    if (!do_call){
      int ai=0;
      while (ai<8 && (L->cur.kind==TK_NUM||L->cur.kind==TK_IDENT||L->cur.kind==TK_STR||L->cur.kind==TK_MINUS||L->cur.kind==TK_LPAREN)){
        if (oop_stmt_kw(L)) break;
        if (L->cur.kind==TK_STR){ lex_next(L); }
        else { (void)parse_expr(vm,L); }
        ai++;
      }
      var_set_num(vm,"CALLED",0);
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    /* CALL obj method — OOP sugar when first name is a live object */
    {
      ObjInst *ob = oop_find_obj(vm, fname);
      if (ob && L->cur.kind == TK_IDENT) {
        ClassDef *cd = &vm->classes[ob->class_idx];
        MethodDef *md = oop_find_method(cd, L->cur.text);
        if (md) {
          lex_next(L);
          oop_bind_args(vm, L, md->params, md->n_params);
          if (oop_run_method(vm, ob, md) < 0) return -1;
          var_set_num(vm, "OK", 1);
          bump(vm);
          return 1;
        }
      }
    }
    for (i=0;i<vm->n_fns;i++) if (strcmp(vm->fns[i].name,fname)==0){ fn=&vm->fns[i]; break; }
    if (!fn){ snprintf(vm->err,sizeof vm->err,"CALL unknown FN %s", fname); fail(vm,vm->err); return -1; }
    oop_bind_args(vm, L, fn->params, fn->n_params);
    var_set_num(vm, "CALLED", 1);
    vm->return_fn = 0;
    {
      Lex fl; lex_init(&fl, fn->body, fn->len);
      if (exec_stmts_until(vm, &fl, "END", NULL)<0) return -1;
    }
    vm->return_fn = 0;
    bump(vm); return 1;
  }
  /* RET [expr] — early return from FN (digit-4 control flow)
     RETIF cond [expr] / RETNZ / RETZ / RETUNLESS — conditional return (digit-1) */
  if (kw(&L->cur,"RET")||kw(&L->cur,"RETURN")||
      kw(&L->cur,"RETIF")||kw(&L->cur,"RETNZ")||kw(&L->cur,"RETZ")||
      kw(&L->cur,"RETUNLESS")||kw(&L->cur,"RETWHEN")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    int mode = 0; /* 0 always, 1 if nz, 2 if z */
    if (strcmp(op,"RETIF")==0 || strcmp(op,"RETNZ")==0 || strcmp(op,"RETWHEN")==0) mode = 1;
    else if (strcmp(op,"RETZ")==0 || strcmp(op,"RETUNLESS")==0) mode = 2;
    int do_ret = 1;
    if (mode){
      long cond = parse_expr(vm, L);
      if (mode == 1) do_ret = (cond != 0);
      else do_ret = (cond == 0);
    }
    /* optional return value when next looks like an expression start */
    int has_val = 0;
    long v = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        L->cur.kind==TK_STR ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"END") && !kw(&L->cur,"ELSE") &&
         !kw(&L->cur,"FN") && !kw(&L->cur,"CALL") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"ASSERT") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"RET") &&
         !kw(&L->cur,"RETURN") && !kw(&L->cur,"RETIF") && !kw(&L->cur,"RETZ") &&
         !kw(&L->cur,"RETNZ") && !kw(&L->cur,"RETUNLESS") && !kw(&L->cur,"RETWHEN") &&
         !kw(&L->cur,"WHEN") && !kw(&L->cur,"DEFAULT") &&
         !kw(&L->cur,"FOR") && !kw(&L->cur,"WHILE") && !kw(&L->cur,"LOOP") &&
         !kw(&L->cur,"IF") && !kw(&L->cur,"UNLESS") && !kw(&L->cur,"BREAK") &&
         !kw(&L->cur,"CONTINUE") && !kw(&L->cur,"CASE") && !kw(&L->cur,"CUBE") &&
         !kw(&L->cur,"SYS"))){
      v = parse_expr(vm, L);
      has_val = 1;
    }
    if (!do_ret){
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    if (has_val){
      var_set_num(vm, "LAST_N", v);
      vm->last_n = v;
      var_set_num(vm, "RETVAL", v);
    }
    vm->return_fn = 1;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* CASE expr ... WHEN n THEN ... [DEFAULT ...] END */
  if (kw(&L->cur,"CASE")||kw(&L->cur,"SWITCH")||kw(&L->cur,"MATCH")){
    lex_next(L);
    long sel = parse_expr(vm, L);
    skip_nl(L);
    int matched = 0;
    int ran = 0;
    for(;;){
      skip_nl(L);
      if (L->cur.kind==TK_EOF){ fail(vm,"CASE without END"); return -1; }
      if (kw(&L->cur,"END")){ lex_next(L); break; }
      if (kw(&L->cur,"WHEN")||kw(&L->cur,"OF")||kw(&L->cur,"CASEIF")){
        lex_next(L);
        long w = parse_expr(vm, L);
        long w_hi = w;
        int is_range = 0;
        /* WHEN lo TO hi THEN — inclusive range arm (digit-1 control_or_branch) */
        if (kw(&L->cur,"TO")||kw(&L->cur,"THROUGH")||kw(&L->cur,"THRU")||
            kw(&L->cur,"DOTDOT")||kw(&L->cur,"RANGE")){
          lex_next(L);
          w_hi = parse_expr(vm, L);
          is_range = 1;
        }
        if (kw(&L->cur,"THEN")) lex_next(L);
        skip_nl(L);
        Lex body_start=*L;
        int depth=1;
        while (L->cur.kind!=TK_EOF && depth>0){
          if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
            lex_next(L); if (kw(&L->cur,"IF")) lex_next(L); continue;
          }
          if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||
              kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FORCELL")||kw(&L->cur,"EACHCELL")||
              kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||
              kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")||
              kw(&L->cur,"CASE")) depth++;
          else if ((kw(&L->cur,"WHEN")||kw(&L->cur,"OF")||kw(&L->cur,"DEFAULT")||kw(&L->cur,"ELSE")) && depth==1) break;
          else if (kw(&L->cur,"END")){ depth--; if (depth==0) break; }
          lex_next(L);
        }
        int arm_hit = 0;
        if (is_range){
          long lo = w, hi = w_hi;
          if (hi < lo){ long t = lo; lo = hi; hi = t; }
          arm_hit = (sel >= lo && sel <= hi);
        } else {
          arm_hit = (w == sel);
        }
        if (!matched && !ran && arm_hit){
          matched = 1; ran = 1;
          Lex body=body_start;
          /* arm body: stop before next WHEN/DEFAULT/END (body copy only) */
          while (!vm->fatal && !vm->halt){
            skip_nl(&body);
            if (body.cur.kind==TK_EOF) break;
            if (kw(&body.cur,"END")||kw(&body.cur,"WHEN")||kw(&body.cur,"OF")||
                kw(&body.cur,"DEFAULT")||kw(&body.cur,"ELSE")||kw(&body.cur,"CASEIF")) break;
            if (vm->return_fn || vm->break_loop) break;
            int r=parse_form(vm,&body);
            if (r<0) return -1;
            if (r==0) break;
          }
          /* skip remaining arms to END on outer L (parked on next arm or END) */
          depth=1;
          while (L->cur.kind!=TK_EOF && depth>0){
            if (kw(&L->cur,"CASE")||kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||
                kw(&L->cur,"FOR")||kw(&L->cur,"FORCELL")||kw(&L->cur,"EACHCELL")||
                kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||
                kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")) depth++;
            else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
            lex_next(L);
          }
          if (kw(&L->cur,"END")) lex_next(L);
          break;
        }
        continue;
      }
      if (kw(&L->cur,"DEFAULT")||kw(&L->cur,"ELSE")){
        lex_next(L);
        if (kw(&L->cur,"THEN")) lex_next(L);
        skip_nl(L);
        if (!matched && !ran){
          Lex body=*L;
          while (!vm->fatal && !vm->halt){
            skip_nl(&body);
            if (body.cur.kind==TK_EOF || kw(&body.cur,"END")) break;
            if (vm->return_fn || vm->break_loop) break;
            int r=parse_form(vm,&body);
            if (r<0) return -1;
            if (r==0) break;
          }
          ran = 1;
        }
        /* always advance outer L to matching END */
        {
          int depth=1;
          while (L->cur.kind!=TK_EOF && depth>0){
            if (kw(&L->cur,"CASE")||kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||
                kw(&L->cur,"FOR")||kw(&L->cur,"FORCELL")||kw(&L->cur,"EACHCELL")||
                kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||
                kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")) depth++;
            else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
            lex_next(L);
          }
        }
        if (kw(&L->cur,"END")) lex_next(L);
        break;
      }
      fail(vm,"CASE expects WHEN|DEFAULT|END"); return -1;
    }
    var_set_num(vm, "MATCHED", matched || ran ? 1 : 0);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* FOR i = a TO b [STEP s] ... END
     FOR i = a DOWNTO b [STEP s] ... END  (digit-1: default step -1) */
  if (kw(&L->cur,"FOR")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FOR var = a TO b"); return -1; }
    char vname[48]; snprintf(vname,sizeof vname,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_EQ){ fail(vm,"FOR var ="); return -1; }
    lex_next(L);
    long lo=parse_expr(vm,L);
    if (!kw(&L->cur,"TO") && !kw(&L->cur,"DOWNTO") && !kw(&L->cur,"DOWN") &&
        !kw(&L->cur,"..") && !(L->cur.kind==TK_IDENT && strcmp(L->cur.text,"TO")==0)){
      /* allow FOR i = n as 0..n-1 */
      long hi=lo-1; lo=0;
      long step=1;
      skip_nl(L);
      Lex body_start=*L;
      int depth=1;
      while (L->cur.kind!=TK_EOF){
        if (block_scan_step(L, &depth, 0)) break;
      }
      if (depth!=0){ fail(vm,"FOR without END"); return -1; }
      for (long i=lo;i<=hi && !vm->fatal && !vm->halt;i+=step){
        var_set_num(vm,vname,i); var_set_num(vm,"IT",i);
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
      }
      if (kw(&L->cur,"END")) lex_next(L);
      bump(vm); return 1;
    }
    int downto = 0;
    if (kw(&L->cur,"DOWNTO")||kw(&L->cur,"DOWN")){ downto = 1; lex_next(L); }
    else if (kw(&L->cur,"TO")||kw(&L->cur,"..")) lex_next(L);
    long hi=parse_expr(vm,L);
    long step = downto ? -1 : 1;
    if (kw(&L->cur,"STEP")||kw(&L->cur,"BY")){
      lex_next(L);
      step=parse_expr(vm,L);
      if (!step) step = downto ? -1 : 1;
      /* DOWNTO with positive step → force negative direction */
      if (downto && step > 0) step = -step;
    }
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FOR without END"); return -1; }
    if (step>0){
      for (long i=lo;i<=hi && !vm->fatal && !vm->halt;i+=step){
        var_set_num(vm,vname,i); var_set_num(vm,"IT",i);
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
      }
    } else {
      for (long i=lo;i>=hi && !vm->fatal && !vm->halt;i+=step){
        var_set_num(vm,vname,i); var_set_num(vm,"IT",i);
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
      }
    }
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  /* EACH CUBE as name ... END  |  EACH CELL [as name] [FROM lo TO hi] ... END
   * EACH LINE [as name] [IN str|LAST] ... END — walk newline fields (LIST/GREP).
   * EACH OBJ [Class] [AS name] ... END — walk live OOP objects (optional class filter).
   * digit-4 control: cell-range iterator binds value to name, IT=index, VAL=value */
  if (kw(&L->cur,"EACH")||kw(&L->cur,"FOREACH")){
    lex_next(L);
    int is_cell = (kw(&L->cur,"CELL")||kw(&L->cur,"CELLS")||kw(&L->cur,"SLOT")||kw(&L->cur,"SLOTS"));
    int is_cube = (kw(&L->cur,"CUBE")||kw(&L->cur,"CUBES"));
    int is_line = (kw(&L->cur,"LINE")||kw(&L->cur,"LINES")||kw(&L->cur,"FIELD")||
                   kw(&L->cur,"FIELDS")||kw(&L->cur,"ROW")||kw(&L->cur,"ROWS")||
                   kw(&L->cur,"ENTRY")||kw(&L->cur,"ENTRIES"));
    int is_obj = (kw(&L->cur,"OBJ")||kw(&L->cur,"OBJS")||kw(&L->cur,"OBJECT")||
                  kw(&L->cur,"OBJECTS")||kw(&L->cur,"INST")||kw(&L->cur,"INSTANCE")||
                  kw(&L->cur,"INSTANCES")||kw(&L->cur,"OOP"));
    if (!is_cell && !is_cube && !is_line && !is_obj){
      fail(vm,"EACH CUBE|CELL|LINE|OBJ as name"); return -1;
    }
    lex_next(L);
    if (is_obj){
      /* EACH OBJ [ClassName] [AS name] ... END
       * Binds each live object name to name (default OBJ); IT=0-based; OBJ_N=1-based.
       * Optional Class filter like LISTOBJS. Usability: no LISTOBJS+EACH LINE glue. */
      char oname_bind[48], filt[48];
      int has_filt = 0, i, n = 0;
      snprintf(oname_bind, sizeof oname_bind, "OBJ");
      filt[0] = 0;
      /* optional class filter before AS (bare IDENT = class filter, even if
       * unknown → empty walk; use EACH OBJ AS name for unfiltered bind). */
      if (L->cur.kind == TK_IDENT && !kw(&L->cur, "AS") &&
          strcmp(L->cur.text, "->") != 0 && !kw(&L->cur, "OF") &&
          !kw(&L->cur, "END")) {
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      } else if (L->cur.kind == TK_STR) {
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
      if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
        lex_next(L);
        if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
          snprintf(filt, sizeof filt, "%s", L->cur.text);
          lex_next(L);
          has_filt = 1;
        }
      }
      if (kw(&L->cur, "AS") || kw(&L->cur, "->")) {
        lex_next(L);
        if (L->cur.kind != TK_IDENT) { fail(vm, "EACH OBJ as name"); return -1; }
        snprintf(oname_bind, sizeof oname_bind, "%s", L->cur.text);
        lex_next(L);
      }
      /* class filter after AS: EACH OBJ AS c OF Cell */
      if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
        lex_next(L);
        if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
          snprintf(filt, sizeof filt, "%s", L->cur.text);
          lex_next(L);
          has_filt = 1;
        }
      }
      skip_nl(L);
      {
        Lex body_start = *L;
        int depth = 1;
        while (L->cur.kind != TK_EOF) {
          if (block_scan_step(L, &depth, 0)) break;
        }
        if (depth != 0) { fail(vm, "EACH OBJ without END"); return -1; }
        for (i = 0; i < vm->n_objs && !vm->fatal && !vm->halt; i++) {
          ObjInst *ob = &vm->objs[i];
          ClassDef *cd;
          if (!ob->live) continue;
          if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
          cd = &vm->classes[ob->class_idx];
          if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
          var_set_str(vm, oname_bind, ob->name);
          var_set_str(vm, "OBJ", ob->name);
          var_set_str(vm, "OBJECT", ob->name);
          var_set_str(vm, "CLASS", cd->name);
          var_set_num(vm, "IT", n);
          var_set_num(vm, "IDX", n);
          var_set_num(vm, "OBJ_N", n + 1);
          var_set_num(vm, "OK", 1);
          vm->break_loop = 0;
          vm->continue_loop = 0;
          {
            Lex body = body_start;
            if (exec_stmts_until(vm, &body, "END", NULL) < 0) return -1;
          }
          if (vm->break_loop) { vm->break_loop = 0; n++; break; }
          vm->continue_loop = 0;
          n++;
        }
        if (kw(&L->cur, "END")) lex_next(L);
        var_set_num(vm, "LAST_N", n);
        var_set_num(vm, "EACH_N", n);
        var_set_num(vm, "NOBJS", n);
        var_set_num(vm, "OK", 1);
        bump(vm);
        return 1;
      }
    }
    if (is_line){
      /* EACH LINE [AS name] [IN|OF|FROM str] ... END
       * Binds each newline field to name (default LINE); IT=0-based, LINE_N=1-based.
       * Source defaults to LAST/last_str — chain after SYS LIST / SYS GREP.
       * Snapshot source before loop so body may clobber LAST. */
      char lname[48];
      char src[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      long idx = 0, nlines = 0;
      snprintf(lname, sizeof lname, "LINE");
      if (kw(&L->cur,"AS")||kw(&L->cur,"->")){
        lex_next(L);
        if (L->cur.kind!=TK_IDENT){ fail(vm,"EACH LINE as name"); return -1; }
        snprintf(lname, sizeof lname, "%s", L->cur.text); lex_next(L);
      } else if (L->cur.kind==TK_IDENT &&
                 strcmp(L->cur.text,"IN")!=0 && strcmp(L->cur.text,"OF")!=0 &&
                 strcmp(L->cur.text,"FROM")!=0 && strcmp(L->cur.text,"IN")!=0){
        snprintf(lname, sizeof lname, "%s", L->cur.text); lex_next(L);
      }
      src[0] = 0;
      if (kw(&L->cur,"IN")||kw(&L->cur,"OF")||kw(&L->cur,"FROM")||kw(&L->cur,"OVER")){
        lex_next(L);
        if (resolve_str_arg(vm, L, src, sizeof src) != 0)
          snprintf(src, sizeof src, "%s", vm->last_str);
      } else {
        /* Prefer last_str (full host buffer after LIST/GREP); fallback LAST var. */
        if (vm->last_str[0])
          snprintf(src, sizeof src, "%s", vm->last_str);
        else {
          Var *lv = var_get(vm, "LAST", 0);
          if (lv && lv->is_str)
            snprintf(src, sizeof src, "%s", lv->sval);
        }
      }
      skip_nl(L);
      Lex body_start=*L;
      int depth=1;
      while (L->cur.kind!=TK_EOF){
        if (block_scan_step(L, &depth, 0)) break;
      }
      if (depth!=0){ fail(vm,"EACH LINE without END"); return -1; }
      /* Walk fields like SYS NTH/GREP: trailing newline does not add empty last field. */
      if (src[0]) {
        p = src;
        while (*p && !vm->fatal && !vm->halt) {
          start = p;
          while (*p && *p != '\n') p++;
          /* end-of-string with empty span after a newline → stop (no field) */
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            char field[512];
            if (flen >= sizeof field) flen = sizeof field - 1;
            memcpy(field, start, flen);
            field[flen] = 0;
            var_set_str(vm, lname, field);
            var_set_str(vm, "LINE", field);
            var_set_num(vm, "IT", idx);
            var_set_num(vm, "IDX", idx);
            var_set_num(vm, "LINE_N", idx + 1);
            var_set_num(vm, "OK", 1);
            vm->break_loop=0; vm->continue_loop=0;
            Lex body=body_start;
            if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
            if (vm->break_loop){ vm->break_loop=0; idx++; nlines = idx; break; }
            vm->continue_loop=0;
            idx++;
            nlines = idx;
          }
          if (*p == '\n') p++;
        }
      }
      if (kw(&L->cur,"END")) lex_next(L);
      var_set_num(vm, "LAST_N", nlines);
      var_set_num(vm, "EACH_N", nlines);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (is_cube){
      if (kw(&L->cur,"AS")||kw(&L->cur,"->")){ lex_next(L); }
      if (L->cur.kind!=TK_IDENT){ fail(vm,"EACH CUBE as name"); return -1; }
      char cname[48]; snprintf(cname,sizeof cname,"%s",L->cur.text); lex_next(L);
      skip_nl(L);
      Lex body_start=*L;
      int depth=1;
      while (L->cur.kind!=TK_EOF){
        if (block_scan_step(L, &depth, 0)) break;
      }
      if (depth!=0){ fail(vm,"EACH without END"); return -1; }
      ensure_world(vm);
      for (int i=0;i<vm->ch.n_cubes && !vm->fatal && !vm->halt;i++){
        var_set_str(vm, cname, vm->ch.cubes[i].id);
        var_set_num(vm, "IT", i);
        var_set_num(vm, "DIGIT", vm->ch.cubes[i].atom.digit);
        var_set_num(vm, "ENERGY", (long)lround(vm->ch.cubes[i].atom.energy*100));
        var_set_num(vm, "SET", cubalc_matrix_popcount(&vm->ch.cubes[i].atom.matrix));
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
      }
      if (kw(&L->cur,"END")) lex_next(L);
      bump(vm); return 1;
    }
    /* EACH CELL [AS name] [FROM lo TO hi | lo TO hi | lo hi] */
    char vname[48]; snprintf(vname,sizeof vname,"VAL");
    if (kw(&L->cur,"AS")||kw(&L->cur,"->")){
      lex_next(L);
      if (L->cur.kind!=TK_IDENT){ fail(vm,"EACH CELL as name"); return -1; }
      snprintf(vname,sizeof vname,"%s",L->cur.text); lex_next(L);
    } else if (L->cur.kind==TK_IDENT &&
               strcmp(L->cur.text,"FROM")!=0 && strcmp(L->cur.text,"TO")!=0 &&
               strcmp(L->cur.text,"FROM")!=0){
      /* EACH CELL name lo hi  (optional bare name) */
      /* only if next tokens look like range — keep default VAL when FROM present */
      /* treat bare IDENT as name when not a keyword */
      if (strcmp(L->cur.text,"FROM")!=0){
        snprintf(vname,sizeof vname,"%s",L->cur.text); lex_next(L);
      }
    }
    long lo = 0, hi = CUBALC_CELL_N - 1;
    if (kw(&L->cur,"FROM")){
      lex_next(L);
      lo = parse_expr(vm,L);
      if (kw(&L->cur,"TO")||kw(&L->cur,"..")||kw(&L->cur,"DOWNTO")) lex_next(L);
      hi = parse_expr(vm,L);
    } else if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
               L->cur.kind==TK_MINUS){
      lo = parse_expr(vm,L);
      if (kw(&L->cur,"TO")||kw(&L->cur,"..")||kw(&L->cur,"DOWNTO")) lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
          L->cur.kind==TK_MINUS)
        hi = parse_expr(vm,L);
      else hi = lo;
    }
    if (lo < 0) lo = 0;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"EACH CELL without END"); return -1; }
    for (long i=lo;i<=hi && !vm->fatal && !vm->halt;i++){
      long val = vm->cells[(int)i];
      var_set_num(vm, vname, val);
      var_set_num(vm, "VAL", val);
      var_set_num(vm, "IT", i);
      var_set_num(vm, "IDX", i);
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* FORCELL [name] lo hi ... END — compact cell-range loop (digit-4) */
  if (kw(&L->cur,"FORCELL")||kw(&L->cur,"EACHCELL")||kw(&L->cur,"FOREACHCELL")){
    lex_next(L);
    char vname[48]; snprintf(vname,sizeof vname,"VAL");
    /* optional bind name if IDENT followed by another expr token */
    if (L->cur.kind==TK_IDENT){
      /* peek: if only one ident then it's lo as var ref — keep VAL */
      char maybe[48]; snprintf(maybe,sizeof maybe,"%s",L->cur.text);
      Lex save=*L;
      lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
          L->cur.kind==TK_MINUS || kw(&L->cur,"TO")||kw(&L->cur,"FROM")){
        snprintf(vname,sizeof vname,"%s",maybe);
      } else {
        *L = save; /* restore — treat as lo expression start */
      }
    }
    long lo = parse_expr(vm,L);
    if (kw(&L->cur,"TO")||kw(&L->cur,"..")||kw(&L->cur,"DOWNTO")||kw(&L->cur,"FROM"))
      lex_next(L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FORCELL without END"); return -1; }
    for (long i=lo;i<=hi && !vm->fatal && !vm->halt;i++){
      long val = vm->cells[(int)i];
      var_set_num(vm, vname, val);
      var_set_num(vm, "VAL", val);
      var_set_num(vm, "IT", i);
      var_set_num(vm, "IDX", i);
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-4 control: FORBIT/EACHBIT cube [AS name] ... END — iterate set-bit indices */
  if (kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||kw(&L->cur,"FOREACHBIT")||
      kw(&L->cur,"EACHBITS")||kw(&L->cur,"FORBITS")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FORBIT cube [AS name]"); return -1; }
    char cid[48]; snprintf(cid,sizeof cid,"%s",L->cur.text); lex_next(L);
    char vname[48]; snprintf(vname,sizeof vname,"IT");
    if (kw(&L->cur,"AS")||kw(&L->cur,"->")){
      lex_next(L);
      if (L->cur.kind!=TK_IDENT){ fail(vm,"FORBIT cube AS name"); return -1; }
      snprintf(vname,sizeof vname,"%s",L->cur.text); lex_next(L);
    }
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FORBIT without END"); return -1; }
    int ix=find_cube(vm,cid);
    if (ix<0){
      /* empty iteration */
      if (kw(&L->cur,"END")) lex_next(L);
      var_set_num(vm,"OK",0);
      var_set_num(vm,"LAST_N",0); vm->last_n=0;
      bump(vm); return 1;
    }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long count = 0;
    for (int bi=0; bi<n && !vm->fatal && !vm->halt; bi++){
      if (!cubalc_matrix_get(m, bi)) continue;
      var_set_num(vm, vname, bi);
      var_set_num(vm, "IT", bi);
      var_set_num(vm, "IDX", bi);
      var_set_num(vm, "BIT", bi);
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      count++;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    var_set_num(vm,"LAST_N",count); vm->last_n=count;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* PASS / NOP — no-op statement (digit-4 control placeholder) */
  if (kw(&L->cur,"PASS")||kw(&L->cur,"NOP")||kw(&L->cur,"NOOP")||kw(&L->cur,"NOTHING")){
    lex_next(L);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* BREAKIF expr · CONTINUEIF expr — single-token conditional loop control (digit-4) */
  if (kw(&L->cur,"BREAKIF")||kw(&L->cur,"BREAK_IF")){
    lex_next(L);
    long c = parse_expr(vm, L);
    if (c) vm->break_loop = 1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CONTINUEIF")||kw(&L->cur,"CONTIF")||kw(&L->cur,"SKIPIF")||
      kw(&L->cur,"NEXTIF")||kw(&L->cur,"CONTINUE_IF")){
    lex_next(L);
    long c = parse_expr(vm, L);
    if (c) vm->continue_loop = 1;
    bump(vm); return 1;
  }
  /* BREAK [IF expr] — leave enclosing loop (digit-4 control flow) */
  if (kw(&L->cur,"BREAK")){
    lex_next(L);
    if (kw(&L->cur,"IF")){
      lex_next(L);
      long c = parse_expr(vm, L);
      if (c) vm->break_loop = 1;
    } else {
      vm->break_loop = 1;
    }
    bump(vm); return 1;
  }
  /* CONTINUE [IF expr] — next loop iteration */
  if (kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
    lex_next(L);
    if (kw(&L->cur,"IF")){
      lex_next(L);
      long c = parse_expr(vm, L);
      if (c) vm->continue_loop = 1;
    } else {
      vm->continue_loop = 1;
    }
    bump(vm); return 1;
  }
  /* digit-1 control: JUMP / JZ / JNZ / CJZ / CJNZ (asm-style loop exits) */
  if (kw(&L->cur,"JUMP")||kw(&L->cur,"JMP")){
    /* JUMP [IF expr] — unconditional or conditional break */
    lex_next(L);
    if (kw(&L->cur,"IF")){
      lex_next(L);
      long c = parse_expr(vm, L);
      if (c) vm->break_loop = 1;
    } else {
      vm->break_loop = 1;
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"JZ")||kw(&L->cur,"JZERO")){
    /* JZ expr — break if expr == 0 */
    lex_next(L);
    long c = parse_expr(vm, L);
    if (!c) vm->break_loop = 1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"JNZ")||kw(&L->cur,"JNEZ")||kw(&L->cur,"JTRUE")){
    /* JNZ expr — break if expr != 0 */
    lex_next(L);
    long c = parse_expr(vm, L);
    if (c) vm->break_loop = 1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CJZ")||kw(&L->cur,"CJZERO")){
    /* CJZ expr — continue if expr == 0 */
    lex_next(L);
    long c = parse_expr(vm, L);
    if (!c) vm->continue_loop = 1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CJNZ")||kw(&L->cur,"CJNEZ")){
    /* CJNZ expr — continue if expr != 0 */
    lex_next(L);
    long c = parse_expr(vm, L);
    if (c) vm->continue_loop = 1;
    bump(vm); return 1;
  }
  /* stack-driven loop control: SJUMP/SBREAK SJZ SJNZ SCONTINUE SCJZ SCJNZ */
  if (kw(&L->cur,"SJUMP")||kw(&L->cur,"SBREAK")||kw(&L->cur,"STACKJUMP")||
      kw(&L->cur,"STACKBREAK")){
    lex_next(L);
    vm->break_loop = 1;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SCONTINUE")||kw(&L->cur,"SNEXT")||kw(&L->cur,"STACKCONTINUE")){
    lex_next(L);
    vm->continue_loop = 1;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SJZ")||kw(&L->cur,"SJZERO")||kw(&L->cur,"STACKJZ")){
    /* pop TOS; break if zero */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long c = vm->stack[--vm->sp];
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",c); vm->last_n=c;
    if (!c) vm->break_loop = 1;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SJNZ")||kw(&L->cur,"SJNEZ")||kw(&L->cur,"STACKJNZ")){
    /* pop TOS; break if nonzero */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long c = vm->stack[--vm->sp];
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",c); vm->last_n=c;
    if (c) vm->break_loop = 1;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SCJZ")||kw(&L->cur,"STACKCJZ")){
    /* pop TOS; continue if zero */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long c = vm->stack[--vm->sp];
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",c); vm->last_n=c;
    if (!c) vm->continue_loop = 1;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SCJNZ")||kw(&L->cur,"STACKCJNZ")){
    /* pop TOS; continue if nonzero */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long c = vm->stack[--vm->sp];
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",c); vm->last_n=c;
    if (c) vm->continue_loop = 1;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-1 control_or_branch: BEQ/BNE/BLT/BLE/BGT/BGE + CB* continue */
  if (kw(&L->cur,"BEQ")||kw(&L->cur,"BNE")||kw(&L->cur,"BLT")||kw(&L->cur,"BLE")||
      kw(&L->cur,"BGT")||kw(&L->cur,"BGE")||
      kw(&L->cur,"CBEQ")||kw(&L->cur,"CBNE")||kw(&L->cur,"CBLT")||kw(&L->cur,"CBLE")||
      kw(&L->cur,"CBGT")||kw(&L->cur,"CBGE")){
    char op[12]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    long a = parse_expr(vm, L);
    long b = parse_expr(vm, L);
    int cont = (op[0]=='C');
    const char *rel = cont ? op+1 : op; /* CBEQ → BEQ */
    int hit = 0;
    if (strcmp(rel,"BEQ")==0) hit = (a == b);
    else if (strcmp(rel,"BNE")==0) hit = (a != b);
    else if (strcmp(rel,"BLT")==0) hit = (a < b);
    else if (strcmp(rel,"BLE")==0) hit = (a <= b);
    else if (strcmp(rel,"BGT")==0) hit = (a > b);
    else if (strcmp(rel,"BGE")==0) hit = (a >= b);
    if (hit){
      if (cont) vm->continue_loop = 1;
      else vm->break_loop = 1;
    }
    var_set_num(vm,"LAST_N", hit ? 1 : 0); vm->last_n = hit ? 1 : 0;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* stack compare-branch: SBEQ..SBGE + SCB* continue (pop a,b under/TOS) */
  if (kw(&L->cur,"SBEQ")||kw(&L->cur,"SBNE")||kw(&L->cur,"SBLT")||kw(&L->cur,"SBLE")||
      kw(&L->cur,"SBGT")||kw(&L->cur,"SBGE")||
      kw(&L->cur,"SCBEQ")||kw(&L->cur,"SCBNE")||kw(&L->cur,"SCBLT")||kw(&L->cur,"SCBLE")||
      kw(&L->cur,"SCBGT")||kw(&L->cur,"SCBGE")||
      kw(&L->cur,"STACKBEQ")||kw(&L->cur,"STACKBNE")||kw(&L->cur,"STACKBLT")||
      kw(&L->cur,"STACKBLE")||kw(&L->cur,"STACKBGT")||kw(&L->cur,"STACKBGE")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    var_set_num(vm,"SP",vm->sp);
    int cont = 0;
    const char *rel = "BEQ";
    if (strncmp(op,"STACK",5)==0) rel = op+5;
    else if (strncmp(op,"SCB",3)==0){ cont = 1; rel = op+2; }
    else if (op[0]=='S') rel = op+1;
    int hit = 0;
    if (strcmp(rel,"BEQ")==0) hit = (a == b);
    else if (strcmp(rel,"BNE")==0) hit = (a != b);
    else if (strcmp(rel,"BLT")==0) hit = (a < b);
    else if (strcmp(rel,"BLE")==0) hit = (a <= b);
    else if (strcmp(rel,"BGT")==0) hit = (a > b);
    else if (strcmp(rel,"BGE")==0) hit = (a >= b);
    if (hit){
      if (cont) vm->continue_loop = 1;
      else vm->break_loop = 1;
    }
    var_set_num(vm,"LAST_N", hit ? 1 : 0); vm->last_n = hit ? 1 : 0;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* UNLESS cond THEN ... [ELSE ...] END — inverted IF */
  if (kw(&L->cur,"UNLESS")){
    lex_next(L);
    long cond = parse_expr(vm, L);
    if (!kw(&L->cur,"THEN")){ fail(vm,"UNLESS expr THEN"); return -1; }
    lex_next(L); skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF && depth>0){
      if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
        lex_next(L);
        if (kw(&L->cur,"IF")) lex_next(L);
        continue;
      }
      if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||
          kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FORCELL")||
          kw(&L->cur,"EACHCELL")||kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||kw(&L->cur,"FN")||
          kw(&L->cur,"REPEAT")||kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")||kw(&L->cur,"CASE"))
        depth++;
      else if ((kw(&L->cur,"ELSE")||kw(&L->cur,"ELIF")||kw(&L->cur,"ELSEIF")) && depth==1) break;
      else if (kw(&L->cur,"END")){ depth--; if (depth==0) break; }
      lex_next(L);
    }
    if (depth>1){ fail(vm,"UNLESS without END"); return -1; }
    if (!cond){
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END","ELSE")<0) return -1;
      depth=1;
      while (L->cur.kind!=TK_EOF && depth>0){
        if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||
            kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FORCELL")||
            kw(&L->cur,"EACHCELL")||kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||kw(&L->cur,"FN")||
            kw(&L->cur,"REPEAT")||kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")||kw(&L->cur,"CASE"))
          depth++;
        else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
        lex_next(L);
      }
      if (kw(&L->cur,"END")) lex_next(L);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"ELSE")){
      lex_next(L); skip_nl(L);
      Lex body=*L;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      /* advance outer L from ELSE-body start to matching END */
      depth=1;
      while (L->cur.kind!=TK_EOF && depth>0){
        if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||
            kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FORCELL")||
            kw(&L->cur,"EACHCELL")||kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||kw(&L->cur,"FN")||
            kw(&L->cur,"REPEAT")||kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")||kw(&L->cur,"CASE"))
          depth++;
        else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
        lex_next(L);
      }
      if (kw(&L->cur,"END")) lex_next(L);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"END")){ lex_next(L); bump(vm); return 1; }
    fail(vm,"UNLESS chain broken"); return -1;
  }
  /* digit-1 control: FOREVER / LOOPINF / INFINITE ... END — unbounded until BREAK */
  if (kw(&L->cur,"FOREVER")||kw(&L->cur,"LOOPINF")||kw(&L->cur,"INFINITE")||
      kw(&L->cur,"LOOPFOREVER")){
    lex_next(L);
    skip_nl(L);
    Lex save=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FOREVER without END"); return -1; }
    long guard=0;
    for (; !vm->fatal && !vm->halt && guard++<100000;){
      long *it=var_slot(vm,"IT",1); if (it) *it=guard-1;
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=save;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"LOOP")||kw(&L->cur,"TIMES")){
    lex_next(L);
    long times=parse_expr(vm,L);
    if (times<0) times=0;
    if (times>100000) times=100000;
    skip_nl(L);
    Lex save=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"LOOP without END"); return -1; }
    for (long t=0;t<times && !vm->fatal && !vm->halt;t++){
      long *it=var_slot(vm,"IT",1); if (it) *it=t;
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=save;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      /* continue_loop: already stopped body via exec_stmts_until */
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  /* SLOOP — stack count: pop TOS as times, same body semantics as LOOP */
  if (kw(&L->cur,"SLOOP")||kw(&L->cur,"STACKLOOP")){
    lex_next(L);
    long times = 0;
    if (vm->sp >= 1){
      times = vm->stack[--vm->sp];
      var_set_num(vm,"SP",vm->sp);
      var_set_num(vm,"OK",1);
    } else {
      var_set_num(vm,"OK",0);
    }
    if (times<0) times=0;
    if (times>100000) times=100000;
    skip_nl(L);
    Lex save=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"SLOOP without END"); return -1; }
    for (long t=0;t<times && !vm->fatal && !vm->halt;t++){
      long *it=var_slot(vm,"IT",1); if (it) *it=t;
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=save;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  /* REPEAT ... UNTIL cond — post-test loop (digit-4 universal control) */
  if (kw(&L->cur,"REPEAT")){
    lex_next(L);
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 1)) break;
    }
    if (!(kw(&L->cur,"UNTIL") || kw(&L->cur,"END"))){ fail(vm,"REPEAT without UNTIL|END"); return -1; }
    int use_until = kw(&L->cur,"UNTIL") ? 1 : 0;
    if (use_until){
      lex_next(L);
      Lex cond_start=*L;
      (void)parse_expr(vm,L); /* advance over cond for outer scan */
      Lex after_cond=*L;
      long guard=0;
      do {
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"UNTIL",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
        Lex clex=cond_start;
        long done = parse_expr(vm,&clex);
        if (done) break;
      } while (!vm->fatal && !vm->halt && guard++<100000);
      *L=after_cond;
    } else {
      /* REPEAT ... END  (same as LOOP 1..∞ with break only — run once as block) */
      if (kw(&L->cur,"END")) lex_next(L);
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      vm->break_loop=0; vm->continue_loop=0;
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"WHILE")){
    lex_next(L);
    Lex cond_start=*L;
    long cond=parse_expr(vm,L);
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"WHILE without END"); return -1; }
    Lex end_tok=*L;
    long guard=0;
    while (cond && !vm->fatal && !vm->halt && guard++<100000){
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
      Lex clex=cond_start;
      cond=parse_expr(vm,&clex);
    }
    *L=end_tok;
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  /* UNTIL cond ... END — pre-test inverse WHILE: run while cond is false (digit-1 loops) */
  if (kw(&L->cur,"UNTIL")){
    lex_next(L);
    Lex cond_start=*L;
    long cond=parse_expr(vm,L);
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"UNTIL without END"); return -1; }
    Lex end_tok=*L;
    long guard=0;
    while (!cond && !vm->fatal && !vm->halt && guard++<100000){
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
      Lex clex=cond_start;
      cond=parse_expr(vm,&clex);
    }
    *L=end_tok;
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"IF")){
    lex_next(L);
    /* chain: IF c THEN ... ELIF c THEN ... ELSE ... END */
    for(;;){
      long cond=parse_expr(vm,L);
      if (!kw(&L->cur,"THEN")){ fail(vm,"IF expr THEN"); return -1; }
      lex_next(L); skip_nl(L);
      Lex body_start=*L;
      int depth=1;
      while (L->cur.kind!=TK_EOF && depth>0){
        if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
          lex_next(L);
          if (kw(&L->cur,"IF")) lex_next(L);
          continue;
        }
        if (kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||kw(&L->cur,"WHILE")||
            kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")||
            kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")) depth++;
        else if ((kw(&L->cur,"ELSE")||kw(&L->cur,"ELIF")||kw(&L->cur,"ELSEIF")) && depth==1) break;
        else if (kw(&L->cur,"END")){ depth--; if (depth==0) break; }
        lex_next(L);
      }
      if (depth>1){ fail(vm,"IF without END"); return -1; }
      if (cond){
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END","ELSE")<0) return -1;
        /* also stop at ELIF */
        /* skip to final END */
        depth=1;
        while (L->cur.kind!=TK_EOF && depth>0){
          if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
            lex_next(L);
            if (kw(&L->cur,"IF")) lex_next(L);
            continue;
          }
          if (kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||kw(&L->cur,"WHILE")||
              kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")||
              kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")) depth++;
          else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
          lex_next(L);
        }
        if (kw(&L->cur,"END")) lex_next(L);
        bump(vm); return 1;
      }
      /* not taken */
      if (kw(&L->cur,"ELIF")||kw(&L->cur,"ELSEIF")){ lex_next(L); continue; }
      if (kw(&L->cur,"ELSE")){
        lex_next(L); skip_nl(L);
        Lex body=*L;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        /* advance outer L from ELSE-body start to matching END (body is a copy) */
        depth=1;
        while (L->cur.kind!=TK_EOF && depth>0){
          if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
            lex_next(L);
            if (kw(&L->cur,"IF")) lex_next(L);
            continue;
          }
          if (kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||kw(&L->cur,"WHILE")||
              kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")||
              kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")||kw(&L->cur,"UNLESS")||kw(&L->cur,"CASE"))
            depth++;
          else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
          lex_next(L);
        }
        if (kw(&L->cur,"END")) lex_next(L);
        bump(vm); return 1;
      }
      if (kw(&L->cur,"END")){ lex_next(L); bump(vm); return 1; }
      fail(vm,"IF chain broken"); return -1;
    }
  }
  if (kw(&L->cur,"END")||kw(&L->cur,"ELSE")||kw(&L->cur,"ELIF")||kw(&L->cur,"ELSEIF")||kw(&L->cur,"THEN")){
    return 0; /* stop marker for nested bodies */
  }

  snprintf(vm->err,sizeof vm->err,"unknown form '%s' line %d — place a unit with [name]",
           L->cur.text, L->cur.line);
  fail(vm, vm->err);
  return 0;
}
