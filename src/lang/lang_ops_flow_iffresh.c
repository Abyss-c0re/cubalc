/* CubalC lang — lang_ops_flow_iffresh.c
 * FLOW IFFRESH·WHENFRESH peel (A3 residual · age-window gates).
 * Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_iffresh.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

int cubalc_lang_ops_flow_try_iffresh(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* IFFRESH path [max_age_sec] â¦ [ELSE â¦] END â body if file age â¤ max (default 86400).
   * IFSTALE path [min_age_sec] â¦ â body if missing or age â¥ min (default 86400).
   * Complements SYS AGE + IF LAST_N. Miss path: IFFRESH false, IFSTALE true.
   * Usability: plate rebuild gates without AGE probe glue. */
  if (kw(&L->cur,"IFFRESH")||kw(&L->cur,"WHENFRESH")||kw(&L->cur,"IFFRESHFILE")||
      kw(&L->cur,"WHENFRESHFILE")||kw(&L->cur,"IFNEWFILE")||kw(&L->cur,"WHENNEWFILE")||
      kw(&L->cur,"IFSTALE")||kw(&L->cur,"WHENSTALE")||kw(&L->cur,"IFSTALEFILE")||
      kw(&L->cur,"WHENSTALEFILE")||kw(&L->cur,"IFOLDFILE")||kw(&L->cur,"WHENOLDFILE")||
      kw(&L->cur,"IFAGED")||kw(&L->cur,"WHENAGED")){
    int aln = L->cur.line;
    int is_stale = 0;
    int take = 0;
    char path[CUBALC_HOST_STR_MAX], op[24];
    long limit = 86400, age = 0;
    cubalc_host_result hr;
    Lex body_start;
    int depth = 1;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "IFSTALE") == 0 || strcmp(op, "WHENSTALE") == 0 ||
        strcmp(op, "IFSTALEFILE") == 0 || strcmp(op, "WHENSTALEFILE") == 0 ||
        strcmp(op, "IFOLDFILE") == 0 || strcmp(op, "WHENOLDFILE") == 0 ||
        strcmp(op, "IFAGED") == 0 || strcmp(op, "WHENAGED") == 0)
      is_stale = 1;
    lex_next(L);
    if (kw(&L->cur, "PATH") || kw(&L->cur, "FILE") || kw(&L->cur, "OF") ||
        kw(&L->cur, "AT") || kw(&L->cur, "FOR"))
      lex_next(L);
    path[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail(vm, is_stale ? "IFSTALE needs path [min_age] â IFSTALE plate 3600 â¦ END"
                        : "IFFRESH needs path [max_age] â IFFRESH plate 3600 â¦ END");
      return -1;
    }
    if (kw(&L->cur, "AGE") || kw(&L->cur, "MAX") || kw(&L->cur, "MIN") ||
        kw(&L->cur, "WITHIN") || kw(&L->cur, "SEC") || kw(&L->cur, "SECONDS") ||
        kw(&L->cur, "UNDER") || kw(&L->cur, "OVER") || kw(&L->cur, "THAN"))
      lex_next(L);
    /* optional age limit: number or numeric var */
    if (L->cur.kind == TK_NUM) {
      limit = L->cur.num;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT &&
               !kw(&L->cur, "THEN") && !kw(&L->cur, "END") && !kw(&L->cur, "ELSE") &&
               !kw(&L->cur, "IF") && !kw(&L->cur, "LET") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "PRINT") && !kw(&L->cur, "PASS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && !vv->is_str) {
        limit = vv->val;
        lex_next(L);
      } else if (vv && vv->is_str && vv->sval[0]) {
        char *end = NULL;
        long v = strtol(vv->sval, &end, 10);
        if (end && end != vv->sval) { limit = v; lex_next(L); }
      }
    }
    if (limit < 0) limit = 0;
    skip_nl(L);
    if (kw(&L->cur,"THEN")) { lex_next(L); skip_nl(L); }
    age = 0;
    take = 0;
    if (cubalc_host_mtime(path, &hr) != 0 || !hr.ok) {
      /* miss: not fresh; is stale */
      take = is_stale ? 1 : 0;
      age = -1;
    } else {
      long now = (long)time(NULL);
      age = now - hr.n;
      if (age < 0) age = 0;
      if (is_stale) take = (age >= limit) ? 1 : 0;
      else take = (age <= limit) ? 1 : 0;
    }
    body_start = *L;
    while (L->cur.kind != TK_EOF && depth > 0) {
      if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
        lex_next(L);
        if (kw(&L->cur,"IF")) lex_next(L);
        continue;
      }
      if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"IFERR")||kw(&L->cur,"IFOK")||
          kw(&L->cur,"IFEMPTY")||kw(&L->cur,"IFNONEMPTY")||kw(&L->cur,"IFDEFINED")||
          kw(&L->cur,"IFUNDEF")||kw(&L->cur,"IFSTARTS")||kw(&L->cur,"IFENDS")||
          kw(&L->cur,"IFCONTAINS")||kw(&L->cur,"IFHAS")||kw(&L->cur,"IFEQS")||
          kw(&L->cur,"IFSTARTSI")||kw(&L->cur,"IFENDSI")||kw(&L->cur,"IFCONTAINSI")||
          kw(&L->cur,"IFHASI")||kw(&L->cur,"IFEQSI")||
          kw(&L->cur,"IFGT")||kw(&L->cur,"IFLT")||kw(&L->cur,"IFGE")||kw(&L->cur,"IFLE")||
          kw(&L->cur,"IFEQN")||kw(&L->cur,"IFNEQN")||
          kw(&L->cur,"IFHASLINE")||kw(&L->cur,"IFINBAG")||kw(&L->cur,"WHENHASLINE")||
          kw(&L->cur,"WHENINBAG")||
          kw(&L->cur,"IFBETWEEN")||kw(&L->cur,"IFINRANGE")||kw(&L->cur,"IFWITHIN")||
          kw(&L->cur,"WHENBETWEEN")||
          kw(&L->cur,"IFFN")||kw(&L->cur,"IFCLASS")||kw(&L->cur,"IFHASFN")||
          kw(&L->cur,"WHENFN")||kw(&L->cur,"WHENCLASS")||
          kw(&L->cur,"IFOBJ")||kw(&L->cur,"IFHASOBJ")||kw(&L->cur,"IFMETHOD")||
          kw(&L->cur,"IFHASMETHOD")||kw(&L->cur,"WHENOBJ")||kw(&L->cur,"WHENMETHOD")||
          kw(&L->cur,"IFFILE")||kw(&L->cur,"IFDIR")||kw(&L->cur,"IFEXIST")||
          kw(&L->cur,"IFEXISTS")||kw(&L->cur,"WHENFILE")||kw(&L->cur,"WHENDIR")||
          kw(&L->cur,"WHENEXIST")||
          kw(&L->cur,"IFENV")||kw(&L->cur,"WHENENV")||kw(&L->cur,"IFHASENV")||
          kw(&L->cur,"WHENHASENV")||
          kw(&L->cur,"IFFIELD")||kw(&L->cur,"IFHASFIELD")||kw(&L->cur,"WHENFIELD")||
          kw(&L->cur,"WHENHASFIELD")||
          kw(&L->cur,"IFFRESH")||kw(&L->cur,"IFSTALE")||kw(&L->cur,"WHENFRESH")||
          kw(&L->cur,"WHENSTALE")||kw(&L->cur,"IFBIG")||kw(&L->cur,"IFSMALL")||kw(&L->cur,"IFSIZE")||
          kw(&L->cur,"WHENBIG")||kw(&L->cur,"WHENSMALL")||kw(&L->cur,"WHENSIZE")||
          kw(&L->cur,"IFNEWER")||kw(&L->cur,"IFOLDER")||kw(&L->cur,"IFOUTOFDATE")||
          kw(&L->cur,"WHENNEWER")||kw(&L->cur,"WHENOLDER")||kw(&L->cur,"WHENOUTOFDATE")||
          kw(&L->cur,"IFEQFILE")||kw(&L->cur,"IFDIFFILE")||kw(&L->cur,"IFSAMEFILE")||
          kw(&L->cur,"WHENEQFILE")||kw(&L->cur,"WHENDIFFILE")||kw(&L->cur,"WHENSAMEFILE")||
          kw(&L->cur,"WHENERR")||kw(&L->cur,"WHENOK")||
          kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||
          kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FN")||
          kw(&L->cur,"REPEAT")||kw(&L->cur,"TRY")||kw(&L->cur,"GUARD")||kw(&L->cur,"CASE")||
          kw(&L->cur,"TIMEIT")||kw(&L->cur,"RETRY")||kw(&L->cur,"TIMES"))
        depth++;
      else if (kw(&L->cur,"ELSE") && depth == 1) break;
      else if (kw(&L->cur,"END")) {
        depth--;
        if (depth == 0) break;
      }
      lex_next(L);
    }
    if (depth > 1 || (depth == 1 && L->cur.kind == TK_EOF)) {
      char ebuf[160];
      snprintf(ebuf, sizeof ebuf,
               "%s without END line %d â %s path [age] â¦ [ELSE â¦] END",
               is_stale ? "IFSTALE" : "IFFRESH", aln,
               is_stale ? "IFSTALE" : "IFFRESH");
      fail(vm, ebuf); return -1;
    }
    {
      const char *ntag = is_stale ? "IFSTALE_N" : "IFFRESH_N";
      var_set_num(vm, "IFFRESH_AGE", age);
      var_set_num(vm, "IFFRESH_LIMIT", limit);
      var_set_str(vm, "PATH", path);
      if (take) {
        Lex body = body_start;
        if (exec_stmts_until(vm, &body, "END", "ELSE") < 0) return -1;
        depth = 1;
        while (L->cur.kind != TK_EOF && depth > 0) {
          if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
            lex_next(L);
            if (kw(&L->cur,"IF")) lex_next(L);
            continue;
          }
          if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"IFERR")||kw(&L->cur,"IFOK")||
              kw(&L->cur,"IFEMPTY")||kw(&L->cur,"IFNONEMPTY")||kw(&L->cur,"IFDEFINED")||
              kw(&L->cur,"IFUNDEF")||kw(&L->cur,"IFSTARTS")||kw(&L->cur,"IFENDS")||
              kw(&L->cur,"IFCONTAINS")||kw(&L->cur,"IFHAS")||kw(&L->cur,"IFEQS")||
              kw(&L->cur,"IFSTARTSI")||kw(&L->cur,"IFENDSI")||kw(&L->cur,"IFCONTAINSI")||
              kw(&L->cur,"IFHASI")||kw(&L->cur,"IFEQSI")||
              kw(&L->cur,"IFGT")||kw(&L->cur,"IFLT")||kw(&L->cur,"IFGE")||kw(&L->cur,"IFLE")||
              kw(&L->cur,"IFEQN")||kw(&L->cur,"IFNEQN")||
              kw(&L->cur,"IFHASLINE")||kw(&L->cur,"IFINBAG")||kw(&L->cur,"WHENHASLINE")||
              kw(&L->cur,"WHENINBAG")||
              kw(&L->cur,"IFBETWEEN")||kw(&L->cur,"IFINRANGE")||kw(&L->cur,"IFWITHIN")||
              kw(&L->cur,"WHENBETWEEN")||
              kw(&L->cur,"IFFN")||kw(&L->cur,"IFCLASS")||kw(&L->cur,"IFHASFN")||
              kw(&L->cur,"WHENFN")||kw(&L->cur,"WHENCLASS")||
              kw(&L->cur,"IFOBJ")||kw(&L->cur,"IFHASOBJ")||kw(&L->cur,"IFMETHOD")||
              kw(&L->cur,"IFHASMETHOD")||kw(&L->cur,"WHENOBJ")||kw(&L->cur,"WHENMETHOD")||
              kw(&L->cur,"IFFILE")||kw(&L->cur,"IFDIR")||kw(&L->cur,"IFEXIST")||
              kw(&L->cur,"IFEXISTS")||kw(&L->cur,"WHENFILE")||kw(&L->cur,"WHENDIR")||
              kw(&L->cur,"WHENEXIST")||
              kw(&L->cur,"IFENV")||kw(&L->cur,"WHENENV")||kw(&L->cur,"IFHASENV")||
              kw(&L->cur,"WHENHASENV")||
              kw(&L->cur,"IFFIELD")||kw(&L->cur,"IFHASFIELD")||kw(&L->cur,"WHENFIELD")||
              kw(&L->cur,"WHENHASFIELD")||
              kw(&L->cur,"IFFRESH")||kw(&L->cur,"IFSTALE")||kw(&L->cur,"WHENFRESH")||
              kw(&L->cur,"WHENSTALE")||kw(&L->cur,"IFBIG")||kw(&L->cur,"IFSMALL")||kw(&L->cur,"IFSIZE")||
              kw(&L->cur,"WHENBIG")||kw(&L->cur,"WHENSMALL")||kw(&L->cur,"WHENSIZE")||
              kw(&L->cur,"IFNEWER")||kw(&L->cur,"IFOLDER")||kw(&L->cur,"IFOUTOFDATE")||
              kw(&L->cur,"WHENNEWER")||kw(&L->cur,"WHENOLDER")||kw(&L->cur,"WHENOUTOFDATE")||
              kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||
              kw(&L->cur,"EACH")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")||kw(&L->cur,"TRY")||
              kw(&L->cur,"CASE")||kw(&L->cur,"TIMEIT")||kw(&L->cur,"RETRY")||kw(&L->cur,"TIMES"))
            depth++;
          else if (kw(&L->cur,"END")) {
            depth--;
            if (depth == 0) break;
          }
          lex_next(L);
        }
        if (kw(&L->cur,"END")) lex_next(L);
        var_set_num(vm, ntag, 1);
        bump(vm); return 1;
      }
      if (kw(&L->cur,"ELSE")) {
        lex_next(L);
        skip_nl(L);
        {
          Lex body = *L;
          if (exec_stmts_until(vm, &body, "END", NULL) < 0) return -1;
        }
        depth = 1;
        while (L->cur.kind != TK_EOF && depth > 0) {
          if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
            lex_next(L);
            if (kw(&L->cur,"IF")) lex_next(L);
            continue;
          }
          if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"IFERR")||kw(&L->cur,"IFOK")||
              kw(&L->cur,"IFEMPTY")||kw(&L->cur,"IFNONEMPTY")||kw(&L->cur,"IFDEFINED")||
              kw(&L->cur,"IFUNDEF")||kw(&L->cur,"IFSTARTS")||kw(&L->cur,"IFENDS")||
              kw(&L->cur,"IFCONTAINS")||kw(&L->cur,"IFHAS")||kw(&L->cur,"IFEQS")||
              kw(&L->cur,"IFSTARTSI")||kw(&L->cur,"IFENDSI")||kw(&L->cur,"IFCONTAINSI")||
              kw(&L->cur,"IFHASI")||kw(&L->cur,"IFEQSI")||
              kw(&L->cur,"IFGT")||kw(&L->cur,"IFLT")||kw(&L->cur,"IFGE")||kw(&L->cur,"IFLE")||
              kw(&L->cur,"IFEQN")||kw(&L->cur,"IFNEQN")||
              kw(&L->cur,"IFHASLINE")||kw(&L->cur,"IFINBAG")||kw(&L->cur,"WHENHASLINE")||
              kw(&L->cur,"WHENINBAG")||
              kw(&L->cur,"IFBETWEEN")||kw(&L->cur,"IFINRANGE")||kw(&L->cur,"IFWITHIN")||
              kw(&L->cur,"WHENBETWEEN")||
              kw(&L->cur,"IFFN")||kw(&L->cur,"IFCLASS")||kw(&L->cur,"IFHASFN")||
              kw(&L->cur,"WHENFN")||kw(&L->cur,"WHENCLASS")||
              kw(&L->cur,"IFOBJ")||kw(&L->cur,"IFHASOBJ")||kw(&L->cur,"IFMETHOD")||
              kw(&L->cur,"IFHASMETHOD")||kw(&L->cur,"WHENOBJ")||kw(&L->cur,"WHENMETHOD")||
              kw(&L->cur,"IFFILE")||kw(&L->cur,"IFDIR")||kw(&L->cur,"IFEXIST")||
              kw(&L->cur,"IFEXISTS")||kw(&L->cur,"WHENFILE")||kw(&L->cur,"WHENDIR")||
              kw(&L->cur,"WHENEXIST")||
              kw(&L->cur,"IFENV")||kw(&L->cur,"WHENENV")||kw(&L->cur,"IFHASENV")||
              kw(&L->cur,"WHENHASENV")||
              kw(&L->cur,"IFFIELD")||kw(&L->cur,"IFHASFIELD")||kw(&L->cur,"WHENFIELD")||
              kw(&L->cur,"WHENHASFIELD")||
              kw(&L->cur,"IFFRESH")||kw(&L->cur,"IFSTALE")||kw(&L->cur,"WHENFRESH")||
              kw(&L->cur,"WHENSTALE")||kw(&L->cur,"IFBIG")||kw(&L->cur,"IFSMALL")||kw(&L->cur,"IFSIZE")||
              kw(&L->cur,"WHENBIG")||kw(&L->cur,"WHENSMALL")||kw(&L->cur,"WHENSIZE")||
              kw(&L->cur,"IFNEWER")||kw(&L->cur,"IFOLDER")||kw(&L->cur,"IFOUTOFDATE")||
              kw(&L->cur,"WHENNEWER")||kw(&L->cur,"WHENOLDER")||kw(&L->cur,"WHENOUTOFDATE")||
              kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||
              kw(&L->cur,"EACH")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")||kw(&L->cur,"TRY")||
              kw(&L->cur,"CASE")||kw(&L->cur,"TIMEIT")||kw(&L->cur,"RETRY")||kw(&L->cur,"TIMES"))
            depth++;
          else if (kw(&L->cur,"END")) {
            depth--;
            if (depth == 0) break;
          }
          lex_next(L);
        }
        if (kw(&L->cur,"END")) lex_next(L);
        var_set_num(vm, ntag, 0);
        bump(vm); return 1;
      }
      if (kw(&L->cur,"END")) lex_next(L);
      var_set_num(vm, ntag, 0);
      bump(vm); return 1;
    }
  }
  return 0;
}
