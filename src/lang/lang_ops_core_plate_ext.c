/* CubalC lang — lang_ops_core_plate_ext.c
 * NEEDP · HASP · SAVEP · LOADP · DUMPP residual plate ops (core carve).
 */
#include "lang/cubalc_lang_ops_core_plate_ext.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

int cubalc_lang_ops_core_try_plate_ext(VM *vm, Lex *L) {
  /* NEEDP|REQUIREP [FROM plate] key… — fail-fast if plate missing any key.
   * HASP [FROM plate] key — soft single-key presence → LAST_N 0|1 (no ERR on miss).
   * HASPALL [FROM plate] key… — soft multi-key all-present → LAST_N 0|1.
   * HASPANY|NEEDPANY [FROM plate] key… — multi-key any-of (HASPALL twin).
   * KEYSP [FROM plate] [path] — bag of keys → LAST · LAST_N=count.
   * Optional path peels nest object first (KEYSOBJ dual without separate form):
   *   KEYSP "cfg" · KEYSP "cfg.flags" · KEYSP FROM PEER "meta"
   * Keys for NEEDP/HASP may be dotted/slash nest paths (same as GETP "freq.error"):
   *   NEEDP "host" "freq.error"
   *   HASPALL "meta.role" "meta.n" FROM PEER
   *   HASPANY "status" "ready" FROM PEER
   * FROM: multi-plate twin of SETP/MERGEP FROM (probe without clobbering PLATE):
   *   HASP FROM peer "host"
   *   NEEDP "agent" "n" FROM session
   *   KEYSP FROM peer
   * NEEDP_FROM/HASP_FROM/HASPALL_FROM/HASPANY_FROM/NEEDPANY_FROM/KEYSP_FROM = 0|1.
   * NEEDP lists missing keys in error (soft twin HASPALL / JSONMISS).
   * NEEDPANY fail-fast if none present (soft twin HASPANY · KEYHAVE bag). */
  if (kw(&L->cur,"NEEDP") || kw(&L->cur,"REQUIREP") || kw(&L->cur,"PLATE_NEED") ||
      kw(&L->cur,"NEEDPLATEK") || kw(&L->cur,"REQUIRE_P") ||
      kw(&L->cur,"HASP") || kw(&L->cur,"PLATE_HAS") || kw(&L->cur,"MHASP") ||
      kw(&L->cur,"HASPALL") || kw(&L->cur,"PLATE_HASALL") || kw(&L->cur,"MHASPALL") ||
      kw(&L->cur,"HASPANY") || kw(&L->cur,"PLATE_HASANY") || kw(&L->cur,"MHASPANY") ||
      kw(&L->cur,"HAS_P_ANY") || kw(&L->cur,"ANYKEYSP") || kw(&L->cur,"HASSOMEP") ||
      kw(&L->cur,"NEEDPANY") || kw(&L->cur,"REQUIREPANY") || kw(&L->cur,"MUSTPANY") ||
      kw(&L->cur,"NEED_P_ANY") || kw(&L->cur,"REQUIRE_P_ANY") || kw(&L->cur,"PLATE_NEEDANY") ||
      kw(&L->cur,"KEYSP") || kw(&L->cur,"PLATE_KEYS") || kw(&L->cur,"MKEYSP") ||
      kw(&L->cur,"KEYS_P") || kw(&L->cur,"LISTPKEYS")) {
    char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
    char arg[CUBALC_HOST_STR_MAX], key[96];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr, miss;
    int is_need = 0, is_has = 0, is_hasall = 0, is_hasany = 0, is_needany = 0, is_keys = 0;
    int have_from = 0;
    size_t olen = 0;
    int aln = L->cur.line;
    Var *pv;

    if (kw(&L->cur,"NEEDPANY") || kw(&L->cur,"REQUIREPANY") || kw(&L->cur,"MUSTPANY") ||
        kw(&L->cur,"NEED_P_ANY") || kw(&L->cur,"REQUIRE_P_ANY") || kw(&L->cur,"PLATE_NEEDANY"))
      is_needany = 1;
    else if (kw(&L->cur,"NEEDP") || kw(&L->cur,"REQUIREP") || kw(&L->cur,"PLATE_NEED") ||
        kw(&L->cur,"NEEDPLATEK") || kw(&L->cur,"REQUIRE_P"))
      is_need = 1;
    else if (kw(&L->cur,"HASPANY") || kw(&L->cur,"PLATE_HASANY") || kw(&L->cur,"MHASPANY") ||
             kw(&L->cur,"HAS_P_ANY") || kw(&L->cur,"ANYKEYSP") || kw(&L->cur,"HASSOMEP"))
      is_hasany = 1;
    else if (kw(&L->cur,"HASPALL") || kw(&L->cur,"PLATE_HASALL") || kw(&L->cur,"MHASPALL"))
      is_hasall = 1;
    else if (kw(&L->cur,"HASP") || kw(&L->cur,"PLATE_HAS") || kw(&L->cur,"MHASP"))
      is_has = 1;
    else
      is_keys = 1;

    lex_next(L);
    plate[0] = 0; keys_nl[0] = 0; key[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

    /* optional leading FROM plate_src */
    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    /* KEYSP: optional path · optional trailing FROM · resolve plate · keys at path */
    if (is_keys) {
      char nest_path[192];
      cubalc_host_result obj;
      nest_path[0] = 0;
      /* optional nest path before trailing FROM: KEYSP "cfg.flags" */
      if (L->cur.kind == TK_STR ||
          (L->cur.kind == TK_IDENT &&
           !kw(&L->cur,"FROM") && !kw(&L->cur,"USING") && !kw(&L->cur,"OF") &&
           !kw(&L->cur,"WITHPLATE") && !kw(&L->cur,"PLATEFROM") &&
           !kw(&L->cur,"END") && !kw(&L->cur,"ELSE") && !kw(&L->cur,"ELIF"))) {
        if (resolve_str_arg(vm, L, nest_path, sizeof nest_path) != 0)
          nest_path[0] = 0;
      }
      if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                         kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                         kw(&L->cur,"PLATEFROM"))) {
        lex_next(L);
        have_from = 1;
        if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
          snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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
        /* path after FROM PEER: KEYSP FROM PEER "cfg" */
        if (!nest_path[0] &&
            (L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT &&
              !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ELSE")))) {
          if (resolve_str_arg(vm, L, nest_path, sizeof nest_path) != 0)
            nest_path[0] = 0;
        }
      }
      if (have_from) {
        const char *b = from_src;
        while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
        if (*b == '{')
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
      /* peel nest object when path given (soft miss → empty keys) */
      memset(&obj, 0, sizeof obj);
      if (cubalc_host_json_path_obj(plate, nest_path[0] ? nest_path : NULL, &obj) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "KEYSP_N", 0);
        var_set_num(vm, "KEYSP_FROM", have_from ? 1 : 0);
        var_set_str(vm, "KEYSP_PATH", nest_path);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", obj.err[0] ? obj.err : "KEYSP: path fail");
        var_set_str(vm, "ERR", obj.err[0] ? obj.err : "KEYSP: path fail");
        bump(vm); return 1;
      }
      snprintf(plate, sizeof plate, "%s", obj.str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_keys(plate, &hr) != 0) {
        /* soft empty bag (non-object already normalized by path_obj) */
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "KEYSP_N", 0);
        var_set_num(vm, "KEYSP_FROM", have_from ? 1 : 0);
        var_set_str(vm, "KEYSP_PATH", nest_path);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "KEYSP", hr.str);
      var_set_num(vm, "KEYSP_N", hr.n);
      var_set_num(vm, "JSONKEYS_N", hr.n);
      var_set_num(vm, "KEYSP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "KEYSP_PATH", nest_path);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# keysp n=%ld from=%d path=%s\n",
                hr.n, have_from, nest_path[0] ? nest_path : ".");
      bump(vm); return 1;
    }

    if (is_has) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(key, sizeof key, "%ld", kv);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
            kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
          fail(vm, "HASP [FROM plate] key — need key");
          return -1;
        }
        if (resolve_str_arg(vm, L, key, sizeof key) != 0)
          key[0] = 0;
      } else {
        fail(vm, "HASP [FROM plate] key — need key");
        return -1;
      }
      /* trailing FROM */
      if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                         kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                         kw(&L->cur,"PLATEFROM"))) {
        lex_next(L);
        have_from = 1;
        if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
          snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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
      if (have_from) {
        const char *b = from_src;
        while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
        if (*b == '{')
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
      if (!key[0]) {
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_num(vm, "HASP_N", 0);
        var_set_num(vm, "HASP_FROM", have_from ? 1 : 0);
        var_set_num(vm, "OK", 1); /* soft probe */
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_path_has(plate, key, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "HASP_N", 0);
        var_set_num(vm, "HASP_HIT", 0);
        var_set_str(vm, "HASP_KEY", key);
        var_set_num(vm, "HASP_FROM", have_from ? 1 : 0);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
      } else if (hr.n) {
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_num(vm, "HASP_N", 1);
        var_set_num(vm, "HASP_HIT", 1);
        var_set_str(vm, "HASP_KEY", key);
        var_set_num(vm, "HASP_FROM", have_from ? 1 : 0);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "1");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "1");
      } else {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "HASP_N", 0);
        var_set_num(vm, "HASP_HIT", 0);
        var_set_str(vm, "HASP_KEY", key);
        var_set_num(vm, "HASP_FROM", have_from ? 1 : 0);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
      }
      if (vm->trace)
        fprintf(vm->trace, "# hasp key=%s hit=%ld from=%d\n", key, vm->last_n, have_from);
      bump(vm); return 1;
    }

    /* NEEDP / HASPALL: collect keys; stop before trailing FROM / statements */
    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
           L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
           kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
           kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
           kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
           kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
           kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"SETP") ||
           kw(&L->cur,"INCP") || kw(&L->cur,"DELP") || kw(&L->cur,"GETP") ||
           kw(&L->cur,"MERGEP") || kw(&L->cur,"DEFAULTP") || kw(&L->cur,"TOGGLEP") ||
           kw(&L->cur,"NEEDP") || kw(&L->cur,"HASP") || kw(&L->cur,"KEYSP") ||
           kw(&L->cur,"HASPALL") || kw(&L->cur,"HASPANY") || kw(&L->cur,"NEEDPANY") ||
           kw(&L->cur,"REQUIREPANY") || kw(&L->cur,"MUSTPANY") ||
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
      if (strchr(arg, '\n') || strchr(arg, ',') || strchr(arg, ' ') ||
          strchr(arg, '|') || strchr(arg, ':')) {
        const char *p = arg;
        while (*p) {
          char tok[96];
          size_t tl = 0;
          while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' ||
                 *p == ',' || *p == ':' || *p == '|')
            p++;
          if (!*p) break;
          while (p[tl] && p[tl] != '\n' && p[tl] != '\r' && p[tl] != ',' &&
                 p[tl] != ' ' && p[tl] != '\t' && p[tl] != ':' && p[tl] != '|' &&
                 tl + 1 < sizeof tok) {
            tok[tl] = p[tl];
            tl++;
          }
          tok[tl] = 0;
          p += tl;
          if (!tok[0]) continue;
          if (olen > 0) {
            if (olen + 1 >= sizeof keys_nl) break;
            keys_nl[olen++] = '\n';
            keys_nl[olen] = 0;
          }
          {
            size_t al = strlen(tok);
            if (olen + al + 1 >= sizeof keys_nl) break;
            memcpy(keys_nl + olen, tok, al + 1);
            olen += al;
          }
        }
        continue;
      }
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
    }
    /* trailing FROM after keys */
    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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
    if (have_from) {
      const char *b = from_src;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b == '{')
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

    if (!keys_nl[0]) {
      if (is_need) {
        fail(vm, "NEEDP [FROM plate] key… — need at least one required key");
        return -1;
      }
      if (is_needany) {
        fail(vm, "NEEDPANY [FROM plate] key… — need at least one candidate key");
        return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "HASPALL_N", 0);
      var_set_num(vm, "HASPALL_FROM", have_from ? 1 : 0);
      var_set_num(vm, "HASPANY_N", 0);
      var_set_num(vm, "HASPANY_FROM", have_from ? 1 : 0);
      var_set_num(vm, "NEEDPANY_N", 0);
      var_set_num(vm, "NEEDPANY_FROM", have_from ? 1 : 0);
      var_set_num(vm, "NEEDP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "KEYHAVE", "");
      var_set_str(vm, "KEYMISS", "");
      var_set_num(vm, "KEYHAVE_N", 0);
      var_set_num(vm, "KEYMISS_N", 0);
      if (is_hasany) {
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "HASPANY: need keys — HASPANY status ready");
        var_set_str(vm, "ERR", "HASPANY: need keys — HASPANY status ready");
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "0");
      } else {
        var_set_num(vm, "OK", 1);
      }
      bump(vm); return 1;
    }

    memset(&hr, 0, sizeof hr);
    /* want_all=0 for any-of; 1 for all-of */
    if (cubalc_host_json_has_keys(plate, keys_nl,
                                  (is_hasany || is_needany) ? 0 : 1, &hr) != 0) {
      if (is_need || is_needany) {
        fail(vm, is_needany ? "NEEDPANY: plate key check failed" : "NEEDP: plate key check failed");
        return -1;
      }
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "HASPALL_N", 0);
      var_set_num(vm, "HASPALL_FROM", have_from ? 1 : 0);
      var_set_num(vm, "HASPANY_N", 0);
      var_set_num(vm, "HASPANY_FROM", have_from ? 1 : 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : ((is_hasany) ? "HASPANY: fail" : "HASPALL: fail"));
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : ((is_hasany) ? "HASPANY: fail" : "HASPALL: fail"));
      bump(vm); return 1;
    }

    if (is_needany || is_hasany) {
      cubalc_host_result have, missb;
      char flat[160];
      size_t mi = 0;
      const char *mp;
      long any_hit = hr.n; /* 0|1 any present */
      long nname = 0;
      const char *p;
      memset(&have, 0, sizeof have);
      memset(&missb, 0, sizeof missb);
      cubalc_host_json_filter_req_keys(plate, keys_nl, 1, &have);
      cubalc_host_json_filter_req_keys(plate, keys_nl, 0, &missb);
      var_set_str(vm, "KEYHAVE", have.str);
      var_set_str(vm, "KEYMISS", missb.str);
      var_set_num(vm, "KEYHAVE_N", have.n);
      var_set_num(vm, "KEYMISS_N", missb.n);
      var_set_num(vm, "HASPANY_N", have.n);
      var_set_num(vm, "HASPANY_HIT", any_hit);
      var_set_num(vm, "HASPANY_FROM", have_from ? 1 : 0);
      p = keys_nl;
      if (*p) nname = 1;
      for (; *p; p++) if (*p == '\n') nname++;
      var_set_num(vm, "NEEDPANY_N", nname);
      var_set_num(vm, "HASPANY_KEYS_N", nname);
      var_set_num(vm, "NEEDPANY_FROM", have_from ? 1 : 0);
      if (any_hit) {
        var_set_num(vm, "LAST_N", 1);
        vm->last_n = 1;
        var_set_str(vm, "LAST", "1");
        snprintf(vm->last_str, sizeof vm->last_str, "1");
        var_set_num(vm, "OK", 1);
        if (is_needany && vm->res) vm->res->asserts_ok++;
        if (vm->trace)
          fprintf(vm->trace, "# %s hit have=%ld from=%d\n",
                  is_needany ? "needpany" : "haspany", have.n, have_from);
        bump(vm); return 1;
      }
      if (is_needany) {
        char msg[320];
        flat[0] = 0; mi = 0;
        for (mp = keys_nl; *mp && mi + 1 < sizeof flat; mp++) {
          if (*mp == '\n' || *mp == '\r') {
            if (mi > 0 && flat[mi - 1] != ',') {
              flat[mi++] = ',';
              if (mi + 1 < sizeof flat) flat[mi++] = ' ';
            }
          } else {
            flat[mi++] = *mp;
          }
        }
        flat[mi] = 0;
        if (!flat[0]) snprintf(flat, sizeof flat, "?");
        snprintf(msg, sizeof msg,
                 "NEEDPANY miss line %d: need one of [%s] — soft twin HASPANY / KEYHAVE",
                 aln, flat);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      /* soft HASPANY miss */
      {
        char em[240];
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "0");
        snprintf(em, sizeof em,
                 "HASPANY miss (0/%ld): need one of %s — NEEDPANY · SETP",
                 nname, missb.str[0] ? missb.str : "?");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", em);
        var_set_str(vm, "ERR", em);
        if (vm->trace)
          fprintf(vm->trace, "# haspany miss n=%ld from=%d\n", nname, have_from);
        bump(vm); return 1;
      }
    }

    if (is_need) {
      if (hr.n == 0) {
        char msg[320], flat[160];
        size_t mi = 0;
        const char *mp;
        memset(&miss, 0, sizeof miss);
        cubalc_host_json_filter_req_keys(plate, keys_nl, 0, &miss);
        flat[0] = 0;
        for (mp = miss.str; *mp && mi + 1 < sizeof flat; mp++) {
          if (*mp == '\n' || *mp == '\r') {
            if (mi > 0 && flat[mi - 1] != ',') {
              flat[mi++] = ',';
              if (mi + 1 < sizeof flat) flat[mi++] = ' ';
            }
          } else {
            flat[mi++] = *mp;
          }
        }
        flat[mi] = 0;
        if (!flat[0])
          snprintf(flat, sizeof flat, "?");
        snprintf(msg, sizeof msg,
                 "NEEDP missing line %d: %s — soft twin HASPALL / JSONMISS",
                 aln, flat);
        var_set_num(vm, "NEEDP_FROM", have_from ? 1 : 0);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "NEEDP_N", 1);
      var_set_num(vm, "NEEDP_HIT", (long)hr.code);
      var_set_num(vm, "REQUIREP_N", 1);
      var_set_num(vm, "NEEDP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "OK", 1);
      if (vm->res) vm->res->asserts_ok++;
      if (vm->trace)
        fprintf(vm->trace, "# needp hit=%d ok from=%d\n", hr.code, have_from);
      bump(vm); return 1;
    }

    /* HASPALL soft */
    {
      long hit = hr.n; /* 0|1 all present */
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "HASPALL_N", hit);
      var_set_num(vm, "HASPALL_HIT", (long)hr.code);
      var_set_num(vm, "JSONHASALL_HIT", (long)hr.code);
      var_set_num(vm, "HASPALL_FROM", have_from ? 1 : 0);
      if (hit) {
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      } else {
        memset(&miss, 0, sizeof miss);
        cubalc_host_json_filter_req_keys(plate, keys_nl, 0, &miss);
        var_set_str(vm, "LAST", miss.str);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", miss.str);
        var_set_str(vm, "HASPALL_MISS", miss.str);
        var_set_num(vm, "HASPALL_MISS_N", miss.n);
      }
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# haspall hit=%ld found=%d from=%d\n",
                hit, hr.code, have_from);
      bump(vm); return 1;
    }
  }
  /* HITKEYP|FIRSTKEYP|WHICHKEYP [FROM plate] key…
   * — first present any-of KEY name from PLATE (or FROM named plate).
   * Multi-plate dual of SYS JSONHITKEY · plate_session agents name WHICH preferred
   * key hit without SYS JSONHITKEY PLATE glue / multi HASP+IF.
   * LAST=key · LAST_N 0|1 · HITKEYP_N=1-based · HITKEYP_HIT · HITKEYP_FROM.
   * Null/false/0/empty still present (presence dual of COALESCEP substance). */
  if (kw(&L->cur,"HITKEYP") || kw(&L->cur,"FIRSTKEYP") || kw(&L->cur,"PLATEHITKEYP") ||
      kw(&L->cur,"HITKEY_P") || kw(&L->cur,"WHICHKEYP") || kw(&L->cur,"PLATEFIRSTKEY") ||
      kw(&L->cur,"MFIRSTKEYP") || kw(&L->cur,"KEYHITP") || kw(&L->cur,"FIRSTPRESENTP") ||
      kw(&L->cur,"HITPKEY")) {
    char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
    char arg[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
    size_t olen = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; keys_nl[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
           L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"END") || kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") ||
           kw(&L->cur,"IF") || kw(&L->cur,"LET") || kw(&L->cur,"SYS") ||
           kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") || kw(&L->cur,"PASS") ||
           kw(&L->cur,"FAIL") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
           kw(&L->cur,"LOOP") || kw(&L->cur,"JSON") || kw(&L->cur,"SETP") ||
           kw(&L->cur,"GETP") || kw(&L->cur,"HASP") || kw(&L->cur,"NEEDP") ||
           kw(&L->cur,"HASPANY") || kw(&L->cur,"COALESCEP") || kw(&L->cur,"NVLP") ||
           kw(&L->cur,"HITKEYP") || kw(&L->cur,"FIRSTKEYP") || kw(&L->cur,"FROM") ||
           kw(&L->cur,"USING") || kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
           kw(&L->cur,"PLATEFROM") || kw(&L->cur,"WITH") || kw(&L->cur,"AS"))) {
        break;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(arg, sizeof arg, "%ld", nv);
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
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    if (have_from) {
      const char *b = from_src;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b == '{')
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

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_first_key(plate, keys_nl, &hr) != 0) {
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HITKEYP_N", 0);
      var_set_num(vm, "HITKEYP_HIT", 0);
      var_set_num(vm, "FIRSTKEYP_N", 0);
      var_set_num(vm, "HITKEYP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "OK", 0);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", "HITKEYP: fail");
        var_set_str(vm, "ERR", "HITKEYP: fail");
      }
      bump(vm); return 1;
    }
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "HITKEYP_N", (long)hr.code);
    var_set_num(vm, "HITKEYP_HIT", hr.n);
    var_set_num(vm, "FIRSTKEYP_N", (long)hr.code);
    var_set_num(vm, "FIRSTKEYP_HIT", hr.n);
    var_set_num(vm, "JSONHITKEY_N", (long)hr.code);
    var_set_num(vm, "JSONHITKEY_HIT", hr.n);
    var_set_num(vm, "HITKEYP_FROM", have_from ? 1 : 0);
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# hitkeyp hit=%ld idx=%d key=%s from=%d\n",
              hr.n, hr.code, hr.str, have_from);
    bump(vm); return 1;
  }

  /* HITVALP|FIRSTVALP|WHICHVALP [FROM plate] key…
   * — first present any-of VALUE from PLATE (or FROM named plate).
   * Multi-plate dual of SYS JSONHITVAL · plate_session agents peel preferred
   * value without SYS JSONHITVAL PLATE glue / multi GETP+IF.
   * LAST=value · LAST_N 0|1 · HITVALP_N=1-based · HITVALP_HIT · HITVALP_KEY · HITVALP_FROM.
   * Null/false/0/empty still present (presence dual of COALESCEP substance). */
  if (kw(&L->cur,"HITVALP") || kw(&L->cur,"FIRSTVALP") || kw(&L->cur,"PLATEHITVALP") ||
      kw(&L->cur,"HITVAL_P") || kw(&L->cur,"WHICHVALP") || kw(&L->cur,"PLATEFIRSTVAL") ||
      kw(&L->cur,"MFIRSTVALP") || kw(&L->cur,"VALHITP") || kw(&L->cur,"FIRSTPRESENTVALP") ||
      kw(&L->cur,"HITPVAL")) {
    char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
    char arg[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
    size_t olen = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; keys_nl[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
           L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"END") || kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") ||
           kw(&L->cur,"IF") || kw(&L->cur,"LET") || kw(&L->cur,"SYS") ||
           kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") || kw(&L->cur,"PASS") ||
           kw(&L->cur,"FAIL") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
           kw(&L->cur,"LOOP") || kw(&L->cur,"JSON") || kw(&L->cur,"SETP") ||
           kw(&L->cur,"GETP") || kw(&L->cur,"HASP") || kw(&L->cur,"NEEDP") ||
           kw(&L->cur,"HASPANY") || kw(&L->cur,"COALESCEP") || kw(&L->cur,"NVLP") ||
           kw(&L->cur,"HITVALP") || kw(&L->cur,"FIRSTVALP") || kw(&L->cur,"FROM") ||
           kw(&L->cur,"USING") || kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
           kw(&L->cur,"PLATEFROM") || kw(&L->cur,"WITH") || kw(&L->cur,"AS"))) {
        break;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(arg, sizeof arg, "%ld", nv);
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
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    if (have_from) {
      const char *b = from_src;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b == '{')
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

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_first_val(plate, keys_nl, &hr) != 0) {
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HITVALP_N", 0);
      var_set_num(vm, "HITVALP_HIT", 0);
      var_set_num(vm, "FIRSTVALP_N", 0);
      var_set_num(vm, "HITVALP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "HITVALP_KEY", "");
      var_set_str(vm, "FIRSTVALP_KEY", "");
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "OK", 0);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", "HITVALP: fail");
        var_set_str(vm, "ERR", "HITVALP: fail");
      }
      bump(vm); return 1;
    }
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "HITVALP_N", (long)hr.code);
    var_set_num(vm, "HITVALP_HIT", hr.n);
    var_set_num(vm, "FIRSTVALP_N", (long)hr.code);
    var_set_num(vm, "FIRSTVALP_HIT", hr.n);
    var_set_num(vm, "JSONHITVAL_N", (long)hr.code);
    var_set_num(vm, "JSONHITVAL_HIT", hr.n);
    var_set_num(vm, "HITVALP_FROM", have_from ? 1 : 0);
    var_set_str(vm, "HITVALP_KEY", hr.err);
    var_set_str(vm, "FIRSTVALP_KEY", hr.err);
    var_set_str(vm, "JSONHITVAL_KEY", hr.err);
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# hitvalp hit=%ld idx=%d key=%s val=%s from=%d\n",
              hr.n, hr.code, hr.err, hr.str, have_from);
    bump(vm); return 1;
  }

  /* MISSP|MISSINGP|ABSENTP [FROM plate] key...
   * PRESENTP|FOUNDP|HAVEP [FROM plate] key...
   * -- bag of missing/present required keys from PLATE (or FROM named plate).
   * Multi-plate dual of SYS JSONMISS / JSONPRESENT.
   * LAST=bag · LAST_N=count · MISSP_N / PRESENTP_N · MISSP_FROM / PRESENTP_FROM.
   * Null/false/0/empty still present (presence dual of COALESCEP substance). */
  if (kw(&L->cur,"MISSP") || kw(&L->cur,"MISSINGP") || kw(&L->cur,"PLATEMISSP") ||
      kw(&L->cur,"ABSENTP") || kw(&L->cur,"MISS_P") || kw(&L->cur,"KEYMISSP") ||
      kw(&L->cur,"MMISSP") || kw(&L->cur,"MISSINGKEYS_P") ||
      kw(&L->cur,"MISSINGKEYSP") ||
      kw(&L->cur,"PRESENTP") || kw(&L->cur,"FOUNDP") || kw(&L->cur,"PLATEPRESENTP") ||
      kw(&L->cur,"HAVEP") || kw(&L->cur,"PRESENT_P") || kw(&L->cur,"KEYPRESENTP") ||
      kw(&L->cur,"MPRESENTP") || kw(&L->cur,"PRESENTKEYS_P") || kw(&L->cur,"FOUNDKEYS_P") ||
      kw(&L->cur,"FOUNDKEYSP")) {
    char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
    char arg[CUBALC_HOST_STR_MAX], op[32];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, want_present = 0;
    size_t olen = 0;
    Var *pv;

    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "PRESENTP") == 0 || strcmp(op, "FOUNDP") == 0 ||
        strcmp(op, "PLATEPRESENTP") == 0 || strcmp(op, "HAVEP") == 0 ||
        strcmp(op, "PRESENT_P") == 0 || strcmp(op, "KEYPRESENTP") == 0 ||
        strcmp(op, "MPRESENTP") == 0 || strcmp(op, "PRESENTKEYS_P") == 0 ||
        strcmp(op, "FOUNDKEYS_P") == 0 || strcmp(op, "FOUNDKEYSP") == 0)
      want_present = 1;

    lex_next(L);
    plate[0] = 0; keys_nl[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
           L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"END") || kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") ||
           kw(&L->cur,"IF") || kw(&L->cur,"LET") || kw(&L->cur,"SYS") ||
           kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") || kw(&L->cur,"PASS") ||
           kw(&L->cur,"FAIL") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
           kw(&L->cur,"LOOP") || kw(&L->cur,"JSON") || kw(&L->cur,"SETP") ||
           kw(&L->cur,"GETP") || kw(&L->cur,"HASP") || kw(&L->cur,"NEEDP") ||
           kw(&L->cur,"HASPANY") || kw(&L->cur,"COALESCEP") || kw(&L->cur,"NVLP") ||
           kw(&L->cur,"HITVALP") || kw(&L->cur,"FIRSTVALP") || kw(&L->cur,"HITKEYP") ||
           kw(&L->cur,"MISSP") || kw(&L->cur,"PRESENTP") || kw(&L->cur,"EXACTP") || kw(&L->cur,"SCHEMAP") || kw(&L->cur,"FROM") ||
           kw(&L->cur,"USING") || kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
           kw(&L->cur,"PLATEFROM") || kw(&L->cur,"WITH") || kw(&L->cur,"AS"))) {
        break;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(arg, sizeof arg, "%ld", nv);
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
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    if (have_from) {
      const char *b = from_src;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b == '{')
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

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_filter_req_keys(plate, keys_nl, want_present, &hr) != 0) {
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      if (want_present) {
        var_set_num(vm, "PRESENTP_N", 0);
        var_set_num(vm, "FOUNDP_N", 0);
        var_set_num(vm, "JSONPRESENT_N", 0);
      } else {
        var_set_num(vm, "MISSP_N", 0);
        var_set_num(vm, "MISSINGP_N", 0);
        var_set_num(vm, "JSONMISS_N", 0);
      }
      var_set_num(vm, want_present ? "PRESENTP_FROM" : "MISSP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "OK", 0);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", want_present ? "PRESENTP: fail" : "MISSP: fail");
        var_set_str(vm, "ERR", want_present ? "PRESENTP: fail" : "MISSP: fail");
      }
      bump(vm); return 1;
    }
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    if (want_present) {
      var_set_num(vm, "PRESENTP_N", hr.n);
      var_set_num(vm, "FOUNDP_N", hr.n);
      var_set_num(vm, "JSONPRESENT_N", hr.n);
      var_set_num(vm, "JSONFOUND_N", hr.n);
      var_set_num(vm, "PRESENTP_FROM", have_from ? 1 : 0);
    } else {
      var_set_num(vm, "MISSP_N", hr.n);
      var_set_num(vm, "MISSINGP_N", hr.n);
      var_set_num(vm, "JSONMISS_N", hr.n);
      var_set_num(vm, "JSONMISSING_N", hr.n);
      var_set_num(vm, "MISSP_FROM", have_from ? 1 : 0);
    }
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld from=%d bag=%s\n",
              want_present ? "presentp" : "missp", hr.n, have_from, hr.str);
    bump(vm); return 1;
  }

    /* EXTRAP|UNKNOWNP|EXTRAKEYSP [FROM plate] allow...
   * KNOWNP|ALLOWEDKEYSP|KNOWNKEYSP [FROM plate] allow...
   * -- bag of plate keys outside/inside allow-list from PLATE (or FROM named plate).
   * Multi-plate dual of SYS JSONEXTRA / JSONKNOWN.
   * LAST=bag · LAST_N=count · EXTRAP_N / KNOWNP_N · EXTRAP_FROM / KNOWNP_FROM.
   * Empty allow-list: EXTRA = all plate keys; KNOWN = empty. */
  if (kw(&L->cur,"EXTRAP") || kw(&L->cur,"EXTRAKEYS_P") || kw(&L->cur,"PLATEEXTRAP") ||
      kw(&L->cur,"EXTRAKEYSP") || kw(&L->cur,"EXTRA_P") || kw(&L->cur,"KEYEXTRAP") ||
      kw(&L->cur,"MEXTRAP") || kw(&L->cur,"UNKNOWNKEYS_P") ||
      kw(&L->cur,"UNKNOWNKEYSP") ||
      kw(&L->cur,"KNOWNP") || kw(&L->cur,"ALLOWEDKEYSP") || kw(&L->cur,"PLATEKNOWNP") ||
      kw(&L->cur,"KNOWNKEYSP") || kw(&L->cur,"KNOWN_P") || kw(&L->cur,"KEYKNOWNP") ||
      kw(&L->cur,"MKNOWNP") || kw(&L->cur,"KNOWNKEYS_P") || kw(&L->cur,"ALLOWEDKEYS_P") ||
      kw(&L->cur,"ALLOWEDKEYSP")) {
    char plate[CUBALC_HOST_STR_MAX], allowed_nl[CUBALC_HOST_STR_MAX];
    char arg[CUBALC_HOST_STR_MAX], op[32];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, want_extra = 1;
    size_t olen = 0;
    Var *pv;

    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "KNOWNP") == 0 || strcmp(op, "ALLOWEDKEYSP") == 0 ||
        strcmp(op, "PLATEKNOWNP") == 0 || strcmp(op, "KNOWNKEYSP") == 0 ||
        strcmp(op, "KNOWN_P") == 0 || strcmp(op, "KEYKNOWNP") == 0 ||
        strcmp(op, "MKNOWNP") == 0 || strcmp(op, "KNOWNKEYS_P") == 0 ||
        strcmp(op, "ALLOWEDKEYS_P") == 0 || strcmp(op, "ALLOWEDKEYSP") == 0)
      want_extra = 0;

    lex_next(L);
    plate[0] = 0; allowed_nl[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
           L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"END") || kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") ||
           kw(&L->cur,"IF") || kw(&L->cur,"LET") || kw(&L->cur,"SYS") ||
           kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") || kw(&L->cur,"PASS") ||
           kw(&L->cur,"FAIL") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
           kw(&L->cur,"LOOP") || kw(&L->cur,"JSON") || kw(&L->cur,"SETP") ||
           kw(&L->cur,"GETP") || kw(&L->cur,"HASP") || kw(&L->cur,"NEEDP") ||
           kw(&L->cur,"HASPANY") || kw(&L->cur,"COALESCEP") || kw(&L->cur,"NVLP") ||
           kw(&L->cur,"HITVALP") || kw(&L->cur,"FIRSTVALP") || kw(&L->cur,"HITKEYP") ||
           kw(&L->cur,"EXTRAP") || kw(&L->cur,"KNOWNP") || kw(&L->cur,"EXACTP") || kw(&L->cur,"SCHEMAP") || kw(&L->cur,"ONLYP") || kw(&L->cur,"NOEXTRAP") || kw(&L->cur,"FROM") ||
           kw(&L->cur,"USING") || kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
           kw(&L->cur,"PLATEFROM") || kw(&L->cur,"WITH") || kw(&L->cur,"AS"))) {
        break;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(arg, sizeof arg, "%ld", nv);
      } else if (resolve_str_arg(vm, L, arg, sizeof arg) != 0) {
        break;
      }
      if (!arg[0]) continue;
      if (olen > 0) {
        if (olen + 1 >= sizeof allowed_nl) break;
        allowed_nl[olen++] = '\n';
        allowed_nl[olen] = 0;
      }
      {
        size_t al = strlen(arg);
        if (olen + al + 1 >= sizeof allowed_nl) break;
        memcpy(allowed_nl + olen, arg, al + 1);
        olen += al;
      }
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    if (have_from) {
      const char *b = from_src;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b == '{')
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

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_filter_plate_keys(plate, allowed_nl, want_extra, &hr) != 0) {
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      if (want_extra) {
        var_set_num(vm, "EXTRAP_N", 0);
        var_set_num(vm, "UNKNOWNKEYSP_N", 0);
        var_set_num(vm, "JSONEXTRA_N", 0);
        var_set_num(vm, "JSONUNKNOWN_N", 0);
      } else {
        var_set_num(vm, "KNOWNP_N", 0);
        var_set_num(vm, "ALLOWEDKEYSP_N", 0);
        var_set_num(vm, "JSONKNOWN_N", 0);
        var_set_num(vm, "JSONALLOWED_N", 0);
      }
      var_set_num(vm, want_extra ? "EXTRAP_FROM" : "KNOWNP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "OK", 0);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", want_extra ? "EXTRAP: fail" : "KNOWNP: fail");
        var_set_str(vm, "ERR", want_extra ? "EXTRAP: fail" : "KNOWNP: fail");
      }
      bump(vm); return 1;
    }
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    if (want_extra) {
      var_set_num(vm, "EXTRAP_N", hr.n);
      var_set_num(vm, "UNKNOWNKEYSP_N", hr.n);
      var_set_num(vm, "JSONEXTRA_N", hr.n);
      var_set_num(vm, "JSONUNKNOWN_N", hr.n);
      var_set_num(vm, "EXTRAP_FROM", have_from ? 1 : 0);
    } else {
      var_set_num(vm, "KNOWNP_N", hr.n);
      var_set_num(vm, "ALLOWEDKEYSP_N", hr.n);
      var_set_num(vm, "JSONKNOWN_N", hr.n);
      var_set_num(vm, "JSONALLOWED_N", hr.n);
      var_set_num(vm, "KNOWNP_FROM", have_from ? 1 : 0);
    }
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld from=%d bag=%s\n",
              want_extra ? "extrap" : "knownp", hr.n, have_from, hr.str);
    bump(vm); return 1;
  }

  /* EXACTP|SCHEMAP|PLATEEXACTP [FROM plate] key...
   * -- exact key-set match on PLATE (or FROM named plate): all listed present AND no extras.
   * Multi-plate dual of SYS JSONEXACT / JSONSCHEMA.
   * LAST kept as plate · LAST_N 0|1 · EXACTP_N · EXACTP_HIT/EXTRA/MISS/LISTED · EXACTP_FROM.
   * Empty schema + empty plate → match. Soft twin of REQUIRE JSONEXACT for plate-session. */
  if (kw(&L->cur,"EXACTP") || kw(&L->cur,"SCHEMAP") || kw(&L->cur,"PLATEEXACTP") ||
      kw(&L->cur,"KEYSEXACTP") || kw(&L->cur,"SAMEKEYSP") || kw(&L->cur,"PEXACT") ||
      kw(&L->cur,"EXACT_P") || kw(&L->cur,"SCHEMA_P") || kw(&L->cur,"KEYSEXACT_P") ||
      kw(&L->cur,"PLATESCHEMA") || kw(&L->cur,"SCHEMAKEYS_P") || kw(&L->cur,"EXACTKEYS_P")) {
    char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
    char arg[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result ha, ex;
    int have_from = 0;
    size_t olen = 0;
    long hit = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; keys_nl[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
           L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"END") || kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") ||
           kw(&L->cur,"IF") || kw(&L->cur,"LET") || kw(&L->cur,"SYS") ||
           kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") || kw(&L->cur,"PASS") ||
           kw(&L->cur,"FAIL") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
           kw(&L->cur,"LOOP") || kw(&L->cur,"JSON") || kw(&L->cur,"SETP") ||
           kw(&L->cur,"GETP") || kw(&L->cur,"HASP") || kw(&L->cur,"NEEDP") ||
           kw(&L->cur,"HASPANY") || kw(&L->cur,"COALESCEP") || kw(&L->cur,"NVLP") ||
           kw(&L->cur,"HITVALP") || kw(&L->cur,"FIRSTVALP") || kw(&L->cur,"HITKEYP") ||
           kw(&L->cur,"MISSP") || kw(&L->cur,"PRESENTP") || kw(&L->cur,"EXTRAP") ||
           kw(&L->cur,"KNOWNP") || kw(&L->cur,"EXACTP") || kw(&L->cur,"SCHEMAP") ||
           kw(&L->cur,"ONLYP") || kw(&L->cur,"NOEXTRAP") ||
           kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
           kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
           kw(&L->cur,"WITH") || kw(&L->cur,"AS"))) {
        break;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(arg, sizeof arg, "%ld", nv);
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
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    if (have_from) {
      const char *b = from_src;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b == '{')
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

    memset(&ha, 0, sizeof ha);
    memset(&ex, 0, sizeof ex);
    cubalc_host_json_has_keys(plate, keys_nl, 1, &ha);
    cubalc_host_json_filter_plate_keys(plate, keys_nl, 1, &ex);
    hit = (ha.n == 1 && ex.n == 0) ? 1 : 0;
    {
      long listed = 0;
      const char *p = keys_nl;
      while (*p) {
        while (*p == '\n' || *p == '\r') p++;
        if (!*p) break;
        listed++;
        while (*p && *p != '\n' && *p != '\r') p++;
      }
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "EXACTP_N", hit);
      var_set_num(vm, "SCHEMAP_N", hit);
      var_set_num(vm, "JSONEXACT_N", hit);
      var_set_num(vm, "EXACTP_HIT", (long)ha.code);
      var_set_num(vm, "EXACTP_EXTRA", ex.n);
      var_set_num(vm, "EXACTP_MISS", listed - (long)ha.code);
      var_set_num(vm, "EXACTP_LISTED", listed);
      var_set_num(vm, "JSONEXACT_HIT", (long)ha.code);
      var_set_num(vm, "JSONEXACT_EXTRA", ex.n);
      var_set_num(vm, "JSONEXACT_MISS", listed - (long)ha.code);
      var_set_num(vm, "JSONEXACT_LISTED", listed);
      var_set_num(vm, "EXACTP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "SCHEMAP_FROM", have_from ? 1 : 0);
    }
    var_set_str(vm, "LAST", plate);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# exactp hit=%ld found=%d extra=%ld from=%d\n",
              hit, ha.code, ex.n, have_from);
    bump(vm); return 1;
  }

  /* ONLYP|NOEXTRAP|JSONONLYP [FROM plate] allow...
   * -- soft no-extra-keys on PLATE (or FROM named plate): all plate keys ⊆ allow-list.
   * Multi-plate dual of REQUIRE JSONONLY / NOEXTRA / JSONSTRICT.
   * LAST kept as plate · LAST_N 0|1 · ONLYP_N · ONLYP_EXTRA · ONLYP_FROM · ONLYP_LISTED.
   * Empty allow-list: empty plate matches; non-empty plate fails (all keys extra).
   * Soft twin of REQUIRE JSONONLY for plate-session agents (no fail-fast). */
  if (kw(&L->cur,"ONLYP") || kw(&L->cur,"NOEXTRAP") || kw(&L->cur,"JSONONLYP") ||
      kw(&L->cur,"STRICTP") || kw(&L->cur,"ALLOWONLYP") || kw(&L->cur,"PLATEONLYP") ||
      kw(&L->cur,"ONLY_P") || kw(&L->cur,"NOEXTRA_P") || kw(&L->cur,"JSONONLY_P") ||
      kw(&L->cur,"STRICT_P") || kw(&L->cur,"ALLOWONLY_P") || kw(&L->cur,"PLATEONLY_P") ||
      kw(&L->cur,"KEYSONLYSOFTP") || kw(&L->cur,"SOFTONLYP")) {
    char plate[CUBALC_HOST_STR_MAX], allowed_nl[CUBALC_HOST_STR_MAX];
    char arg[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result ex;
    int have_from = 0;
    size_t olen = 0;
    long hit = 0;
    long listed = 0;
    Var *pv;
    const char *p;

    lex_next(L);
    plate[0] = 0; allowed_nl[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
           L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"END") || kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") ||
           kw(&L->cur,"IF") || kw(&L->cur,"LET") || kw(&L->cur,"SYS") ||
           kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") || kw(&L->cur,"PASS") ||
           kw(&L->cur,"FAIL") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
           kw(&L->cur,"LOOP") || kw(&L->cur,"JSON") || kw(&L->cur,"SETP") ||
           kw(&L->cur,"GETP") || kw(&L->cur,"HASP") || kw(&L->cur,"NEEDP") ||
           kw(&L->cur,"HASPANY") || kw(&L->cur,"COALESCEP") || kw(&L->cur,"NVLP") ||
           kw(&L->cur,"HITVALP") || kw(&L->cur,"FIRSTVALP") || kw(&L->cur,"HITKEYP") ||
           kw(&L->cur,"MISSP") || kw(&L->cur,"PRESENTP") || kw(&L->cur,"EXTRAP") ||
           kw(&L->cur,"KNOWNP") || kw(&L->cur,"EXACTP") || kw(&L->cur,"SCHEMAP") ||
           kw(&L->cur,"ONLYP") || kw(&L->cur,"NOEXTRAP") ||
           kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
           kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
           kw(&L->cur,"WITH") || kw(&L->cur,"AS"))) {
        break;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(arg, sizeof arg, "%ld", nv);
      } else if (resolve_str_arg(vm, L, arg, sizeof arg) != 0) {
        break;
      }
      if (!arg[0]) continue;
      if (olen > 0) {
        if (olen + 1 >= sizeof allowed_nl) break;
        allowed_nl[olen++] = '\n';
        allowed_nl[olen] = 0;
      }
      {
        size_t al = strlen(arg);
        if (olen + al + 1 >= sizeof allowed_nl) break;
        memcpy(allowed_nl + olen, arg, al + 1);
        olen += al;
      }
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    if (have_from) {
      const char *b = from_src;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b == '{')
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

    listed = 0;
    p = allowed_nl;
    while (*p) {
      while (*p == '\n' || *p == '\r') p++;
      if (!*p) break;
      listed++;
      while (*p && *p != '\n' && *p != '\r') p++;
    }

    memset(&ex, 0, sizeof ex);
    if (cubalc_host_json_filter_plate_keys(plate, allowed_nl, 1, &ex) != 0) {
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "ONLYP_N", 0);
      var_set_num(vm, "NOEXTRAP_N", 0);
      var_set_num(vm, "JSONONLY_N", 0);
      var_set_num(vm, "ONLYP_EXTRA", 0);
      var_set_num(vm, "ONLYP_LISTED", listed);
      var_set_num(vm, "ONLYP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "NOEXTRAP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      var_set_num(vm, "OK", 0);
      if (ex.err[0]) {
        var_set_str(vm, "LAST_ERR", ex.err);
        var_set_str(vm, "ERR", ex.err);
      } else {
        var_set_str(vm, "LAST_ERR", "ONLYP: fail");
        var_set_str(vm, "ERR", "ONLYP: fail");
      }
      bump(vm); return 1;
    }
    hit = (ex.n == 0) ? 1 : 0;
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "ONLYP_N", hit);
    var_set_num(vm, "NOEXTRAP_N", hit);
    var_set_num(vm, "JSONONLY_N", hit);
    var_set_num(vm, "JSONSTRICT_N", hit);
    var_set_num(vm, "ONLYP_EXTRA", ex.n);
    var_set_num(vm, "NOEXTRAP_EXTRA", ex.n);
    var_set_num(vm, "JSONEXTRA_N", ex.n);
    var_set_num(vm, "ONLYP_LISTED", listed);
    var_set_num(vm, "NOEXTRAP_LISTED", listed);
    var_set_num(vm, "ONLYP_FROM", have_from ? 1 : 0);
    var_set_num(vm, "NOEXTRAP_FROM", have_from ? 1 : 0);
    var_set_str(vm, "LAST", plate);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# onlyp hit=%ld extra=%ld listed=%ld from=%d\n",
              hit, ex.n, listed, have_from);
    bump(vm); return 1;
  }

