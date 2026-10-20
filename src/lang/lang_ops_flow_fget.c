/* flow fget peel from flow_class */
#include "lang/cubalc_lang_ops_flow_fget.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fget(VM *vm, Lex *L) {
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
      fail_at(vm, L, "GETF needs object field â GETF obj status"); return -1;
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
      fail_at(vm, L, "GETF needs field name â GETF obj status"); return -1;
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
      char sug[48], ebuf[160], base[96];
      oop_suggest_obj(vm, oname, sug, sizeof sug);
      snprintf(base, sizeof base, "GETF unknown object %s", oname);
      oop_err_suggest(ebuf, sizeof ebuf, base, sug);
      if (!soft) {
        fail(vm, ebuf); return -1;
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
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
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
      char sug[48], ebuf[160], base[96];
      oop_suggest_field(cd, fname, sug, sizeof sug);
      snprintf(base, sizeof base, "GETF unknown FIELD %s", fname);
      oop_err_suggest(ebuf, sizeof ebuf, base, sug);
      if (!soft) {
        fail(vm, ebuf); return -1;
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
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
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
   * TRYSETF|SETFSOFT|SETF SOFT â soft miss OK=0 (no fatal) for unknown obj/field.
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
      fail_at(vm, L, "SETF needs object field value â SETF obj status val"); return -1;
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
      fail_at(vm, L, "SETF needs field name â SETF obj status val"); return -1;
    }
    if (L->cur.kind == TK_EQ) lex_next(L);
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      char sug[48], ebuf[160], base[96];
      oop_suggest_obj(vm, oname, sug, sizeof sug);
      snprintf(base, sizeof base, "SETF unknown object %s", oname);
      oop_err_suggest(ebuf, sizeof ebuf, base, sug);
      if (!soft) {
        fail(vm, ebuf); return -1;
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
      var_set_str(vm, "LAST_ERR", ebuf);
      var_set_str(vm, "ERR", ebuf);
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
      char sug[48], ebuf[160], base[96];
      oop_suggest_field(cd, fname, sug, sizeof sug);
      snprintf(base, sizeof base, "SETF unknown FIELD %s", fname);
      oop_err_suggest(ebuf, sizeof ebuf, base, sug);
      if (!soft) {
        fail(vm, ebuf); return -1;
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
      var_set_str(vm, "LAST_ERR", ebuf);
      var_set_str(vm, "ERR", ebuf);
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
   * TRYINCF|INCF SOFT â soft miss OK=0.
   * Default delta: +1 for INCF/ADDF/BUMPF, â1 for DECF/SUBF.
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
    /* optional delta (default Â±1) */
    delta = is_dec ? -1L : 1L;
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
         strcmp(L->cur.text, "END") != 0)) {
      /* only parse if it looks like a value; avoid eating next form */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        delta = parse_expr(vm, L);
        if (is_dec && delta > 0) delta = -delta; /* DECF 3 â â3 */
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
   * TRYBOUNDF|BOUNDF SOFT â soft miss OK=0.
   * Clamp one object's numeric field into [lo,hi] (auto-swap lo/hi).
   * Note: CLAMPF/CLIPF remain fleet CLAMPFALL aliases â this is the single-obj form.
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

  /* MINF|CAPF|ATMOSTF obj field limit â field = min(field, limit) (ceiling).
   * MAXF|FLOORF|ATLEASTF obj field limit â field = max(field, limit) (floor).
   * TRYMINF|MINF SOFT / TRYMAXF|MAXF SOFT â soft miss OK=0 sticky LAST_ERR.
   * One-sided bound without full BOUNDF lo..hi. Note: fleet MINWHERE/MAXWHERE
   * stay select forms; this mutates one object field.
   * Usability: floor energy at 0 or cap retries without GETF+IF+SETF (METHOD/THIS). */
  if (kw(&L->cur, "MINF") || kw(&L->cur, "CAPF") ||
      kw(&L->cur, "ATMOSTF") || kw(&L->cur, "CEILF") ||
      kw(&L->cur, "CAPOBJ") || kw(&L->cur, "MINFIELD") ||
      kw(&L->cur, "CAPFIELD") || kw(&L->cur, "MAXF") ||
      kw(&L->cur, "FLOORF") || kw(&L->cur, "ATLEASTF") ||
      kw(&L->cur, "RAISEF") || kw(&L->cur, "FLOOROBJ") ||
      kw(&L->cur, "MAXFIELD") || kw(&L->cur, "FLOORFIELD") ||
      kw(&L->cur, "TRYMINF") || kw(&L->cur, "MINFSOFT") ||
      kw(&L->cur, "SOFTMINF") || kw(&L->cur, "TRYCAPF") ||
      kw(&L->cur, "TRYMAXF") || kw(&L->cur, "MAXFSOFT") ||
      kw(&L->cur, "SOFTMAXF") || kw(&L->cur, "TRYFLOORF") ||
      kw(&L->cur, "TRYATMOSTF") || kw(&L->cur, "TRYATLEASTF")) {
    char oname[48], fname[48], op[24];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, is_max = 0, changed = 0;
    long lim = 0, nv, oldv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    /* MAXF / FLOORF / ATLEASTF / RAISEF â raise floor (max with limit)
     * MINF / CAPF / ATMOSTF / CEILF â lower ceiling (min with limit)
     * Note: fleet MAXOBJ/MINOBJ stay extreme-object pickers (not aliases). */
    if (strcmp(op, "MAXF") == 0 || strcmp(op, "FLOORF") == 0 ||
        strcmp(op, "ATLEASTF") == 0 || strcmp(op, "RAISEF") == 0 ||
        strcmp(op, "FLOOROBJ") == 0 || strcmp(op, "MAXFIELD") == 0 ||
        strcmp(op, "FLOORFIELD") == 0 || strcmp(op, "TRYMAXF") == 0 ||
        strcmp(op, "MAXFSOFT") == 0 || strcmp(op, "SOFTMAXF") == 0 ||
        strcmp(op, "TRYFLOORF") == 0 || strcmp(op, "TRYATLEASTF") == 0)
      is_max = 1;
    if (strcmp(op, "TRYMINF") == 0 || strcmp(op, "MINFSOFT") == 0 ||
        strcmp(op, "SOFTMINF") == 0 || strcmp(op, "TRYCAPF") == 0 ||
        strcmp(op, "TRYMAXF") == 0 || strcmp(op, "MAXFSOFT") == 0 ||
        strcmp(op, "SOFTMAXF") == 0 || strcmp(op, "TRYFLOORF") == 0 ||
        strcmp(op, "TRYATMOSTF") == 0 || strcmp(op, "TRYATLEASTF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, is_max ? "MAXF object field limit" : "MINF object field limit");
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
      fail(vm, is_max ? "MAXF field" : "MINF field"); return -1;
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "AT") || kw(&L->cur, "WITH") ||
        kw(&L->cur, "LIMIT") || kw(&L->cur, "CAP") || kw(&L->cur, "FLOOR"))
      lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        lim = parse_expr(vm, L);
      } else {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          lim = dv->val;
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          lim = vm->last_n;
          lex_next(L);
        } else {
          fail(vm, is_max ? "MAXF object field limit" : "MINF object field limit");
          return -1;
        }
      }
    } else {
      fail(vm, is_max ? "MAXF object field limit" : "MINF object field limit");
      return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      const char *tag = is_max ? "MAXF" : "MINF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown object %s", tag, oname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "MINF_N", 0);
      var_set_num(vm, "MAXF_N", 0);
      var_set_num(vm, "MINF_CHANGED", 0);
      var_set_num(vm, "MAXF_CHANGED", 0);
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
      const char *tag = is_max ? "MAXF" : "MINF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown FIELD %s", tag, fname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "MINF_N", 0);
      var_set_num(vm, "MAXF_N", 0);
      var_set_num(vm, "MINF_CHANGED", 0);
      var_set_num(vm, "MAXF_CHANGED", 0);
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
      const char *tag = is_max ? "MAXF" : "MINF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s field %s is string", tag, fname);
        fail(vm, vm->err); return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "MINF_N", 0);
      var_set_num(vm, "MAXF_N", 0);
      var_set_num(vm, "MINF_CHANGED", 0);
      var_set_num(vm, "MAXF_CHANGED", 0);
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
    oldv = ob->fnum[fi];
    if (is_max)
      nv = (oldv > lim) ? oldv : lim;
    else
      nv = (oldv < lim) ? oldv : lim;
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
    if (is_max) {
      var_set_num(vm, "MAXF_N", 1);
      var_set_num(vm, "MAXF_CHANGED", changed);
      var_set_num(vm, "MAXF_LIM", lim);
      var_set_num(vm, "FLOORF_N", 1);
      var_set_num(vm, "MINF_N", 0);
      var_set_num(vm, "MINF_CHANGED", 0);
    } else {
      var_set_num(vm, "MINF_N", 1);
      var_set_num(vm, "MINF_CHANGED", changed);
      var_set_num(vm, "MINF_LIM", lim);
      var_set_num(vm, "CAPF_N", 1);
      var_set_num(vm, "MAXF_N", 0);
      var_set_num(vm, "MAXF_CHANGED", 0);
    }
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* TIMESF|SCALEOBJ|MULOBJ obj field factor
   * TRYTIMESF|TIMESF SOFT â soft miss OK=0.
   * Multiply one object's numeric field by factor (integer).
   * Note: MULF/SCALEF remain fleet MULFALL aliases â this is the single-obj form.
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

  /* DIVF|IDIVF|QUOTF obj field divisor â integer divide one numeric field.
   * MODF|REMF|MODULO obj field modulus â remainder one numeric field.
   * TRYDIVF|DIVF SOFT / TRYMODF|MODF SOFT â soft miss OK=0 sticky LAST_ERR.
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

  /* ABSF|IABSF|ABSVALF obj field â absolute value of one numeric field.
   * NEGF|NEGAF|FLIPF obj field â negate one numeric field.
   * SIGNF|SGNF|SIGNUMF obj field â write signum â1|0|1 into field.
   * TRYABSF|ABSF SOFT / TRYNEGF / TRYSIGNF â soft miss OK=0 sticky LAST_ERR.
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

  /* LPADF|PADLEFTF|PADF obj field width [padchar]
   * RPADF|PADRIGHTF obj field width [padchar]
   * TRYLPADF|LPADF SOFT / TRYRPADF â soft miss OK=0 sticky LAST_ERR.
   * Pad (or truncate) one string field to width; promotes numâdecimal first.
   * Default pad char space. Width capped to field buffer â 1.
   * Usability: fixed-width plate IDs/columns without GETF+SYS LPAD/RPAD+SETF
   * (METHOD/THIS; pairs STRF/CATF/LEFTF). */
  if (kw(&L->cur, "LPADF") || kw(&L->cur, "PADLEFTF") ||
      kw(&L->cur, "PADF") || kw(&L->cur, "STRPADF") ||
      kw(&L->cur, "LPADFIELD") || kw(&L->cur, "PADOBJ") ||
      kw(&L->cur, "RPADF") || kw(&L->cur, "PADRIGHTF") ||
      kw(&L->cur, "RPADFIELD") || kw(&L->cur, "RPADOBJ") ||
      kw(&L->cur, "TRYLPADF") || kw(&L->cur, "LPADFSOFT") ||
      kw(&L->cur, "SOFTLPADF") || kw(&L->cur, "TRYPADF") ||
      kw(&L->cur, "TRYRPADF") || kw(&L->cur, "RPADFSOFT") ||
      kw(&L->cur, "SOFTRPADF") || kw(&L->cur, "TRYPADLEFTF") ||
      kw(&L->cur, "TRYPADRIGHTF")) {
    char oname[48], fname[48], op[24], src[128], out[128], padbuf[16];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, is_left = 1;
    long width = 0;
    char padc = ' ';
    size_t sn, need, o, take;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "RPADF") == 0 || strcmp(op, "PADRIGHTF") == 0 ||
        strcmp(op, "RPADFIELD") == 0 || strcmp(op, "RPADOBJ") == 0 ||
        strcmp(op, "TRYRPADF") == 0 || strcmp(op, "RPADFSOFT") == 0 ||
        strcmp(op, "SOFTRPADF") == 0 || strcmp(op, "TRYPADRIGHTF") == 0)
      is_left = 0;
    if (strcmp(op, "TRYLPADF") == 0 || strcmp(op, "LPADFSOFT") == 0 ||
        strcmp(op, "SOFTLPADF") == 0 || strcmp(op, "TRYPADF") == 0 ||
        strcmp(op, "TRYRPADF") == 0 || strcmp(op, "RPADFSOFT") == 0 ||
        strcmp(op, "SOFTRPADF") == 0 || strcmp(op, "TRYPADLEFTF") == 0 ||
        strcmp(op, "TRYPADRIGHTF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, is_left ? "LPADF object field width [pad]"
                       : "RPADF object field width [pad]");
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
      fail(vm, is_left ? "LPADF field" : "RPADF field"); return -1;
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "WIDTH") || kw(&L->cur, "W") ||
        kw(&L->cur, "LEN") || kw(&L->cur, "SIZE"))
      lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        width = parse_expr(vm, L);
      } else {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          width = dv->val;
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          width = vm->last_n;
          lex_next(L);
        } else {
          fail(vm, is_left ? "LPADF object field width [pad]"
                           : "RPADF object field width [pad]");
          return -1;
        }
      }
    } else {
      fail(vm, is_left ? "LPADF object field width [pad]"
                       : "RPADF object field width [pad]");
      return -1;
    }
    if (width < 0) width = 0;
    if (width > 127) width = 127;
    if (kw(&L->cur, "WITH") || kw(&L->cur, "PAD") || kw(&L->cur, "CHAR") ||
        kw(&L->cur, "USING"))
      lex_next(L);
    padc = ' ';
    if (L->cur.kind == TK_STR) {
      if (L->cur.text[0]) padc = L->cur.text[0];
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      if (vm->last_str[0]) padc = vm->last_str[0];
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str && sv->sval[0]) {
        padc = sv->sval[0];
        lex_next(L);
      } else if (sv && !sv->is_str) {
        /* numeric pad char as decimal digit of ones place â uncommon; skip */
      }
    }
    (void)padbuf;
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      const char *tag = is_left ? "LPADF" : "RPADF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown object %s", tag, oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "LPADF_N", 0);
      var_set_num(vm, "RPADF_N", 0);
      var_set_num(vm, "PADF_W", 0);
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
      const char *tag = is_left ? "LPADF" : "RPADF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown FIELD %s", tag, fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "LPADF_N", 0);
      var_set_num(vm, "RPADF_N", 0);
      var_set_num(vm, "PADF_W", 0);
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
    /* promote numeric to decimal text */
    if (ob->fis_str[fi])
      snprintf(src, sizeof src, "%s", ob->fstr[fi]);
    else
      snprintf(src, sizeof src, "%ld", ob->fnum[fi]);
    sn = strlen(src);
    if ((long)sn >= width) {
      take = (size_t)width;
      if (take >= sizeof out) take = sizeof out - 1;
      memcpy(out, src, take);
      out[take] = 0;
    } else {
      need = (size_t)width - sn;
      if (is_left) {
        o = 0;
        for (; o < need && o + 1 < sizeof out; o++) out[o] = padc;
        take = sn;
        if (o + take >= sizeof out) take = sizeof out - 1 - o;
        memcpy(out + o, src, take);
        o += take;
        out[o] = 0;
      } else {
        take = sn;
        if (take >= sizeof out) take = sizeof out - 1;
        memcpy(out, src, take);
        o = take;
        for (; need > 0 && o + 1 < sizeof out; need--, o++) out[o] = padc;
        out[o] = 0;
      }
    }
    snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
    ob->fis_str[fi] = 1;
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = (long)strlen(out);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "LPADF_N", is_left ? 1 : 0);
    var_set_num(vm, "RPADF_N", is_left ? 0 : 1);
    var_set_num(vm, "PADF_N", 1);
    var_set_num(vm, "PADF_W", width);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* CATF|APPENDF|STRCATF|PREPENDF obj field string
   * TRYCATF|CATF SOFT â soft miss OK=0.
   * Append (or PREPENDF prefix) text onto a string field; promotes numâstr.
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
   * â append (or prepend) text onto string field of every live obj.
   * Soft always; missing fields skipped (CATFALL_SKIP). Promotes numâstr.
   * LAST_N = update count; LAST = piece applied.
   * Usability: fleet note/status tags without EACH OBJ + GETF+CAT+SETF.
   * String dual of INCFALL; fleet dual of CATF. */
  return 0;
}
