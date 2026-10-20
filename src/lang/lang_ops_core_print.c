/* CubalC lang — lang_ops_core_print.c
 * PRINT · SAY · ECHO · PRINT_JSON · VARROOM · VARS (core carve).
 */
#include "lang/cubalc_lang_ops_core_print.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_core_try_print(VM *vm, Lex *L) {
  if (kw(&L->cur,"PRINT") || kw(&L->cur,"SAY") || kw(&L->cur,"ECHO")){
    lex_next(L);
    char line[CUBALC_HOST_STR_MAX];
    size_t o = 0;
    int nparts = 0;
    line[0] = 0;
    while (L->cur.kind != TK_NL && L->cur.kind != TK_EOF && L->cur.kind != TK_LBRACK){
      char part[CUBALC_HOST_STR_MAX];
      size_t pl;
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"CUBE") ||
           kw(&L->cur,"PRINT") || kw(&L->cur,"SAY") || kw(&L->cur,"ECHO") ||
           kw(&L->cur,"PRINT_JSON") || kw(&L->cur,"DUMP") || kw(&L->cur,"PRINTJSON") ||
           kw(&L->cur,"SYS") || kw(&L->cur,"IF") || kw(&L->cur,"END") ||
           kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") ||
           kw(&L->cur,"WHILE") || kw(&L->cur,"LOOP") || kw(&L->cur,"FN") ||
           kw(&L->cur,"CALL") || kw(&L->cur,"SEND") || kw(&L->cur,"CLASS") ||
           kw(&L->cur,"NEW") || kw(&L->cur,"INCLUDE") || kw(&L->cur,"EXPECT") ||
           kw(&L->cur,"FAIL") || kw(&L->cur,"PASS") || kw(&L->cur,"NOTE") ||
           kw(&L->cur,"EXIT") || kw(&L->cur,"RET") || kw(&L->cur,"RETURN")))
        break;
      part[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(part, sizeof part, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(part, sizeof part, "%s", vm->last_str);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *sv = var_get(vm, L->cur.text, 0);
        if (sv && sv->is_str) {
          snprintf(part, sizeof part, "%s", sv->sval);
          lex_next(L);
        } else {
          long v = parse_expr(vm, L);
          snprintf(part, sizeof part, "%ld", v);
        }
      } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                 L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(part, sizeof part, "%ld", v);
      } else {
        break;
      }
      if (nparts && o + 1 < sizeof line) {
        line[o++] = ' ';
        line[o] = 0;
      }
      pl = strlen(part);
      if (o + pl >= sizeof line) {
        if (o + 1 < sizeof line) {
          pl = sizeof line - o - 1;
          memcpy(line + o, part, pl);
          o += pl;
          line[o] = 0;
        }
        break;
      }
      memcpy(line + o, part, pl + 1);
      o += pl;
      nparts++;
    }
    /* empty PRINT → echo LAST (agent log mirror) */
    if (nparts == 0) {
      snprintf(line, sizeof line, "%s", vm->last_str);
      o = strlen(line);
    }
    var_set_str(vm, "LAST", line);
    var_set_str(vm, "PRINT", line);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", line);
    vm->last_n = (long)o;
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "PRINT_N", (long)nparts);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "%s\n", line);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", line);
    bump(vm); return 1;
  }
  /* PRINT_JSON / DUMP [ident…] — one stable JSON line for agents
   * bare: runtime snapshot; with idents: {"name":value,…} (str vars quoted) */
  if (kw(&L->cur,"PRINT_JSON")||kw(&L->cur,"DUMP")||kw(&L->cur,"PRINTJSON")){
    lex_next(L);
    char line[512]; size_t o=0; line[0]=0;
    int first=1;
    o += (size_t)snprintf(line+o, sizeof line-o, "{");
    if (L->cur.kind==TK_NL || L->cur.kind==TK_EOF || L->cur.kind==TK_LBRACK){
      /* bare snapshot of host-facing scalars */
      long okv=0, dec=0, smx=0, talks=0;
      Var *vo=var_get(vm,"OK",0); if (vo && !vo->is_str) okv=vo->val;
      Var *vd=var_get(vm,"DECIDE",0); if (vd && !vd->is_str) dec=vd->val;
      Var *vs=var_get(vm,"SMX_OK",0); smx = vs ? vs->val : (long)vm->smx_ok;
      Var *vt=var_get(vm,"SMX_TALKS",0); talks = vt ? vt->val : (long)vm->smx_talks;
      o += (size_t)snprintf(line+o, sizeof line-o,
        "\"schema\":\"cubalc.print_json.v1\",\"CUBES\":%d,\"LAST_N\":%ld,"
        "\"OK\":%ld,\"SP\":%d,\"UNITY\":%ld,\"DECIDE\":%ld,\"SMX_OK\":%ld,"
        "\"SMX_TALKS\":%ld,\"hold_flash\":%d",
        vm->ch.n_cubes, vm->last_n, okv, vm->sp,
        (long)lround(vm->ch.unity * 100.0), dec, smx, talks, vm->hold_flash);
      first=0;
    } else {
      while (L->cur.kind!=TK_NL && L->cur.kind!=TK_EOF && L->cur.kind!=TK_LBRACK){
        if (L->cur.kind!=TK_IDENT) break;
        if (kw(&L->cur,"ASSERT")||kw(&L->cur,"LET")||kw(&L->cur,"CUBE")||
            kw(&L->cur,"PRINT")||kw(&L->cur,"PRINT_JSON")||kw(&L->cur,"DUMP")||
            kw(&L->cur,"PRINTJSON")||kw(&L->cur,"SYS")||kw(&L->cur,"IF")||
            kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||kw(&L->cur,"END")||
            kw(&L->cur,"RET")||kw(&L->cur,"ELSE")) break;
        char name[48];
        snprintf(name, sizeof name, "%s", L->cur.text);
        lex_next(L);
        if (o + 64 >= sizeof line) break;
        if (!first) o += (size_t)snprintf(line+o, sizeof line-o, ",");
        first=0;
        /* string var or LAST */
        Var *vv = var_get(vm, name, 0);
        if ((vv && vv->is_str) || strcmp(name,"LAST")==0){
          const char *sv = (vv && vv->is_str) ? vv->sval : vm->last_str;
          char esc[240]; size_t eo=0;
          for (const char *p=sv?sv:""; *p && eo+2<sizeof esc; p++){
            if (*p=='"' || *p=='\\'){ esc[eo++]='\\'; esc[eo++]=*p; }
            else if ((unsigned char)*p < 0x20) continue;
            else esc[eo++]=*p;
          }
          esc[eo]=0;
          o += (size_t)snprintf(line+o, sizeof line-o, "\"%s\":\"%s\"", name, esc);
        } else {
          long val=0;
          if (vv) val=vv->val;
          else if (strcmp(name,"CUBES")==0) val=(long)vm->ch.n_cubes;
          else if (strcmp(name,"LAST_N")==0) val=vm->last_n;
          else if (strcmp(name,"SP")==0 || strcmp(name,"STACKLEN")==0) val=(long)vm->sp;
          else if (strcmp(name,"UNITY")==0) val=(long)lround(vm->ch.unity*100.0);
          else if (strcmp(name,"SMX_OK")==0) val=(long)vm->smx_ok;
          else if (strcmp(name,"SMX_TALKS")==0) val=(long)vm->smx_talks;
          else if (strcmp(name,"SMX_OOB")==0) val=(long)vm->smx_oob;
          else if (strcmp(name,"HTTP_CODE")==0) val=(long)vm->last_code;
          else if (strcmp(name,"CELLS")==0) val=(long)CUBALC_CELL_N;
          else if (strcmp(name,"HOLD_FLASH")==0) val=(long)vm->hold_flash;
          o += (size_t)snprintf(line+o, sizeof line-o, "\"%s\":%ld", name, val);
        }
      }
    }
    if (o + 2 < sizeof line) snprintf(line+o, sizeof line-o, "}");
    else line[sizeof line - 2] = '}';
    if (vm->trace) fprintf(vm->trace,"%s\n",line);
    if (vm->res) snprintf(vm->res->last_print,sizeof vm->res->last_print,"%s",line);
    bump(vm); return 1;
  }
  /* VARROOM / VARSLEFT — free var-table slots → LAST_N.
   * HASVARROOM n — soft 0|1 room >= n (sticky LAST_ERR on miss).
   * NEEDVARROOM n — fail-fast if fewer than n free (fat nest specials guard).
   * Usability: IF before SETP/ALLEQFLAT without VARS dump or plate parse. */
  if (kw(&L->cur, "VARROOM") || kw(&L->cur, "VARSLEFT") ||
      kw(&L->cur, "VARS_ROOM") || kw(&L->cur, "FREEVARS") ||
      kw(&L->cur, "HASVARROOM") || kw(&L->cur, "HAVEVARROOM") ||
      kw(&L->cur, "HAS_VARROOM") || kw(&L->cur, "ENOUGHVARS") ||
      kw(&L->cur, "NEEDVARROOM") || kw(&L->cur, "REQUIREVARROOM") ||
      kw(&L->cur, "NEED_VARROOM") || kw(&L->cur, "REQUIRE_VARS")) {
    int hard = kw(&L->cur, "NEEDVARROOM") || kw(&L->cur, "REQUIREVARROOM") ||
               kw(&L->cur, "NEED_VARROOM") || kw(&L->cur, "REQUIRE_VARS");
    int soft_need = kw(&L->cur, "HASVARROOM") || kw(&L->cur, "HAVEVARROOM") ||
                    kw(&L->cur, "HAS_VARROOM") || kw(&L->cur, "ENOUGHVARS");
    int aln = L->cur.line;
    long need = 1;
    long room;
    int ok_room;
    lex_next(L);
    if (hard || soft_need) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)
        need = parse_expr(vm, L);
      if (need < 0) need = 0;
    }
    room = (long)CUBALC_MAX_VARS - (long)vm->n_vars;
    if (room < 0) room = 0;
    ok_room = (room >= need && !vm->vars_full) ? 1 : 0;
    var_set_num(vm, "VARS_N", (long)vm->n_vars);
    var_set_num(vm, "VARS_MAX", (long)CUBALC_MAX_VARS);
    var_set_num(vm, "VARS_FULL", vm->vars_full ? 1L : 0L);
    var_set_num(vm, "VARROOM", room);
    var_set_num(vm, "VARS_LEFT", room);
    var_set_num(vm, "VARROOM_NEED", need);
    if (hard) {
      if (ok_room) {
        var_set_num(vm, "LAST_N", room);
        vm->last_n = room;
        {
          char nb[24];
          snprintf(nb, sizeof nb, "%ld", room);
          var_set_str(vm, "LAST", nb);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
        }
        var_set_num(vm, "OK", 1);
        if (vm->res) vm->res->asserts_ok++;
        bump(vm);
        return 1;
      }
      {
        char msg[200];
        snprintf(msg, sizeof msg,
                 "NEEDVARROOM line %d: need %ld free have %ld (used %d/%d%s) — "
                 "lean LETs or raise CUBALC_MAX_VARS",
                 aln, need, room, vm->n_vars, CUBALC_MAX_VARS,
                 vm->vars_full ? ",full" : "");
        var_set_str(vm, "ERR", msg);
        var_set_str(vm, "LAST_ERR", msg);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
    }
    if (soft_need) {
      /* HASVARROOM n — soft 0|1 · dual of NEEDVARROOM · IF without count compare */
      var_set_num(vm, "LAST_N", ok_room);
      vm->last_n = ok_room;
      var_set_num(vm, "HASVARROOM_N", ok_room);
      {
        char nb[8];
        snprintf(nb, sizeof nb, "%d", ok_room);
        var_set_str(vm, "LAST", nb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      }
      if (ok_room) {
        var_set_num(vm, "OK", 1);
      } else {
        char em[192];
        snprintf(em, sizeof em,
                 "HASVARROOM miss line %d: need %ld free have %ld (used %d/%d) — "
                 "lean LETs or NEEDVARROOM",
                 aln, need, room, vm->n_vars, CUBALC_MAX_VARS);
        var_set_str(vm, "ERR", em);
        var_set_str(vm, "LAST_ERR", em);
        var_set_num(vm, "OK", 0);
      }
      bump(vm);
      return 1;
    }
    /* soft VARROOM count */
    var_set_num(vm, "LAST_N", room);
    vm->last_n = room;
    {
      char nb[24];
      snprintf(nb, sizeof nb, "%ld", room);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* REMAIN_MS / BUDGETLEFT / TIMELEFT — wall budget remaining → LAST_N.
   * HAS_TIME n / HASTIME — soft 0|1 if remaining >= n (unlimited → 1).
   * NEEDTIME n / NEED_TIME — fail-fast if budget set and remaining < n.
   * Usability: mid-run early EXIT before heavy work without shell date glue.
   * Complements CUBALC_RUN_TIMEOUT / -T (TIMEOUT_MS / TIMED_OUT). */
  if (kw(&L->cur, "REMAIN_MS") || kw(&L->cur, "REMAINMS") ||
      kw(&L->cur, "BUDGETLEFT") || kw(&L->cur, "BUDGET_LEFT") ||
      kw(&L->cur, "TIMELEFT") || kw(&L->cur, "TIME_LEFT") ||
      kw(&L->cur, "TIMEOUT_LEFT") || kw(&L->cur, "MSLEFT") ||
      kw(&L->cur, "HAS_TIME") || kw(&L->cur, "HASTIME") ||
      kw(&L->cur, "HAS_BUDGET") || kw(&L->cur, "HASBUDGET") ||
      kw(&L->cur, "ENOUGHTIME") || kw(&L->cur, "ENOUGH_TIME") ||
      kw(&L->cur, "NEEDTIME") || kw(&L->cur, "NEED_TIME") ||
      kw(&L->cur, "REQUIRE_TIME") || kw(&L->cur, "REQUIRETIME") ||
      kw(&L->cur, "NEEDBUDGET") || kw(&L->cur, "NEED_BUDGET")) {
    int hard = kw(&L->cur, "NEEDTIME") || kw(&L->cur, "NEED_TIME") ||
               kw(&L->cur, "REQUIRE_TIME") || kw(&L->cur, "REQUIRETIME") ||
               kw(&L->cur, "NEEDBUDGET") || kw(&L->cur, "NEED_BUDGET");
    int soft_need = kw(&L->cur, "HAS_TIME") || kw(&L->cur, "HASTIME") ||
                    kw(&L->cur, "HAS_BUDGET") || kw(&L->cur, "HASBUDGET") ||
                    kw(&L->cur, "ENOUGHTIME") || kw(&L->cur, "ENOUGH_TIME");
    int aln = L->cur.line;
    long need = 1;
    long rem;
    int unlimited;
    int ok_time;
    lex_next(L);
    if (hard || soft_need) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)
        need = parse_expr(vm, L);
      if (need < 0) need = 0;
    }
    rem = cubalc_lang_timeout_remain_ms(vm);
    unlimited = (rem < 0) ? 1 : 0;
    if (unlimited)
      ok_time = 1;
    else
      ok_time = (rem >= need) ? 1 : 0;
    var_set_num(vm, "TIMEOUT_MS",
                vm->run_timeout_ms > 0 ? vm->run_timeout_ms : 0L);
    var_set_num(vm, "REMAIN_MS", unlimited ? -1L : rem);
    var_set_num(vm, "BUDGET_LEFT", unlimited ? -1L : rem);
    var_set_num(vm, "TIMELEFT", unlimited ? -1L : rem);
    var_set_num(vm, "NEEDTIME_N", need);
    var_set_num(vm, "HAS_TIME_N", ok_time);
    var_set_num(vm, "TIMEOUT_UNLIMITED", unlimited ? 1L : 0L);
    if (hard) {
      if (ok_time) {
        long outn = unlimited ? -1L : rem;
        var_set_num(vm, "LAST_N", outn);
        vm->last_n = outn;
        {
          char nb[24];
          snprintf(nb, sizeof nb, "%ld", outn);
          var_set_str(vm, "LAST", nb);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
        }
        var_set_num(vm, "OK", 1);
        if (vm->res) vm->res->asserts_ok++;
        bump(vm);
        return 1;
      }
      {
        char msg[200];
        snprintf(msg, sizeof msg,
                 "NEEDTIME line %d: need %ldms have %ldms (budget %ldms) — "
                 "raise -T / CUBALC_RUN_TIMEOUT or lean the work",
                 aln, need, rem < 0 ? 0L : rem,
                 vm->run_timeout_ms > 0 ? vm->run_timeout_ms : 0L);
        var_set_str(vm, "ERR", msg);
        var_set_str(vm, "LAST_ERR", msg);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
    }
    if (soft_need) {
      var_set_num(vm, "LAST_N", ok_time);
      vm->last_n = ok_time;
      {
        char nb[8];
        snprintf(nb, sizeof nb, "%d", ok_time);
        var_set_str(vm, "LAST", nb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      }
      if (ok_time) {
        var_set_num(vm, "OK", 1);
      } else {
        char em[192];
        snprintf(em, sizeof em,
                 "HAS_TIME miss line %d: need %ldms have %ldms (budget %ldms) — "
                 "EXIT early or raise -T",
                 aln, need, rem < 0 ? 0L : rem,
                 vm->run_timeout_ms > 0 ? vm->run_timeout_ms : 0L);
        var_set_str(vm, "ERR", em);
        var_set_str(vm, "LAST_ERR", em);
        var_set_num(vm, "OK", 0);
      }
      bump(vm);
      return 1;
    }
    {
      long outn = unlimited ? -1L : rem;
      char nb[24];
      var_set_num(vm, "LAST_N", outn);
      vm->last_n = outn;
      snprintf(nb, sizeof nb, "%ld", outn);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
  }

  /* WALL_MS / ELAPSED / ELAPSED_MS — mono ms since run start → LAST_N.
   * Usability: profile mid-run without SYS MONOTONIC glue; dual of REMAIN_MS. */
  if (kw(&L->cur, "WALL_MS") || kw(&L->cur, "WALLMS") ||
      kw(&L->cur, "ELAPSED") || kw(&L->cur, "ELAPSED_MS") ||
      kw(&L->cur, "RUN_ELAPSED") || kw(&L->cur, "RUNTIME_MS")) {
    long wall = 0;
    char nb[24];
    lex_next(L);
    if (vm->run_start_ms > 0) {
      long now = cubalc_lang_mono_ms();
      if (now >= vm->run_start_ms)
        wall = now - vm->run_start_ms;
    }
    var_set_num(vm, "WALL_MS", wall);
    var_set_num(vm, "ELAPSED_MS", wall);
    var_set_num(vm, "LAST_N", wall);
    vm->last_n = wall;
    snprintf(nb, sizeof nb, "%ld", wall);
    var_set_str(vm, "LAST", nb);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* VARS / LOCALS — dump all program variables as one JSON line for agents.
   * Complements PRINT_JSON (named/snapshot): full LET/SYS table without guessing names. */
  if (kw(&L->cur,"VARS")||kw(&L->cur,"LOCALS")||kw(&L->cur,"SHOW_VARS")||
      kw(&L->cur,"VARS_JSON")||kw(&L->cur,"DUMP_VARS")){
    lex_next(L);
    char line[CUBALC_HOST_STR_MAX];
    size_t o = 0;
    int i, first = 1, n = 0;
    o += (size_t)snprintf(line + o, sizeof line - o,
      "{\"schema\":\"cubalc.vars.v1\",\"n\":0,\"vars\":{");
    /* rewrite n after count — fill vars first into temp then wrap is heavy;
     * emit and patch is awkward; emit n at end via second pass count. */
    o = 0;
    o += (size_t)snprintf(line + o, sizeof line - o,
      "{\"schema\":\"cubalc.vars.v1\",\"ok\":true,\"vars\":{");
    for (i = 0; i < vm->n_vars; i++) {
      Var *v = &vm->vars[i];
      char esc[280];
      size_t eo = 0;
      int add;
      if (!v->name[0]) continue;
      if (o + 80 >= sizeof line) break;
      if (!first) o += (size_t)snprintf(line + o, sizeof line - o, ",");
      first = 0;
      n++;
      if (v->is_str) {
        const char *p;
        for (p = v->sval; *p && eo + 2 < sizeof esc; p++) {
          if (*p == '"' || *p == '\\') { esc[eo++] = '\\'; esc[eo++] = *p; }
          else if ((unsigned char)*p < 0x20) continue;
          else esc[eo++] = *p;
        }
        esc[eo] = 0;
        add = snprintf(line + o, sizeof line - o, "\"%s\":\"%s\"", v->name, esc);
      } else {
        add = snprintf(line + o, sizeof line - o, "\"%s\":%ld", v->name, v->val);
      }
      if (add > 0) o += (size_t)add;
    }
    /* also surface LAST if not already a var */
    if (o + 64 < sizeof line) {
      int has_last = 0;
      for (i = 0; i < vm->n_vars; i++)
        if (strcmp(vm->vars[i].name, "LAST") == 0) { has_last = 1; break; }
      if (!has_last && vm->last_str[0]) {
        char esc[280];
        size_t eo = 0;
        const char *p;
        for (p = vm->last_str; *p && eo + 2 < sizeof esc; p++) {
          if (*p == '"' || *p == '\\') { esc[eo++] = '\\'; esc[eo++] = *p; }
          else if ((unsigned char)*p < 0x20) continue;
          else esc[eo++] = *p;
        }
        esc[eo] = 0;
        o += (size_t)snprintf(line + o, sizeof line - o, "%s\"LAST\":\"%s\"",
                              first ? "" : ",", esc);
        first = 0;
        n++;
      }
    }
    o += (size_t)snprintf(line + o, sizeof line - o,
                          "},\"n\":%d,\"max\":%d,\"full\":%s,\"version\":\"%s\"}",
                          n, CUBALC_MAX_VARS,
                          vm->vars_full ? "true" : "false",
                          CUBALC_LANG_VERSION);
    if (vm->trace) fprintf(vm->trace, "%s\n", line);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", line);
    var_set_num(vm, "VARS_N", (long)n);
    var_set_num(vm, "VARS_MAX", (long)CUBALC_MAX_VARS);
    var_set_num(vm, "VARS_FULL", vm->vars_full ? 1L : 0L);
    var_set_num(vm, "LAST_N", (long)n);
    var_set_num(vm, "OK", 1);
    vm->last_n = (long)n;
    /* keep LAST as count note for agents that only read LAST string */
    {
      char note[48];
      snprintf(note, sizeof note, "vars:%d", n);
      var_set_str(vm, "LAST", note);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", note);
    }
    bump(vm); return 1;
  }
  return 0;
}