/* COALESCEP|NVLP|HITCOALESCEP [FROM plate] key… [OR|DEFAULT def]
   * — first non-empty any-of value from PLATE (or FROM named plate).
   * Multi-plate dual of SYS JSONCOALESCE · plate_session agents peel preferred keys
   * without SYS JSONCOALESCE PLATE glue / multi GETP+IFEMPTY.
   * LAST=value · LAST_N 0|1 · COALESCEP_N=1-based · COALESCEP_KEY · COALESCEP_HIT · OR default. */
  if (kw(&L->cur,"COALESCEP") || kw(&L->cur,"NVLP") || kw(&L->cur,"PLATECOALESCE") ||
      kw(&L->cur,"COALESCE_P") || kw(&L->cur,"PNVL") || kw(&L->cur,"HITCOALESCEP") ||
      kw(&L->cur,"FIRSTNONEMPTYP") || kw(&L->cur,"PLATENVL") || kw(&L->cur,"MCOALESCEP") ||
      kw(&L->cur,"ORP") || kw(&L->cur,"DEFAULTVALP")) {
    char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
    char arg[CUBALC_HOST_STR_MAX], deflt[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
    size_t olen = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; keys_nl[0] = 0; deflt[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
           L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSEVAL") ||
           kw(&L->cur,"FALLBACK") || kw(&L->cur,"DEF") || kw(&L->cur,"WITH") ||
           kw(&L->cur,"AS") || kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
           kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
           kw(&L->cur,"END") || kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") ||
           kw(&L->cur,"IF") || kw(&L->cur,"LET") || kw(&L->cur,"SYS") ||
           kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") || kw(&L->cur,"PASS") ||
           kw(&L->cur,"FAIL") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
           kw(&L->cur,"LOOP") || kw(&L->cur,"JSON") || kw(&L->cur,"SETP") ||
           kw(&L->cur,"GETP") || kw(&L->cur,"HASP") || kw(&L->cur,"NEEDP") ||
           kw(&L->cur,"HASPANY") || kw(&L->cur,"COALESCEP") || kw(&L->cur,"NVLP"))) {
        break;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(arg, sizeof arg, "%ld", nv);
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
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSEVAL") ||
        kw(&L->cur,"FALLBACK") || kw(&L->cur,"DEF") || kw(&L->cur,"WITH") ||
        kw(&L->cur,"AS")) {
      lex_next(L);
      if (resolve_str_arg(vm, L, arg, sizeof arg) == 0)
        snprintf(deflt, sizeof deflt, "%s", arg);
    }

    if (have_from) {
      const char *b = from_src;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b == '{')
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

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_coalesce(plate, keys_nl, deflt, &hr) != 0) {
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "COALESCEP_N", 0);
      var_set_num(vm, "COALESCEP_HIT", 0);
      var_set_num(vm, "NVLP_N", 0);
      var_set_num(vm, "COALESCEP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "COALESCEP_KEY", "");
      var_set_str(vm, "NVLP_KEY", "");
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "OK", 0);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", "COALESCEP: fail");
        var_set_str(vm, "ERR", "COALESCEP: fail");
      }
      bump(vm); return 1;
    }
    if (hr.n) {
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "COALESCEP_HIT", 1);
    } else if (hr.str[0]) {
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "COALESCEP_HIT", 0);
    } else {
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "COALESCEP_HIT", 0);
    }
    var_set_num(vm, "COALESCEP_N", (long)hr.code);
    var_set_num(vm, "NVLP_N", (long)hr.code);
    var_set_num(vm, "JSONCOALESCE_N", (long)hr.code);
    var_set_str(vm, "COALESCEP_KEY", hr.err);
    var_set_str(vm, "NVLP_KEY", hr.err);
    var_set_str(vm, "JSONCOALESCE_KEY", hr.err);
    var_set_num(vm, "COALESCEP_FROM", have_from ? 1 : 0);
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# coalescep hit=%ld idx=%d key=%s from=%d val=%s\n",
              hr.n, hr.code, hr.err, have_from, hr.str);
    bump(vm); return 1;
  }

  /* SAVEP [FROM plate] path — persist PLATE (default) or any plate var to disk.
   * Twin of plate_save INCLUDE + multi-plate dual of LOADPLATE INTO:
   *   SETP "role" "worker"
   *   SAVEP "state/agent.json"          # uses PLATE (not LAST footgun)
   *   SAVEP FROM peer "state/peer.json"
   *   SAVEP path FROM session
   * LAST = abs path · LAST_N = bytes · SAVEP_FROM · SAVEPLATE_* knobs.
   * Soft OK=0 if non-object. Does not clobber plate var content. */
  if (kw(&L->cur,"SAVEP") || kw(&L->cur,"SAVE_P") || kw(&L->cur,"PLATE_SAVE") ||
      kw(&L->cur,"MSAVEP") || kw(&L->cur,"WRITE_P") || kw(&L->cur,"PERSIST_P")) {
    char path[CUBALC_HOST_STR_MAX], plate[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    char from_name[96];
    const char *slash, *body;
    size_t n;
    int created = 0, have_from = 0;
    cubalc_host_result hr, keys;
    Var *pv;

    lex_next(L);
    path[0] = 0; plate[0] = 0; from_name[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(plate, sizeof plate, "%s", vm->last_str);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && pv->is_str) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(plate, sizeof plate, "%s", pv->sval);
          lex_next(L);
        } else if (pv) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(plate, sizeof plate, "%ld", pv->val);
          lex_next(L);
        } else if (resolve_str_arg(vm, L, plate, sizeof plate) != 0) {
          plate[0] = 0;
        }
      } else if (resolve_str_arg(vm, L, plate, sizeof plate) != 0) {
        plate[0] = 0;
      }
    }

    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail(vm, "SAVEP [FROM plate] path — need path");
      return -1;
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(plate, sizeof plate, "%s", vm->last_str);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && pv->is_str) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(plate, sizeof plate, "%s", pv->sval);
          lex_next(L);
        } else if (pv) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(plate, sizeof plate, "%ld", pv->val);
          lex_next(L);
        } else if (resolve_str_arg(vm, L, plate, sizeof plate) != 0) {
          plate[0] = 0;
        }
      } else if (resolve_str_arg(vm, L, plate, sizeof plate) != 0) {
        plate[0] = 0;
      }
    }

    if (!have_from || !plate[0]) {
      if (!plate[0]) {
        pv = var_get(vm, "PLATE", 0);
        if (pv && pv->is_str && pv->sval[0])
          snprintf(plate, sizeof plate, "%s", pv->sval);
        else
          snprintf(plate, sizeof plate, "%s", "{}");
      }
    }

    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "SAVEP: empty path");
      var_set_str(vm, "ERR", "SAVEP: empty path");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SAVEP_N", 0);
      var_set_num(vm, "SAVEPLATE_N", 0);
      var_set_num(vm, "SAVEP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    body = plate;
    while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
      body++;
    if (*body != '{') {
      var_set_str(vm, "LAST", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      var_set_str(vm, "LAST_ERR", "SAVEP: need object plate (starts with {)");
      var_set_str(vm, "ERR", "SAVEP: need object plate (starts with {)");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SAVEP_N", 0);
      var_set_num(vm, "SAVEPLATE_N", 0);
      var_set_num(vm, "SAVEPLATE_OK", 0);
      var_set_num(vm, "SAVEP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "SAVEPLATE_PATH", path);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&keys, 0, sizeof keys);
    if (cubalc_host_json_keys(body, &keys) != 0) {
      var_set_str(vm, "LAST", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      var_set_str(vm, "LAST_ERR", "SAVEP: invalid object plate");
      var_set_str(vm, "ERR", "SAVEP: invalid object plate");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SAVEP_N", 0);
      var_set_num(vm, "SAVEPLATE_OK", 0);
      var_set_num(vm, "SAVEP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else {
        parent[0] = '.';
        parent[1] = 0;
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "SAVEP: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      var_set_str(vm, "SAVEPLATE_PATH", absfile);
      var_set_num(vm, "SAVEP_FROM", have_from ? 1 : 0);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SAVEP_N", 0);
      var_set_num(vm, "SAVEPLATE_OK", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_write(absfile, body, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "SAVEP: write failed";
      var_set_str(vm, "LAST", absfile);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      var_set_str(vm, "SAVEPLATE_PATH", absfile);
      var_set_num(vm, "SAVEP_FROM", have_from ? 1 : 0);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SAVEP_N", 0);
      var_set_num(vm, "SAVEPLATE_OK", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", absfile);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "SAVEP", absfile);
    var_set_str(vm, "SAVEPLATE", absfile);
    var_set_str(vm, "SAVEPLATE_PATH", absfile);
    var_set_str(vm, "SAVEPLATE_PARENT", absparent);
    var_set_num(vm, "SAVEP_N", hr.n);
    var_set_num(vm, "SAVEPLATE_N", hr.n);
    var_set_num(vm, "SAVEPLATE_OK", 1);
    var_set_num(vm, "SAVEP_FROM", have_from ? 1 : 0);
    var_set_num(vm, "SAVEPLATE_FROM", have_from ? 1 : 0);
    var_set_str(vm, "SAVEP_SRC", from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_str(vm, "SAVEPLATE_SRC", from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "SAVEPLATE_CREATED", (long)created);
    var_set_num(vm, "WRITE_OK", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# savep → %s n=%ld from=%d src=%s\n",
              absfile, hr.n, have_from, from_name[0] ? from_name : "PLATE");
    bump(vm); return 1;
  }
  /* LOADP [INTO name] path [OR defaults] — top-level soft plate load (no SYS).
   * SEEDP|BOOTP [INTO name] path [OR seed] — top-level create-or-load (no SYS).
   * Note: bare ENSUREP remains DEFAULTP alias; use SEEDP for disk seed.
   * Dual of SAVEP for multi-plate disk boot without SYS keyword soup.
   * Same knobs as SYS LOADPLATE/ENSUREPLATE (HIT/FALLBACK/INTO/CREATED/VAR). */
  if (kw(&L->cur,"LOADP") || kw(&L->cur,"LOAD_P") || kw(&L->cur,"READ_P") ||
      kw(&L->cur,"MLOADP") || kw(&L->cur,"PLATE_LOAD") ||
      kw(&L->cur,"SEEDP") || kw(&L->cur,"BOOTP") || kw(&L->cur,"ENSURE_P") ||
      kw(&L->cur,"SEED_P") || kw(&L->cur,"MENSUREP") || kw(&L->cur,"PLATE_ENSURE") ||
      kw(&L->cur,"BOOT_P") || kw(&L->cur,"DISKENSURE") || kw(&L->cur,"ENSUREDISK")) {
    char pathb[CUBALC_HOST_STR_MAX], fb[CUBALC_HOST_STR_MAX], plate[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    char into_name[96];
    const char *slash, *body;
    size_t n;
    int is_ensure = 0, have_into = 0, have_fb = 0, hit = 0, created = 0;
    cubalc_host_result hr, keys;

    if (kw(&L->cur,"SEEDP") || kw(&L->cur,"BOOTP") || kw(&L->cur,"ENSURE_P") ||
        kw(&L->cur,"SEED_P") || kw(&L->cur,"MENSUREP") || kw(&L->cur,"PLATE_ENSURE") ||
        kw(&L->cur,"BOOT_P") || kw(&L->cur,"DISKENSURE") || kw(&L->cur,"ENSUREDISK"))
      is_ensure = 1;

    lex_next(L);
    pathb[0] = 0; fb[0] = 0; plate[0] = 0; into_name[0] = 0;

    if (kw(&L->cur,"INTO") || kw(&L->cur,"AS") || kw(&L->cur,"TOVAR") ||
        kw(&L->cur,"BIND") || kw(&L->cur,"STOREIN")) {
      lex_next(L);
      have_into = 1;
      if (L->cur.kind == TK_IDENT) {
        snprintf(into_name, sizeof into_name, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, is_ensure ? "SEEDP INTO name path — need var name"
                           : "LOADP INTO name path — need var name");
        return -1;
      }
    }

    if (resolve_str_arg(vm, L, pathb, sizeof pathb) != 0) {
      fail(vm, is_ensure ? "SEEDP [INTO name] path [OR seed] — need path"
                         : "LOADP [INTO name] path [OR defaults] — need path");
      return -1;
    }

    if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
        kw(&L->cur,"FALLBACK") || kw(&L->cur,"FROM") || kw(&L->cur,"WITH") ||
        kw(&L->cur,"USING") || kw(&L->cur,"SEED")) {
      lex_next(L);
      if (resolve_str_arg(vm, L, fb, sizeof fb) == 0)
        have_fb = 1;
      else if (!is_ensure) {
        fail(vm, "LOADP path OR defaults — need fallback plate");
        return -1;
      }
    }

    if (!have_into && (kw(&L->cur,"INTO") || kw(&L->cur,"AS") || kw(&L->cur,"TOVAR") ||
                       kw(&L->cur,"BIND") || kw(&L->cur,"STOREIN"))) {
      lex_next(L);
      have_into = 1;
      if (L->cur.kind == TK_IDENT) {
        snprintf(into_name, sizeof into_name, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, is_ensure ? "SEEDP path INTO name — need var name"
                           : "LOADP path INTO name — need var name");
        return -1;
      }
    }

    if (!have_fb)
      snprintf(fb, sizeof fb, "%s", "{}");
    {
      const char *f = fb;
      while (*f == ' ' || *f == '\t' || *f == '\n' || *f == '\r') f++;
      if (*f != '{')
        snprintf(fb, sizeof fb, "%s", "{}");
    }

    if (!pathb[0]) {
      var_set_str(vm, "LAST_ERR", is_ensure ? "SEEDP: empty path" : "LOADP: empty path");
      var_set_str(vm, "ERR", is_ensure ? "SEEDP: empty path" : "LOADP: empty path");
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, is_ensure ? "ENSUREPLATE_INTO" : "LOADPLATE_INTO", have_into ? 1 : 0);
      bump(vm); return 1;
    }

    /* try load existing object plate */
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_read(pathb, &hr) == 0 && hr.str[0]) {
      body = hr.str;
      while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
        body++;
      if (*body == '{') {
        memset(&keys, 0, sizeof keys);
        if (cubalc_host_json_keys(body, &keys) == 0) {
          snprintf(plate, sizeof plate, "%s", body);
          hit = 1;
        }
      }
    }

    if (hit) {
      if (have_into && into_name[0])
        var_set_str(vm, into_name, plate);
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      if (is_ensure) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "ENSUREPLATE", plate);
        var_set_str(vm, "ENSUREP", plate);
        var_set_str(vm, "ENSUREPLATE_PATH", pathb);
        var_set_num(vm, "ENSUREPLATE_CREATED", 0);
        var_set_num(vm, "ENSUREPLATE_HIT", 1);
        var_set_num(vm, "ENSUREPLATE_INTO", have_into ? 1 : 0);
        var_set_str(vm, "ENSUREPLATE_VAR", into_name);
        var_set_num(vm, "ENSUREP_INTO", have_into ? 1 : 0);
      var_set_num(vm, "SEEDP_INTO", have_into ? 1 : 0);
      } else {
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_num(vm, "LOADPLATE_FALLBACK", 0);
      }
      var_set_str(vm, "LOADPLATE", plate);
      var_set_str(vm, "LOADP", plate);
      var_set_str(vm, "LOADPLATE_PATH", pathb);
      var_set_num(vm, "LOADPLATE_HIT", 1);
      var_set_num(vm, "LOADPLATE_INTO", have_into ? 1 : 0);
      var_set_str(vm, "LOADPLATE_VAR", into_name);
      var_set_num(vm, "LOADP_INTO", have_into ? 1 : 0);
      var_set_num(vm, "READ_OK", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# %s → hit into=%s path=%s\n",
                is_ensure ? "ensurep" : "loadp",
                into_name[0] ? into_name : "-", pathb);
      bump(vm); return 1;
    }

    /* miss */
    if (!is_ensure) {
      snprintf(plate, sizeof plate, "%s", fb);
      if (have_into && into_name[0])
        var_set_str(vm, into_name, plate);
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "LAST_ERR",
                  hr.err[0] ? hr.err : "LOADP: miss or non-object — using fallback");
      var_set_str(vm, "LOADPLATE", plate);
      var_set_str(vm, "LOADP", plate);
      var_set_str(vm, "LOADPLATE_PATH", pathb);
      var_set_num(vm, "LOADPLATE_HIT", 0);
      var_set_num(vm, "LOADPLATE_FALLBACK", 1);
      var_set_num(vm, "LOADPLATE_INTO", have_into ? 1 : 0);
      var_set_str(vm, "LOADPLATE_VAR", into_name);
      var_set_num(vm, "LOADP_INTO", have_into ? 1 : 0);
      var_set_num(vm, "READ_OK", 0);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# loadp → fallback into=%s path=%s\n",
                into_name[0] ? into_name : "-", pathb);
      bump(vm); return 1;
    }

    /* ENSUREP miss → write seed */
    body = fb;
    while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
      body++;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(pathb, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", pathb);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else {
        parent[0] = '.';
        parent[1] = 0;
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "SEEDP: mkdir parent failed";
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      var_set_str(vm, "ENSUREPLATE_PATH", absfile);
      var_set_num(vm, "ENSUREPLATE_CREATED", 0);
      var_set_num(vm, "ENSUREPLATE_INTO", have_into ? 1 : 0);
      var_set_num(vm, "ENSUREP_INTO", have_into ? 1 : 0);
      var_set_num(vm, "SEEDP_INTO", have_into ? 1 : 0);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_write(absfile, body, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "SEEDP: write failed";
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      var_set_str(vm, "ENSUREPLATE_PATH", absfile);
      var_set_num(vm, "ENSUREPLATE_CREATED", 0);
      var_set_num(vm, "ENSUREPLATE_INTO", have_into ? 1 : 0);
      var_set_num(vm, "ENSUREP_INTO", have_into ? 1 : 0);
      var_set_num(vm, "SEEDP_INTO", have_into ? 1 : 0);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    snprintf(plate, sizeof plate, "%s", body);
    if (have_into && into_name[0])
      var_set_str(vm, into_name, plate);
    var_set_str(vm, "LAST", plate);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
    vm->last_n = 1;
    var_set_num(vm, "LAST_N", 1);
    var_set_str(vm, "ENSUREPLATE", plate);
    var_set_str(vm, "ENSUREP", plate);
    var_set_str(vm, "LOADPLATE", plate);
    var_set_str(vm, "LOADP", plate);
    var_set_str(vm, "ENSUREPLATE_PATH", absfile);
    var_set_str(vm, "ENSUREPLATE_PARENT", absparent);
    var_set_str(vm, "LOADPLATE_PATH", absfile);
    var_set_num(vm, "ENSUREPLATE_CREATED", 1);
    var_set_num(vm, "ENSUREPLATE_HIT", 0);
    var_set_num(vm, "ENSUREPLATE_INTO", have_into ? 1 : 0);
    var_set_str(vm, "ENSUREPLATE_VAR", into_name);
    var_set_num(vm, "ENSUREP_INTO", have_into ? 1 : 0);
      var_set_num(vm, "SEEDP_INTO", have_into ? 1 : 0);
    var_set_num(vm, "LOADPLATE_HIT", 0);
    var_set_num(vm, "LOADPLATE_INTO", have_into ? 1 : 0);
    var_set_num(vm, "LOADP_INTO", have_into ? 1 : 0);
    var_set_str(vm, "LOADPLATE_VAR", into_name);
    var_set_num(vm, "SAVEPLATE_N", hr.n);
    var_set_num(vm, "SAVEPLATE_OK", 1);
    var_set_num(vm, "READ_OK", 0);
    var_set_num(vm, "WRITE_OK", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# ensurep → created into=%s path=%s\n",
              into_name[0] ? into_name : "-", absfile);
    (void)created;
    bump(vm); return 1;
  }
  /* DUMPP|PLATEINFO [FROM plate] — agent snapshot of PLATE or any plate object.
   * LAST = cubalc.plate_info.v1 JSON:
   *   keys_n · bytes · path (PLATE_PATH if default; var name if FROM named) · keys · has_plate
   * Does not clobber PLATE. Soft empty → keys_n=0 has_plate=0 OK=1.
   * FROM: multi-plate twin of KEYSP/NEEDP FROM:
   *   DUMPP FROM peer
   *   PLATEINFO FROM LAST
   * DUMPP_FROM = 0|1 · DUMPP_SRC = source name when named FROM.
   * Usability: one form for agents instead of KEYSP+LEN+JSONLEN+PATH glue. */
  if (kw(&L->cur,"DUMPP") || kw(&L->cur,"PLATEINFO") || kw(&L->cur,"INFO_P") ||
      kw(&L->cur,"PLATE_INFO") || kw(&L->cur,"DUMP_PLATE") || kw(&L->cur,"DESCRIBE_PLATE") ||
      kw(&L->cur,"SHOWPLATE") || kw(&L->cur,"PLATEMETA")) {
    char plate[CUBALC_HOST_STR_MAX], path[CUBALC_HOST_STR_MAX];
    char keys_flat[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
    char path_esc[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result keys;
    Var *pv, *pp;
    size_t i, o = 0;
    long bytes = 0, kn = 0;
    int has_plate = 0, have_from = 0;

    lex_next(L);
    plate[0] = 0; path[0] = 0; keys_flat[0] = 0; path_esc[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

    /* optional FROM plate_src (leading or only arg) */
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

    if (have_from) {
      const char *b = from_src;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b == '{') {
        snprintf(plate, sizeof plate, "%s", from_src);
        has_plate = 1;
      } else {
        snprintf(plate, sizeof plate, "%s", "{}");
        has_plate = 0;
      }
      /* path field: named source for agents (not PLATE_PATH file) */
      if (from_name[0])
        snprintf(path, sizeof path, "%s", from_name);
    } else {
      pv = var_get(vm, "PLATE", 0);
      if (pv && pv->is_str && pv->sval[0]) {
        snprintf(plate, sizeof plate, "%s", pv->sval);
        has_plate = 1;
      } else {
        snprintf(plate, sizeof plate, "%s", "{}");
      }
      pp = var_get(vm, "PLATE_PATH", 0);
      if (pp && pp->is_str && pp->sval[0])
        snprintf(path, sizeof path, "%s", pp->sval);
    }
    bytes = (long)strlen(plate);

    memset(&keys, 0, sizeof keys);
    if (cubalc_host_json_keys(plate, &keys) == 0)
      kn = keys.n;
    /* flatten keys bag to comma list for one-line plate (escape " \) */
    o = 0;
    for (i = 0; keys.str[i] && o + 2 < sizeof keys_flat; i++) {
      char c = keys.str[i];
      if (c == '\n' || c == '\r') {
        if (o > 0 && keys_flat[o - 1] != ',')
          keys_flat[o++] = ',';
      } else if (c == '"' || c == '\\') {
        keys_flat[o++] = '_';
      } else if ((unsigned char)c < 32) {
        /* skip */
      } else {
        keys_flat[o++] = c;
      }
    }
    keys_flat[o] = 0;

    o = 0;
    for (i = 0; path[i] && o + 2 < sizeof path_esc; i++) {
      char c = path[i];
      if (c == '"' || c == '\\') {
        path_esc[o++] = '\\';
        path_esc[o++] = c;
      } else if ((unsigned char)c < 32) {
        path_esc[o++] = ' ';
      } else {
        path_esc[o++] = c;
      }
    }
    path_esc[o] = 0;

    snprintf(out, sizeof out,
             "{\"schema\":\"cubalc.plate_info.v1\",\"ok\":true,"
             "\"has_plate\":%s,\"keys_n\":%ld,\"bytes\":%ld,"
             "\"path\":\"%s\",\"keys\":\"%s\",\"from\":%s,\"version\":\"%s\"}",
             has_plate ? "true" : "false", kn, bytes, path_esc, keys_flat,
             have_from ? "true" : "false", CUBALC_LANG_VERSION);

    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = kn;
    var_set_num(vm, "LAST_N", kn);
    var_set_str(vm, "DUMPP", out);
    var_set_str(vm, "PLATEINFO", out);
    var_set_num(vm, "DUMPP_N", kn);
    var_set_num(vm, "DUMPP_BYTES", bytes);
    var_set_num(vm, "DUMPP_HAS", has_plate ? 1 : 0);
    var_set_num(vm, "DUMPP_FROM", have_from ? 1 : 0);
    var_set_str(vm, "DUMPP_PATH", path);
    var_set_str(vm, "DUMPP_KEYS", keys_flat);
    var_set_str(vm, "DUMPP_SRC", from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# dumpp keys_n=%ld bytes=%ld has=%d from=%d\n",
              kn, bytes, has_plate, have_from);
    if (vm->res)
      snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", out);
    bump(vm); return 1;
  }
  /* EQP|SAMEP a b — soft order-independent plate equality (top-level JSONEQ dual).
   * NEQP a b — inverse · DIFFP|CHANGEDP a b — changed key bag · CHANGELOGP a b — value log.
   * STABLEP|SAMEKEYSP a b — unchanged key bag (JSONSTABLE dual · dual of DIFFP).
   * Args: plate var names (PLATE/PEER/…), LAST, or JSON object string.
   * Soft always OK=1. LAST_N: eq 0|1 · neq 0|1 · diff/stable/changelog = count.
   * Usability: multi-plate mesh verify without SYS JSONEQ/JSONCHANGED/JSONSTABLE glue:
   *   EQP PLATE PEER
   *   DIFFP session peer
   *   STABLEP PLATE PEER   # keys that still match
   *   CHANGELOGP a b
   */
  if (kw(&L->cur,"EQP") || kw(&L->cur,"SAMEP") || kw(&L->cur,"PLATE_EQ") ||
      kw(&L->cur,"EQPLATE") || kw(&L->cur,"MEQP") ||
      kw(&L->cur,"NEQP") || kw(&L->cur,"PLATE_NE") || kw(&L->cur,"NEPLATE") ||
      kw(&L->cur,"MNEQP") ||
      kw(&L->cur,"DIFFP") || kw(&L->cur,"CHANGEDP") || kw(&L->cur,"PLATE_DIFF") ||
      kw(&L->cur,"MDIFFP") || kw(&L->cur,"KEYSDIFFP") ||
      kw(&L->cur,"STABLEP") || kw(&L->cur,"SAMEKEYSP") || kw(&L->cur,"EQKEYSP") ||
      kw(&L->cur,"UNCHANGEDP") || kw(&L->cur,"PLATE_STABLE") || kw(&L->cur,"MSTABLEP") ||
      kw(&L->cur,"CHANGELOGP") || kw(&L->cur,"CLOGP") || kw(&L->cur,"PLATE_CHANGELOG") ||
      kw(&L->cur,"MCHANGELOGP") || kw(&L->cur,"LOGDIFFP")) {
    char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
    char aname[96], bname[96];
    cubalc_host_result hr;
    int is_eq = 0, is_ne = 0, is_diff = 0, is_stable = 0, is_clog = 0;
    Var *pv;

    if (kw(&L->cur,"EQP") || kw(&L->cur,"SAMEP") || kw(&L->cur,"PLATE_EQ") ||
        kw(&L->cur,"EQPLATE") || kw(&L->cur,"MEQP"))
      is_eq = 1;
    else if (kw(&L->cur,"NEQP") || kw(&L->cur,"PLATE_NE") || kw(&L->cur,"NEPLATE") ||
             kw(&L->cur,"MNEQP"))
      is_ne = 1;
    else if (kw(&L->cur,"DIFFP") || kw(&L->cur,"CHANGEDP") || kw(&L->cur,"PLATE_DIFF") ||
             kw(&L->cur,"MDIFFP") || kw(&L->cur,"KEYSDIFFP"))
      is_diff = 1;
    else if (kw(&L->cur,"STABLEP") || kw(&L->cur,"SAMEKEYSP") || kw(&L->cur,"EQKEYSP") ||
             kw(&L->cur,"UNCHANGEDP") || kw(&L->cur,"PLATE_STABLE") || kw(&L->cur,"MSTABLEP"))
      is_stable = 1;
    else
      is_clog = 1;

    lex_next(L);
    a[0] = 0; b[0] = 0; aname[0] = 0; bname[0] = 0;

    /* resolve plate arg → object text (non-object soft {}) */
#define CUBALC_RESOLVE_PLATE_ARG(dst, dname) do { \
      (dst)[0] = 0; (dname)[0] = 0; \
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) { \
        snprintf((dname), sizeof(dname), "%s", "LAST"); \
        snprintf((dst), sizeof(dst), "%s", vm->last_str); \
        lex_next(L); \
      } else if (L->cur.kind == TK_IDENT) { \
        pv = var_get(vm, L->cur.text, 0); \
        if (pv && pv->is_str) { \
          snprintf((dname), sizeof(dname), "%s", L->cur.text); \
          snprintf((dst), sizeof(dst), "%s", pv->sval); \
          lex_next(L); \
        } else if (pv) { \
          snprintf((dname), sizeof(dname), "%s", L->cur.text); \
          snprintf((dst), sizeof(dst), "%ld", pv->val); \
          lex_next(L); \
        } else if (resolve_str_arg(vm, L, (dst), sizeof(dst)) != 0) { \
          (dst)[0] = 0; \
        } \
      } else if (resolve_str_arg(vm, L, (dst), sizeof(dst)) != 0) { \
        snprintf((dst), sizeof(dst), "%s", vm->last_str); \
      } \
      { const char *bp = (dst); \
        while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++; \
        if (*bp != '{') snprintf((dst), sizeof(dst), "%s", "{}"); \
      } \
    } while (0)

    CUBALC_RESOLVE_PLATE_ARG(a, aname);
    CUBALC_RESOLVE_PLATE_ARG(b, bname);
#undef CUBALC_RESOLVE_PLATE_ARG

    if (!a[0] && !b[0] && !aname[0] && !bname[0]) {
      /* bare: compare PLATE to LAST if LAST is object, else soft unequal empty */
      pv = var_get(vm, "PLATE", 0);
      if (pv && pv->is_str && pv->sval[0]) {
        snprintf(a, sizeof a, "%s", pv->sval);
        snprintf(aname, sizeof aname, "%s", "PLATE");
      } else {
        snprintf(a, sizeof a, "%s", "{}");
      }
      snprintf(b, sizeof b, "%s", vm->last_str);
      {
        const char *bp = b;
        while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++;
        if (*bp != '{') snprintf(b, sizeof b, "%s", "{}");
      }
      snprintf(bname, sizeof bname, "%s", "LAST");
    }

    memset(&hr, 0, sizeof hr);
    if (is_eq || is_ne) {
      long hit = 0;
      cubalc_host_json_eq(a, b, &hr);
      hit = hr.n;
      if (is_ne) hit = hit ? 0 : 1;
      var_set_str(vm, "LAST", hit ? "1" : "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hit ? "1" : "0");
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "EQP_N", hit);
      var_set_num(vm, "NEQP_N", hit);
      var_set_num(vm, "EQP_EQ", is_eq ? hit : (hit ? 0 : 1));
      var_set_str(vm, "EQP_A", aname);
      var_set_str(vm, "EQP_B", bname);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# %s a=%s b=%s hit=%ld\n",
                is_ne ? "neqp" : "eqp", aname[0] ? aname : "?", bname[0] ? bname : "?", hit);
      bump(vm); return 1;
    }

    if (is_diff || is_stable) {
      cubalc_host_json_changed_keys(a, b, is_stable ? 1 : 0, &hr);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      if (is_stable) {
        var_set_str(vm, "STABLEP", hr.str);
        var_set_num(vm, "STABLEP_N", hr.n);
        var_set_str(vm, "STABLEP_A", aname);
        var_set_str(vm, "STABLEP_B", bname);
        var_set_num(vm, "JSONSTABLE_N", hr.n);
        var_set_num(vm, "JSONEQKEYS_N", hr.n);
      } else {
        var_set_str(vm, "DIFFP", hr.str);
        var_set_num(vm, "DIFFP_N", hr.n);
        var_set_str(vm, "DIFFP_A", aname);
        var_set_str(vm, "DIFFP_B", bname);
        var_set_num(vm, "JSONCHANGED_N", hr.n);
      }
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# %s n=%ld a=%s b=%s\n",
                is_stable ? "stablep" : "diffp",
                hr.n, aname[0] ? aname : "?", bname[0] ? bname : "?");
      bump(vm); return 1;
    }

    /* changelog */
    cubalc_host_json_changelog(a, b, &hr);
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "CHANGELOGP", hr.str);
    var_set_num(vm, "CHANGELOGP_N", hr.n);
    var_set_str(vm, "CHANGELOGP_A", aname);
    var_set_str(vm, "CHANGELOGP_B", bname);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# changelogp n=%ld a=%s b=%s\n",
              hr.n, aname[0] ? aname : "?", bname[0] ? bname : "?");
    bump(vm); return 1;
  }
  /* SUBSETP|ISSUBSETP sub super — soft required-field match (top-level JSONSUBSET dual).
   * COVERSP|SUPERSETP super sub — super covers sub (dual · JSONCOVERS dual).
   * Args: plate vars / LAST / JSON strings (same resolve as EQP).
   * LAST_N 0|1 · SUBSETP_MISS / COVERSP_MISS = bad key bag on fail.
   * Usability: mesh partial contract without SYS JSONSUBSET glue:
   *   SUBSETP need PEER
   *   COVERSP PEER need
   *   REQUIRE SUBSETP need PEER
   */
  if (kw(&L->cur,"SUBSETP") || kw(&L->cur,"ISSUBSETP") || kw(&L->cur,"MSUBSETP") ||
      kw(&L->cur,"PLATE_SUBSET") || kw(&L->cur,"SUBSETOF") ||
      kw(&L->cur,"COVERSP") || kw(&L->cur,"SUPERSETP") || kw(&L->cur,"MSUPERSETP") ||
      kw(&L->cur,"PLATE_COVERS") || kw(&L->cur,"COVERSPLATE") || kw(&L->cur,"COVERSOF")) {
    char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
    char aname[96], bname[96];
    cubalc_host_result hr, bad;
    int want_cover = 0;
    const char *sub, *sup;
    long hit;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "COVERSP") == 0 || strcmp(op0, "SUPERSETP") == 0 ||
        strcmp(op0, "MSUPERSETP") == 0 || strcmp(op0, "PLATE_COVERS") == 0 ||
        strcmp(op0, "COVERSPLATE") == 0 || strcmp(op0, "COVERSOF") == 0)
      want_cover = 1;

    lex_next(L);
    a[0] = 0; b[0] = 0; aname[0] = 0; bname[0] = 0;

#define CUBALC_RESOLVE_PLATE_ARG2(dst, dname) do { \
      (dst)[0] = 0; (dname)[0] = 0; \
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) { \
        snprintf((dname), sizeof(dname), "%s", "LAST"); \
        snprintf((dst), sizeof(dst), "%s", vm->last_str); \
        lex_next(L); \
      } else if (L->cur.kind == TK_IDENT) { \
        pv = var_get(vm, L->cur.text, 0); \
        if (pv && pv->is_str) { \
          snprintf((dname), sizeof(dname), "%s", L->cur.text); \
          snprintf((dst), sizeof(dst), "%s", pv->sval); \
          lex_next(L); \
        } else if (pv) { \
          snprintf((dname), sizeof(dname), "%s", L->cur.text); \
          snprintf((dst), sizeof(dst), "%ld", pv->val); \
          lex_next(L); \
        } else if (resolve_str_arg(vm, L, (dst), sizeof(dst)) != 0) { \
          (dst)[0] = 0; \
        } \
      } else if (resolve_str_arg(vm, L, (dst), sizeof(dst)) != 0) { \
        snprintf((dst), sizeof(dst), "%s", vm->last_str); \
      } \
      { const char *bp = (dst); \
        while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++; \
        if (*bp != '{') snprintf((dst), sizeof(dst), "%s", "{}"); \
      } \
    } while (0)

    CUBALC_RESOLVE_PLATE_ARG2(a, aname);
    CUBALC_RESOLVE_PLATE_ARG2(b, bname);
