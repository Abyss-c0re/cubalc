/* CubalC lang — LET · INC/DEC · SWAP · REQUIRE RESTARGS (core var plane) */
#include "lang/cubalc_lang_ops_core_let.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cubalc_law.h"

int cubalc_lang_ops_core_try_let(VM *vm, Lex *L) {
  if (kw(&L->cur,"LET")){
    int aln = L->cur.line;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){
      char ebuf[160];
      snprintf(ebuf, sizeof ebuf,
               "LET needs name [=] value line %d — LET x = 1", aln);
      fail(vm, ebuf); return -1;
    }
    char name[48]; snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    /* '=' optional when next token is a value — LET x 1 same as LET x = 1 */
    if (L->cur.kind==TK_EQ){
      lex_next(L);
    } else if (!(L->cur.kind==TK_NUM || L->cur.kind==TK_STR ||
                 L->cur.kind==TK_MINUS || L->cur.kind==TK_LPAREN ||
                 L->cur.kind==TK_IDENT)){
      char ebuf[160];
      snprintf(ebuf, sizeof ebuf,
               "LET '%s' needs value line %d — LET %s = 1", name, aln, name);
      fail(vm, ebuf); return -1;
    }
    if (L->cur.kind==TK_STR || (L->cur.kind==TK_IDENT && (
          strcmp(L->cur.text,"LAST")==0 ||
          (var_get(vm,L->cur.text,0) && var_get(vm,L->cur.text,0)->is_str)))){
      char buf[CUBALC_HOST_STR_MAX]; buf[0]=0;
      for(;;){
        if (L->cur.kind==TK_STR){
          size_t bl=strlen(buf), al=strlen(L->cur.text);
          if (bl+al+1 < sizeof buf) memcpy(buf+bl, L->cur.text, al+1);
          lex_next(L);
        } else if (L->cur.kind==TK_IDENT){
          if (strcmp(L->cur.text,"LAST")==0){
            size_t bl=strlen(buf), al=strlen(vm->last_str);
            if (bl+al+1 < sizeof buf) memcpy(buf+bl, vm->last_str, al+1);
            lex_next(L);
          } else {
            Var *v = var_get(vm, L->cur.text, 0);
            if (v && v->is_str){
              size_t bl=strlen(buf), al=strlen(v->sval);
              if (bl+al+1 < sizeof buf) memcpy(buf+bl, v->sval, al+1);
            } else if (v){
              char nb[32]; snprintf(nb,sizeof nb,"%ld", v->val);
              size_t bl=strlen(buf), al=strlen(nb);
              if (bl+al+1 < sizeof buf) memcpy(buf+bl, nb, al+1);
            } else break;
            lex_next(L);
          }
        } else break;
        if (L->cur.kind==TK_PLUS){ lex_next(L); continue; }
        break;
      }
      var_set_str(vm, name, buf);
      bump(vm); return 1;
    }
    long v=parse_expr(vm,L);
    var_set_num(vm, name, v);
    bump(vm); return 1;
  }
  /* INC|BUMP|ADDTO name [delta] — numeric var += delta (default 1).
   * DEC|SUBFROM name [delta] — numeric var -= delta (default 1).
   * Creates var as 0 if missing. Str var promoted via strtol (0 if non-num).
   * Usability: counters/retries without LET x = x + 1 glue. */
  if (kw(&L->cur,"INC")||kw(&L->cur,"BUMP")||kw(&L->cur,"ADDTO")||
      kw(&L->cur,"INCR")||kw(&L->cur,"INCREMENT")||
      kw(&L->cur,"DEC")||kw(&L->cur,"SUBFROM")||kw(&L->cur,"DECR")||
      kw(&L->cur,"DECREMENT")||kw(&L->cur,"SUBTO")){
    char op[24];
    int is_dec = 0;
    char name[48];
    long delta = 1, cur = 0, nv;
    Var *vv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "DEC") == 0 || strcmp(op, "SUBFROM") == 0 ||
        strcmp(op, "DECR") == 0 || strcmp(op, "DECREMENT") == 0 ||
        strcmp(op, "SUBTO") == 0)
      is_dec = 1;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, is_dec ? "DEC needs name [n] — DEC retries" : "INC needs name [n] — INC retries");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "BY") || kw(&L->cur, "OF") || kw(&L->cur, "WITH"))
      lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !kw(&L->cur, "END") && !kw(&L->cur, "IF") &&
         !kw(&L->cur, "LET") && !kw(&L->cur, "ASSERT") && !kw(&L->cur, "PRINT") &&
         !kw(&L->cur, "PASS") && !kw(&L->cur, "FAIL") && !kw(&L->cur, "INC") &&
         !kw(&L->cur, "DEC") && !kw(&L->cur, "SWAP")))
      delta = parse_expr(vm, L);
    if (delta < 0) {
      /* allow negative delta as opposite direction */
    }
    vv = var_get(vm, name, 0);
    if (vv && !vv->is_str) {
      cur = vv->val;
    } else if (vv && vv->is_str) {
      char *end = NULL;
      cur = strtol(vv->sval, &end, 10);
      if (!(end && end != vv->sval && *end == 0))
        cur = 0;
    } else {
      cur = 0;
    }
    nv = is_dec ? (cur - delta) : (cur + delta);
    var_set_num(vm, name, nv);
    var_set_num(vm, "LAST_N", nv);
    vm->last_n = nv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", nv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, is_dec ? "DEC_N" : "INC_N", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> %ld\n", is_dec ? "DEC" : "INC", name, nv);
    bump(vm);
    return 1;
  }
  /* SWAP|XCHG a [WITH|AND] b — exchange two vars (num or str).
   * Usability: dual buffers without temp LET t = a glue. */
  if (kw(&L->cur,"SWAP")||kw(&L->cur,"XCHG")||kw(&L->cur,"EXCHANGE")||
      kw(&L->cur,"SWAPVARS")||kw(&L->cur,"SWAPV")){
    char a[48], b[48];
    Var *va, *vb;
    int a_str = 0, b_str = 0;
    long a_n = 0, b_n = 0;
    char a_s[CUBALC_HOST_STR_MAX], b_s[CUBALC_HOST_STR_MAX];
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, "SWAP needs two names — SWAP a b");
      return -1;
    }
    snprintf(a, sizeof a, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "WITH") || kw(&L->cur, "AND") || kw(&L->cur, "OR") ||
        kw(&L->cur, ",") || kw(&L->cur, "TO"))
      lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, "SWAP needs two names — SWAP a b");
      return -1;
    }
    snprintf(b, sizeof b, "%s", L->cur.text);
    lex_next(L);
    a_s[0] = b_s[0] = 0;
    va = var_get(vm, a, 0);
    vb = var_get(vm, b, 0);
    if (va) {
      if (va->is_str) {
        a_str = 1;
        snprintf(a_s, sizeof a_s, "%s", va->sval);
      } else {
        a_n = va->val;
      }
    }
    if (vb) {
      if (vb->is_str) {
        b_str = 1;
        snprintf(b_s, sizeof b_s, "%s", vb->sval);
      } else {
        b_n = vb->val;
      }
    }
    /* write b's value into a, a's into b */
    if (b_str)
      var_set_str(vm, a, b_s);
    else
      var_set_num(vm, a, b_n);
    if (a_str)
      var_set_str(vm, b, a_s);
    else
      var_set_num(vm, b, a_n);
    var_set_num(vm, "SWAP_N", 1);
    var_set_num(vm, "OK", 1);
    var_set_num(vm, "LAST_N", 1);
    vm->last_n = 1;
    var_set_str(vm, "LAST", "swapped");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", "swapped");
    if (vm->trace)
      fprintf(vm->trace, "# SWAP %s <-> %s\n", a, b);
    bump(vm);
    return 1;
  }
  /* DEFAULT name = expr|str — assign only if name is not yet defined.
   * Usability: INCLUDE libs set knobs without clobbering caller LET.
   * LAST_N/DEFAULT_SET = 1 if applied, 0 if skipped; OK always 1. */
  if (kw(&L->cur,"DEFAULT")||kw(&L->cur,"SETDEFAULT")||kw(&L->cur,"DEFAULT_LET")||
      kw(&L->cur,"LET_DEFAULT")||kw(&L->cur,"ORLET")){
    int applied = 0;
    int exists;
    char name[48];
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DEFAULT name = value"); return -1; }
    snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_EQ){ fail(vm,"DEFAULT name = value"); return -1; }
    lex_next(L);
    exists = (var_get(vm, name, 0) != NULL);
    if (L->cur.kind==TK_STR || (L->cur.kind==TK_IDENT && (
          strcmp(L->cur.text,"LAST")==0 ||
          (var_get(vm,L->cur.text,0) && var_get(vm,L->cur.text,0)->is_str)))){
      char buf[CUBALC_HOST_STR_MAX]; buf[0]=0;
      for(;;){
        if (L->cur.kind==TK_STR){
          size_t bl=strlen(buf), al=strlen(L->cur.text);
          if (bl+al+1 < sizeof buf) memcpy(buf+bl, L->cur.text, al+1);
          lex_next(L);
        } else if (L->cur.kind==TK_IDENT){
          if (strcmp(L->cur.text,"LAST")==0){
            size_t bl=strlen(buf), al=strlen(vm->last_str);
            if (bl+al+1 < sizeof buf) memcpy(buf+bl, vm->last_str, al+1);
            lex_next(L);
          } else {
            Var *v = var_get(vm, L->cur.text, 0);
            if (v && v->is_str){
              size_t bl=strlen(buf), al=strlen(v->sval);
              if (bl+al+1 < sizeof buf) memcpy(buf+bl, v->sval, al+1);
            } else if (v){
              char nb[32]; snprintf(nb,sizeof nb,"%ld", v->val);
              size_t bl=strlen(buf), al=strlen(nb);
              if (bl+al+1 < sizeof buf) memcpy(buf+bl, nb, al+1);
            } else break;
            lex_next(L);
          }
        } else break;
        if (L->cur.kind==TK_PLUS){ lex_next(L); continue; }
        break;
      }
      if (!exists){ var_set_str(vm, name, buf); applied = 1; }
    } else {
      long v=parse_expr(vm,L);
      if (!exists){ var_set_num(vm, name, v); applied = 1; }
    }
    var_set_num(vm, "DEFAULT_SET", (long)applied);
    var_set_num(vm, "LAST_N", (long)applied);
    var_set_num(vm, "OK", 1);
    vm->last_n = (long)applied;
    { char nb[8]; snprintf(nb, sizeof nb, "%d", applied);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb); }
    if (vm->trace)
      fprintf(vm->trace, "# default %s %s\n", name, applied ? "set" : "skip");
    bump(vm); return 1;
  }

  /* REQUIRE VERSION floor + REQUIRE RESTARGS hard gate (mesh vote B_E). */
  if (kw(&L->cur, "REQUIRE") || kw(&L->cur, "NEED") || kw(&L->cur, "REQUIRES")) {
    int aln = L->cur.line;
    lex_next(L);
    /* REQUIRE ARG n|name — CUBALC_ARGn / named env must be non-empty. */
    if (kw(&L->cur, "ARG") || kw(&L->cur, "ARGV") || kw(&L->cur, "ARGN")) {
      char key[96], envn[96], out[CUBALC_HOST_STR_MAX];
      const char *val = NULL;
      int is_num = 0;
      long idx = -1;
      size_t vl;
      lex_next(L);
      if (L->cur.kind == TK_NUM) {
        idx = L->cur.num;
        is_num = 1;
        snprintf(envn, sizeof envn, "CUBALC_ARG%ld", idx);
        snprintf(key, sizeof key, "%ld", idx);
        lex_next(L);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        int all_digit = 1, pi;
        snprintf(key, sizeof key, "%s", L->cur.text);
        for (pi = 0; key[pi]; pi++) {
          if (key[pi] < '0' || key[pi] > '9') {
            all_digit = 0;
            break;
          }
        }
        if (all_digit && key[0]) {
          snprintf(envn, sizeof envn, "CUBALC_ARG%s", key);
          is_num = 1;
          idx = strtol(key, NULL, 10);
        } else if (strncmp(key, "CUBALC_ARG", 10) == 0) {
          snprintf(envn, sizeof envn, "%s", key);
        } else {
          snprintf(envn, sizeof envn, "%s", key);
        }
        lex_next(L);
      } else {
        fail(vm, "REQUIRE ARG n|name");
        return -1;
      }
      val = getenv(envn);
      if (!val || !val[0]) {
        char msg[180];
        if (is_num)
          snprintf(msg, sizeof msg,
                   "REQUIRE ARG %ld missing line %d — cubalc run … -- arg (SYS ARG %ld)",
                   idx, aln, idx);
        else
          snprintf(msg, sizeof msg,
                   "REQUIRE ARG '%s' missing line %d — set env or pass run arg", key, aln);
        cubalc_append_usage_tip(vm, msg, sizeof msg);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      vl = strlen(val);
      if (vl >= sizeof out) vl = sizeof out - 1;
      memcpy(out, val, vl);
      out[vl] = 0;
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "ARG", out);
      var_set_str(vm, "REQUIRE_ARG", key);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)vl;
      var_set_num(vm, "LAST_N", (long)vl);
      var_set_num(vm, "OK", 1);
      if (vm->res) vm->res->asserts_ok++;
      if (vm->trace)
        fprintf(vm->trace, "# require arg %s ok\n", key);
      bump(vm);
      return 1;
    }

    /* REQUIRE RESTARGS|POS [min] — fail if non-flag count < min (default 1). */
    if (kw(&L->cur, "RESTARGS") || kw(&L->cur, "POSITIONALS") || kw(&L->cur, "POSARGS") ||
        kw(&L->cur, "POS") || kw(&L->cur, "NFILES") || kw(&L->cur, "FILES") ||
        kw(&L->cur, "REST_ARGS") || kw(&L->cur, "NONFLAGS")) {
      long need = 1, have;
      char bag[CUBALC_HOST_STR_MAX], msg[180];
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
          (L->cur.kind == TK_IDENT && !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
           !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") && !kw(&L->cur, "END") &&
           !kw(&L->cur, "REQUIRE") && !kw(&L->cur, "NEED") && !kw(&L->cur, "PATH") &&
           !kw(&L->cur, "FLAG") && !kw(&L->cur, "ARG") && !kw(&L->cur, "ARGC"))) {
        need = parse_expr(vm, L);
      }
      if (need < 0) need = 0;
      if (need > 32) need = 32;
      have = cubalc_collect_restargs(bag, sizeof bag);
      if (have < need) {
        snprintf(msg, sizeof msg,
                 "REQUIRE RESTARGS %ld got %ld line %d — pass files after --flags (RESTARGS)",
                 need, have, aln);
        cubalc_append_usage_tip(vm, msg, sizeof msg);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      var_set_str(vm, "LAST", bag);
      var_set_str(vm, "RESTARGS", bag);
      var_set_str(vm, "POSITIONALS", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      vm->last_n = have;
      var_set_num(vm, "LAST_N", have);
      var_set_num(vm, "RESTARGS_N", have);
      var_set_num(vm, "REQUIRE_RESTARGS", need);
      var_set_num(vm, "OK", 1);
      if (vm->res) vm->res->asserts_ok++;
      if (vm->trace)
        fprintf(vm->trace, "# require restargs >= %ld (have %ld)\n", need, have);
      bump(vm);
      return 1;
    }
    if (kw(&L->cur, "VERSION") || kw(&L->cur, "VER") || kw(&L->cur, "LANG") ||
        kw(&L->cur, "CUBALC"))
      lex_next(L);
    if (L->cur.kind != TK_STR && L->cur.kind != TK_IDENT && L->cur.kind != TK_NUM) {
      fail(vm, "REQUIRE VERSION \"x.y[.z]\" | REQUIRE RESTARGS|LIB|ENV|… (use NEED* duals)");
      return -1;
    }
    {
      char need[64];
      const char *have = CUBALC_LANG_VERSION;
      long hn[3] = {0,0,0}, nn[3] = {0,0,0};
      int hi = 0, ni = 0;
      char *p;
      char hbuf[64], nbuf[64];
      if (L->cur.kind == TK_NUM)
        snprintf(need, sizeof need, "%ld", L->cur.num);
      else
        snprintf(need, sizeof need, "%s", L->cur.text);
      lex_next(L);
      snprintf(hbuf, sizeof hbuf, "%s", have);
      snprintf(nbuf, sizeof nbuf, "%s", need);
      for (p = hbuf; *p && hi < 3; ) {
        if (*p >= '0' && *p <= '9') {
          long v = 0;
          while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; }
          hn[hi++] = v;
          if (*p == '.' || *p == '-') p++;
          else break;
        } else p++;
      }
      for (p = nbuf; *p && ni < 3; ) {
        if (*p >= '0' && *p <= '9') {
          long v = 0;
          while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; }
          nn[ni++] = v;
          if (*p == '.' || *p == '-') p++;
          else break;
        } else p++;
      }
      {
        int okv = 1, i;
        for (i = 0; i < 3; i++) {
          if (hn[i] > nn[i]) break;
          if (hn[i] < nn[i]) { okv = 0; break; }
        }
        if (!okv) {
          char msg[200];
          snprintf(msg, sizeof msg,
                   "REQUIRE VERSION %s failed: have %s — upgrade runtime · cubalc version",
                   need, have);
          fail(vm, msg);
          return -1;
        }
      }
      var_set_str(vm, "LAST", have);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", have);
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_num(vm, "OK", 1);
      if (vm->res) vm->res->asserts_ok++;
      bump(vm);
      return 1;
    }
  }
  return 0;
}
