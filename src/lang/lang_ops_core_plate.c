/* CubalC lang — lang_ops_core_plate.c
 * Plate mut/get plane: SETP GETP INCP DECP DELP GETPN MERGEP (A_core peel).
 * Mesh vote A_core_forms_agent_peel · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_core_plate.h"
#include "lang/cubalc_lang_ops_sys_plate.h"
#include "lang/cubalc_lang_ops_sys_plate_expand.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_core_try_plate(VM *vm, Lex *L) {
  /* Plate mut/get plane.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
  if (kw(&L->cur,"SETP") || kw(&L->cur,"PUTP") || kw(&L->cur,"PLATE_SET") ||
      kw(&L->cur,"MSETP") || kw(&L->cur,"SETPLATEV") ||
      kw(&L->cur,"INCP") || kw(&L->cur,"BUMPP") || kw(&L->cur,"PLATE_INC") ||
      kw(&L->cur,"MINCP") || kw(&L->cur,"INCPLATEV") ||
      kw(&L->cur,"DECP") || kw(&L->cur,"PLATE_DEC") || kw(&L->cur,"MDECP") ||
      kw(&L->cur,"DELP") || kw(&L->cur,"DROPP") || kw(&L->cur,"PLATE_DEL") ||
      kw(&L->cur,"MDELP") || kw(&L->cur,"DELPLATEV") ||
      kw(&L->cur,"GETP") || kw(&L->cur,"PEEKP") || kw(&L->cur,"PLATE_GET") ||
      kw(&L->cur,"MGETP") || kw(&L->cur,"GETPLATEV")) {
    char plate[CUBALC_HOST_STR_MAX], key[96], val[CUBALC_HOST_STR_MAX];
    char fb[CUBALC_HOST_STR_MAX], raw[32], from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr, gr;
    int is_set = 0, is_inc = 0, is_dec = 0, is_del = 0, is_get = 0;
    int val_kind = 0, have_fb = 0, have_from = 0;
    long delta = 1, cur = 0, nv = 0;
    Var *pv;

    if (kw(&L->cur,"SETP") || kw(&L->cur,"PUTP") || kw(&L->cur,"PLATE_SET") ||
        kw(&L->cur,"MSETP") || kw(&L->cur,"SETPLATEV"))
      is_set = 1;
    else if (kw(&L->cur,"DECP") || kw(&L->cur,"PLATE_DEC") || kw(&L->cur,"MDECP")) {
      is_inc = 1;
      is_dec = 1;
      delta = -1;
    } else if (kw(&L->cur,"INCP") || kw(&L->cur,"BUMPP") || kw(&L->cur,"PLATE_INC") ||
               kw(&L->cur,"MINCP") || kw(&L->cur,"INCPLATEV"))
      is_inc = 1;
    else if (kw(&L->cur,"DELP") || kw(&L->cur,"DROPP") || kw(&L->cur,"PLATE_DEL") ||
             kw(&L->cur,"MDELP") || kw(&L->cur,"DELPLATEV"))
      is_del = 1;
    else
      is_get = 1;

    lex_next(L);
    plate[0] = 0; key[0] = 0; val[0] = 0; fb[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

    /* optional leading FROM plate_src (named var write-back, LAST, or raw object) */
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

    /* key */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(key, sizeof key, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      /* do not swallow trailing/leading FROM as key */
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
        fail(vm, is_set ? "SETP [FROM plate] key value — need key"
             : is_inc ? (is_dec ? "DECP [FROM plate] key [delta]" : "INCP [FROM plate] key [delta]")
             : is_del ? "DELP [FROM plate] key" : "GETP [FROM plate] key [OR fallback]");
        return -1;
      }
      if (resolve_str_arg(vm, L, key, sizeof key) != 0)
        key[0] = 0;
    } else {
      fail(vm, is_set ? "SETP [FROM plate] key value — need key"
           : is_inc ? (is_dec ? "DECP [FROM plate] key [delta]" : "INCP [FROM plate] key [delta]")
           : is_del ? "DELP [FROM plate] key" : "GETP [FROM plate] key [OR fallback]");
      return -1;
    }
    if (!key[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "SETP/INCP/DELP/GETP: empty key");
      var_set_str(vm, "ERR", "SETP/INCP/DELP/GETP: empty key");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "SETP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "GETP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "INCP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "DELP_FROM", have_from ? 1 : 0);
      bump(vm); return 1;
    }

    /* op-specific args before optional trailing FROM */
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
          fail(vm, "GETP key OR fallback — need fallback");
          return -1;
        } else if (resolve_str_arg(vm, L, fb, sizeof fb) == 0) {
          have_fb = 1;
        } else {
          fail(vm, "GETP key OR fallback — need fallback");
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
        fail(vm, "SETP [FROM plate] key value — need value");
        return -1;
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
          fail(vm, "SETP [FROM plate] key value — need value");
          return -1;
        }
        /* Auto-raw JSON-shaped strings (match CLI plate set / agent plate seed):
         * SETP "flags" "{\"debug\":0}" · SETP "n" "3" · true/false/null.
         * Explicit string that looks like JSON still raw; use STR if need quote. */
        if (!val_kind) {
          const char *p = val;
          size_t i;
          int allnum = 1;
          while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
          if (*p == '{' || *p == '[')
            val_kind = 1;
          else if (!strcmp(p, "true") || !strcmp(p, "false") || !strcmp(p, "null"))
            val_kind = 1;
          else {
            i = 0;
            if (p[0] == '-' && p[1]) i = 1;
            if (!p[i]) allnum = 0;
            for (; p[i]; i++) {
              if (p[i] < '0' || p[i] > '9') {
                allnum = 0;
                break;
              }
            }
            if (allnum) val_kind = 1;
          }
        }
      } else {
        fail(vm, "SETP [FROM plate] key value — need value");
        return -1;
      }
    } else if (is_inc) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        delta = parse_expr(vm, L);
        if (is_dec && delta > 0) delta = -delta; /* DECP 3 means -3 if positive given */
      }
    }

    /* trailing FROM plate_src if not already */
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

    /* resolve working plate: FROM source or conventional PLATE */
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

    if (is_get) {
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
        var_set_str(vm, "LAST", gr.str);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", gr.str);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_str(vm, "GETP", gr.str);
        var_set_str(vm, "PEEKP", gr.str);
        var_set_num(vm, "GETP_HIT", 1);
        var_set_num(vm, "JSON_HIT", 1);
        var_set_num(vm, "PATH_DEPTH", (long)gr.code > 0 ? (long)gr.code : 1L);
        var_set_num(vm, "OK", 1);
      } else if (have_fb) {
        var_set_str(vm, "LAST", fb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "GETP", fb);
        var_set_num(vm, "GETP_HIT", 0);
        var_set_num(vm, "GETP_OR", 1);
        var_set_num(vm, "OK", 1);
      } else {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "GETP_HIT", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", gr.err[0] ? gr.err : "GETP: key miss");
        var_set_str(vm, "ERR", gr.err[0] ? gr.err : "GETP: key miss");
      }
      var_set_str(vm, "GETP_KEY", key);
      var_set_num(vm, "GETP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "SETP_FROM", have_from ? 1 : 0);
      if (vm->trace)
        fprintf(vm->trace, "# getp key=%s from=%d\n", key, have_from);
      bump(vm); return 1;
    }

    if (is_set) {
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_path_set(plate, key, val, val_kind, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SETP_N", 0);
        var_set_num(vm, "SETP_FROM", have_from ? 1 : 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "SETP: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "SETP: fail");
        bump(vm); return 1;
      }
      if (have_from && from_name[0])
        var_set_str(vm, from_name, hr.str);
      else if (!have_from)
        var_set_str(vm, "PLATE", hr.str);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "SETP", hr.str);
      var_set_num(vm, "SETP_N", hr.n);
      var_set_str(vm, "SETP_KEY", key);
      var_set_num(vm, "SETP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "PATH_DEPTH", (long)hr.code > 0 ? (long)hr.code : 1L);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# setp key=%s updated=%ld from=%d\n", key, hr.n, have_from);
      bump(vm); return 1;
    }

    if (is_inc) {
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
        char *end = NULL;
        cur = strtol(gr.str, &end, 10);
        if (end == gr.str) cur = 0;
      } else {
        cur = 0;
      }
      nv = cur + delta;
      snprintf(raw, sizeof raw, "%ld", nv);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_path_set(plate, key, raw, 1, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "INCP_N", 0);
        var_set_num(vm, "INCP_FROM", have_from ? 1 : 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "INCP: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "INCP: fail");
        bump(vm); return 1;
      }
      if (have_from && from_name[0])
        var_set_str(vm, from_name, hr.str);
      else if (!have_from)
        var_set_str(vm, "PLATE", hr.str);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = nv;
      var_set_num(vm, "LAST_N", nv);
      var_set_str(vm, "INCP", hr.str);
      var_set_num(vm, "INCP_V", nv);
      var_set_num(vm, "INCP_N", 1);
      var_set_num(vm, "INCP_DELTA", delta);
      var_set_str(vm, "INCP_KEY", key);
      var_set_num(vm, "INCP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "SETP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "PATH_DEPTH", (long)hr.code > 0 ? (long)hr.code : 1L);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# incp key=%s v=%ld delta=%ld from=%d\n",
                key, nv, delta, have_from);
      bump(vm); return 1;
    }

    if (is_del) {
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_path_del(plate, key, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DELP_N", 0);
        var_set_num(vm, "DELP_FROM", have_from ? 1 : 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "DELP: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "DELP: fail");
        bump(vm); return 1;
      }
      if (have_from && from_name[0])
        var_set_str(vm, from_name, hr.str);
      else if (!have_from)
        var_set_str(vm, "PLATE", hr.str);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "DELP", hr.str);
      var_set_num(vm, "DELP_N", hr.n);
      var_set_num(vm, "DELP_HIT", hr.n);
      var_set_str(vm, "DELP_KEY", key);
      var_set_num(vm, "DELP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "SETP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "PATH_DEPTH", (long)hr.code > 0 ? (long)hr.code : 1L);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# delp key=%s removed=%ld from=%d\n",
                key, hr.n, have_from);
      bump(vm); return 1;
    }

    fail(vm, "SETP|INCP|DELP|GETP internal");
    return -1;
  }
  /* GETPN|NUMP|GETP_N|JGETN_P [FROM plate] key [OR n]
   * Peel numeric field → LAST_N=value · LAST=decimal · GETPN_N=1|0 hit.
   * Paths ok (same as GETP). Soft miss + OR → fallback value · OK=1.
   * Hard miss without OR → OK=0 sticky LAST_ERR.
   * Usability: nest/arith without GETP+SYS NUM / SYS JSONN glue:
   *   GETPN "cfg.port"          # LAST_N=8080
   *   GETPN "freq.error" OR 0
   *   GETPN FROM PEER "meta.n"
   * Twin of SYS JSONN for conventional PLATE multi-plate plane.
   */
  if (kw(&L->cur,"GETPN") || kw(&L->cur,"NUMP") || kw(&L->cur,"GETP_N") ||
      kw(&L->cur,"MGETPN") || kw(&L->cur,"PLATE_GETN") || kw(&L->cur,"PEEKPN") ||
      kw(&L->cur,"GETINTP") || kw(&L->cur,"INTP") || kw(&L->cur,"JGETN_P")) {
    char plate[CUBALC_HOST_STR_MAX], key[192], fb[64];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result gr;
    int have_fb = 0, have_from = 0;
    long fb_n = 0, nval = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; key[0] = 0; fb[0] = 0;
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
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
          kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE")) {
        fail(vm, "GETPN [FROM plate] key [OR n] — need key");
        return -1;
      }
      if (resolve_str_arg(vm, L, key, sizeof key) != 0)
        key[0] = 0;
    } else {
      fail(vm, "GETPN [FROM plate] key [OR n] — need key");
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

    if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
        kw(&L->cur,"SOFT")) {
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        fb_n = parse_expr(vm, L);
        snprintf(fb, sizeof fb, "%ld", fb_n);
        have_fb = 1;
      } else if (resolve_str_arg(vm, L, fb, sizeof fb) == 0) {
        fb_n = strtol(fb, NULL, 10);
        have_fb = 1;
      } else {
        fail(vm, "GETPN [FROM plate] key OR n — need fallback");
        return -1;
      }
    }

    if (!key[0]) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "GETPN: empty key");
      var_set_str(vm, "ERR", "GETPN: empty key");
      var_set_num(vm, "GETPN_N", 0);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "GETPN_FROM", have_from ? 1 : 0);
      bump(vm); return 1;
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

    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
      char *end = NULL;
      nval = strtol(gr.str, &end, 10);
      if (end == gr.str) nval = 0;
      var_set_str(vm, "LAST", gr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", gr.str);
      vm->last_n = nval;
      var_set_num(vm, "LAST_N", nval);
      var_set_str(vm, "GETPN", gr.str);
      var_set_num(vm, "GETPN_V", nval);
      var_set_num(vm, "GETPN_N", 1);
      var_set_num(vm, "JSONN_V", nval);
      var_set_num(vm, "JSONN_N", 1);
      var_set_num(vm, "JSON_HIT", 1);
      var_set_num(vm, "GETPN_OR", 0);
      var_set_num(vm, "OK", 1);
    } else if (have_fb) {
      var_set_str(vm, "LAST", fb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
      vm->last_n = fb_n;
      var_set_num(vm, "LAST_N", fb_n);
      var_set_str(vm, "GETPN", fb);
      var_set_num(vm, "GETPN_V", fb_n);
      var_set_num(vm, "GETPN_N", 0);
      var_set_num(vm, "JSONN_V", fb_n);
      var_set_num(vm, "JSONN_N", 0);
      var_set_num(vm, "JSON_HIT", 0);
      var_set_num(vm, "GETPN_OR", 1);
      var_set_num(vm, "OK", 1);
    } else {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "GETPN_V", 0);
      var_set_num(vm, "GETPN_N", 0);
      var_set_num(vm, "JSON_HIT", 0);
      var_set_num(vm, "GETPN_OR", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", gr.err[0] ? gr.err : "GETPN: key miss");
      var_set_str(vm, "ERR", gr.err[0] ? gr.err : "GETPN: key miss");
    }
    var_set_str(vm, "GETPN_KEY", key);
    var_set_num(vm, "GETPN_FROM", have_from ? 1 : 0);
    var_set_str(vm, "JSON_KEY", key);
    var_set_num(vm, "JSON_FROM", have_from ? 1 : 0);
    if (vm->trace)
      fprintf(vm->trace, "# getpn key=%s v=%ld hit=%ld from=%d\n",
              key, vm->last_n, (long)(have_fb ? 0 : (vm->last_str[0] ? 1 : 0)),
              have_from);
    bump(vm); return 1;
  }
  /* MERGEP|PATCHP [FROM plate] overlay — multi-key overlay into PLATE or named plate.
   * DEFAULTP|ENSUREP [FROM plate] key value — set key only if missing (no clobber).
   * TOGGLEP [FROM plate] key — flip 0↔1 flag (miss → 1).
   * FROM: multi-plate twin of SETP FROM / FILLP FROM:
   *   MERGEP FROM peer LAST
   *   DEFAULTP "retries" 3 FROM session
   *   TOGGLEP FROM peer "debug"
   * Named FROM write-back updates that var; conventional PLATE stays untouched.
   * MERGEP_FROM/DEFAULTP_FROM/TOGGLEP_FROM = 0|1.
   * MERGEP LAST_N = keys applied · DEFAULTP LAST_N = 1 inserted|0 kept · TOGGLEP LAST_N = new 0|1.
   * Missing default PLATE → {}. */
  if (kw(&L->cur,"MERGEP") || kw(&L->cur,"PATCHP") || kw(&L->cur,"PLATE_MERGE") ||
      kw(&L->cur,"MMERGEP") || kw(&L->cur,"OVERLAYP") || kw(&L->cur,"APPLY_P") ||
      kw(&L->cur,"DEFAULTP") || kw(&L->cur,"ENSUREP") || kw(&L->cur,"PLATE_DEFAULT") ||
      kw(&L->cur,"MDEFAULTP") || kw(&L->cur,"DEFAULTPLATEV") ||
      kw(&L->cur,"TOGGLEP") || kw(&L->cur,"FLIPP") || kw(&L->cur,"PLATE_TOGGLE") ||
      kw(&L->cur,"MTOGGLEP") || kw(&L->cur,"TOGGLEPLATEV")) {
    char plate[CUBALC_HOST_STR_MAX], over[CUBALC_HOST_STR_MAX];
    char key[96], val[CUBALC_HOST_STR_MAX], raw[8];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr, gr;
    int is_merge = 0, is_def = 0, is_tog = 0;
    int val_kind = 0, present = 0, have_from = 0;
    long cur = 0, nv = 0;
    Var *pv;

    if (kw(&L->cur,"MERGEP") || kw(&L->cur,"PATCHP") || kw(&L->cur,"PLATE_MERGE") ||
        kw(&L->cur,"MMERGEP") || kw(&L->cur,"OVERLAYP") || kw(&L->cur,"APPLY_P"))
      is_merge = 1;
    else if (kw(&L->cur,"DEFAULTP") || kw(&L->cur,"ENSUREP") || kw(&L->cur,"PLATE_DEFAULT") ||
             kw(&L->cur,"MDEFAULTP") || kw(&L->cur,"DEFAULTPLATEV"))
      is_def = 1;
    else
      is_tog = 1;

    lex_next(L);
    plate[0] = 0; over[0] = 0; key[0] = 0; val[0] = 0;
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

    /* ENSUREP INTO path is disk seed — not DEFAULTP key-value (use SEEDP) */
    if (is_def && (kw(&L->cur,"INTO") || kw(&L->cur,"AS") || kw(&L->cur,"TOVAR"))) {
      fail(vm, "ENSUREP INTO is disk seed — use SEEDP|BOOTP INTO name path (ENSUREP = DEFAULTP key)");
      return -1;
    }

    if (is_merge) {
      /* overlay: bare → LAST; else str/ident (not trailing FROM) */
      if (L->cur.kind == TK_NL || L->cur.kind == TK_EOF ||
          (L->cur.kind == TK_IDENT &&
           (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
            kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
            kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
            kw(&L->cur,"SYS") || kw(&L->cur,"IF") || kw(&L->cur,"END") ||
            kw(&L->cur,"INCLUDE") || kw(&L->cur,"SETP") || kw(&L->cur,"INCP") ||
            kw(&L->cur,"DELP") || kw(&L->cur,"GETP") || kw(&L->cur,"DEFAULTP") ||
            kw(&L->cur,"ENSUREP") || kw(&L->cur,"TOGGLEP") || kw(&L->cur,"MERGEP")))) {
        snprintf(over, sizeof over, "%s", vm->last_str);
      } else if (resolve_str_arg(vm, L, over, sizeof over) != 0) {
        fail(vm, "MERGEP [FROM plate] overlay — need object plate (or bare uses LAST)");
        return -1;
      }
      if (!over[0])
        snprintf(over, sizeof over, "%s", "{}");
    } else if (is_def) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(key, sizeof key, "%ld", kv);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
            kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
          fail(vm, "DEFAULTP [FROM plate] key value — need key");
          return -1;
        }
        if (resolve_str_arg(vm, L, key, sizeof key) != 0)
          key[0] = 0;
      } else {
        fail(vm, "DEFAULTP [FROM plate] key value — need key");
        return -1;
      }
      if (!key[0]) {
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "DEFAULTP: empty key");
        var_set_str(vm, "ERR", "DEFAULTP: empty key");
        var_set_num(vm, "DEFAULTP_N", 0);
        var_set_num(vm, "DEFAULTP_FROM", have_from ? 1 : 0);
        var_set_num(vm, "LAST_N", 0);
        bump(vm); return 1;
      }
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
        fail(vm, "DEFAULTP [FROM plate] key value — need value");
        return -1;
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
          fail(vm, "DEFAULTP [FROM plate] key value — need value");
          return -1;
        }
        /* Auto-raw JSON-shaped strings (same as SETP / CLI plate set). */
        if (!val_kind) {
          const char *p = val;
          size_t i;
          int allnum = 1;
          while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
          if (*p == '{' || *p == '[')
            val_kind = 1;
          else if (!strcmp(p, "true") || !strcmp(p, "false") || !strcmp(p, "null"))
            val_kind = 1;
          else {
            i = 0;
            if (p[0] == '-' && p[1]) i = 1;
            if (!p[i]) allnum = 0;
            for (; p[i]; i++) {
              if (p[i] < '0' || p[i] > '9') {
                allnum = 0;
                break;
              }
            }
            if (allnum) val_kind = 1;
          }
        }
      } else {
        fail(vm, "DEFAULTP [FROM plate] key value — need value");
        return -1;
      }
    } else {
      /* TOGGLEP key */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(key, sizeof key, "%ld", kv);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
            kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
          fail(vm, "TOGGLEP [FROM plate] key — need key");
          return -1;
        }
        if (resolve_str_arg(vm, L, key, sizeof key) != 0)
          key[0] = 0;
      } else {
        fail(vm, "TOGGLEP [FROM plate] key — need key");
        return -1;
      }
      if (!key[0]) {
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "TOGGLEP: empty key");
        var_set_str(vm, "ERR", "TOGGLEP: empty key");
        var_set_num(vm, "TOGGLEP_N", 0);
        var_set_num(vm, "TOGGLEP_FROM", have_from ? 1 : 0);
        var_set_num(vm, "LAST_N", 0);
        bump(vm); return 1;
      }
    }

    /* trailing FROM plate_src */
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

    /* resolve working plate */
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

    if (is_merge) {
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_merge(plate, over, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "MERGEP_N", 0);
        var_set_num(vm, "MERGEP_FROM", have_from ? 1 : 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "MERGEP: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "MERGEP: fail");
        bump(vm); return 1;
      }
      if (have_from && from_name[0])
        var_set_str(vm, from_name, hr.str);
      else if (!have_from)
        var_set_str(vm, "PLATE", hr.str);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "MERGEP", hr.str);
      var_set_str(vm, "PATCHP", hr.str);
      var_set_num(vm, "MERGEP_N", hr.n);
      var_set_num(vm, "PATCHP_N", hr.n);
      var_set_num(vm, "MERGEP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# mergep applied=%ld from=%d\n", hr.n, have_from);
      bump(vm); return 1;
    }

    if (is_def) {
      memset(&gr, 0, sizeof gr);
      present = (cubalc_host_json_path_has(plate, key, &gr) == 0 && gr.n) ? 1 : 0;
      if (present) {
        /* keep — no clobber; still write-back named plate as-is for consistency */
        if (have_from && from_name[0])
          var_set_str(vm, from_name, plate);
        else if (!have_from)
          var_set_str(vm, "PLATE", plate);
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DEFAULTP_N", 0);
        var_set_num(vm, "ENSUREP_N", 0);
        var_set_num(vm, "DEFAULTP_KEPT", 1);
        var_set_str(vm, "DEFAULTP_KEY", key);
        var_set_num(vm, "DEFAULTP_FROM", have_from ? 1 : 0);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# defaultp key=%s kept from=%d\n", key, have_from);
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_path_set(plate, key, val, val_kind, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DEFAULTP_N", 0);
        var_set_num(vm, "DEFAULTP_FROM", have_from ? 1 : 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "DEFAULTP: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "DEFAULTP: fail");
        bump(vm); return 1;
      }
      if (have_from && from_name[0])
        var_set_str(vm, from_name, hr.str);
      else if (!have_from)
        var_set_str(vm, "PLATE", hr.str);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "DEFAULTP", hr.str);
      var_set_str(vm, "ENSUREP", hr.str);
      var_set_num(vm, "DEFAULTP_N", 1);
      var_set_num(vm, "ENSUREP_N", 1);
      var_set_num(vm, "DEFAULTP_KEPT", 0);
      var_set_str(vm, "DEFAULTP_KEY", key);
      var_set_num(vm, "DEFAULTP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# defaultp key=%s inserted from=%d\n", key, have_from);
      bump(vm); return 1;
    }

    /* TOGGLEP (dotted path ok) */
    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
      char *end = NULL;
      if (!gr.str[0]) cur = 0;
      else {
        cur = strtol(gr.str, &end, 10);
        if (end == gr.str) cur = 1; /* non-numeric non-empty → truthy */
      }
    } else {
      cur = 0;
    }
    nv = cur ? 0 : 1;
    snprintf(raw, sizeof raw, "%ld", nv);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_path_set(plate, key, raw, 1, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TOGGLEP_N", 0);
      var_set_num(vm, "TOGGLEP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "TOGGLEP: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "TOGGLEP: fail");
      bump(vm); return 1;
    }
    if (have_from && from_name[0])
      var_set_str(vm, from_name, hr.str);
    else if (!have_from)
      var_set_str(vm, "PLATE", hr.str);
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = nv;
    var_set_num(vm, "LAST_N", nv);
    var_set_str(vm, "TOGGLEP", hr.str);
    var_set_num(vm, "TOGGLEP_V", nv);
    var_set_num(vm, "TOGGLEP_N", 1);
    var_set_num(vm, "TOGGLEP_PREV", cur ? 1 : 0);
    var_set_str(vm, "TOGGLEP_KEY", key);
    var_set_num(vm, "TOGGLEP_FROM", have_from ? 1 : 0);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# togglep key=%s %ld→%ld from=%d\n",
              key, cur ? 1L : 0L, nv, have_from);
    bump(vm); return 1;
  }
  return 0;
}