#undef CUBALC_RESOLVE_PLATE_ARG2

    if (want_cover) {
      sub = b;
      sup = a;
    } else {
      sub = a;
      sup = b;
    }

    memset(&hr, 0, sizeof hr);
    cubalc_host_json_subset(sub, sup, &hr);
    hit = hr.n;
    memset(&bad, 0, sizeof bad);
    if (!hit)
      cubalc_host_json_subset_bad_keys(sub, sup, &bad);

    var_set_str(vm, "LAST", hit ? "1" : "0");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hit ? "1" : "0");
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "SUBSETP_N", hit);
    var_set_num(vm, "COVERSP_N", hit);
    var_set_str(vm, "SUBSETP_MISS", bad.str);
    var_set_str(vm, "COVERSP_MISS", bad.str);
    var_set_num(vm, "SUBSETP_MISS_N", bad.n);
    var_set_str(vm, "SUBSETP_A", aname);
    var_set_str(vm, "SUBSETP_B", bname);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s a=%s b=%s hit=%ld miss=%ld\n",
              want_cover ? "coversp" : "subsetp",
              aname[0] ? aname : "?", bname[0] ? bname : "?", hit, bad.n);
    bump(vm); return 1;
  }
  /* PLUCKP|GETPALL [FROM plate] key… — multi-key peel → value bag (JSONPLUCK dual).
   * Empty field on miss · LAST_N = listed · PLUCKP_HIT = found · PLUCKP_MISS = n−hit.
   * Keys may be dotted/slash nest paths (same as GETP "freq.error"):
   *   PLUCKP "host" "freq.error" "freq.warn"
   *   PLUCKP FROM PEER "meta.role" "meta.n"
   * Trailing FROM plate ok. Bare keys use conventional PLATE.
   * Usability: multi-field extract without multi GETP+PUSH / GETPOBJ glue:
   *   PLUCKP FROM PEER "host" "agent" "n"
   *   PLUCKP "host" "n" FROM session
   */
  if (kw(&L->cur,"PLUCKP") || kw(&L->cur,"GETPALL") || kw(&L->cur,"MPLUCKP") ||
      kw(&L->cur,"PLATE_PLUCK") || kw(&L->cur,"GETALLP") || kw(&L->cur,"PEELP") ||
      kw(&L->cur,"VALUESP") || kw(&L->cur,"PLUCKPLATE")) {
    char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
    char arg[CUBALC_HOST_STR_MAX], from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
    size_t olen = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; keys_nl[0] = 0; from_name[0] = 0; from_src[0] = 0;

    /* optional leading FROM plate */
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

    /* collect keys; stop before trailing FROM / statements */
    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
           L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
           kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
           kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
           kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
           kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
           kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"SETP") ||
           kw(&L->cur,"INCP") || kw(&L->cur,"DELP") || kw(&L->cur,"GETP") ||
           kw(&L->cur,"MERGEP") || kw(&L->cur,"NEEDP") || kw(&L->cur,"HASP") ||
           kw(&L->cur,"KEYSP") || kw(&L->cur,"PLUCKP") || kw(&L->cur,"EQP") ||
           kw(&L->cur,"SUBSETP") || kw(&L->cur,"INCLUDE") || kw(&L->cur,"REQUIRE") ||
           kw(&L->cur,"FAIL") || kw(&L->cur,"PASS") || kw(&L->cur,"NOTE") ||
           kw(&L->cur,"EXIT")))
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
    }

    /* trailing FROM plate */
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

    if (!keys_nl[0]) {
      fail(vm, "PLUCKP [FROM plate] key… — need at least one key");
      return -1;
    }

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_pluck(plate, keys_nl, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "PLUCKP_N", 0);
      var_set_num(vm, "PLUCKP_HIT", 0);
      var_set_num(vm, "PLUCKP_MISS", 0);
      var_set_num(vm, "PLUCKP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST_ERR", "PLUCKP: fail");
      var_set_str(vm, "ERR", "PLUCKP: fail");
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "PLUCKP", hr.str);
    var_set_num(vm, "PLUCKP_N", hr.n);
    var_set_num(vm, "PLUCKP_HIT", (long)hr.code);
    var_set_num(vm, "PLUCKP_MISS", hr.n - (long)hr.code);
    var_set_num(vm, "PLUCKP_FROM", have_from ? 1 : 0);
    var_set_str(vm, "PLUCKP_SRC", from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# pluckp n=%ld hit=%ld from=%d src=%s\n",
              hr.n, (long)hr.code, have_from,
              from_name[0] ? from_name : (have_from ? "?" : "PLATE"));
    bump(vm); return 1;
  }
  /* DELTAP|CHANGEOBJP a b [FROM NEW|OLD] — changed keys as plate object (JSONDELTA dual).
   * Values from b/new by default; FROM OLD takes a. Soft {} if identical.
   * Complements DIFFP (key names only) and CHANGELOGP (text lines).
   * Usability: mesh sync payload without SYS JSONDELTA / DIFFP+EACH+SETP glue:
   *   DELTAP PLATE PEER
   *   DELTAP session peer FROM OLD
   *   MERGEP FROM PLATE LAST   # apply peer delta
   */
  if (kw(&L->cur,"DELTAP") || kw(&L->cur,"CHANGEOBJP") || kw(&L->cur,"MDELTAP") ||
      kw(&L->cur,"PLATE_DELTA") || kw(&L->cur,"DELTA_P") || kw(&L->cur,"SYNCP") ||
      kw(&L->cur,"PATCHOBJP") || kw(&L->cur,"DIFFOBJP")) {
    char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
    char aname[96], bname[96];
    cubalc_host_result hr;
    int prefer_b = 1;
    Var *pv;

    lex_next(L);
    a[0] = 0; b[0] = 0; aname[0] = 0; bname[0] = 0;

#define CUBALC_RESOLVE_PLATE_ARG3(dst, dname) do { \
      (dst)[0] = 0; (dname)[0] = 0; \
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) { \
        snprintf((dname), sizeof(dname), "%s", "LAST"); \
        snprintf((dst), sizeof(dst), "%s", vm->last_str); \
        lex_next(L); \
      } else if (L->cur.kind == TK_IDENT) { \
        pv = var_get(vm, L->cur.text, 0); \
        if (pv && pv->is_str) { \
          snprintf((dname), sizeof(dname), "%s", L->cur.text); \
          snprintf((dst), sizeof(dst), "%s", pv->sval); \
          lex_next(L); \
        } else if (pv) { \
          snprintf((dname), sizeof(dname), "%s", L->cur.text); \
          snprintf((dst), sizeof(dst), "%ld", pv->val); \
          lex_next(L); \
        } else if (resolve_str_arg(vm, L, (dst), sizeof(dst)) != 0) { \
          (dst)[0] = 0; \
        } \
      } else if (resolve_str_arg(vm, L, (dst), sizeof(dst)) != 0) { \
        snprintf((dst), sizeof(dst), "%s", vm->last_str); \
      } \
      { const char *bp = (dst); \
        while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++; \
        if (*bp != '{') snprintf((dst), sizeof(dst), "%s", "{}"); \
      } \
    } while (0)

    CUBALC_RESOLVE_PLATE_ARG3(a, aname);
    CUBALC_RESOLVE_PLATE_ARG3(b, bname);
