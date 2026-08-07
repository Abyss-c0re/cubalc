/* CubalC lang — lang_ops_flow.c (COP/flow · pure C · cube is SoT)
 * OOP plane: CLASS/FIELD/METHOD/NEW/SEND/GETF/SETF/ISOF
 * Engine plane: SCENE/ENTITY/SPAWN/TICK — game loop rides FLOW
 * Advanced vs C++: composition by PLUG, State Matrix SoT, flow-before-compile
 */
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

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
  if (L->cur.kind != TK_IDENT) return -1;
  if (strcasecmp(L->cur.text, "THIS") == 0 || strcasecmp(L->cur.text, "SELF") == 0) {
    if (!vm->this_obj[0]) return -1;
    snprintf(out, outn, "%s", vm->this_obj);
    lex_next(L);
    return 0;
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
  int fi;
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
  if (vm->n_objs >= CUBALC_MAX_OBJS) {
    fail(vm, "too many objects");
    return -1;
  }
  cd = &vm->classes[ci];
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

  /* NEW ClassName instance [ctor args…] */
  if (kw(&L->cur, "NEW") || kw(&L->cur, "MAKE") || kw(&L->cur, "CREATE")) {
    char cname[48], oname[48];
    lex_next(L);
    if (L->cur.kind != TK_IDENT) { fail(vm, "NEW ClassName"); return -1; }
    snprintf(cname, sizeof cname, "%s", L->cur.text);
    lex_next(L);
    if (L->cur.kind != TK_IDENT) { fail(vm, "NEW instance name"); return -1; }
    snprintf(oname, sizeof oname, "%s", L->cur.text);
    lex_next(L);
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
    if (L->cur.kind != TK_IDENT) { fail(vm, "SPAWN name"); return -1; }
    snprintf(oname, sizeof oname, "%s", L->cur.text);
    lex_next(L);
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

  /* SEND obj method [args] */
  if (kw(&L->cur, "SEND") || kw(&L->cur, "CALLMETHOD") || kw(&L->cur, "INVOKE") ||
      kw(&L->cur, "DOMETHOD") || kw(&L->cur, "MSG") || kw(&L->cur, "EMIT")) {
    char oname[48], mname[48];
    ObjInst *ob;
    ClassDef *cd;
    MethodDef *md;
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "SEND object"); return -1;
    }
    if (L->cur.kind != TK_IDENT) { fail(vm, "SEND method"); return -1; }
    snprintf(mname, sizeof mname, "%s", L->cur.text);
    lex_next(L);
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      snprintf(vm->err, sizeof vm->err, "SEND unknown object %s", oname);
      fail(vm, vm->err); return -1;
    }
    cd = &vm->classes[ob->class_idx];
    md = oop_find_method(cd, mname);
    if (!md) {
      snprintf(vm->err, sizeof vm->err, "SEND unknown METHOD %s.%s", cd->name,
               mname);
      fail(vm, vm->err); return -1;
    }
    oop_bind_args(vm, L, md->params, md->n_params);
    if (oop_run_method(vm, ob, md) < 0) return -1;
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* GETF obj field */
  if (kw(&L->cur, "GETF") || kw(&L->cur, "GETFIELD") || kw(&L->cur, "FIELDGET") ||
      kw(&L->cur, "READF")) {
    char oname[48], fname[48];
    ObjInst *ob;
    ClassDef *cd;
    int fi;
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "GETF object"); return -1;
    }
    if (L->cur.kind != TK_IDENT) { fail(vm, "GETF field"); return -1; }
    snprintf(fname, sizeof fname, "%s", L->cur.text);
    lex_next(L);
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      snprintf(vm->err, sizeof vm->err, "GETF unknown object %s", oname);
      fail(vm, vm->err); return -1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      snprintf(vm->err, sizeof vm->err, "GETF unknown FIELD %s", fname);
      fail(vm, vm->err); return -1;
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
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SETF obj field value */
  if (kw(&L->cur, "SETF") || kw(&L->cur, "SETFIELD") || kw(&L->cur, "FIELDSET") ||
      kw(&L->cur, "PUTF") || kw(&L->cur, "WRITEF")) {
    char oname[48], fname[48];
    ObjInst *ob;
    ClassDef *cd;
    int fi;
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "SETF object"); return -1;
    }
    if (L->cur.kind != TK_IDENT) { fail(vm, "SETF field"); return -1; }
    snprintf(fname, sizeof fname, "%s", L->cur.text);
    lex_next(L);
    if (L->cur.kind == TK_EQ) lex_next(L);
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      snprintf(vm->err, sizeof vm->err, "SETF unknown object %s", oname);
      fail(vm, vm->err); return -1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      snprintf(vm->err, sizeof vm->err, "SETF unknown FIELD %s", fname);
      fail(vm, vm->err); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", L->cur.text);
      ob->fis_str[fi] = 1;
      var_set_str(vm, "LAST", L->cur.text);
      lex_next(L);
    } else {
      long v = parse_expr(vm, L);
      ob->fnum[fi] = v;
      ob->fis_str[fi] = 0;
      var_set_num(vm, "LAST_N", v);
      vm->last_n = v;
    }
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
   * digit-4 control: cell-range iterator binds value to name, IT=index, VAL=value */
  if (kw(&L->cur,"EACH")||kw(&L->cur,"FOREACH")){
    lex_next(L);
    int is_cell = (kw(&L->cur,"CELL")||kw(&L->cur,"CELLS")||kw(&L->cur,"SLOT")||kw(&L->cur,"SLOTS"));
    int is_cube = (kw(&L->cur,"CUBE")||kw(&L->cur,"CUBES"));
    int is_line = (kw(&L->cur,"LINE")||kw(&L->cur,"LINES")||kw(&L->cur,"FIELD")||
                   kw(&L->cur,"FIELDS")||kw(&L->cur,"ROW")||kw(&L->cur,"ROWS")||
                   kw(&L->cur,"ENTRY")||kw(&L->cur,"ENTRIES"));
    if (!is_cell && !is_cube && !is_line){ fail(vm,"EACH CUBE|CELL|LINE as name"); return -1; }
    lex_next(L);
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
