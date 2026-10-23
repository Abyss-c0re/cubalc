/* CubalC lang — lang_ops_sys_json.c
 * SYS JSON* plate peel/mutate theme (A2 peel from core_sys).
 * Mesh vote A2_theme_peel_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_json.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int cubalc_lang_ops_sys_try_json(VM *vm, Lex *L) {
  /* SYS JSON family. Returns 1 handled, 0 not this plane, -1 fail.
   * Caller already consumed SYS|HOST.
   */
    /* SYS JSON|JGET "key" [FROM|IN|OF src] [OR fallback]
     * — peel field from explicit plate or LAST. Miss without OR → soft OK=0.
     * OR → LAST=fallback OK=1 (like ENV OR). FROM avoids reloading LAST each peel.
     * Usability: multi-field agent plate peel without JOIN "" plate glue. */
    if (kw(&L->cur,"JSON") || kw(&L->cur,"JGET")){
      char key[96]="content";
      char fallback[CUBALC_HOST_STR_MAX];
      char srcbuf[CUBALC_HOST_STR_MAX];
      int have_fb = 0, have_src = 0;
      const char *src;
      cubalc_host_result hr;
      lex_next(L);
      fallback[0] = 0;
      srcbuf[0] = 0;
      if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
        snprintf(key,sizeof key,"%s",L->cur.text); lex_next(L);
      }
      /* optional FROM|IN|OF source (before OR so "OR" is never a source name) */
      if (kw(&L->cur,"FROM") || kw(&L->cur,"IN") || kw(&L->cur,"OF") ||
          kw(&L->cur,"SRC") || kw(&L->cur,"PLATE")){
        lex_next(L);
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(srcbuf, sizeof srcbuf, "%ld", nv);
          have_src = 1;
        } else if (resolve_str_arg(vm, L, srcbuf, sizeof srcbuf) == 0) {
          have_src = 1;
        } else {
          fail(vm, "SYS JSON key FROM plate [OR fallback]");
          return -1;
        }
      }
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"SOFT")){
        lex_next(L);
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(fallback, sizeof fallback, "%ld", nv);
          have_fb = 1;
        } else if (resolve_str_arg(vm, L, fallback, sizeof fallback) == 0) {
          have_fb = 1;
        } else {
          fail(vm, "SYS JSON key [FROM plate] OR \"fallback\"");
          return -1;
        }
      }
      src = have_src ? srcbuf : vm->last_str;
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_get(src, key, &hr)!=0){
        if (have_fb) {
          snprintf(vm->last_str, sizeof vm->last_str, "%s", fallback);
          vm->last_n = (long)strlen(fallback);
          var_set_str(vm, "LAST", fallback);
          var_set_num(vm, "LAST_N", vm->last_n);
          var_set_str(vm, "JSON", fallback);
          var_set_num(vm, "JSON_HIT", 0);
          var_set_num(vm, "JSON_OR", 1);
          var_set_num(vm, "JSON_FROM", have_src ? 1 : 0);
          var_set_str(vm, "JSON_KEY", key);
          var_set_num(vm, "OK", 1);
          bump(vm); return 1;
        }
        var_set_str(vm,"LAST","");
        vm->last_str[0] = 0;
        var_set_num(vm,"LAST_N",0);
        vm->last_n = 0;
        var_set_num(vm, "JSON_HIT", 0);
        var_set_num(vm, "JSON_OR", 0);
        var_set_num(vm, "JSON_FROM", have_src ? 1 : 0);
        var_set_str(vm, "JSON_KEY", key);
        var_set_num(vm,"OK",0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        }
        bump(vm); return 1;
      }
      snprintf(vm->last_str,sizeof vm->last_str,"%s",hr.str);
      vm->last_n = hr.n;
      var_set_str(vm,"LAST",hr.str);
      var_set_num(vm,"LAST_N",hr.n);
      var_set_str(vm, "JSON", hr.str);
      var_set_num(vm, "JSON_HIT", 1);
      var_set_num(vm, "JSON_OR", 0);
      var_set_num(vm, "JSON_FROM", have_src ? 1 : 0);
      var_set_str(vm, "JSON_KEY", key);
      var_set_num(vm,"OK",1);
      if (vm->trace)
        fprintf(vm->trace, "# sys json → key=%s hit=1 from=%d\n", key, have_src);
      bump(vm); return 1;
    }
    /* SYS JSONN|JGETN|JSONNUM|JSON_N "key" [FROM src] [OR n]
     * — peel field as int → LAST_N (and LAST=decimal). Twin of LOOKUPN for plates.
     * true→1 false→0; OR on miss. Paths ok: JSONN "cfg.port" FROM PLATE.
     * Usability: IF/arith without JSON+NUM glue · nest without GETPOBJ. */
    if (kw(&L->cur,"JSONN") || kw(&L->cur,"JGETN") || kw(&L->cur,"JSONNUM") ||
        kw(&L->cur,"JSON_N") || kw(&L->cur,"JSONINT") || kw(&L->cur,"JGETINT") ||
        kw(&L->cur,"PLATEN") || kw(&L->cur,"JSONGETN")){
      char key[192]="n";
      char fallback[64];
      char srcbuf[CUBALC_HOST_STR_MAX];
      int have_fb = 0, have_src = 0;
      long fb_n = 0, nval = 0;
      const char *src;
      cubalc_host_result hr;
      lex_next(L);
      fallback[0] = 0;
      srcbuf[0] = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(key, sizeof key, "%ld", kv);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (kw(&L->cur,"FROM") || kw(&L->cur,"IN") || kw(&L->cur,"OF") ||
            kw(&L->cur,"SRC") || kw(&L->cur,"PLATE") ||
            kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE")) {
          /* bare key default "n" */
        } else if (resolve_str_arg(vm, L, key, sizeof key) != 0) {
          snprintf(key, sizeof key, "%s", "n");
        }
      }
      if (kw(&L->cur,"FROM") || kw(&L->cur,"IN") || kw(&L->cur,"OF") ||
          kw(&L->cur,"SRC") || kw(&L->cur,"PLATE")){
        lex_next(L);
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(srcbuf, sizeof srcbuf, "%ld", nv);
          have_src = 1;
        } else if (resolve_str_arg(vm, L, srcbuf, sizeof srcbuf) == 0) {
          have_src = 1;
        } else {
          fail(vm, "SYS JSONN key FROM plate [OR n]");
          return -1;
        }
      }
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"SOFT")){
        lex_next(L);
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          fb_n = parse_expr(vm, L);
          snprintf(fallback, sizeof fallback, "%ld", fb_n);
          have_fb = 1;
        } else if (resolve_str_arg(vm, L, fallback, sizeof fallback) == 0) {
          fb_n = strtol(fallback, NULL, 10);
          have_fb = 1;
        } else {
          fail(vm, "SYS JSONN key [FROM plate] OR n");
          return -1;
        }
      }
      src = have_src ? srcbuf : vm->last_str;
      memset(&hr, 0, sizeof hr);
      /* path-aware: dotted/slash keys via path_get (shallow falls through) */
      if (cubalc_host_json_path_get(src, key, &hr) != 0) {
        if (have_fb) {
          snprintf(vm->last_str, sizeof vm->last_str, "%s", fallback);
          vm->last_n = fb_n;
          var_set_str(vm, "LAST", fallback);
          var_set_num(vm, "LAST_N", fb_n);
          var_set_num(vm, "JSONN_V", fb_n);
          var_set_num(vm, "JSONN_N", 0);
          var_set_num(vm, "JSON_HIT", 0);
          var_set_num(vm, "JSON_OR", 1);
          var_set_num(vm, "JSON_FROM", have_src ? 1 : 0);
          var_set_str(vm, "JSON_KEY", key);
          var_set_num(vm, "OK", 1);
          bump(vm); return 1;
        }
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONN_V", 0);
        var_set_num(vm, "JSONN_N", 0);
        var_set_num(vm, "JSON_HIT", 0);
        var_set_num(vm, "JSON_OR", 0);
        var_set_num(vm, "JSON_FROM", have_src ? 1 : 0);
        var_set_str(vm, "JSON_KEY", key);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        }
        bump(vm); return 1;
      }
      /* parse number; true/false already 1/0 from host_json_get/path_get */
      {
        char *end = NULL;
        nval = strtol(hr.str, &end, 10);
        if (end == hr.str) nval = 0;
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = nval;
      var_set_str(vm, "LAST", hr.str);
      var_set_num(vm, "LAST_N", nval);
      var_set_str(vm, "JSONN", hr.str);
      var_set_num(vm, "JSONN_V", nval);
      var_set_num(vm, "JSONN_N", 1);
      var_set_num(vm, "JGETN_V", nval);
      var_set_num(vm, "JSON_HIT", 1);
      var_set_num(vm, "JSON_OR", 0);
      var_set_num(vm, "JSON_FROM", have_src ? 1 : 0);
      var_set_str(vm, "JSON_KEY", key);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonn → key=%s v=%ld from=%d\n", key, nval, have_src);
      bump(vm); return 1;
    }
    /* SYS JSONKEYS|KEYSJSON|JKEYS [json|LAST]
     * — top-level object keys → newline bag · LAST_N=count.
     * Usability: walk unknown agent plates without guessing field names. */
    if (kw(&L->cur,"JSONKEYS") || kw(&L->cur,"KEYSJSON") || kw(&L->cur,"JKEYS") ||
        kw(&L->cur,"JSON_KEYS") || kw(&L->cur,"OBJECTKEYS") || kw(&L->cur,"PLATEKEYS")){
      char src[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      lex_next(L);
      src[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, src, sizeof src) != 0)
          src[0] = 0;
      }
      if (!src[0])
        snprintf(src, sizeof src, "%s", vm->last_str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_keys(src, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONKEYS_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONKEYS: fail");
          var_set_str(vm, "ERR", "JSONKEYS: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "JSONKEYS", hr.str);
      var_set_str(vm, "KEYSJSON", hr.str);
      var_set_num(vm, "JSONKEYS_N", hr.n);
      var_set_num(vm, "KEYSJSON_N", hr.n);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonkeys → n=%ld\n", hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONHAS|HASJSON|JHAS key [json|LAST]
     * — soft 0|1 if top-level (or any) key exists · LAST string kept (probe).
     * Usability: IF before JSON peel without OK-clobbering glue. */
    if (kw(&L->cur,"JSONHAS") || kw(&L->cur,"HASJSON") || kw(&L->cur,"JHAS") ||
        kw(&L->cur,"HASJKEY") || kw(&L->cur,"JSONKEYHAS") || kw(&L->cur,"PLATEHAS")){
      char key[96], src[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      long hit;
      lex_next(L);
      key[0] = 0; src[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        snprintf(key, sizeof key, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(key, sizeof key, "%ld", nv);
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, src, sizeof src) != 0)
          src[0] = 0;
      }
      if (!src[0])
        snprintf(src, sizeof src, "%s", vm->last_str);
      if (!key[0]) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONHAS_N", 0);
        var_set_num(vm, "JSONHAS_HIT", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "JSONHAS: empty key");
        var_set_str(vm, "ERR", "JSONHAS: empty key");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      hit = (cubalc_host_json_get(src, key, &hr) == 0) ? 1 : 0;
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "JSONHAS_N", hit);
      var_set_num(vm, "JSONHAS_HIT", hit);
      var_set_num(vm, "HASJSON_N", hit);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonhas → hit=%ld key=%s\n", hit, key);
      bump(vm); return 1;
    }
    /* SYS JSONSET|JSET|JSONPUT [plate] key value
     * — set/update top-level key → LAST = new object plate.
     * 3-arg: plate key value · 2-arg: key value (plate = prior LAST).
     * Numbers and bare true/false/null → raw JSON; strings quoted.
     * Optional RAW before value forces raw. Empty/non-object → fresh {}.
     * Usability: agent plate write without hand string rebuild. */
    if (kw(&L->cur,"JSONSET") || kw(&L->cur,"JSET") || kw(&L->cur,"JSONPUT") ||
        kw(&L->cur,"SETJSON") || kw(&L->cur,"PUTJSON") || kw(&L->cur,"JSONUPDATE")){
      char plate[CUBALC_HOST_STR_MAX], key[96], val[CUBALC_HOST_STR_MAX];
      char args[3][CUBALC_HOST_STR_MAX];
      int kinds[3]; /* 0=str, 1=num/raw-literal */
      int narg = 0, val_kind = 0, force_raw = 0, i;
      cubalc_host_result hr;
      lex_next(L);
      plate[0] = 0; key[0] = 0; val[0] = 0;
      for (i = 0; i < 3; i++) {
        args[i][0] = 0;
        kinds[i] = 0;
      }
      while (narg < 3) {
        if (kw(&L->cur,"RAW") || kw(&L->cur,"ASRAW") || kw(&L->cur,"LITERAL")) {
          force_raw = 1;
          lex_next(L);
          continue;
        }
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(args[narg], sizeof args[narg], "%ld", nv);
          kinds[narg] = 1;
          narg++;
          continue;
        }
        if (L->cur.kind == TK_IDENT) {
          const char *id = L->cur.text;
          if (strcmp(id, "true") == 0 || strcmp(id, "false") == 0 ||
              strcmp(id, "null") == 0) {
            snprintf(args[narg], sizeof args[narg], "%s", id);
            kinds[narg] = 1;
            lex_next(L);
            narg++;
            continue;
          }
        }
        if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (resolve_str_arg(vm, L, args[narg], sizeof args[narg]) != 0)
            break;
          kinds[narg] = 0;
          narg++;
          continue;
        }
        break;
      }
      if (narg == 3) {
        snprintf(plate, sizeof plate, "%s", args[0]);
        snprintf(key, sizeof key, "%s", args[1]);
        snprintf(val, sizeof val, "%s", args[2]);
        val_kind = kinds[2];
      } else if (narg == 2) {
        snprintf(plate, sizeof plate, "%s", vm->last_str);
        snprintf(key, sizeof key, "%s", args[0]);
        snprintf(val, sizeof val, "%s", args[1]);
        val_kind = kinds[1];
      } else {
        fail(vm, "SYS JSONSET [plate] key value — JSONSET plate \"ok\" 1");
        return -1;
      }
      if (force_raw) val_kind = 1;
      if (!key[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONSET_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "JSONSET: empty key");
        var_set_str(vm, "ERR", "JSONSET: empty key");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_set(plate, key, val, val_kind, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONSET_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONSET: fail");
          var_set_str(vm, "ERR", "JSONSET: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = (long)strlen(hr.str);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_str(vm, "JSONSET", hr.str);
      var_set_str(vm, "JSET", hr.str);
      var_set_num(vm, "JSONSET_N", hr.n);
      var_set_num(vm, "JSONSET_UPDATED", hr.n);
      var_set_str(vm, "JSONSET_KEY", key);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonset → key=%s updated=%ld\n", key, hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONOBJ|MKJSON|JOBJ key value [key value ...]
     * — one-shot build top-level object from pairs → LAST = plate.
     * Same value rules as JSONSET (num/true/false/null raw; strings quoted; RAW).
     * Optional leading base object: JSONOBJ FROM plate key val …
     * Usability: agent plates without chained JSONSET glue. */
    if (kw(&L->cur,"JSONOBJ") || kw(&L->cur,"MKJSON") || kw(&L->cur,"JOBJ") ||
        kw(&L->cur,"MAKEJSON") || kw(&L->cur,"NEWJSON") || kw(&L->cur,"JSONMAKE") ||
        kw(&L->cur,"BUILDOBJ") || kw(&L->cur,"PLATEOBJ")){
      char plate[CUBALC_HOST_STR_MAX], key[96], val[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      int pairs = 0, force_raw = 0;
      lex_next(L);
      plate[0] = 0;
      /* optional FROM|OF base plate */
      if (kw(&L->cur,"FROM") || kw(&L->cur,"OF") || kw(&L->cur,"ON") ||
          kw(&L->cur,"BASE") || kw(&L->cur,"INTO")){
        lex_next(L);
        if (resolve_str_arg(vm, L, plate, sizeof plate) != 0)
          plate[0] = 0;
      } else {
        snprintf(plate, sizeof plate, "%s", "{}");
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      while (1) {
        int val_kind = 0;
        force_raw = 0;
        key[0] = 0;
        val[0] = 0;
        if (kw(&L->cur,"RAW") || kw(&L->cur,"ASRAW") || kw(&L->cur,"LITERAL")) {
          force_raw = 1;
          lex_next(L);
        }
        /* key */
        if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (L->cur.kind == TK_IDENT &&
              (kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
               kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
               kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
               kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"JSONSET") ||
               kw(&L->cur,"JSON") || kw(&L->cur,"JSONKEYS") || kw(&L->cur,"JSONHAS") ||
               kw(&L->cur,"JSONOBJ") || kw(&L->cur,"MKJSON"))) {
            break;
          }
          if (resolve_str_arg(vm, L, key, sizeof key) != 0)
            break;
        } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(key, sizeof key, "%ld", nv);
        } else {
          break;
        }
        if (!key[0]) break;
        /* value */
        if (kw(&L->cur,"RAW") || kw(&L->cur,"ASRAW") || kw(&L->cur,"LITERAL")) {
          force_raw = 1;
          lex_next(L);
        }
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(val, sizeof val, "%ld", nv);
          val_kind = 1;
        } else if (L->cur.kind == TK_IDENT &&
                   (strcmp(L->cur.text, "true") == 0 || strcmp(L->cur.text, "false") == 0 ||
                    strcmp(L->cur.text, "null") == 0)) {
          snprintf(val, sizeof val, "%s", L->cur.text);
          val_kind = 1;
          lex_next(L);
        } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
            fail(vm, "SYS JSONOBJ key value [key value ...] — missing value");
            return -1;
          }
          val_kind = 0;
        } else {
          fail(vm, "SYS JSONOBJ key value [key value ...] — missing value");
          return -1;
        }
        if (force_raw) val_kind = 1;
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_json_set(plate, key, val, val_kind, &hr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "JSONOBJ_N", pairs);
          var_set_num(vm, "OK", 0);
          if (hr.err[0]) {
            var_set_str(vm, "LAST_ERR", hr.err);
            var_set_str(vm, "ERR", hr.err);
          } else {
            var_set_str(vm, "LAST_ERR", "JSONOBJ: fail");
            var_set_str(vm, "ERR", "JSONOBJ: fail");
          }
          bump(vm); return 1;
        }
        snprintf(plate, sizeof plate, "%s", hr.str);
        pairs++;
      }
      if (pairs == 0 && (plate[0] == 0 || (plate[0] == '{' && plate[1] == '}'))) {
        /* bare JSONOBJ → empty object still useful */
        snprintf(plate, sizeof plate, "%s", "{}");
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = pairs;
      var_set_num(vm, "LAST_N", pairs);
      var_set_str(vm, "JSONOBJ", plate);
      var_set_str(vm, "MKJSON", plate);
      var_set_num(vm, "JSONOBJ_N", pairs);
      var_set_num(vm, "MKJSON_N", pairs);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonobj → pairs=%d\n", pairs);
      bump(vm); return 1;
    }
    /* SYS JSONDEL|JDEL|JSONRM [plate] key
     * — drop top-level key → LAST = new plate. Soft miss (key absent) OK=1 JSONDEL_N=0.
     * 2-arg: plate key · 1-arg: key (plate = prior LAST).
     * Usability: plate field ack/drop without hand string rebuild. */
    /* JSONDROP/DROPJSON reclaimed as multi-key bulk drop (see below). */
    if (kw(&L->cur,"JSONDEL") || kw(&L->cur,"JDEL") || kw(&L->cur,"JSONRM") ||
        kw(&L->cur,"DELJSON") || kw(&L->cur,"RMJSON") || kw(&L->cur,"UNSETJSON")){
      char plate[CUBALC_HOST_STR_MAX], key[96];
      char a[CUBALC_HOST_STR_MAX], b[96];
      cubalc_host_result hr;
      int narg = 0;
      lex_next(L);
      plate[0] = 0; key[0] = 0; a[0] = 0; b[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, a, sizeof a) == 0)
          narg = 1;
      } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(a, sizeof a, "%ld", nv);
        narg = 1;
      }
      if (narg >= 1 && (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
                        L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                        L->cur.kind == TK_LPAREN)) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(b, sizeof b, "%ld", nv);
          narg = 2;
        } else if (L->cur.kind == TK_STR) {
          /* allow empty key string for soft empty-key error path */
          snprintf(b, sizeof b, "%s", L->cur.text);
          lex_next(L);
          narg = 2;
        } else if (resolve_str_arg(vm, L, b, sizeof b) == 0) {
          narg = 2;
        }
      }
      if (narg == 2) {
        snprintf(plate, sizeof plate, "%s", a);
        snprintf(key, sizeof key, "%s", b);
      } else if (narg == 1) {
        snprintf(plate, sizeof plate, "%s", vm->last_str);
        snprintf(key, sizeof key, "%s", a);
      } else {
        fail(vm, "SYS JSONDEL [plate] key — JSONDEL plate \"tmp\"");
        return -1;
      }
      if (!key[0]) {
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONDEL_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "JSONDEL: empty key");
        var_set_str(vm, "ERR", "JSONDEL: empty key");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_del(plate, key, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONDEL_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONDEL: fail");
          var_set_str(vm, "ERR", "JSONDEL: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "JSONDEL", hr.str);
      var_set_str(vm, "JDEL", hr.str);
      var_set_num(vm, "JSONDEL_N", hr.n);
      var_set_num(vm, "JSONDEL_HIT", hr.n);
      var_set_str(vm, "JSONDEL_KEY", key);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsondel → key=%s removed=%ld\n", key, hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONMERGE|MERGEJSON|JMERGE base overlay
     * — apply overlay top-level keys onto base (overlay wins) → LAST = plate.
     * LAST_N / JSONMERGE_N = keys applied. Raw values preserved.
     * Usability: combine partial agent plates without EACH+JSONSET glue. */
    if (kw(&L->cur,"JSONMERGE") || kw(&L->cur,"MERGEJSON") || kw(&L->cur,"JMERGE") ||
        kw(&L->cur,"JSONCOMBINE") || kw(&L->cur,"COMBINEJSON") ||
        kw(&L->cur,"PLATEMERGE") || kw(&L->cur,"MERGEPLATE")){
      char base[CUBALC_HOST_STR_MAX], over[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      lex_next(L);
      base[0] = 0; over[0] = 0;
      if (resolve_str_arg(vm, L, base, sizeof base) != 0)
        snprintf(base, sizeof base, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, over, sizeof over) != 0) {
        fail(vm, "SYS JSONMERGE base overlay — JSONMERGE base over");
        return -1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_merge(base, over, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONMERGE_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONMERGE: fail");
          var_set_str(vm, "ERR", "JSONMERGE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "JSONMERGE", hr.str);
      var_set_str(vm, "MERGEJSON", hr.str);
      var_set_num(vm, "JSONMERGE_N", hr.n);
      var_set_num(vm, "MERGEJSON_N", hr.n);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonmerge → applied=%ld\n", hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONINC|JINC|JSONDEC [plate] key [delta]
     * — bump numeric top-level key → LAST = new plate · LAST_N = new value.
     * Default delta +1 (JSONINC) or −1 (JSONDEC). Missing key treated as 0.
     * Forms: plate key [delta] | key [delta] (plate=LAST).
     * Usability: retry/count plate counters without JSONN+arith+JSONSET glue. */
    if (kw(&L->cur,"JSONINC") || kw(&L->cur,"JINC") || kw(&L->cur,"INCJSON") ||
        kw(&L->cur,"JSONBUMP") || kw(&L->cur,"BUMPJSON") || kw(&L->cur,"PLATEINC") ||
        kw(&L->cur,"JSONDEC") || kw(&L->cur,"JDEC") || kw(&L->cur,"DECJSON") ||
        kw(&L->cur,"JSONDECR") || kw(&L->cur,"PLATEDEC")){
      char plate[CUBALC_HOST_STR_MAX], key[96], raw[32];
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr, gr;
      int is_dec = 0, narg = 0, have_delta = 0;
      long delta = 1, cur = 0, nv = 0;
      if (kw(&L->cur,"JSONDEC") || kw(&L->cur,"JDEC") || kw(&L->cur,"DECJSON") ||
          kw(&L->cur,"JSONDECR") || kw(&L->cur,"PLATEDEC")) {
        is_dec = 1;
        delta = -1;
      }
      lex_next(L);
      plate[0] = 0; key[0] = 0; a[0] = 0; b[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, a, sizeof a) == 0) narg = 1;
      }
      if (narg >= 1 && (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
        if (resolve_str_arg(vm, L, b, sizeof b) == 0) narg = 2;
      }
      if (narg >= 1 && (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                        L->cur.kind == TK_LPAREN)) {
        delta = parse_expr(vm, L);
        have_delta = 1;
        if (narg == 1) {
          /* key delta on LAST */
          snprintf(plate, sizeof plate, "%s", vm->last_str);
          snprintf(key, sizeof key, "%s", a);
          narg = 2; /* key+delta resolved */
        } else if (narg == 2) {
          /* plate key delta */
          snprintf(plate, sizeof plate, "%s", a);
          snprintf(key, sizeof key, "%s", b);
          narg = 3;
        }
      } else if (narg == 2) {
        /* plate key (no explicit delta) OR key with non-num second as plate key */
        if (a[0] == '{') {
          snprintf(plate, sizeof plate, "%s", a);
          snprintf(key, sizeof key, "%s", b);
        } else {
          /* ambiguous: prefer plate key if a looks like plate, else key on LAST */
          snprintf(plate, sizeof plate, "%s", a);
          snprintf(key, sizeof key, "%s", b);
        }
      } else if (narg == 1) {
        snprintf(plate, sizeof plate, "%s", vm->last_str);
        snprintf(key, sizeof key, "%s", a);
      } else {
        fail(vm, is_dec ? "SYS JSONDEC [plate] key [delta]"
                        : "SYS JSONINC [plate] key [delta]");
        return -1;
      }
      (void)have_delta;
      if (!key[0]) {
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "JSONINC: empty key");
        var_set_str(vm, "ERR", "JSONINC: empty key");
        var_set_num(vm, "JSONINC_N", 0);
        bump(vm); return 1;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_get(plate, key, &gr) == 0) {
        char *end = NULL;
        cur = strtol(gr.str, &end, 10);
        if (end == gr.str) cur = 0;
      } else {
        cur = 0;
      }
      nv = cur + delta;
      snprintf(raw, sizeof raw, "%ld", nv);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_set(plate, key, raw, 1, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONINC_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONINC: fail");
          var_set_str(vm, "ERR", "JSONINC: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = nv;
      var_set_num(vm, "LAST_N", nv);
      var_set_str(vm, "JSONINC", hr.str);
      var_set_str(vm, "JINC", hr.str);
      var_set_num(vm, "JSONINC_V", nv);
      var_set_num(vm, "JSONINC_N", 1);
      var_set_num(vm, "JSONINC_DELTA", delta);
      var_set_str(vm, "JSONINC_KEY", key);
      var_set_num(vm, "JSONDEC_V", nv);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsoninc → key=%s v=%ld delta=%ld\n", key, nv, delta);
      bump(vm); return 1;
    }
    /* SYS JSONENSURE|JDEFAULT|JSONDEFAULT [plate] key value
     * — set top-level key only if missing → LAST = plate · LAST_N = 1 inserted | 0 kept.
     * 3-arg: plate key value · 2-arg: key value (plate = prior LAST).
     * Same value rules as JSONSET (num/true/false/null raw; strings quoted; RAW).
     * Usability: agent plate boot defaults without clobbering live counters/status.
     * Twin of DEFAULT for vars / KVFILEENSURE for plate files. */
    if (kw(&L->cur,"JSONENSURE") || kw(&L->cur,"JDEFAULT") || kw(&L->cur,"JSONDEFAULT") ||
        kw(&L->cur,"ENSUREJSON") || kw(&L->cur,"DEFAULTJSON") || kw(&L->cur,"JENSURE") ||
        kw(&L->cur,"PLATEDEFAULT") || kw(&L->cur,"ENSUREKEY")){
      char plate[CUBALC_HOST_STR_MAX], key[96], val[CUBALC_HOST_STR_MAX];
      char args[3][CUBALC_HOST_STR_MAX];
      int kinds[3];
      int narg = 0, val_kind = 0, force_raw = 0, i, present = 0;
      cubalc_host_result hr, gr;
      lex_next(L);
      plate[0] = 0; key[0] = 0; val[0] = 0;
      for (i = 0; i < 3; i++) {
        args[i][0] = 0;
        kinds[i] = 0;
      }
      while (narg < 3) {
        if (kw(&L->cur,"RAW") || kw(&L->cur,"ASRAW") || kw(&L->cur,"LITERAL")) {
          force_raw = 1;
          lex_next(L);
          continue;
        }
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(args[narg], sizeof args[narg], "%ld", nv);
          kinds[narg] = 1;
          narg++;
          continue;
        }
        if (L->cur.kind == TK_IDENT) {
          const char *id = L->cur.text;
          if (strcmp(id, "true") == 0 || strcmp(id, "false") == 0 ||
              strcmp(id, "null") == 0) {
            snprintf(args[narg], sizeof args[narg], "%s", id);
            kinds[narg] = 1;
            lex_next(L);
            narg++;
            continue;
          }
        }
        if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (resolve_str_arg(vm, L, args[narg], sizeof args[narg]) != 0)
            break;
          kinds[narg] = 0;
          narg++;
          continue;
        }
        break;
      }
      if (narg == 3) {
        snprintf(plate, sizeof plate, "%s", args[0]);
        snprintf(key, sizeof key, "%s", args[1]);
        snprintf(val, sizeof val, "%s", args[2]);
        val_kind = kinds[2];
      } else if (narg == 2) {
        snprintf(plate, sizeof plate, "%s", vm->last_str);
        snprintf(key, sizeof key, "%s", args[0]);
        snprintf(val, sizeof val, "%s", args[1]);
        val_kind = kinds[1];
      } else {
        fail(vm, "SYS JSONENSURE [plate] key value — JSONENSURE plate \"retries\" 0");
        return -1;
      }
      if (force_raw) val_kind = 1;
      if (!key[0]) {
        var_set_str(vm, "LAST", plate[0] ? plate : "");
        if (plate[0]) {
          snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
          vm->last_n = 0;
        } else {
          vm->last_str[0] = 0;
          vm->last_n = 0;
        }
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONENSURE_N", 0);
        var_set_num(vm, "JSONENSURE_SET", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "JSONENSURE: empty key");
        var_set_str(vm, "ERR", "JSONENSURE: empty key");
        bump(vm); return 1;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&gr, 0, sizeof gr);
      present = (cubalc_host_json_get(plate, key, &gr) == 0) ? 1 : 0;
      if (present) {
        /* keep existing value — return plate copy, LAST_N=0 */
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "JSONENSURE", plate);
        var_set_str(vm, "JDEFAULT", plate);
        var_set_num(vm, "JSONENSURE_N", 0);
        var_set_num(vm, "JSONENSURE_SET", 0);
        var_set_num(vm, "JSONENSURE_HAD", 1);
        var_set_str(vm, "JSONENSURE_KEY", key);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# sys jsonensure → key=%s kept (present)\n", key);
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_set(plate, key, val, val_kind, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONENSURE_N", 0);
        var_set_num(vm, "JSONENSURE_SET", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONENSURE: fail");
          var_set_str(vm, "ERR", "JSONENSURE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "JSONENSURE", hr.str);
      var_set_str(vm, "JDEFAULT", hr.str);
      var_set_num(vm, "JSONENSURE_N", 1);
      var_set_num(vm, "JSONENSURE_SET", 1);
      var_set_num(vm, "JSONENSURE_HAD", 0);
      var_set_str(vm, "JSONENSURE_KEY", key);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonensure → key=%s inserted\n", key);
      bump(vm); return 1;
    }
    /* SYS JSONDEFAULTS|JSONENSUREALL|JDEFAULTS plate defaults
     * — fill missing keys on plate from defaults plate (plate wins when present).
     * LAST = result · LAST_N / JSONDEFAULTS_N = keys inserted · JSONDEFAULTS_HAD = skipped.
     * Forms: plate defaults · defaults (plate=LAST) · plate FROM|WITH defaults.
     * Usability: multi-key agent boot without multi JSONENSURE or clobber live fields.
     * Dual of JSONMERGE (overlay wins). Singular JSONDEFAULT remains single-key ENSURE. */
    if (kw(&L->cur,"JSONDEFAULTS") || kw(&L->cur,"JSONENSUREALL") ||
        kw(&L->cur,"JDEFAULTS") || kw(&L->cur,"ENSUREDEFAULTS") ||
        kw(&L->cur,"PLATEDEFAULTS") || kw(&L->cur,"FILLDEFAULTS") ||
        kw(&L->cur,"JSONFILL") || kw(&L->cur,"BOOTDEFAULTS") ||
        kw(&L->cur,"WITHDEFAULTS") || kw(&L->cur,"DEFAULTSPLACE")){
      char plate[CUBALC_HOST_STR_MAX], defs[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      lex_next(L);
      plate[0] = 0;
      defs[0] = 0;
      if (resolve_str_arg(vm, L, plate, sizeof plate) != 0)
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      /* optional FROM|WITH|USING|OF before defaults */
      if (kw(&L->cur,"FROM") || kw(&L->cur,"WITH") || kw(&L->cur,"USING") ||
          kw(&L->cur,"OF") || kw(&L->cur,"DEFAULTS") || kw(&L->cur,"BASE"))
        lex_next(L);
      if (resolve_str_arg(vm, L, defs, sizeof defs) != 0) {
        /* one-arg form: plate=LAST, defaults=first arg already in plate */
        if (plate[0] && !defs[0]) {
          snprintf(defs, sizeof defs, "%s", plate);
          snprintf(plate, sizeof plate, "%s", vm->last_str);
        }
      }
      if (!defs[0]) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONDEFAULTS_N", 0);
        var_set_num(vm, "JSONDEFAULTS_HAD", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "JSONDEFAULTS: need plate and defaults");
        var_set_str(vm, "ERR", "JSONDEFAULTS: need plate and defaults");
        bump(vm); return 1;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_defaults(plate, defs, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONDEFAULTS_N", 0);
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONDEFAULTS: fail");
          var_set_str(vm, "ERR", "JSONDEFAULTS: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "JSONDEFAULTS", hr.str);
      var_set_str(vm, "JSONENSUREALL", hr.str);
      var_set_num(vm, "JSONDEFAULTS_N", hr.n);
      var_set_num(vm, "JSONENSUREALL_N", hr.n);
      var_set_num(vm, "JSONDEFAULTS_HAD", hr.code);
      var_set_num(vm, "JSONDEFAULTS_SET", hr.n);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsondefaults → set=%ld had=%d\n", hr.n, hr.code);
      bump(vm); return 1;
    }
    /* SYS JSONPICK|JSONKEEP|JKEEP|JSELECT [plate] key [key ...]
     * — keep only listed top-level keys → LAST = subset plate · LAST_N = kept.
     * Missing keys soft-skipped. Raw values preserved (nums/bools/nested).
     * Forms: plate k1 k2… · k1 k2… (plate=LAST) · plate OF|KEYS bag.
     * Usability: strip noisy plates before peer/log without EACH+JSONSET rebuild. */
    if (kw(&L->cur,"JSONPICK") || kw(&L->cur,"JSONKEEP") || kw(&L->cur,"JKEEP") ||
        kw(&L->cur,"JSELECT") || kw(&L->cur,"PICKJSON") || kw(&L->cur,"KEEPJSON") ||
        kw(&L->cur,"SELECTJSON") || kw(&L->cur,"PLATEPICK") || kw(&L->cur,"PLATEKEEP")){
      char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
      char arg[CUBALC_HOST_STR_MAX];
      int nkeys = 0, have_plate = 0;
      size_t olen = 0;
      cubalc_host_result hr;
      lex_next(L);
      plate[0] = 0;
      keys_nl[0] = 0;
      /* optional plate first if object-looking, or OF bag form */
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          const char *s = arg;
          while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
          if (*s == '{') {
            snprintf(plate, sizeof plate, "%s", arg);
            have_plate = 1;
          } else {
            /* first key (or bag) — plate = LAST later */
            size_t al = strlen(arg);
            if (al + 1 < sizeof keys_nl) {
              memcpy(keys_nl, arg, al + 1);
              olen = al;
              nkeys = 1;
              /* if bag with newlines, count fields */
              {
                const char *q = arg;
                int lines = 1;
                for (; *q; q++) if (*q == '\n') lines++;
                if (strchr(arg, '\n')) nkeys = lines;
              }
            }
          }
        }
      }
      /* OF|KEYS|FROM bag after plate */
      if (have_plate && (kw(&L->cur,"OF") || kw(&L->cur,"KEYS") || kw(&L->cur,"FROM") ||
                         kw(&L->cur,"KEEP") || kw(&L->cur,"ONLY"))) {
        lex_next(L);
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          snprintf(keys_nl, sizeof keys_nl, "%s", arg);
          nkeys = 0;
          {
            const char *q = arg;
            if (*q) {
              nkeys = 1;
              for (; *q; q++) if (*q == '\n') nkeys++;
            }
          }
          olen = strlen(keys_nl);
        }
      } else {
        /* more keys as string args */
        while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (L->cur.kind == TK_IDENT &&
              (kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
               kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
               kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
               kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"JSON") ||
               kw(&L->cur,"JSONSET") || kw(&L->cur,"JSONPICK"))) {
            break;
          }
          if (resolve_str_arg(vm, L, arg, sizeof arg) != 0)
            break;
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
          nkeys++;
        }
      }
      if (!have_plate)
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      if (!keys_nl[0]) {
        fail(vm, "SYS JSONPICK [plate] key [key ...] — JSONPICK plate \"ok\" \"n\"");
        return -1;
      }
      (void)nkeys;
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_pick(plate, keys_nl, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONPICK_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONPICK: fail");
          var_set_str(vm, "ERR", "JSONPICK: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "JSONPICK", hr.str);
      var_set_str(vm, "JSONKEEP", hr.str);
      var_set_str(vm, "JKEEP", hr.str);
      var_set_num(vm, "JSONPICK_N", hr.n);
      var_set_num(vm, "JSONKEEP_N", hr.n);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonpick → kept=%ld\n", hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONRENAME|JRENAME|RENAMEJSON [plate] old new
     * — rename top-level key keeping raw value → LAST = plate · LAST_N = 1|0.
     * 3-arg: plate old new · 2-arg: old new (plate=LAST). Soft miss (old absent).
     * Dest key overwritten if present. Same name is no-op when present.
     * Usability: promote temp/status field names without get_raw+del+set glue. */
    if (kw(&L->cur,"JSONRENAME") || kw(&L->cur,"JRENAME") || kw(&L->cur,"RENAMEJSON") ||
        kw(&L->cur,"JSONMV") || kw(&L->cur,"JMV") || kw(&L->cur,"MOVEJSON") ||
        kw(&L->cur,"PLATERENAME") || kw(&L->cur,"RENAMEKEY")){
      char plate[CUBALC_HOST_STR_MAX], oldk[96], newk[96];
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX], c[CUBALC_HOST_STR_MAX];
      int narg = 0;
      cubalc_host_result hr;
      lex_next(L);
      plate[0] = 0; oldk[0] = 0; newk[0] = 0;
      a[0] = 0; b[0] = 0; c[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, a, sizeof a) == 0) narg = 1;
      }
      if (narg >= 1 && (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
        if (resolve_str_arg(vm, L, b, sizeof b) == 0) narg = 2;
      }
      if (narg >= 2 && (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
        if (resolve_str_arg(vm, L, c, sizeof c) == 0) narg = 3;
      }
      if (narg == 3) {
        snprintf(plate, sizeof plate, "%s", a);
        snprintf(oldk, sizeof oldk, "%s", b);
        snprintf(newk, sizeof newk, "%s", c);
      } else if (narg == 2) {
        snprintf(plate, sizeof plate, "%s", vm->last_str);
        snprintf(oldk, sizeof oldk, "%s", a);
        snprintf(newk, sizeof newk, "%s", b);
      } else {
        fail(vm, "SYS JSONRENAME [plate] old new — JSONRENAME plate \"tmp\" \"status\"");
        return -1;
      }
      if (!oldk[0] || !newk[0]) {
        var_set_str(vm, "LAST", plate[0] ? plate : "");
        if (plate[0])
          snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        else
          vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONRENAME_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "JSONRENAME: empty key");
        var_set_str(vm, "ERR", "JSONRENAME: empty key");
        bump(vm); return 1;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_rename(plate, oldk, newk, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONRENAME_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONRENAME: fail");
          var_set_str(vm, "ERR", "JSONRENAME: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "JSONRENAME", hr.str);
      var_set_str(vm, "JRENAME", hr.str);
      var_set_num(vm, "JSONRENAME_N", hr.n);
      var_set_str(vm, "JSONRENAME_FROM", oldk);
      var_set_str(vm, "JSONRENAME_TO", newk);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonrename → %s→%s n=%ld\n", oldk, newk, hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONDROP|JSONOMIT|JDROPK|JOMIT [plate] key [key ...]
     * — drop listed top-level keys → LAST = plate · LAST_N = removed count.
     * Missing keys soft-skipped. Forms: plate k1 k2… · k1 k2… (plate=LAST) · plate OF bag.
     * Usability: strip known noise fields without chained JSONDEL · twin of JSONPICK. */
    if (kw(&L->cur,"JSONDROP") || kw(&L->cur,"JSONOMIT") || kw(&L->cur,"JDROPK") ||
        kw(&L->cur,"JOMIT") || kw(&L->cur,"DROPJSON") || kw(&L->cur,"OMITJSON") ||
        kw(&L->cur,"JSONEXCEPT") || kw(&L->cur,"PLATEDROP") || kw(&L->cur,"DROPKEYS")){
      char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
      char arg[CUBALC_HOST_STR_MAX];
      int nkeys = 0, have_plate = 0;
      size_t olen = 0;
      cubalc_host_result hr;
      lex_next(L);
      plate[0] = 0;
      keys_nl[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          const char *s = arg;
          while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
          if (*s == '{') {
            snprintf(plate, sizeof plate, "%s", arg);
            have_plate = 1;
          } else {
            size_t al = strlen(arg);
            if (al + 1 < sizeof keys_nl) {
              memcpy(keys_nl, arg, al + 1);
              olen = al;
              nkeys = 1;
              if (strchr(arg, '\n')) {
                const char *q = arg;
                nkeys = 1;
                for (; *q; q++) if (*q == '\n') nkeys++;
              }
            }
          }
        }
      }
      if (have_plate && (kw(&L->cur,"OF") || kw(&L->cur,"KEYS") || kw(&L->cur,"FROM") ||
                         kw(&L->cur,"DROP") || kw(&L->cur,"EXCEPT") || kw(&L->cur,"OMIT"))) {
        lex_next(L);
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          snprintf(keys_nl, sizeof keys_nl, "%s", arg);
          nkeys = 0;
          {
            const char *q = arg;
            if (*q) {
              nkeys = 1;
              for (; *q; q++) if (*q == '\n') nkeys++;
            }
          }
          olen = strlen(keys_nl);
        }
      } else {
        while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (L->cur.kind == TK_IDENT &&
              (kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
               kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
               kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
               kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"JSON") ||
               kw(&L->cur,"JSONSET") || kw(&L->cur,"JSONDROP") || kw(&L->cur,"JSONPICK"))) {
            break;
          }
          if (resolve_str_arg(vm, L, arg, sizeof arg) != 0)
            break;
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
          nkeys++;
        }
      }
      if (!have_plate)
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      if (!keys_nl[0]) {
        fail(vm, "SYS JSONDROP [plate] key [key ...] — JSONDROP plate \"noise\" \"tmp\"");
        return -1;
      }
      (void)nkeys;
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_drop(plate, keys_nl, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONDROP_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONDROP: fail");
          var_set_str(vm, "ERR", "JSONDROP: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "JSONDROP", hr.str);
      var_set_str(vm, "JSONOMIT", hr.str);
      var_set_str(vm, "JDROPK", hr.str);
      var_set_num(vm, "JSONDROP_N", hr.n);
      var_set_num(vm, "JSONOMIT_N", hr.n);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsondrop → removed=%ld\n", hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONTOGGLE|JTOGGLE|FLIPJSON [plate] key
     * — flip plate flag 0↔1 → LAST = plate · LAST_N = new 0|1.
     * Missing / false / 0 / empty / null → 1; truthy → 0.
     * 2-arg: plate key · 1-arg: key (plate=LAST).
     * Usability: feature flags without JSONN+IF+JSONSET glue. */
    if (kw(&L->cur,"JSONTOGGLE") || kw(&L->cur,"JTOGGLE") || kw(&L->cur,"FLIPJSON") ||
        kw(&L->cur,"TOGGLEJSON") || kw(&L->cur,"JSONFLIP") || kw(&L->cur,"JFLIP") ||
        kw(&L->cur,"PLATETOGGLE") || kw(&L->cur,"TOGGLEKEY")){
      char plate[CUBALC_HOST_STR_MAX], key[96];
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      char raw[8];
      cubalc_host_result gr, hr;
      int narg = 0;
      long cur = 0, nv = 0;
      lex_next(L);
      plate[0] = 0; key[0] = 0; a[0] = 0; b[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, a, sizeof a) == 0) narg = 1;
      }
      if (narg >= 1 && (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
        if (resolve_str_arg(vm, L, b, sizeof b) == 0) narg = 2;
      }
      if (narg == 2) {
        snprintf(plate, sizeof plate, "%s", a);
        snprintf(key, sizeof key, "%s", b);
      } else if (narg == 1) {
        snprintf(plate, sizeof plate, "%s", vm->last_str);
        snprintf(key, sizeof key, "%s", a);
      } else {
        fail(vm, "SYS JSONTOGGLE [plate] key — JSONTOGGLE plate \"debug\"");
        return -1;
      }
      if (!key[0]) {
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "JSONTOGGLE: empty key");
        var_set_str(vm, "ERR", "JSONTOGGLE: empty key");
        var_set_num(vm, "JSONTOGGLE_N", 0);
        var_set_num(vm, "LAST_N", 0);
        bump(vm); return 1;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_get(plate, key, &gr) == 0) {
        char *end = NULL;
        /* host maps true→"1", false→"0", null→"" */
        if (!gr.str[0]) {
          cur = 0;
        } else {
          cur = strtol(gr.str, &end, 10);
          if (end == gr.str) {
            /* non-numeric non-empty → truthy */
            cur = 1;
          }
        }
      } else {
        cur = 0; /* missing → treat as off */
      }
      nv = cur ? 0 : 1;
      snprintf(raw, sizeof raw, "%ld", nv);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_set(plate, key, raw, 1, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONTOGGLE_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONTOGGLE: fail");
          var_set_str(vm, "ERR", "JSONTOGGLE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = nv;
      var_set_num(vm, "LAST_N", nv);
      var_set_str(vm, "JSONTOGGLE", hr.str);
      var_set_str(vm, "JTOGGLE", hr.str);
      var_set_num(vm, "JSONTOGGLE_V", nv);
      var_set_num(vm, "JSONTOGGLE_N", 1);
      var_set_num(vm, "JSONTOGGLE_PREV", cur ? 1 : 0);
      var_set_str(vm, "JSONTOGGLE_KEY", key);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsontoggle → key=%s %ld→%ld\n", key, cur ? 1L : 0L, nv);
      bump(vm); return 1;
    }
    /* SYS JSONTYPE|JTYPE|JSONKIND [plate] key
     * — probe top-level value kind → LAST = kind string · LAST_N = kind code.
     * Codes: 0 missing · 1 num · 2 str · 3 bool · 4 null · 5 obj · 6 arr.
     * Forms: plate key · key (plate=LAST) · key FROM plate.
     * Usability: choose JSONN vs string peel vs nested without guess/IF glue. */
    if (kw(&L->cur,"JSONTYPE") || kw(&L->cur,"JTYPE") || kw(&L->cur,"JSONKIND") ||
        kw(&L->cur,"TYPEJSON") || kw(&L->cur,"KINDJSON") || kw(&L->cur,"JKIND") ||
        kw(&L->cur,"PLATETYPE") || kw(&L->cur,"KEYTYPE")){
      char plate[CUBALC_HOST_STR_MAX], key[96];
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      const char *kind_s = "missing";
      long kind_n = 0;
      int narg = 0;
      cubalc_host_result gr;
      lex_next(L);
      plate[0] = 0; key[0] = 0; a[0] = 0; b[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, a, sizeof a) == 0) narg = 1;
      }
      /* optional FROM|IN|OF plate after key (1-arg so far) */
      if (narg == 1 && (kw(&L->cur,"FROM") || kw(&L->cur,"IN") || kw(&L->cur,"OF") ||
                        kw(&L->cur,"ON"))) {
        lex_next(L);
        if (resolve_str_arg(vm, L, plate, sizeof plate) != 0)
          plate[0] = 0;
        snprintf(key, sizeof key, "%s", a);
        narg = 2;
      } else if (narg >= 1 && (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
        if (resolve_str_arg(vm, L, b, sizeof b) == 0) narg = 2;
      }
      if (narg == 2 && !key[0]) {
        /* plate key form (first may be object or var) */
        const char *s = a;
        while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
        if (*s == '{' || (a[0] && b[0])) {
          snprintf(plate, sizeof plate, "%s", a);
          snprintf(key, sizeof key, "%s", b);
        } else {
          snprintf(plate, sizeof plate, "%s", vm->last_str);
          snprintf(key, sizeof key, "%s", a);
        }
      } else if (narg == 2 && key[0]) {
        /* FROM form already filled key+plate */
      } else if (narg == 1) {
        snprintf(plate, sizeof plate, "%s", vm->last_str);
        snprintf(key, sizeof key, "%s", a);
      } else {
        fail(vm, "SYS JSONTYPE [plate] key — JSONTYPE plate \"retries\"");
        return -1;
      }
      if (!key[0]) {
        var_set_str(vm, "LAST", "missing");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "missing");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONTYPE_N", 0);
        var_set_str(vm, "JSONTYPE", "missing");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "JSONTYPE: empty key");
        var_set_str(vm, "ERR", "JSONTYPE: empty key");
        bump(vm); return 1;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_get_raw(plate, key, &gr) != 0) {
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
      var_set_str(vm, "JSONTYPE", kind_s);
      var_set_str(vm, "JTYPE", kind_s);
      var_set_str(vm, "JSONKIND", kind_s);
      var_set_num(vm, "JSONTYPE_N", kind_n);
      var_set_num(vm, "JSONKIND_N", kind_n);
      var_set_str(vm, "JSONTYPE_KEY", key);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsontype → key=%s kind=%s n=%ld\n", key, kind_s, kind_n);
      bump(vm); return 1;
    }
    /* SYS JSONCOPY|JCOPY|COPYJSON [plate] src dst
     * — copy top-level key raw value to new key → LAST = plate · LAST_N = 1|0.
     * Soft miss (src absent) keeps plate, LAST_N=0. Dest overwritten if present.
     * 3-arg: plate src dst · 2-arg: src dst (plate=LAST).
     * Usability: snapshot field before mutate without get_raw+JSONSET glue. */
    if (kw(&L->cur,"JSONCOPY") || kw(&L->cur,"JCOPY") || kw(&L->cur,"COPYJSON") ||
        kw(&L->cur,"JSONDUP") || kw(&L->cur,"JDUP") || kw(&L->cur,"DUPJSON") ||
        kw(&L->cur,"PLATECOPY") || kw(&L->cur,"COPYKEY")){
      char plate[CUBALC_HOST_STR_MAX], src[96], dst[96];
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX], c[CUBALC_HOST_STR_MAX];
      int narg = 0;
      cubalc_host_result gr, hr;
      lex_next(L);
      plate[0] = 0; src[0] = 0; dst[0] = 0;
      a[0] = 0; b[0] = 0; c[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, a, sizeof a) == 0) narg = 1;
      }
      if (narg >= 1 && (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
        if (resolve_str_arg(vm, L, b, sizeof b) == 0) narg = 2;
      }
      if (narg >= 2 && (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
        if (resolve_str_arg(vm, L, c, sizeof c) == 0) narg = 3;
      }
      if (narg == 3) {
        snprintf(plate, sizeof plate, "%s", a);
        snprintf(src, sizeof src, "%s", b);
        snprintf(dst, sizeof dst, "%s", c);
      } else if (narg == 2) {
        snprintf(plate, sizeof plate, "%s", vm->last_str);
        snprintf(src, sizeof src, "%s", a);
        snprintf(dst, sizeof dst, "%s", b);
      } else {
        fail(vm, "SYS JSONCOPY [plate] src dst — JSONCOPY plate \"n\" \"n0\"");
        return -1;
      }
      if (!src[0] || !dst[0]) {
        var_set_str(vm, "LAST", plate[0] ? plate : "");
        if (plate[0])
          snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        else
          vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONCOPY_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "JSONCOPY: empty key");
        var_set_str(vm, "ERR", "JSONCOPY: empty key");
        bump(vm); return 1;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_get_raw(plate, src, &gr) != 0) {
        /* soft miss — return plate copy */
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "JSONCOPY", plate);
        var_set_str(vm, "JCOPY", plate);
        var_set_num(vm, "JSONCOPY_N", 0);
        var_set_str(vm, "JSONCOPY_SRC", src);
        var_set_str(vm, "JSONCOPY_DST", dst);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# sys jsoncopy → %s→%s miss\n", src, dst);
        bump(vm); return 1;
      }
      if (strcmp(src, dst) == 0) {
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_str(vm, "JSONCOPY", plate);
        var_set_num(vm, "JSONCOPY_N", 1);
        var_set_str(vm, "JSONCOPY_SRC", src);
        var_set_str(vm, "JSONCOPY_DST", dst);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_set(plate, dst, gr.str, 1, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONCOPY_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONCOPY: fail");
          var_set_str(vm, "ERR", "JSONCOPY: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "JSONCOPY", hr.str);
      var_set_str(vm, "JCOPY", hr.str);
      var_set_num(vm, "JSONCOPY_N", 1);
      var_set_str(vm, "JSONCOPY_SRC", src);
      var_set_str(vm, "JSONCOPY_DST", dst);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsoncopy → %s→%s ok\n", src, dst);
      bump(vm); return 1;
    }
    /* SYS JSONSWAP|JSWAP|SWAPJSON [plate] keyA keyB
     * — exchange two top-level keys' raw values → LAST = plate · LAST_N = 1|0.
     * Soft: both missing → n=0. One missing → move (rename). Same name no-op.
     * 3-arg: plate a b · 2-arg: a b (plate=LAST).
     * Usability: dual-buffer fields without temp+JSONRENAME glue. */
    if (kw(&L->cur,"JSONSWAP") || kw(&L->cur,"JSWAP") || kw(&L->cur,"SWAPJSON") ||
        kw(&L->cur,"JSONXCHG") || kw(&L->cur,"JXCHG") || kw(&L->cur,"EXCHJSON") ||
        kw(&L->cur,"PLATESWAP") || kw(&L->cur,"SWAPKEYS")){
      char plate[CUBALC_HOST_STR_MAX], ka[96], kb[96];
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX], c[CUBALC_HOST_STR_MAX];
      char ra[CUBALC_HOST_STR_MAX], rb[CUBALC_HOST_STR_MAX];
      int narg = 0, has_a = 0, has_b = 0;
      cubalc_host_result gr, hr, dr;
      lex_next(L);
      plate[0] = 0; ka[0] = 0; kb[0] = 0;
      a[0] = 0; b[0] = 0; c[0] = 0;
      ra[0] = 0; rb[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, a, sizeof a) == 0) narg = 1;
      }
      if (narg >= 1 && (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
        if (resolve_str_arg(vm, L, b, sizeof b) == 0) narg = 2;
      }
      if (narg >= 2 && (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
        if (resolve_str_arg(vm, L, c, sizeof c) == 0) narg = 3;
      }
      if (narg == 3) {
        snprintf(plate, sizeof plate, "%s", a);
        snprintf(ka, sizeof ka, "%s", b);
        snprintf(kb, sizeof kb, "%s", c);
      } else if (narg == 2) {
        snprintf(plate, sizeof plate, "%s", vm->last_str);
        snprintf(ka, sizeof ka, "%s", a);
        snprintf(kb, sizeof kb, "%s", b);
      } else {
        fail(vm, "SYS JSONSWAP [plate] keyA keyB — JSONSWAP plate \"cur\" \"prev\"");
        return -1;
      }
      if (!ka[0] || !kb[0]) {
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "JSONSWAP: empty key");
        var_set_str(vm, "ERR", "JSONSWAP: empty key");
        var_set_num(vm, "JSONSWAP_N", 0);
        var_set_num(vm, "LAST_N", 0);
        bump(vm); return 1;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      if (strcmp(ka, kb) == 0) {
        memset(&gr, 0, sizeof gr);
        has_a = (cubalc_host_json_get_raw(plate, ka, &gr) == 0) ? 1 : 0;
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = has_a;
        var_set_num(vm, "LAST_N", has_a);
        var_set_str(vm, "JSONSWAP", plate);
        var_set_num(vm, "JSONSWAP_N", has_a);
        var_set_str(vm, "JSONSWAP_A", ka);
        var_set_str(vm, "JSONSWAP_B", kb);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_get_raw(plate, ka, &gr) == 0) {
        has_a = 1;
        snprintf(ra, sizeof ra, "%s", gr.str);
      }
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_get_raw(plate, kb, &gr) == 0) {
        has_b = 1;
        snprintf(rb, sizeof rb, "%s", gr.str);
      }
      if (!has_a && !has_b) {
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "JSONSWAP", plate);
        var_set_num(vm, "JSONSWAP_N", 0);
        var_set_str(vm, "JSONSWAP_A", ka);
        var_set_str(vm, "JSONSWAP_B", kb);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# sys jsonswap → both miss\n");
        bump(vm); return 1;
      }
      /* apply swap / move */
      if (has_a && has_b) {
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_json_set(plate, ka, rb, 1, &hr) != 0) goto jswap_fail;
        snprintf(plate, sizeof plate, "%s", hr.str);
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_json_set(plate, kb, ra, 1, &hr) != 0) goto jswap_fail;
        snprintf(plate, sizeof plate, "%s", hr.str);
      } else if (has_a && !has_b) {
        /* move a → b */
        memset(&dr, 0, sizeof dr);
        if (cubalc_host_json_del(plate, ka, &dr) != 0) goto jswap_fail;
        snprintf(plate, sizeof plate, "%s", dr.str);
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_json_set(plate, kb, ra, 1, &hr) != 0) goto jswap_fail;
        snprintf(plate, sizeof plate, "%s", hr.str);
      } else {
        /* move b → a */
        memset(&dr, 0, sizeof dr);
        if (cubalc_host_json_del(plate, kb, &dr) != 0) goto jswap_fail;
        snprintf(plate, sizeof plate, "%s", dr.str);
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_json_set(plate, ka, rb, 1, &hr) != 0) goto jswap_fail;
        snprintf(plate, sizeof plate, "%s", hr.str);
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "JSONSWAP", plate);
      var_set_str(vm, "JSWAP", plate);
      var_set_num(vm, "JSONSWAP_N", 1);
      var_set_str(vm, "JSONSWAP_A", ka);
      var_set_str(vm, "JSONSWAP_B", kb);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonswap → %s↔%s ok\n", ka, kb);
      bump(vm); return 1;
    jswap_fail:
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "JSONSWAP_N", 0);
      var_set_num(vm, "OK", 0);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else if (dr.err[0]) {
        var_set_str(vm, "LAST_ERR", dr.err);
        var_set_str(vm, "ERR", dr.err);
      } else {
        var_set_str(vm, "LAST_ERR", "JSONSWAP: fail");
        var_set_str(vm, "ERR", "JSONSWAP: fail");
      }
      bump(vm); return 1;
    }
    /* SYS JSONVALUES|JVALS|JSONVALS|PLATEVALS [plate|LAST]
     * — top-level values → newline bag (JSONKEYS order) · LAST_N = count.
     * Strings decoded; bool→0|1; null→empty; nested raw. Twin of JSONKEYS.
     * Usability: ZIP keys+vals / SUM / FREQ without EACH+JSON peel glue. */
    if (kw(&L->cur,"JSONVALUES") || kw(&L->cur,"JVALS") || kw(&L->cur,"JSONVALS") ||
        kw(&L->cur,"VALSJSON") || kw(&L->cur,"PLATEVALS") || kw(&L->cur,"JSONVS") ||
        kw(&L->cur,"JVALUES")){
      char plate[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      lex_next(L);
      plate[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, plate, sizeof plate) != 0)
          plate[0] = 0;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_values(plate, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONVALUES_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONVALUES: fail");
          var_set_str(vm, "ERR", "JSONVALUES: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "JSONVALUES", hr.str);
      var_set_str(vm, "JVALS", hr.str);
      var_set_str(vm, "JSONVALS", hr.str);
      var_set_num(vm, "JSONVALUES_N", hr.n);
      var_set_num(vm, "JVALS_N", hr.n);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonvalues → n=%ld\n", hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONFROMKV|JFROMKV|KVTOJSON|JSONFROMBAG [bag|LAST]
     * — key=val / key:val newline bag → object plate · LAST_N = pairs.
     * Integers/true/false/null raw; other values quoted strings.
     * Usability: FREQ/FLAGMAP/LOOKUP bags → plate without EACH+JSONSET glue. */
    if (kw(&L->cur,"JSONFROMKV") || kw(&L->cur,"JFROMKV") || kw(&L->cur,"KVTOJSON") ||
        kw(&L->cur,"JSONFROMBAG") || kw(&L->cur,"BAGTOJSON") || kw(&L->cur,"FROMKV") ||
        kw(&L->cur,"PLATEFROMKV") || kw(&L->cur,"JSONLOADKV")){
      char bag[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      lex_next(L);
      bag[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
          bag[0] = 0;
      }
      if (!bag[0])
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_from_kv(bag, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFROMKV_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONFROMKV: fail");
          var_set_str(vm, "ERR", "JSONFROMKV: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "JSONFROMKV", hr.str);
      var_set_str(vm, "JFROMKV", hr.str);
      var_set_str(vm, "KVTOJSON", hr.str);
      var_set_num(vm, "JSONFROMKV_N", hr.n);
      var_set_num(vm, "KVTOJSON_N", hr.n);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonfromkv → pairs=%ld\n", hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONTOKV|JTOKV|JSONTOBAG|PLATETOKV [plate|LAST]
     * — object plate → key:val newline bag · LAST_N = pairs.
     * Dual of JSONFROMKV. ':' matches LOOKUP/FREQ default (no sep glue).
     * Strings decoded; true/false/null tokens; nums raw. */
    if (kw(&L->cur,"JSONTOKV") || kw(&L->cur,"JTOKV") || kw(&L->cur,"JSONTOBAG") ||
        kw(&L->cur,"PLATETOKV") || kw(&L->cur,"TOKV") || kw(&L->cur,"JSON2KV") ||
        kw(&L->cur,"PLATE2KV") || kw(&L->cur,"KVFROMJSON")){
      char plate[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      lex_next(L);
      plate[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, plate, sizeof plate) != 0)
          plate[0] = 0;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_to_kv(plate, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONTOKV_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONTOKV: fail");
          var_set_str(vm, "ERR", "JSONTOKV: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "JSONTOKV", hr.str);
      var_set_str(vm, "JTOKV", hr.str);
      var_set_str(vm, "JSONTOBAG", hr.str);
      var_set_num(vm, "JSONTOKV_N", hr.n);
      var_set_num(vm, "JTOKV_N", hr.n);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsontokv → pairs=%ld\n", hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONLEN|JLEN|JSONCOUNT|NKEYS|KEYCOUNT [plate|LAST]
     * — top-level key count → LAST_N · LAST kept as plate (chain).
     * Soft 0 on non-object (OK=1). Empty-plate IF without JSONKEYS bag. */
    if (kw(&L->cur,"JSONLEN") || kw(&L->cur,"JLEN") || kw(&L->cur,"JSONCOUNT") ||
        kw(&L->cur,"NKEYS") || kw(&L->cur,"KEYCOUNT") || kw(&L->cur,"JSONSIZE") ||
        kw(&L->cur,"PLATELEN") || kw(&L->cur,"COUNTKEYS")){
      char plate[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      lex_next(L);
      plate[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, plate, sizeof plate) != 0)
          plate[0] = 0;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_len(plate, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONLEN_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONLEN: fail");
          var_set_str(vm, "ERR", "JSONLEN: fail");
        }
        bump(vm); return 1;
      }
      /* keep plate in LAST for chain (JSONTOKV bare / further peels) */
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "JSONLEN_N", hr.n);
      var_set_num(vm, "JLEN_N", hr.n);
      var_set_num(vm, "JSONCOUNT_N", hr.n);
      var_set_str(vm, "JSONLEN", hr.str);
      var_set_str(vm, "JLEN", hr.str);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonlen → n=%ld\n", hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONEMPTY|JEMPTY|ISEMPTYJSON [plate|LAST]
     * — soft LAST_N 1 if zero keys / non-object · LAST kept as plate.
     * SYS JSONNONEMPTY|JNONEMPTY|HASKEYS — inverse LAST_N 1 if count>0.
     * Usability: IF empty plate without JSONLEN+CMP glue. */
    if (kw(&L->cur,"JSONEMPTY") || kw(&L->cur,"JEMPTY") || kw(&L->cur,"ISEMPTYJSON") ||
        kw(&L->cur,"PLATEEMPTY") || kw(&L->cur,"EMPTYJSON") || kw(&L->cur,"JSONISEMPTY") ||
        kw(&L->cur,"JSONNONEMPTY") || kw(&L->cur,"JNONEMPTY") || kw(&L->cur,"HASKEYS") ||
        kw(&L->cur,"JSONHASKEYS") || kw(&L->cur,"NONEMPTYJSON") || kw(&L->cur,"PLATENONEMPTY")){
      char plate[CUBALC_HOST_STR_MAX];
      char op[32];
      cubalc_host_result hr;
      int want_nonempty = 0;
      long hit;
      snprintf(op, sizeof op, "%s", L->cur.text);
      {
        char *q;
        for (q = op; *q; q++)
          if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      }
      if (strcmp(op, "JSONNONEMPTY") == 0 || strcmp(op, "JNONEMPTY") == 0 ||
          strcmp(op, "HASKEYS") == 0 || strcmp(op, "JSONHASKEYS") == 0 ||
          strcmp(op, "NONEMPTYJSON") == 0 || strcmp(op, "PLATENONEMPTY") == 0)
        want_nonempty = 1;
      lex_next(L);
      plate[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, plate, sizeof plate) != 0)
          plate[0] = 0;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_len(plate, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONEMPTY_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONEMPTY: fail");
          var_set_str(vm, "ERR", "JSONEMPTY: fail");
        }
        bump(vm); return 1;
      }
      hit = want_nonempty ? (hr.n > 0 ? 1 : 0) : (hr.n == 0 ? 1 : 0);
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "JSONEMPTY_N", hit);
      var_set_num(vm, "JSONNONEMPTY_N", hit);
      var_set_num(vm, "JSONEMPTY_LEN", hr.n);
      var_set_num(vm, "JSONLEN_N", hr.n);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys %s → hit=%ld len=%ld\n",
                want_nonempty ? "jsonnonempty" : "jsonempty", hit, hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONSUMN|JSONSUM|JSUMN|SUMJSON [plate|LAST]
     * — sum pure-integer top-level values → LAST_N · LAST kept as plate.
     * Bools count (true=1/false=0); strings/null/nested skipped.
     * Usability: score/status totals without JSONVALUES+SUM glue. */
    if (kw(&L->cur,"JSONSUMN") || kw(&L->cur,"JSONSUM") || kw(&L->cur,"JSUMN") ||
        kw(&L->cur,"SUMJSON") || kw(&L->cur,"PLATESUM") || kw(&L->cur,"JSUM") ||
        kw(&L->cur,"JSONTOTAL") || kw(&L->cur,"TOTALJSON")){
      char plate[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      lex_next(L);
      plate[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, plate, sizeof plate) != 0)
          plate[0] = 0;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_sum(plate, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONSUMN_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONSUMN: fail");
          var_set_str(vm, "ERR", "JSONSUMN: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "JSONSUMN_N", hr.n);
      var_set_num(vm, "JSONSUM_N", hr.n);
      var_set_str(vm, "JSONSUMN", hr.str);
      var_set_str(vm, "JSONSUM", hr.str);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonsumn → sum=%ld\n", hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONTOPKEY|JTOPKEY|JSONMAXKEY [plate|LAST]
     * SYS JSONBOTKEY|JBOTKEY|JSONMINKEY [plate|LAST]
     * — key with max/min pure-integer value · LAST=key · LAST_N=value.
     * First wins ties. Soft empty if no numeric fields.
     * Usability: dominant FREQ key without JSONTOKV+TOPKEY glue. */
    if (kw(&L->cur,"JSONTOPKEY") || kw(&L->cur,"JTOPKEY") || kw(&L->cur,"JSONMAXKEY") ||
        kw(&L->cur,"MAXJSONKEY") || kw(&L->cur,"PLATETOPKEY") || kw(&L->cur,"JMAXKEY") ||
        kw(&L->cur,"JSONBOTKEY") || kw(&L->cur,"JBOTKEY") || kw(&L->cur,"JSONMINKEY") ||
        kw(&L->cur,"MINJSONKEY") || kw(&L->cur,"PLATEBOTKEY") || kw(&L->cur,"JMINKEY")){
      char plate[CUBALC_HOST_STR_MAX];
      char op[32];
      cubalc_host_result hr;
      int want_min = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      {
        char *q;
        for (q = op; *q; q++)
          if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      }
      if (strcmp(op, "JSONBOTKEY") == 0 || strcmp(op, "JBOTKEY") == 0 ||
          strcmp(op, "JSONMINKEY") == 0 || strcmp(op, "MINJSONKEY") == 0 ||
          strcmp(op, "PLATEBOTKEY") == 0 || strcmp(op, "JMINKEY") == 0)
        want_min = 1;
      lex_next(L);
      plate[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, plate, sizeof plate) != 0)
          plate[0] = 0;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_topkey(plate, want_min, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", want_min ? "JSONBOTKEY: fail" : "JSONTOPKEY: fail");
          var_set_str(vm, "ERR", want_min ? "JSONBOTKEY: fail" : "JSONTOPKEY: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      if (want_min) {
        var_set_str(vm, "JSONBOTKEY", hr.str);
        var_set_str(vm, "JBOTKEY", hr.str);
        var_set_num(vm, "JSONBOTKEY_V", hr.n);
        var_set_num(vm, "JSONBOTKEY_N", hr.n);
      } else {
        var_set_str(vm, "JSONTOPKEY", hr.str);
        var_set_str(vm, "JTOPKEY", hr.str);
        var_set_num(vm, "JSONTOPKEY_V", hr.n);
        var_set_num(vm, "JSONTOPKEY_N", hr.n);
      }
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys %s → key=%s v=%ld\n",
                want_min ? "jsonbotkey" : "jsontopkey", hr.str, hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONMAXN|JMAXN|PLATEMAX [plate|LAST]
     * SYS JSONMINN|JMINN|PLATEMIN [plate|LAST]
     * — max/min pure-integer top-level value → LAST_N · LAST kept as plate.
     * Soft 0 if none. Usability: score extremes without JSONVALUES+MIN/MAX. */
    if (kw(&L->cur,"JSONMAXN") || kw(&L->cur,"JMAXN") || kw(&L->cur,"PLATEMAX") ||
        kw(&L->cur,"JSONVMAX") || kw(&L->cur,"MAXJSONN") || kw(&L->cur,"JMAX") ||
        kw(&L->cur,"JSONMINN") || kw(&L->cur,"JMINN") || kw(&L->cur,"PLATEMIN") ||
        kw(&L->cur,"JSONVMIN") || kw(&L->cur,"MINJSONN") || kw(&L->cur,"JMIN")){
      char plate[CUBALC_HOST_STR_MAX];
      char op[32];
      cubalc_host_result hr;
      int want_min = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      {
        char *q;
        for (q = op; *q; q++)
          if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      }
      if (strcmp(op, "JSONMINN") == 0 || strcmp(op, "JMINN") == 0 ||
          strcmp(op, "PLATEMIN") == 0 || strcmp(op, "JSONVMIN") == 0 ||
          strcmp(op, "MINJSONN") == 0 || strcmp(op, "JMIN") == 0)
        want_min = 1;
      lex_next(L);
      plate[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, plate, sizeof plate) != 0)
          plate[0] = 0;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_minmax(plate, want_min, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", want_min ? "JSONMINN: fail" : "JSONMAXN: fail");
          var_set_str(vm, "ERR", want_min ? "JSONMINN: fail" : "JSONMAXN: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      if (want_min) {
        var_set_num(vm, "JSONMINN_N", hr.n);
        var_set_str(vm, "JSONMINN", hr.str);
        var_set_str(vm, "JMINN", hr.str);
      } else {
        var_set_num(vm, "JSONMAXN_N", hr.n);
        var_set_str(vm, "JSONMAXN", hr.str);
        var_set_str(vm, "JMAXN", hr.str);
      }
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys %s → v=%ld\n",
                want_min ? "jsonminn" : "jsonmaxn", hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONAVGN|JAVGN|JSONMEAN|MEANJSON [plate|LAST]
     * — integer mean of pure-integer top-level values → LAST_N · LAST plate kept.
     * JSONAVGN_USED = field count. Soft 0 if none.
     * Usability: typical score without JSONVALUES+AVG glue. */
    if (kw(&L->cur,"JSONAVGN") || kw(&L->cur,"JAVGN") || kw(&L->cur,"JSONMEAN") ||
        kw(&L->cur,"MEANJSON") || kw(&L->cur,"PLATEAVG") || kw(&L->cur,"JAVERAGE") ||
        kw(&L->cur,"JSONAVERAGE") || kw(&L->cur,"AVGJSON")){
      char plate[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      lex_next(L);
      plate[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, plate, sizeof plate) != 0)
          plate[0] = 0;
      }
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_avg(plate, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONAVGN_N", 0);
        var_set_num(vm, "JSONAVGN_USED", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONAVGN: fail");
          var_set_str(vm, "ERR", "JSONAVGN: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "JSONAVGN_N", hr.n);
      var_set_num(vm, "JSONAVGN_USED", (long)hr.code);
      var_set_num(vm, "JSONMEAN_N", hr.n);
      var_set_str(vm, "JSONAVGN", hr.str);
      var_set_str(vm, "JAVGN", hr.str);
      var_set_str(vm, "JSONMEAN", hr.str);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonavgn → avg=%ld used=%d\n", hr.n, hr.code);
      bump(vm); return 1;
    }
    /* SYS JSONEQ|JEQ|SAMEJSON a b
     * SYS JSONNEQ|JNEQ|DIFFJSON a b
     * — order-independent top-level plate equality → LAST_N 1|0.
     * Same keys + same raw values. Soft. Usability: verify WRITE/MERGE without EQS. */
    if (kw(&L->cur,"JSONEQ") || kw(&L->cur,"JEQ") || kw(&L->cur,"SAMEJSON") ||
        kw(&L->cur,"EQJSON") || kw(&L->cur,"PLATEEQ") || kw(&L->cur,"JSONSAME") ||
        kw(&L->cur,"JSONNEQ") || kw(&L->cur,"JNEQ") || kw(&L->cur,"DIFFJSON") ||
        kw(&L->cur,"NEQJSON") || kw(&L->cur,"PLATENEQ") || kw(&L->cur,"JSONDIFF")){
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      char op[32];
      cubalc_host_result hr;
      int want_neq = 0;
      long hit;
      snprintf(op, sizeof op, "%s", L->cur.text);
      {
        char *q;
        for (q = op; *q; q++)
          if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      }
      if (strcmp(op, "JSONNEQ") == 0 || strcmp(op, "JNEQ") == 0 ||
          strcmp(op, "DIFFJSON") == 0 || strcmp(op, "NEQJSON") == 0 ||
          strcmp(op, "PLATENEQ") == 0 || strcmp(op, "JSONDIFF") == 0)
        want_neq = 1;
      lex_next(L);
      a[0] = 0;
      b[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0)
        b[0] = 0;
      if (!a[0] || !b[0]) {
        /* missing arg: soft miss */
        vm->last_n = want_neq ? 1 : 0;
        var_set_num(vm, "LAST_N", vm->last_n);
        var_set_num(vm, "JSONEQ_N", vm->last_n);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST_ERR", want_neq ? "JSONNEQ: need two plates" : "JSONEQ: need two plates");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_eq(a, b, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONEQ_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONEQ: fail");
          var_set_str(vm, "ERR", "JSONEQ: fail");
        }
        bump(vm); return 1;
      }
      hit = want_neq ? (hr.n ? 0L : 1L) : hr.n;
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "JSONEQ_N", hit);
      var_set_num(vm, "JSONNEQ_N", hit);
      /* keep first plate in LAST for chain */
      var_set_str(vm, "LAST", a);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", a);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys %s → %ld\n", want_neq ? "jsonneq" : "jsoneq", hit);
      bump(vm); return 1;
    }
    /* SYS JSONSUBSET|JSUBSET sub super
     * SYS JSONSUPERSET|JSUPERSET super sub  (or SUPERSET a b means a ⊇ b)
     * — every key in smaller plate matches larger raw value → LAST_N 0|1.
     * Empty {} is subset of any object. Soft non-object → 0.
     * Usability: required-field verify without multi JSONHAS+JSON peels. */
    if (kw(&L->cur,"JSONSUBSET") || kw(&L->cur,"JSUBSET") || kw(&L->cur,"SUBSETJSON") ||
        kw(&L->cur,"ISSUBSET") || kw(&L->cur,"PLATESUBSET") || kw(&L->cur,"JSONIN") ||
        kw(&L->cur,"JSONSUPERSET") || kw(&L->cur,"JSUPERSET") || kw(&L->cur,"SUPERSETJSON") ||
        kw(&L->cur,"ISSUPERSET") || kw(&L->cur,"PLATESUPERSET") || kw(&L->cur,"JSONCOVERS")){
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      char op[32];
      cubalc_host_result hr;
      int want_super = 0;
      const char *sub, *sup;
      long hit;
      snprintf(op, sizeof op, "%s", L->cur.text);
      {
        char *q;
        for (q = op; *q; q++)
          if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      }
      if (strcmp(op, "JSONSUPERSET") == 0 || strcmp(op, "JSUPERSET") == 0 ||
          strcmp(op, "SUPERSETJSON") == 0 || strcmp(op, "ISSUPERSET") == 0 ||
          strcmp(op, "PLATESUPERSET") == 0 || strcmp(op, "JSONCOVERS") == 0)
        want_super = 1;
      lex_next(L);
      a[0] = 0;
      b[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0)
        b[0] = 0;
      if (!a[0] || !b[0]) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONSUBSET_N", 0);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST_ERR", "JSONSUBSET: need sub super plates");
        bump(vm); return 1;
      }
      /* SUBSET sub super · SUPERSET a b means a ⊇ b i.e. subset(b,a) */
      if (want_super) {
        sub = b;
        sup = a;
      } else {
        sub = a;
        sup = b;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_subset(sub, sup, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONSUBSET_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONSUBSET: fail");
          var_set_str(vm, "ERR", "JSONSUBSET: fail");
        }
        bump(vm); return 1;
      }
      hit = hr.n;
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "JSONSUBSET_N", hit);
      var_set_num(vm, "JSONSUPERSET_N", hit);
      var_set_str(vm, "LAST", a);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", a);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys %s → %ld\n",
                want_super ? "jsonsuperset" : "jsonsubset", hit);
      bump(vm); return 1;
    }
    /* SYS JSONHASALL|JHASALL [plate] key [key…]
     * SYS JSONHASANY|JHASANY [plate] key [key…]
     * — multi-key presence → LAST_N 0|1 · JSONHASALL_HIT = found count.
     * plate first if looks like {…}; else plate=LAST. OF|KEYS bag after plate.
     * Null/false still present. Usability: contract keys without multi JSONHAS. */
    if (kw(&L->cur,"JSONHASALL") || kw(&L->cur,"JHASALL") || kw(&L->cur,"HASALLJSON") ||
        kw(&L->cur,"JSONNEED") || kw(&L->cur,"NEEDKEYS") || kw(&L->cur,"PLATEHASALL") ||
        kw(&L->cur,"JSONHASANY") || kw(&L->cur,"JHASANY") || kw(&L->cur,"HASANYJSON") ||
        kw(&L->cur,"JSONANYKEY") || kw(&L->cur,"PLATEHASANY") || kw(&L->cur,"HASSOMEJSON")){
      char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
      char arg[CUBALC_HOST_STR_MAX], op[32];
      int want_all = 1, have_plate = 0;
      size_t olen = 0;
      cubalc_host_result hr;
      long hit;
      snprintf(op, sizeof op, "%s", L->cur.text);
      {
        char *q;
        for (q = op; *q; q++)
          if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      }
      if (strcmp(op, "JSONHASANY") == 0 || strcmp(op, "JHASANY") == 0 ||
          strcmp(op, "HASANYJSON") == 0 || strcmp(op, "JSONANYKEY") == 0 ||
          strcmp(op, "PLATEHASANY") == 0 || strcmp(op, "HASSOMEJSON") == 0)
        want_all = 0;
      lex_next(L);
      plate[0] = 0;
      keys_nl[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          const char *s = arg;
          while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
          if (*s == '{') {
            snprintf(plate, sizeof plate, "%s", arg);
            have_plate = 1;
          } else {
            size_t al = strlen(arg);
            if (al + 1 < sizeof keys_nl) {
              memcpy(keys_nl, arg, al + 1);
              olen = al;
            }
          }
        }
      }
      if (have_plate && (kw(&L->cur,"OF") || kw(&L->cur,"KEYS") || kw(&L->cur,"FROM") ||
                         kw(&L->cur,"NEED") || kw(&L->cur,"REQUIRE"))) {
        lex_next(L);
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          snprintf(keys_nl, sizeof keys_nl, "%s", arg);
          olen = strlen(keys_nl);
        }
      } else {
        while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (L->cur.kind == TK_IDENT &&
              (kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
               kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
               kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
               kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"JSON") ||
               kw(&L->cur,"JSONSET") || kw(&L->cur,"JSONHAS") || kw(&L->cur,"JSONPICK"))) {
            break;
          }
          if (resolve_str_arg(vm, L, arg, sizeof arg) != 0)
            break;
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
      }
      if (!have_plate)
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_has_keys(plate, keys_nl, want_all, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONHASALL_N", 0);
        var_set_num(vm, "JSONHASALL_HIT", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONHASALL: fail");
          var_set_str(vm, "ERR", "JSONHASALL: fail");
        }
        bump(vm); return 1;
      }
      hit = hr.n;
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "JSONHASALL_N", hit);
      var_set_num(vm, "JSONHASANY_N", hit);
      var_set_num(vm, "JSONHASALL_HIT", (long)hr.code);
      var_set_num(vm, "JSONHASANY_HIT", (long)hr.code);
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys %s → hit=%ld found=%d\n",
                want_all ? "jsonhasall" : "jsonhasany", hit, hr.code);
      bump(vm); return 1;
    }
    /* SYS JSONHITKEY|JSONFIRST [plate] key [key…]
     * — first present candidate key → LAST=key · LAST_N 0|1 · JSONHITKEY_N=1-based index.
     * plate first if looks like {…}; else plate=LAST. OF|KEYS bag after plate.
     * Null/false still present. Usability: JSONHASANY twin that names WHICH key hit. */
    if (kw(&L->cur,"JSONHITKEY") || kw(&L->cur,"JSONFIRST") || kw(&L->cur,"JHITKEY") ||
        kw(&L->cur,"FIRSTJSONKEY") || kw(&L->cur,"HITJSONKEY") || kw(&L->cur,"JSONWHICH") ||
        kw(&L->cur,"WHICHJSONKEY") || kw(&L->cur,"PLATEHITKEY") || kw(&L->cur,"FIRSTKEY")){
      char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
      char arg[CUBALC_HOST_STR_MAX];
      int have_plate = 0;
      size_t olen = 0;
      cubalc_host_result hr;
      lex_next(L);
      plate[0] = 0;
      keys_nl[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          const char *s = arg;
          while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
          if (*s == '{') {
            snprintf(plate, sizeof plate, "%s", arg);
            have_plate = 1;
          } else {
            size_t al = strlen(arg);
            if (al + 1 < sizeof keys_nl) {
              memcpy(keys_nl, arg, al + 1);
              olen = al;
            }
          }
        }
      }
      if (have_plate && (kw(&L->cur,"OF") || kw(&L->cur,"KEYS") || kw(&L->cur,"FROM") ||
                         kw(&L->cur,"NEED") || kw(&L->cur,"REQUIRE"))) {
        lex_next(L);
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          snprintf(keys_nl, sizeof keys_nl, "%s", arg);
          olen = strlen(keys_nl);
        }
      } else {
        while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (L->cur.kind == TK_IDENT &&
              (kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
               kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
               kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
               kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"JSON") ||
               kw(&L->cur,"JSONSET") || kw(&L->cur,"JSONHAS") || kw(&L->cur,"JSONPICK") ||
               kw(&L->cur,"JSONHASALL") || kw(&L->cur,"JSONMISS") || kw(&L->cur,"JSONPRESENT") ||
               kw(&L->cur,"JSONHITKEY") || kw(&L->cur,"JSONFIRST"))) {
            break;
          }
          if (resolve_str_arg(vm, L, arg, sizeof arg) != 0)
            break;
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
      }
      if (!have_plate)
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_first_key(plate, keys_nl, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONHITKEY_N", 0);
        var_set_num(vm, "JSONHITKEY_HIT", 0);
        var_set_num(vm, "JSONFIRST_N", 0);
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONHITKEY: fail");
          var_set_str(vm, "ERR", "JSONHITKEY: fail");
        }
        bump(vm); return 1;
      }
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "JSONHITKEY_N", (long)hr.code);
      var_set_num(vm, "JSONHITKEY_HIT", hr.n);
      var_set_num(vm, "JSONFIRST_N", (long)hr.code);
      var_set_num(vm, "JSONFIRST_HIT", hr.n);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonhitkey → hit=%ld idx=%d key=%s\n",
                hr.n, hr.code, hr.str);
      bump(vm); return 1;
    }
    /* SYS JSONHITVAL|JSONFIRSTVAL [plate] key [key…]
     * — first present candidate VALUE → LAST=value · LAST_N 0|1 · JSONHITVAL_N=1-based index.
     * JSONHITVAL_KEY / JSONFIRSTVAL_KEY = key name on hit. plate first if {…}; else plate=LAST.
     * OF|KEYS bag after plate. Null/false/0 still present; empty string value is a hit.
     * Usability: JSONHITKEY twin that peels VALUE without multi JSON+IF glue. */
    if (kw(&L->cur,"JSONHITVAL") || kw(&L->cur,"JSONFIRSTVAL") || kw(&L->cur,"JHITVAL") ||
        kw(&L->cur,"FIRSTJSONVAL") || kw(&L->cur,"HITJSONVAL") || kw(&L->cur,"JSONWHICHVAL") ||
        kw(&L->cur,"WHICHJSONVAL") || kw(&L->cur,"PLATEHITVAL") || kw(&L->cur,"FIRSTVAL") ||
        kw(&L->cur,"JSONVAL") || kw(&L->cur,"HITVAL") || kw(&L->cur,"FIRSTJSONVALUE") ||
        kw(&L->cur,"JSONFIRSTVALUE")){
      char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
      char arg[CUBALC_HOST_STR_MAX];
      int have_plate = 0;
      size_t olen = 0;
      cubalc_host_result hr;
      lex_next(L);
      plate[0] = 0;
      keys_nl[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          const char *s0 = arg;
          while (*s0 == ' ' || *s0 == '\t' || *s0 == '\n' || *s0 == '\r') s0++;
          if (*s0 == '{') {
            snprintf(plate, sizeof plate, "%s", arg);
            have_plate = 1;
          } else {
            size_t al = strlen(arg);
            if (al + 1 < sizeof keys_nl) {
              memcpy(keys_nl, arg, al + 1);
              olen = al;
            }
          }
        }
      }
      if (have_plate && (kw(&L->cur,"OF") || kw(&L->cur,"KEYS") || kw(&L->cur,"FROM") ||
                         kw(&L->cur,"NEED") || kw(&L->cur,"REQUIRE"))) {
        lex_next(L);
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          snprintf(keys_nl, sizeof keys_nl, "%s", arg);
          olen = strlen(keys_nl);
        }
      } else {
        while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (L->cur.kind == TK_IDENT &&
              (kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
               kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
               kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
               kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"JSON") ||
               kw(&L->cur,"JSONSET") || kw(&L->cur,"JSONHAS") || kw(&L->cur,"JSONPICK") ||
               kw(&L->cur,"JSONHASALL") || kw(&L->cur,"JSONMISS") || kw(&L->cur,"JSONPRESENT") ||
               kw(&L->cur,"JSONHITKEY") || kw(&L->cur,"JSONFIRST") ||
               kw(&L->cur,"JSONHITVAL") || kw(&L->cur,"JSONFIRSTVAL"))) {
            break;
          }
          if (resolve_str_arg(vm, L, arg, sizeof arg) != 0)
            break;
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
      }
      if (!have_plate)
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_first_val(plate, keys_nl, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONHITVAL_N", 0);
        var_set_num(vm, "JSONHITVAL_HIT", 0);
        var_set_num(vm, "JSONFIRSTVAL_N", 0);
        var_set_str(vm, "JSONHITVAL_KEY", "");
        var_set_str(vm, "JSONFIRSTVAL_KEY", "");
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONHITVAL: fail");
          var_set_str(vm, "ERR", "JSONHITVAL: fail");
        }
        bump(vm); return 1;
      }
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "JSONHITVAL_N", (long)hr.code);
      var_set_num(vm, "JSONHITVAL_HIT", hr.n);
      var_set_num(vm, "JSONFIRSTVAL_N", (long)hr.code);
      var_set_num(vm, "JSONFIRSTVAL_HIT", hr.n);
      var_set_str(vm, "JSONHITVAL_KEY", hr.err);
      var_set_str(vm, "JSONFIRSTVAL_KEY", hr.err);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonhitval → hit=%ld idx=%d key=%s val=%s\n",
                hr.n, hr.code, hr.err, hr.str);
      bump(vm); return 1;
    }
    /* SYS JSONCOALESCE|JCOALESCE|JSONNVL [plate] key [key…] [OR|DEFAULT def]
     * — first non-empty present candidate VALUE → LAST · LAST_N 0|1 · JSONCOALESCE_N=1-based.
     * JSONCOALESCE_KEY = key name on hit. OR|DEFAULT def after keys (or ELSEVAL|FALLBACK).
     * plate first if {…}; else plate=LAST. OF|KEYS bag after plate.
     * Skips missing keys and empty/blank values (null→"" skipped). Soft dual of JSONHITVAL
     * that prefers substance + optional default without IF/COALESCE glue. */
    if (kw(&L->cur,"JSONCOALESCE") || kw(&L->cur,"JCOALESCE") || kw(&L->cur,"JSONNVL") ||
        kw(&L->cur,"JNVL") || kw(&L->cur,"COALESCEJSON") || kw(&L->cur,"JSONOR") ||
        kw(&L->cur,"JSONDEFAULTVAL") || kw(&L->cur,"JDEFAULTVAL") ||
        kw(&L->cur,"COALESCEVAL") || kw(&L->cur,"JSONFALLBACK") ||
        kw(&L->cur,"JFALLBACK") || kw(&L->cur,"JSONPICKFIRST")){
      char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
      char arg[CUBALC_HOST_STR_MAX], deflt[CUBALC_HOST_STR_MAX];
      int have_plate = 0;
      size_t olen = 0;
      cubalc_host_result hr;
      lex_next(L);
      plate[0] = 0;
      keys_nl[0] = 0;
      deflt[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          const char *s0 = arg;
          while (*s0 == ' ' || *s0 == '\t' || *s0 == '\n' || *s0 == '\r') s0++;
          if (*s0 == '{') {
            snprintf(plate, sizeof plate, "%s", arg);
            have_plate = 1;
          } else {
            size_t al = strlen(arg);
            if (al + 1 < sizeof keys_nl) {
              memcpy(keys_nl, arg, al + 1);
              olen = al;
            }
          }
        }
      }
      if (have_plate && (kw(&L->cur,"OF") || kw(&L->cur,"KEYS") || kw(&L->cur,"FROM") ||
                         kw(&L->cur,"NEED") || kw(&L->cur,"REQUIRE"))) {
        lex_next(L);
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          snprintf(keys_nl, sizeof keys_nl, "%s", arg);
          olen = strlen(keys_nl);
        }
      } else {
        while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (L->cur.kind == TK_IDENT &&
              (kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
               kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
               kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
               kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"JSON") ||
               kw(&L->cur,"JSONSET") || kw(&L->cur,"JSONHAS") || kw(&L->cur,"JSONPICK") ||
               kw(&L->cur,"JSONHASALL") || kw(&L->cur,"JSONMISS") || kw(&L->cur,"JSONPRESENT") ||
               kw(&L->cur,"JSONHITKEY") || kw(&L->cur,"JSONFIRST") ||
               kw(&L->cur,"JSONHITVAL") || kw(&L->cur,"JSONFIRSTVAL") ||
               kw(&L->cur,"JSONCOALESCE") || kw(&L->cur,"JCOALESCE") ||
               kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSEVAL") ||
               kw(&L->cur,"FALLBACK") || kw(&L->cur,"DEF"))) {
            break;
          }
          if (resolve_str_arg(vm, L, arg, sizeof arg) != 0)
            break;
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
      }
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSEVAL") ||
          kw(&L->cur,"FALLBACK") || kw(&L->cur,"DEF") || kw(&L->cur,"WITH") ||
          kw(&L->cur,"AS")) {
        lex_next(L);
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0)
          snprintf(deflt, sizeof deflt, "%s", arg);
      }
      if (!have_plate)
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_coalesce(plate, keys_nl, deflt, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONCOALESCE_N", 0);
        var_set_num(vm, "JSONCOALESCE_HIT", 0);
        var_set_str(vm, "JSONCOALESCE_KEY", "");
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONCOALESCE: fail");
          var_set_str(vm, "ERR", "JSONCOALESCE: fail");
        }
        bump(vm); return 1;
      }
      /* hit: n=1 key present non-empty; n=0 with str = default used; n=0 empty = total miss */
      if (hr.n) {
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_num(vm, "JSONCOALESCE_HIT", 1);
      } else if (hr.str[0]) {
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_num(vm, "JSONCOALESCE_HIT", 0);
      } else {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONCOALESCE_HIT", 0);
      }
      var_set_num(vm, "JSONCOALESCE_N", (long)hr.code);
      var_set_num(vm, "JSONNVL_N", (long)hr.code);
      var_set_str(vm, "JSONCOALESCE_KEY", hr.err);
      var_set_str(vm, "JSONNVL_KEY", hr.err);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsoncoalesce → hit=%ld idx=%d key=%s val=%s\n",
                hr.n, hr.code, hr.err, hr.str);
      bump(vm); return 1;
    }
    /* SYS JSONMISS|JSONMISSING [plate] key [key…]
     * SYS JSONPRESENT|JSONFOUND [plate] key [key…]
     * — bag of missing/present required keys → LAST · LAST_N=count.
     * plate first if looks like {…}; else plate=LAST. OF|KEYS bag after plate.
     * Null/false still present. Usability: after HASALL fail, name missing keys. */
    if (kw(&L->cur,"JSONMISS") || kw(&L->cur,"JMISS") || kw(&L->cur,"JSONMISSING") ||
        kw(&L->cur,"MISSINGKEYS") || kw(&L->cur,"PLATEMISS") || kw(&L->cur,"JSONABSENT") ||
        kw(&L->cur,"JSONPRESENT") || kw(&L->cur,"JPRESENT") || kw(&L->cur,"JSONFOUND") ||
        kw(&L->cur,"PRESENTKEYS") || kw(&L->cur,"PLATEFOUND") || kw(&L->cur,"JFOUND")){
      char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
      char arg[CUBALC_HOST_STR_MAX], op[32];
      int want_present = 0, have_plate = 0;
      size_t olen = 0;
      cubalc_host_result hr;
      snprintf(op, sizeof op, "%s", L->cur.text);
      {
        char *q;
        for (q = op; *q; q++)
          if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      }
      if (strcmp(op, "JSONPRESENT") == 0 || strcmp(op, "JPRESENT") == 0 ||
          strcmp(op, "JSONFOUND") == 0 || strcmp(op, "PRESENTKEYS") == 0 ||
          strcmp(op, "PLATEFOUND") == 0 || strcmp(op, "JFOUND") == 0)
        want_present = 1;
      lex_next(L);
      plate[0] = 0;
      keys_nl[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          const char *s = arg;
          while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
          if (*s == '{') {
            snprintf(plate, sizeof plate, "%s", arg);
            have_plate = 1;
          } else {
            size_t al = strlen(arg);
            if (al + 1 < sizeof keys_nl) {
              memcpy(keys_nl, arg, al + 1);
              olen = al;
            }
          }
        }
      }
      if (have_plate && (kw(&L->cur,"OF") || kw(&L->cur,"KEYS") || kw(&L->cur,"FROM") ||
                         kw(&L->cur,"NEED") || kw(&L->cur,"REQUIRE"))) {
        lex_next(L);
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          snprintf(keys_nl, sizeof keys_nl, "%s", arg);
          olen = strlen(keys_nl);
        }
      } else {
        while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (L->cur.kind == TK_IDENT &&
              (kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
               kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
               kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
               kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"JSON") ||
               kw(&L->cur,"JSONSET") || kw(&L->cur,"JSONHAS") || kw(&L->cur,"JSONPICK") ||
               kw(&L->cur,"JSONHASALL") || kw(&L->cur,"JSONMISS") || kw(&L->cur,"JSONPRESENT"))) {
            break;
          }
          if (resolve_str_arg(vm, L, arg, sizeof arg) != 0)
            break;
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
      }
      if (!have_plate)
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_filter_req_keys(plate, keys_nl, want_present, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        if (want_present) {
          var_set_num(vm, "JSONPRESENT_N", 0);
          var_set_num(vm, "JSONFOUND_N", 0);
        } else {
          var_set_num(vm, "JSONMISS_N", 0);
          var_set_num(vm, "JSONMISSING_N", 0);
        }
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", want_present ? "JSONPRESENT: fail" : "JSONMISS: fail");
          var_set_str(vm, "ERR", want_present ? "JSONPRESENT: fail" : "JSONMISS: fail");
        }
        bump(vm); return 1;
      }
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      if (want_present) {
        var_set_num(vm, "JSONPRESENT_N", hr.n);
        var_set_num(vm, "JSONFOUND_N", hr.n);
      } else {
        var_set_num(vm, "JSONMISS_N", hr.n);
        var_set_num(vm, "JSONMISSING_N", hr.n);
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys %s → n=%ld\n",
                want_present ? "jsonpresent" : "jsonmiss", hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONEXTRA|JSONUNKNOWN [plate] allowed_key [key…]
     * SYS JSONKNOWN|JSONALLOWED [plate] allowed_key [key…]
     * — bag of plate keys outside/inside allow-list → LAST · LAST_N=count.
     * plate first if looks like {…}; else plate=LAST. OF|KEYS bag after plate.
     * Empty allow-list: EXTRA = all plate keys; KNOWN = empty.
     * Usability: schema reject unknown fields without JSONKEYS+EACH+HASLINE. */
    if (kw(&L->cur,"JSONEXTRA") || kw(&L->cur,"JEXTRA") || kw(&L->cur,"JSONUNKNOWN") ||
        kw(&L->cur,"UNKNOWNKEYS") || kw(&L->cur,"PLATEEXTRA") || kw(&L->cur,"EXTRAKEYS") ||
        kw(&L->cur,"JSONKNOWN") || kw(&L->cur,"JKNOWN") || kw(&L->cur,"JSONALLOWED") ||
        kw(&L->cur,"ALLOWEDKEYS") || kw(&L->cur,"PLATEKNOWN") || kw(&L->cur,"KNOWNKEYS")){
      char plate[CUBALC_HOST_STR_MAX], allowed_nl[CUBALC_HOST_STR_MAX];
      char arg[CUBALC_HOST_STR_MAX], op[32];
      int want_extra = 1, have_plate = 0;
      size_t olen = 0;
      cubalc_host_result hr;
      snprintf(op, sizeof op, "%s", L->cur.text);
      {
        char *q;
        for (q = op; *q; q++)
          if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      }
      if (strcmp(op, "JSONKNOWN") == 0 || strcmp(op, "JKNOWN") == 0 ||
          strcmp(op, "JSONALLOWED") == 0 || strcmp(op, "ALLOWEDKEYS") == 0 ||
          strcmp(op, "PLATEKNOWN") == 0 || strcmp(op, "KNOWNKEYS") == 0)
        want_extra = 0;
      lex_next(L);
      plate[0] = 0;
      allowed_nl[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          const char *s = arg;
          while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
          if (*s == '{') {
            snprintf(plate, sizeof plate, "%s", arg);
            have_plate = 1;
          } else {
            size_t al = strlen(arg);
            if (al + 1 < sizeof allowed_nl) {
              memcpy(allowed_nl, arg, al + 1);
              olen = al;
            }
          }
        }
      }
      if (have_plate && (kw(&L->cur,"OF") || kw(&L->cur,"KEYS") || kw(&L->cur,"FROM") ||
                         kw(&L->cur,"ALLOW") || kw(&L->cur,"ALLOWED") ||
                         kw(&L->cur,"NEED") || kw(&L->cur,"REQUIRE"))) {
        lex_next(L);
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          snprintf(allowed_nl, sizeof allowed_nl, "%s", arg);
          olen = strlen(allowed_nl);
        }
      } else {
        while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (L->cur.kind == TK_IDENT &&
              (kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
               kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
               kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
               kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"JSON") ||
               kw(&L->cur,"JSONSET") || kw(&L->cur,"JSONHAS") || kw(&L->cur,"JSONPICK") ||
               kw(&L->cur,"JSONHASALL") || kw(&L->cur,"JSONMISS") ||
               kw(&L->cur,"JSONPRESENT") || kw(&L->cur,"JSONEXTRA") ||
               kw(&L->cur,"JSONKNOWN"))) {
            break;
          }
          if (resolve_str_arg(vm, L, arg, sizeof arg) != 0)
            break;
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
      }
      if (!have_plate)
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_filter_plate_keys(plate, allowed_nl, want_extra, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        if (want_extra) {
          var_set_num(vm, "JSONEXTRA_N", 0);
          var_set_num(vm, "JSONUNKNOWN_N", 0);
        } else {
          var_set_num(vm, "JSONKNOWN_N", 0);
          var_set_num(vm, "JSONALLOWED_N", 0);
        }
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", want_extra ? "JSONEXTRA: fail" : "JSONKNOWN: fail");
          var_set_str(vm, "ERR", want_extra ? "JSONEXTRA: fail" : "JSONKNOWN: fail");
        }
        bump(vm); return 1;
      }
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      if (want_extra) {
        var_set_num(vm, "JSONEXTRA_N", hr.n);
        var_set_num(vm, "JSONUNKNOWN_N", hr.n);
      } else {
        var_set_num(vm, "JSONKNOWN_N", hr.n);
        var_set_num(vm, "JSONALLOWED_N", hr.n);
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys %s → n=%ld\n",
                want_extra ? "jsonextra" : "jsonknown", hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONPLUCK|JSONGETALL [plate] key [key…]
     * — multi-key peel → newline bag of values (empty field on miss).
     * LAST_N = listed keys · JSONPLUCK_HIT = found. OF|KEYS bag after plate.
     * plate first if looks like {…}; else plate=LAST.
     * Usability: multi-field extract without multi JSON+PUSH glue. */
    if (kw(&L->cur,"JSONPLUCK") || kw(&L->cur,"JPLUCK") || kw(&L->cur,"JSONGETALL") ||
        kw(&L->cur,"JSONGETS") || kw(&L->cur,"JGETALL") || kw(&L->cur,"PLUCKJSON") ||
        kw(&L->cur,"JSONMULTI") || kw(&L->cur,"GETALLJSON") || kw(&L->cur,"PLATEPLUCK")){
      char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
      char arg[CUBALC_HOST_STR_MAX];
      int have_plate = 0;
      size_t olen = 0;
      cubalc_host_result hr;
      lex_next(L);
      plate[0] = 0;
      keys_nl[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          const char *s = arg;
          while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
          if (*s == '{') {
            snprintf(plate, sizeof plate, "%s", arg);
            have_plate = 1;
          } else {
            size_t al = strlen(arg);
            if (al + 1 < sizeof keys_nl) {
              memcpy(keys_nl, arg, al + 1);
              olen = al;
            }
          }
        }
      }
      if (have_plate && (kw(&L->cur,"OF") || kw(&L->cur,"KEYS") || kw(&L->cur,"FROM") ||
                         kw(&L->cur,"NEED") || kw(&L->cur,"REQUIRE"))) {
        lex_next(L);
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          snprintf(keys_nl, sizeof keys_nl, "%s", arg);
          olen = strlen(keys_nl);
        }
      } else {
        while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (L->cur.kind == TK_IDENT &&
              (kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
               kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
               kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
               kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"JSON") ||
               kw(&L->cur,"JSONSET") || kw(&L->cur,"JSONHAS") || kw(&L->cur,"JSONPICK") ||
               kw(&L->cur,"JSONHASALL") || kw(&L->cur,"JSONMISS") ||
               kw(&L->cur,"JSONPRESENT") || kw(&L->cur,"JSONEXTRA") ||
               kw(&L->cur,"JSONKNOWN") || kw(&L->cur,"JSONPLUCK"))) {
            break;
          }
          if (resolve_str_arg(vm, L, arg, sizeof arg) != 0)
            break;
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
      }
      if (!have_plate)
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_pluck(plate, keys_nl, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONPLUCK_N", 0);
        var_set_num(vm, "JSONPLUCK_HIT", 0);
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONPLUCK: fail");
          var_set_str(vm, "ERR", "JSONPLUCK: fail");
        }
        bump(vm); return 1;
      }
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "JSONPLUCK_N", hr.n);
      var_set_num(vm, "JSONGETALL_N", hr.n);
      var_set_num(vm, "JSONPLUCK_HIT", (long)hr.code);
      var_set_num(vm, "JSONGETALL_HIT", (long)hr.code);
      var_set_num(vm, "JSONPLUCK_MISS", hr.n - (long)hr.code);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonpluck → listed=%ld hit=%d\n", hr.n, hr.code);
      bump(vm); return 1;
    }
    /* SYS JSONEXACT|JSONSCHEMA [plate] key [key…]
     * — exact key-set match: all listed present AND no extras → LAST_N 0|1.
     * LAST kept as plate. JSONEXACT_HIT=found · JSONEXACT_EXTRA=unknown count.
     * OF|KEYS bag after plate. Soft twin of REQUIRE JSONEXACT.
     * Usability: one-shot schema probe without HASALL+EXTRA dual IF. */
    if (kw(&L->cur,"JSONEXACT") || kw(&L->cur,"JEXACT") || kw(&L->cur,"EXACTKEYS") ||
        kw(&L->cur,"JSONSCHEMA") || kw(&L->cur,"JSCHEMA") || kw(&L->cur,"PLATEEXACT") ||
        kw(&L->cur,"KEYSEXACT") || kw(&L->cur,"SAMEKEYS")){
      char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
      char arg[CUBALC_HOST_STR_MAX];
      int have_plate = 0;
      size_t olen = 0;
      cubalc_host_result ha, ex;
      long hit = 0;
      lex_next(L);
      plate[0] = 0;
      keys_nl[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          const char *s = arg;
          while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
          if (*s == '{') {
            snprintf(plate, sizeof plate, "%s", arg);
            have_plate = 1;
          } else {
            size_t al = strlen(arg);
            if (al + 1 < sizeof keys_nl) {
              memcpy(keys_nl, arg, al + 1);
              olen = al;
            }
          }
        }
      }
      if (have_plate && (kw(&L->cur,"OF") || kw(&L->cur,"KEYS") || kw(&L->cur,"FROM") ||
                         kw(&L->cur,"NEED") || kw(&L->cur,"REQUIRE") ||
                         kw(&L->cur,"ALLOW") || kw(&L->cur,"SCHEMA"))) {
        lex_next(L);
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          snprintf(keys_nl, sizeof keys_nl, "%s", arg);
          olen = strlen(keys_nl);
        }
      } else {
        while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (L->cur.kind == TK_IDENT &&
              (kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
               kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
               kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
               kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"JSON") ||
               kw(&L->cur,"JSONSET") || kw(&L->cur,"JSONHAS") || kw(&L->cur,"JSONPICK") ||
               kw(&L->cur,"JSONHASALL") || kw(&L->cur,"JSONMISS") ||
               kw(&L->cur,"JSONEXTRA") || kw(&L->cur,"JSONPLUCK") ||
               kw(&L->cur,"JSONEXACT"))) {
            break;
          }
          if (resolve_str_arg(vm, L, arg, sizeof arg) != 0)
            break;
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
      }
      if (!have_plate)
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&ha, 0, sizeof ha);
      memset(&ex, 0, sizeof ex);
      cubalc_host_json_has_keys(plate, keys_nl, 1, &ha);
      cubalc_host_json_filter_plate_keys(plate, keys_nl, 1, &ex);
      hit = (ha.n == 1 && ex.n == 0) ? 1 : 0;
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "JSONEXACT_N", hit);
      var_set_num(vm, "JSONSCHEMA_N", hit);
      var_set_num(vm, "JSONEXACT_HIT", (long)ha.code);
      var_set_num(vm, "JSONEXACT_EXTRA", ex.n);
      {
        /* miss count = listed - found */
        long listed = 0;
        const char *p = keys_nl;
        while (*p) {
          while (*p == '\n' || *p == '\r') p++;
          if (!*p) break;
          listed++;
          while (*p && *p != '\n' && *p != '\r') p++;
        }
        var_set_num(vm, "JSONEXACT_MISS", listed - (long)ha.code);
        var_set_num(vm, "JSONEXACT_LISTED", listed);
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonexact → hit=%ld found=%d extra=%ld\n",
                hit, ha.code, ex.n);
      bump(vm); return 1;
    }
    /* SYS JSONKEYDIFF|KEYSDIFF a b — keys in a not in b → bag · LAST_N=count
     * SYS JSONKEYCOMM|KEYSINTER|JSONCOMMON a b — key intersection bag
     * (not JSONDIFF which is JSONNEQ alias). Soft empty non-object.
     * Usability: plate key delta without JSONKEYS+DIFF/INTERSECT glue. */
    if (kw(&L->cur,"JSONKEYDIFF") || kw(&L->cur,"KEYSDIFF") || kw(&L->cur,"KEYDIFF") ||
        kw(&L->cur,"JSONONLYA") || kw(&L->cur,"ONLYKEYS") || kw(&L->cur,"KEYSONLYA") ||
        kw(&L->cur,"JSONKEYCOMM") || kw(&L->cur,"KEYSINTER") || kw(&L->cur,"JSONCOMMON") ||
        kw(&L->cur,"KEYCOMMON") || kw(&L->cur,"JSONKEYAND") || kw(&L->cur,"KEYINTERSECT")){
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX], op[32];
      int want_inter = 0;
      cubalc_host_result hr;
      snprintf(op, sizeof op, "%s", L->cur.text);
      {
        char *q;
        for (q = op; *q; q++)
          if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      }
      if (strcmp(op, "JSONKEYCOMM") == 0 || strcmp(op, "KEYSINTER") == 0 ||
          strcmp(op, "JSONCOMMON") == 0 || strcmp(op, "KEYCOMMON") == 0 ||
          strcmp(op, "JSONKEYAND") == 0 || strcmp(op, "KEYINTERSECT") == 0)
        want_inter = 1;
      lex_next(L);
      a[0] = 0;
      b[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0)
        b[0] = 0;
      if (!a[0])
        snprintf(a, sizeof a, "%s", "{}");
      if (!b[0])
        snprintf(b, sizeof b, "%s", "{}");
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_key_set_op(a, b, want_inter, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONKEYDIFF_N", 0);
        var_set_num(vm, "JSONKEYCOMM_N", 0);
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR",
                      want_inter ? "JSONKEYCOMM: fail" : "JSONKEYDIFF: fail");
          var_set_str(vm, "ERR",
                      want_inter ? "JSONKEYCOMM: fail" : "JSONKEYDIFF: fail");
        }
        bump(vm); return 1;
      }
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      if (want_inter) {
        var_set_num(vm, "JSONKEYCOMM_N", hr.n);
        var_set_num(vm, "KEYSINTER_N", hr.n);
      } else {
        var_set_num(vm, "JSONKEYDIFF_N", hr.n);
        var_set_num(vm, "KEYSDIFF_N", hr.n);
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys %s → n=%ld\n",
                want_inter ? "jsonkeycomm" : "jsonkeydiff", hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONCHANGED|JSONNEQKEYS a b — keys that differ (only-a/only-b/value≠)
     * SYS JSONSTABLE|JSONEQKEYS a b — keys same in both (raw value match)
     * LAST = bag · LAST_N=count. Soft empty non-object.
     * Usability: plate sync delta without multi JSONKEYS+JSON peels. */
    if (kw(&L->cur,"JSONCHANGED") || kw(&L->cur,"JCHANGED") || kw(&L->cur,"JSONNEQKEYS") ||
        kw(&L->cur,"CHANGEDKEYS") || kw(&L->cur,"PLATECHANGED") ||
        kw(&L->cur,"JSONSTABLE") || kw(&L->cur,"JSTABLE") || kw(&L->cur,"JSONEQKEYS") ||
        kw(&L->cur,"STABLEKEYS") || kw(&L->cur,"PLATESTABLE") || kw(&L->cur,"JSONUNCHANGED")){
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX], op[32];
      int want_same = 0;
      cubalc_host_result hr;
      snprintf(op, sizeof op, "%s", L->cur.text);
      {
        char *q;
        for (q = op; *q; q++)
          if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      }
      if (strcmp(op, "JSONSTABLE") == 0 || strcmp(op, "JSTABLE") == 0 ||
          strcmp(op, "JSONEQKEYS") == 0 || strcmp(op, "STABLEKEYS") == 0 ||
          strcmp(op, "PLATESTABLE") == 0 || strcmp(op, "JSONUNCHANGED") == 0)
        want_same = 1;
      lex_next(L);
      a[0] = 0;
      b[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0)
        b[0] = 0;
      if (!a[0])
        snprintf(a, sizeof a, "%s", "{}");
      if (!b[0])
        snprintf(b, sizeof b, "%s", "{}");
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_changed_keys(a, b, want_same, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONCHANGED_N", 0);
        var_set_num(vm, "JSONSTABLE_N", 0);
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR",
                      want_same ? "JSONSTABLE: fail" : "JSONCHANGED: fail");
          var_set_str(vm, "ERR",
                      want_same ? "JSONSTABLE: fail" : "JSONCHANGED: fail");
        }
        bump(vm); return 1;
      }
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      if (want_same) {
        var_set_num(vm, "JSONSTABLE_N", hr.n);
        var_set_num(vm, "JSONEQKEYS_N", hr.n);
      } else {
        var_set_num(vm, "JSONCHANGED_N", hr.n);
        var_set_num(vm, "JSONNEQKEYS_N", hr.n);
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys %s → n=%ld\n",
                want_same ? "jsonstable" : "jsonchanged", hr.n);
      bump(vm); return 1;
    }
    /* SYS JSONDELTA|JDELTA|DELTAOBJ a b [FROM OLD|NEW]
     * — plate object of changed keys; values from b/new by default (FROM OLD → a).
     * LAST = plate · LAST_N = key count. Soft {} if identical.
     * Usability: sync payload without CHANGED+EACH+JSONSET glue. */
    if (kw(&L->cur,"JSONDELTA") || kw(&L->cur,"JDELTA") || kw(&L->cur,"DELTAOBJ") ||
        kw(&L->cur,"CHANGEDOBJ") || kw(&L->cur,"JSONDELTAOBJ") || kw(&L->cur,"PLATEDELTA") ||
        kw(&L->cur,"JSONDIFFOBJ") || kw(&L->cur,"DELTAPLATE")){
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      int prefer_b = 1;
      cubalc_host_result hr;
      lex_next(L);
      a[0] = 0;
      b[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0)
        b[0] = 0;
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USE") || kw(&L->cur,"VALUES")) {
        lex_next(L);
        if (kw(&L->cur,"OLD") || kw(&L->cur,"LEFT") || kw(&L->cur,"A") ||
            kw(&L->cur,"BASE") || kw(&L->cur,"PREV")) {
          prefer_b = 0;
          lex_next(L);
        } else if (kw(&L->cur,"NEW") || kw(&L->cur,"RIGHT") || kw(&L->cur,"B") ||
                   kw(&L->cur,"CUR") || kw(&L->cur,"NEXT")) {
          prefer_b = 1;
          lex_next(L);
        }
      }
      if (!a[0])
        snprintf(a, sizeof a, "%s", "{}");
      if (!b[0])
        snprintf(b, sizeof b, "%s", "{}");
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_delta_obj(a, b, prefer_b, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONDELTA_N", 0);
        var_set_str(vm, "LAST", "{}");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONDELTA: fail");
          var_set_str(vm, "ERR", "JSONDELTA: fail");
        }
        bump(vm); return 1;
      }
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "JSONDELTA_N", hr.n);
      var_set_num(vm, "DELTAOBJ_N", hr.n);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsondelta → n=%ld prefer_b=%d\n", hr.n, prefer_b);
      bump(vm); return 1;
    }

    /* Complements JSONCHANGED (names) and JSONDELTA (new values only). */
    if (kw(&L->cur,"JSONCHANGELOG") || kw(&L->cur,"JCHANGELOG") ||
        kw(&L->cur,"PLATECHANGELOG") || kw(&L->cur,"JSONCHANGES") ||
        kw(&L->cur,"CHANGELOG") || kw(&L->cur,"PLATELOG") ||
        kw(&L->cur,"JSONUD") || kw(&L->cur,"DIFFLINES") ||
        kw(&L->cur,"JSONDIFFLINES") || kw(&L->cur,"JDIFFLINES")){
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      lex_next(L);
      a[0] = 0;
      b[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0)
        b[0] = 0;
      if (!a[0])
        snprintf(a, sizeof a, "%s", "{}");
      if (!b[0])
        snprintf(b, sizeof b, "%s", "{}");
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_changelog(a, b, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONCHANGELOG_N", 0);
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONCHANGELOG: fail");
          var_set_str(vm, "ERR", "JSONCHANGELOG: fail");
        }
        bump(vm); return 1;
      }
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "JSONCHANGELOG_N", hr.n);
      var_set_num(vm, "JCHANGELOG_N", hr.n);
      var_set_num(vm, "JSONCHANGED_N", hr.n);
      var_set_str(vm, "JSONCHANGELOG", hr.str);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonchangelog → n=%ld\n", hr.n);
      bump(vm); return 1;
    }
  return 0;
}