#undef CUBALC_RESOLVE_PLATE_ARG3

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USE") || kw(&L->cur,"VALUES")) {
      lex_next(L);
      if (kw(&L->cur,"OLD") || kw(&L->cur,"LEFT") || kw(&L->cur,"A") ||
          kw(&L->cur,"BASE") || kw(&L->cur,"PREV") || kw(&L->cur,"PRIOR")) {
        prefer_b = 0;
        lex_next(L);
      } else if (kw(&L->cur,"NEW") || kw(&L->cur,"RIGHT") || kw(&L->cur,"B") ||
                 kw(&L->cur,"CUR") || kw(&L->cur,"NEXT") || kw(&L->cur,"PEER")) {
        prefer_b = 1;
        lex_next(L);
      }
    }

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_delta_obj(a, b, prefer_b, &hr) != 0) {
      var_set_str(vm, "LAST", "{}");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "DELTAP_N", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "DELTAP: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "DELTAP: fail");
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "DELTAP", hr.str);
    var_set_num(vm, "DELTAP_N", hr.n);
    var_set_num(vm, "JSONDELTA_N", hr.n);
    var_set_str(vm, "DELTAP_A", aname);
    var_set_str(vm, "DELTAP_B", bname);
    var_set_num(vm, "DELTAP_NEW", prefer_b ? 1 : 0);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# deltap n=%ld a=%s b=%s prefer_b=%d\n",
              hr.n, aname[0] ? aname : "?", bname[0] ? bname : "?", prefer_b);
    bump(vm); return 1;
  }
  /* PICKP|KEEPP [FROM plate] key… — keep listed keys → subset plate (JSONPICK dual).
   * OMITP|DROPKEYSP [FROM plate] key… — drop listed keys → remainder plate (JSONDROP dual).
   * Missing keys soft-skipped (pick) / soft no-op (omit). Bare keys use PLATE.
   * LAST = plate object · LAST_N = kept|dropped count · does not mutate source.
   * Usability: slim/strip multi-plate peer/log payload without SYS JSONPICK/JSONDROP:
   *   PICKP FROM PEER "host" "status" "n"
   *   OMITP "tmp" "debug"
   *   MERGEP FROM PLATE LAST
   */
  if (kw(&L->cur,"PICKP") || kw(&L->cur,"KEEPP") || kw(&L->cur,"MPICKP") ||
      kw(&L->cur,"PLATE_PICK") || kw(&L->cur,"SELECTP") || kw(&L->cur,"KEEPKEYS") ||
      kw(&L->cur,"PROJECTP") ||
      kw(&L->cur,"OMITP") || kw(&L->cur,"DROPKEYSP") || kw(&L->cur,"MOMITP") ||
      kw(&L->cur,"PLATE_OMIT") || kw(&L->cur,"STRIPP") || kw(&L->cur,"DROPKEYS") ||
      kw(&L->cur,"STRIPKEYS")) {
    char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
    char arg[CUBALC_HOST_STR_MAX], from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, is_omit = 0;
    size_t olen = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "OMITP") == 0 || strcmp(op0, "DROPKEYSP") == 0 ||
        strcmp(op0, "MOMITP") == 0 || strcmp(op0, "PLATE_OMIT") == 0 ||
        strcmp(op0, "STRIPP") == 0 || strcmp(op0, "DROPKEYS") == 0 ||
        strcmp(op0, "STRIPKEYS") == 0)
      is_omit = 1;

    lex_next(L);
    plate[0] = 0; keys_nl[0] = 0; from_name[0] = 0; from_src[0] = 0;

    /* optional leading FROM plate */
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

    /* collect keys; stop before trailing FROM / statements */
    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
           L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
           kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
           kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
           kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
           kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
           kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"SETP") ||
           kw(&L->cur,"INCP") || kw(&L->cur,"DELP") || kw(&L->cur,"GETP") ||
           kw(&L->cur,"MERGEP") || kw(&L->cur,"NEEDP") || kw(&L->cur,"HASP") ||
           kw(&L->cur,"KEYSP") || kw(&L->cur,"PLUCKP") || kw(&L->cur,"PICKP") ||
           kw(&L->cur,"OMITP") || kw(&L->cur,"EQP") || kw(&L->cur,"SUBSETP") ||
           kw(&L->cur,"INCLUDE") || kw(&L->cur,"REQUIRE") ||
           kw(&L->cur,"FAIL") || kw(&L->cur,"PASS") || kw(&L->cur,"NOTE") ||
           kw(&L->cur,"EXIT")))
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
    }

    /* trailing FROM plate */
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

    if (!keys_nl[0]) {
      fail(vm, is_omit
           ? "OMITP [FROM plate] key… — need at least one key"
           : "PICKP [FROM plate] key… — need at least one key");
      return -1;
    }

    memset(&hr, 0, sizeof hr);
    if (is_omit) {
      if (cubalc_host_json_drop(plate, keys_nl, &hr) != 0) {
        var_set_str(vm, "LAST", "{}");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OMITP_N", 0);
        var_set_num(vm, "PICKP_N", 0);
        var_set_num(vm, "OMITP_FROM", have_from ? 1 : 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "OMITP: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "OMITP: fail");
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "OMITP", hr.str);
      var_set_str(vm, "DROPKEYSP", hr.str);
      var_set_num(vm, "OMITP_N", hr.n);
      var_set_num(vm, "JSONDROP_N", hr.n);
      var_set_num(vm, "OMITP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "OMITP_SRC", from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# omitp n=%ld from=%d src=%s\n",
                hr.n, have_from,
                from_name[0] ? from_name : (have_from ? "?" : "PLATE"));
      bump(vm); return 1;
    }

    if (cubalc_host_json_pick(plate, keys_nl, &hr) != 0) {
      var_set_str(vm, "LAST", "{}");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "PICKP_N", 0);
      var_set_num(vm, "PICKP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "PICKP: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "PICKP: fail");
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "PICKP", hr.str);
    var_set_str(vm, "KEEPP", hr.str);
    var_set_num(vm, "PICKP_N", hr.n);
    var_set_num(vm, "JSONPICK_N", hr.n);
    var_set_num(vm, "PICKP_FROM", have_from ? 1 : 0);
    var_set_str(vm, "PICKP_SRC", from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# pickp n=%ld from=%d src=%s\n",
              hr.n, have_from,
              from_name[0] ? from_name : (have_from ? "?" : "PLATE"));
    bump(vm); return 1;
  }
  /* RENAMEP|MOVEKEYP [FROM plate] old new — rename key in place (JSONRENAME dual).
   * Soft miss: LAST_N=0 plate unchanged structure · dest overwrites if present.
   * Write-back: mutates PLATE or named FROM plate var (SETP/DELP style).
   * Paths: "freq.error" / "a/b/c" · cross-nest move ok · create intermediate {}.
   * Usability: promote tmp→status without GETP+DELP+SETP glue:
   *   RENAMEP "tmp" "status"
   *   RENAMEP "cfg.port" "net.listen"
   *   RENAMEP FROM PEER "tmp_host" "host"
   */
  if (kw(&L->cur,"RENAMEP") || kw(&L->cur,"MOVEKEYP") || kw(&L->cur,"MRENAMEP") ||
      kw(&L->cur,"PLATE_RENAME") || kw(&L->cur,"MVKEYP") || kw(&L->cur,"RENAMEKEYP") ||
      kw(&L->cur,"MOVEP") || kw(&L->cur,"REKEYP")) {
    char plate[CUBALC_HOST_STR_MAX], oldk[192], newk[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; oldk[0] = 0; newk[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

    /* optional leading FROM plate */
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

    /* old key */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(oldk, sizeof oldk, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
        fail(vm, "RENAMEP [FROM plate] old new — need old key");
        return -1;
      }
      if (resolve_str_arg(vm, L, oldk, sizeof oldk) != 0)
        oldk[0] = 0;
    } else {
      fail(vm, "RENAMEP [FROM plate] old new — need old key");
      return -1;
    }

    /* new key */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(newk, sizeof newk, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
        fail(vm, "RENAMEP [FROM plate] old new — need new key");
        return -1;
      }
      if (resolve_str_arg(vm, L, newk, sizeof newk) != 0)
        newk[0] = 0;
    } else {
      fail(vm, "RENAMEP [FROM plate] old new — need new key");
      return -1;
    }

    /* trailing FROM plate */
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

    if (!oldk[0] || !newk[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RENAMEP_N", 0);
      var_set_num(vm, "RENAMEP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST_ERR", "RENAMEP: empty key");
      var_set_str(vm, "ERR", "RENAMEP: empty key");
      var_set_num(vm, "OK", 0);
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

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_rename(plate, oldk, newk, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RENAMEP_N", 0);
      var_set_num(vm, "RENAMEP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "RENAMEP: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "RENAMEP: fail");
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }

    /* write-back like SETP/DELP */
    if (have_from && from_name[0] && strcmp(from_name, "LAST") != 0)
      var_set_str(vm, from_name, hr.str);
    else if (!have_from)
      var_set_str(vm, "PLATE", hr.str);

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "RENAMEP", hr.str);
    var_set_str(vm, "MOVEKEYP", hr.str);
    var_set_num(vm, "RENAMEP_N", hr.n);
    var_set_num(vm, "JSONRENAME_N", hr.n);
    var_set_str(vm, "RENAMEP_OLD", oldk);
    var_set_str(vm, "RENAMEP_NEW", newk);
    var_set_num(vm, "RENAMEP_FROM", have_from ? 1 : 0);
    var_set_str(vm, "RENAMEP_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# renamep %s→%s n=%ld from=%d src=%s\n",
              oldk, newk, hr.n, have_from,
              from_name[0] ? from_name : (have_from ? "?" : "PLATE"));
    bump(vm); return 1;
  }
  /* KEYDIFFP|ONLYKEYSP a b — keys in a not in b → bag (JSONKEYDIFF dual).
   * KEYCOMMP|KEYINTERP a b — keys in both → bag (JSONKEYCOMM dual).
   * Structure-only (ignores values). Soft always OK=1 · LAST_N=count.
   * Note: KEYSDIFFP stays DIFFP alias (value+structure changes) — not this.
   * Usability: mesh additions/shared keys without SYS JSONKEYDIFF/JSONKEYCOMM:
   *   KEYDIFFP PEER PLATE   # keys peer has that local lacks
   *   KEYCOMMP PLATE PEER   # shared field names
   */
  if (kw(&L->cur,"KEYDIFFP") || kw(&L->cur,"ONLYKEYSP") || kw(&L->cur,"KEYSONLYP") ||
      kw(&L->cur,"ONLYINP") || kw(&L->cur,"MKEYDIFFP") || kw(&L->cur,"PLATE_KEYDIFF") ||
      kw(&L->cur,"KEYCOMMP") || kw(&L->cur,"KEYINTERP") || kw(&L->cur,"COMMONKEYSP") ||
      kw(&L->cur,"KEYANDP") || kw(&L->cur,"MKEYCOMMP") || kw(&L->cur,"PLATE_KEYCOMM") ||
      kw(&L->cur,"SHAREDKEYSP")) {
    char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
    char aname[96], bname[96];
    cubalc_host_result hr;
    int want_inter = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "KEYCOMMP") == 0 || strcmp(op0, "KEYINTERP") == 0 ||
        strcmp(op0, "COMMONKEYSP") == 0 || strcmp(op0, "KEYANDP") == 0 ||
        strcmp(op0, "MKEYCOMMP") == 0 || strcmp(op0, "PLATE_KEYCOMM") == 0 ||
        strcmp(op0, "SHAREDKEYSP") == 0)
      want_inter = 1;

    lex_next(L);
    a[0] = 0; b[0] = 0; aname[0] = 0; bname[0] = 0;

