/* CubalC lang — lang_ops_sys_misc.c
 * SYS residual: SPAWN · CHAT · NUM/STR · LASTMATCH
 * Peel from core_sys (B_core_sys). Leaves pure dispatcher. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_misc.h"
#include "lang/cubalc_lang_internal.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

int cubalc_lang_ops_sys_try_misc(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
  if (kw(&L->cur,"SPAWN")){
    lex_next(L);
    if (L->cur.kind!=TK_STR && L->cur.kind!=TK_IDENT){ fail(vm,"SYS SPAWN bin [args…]"); return -1; }
    char bin[512];
    if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
      /* if not path, which */
      if (strchr(L->cur.text, '/')) snprintf(bin,sizeof bin,"%s",L->cur.text);
      else {
        cubalc_host_result wh;
        if (cubalc_host_which(L->cur.text, &wh)==0) snprintf(bin,sizeof bin,"%s",wh.str);
        else snprintf(bin,sizeof bin,"%s",L->cur.text);
      }
      lex_next(L);
    }
    char *av[16]; char abuf[16][256]; int ac=0;
    av[ac++] = bin;
    while (ac < 15 && (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT || L->cur.kind==TK_NUM)){
      snprintf(abuf[ac], sizeof abuf[ac], "%s", L->cur.text);
      av[ac] = abuf[ac];
      ac++; lex_next(L);
    }
    av[ac] = NULL;
    cubalc_host_result hr;
    cubalc_host_spawn(bin, av, &hr);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    var_set_str(vm, "LAST", hr.str);
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "EXIT", hr.code);
    var_set_num(vm, "OK", hr.ok ? 1 : 0);
    bump(vm); return 1;
  }

  /* SYS CHAT "local"|"grok" ["model"] — msg from GROKIUM_MSG / CUBALC_MSG env */
  if (kw(&L->cur,"CHAT") || kw(&L->cur,"ASK")){
    lex_next(L);
    char be[32]="local", model[128]="";
    if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
      snprintf(be,sizeof be,"%s",L->cur.text); lex_next(L);
    }
    if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
      /* model or message string */
      if (strcmp(L->cur.text,"local")==0 || strcmp(L->cur.text,"grok")==0 ||
          strncmp(L->cur.text,"grok",4)==0 || strstr(L->cur.text,"/") ||
          strstr(L->cur.text,".gguf")) {
        snprintf(model,sizeof model,"%s",L->cur.text); lex_next(L);
      }
    }
    char msg[2000]; msg[0]=0;
    if (L->cur.kind==TK_STR){
      snprintf(msg,sizeof msg,"%s",L->cur.text); lex_next(L);
    } else {
      const char *e = getenv("GROKIUM_MSG");
      if (!e || !e[0]) e = getenv("CUBALC_MSG");
      if (e) snprintf(msg,sizeof msg,"%s",e);
    }
    if (!msg[0]){ fail(vm,"SYS CHAT needs msg or GROKIUM_MSG"); return -1; }
    if (!model[0]){
      const char *em = getenv("GROKIUM_MODEL");
      if (em && em[0]) snprintf(model,sizeof model,"%s",em);
    }
    const char *st = getenv("CUBALC_STATE");
    cubalc_host_result hr;
    if (cubalc_host_chat(be, model, msg, st, &hr)!=0){
      var_set_str(vm,"LAST", hr.err[0]?hr.err:"chat fail");
      var_set_num(vm,"OK",0);
      var_set_num(vm,"HTTP_CODE", hr.code);
      if (vm->trace) fprintf(vm->trace, "# chat fail %s\n", hr.err);
      bump(vm); return 1; /* soft */
    }
    snprintf(vm->last_str,sizeof vm->last_str,"%s",hr.str);
    vm->last_n = hr.n;
    vm->last_code = hr.code;
    var_set_str(vm,"LAST",hr.str);
    var_set_num(vm,"LAST_N",hr.n);
    var_set_num(vm,"HTTP_CODE",hr.code);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }

  /* SYS NUM|INT — parse LAST as integer → LAST_N (CubeBrain digit fold) */
  if (kw(&L->cur,"NUM") || kw(&L->cur,"INT") || kw(&L->cur,"ATOI")){
    lex_next(L);
    const char *s = vm->last_str;
    if (L->cur.kind==TK_STR){ s = L->cur.text; lex_next(L); }
    else if (L->cur.kind==TK_IDENT && strcmp(L->cur.text,"LAST")!=0){
      Var *v = var_get(vm, L->cur.text, 0);
      if (v && v->is_str) s = v->sval;
      else if (v) { var_set_num(vm,"LAST_N", v->val); vm->last_n = v->val; bump(vm); return 1; }
      lex_next(L);
    } else if (L->cur.kind==TK_IDENT && strcmp(L->cur.text,"LAST")==0){
      lex_next(L);
    }
    long n = 0;
    if (s && s[0]) n = strtol(s, NULL, 10);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }

  /* SYS STR|ITOA|NUMSTR|TOSTR [expr|LAST_N] — integer → decimal string LAST.
   * Usability: dual of SYS NUM; fill REPLACEALL templates {{COUNT}} without shell. */
  if (kw(&L->cur,"STR") || kw(&L->cur,"ITOA") || kw(&L->cur,"NUMSTR") ||
      kw(&L->cur,"TOSTR") || kw(&L->cur,"STRNUM") || kw(&L->cur,"DECIMAL")){
    lex_next(L);
    long n = vm->last_n;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
      n = parse_expr(vm, L);
    }
    char buf[40];
    snprintf(buf, sizeof buf, "%ld", n);
    var_set_str(vm, "LAST", buf);
    var_set_str(vm, "STR", buf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }

  /* SYS LASTMATCH|GREP1L|FINDFIELDL [I] bag needle — last field containing needle.
   * SYS LASTMATCHI — case-insensitive. LAST = field (empty if miss); LAST_N 0|1.
   * LASTMATCH_I = 0-based index of hit (-1 miss). Empty needle → last field.
   * Usability: latest log/error line without REVL+FIRSTMATCH or GREP+TAIL glue. */
  if (kw(&L->cur,"LASTMATCH") || kw(&L->cur,"GREP1L") || kw(&L->cur,"FINDFIELDL") ||
      kw(&L->cur,"LASTGREP") || kw(&L->cur,"MATCHLAST") || kw(&L->cur,"GREPLAST") ||
      kw(&L->cur,"LASTMATCHI") || kw(&L->cur,"GREP1LI") || kw(&L->cur,"FINDFIELDLI")){
    char op[20];
    int icase = 0;
    char bag[CUBALC_HOST_STR_MAX], needle[256], out[512];
    const char *p, *start;
    size_t flen, nn;
    long idx = 0, found = 0, found_i = -1;
    snprintf(op, sizeof op, "%s", L->cur.text);
    for (char *q = op; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op, "LASTMATCHI") == 0 || strcmp(op, "GREP1LI") == 0 ||
        strcmp(op, "FINDFIELDLI") == 0)
      icase = 1;
    lex_next(L);
    if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                   kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                   kw(&L->cur,"CI"))){
      icase = 1;
      lex_next(L);
    }
    bag[0] = 0; needle[0] = 0; out[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
    nn = strlen(needle);
    if (bag[0]) {
      p = bag;
      while (*p) {
        int hit = 0;
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        {
          char field[512];
          size_t take = flen;
          if (take >= sizeof field) take = sizeof field - 1;
          memcpy(field, start, take);
          field[take] = 0;
          if (nn == 0) {
            hit = 1;
          } else if (!icase) {
            if (strstr(field, needle) != NULL) hit = 1;
          } else {
            size_t fi, j;
            for (fi = 0; field[fi] && !hit; fi++) {
              for (j = 0; j < nn; j++) {
                char a = field[fi + j], b = needle[j];
                if (!a) break;
                if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                if (a != b) break;
              }
              if (j == nn) hit = 1;
            }
          }
          if (hit) {
            /* keep scanning — last hit wins */
            if (flen >= sizeof out) flen = sizeof out - 1;
            memcpy(out, start, flen);
            out[flen] = 0;
            found = 1;
            found_i = idx;
          }
        }
        idx++;
        if (*p == '\n') p++;
      }
    }
    var_set_str(vm, "LAST", out);
    var_set_str(vm, "GREP", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = found;
    var_set_num(vm, "LAST_N", found);
    var_set_num(vm, "LASTMATCH_N", found);
    var_set_num(vm, "LASTMATCH_I", found_i);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }

  return 0;
}
