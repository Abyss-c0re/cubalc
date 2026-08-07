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

  /* GETFWHERE|COLLECTWHERE|PLUCKWHERE|MAPGETWHERE [Class] getfield matchfield matchvalue
   * — bag of getfield values from live objs where matchfield == matchvalue.
   * Optional AS KV → name:value. Soft skip missing fields.
   * LAST_N = count. WHERE sugar: GETFWHERE Cell energy WHERE ready 1.
   * Usability: filtered GETFALL without WHEREOBJ bag + EACH+GETF. */
  if (kw(&L->cur, "GETFWHERE") || kw(&L->cur, "COLLECTWHERE") ||
      kw(&L->cur, "PLUCKWHERE") || kw(&L->cur, "MAPGETWHERE") ||
      kw(&L->cur, "GETWHERE") || kw(&L->cur, "WHEREGETF") ||
      kw(&L->cur, "HARVESTWHERE") || kw(&L->cur, "FIELDWHERE") ||
      kw(&L->cur, "COLLECTFWHERE") || kw(&L->cur, "GETFALLWHERE")) {
    char filt[48], gfield[48], mfield[48], tok1[48];
    char m_sval[512], bag[4096];
    int has_filt = 0, m_is_str = 0, as_kv = 0, i, n = 0, n_skip = 0;
    long m_nval = 0;
    size_t o = 0;
    lex_next(L);
    filt[0] = 0;
    gfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "GETFWHERE OF Class getfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") &&
          !kw(&L->cur, "AS") && !kw(&L->cur, "WITH") && !kw(&L->cur, "KV")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(gfield, sizeof gfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(gfield, sizeof gfield, "%s", vv->sval);
          else
            snprintf(gfield, sizeof gfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(gfield, sizeof gfield, "%s", vv->sval);
        else
          snprintf(gfield, sizeof gfield, "%s", tok1);
      }
    } else {
      fail(vm, "GETFWHERE [Class] getfield matchfield matchvalue");
      return -1;
    }
    if (!gfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(gfield, sizeof gfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "AS")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(gfield, sizeof gfield, "%s", vv->sval);
        else
          snprintf(gfield, sizeof gfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "GETFWHERE [Class] getfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    /* match field */
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "AS") && !kw(&L->cur, "KV")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "GETFWHERE [Class] getfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS") ||
        kw(&L->cur, "=="))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    /* match value */
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "AS") && !kw(&L->cur, "WITH") && !kw(&L->cur, "KV") &&
               !kw(&L->cur, "NAMES")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    /* optional AS KV after match */
    if (kw(&L->cur, "AS") || kw(&L->cur, "WITH")) {
      lex_next(L);
      if (kw(&L->cur, "KV") || kw(&L->cur, "NAMES") || kw(&L->cur, "NAME") ||
          kw(&L->cur, "PAIRS") || kw(&L->cur, "MAP") || kw(&L->cur, "OBJECTS")) {
        as_kv = 1;
        lex_next(L);
      }
    } else if (kw(&L->cur, "KV") || kw(&L->cur, "NAMES") ||
               kw(&L->cur, "ASNAMES") || kw(&L->cur, "PAIRS")) {
      as_kv = 1;
      lex_next(L);
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int gfi, mfi, hit = 0;
      char line[256];
      size_t ln;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      gfi = oop_field_idx(cd, gfield);
      if (mfi < 0 || gfi < 0) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      if (as_kv) {
        if (ob->fis_str[gfi])
          snprintf(line, sizeof line, "%s:%s", ob->name, ob->fstr[gfi]);
        else
          snprintf(line, sizeof line, "%s:%ld", ob->name, ob->fnum[gfi]);
      } else {
        if (ob->fis_str[gfi])
          snprintf(line, sizeof line, "%s", ob->fstr[gfi]);
        else
          snprintf(line, sizeof line, "%ld", ob->fnum[gfi]);
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
    var_set_str(vm, "GETFWHERE", bag);
    var_set_str(vm, "COLLECTWHERE", bag);
    var_set_str(vm, "GETWHERE", bag);
    var_set_str(vm, "PLUCKWHERE", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "GETFWHERE_N", n);
    var_set_num(vm, "COLLECTWHERE_N", n);
    var_set_num(vm, "GETWHERE_N", n);
    var_set_num(vm, "PLUCKWHERE_N", n);
    var_set_num(vm, "GETFWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", gfield);
    var_set_str(vm, "SRC", mfield);
    var_set_str(vm, "DST", gfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
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

  /* SETFWHERE|WHERESETF|PUTWHERE|MAPSETWHERE [Class] matchfield matchvalue setfield setvalue
   * — write setfield=setvalue on live objs where matchfield == matchvalue.
   * Optional SET/TO before setfield. Soft skip missing fields.
   * LAST_N = update count. Usability: WHEREOBJ + SETF without EACH+GETF+IF.
   * Same-field rewrite: SETFWHERE Ticket status "open" status "done". */
  if (kw(&L->cur, "SETFWHERE") || kw(&L->cur, "WHERESETF") ||
      kw(&L->cur, "PUTWHERE") || kw(&L->cur, "MAPSETWHERE") ||
      kw(&L->cur, "SETWHERE") || kw(&L->cur, "ASSIGNWHERE") ||
      kw(&L->cur, "UPDATEWHERE") || kw(&L->cur, "WRITEWHERE") ||
      kw(&L->cur, "SETF_WHERE") || kw(&L->cur, "WHERESET")) {
    char filt[48], mfield[48], sfield[48], tok1[48];
    char m_sval[512], s_sval[512];
    int has_filt = 0, m_is_str = 0, s_is_str = 0, i, n = 0, n_skip = 0;
    long m_nval = 0, s_nval = 0;
    lex_next(L);
    filt[0] = 0;
    mfield[0] = 0;
    sfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    s_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SETFWHERE OF Class matchfield matchvalue setfield setvalue");
        return -1;
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
          snprintf(mfield, sizeof mfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(mfield, sizeof mfield, "%s", vv->sval);
          else
            snprintf(mfield, sizeof mfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(mfield, sizeof mfield, "%s", vv->sval);
        else
          snprintf(mfield, sizeof mfield, "%s", tok1);
      }
    } else {
      fail(vm, "SETFWHERE [Class] matchfield matchvalue setfield setvalue");
      return -1;
    }
    if (!mfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(mfield, sizeof mfield, "%s", vv->sval);
        else
          snprintf(mfield, sizeof mfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SETFWHERE [Class] matchfield matchvalue setfield setvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    /* match value */
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "SET") && !kw(&L->cur, "TO") &&
               !kw(&L->cur, "PUT") && !kw(&L->cur, "AS")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      /* If next looks like setfield, this bare ident is match value only when
       * it's a known string var; else try parse_expr for numbers. If next is
       * SET/TO keyword, same. Heuristic: if pure string var use it; else num. */
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    if (kw(&L->cur, "SET") || kw(&L->cur, "TO") || kw(&L->cur, "PUT") ||
        kw(&L->cur, "AS") || kw(&L->cur, "WRITE"))
      lex_next(L);
    /* setfield */
    if (L->cur.kind == TK_STR) {
      snprintf(sfield, sizeof sfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(sfield, sizeof sfield, "%s", vv->sval);
      else
        snprintf(sfield, sizeof sfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "SETFWHERE [Class] matchfield matchvalue setfield setvalue");
      return -1;
    }
    if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    /* set value */
    if (L->cur.kind == TK_STR) {
      snprintf(s_sval, sizeof s_sval, "%s", L->cur.text);
      s_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(s_sval, sizeof s_sval, "%s", vm->last_str);
      s_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(s_sval, sizeof s_sval, "%s", sv->sval);
        s_is_str = 1;
        lex_next(L);
      } else {
        s_nval = parse_expr(vm, L);
        s_is_str = 0;
      }
    } else {
      s_nval = parse_expr(vm, L);
      s_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int mfi, sfi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      sfi = oop_field_idx(cd, sfield);
      if (mfi < 0 || sfi < 0) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      if (s_is_str) {
        snprintf(ob->fstr[sfi], sizeof ob->fstr[sfi], "%s", s_sval);
        ob->fis_str[sfi] = 1;
      } else {
        ob->fnum[sfi] = s_nval;
        ob->fis_str[sfi] = 0;
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
    var_set_num(vm, "SETFWHERE_N", n);
    var_set_num(vm, "WHERESETF_N", n);
    var_set_num(vm, "PUTWHERE_N", n);
    var_set_num(vm, "SETFWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", mfield);
    var_set_str(vm, "SRC", mfield);
    var_set_str(vm, "DST", sfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* INCFWHERE|ADDWHERE|BUMPWHERE|DECFWHERE [Class] matchfield matchvalue targetfield [delta]
   * — add delta (default +1; DECFWHERE default −1) to numeric targetfield on objs
   * where matchfield == matchvalue. Soft skip missing/string target.
   * BY sugar before delta. LAST_N = update count.
   * Usability: selective counter/retry bump without EACH+GETF+IF+arith+SETF. */
  if (kw(&L->cur, "INCFWHERE") || kw(&L->cur, "ADDWHERE") ||
      kw(&L->cur, "BUMPWHERE") || kw(&L->cur, "DECFWHERE") ||
      kw(&L->cur, "SUBWHERE") || kw(&L->cur, "INCALLWHERE") ||
      kw(&L->cur, "ADDFIELDWHERE") || kw(&L->cur, "WHEREINC") ||
      kw(&L->cur, "WHEREADD") || kw(&L->cur, "WHEREBUMP") ||
      kw(&L->cur, "WHEREDEC") || kw(&L->cur, "DECWHERE")) {
    char filt[48], mfield[48], tfield[48], tok1[48], op[24];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, i, n = 0, n_skip = 0, dec_mode = 0;
    long m_nval = 0, delta = 1;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "DECFWHERE") == 0 || strcmp(op, "SUBWHERE") == 0 ||
        strcmp(op, "WHEREDEC") == 0 || strcmp(op, "DECWHERE") == 0) {
      dec_mode = 1;
      delta = -1;
    }
    lex_next(L);
    filt[0] = 0;
    mfield[0] = 0;
    tfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "INCFWHERE OF Class matchfield matchvalue target [delta]");
        return -1;
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
          snprintf(mfield, sizeof mfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(mfield, sizeof mfield, "%s", vv->sval);
          else
            snprintf(mfield, sizeof mfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(mfield, sizeof mfield, "%s", vv->sval);
        else
          snprintf(mfield, sizeof mfield, "%s", tok1);
      }
    } else {
      fail(vm, "INCFWHERE [Class] matchfield matchvalue target [delta]");
      return -1;
    }
    if (!mfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(mfield, sizeof mfield, "%s", vv->sval);
        else
          snprintf(mfield, sizeof mfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "INCFWHERE [Class] matchfield matchvalue target [delta]");
        return -1;
      }
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    /* match value */
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "BY") && !kw(&L->cur, "SET") &&
               !kw(&L->cur, "ADD") && !kw(&L->cur, "INC")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    if (kw(&L->cur, "SET") || kw(&L->cur, "ADD") || kw(&L->cur, "INC") ||
        kw(&L->cur, "ON") || kw(&L->cur, "FIELD"))
      lex_next(L);
    /* target field */
    if (L->cur.kind == TK_STR) {
      snprintf(tfield, sizeof tfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "BY")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(tfield, sizeof tfield, "%s", vv->sval);
      else
        snprintf(tfield, sizeof tfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "INCFWHERE [Class] matchfield matchvalue target [delta]");
      return -1;
    }
    if (kw(&L->cur, "BY") || kw(&L->cur, "DELTA") || kw(&L->cur, "PLUS"))
      lex_next(L);
    /* optional delta */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
         !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
         !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
         !kw(&L->cur, "END") && !kw(&L->cur, "NEW") &&
         !kw(&L->cur, "CLASS") && !kw(&L->cur, "GETF") &&
         !kw(&L->cur, "SETF") && !kw(&L->cur, "INCFWHERE") &&
         !kw(&L->cur, "SETFWHERE") && !kw(&L->cur, "COPYF"))) {
      long d = parse_expr(vm, L);
      if (dec_mode && d > 0)
        delta = -d;
      else if (dec_mode && d < 0)
        delta = d; /* already negative */
      else
        delta = d;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int mfi, tfi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      tfi = oop_field_idx(cd, tfield);
      if (mfi < 0 || tfi < 0) { n_skip++; continue; }
      if (ob->fis_str[tfi]) { n_skip++; continue; } /* target numeric only */
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      ob->fnum[tfi] += delta;
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
    var_set_num(vm, "INCFWHERE_N", n);
    var_set_num(vm, "ADDWHERE_N", n);
    var_set_num(vm, "BUMPWHERE_N", n);
    var_set_num(vm, "DECFWHERE_N", n);
    var_set_num(vm, "WHEREINC_N", n);
    var_set_num(vm, "INCFWHERE_SKIP", n_skip);
    var_set_num(vm, "INCFWHERE_DELTA", delta);
    var_set_str(vm, "FIELD", tfield);
    var_set_str(vm, "SRC", mfield);
    var_set_str(vm, "DST", tfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
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

  /* CLAMPWHERE|CLIPWHERE|BOUNDWHERE [Class] valfield matchfield matchvalue lo [TO] hi
   * — clamp numeric valfield into [lo,hi] where matchfield == matchvalue.
   * WHERE sugar: CLAMPWHERE Cell energy WHERE age 1 0 TO 100.
   * LAST_N = matched objs with numeric field; CLAMPWHERE_CHANGED = values moved.
   * Soft empty → 0. Usability: selective clamp without EACH+GETF+IF+SETF. */
  if (kw(&L->cur, "CLAMPWHERE") || kw(&L->cur, "CLIPWHERE") ||
      kw(&L->cur, "BOUNDWHERE") || kw(&L->cur, "CLAMPFWHERE") ||
      kw(&L->cur, "WHERECLAMP") || kw(&L->cur, "CLAMPIF") ||
      kw(&L->cur, "CLIPIF") || kw(&L->cur, "BOUNDIF") ||
      kw(&L->cur, "LIMITWHERE") || kw(&L->cur, "SATWHERE")) {
    char filt[48], vfield[48], mfield[48], tok1[48];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, i, n = 0, n_skip = 0, n_chg = 0;
    long m_nval = 0, lo = 0, hi = 0, t;
    lex_next(L);
    filt[0] = 0;
    vfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "CLAMPWHERE OF Class valfield matchfield matchvalue lo hi");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(vfield, sizeof vfield, "%s", vv->sval);
          else
            snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", tok1);
      }
    } else {
      fail(vm, "CLAMPWHERE [Class] valfield matchfield matchvalue lo hi");
      return -1;
    }
    if (!vfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "CLAMPWHERE [Class] valfield matchfield matchvalue lo hi");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "CLAMPWHERE [Class] valfield matchfield matchvalue lo hi");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    /* optional glue before lo */
    if (kw(&L->cur, "IN") || kw(&L->cur, "BETWEEN") || kw(&L->cur, "RANGE") ||
        kw(&L->cur, "TO") || kw(&L->cur, "CLAMP"))
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
      int vfi, mfi, hit = 0;
      long v, nv;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      vfi = oop_field_idx(cd, vfield);
      if (mfi < 0 || vfi < 0) { n_skip++; continue; }
      if (ob->fis_str[vfi]) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      v = ob->fnum[vfi];
      nv = v;
      if (nv < lo) nv = lo;
      if (nv > hi) nv = hi;
      if (nv != v) {
        ob->fnum[vfi] = nv;
        n_chg++;
      }
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "CLAMPWHERE_N", n);
    var_set_num(vm, "CLIPWHERE_N", n);
    var_set_num(vm, "BOUNDWHERE_N", n);
    var_set_num(vm, "CLAMPFWHERE_N", n);
    var_set_num(vm, "CLAMPWHERE_SKIP", n_skip);
    var_set_num(vm, "CLAMPWHERE_CHANGED", n_chg);
    var_set_num(vm, "CLAMPWHERE_LO", lo);
    var_set_num(vm, "CLAMPWHERE_HI", hi);
    var_set_str(vm, "FIELD", vfield);
    var_set_str(vm, "SRC", mfield);
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

  /* MULWHERE|SCALEWHERE|MULFWHERE [Class] valfield matchfield matchvalue factor
   * — multiply numeric valfield by factor where matchfield == matchvalue.
   * WHERE sugar: MULWHERE Cell energy WHERE age 1 BY 2.
   * LAST_N = matched updates; MULWHERE_FACTOR = factor; soft empty → 0.
   * Usability: selective scale without EACH+GETF+IF+SETF (pairs with CLAMPWHERE). */
  if (kw(&L->cur, "MULWHERE") || kw(&L->cur, "SCALEWHERE") ||
      kw(&L->cur, "MULFWHERE") || kw(&L->cur, "WHEREMUL") ||
      kw(&L->cur, "MULIF") || kw(&L->cur, "SCALEIF") ||
      kw(&L->cur, "TIMESWHERE") || kw(&L->cur, "WHERESCALE") ||
      kw(&L->cur, "PRODUCTWHERE") || kw(&L->cur, "SCALEFWHERE")) {
    char filt[48], vfield[48], mfield[48], tok1[48];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, i, n = 0, n_skip = 0;
    long m_nval = 0, factor = 1;
    lex_next(L);
    filt[0] = 0;
    vfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "MULWHERE OF Class valfield matchfield matchvalue factor");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(vfield, sizeof vfield, "%s", vv->sval);
          else
            snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", tok1);
      }
    } else {
      fail(vm, "MULWHERE [Class] valfield matchfield matchvalue factor");
      return -1;
    }
    if (!vfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "MULWHERE [Class] valfield matchfield matchvalue factor");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "MULWHERE [Class] valfield matchfield matchvalue factor");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    /* factor: BY n | TIMES n | bare */
    if (kw(&L->cur, "BY") || kw(&L->cur, "TIMES") || kw(&L->cur, "FACTOR") ||
        kw(&L->cur, "MUL") || kw(&L->cur, "SCALE") || kw(&L->cur, "WITH"))
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
               !kw(&L->cur, "GETF") && !kw(&L->cur, "SETF")) {
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
      fail(vm, "MULWHERE [Class] valfield matchfield matchvalue factor");
      return -1;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int vfi, mfi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      vfi = oop_field_idx(cd, vfield);
      if (mfi < 0 || vfi < 0) { n_skip++; continue; }
      if (ob->fis_str[vfi]) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      ob->fnum[vfi] *= factor;
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "MULWHERE_N", n);
    var_set_num(vm, "SCALEWHERE_N", n);
    var_set_num(vm, "MULFWHERE_N", n);
    var_set_num(vm, "WHEREMUL_N", n);
    var_set_num(vm, "MULWHERE_SKIP", n_skip);
    var_set_num(vm, "MULWHERE_FACTOR", factor);
    var_set_num(vm, "SCALEWHERE_FACTOR", factor);
    var_set_str(vm, "FIELD", vfield);
    var_set_str(vm, "SRC", mfield);
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

  /* COPYFWHERE|SNAPSHOTWHERE|BACKUPWHERE [Class] src [TO] dst matchfield matchvalue
   * — copy src→dst field where matchfield == matchvalue.
   * WHERE sugar: COPYFWHERE Cell energy TO prev WHERE age 1.
   * LAST_N = copy count; soft empty → 0.
   * Usability: selective snapshot without EACH+GETF+IF+SETF. */
  if (kw(&L->cur, "COPYFWHERE") || kw(&L->cur, "SNAPSHOTWHERE") ||
      kw(&L->cur, "BACKUPWHERE") || kw(&L->cur, "WHERECOPYF") ||
      kw(&L->cur, "COPYIF") || kw(&L->cur, "SNAPSHOTIF") ||
      kw(&L->cur, "BACKUPIF") || kw(&L->cur, "DUPWHERE") ||
      kw(&L->cur, "CLONEWHERE") || kw(&L->cur, "FIELDCOPYWHERE")) {
    char filt[48], srcf[48], dstf[48], mfield[48], tok1[48];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, i, n = 0, n_skip = 0;
    long m_nval = 0;
    lex_next(L);
    filt[0] = 0;
    srcf[0] = 0;
    dstf[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "COPYFWHERE OF Class src dst matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "TO") && !kw(&L->cur, "INTO") && !kw(&L->cur, "AS") &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !oop_stmt_kw(L)) {
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
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "TO") || kw(&L->cur, "WHERE")))
          snprintf(srcf, sizeof srcf, "%s", vv->sval);
        else
          snprintf(srcf, sizeof srcf, "%s", tok1);
      }
    } else {
      fail(vm, "COPYFWHERE [Class] src dst matchfield matchvalue");
      return -1;
    }
    if (!srcf[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(srcf, sizeof srcf, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "TO") && !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(srcf, sizeof srcf, "%s", vv->sval);
        else
          snprintf(srcf, sizeof srcf, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "COPYFWHERE [Class] src dst matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "INTO") || kw(&L->cur, "AS") ||
        kw(&L->cur, "->") || (L->cur.kind == TK_EQ))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(dstf, sizeof dstf, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(dstf, sizeof dstf, "%s", vv->sval);
      else
        snprintf(dstf, sizeof dstf, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "COPYFWHERE [Class] src dst matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "COPYFWHERE [Class] src dst matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int si, di, mfi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      si = oop_field_idx(cd, srcf);
      di = oop_field_idx(cd, dstf);
      if (mfi < 0 || si < 0 || di < 0) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
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
    var_set_num(vm, "COPYFWHERE_N", n);
    var_set_num(vm, "SNAPSHOTWHERE_N", n);
    var_set_num(vm, "BACKUPWHERE_N", n);
    var_set_num(vm, "WHERECOPYF_N", n);
    var_set_num(vm, "COPYFWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", srcf);
    var_set_str(vm, "SRC", srcf);
    var_set_str(vm, "DST", dstf);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SWAPFWHERE|EXCHANGEWHERE|XCHFWHERE [Class] a [WITH] b matchfield matchvalue
   * — exchange fields a and b where matchfield == matchvalue.
   * WHERE sugar: SWAPFWHERE Cell energy WITH age WHERE ready 1.
   * LAST_N = swap count; soft empty → 0.
   * Usability: selective dual-buffer rotate without EACH+GETF+IF+temp. */
  /* Note: SWAPIF is stack-plane (lang_ops_stack) — do not alias here. */
  if (kw(&L->cur, "SWAPFWHERE") || kw(&L->cur, "EXCHANGEWHERE") ||
      kw(&L->cur, "XCHFWHERE") || kw(&L->cur, "WHERESWAPF") ||
      kw(&L->cur, "EXCHANGEIF") || kw(&L->cur, "FLIPWHERE") ||
      kw(&L->cur, "FIELDSWAPWHERE") || kw(&L->cur, "WHEREEXCHANGE") ||
      kw(&L->cur, "XCHGIF") || kw(&L->cur, "SWAPFIELDWHERE")) {
    char filt[48], af[48], bf[48], mfield[48], tok1[48];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, i, n = 0, n_skip = 0;
    long m_nval = 0;
    lex_next(L);
    filt[0] = 0;
    af[0] = 0;
    bf[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SWAPFWHERE OF Class a b matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WITH") && !kw(&L->cur, "AND") && !kw(&L->cur, "TO") &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !oop_stmt_kw(L)) {
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
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WITH") || kw(&L->cur, "WHERE")))
          snprintf(af, sizeof af, "%s", vv->sval);
        else
          snprintf(af, sizeof af, "%s", tok1);
      }
    } else {
      fail(vm, "SWAPFWHERE [Class] a b matchfield matchvalue");
      return -1;
    }
    if (!af[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(af, sizeof af, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WITH") && !kw(&L->cur, "AND") &&
                 !kw(&L->cur, "TO") && !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(af, sizeof af, "%s", vv->sval);
        else
          snprintf(af, sizeof af, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SWAPFWHERE [Class] a b matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WITH") || kw(&L->cur, "AND") || kw(&L->cur, "TO") ||
        (L->cur.kind == TK_IDENT && strcmp(L->cur.text, ",") == 0))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(bf, sizeof bf, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(bf, sizeof bf, "%s", vv->sval);
      else
        snprintf(bf, sizeof bf, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "SWAPFWHERE [Class] a b matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "SWAPFWHERE [Class] a b matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int ia, ib, mfi, hit = 0;
      long tnum;
      char tstr[128];
      int t_is_str;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      ia = oop_field_idx(cd, af);
      ib = oop_field_idx(cd, bf);
      if (mfi < 0 || ia < 0 || ib < 0) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
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
    var_set_num(vm, "SWAPFWHERE_N", n);
    var_set_num(vm, "EXCHANGEWHERE_N", n);
    var_set_num(vm, "XCHFWHERE_N", n);
    var_set_num(vm, "WHERESWAPF_N", n);
    var_set_num(vm, "SWAPFWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", af);
    var_set_str(vm, "SRC", af);
    var_set_str(vm, "DST", bf);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
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

  /* MAXOBJWHERE|MINOBJWHERE|BESTWHERE|WORSTWHERE [Class] valfield matchfield matchvalue
   * — name of first live object with max/min numeric valfield among matchfield==matchvalue.
   * WHERE sugar: MAXOBJWHERE Cell energy WHERE age 1.
   * LAST=name, LAST_N=extreme value, OBJECT=name; soft empty → OK=0 LAST="".
   * Usability: pick extreme in subset without WHEREOBJ+MAXOBJ or EACH+GETF glue. */
  /* Note: HIGHESTWHERE/LOWESTWHERE are MAXWHERE/MINWHERE value extremes — not here. */
  if (kw(&L->cur, "MAXOBJWHERE") || kw(&L->cur, "MINOBJWHERE") ||
      kw(&L->cur, "BESTWHERE") || kw(&L->cur, "WORSTWHERE") ||
      kw(&L->cur, "ARGMAXWHERE") || kw(&L->cur, "ARGMINWHERE") ||
      kw(&L->cur, "TOPOBJWHERE") || kw(&L->cur, "BOTOBJWHERE") ||
      kw(&L->cur, "MAXIFOBJ") || kw(&L->cur, "MINIFOBJ") ||
      kw(&L->cur, "BESTOBJWHERE") || kw(&L->cur, "WORSTOBJWHERE")) {
    char filt[48], vfield[48], mfield[48], tok1[48], op[24], best_name[48];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, want_min = 0, i, n = 0, n_skip = 0, found = 0;
    long m_nval = 0, best = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "MINOBJWHERE") == 0 || strcmp(op, "WORSTWHERE") == 0 ||
        strcmp(op, "ARGMINWHERE") == 0 || strcmp(op, "BOTOBJWHERE") == 0 ||
        strcmp(op, "MINIFOBJ") == 0 || strcmp(op, "WORSTOBJWHERE") == 0)
      want_min = 1;
    lex_next(L);
    filt[0] = 0;
    vfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    best_name[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "MAXOBJWHERE OF Class valfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(vfield, sizeof vfield, "%s", vv->sval);
          else
            snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", tok1);
      }
    } else {
      fail(vm, "MAXOBJWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (!vfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "MAXOBJWHERE [Class] valfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "MAXOBJWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int vfi, mfi, hit = 0;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      vfi = oop_field_idx(cd, vfield);
      if (mfi < 0 || vfi < 0) { n_skip++; continue; }
      if (ob->fis_str[vfi]) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      v = ob->fnum[vfi];
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
      var_set_str(vm, "MAXOBJWHERE", "");
      var_set_str(vm, "MINOBJWHERE", "");
      var_set_str(vm, "OBJECT", "");
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "MAXOBJWHERE_N", 0);
      var_set_num(vm, "MINOBJWHERE_N", 0);
      var_set_num(vm, "MAXOBJWHERE_VAL", 0);
      var_set_num(vm, "MINOBJWHERE_VAL", 0);
      var_set_num(vm, "MAXOBJWHERE_SKIP", n_skip);
      var_set_num(vm, "MAXOBJWHERE_MATCH", 0);
      var_set_str(vm, "FIELD", vfield);
      var_set_str(vm, "SRC", mfield);
      if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "MAXOBJWHERE: no matching objects");
      var_set_str(vm, "ERR", "MAXOBJWHERE: no matching objects");
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", best_name);
    var_set_str(vm, "MAXOBJWHERE", best_name);
    var_set_str(vm, "MINOBJWHERE", best_name);
    var_set_str(vm, "BESTWHERE", best_name);
    var_set_str(vm, "OBJECT", best_name);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", best_name);
    var_set_num(vm, "LAST_N", best);
    vm->last_n = best;
    var_set_num(vm, "MAXOBJWHERE_N", 1);
    var_set_num(vm, "MINOBJWHERE_N", 1);
    var_set_num(vm, "MAXOBJWHERE_VAL", best);
    var_set_num(vm, "MINOBJWHERE_VAL", best);
    var_set_num(vm, "MAXOBJWHERE_MATCH", n);
    var_set_num(vm, "MINOBJWHERE_MATCH", n);
    var_set_num(vm, "MAXOBJWHERE_SKIP", n_skip);
    var_set_num(vm, "NOBJS", n);
    var_set_str(vm, "FIELD", vfield);
    var_set_str(vm, "SRC", mfield);
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

  /* SORTBYFWHERE|ORDERWHERE|RANKWHERE [Class] sortfield matchfield matchvalue [ASC|DESC]
   * — bag of matching object names ordered by numeric sortfield.
   * WHERE sugar: SORTBYFWHERE Cell energy WHERE age 1 DESC.
   * Soft empty → "" / 0. Usability: ranked subset without WHEREOBJ+SORTBYF glue. */
  if (kw(&L->cur, "SORTBYFWHERE") || kw(&L->cur, "ORDERWHERE") ||
      kw(&L->cur, "RANKWHERE") || kw(&L->cur, "WHERESORTBYF") ||
      kw(&L->cur, "SORTOBJSWHERE") || kw(&L->cur, "ORDERBYFWHERE") ||
      kw(&L->cur, "RANKBYFWHERE") || kw(&L->cur, "SORTIF") ||
      kw(&L->cur, "ORDERIF") || kw(&L->cur, "RANKIF")) {
    char filt[48], sfield[48], mfield[48], tok1[48], bag[4096];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, desc = 0, i, n = 0, n_skip = 0;
    long m_nval = 0;
    size_t o = 0;
    typedef struct { char name[48]; long v; int idx; } SortByFWRow;
    SortByFWRow rows[CUBALC_MAX_OBJS];
    SortByFWRow key;
    lex_next(L);
    filt[0] = 0;
    sfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SORTBYFWHERE OF Class sortfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN") &&
          !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(sfield, sizeof sfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(sfield, sizeof sfield, "%s", vv->sval);
          else
            snprintf(sfield, sizeof sfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", tok1);
      }
    } else {
      fail(vm, "SORTBYFWHERE [Class] sortfield matchfield matchvalue");
      return -1;
    }
    if (!sfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") &&
                 !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SORTBYFWHERE [Class] sortfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS") &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "SORTBYFWHERE [Class] sortfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
               !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
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
      int sfi, mfi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      sfi = oop_field_idx(cd, sfield);
      if (mfi < 0 || sfi < 0) { n_skip++; continue; }
      if (ob->fis_str[sfi]) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      snprintf(rows[n].name, sizeof rows[n].name, "%s", ob->name);
      rows[n].v = ob->fnum[sfi];
      rows[n].idx = n;
      n++;
    }
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
    var_set_str(vm, "SORTBYFWHERE", bag);
    var_set_str(vm, "ORDERWHERE", bag);
    var_set_str(vm, "RANKWHERE", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "SORTBYFWHERE_N", n);
    var_set_num(vm, "ORDERWHERE_N", n);
    var_set_num(vm, "RANKWHERE_N", n);
    var_set_num(vm, "SORTBYFWHERE_SKIP", n_skip);
    var_set_num(vm, "SORTBYFWHERE_DESC", desc);
    var_set_str(vm, "FIELD", sfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* TOPNBYF|HEADBYF|TAKEBYF [Class] field n [ASC|DESC]
   * — first n live object names ordered by numeric field (stable ties).
   * Soft always; string/missing fields skipped. n<=0 → empty bag.
   * LAST = name bag; LAST_N/TOPNBYF_N = returned count; TOPNBYF_TOTAL = ranked pool.
   * Usability: top-k fleet without SORTBYF + SYS TAKE glue (peer pick, leaderboard). */
  if (kw(&L->cur, "TOPNBYF") || kw(&L->cur, "HEADBYF") ||
      kw(&L->cur, "TAKEBYF") || kw(&L->cur, "FIRSTNBYF") ||
      kw(&L->cur, "LIMITBYF") || kw(&L->cur, "TOPNOBJS") ||
      kw(&L->cur, "HEADOBJS") || kw(&L->cur, "TAKEOBJS") ||
      kw(&L->cur, "TOPNFIELD") || kw(&L->cur, "HEADNBYF")) {
    char filt[48], fname[48], tok1[48], bag[4096];
    int has_filt = 0, desc = 0, i, n = 0, n_skip = 0, take = 0, out_n = 0;
    long narg = 0;
    size_t o = 0;
    typedef struct { char name[48]; long v; int idx; } TopNByFRow;
    TopNByFRow rows[CUBALC_MAX_OBJS];
    TopNByFRow key;
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
          fail(vm, "TOPNBYF OF Class field n [ASC|DESC]"); return -1;
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
          !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
          L->cur.kind != TK_NUM && L->cur.kind != TK_MINUS &&
          L->cur.kind != TK_LPAREN) {
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
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
              !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC"))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "TOPNBYF [Class] field n [ASC|DESC]"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
                 L->cur.kind != TK_NUM) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "TOPNBYF [Class] field n [ASC|DESC]"); return -1;
      }
    }
    /* optional sugar: TOP n | FIRST n | LIMIT n | TAKE n */
    if (kw(&L->cur, "TOP") || kw(&L->cur, "FIRST") || kw(&L->cur, "LIMIT") ||
        kw(&L->cur, "TAKE") || kw(&L->cur, "N") || kw(&L->cur, "COUNT") ||
        kw(&L->cur, "HEAD"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      narg = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      narg = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
               !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        narg = atol(sv->sval);
      else
        narg = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      narg = parse_expr(vm, L);
    } else {
      fail(vm, "TOPNBYF [Class] field n [ASC|DESC]"); return -1;
    }
    if (narg < 0) narg = 0;
    take = (int)narg;
    if (take > CUBALC_MAX_OBJS) take = CUBALC_MAX_OBJS;
    if (kw(&L->cur, "DESC") || kw(&L->cur, "DOWN") || kw(&L->cur, "REV") ||
        kw(&L->cur, "REVERSE") || kw(&L->cur, "HIGHFIRST") ||
        kw(&L->cur, "TOP") || kw(&L->cur, "BEST")) {
      desc = 1;
      lex_next(L);
    } else if (kw(&L->cur, "ASC") || kw(&L->cur, "UP") ||
               kw(&L->cur, "LOWFIRST") || kw(&L->cur, "WORST") ||
               kw(&L->cur, "BOTTOM")) {
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
    out_n = n < take ? n : take;
    for (i = 0; i < out_n; i++) {
      size_t ln = strlen(rows[i].name);
      if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, rows[i].name, ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "TOPNBYF", bag);
    var_set_str(vm, "HEADBYF", bag);
    var_set_str(vm, "TAKEBYF", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = out_n;
    var_set_num(vm, "LAST_N", out_n);
    var_set_num(vm, "TOPNBYF_N", out_n);
    var_set_num(vm, "HEADBYF_N", out_n);
    var_set_num(vm, "TAKEBYF_N", out_n);
    var_set_num(vm, "TOPNBYF_TOTAL", n);
    var_set_num(vm, "TOPNBYF_REQ", narg);
    var_set_num(vm, "TOPNBYF_SKIP", n_skip);
    var_set_num(vm, "TOPNBYF_DESC", desc);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* TOPNBYFWHERE|HEADWHERE|TAKEWHERE [Class] sortfield n [WHERE] matchfield matchvalue [ASC|DESC]
   * — top-n matching object names ordered by numeric sortfield.
   * WHERE sugar: TOPNBYFWHERE Cell energy 2 WHERE age 1 DESC.
   * Soft empty → "" / 0. Usability: filtered leaderboard without WHERE+TOPNBYF or SORTBYFWHERE+TAKE. */
  if (kw(&L->cur, "TOPNBYFWHERE") || kw(&L->cur, "HEADWHERE") ||
      kw(&L->cur, "TAKEWHERE") || kw(&L->cur, "FIRSTNWHERE") ||
      kw(&L->cur, "LIMITWHERE") || kw(&L->cur, "TOPNIF") ||
      kw(&L->cur, "HEADIF") || kw(&L->cur, "TAKEIF") ||
      kw(&L->cur, "WHERETOPN") || kw(&L->cur, "TOPNOBJSWHERE") ||
      kw(&L->cur, "HEADBYFWHERE") || kw(&L->cur, "TAKEBYFWHERE")) {
    char filt[48], sfield[48], mfield[48], tok1[48], bag[4096];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, desc = 0, i, n = 0, n_skip = 0, take = 0, out_n = 0;
    long m_nval = 0, narg = 0;
    size_t o = 0;
    typedef struct { char name[48]; long v; int idx; } TopNByFWRow;
    TopNByFWRow rows[CUBALC_MAX_OBJS];
    TopNByFWRow key;
    lex_next(L);
    filt[0] = 0;
    sfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "TOPNBYFWHERE OF Class sortfield n matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN") &&
          !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
          L->cur.kind != TK_NUM && L->cur.kind != TK_MINUS &&
          L->cur.kind != TK_LPAREN) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(sfield, sizeof sfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(sfield, sizeof sfield, "%s", vv->sval);
          else
            snprintf(sfield, sizeof sfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_IDENT ||
             L->cur.kind == TK_STR || kw(&L->cur, "WHERE") ||
             kw(&L->cur, "TOP") || kw(&L->cur, "TAKE") || kw(&L->cur, "N")))
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", tok1);
      }
    } else {
      fail(vm, "TOPNBYFWHERE [Class] sortfield n matchfield matchvalue");
      return -1;
    }
    if (!sfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") &&
                 !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
                 !kw(&L->cur, "TOP") && !kw(&L->cur, "TAKE") &&
                 !kw(&L->cur, "N") && L->cur.kind != TK_NUM) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "TOPNBYFWHERE [Class] sortfield n matchfield matchvalue");
        return -1;
      }
    }
    /* n: optional TOP|TAKE|FIRST|LIMIT|N sugar then count */
    if (kw(&L->cur, "TOP") || kw(&L->cur, "FIRST") || kw(&L->cur, "LIMIT") ||
        kw(&L->cur, "TAKE") || kw(&L->cur, "N") || kw(&L->cur, "COUNT") ||
        kw(&L->cur, "HEAD"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      narg = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      narg = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") &&
               !kw(&L->cur, "WHEN") && !kw(&L->cur, "MATCH") &&
               !kw(&L->cur, "ON") && !kw(&L->cur, "ASC") &&
               !kw(&L->cur, "DESC") && !kw(&L->cur, "ASSERT") &&
               !kw(&L->cur, "LET") && !kw(&L->cur, "PRINT") &&
               !kw(&L->cur, "SYS") && !kw(&L->cur, "END") &&
               !kw(&L->cur, "NEW")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        narg = atol(sv->sval);
      else
        narg = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      narg = parse_expr(vm, L);
    } else {
      fail(vm, "TOPNBYFWHERE [Class] sortfield n matchfield matchvalue");
      return -1;
    }
    if (narg < 0) narg = 0;
    take = (int)narg;
    if (take > CUBALC_MAX_OBJS) take = CUBALC_MAX_OBJS;
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS") &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "TOPNBYFWHERE [Class] sortfield n matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
               !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
               !kw(&L->cur, "HIGHFIRST") && !kw(&L->cur, "LOWFIRST") &&
               !kw(&L->cur, "TOP") && !kw(&L->cur, "BEST") &&
               !kw(&L->cur, "BOTTOM") && !kw(&L->cur, "WORST")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    if (kw(&L->cur, "DESC") || kw(&L->cur, "DOWN") || kw(&L->cur, "REV") ||
        kw(&L->cur, "REVERSE") || kw(&L->cur, "HIGHFIRST") ||
        kw(&L->cur, "TOP") || kw(&L->cur, "BEST")) {
      desc = 1;
      lex_next(L);
    } else if (kw(&L->cur, "ASC") || kw(&L->cur, "UP") ||
               kw(&L->cur, "LOWFIRST") || kw(&L->cur, "WORST") ||
               kw(&L->cur, "BOTTOM")) {
      desc = 0;
      lex_next(L);
    }
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int sfi, mfi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      sfi = oop_field_idx(cd, sfield);
      if (mfi < 0 || sfi < 0) { n_skip++; continue; }
      if (ob->fis_str[sfi]) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      snprintf(rows[n].name, sizeof rows[n].name, "%s", ob->name);
      rows[n].v = ob->fnum[sfi];
      rows[n].idx = n;
      n++;
    }
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
    out_n = n < take ? n : take;
    for (i = 0; i < out_n; i++) {
      size_t ln = strlen(rows[i].name);
      if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, rows[i].name, ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "TOPNBYFWHERE", bag);
    var_set_str(vm, "HEADWHERE", bag);
    var_set_str(vm, "TAKEWHERE", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = out_n;
    var_set_num(vm, "LAST_N", out_n);
    var_set_num(vm, "TOPNBYFWHERE_N", out_n);
    var_set_num(vm, "HEADWHERE_N", out_n);
    var_set_num(vm, "TAKEWHERE_N", out_n);
    var_set_num(vm, "TOPNBYFWHERE_TOTAL", n);
    var_set_num(vm, "TOPNBYFWHERE_REQ", narg);
    var_set_num(vm, "TOPNBYFWHERE_SKIP", n_skip);
    var_set_num(vm, "TOPNBYFWHERE_DESC", desc);
    var_set_str(vm, "FIELD", sfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* NTHBYF|ATBYF|INDEXBYF [Class] field index [ASC|DESC]
   * — 0-based rank pick: object name at index in numeric-field order (stable ties).
   * Soft OOB/empty → "" / 0. LAST=name; LAST_N=field value when hit else 0;
   * NTHBYF_TOTAL = ranked pool; NTHBYF_I = requested index; NTHBYF_DESC = 0|1.
   * Usability: 2nd-best / runner-up without SORTBYF + SYS NTH glue. */
  if (kw(&L->cur, "NTHBYF") || kw(&L->cur, "ATBYF") ||
      kw(&L->cur, "INDEXBYF") || kw(&L->cur, "OBJAT") ||
      kw(&L->cur, "NTHOBJ") || kw(&L->cur, "PICKNBYF") ||
      kw(&L->cur, "ATRANK") || kw(&L->cur, "RANKAT") ||
      kw(&L->cur, "ITHBYF") || kw(&L->cur, "KTHBYF")) {
    char filt[48], fname[48], tok1[48], pick[48];
    int has_filt = 0, desc = 0, i, n = 0, n_skip = 0;
    long idx_arg = 0;
    int idx = 0;
    typedef struct { char name[48]; long v; int idx; } NthByFRow;
    NthByFRow rows[CUBALC_MAX_OBJS];
    NthByFRow key;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    pick[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "NTHBYF OF Class field index [ASC|DESC]"); return -1;
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
          !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
          L->cur.kind != TK_NUM && L->cur.kind != TK_MINUS &&
          L->cur.kind != TK_LPAREN) {
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
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
              !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC"))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "NTHBYF [Class] field index [ASC|DESC]"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
                 L->cur.kind != TK_NUM) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "NTHBYF [Class] field index [ASC|DESC]"); return -1;
      }
    }
    /* optional sugar: AT i | INDEX i | RANK i | # i */
    if (kw(&L->cur, "AT") || kw(&L->cur, "INDEX") || kw(&L->cur, "RANK") ||
        kw(&L->cur, "POS") || kw(&L->cur, "OFFSET") || kw(&L->cur, "ITH"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      idx_arg = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      idx_arg = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
               !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        idx_arg = atol(sv->sval);
      else
        idx_arg = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      idx_arg = parse_expr(vm, L);
    } else {
      fail(vm, "NTHBYF [Class] field index [ASC|DESC]"); return -1;
    }
    if (kw(&L->cur, "DESC") || kw(&L->cur, "DOWN") || kw(&L->cur, "REV") ||
        kw(&L->cur, "REVERSE") || kw(&L->cur, "HIGHFIRST") ||
        kw(&L->cur, "TOP") || kw(&L->cur, "BEST")) {
      desc = 1;
      lex_next(L);
    } else if (kw(&L->cur, "ASC") || kw(&L->cur, "UP") ||
               kw(&L->cur, "LOWFIRST") || kw(&L->cur, "WORST") ||
               kw(&L->cur, "BOTTOM")) {
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
    idx = (int)idx_arg;
    if (idx_arg < 0 || idx_arg >= n) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "NTHBYF", "");
      var_set_str(vm, "ATBYF", "");
      var_set_str(vm, "INDEXBYF", "");
      var_set_str(vm, "OBJECT", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "NTHBYF_N", 0);
      var_set_num(vm, "NTHBYF_VAL", 0);
      var_set_num(vm, "NTHBYF_TOTAL", n);
      var_set_num(vm, "NTHBYF_I", idx_arg);
      var_set_num(vm, "NTHBYF_SKIP", n_skip);
      var_set_num(vm, "NTHBYF_DESC", desc);
      var_set_num(vm, "NTHBYF_HIT", 0);
      var_set_str(vm, "FIELD", fname);
      if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    snprintf(pick, sizeof pick, "%s", rows[idx].name);
    var_set_str(vm, "LAST", pick);
    var_set_str(vm, "NTHBYF", pick);
    var_set_str(vm, "ATBYF", pick);
    var_set_str(vm, "INDEXBYF", pick);
    var_set_str(vm, "OBJECT", pick);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
    vm->last_n = rows[idx].v;
    var_set_num(vm, "LAST_N", rows[idx].v);
    var_set_num(vm, "NTHBYF_N", 1);
    var_set_num(vm, "NTHBYF_VAL", rows[idx].v);
    var_set_num(vm, "NTHBYF_TOTAL", n);
    var_set_num(vm, "NTHBYF_I", idx_arg);
    var_set_num(vm, "NTHBYF_SKIP", n_skip);
    var_set_num(vm, "NTHBYF_DESC", desc);
    var_set_num(vm, "NTHBYF_HIT", 1);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* NTHBYFWHERE|ATWHERE|INDEXWHERE [Class] sortfield index [WHERE] matchfield matchvalue [ASC|DESC]
   * — 0-based rank pick among field==value matches (stable ties).
   * WHERE sugar: NTHBYFWHERE Cell energy 1 WHERE age 1 DESC.
   * Soft OOB/empty → "" / 0. LAST=name; LAST_N=val when hit; NTHBYFWHERE_TOTAL=match pool.
   * Usability: kth-best in subset without WHERE+NTHBYF or SORTBYFWHERE+NTH glue. */
  if (kw(&L->cur, "NTHBYFWHERE") || kw(&L->cur, "ATWHERE") ||
      kw(&L->cur, "INDEXWHERE") || kw(&L->cur, "NTHWHERE") ||
      kw(&L->cur, "OBJATWHERE") || kw(&L->cur, "NTHOBJWHERE") ||
      kw(&L->cur, "PICKNWHERE") || kw(&L->cur, "ATRANKWHERE") ||
      kw(&L->cur, "RANKATWHERE") || kw(&L->cur, "ITHWHERE") ||
      kw(&L->cur, "KTHWHERE") || kw(&L->cur, "WHERENTH")) {
    char filt[48], sfield[48], mfield[48], tok1[48], pick[48];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, desc = 0, i, n = 0, n_skip = 0;
    long m_nval = 0, idx_arg = 0;
    int idx = 0;
    typedef struct { char name[48]; long v; int idx; } NthByFWRow;
    NthByFWRow rows[CUBALC_MAX_OBJS];
    NthByFWRow key;
    lex_next(L);
    filt[0] = 0;
    sfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    pick[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "NTHBYFWHERE OF Class sortfield index matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN") &&
          !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
          L->cur.kind != TK_NUM && L->cur.kind != TK_MINUS &&
          L->cur.kind != TK_LPAREN) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(sfield, sizeof sfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(sfield, sizeof sfield, "%s", vv->sval);
          else
            snprintf(sfield, sizeof sfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_IDENT ||
             L->cur.kind == TK_STR || kw(&L->cur, "WHERE") ||
             kw(&L->cur, "AT") || kw(&L->cur, "INDEX") || kw(&L->cur, "RANK")))
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", tok1);
      }
    } else {
      fail(vm, "NTHBYFWHERE [Class] sortfield index matchfield matchvalue");
      return -1;
    }
    if (!sfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") &&
                 !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
                 !kw(&L->cur, "AT") && !kw(&L->cur, "INDEX") &&
                 !kw(&L->cur, "RANK") && L->cur.kind != TK_NUM) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "NTHBYFWHERE [Class] sortfield index matchfield matchvalue");
        return -1;
      }
    }
    /* index: optional AT|INDEX|RANK|POS|OFFSET|ITH (not I/N — steals vars) */
    if (kw(&L->cur, "AT") || kw(&L->cur, "INDEX") || kw(&L->cur, "RANK") ||
        kw(&L->cur, "POS") || kw(&L->cur, "OFFSET") || kw(&L->cur, "ITH"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      idx_arg = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      idx_arg = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") &&
               !kw(&L->cur, "WHEN") && !kw(&L->cur, "MATCH") &&
               !kw(&L->cur, "ON") && !kw(&L->cur, "ASC") &&
               !kw(&L->cur, "DESC") && !kw(&L->cur, "ASSERT") &&
               !kw(&L->cur, "LET") && !kw(&L->cur, "PRINT") &&
               !kw(&L->cur, "SYS") && !kw(&L->cur, "END") &&
               !kw(&L->cur, "NEW")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        idx_arg = atol(sv->sval);
      else
        idx_arg = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      idx_arg = parse_expr(vm, L);
    } else {
      fail(vm, "NTHBYFWHERE [Class] sortfield index matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS") &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "NTHBYFWHERE [Class] sortfield index matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
               !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
               !kw(&L->cur, "HIGHFIRST") && !kw(&L->cur, "LOWFIRST") &&
               !kw(&L->cur, "TOP") && !kw(&L->cur, "BEST") &&
               !kw(&L->cur, "BOTTOM") && !kw(&L->cur, "WORST")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    if (kw(&L->cur, "DESC") || kw(&L->cur, "DOWN") || kw(&L->cur, "REV") ||
        kw(&L->cur, "REVERSE") || kw(&L->cur, "HIGHFIRST") ||
        kw(&L->cur, "TOP") || kw(&L->cur, "BEST")) {
      desc = 1;
      lex_next(L);
    } else if (kw(&L->cur, "ASC") || kw(&L->cur, "UP") ||
               kw(&L->cur, "LOWFIRST") || kw(&L->cur, "WORST") ||
               kw(&L->cur, "BOTTOM")) {
      desc = 0;
      lex_next(L);
    }
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int sfi, mfi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      sfi = oop_field_idx(cd, sfield);
      if (mfi < 0 || sfi < 0) { n_skip++; continue; }
      if (ob->fis_str[sfi]) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      snprintf(rows[n].name, sizeof rows[n].name, "%s", ob->name);
      rows[n].v = ob->fnum[sfi];
      rows[n].idx = n;
      n++;
    }
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
    idx = (int)idx_arg;
    if (idx_arg < 0 || idx_arg >= n) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "NTHBYFWHERE", "");
      var_set_str(vm, "ATWHERE", "");
      var_set_str(vm, "INDEXWHERE", "");
      var_set_str(vm, "OBJECT", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "NTHBYFWHERE_N", 0);
      var_set_num(vm, "NTHBYFWHERE_VAL", 0);
      var_set_num(vm, "NTHBYFWHERE_TOTAL", n);
      var_set_num(vm, "NTHBYFWHERE_I", idx_arg);
      var_set_num(vm, "NTHBYFWHERE_SKIP", n_skip);
      var_set_num(vm, "NTHBYFWHERE_DESC", desc);
      var_set_num(vm, "NTHBYFWHERE_HIT", 0);
      var_set_str(vm, "FIELD", sfield);
      var_set_str(vm, "SRC", mfield);
      if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    snprintf(pick, sizeof pick, "%s", rows[idx].name);
    var_set_str(vm, "LAST", pick);
    var_set_str(vm, "NTHBYFWHERE", pick);
    var_set_str(vm, "ATWHERE", pick);
    var_set_str(vm, "INDEXWHERE", pick);
    var_set_str(vm, "OBJECT", pick);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
    vm->last_n = rows[idx].v;
    var_set_num(vm, "LAST_N", rows[idx].v);
    var_set_num(vm, "NTHBYFWHERE_N", 1);
    var_set_num(vm, "NTHBYFWHERE_VAL", rows[idx].v);
    var_set_num(vm, "NTHBYFWHERE_TOTAL", n);
    var_set_num(vm, "NTHBYFWHERE_I", idx_arg);
    var_set_num(vm, "NTHBYFWHERE_SKIP", n_skip);
    var_set_num(vm, "NTHBYFWHERE_DESC", desc);
    var_set_num(vm, "NTHBYFWHERE_HIT", 1);
    var_set_str(vm, "FIELD", sfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SLICEBYF|MIDBYF|WINDOWBYF [Class] field start count [ASC|DESC]
   * — ranked name bag window [start, start+count) by numeric field (stable ties).
   * Soft always; start OOB or count<=0 → empty. LAST bag; LAST_N returned count.
   * SLICEBYF_TOTAL = ranked pool; SLICEBYF_START / SLICEBYF_REQ / SLICEBYF_DESC.
   * Usability: page/skip ranked fleet without SORTBYF + DROP + TAKE glue. */
  if (kw(&L->cur, "SLICEBYF") || kw(&L->cur, "MIDBYF") ||
      kw(&L->cur, "WINDOWBYF") || kw(&L->cur, "RANKSLICE") ||
      kw(&L->cur, "SLICEOBJS") || kw(&L->cur, "SKIPTAKEBYF") ||
      kw(&L->cur, "PAGEBYF") || kw(&L->cur, "RANGEBYF") ||
      kw(&L->cur, "SUBRANK") || kw(&L->cur, "SLICEFIELD")) {
    char filt[48], fname[48], tok1[48], bag[4096];
    int has_filt = 0, desc = 0, i, n = 0, n_skip = 0, out_n = 0;
    long start_arg = 0, count_arg = 0;
    int start = 0, take = 0, end = 0;
    size_t o = 0;
    typedef struct { char name[48]; long v; int idx; } SliceByFRow;
    SliceByFRow rows[CUBALC_MAX_OBJS];
    SliceByFRow key;
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
          fail(vm, "SLICEBYF OF Class field start count [ASC|DESC]"); return -1;
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
          !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
          L->cur.kind != TK_NUM && L->cur.kind != TK_MINUS &&
          L->cur.kind != TK_LPAREN) {
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
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
              !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC"))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "SLICEBYF [Class] field start count [ASC|DESC]"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
                 L->cur.kind != TK_NUM) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SLICEBYF [Class] field start count [ASC|DESC]"); return -1;
      }
    }
    /* optional FROM|START|OFFSET then start */
    if (kw(&L->cur, "FROM") || kw(&L->cur, "START") || kw(&L->cur, "OFFSET") ||
        kw(&L->cur, "SKIP") || kw(&L->cur, "DROP") || kw(&L->cur, "AT"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      start_arg = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      start_arg = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
               !kw(&L->cur, "FOR") && !kw(&L->cur, "COUNT") &&
               !kw(&L->cur, "TAKE") && !kw(&L->cur, "LIMIT") &&
               !kw(&L->cur, "LEN") && !kw(&L->cur, "SIZE") &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        start_arg = atol(sv->sval);
      else
        start_arg = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      start_arg = parse_expr(vm, L);
    } else {
      fail(vm, "SLICEBYF [Class] field start count [ASC|DESC]"); return -1;
    }
    /* optional FOR|COUNT|TAKE|LIMIT|LEN then count */
    if (kw(&L->cur, "FOR") || kw(&L->cur, "COUNT") || kw(&L->cur, "TAKE") ||
        kw(&L->cur, "LIMIT") || kw(&L->cur, "LEN") || kw(&L->cur, "SIZE"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      count_arg = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      count_arg = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
               !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        count_arg = atol(sv->sval);
      else
        count_arg = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      count_arg = parse_expr(vm, L);
    } else {
      fail(vm, "SLICEBYF [Class] field start count [ASC|DESC]"); return -1;
    }
    if (kw(&L->cur, "DESC") || kw(&L->cur, "DOWN") || kw(&L->cur, "REV") ||
        kw(&L->cur, "REVERSE") || kw(&L->cur, "HIGHFIRST") ||
        kw(&L->cur, "TOP") || kw(&L->cur, "BEST")) {
      desc = 1;
      lex_next(L);
    } else if (kw(&L->cur, "ASC") || kw(&L->cur, "UP") ||
               kw(&L->cur, "LOWFIRST") || kw(&L->cur, "WORST") ||
               kw(&L->cur, "BOTTOM")) {
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
    if (start_arg < 0) start_arg = 0;
    if (count_arg < 0) count_arg = 0;
    start = (int)start_arg;
    take = (int)count_arg;
    if (start > n) start = n;
    end = start + take;
    if (end > n) end = n;
    if (end < start) end = start;
    out_n = end - start;
    for (i = start; i < end; i++) {
      size_t ln = strlen(rows[i].name);
      if (o > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, rows[i].name, ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "SLICEBYF", bag);
    var_set_str(vm, "MIDBYF", bag);
    var_set_str(vm, "WINDOWBYF", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = out_n;
    var_set_num(vm, "LAST_N", out_n);
    var_set_num(vm, "SLICEBYF_N", out_n);
    var_set_num(vm, "MIDBYF_N", out_n);
    var_set_num(vm, "WINDOWBYF_N", out_n);
    var_set_num(vm, "SLICEBYF_TOTAL", n);
    var_set_num(vm, "SLICEBYF_START", start_arg);
    var_set_num(vm, "SLICEBYF_REQ", count_arg);
    var_set_num(vm, "SLICEBYF_SKIP", n_skip);
    var_set_num(vm, "SLICEBYF_DESC", desc);
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

  /* SUMWHERE|TOTALWHERE|SUMFWHERE|AVGWHERE|MEANWHERE [Class] sumfield matchfield matchvalue
   * — sum or integer mean of numeric sumfield where matchfield == matchvalue.
   * WHERE sugar: SUMWHERE Cell energy WHERE age 1.
   * LAST_N = result; SUMWHERE_N = sample count; soft empty → 0.
   * Usability: filtered SUMF without GETFWHERE+SYS SUM glue. */
  if (kw(&L->cur, "SUMWHERE") || kw(&L->cur, "TOTALWHERE") ||
      kw(&L->cur, "SUMFWHERE") || kw(&L->cur, "WHERESUM") ||
      kw(&L->cur, "SUMIF") || kw(&L->cur, "FIELDSUMWHERE") ||
      kw(&L->cur, "AVGWHERE") || kw(&L->cur, "MEANWHERE") ||
      kw(&L->cur, "AVGFWHERE") || kw(&L->cur, "WHEREAVG") ||
      kw(&L->cur, "AVGIF") || kw(&L->cur, "MEANIF")) {
    char filt[48], sfield[48], mfield[48], tok1[48], op[24];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, want_avg = 0, i, n = 0, n_skip = 0;
    long m_nval = 0, sum = 0, outv = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "AVGWHERE") == 0 || strcmp(op, "MEANWHERE") == 0 ||
        strcmp(op, "AVGFWHERE") == 0 || strcmp(op, "WHEREAVG") == 0 ||
        strcmp(op, "AVGIF") == 0 || strcmp(op, "MEANIF") == 0)
      want_avg = 1;
    lex_next(L);
    filt[0] = 0;
    sfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SUMWHERE OF Class sumfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(sfield, sizeof sfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(sfield, sizeof sfield, "%s", vv->sval);
          else
            snprintf(sfield, sizeof sfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", tok1);
      }
    } else {
      fail(vm, "SUMWHERE [Class] sumfield matchfield matchvalue");
      return -1;
    }
    if (!sfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SUMWHERE [Class] sumfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "SUMWHERE [Class] sumfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int sfi, mfi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      sfi = oop_field_idx(cd, sfield);
      if (mfi < 0 || sfi < 0) { n_skip++; continue; }
      if (ob->fis_str[sfi]) { n_skip++; continue; } /* sum numeric only */
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      sum += ob->fnum[sfi];
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
    var_set_num(vm, "SUMWHERE_N", n);
    var_set_num(vm, "AVGWHERE_N", n);
    var_set_num(vm, "TOTALWHERE_N", n);
    var_set_num(vm, "SUMFWHERE_N", n);
    var_set_num(vm, "WHERESUM_N", n);
    var_set_num(vm, "SUMWHERE_SKIP", n_skip);
    var_set_num(vm, "SUMWHERE_SUM", sum);
    var_set_num(vm, "AVGWHERE_SUM", sum);
    var_set_str(vm, "FIELD", sfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MINWHERE|MAXWHERE|MINFWHERE|MAXFWHERE [Class] valfield matchfield matchvalue
   * — min or max of numeric valfield where matchfield == matchvalue.
   * WHERE sugar: MINWHERE Cell energy WHERE age 1.
   * LAST_N = extreme; MINWHERE_N = sample count; soft empty → 0.
   * Usability: filtered MIN/MAX without GETFWHERE+SYS MIN/MAX glue. */
  if (kw(&L->cur, "MINWHERE") || kw(&L->cur, "MAXWHERE") ||
      kw(&L->cur, "MINFWHERE") || kw(&L->cur, "MAXFWHERE") ||
      kw(&L->cur, "WHEREMIN") || kw(&L->cur, "WHEREMAX") ||
      kw(&L->cur, "MINIF") || kw(&L->cur, "MAXIF") ||
      kw(&L->cur, "LOWESTWHERE") || kw(&L->cur, "HIGHESTWHERE") ||
      kw(&L->cur, "BOTWHERE") || kw(&L->cur, "TOPWHERE")) {
    char filt[48], vfield[48], mfield[48], tok1[48], op[24];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, want_max = 0, i, n = 0, n_skip = 0, have = 0;
    long m_nval = 0, best = 0, outv = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "MAXWHERE") == 0 || strcmp(op, "MAXFWHERE") == 0 ||
        strcmp(op, "WHEREMAX") == 0 || strcmp(op, "MAXIF") == 0 ||
        strcmp(op, "HIGHESTWHERE") == 0 || strcmp(op, "TOPWHERE") == 0)
      want_max = 1;
    lex_next(L);
    filt[0] = 0;
    vfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "MINWHERE OF Class valfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(vfield, sizeof vfield, "%s", vv->sval);
          else
            snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", tok1);
      }
    } else {
      fail(vm, "MINWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (!vfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "MINWHERE [Class] valfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "MINWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int vfi, mfi, hit = 0;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      vfi = oop_field_idx(cd, vfield);
      if (mfi < 0 || vfi < 0) { n_skip++; continue; }
      if (ob->fis_str[vfi]) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      v = ob->fnum[vfi];
      if (!have) {
        best = v;
        have = 1;
      } else if (want_max) {
        if (v > best) best = v;
      } else {
        if (v < best) best = v;
      }
      n++;
    }
    outv = have ? best : 0;
    var_set_num(vm, "LAST_N", outv);
    vm->last_n = outv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", outv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "MINWHERE_N", n);
    var_set_num(vm, "MAXWHERE_N", n);
    var_set_num(vm, "MINFWHERE_N", n);
    var_set_num(vm, "MAXFWHERE_N", n);
    var_set_num(vm, "WHEREMIN_N", n);
    var_set_num(vm, "WHEREMAX_N", n);
    var_set_num(vm, "MINWHERE_SKIP", n_skip);
    var_set_num(vm, "MAXWHERE_SKIP", n_skip);
    var_set_num(vm, "MINWHERE_VAL", outv);
    var_set_num(vm, "MAXWHERE_VAL", outv);
    var_set_str(vm, "FIELD", vfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MEDIANWHERE|P50WHERE|MIDWHERE|MEDIANFWHERE [Class] valfield matchfield matchvalue
   * — integer median of numeric valfield where matchfield == matchvalue.
   * WHERE sugar: MEDIANWHERE Cell energy WHERE age 1.
   * Sort ascending; odd → middle; even → lower mid (matches MEDIANF/SYS MEDIAN).
   * LAST_N = median; MEDIANWHERE_N = sample count; soft empty → 0.
   * Usability: filtered MEDIANF without GETFWHERE+SYS MEDIAN glue. */
  if (kw(&L->cur, "MEDIANWHERE") || kw(&L->cur, "P50WHERE") ||
      kw(&L->cur, "MIDWHERE") || kw(&L->cur, "MEDIANFWHERE") ||
      kw(&L->cur, "WHEREMEDIAN") || kw(&L->cur, "MEDIANIF") ||
      kw(&L->cur, "P50IF") || kw(&L->cur, "MIDIF") ||
      kw(&L->cur, "FIELDMEDIANWHERE") || kw(&L->cur, "WHEREP50")) {
    char filt[48], vfield[48], mfield[48], tok1[48];
    char m_sval[512];
    long vals[CUBALC_MAX_OBJS];
    int has_filt = 0, m_is_str = 0, i, j, n = 0, n_skip = 0;
    long m_nval = 0, outv = 0;
    lex_next(L);
    filt[0] = 0;
    vfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "MEDIANWHERE OF Class valfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(vfield, sizeof vfield, "%s", vv->sval);
          else
            snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", tok1);
      }
    } else {
      fail(vm, "MEDIANWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (!vfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "MEDIANWHERE [Class] valfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "MEDIANWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int vfi, mfi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      vfi = oop_field_idx(cd, vfield);
      if (mfi < 0 || vfi < 0) { n_skip++; continue; }
      if (ob->fis_str[vfi]) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      vals[n++] = ob->fnum[vfi];
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
      outv = vals[n / 2 - 1]; /* lower mid for even — match MEDIANF/SYS MEDIAN */
    var_set_num(vm, "LAST_N", outv);
    vm->last_n = outv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", outv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "MEDIANWHERE", outv);
    var_set_num(vm, "MEDIANWHERE_N", n);
    var_set_num(vm, "P50WHERE_N", n);
    var_set_num(vm, "MIDWHERE_N", n);
    var_set_num(vm, "MEDIANFWHERE_N", n);
    var_set_num(vm, "WHEREMEDIAN_N", n);
    var_set_num(vm, "MEDIANWHERE_SKIP", n_skip);
    var_set_num(vm, "MEDIANWHERE_VAL", outv);
    var_set_str(vm, "FIELD", vfield);
    var_set_str(vm, "SRC", mfield);
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

  /* FREQWHERE|HISTWHERE|FREQFWHERE [Class] histfield matchfield matchvalue
   * — frequency histogram of histfield where matchfield == matchvalue.
   * WHERE sugar: FREQWHERE Ticket severity WHERE status "open".
   * LAST = "val:count" bag (first-seen order); LAST_N = distinct keys.
   * FREQWHERE_TOTAL = matched samples; soft empty → empty bag / 0.
   * Usability: filtered FREQF without GETFWHERE+SYS FREQ glue. */
  if (kw(&L->cur, "FREQWHERE") || kw(&L->cur, "HISTWHERE") ||
      kw(&L->cur, "FREQFWHERE") || kw(&L->cur, "WHEREFREQ") ||
      kw(&L->cur, "FREQIF") || kw(&L->cur, "HISTIF") ||
      kw(&L->cur, "HISTFWHERE") || kw(&L->cur, "FIELDFREQWHERE") ||
      kw(&L->cur, "WHEREHIST") || kw(&L->cur, "ROLLUPWHERE")) {
    char filt[48], hfield[48], mfield[48], tok1[48];
    char m_sval[512];
    char keys[64][128];
    long counts[64];
    char out[CUBALC_HOST_STR_MAX];
    int has_filt = 0, m_is_str = 0, i, k, nk = 0, n_skip = 0, total = 0;
    long m_nval = 0;
    size_t olen = 0;
    lex_next(L);
    filt[0] = 0;
    hfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    out[0] = 0;
    memset(counts, 0, sizeof counts);
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "FREQWHERE OF Class histfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(hfield, sizeof hfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(hfield, sizeof hfield, "%s", vv->sval);
          else
            snprintf(hfield, sizeof hfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(hfield, sizeof hfield, "%s", vv->sval);
        else
          snprintf(hfield, sizeof hfield, "%s", tok1);
      }
    } else {
      fail(vm, "FREQWHERE [Class] histfield matchfield matchvalue");
      return -1;
    }
    if (!hfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(hfield, sizeof hfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(hfield, sizeof hfield, "%s", vv->sval);
        else
          snprintf(hfield, sizeof hfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "FREQWHERE [Class] histfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "FREQWHERE [Class] histfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int hfi, mfi, hit = 0;
      char field[128];
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      hfi = oop_field_idx(cd, hfield);
      if (mfi < 0 || hfi < 0) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      if (ob->fis_str[hfi])
        snprintf(field, sizeof field, "%s", ob->fstr[hfi]);
      else
        snprintf(field, sizeof field, "%ld", ob->fnum[hfi]);
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
    var_set_str(vm, "FREQWHERE", out);
    var_set_str(vm, "HISTWHERE", out);
    var_set_str(vm, "FREQFWHERE", out);
    var_set_str(vm, "FREQ", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = nk;
    var_set_num(vm, "LAST_N", nk);
    var_set_num(vm, "FREQWHERE_N", nk);
    var_set_num(vm, "HISTWHERE_N", nk);
    var_set_num(vm, "FREQFWHERE_N", nk);
    var_set_num(vm, "WHEREFREQ_N", nk);
    var_set_num(vm, "FREQWHERE_TOTAL", total);
    var_set_num(vm, "HISTWHERE_TOTAL", total);
    var_set_num(vm, "FREQWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", hfield);
    var_set_str(vm, "SRC", mfield);
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

  /* MODEWHERE|DOMINANTWHERE|MODEFWHERE [Class] valfield matchfield matchvalue
   * — most frequent valfield where matchfield == matchvalue.
   * WHERE sugar: MODEWHERE Ticket severity WHERE status "open".
   * LAST = mode value (first-seen on ties); LAST_N = mode count.
   * MODEWHERE_TOTAL = matched samples; soft empty → "" / 0.
   * Usability: filtered MODEF without FREQWHERE+TOPKEY glue. */
  if (kw(&L->cur, "MODEWHERE") || kw(&L->cur, "DOMINANTWHERE") ||
      kw(&L->cur, "MODEFWHERE") || kw(&L->cur, "WHEREMODE") ||
      kw(&L->cur, "MODEIF") || kw(&L->cur, "MOSTWHERE") ||
      kw(&L->cur, "COMMONWHERE") || kw(&L->cur, "FIELDMODEWHERE") ||
      kw(&L->cur, "WHEREDOMINANT") || kw(&L->cur, "MODEMATCH")) {
    char filt[48], vfield[48], mfield[48], tok1[48];
    char m_sval[512];
    char keys[64][128];
    long counts[64];
    char best[128];
    int has_filt = 0, m_is_str = 0, i, k, nk = 0, n_skip = 0, total = 0, best_i = -1;
    long m_nval = 0, best_c = 0;
    lex_next(L);
    filt[0] = 0;
    vfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    best[0] = 0;
    memset(counts, 0, sizeof counts);
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "MODEWHERE OF Class valfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(vfield, sizeof vfield, "%s", vv->sval);
          else
            snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", tok1);
      }
    } else {
      fail(vm, "MODEWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (!vfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "MODEWHERE [Class] valfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "MODEWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int vfi, mfi, hit = 0;
      char field[128];
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      vfi = oop_field_idx(cd, vfield);
      if (mfi < 0 || vfi < 0) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      if (ob->fis_str[vfi])
        snprintf(field, sizeof field, "%s", ob->fstr[vfi]);
      else
        snprintf(field, sizeof field, "%ld", ob->fnum[vfi]);
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
    var_set_str(vm, "MODEWHERE", best);
    var_set_str(vm, "DOMINANTWHERE", best);
    var_set_str(vm, "MODEFWHERE", best);
    var_set_str(vm, "MODE", best);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", best);
    vm->last_n = best_c;
    var_set_num(vm, "LAST_N", best_c);
    var_set_num(vm, "MODEWHERE_COUNT", best_c);
    var_set_num(vm, "DOMINANTWHERE_COUNT", best_c);
    var_set_num(vm, "MODEWHERE_N", total);
    var_set_num(vm, "MODEFWHERE_N", total);
    var_set_num(vm, "MODEWHERE_TOTAL", total);
    var_set_num(vm, "MODEWHERE_DISTINCT", nk);
    var_set_num(vm, "MODEWHERE_SKIP", n_skip);
    {
      char *end = 0;
      long nv;
      if (best[0]) {
        nv = strtol(best, &end, 10);
        if (end && end != best && *end == 0)
          var_set_num(vm, "MODEWHERE_NUM", nv);
        else
          var_set_num(vm, "MODEWHERE_NUM", 0);
      } else {
        var_set_num(vm, "MODEWHERE_NUM", 0);
      }
    }
    var_set_str(vm, "FIELD", vfield);
    var_set_str(vm, "SRC", mfield);
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

  /* UNIQUWHERE|DISTINCTWHERE|UNIQUEFWHERE [Class] valfield matchfield matchvalue
   * — unique valfield values where matchfield == matchvalue (first-seen bag).
   * WHERE sugar: UNIQUWHERE Ticket severity WHERE status "open".
   * LAST = newline bag; LAST_N = distinct; UNIQUWHERE_TOTAL = matched samples.
   * Soft empty → "" / 0.
   * Usability: filtered UNIQUF without GETFWHERE+UNIQ glue. */
  if (kw(&L->cur, "UNIQUWHERE") || kw(&L->cur, "DISTINCTWHERE") ||
      kw(&L->cur, "UNIQUEFWHERE") || kw(&L->cur, "WHEREUNIQ") ||
      kw(&L->cur, "UNIQWHERE") || kw(&L->cur, "UNIQUEWHERE") ||
      kw(&L->cur, "UNIQUIF") || kw(&L->cur, "DISTINCTIF") ||
      kw(&L->cur, "WHEREDISTINCT") || kw(&L->cur, "FIELDUNIQUWHERE")) {
    char filt[48], vfield[48], mfield[48], tok1[48];
    char m_sval[512];
    char keys[64][128];
    char out[CUBALC_HOST_STR_MAX];
    int has_filt = 0, m_is_str = 0, i, k, nk = 0, n_skip = 0, total = 0;
    long m_nval = 0;
    size_t olen = 0;
    lex_next(L);
    filt[0] = 0;
    vfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    out[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "UNIQUWHERE OF Class valfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(vfield, sizeof vfield, "%s", vv->sval);
          else
            snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", tok1);
      }
    } else {
      fail(vm, "UNIQUWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (!vfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "UNIQUWHERE [Class] valfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "UNIQUWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int vfi, mfi, hit = 0;
      char field[128];
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      vfi = oop_field_idx(cd, vfield);
      if (mfi < 0 || vfi < 0) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      if (ob->fis_str[vfi])
        snprintf(field, sizeof field, "%s", ob->fstr[vfi]);
      else
        snprintf(field, sizeof field, "%ld", ob->fnum[vfi]);
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
    var_set_str(vm, "UNIQUWHERE", out);
    var_set_str(vm, "DISTINCTWHERE", out);
    var_set_str(vm, "UNIQUEFWHERE", out);
    var_set_str(vm, "UNIQWHERE", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = nk;
    var_set_num(vm, "LAST_N", nk);
    var_set_num(vm, "UNIQUWHERE_N", nk);
    var_set_num(vm, "DISTINCTWHERE_N", nk);
    var_set_num(vm, "UNIQUEFWHERE_N", nk);
    var_set_num(vm, "WHEREUNIQ_N", nk);
    var_set_num(vm, "UNIQUWHERE_TOTAL", total);
    var_set_num(vm, "DISTINCTWHERE_TOTAL", total);
    var_set_num(vm, "UNIQUWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", vfield);
    var_set_str(vm, "SRC", mfield);
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

  /* INCF|ADDF|BUMPF|DECF|SUBF obj field [delta]
   * TRYINCF|INCF SOFT — soft miss OK=0.
   * Default delta: +1 for INCF/ADDF/BUMPF, −1 for DECF/SUBF.
   * Numeric fields only; string field soft-skips or fatal if bare.
   * Usability: counter/retry bump without GETF+arith+SETF (fills gap vs
   * INCFALL/INCFWHERE; enables METHOD bodies like INCF THIS retries). */
  if (kw(&L->cur, "INCF") || kw(&L->cur, "ADDF") || kw(&L->cur, "BUMPF") ||
      kw(&L->cur, "INCFIELD") || kw(&L->cur, "ADDFIELD") ||
      kw(&L->cur, "DECF") || kw(&L->cur, "SUBF") || kw(&L->cur, "DECFIELD") ||
      kw(&L->cur, "TRYINCF") || kw(&L->cur, "INCFSOFT") ||
      kw(&L->cur, "SOFTINCF") || kw(&L->cur, "TRYDECF") ||
      kw(&L->cur, "DECFSOFT") || kw(&L->cur, "BUMP") ||
      kw(&L->cur, "ADDTO") || kw(&L->cur, "SUBFROM")) {
    char oname[48], fname[48], op[24];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, is_dec = 0;
    long delta, nv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "DECF") == 0 || strcmp(op, "SUBF") == 0 ||
        strcmp(op, "DECFIELD") == 0 || strcmp(op, "TRYDECF") == 0 ||
        strcmp(op, "DECFSOFT") == 0 || strcmp(op, "SUBFROM") == 0)
      is_dec = 1;
    if (strcmp(op, "TRYINCF") == 0 || strcmp(op, "INCFSOFT") == 0 ||
        strcmp(op, "SOFTINCF") == 0 || strcmp(op, "TRYDECF") == 0 ||
        strcmp(op, "DECFSOFT") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, is_dec ? "DECF object field [delta]" : "INCF object field [delta]");
      return -1;
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
      fail(vm, is_dec ? "DECF field" : "INCF field");
      return -1;
    }
    /* optional delta (default ±1) */
    delta = is_dec ? -1L : 1L;
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
         strcmp(L->cur.text, "END") != 0)) {
      /* only parse if it looks like a value; avoid eating next form */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        delta = parse_expr(vm, L);
        if (is_dec && delta > 0) delta = -delta; /* DECF 3 → −3 */
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        /* numeric var or LAST as delta; skip if looks like next form */
        if (dv && !dv->is_str) {
          delta = dv->val;
          if (is_dec && delta > 0) delta = -delta;
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          delta = vm->last_n;
          if (is_dec && delta > 0) delta = -delta;
          lex_next(L);
        }
        /* else leave default; do not consume */
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown object %s",
                 is_dec ? "DECF" : "INCF", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "INCF_N", 0);
      var_set_num(vm, "DECF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  is_dec ? "DECF: unknown object" : "INCF: unknown object");
      var_set_str(vm, "ERR",
                  is_dec ? "DECF: unknown object" : "INCF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown FIELD %s",
                 is_dec ? "DECF" : "INCF", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "INCF_N", 0);
      var_set_num(vm, "DECF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  is_dec ? "DECF: unknown field" : "INCF: unknown field");
      var_set_str(vm, "ERR",
                  is_dec ? "DECF: unknown field" : "INCF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi]) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s field %s is string",
                 is_dec ? "DECF" : "INCF", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "INCF_N", 0);
      var_set_num(vm, "DECF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  is_dec ? "DECF: string field" : "INCF: string field");
      var_set_str(vm, "ERR",
                  is_dec ? "DECF: string field" : "INCF: string field");
      bump(vm);
      return 1;
    }
    nv = ob->fnum[fi] + delta;
    ob->fnum[fi] = nv;
    ob->fis_str[fi] = 0;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", nv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = nv;
    var_set_num(vm, "LAST_N", nv);
    var_set_num(vm, "INCF_N", 1);
    var_set_num(vm, "DECF_N", is_dec ? 1 : 0);
    var_set_num(vm, "INCF_DELTA", delta);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* BOUNDF|LIMITF|CLIPOBJ|CLAMPOBJ obj field lo [TO] hi
   * TRYBOUNDF|BOUNDF SOFT — soft miss OK=0.
   * Clamp one object's numeric field into [lo,hi] (auto-swap lo/hi).
   * Note: CLAMPF/CLIPF remain fleet CLAMPFALL aliases — this is the single-obj form.
   * Usability: bound energy/retries after INCF without GETF+IF+SETF (METHOD/THIS). */
  if (kw(&L->cur, "BOUNDF") || kw(&L->cur, "LIMITF") ||
      kw(&L->cur, "CLIPOBJ") || kw(&L->cur, "CLAMPOBJ") ||
      kw(&L->cur, "BOUNDFIELD") || kw(&L->cur, "LIMITFIELD") ||
      kw(&L->cur, "SATOBJ") || kw(&L->cur, "SATFIELD") ||
      kw(&L->cur, "CLAMPONE") || kw(&L->cur, "BOUNDONE") ||
      kw(&L->cur, "TRYBOUNDF") || kw(&L->cur, "BOUNDFSOFT") ||
      kw(&L->cur, "SOFTBOUNDF") || kw(&L->cur, "TRYLIMITF") ||
      kw(&L->cur, "TRYCLIPOBJ")) {
    char oname[48], fname[48], op[24];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, changed = 0;
    long lo = 0, hi = 0, t, nv, oldv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "TRYBOUNDF") == 0 || strcmp(op, "BOUNDFSOFT") == 0 ||
        strcmp(op, "SOFTBOUNDF") == 0 || strcmp(op, "TRYLIMITF") == 0 ||
        strcmp(op, "TRYCLIPOBJ") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "BOUNDF object field lo hi"); return -1;
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
      fail(vm, "BOUNDF field"); return -1;
    }
    if (kw(&L->cur, "IN") || kw(&L->cur, "BETWEEN") || kw(&L->cur, "RANGE") ||
        kw(&L->cur, "TO") || kw(&L->cur, "WITHIN"))
      lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        L->cur.kind == TK_LPAREN || L->cur.kind == TK_IDENT) {
      lo = parse_expr(vm, L);
    } else {
      fail(vm, "BOUNDF object field lo hi"); return -1;
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "AND") || kw(&L->cur, "..") ||
        kw(&L->cur, "THRU") || kw(&L->cur, "THROUGH"))
      lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        L->cur.kind == TK_LPAREN || L->cur.kind == TK_IDENT) {
      hi = parse_expr(vm, L);
    } else {
      fail(vm, "BOUNDF object field lo hi"); return -1;
    }
    if (lo > hi) { t = lo; lo = hi; hi = t; }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "BOUNDF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "BOUNDF_N", 0);
      var_set_num(vm, "BOUNDF_CHANGED", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "BOUNDF: unknown object");
      var_set_str(vm, "ERR", "BOUNDF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "BOUNDF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "BOUNDF_N", 0);
      var_set_num(vm, "BOUNDF_CHANGED", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "BOUNDF: unknown field");
      var_set_str(vm, "ERR", "BOUNDF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi]) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "BOUNDF field %s is string", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "BOUNDF_N", 0);
      var_set_num(vm, "BOUNDF_CHANGED", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "BOUNDF: string field");
      var_set_str(vm, "ERR", "BOUNDF: string field");
      bump(vm);
      return 1;
    }
    oldv = ob->fnum[fi];
    nv = oldv;
    if (nv < lo) nv = lo;
    if (nv > hi) nv = hi;
    if (nv != oldv) changed = 1;
    ob->fnum[fi] = nv;
    ob->fis_str[fi] = 0;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", nv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = nv;
    var_set_num(vm, "LAST_N", nv);
    var_set_num(vm, "BOUNDF_N", 1);
    var_set_num(vm, "BOUNDF_CHANGED", changed);
    var_set_num(vm, "BOUNDF_LO", lo);
    var_set_num(vm, "BOUNDF_HI", hi);
    var_set_num(vm, "LIMITF_N", 1);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* TIMESF|SCALEOBJ|MULOBJ obj field factor
   * TRYTIMESF|TIMESF SOFT — soft miss OK=0.
   * Multiply one object's numeric field by factor (integer).
   * Note: MULF/SCALEF remain fleet MULFALL aliases — this is the single-obj form.
   * Usability: double/zero/decay energy without GETF+*+SETF (METHOD/THIS; pairs INCF/BOUNDF). */
  if (kw(&L->cur, "TIMESF") || kw(&L->cur, "SCALEOBJ") ||
      kw(&L->cur, "MULOBJ") || kw(&L->cur, "TIMESOBJ") ||
      kw(&L->cur, "SCALEONE") || kw(&L->cur, "MULONE") ||
      kw(&L->cur, "PRODUCTOBJ") || kw(&L->cur, "MULFIELD") ||
      kw(&L->cur, "SCALEFIELD") || kw(&L->cur, "TIMESFIELD") ||
      kw(&L->cur, "TRYTIMESF") || kw(&L->cur, "TIMESFSOFT") ||
      kw(&L->cur, "SOFTTIMESF") || kw(&L->cur, "TRYSCALEOBJ") ||
      kw(&L->cur, "TRYMULOBJ")) {
    char oname[48], fname[48], op[24];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0;
    long factor = 1, nv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "TRYTIMESF") == 0 || strcmp(op, "TIMESFSOFT") == 0 ||
        strcmp(op, "SOFTTIMESF") == 0 || strcmp(op, "TRYSCALEOBJ") == 0 ||
        strcmp(op, "TRYMULOBJ") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "TIMESF object field factor"); return -1;
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
      fail(vm, "TIMESF field"); return -1;
    }
    if (kw(&L->cur, "BY") || kw(&L->cur, "TIMES") || kw(&L->cur, "MUL") ||
        kw(&L->cur, "SCALE") || kw(&L->cur, "*"))
      lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        factor = parse_expr(vm, L);
      } else {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          factor = dv->val;
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          factor = vm->last_n;
          lex_next(L);
        } else {
          fail(vm, "TIMESF object field factor"); return -1;
        }
      }
    } else {
      fail(vm, "TIMESF object field factor"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "TIMESF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "TIMESF_N", 0);
      var_set_num(vm, "TIMESF_FACTOR", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "TIMESF: unknown object");
      var_set_str(vm, "ERR", "TIMESF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "TIMESF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "TIMESF_N", 0);
      var_set_num(vm, "TIMESF_FACTOR", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "TIMESF: unknown field");
      var_set_str(vm, "ERR", "TIMESF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi]) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "TIMESF field %s is string", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "TIMESF_N", 0);
      var_set_num(vm, "TIMESF_FACTOR", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "TIMESF: string field");
      var_set_str(vm, "ERR", "TIMESF: string field");
      bump(vm);
      return 1;
    }
    nv = ob->fnum[fi] * factor;
    ob->fnum[fi] = nv;
    ob->fis_str[fi] = 0;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", nv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = nv;
    var_set_num(vm, "LAST_N", nv);
    var_set_num(vm, "TIMESF_N", 1);
    var_set_num(vm, "TIMESF_FACTOR", factor);
    var_set_num(vm, "SCALEOBJ_N", 1);
    var_set_num(vm, "MULOBJ_N", 1);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* DIVF|IDIVF|QUOTF obj field divisor — integer divide one numeric field.
   * MODF|REMF|MODULO obj field modulus — remainder one numeric field.
   * TRYDIVF|DIVF SOFT / TRYMODF|MODF SOFT — soft miss OK=0 sticky LAST_ERR.
   * Divisor/modulus 0: hard fail, or soft OK=0 DIVF_OK/MODF_OK=0 field unchanged.
   * Usability: share/wrap counters without GETF+/+SETF (METHOD/THIS; pairs
   * INCF/TIMESF/NUMF after peel). Note: fleet DIVFALL not this form. */
  if (kw(&L->cur, "DIVF") || kw(&L->cur, "IDIVF") ||
      kw(&L->cur, "QUOTF") || kw(&L->cur, "DIVOBJ") ||
      kw(&L->cur, "IDIVOBJ") || kw(&L->cur, "DIVFIELD") ||
      kw(&L->cur, "QUOTFIELD") || kw(&L->cur, "INTDIVF") ||
      kw(&L->cur, "MODF") || kw(&L->cur, "REMF") ||
      kw(&L->cur, "MODULO") || kw(&L->cur, "MODOBJ") ||
      kw(&L->cur, "REMOBJ") || kw(&L->cur, "MODFIELD") ||
      kw(&L->cur, "REMFIELD") || kw(&L->cur, "MODULOF") ||
      kw(&L->cur, "TRYDIVF") || kw(&L->cur, "DIVFSOFT") ||
      kw(&L->cur, "SOFTDIVF") || kw(&L->cur, "TRYIDIVF") ||
      kw(&L->cur, "TRYQUOTF") || kw(&L->cur, "TRYMODF") ||
      kw(&L->cur, "MODFSOFT") || kw(&L->cur, "SOFTMODF") ||
      kw(&L->cur, "TRYREMF") || kw(&L->cur, "TRYMODOBJ")) {
    char oname[48], fname[48], op[24];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, is_mod = 0;
    long rhs = 1, nv, cur;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "MODF") == 0 || strcmp(op, "REMF") == 0 ||
        strcmp(op, "MODULO") == 0 || strcmp(op, "MODOBJ") == 0 ||
        strcmp(op, "REMOBJ") == 0 || strcmp(op, "MODFIELD") == 0 ||
        strcmp(op, "REMFIELD") == 0 || strcmp(op, "MODULOF") == 0 ||
        strcmp(op, "TRYMODF") == 0 || strcmp(op, "MODFSOFT") == 0 ||
        strcmp(op, "SOFTMODF") == 0 || strcmp(op, "TRYREMF") == 0 ||
        strcmp(op, "TRYMODOBJ") == 0)
      is_mod = 1;
    if (strcmp(op, "TRYDIVF") == 0 || strcmp(op, "DIVFSOFT") == 0 ||
        strcmp(op, "SOFTDIVF") == 0 || strcmp(op, "TRYIDIVF") == 0 ||
        strcmp(op, "TRYQUOTF") == 0 || strcmp(op, "TRYMODF") == 0 ||
        strcmp(op, "MODFSOFT") == 0 || strcmp(op, "SOFTMODF") == 0 ||
        strcmp(op, "TRYREMF") == 0 || strcmp(op, "TRYMODOBJ") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, is_mod ? "MODF object field modulus" : "DIVF object field divisor");
      return -1;
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
      fail(vm, is_mod ? "MODF field" : "DIVF field"); return -1;
    }
    if (kw(&L->cur, "BY") || kw(&L->cur, "DIV") || kw(&L->cur, "IDIV") ||
        kw(&L->cur, "MOD") || kw(&L->cur, "REM") || kw(&L->cur, "/") ||
        kw(&L->cur, "%"))
      lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        rhs = parse_expr(vm, L);
      } else {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          rhs = dv->val;
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          rhs = vm->last_n;
          lex_next(L);
        } else {
          fail(vm, is_mod ? "MODF object field modulus" : "DIVF object field divisor");
          return -1;
        }
      }
    } else {
      fail(vm, is_mod ? "MODF object field modulus" : "DIVF object field divisor");
      return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown object %s",
                 is_mod ? "MODF" : "DIVF", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "DIVF_N", 0);
      var_set_num(vm, "MODF_N", 0);
      var_set_num(vm, "DIVF_OK", 0);
      var_set_num(vm, "MODF_OK", 0);
      var_set_num(vm, "DIVF_RHS", 0);
      var_set_num(vm, "MODF_RHS", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  is_mod ? "MODF: unknown object" : "DIVF: unknown object");
      var_set_str(vm, "ERR",
                  is_mod ? "MODF: unknown object" : "DIVF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown FIELD %s",
                 is_mod ? "MODF" : "DIVF", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "DIVF_N", 0);
      var_set_num(vm, "MODF_N", 0);
      var_set_num(vm, "DIVF_OK", 0);
      var_set_num(vm, "MODF_OK", 0);
      var_set_num(vm, "DIVF_RHS", 0);
      var_set_num(vm, "MODF_RHS", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  is_mod ? "MODF: unknown field" : "DIVF: unknown field");
      var_set_str(vm, "ERR",
                  is_mod ? "MODF: unknown field" : "DIVF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi]) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s field %s is string",
                 is_mod ? "MODF" : "DIVF", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "DIVF_N", 0);
      var_set_num(vm, "MODF_N", 0);
      var_set_num(vm, "DIVF_OK", 0);
      var_set_num(vm, "MODF_OK", 0);
      var_set_num(vm, "DIVF_RHS", 0);
      var_set_num(vm, "MODF_RHS", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  is_mod ? "MODF: string field" : "DIVF: string field");
      var_set_str(vm, "ERR",
                  is_mod ? "MODF: string field" : "DIVF: string field");
      bump(vm);
      return 1;
    }
    if (rhs == 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s by zero", is_mod ? "MODF" : "DIVF");
        fail(vm, vm->err); return -1;
      }
      cur = ob->fnum[fi];
      {
        char nb[32];
        snprintf(nb, sizeof nb, "%ld", cur);
        var_set_str(vm, "LAST", nb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      }
      vm->last_n = cur;
      var_set_num(vm, "LAST_N", cur);
      var_set_num(vm, "DIVF_N", is_mod ? 0 : 0);
      var_set_num(vm, "MODF_N", is_mod ? 0 : 0);
      var_set_num(vm, "DIVF_OK", 0);
      var_set_num(vm, "MODF_OK", 0);
      var_set_num(vm, "DIVF_RHS", 0);
      var_set_num(vm, "MODF_RHS", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  is_mod ? "MODF: by zero" : "DIVF: by zero");
      var_set_str(vm, "ERR",
                  is_mod ? "MODF: by zero" : "DIVF: by zero");
      var_set_str(vm, "FIELD", fname);
      var_set_str(vm, "OBJECT", oname);
      bump(vm);
      return 1;
    }
    cur = ob->fnum[fi];
    if (is_mod)
      nv = cur % rhs;
    else
      nv = cur / rhs;
    ob->fnum[fi] = nv;
    ob->fis_str[fi] = 0;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", nv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = nv;
    var_set_num(vm, "LAST_N", nv);
    if (is_mod) {
      var_set_num(vm, "MODF_N", 1);
      var_set_num(vm, "MODF_OK", 1);
      var_set_num(vm, "MODF_RHS", rhs);
      var_set_num(vm, "REMF_N", 1);
      var_set_num(vm, "DIVF_N", 0);
      var_set_num(vm, "DIVF_OK", 0);
    } else {
      var_set_num(vm, "DIVF_N", 1);
      var_set_num(vm, "DIVF_OK", 1);
      var_set_num(vm, "DIVF_RHS", rhs);
      var_set_num(vm, "IDIVF_N", 1);
      var_set_num(vm, "QUOTF_N", 1);
      var_set_num(vm, "MODF_N", 0);
      var_set_num(vm, "MODF_OK", 0);
    }
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* ABSF|IABSF|ABSVALF obj field — absolute value of one numeric field.
   * NEGF|NEGAF|FLIPF obj field — negate one numeric field.
   * SIGNF|SGNF|SIGNUMF obj field — write signum −1|0|1 into field.
   * TRYABSF|ABSF SOFT / TRYNEGF / TRYSIGNF — soft miss OK=0 sticky LAST_ERR.
   * Usability: polarity/magnitude without GETF+SYS IABS/SIGN/NEG+SETF
   * (METHOD/THIS; pairs INCF/DIVF after delta math). */
  if (kw(&L->cur, "ABSF") || kw(&L->cur, "IABSF") ||
      kw(&L->cur, "ABSVALF") || kw(&L->cur, "ABSOBJ") ||
      kw(&L->cur, "ABSFIELD") || kw(&L->cur, "MAGF") ||
      kw(&L->cur, "NEGF") || kw(&L->cur, "NEGAF") ||
      kw(&L->cur, "NEGATEF") || kw(&L->cur, "FLIPF") ||
      kw(&L->cur, "NEGOBJ") || kw(&L->cur, "NEGFIELD") ||
      kw(&L->cur, "SIGNF") || kw(&L->cur, "SGNF") ||
      kw(&L->cur, "SIGNUMF") || kw(&L->cur, "SIGNOBJ") ||
      kw(&L->cur, "SIGNFIELD") || kw(&L->cur, "DIRF") ||
      kw(&L->cur, "TRYABSF") || kw(&L->cur, "ABSFSOFT") ||
      kw(&L->cur, "SOFTABSF") || kw(&L->cur, "TRYNEGF") ||
      kw(&L->cur, "NEGFSOFT") || kw(&L->cur, "SOFTNEGF") ||
      kw(&L->cur, "TRYSIGNF") || kw(&L->cur, "SIGNFSOFT") ||
      kw(&L->cur, "SOFTSIGNF") || kw(&L->cur, "TRYSGNF")) {
    char oname[48], fname[48], op[24];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=abs 1=neg 2=sign */
    long cur, nv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "NEGF") == 0 || strcmp(op, "NEGAF") == 0 ||
        strcmp(op, "NEGATEF") == 0 || strcmp(op, "FLIPF") == 0 ||
        strcmp(op, "NEGOBJ") == 0 || strcmp(op, "NEGFIELD") == 0 ||
        strcmp(op, "TRYNEGF") == 0 || strcmp(op, "NEGFSOFT") == 0 ||
        strcmp(op, "SOFTNEGF") == 0)
      mode = 1;
    else if (strcmp(op, "SIGNF") == 0 || strcmp(op, "SGNF") == 0 ||
             strcmp(op, "SIGNUMF") == 0 || strcmp(op, "SIGNOBJ") == 0 ||
             strcmp(op, "SIGNFIELD") == 0 || strcmp(op, "DIRF") == 0 ||
             strcmp(op, "TRYSIGNF") == 0 || strcmp(op, "SIGNFSOFT") == 0 ||
             strcmp(op, "SOFTSIGNF") == 0 || strcmp(op, "TRYSGNF") == 0)
      mode = 2;
    if (strcmp(op, "TRYABSF") == 0 || strcmp(op, "ABSFSOFT") == 0 ||
        strcmp(op, "SOFTABSF") == 0 || strcmp(op, "TRYNEGF") == 0 ||
        strcmp(op, "NEGFSOFT") == 0 || strcmp(op, "SOFTNEGF") == 0 ||
        strcmp(op, "TRYSIGNF") == 0 || strcmp(op, "SIGNFSOFT") == 0 ||
        strcmp(op, "SOFTSIGNF") == 0 || strcmp(op, "TRYSGNF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, mode == 1 ? "NEGF object field"
                         : (mode == 2 ? "SIGNF object field" : "ABSF object field"));
      return -1;
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
      fail(vm, mode == 1 ? "NEGF field"
                         : (mode == 2 ? "SIGNF field" : "ABSF field"));
      return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      const char *tag = mode == 1 ? "NEGF" : (mode == 2 ? "SIGNF" : "ABSF");
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown object %s", tag, oname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "ABSF_N", 0);
      var_set_num(vm, "NEGF_N", 0);
      var_set_num(vm, "SIGNF_N", 0);
      var_set_num(vm, "OK", 0);
      {
        char ebuf[48];
        snprintf(ebuf, sizeof ebuf, "%s: unknown object", tag);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
      }
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      const char *tag = mode == 1 ? "NEGF" : (mode == 2 ? "SIGNF" : "ABSF");
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown FIELD %s", tag, fname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "ABSF_N", 0);
      var_set_num(vm, "NEGF_N", 0);
      var_set_num(vm, "SIGNF_N", 0);
      var_set_num(vm, "OK", 0);
      {
        char ebuf[48];
        snprintf(ebuf, sizeof ebuf, "%s: unknown field", tag);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
      }
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi]) {
      const char *tag = mode == 1 ? "NEGF" : (mode == 2 ? "SIGNF" : "ABSF");
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s field %s is string", tag, fname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "ABSF_N", 0);
      var_set_num(vm, "NEGF_N", 0);
      var_set_num(vm, "SIGNF_N", 0);
      var_set_num(vm, "OK", 0);
      {
        char ebuf[48];
        snprintf(ebuf, sizeof ebuf, "%s: string field", tag);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
      }
      bump(vm);
      return 1;
    }
    cur = ob->fnum[fi];
    if (mode == 1) {
      /* negate; LONG_MIN stays LONG_MIN on two's complement overflow */
      nv = -cur;
    } else if (mode == 2) {
      nv = (cur > 0) ? 1L : ((cur < 0) ? -1L : 0L);
    } else {
      nv = (cur < 0) ? -cur : cur;
    }
    ob->fnum[fi] = nv;
    ob->fis_str[fi] = 0;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", nv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = nv;
    var_set_num(vm, "LAST_N", nv);
    var_set_num(vm, "ABSF_N", mode == 0 ? 1 : 0);
    var_set_num(vm, "NEGF_N", mode == 1 ? 1 : 0);
    var_set_num(vm, "SIGNF_N", mode == 2 ? 1 : 0);
    if (mode == 0) {
      var_set_num(vm, "IABSF_N", 1);
      var_set_num(vm, "MAGF_N", 1);
    } else if (mode == 1) {
      var_set_num(vm, "FLIPF_N", 1);
    } else {
      var_set_num(vm, "SGNF_N", 1);
      var_set_num(vm, "DIRF_N", 1);
    }
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* CATF|APPENDF|STRCATF|PREPENDF obj field string
   * TRYCATF|CATF SOFT — soft miss OK=0.
   * Append (or PREPENDF prefix) text onto a string field; promotes num→str.
   * Usability: status/note logs without GETF+CAT+SETF (METHOD/THIS; dual of INCF). */
  if (kw(&L->cur, "CATF") || kw(&L->cur, "APPENDF") ||
      kw(&L->cur, "STRCATF") || kw(&L->cur, "ADDSUF") ||
      kw(&L->cur, "SUFFIXF") || kw(&L->cur, "CONCATF") ||
      kw(&L->cur, "PREPENDF") || kw(&L->cur, "PREFIXF") ||
      kw(&L->cur, "ADDPREF") || kw(&L->cur, "TRYCATF") ||
      kw(&L->cur, "CATFSOFT") || kw(&L->cur, "SOFTCATF") ||
      kw(&L->cur, "TRYAPPENDF") || kw(&L->cur, "TRYPREPENDF") ||
      kw(&L->cur, "STRADDF")) {
    char oname[48], fname[48], piece[256], op[24], out[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, is_pre = 0;
    size_t pl, ol;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "PREPENDF") == 0 || strcmp(op, "PREFIXF") == 0 ||
        strcmp(op, "ADDPREF") == 0 || strcmp(op, "TRYPREPENDF") == 0)
      is_pre = 1;
    if (strcmp(op, "TRYCATF") == 0 || strcmp(op, "CATFSOFT") == 0 ||
        strcmp(op, "SOFTCATF") == 0 || strcmp(op, "TRYAPPENDF") == 0 ||
        strcmp(op, "TRYPREPENDF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "CATF object field string"); return -1;
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
      fail(vm, "CATF field"); return -1;
    }
    if (kw(&L->cur, "WITH") || kw(&L->cur, "PLUS") || kw(&L->cur, "AND") ||
        kw(&L->cur, "APPEND") || kw(&L->cur, "PREPEND"))
      lex_next(L);
    piece[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(piece, sizeof piece, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(piece, sizeof piece, "%s", vm->last_str);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(piece, sizeof piece, "%s", sv->sval);
        lex_next(L);
      } else if (sv && !sv->is_str) {
        snprintf(piece, sizeof piece, "%ld", sv->val);
        lex_next(L);
      } else {
        fail(vm, "CATF object field string"); return -1;
      }
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      long v = parse_expr(vm, L);
      snprintf(piece, sizeof piece, "%ld", v);
    } else {
      fail(vm, "CATF object field string"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "CATF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "CATF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CATF: unknown object");
      var_set_str(vm, "ERR", "CATF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "CATF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "CATF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CATF: unknown field");
      var_set_str(vm, "ERR", "CATF: unknown field");
      bump(vm);
      return 1;
    }
    /* base text: string field or decimal of numeric */
    if (ob->fis_str[fi])
      snprintf(out, sizeof out, "%s", ob->fstr[fi]);
    else
      snprintf(out, sizeof out, "%ld", ob->fnum[fi]);
    pl = strlen(piece);
    ol = strlen(out);
    if (is_pre) {
      /* piece + out into fstr */
      if (pl + ol >= sizeof ob->fstr[fi]) {
        /* truncate piece to fit */
        if (pl >= sizeof ob->fstr[fi]) pl = sizeof ob->fstr[fi] - 1;
        if (pl + ol >= sizeof ob->fstr[fi]) ol = sizeof ob->fstr[fi] - 1 - pl;
      }
      {
        char tmp[sizeof ob->fstr[0]];
        size_t i;
        for (i = 0; i < pl; i++) tmp[i] = piece[i];
        for (i = 0; i < ol && pl + i + 1 < sizeof tmp; i++)
          tmp[pl + i] = out[i];
        tmp[pl + ol < sizeof tmp ? pl + ol : sizeof tmp - 1] = 0;
        snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", tmp);
      }
    } else {
      if (ol + pl >= sizeof ob->fstr[fi]) {
        if (ol >= sizeof ob->fstr[fi]) ol = sizeof ob->fstr[fi] - 1;
        if (ol + pl >= sizeof ob->fstr[fi]) pl = sizeof ob->fstr[fi] - 1 - ol;
      }
      {
        size_t i;
        for (i = 0; i < ol && i + 1 < sizeof ob->fstr[fi]; i++)
          ob->fstr[fi][i] = out[i];
        for (i = 0; i < pl && ol + i + 1 < sizeof ob->fstr[fi]; i++)
          ob->fstr[fi][ol + i] = piece[i];
        ob->fstr[fi][ol + pl < sizeof ob->fstr[fi] ? ol + pl
                                                   : sizeof ob->fstr[fi] - 1] = 0;
      }
    }
    ob->fis_str[fi] = 1;
    var_set_str(vm, "LAST", ob->fstr[fi]);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->fstr[fi]);
    vm->last_n = (long)strlen(ob->fstr[fi]);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "CATF_N", 1);
    var_set_num(vm, "APPENDF_N", is_pre ? 0 : 1);
    var_set_num(vm, "PREPENDF_N", is_pre ? 1 : 0);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* CATFALL|APPENDFALL|PREPENDFALL [Class] field string
   * — append (or prepend) text onto string field of every live obj.
   * Soft always; missing fields skipped (CATFALL_SKIP). Promotes num→str.
   * LAST_N = update count; LAST = piece applied.
   * Usability: fleet note/status tags without EACH OBJ + GETF+CAT+SETF.
   * String dual of INCFALL; fleet dual of CATF. */
  if (kw(&L->cur, "CATFALL") || kw(&L->cur, "APPENDFALL") ||
      kw(&L->cur, "STRCATFALL") || kw(&L->cur, "SUFFIXFALL") ||
      kw(&L->cur, "CONCATFALL") || kw(&L->cur, "ADDSUFALL") ||
      kw(&L->cur, "PREPENDFALL") || kw(&L->cur, "PREFIXFALL") ||
      kw(&L->cur, "ADDPREFALL") || kw(&L->cur, "CATF_ALL") ||
      kw(&L->cur, "APPENDF_ALL") || kw(&L->cur, "PREPENDF_ALL") ||
      kw(&L->cur, "BULKCATF") || kw(&L->cur, "BULKAPPENDF") ||
      kw(&L->cur, "BULKPREPENDF") || kw(&L->cur, "MAPCATF") ||
      kw(&L->cur, "MAPAPPENDF") || kw(&L->cur, "MAPPREPENDF")) {
    char filt[48], fname[48], tok1[48], piece[256], op[24], out[256];
    int has_filt = 0, is_pre = 0, i, n = 0, n_skip = 0;
    size_t pl, ol;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "PREPENDFALL") == 0 || strcmp(op, "PREFIXFALL") == 0 ||
        strcmp(op, "ADDPREFALL") == 0 || strcmp(op, "PREPENDF_ALL") == 0 ||
        strcmp(op, "BULKPREPENDF") == 0 || strcmp(op, "MAPPREPENDF") == 0)
      is_pre = 1;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    piece[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "CATFALL OF Class field string"); return -1;
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
            (L->cur.kind == TK_STR || L->cur.kind == TK_NUM ||
             L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "CATFALL [Class] field string"); return -1;
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
        fail(vm, "CATFALL [Class] field string"); return -1;
      }
    }
    if (kw(&L->cur, "WITH") || kw(&L->cur, "PLUS") || kw(&L->cur, "AND") ||
        kw(&L->cur, "APPEND") || kw(&L->cur, "PREPEND") || kw(&L->cur, "BY"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(piece, sizeof piece, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(piece, sizeof piece, "%s", vm->last_str);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(piece, sizeof piece, "%s", sv->sval);
        lex_next(L);
      } else if (sv && !sv->is_str) {
        snprintf(piece, sizeof piece, "%ld", sv->val);
        lex_next(L);
      } else {
        fail(vm, "CATFALL [Class] field string"); return -1;
      }
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      long v = parse_expr(vm, L);
      snprintf(piece, sizeof piece, "%ld", v);
    } else {
      fail(vm, "CATFALL [Class] field string"); return -1;
    }
    pl = strlen(piece);
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
      if (ob->fis_str[fi])
        snprintf(out, sizeof out, "%s", ob->fstr[fi]);
      else
        snprintf(out, sizeof out, "%ld", ob->fnum[fi]);
      ol = strlen(out);
      if (is_pre) {
        size_t puse = pl, ouse = ol;
        if (puse + ouse >= sizeof ob->fstr[fi]) {
          if (puse >= sizeof ob->fstr[fi]) puse = sizeof ob->fstr[fi] - 1;
          if (puse + ouse >= sizeof ob->fstr[fi])
            ouse = sizeof ob->fstr[fi] - 1 - puse;
        }
        {
          char tmp[sizeof ob->fstr[0]];
          size_t j;
          for (j = 0; j < puse; j++) tmp[j] = piece[j];
          for (j = 0; j < ouse && puse + j + 1 < sizeof tmp; j++)
            tmp[puse + j] = out[j];
          tmp[puse + ouse < sizeof tmp ? puse + ouse : sizeof tmp - 1] = 0;
          snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", tmp);
        }
      } else {
        size_t puse = pl, ouse = ol;
        if (ouse + puse >= sizeof ob->fstr[fi]) {
          if (ouse >= sizeof ob->fstr[fi]) ouse = sizeof ob->fstr[fi] - 1;
          if (ouse + puse >= sizeof ob->fstr[fi])
            puse = sizeof ob->fstr[fi] - 1 - ouse;
        }
        {
          size_t j;
          for (j = 0; j < ouse && j + 1 < sizeof ob->fstr[fi]; j++)
            ob->fstr[fi][j] = out[j];
          for (j = 0; j < puse && ouse + j + 1 < sizeof ob->fstr[fi]; j++)
            ob->fstr[fi][ouse + j] = piece[j];
          ob->fstr[fi][ouse + puse < sizeof ob->fstr[fi] ? ouse + puse
                                                         : sizeof ob->fstr[fi] - 1] = 0;
        }
      }
      ob->fis_str[fi] = 1;
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "CATFALL_N", n);
    var_set_num(vm, "APPENDFALL_N", is_pre ? 0 : n);
    var_set_num(vm, "PREPENDFALL_N", is_pre ? n : 0);
    var_set_num(vm, "CATFALL_SKIP", n_skip);
    var_set_num(vm, "APPENDFALL_SKIP", n_skip);
    var_set_num(vm, "PREPENDFALL_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0])
      var_set_str(vm, "CLASS", filt);
    var_set_str(vm, "LAST", piece);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", piece);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* REPLACEF|GSUBF|SUBSTF obj field old new
   * TRYREPLACEF|REPLACEF SOFT — soft miss OK=0.
   * REPLACEF FIRST — first occurrence only (default: all like GSUB).
   * In-place string surgery on a field; promotes num→str.
   * LAST = result; LAST_N = replace count. Empty old = no-op.
   * Usability: rewrite status/note/templates without GETF+REPLACEALL+SETF
   * (METHOD/THIS; complements CATF concat). */
  if (kw(&L->cur, "REPLACEF") || kw(&L->cur, "GSUBF") ||
      kw(&L->cur, "SUBSTF") || kw(&L->cur, "FIELDREPLACE") ||
      kw(&L->cur, "REPF") || kw(&L->cur, "STRREPF") ||
      kw(&L->cur, "SUBSTFIELD") || kw(&L->cur, "GSUBFIELD") ||
      kw(&L->cur, "TRYREPLACEF") || kw(&L->cur, "REPLACEFSOFT") ||
      kw(&L->cur, "SOFTREPLACEF") || kw(&L->cur, "TRYGSUBF") ||
      kw(&L->cur, "TRYSUBSTF")) {
    char oname[48], fname[48], olds[256], news[256], hay[256], out[256];
    char op[24];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, do_all = 1;
    long did = 0;
    size_t oldn, newn;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "TRYREPLACEF") == 0 || strcmp(op, "REPLACEFSOFT") == 0 ||
        strcmp(op, "SOFTREPLACEF") == 0 || strcmp(op, "TRYGSUBF") == 0 ||
        strcmp(op, "TRYSUBSTF") == 0)
      soft = 1;
    /* GSUBF always all; plain REPLACEF defaults all, FIRST optional */
    if (strcmp(op, "GSUBF") == 0 || strcmp(op, "GSUBFIELD") == 0 ||
        strcmp(op, "TRYGSUBF") == 0)
      do_all = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (kw(&L->cur, "FIRST") || kw(&L->cur, "ONCE") || kw(&L->cur, "ONE")) {
      do_all = 0;
      lex_next(L);
    } else if (kw(&L->cur, "ALL") || kw(&L->cur, "GLOBAL") ||
               kw(&L->cur, "EVERY") || kw(&L->cur, "G")) {
      do_all = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "REPLACEF object field old new"); return -1;
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
      fail(vm, "REPLACEF field"); return -1;
    }
    olds[0] = 0;
    news[0] = 0;
    if (resolve_str_arg(vm, L, olds, sizeof olds) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(olds, sizeof olds, "%ld", v);
      } else {
        fail(vm, "REPLACEF object field old new"); return -1;
      }
    }
    if (kw(&L->cur, "WITH") || kw(&L->cur, "BY") || kw(&L->cur, "TO") ||
        kw(&L->cur, "AS") || kw(&L->cur, "INTO"))
      lex_next(L);
    if (resolve_str_arg(vm, L, news, sizeof news) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(news, sizeof news, "%ld", v);
      } else {
        fail(vm, "REPLACEF object field old new"); return -1;
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "REPLACEF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "REPLACEF_N", 0);
      var_set_num(vm, "GSUBF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "REPLACEF: unknown object");
      var_set_str(vm, "ERR", "REPLACEF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "REPLACEF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "REPLACEF_N", 0);
      var_set_num(vm, "GSUBF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "REPLACEF: unknown field");
      var_set_str(vm, "ERR", "REPLACEF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    oldn = strlen(olds);
    newn = strlen(news);
    did = 0;
    if (olds[0] == 0) {
      snprintf(out, sizeof out, "%s", hay);
    } else {
      const char *src = hay;
      size_t o = 0;
      out[0] = 0;
      for (;;) {
        const char *p = strstr(src, olds);
        if (!p) {
          size_t rest = strlen(src);
          if (o + rest >= sizeof out) rest = sizeof out - 1 - o;
          memcpy(out + o, src, rest);
          o += rest;
          out[o] = 0;
          break;
        }
        {
          size_t pre = (size_t)(p - src);
          if (o + pre >= sizeof out) pre = sizeof out - 1 - o;
          memcpy(out + o, src, pre);
          o += pre;
          if (o + newn < sizeof out) {
            memcpy(out + o, news, newn);
            o += newn;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, news, take);
            o += take;
          }
          out[o] = 0;
          did++;
          src = p + oldn;
          if (!do_all) {
            size_t rest = strlen(src);
            if (o + rest >= sizeof out) rest = sizeof out - 1 - o;
            memcpy(out + o, src, rest);
            o += rest;
            out[o] = 0;
            break;
          }
          if (o >= sizeof out - 1) break;
        }
      }
    }
    snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
    ob->fis_str[fi] = 1;
    var_set_str(vm, "LAST", ob->fstr[fi]);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->fstr[fi]);
    vm->last_n = did;
    var_set_num(vm, "LAST_N", did);
    var_set_num(vm, "REPLACEF_N", 1);
    var_set_num(vm, "GSUBF_N", 1);
    var_set_num(vm, "REPLACEF_COUNT", did);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* TRIMF|LTRIMF|RTRIMF|UPPERF|LOWERF obj field
   * TRYTRIMF|TRIMF SOFT — soft miss OK=0.
   * In-place normalize string field (whitespace trim / ASCII case);
   * promotes num→str. LAST = result; LAST_N = length.
   * Usability: clean status/note before IF without GETF+TRIM/UPPER/LOWER+SETF
   * (METHOD/THIS; pairs REPLACEF/CATF). */
  if (kw(&L->cur, "TRIMF") || kw(&L->cur, "STRIPF") ||
      kw(&L->cur, "LTRIMF") || kw(&L->cur, "RTRIMF") ||
      kw(&L->cur, "FIELDTRIM") || kw(&L->cur, "TRIMFIELD") ||
      kw(&L->cur, "UPPERF") || kw(&L->cur, "UCASEF") ||
      kw(&L->cur, "TOUPPERF") || kw(&L->cur, "FIELDUPPER") ||
      kw(&L->cur, "LOWERF") || kw(&L->cur, "LCASEF") ||
      kw(&L->cur, "TOLOWERF") || kw(&L->cur, "FIELDLOWER") ||
      kw(&L->cur, "TRYTRIMF") || kw(&L->cur, "TRIMFSOFT") ||
      kw(&L->cur, "SOFTTRIMF") || kw(&L->cur, "TRYUPPERF") ||
      kw(&L->cur, "TRYLOWERF")) {
    char oname[48], fname[48], op[24], hay[256], out[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=trim both, 1=ltrim, 2=rtrim, 3=upper, 4=lower */
    char *a, *b;
    size_t n;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "LTRIMF") == 0)
      mode = 1;
    else if (strcmp(op, "RTRIMF") == 0)
      mode = 2;
    else if (strcmp(op, "UPPERF") == 0 || strcmp(op, "UCASEF") == 0 ||
             strcmp(op, "TOUPPERF") == 0 || strcmp(op, "FIELDUPPER") == 0 ||
             strcmp(op, "TRYUPPERF") == 0)
      mode = 3;
    else if (strcmp(op, "LOWERF") == 0 || strcmp(op, "LCASEF") == 0 ||
             strcmp(op, "TOLOWERF") == 0 || strcmp(op, "FIELDLOWER") == 0 ||
             strcmp(op, "TRYLOWERF") == 0)
      mode = 4;
    else
      mode = 0; /* TRIMF / STRIPF / FIELDTRIM */
    if (strcmp(op, "TRYTRIMF") == 0 || strcmp(op, "TRIMFSOFT") == 0 ||
        strcmp(op, "SOFTTRIMF") == 0 || strcmp(op, "TRYUPPERF") == 0 ||
        strcmp(op, "TRYLOWERF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    /* TRIMF L|R|LEFT|RIGHT | TRIMF BOTH */
    if (mode == 0) {
      if (kw(&L->cur, "L") || kw(&L->cur, "LEFT") || kw(&L->cur, "LTRIM")) {
        mode = 1;
        lex_next(L);
      } else if (kw(&L->cur, "R") || kw(&L->cur, "RIGHT") ||
                 kw(&L->cur, "RTRIM")) {
        mode = 2;
        lex_next(L);
      } else if (kw(&L->cur, "BOTH") || kw(&L->cur, "ALL") ||
                 kw(&L->cur, "LR")) {
        mode = 0;
        lex_next(L);
      }
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "TRIMF object field"); return -1;
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
      fail(vm, "TRIMF field"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "TRIMF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "TRIMF_N", 0);
      var_set_num(vm, "UPPERF_N", 0);
      var_set_num(vm, "LOWERF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "TRIMF: unknown object");
      var_set_str(vm, "ERR", "TRIMF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "TRIMF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "TRIMF_N", 0);
      var_set_num(vm, "UPPERF_N", 0);
      var_set_num(vm, "LOWERF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "TRIMF: unknown field");
      var_set_str(vm, "ERR", "TRIMF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    if (mode == 3) {
      /* UPPER */
      {
        char *p;
        for (p = hay; *p; p++)
          if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
      }
      snprintf(out, sizeof out, "%s", hay);
    } else if (mode == 4) {
      /* LOWER */
      {
        char *p;
        for (p = hay; *p; p++)
          if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
      }
      snprintf(out, sizeof out, "%s", hay);
    } else {
      a = hay;
      b = hay + strlen(hay);
      if (mode == 0 || mode == 1)
        while (*a == ' ' || *a == '\t' || *a == '\n' || *a == '\r') a++;
      if (mode == 0 || mode == 2)
        while (b > a &&
               (b[-1] == ' ' || b[-1] == '\t' || b[-1] == '\n' ||
                b[-1] == '\r'))
          b--;
      n = (size_t)(b - a);
      if (n >= sizeof out) n = sizeof out - 1;
      memcpy(out, a, n);
      out[n] = 0;
    }
    snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
    ob->fis_str[fi] = 1;
    n = strlen(out);
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = (long)n;
    var_set_num(vm, "LAST_N", (long)n);
    var_set_num(vm, "TRIMF_N", (mode <= 2) ? 1 : 0);
    var_set_num(vm, "LTRIMF_N", (mode == 1) ? 1 : 0);
    var_set_num(vm, "RTRIMF_N", (mode == 2) ? 1 : 0);
    var_set_num(vm, "UPPERF_N", (mode == 3) ? 1 : 0);
    var_set_num(vm, "LOWERF_N", (mode == 4) ? 1 : 0);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LENF|STRLENF|EMPTYF|BLANKF|NONEMPTYF obj field
   * TRYLENF|LENF SOFT — soft miss OK=0.
   * Field probes without mutating: length / empty / blank / nonempty.
   * LAST = field text (num promoted); LAST_N = length (LENF) or 0|1 probe.
   * Usability: IF guards on note/status without GETF+SYS LEN/EMPTY glue
   * (METHOD/THIS; complements TRIMF mutators). */
  if (kw(&L->cur, "LENF") || kw(&L->cur, "STRLENF") ||
      kw(&L->cur, "FIELDLEN") || kw(&L->cur, "LENFIELD") ||
      kw(&L->cur, "EMPTYF") || kw(&L->cur, "ISEMPTYF") ||
      kw(&L->cur, "FIELDEMPTY") || kw(&L->cur, "EMPTYFIELD") ||
      kw(&L->cur, "BLANKF") || kw(&L->cur, "ISBLANKF") ||
      kw(&L->cur, "FIELDBLANK") || kw(&L->cur, "BLANKFIELD") ||
      kw(&L->cur, "NONEMPTYF") || kw(&L->cur, "NOTEMPTYF") ||
      kw(&L->cur, "HASCHARSF") || kw(&L->cur, "FIELDFULL") ||
      kw(&L->cur, "TRYLENF") || kw(&L->cur, "LENFSOFT") ||
      kw(&L->cur, "SOFTLENF") || kw(&L->cur, "TRYEMPTYF") ||
      kw(&L->cur, "TRYBLANKF") || kw(&L->cur, "TRYNONEMPTYF")) {
    char oname[48], fname[48], op[24], hay[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=len, 1=empty, 2=blank, 3=nonempty */
    long hit = 0, len = 0;
    const char *p;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "EMPTYF") == 0 || strcmp(op, "ISEMPTYF") == 0 ||
        strcmp(op, "FIELDEMPTY") == 0 || strcmp(op, "EMPTYFIELD") == 0 ||
        strcmp(op, "TRYEMPTYF") == 0)
      mode = 1;
    else if (strcmp(op, "BLANKF") == 0 || strcmp(op, "ISBLANKF") == 0 ||
             strcmp(op, "FIELDBLANK") == 0 || strcmp(op, "BLANKFIELD") == 0 ||
             strcmp(op, "TRYBLANKF") == 0)
      mode = 2;
    else if (strcmp(op, "NONEMPTYF") == 0 || strcmp(op, "NOTEMPTYF") == 0 ||
             strcmp(op, "HASCHARSF") == 0 || strcmp(op, "FIELDFULL") == 0 ||
             strcmp(op, "TRYNONEMPTYF") == 0)
      mode = 3;
    else
      mode = 0; /* LENF */
    if (strcmp(op, "TRYLENF") == 0 || strcmp(op, "LENFSOFT") == 0 ||
        strcmp(op, "SOFTLENF") == 0 || strcmp(op, "TRYEMPTYF") == 0 ||
        strcmp(op, "TRYBLANKF") == 0 || strcmp(op, "TRYNONEMPTYF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "LENF object field"); return -1;
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
      fail(vm, "LENF field"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "LENF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "LENF_N", 0);
      var_set_num(vm, "EMPTYF_N", 0);
      var_set_num(vm, "BLANKF_N", 0);
      var_set_num(vm, "NONEMPTYF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LENF: unknown object");
      var_set_str(vm, "ERR", "LENF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "LENF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "LENF_N", 0);
      var_set_num(vm, "EMPTYF_N", 0);
      var_set_num(vm, "BLANKF_N", 0);
      var_set_num(vm, "NONEMPTYF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LENF: unknown field");
      var_set_str(vm, "ERR", "LENF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    len = (long)strlen(hay);
    if (mode == 1) {
      hit = (hay[0] == 0) ? 1 : 0;
    } else if (mode == 2) {
      p = hay;
      while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
      hit = (*p == 0) ? 1 : 0;
    } else if (mode == 3) {
      hit = (hay[0] != 0) ? 1 : 0;
    } else {
      hit = len; /* LENF reports length in LAST_N */
    }
    var_set_str(vm, "LAST", hay);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hay);
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "LENF_N", len);
    var_set_num(vm, "EMPTYF_N", (hay[0] == 0) ? 1 : 0);
    {
      p = hay;
      while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
      var_set_num(vm, "BLANKF_N", (*p == 0) ? 1 : 0);
    }
    var_set_num(vm, "NONEMPTYF_N", (hay[0] != 0) ? 1 : 0);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASINF|CONTAINSF|STARTSF|ENDSF obj field needle
   * HASIFI|STARTSIF|ENDSIF — case-insensitive.
   * TRYHASINF|HASINF SOFT — soft miss OK=0.
   * Field membership probes (no mutate). LAST=field text; LAST_N 0|1.
   * Usability: IF on status/note without GETF+SYS HAS/STARTS/ENDS
   * (METHOD/THIS; complements LENF/EMPTYF probes). */
  if (kw(&L->cur, "HASINF") || kw(&L->cur, "CONTAINSF") ||
      kw(&L->cur, "FIELDHAS") || kw(&L->cur, "HASFIELDSTR") ||
      kw(&L->cur, "INSTRF") || kw(&L->cur, "INCLUDESF") ||
      kw(&L->cur, "HASIFI") || kw(&L->cur, "CONTAINSFI") ||
      kw(&L->cur, "IHASINF") || kw(&L->cur, "FIELDHASI") ||
      kw(&L->cur, "STARTSF") || kw(&L->cur, "PREFIXFHAS") ||
      kw(&L->cur, "FIELDSTARTS") || kw(&L->cur, "STARTSWITHF") ||
      kw(&L->cur, "STARTSIF") || kw(&L->cur, "ISTARTSF") ||
      kw(&L->cur, "PREFIXFI") ||
      kw(&L->cur, "ENDSF") || kw(&L->cur, "SUFFIXFHAS") ||
      kw(&L->cur, "FIELDENDS") || kw(&L->cur, "ENDSWITHF") ||
      kw(&L->cur, "ENDSIF") || kw(&L->cur, "IENDSF") ||
      kw(&L->cur, "SUFFIXFI") ||
      kw(&L->cur, "TRYHASINF") || kw(&L->cur, "HASINFSOFT") ||
      kw(&L->cur, "SOFTHASINF") || kw(&L->cur, "TRYSTARTSF") ||
      kw(&L->cur, "TRYENDSF") || kw(&L->cur, "TRYHASIFI")) {
    char oname[48], fname[48], op[24], hay[256], needle[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=has, 1=starts, 2=ends */
    int icase = 0;
    long hit = 0;
    size_t hn, nn;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "STARTSF") == 0 || strcmp(op, "PREFIXFHAS") == 0 ||
        strcmp(op, "FIELDSTARTS") == 0 || strcmp(op, "STARTSWITHF") == 0 ||
        strcmp(op, "STARTSIF") == 0 || strcmp(op, "ISTARTSF") == 0 ||
        strcmp(op, "PREFIXFI") == 0 || strcmp(op, "TRYSTARTSF") == 0)
      mode = 1;
    else if (strcmp(op, "ENDSF") == 0 || strcmp(op, "SUFFIXFHAS") == 0 ||
             strcmp(op, "FIELDENDS") == 0 || strcmp(op, "ENDSWITHF") == 0 ||
             strcmp(op, "ENDSIF") == 0 || strcmp(op, "IENDSF") == 0 ||
             strcmp(op, "SUFFIXFI") == 0 || strcmp(op, "TRYENDSF") == 0)
      mode = 2;
    else
      mode = 0; /* HAS */
    if (strcmp(op, "HASIFI") == 0 || strcmp(op, "CONTAINSFI") == 0 ||
        strcmp(op, "IHASINF") == 0 || strcmp(op, "FIELDHASI") == 0 ||
        strcmp(op, "STARTSIF") == 0 || strcmp(op, "ISTARTSF") == 0 ||
        strcmp(op, "PREFIXFI") == 0 || strcmp(op, "ENDSIF") == 0 ||
        strcmp(op, "IENDSF") == 0 || strcmp(op, "SUFFIXFI") == 0 ||
        strcmp(op, "TRYHASIFI") == 0)
      icase = 1;
    if (strcmp(op, "TRYHASINF") == 0 || strcmp(op, "HASINFSOFT") == 0 ||
        strcmp(op, "SOFTHASINF") == 0 || strcmp(op, "TRYSTARTSF") == 0 ||
        strcmp(op, "TRYENDSF") == 0 || strcmp(op, "TRYHASIFI") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (!icase && (kw(&L->cur, "I") || kw(&L->cur, "ICASE") ||
                   kw(&L->cur, "IGNORECASE") || kw(&L->cur, "-I") ||
                   kw(&L->cur, "CI"))) {
      icase = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "HASINF object field needle"); return -1;
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
      fail(vm, "HASINF field"); return -1;
    }
    needle[0] = 0;
    if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(needle, sizeof needle, "%ld", v);
      } else {
        fail(vm, "HASINF object field needle"); return -1;
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "HASINF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "HASINF_N", 0);
      var_set_num(vm, "STARTSF_N", 0);
      var_set_num(vm, "ENDSF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASINF: unknown object");
      var_set_str(vm, "ERR", "HASINF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "HASINF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "HASINF_N", 0);
      var_set_num(vm, "STARTSF_N", 0);
      var_set_num(vm, "ENDSF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASINF: unknown field");
      var_set_str(vm, "ERR", "HASINF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    hn = strlen(hay);
    nn = strlen(needle);
    hit = 0;
    if (nn == 0) {
      hit = 1; /* empty needle matches (like SYS HAS) */
    } else if (mode == 0) {
      /* contains */
      if (!icase) {
        hit = (strstr(hay, needle) != NULL) ? 1 : 0;
      } else {
        size_t i, j;
        for (i = 0; i + nn <= hn; i++) {
          int ok = 1;
          for (j = 0; j < nn; j++) {
            char ca = hay[i + j], cb = needle[j];
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) { ok = 0; break; }
          }
          if (ok) { hit = 1; break; }
        }
      }
    } else if (mode == 1) {
      /* starts */
      if (nn <= hn) {
        if (!icase) {
          hit = (strncmp(hay, needle, nn) == 0) ? 1 : 0;
        } else {
          size_t j;
          hit = 1;
          for (j = 0; j < nn; j++) {
            char ca = hay[j], cb = needle[j];
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) { hit = 0; break; }
          }
        }
      }
    } else {
      /* ends */
      if (nn <= hn) {
        if (!icase) {
          hit = (strcmp(hay + (hn - nn), needle) == 0) ? 1 : 0;
        } else {
          size_t j, base = hn - nn;
          hit = 1;
          for (j = 0; j < nn; j++) {
            char ca = hay[base + j], cb = needle[j];
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) { hit = 0; break; }
          }
        }
      }
    }
    var_set_str(vm, "LAST", hay);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hay);
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "HASINF_N", (mode == 0) ? hit : 0);
    var_set_num(vm, "STARTSF_N", (mode == 1) ? hit : 0);
    var_set_num(vm, "ENDSF_N", (mode == 2) ? hit : 0);
    var_set_num(vm, "HASIFI_N", (mode == 0 && icase) ? hit : 0);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* CLEARF|ZEROF|RESETF|DEFAULTF obj field [value]
   * TRYCLEARF|CLEARF SOFT — soft miss OK=0.
   * CLEARF — wipe to "" (str class/cur) or 0 (num).
   * RESETF — restore CLASS field default (NEW init value).
   * DEFAULTF field value — set only if empty/blank (str) or 0 (num).
   * LAST = result; LAST_N = 1 if wrote, 0 if DEFAULTF skipped.
   * Usability: clean/reset/fill-if-missing without GETF+IF+SETF (METHOD/THIS). */
  if (kw(&L->cur, "CLEARF") || kw(&L->cur, "ZEROF") ||
      kw(&L->cur, "WIPEF") || kw(&L->cur, "EMPTYSETF") ||
      kw(&L->cur, "CLEAREIELD") || kw(&L->cur, "CLEARFIELD") ||
      kw(&L->cur, "RESETF") || kw(&L->cur, "RESTOREF") ||
      kw(&L->cur, "DEFAULTF") || kw(&L->cur, "DEFAULTFIELD") ||
      kw(&L->cur, "FILLF") || kw(&L->cur, "IFNULLF") ||
      kw(&L->cur, "TRYCLEARF") || kw(&L->cur, "CLEARFSOFT") ||
      kw(&L->cur, "SOFTCLEARF") || kw(&L->cur, "TRYRESETF") ||
      kw(&L->cur, "TRYDEFAULTF")) {
    char oname[48], fname[48], op[24], sval[256];
    ObjInst *ob;
    ClassDef *cd;
    FieldDef *fd;
    int fi, soft = 0, mode = 0; /* 0=clear, 1=reset, 2=default */
    int wrote = 0, want_str = 0;
    long nval = 0;
    const char *p;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "RESETF") == 0 || strcmp(op, "RESTOREF") == 0 ||
        strcmp(op, "TRYRESETF") == 0)
      mode = 1;
    else if (strcmp(op, "DEFAULTF") == 0 || strcmp(op, "DEFAULTFIELD") == 0 ||
             strcmp(op, "FILLF") == 0 || strcmp(op, "IFNULLF") == 0 ||
             strcmp(op, "TRYDEFAULTF") == 0)
      mode = 2;
    else
      mode = 0; /* CLEARF */
    if (strcmp(op, "TRYCLEARF") == 0 || strcmp(op, "CLEARFSOFT") == 0 ||
        strcmp(op, "SOFTCLEARF") == 0 || strcmp(op, "TRYRESETF") == 0 ||
        strcmp(op, "TRYDEFAULTF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "CLEARF object field"); return -1;
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
      fail(vm, "CLEARF field"); return -1;
    }
    sval[0] = 0;
    want_str = 0;
    nval = 0;
    if (mode == 2) {
      /* DEFAULTF requires value */
      if (kw(&L->cur, "WITH") || kw(&L->cur, "TO") || kw(&L->cur, "AS") ||
          kw(&L->cur, "BY") || kw(&L->cur, "=") || L->cur.kind == TK_EQ) {
        if (L->cur.kind == TK_EQ) lex_next(L);
        else lex_next(L);
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sval, sizeof sval, "%s", L->cur.text);
        want_str = 1;
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(sval, sizeof sval, "%s", vm->last_str);
        want_str = 1;
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *sv = var_get(vm, L->cur.text, 0);
        if (sv && sv->is_str) {
          snprintf(sval, sizeof sval, "%s", sv->sval);
          want_str = 1;
          lex_next(L);
        } else {
          nval = parse_expr(vm, L);
          want_str = 0;
        }
      } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                 L->cur.kind == TK_LPAREN) {
        nval = parse_expr(vm, L);
        want_str = 0;
      } else {
        fail(vm, "DEFAULTF object field value"); return -1;
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "CLEARF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "CLEARF_N", 0);
      var_set_num(vm, "RESETF_N", 0);
      var_set_num(vm, "DEFAULTF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CLEARF: unknown object");
      var_set_str(vm, "ERR", "CLEARF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "CLEARF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "CLEARF_N", 0);
      var_set_num(vm, "RESETF_N", 0);
      var_set_num(vm, "DEFAULTF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CLEARF: unknown field");
      var_set_str(vm, "ERR", "CLEARF: unknown field");
      bump(vm);
      return 1;
    }
    fd = &cd->fields[fi];
    wrote = 0;
    if (mode == 1) {
      /* RESETF → class default */
      if (fd->has_def && fd->is_str) {
        snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", fd->def_str);
        ob->fis_str[fi] = 1;
      } else if (fd->has_def) {
        ob->fnum[fi] = fd->def_num;
        ob->fis_str[fi] = 0;
      } else if (fd->is_str) {
        ob->fstr[fi][0] = 0;
        ob->fis_str[fi] = 1;
      } else {
        ob->fnum[fi] = 0;
        ob->fis_str[fi] = 0;
      }
      wrote = 1;
    } else if (mode == 2) {
      /* DEFAULTF — only if empty/blank or zero */
      int is_empty = 0;
      if (ob->fis_str[fi]) {
        p = ob->fstr[fi];
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        is_empty = (*p == 0);
      } else {
        is_empty = (ob->fnum[fi] == 0);
      }
      if (is_empty) {
        if (want_str) {
          snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", sval);
          ob->fis_str[fi] = 1;
        } else {
          ob->fnum[fi] = nval;
          ob->fis_str[fi] = 0;
        }
        wrote = 1;
      }
    } else {
      /* CLEARF — wipe */
      if (fd->is_str || ob->fis_str[fi]) {
        ob->fstr[fi][0] = 0;
        ob->fis_str[fi] = 1;
      } else {
        ob->fnum[fi] = 0;
        ob->fis_str[fi] = 0;
      }
      wrote = 1;
    }
    if (ob->fis_str[fi]) {
      var_set_str(vm, "LAST", ob->fstr[fi]);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->fstr[fi]);
      vm->last_n = wrote ? 1 : 0;
      if (mode != 2)
        vm->last_n = (long)strlen(ob->fstr[fi]);
    } else {
      char nb[24];
      snprintf(nb, sizeof nb, "%ld", ob->fnum[fi]);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      vm->last_n = (mode == 2) ? (wrote ? 1 : 0) : ob->fnum[fi];
    }
    /* DEFAULTF: LAST_N always 0|1 applied; CLEARF length/value; RESETF too */
    if (mode == 2)
      var_set_num(vm, "LAST_N", wrote ? 1 : 0);
    else if (ob->fis_str[fi])
      var_set_num(vm, "LAST_N", (long)strlen(ob->fstr[fi]));
    else
      var_set_num(vm, "LAST_N", ob->fnum[fi]);
    vm->last_n = (mode == 2) ? (wrote ? 1 : 0)
                             : (ob->fis_str[fi] ? (long)strlen(ob->fstr[fi])
                                                : ob->fnum[fi]);
    var_set_num(vm, "CLEARF_N", (mode == 0 && wrote) ? 1 : 0);
    var_set_num(vm, "RESETF_N", (mode == 1 && wrote) ? 1 : 0);
    var_set_num(vm, "DEFAULTF_N", (mode == 2) ? (wrote ? 1 : 0) : 0);
    var_set_num(vm, "DEFAULTF_APPLIED", (mode == 2 && wrote) ? 1 : 0);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* EQF|NEQF|EQFI obj field value
   * FINDF|FINDFI obj field needle — index of first needle or -1.
   * TRYEQF|EQF SOFT — soft miss OK=0.
   * Exact equality / inequality / locate without GETF+SYS EQS/FIND.
   * LAST = field text; LAST_N = 0|1 (EQ*) or index|-1 (FIND*).
   * Usability: IF status=="open" / locate error offset (METHOD/THIS;
   * complements HASINF contains probes). */
  if (kw(&L->cur, "EQF") || kw(&L->cur, "EQFIELD") ||
      kw(&L->cur, "FIELDEQ") || kw(&L->cur, "SAMEF") ||
      kw(&L->cur, "NEQF") || kw(&L->cur, "NEFIELD") ||
      kw(&L->cur, "FIELDNE") || kw(&L->cur, "DIFFF") ||
      kw(&L->cur, "EQFI") || kw(&L->cur, "IEQF") ||
      kw(&L->cur, "FIELDEQI") || kw(&L->cur, "SAMEFI") ||
      kw(&L->cur, "FINDF") || kw(&L->cur, "INDEXF") ||
      kw(&L->cur, "FIELDFIND") || kw(&L->cur, "STRINDF") ||
      kw(&L->cur, "FINDFI") || kw(&L->cur, "IFINDF") ||
      kw(&L->cur, "INDEXFI") || kw(&L->cur, "FIELDFINDI") ||
      kw(&L->cur, "TRYEQF") || kw(&L->cur, "EQFSOFT") ||
      kw(&L->cur, "SOFTEQF") || kw(&L->cur, "TRYNEQF") ||
      kw(&L->cur, "TRYFINDF") || kw(&L->cur, "TRYFINDFI")) {
    char oname[48], fname[48], op[24], hay[256], rhs[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=eq, 1=ne, 2=find */
    int icase = 0;
    long hit = 0;
    size_t hn, nn, i, j;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "NEQF") == 0 || strcmp(op, "NEFIELD") == 0 ||
        strcmp(op, "FIELDNE") == 0 || strcmp(op, "DIFFF") == 0 ||
        strcmp(op, "TRYNEQF") == 0)
      mode = 1;
    else if (strcmp(op, "FINDF") == 0 || strcmp(op, "INDEXF") == 0 ||
             strcmp(op, "FIELDFIND") == 0 || strcmp(op, "STRINDF") == 0 ||
             strcmp(op, "FINDFI") == 0 || strcmp(op, "IFINDF") == 0 ||
             strcmp(op, "INDEXFI") == 0 || strcmp(op, "FIELDFINDI") == 0 ||
             strcmp(op, "TRYFINDF") == 0 || strcmp(op, "TRYFINDFI") == 0)
      mode = 2;
    else
      mode = 0; /* EQF */
    if (strcmp(op, "EQFI") == 0 || strcmp(op, "IEQF") == 0 ||
        strcmp(op, "FIELDEQI") == 0 || strcmp(op, "SAMEFI") == 0 ||
        strcmp(op, "FINDFI") == 0 || strcmp(op, "IFINDF") == 0 ||
        strcmp(op, "INDEXFI") == 0 || strcmp(op, "FIELDFINDI") == 0 ||
        strcmp(op, "TRYFINDFI") == 0)
      icase = 1;
    if (strcmp(op, "TRYEQF") == 0 || strcmp(op, "EQFSOFT") == 0 ||
        strcmp(op, "SOFTEQF") == 0 || strcmp(op, "TRYNEQF") == 0 ||
        strcmp(op, "TRYFINDF") == 0 || strcmp(op, "TRYFINDFI") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (!icase && (kw(&L->cur, "I") || kw(&L->cur, "ICASE") ||
                   kw(&L->cur, "IGNORECASE") || kw(&L->cur, "-I") ||
                   kw(&L->cur, "CI"))) {
      icase = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "EQF object field value"); return -1;
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
      fail(vm, "EQF field"); return -1;
    }
    if (kw(&L->cur, "WITH") || kw(&L->cur, "TO") || kw(&L->cur, "AS") ||
        kw(&L->cur, "EQ") || kw(&L->cur, "IS") || L->cur.kind == TK_EQ ||
        L->cur.kind == TK_EQEQ) {
      if (L->cur.kind == TK_EQ || L->cur.kind == TK_EQEQ) lex_next(L);
      else lex_next(L);
    }
    rhs[0] = 0;
    if (resolve_str_arg(vm, L, rhs, sizeof rhs) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(rhs, sizeof rhs, "%ld", v);
      } else {
        fail(vm, "EQF object field value"); return -1;
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "EQF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", (mode == 2) ? -1 : 0);
      vm->last_n = (mode == 2) ? -1 : 0;
      var_set_num(vm, "EQF_N", 0);
      var_set_num(vm, "FINDF_N", -1);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "EQF: unknown object");
      var_set_str(vm, "ERR", "EQF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "EQF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", (mode == 2) ? -1 : 0);
      vm->last_n = (mode == 2) ? -1 : 0;
      var_set_num(vm, "EQF_N", 0);
      var_set_num(vm, "FINDF_N", -1);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "EQF: unknown field");
      var_set_str(vm, "ERR", "EQF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    hn = strlen(hay);
    nn = strlen(rhs);
    if (mode == 2) {
      /* FINDF → first index or -1 */
      hit = -1;
      if (nn == 0) {
        hit = 0;
      } else if (!icase) {
        const char *p = strstr(hay, rhs);
        if (p) hit = (long)(p - hay);
      } else {
        for (i = 0; i + nn <= hn; i++) {
          int ok = 1;
          for (j = 0; j < nn; j++) {
            char ca = hay[i + j], cb = rhs[j];
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) { ok = 0; break; }
          }
          if (ok) { hit = (long)i; break; }
        }
      }
    } else {
      /* EQ / NE */
      if (!icase) {
        hit = (strcmp(hay, rhs) == 0) ? 1 : 0;
      } else {
        hit = 1;
        for (i = 0; ; i++) {
          char ca = hay[i], cb = rhs[i];
          if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
          if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
          if (ca != cb) { hit = 0; break; }
          if (!hay[i]) break;
        }
      }
      if (mode == 1) hit = hit ? 0 : 1;
    }
    var_set_str(vm, "LAST", hay);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hay);
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "EQF_N", (mode == 0) ? hit : 0);
    var_set_num(vm, "NEQF_N", (mode == 1) ? hit : 0);
    var_set_num(vm, "EQFI_N", (mode == 0 && icase) ? hit : 0);
    var_set_num(vm, "FINDF_N", (mode == 2) ? hit : -1);
    var_set_num(vm, "FINDF_I", (mode == 2) ? hit : -1);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LEFTF|RIGHTF|SLICEF|SUBSTRF|TRUNCF obj field n [count]
   * TRYLEFTF|LEFTF SOFT — soft miss OK=0.
   * In-place string slice on a field (promotes num→str).
   * LEFTF f n — keep first n; RIGHTF f n — keep last n;
   * SLICEF f start count — window; TRUNCF f max — keep first max (clip).
   * LAST = result; LAST_N = length. Note: MIDF/CLIPF stay fleet median/clamp.
   * Usability: peel/clip note after FINDF without GETF+MID/LEFT+SETF
   * (METHOD/THIS; pairs REPLACEF/TRIMF). */
  if (kw(&L->cur, "LEFTF") || kw(&L->cur, "KEEPLEFTF") ||
      kw(&L->cur, "TAKELEFTF") || kw(&L->cur, "FIELDLEFT") ||
      kw(&L->cur, "RIGHTF") || kw(&L->cur, "KEEPRIGHTF") ||
      kw(&L->cur, "TAKERIGHTF") || kw(&L->cur, "FIELDRIGHT") ||
      kw(&L->cur, "SLICEF") || kw(&L->cur, "SUBSTRF") ||
      kw(&L->cur, "MIDSTRF") || kw(&L->cur, "FIELDMID") ||
      kw(&L->cur, "SLICEFIELD") || kw(&L->cur, "SUBSTRFIELD") ||
      kw(&L->cur, "TRUNCF") || kw(&L->cur, "TRUNCFIELD") ||
      kw(&L->cur, "STRCLIPF") || kw(&L->cur, "CLIPSTRF") ||
      kw(&L->cur, "MAXLENF") ||
      kw(&L->cur, "TRYLEFTF") || kw(&L->cur, "LEFTFSOFT") ||
      kw(&L->cur, "SOFTLEFTF") || kw(&L->cur, "TRYRIGHTF") ||
      kw(&L->cur, "TRYSLICEF") || kw(&L->cur, "TRYTRUNCF")) {
    char oname[48], fname[48], op[24], hay[256], out[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=left, 1=right, 2=slice, 3=trunc */
    long n1 = 0, n2 = -1;
    size_t hn, start, count, i;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "RIGHTF") == 0 || strcmp(op, "KEEPRIGHTF") == 0 ||
        strcmp(op, "TAKERIGHTF") == 0 || strcmp(op, "FIELDRIGHT") == 0 ||
        strcmp(op, "TRYRIGHTF") == 0)
      mode = 1;
    else if (strcmp(op, "SLICEF") == 0 || strcmp(op, "SUBSTRF") == 0 ||
             strcmp(op, "MIDSTRF") == 0 || strcmp(op, "FIELDMID") == 0 ||
             strcmp(op, "SLICEFIELD") == 0 || strcmp(op, "SUBSTRFIELD") == 0 ||
             strcmp(op, "TRYSLICEF") == 0)
      mode = 2;
    else if (strcmp(op, "TRUNCF") == 0 || strcmp(op, "TRUNCFIELD") == 0 ||
             strcmp(op, "STRCLIPF") == 0 || strcmp(op, "CLIPSTRF") == 0 ||
             strcmp(op, "MAXLENF") == 0 || strcmp(op, "TRYTRUNCF") == 0)
      mode = 3;
    else
      mode = 0; /* LEFTF */
    if (strcmp(op, "TRYLEFTF") == 0 || strcmp(op, "LEFTFSOFT") == 0 ||
        strcmp(op, "SOFTLEFTF") == 0 || strcmp(op, "TRYRIGHTF") == 0 ||
        strcmp(op, "TRYSLICEF") == 0 || strcmp(op, "TRYTRUNCF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "LEFTF object field n"); return -1;
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
      fail(vm, "LEFTF field"); return -1;
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "OF") || kw(&L->cur, "LEN") ||
        kw(&L->cur, "WIDTH") || kw(&L->cur, "N") || kw(&L->cur, "BY"))
      lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
      n1 = parse_expr(vm, L);
    } else {
      fail(vm, "LEFTF object field n"); return -1;
    }
    if (mode == 2) {
      if (kw(&L->cur, "FOR") || kw(&L->cur, "COUNT") || kw(&L->cur, "LEN") ||
          kw(&L->cur, "WIDTH") || kw(&L->cur, "TO"))
        lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN ||
          (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
           !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
           !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
           !kw(&L->cur, "END") && !kw(&L->cur, "GETF") &&
           !kw(&L->cur, "SETF"))) {
        n2 = parse_expr(vm, L);
      } else {
        n2 = -1; /* to end */
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "LEFTF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "LEFTF_N", 0);
      var_set_num(vm, "RIGHTF_N", 0);
      var_set_num(vm, "SLICEF_N", 0);
      var_set_num(vm, "TRUNCF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LEFTF: unknown object");
      var_set_str(vm, "ERR", "LEFTF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "LEFTF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "LEFTF_N", 0);
      var_set_num(vm, "RIGHTF_N", 0);
      var_set_num(vm, "SLICEF_N", 0);
      var_set_num(vm, "TRUNCF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LEFTF: unknown field");
      var_set_str(vm, "ERR", "LEFTF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    hn = strlen(hay);
    out[0] = 0;
    if (mode == 0 || mode == 3) {
      /* left / trunc — first n1 chars */
      if (n1 < 0) n1 = 0;
      count = (size_t)n1;
      if (count > hn) count = hn;
      if (count >= sizeof out) count = sizeof out - 1;
      for (i = 0; i < count; i++) out[i] = hay[i];
      out[count] = 0;
    } else if (mode == 1) {
      /* right — last n1 */
      if (n1 < 0) n1 = 0;
      count = (size_t)n1;
      if (count > hn) count = hn;
      start = hn - count;
      if (count >= sizeof out) count = sizeof out - 1;
      for (i = 0; i < count; i++) out[i] = hay[start + i];
      out[count] = 0;
    } else {
      /* slice start n1, count n2 (-1 = to end) */
      if (n1 < 0) n1 = 0;
      start = (size_t)n1;
      if (start > hn) start = hn;
      if (n2 < 0)
        count = hn - start;
      else
        count = (size_t)n2;
      if (start + count > hn) count = hn - start;
      if (count >= sizeof out) count = sizeof out - 1;
      for (i = 0; i < count; i++) out[i] = hay[start + i];
      out[count] = 0;
    }
    snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
    ob->fis_str[fi] = 1;
    {
      size_t ol = strlen(out);
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)ol;
      var_set_num(vm, "LAST_N", (long)ol);
    }
    var_set_num(vm, "LEFTF_N", (mode == 0) ? 1 : 0);
    var_set_num(vm, "RIGHTF_N", (mode == 1) ? 1 : 0);
    var_set_num(vm, "SLICEF_N", (mode == 2) ? 1 : 0);
    var_set_num(vm, "TRUNCF_N", (mode == 3) ? 1 : 0);
    var_set_num(vm, "SUBSTRF_N", (mode == 2) ? 1 : 0);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* BEFOREF|AFTERF|BETWEENF obj field needle [close]
   * TRYBEFOREF|BEFOREF SOFT — soft miss OK=0.
   * In-place delimiter peel (promotes num→str).
   * BEFOREF: keep left of first needle (whole if miss).
   * AFTERF: keep right of first needle (empty if miss).
   * BETWEENF open close: keep interior (empty if pair miss).
   * LAST = result; LAST_N = 1 if delimiter found, 0 soft miss peel.
   * Usability: kv/log peel without GETF+SYS BEFORE/AFTER/BETWEEN+SETF
   * (METHOD/THIS; pairs FINDF/LEFTF count slices). */
  if (kw(&L->cur, "BEFOREF") || kw(&L->cur, "LEFTOFF") ||
      kw(&L->cur, "HEADOFF") || kw(&L->cur, "SPLITLEFTF") ||
      kw(&L->cur, "FIELDBEFORE") ||
      kw(&L->cur, "AFTERF") || kw(&L->cur, "RIGHTOFF") ||
      kw(&L->cur, "TAILOFF") || kw(&L->cur, "SPLITRIGHTF") ||
      kw(&L->cur, "FIELDAFTER") ||
      kw(&L->cur, "BETWEENF") || kw(&L->cur, "MIDOFF") ||
      kw(&L->cur, "EXTRACTF") || kw(&L->cur, "INNERF") ||
      kw(&L->cur, "FIELDBETWEEN") ||
      kw(&L->cur, "TRYBEFOREF") || kw(&L->cur, "BEFOREFSOFT") ||
      kw(&L->cur, "SOFTBEFOREF") || kw(&L->cur, "TRYAFTERF") ||
      kw(&L->cur, "TRYBETWEENF")) {
    char oname[48], fname[48], op[24], hay[256], open[256], close[256], out[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=before, 1=after, 2=between */
    long found = 0;
    const char *po, *start, *pc;
    size_t n;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "AFTERF") == 0 || strcmp(op, "RIGHTOFF") == 0 ||
        strcmp(op, "TAILOFF") == 0 || strcmp(op, "SPLITRIGHTF") == 0 ||
        strcmp(op, "FIELDAFTER") == 0 || strcmp(op, "TRYAFTERF") == 0)
      mode = 1;
    else if (strcmp(op, "BETWEENF") == 0 || strcmp(op, "MIDOFF") == 0 ||
             strcmp(op, "EXTRACTF") == 0 || strcmp(op, "INNERF") == 0 ||
             strcmp(op, "FIELDBETWEEN") == 0 || strcmp(op, "TRYBETWEENF") == 0)
      mode = 2;
    else
      mode = 0; /* BEFORE */
    if (strcmp(op, "TRYBEFOREF") == 0 || strcmp(op, "BEFOREFSOFT") == 0 ||
        strcmp(op, "SOFTBEFOREF") == 0 || strcmp(op, "TRYAFTERF") == 0 ||
        strcmp(op, "TRYBETWEENF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "BEFOREF object field needle"); return -1;
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
      fail(vm, "BEFOREF field"); return -1;
    }
    open[0] = 0;
    close[0] = 0;
    if (resolve_str_arg(vm, L, open, sizeof open) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(open, sizeof open, "%ld", v);
      } else {
        fail(vm, "BEFOREF object field needle"); return -1;
      }
    }
    if (mode == 2) {
      if (kw(&L->cur, "AND") || kw(&L->cur, "TO") || kw(&L->cur, "CLOSE") ||
          kw(&L->cur, "WITH"))
        lex_next(L);
      if (resolve_str_arg(vm, L, close, sizeof close) != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
            L->cur.kind == TK_LPAREN) {
          long v = parse_expr(vm, L);
          snprintf(close, sizeof close, "%ld", v);
        } else {
          fail(vm, "BETWEENF object field open close"); return -1;
        }
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "BEFOREF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "BEFOREF_N", 0);
      var_set_num(vm, "AFTERF_N", 0);
      var_set_num(vm, "BETWEENF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "BEFOREF: unknown object");
      var_set_str(vm, "ERR", "BEFOREF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "BEFOREF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "BEFOREF_N", 0);
      var_set_num(vm, "AFTERF_N", 0);
      var_set_num(vm, "BETWEENF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "BEFOREF: unknown field");
      var_set_str(vm, "ERR", "BEFOREF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    out[0] = 0;
    found = 0;
    if (mode == 2) {
      /* BETWEENF */
      if (open[0] == 0) {
        po = hay;
        start = hay;
      } else {
        po = strstr(hay, open);
        if (!po) {
          found = 0;
          out[0] = 0;
          goto between_done;
        }
        start = po + strlen(open);
      }
      if (close[0] == 0) {
        pc = start + strlen(start);
        found = 1;
      } else {
        pc = strstr(start, close);
        if (!pc) {
          found = 0;
          out[0] = 0;
          goto between_done;
        }
        found = 1;
      }
      n = (size_t)(pc - start);
      if (n >= sizeof out) n = sizeof out - 1;
      memcpy(out, start, n);
      out[n] = 0;
    between_done: ;
    } else if (open[0] == 0) {
      found = 1;
      if (mode == 1)
        snprintf(out, sizeof out, "%s", hay);
      else
        out[0] = 0;
    } else {
      po = strstr(hay, open);
      if (!po) {
        found = 0;
        if (mode == 1)
          out[0] = 0;
        else
          snprintf(out, sizeof out, "%s", hay);
      } else {
        found = 1;
        if (mode == 1) {
          snprintf(out, sizeof out, "%s", po + strlen(open));
        } else {
          n = (size_t)(po - hay);
          if (n >= sizeof out) n = sizeof out - 1;
          memcpy(out, hay, n);
          out[n] = 0;
        }
      }
    }
    snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
    ob->fis_str[fi] = 1;
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = found;
    var_set_num(vm, "LAST_N", found);
    var_set_num(vm, "BEFOREF_N", (mode == 0) ? found : 0);
    var_set_num(vm, "AFTERF_N", (mode == 1) ? found : 0);
    var_set_num(vm, "BETWEENF_N", (mode == 2) ? found : 0);
    var_set_num(vm, "BEFOREF_HIT", found);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* NUMF|INTF|ATOIF obj field — coerce field to numeric (strtol).
   * STRF|ASSTRF|ITOAF obj field — coerce field to decimal string.
   * TRYNUMF|NUMF SOFT — soft miss OK=0.
   * LAST = decimal text always; LAST_N = numeric value.
   * NUMF_OK = 1 if parse consumed digits (partial "12x"→12 OK).
   * Usability: after AFTERF/BETWEENF peel, INCF without GETF+SYS NUM+SETF;
   * dual STRF for CATF/templates (METHOD/THIS). */
  if (kw(&L->cur, "NUMF") || kw(&L->cur, "INTF") ||
      kw(&L->cur, "ATOIF") || kw(&L->cur, "TONUMF") ||
      kw(&L->cur, "FIELDNUM") || kw(&L->cur, "ASNUMF") ||
      kw(&L->cur, "STRF") || kw(&L->cur, "ASSTRF") ||
      kw(&L->cur, "ITOAF") || kw(&L->cur, "TOSTRF") ||
      kw(&L->cur, "FIELDSTR") || kw(&L->cur, "NUMSTRF") ||
      kw(&L->cur, "TRYNUMF") || kw(&L->cur, "NUMFSOFT") ||
      kw(&L->cur, "SOFTNUMF") || kw(&L->cur, "TRYSTRF") ||
      kw(&L->cur, "TRYINTF")) {
    char oname[48], fname[48], op[24], hay[256], buf[40];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, to_str = 0, parse_ok = 0;
    long nval = 0;
    char *endp = NULL;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "STRF") == 0 || strcmp(op, "ASSTRF") == 0 ||
        strcmp(op, "ITOAF") == 0 || strcmp(op, "TOSTRF") == 0 ||
        strcmp(op, "FIELDSTR") == 0 || strcmp(op, "NUMSTRF") == 0 ||
        strcmp(op, "TRYSTRF") == 0)
      to_str = 1;
    if (strcmp(op, "TRYNUMF") == 0 || strcmp(op, "NUMFSOFT") == 0 ||
        strcmp(op, "SOFTNUMF") == 0 || strcmp(op, "TRYSTRF") == 0 ||
        strcmp(op, "TRYINTF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "NUMF object field"); return -1;
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
      fail(vm, "NUMF field"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "NUMF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "NUMF_N", 0);
      var_set_num(vm, "STRF_N", 0);
      var_set_num(vm, "NUMF_OK", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "NUMF: unknown object");
      var_set_str(vm, "ERR", "NUMF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "NUMF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "NUMF_N", 0);
      var_set_num(vm, "STRF_N", 0);
      var_set_num(vm, "NUMF_OK", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "NUMF: unknown field");
      var_set_str(vm, "ERR", "NUMF: unknown field");
      bump(vm);
      return 1;
    }
    if (to_str) {
      /* force decimal string form */
      if (ob->fis_str[fi]) {
        nval = strtol(ob->fstr[fi], &endp, 10);
        parse_ok = (ob->fstr[fi][0] != 0 && endp != ob->fstr[fi]);
        if (parse_ok) {
          /* pure/leading-digit string → normalize decimal */
          snprintf(buf, sizeof buf, "%ld", nval);
          snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", buf);
        } else {
          /* non-numeric: keep text; LAST_N = 0 */
          nval = 0;
        }
      } else {
        nval = ob->fnum[fi];
        parse_ok = 1;
        snprintf(buf, sizeof buf, "%ld", nval);
        snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", buf);
      }
      ob->fis_str[fi] = 1;
      var_set_str(vm, "LAST", ob->fstr[fi]);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->fstr[fi]);
      vm->last_n = nval;
      var_set_num(vm, "LAST_N", nval);
      var_set_num(vm, "STRF_N", 1);
      var_set_num(vm, "NUMF_N", 0);
      var_set_num(vm, "NUMF_OK", parse_ok ? 1 : 0);
    } else {
      /* coerce to numeric */
      if (ob->fis_str[fi]) {
        snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
        endp = NULL;
        nval = strtol(hay, &endp, 10);
        parse_ok = (hay[0] != 0 && endp != hay);
        if (!parse_ok) nval = 0;
      } else {
        nval = ob->fnum[fi];
        parse_ok = 1;
      }
      ob->fnum[fi] = nval;
      ob->fis_str[fi] = 0;
      snprintf(buf, sizeof buf, "%ld", nval);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = nval;
      var_set_num(vm, "LAST_N", nval);
      var_set_num(vm, "NUMF_N", 1);
      var_set_num(vm, "STRF_N", 0);
      var_set_num(vm, "NUMF_OK", parse_ok ? 1 : 0);
      var_set_num(vm, "INTF_N", 1);
    }
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
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

  /* PICKOBJ|RANDOBJ|SAMPLEOBJ [Class]
   * — random live object name (optional class filter). Soft empty → "" / -1.
   * LAST=name; LAST_N/PICKOBJ_I = 0-based index among candidates; PICKOBJ_TOTAL=pool.
   * Uses vm->rng (SEED / CUBALC_SEED). Usability: peer/work sample without LISTOBJS+SYS PICK. */
  if (kw(&L->cur, "PICKOBJ") || kw(&L->cur, "RANDOBJ") ||
      kw(&L->cur, "SAMPLEOBJ") || kw(&L->cur, "ANYOBJ") ||
      kw(&L->cur, "DRAWOBJ") || kw(&L->cur, "LOTTOBJ") ||
      kw(&L->cur, "CHOICEOBJ") || kw(&L->cur, "RANDINST") ||
      kw(&L->cur, "PICKINST") || kw(&L->cur, "SAMPLEINST")) {
    char filt[48], bag_names[CUBALC_MAX_OBJS][48];
    int has_filt = 0, i, n = 0, idx = -1;
    uint32_t x;
    lex_next(L);
    filt[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "FROM")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "PICKOBJ OF Class"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW") &&
               !kw(&L->cur, "CUBE") && oop_find_class(vm, L->cur.text)) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_STR) {
      /* string class name even if not yet defined → empty pool */
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    }
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      snprintf(bag_names[n], sizeof bag_names[n], "%s", ob->name);
      n++;
    }
    if (n <= 0) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "PICKOBJ", "");
      var_set_str(vm, "RANDOBJ", "");
      var_set_str(vm, "OBJECT", "");
      vm->last_str[0] = 0;
      vm->last_n = -1;
      var_set_num(vm, "LAST_N", -1);
      var_set_num(vm, "PICKOBJ_I", -1);
      var_set_num(vm, "PICKOBJ_N", -1);
      var_set_num(vm, "PICKOBJ_TOTAL", 0);
      var_set_num(vm, "PICKOBJ_HIT", 0);
      if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    x = vm->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    idx = (int)(x % (uint32_t)n);
    var_set_str(vm, "LAST", bag_names[idx]);
    var_set_str(vm, "PICKOBJ", bag_names[idx]);
    var_set_str(vm, "RANDOBJ", bag_names[idx]);
    var_set_str(vm, "SAMPLEOBJ", bag_names[idx]);
    var_set_str(vm, "OBJECT", bag_names[idx]);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag_names[idx]);
    vm->last_n = idx;
    var_set_num(vm, "LAST_N", idx);
    var_set_num(vm, "PICKOBJ_I", idx);
    var_set_num(vm, "PICKOBJ_N", idx);
    var_set_num(vm, "PICKOBJ_TOTAL", n);
    var_set_num(vm, "PICKOBJ_HIT", 1);
    var_set_num(vm, "NOBJS", n);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* DRAWNOBJ|SAMPLEKOBJ|PICKNOBJ [Class] k
   * — sample k unique live object names without replacement (partial Fisher–Yates).
   * k<=0 → empty; k>=pool → all shuffled. LAST bag; LAST_N=count; DRAWNOBJ_TOTAL=pool.
   * Uses vm->rng (SEED/CUBALC_SEED). Usability: multi-peer sample without LISTOBJS+DRAWN. */
  if (kw(&L->cur, "DRAWNOBJ") || kw(&L->cur, "SAMPLEKOBJ") ||
      kw(&L->cur, "PICKNOBJ") || kw(&L->cur, "NPICKOBJ") ||
      kw(&L->cur, "TAKERANDOBJ") || kw(&L->cur, "DRAWNINST") ||
      kw(&L->cur, "SAMPLEKINST") || kw(&L->cur, "DRAWKOBJ") ||
      kw(&L->cur, "RSAMPLEOBJ") || kw(&L->cur, "MULTIDRAWOBJ")) {
    char filt[48], bag_names[CUBALC_MAX_OBJS][48], bag[4096], tmp[48];
    int has_filt = 0, i, n = 0, take = 0, out_n = 0;
    long karg = 0;
    uint32_t x;
    size_t o = 0;
    lex_next(L);
    filt[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "FROM")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "DRAWNOBJ OF Class k"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
               L->cur.kind != TK_NUM) {
      /* Class name only if known class and next looks like k */
      if (L->cur.kind == TK_STR) {
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      } else if (oop_find_class(vm, L->cur.text) &&
                 !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
                 !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
                 !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
        char maybe[48];
        snprintf(maybe, sizeof maybe, "%s", L->cur.text);
        lex_next(L);
        /* if next is k (num/ident/TAKE/K) treat maybe as class; else fail later as k */
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
            L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
            kw(&L->cur, "K") || kw(&L->cur, "TAKE") || kw(&L->cur, "COUNT") ||
            kw(&L->cur, "LIMIT") ||
            (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
             !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET"))) {
          snprintf(filt, sizeof filt, "%s", maybe);
          has_filt = 1;
        } else {
          /* put back: treat maybe as k via re-parse impossible; use as k string */
          fail(vm, "DRAWNOBJ [Class] k"); return -1;
        }
      }
    }
    /* optional K|TAKE|COUNT|LIMIT sugar — avoid bare N (steals var n) */
    if (kw(&L->cur, "TAKE") || kw(&L->cur, "COUNT") ||
        kw(&L->cur, "LIMIT") || kw(&L->cur, "SIZE") || kw(&L->cur, "DRAW"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      karg = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      karg = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        karg = atol(sv->sval);
      else
        karg = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      karg = parse_expr(vm, L);
    } else {
      fail(vm, "DRAWNOBJ [Class] k"); return -1;
    }
    if (karg < 0) karg = 0;
    take = (int)karg;
    if (take > CUBALC_MAX_OBJS) take = CUBALC_MAX_OBJS;
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      snprintf(bag_names[n], sizeof bag_names[n], "%s", ob->name);
      n++;
    }
    out_n = take < n ? take : n;
    /* partial Fisher–Yates: shuffle first out_n into front */
    for (i = 0; i < out_n; i++) {
      int r;
      x = vm->rng;
      x ^= x << 13;
      x ^= x >> 17;
      x ^= x << 5;
      if (!x) x = 1;
      vm->rng = x;
      r = i + (int)(x % (uint32_t)(n - i));
      if (r != i) {
        snprintf(tmp, sizeof tmp, "%s", bag_names[i]);
        snprintf(bag_names[i], sizeof bag_names[i], "%s", bag_names[r]);
        snprintf(bag_names[r], sizeof bag_names[r], "%s", tmp);
      }
    }
    for (i = 0; i < out_n; i++) {
      size_t ln = strlen(bag_names[i]);
      if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, bag_names[i], ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "DRAWNOBJ", bag);
    var_set_str(vm, "SAMPLEKOBJ", bag);
    var_set_str(vm, "PICKNOBJ", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = out_n;
    var_set_num(vm, "LAST_N", out_n);
    var_set_num(vm, "DRAWNOBJ_N", out_n);
    var_set_num(vm, "SAMPLEKOBJ_N", out_n);
    var_set_num(vm, "PICKNOBJ_N", out_n);
    var_set_num(vm, "DRAWNOBJ_TOTAL", n);
    var_set_num(vm, "DRAWNOBJ_REQ", karg);
    var_set_num(vm, "NOBJS", n);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SHUFFLEOBJS|SHUFOBJS|PERMUTEOBJS [Class]
   * — Fisher–Yates shuffle of live object names → bag (optional class filter).
   * Soft empty → "". LAST bag; LAST_N/SHUFFLEOBJS_N = count. Uses vm->rng (SEED).
   * Usability: randomize fleet order without LISTOBJS + SYS SHUFFLE glue. */
  if (kw(&L->cur, "SHUFFLEOBJS") || kw(&L->cur, "SHUFOBJS") ||
      kw(&L->cur, "PERMUTEOBJS") || kw(&L->cur, "SHUFFLEOBJ") ||
      kw(&L->cur, "MIXOBJS") || kw(&L->cur, "SCRAMBLEOBJS") ||
      kw(&L->cur, "RANDOBJORDER") || kw(&L->cur, "SHUFINST") ||
      kw(&L->cur, "SHUFFLEINST") || kw(&L->cur, "OBJSHUFFLE")) {
    char filt[48], bag_names[CUBALC_MAX_OBJS][48], bag[4096], tmp[48];
    int has_filt = 0, i, n = 0;
    uint32_t x;
    size_t o = 0;
    lex_next(L);
    filt[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "FROM")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SHUFFLEOBJS OF Class"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW") &&
               !kw(&L->cur, "CUBE") && oop_find_class(vm, L->cur.text)) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_STR) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    }
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      snprintf(bag_names[n], sizeof bag_names[n], "%s", ob->name);
      n++;
    }
    /* Fisher–Yates */
    for (i = n - 1; i > 0; i--) {
      int r;
      x = vm->rng;
      x ^= x << 13;
      x ^= x >> 17;
      x ^= x << 5;
      if (!x) x = 1;
      vm->rng = x;
      r = (int)(x % (uint32_t)(i + 1));
      if (r != i) {
        snprintf(tmp, sizeof tmp, "%s", bag_names[i]);
        snprintf(bag_names[i], sizeof bag_names[i], "%s", bag_names[r]);
        snprintf(bag_names[r], sizeof bag_names[r], "%s", tmp);
      }
    }
    for (i = 0; i < n; i++) {
      size_t ln = strlen(bag_names[i]);
      if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, bag_names[i], ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "SHUFFLEOBJS", bag);
    var_set_str(vm, "SHUFOBJS", bag);
    var_set_str(vm, "PERMUTEOBJS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "SHUFFLEOBJS_N", n);
    var_set_num(vm, "SHUFOBJS_N", n);
    var_set_num(vm, "PERMUTEOBJS_N", n);
    var_set_num(vm, "NOBJS", n);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
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

  /* FIELDINFO|DUMPFIELD|DESCRIBEFIELD [JSON] Class|obj field
   * — field kind/default plate for agent SETF/NEW prep (twin of METHODINFO).
   * Default bag: name/class/kind/has_def/default[/value if live obj].
   * JSON → cubalc.field.v1. Soft OK=0 if class or field missing. */
  if (kw(&L->cur, "FIELDINFO") || kw(&L->cur, "DUMPFIELD") ||
      kw(&L->cur, "DESCRIBEFIELD") || kw(&L->cur, "FIELDDEF") ||
      kw(&L->cur, "INSPECTFIELD") || kw(&L->cur, "FIELDMETA") ||
      kw(&L->cur, "FIELDSCHEMA") || kw(&L->cur, "PROPINFO") ||
      kw(&L->cur, "ATTRINFO") || kw(&L->cur, "MEMBERINFO")) {
    char a[48], fname[48], bag[2048], defbuf[160], valbuf[160];
    ClassDef *cd = NULL;
    FieldDef *fd = NULL;
    ObjInst *ob = NULL;
    int fi = -1, as_json = 0, has_live = 0;
    const char *kind;
    lex_next(L);
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "FIELDINFO [JSON] Class|obj field"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    } else {
      if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      else
        snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    /* optional FIELD|PROP keyword (not NAME — conflicts with field id) */
    if (kw(&L->cur, "FIELD") || kw(&L->cur, "PROP") || kw(&L->cur, "ATTR") ||
        kw(&L->cur, "MEMBER") || kw(&L->cur, "OF"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(fname, sizeof fname, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(fname, sizeof fname, "%s", vm->last_str);
      else
        snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "FIELDINFO [JSON] Class|obj field"); return -1;
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    if (cd) {
      fi = oop_field_idx(cd, fname);
      if (fi >= 0) fd = &cd->fields[fi];
    }
    bag[0] = 0;
    defbuf[0] = 0;
    valbuf[0] = 0;
    if (!cd || !fd) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "FIELDINFO_N", 0);
      var_set_num(vm, "HAS_DEF", 0);
      var_set_num(vm, "IS_STR", 0);
      var_set_str(vm, "FIELDINFO", "");
      var_set_str(vm, "KIND", "");
      var_set_str(vm, "DEFAULT", "");
      var_set_str(vm, "FIELD", fname);
      if (cd) var_set_str(vm, "CLASS", cd->name);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  !cd ? "FIELDINFO: unknown class/obj"
                      : "FIELDINFO: unknown field");
      var_set_str(vm, "ERR",
                  !cd ? "FIELDINFO: unknown class/obj"
                      : "FIELDINFO: unknown field");
      bump(vm);
      return 1;
    }
    if (fd->is_str) kind = "str";
    else if (fd->has_def) kind = "num";
    else kind = "num"; /* unset defaults act as numeric 0 on NEW */
    if (fd->has_def) {
      if (fd->is_str)
        snprintf(defbuf, sizeof defbuf, "%s", fd->def_str);
      else
        snprintf(defbuf, sizeof defbuf, "%ld", fd->def_num);
    }
    if (ob && fi >= 0 && fi < CUBALC_MAX_FIELDS) {
      has_live = 1;
      if (ob->fis_str[fi])
        snprintf(valbuf, sizeof valbuf, "%s", ob->fstr[fi]);
      else
        snprintf(valbuf, sizeof valbuf, "%ld", ob->fnum[fi]);
    }
    if (as_json) {
      if (has_live) {
        if (ob->fis_str[fi])
          snprintf(bag, sizeof bag,
                   "{\"schema\":\"cubalc.field.v1\",\"class\":\"%s\","
                   "\"name\":\"%s\",\"kind\":\"%s\",\"has_def\":%d,"
                   "\"is_str\":%d,\"default\":\"%s\",\"value\":\"%s\"}",
                   cd->name, fd->name, kind, fd->has_def ? 1 : 0,
                   fd->is_str ? 1 : 0, defbuf, valbuf);
        else
          snprintf(bag, sizeof bag,
                   "{\"schema\":\"cubalc.field.v1\",\"class\":\"%s\","
                   "\"name\":\"%s\",\"kind\":\"%s\",\"has_def\":%d,"
                   "\"is_str\":%d,\"default\":%s,\"value\":%s}",
                   cd->name, fd->name, kind, fd->has_def ? 1 : 0,
                   fd->is_str ? 1 : 0,
                   fd->has_def ? defbuf : "null",
                   valbuf);
      } else if (fd->is_str) {
        snprintf(bag, sizeof bag,
                 "{\"schema\":\"cubalc.field.v1\",\"class\":\"%s\","
                 "\"name\":\"%s\",\"kind\":\"%s\",\"has_def\":%d,"
                 "\"is_str\":1,\"default\":\"%s\"}",
                 cd->name, fd->name, kind, fd->has_def ? 1 : 0, defbuf);
      } else {
        snprintf(bag, sizeof bag,
                 "{\"schema\":\"cubalc.field.v1\",\"class\":\"%s\","
                 "\"name\":\"%s\",\"kind\":\"%s\",\"has_def\":%d,"
                 "\"is_str\":0,\"default\":%s}",
                 cd->name, fd->name, kind, fd->has_def ? 1 : 0,
                 fd->has_def ? defbuf : "null");
      }
    } else {
      if (has_live)
        snprintf(bag, sizeof bag,
                 "name:%s\nclass:%s\nkind:%s\nhas_def:%d\nis_str:%d\n"
                 "default:%s\nvalue:%s",
                 fd->name, cd->name, kind, fd->has_def ? 1 : 0,
                 fd->is_str ? 1 : 0, defbuf, valbuf);
      else
        snprintf(bag, sizeof bag,
                 "name:%s\nclass:%s\nkind:%s\nhas_def:%d\nis_str:%d\n"
                 "default:%s",
                 fd->name, cd->name, kind, fd->has_def ? 1 : 0,
                 fd->is_str ? 1 : 0, defbuf);
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "FIELDINFO", bag);
    var_set_str(vm, "DUMPFIELD", bag);
    var_set_str(vm, "DESCRIBEFIELD", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = fd->has_def ? 1 : 0;
    var_set_num(vm, "LAST_N", fd->has_def ? 1 : 0);
    var_set_num(vm, "FIELDINFO_N", 1);
    var_set_num(vm, "HAS_DEF", fd->has_def ? 1 : 0);
    var_set_num(vm, "IS_STR", fd->is_str ? 1 : 0);
    var_set_str(vm, "FIELD", fd->name);
    var_set_str(vm, "CLASS", cd->name);
    var_set_str(vm, "KIND", kind);
    var_set_str(vm, "DEFAULT", defbuf);
    if (has_live) var_set_str(vm, "VALUE", valbuf);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* CLASSINFO|DUMPCLASS|DESCRIBECLASS [JSON] Class|obj
   * — one-shot class schema plate for agents (fields + methods + live count).
   * Default: key:value bag (LOOKUP-ready). JSON|ASJSON → cubalc.class.v1.
   * Soft OK=0 if unknown. Usability: no LISTFIELDS+LISTMETHODS+COUNTOBJ glue. */
  if (kw(&L->cur, "CLASSINFO") || kw(&L->cur, "DUMPCLASS") ||
      kw(&L->cur, "DESCRIBECLASS") || kw(&L->cur, "CLASSSCHEMA") ||
      kw(&L->cur, "INSPECTCLASS") || kw(&L->cur, "SCHEMACLASS") ||
      kw(&L->cur, "CLASSMETA") || kw(&L->cur, "TYPEINFO") ||
      kw(&L->cur, "DESCRIBE") || kw(&L->cur, "CLASSDEF")) {
    char a[48], bag[4096], fbag[1024], mbag[1024];
    ClassDef *cd = NULL;
    ObjInst *ob;
    size_t o = 0, fo = 0, mo = 0;
    int i, n_live = 0, as_json = 0;
    /* DESCRIBE alone may be too broad — keep as alias */
    lex_next(L);
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "CLASSINFO [JSON] Class|obj"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    } else {
      if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      else
        snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    bag[0] = 0;
    fbag[0] = 0;
    mbag[0] = 0;
    if (!cd) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "NFIELDS", 0);
      var_set_num(vm, "NMETHODS", 0);
      var_set_num(vm, "CLASSINFO_N", 0);
      var_set_num(vm, "CLASSINFO_LIVE", 0);
      var_set_str(vm, "CLASSINFO", "");
      var_set_str(vm, "DUMPCLASS", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CLASSINFO: unknown class/obj");
      var_set_str(vm, "ERR", "CLASSINFO: unknown class/obj");
      bump(vm);
      return 1;
    }
    for (i = 0; i < vm->n_objs; i++) {
      if (vm->objs[i].live && vm->objs[i].class_idx >= 0 &&
          vm->objs[i].class_idx < vm->n_classes &&
          &vm->classes[vm->objs[i].class_idx] == cd)
        n_live++;
    }
    /* field/method name bags (comma-joined for compact plate) */
    for (i = 0; i < cd->n_fields; i++) {
      size_t ln = strlen(cd->fields[i].name);
      if (i > 0 && fo + 1 < sizeof fbag) fbag[fo++] = ',';
      if (fo + ln < sizeof fbag) {
        memcpy(fbag + fo, cd->fields[i].name, ln);
        fo += ln;
      }
      fbag[fo] = 0;
    }
    for (i = 0; i < cd->n_methods; i++) {
      size_t ln = strlen(cd->methods[i].name);
      if (i > 0 && mo + 1 < sizeof mbag) mbag[mo++] = ',';
      if (mo + ln < sizeof mbag) {
        memcpy(mbag + mo, cd->methods[i].name, ln);
        mo += ln;
      }
      mbag[mo] = 0;
    }
    if (as_json) {
      o = (size_t)snprintf(
          bag, sizeof bag,
          "{\"schema\":\"cubalc.class.v1\",\"name\":\"%s\",\"role\":\"%s\","
          "\"n_fields\":%d,\"n_methods\":%d,\"live\":%d,\"fields\":[",
          cd->name, cd->role[0] ? cd->role : "", cd->n_fields, cd->n_methods,
          n_live);
      for (i = 0; i < cd->n_fields && o + 8 < sizeof bag; i++) {
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = ',';
        o += (size_t)snprintf(bag + o, sizeof bag - o, "\"%s\"",
                              cd->fields[i].name);
      }
      if (o + 16 < sizeof bag)
        o += (size_t)snprintf(bag + o, sizeof bag - o, "],\"methods\":[");
      for (i = 0; i < cd->n_methods && o + 8 < sizeof bag; i++) {
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = ',';
        o += (size_t)snprintf(bag + o, sizeof bag - o, "\"%s\"",
                              cd->methods[i].name);
      }
      if (o + 3 < sizeof bag) {
        bag[o++] = ']';
        bag[o++] = '}';
        bag[o] = 0;
      }
    } else {
      o = (size_t)snprintf(
          bag, sizeof bag,
          "name:%s\nrole:%s\nn_fields:%d\nn_methods:%d\nlive:%d\nfields:%s\n"
          "methods:%s",
          cd->name, cd->role[0] ? cd->role : "", cd->n_fields, cd->n_methods,
          n_live, fbag, mbag);
      if (o >= sizeof bag) bag[sizeof bag - 1] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "CLASSINFO", bag);
    var_set_str(vm, "DUMPCLASS", bag);
    var_set_str(vm, "DESCRIBECLASS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = cd->n_fields;
    var_set_num(vm, "LAST_N", cd->n_fields);
    var_set_num(vm, "NFIELDS", cd->n_fields);
    var_set_num(vm, "NMETHODS", cd->n_methods);
    var_set_num(vm, "CLASSINFO_N", cd->n_fields);
    var_set_num(vm, "CLASSINFO_LIVE", n_live);
    var_set_num(vm, "CLASSINFO_METHODS", cd->n_methods);
    var_set_str(vm, "CLASS", cd->name);
    var_set_str(vm, "FIELDS", fbag);
    var_set_str(vm, "METHODS", mbag);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* METHODINFO|DUMPMETHOD|DESCRIBEMETHOD [JSON] Class|obj method
   * — method arity/params plate for agent SEND prep.
   * Default bag: name/class/n_params/params. JSON → cubalc.method.v1.
   * Soft OK=0 if class or method missing. Complements HASMETHOD + CLASSINFO. */
  if (kw(&L->cur, "METHODINFO") || kw(&L->cur, "DUMPMETHOD") ||
      kw(&L->cur, "DESCRIBEMETHOD") || kw(&L->cur, "METHINFO") ||
      kw(&L->cur, "INSPECTMETHOD") || kw(&L->cur, "METHODMETA") ||
      kw(&L->cur, "ARITY") || kw(&L->cur, "METHODDEF") ||
      kw(&L->cur, "METHODSCHEMA") || kw(&L->cur, "PARAMINFO")) {
    char a[48], mname[48], bag[2048], pbag[512];
    ClassDef *cd = NULL;
    MethodDef *md = NULL;
    ObjInst *ob;
    size_t o = 0, po = 0;
    int i, as_json = 0;
    lex_next(L);
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "METHODINFO [JSON] Class|obj method"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    } else {
      if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      else
        snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    /* optional METHOD keyword before name */
    if (kw(&L->cur, "METHOD") || kw(&L->cur, "METH") || kw(&L->cur, "OF"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(mname, sizeof mname, "%s", vm->last_str);
      else
        snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "METHODINFO [JSON] Class|obj method"); return -1;
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    if (cd) md = oop_find_method(cd, mname);
    bag[0] = 0;
    pbag[0] = 0;
    if (!cd || !md) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "NPARAMS", 0);
      var_set_num(vm, "METHODINFO_N", 0);
      var_set_num(vm, "ARITY", 0);
      var_set_str(vm, "METHODINFO", "");
      var_set_str(vm, "PARAMS", "");
      var_set_str(vm, "METHOD", mname);
      if (cd) var_set_str(vm, "CLASS", cd->name);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  !cd ? "METHODINFO: unknown class/obj"
                      : "METHODINFO: unknown method");
      var_set_str(vm, "ERR",
                  !cd ? "METHODINFO: unknown class/obj"
                      : "METHODINFO: unknown method");
      bump(vm);
      return 1;
    }
    for (i = 0; i < md->n_params; i++) {
      size_t ln = strlen(md->params[i]);
      if (i > 0 && po + 1 < sizeof pbag) pbag[po++] = ',';
      if (po + ln < sizeof pbag) {
        memcpy(pbag + po, md->params[i], ln);
        po += ln;
      }
      pbag[po] = 0;
    }
    if (as_json) {
      o = (size_t)snprintf(
          bag, sizeof bag,
          "{\"schema\":\"cubalc.method.v1\",\"class\":\"%s\",\"name\":\"%s\","
          "\"n_params\":%d,\"params\":[",
          cd->name, md->name, md->n_params);
      for (i = 0; i < md->n_params && o + 8 < sizeof bag; i++) {
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = ',';
        o += (size_t)snprintf(bag + o, sizeof bag - o, "\"%s\"", md->params[i]);
      }
      if (o + 3 < sizeof bag) {
        bag[o++] = ']';
        bag[o++] = '}';
        bag[o] = 0;
      }
    } else {
      o = (size_t)snprintf(
          bag, sizeof bag,
          "name:%s\nclass:%s\nn_params:%d\nparams:%s",
          md->name, cd->name, md->n_params, pbag);
      if (o >= sizeof bag) bag[sizeof bag - 1] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "METHODINFO", bag);
    var_set_str(vm, "DUMPMETHOD", bag);
    var_set_str(vm, "DESCRIBEMETHOD", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = md->n_params;
    var_set_num(vm, "LAST_N", md->n_params);
    var_set_num(vm, "NPARAMS", md->n_params);
    var_set_num(vm, "METHODINFO_N", md->n_params);
    var_set_num(vm, "ARITY", md->n_params);
    var_set_str(vm, "METHOD", md->name);
    var_set_str(vm, "CLASS", cd->name);
    var_set_str(vm, "PARAMS", pbag);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* FNINFO|DUMPFN|DESCRIBEFN [JSON] name
   * — FN arity/params plate for agent CALL prep (twin of METHODINFO for SEND).
   * Default bag: name/n_params/params. JSON → cubalc.fn.v1.
   * Soft OK=0 if FN missing. Sets NPARAMS/ARITY/FNINFO_N, PARAMS, FN. */
  if (kw(&L->cur, "FNINFO") || kw(&L->cur, "DUMPFN") ||
      kw(&L->cur, "DESCRIBEFN") || kw(&L->cur, "FUNCINFO") ||
      kw(&L->cur, "INSPECTFN") || kw(&L->cur, "FNMETA") ||
      kw(&L->cur, "FNDEF") || kw(&L->cur, "FNSCHEMA") ||
      kw(&L->cur, "FUNCTIONINFO") || kw(&L->cur, "FNARITY") ||
      kw(&L->cur, "DESCRIBEFUNC") || kw(&L->cur, "DUMPFUNC")) {
    char fname[48], bag[2048], pbag[512];
    FnDef *fn = NULL;
    size_t o = 0, po = 0;
    int i, as_json = 0;
    lex_next(L);
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    /* optional FN|FUNC keyword before name (not NAME — conflicts with var id) */
    if (kw(&L->cur, "FN") || kw(&L->cur, "FUNC") || kw(&L->cur, "FUNCTION") ||
        kw(&L->cur, "OF"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(fname, sizeof fname, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(fname, sizeof fname, "%s", vm->last_str);
      else
        snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "FNINFO [JSON] name"); return -1;
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    for (i = 0; i < vm->n_fns; i++) {
      if (strcmp(vm->fns[i].name, fname) == 0) {
        fn = &vm->fns[i];
        break;
      }
    }
    bag[0] = 0;
    pbag[0] = 0;
    if (!fn) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "NPARAMS", 0);
      var_set_num(vm, "FNINFO_N", 0);
      var_set_num(vm, "ARITY", 0);
      var_set_str(vm, "FNINFO", "");
      var_set_str(vm, "PARAMS", "");
      var_set_str(vm, "FN", fname);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "FNINFO: unknown FN");
      var_set_str(vm, "ERR", "FNINFO: unknown FN");
      bump(vm);
      return 1;
    }
    for (i = 0; i < fn->n_params; i++) {
      size_t ln = strlen(fn->params[i]);
      if (i > 0 && po + 1 < sizeof pbag) pbag[po++] = ',';
      if (po + ln < sizeof pbag) {
        memcpy(pbag + po, fn->params[i], ln);
        po += ln;
      }
      pbag[po] = 0;
    }
    if (as_json) {
      o = (size_t)snprintf(
          bag, sizeof bag,
          "{\"schema\":\"cubalc.fn.v1\",\"name\":\"%s\","
          "\"n_params\":%d,\"params\":[",
          fn->name, fn->n_params);
      for (i = 0; i < fn->n_params && o + 8 < sizeof bag; i++) {
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = ',';
        o += (size_t)snprintf(bag + o, sizeof bag - o, "\"%s\"", fn->params[i]);
      }
      if (o + 3 < sizeof bag) {
        bag[o++] = ']';
        bag[o++] = '}';
        bag[o] = 0;
      }
    } else {
      o = (size_t)snprintf(
          bag, sizeof bag,
          "name:%s\nn_params:%d\nparams:%s",
          fn->name, fn->n_params, pbag);
      if (o >= sizeof bag) bag[sizeof bag - 1] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "FNINFO", bag);
    var_set_str(vm, "DUMPFN", bag);
    var_set_str(vm, "DESCRIBEFN", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = fn->n_params;
    var_set_num(vm, "LAST_N", fn->n_params);
    var_set_num(vm, "NPARAMS", fn->n_params);
    var_set_num(vm, "FNINFO_N", fn->n_params);
    var_set_num(vm, "ARITY", fn->n_params);
    var_set_str(vm, "FN", fn->name);
    var_set_str(vm, "PARAMS", pbag);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LISTFNS|FUNCTIONS — newline bag of defined FN names for agent discovery.
   * LAST_N / NFNS = count. Complements FNINFO (per-FN arity) and LISTMETHODS. */
  if (kw(&L->cur, "LISTFNS") || kw(&L->cur, "FUNCTIONS") ||
      kw(&L->cur, "LISTFUNCS") || kw(&L->cur, "FUNCS") ||
      kw(&L->cur, "FNLIST") || kw(&L->cur, "LISTFUNCTIONS") ||
      kw(&L->cur, "FNS") || kw(&L->cur, "DEFS")) {
    char bag[4096];
    size_t o = 0;
    int i, n = 0;
    lex_next(L);
    bag[0] = 0;
    for (i = 0; i < vm->n_fns; i++) {
      size_t ln = strlen(vm->fns[i].name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, vm->fns[i].name, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTFNS", bag);
    var_set_str(vm, "FUNCTIONS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "NFNS", n);
    var_set_num(vm, "LISTFNS_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASFN|HASFUNC name — soft 0|1 probe before CALL.
   * Name may be IDENT, "string", or string-var. Usability: IF without fatal. */
  if (kw(&L->cur, "HASFN") || kw(&L->cur, "HASFUNC") ||
      kw(&L->cur, "HASFUNCTION") || kw(&L->cur, "FN?") ||
      kw(&L->cur, "FUNC?") || kw(&L->cur, "FNEXISTS") ||
      kw(&L->cur, "FUNCEXISTS") || kw(&L->cur, "DEFINEDFN") ||
      kw(&L->cur, "ISFN") || kw(&L->cur, "CANCALLFN")) {
    char fname[48];
    int i, hit = 0;
    lex_next(L);
    /* optional FN|FUNC keyword before name (not NAME — conflicts with var id) */
    if (kw(&L->cur, "FN") || kw(&L->cur, "FUNC") || kw(&L->cur, "FUNCTION") ||
        kw(&L->cur, "OF"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(fname, sizeof fname, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(fname, sizeof fname, "%s", vm->last_str);
      else
        snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "HASFN name"); return -1;
    }
    for (i = 0; i < vm->n_fns; i++) {
      if (strcmp(vm->fns[i].name, fname) == 0) {
        hit = 1;
        break;
      }
    }
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASFN_N", hit);
    var_set_str(vm, "FN", fname);
    var_set_num(vm, "OK", 1);
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

  /* OBJINFO|DESCRIBEOBJ|OBJMETA [JSON] obj
   * — live object plate: name/class/methods + field values in one shot.
   * Complements DUMPOBJ (fields only) + CLASSINFO/LISTMETHODS (schema only).
   * Soft OK=0 if missing. JSON → cubalc.objinfo.v1. */
  if (kw(&L->cur, "OBJINFO") || kw(&L->cur, "DESCRIBEOBJ") ||
      kw(&L->cur, "OBJMETA") || kw(&L->cur, "OBJSCHEMA") ||
      kw(&L->cur, "OBJECTINFO") || kw(&L->cur, "INFOOBJ") ||
      kw(&L->cur, "SUMMARYOBJ") || kw(&L->cur, "OBJSUMMARY") ||
      kw(&L->cur, "SHOWOBJINFO") || kw(&L->cur, "EXPLAINOBJ")) {
    char oname[48], bag[4096], mbag[1024];
    ObjInst *ob;
    ClassDef *cd;
    size_t o = 0, mo = 0;
    int i, n = 0, as_json = 0;
    lex_next(L);
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    /* Use oop_resolve_obj_name: NEW sets var(name)=Class — do not expand that. */
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "OBJINFO [JSON] name"); return -1;
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    ob = oop_find_obj(vm, oname);
    if (!ob || ob->class_idx < 0 || ob->class_idx >= vm->n_classes) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "NFIELDS", 0);
      var_set_num(vm, "NMETHODS", 0);
      var_set_num(vm, "OBJINFO_N", 0);
      var_set_str(vm, "OBJINFO", "");
      var_set_str(vm, "METHODS", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "OBJINFO: unknown object");
      var_set_str(vm, "ERR", "OBJINFO: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    bag[0] = 0;
    mbag[0] = 0;
    for (i = 0; i < cd->n_methods; i++) {
      size_t ln = strlen(cd->methods[i].name);
      if (i > 0 && mo + 1 < sizeof mbag) mbag[mo++] = ',';
      if (mo + ln < sizeof mbag) {
        memcpy(mbag + mo, cd->methods[i].name, ln);
        mo += ln;
      }
      mbag[mo] = 0;
    }
    if (as_json) {
      o = (size_t)snprintf(
          bag, sizeof bag,
          "{\"schema\":\"cubalc.objinfo.v1\",\"name\":\"%s\",\"class\":\"%s\","
          "\"n_fields\":%d,\"n_methods\":%d,\"methods\":[",
          oname, cd->name, cd->n_fields, cd->n_methods);
      for (i = 0; i < cd->n_methods && o + 8 < sizeof bag; i++) {
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = ',';
        o += (size_t)snprintf(bag + o, sizeof bag - o, "\"%s\"",
                              cd->methods[i].name);
      }
      if (o + 16 < sizeof bag)
        o += (size_t)snprintf(bag + o, sizeof bag - o, "],\"fields\":{");
      for (i = 0; i < cd->n_fields && o + 8 < sizeof bag; i++) {
        FieldDef *fd = &cd->fields[i];
        char vb[160];
        size_t need;
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = ',';
        if (ob->fis_str[i]) {
          size_t vi, vo = 0;
          vb[vo++] = '"';
          for (vi = 0; ob->fstr[i][vi] && vo + 3 < sizeof vb; vi++) {
            char c = ob->fstr[i][vi];
            if (c == '"' || c == '\\') { vb[vo++] = '\\'; vb[vo++] = c; }
            else if ((unsigned char)c < 0x20) { /* skip */ }
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
      o = (size_t)snprintf(
          bag, sizeof bag,
          "name:%s\nclass:%s\nn_fields:%d\nn_methods:%d\nmethods:%s",
          oname, cd->name, cd->n_fields, cd->n_methods, mbag);
      if (o >= sizeof bag) o = sizeof bag - 1;
      for (i = 0; i < cd->n_fields; i++) {
        FieldDef *fd = &cd->fields[i];
        char line[192];
        size_t ln;
        if (ob->fis_str[i])
          snprintf(line, sizeof line, "\n%s:%s", fd->name, ob->fstr[i]);
        else
          snprintf(line, sizeof line, "\n%s:%ld", fd->name, ob->fnum[i]);
        ln = strlen(line);
        if (o + ln < sizeof bag) {
          memcpy(bag + o, line, ln);
          o += ln;
          bag[o] = 0;
        }
        n++;
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "OBJINFO", bag);
    var_set_str(vm, "DESCRIBEOBJ", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "NFIELDS", cd->n_fields);
    var_set_num(vm, "NMETHODS", cd->n_methods);
    var_set_num(vm, "OBJINFO_N", n);
    var_set_str(vm, "OBJECT", oname);
    var_set_str(vm, "CLASS", cd->name);
    var_set_str(vm, "METHODS", mbag);
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
  /* CALL name [args] · TRYCALL|CALLSOFT|CALL SOFT — soft miss OK=0.
   * Complements HASFN/FNINFO. Bare CALL still fatal on unknown FN.
   * Usability: agent optional hooks after LISTFNS without fatal CALL. */
  if (kw(&L->cur,"CALL")||kw(&L->cur,"RUNFN")||kw(&L->cur,"DO")||
      kw(&L->cur,"CALLIF")||kw(&L->cur,"CALLNZ")||kw(&L->cur,"CALLZ")||
      kw(&L->cur,"CALLWHEN")||kw(&L->cur,"CALLUNLESS")||
      kw(&L->cur,"TRYCALL")||kw(&L->cur,"CALLSOFT")||kw(&L->cur,"SOFTCALL")||
      kw(&L->cur,"TRYFN")||kw(&L->cur,"TRYRUNFN")||
      kw(&L->cur,"TRYCALLFN")||kw(&L->cur,"CALLTRY")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    char fname[48];
    int mode = 0;
    long cond = 1;
    int do_call = 1;
    int soft = 0;
    FnDef *fn=NULL;
    int i;
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    if (strcmp(op,"TRYCALL")==0 || strcmp(op,"CALLSOFT")==0 ||
        strcmp(op,"SOFTCALL")==0 || strcmp(op,"TRYFN")==0 ||
        strcmp(op,"TRYRUNFN")==0 || strcmp(op,"TRYCALLFN")==0 ||
        strcmp(op,"CALLTRY")==0)
      soft = 1;
    lex_next(L);
    /* CALL SOFT|TRY|OPT name — two-token soft form (like SEND SOFT) */
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (strcmp(op,"CALLIF")==0 || strcmp(op,"CALLNZ")==0 || strcmp(op,"CALLWHEN")==0) mode = 1;
    else if (strcmp(op,"CALLZ")==0 || strcmp(op,"CALLUNLESS")==0) mode = 2;
    if (mode) cond = parse_expr(vm, L);
    if (L->cur.kind == TK_STR) {
      snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(fname, sizeof fname, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(fname, sizeof fname, "%s", vm->last_str);
      else
        snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, soft ? "TRYCALL name" : "CALL name"); return -1;
    }
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
      var_set_num(vm,"TRYCALL_N",0);
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    /* CALL obj method — OOP sugar when first name is a live object.
     * Peek method name without consuming unless method exists (so FN args stay). */
    {
      ObjInst *ob = oop_find_obj(vm, fname);
      if (ob && (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR)) {
        ClassDef *cd = &vm->classes[ob->class_idx];
        char mname[48];
        MethodDef *md;
        if (L->cur.kind == TK_STR)
          snprintf(mname, sizeof mname, "%s", L->cur.text);
        else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(mname, sizeof mname, "%s", vv->sval);
          else
            snprintf(mname, sizeof mname, "%s", L->cur.text);
        }
        md = oop_find_method(cd, mname);
        if (md) {
          lex_next(L);
          oop_bind_args(vm, L, md->params, md->n_params);
          if (oop_run_method(vm, ob, md) < 0) return -1;
          var_set_num(vm, "CALLED", 1);
          var_set_num(vm, "TRYCALL_N", 1);
          var_set_num(vm, "OK", 1);
          bump(vm);
          return 1;
        }
        /* no method — fall through to FN lookup; leave token for args */
      }
    }
    for (i=0;i<vm->n_fns;i++) if (strcmp(vm->fns[i].name,fname)==0){ fn=&vm->fns[i]; break; }
    if (!fn){
      if (soft) {
        while (L->cur.kind == TK_NUM || L->cur.kind == TK_STR ||
               L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
               (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
          if (L->cur.kind == TK_STR) lex_next(L);
          else (void)parse_expr(vm, L);
        }
        var_set_num(vm, "CALLED", 0);
        var_set_num(vm, "TRYCALL_N", 0);
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "CALL: unknown FN");
        var_set_str(vm, "ERR", "CALL: unknown FN");
        var_set_str(vm, "FN", fname);
        bump(vm);
        return 1;
      }
      snprintf(vm->err,sizeof vm->err,"CALL unknown FN %s", fname);
      fail(vm,vm->err); return -1;
    }
    oop_bind_args(vm, L, fn->params, fn->n_params);
    var_set_num(vm, "CALLED", 1);
    var_set_num(vm, "TRYCALL_N", 1);
    var_set_str(vm, "FN", fn->name);
    vm->return_fn = 0;
    {
      Lex fl; lex_init(&fl, fn->body, fn->len);
      if (exec_stmts_until(vm, &fl, "END", NULL)<0) return -1;
    }
    vm->return_fn = 0;
    var_set_num(vm, "OK", 1);
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