#define CUBALC_RESOLVE_PLATE_ARG_KD(dst, dname) do { \
      (dst)[0] = 0; (dname)[0] = 0; \
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) { \
        snprintf((dname), sizeof(dname), "%s", "LAST"); \
        snprintf((dst), sizeof(dst), "%s", vm->last_str); \
        lex_next(L); \
      } else if (L->cur.kind == TK_IDENT) { \
        pv = var_get(vm, L->cur.text, 0); \
        if (pv && pv->is_str) { \
          snprintf((dname), sizeof(dname), "%s", L->cur.text); \
          snprintf((dst), sizeof(dst), "%s", pv->sval); \
          lex_next(L); \
        } else if (pv) { \
          snprintf((dname), sizeof(dname), "%s", L->cur.text); \
          snprintf((dst), sizeof(dst), "%ld", pv->val); \
          lex_next(L); \
        } else if (resolve_str_arg(vm, L, (dst), sizeof(dst)) != 0) { \
          (dst)[0] = 0; \
        } \
      } else if (resolve_str_arg(vm, L, (dst), sizeof(dst)) != 0) { \
        snprintf((dst), sizeof(dst), "%s", vm->last_str); \
      } \
      { const char *bp = (dst); \
        while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++; \
        if (*bp != '{') snprintf((dst), sizeof(dst), "%s", "{}"); \
      } \
    } while (0)

    CUBALC_RESOLVE_PLATE_ARG_KD(a, aname);
    CUBALC_RESOLVE_PLATE_ARG_KD(b, bname);
