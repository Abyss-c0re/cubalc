/* CubalC lang — lang_ops_flow_class.c
 * CLASS · TYPE · EXTEND · NEW · GETF · SETF · ISOF (A3; scene+send peeled).
 */
#include "lang/cubalc_lang_ops_flow_class.h"
#include "lang/cubalc_lang_ops_flow_send.h"
#include "lang/cubalc_lang_ops_flow_field.h"
#include "lang/cubalc_lang_ops_flow_fget.h"
#include "lang/cubalc_lang_ops_flow_fstr.h"
#include "lang/cubalc_lang_ops_flow_scene.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "cubalc_sot.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_class(VM *vm, Lex *L) {
  /* ---- OOP: CLASS / TYPE … FIELD … METHOD … END ----
   * CLASS Name [EXTEND|EXTENDS|OF|FROM|: Base] … END
   * EXTEND Name … END — multi-file reopen: add/override FIELD/METHOD on existing CLASS.
   * Usability: base in lib A, methods in unit B after INCLUDE — no hollow redefine. */
  if (kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
      kw(&L->cur, "EXTEND") || kw(&L->cur, "EXTENDS")) {
    int is_extend_kw = kw(&L->cur, "EXTEND") || kw(&L->cur, "EXTENDS");
    char cname[48];
    char base_name[48];
    ClassDef *cd;
    ClassDef *base = NULL;
    int reopen = 0;
    int base_idx = -1;
    lex_next(L);
    /* Usability: EXTEND CLASS Greeter … END  ≡  EXTEND Greeter … END */
    if (is_extend_kw && kw(&L->cur, "CLASS"))
      lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail_at(vm, L, is_extend_kw
              ? "EXTEND needs ClassName — EXTEND Greeter … END"
              : "CLASS needs name — CLASS Ticket … END");
      return -1;
    }
    snprintf(cname, sizeof cname, "%s", L->cur.text);
    lex_next(L);
    base_name[0] = 0;
    /* CLASS Child EXTEND Parent | CLASS Child : Parent | CLASS Child OF Parent */
    if (!is_extend_kw &&
        (kw(&L->cur, "EXTEND") || kw(&L->cur, "EXTENDS") ||
         kw(&L->cur, "OF") || kw(&L->cur, "FROM") ||
         kw(&L->cur, "BASE") || kw(&L->cur, "ISA") ||
         (L->cur.kind == TK_COLON))) {
      if (L->cur.kind == TK_COLON) lex_next(L);
      else lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail_at(vm, L, "CLASS EXTEND needs base name — CLASS Child EXTEND Parent");
        return -1;
      }
      snprintf(base_name, sizeof base_name, "%s", L->cur.text);
      lex_next(L);
    }
    skip_nl(L);
    if (is_extend_kw) {
      /* reopen existing class (multi-file link) */
      cd = oop_find_class(vm, cname);
      if (!cd) {
        snprintf(vm->err, sizeof vm->err,
                 "EXTEND unknown CLASS %s — CLASS/INCLUDE first", cname);
        fail(vm, vm->err);
        return -1;
      }
      reopen = 1;
    } else {
      if (vm->n_classes >= CUBALC_MAX_CLASSES) {
        fail(vm, "too many CLASS"); return -1;
      }
      if (oop_find_class(vm, cname)) {
        snprintf(vm->err, sizeof vm->err, "CLASS redefine %s — use EXTEND %s", cname, cname);
        fail(vm, vm->err); return -1;
      }
      if (base_name[0]) {
        base = oop_find_class(vm, base_name);
        if (!base) {
          snprintf(vm->err, sizeof vm->err,
                   "CLASS EXTEND base '%s' missing — INCLUDE/CLASS first", base_name);
          fail(vm, vm->err);
          return -1;
        }
        base_idx = oop_find_class_idx(vm, base_name);
      }
      cd = &vm->classes[vm->n_classes++];
      memset(cd, 0, sizeof *cd);
      snprintf(cd->name, sizeof cd->name, "%s", cname);
      snprintf(cd->role, sizeof cd->role, "body");
      cd->parent_idx = base_idx;
      if (base) {
        if (oop_inherit_flatten(vm, cd, base) < 0) return -1;
      } else {
        cd->parent_idx = -1;
      }
    }
    if (oop_parse_class_body(vm, L, cd, cname) < 0) return -1;
    var_set_str(vm, "CLASS", cname);
    var_set_num(vm, "NFIELDS", cd->n_fields);
    var_set_num(vm, "NMETHODS", cd->n_methods);
    var_set_num(vm, "OK", 1);
    var_set_num(vm, "EXTEND_N", reopen ? 1 : (base_name[0] ? 1 : 0));
    if (base_name[0])
      var_set_str(vm, "BASE", base_name);
    else if (reopen && cd->parent_idx >= 0 && cd->parent_idx < vm->n_classes)
      var_set_str(vm, "BASE", vm->classes[cd->parent_idx].name);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s fields=%d methods=%d parent=%d\n",
              reopen ? "EXTEND" : "CLASS", cname, cd->n_fields, cd->n_methods,
              cd->parent_idx);
    {
      int src_idx = oop_find_class_idx(vm, cname);
      int prop = 0;
      if (src_idx >= 0) {
        prop = oop_propagate_extend(vm, src_idx);
        if (prop < 0) return -1;
      }
      var_set_num(vm, "PROPAGATE_N", prop);
      var_set_num(vm, "EXTEND_PROPAGATE_N", prop);
      if (vm->trace && prop > 0)
        fprintf(vm->trace, "# EXTEND propagate %s -> %d descendant class(es)\n",
                cname, prop);
    }
    bump(vm);
    return 1;
  }

  /* TRYNEW|SOFTNEW|NEW SOFT Class instance [ctor]
   * Soft miss: unknown CLASS / redefine / pool full â OK=0 LAST_ERR, no fatal.
   * Usability: optional plugin classes without dual HASCLASS+IF+NEW glue. */
  if (kw(&L->cur, "TRYNEW") || kw(&L->cur, "SOFTNEW") || kw(&L->cur, "NEWSOFT") ||
      kw(&L->cur, "TRYMAKE") || kw(&L->cur, "TRYCREATE") || kw(&L->cur, "MAYBENEW") ||
      kw(&L->cur, "OPTNEW") || kw(&L->cur, "NEWTRY")) {
    char cname[48], oname[48];
    int rc;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail_at(vm, L, "TRYNEW needs ClassName instance â TRYNEW Ticket t"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(cname, sizeof cname, "%s", L->cur.text); lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      if (oop_find_class(vm, id))
        snprintf(cname, sizeof cname, "%s", id);
      else {
        vv = var_get(vm, id, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(cname, sizeof cname, "%s", vv->sval);
        else
          snprintf(cname, sizeof cname, "%s", id);
      }
    }
    if (oop_read_name(vm, L, oname, sizeof oname, "TRYNEW instance name") < 0)
      return -1;
    rc = oop_new_instance(vm, cname, oname, -1, L, 1, 1);
    if (rc == -1) return -1;
    if (rc == -2) {
      if (vm->trace) fprintf(vm->trace, "# TRYNEW soft miss %s %s\n", cname, oname);
      bump(vm);
      return 1;
    }
    var_set_num(vm, "OK", 1);
    var_set_num(vm, "TRYNEW_N", 1);
    var_set_str(vm, "LAST", oname);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", oname);
    var_set_num(vm, "LAST_N", 1);
    vm->last_n = 1;
    if (vm->trace) fprintf(vm->trace, "# TRYNEW %s %s\n", cname, oname);
    bump(vm);
    return 1;
  }

  /* ENSURENEW|GETORNEW|NEW OR Class instance [ctor]
   * If live instance already exists under name â OK=1 ENSURENEW_N=0 (no re-init).
   * If missing â soft NEW (class miss / pool full â OK=0 LAST_ERR like TRYNEW).
   * Wrong-class live name â soft OK=0 LAST_ERR (no silent retype).
   * Usability: agent boot âneed this objectâ without HASOBJ+IF+NEW glue. */
  if (kw(&L->cur, "ENSURENEW") || kw(&L->cur, "GETORNEW") ||
      kw(&L->cur, "ENSUREOBJ") || kw(&L->cur, "NEEDINSTANCE") ||
      kw(&L->cur, "MAKEIF") || kw(&L->cur, "CREATEIF") ||
      kw(&L->cur, "NEWIFMISS") || kw(&L->cur, "ENSUREINSTANCE")) {
    char cname[48], oname[48];
    ObjInst *ob;
    ClassDef *cd;
    int rc, soft_new = 1;
    char op0[32];
    snprintf(op0, sizeof op0, "%s", L->cur.text);
    lex_next(L);
    /* NEW OR Class â¦ two-token form handled under NEW below; this is one-token ENSURE* */
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail_at(vm, L, "ENSURENEW needs ClassName instance â ENSURENEW Ticket t"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(cname, sizeof cname, "%s", L->cur.text); lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      if (oop_find_class(vm, id))
        snprintf(cname, sizeof cname, "%s", id);
      else {
        vv = var_get(vm, id, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(cname, sizeof cname, "%s", vv->sval);
        else
          snprintf(cname, sizeof cname, "%s", id);
      }
    }
    if (oop_read_name(vm, L, oname, sizeof oname, "ENSURENEW instance name") < 0)
      return -1;
    ob = oop_find_obj(vm, oname);
    if (ob) {
      /* already live */
      if (ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
        cd = &vm->classes[ob->class_idx];
        if (cname[0] && strcmp(cd->name, cname) != 0) {
          char msg[96];
          snprintf(msg, sizeof msg, "ENSURENEW: class mismatch %s is %s not %s",
                   oname, cd->name, cname);
          var_set_str(vm, "LAST_ERR", msg);
          var_set_str(vm, "ERR", msg);
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          var_set_num(vm, "LAST_N", 0);
          vm->last_n = 0;
          var_set_num(vm, "OK", 0);
          var_set_num(vm, "ENSURENEW_N", 0);
          var_set_num(vm, "ENSURED", 0);
          if (vm->trace) fprintf(vm->trace, "# ENSURENEW class mismatch %s\n", oname);
          bump(vm);
          return 1;
        }
        var_set_str(vm, "CLASS", cd->name);
      }
      var_set_str(vm, "LAST", oname);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", oname);
      var_set_str(vm, "OBJECT", oname);
      var_set_num(vm, "LAST_N", 0); /* 0 = already present (created=0) */
      vm->last_n = 0;
      var_set_num(vm, "ENSURENEW_N", 0);
      var_set_num(vm, "ENSURED", 1);
      var_set_num(vm, "OK", 1);
      /* wipe sticky err after successful ensure-hit */
      var_set_str(vm, "LAST_ERR", "");
      var_set_str(vm, "ERR", "");
      if (vm->trace) fprintf(vm->trace, "# ENSURENEW already %s\n", oname);
      bump(vm);
      return 1;
    }
    /* missing â soft NEW */
    (void)soft_new;
    rc = oop_new_instance(vm, cname, oname, -1, L, 1, 1);
    if (rc == -1) return -1;
    if (rc == -2) {
      var_set_num(vm, "ENSURENEW_N", 0);
      var_set_num(vm, "ENSURED", 0);
      if (vm->trace) fprintf(vm->trace, "# ENSURENEW soft miss %s %s\n", cname, oname);
      bump(vm);
      return 1;
    }
    var_set_num(vm, "OK", 1);
    var_set_num(vm, "ENSURENEW_N", 1);
    var_set_num(vm, "ENSURED", 1);
    var_set_num(vm, "TRYNEW_N", 1);
    var_set_str(vm, "LAST", oname);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", oname);
    var_set_num(vm, "LAST_N", 1); /* 1 = newly created */
    vm->last_n = 1;
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "ERR", "");
    if (vm->trace) fprintf(vm->trace, "# ENSURENEW created %s %s\n", cname, oname);
    bump(vm);
    return 1;
  }

  /* NEW ClassName instance [ctor argsâ¦]
   * NEW SOFT|TRY â¦ â soft dual of TRYNEW (same sticky miss).
   * instance may be bare token, "str", or string-var value (dynamic slot names). */
  if (kw(&L->cur, "NEW") || kw(&L->cur, "MAKE") || kw(&L->cur, "CREATE")) {
    char cname[48], oname[48];
    int soft = 0, ensure = 0, rc;
    lex_next(L);
    if (kw(&L->cur, "OR") || kw(&L->cur, "ENSURE") || kw(&L->cur, "IFMISS") ||
        kw(&L->cur, "UNLESS")) {
      /* NEW OR Class name â ensure-live dual of ENSURENEW */
      ensure = 1;
      soft = 1;
      lex_next(L);
    } else if (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") || kw(&L->cur, "OPTIONAL") ||
        kw(&L->cur, "MAYBE") || kw(&L->cur, "OPT")) {
      soft = 1;
      lex_next(L);
    }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail_at(vm, L, "NEW needs ClassName instance â NEW Ticket t"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(cname, sizeof cname, "%s", L->cur.text); lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      if (oop_find_class(vm, id))
        snprintf(cname, sizeof cname, "%s", id);
      else if (!soft) {
        /* hard NEW: keep bare class token (historical) */
        snprintf(cname, sizeof cname, "%s", id);
      } else {
        vv = var_get(vm, id, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(cname, sizeof cname, "%s", vv->sval);
        else
          snprintf(cname, sizeof cname, "%s", id);
      }
    }
    if (oop_read_name(vm, L, oname, sizeof oname, "NEW instance name") < 0)
      return -1;
    if (ensure) {
      ObjInst *eob = oop_find_obj(vm, oname);
      if (eob) {
        ClassDef *ecd;
        if (eob->class_idx >= 0 && eob->class_idx < vm->n_classes) {
          ecd = &vm->classes[eob->class_idx];
          if (cname[0] && strcmp(ecd->name, cname) != 0) {
            char msg[96];
            snprintf(msg, sizeof msg, "ENSURENEW: class mismatch %s is %s not %s",
                     oname, ecd->name, cname);
            var_set_str(vm, "LAST_ERR", msg);
            var_set_str(vm, "ERR", msg);
            var_set_num(vm, "OK", 0);
            var_set_num(vm, "ENSURENEW_N", 0);
            var_set_num(vm, "ENSURED", 0);
            var_set_num(vm, "LAST_N", 0);
            vm->last_n = 0;
            bump(vm);
            return 1;
          }
          var_set_str(vm, "CLASS", ecd->name);
        }
        var_set_str(vm, "LAST", oname);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", oname);
        var_set_str(vm, "OBJECT", oname);
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_num(vm, "ENSURENEW_N", 0);
        var_set_num(vm, "ENSURED", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST_ERR", "");
        var_set_str(vm, "ERR", "");
        if (vm->trace) fprintf(vm->trace, "# NEW OR already %s\n", oname);
        bump(vm);
        return 1;
      }
    }
    rc = oop_new_instance(vm, cname, oname, -1, L, 1, soft);
    if (rc == -1) return -1;
    if (rc == -2) {
      if (ensure) {
        var_set_num(vm, "ENSURENEW_N", 0);
        var_set_num(vm, "ENSURED", 0);
      }
      if (vm->trace) fprintf(vm->trace, "# NEW SOFT miss %s %s\n", cname, oname);
      bump(vm);
      return 1;
    }
    var_set_num(vm, "OK", 1);
    if (soft) {
      var_set_num(vm, "TRYNEW_N", 1);
      var_set_str(vm, "LAST", oname);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", oname);
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
    }
    if (ensure) {
      var_set_num(vm, "ENSURENEW_N", 1);
      var_set_num(vm, "ENSURED", 1);
      var_set_str(vm, "LAST", oname);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", oname);
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST_ERR", "");
      var_set_str(vm, "ERR", "");
    }
    if (vm->trace) fprintf(vm->trace, "# NEW %s %s\n", cname, oname);
    bump(vm);
    return 1;
  }

  /* SPAWN/ENTITY/SCENE/TICK → lang_ops_flow_scene.c (A3) */
  {
    int sc = cubalc_lang_ops_flow_try_scene(vm, L);
    if (sc != 0) return sc;
  }


  /* SEND* family → lang_ops_flow_send.c (A3) */
  {
    int sd = cubalc_lang_ops_flow_try_send(vm, L);
    if (sd != 0) return sd;
  }


  /* GETFALL/SETFALL field bulk → lang_ops_flow_field.c (A3) */
  {
    int fd = cubalc_lang_ops_flow_try_field(vm, L);
    if (fd != 0) return fd;
  }

  /* GETF obj field [OR|DEFAULT fallback]
   * TRYGETF|GETFSOFT|GETF SOFT â soft miss OK=0 (no fatal) without fallback.
   * GETF â¦ OR val â like SYS ENV/LOOKUP: miss â LAST=fallback, OK=1, GETF_OR=1.
   * Bare GETF still fatal on unknown object/field (strict).
   * Field may be IDENT or "string" (dynamic LISTFIELDS walk).
   * Usability: agent probes without HASFIELD+IF glue. */
  /* GETF/SETF/INCF… → flow_fget; CATF/TRIMF… → flow_fstr */
  {
    int g = cubalc_lang_ops_flow_try_fget(vm, L);
    if (g != 0) return g;
  }
  {
    int s = cubalc_lang_ops_flow_try_fstr(vm, L);
    if (s != 0) return s;
  }
  return 0;
}
