/* CubalC lang — lang_ops_core_plate_nest.c
 * Nest plate plane: GETPOBJ HASPOBJ PLUCKOBJ (A_core peel).
 * Mesh vote A_core_forms_agent_peel · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_core_plate_nest.h"
#include "lang/cubalc_lang_ops_sys_plate_expand.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_core_try_plate_nest(VM *vm, Lex *L) {
  /* Nest plate get/has/pluck plane.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
  if (kw(&L->cur,"GETPOBJ") || kw(&L->cur,"PEEKNEST") || kw(&L->cur,"NESTGETP") ||
      kw(&L->cur,"GETNESTF") || kw(&L->cur,"OBJGETP") || kw(&L->cur,"MGETPOBJ") ||
      kw(&L->cur,"SETPOBJ") || kw(&L->cur,"PUTNESTF") || kw(&L->cur,"NESTSETP") ||
      kw(&L->cur,"SETOBJF") || kw(&L->cur,"PUTNESTKEY") || kw(&L->cur,"MSETPOBJ") ||
      kw(&L->cur,"INCOBJ") || kw(&L->cur,"BUMPNEST") || kw(&L->cur,"NESTINCP") ||
      kw(&L->cur,"INCNEST") || kw(&L->cur,"INCOBJF") || kw(&L->cur,"MINCOBJ") ||
      kw(&L->cur,"DECOBJ") || kw(&L->cur,"DECNNEST") ||
      kw(&L->cur,"DELPOBJ") || kw(&L->cur,"DROPNEST") || kw(&L->cur,"NESTDELP") ||
      kw(&L->cur,"DELOBJF") || kw(&L->cur,"RMNESTF") || kw(&L->cur,"MDELPOBJ")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[96], field[96], val[CUBALC_HOST_STR_MAX];
    char fb[CUBALC_HOST_STR_MAX], nest[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX], raw[32];
    cubalc_host_result gr, ngr, hr, setr;
    int is_set = 0, is_inc = 0, is_dec = 0, is_del = 0, is_get = 0;
    int have_from = 0, have_fb = 0, val_kind = 0;
    long delta = 1, cur = 0, nv = 0;
    Var *pv;
    char op0[24];
    const char *v;

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "SETPOBJ") == 0 || strcmp(op0, "PUTNESTF") == 0 ||
        strcmp(op0, "NESTSETP") == 0 || strcmp(op0, "SETOBJF") == 0 ||
        strcmp(op0, "PUTNESTKEY") == 0 || strcmp(op0, "MSETPOBJ") == 0)
      is_set = 1;
    else if (strcmp(op0, "DECOBJ") == 0 || strcmp(op0, "DECNNEST") == 0) {
      is_inc = 1;
      is_dec = 1;
      delta = -1;
    } else if (strcmp(op0, "INCOBJ") == 0 || strcmp(op0, "BUMPNEST") == 0 ||
               strcmp(op0, "NESTINCP") == 0 || strcmp(op0, "INCNEST") == 0 ||
               strcmp(op0, "INCOBJF") == 0 || strcmp(op0, "MINCOBJ") == 0)
      is_inc = 1;
    else if (strcmp(op0, "DELPOBJ") == 0 || strcmp(op0, "DROPNEST") == 0 ||
             strcmp(op0, "NESTDELP") == 0 || strcmp(op0, "DELOBJF") == 0 ||
             strcmp(op0, "RMNESTF") == 0 || strcmp(op0, "MDELPOBJ") == 0)
      is_del = 1;
    else
      is_get = 1;

    lex_next(L);
    plate[0] = 0; nestk[0] = 0; field[0] = 0; val[0] = 0; fb[0] = 0;
    nest[0] = 0; from_name[0] = 0; from_src[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
        snprintf(from_name, sizeof from_name, "%s", "LAST");
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && pv->is_str) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(from_src, sizeof from_src, "%s", pv->sval);
          lex_next(L);
        } else if (pv) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(from_src, sizeof from_src, "%ld", pv->val);
          lex_next(L);
        } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
          from_src[0] = 0;
        }
      } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
      }
    }

    /* nest key */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(nestk, sizeof nestk, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
          kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT")) {
        fail(vm, "GETPOBJ|SETPOBJ|INCOBJ|DELPOBJ — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, "GETPOBJ|SETPOBJ|INCOBJ|DELPOBJ — need nest key");
      return -1;
    }

    /* field key */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(field, sizeof field, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
          kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT")) {
        fail(vm, "GETPOBJ|SETPOBJ|INCOBJ|DELPOBJ — need field key");
        return -1;
      }
      if (resolve_str_arg(vm, L, field, sizeof field) != 0)
        field[0] = 0;
    } else {
      fail(vm, "GETPOBJ|SETPOBJ|INCOBJ|DELPOBJ — need field key");
      return -1;
    }

    if (is_get) {
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK") || kw(&L->cur,"SOFT")) {
        lex_next(L);
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(fb, sizeof fb, "%ld", n);
          have_fb = 1;
        } else if (L->cur.kind == TK_IDENT &&
                   (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF"))) {
          fail(vm, "GETPOBJ nest field OR fallback — need fallback");
          return -1;
        } else if (resolve_str_arg(vm, L, fb, sizeof fb) == 0) {
          have_fb = 1;
        } else {
          fail(vm, "GETPOBJ nest field OR fallback — need fallback");
          return -1;
        }
      }
    } else if (is_set) {
      if (kw(&L->cur,"RAW") || kw(&L->cur,"ASRAW") || kw(&L->cur,"LITERAL")) {
        val_kind = 1;
        lex_next(L);
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long n = parse_expr(vm, L);
        snprintf(val, sizeof val, "%ld", n);
        val_kind = 1;
      } else if (L->cur.kind == TK_IDENT &&
                 (strcmp(L->cur.text, "true") == 0 || strcmp(L->cur.text, "false") == 0 ||
                  strcmp(L->cur.text, "null") == 0)) {
        snprintf(val, sizeof val, "%s", L->cur.text);
        val_kind = 1;
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT &&
                 (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
                  kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM"))) {
        fail(vm, "SETPOBJ nest field value — need value");
        return -1;
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
          fail(vm, "SETPOBJ nest field value — need value");
          return -1;
        }
      } else {
        fail(vm, "SETPOBJ nest field value — need value");
        return -1;
      }
    } else if (is_inc) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        delta = parse_expr(vm, L);
        if (is_dec && delta > 0) delta = -delta;
      }
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
        snprintf(from_name, sizeof from_name, "%s", "LAST");
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && pv->is_str) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(from_src, sizeof from_src, "%s", pv->sval);
          lex_next(L);
        } else if (pv) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(from_src, sizeof from_src, "%ld", pv->val);
          lex_next(L);
        } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
          from_src[0] = 0;
        }
      } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
      }
    }

    if (!nestk[0] || !field[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "GETPOBJ|SETPOBJ|INCOBJ|DELPOBJ: empty nest/field");
      var_set_str(vm, "ERR", "GETPOBJ|SETPOBJ|INCOBJ|DELPOBJ: empty nest/field");
      var_set_num(vm, "GETPOBJ_FROM", have_from ? 1 : 0);
      bump(vm); return 1;
    }

    if (have_from) {
      const char *bp = from_src;
      while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++;
      if (*bp == '{')
        snprintf(plate, sizeof plate, "%s", from_src);
      else
        snprintf(plate, sizeof plate, "%s", "{}");
    } else {
      pv = var_get(vm, "PLATE", 0);
      if (pv && pv->is_str && pv->sval[0])
        snprintf(plate, sizeof plate, "%s", pv->sval);
      else
        snprintf(plate, sizeof plate, "%s", "{}");
    }

    /* peel nest object (or {} for mutators) */
    snprintf(nest, sizeof nest, "%s", "{}");
    memset(&ngr, 0, sizeof ngr);
    if (cubalc_host_json_get_raw(plate, nestk, &ngr) == 0) {
      v = ngr.str;
      while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
      if (*v == '{') {
        if (v != ngr.str) {
          size_t n = strlen(v);
          memmove(ngr.str, v, n + 1);
        }
        snprintf(nest, sizeof nest, "%s", ngr.str);
      } else if (is_get) {
        /* nest exists but not object → miss */
        nest[0] = 0;
      }
      /* mutators: non-object nest replaced by {} */
    } else if (is_get) {
      nest[0] = 0;
    }

    if (is_get) {
      if (!nest[0]) {
        if (have_fb) {
          var_set_str(vm, "LAST", fb);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_str(vm, "GETPOBJ", fb);
          var_set_num(vm, "GETPOBJ_HIT", 0);
          var_set_num(vm, "GETPOBJ_OR", 1);
          var_set_num(vm, "OK", 1);
        } else {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "GETPOBJ_HIT", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "GETPOBJ: nest miss or not object");
          var_set_str(vm, "ERR", "GETPOBJ: nest miss or not object");
        }
      } else {
        memset(&gr, 0, sizeof gr);
        if (cubalc_host_json_get(nest, field, &gr) == 0) {
          var_set_str(vm, "LAST", gr.str);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", gr.str);
          vm->last_n = 1;
          var_set_num(vm, "LAST_N", 1);
          var_set_str(vm, "GETPOBJ", gr.str);
          var_set_str(vm, "PEEKNEST", gr.str);
          var_set_num(vm, "GETPOBJ_HIT", 1);
          var_set_num(vm, "JSON_HIT", 1);
          var_set_num(vm, "OK", 1);
        } else if (have_fb) {
          var_set_str(vm, "LAST", fb);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_str(vm, "GETPOBJ", fb);
          var_set_num(vm, "GETPOBJ_HIT", 0);
          var_set_num(vm, "GETPOBJ_OR", 1);
          var_set_num(vm, "OK", 1);
        } else {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "GETPOBJ_HIT", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "GETPOBJ: field miss");
          var_set_str(vm, "ERR", "GETPOBJ: field miss");
        }
      }
      var_set_str(vm, "GETPOBJ_NEST", nestk);
      var_set_str(vm, "GETPOBJ_KEY", field);
      var_set_num(vm, "GETPOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "GETPOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      if (vm->trace)
        fprintf(vm->trace, "# getpobj nest=%s field=%s from=%d\n",
                nestk, field, have_from);
      bump(vm); return 1;
    }

    /* mutators need nest object */
    if (!nest[0])
      snprintf(nest, sizeof nest, "%s", "{}");

    if (is_set) {
      memset(&setr, 0, sizeof setr);
      if (cubalc_host_json_set(nest, field, val, val_kind, &setr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", setr.err[0] ? setr.err : "SETPOBJ: nest set fail");
        var_set_str(vm, "ERR", setr.err[0] ? setr.err : "SETPOBJ: nest set fail");
        bump(vm); return 1;
      }
      snprintf(nest, sizeof nest, "%s", setr.str);
    } else if (is_inc) {
      memset(&gr, 0, sizeof gr);
      cur = 0;
      if (cubalc_host_json_get(nest, field, &gr) == 0) {
        char *end = NULL;
        cur = strtol(gr.str, &end, 10);
        if (end == gr.str) cur = 0;
      }
      nv = cur + delta;
      snprintf(raw, sizeof raw, "%ld", nv);
      memset(&setr, 0, sizeof setr);
      if (cubalc_host_json_set(nest, field, raw, 1, &setr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", setr.err[0] ? setr.err : "INCOBJ: nest set fail");
        var_set_str(vm, "ERR", setr.err[0] ? setr.err : "INCOBJ: nest set fail");
        bump(vm); return 1;
      }
      snprintf(nest, sizeof nest, "%s", setr.str);
    } else if (is_del) {
      memset(&setr, 0, sizeof setr);
      if (cubalc_host_json_del(nest, field, &setr) != 0) {
        /* soft: keep nest as-is if del fails? treat as miss nest field */
        snprintf(nest, sizeof nest, "%s", nest); /* no-op */
        setr.n = 0;
        /* still write-back nest so outer plate consistent */
      } else {
        snprintf(nest, sizeof nest, "%s", setr.str);
      }
    }

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_set(plate, nestk, nest, 1, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "SETPOBJ: write-back fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "SETPOBJ: write-back fail");
      bump(vm); return 1;
    }

    if (have_from && from_name[0] && strcmp(from_name, "LAST") != 0)
      var_set_str(vm, from_name, hr.str);
    else if (!have_from)
      var_set_str(vm, "PLATE", hr.str);

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    var_set_str(vm, "NEST", nest);
    var_set_str(vm, "MERGED", nest);
    if (is_inc) {
      vm->last_n = nv;
      var_set_num(vm, "LAST_N", nv);
      var_set_num(vm, "INCOBJ_V", nv);
      var_set_num(vm, "INCOBJ_DELTA", delta);
      var_set_str(vm, "INCOBJ", hr.str);
    } else if (is_del) {
      vm->last_n = setr.n;
      var_set_num(vm, "LAST_N", setr.n);
      var_set_num(vm, "DELPOBJ_N", setr.n);
      var_set_str(vm, "DELPOBJ", hr.str);
    } else {
      vm->last_n = setr.n;
      var_set_num(vm, "LAST_N", setr.n);
      var_set_num(vm, "SETPOBJ_N", setr.n);
      var_set_str(vm, "SETPOBJ", hr.str);
    }
    var_set_str(vm, "GETPOBJ_NEST", nestk);
    var_set_str(vm, "SETPOBJ_NEST", nestk);
    var_set_str(vm, "SETPOBJ_KEY", field);
    var_set_num(vm, "GETPOBJ_FROM", have_from ? 1 : 0);
    var_set_num(vm, "SETPOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "SETPOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s nest=%s field=%s from=%d\n",
              is_inc ? "incobj" : (is_del ? "delpobj" : "setpobj"),
              nestk, field, have_from);
    bump(vm); return 1;
  }

  /* HASPOBJ|NESTHAS [FROM plate] nest field — soft 0|1 nested field presence.
   * TYPEPOBJ|NESTTYPE [FROM plate] nest field — kind missing|num|str|bool|null|obj|arr.
   * KEYSOBJ|NESTKEYS [FROM plate] nest — nested object key bag → LAST · count LAST_N.
   * NEEDPOBJ|REQUIRENEST [FROM plate] nest field… — fail-fast nested keys · miss bag.
   * Completes nested contract/probe surface (HASP/TYPEP/KEYSP/NEEDP duals for nests):
   *   HASPOBJ "meta" "role"
   *   TYPEPOBJ "meta" "x"
   *   KEYSOBJ "meta"
   *   NEEDPOBJ "meta" "x" "role"
   * Soft nest miss → HAS=0 · TYPE=missing · KEYS empty · NEED lists fields.
   */
  if (kw(&L->cur,"HASPOBJ") || kw(&L->cur,"NESTHAS") || kw(&L->cur,"HASNEST") ||
      kw(&L->cur,"HASNESTF") || kw(&L->cur,"OBJHASP") || kw(&L->cur,"MHASPOBJ") ||
      kw(&L->cur,"TYPEPOBJ") || kw(&L->cur,"NESTTYPE") || kw(&L->cur,"TYPEOFNEST") ||
      kw(&L->cur,"NESTKIND") || kw(&L->cur,"OBJTYPEP") || kw(&L->cur,"MTYPEPOBJ") ||
      kw(&L->cur,"KEYSOBJ") || kw(&L->cur,"NESTKEYS") || kw(&L->cur,"KEYSNEST") ||
      kw(&L->cur,"OBJKEYSP") || kw(&L->cur,"MKEYSOBJ") || kw(&L->cur,"LISTNESTKEYS") ||
      kw(&L->cur,"NEEDPOBJ") || kw(&L->cur,"REQUIRENEST") || kw(&L->cur,"NEEDNEST") ||
      kw(&L->cur,"NESTNEED") || kw(&L->cur,"MNEEDPOBJ") || kw(&L->cur,"REQUIRENESTKEYS")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[96], field[96], nest[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    char keys_req[CUBALC_HOST_STR_MAX], miss[CUBALC_HOST_STR_MAX];
    cubalc_host_result ngr, gr, kr, hr;
    int have_from = 0, is_has = 0, is_type = 0, is_keys = 0, is_need = 0;
    int n_req = 0, n_miss = 0, hit = 0;
    long kind_n = 0;
    const char *kind_s = "missing";
    Var *pv;
    char op0[24];
    const char *v;

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "HASPOBJ") == 0 || strcmp(op0, "NESTHAS") == 0 ||
        strcmp(op0, "HASNEST") == 0 || strcmp(op0, "HASNESTF") == 0 ||
        strcmp(op0, "OBJHASP") == 0 || strcmp(op0, "MHASPOBJ") == 0)
      is_has = 1;
    else if (strcmp(op0, "TYPEPOBJ") == 0 || strcmp(op0, "NESTTYPE") == 0 ||
             strcmp(op0, "TYPEOFNEST") == 0 || strcmp(op0, "NESTKIND") == 0 ||
             strcmp(op0, "OBJTYPEP") == 0 || strcmp(op0, "MTYPEPOBJ") == 0)
      is_type = 1;
    else if (strcmp(op0, "KEYSOBJ") == 0 || strcmp(op0, "NESTKEYS") == 0 ||
             strcmp(op0, "KEYSNEST") == 0 || strcmp(op0, "OBJKEYSP") == 0 ||
             strcmp(op0, "MKEYSOBJ") == 0 || strcmp(op0, "LISTNESTKEYS") == 0)
      is_keys = 1;
    else
      is_need = 1;

    lex_next(L);
    plate[0] = 0; nestk[0] = 0; field[0] = 0; nest[0] = 0;
    from_name[0] = 0; from_src[0] = 0; keys_req[0] = 0; miss[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
        snprintf(from_name, sizeof from_name, "%s", "LAST");
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && pv->is_str) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(from_src, sizeof from_src, "%s", pv->sval);
          lex_next(L);
        } else if (pv) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(from_src, sizeof from_src, "%ld", pv->val);
          lex_next(L);
        } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
          from_src[0] = 0;
        }
      } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
      }
    }

    /* nest key */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(nestk, sizeof nestk, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
        fail(vm, "HASPOBJ|TYPEPOBJ|KEYSOBJ|NEEDPOBJ — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, "HASPOBJ|TYPEPOBJ|KEYSOBJ|NEEDPOBJ — need nest key");
      return -1;
    }

    if (is_keys) {
      /* optional trailing FROM only */
      if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                         kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                         kw(&L->cur,"PLATEFROM"))) {
        lex_next(L);
        have_from = 1;
        if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
          snprintf(from_src, sizeof from_src, "%s", vm->last_str);
          snprintf(from_name, sizeof from_name, "%s", "LAST");
          lex_next(L);
        } else if (L->cur.kind == TK_IDENT) {
          pv = var_get(vm, L->cur.text, 0);
          if (pv && pv->is_str) {
            snprintf(from_name, sizeof from_name, "%s", L->cur.text);
            snprintf(from_src, sizeof from_src, "%s", pv->sval);
            lex_next(L);
          } else if (pv) {
            snprintf(from_name, sizeof from_name, "%s", L->cur.text);
            snprintf(from_src, sizeof from_src, "%ld", pv->val);
            lex_next(L);
          } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
            from_src[0] = 0;
          }
        } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
          snprintf(from_src, sizeof from_src, "%s", vm->last_str);
        }
      }
    } else if (is_need) {
      /* collect field keys until FROM or stop */
      size_t olen = 0;
      keys_req[0] = 0;
      while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
             L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN) {
        char fk[96];
        fk[0] = 0;
        if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
            kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
            kw(&L->cur,"END") || kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") ||
            kw(&L->cur,"IF") || kw(&L->cur,"PRINT") || kw(&L->cur,"SYS") ||
            kw(&L->cur,"GETPOBJ") || kw(&L->cur,"SETPOBJ") || kw(&L->cur,"HASPOBJ") ||
            kw(&L->cur,"NEEDPOBJ") || kw(&L->cur,"KEYSOBJ") || kw(&L->cur,"TYPEPOBJ"))
          break;
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long kv = parse_expr(vm, L);
          snprintf(fk, sizeof fk, "%ld", kv);
        } else if (resolve_str_arg(vm, L, fk, sizeof fk) != 0) {
          break;
        }
        if (!fk[0]) break;
        size_t fl = strlen(fk);
        if (olen + fl + 2 >= sizeof keys_req) break;
        if (olen) keys_req[olen++] = '\n';
        memcpy(keys_req + olen, fk, fl);
        olen += fl;
        keys_req[olen] = 0;
        n_req++;
      }
      if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                         kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                         kw(&L->cur,"PLATEFROM"))) {
        lex_next(L);
        have_from = 1;
        if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
          snprintf(from_src, sizeof from_src, "%s", vm->last_str);
          snprintf(from_name, sizeof from_name, "%s", "LAST");
          lex_next(L);
        } else if (L->cur.kind == TK_IDENT) {
          pv = var_get(vm, L->cur.text, 0);
          if (pv && pv->is_str) {
            snprintf(from_name, sizeof from_name, "%s", L->cur.text);
            snprintf(from_src, sizeof from_src, "%s", pv->sval);
            lex_next(L);
          } else if (pv) {
            snprintf(from_name, sizeof from_name, "%s", L->cur.text);
            snprintf(from_src, sizeof from_src, "%ld", pv->val);
            lex_next(L);
          } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
            from_src[0] = 0;
          }
        } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
          snprintf(from_src, sizeof from_src, "%s", vm->last_str);
        }
      }
    } else {
      /* HAS / TYPE: one field */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(field, sizeof field, "%ld", kv);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
            kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
          fail(vm, "HASPOBJ|TYPEPOBJ nest field — need field");
          return -1;
        }
        if (resolve_str_arg(vm, L, field, sizeof field) != 0)
          field[0] = 0;
      } else {
        fail(vm, "HASPOBJ|TYPEPOBJ nest field — need field");
        return -1;
      }
      if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                         kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                         kw(&L->cur,"PLATEFROM"))) {
        lex_next(L);
        have_from = 1;
        if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
          snprintf(from_src, sizeof from_src, "%s", vm->last_str);
          snprintf(from_name, sizeof from_name, "%s", "LAST");
          lex_next(L);
        } else if (L->cur.kind == TK_IDENT) {
          pv = var_get(vm, L->cur.text, 0);
          if (pv && pv->is_str) {
            snprintf(from_name, sizeof from_name, "%s", L->cur.text);
            snprintf(from_src, sizeof from_src, "%s", pv->sval);
            lex_next(L);
          } else if (pv) {
            snprintf(from_name, sizeof from_name, "%s", L->cur.text);
            snprintf(from_src, sizeof from_src, "%ld", pv->val);
            lex_next(L);
          } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
            from_src[0] = 0;
          }
        } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
          snprintf(from_src, sizeof from_src, "%s", vm->last_str);
        }
      }
    }

    if (!nestk[0]) {
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASPOBJ|TYPEPOBJ|KEYSOBJ|NEEDPOBJ: empty nest");
      var_set_str(vm, "ERR", "HASPOBJ|TYPEPOBJ|KEYSOBJ|NEEDPOBJ: empty nest");
      bump(vm); return 1;
    }

    if (have_from) {
      const char *bp = from_src;
      while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++;
      if (*bp == '{')
        snprintf(plate, sizeof plate, "%s", from_src);
      else
        snprintf(plate, sizeof plate, "%s", "{}");
    } else {
      pv = var_get(vm, "PLATE", 0);
      if (pv && pv->is_str && pv->sval[0])
        snprintf(plate, sizeof plate, "%s", pv->sval);
      else
        snprintf(plate, sizeof plate, "%s", "{}");
    }

    /* peel nest object */
    nest[0] = 0;
    memset(&ngr, 0, sizeof ngr);
    if (cubalc_host_json_get_raw(plate, nestk, &ngr) == 0) {
      v = ngr.str;
      while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
      if (*v == '{') {
        if (v != ngr.str) {
          size_t n = strlen(v);
          memmove(ngr.str, v, n + 1);
        }
        snprintf(nest, sizeof nest, "%s", ngr.str);
      }
    }

    var_set_str(vm, "HASPOBJ_NEST", nestk);
    var_set_num(vm, "HASPOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "HASPOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));

    if (is_keys) {
      memset(&kr, 0, sizeof kr);
      if (nest[0] && cubalc_host_json_keys(nest, &kr) == 0) {
        var_set_str(vm, "LAST", kr.str);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", kr.str);
        vm->last_n = kr.n;
        var_set_num(vm, "LAST_N", kr.n);
        var_set_str(vm, "KEYSOBJ", kr.str);
        var_set_str(vm, "NESTKEYS", kr.str);
        var_set_num(vm, "KEYSOBJ_N", kr.n);
        var_set_num(vm, "NKEYS", kr.n);
        var_set_num(vm, "OK", 1);
      } else {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "KEYSOBJ", "");
        var_set_num(vm, "KEYSOBJ_N", 0);
        var_set_num(vm, "OK", nest[0] ? 1 : 0);
        if (!nest[0]) {
          var_set_str(vm, "LAST_ERR", "KEYSOBJ: nest miss or not object");
          var_set_str(vm, "ERR", "KEYSOBJ: nest miss or not object");
        }
      }
      if (vm->trace)
        fprintf(vm->trace, "# keysobj nest=%s n=%ld from=%d\n",
                nestk, vm->last_n, have_from);
      bump(vm); return 1;
    }

    if (is_has || is_type) {
      if (!field[0]) {
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "HASPOBJ|TYPEPOBJ: empty field");
        var_set_str(vm, "ERR", "HASPOBJ|TYPEPOBJ: empty field");
        bump(vm); return 1;
      }
      var_set_str(vm, "HASPOBJ_KEY", field);
      hit = 0;
      kind_s = "missing";
      kind_n = 0;
      if (nest[0]) {
        memset(&gr, 0, sizeof gr);
        if (is_has) {
          /* presence via raw get */
          if (cubalc_host_json_get_raw(nest, field, &gr) == 0)
            hit = 1;
        } else {
          if (cubalc_host_json_get_raw(nest, field, &gr) == 0) {
            hit = 1;
            v = gr.str;
            while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
            if (*v == '"') { kind_s = "str"; kind_n = 2; }
            else if (*v == '{') { kind_s = "obj"; kind_n = 5; }
            else if (*v == '[') { kind_s = "arr"; kind_n = 6; }
            else if (strncmp(v, "true", 4) == 0 || strncmp(v, "false", 5) == 0) {
              kind_s = "bool"; kind_n = 3;
            } else if (strncmp(v, "null", 4) == 0) {
              kind_s = "null"; kind_n = 4;
            } else if (*v == '-' || (*v >= '0' && *v <= '9')) {
              kind_s = "num"; kind_n = 1;
            } else {
              kind_s = "str"; kind_n = 2;
            }
          }
        }
      }
      if (is_has) {
        var_set_num(vm, "LAST_N", hit ? 1 : 0);
        vm->last_n = hit ? 1 : 0;
        var_set_str(vm, "LAST", hit ? "1" : "0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hit ? "1" : "0");
        var_set_num(vm, "HASPOBJ", hit ? 1 : 0);
        var_set_num(vm, "HASPOBJ_N", hit ? 1 : 0);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# haspobj nest=%s field=%s hit=%d\n",
                  nestk, field, hit);
      } else {
        var_set_str(vm, "LAST", kind_s);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", kind_s);
        vm->last_n = kind_n;
        var_set_num(vm, "LAST_N", kind_n);
        var_set_str(vm, "TYPEPOBJ", kind_s);
        var_set_str(vm, "NESTTYPE", kind_s);
        var_set_num(vm, "TYPEPOBJ_N", kind_n);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# typepobj nest=%s field=%s kind=%s\n",
                  nestk, field, kind_s);
      }
      bump(vm); return 1;
    }

    /* NEEDPOBJ */
    if (n_req == 0) {
      fail(vm, "NEEDPOBJ nest field… — need at least one field");
      return -1;
    }
    n_miss = 0;
    miss[0] = 0;
    if (!nest[0]) {
      /* all required missing */
      snprintf(miss, sizeof miss, "%s", keys_req);
      n_miss = n_req;
    } else {
      const char *p = keys_req;
      size_t mlen = 0;
      while (*p) {
        char fk[96];
        size_t kn = 0;
        while (*p == '\n' || *p == '\r') p++;
        if (!*p) break;
        while (*p && *p != '\n' && *p != '\r' && kn + 1 < sizeof fk)
          fk[kn++] = *p++;
        fk[kn] = 0;
        if (*p == '\n' || *p == '\r') p++;
        if (!fk[0]) continue;
        memset(&gr, 0, sizeof gr);
        if (cubalc_host_json_get_raw(nest, fk, &gr) != 0) {
          size_t fl = strlen(fk);
          if (mlen + fl + 2 < sizeof miss) {
            if (mlen) miss[mlen++] = '\n';
            memcpy(miss + mlen, fk, fl);
            mlen += fl;
            miss[mlen] = 0;
          }
          n_miss++;
        }
      }
    }
    var_set_str(vm, "NEEDPOBJ_MISS", miss);
    var_set_str(vm, "MISS", miss);
    var_set_num(vm, "NEEDPOBJ_N", n_req);
    var_set_num(vm, "NEEDPOBJ_MISS_N", n_miss);
    var_set_str(vm, "NEEDPOBJ_NEST", nestk);
    if (n_miss == 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "1");
      var_set_num(vm, "OK", 1);
      var_set_str(vm, "NEEDPOBJ", "ok");
      if (vm->trace)
        fprintf(vm->trace, "# needpobj nest=%s ok n=%d\n", nestk, n_req);
      bump(vm); return 1;
    }
    /* fail-fast like NEEDP */
    {
      char errb[320];
      snprintf(errb, sizeof errb, "NEEDPOBJ: missing in %s: %s", nestk, miss);
      /* collapse newlines in err for plate */
      for (char *e = errb; *e; e++) if (*e == '\n') *e = ',';
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", errb);
      var_set_str(vm, "ERR", errb);
      var_set_str(vm, "NEEDPOBJ", miss);
      fail(vm, errb);
      return -1;
    }
  }

  /* PLUCKOBJ|GETPALLNEST|NESTPLUCK [FROM plate] nest field… —
   * Multi-key peel from nested object → value bag (PLUCKP dual for nests).
   * Nest may be dotted/slash path: PLUCKOBJ "cfg.meta" "x" "role"
   * Empty field on miss · LAST_N = listed · PLUCKOBJ_HIT = found.
   * Soft nest miss → all empty fields, HIT=0.
   * Usability: no GETOBJ+PLUCKP or multi GETPOBJ+PUSH glue:
   *   PLUCKOBJ "meta" "x" "role" "zone"
   *   PLUCKOBJ "cfg.meta" "zone" "role"
   *   PLUCKOBJ FROM PEER "cfg" "port" "tls"
   */
  if (kw(&L->cur,"PLUCKOBJ") || kw(&L->cur,"GETPALLNEST") || kw(&L->cur,"NESTPLUCK") ||
      kw(&L->cur,"PLUCKNEST") || kw(&L->cur,"OBJPLUCK") || kw(&L->cur,"MPLUCKOBJ") ||
      kw(&L->cur,"GETALLNEST") || kw(&L->cur,"PEELNEST") || kw(&L->cur,"NESTGETALL")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[192], nest[CUBALC_HOST_STR_MAX];
    char keys_nl[CUBALC_HOST_STR_MAX], arg[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    char bag[CUBALC_HOST_STR_MAX];
    cubalc_host_result ngr, gr;
    int have_from = 0, n_req = 0, n_hit = 0, nest_hit = 0;
    size_t olen = 0, blen = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; nestk[0] = 0; nest[0] = 0; keys_nl[0] = 0;
    from_name[0] = 0; from_src[0] = 0; bag[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
        snprintf(from_name, sizeof from_name, "%s", "LAST");
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && pv->is_str) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(from_src, sizeof from_src, "%s", pv->sval);
          lex_next(L);
        } else if (pv) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(from_src, sizeof from_src, "%ld", pv->val);
          lex_next(L);
        } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
          from_src[0] = 0;
        }
      } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
      }
    }

    /* nest key */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(nestk, sizeof nestk, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
          kw(&L->cur,"END") || kw(&L->cur,"ASSERT") || kw(&L->cur,"LET")) {
        fail(vm, "PLUCKOBJ [FROM plate] nest field… — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, "PLUCKOBJ [FROM plate] nest field… — need nest key");
      return -1;
    }

    /* collect fields */
    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
           L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
           kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
           kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
           kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") || kw(&L->cur,"LET") ||
           kw(&L->cur,"SYS") || kw(&L->cur,"SETP") || kw(&L->cur,"GETP") ||
           kw(&L->cur,"GETPOBJ") || kw(&L->cur,"PLUCKP") || kw(&L->cur,"PLUCKOBJ") ||
           kw(&L->cur,"HASPOBJ") || kw(&L->cur,"NEEDPOBJ") || kw(&L->cur,"KEYSOBJ") ||
           kw(&L->cur,"INCLUDE") || kw(&L->cur,"REQUIRE") || kw(&L->cur,"FAIL") ||
           kw(&L->cur,"PASS") || kw(&L->cur,"NOTE") || kw(&L->cur,"EXIT")))
        break;
      arg[0] = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(arg, sizeof arg, "%ld", kv);
      } else if (resolve_str_arg(vm, L, arg, sizeof arg) != 0) {
        break;
      }
      if (!arg[0]) continue;
      if (olen > 0) {
        if (olen + 1 >= sizeof keys_nl) break;
        keys_nl[olen++] = '\n';
        keys_nl[olen] = 0;
      }
      {
        size_t al = strlen(arg);
        if (olen + al + 1 >= sizeof keys_nl) break;
        memcpy(keys_nl + olen, arg, al + 1);
        olen += al;
      }
      n_req++;
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
        snprintf(from_name, sizeof from_name, "%s", "LAST");
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && pv->is_str) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(from_src, sizeof from_src, "%s", pv->sval);
          lex_next(L);
        } else if (pv) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(from_src, sizeof from_src, "%ld", pv->val);
          lex_next(L);
        } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
          from_src[0] = 0;
        }
      } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
      }
    }

    if (!nestk[0] || n_req == 0) {
      if (n_req == 0) {
        fail(vm, "PLUCKOBJ nest field… — need at least one field");
        return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "PLUCKOBJ_HIT", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "PLUCKOBJ: empty nest");
      var_set_str(vm, "ERR", "PLUCKOBJ: empty nest");
      bump(vm); return 1;
    }

    if (have_from) {
      const char *bp = from_src;
      while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++;
      if (*bp == '{')
        snprintf(plate, sizeof plate, "%s", from_src);
      else
        snprintf(plate, sizeof plate, "%s", "{}");
    } else {
      pv = var_get(vm, "PLATE", 0);
      if (pv && pv->is_str && pv->sval[0])
        snprintf(plate, sizeof plate, "%s", pv->sval);
      else
        snprintf(plate, sizeof plate, "%s", "{}");
    }

    /* path-aware peel: "cfg.meta" · soft miss → empty bag HIT=0 */
    nest[0] = 0;
    memset(&ngr, 0, sizeof ngr);
    cubalc_host_json_path_obj(plate, nestk, &ngr);
    snprintf(nest, sizeof nest, "%s", ngr.str);
    nest_hit = (ngr.n != 0) ? 1 : 0;

    /* peel each field → bag (empty line on miss) */
    bag[0] = 0;
    blen = 0;
    n_hit = 0;
    if (nest_hit) {
      const char *pk = keys_nl;
      while (*pk) {
        char fk[96];
        size_t kn = 0;
        while (*pk == '\n' || *pk == '\r') pk++;
        if (!*pk) break;
        while (*pk && *pk != '\n' && *pk != '\r' && kn + 1 < sizeof fk)
          fk[kn++] = *pk++;
        fk[kn] = 0;
        if (*pk == '\n' || *pk == '\r') pk++;
        if (!fk[0]) continue;
        memset(&gr, 0, sizeof gr);
        if (cubalc_host_json_get(nest, fk, &gr) == 0) {
          size_t vl = strlen(gr.str);
          if (blen > 0) {
            if (blen + 1 < sizeof bag) bag[blen++] = '\n';
          }
          if (blen + vl + 1 < sizeof bag) {
            memcpy(bag + blen, gr.str, vl + 1);
            blen += vl;
          }
          n_hit++;
        } else {
          /* empty field on miss */
          if (blen > 0 && blen + 1 < sizeof bag) bag[blen++] = '\n';
          if (blen < sizeof bag) bag[blen] = 0;
        }
      }
    } else {
      /* nest miss: n_req empty fields */
      int i;
      for (i = 0; i < n_req; i++) {
        if (i > 0 && blen + 1 < sizeof bag) bag[blen++] = '\n';
        if (blen < sizeof bag) bag[blen] = 0;
      }
    }

    var_set_str(vm, "LAST", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n_req;
    var_set_num(vm, "LAST_N", n_req);
    var_set_str(vm, "PLUCKOBJ", bag);
    var_set_str(vm, "NESTPLUCK", bag);
    var_set_num(vm, "PLUCKOBJ_N", n_req);
    var_set_num(vm, "PLUCKOBJ_HIT", n_hit);
    var_set_num(vm, "PLUCKOBJ_MISS", n_req - n_hit);
    var_set_num(vm, "PLUCKP_HIT", n_hit); /* agent familiarity */
    var_set_str(vm, "PLUCKOBJ_NEST", nestk);
    var_set_num(vm, "PLUCKOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "PLUCKOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "PLUCKOBJ_NEST_HIT", nest_hit ? 1 : 0);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# pluckobj nest=%s n=%d hit=%d from=%d\n",
              nestk, n_req, n_hit, have_from);
    bump(vm); return 1;
  }

  return 0;
}
