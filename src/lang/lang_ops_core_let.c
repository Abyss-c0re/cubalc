/* CubalC lang — LET · INC/DEC · SWAP · REQUIRE RESTARGS (core var plane) */
#include "lang/cubalc_lang_ops_core_let.h"
#include "cubalc_hostops.h"
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

  /* DEFINED|ISSET name — var exists probe (B_E_science selftest).
   * LAST_N/DEFINED = 1 if present, 0 if missing; OK always 1. */
  if (kw(&L->cur,"DEFINED")||kw(&L->cur,"ISSET")||kw(&L->cur,"HASVAR")||
      kw(&L->cur,"VAREXISTS")||kw(&L->cur,"VAR?")){
    char name[48];
    int exists;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DEFINED needs name — DEFINED x"); return -1; }
    snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    exists = (var_get(vm, name, 0) != NULL);
    var_set_num(vm, "DEFINED", (long)exists);
    var_set_num(vm, "ISSET", (long)exists);
    var_set_num(vm, "LAST_N", (long)exists);
    vm->last_n = (long)exists;
    var_set_num(vm, "OK", 1);
    { char nb[8]; snprintf(nb, sizeof nb, "%d", exists);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb); }
    if (vm->trace)
      fprintf(vm->trace, "# defined %s → %d\n", name, exists);
    bump(vm); return 1;
  }

  /* TYPEOF|TYPE name — undef=0 num=1 str=2 (B_E_science selftest).
   * LAST_N/TYPE_N kind code; LAST kind word. OK always 1. */
  if (kw(&L->cur,"TYPEOF")||kw(&L->cur,"TYPE")||kw(&L->cur,"VARTYPE")||
      kw(&L->cur,"KIND OF")||kw(&L->cur,"VARKIND")){
    char name[48];
    Var *v;
    int kind = 0; /* undef */
    const char *kwd = "undef";
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"TYPEOF needs name — TYPEOF x"); return -1; }
    snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    v = var_get(vm, name, 0);
    if (v) {
      if (v->is_str) { kind = 2; kwd = "str"; }
      else { kind = 1; kwd = "num"; }
    }
    var_set_num(vm, "TYPE_N", (long)kind);
    var_set_num(vm, "TYPEOF_N", (long)kind);
    var_set_num(vm, "LAST_N", (long)kind);
    vm->last_n = (long)kind;
    var_set_str(vm, "TYPEOF", kwd);
    var_set_str(vm, "LAST", kwd);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", kwd);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# typeof %s → %s\n", name, kwd);
    bump(vm); return 1;
  }

  /* UNSET name — remove program var so DEFAULT can re-apply (B_E_science).
   * LAST_N/UNSET_N = 1 if removed, 0 if missing; OK always 1. */
  if (kw(&L->cur,"UNSET")||kw(&L->cur,"CLEARVAR")||kw(&L->cur,"DELVAR")||
      kw(&L->cur,"RMVAR")||kw(&L->cur,"VARDEL")){
    char name[48];
    int removed = 0;
    int i;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNSET needs name — UNSET x"); return -1; }
    snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    for (i = 0; i < vm->n_vars; i++) {
      if (strcmp(vm->vars[i].name, name) == 0) {
        if (i + 1 < vm->n_vars)
          memmove(&vm->vars[i], &vm->vars[i + 1],
                  (size_t)(vm->n_vars - i - 1) * sizeof(vm->vars[0]));
        vm->n_vars--;
        removed = 1;
        break;
      }
    }
    var_set_num(vm, "UNSET_N", (long)removed);
    var_set_num(vm, "LAST_N", (long)removed);
    vm->last_n = (long)removed;
    var_set_num(vm, "OK", 1);
    { char nb[8]; snprintf(nb, sizeof nb, "%d", removed);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb); }
    if (vm->trace)
      fprintf(vm->trace, "# unset %s → %d\n", name, removed);
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

    /* REQUIRE FLAGPATH|FLAGFILE|FLAGDIR — peel path flag + ABSPATH + exist/kind.
     * Twin of soft GETFLAGPATH (B_E science). */
    if (kw(&L->cur, "FLAGPATH") || kw(&L->cur, "PATHFLAG") || kw(&L->cur, "REQPATHFLAG") ||
        kw(&L->cur, "REQUIREPATHFLAG") || kw(&L->cur, "FLAG_PATH") ||
        kw(&L->cur, "FLAGFILE") || kw(&L->cur, "REQFLAGFILE") || kw(&L->cur, "PATHFLAGFILE") ||
        kw(&L->cur, "FLAGREG") || kw(&L->cur, "REQFLAGREG") ||
        kw(&L->cur, "FLAGDIR") || kw(&L->cur, "REQFLAGDIR") || kw(&L->cur, "PATHFLAGDIR") ||
        kw(&L->cur, "FLAGFOLDER") || kw(&L->cur, "REQFLAGFOLDER")) {
      char name[96], aliases[384], hitnm[96], val[CUBALC_HOST_STR_MAX], fb[512];
      char env_used[96], src_tag[16], absbuf[CUBALC_HOST_STR_MAX];
      int hit, have_fb = 0, nac, from_env = 0;
      int want_file = 0, want_dir = 0;
      const char *kind = "FLAGPATH";
      cubalc_host_result hr;
      if (kw(&L->cur, "FLAGFILE") || kw(&L->cur, "REQFLAGFILE") ||
          kw(&L->cur, "PATHFLAGFILE") || kw(&L->cur, "FLAGREG") ||
          kw(&L->cur, "REQFLAGREG")) {
        want_file = 1;
        kind = "FLAGFILE";
      } else if (kw(&L->cur, "FLAGDIR") || kw(&L->cur, "REQFLAGDIR") ||
                 kw(&L->cur, "PATHFLAGDIR") || kw(&L->cur, "FLAGFOLDER") ||
                 kw(&L->cur, "REQFLAGFOLDER")) {
        want_dir = 1;
        kind = "FLAGDIR";
      }
      lex_next(L);
      name[0] = 0;
      aliases[0] = 0;
      hitnm[0] = 0;
      val[0] = 0;
      fb[0] = 0;
      env_used[0] = 0;
      absbuf[0] = 0;
      snprintf(src_tag, sizeof src_tag, "%s", "none");
      nac = cubalc_read_flag_aliases(L, aliases, sizeof aliases, name, sizeof name);
      if (nac <= 0 || !name[0]) {
        fail_at(vm, L,
                "REQUIRE FLAGPATH needs name — REQUIRE FLAGPATH conf|c OR ENV CONFIG OR \"cfg.json\"");
        return -1;
      }
      while (kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") || kw(&L->cur, "ELSE") ||
             kw(&L->cur, "FALLBACK")) {
        lex_next(L);
        if (kw(&L->cur, "ENV") || kw(&L->cur, "FROMENV") || kw(&L->cur, "ENVOR") ||
            kw(&L->cur, "GETENV") || kw(&L->cur, "ENV_GET")) {
          char en[96];
          const char *ev;
          lex_next(L);
          en[0] = 0;
          while (L->cur.kind == TK_MINUS) lex_next(L);
          if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
            snprintf(en, sizeof en, "%s", L->cur.text);
            lex_next(L);
          } else {
            fail_at(vm, L, "REQUIRE FLAGPATH OR ENV NAME");
            return -1;
          }
          if (!have_fb && en[0]) {
            ev = getenv(en);
            if (ev && ev[0]) {
              snprintf(fb, sizeof fb, "%s", ev);
              have_fb = 1;
              from_env = 1;
              snprintf(env_used, sizeof env_used, "%s", en);
            }
          }
          continue;
        }
        {
          char lit[512];
          lit[0] = 0;
          if (resolve_str_arg(vm, L, lit, sizeof lit) != 0) {
            fail_at(vm, L, "REQUIRE FLAGPATH name OR \"fallback\"");
            return -1;
          }
          if (!have_fb) {
            snprintf(fb, sizeof fb, "%s", lit);
            have_fb = 1;
            from_env = 0;
          }
        }
        break;
      }
      hit = cubalc_scan_cli_flag_any(aliases, val, sizeof val, hitnm, sizeof hitnm);
      if (!hit) {
        if (have_fb)
          snprintf(val, sizeof val, "%s", fb);
        else
          val[0] = 0;
      }
      if (hit)
        snprintf(src_tag, sizeof src_tag, "%s", "cli");
      else if (have_fb && from_env)
        snprintf(src_tag, sizeof src_tag, "%s", "env");
      else if (have_fb)
        snprintf(src_tag, sizeof src_tag, "%s", "default");
      else
        snprintf(src_tag, sizeof src_tag, "%s", "none");
      if (!val[0]) {
        char msg[240];
        snprintf(msg, sizeof msg,
                 "REQUIRE %s '%s' missing line %d — pass --%s path or OR ENV/default (GETFLAGPATH soft)",
                 kind, name, aln, name);
        cubalc_append_usage_tip(vm, msg, sizeof msg);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(val, &hr) == 0 && hr.str[0])
        snprintf(absbuf, sizeof absbuf, "%s", hr.str);
      else
        snprintf(absbuf, sizeof absbuf, "%s", val);
      if (want_file || want_dir) {
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_path_kind(absbuf, &hr) != 0 || (want_dir && hr.code != 2) ||
            (want_file && hr.code != 1)) {
          char msg[240];
          const char *why = (!hr.ok && hr.err[0]) ? hr.err
                                                  : (want_dir ? "not a directory" : "not a regular file");
          snprintf(msg, sizeof msg, "REQUIRE %s '%s' failed line %d — %s (%s)", kind, absbuf, aln,
                   why, name);
          cubalc_append_usage_tip(vm, msg, sizeof msg);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
      } else {
        if (!cubalc_host_exists(absbuf) && !cubalc_host_exists(val)) {
          char msg[240];
          snprintf(msg, sizeof msg,
                   "REQUIRE FLAGPATH '%s' missing line %d — create path or fix --%s (GETFLAGPATH soft)",
                   absbuf, aln, name);
          cubalc_append_usage_tip(vm, msg, sizeof msg);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
      }
      var_set_str(vm, "LAST", absbuf);
      var_set_str(vm, "FLAG", absbuf);
      var_set_str(vm, "FLAGPATH", absbuf);
      var_set_str(vm, "PATH", absbuf);
      var_set_str(vm, "REQUIRE_FLAGPATH", absbuf);
      var_set_str(vm, "REQUIRE_FLAG", name);
      var_set_str(vm, "GETFLAGPATH", name);
      var_set_str(vm, "GETFLAGPATH_RAW", val);
      var_set_str(vm, "FLAG_HIT_NAME", hit ? hitnm : "");
      var_set_str(vm, "FLAG_ALIAS", hit ? hitnm : "");
      var_set_str(vm, "GETFLAGPATH_SRC", src_tag);
      var_set_str(vm, "FLAG_SRC", src_tag);
      var_set_str(vm, "FLAG_ENV", (!hit && from_env) ? env_used : "");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absbuf);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "GETFLAGPATH_N", (long)strlen(absbuf));
      var_set_num(vm, "GETFLAGPATH_HIT", hit ? 1L : 0L);
      var_set_num(vm, "GETFLAGPATH_EXIST", 1L);
      var_set_num(vm, "PATH_EXIST", 1L);
      var_set_num(vm, "OK", 1);
      if (want_dir) {
        var_set_num(vm, "ISDIR", 1);
        var_set_str(vm, "REQUIRE_FLAGDIR", absbuf);
        var_set_str(vm, "REQUIRE_DIR", absbuf);
      }
      if (want_file) {
        var_set_num(vm, "ISFILE", 1);
        var_set_str(vm, "REQUIRE_FLAGFILE", absbuf);
        var_set_str(vm, "REQUIRE_REG", absbuf);
      }
      if (vm->res) vm->res->asserts_ok++;
      if (vm->trace)
        fprintf(vm->trace, "# require %s %s hit=%s src=%s → %s\n", kind, name, hitnm, src_tag,
                absbuf);
      bump(vm);
      return 1;
    }

    /* REQUIRE ENV|VAR|ENVVAR|HOSTENV name — getenv must be non-empty.
     * On success: LAST = value, LAST_N = len, REQUIRE_ENV = name, OK=1.
     * Usability: fail-fast host config (CUBALC_STATE/SMX key) without SYS ENV + IF glue. */
    if (kw(&L->cur,"ENV")||kw(&L->cur,"VAR")||kw(&L->cur,"ENVVAR")||
        kw(&L->cur,"HOSTENV")||kw(&L->cur,"ENVIRONMENT")){
      char name[96];
      const char *val;
      lex_next(L);
      if (L->cur.kind != TK_STR && L->cur.kind != TK_IDENT){
        fail(vm, "REQUIRE ENV \"NAME\"");
        return -1;
      }
      snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
      if (!name[0]) {
        fail(vm, "REQUIRE ENV empty name");
        return -1;
      }
      val = getenv(name);
      if (!val || !val[0]) {
        char msg[160];
        snprintf(msg, sizeof msg,
                 "REQUIRE ENV '%s' missing line %d — set host env or SYS ENV SET",
                 name, aln);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      {
        size_t vl = strlen(val);
        char out[CUBALC_HOST_STR_MAX];
        if (vl >= sizeof out) vl = sizeof out - 1;
        memcpy(out, val, vl);
        out[vl] = 0;
        var_set_str(vm, "LAST", out);
        var_set_str(vm, "ENV", out);
        var_set_str(vm, "REQUIRE_ENV", name);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
        vm->last_n = (long)vl;
        var_set_num(vm, "LAST_N", (long)vl);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# require env %s ok (len %zu)\n", name, vl);
        if (vm->res) vm->res->asserts_ok++;
        bump(vm); return 1;
      }
    }

    /* REQUIRE PATH|EXIST path — fail if host path missing.
     * REQUIRE DIR|DIRECTORY path — fail if not a directory.
     * REQUIRE REG|REGFILE|ISFILE path — fail if not a regular file.
     * On success: LAST = path, LAST_N=1, REQUIRE_PATH = path, OK=1.
     * Usability: gate plate/config/dir before READ without SYS EXIST + IF + FAIL. */
    if (kw(&L->cur,"PATH")||kw(&L->cur,"EXIST")||kw(&L->cur,"EXISTS")||
        kw(&L->cur,"PRESENT")||kw(&L->cur,"DIR")||kw(&L->cur,"DIRECTORY")||
        kw(&L->cur,"FOLDER")||kw(&L->cur,"ISDIR")||kw(&L->cur,"REG")||
        kw(&L->cur,"REGFILE")||kw(&L->cur,"REGULAR")||kw(&L->cur,"ISFILE")||
        kw(&L->cur,"PLATE")||kw(&L->cur,"HOSTPATH")){
      char path[512];
      cubalc_host_result hr;
      int want_dir = 0, want_file = 0;
      const char *kind = "PATH";
      if (kw(&L->cur,"DIR")||kw(&L->cur,"DIRECTORY")||kw(&L->cur,"FOLDER")||
          kw(&L->cur,"ISDIR")){
        want_dir = 1;
        kind = "DIR";
      } else if (kw(&L->cur,"REG")||kw(&L->cur,"REGFILE")||kw(&L->cur,"REGULAR")||
                 kw(&L->cur,"ISFILE")||kw(&L->cur,"PLATE")){
        want_file = 1;
        kind = "REG";
      }
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "REQUIRE PATH|DIR|REG \"path\"|var|LAST");
        return -1;
      }
      if (!path[0]) {
        fail(vm, "REQUIRE PATH empty path");
        return -1;
      }
      if (want_dir || want_file) {
        if (cubalc_host_path_kind(path, &hr) != 0 ||
            (want_dir && hr.code != 2) ||
            (want_file && hr.code != 1)) {
          char msg[200];
          const char *why = (!hr.ok && hr.err[0]) ? hr.err
              : (want_dir ? "not a directory" : "not a regular file");
          snprintf(msg, sizeof msg,
                   "REQUIRE %s '%s' failed line %d — %s",
                   kind, path, aln, why);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
      } else {
        if (!cubalc_host_exists(path)) {
          char msg[200];
          snprintf(msg, sizeof msg,
                   "REQUIRE PATH '%s' missing line %d — create path or fix layout",
                   path, aln);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
      }
      var_set_str(vm, "LAST", path);
      var_set_str(vm, "REQUIRE_PATH", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
      if (want_dir) {
        var_set_num(vm, "ISDIR", 1);
        var_set_str(vm, "REQUIRE_DIR", path);
      }
      if (want_file) {
        var_set_num(vm, "ISFILE", 1);
        var_set_str(vm, "REQUIRE_REG", path);
      }
      if (vm->trace)
        fprintf(vm->trace, "# require %s %s ok\n", kind, path);
      if (vm->res) vm->res->asserts_ok++;
      bump(vm); return 1;
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

  /* ZERO name — numeric 0. CLEARSTR|EMPTYTO name — empty string. */
  if (kw(&L->cur, "ZERO") || kw(&L->cur, "ZEROV") || kw(&L->cur, "ZEROOUT") ||
      kw(&L->cur, "CLEARSTR") || kw(&L->cur, "EMPTYTO") || kw(&L->cur, "CLEARS") ||
      kw(&L->cur, "BLANKTO") || kw(&L->cur, "ERASESTR")) {
    char op[24], name[48];
    int is_zero = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "ZERO") == 0 || strcmp(op, "ZEROV") == 0 || strcmp(op, "ZEROOUT") == 0)
      is_zero = 1;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, is_zero ? "ZERO needs name — ZERO retries" : "CLEARSTR needs name — CLEARSTR msg");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (is_zero) {
      var_set_num(vm, name, 0);
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
      var_set_num(vm, "ZERO_N", 1);
    } else {
      var_set_str(vm, name, "");
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "CLEARSTR_N", 1);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s\n", is_zero ? "ZERO" : "CLEARSTR", name);
    bump(vm);
    return 1;
  }
  return 0;
}
