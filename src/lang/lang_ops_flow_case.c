/* CubalC lang — lang_ops_flow_case.c
 * FLOW CASE/SWITCH/MATCH residual (A3 peel).
 * Mesh vote A3_carve_lang_ops_flow · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_flow_case.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include "cubalc_sot.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_case(VM *vm, Lex *L) {
  /* FLOW CASE residual. Returns 1 handled, 0 not this plane, -1 fail.
   */
  /* CASE expr|str ... WHEN n|"s" [,|"OR"|] â¦ THEN ... [DEFAULT ...] END
   * CASEI|MATCHI|SWITCHI or CASE ICASE â case-insensitive string arms (strcasecmp).
   * Numeric arms keep TO-range. Multi-alias WHEN: comma / OR / |.
   * MATCH_ARM = which synonym hit. Usability: CLI GETFLAG mixed-case actions. */
  if (kw(&L->cur,"CASE")||kw(&L->cur,"SWITCH")||kw(&L->cur,"MATCH")||
      kw(&L->cur,"CASEI")||kw(&L->cur,"SWITCHI")||kw(&L->cur,"MATCHI")||
      kw(&L->cur,"ICASE")||kw(&L->cur,"CASECI")){
    long sel = 0;
    char sel_s[512];
    char match_arm[512];
    int sel_is_str = 0;
    int icase = 0;
    if (kw(&L->cur,"CASEI")||kw(&L->cur,"SWITCHI")||kw(&L->cur,"MATCHI")||
        kw(&L->cur,"ICASE")||kw(&L->cur,"CASECI"))
      icase = 1;
    lex_next(L);
    /* CASE ICASE|"CI"|IGNORECASE selector â modifier after CASE/MATCH/SWITCH */
    if (!icase && (kw(&L->cur,"ICASE")||kw(&L->cur,"IGNORECASE")||
                   kw(&L->cur,"NOCASE")||kw(&L->cur,"CI")||
                   kw(&L->cur,"CASEFOLD"))){
      icase = 1;
      lex_next(L);
    }
    sel_s[0] = 0;
    match_arm[0] = 0;
    /* string selector: "â¦", LAST (str), or string var â before parse_expr */
    if (L->cur.kind == TK_STR) {
      snprintf(sel_s, sizeof sel_s, "%s", L->cur.text);
      lex_next(L);
      sel_is_str = 1;
    } else if (L->cur.kind == TK_IDENT) {
      if (strcmp(L->cur.text, "LAST") == 0) {
        Var *lv = var_get(vm, "LAST", 0);
        if ((lv && lv->is_str) || vm->last_str[0]) {
          if (lv && lv->is_str)
            snprintf(sel_s, sizeof sel_s, "%s", lv->sval);
          else
            snprintf(sel_s, sizeof sel_s, "%s", vm->last_str);
          lex_next(L);
          sel_is_str = 1;
        }
      } else {
        Var *sv = var_get(vm, L->cur.text, 0);
        if (sv && sv->is_str) {
          snprintf(sel_s, sizeof sel_s, "%s", sv->sval);
          lex_next(L);
          sel_is_str = 1;
        }
      }
    }
    if (!sel_is_str)
      sel = parse_expr(vm, L);
    skip_nl(L);
    int matched = 0;
    int ran = 0;
    for(;;){
      skip_nl(L);
      if (L->cur.kind==TK_EOF){ fail(vm,"CASE without END"); return -1; }
      if (kw(&L->cur,"END")){ lex_next(L); break; }
      if (kw(&L->cur,"WHEN")||kw(&L->cur,"OF")||kw(&L->cur,"CASEIF")){
        long w = 0, w_hi = 0;
        char w_s[512];
        int is_range = 0, arm_is_str = 0;
        int arm_hit = 0;
        lex_next(L);
        w_s[0] = 0;
        if (sel_is_str) {
          /* string arm: literal or string var (+ multi-alias below) */
          if (L->cur.kind == TK_STR) {
            snprintf(w_s, sizeof w_s, "%s", L->cur.text);
            lex_next(L);
            arm_is_str = 1;
          } else if (L->cur.kind == TK_IDENT && !kw(&L->cur,"TO") &&
                     !kw(&L->cur,"THEN") && !kw(&L->cur,"THROUGH") &&
                     !kw(&L->cur,"OR")) {
            if (strcmp(L->cur.text, "LAST") == 0) {
              Var *lv = var_get(vm, "LAST", 0);
              if (lv && lv->is_str)
                snprintf(w_s, sizeof w_s, "%s", lv->sval);
              else
                snprintf(w_s, sizeof w_s, "%s", vm->last_str);
              lex_next(L);
              arm_is_str = 1;
            } else {
              Var *sv = var_get(vm, L->cur.text, 0);
              if (sv && sv->is_str) {
                snprintf(w_s, sizeof w_s, "%s", sv->sval);
                lex_next(L);
                arm_is_str = 1;
              } else {
                /* numeric WHEN under string CASE â no match (coerce fail soft) */
                w = parse_expr(vm, L);
                arm_is_str = 0;
              }
            }
          } else {
            w = parse_expr(vm, L);
            arm_is_str = 0;
          }
          if (arm_is_str &&
              (icase ? strcasecmp(sel_s, w_s) == 0 : strcmp(sel_s, w_s) == 0)) {
            arm_hit = 1;
            snprintf(match_arm, sizeof match_arm, "%s", w_s);
          }
          /* Multi-alias: WHEN "list", "ls" OR "l" | "L" THEN */
          for (;;) {
            int more = 0;
            if (L->cur.kind == TK_COMMA) { lex_next(L); more = 1; }
            else if (L->cur.kind == TK_PIPE) { lex_next(L); more = 1; }
            else if (kw(&L->cur, "OR")) {
              lex_next(L);
              more = 1;
            }
            if (!more) break;
            skip_nl(L);
            w_s[0] = 0;
            arm_is_str = 0;
            if (L->cur.kind == TK_STR) {
              snprintf(w_s, sizeof w_s, "%s", L->cur.text);
              lex_next(L);
              arm_is_str = 1;
            } else if (L->cur.kind == TK_IDENT && !kw(&L->cur, "THEN") &&
                       !kw(&L->cur, "OR") && !kw(&L->cur, "WHEN") &&
                       !kw(&L->cur, "DEFAULT") && !kw(&L->cur, "ELSE") &&
                       !kw(&L->cur, "END")) {
              if (strcmp(L->cur.text, "LAST") == 0) {
                Var *lv = var_get(vm, "LAST", 0);
                if (lv && lv->is_str)
                  snprintf(w_s, sizeof w_s, "%s", lv->sval);
                else
                  snprintf(w_s, sizeof w_s, "%s", vm->last_str);
                lex_next(L);
                arm_is_str = 1;
              } else {
                Var *sv = var_get(vm, L->cur.text, 0);
                if (sv && sv->is_str) {
                  snprintf(w_s, sizeof w_s, "%s", sv->sval);
                  lex_next(L);
                  arm_is_str = 1;
                } else {
                  (void)parse_expr(vm, L); /* soft skip non-str */
                  arm_is_str = 0;
                }
              }
            } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
                       L->cur.kind == TK_MINUS || L->cur.kind == TK_PLUS) {
              (void)parse_expr(vm, L);
              arm_is_str = 0;
            } else {
              break; /* THEN / WHEN / END â stop alias list */
            }
            if (arm_is_str &&
                (icase ? strcasecmp(sel_s, w_s) == 0 : strcmp(sel_s, w_s) == 0)) {
              arm_hit = 1;
              if (!match_arm[0])
                snprintf(match_arm, sizeof match_arm, "%s", w_s);
            }
          }
        } else {
          /* atom parse â leaves OR for multi-alias (not logical OR expr) */
          w = case_when_num_atom(vm, L);
          w_hi = w;
          /* WHEN lo TO hi THEN â inclusive range arm (digit-1 control_or_branch) */
          if (kw(&L->cur,"TO")||kw(&L->cur,"THROUGH")||kw(&L->cur,"THRU")||
              kw(&L->cur,"DOTDOT")||kw(&L->cur,"RANGE")){
            lex_next(L);
            w_hi = case_when_num_atom(vm, L);
            is_range = 1;
            {
              long lo = w, hi = w_hi;
              if (hi < lo){ long t = lo; lo = hi; hi = t; }
              arm_hit = (sel >= lo && sel <= hi);
            }
          } else {
            arm_hit = (w == sel);
            /* Multi-alias numeric: WHEN 1, 2 OR 3 THEN (not mixed with TO) */
            for (;;) {
              int more = 0;
              if (L->cur.kind == TK_COMMA) { lex_next(L); more = 1; }
              else if (L->cur.kind == TK_PIPE) { lex_next(L); more = 1; }
              else if (kw(&L->cur, "OR")) {
                lex_next(L);
                more = 1;
              }
              if (!more) break;
              skip_nl(L);
              if (kw(&L->cur, "THEN") || kw(&L->cur, "WHEN") ||
                  kw(&L->cur, "DEFAULT") || kw(&L->cur, "ELSE") ||
                  kw(&L->cur, "END") || L->cur.kind == TK_EOF)
                break;
              w = case_when_num_atom(vm, L);
              if (w == sel) arm_hit = 1;
            }
          }
        }
        if (kw(&L->cur,"THEN")) lex_next(L);
        skip_nl(L);
        Lex body_start=*L;
        int depth=1;
        while (L->cur.kind!=TK_EOF && depth>0){
          if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
            lex_next(L); if (kw(&L->cur,"IF")) lex_next(L); continue;
          }
          if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||
              kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FORCELL")||kw(&L->cur,"EACHCELL")||
              kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||
              kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")||
              kw(&L->cur,"CASE")||kw(&L->cur,"CASEI")||kw(&L->cur,"MATCHI")||
              kw(&L->cur,"SWITCHI")) depth++;
          else if ((kw(&L->cur,"WHEN")||kw(&L->cur,"OF")||kw(&L->cur,"DEFAULT")||kw(&L->cur,"ELSE")) && depth==1) break;
          else if (kw(&L->cur,"END")){ depth--; if (depth==0) break; }
          lex_next(L);
        }
        if (!matched && !ran && arm_hit){
          matched = 1; ran = 1;
          Lex body=body_start;
          /* arm body: stop before next WHEN/DEFAULT/END (body copy only) */
          while (!vm->fatal && !vm->halt){
            skip_nl(&body);
            if (body.cur.kind==TK_EOF) break;
            if (kw(&body.cur,"END")||kw(&body.cur,"WHEN")||kw(&body.cur,"OF")||
                kw(&body.cur,"DEFAULT")||kw(&body.cur,"ELSE")||kw(&body.cur,"CASEIF")) break;
            if (vm->return_fn || vm->break_loop) break;
            int r=parse_form(vm,&body);
            if (r<0) return -1;
            if (r==0) break;
          }
          /* skip remaining arms to END on outer L (parked on next arm or END) */
          depth=1;
          while (L->cur.kind!=TK_EOF && depth>0){
            if (kw(&L->cur,"CASE")||kw(&L->cur,"CASEI")||kw(&L->cur,"MATCHI")||
                kw(&L->cur,"SWITCHI")||
                kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||
                kw(&L->cur,"FOR")||kw(&L->cur,"FORCELL")||kw(&L->cur,"EACHCELL")||
                kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||
                kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")) depth++;
            else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
            lex_next(L);
          }
          if (kw(&L->cur,"END")) lex_next(L);
          break;
        }
        /* non-hit: clear provisional MATCH_ARM from this arm */
        if (!arm_hit) match_arm[0] = 0;
        continue;
      }
      if (kw(&L->cur,"DEFAULT")||kw(&L->cur,"ELSE")){
        lex_next(L);
        if (kw(&L->cur,"THEN")) lex_next(L);
        skip_nl(L);
        if (!matched && !ran){
          Lex body=*L;
          while (!vm->fatal && !vm->halt){
            skip_nl(&body);
            if (body.cur.kind==TK_EOF || kw(&body.cur,"END")) break;
            if (vm->return_fn || vm->break_loop) break;
            int r=parse_form(vm,&body);
            if (r<0) return -1;
            if (r==0) break;
          }
          ran = 1;
          match_arm[0] = 0; /* DEFAULT has no WHEN alias */
        }
        /* always advance outer L to matching END */
        {
          int depth=1;
          while (L->cur.kind!=TK_EOF && depth>0){
            if (kw(&L->cur,"CASE")||kw(&L->cur,"CASEI")||kw(&L->cur,"MATCHI")||
                kw(&L->cur,"SWITCHI")||
                kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||
                kw(&L->cur,"FOR")||kw(&L->cur,"FORCELL")||kw(&L->cur,"EACHCELL")||
                kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||
                kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")) depth++;
            else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
            lex_next(L);
          }
        }
        if (kw(&L->cur,"END")) lex_next(L);
        break;
      }
      fail(vm,"CASE expects WHEN|DEFAULT|END"); return -1;
    }
    var_set_num(vm, "MATCHED", matched || ran ? 1 : 0);
    if (sel_is_str) {
      var_set_str(vm, "CASE", sel_s);
      var_set_str(vm, "MATCH", sel_s);
    } else {
      var_set_num(vm, "CASE", sel);
      var_set_num(vm, "MATCH", sel);
    }
    /* which synonym matched (empty on DEFAULT / no arm) */
    var_set_str(vm, "MATCH_ARM", match_arm);
    var_set_str(vm, "WHEN_HIT", match_arm);
    var_set_num(vm, "CASE_ICASE", icase ? 1 : 0);
    var_set_num(vm, "ICASE", icase ? 1 : 0);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }

  return 0;
}