#undef CUBALC_RESOLVE_PLATE_ARG_KD

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_key_set_op(a, b, want_inter, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "KEYDIFFP_N", 0);
      var_set_num(vm, "KEYCOMMP_N", 0);
      var_set_str(vm, "LAST_ERR",
                  want_inter ? "KEYCOMMP: fail" : "KEYDIFFP: fail");
      var_set_str(vm, "ERR",
                  want_inter ? "KEYCOMMP: fail" : "KEYDIFFP: fail");
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    if (want_inter) {
      var_set_str(vm, "KEYCOMMP", hr.str);
      var_set_str(vm, "KEYINTERP", hr.str);
      var_set_num(vm, "KEYCOMMP_N", hr.n);
      var_set_num(vm, "JSONKEYCOMM_N", hr.n);
      var_set_str(vm, "KEYCOMMP_A", aname);
      var_set_str(vm, "KEYCOMMP_B", bname);
    } else {
      var_set_str(vm, "KEYDIFFP", hr.str);
      var_set_str(vm, "ONLYKEYSP", hr.str);
      var_set_num(vm, "KEYDIFFP_N", hr.n);
      var_set_num(vm, "JSONKEYDIFF_N", hr.n);
      var_set_str(vm, "KEYDIFFP_A", aname);
      var_set_str(vm, "KEYDIFFP_B", bname);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld a=%s b=%s\n",
              want_inter ? "keycommp" : "keydiffp",
              hr.n, aname[0] ? aname : "?", bname[0] ? bname : "?");
    bump(vm); return 1;
  }
  /* COPYP|DUPKEYP [FROM plate] src dst — copy key raw value (JSONCOPY dual).
   * SWAPP|XCHGP [FROM plate] a b — exchange two keys (JSONSWAP dual).
   * Soft miss: COPYP_N=0 plate unchanged · SWAPP both miss n=0 · one miss = move.
   * Write-back: mutates PLATE or named FROM plate (SETP/RENAMEP style).
   * Usability: dual-buffer / snapshot fields without GETP+SETP glue:
   *   COPYP "status" "prev"
   *   SWAPP FROM PEER "cur" "prev"
   */
  if (kw(&L->cur,"COPYP") || kw(&L->cur,"DUPKEYP") || kw(&L->cur,"MCOPYP") ||
      kw(&L->cur,"PLATE_COPY") || kw(&L->cur,"COPYKEYP") || kw(&L->cur,"SNAPSHOTP") ||
      kw(&L->cur,"SWAPP") || kw(&L->cur,"XCHGP") || kw(&L->cur,"MSWAPP") ||
      kw(&L->cur,"PLATE_SWAP") || kw(&L->cur,"SWAPKEYP") || kw(&L->cur,"EXCHP") ||
      kw(&L->cur,"FLIPKEYP")) {
    /* Paths ok: "cfg.port" / "flags.debug" via host copy_key / swap_keys. */
    char plate[CUBALC_HOST_STR_MAX], ka[192], kb[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, is_swap = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "SWAPP") == 0 || strcmp(op0, "XCHGP") == 0 ||
        strcmp(op0, "MSWAPP") == 0 || strcmp(op0, "PLATE_SWAP") == 0 ||
        strcmp(op0, "SWAPKEYP") == 0 || strcmp(op0, "EXCHP") == 0 ||
        strcmp(op0, "FLIPKEYP") == 0)
      is_swap = 1;

    lex_next(L);
    plate[0] = 0; ka[0] = 0; kb[0] = 0;
    from_name[0] = 0; from_src[0] = 0;
    memset(&hr, 0, sizeof hr);

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

    /* key A (src) */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(ka, sizeof ka, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
        fail(vm, is_swap ? "SWAPP [FROM plate] a b — need key A"
                         : "COPYP [FROM plate] src dst — need src key");
        return -1;
      }
      if (resolve_str_arg(vm, L, ka, sizeof ka) != 0)
        ka[0] = 0;
    } else {
      fail(vm, is_swap ? "SWAPP [FROM plate] a b — need key A"
                       : "COPYP [FROM plate] src dst — need src key");
      return -1;
    }

    /* key B (dst) */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(kb, sizeof kb, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
        fail(vm, is_swap ? "SWAPP [FROM plate] a b — need key B"
                         : "COPYP [FROM plate] src dst — need dst key");
        return -1;
      }
      if (resolve_str_arg(vm, L, kb, sizeof kb) != 0)
        kb[0] = 0;
    } else {
      fail(vm, is_swap ? "SWAPP [FROM plate] a b — need key B"
                       : "COPYP [FROM plate] src dst — need dst key");
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

    if (!ka[0] || !kb[0]) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", is_swap ? "SWAPP: empty key" : "COPYP: empty key");
      var_set_str(vm, "ERR", is_swap ? "SWAPP: empty key" : "COPYP: empty key");
      var_set_num(vm, "COPYP_N", 0);
      var_set_num(vm, "SWAPP_N", 0);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "COPYP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "SWAPP_FROM", have_from ? 1 : 0);
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

    memset(&hr, 0, sizeof hr);
    if (!is_swap) {
      if (cubalc_host_json_copy_key(plate, ka, kb, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "COPYP_N", 0);
        var_set_num(vm, "COPYP_FROM", have_from ? 1 : 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "COPYP: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "COPYP: fail");
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (have_from && from_name[0] && strcmp(from_name, "LAST") != 0)
        var_set_str(vm, from_name, hr.str);
      else if (!have_from)
        var_set_str(vm, "PLATE", hr.str);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "COPYP", hr.str);
      if (hr.n)
        var_set_str(vm, "DUPKEYP", hr.str);
      var_set_num(vm, "COPYP_N", hr.n);
      if (hr.n)
        var_set_num(vm, "JSONCOPY_N", hr.n);
      var_set_str(vm, "COPYP_SRC", ka);
      var_set_str(vm, "COPYP_DST", kb);
      var_set_num(vm, "COPYP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "COPYP_VAR",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# copyp %s→%s n=%ld from=%d\n", ka, kb, hr.n, have_from);
      bump(vm); return 1;
    }

    /* SWAPP — path-aware via host swap_keys */
    if (cubalc_host_json_swap_keys(plate, ka, kb, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SWAPP_N", 0);
      var_set_num(vm, "SWAPP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "SWAPP: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "SWAPP: fail");
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (have_from && from_name[0] && strcmp(from_name, "LAST") != 0)
      var_set_str(vm, from_name, hr.str);
    else if (!have_from)
      var_set_str(vm, "PLATE", hr.str);
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "SWAPP", hr.str);
    if (hr.n)
      var_set_str(vm, "XCHGP", hr.str);
    var_set_num(vm, "SWAPP_N", hr.n);
    if (hr.n)
      var_set_num(vm, "JSONSWAP_N", hr.n);
    var_set_str(vm, "SWAPP_A", ka);
    var_set_str(vm, "SWAPP_B", kb);
    var_set_num(vm, "SWAPP_FROM", have_from ? 1 : 0);
    var_set_str(vm, "SWAPP_VAR",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# swapp %s↔%s n=%ld from=%d\n", ka, kb, hr.n, have_from);
    bump(vm); return 1;
  }
  /* LENP|NKEYSP [FROM plate] [path] — key count → LAST_N (JSONLEN dual).
   * EMPTYP|ISEMPTYP [FROM plate] [path] — soft 0|1 empty plate (JSONEMPTY dual).
   * NONEMPTYP|HASKEYSP [FROM plate] [path] — inverse of EMPTYP.
   * VALSP|VALUEP [FROM plate] [path] — values bag · twin KEYSP (JSONVALUES dual).
   * Optional path peels nest object (LENOBJ/EMPTYOBJ/VALSOBJ dual without glue):
   *   LENP "cfg" · EMPTYP "cfg.flags" · VALSP FROM PEER "meta"
   * Bare uses conventional PLATE. Soft non-object / miss path → 0 / empty bag.
   * Usability: size/empty/value probes without SYS JSONLEN/JSONEMPTY/JSONVALUES:
   *   LENP FROM PEER
   *   IF EMPTYP "cfg" … END
   *   VALSP FROM session
   */
  if (kw(&L->cur,"LENP") || kw(&L->cur,"NKEYSP") || kw(&L->cur,"MLENP") ||
      kw(&L->cur,"PLATE_LEN") || kw(&L->cur,"COUNTKEYSP") || kw(&L->cur,"SIZEP") ||
      kw(&L->cur,"EMPTYP") || kw(&L->cur,"ISEMPTYP") || kw(&L->cur,"MEMPTYP") ||
      kw(&L->cur,"PLATE_EMPTY") || kw(&L->cur,"EMPTYPLATE") ||
      kw(&L->cur,"NONEMPTYP") || kw(&L->cur,"HASKEYSP") || kw(&L->cur,"MNONEMPTYP") ||
      kw(&L->cur,"PLATE_NONEMPTY") || kw(&L->cur,"NONEMPTYPLATE") ||
      kw(&L->cur,"VALSP") || kw(&L->cur,"VALUEP") || kw(&L->cur,"MVALSP") ||
      kw(&L->cur,"PLATE_VALS") || kw(&L->cur,"VALP")) {
    char plate[CUBALC_HOST_STR_MAX], from_name[96], from_src[CUBALC_HOST_STR_MAX];
    char nest_path[192];
    cubalc_host_result hr, obj;
    int have_from = 0, mode = 0; /* 0=len 1=empty 2=nonempty 3=vals */
    long hit;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "EMPTYP") == 0 || strcmp(op0, "ISEMPTYP") == 0 ||
        strcmp(op0, "MEMPTYP") == 0 || strcmp(op0, "PLATE_EMPTY") == 0 ||
        strcmp(op0, "EMPTYPLATE") == 0)
      mode = 1;
    else if (strcmp(op0, "NONEMPTYP") == 0 || strcmp(op0, "HASKEYSP") == 0 ||
             strcmp(op0, "MNONEMPTYP") == 0 || strcmp(op0, "PLATE_NONEMPTY") == 0 ||
             strcmp(op0, "NONEMPTYPLATE") == 0)
      mode = 2;
    else if (strcmp(op0, "VALSP") == 0 || strcmp(op0, "VALUEP") == 0 ||
             strcmp(op0, "MVALSP") == 0 || strcmp(op0, "PLATE_VALS") == 0 ||
             strcmp(op0, "VALP") == 0)
      mode = 3;
    else
      mode = 0;

    lex_next(L);
    plate[0] = 0; from_name[0] = 0; from_src[0] = 0; nest_path[0] = 0;

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

    /* optional nest path: LENP "cfg" · EMPTYP FROM PEER "meta" */
    if (L->cur.kind == TK_STR ||
        (L->cur.kind == TK_IDENT &&
         !kw(&L->cur,"FROM") && !kw(&L->cur,"USING") && !kw(&L->cur,"OF") &&
         !kw(&L->cur,"WITHPLATE") && !kw(&L->cur,"PLATEFROM") &&
         !kw(&L->cur,"END") && !kw(&L->cur,"ELSE") && !kw(&L->cur,"ELIF"))) {
      if (resolve_str_arg(vm, L, nest_path, sizeof nest_path) != 0)
        nest_path[0] = 0;
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

    memset(&obj, 0, sizeof obj);
    cubalc_host_json_path_obj(plate, nest_path[0] ? nest_path : NULL, &obj);
    snprintf(plate, sizeof plate, "%s", obj.str);

    if (mode == 3) {
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_values(plate, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "VALSP_N", 0);
        var_set_num(vm, "VALSP_FROM", have_from ? 1 : 0);
        var_set_str(vm, "VALSP_PATH", nest_path);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "VALSP: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "VALSP: fail");
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "VALSP", hr.str);
      var_set_str(vm, "VALUEP", hr.str);
      var_set_num(vm, "VALSP_N", hr.n);
      var_set_num(vm, "JSONVALUES_N", hr.n);
      var_set_num(vm, "VALSP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "VALSP_PATH", nest_path);
      var_set_str(vm, "VALSP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# valsp n=%ld from=%d path=%s\n",
                hr.n, have_from, nest_path[0] ? nest_path : ".");
      bump(vm); return 1;
    }

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_len(plate, &hr) != 0) {
      /* soft zero on empty object */
      hr.n = 0;
      hr.ok = 1;
    }

    if (mode == 0) {
      /* LENP — keep (sub)plate in LAST for chain (JSONLEN style) */
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "LENP_N", hr.n);
      var_set_num(vm, "JSONLEN_N", hr.n);
      var_set_num(vm, "KEYSP_N", hr.n); /* size dual of KEYSP count */
      var_set_num(vm, "LENP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LENP_PATH", nest_path);
      var_set_str(vm, "LENP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# lenp n=%ld from=%d path=%s\n",
                hr.n, have_from, nest_path[0] ? nest_path : ".");
      bump(vm); return 1;
    }

    /* EMPTYP / NONEMPTYP */
    hit = (hr.n == 0) ? 1 : 0;
    if (mode == 2)
      hit = hit ? 0 : 1;
    var_set_str(vm, "LAST", hit ? "1" : "0");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hit ? "1" : "0");
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "EMPTYP_N", hit);
    var_set_num(vm, "NONEMPTYP_N", hit);
    var_set_num(vm, "JSONEMPTY_N", (mode == 1) ? hit : (hit ? 0 : 1));
    var_set_num(vm, "LENP_N", hr.n);
    var_set_num(vm, "EMPTYP_FROM", have_from ? 1 : 0);
    var_set_num(vm, "NONEMPTYP_FROM", have_from ? 1 : 0);
    var_set_str(vm, "EMPTYP_PATH", nest_path);
    var_set_str(vm, "EMPTYP_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s hit=%ld keys=%ld from=%d path=%s\n",
              mode == 2 ? "nonemptyp" : "emptyp", hit, hr.n, have_from,
              nest_path[0] ? nest_path : ".");
    bump(vm); return 1;
  }
  /* TYPEP|KINDP [FROM plate] key — field kind probe (JSONTYPE dual).
   * LAST = missing|num|str|bool|null|obj|arr · LAST_N = 0..6.
   * Soft miss → missing/0 · multi-plate FROM · no SYS JSONTYPE glue.
   * Dotted/slash paths (same as GETP): TYPEP "freq.error" · TYPEP "meta/role".
   * Usability: choose JSONN vs string peel vs nested without IF-guess:
   *   TYPEP "retries"
   *   TYPEP FROM PEER "payload"
   *   TYPEP "freq.error"
   */
  if (kw(&L->cur,"TYPEP") || kw(&L->cur,"KINDP") || kw(&L->cur,"MTYPEP") ||
      kw(&L->cur,"PLATE_TYPE") || kw(&L->cur,"KEYTYPEP") || kw(&L->cur,"TYPEOFP") ||
      kw(&L->cur,"FIELDTYPEP") || kw(&L->cur,"JTYPEP")) {
    char plate[CUBALC_HOST_STR_MAX], key[96];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    const char *kind_s = "missing";
    long kind_n = 0;
    int have_from = 0;
    cubalc_host_result gr;
    Var *pv;

    lex_next(L);
    plate[0] = 0; key[0] = 0; from_name[0] = 0; from_src[0] = 0;

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

    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(key, sizeof key, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
        fail(vm, "TYPEP [FROM plate] key — need key");
        return -1;
      }
      if (resolve_str_arg(vm, L, key, sizeof key) != 0)
        key[0] = 0;
    } else {
      fail(vm, "TYPEP [FROM plate] key — need key");
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

    if (!key[0]) {
      var_set_str(vm, "LAST", "missing");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "missing");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TYPEP_N", 0);
      var_set_str(vm, "TYPEP", "missing");
      var_set_num(vm, "TYPEP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST_ERR", "TYPEP: empty key");
      var_set_str(vm, "ERR", "TYPEP: empty key");
      var_set_num(vm, "OK", 0);
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

    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_path_get_raw(plate, key, &gr) != 0) {
      kind_s = "missing";
      kind_n = 0;
    } else {
      const char *v = gr.str;
      while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
      if (*v == '"') {
        kind_s = "str";
        kind_n = 2;
      } else if (*v == '{') {
        kind_s = "obj";
        kind_n = 5;
      } else if (*v == '[') {
        kind_s = "arr";
        kind_n = 6;
      } else if (strncmp(v, "true", 4) == 0 || strncmp(v, "false", 5) == 0) {
        kind_s = "bool";
        kind_n = 3;
      } else if (strncmp(v, "null", 4) == 0) {
        kind_s = "null";
        kind_n = 4;
      } else if (*v == '-' || (*v >= '0' && *v <= '9')) {
        kind_s = "num";
        kind_n = 1;
      } else {
        kind_s = "str";
        kind_n = 2;
      }
    }

    var_set_str(vm, "LAST", kind_s);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", kind_s);
    vm->last_n = kind_n;
    var_set_num(vm, "LAST_N", kind_n);
    var_set_str(vm, "TYPEP", kind_s);
    var_set_str(vm, "KINDP", kind_s);
    var_set_str(vm, "JSONTYPE", kind_s);
    var_set_num(vm, "TYPEP_N", kind_n);
    var_set_num(vm, "JSONTYPE_N", kind_n);
    var_set_str(vm, "TYPEP_KEY", key);
    var_set_num(vm, "TYPEP_FROM", have_from ? 1 : 0);
    var_set_str(vm, "TYPEP_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# typep key=%s kind=%s n=%ld from=%d\n",
              key, kind_s, kind_n, have_from);
    bump(vm); return 1;
  }

  /* GETOBJ|PEEKOBJ|OBJGET|GETNEST [FROM plate] key [OR|DEFAULT fb] [INTO name]
   * Peel nested JSON object field as plate text → LAST (and optional INTO var).
   * GETP only peels scalars (string/num/bool); nested objects miss — this is the fix:
   *   TYPEP "meta"          → obj
   *   GETOBJ "meta" INTO M  → M={"x":1}
   *   GETOBJ "cfg.meta"     → deep path peel (same plane as GETP paths)
   *   SETP FROM M "x" 9
   *   SETOBJ "meta" M
   * Soft miss / non-object → OR fallback or sticky LAST_ERR. Multi-plate FROM.
   * SETOBJ|PUTOBJ|NESTP|PUTNEST [FROM plate] key value
   * Nest a plate object under key (write-back) — paths create intermediate {}.
   */
  if (kw(&L->cur,"GETOBJ") || kw(&L->cur,"PEEKOBJ") || kw(&L->cur,"OBJGET") ||
      kw(&L->cur,"GETNEST") || kw(&L->cur,"NESTGET") || kw(&L->cur,"SUBOBJ") ||
      kw(&L->cur,"OBJPEEL") || kw(&L->cur,"PLATEOBJ") || kw(&L->cur,"MGETOBJ") ||
      kw(&L->cur,"SETOBJ") || kw(&L->cur,"PUTOBJ") || kw(&L->cur,"NESTP") ||
      kw(&L->cur,"PUTNEST") || kw(&L->cur,"SETOBJECT") || kw(&L->cur,"NESTPLATE") ||
      kw(&L->cur,"MSETOBJ") || kw(&L->cur,"PLATE_SETOBJ") || kw(&L->cur,"PLATE_GETOBJ")) {
    char plate[CUBALC_HOST_STR_MAX], key[192], val[CUBALC_HOST_STR_MAX];
    char fb[CUBALC_HOST_STR_MAX], from_name[96], from_src[CUBALC_HOST_STR_MAX];
    char into_name[96];
    cubalc_host_result gr, hr;
    int is_set = 0, have_from = 0, have_fb = 0, have_into = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "SETOBJ") == 0 || strcmp(op0, "PUTOBJ") == 0 ||
        strcmp(op0, "NESTP") == 0 || strcmp(op0, "PUTNEST") == 0 ||
        strcmp(op0, "SETOBJECT") == 0 || strcmp(op0, "NESTPLATE") == 0 ||
        strcmp(op0, "MSETOBJ") == 0 || strcmp(op0, "PLATE_SETOBJ") == 0)
      is_set = 1;

    lex_next(L);
    plate[0] = 0; key[0] = 0; val[0] = 0; fb[0] = 0;
    from_name[0] = 0; from_src[0] = 0; into_name[0] = 0;

    /* optional leading FROM plate */
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

    /* key */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(key, sizeof key, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
          kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"INTO") ||
          kw(&L->cur,"AS")) {
        fail(vm, is_set ? "SETOBJ [FROM plate] key value — need key"
                        : "GETOBJ [FROM plate] key [OR fb] [INTO name] — need key");
        return -1;
      }
      if (resolve_str_arg(vm, L, key, sizeof key) != 0)
        key[0] = 0;
    } else {
      fail(vm, is_set ? "SETOBJ [FROM plate] key value — need key"
                      : "GETOBJ [FROM plate] key [OR fb] [INTO name] — need key");
      return -1;
    }

    if (is_set) {
      /* value: plate object string / var / LAST */
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
           kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM"))) {
        /* trailing FROM handled below — missing value */
        fail(vm, "SETOBJ [FROM plate] key value — need value");
        return -1;
      }
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(val, sizeof val, "%s", vm->last_str);
        lex_next(L);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
          fail(vm, "SETOBJ [FROM plate] key value — need value");
          return -1;
        }
      } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        /* allow nesting a number as raw (rare) */
        long n = parse_expr(vm, L);
        snprintf(val, sizeof val, "%ld", n);
      } else {
        fail(vm, "SETOBJ [FROM plate] key value — need value");
        return -1;
      }
    } else {
      /* GETOBJ: optional OR fallback then optional INTO */
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK") || kw(&L->cur,"SOFT")) {
        lex_next(L);
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(fb, sizeof fb, "%ld", n);
          have_fb = 1;
        } else if (L->cur.kind == TK_IDENT &&
                   (kw(&L->cur,"FROM") || kw(&L->cur,"INTO") || kw(&L->cur,"AS"))) {
          fail(vm, "GETOBJ key OR fallback — need fallback");
          return -1;
        } else if (resolve_str_arg(vm, L, fb, sizeof fb) == 0) {
          have_fb = 1;
        } else {
          fail(vm, "GETOBJ key OR fallback — need fallback");
          return -1;
        }
      }
      if (kw(&L->cur,"INTO") || kw(&L->cur,"AS") || kw(&L->cur,"->") ||
          kw(&L->cur,"BIND") || kw(&L->cur,"TO")) {
        lex_next(L);
        if (L->cur.kind != TK_IDENT) {
          fail(vm, "GETOBJ … INTO name — need var name");
          return -1;
        }
        snprintf(into_name, sizeof into_name, "%s", L->cur.text);
        have_into = 1;
        lex_next(L);
      }
    }

    /* trailing FROM if not already */
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

    /* trailing INTO after FROM for GETOBJ "k" FROM plate INTO name */
    if (!is_set && !have_into && (kw(&L->cur,"INTO") || kw(&L->cur,"AS") ||
                                  kw(&L->cur,"->") || kw(&L->cur,"BIND") ||
                                  kw(&L->cur,"TO"))) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT) {
        fail(vm, "GETOBJ … INTO name — need var name");
        return -1;
      }
      snprintf(into_name, sizeof into_name, "%s", L->cur.text);
      have_into = 1;
      lex_next(L);
    }

    /* resolve working plate */
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

    if (!key[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", is_set ? "SETOBJ: empty key" : "GETOBJ: empty key");
      var_set_str(vm, "ERR", is_set ? "SETOBJ: empty key" : "GETOBJ: empty key");
      var_set_num(vm, is_set ? "SETOBJ_FROM" : "GETOBJ_FROM", have_from ? 1 : 0);
      bump(vm); return 1;
    }

    if (!is_set) {
      /* peel nested object (paths ok: GETOBJ "cfg.meta") */
      int hit = 0;
      const char *v;
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get_raw(plate, key, &gr) == 0) {
        v = gr.str;
        while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
        if (*v == '{') {
          /* normalize to start at { */
          if (v != gr.str) {
            size_t n = strlen(v);
            memmove(gr.str, v, n + 1);
            gr.n = (long)n;
          }
          hit = 1;
        }
      }
      if (hit) {
        var_set_str(vm, "LAST", gr.str);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", gr.str);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_str(vm, "GETOBJ", gr.str);
        var_set_str(vm, "PEEKOBJ", gr.str);
        var_set_num(vm, "GETOBJ_HIT", 1);
        var_set_num(vm, "JSON_HIT", 1);
        var_set_num(vm, "PATH_DEPTH", (long)gr.code > 0 ? (long)gr.code : 1L);
        var_set_num(vm, "OK", 1);
        if (have_into && into_name[0])
          var_set_str(vm, into_name, gr.str);
      } else if (have_fb) {
        var_set_str(vm, "LAST", fb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "GETOBJ", fb);
        var_set_num(vm, "GETOBJ_HIT", 0);
        var_set_num(vm, "GETOBJ_OR", 1);
        var_set_num(vm, "OK", 1);
        if (have_into && into_name[0])
          var_set_str(vm, into_name, fb);
      } else {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "GETOBJ_HIT", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "GETOBJ: key miss or not object");
        var_set_str(vm, "ERR", "GETOBJ: key miss or not object");
      }
      var_set_str(vm, "GETOBJ_KEY", key);
      var_set_num(vm, "GETOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "GETOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "GETOBJ_INTO", have_into ? 1 : 0);
      if (have_into)
        var_set_str(vm, "GETOBJ_INTO_NAME", into_name);
      if (vm->trace)
        fprintf(vm->trace, "# getobj key=%s hit=%d from=%d into=%d\n",
                key, hit, have_from, have_into);
      bump(vm); return 1;
    }

    /* SETOBJ: require value is object (or empty → {}) */
    {
      const char *bp = val;
      while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++;
      if (!*bp)
        snprintf(val, sizeof val, "%s", "{}");
      else if (*bp != '{') {
        /* wrap non-object as refused soft fail — agents should nest plates */
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SETOBJ_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "SETOBJ: value not object");
        var_set_str(vm, "ERR", "SETOBJ: value not object");
        var_set_str(vm, "SETOBJ_KEY", key);
        var_set_num(vm, "SETOBJ_FROM", have_from ? 1 : 0);
        bump(vm); return 1;
      } else if (bp != val) {
        size_t n = strlen(bp);
        memmove(val, bp, n + 1);
      }
    }

    memset(&hr, 0, sizeof hr);
    /* path-aware raw object insert (creates intermediate {} on deep path) */
    if (cubalc_host_json_path_set(plate, key, val, 1, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SETOBJ_N", 0);
      var_set_num(vm, "SETOBJ_FROM", have_from ? 1 : 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "SETOBJ: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "SETOBJ: fail");
      bump(vm); return 1;
    }
    if (have_from && from_name[0] && strcmp(from_name, "LAST") != 0)
      var_set_str(vm, from_name, hr.str);
    else if (!have_from)
      var_set_str(vm, "PLATE", hr.str);
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "SETOBJ", hr.str);
    var_set_str(vm, "NESTP", hr.str);
    var_set_num(vm, "SETOBJ_N", hr.n);
    var_set_str(vm, "SETOBJ_KEY", key);
    var_set_num(vm, "SETOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "SETOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "PATH_DEPTH", (long)hr.code > 0 ? (long)hr.code : 1L);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# setobj key=%s updated=%ld from=%d\n",
              key, hr.n, have_from);
    bump(vm); return 1;
  }

  /* MERGEOBJ|PATCHNEST|UPDATEOBJ [FROM plate] key overlay
   * DEFAULTOBJ|ENSUREOBJ|DEFAULTNEST [FROM plate] key defaults
   * One-shot nest patch: peel nested object, MERGEP/DEFAULTP, SETOBJ write-back.
   * Key may be dotted path: MERGEOBJ "cfg.meta" "{\"role\":\"leader\"}"
   * Miss nested key → start from {} then apply overlay/defaults.
   * Overlay wins (MERGEOBJ); DEFAULTOBJ only fills missing nested keys.
   * Usability: no GETOBJ+MERGEP+SETOBJ glue for agent nested config:
   *   MERGEOBJ "meta" "{\"role\":\"leader\"}"
   *   DEFAULTOBJ FROM PEER "cfg" "{\"port\":8080,\"tls\":0}"
   * LAST = outer plate · LAST_N = keys applied · MERGED = nested result.
   */
  if (kw(&L->cur,"MERGEOBJ") || kw(&L->cur,"PATCHNEST") || kw(&L->cur,"UPDATEOBJ") ||
      kw(&L->cur,"PATCHOBJ") || kw(&L->cur,"MMERGEOBJ") || kw(&L->cur,"NESTMERGE") ||
      kw(&L->cur,"MERGEINTO") || kw(&L->cur,"NESTPATCH") ||
      kw(&L->cur,"DEFAULTOBJ") || kw(&L->cur,"ENSUREOBJ") || kw(&L->cur,"DEFAULTNEST") ||
      kw(&L->cur,"ENSURENEST") || kw(&L->cur,"SEEDOBJ") || kw(&L->cur,"MDEFAULTOBJ") ||
      kw(&L->cur,"NESTDEFAULT") || kw(&L->cur,"OBJDEFAULT")) {
    char plate[CUBALC_HOST_STR_MAX], key[192], over[CUBALC_HOST_STR_MAX];
    char nest[CUBALC_HOST_STR_MAX], from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result gr, mer, hr;
    int have_from = 0, is_def = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "DEFAULTOBJ") == 0 || strcmp(op0, "ENSUREOBJ") == 0 ||
        strcmp(op0, "DEFAULTNEST") == 0 || strcmp(op0, "ENSURENEST") == 0 ||
        strcmp(op0, "SEEDOBJ") == 0 || strcmp(op0, "MDEFAULTOBJ") == 0 ||
        strcmp(op0, "NESTDEFAULT") == 0 || strcmp(op0, "OBJDEFAULT") == 0)
      is_def = 1;

    lex_next(L);
    plate[0] = 0; key[0] = 0; over[0] = 0; nest[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

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

    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(key, sizeof key, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
        fail(vm, is_def ? "DEFAULTOBJ [FROM plate] key defaults — need key"
                        : "MERGEOBJ [FROM plate] key overlay — need key");
        return -1;
      }
      if (resolve_str_arg(vm, L, key, sizeof key) != 0)
        key[0] = 0;
    } else {
      fail(vm, is_def ? "DEFAULTOBJ [FROM plate] key defaults — need key"
                      : "MERGEOBJ [FROM plate] key overlay — need key");
      return -1;
    }

    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
         kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM"))) {
      fail(vm, is_def ? "DEFAULTOBJ key defaults — need defaults object"
                      : "MERGEOBJ key overlay — need overlay object");
      return -1;
    }
    if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(over, sizeof over, "%s", vm->last_str);
      lex_next(L);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (resolve_str_arg(vm, L, over, sizeof over) != 0) {
        fail(vm, is_def ? "DEFAULTOBJ key defaults — need defaults object"
                        : "MERGEOBJ key overlay — need overlay object");
        return -1;
      }
    } else {
      fail(vm, is_def ? "DEFAULTOBJ key defaults — need defaults object"
                      : "MERGEOBJ key overlay — need overlay object");
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

    if (!key[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", is_def ? "DEFAULTOBJ: empty key" : "MERGEOBJ: empty key");
      var_set_str(vm, "ERR", is_def ? "DEFAULTOBJ: empty key" : "MERGEOBJ: empty key");
      var_set_num(vm, "MERGEOBJ_FROM", have_from ? 1 : 0);
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

    {
      const char *bp = over;
      while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++;
      if (!*bp || *bp != '{') {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR",
                    is_def ? "DEFAULTOBJ: defaults not object"
                           : "MERGEOBJ: overlay not object");
        var_set_str(vm, "ERR",
                    is_def ? "DEFAULTOBJ: defaults not object"
                           : "MERGEOBJ: overlay not object");
        var_set_str(vm, "MERGEOBJ_KEY", key);
        var_set_num(vm, "MERGEOBJ_FROM", have_from ? 1 : 0);
        bump(vm); return 1;
      }
      if (bp != over) {
        size_t n = strlen(bp);
        memmove(over, bp, n + 1);
      }
    }

    /* path-aware peel: miss/non-object → {} then apply overlay/defaults */
    snprintf(nest, sizeof nest, "%s", "{}");
    memset(&gr, 0, sizeof gr);
    cubalc_host_json_path_obj(plate, key, &gr);
    snprintf(nest, sizeof nest, "%s", gr.str);

    memset(&mer, 0, sizeof mer);
    if (is_def) {
      if (cubalc_host_json_defaults(nest, over, &mer) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", mer.err[0] ? mer.err : "DEFAULTOBJ: fail");
        var_set_str(vm, "ERR", mer.err[0] ? mer.err : "DEFAULTOBJ: fail");
        var_set_str(vm, "MERGEOBJ_KEY", key);
        var_set_num(vm, "MERGEOBJ_FROM", have_from ? 1 : 0);
        bump(vm); return 1;
      }
    } else {
      if (cubalc_host_json_merge(nest, over, &mer) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", mer.err[0] ? mer.err : "MERGEOBJ: fail");
        var_set_str(vm, "ERR", mer.err[0] ? mer.err : "MERGEOBJ: fail");
        var_set_str(vm, "MERGEOBJ_KEY", key);
        var_set_num(vm, "MERGEOBJ_FROM", have_from ? 1 : 0);
        bump(vm); return 1;
      }
    }
    snprintf(nest, sizeof nest, "%s", mer.str);

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_path_set(plate, key, nest, 1, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "MERGEOBJ: write-back fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "MERGEOBJ: write-back fail");
      var_set_str(vm, "MERGEOBJ_KEY", key);
      var_set_num(vm, "MERGEOBJ_FROM", have_from ? 1 : 0);
      bump(vm); return 1;
    }

    if (have_from && from_name[0] && strcmp(from_name, "LAST") != 0)
      var_set_str(vm, from_name, hr.str);
    else if (!have_from)
      var_set_str(vm, "PLATE", hr.str);

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = mer.n;
    var_set_num(vm, "LAST_N", mer.n);
    var_set_str(vm, "MERGEOBJ", hr.str);
    var_set_str(vm, "MERGED", nest);
    var_set_str(vm, "NEST", nest);
    var_set_num(vm, "MERGEOBJ_N", mer.n);
    var_set_str(vm, "MERGEOBJ_KEY", key);
    var_set_num(vm, "MERGEOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "MERGEOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "MERGEOBJ_DEFAULT", is_def ? 1 : 0);
    var_set_num(vm, "PATH_DEPTH", (long)hr.code > 0 ? (long)hr.code : 1L);
    if (is_def) {
      var_set_str(vm, "DEFAULTOBJ", hr.str);
      var_set_num(vm, "DEFAULTOBJ_N", mer.n);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s key=%s applied=%ld from=%d\n",
              is_def ? "defaultobj" : "mergeobj", key, mer.n, have_from);
    bump(vm); return 1;
  }

  /* GETPOBJ|PEEKNEST [FROM plate] nest field [OR fb]
   * SETPOBJ|PUTNESTF [FROM plate] nest field value
   * INCOBJ|BUMPNEST [FROM plate] nest field [delta]
   * DELPOBJ|DROPNEST [FROM plate] nest field
   * Nested scalar field plane — peel/set/inc/del inside a nested object without
   * GETOBJ+GETP/SETP+SETOBJ glue. Creates nest {} if missing on set/inc.
   * Soft miss on get (OR fallback). Multi-plate FROM write-back.
   * Usability for agent nested counters/config:
   *   GETPOBJ "meta" "role"
   *   SETPOBJ "meta" "role" "leader"
   *   INCOBJ "stats" "hits"
   *   DELPOBJ FROM PEER "cfg" "tmp"
   */
  return 0;
}
