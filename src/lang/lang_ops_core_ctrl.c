/* CubalC lang — lang_ops_core_ctrl.c
 * EXIT / TIPS / FORMSFOR / agent control tips plane (A_core peel).
 * Mesh vote A_core_forms_agent_peel · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_core_ctrl.h"
#include "lang/cubalc_lang_ops_core_help.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_core_try_ctrl(VM *vm, Lex *L) {
  /* EXIT/TIPS/FORMSFOR control plane.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
  /* EXIT [code] ["why"] — intentional early program stop for agents/CI.
   * EXIT / EXIT 0 → clean halt (ok if no asserts_fail). EXIT n → fail plate + rc.
   * Stops further statements (including outer loops). */
  if (kw(&L->cur,"EXIT")||kw(&L->cur,"HALT")||kw(&L->cur,"QUIT")||
      kw(&L->cur,"STOP_PROG")||kw(&L->cur,"DIE")){
    int aln = L->cur.line;
    long code = 0;
    char why[120];
    lex_next(L);
    why[0] = 0;
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        (L->cur.kind == TK_IDENT && (strcmp(L->cur.text,"OK")==0 ||
         strcmp(L->cur.text,"LAST_N")==0 || strcmp(L->cur.text,"EXIT")==0))){
      code = parse_expr(vm, L);
    }
    if (L->cur.kind == TK_STR){
      snprintf(why, sizeof why, "%s", L->cur.text);
      lex_next(L);
    }
    if (code < 0) code = 1;
    if (code > 125) code = 1;
    vm->halt = 1;
    vm->exit_code = (int)code;
    vm->break_loop = 1;
    vm->return_fn = 1;
    var_set_num(vm, "EXIT", code);
    var_set_num(vm, "LAST_N", code);
    vm->last_n = code;
    if (code != 0) {
      char msg[160];
      if (why[0])
        snprintf(msg, sizeof msg, "EXIT %ld line %d: %s", code, aln, why);
      else
        snprintf(msg, sizeof msg, "EXIT %ld line %d", code, aln);
      var_set_str(vm, "ERR", msg);
      var_set_str(vm, "LAST_ERR", msg);
      var_set_str(vm, "LAST", msg);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", msg);
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "EXPECT_OK", 0);
      if (vm->trace) fprintf(vm->trace, "# exit %ld: %s\n", code, why[0]?why:"");
    } else {
      if (why[0]) {
        var_set_str(vm, "LAST", why);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", why);
      } else {
        var_set_str(vm, "LAST", "exit:0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "exit:0");
      }
      var_set_num(vm, "OK", 1);
      if (vm->trace) fprintf(vm->trace, "# exit 0\n");
    }
    bump(vm); return 1;
  }
  /* TIPS [topic] — curated agent next-steps bag (dual of cubalc tips).
   * Usability: recover/start without cookbook · topics general|cap|fat|plate|p2p|run|lib|protect.
   * LAST = newline tips · LAST_N = count · TIPS_TOPIC set · OK=1. */
  if (kw(&L->cur,"TIPS")||kw(&L->cur,"TIP")||kw(&L->cur,"HOWTO")||
      kw(&L->cur,"PLAYBOOK")||kw(&L->cur,"NEXTSTEPS")||kw(&L->cur,"AGENTTIPS")){
    static const struct { const char *topic; const char *tip; } tips[] = {
      {"general", "cubalc doctor — install readiness plate"},
      {"general", "cubalc init --list — scaffold catalog"},
      {"general", "cubalc search KEYWORD — forms/libs/examples"},
      {"general", "INCLUDE agent_boot — REQUIRE VERSION + VERSION plate"},
      {"general", "HOLD_FLASH device/mesh-join only · default 1"},
      {"cap", "INCLUDE cap_boot — agent_boot + form_guard"},
      {"cap", "cubalc init --cap · HASFORM · NEEDFORMS · FORMHINT"},
      {"cap", "cubalc run -C FORMS · CUBALC_REQUIRE_FORMS floor"},
      {"cap", "LISTFORMS prefix · COUNTFORMS · cubalc listforms"},
      {"fat", "INCLUDE fat_session durable nest"},
      {"fat", "cubalc init --fat-session · NEED_VARROOM · time_guard"},
      {"fat", "cubalc run -T ms · REMAIN_MS · VARROOM"},
      {"plate", "INCLUDE plate_session · SETP NEEDP SAVEPLATE"},
      {"plate", "cubalc plate get|set|uniform · PRETTYP"},
      {"plate", "cubalc init --plate plate_uniform starter"},
      {"p2p", "export CUBALC_SMX_KEY hex64 · SMX SERVE/DIAL"},
      {"p2p", "CUBALC_P2P_TIMEOUT · CUBALC_P2P_SOFT · smx-bus prove-tcp"},
      {"run", "cubalc run -q -e CODE · -I -L -R -C -T -s"},
      {"run", "WHY / why_hint after ASSERT or sticky LAST_ERR"},
      {"lib", "cubalc libs|recipe|checkdeps · INCLUDE MATCH|NEWEST"},
      {"lib", "SORTLIBS · FRESHLIBS · LIBAGE · HASFRESH"},
      {"protect", "cubalc protect status · docs/CORE_PROTECT.md"},
    };
    char topic[32], tup[32], bag[4096];
    size_t k, o = 0;
    int i, n = 0, nall = (int)(sizeof tips / sizeof tips[0]);
    lex_next(L);
    topic[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(topic, sizeof topic, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      /* Always lex_next after consuming topic (var/LAST/bare) — same class as LISTFORMS.
       * Missing advance: FORMSFOR t / TIPS t → unknown form 't' (realworld 299). */
      if (vv && vv->is_str && vv->sval[0]) {
        snprintf(topic, sizeof topic, "%s", vv->sval);
        lex_next(L);
      } else if (strcmp(L->cur.text, "LAST") == 0) {
        snprintf(topic, sizeof topic, "%s", vm->last_str);
        lex_next(L);
      } else if (!(kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
                 kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"PASS") ||
                 kw(&L->cur,"FAIL") || kw(&L->cur,"NOTE") || kw(&L->cur,"STATUS"))) {
        snprintf(topic, sizeof topic, "%s", L->cur.text);
        lex_next(L);
      }
    }
    if (!topic[0])
      snprintf(topic, sizeof topic, "%s", "general");
    for (k = 0; topic[k] && k + 1 < sizeof tup; k++) {
      char c = topic[k];
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      tup[k] = c;
    }
    tup[k] = 0;
    if (!strcmp(tup, "capability") || !strcmp(tup, "forms") || !strcmp(tup, "form"))
      snprintf(tup, sizeof tup, "%s", "cap");
    if (!strcmp(tup, "mesh") || !strcmp(tup, "smx") || !strcmp(tup, "peer"))
      snprintf(tup, sizeof tup, "%s", "p2p");
    if (!strcmp(tup, "nest") || !strcmp(tup, "var"))
      snprintf(tup, sizeof tup, "%s", "fat");
    if (!strcmp(tup, "json") || !strcmp(tup, "agent"))
      snprintf(tup, sizeof tup, "%s", "plate");
    if (!strcmp(tup, "start") || !strcmp(tup, "all") || !strcmp(tup, "help"))
      snprintf(tup, sizeof tup, "%s", "general");
    bag[0] = 0;
    for (i = 0; i < nall; i++) {
      int hit = !strcmp(tips[i].topic, tup);
      size_t ln;
      if (!hit) continue;
      ln = strlen(tips[i].tip);
      if (o && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, tips[i].tip, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "TIPS", bag);
    var_set_str(vm, "TIPS_TOPIC", tup);
    var_set_str(vm, "HOWTO_TOPIC", tup);
    var_set_str(vm, "PLAYBOOK_TOPIC", tup);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "TIPS_N", n);
    var_set_num(vm, "HOWTO_N", n);
    var_set_num(vm, "OK", n > 0 ? 1 : 0);
    if (n == 0) {
      var_set_str(vm, "LAST_ERR",
                  "TIPS: unknown topic — TIPS general|cap|fat|plate|p2p|run|lib|protect");
      var_set_str(vm, "ERR",
                  "TIPS: unknown topic — TIPS general|cap|fat|plate|p2p|run|lib|protect");
    }
    bump(vm);
    return 1;
  }
  /* FORMSFOR|TOPICFORMS [topic] — curated form-name bag by usability topic.
   * Dual of TIPS (tips=prose next-steps; FORMSFOR=capability surface names).
   * Topics match TIPS: general|cap|fat|plate|p2p|run|lib|protect.
   * LAST = newline form names · LAST_N = count · FORMSFOR_TOPIC · OK=1. */
  if (kw(&L->cur,"FORMSFOR")||kw(&L->cur,"TOPICFORMS")||kw(&L->cur,"FORMSTOPIC")||
      kw(&L->cur,"FORMS_FOR")||kw(&L->cur,"SURFACE")||kw(&L->cur,"TOPICSURFACE")||
      kw(&L->cur,"CAPFORMS")||kw(&L->cur,"FORMSBYTOPIC")){
    static const struct { const char *topic; const char *form; } rows[] = {
      {"general", "VERSION"},
      {"general", "STATUS"},
      {"general", "IDENTITY"},
      {"general", "TIPS"},
      {"general", "FORMSFOR"},
      {"general", "LISTTOPICS"},
      {"general", "TOPICHINT"},
      {"general", "RELATEDTOPIC"},
      {"general", "FORMTOPICS"},
      {"general", "FORMGUIDE"},
      {"general", "GUIDE"},
      {"general", "START"},
      {"general", "WHY"},
      {"general", "CLEAR_ERR"},
      {"general", "INCLUDE"},
      {"general", "REQUIRE VERSION"},
      {"general", "VARS"},
      {"cap", "HASFORM"},
      {"cap", "NEEDFORM"},
      {"cap", "HASFORMS"},
      {"cap", "NEEDFORMS"},
      {"cap", "HASFORMANY"},
      {"cap", "NEEDFORMANY"},
      {"cap", "FORMHINT"},
      {"cap", "LISTFORMS"},
      {"cap", "COUNTFORMS"},
      {"cap", "REQUIRE FORM"},
      {"cap", "RELATEDTOPIC"},
      {"cap", "FORMTOPICS"},
      {"cap", "GUIDE"},
      {"cap", "TIPS"},
      {"cap", "FORMSFOR"},
      {"fat", "VARROOM"},
      {"fat", "HASVARROOM"},
      {"fat", "NEEDVARROOM"},
      {"fat", "REMAIN_MS"},
      {"fat", "HAS_TIME"},
      {"fat", "NEEDTIME"},
      {"fat", "STATUS"},
      {"fat", "VARS"},
      {"fat", "TIPS"},
      {"plate", "SETP"},
      {"plate", "GETP"},
      {"plate", "NEEDP"},
      {"plate", "DEFAULTP"},
      {"plate", "INCP"},
      {"plate", "DELP"},
      {"plate", "PRETTYP"},
      {"plate", "DUMPP"},
      {"plate", "SAVEPLATE"},
      {"plate", "LOADPLATE"},
      {"p2p", "SMX"},
      {"p2p", "SERVE"},
      {"p2p", "DIAL"},
      {"p2p", "TALK"},
      {"p2p", "EXCHANGE"},
      {"p2p", "TIPS"},
      {"run", "ASSERT"},
      {"run", "EXPECT"},
      {"run", "FAIL"},
      {"run", "PASS"},
      {"run", "NOTE"},
      {"run", "EXIT"},
      {"run", "WHY"},
      {"run", "CLEAR_ERR"},
      {"run", "TIPS"},
      {"lib", "LISTLIBS"},
      {"lib", "HASLIB"},
      {"lib", "MATCHLIBS"},
      {"lib", "PICKLIB"},
      {"lib", "RECIPE"},
      {"lib", "CHECKDEPS"},
      {"lib", "SORTLIBS"},
      {"lib", "FRESHLIBS"},
      {"lib", "LIBAGE"},
      {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"},
      {"protect", "STATUS"},
      {"protect", "VERSION"},
      {"protect", "TIPS"},
    };
    char topic[32], tup[32], bag[4096];
    size_t k, o = 0;
    int i, n = 0, nall = (int)(sizeof rows / sizeof rows[0]);
    lex_next(L);
    topic[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(topic, sizeof topic, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      /* Always lex_next after consuming topic (var/LAST/bare) — same class as LISTFORMS.
       * Missing advance: FORMSFOR t / TIPS t → unknown form 't' (realworld 299). */
      if (vv && vv->is_str && vv->sval[0]) {
        snprintf(topic, sizeof topic, "%s", vv->sval);
        lex_next(L);
      } else if (strcmp(L->cur.text, "LAST") == 0) {
        snprintf(topic, sizeof topic, "%s", vm->last_str);
        lex_next(L);
      } else if (!(kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
                 kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"PASS") ||
                 kw(&L->cur,"FAIL") || kw(&L->cur,"NOTE") || kw(&L->cur,"STATUS"))) {
        snprintf(topic, sizeof topic, "%s", L->cur.text);
        lex_next(L);
      }
    }
    if (!topic[0])
      snprintf(topic, sizeof topic, "%s", "general");
    for (k = 0; topic[k] && k + 1 < sizeof tup; k++) {
      char c = topic[k];
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      tup[k] = c;
    }
    tup[k] = 0;
    if (!strcmp(tup, "capability") || !strcmp(tup, "forms") || !strcmp(tup, "form"))
      snprintf(tup, sizeof tup, "%s", "cap");
    if (!strcmp(tup, "mesh") || !strcmp(tup, "smx") || !strcmp(tup, "peer"))
      snprintf(tup, sizeof tup, "%s", "p2p");
    if (!strcmp(tup, "nest") || !strcmp(tup, "var") || !strcmp(tup, "timeout"))
      snprintf(tup, sizeof tup, "%s", "fat");
    if (!strcmp(tup, "json") || !strcmp(tup, "agent"))
      snprintf(tup, sizeof tup, "%s", "plate");
    if (!strcmp(tup, "start") || !strcmp(tup, "all") || !strcmp(tup, "help") ||
        !strcmp(tup, "default") || !strcmp(tup, "*"))
      snprintf(tup, sizeof tup, "%s", "general");
    bag[0] = 0;
    for (i = 0; i < nall; i++) {
      int hit = !strcmp(rows[i].topic, tup);
      size_t ln;
      if (!hit) continue;
      ln = strlen(rows[i].form);
      if (o && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, rows[i].form, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "FORMSFOR", bag);
    var_set_str(vm, "TOPICFORMS", bag);
    var_set_str(vm, "FORMSFOR_TOPIC", tup);
    var_set_str(vm, "TOPICFORMS_TOPIC", tup);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "FORMSFOR_N", n);
    var_set_num(vm, "TOPICFORMS_N", n);
    var_set_num(vm, "OK", n > 0 ? 1 : 0);
    if (n == 0) {
      var_set_str(vm, "LAST_ERR",
                  "FORMSFOR: unknown topic — FORMSFOR general|cap|fat|plate|p2p|run|lib|protect");
      var_set_str(vm, "ERR",
                  "FORMSFOR: unknown topic — FORMSFOR general|cap|fat|plate|p2p|run|lib|protect");
    }
    bump(vm);
    return 1;
  }

  return 0;
}
