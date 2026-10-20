/* CubalC lang — lang_ops_flow_fn.c
 * FLOW FN/CALL/CALLANY/RET residual (A3 peel).
 * Mesh vote A3_carve_lang_ops_flow · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_flow_fn.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include "cubalc_sot.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cubalc_lang_ops_flow_try_fn(VM *vm, Lex *L) {
  /* FLOW FN/CALL residual. Returns 1 handled, 0 not this plane, -1 fail.
   */
  if (kw(&L->cur,"FN")||kw(&L->cur,"FUNC")||kw(&L->cur,"FUNCTION")||kw(&L->cur,"DEF")){
    char fname[48];
    char params[8][32];
    int n_params = 0;
    size_t b0, b1, blen;
    int depth = 1;
    FnDef *fn;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail_at(vm,L,"FN needs name [params] â¦ END â FN add a b"); return -1; }
    snprintf(fname,sizeof fname,"%s",L->cur.text); lex_next(L);
    memset(params, 0, sizeof params);
    while (n_params < 8 && L->cur.kind == TK_IDENT &&
           !kw(&L->cur, "END") && !kw(&L->cur, "THEN")) {
      snprintf(params[n_params], sizeof params[0], "%s", L->cur.text);
      n_params++;
      lex_next(L);
    }
    skip_nl(L);
    b0 = L->tok_off;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail_at(vm,L,"FN without END â close FN body with END"); return -1; }
    b1 = L->tok_off;
    if (b1 < b0) b1 = b0;
    blen = b1 - b0;
    if (vm->n_fns >= CUBALC_MAX_FNS){ fail(vm,"too many FN"); return -1; }
    fn = &vm->fns[vm->n_fns++];
    memset(fn, 0, sizeof *fn);
    snprintf(fn->name, sizeof fn->name, "%s", fname);
    fn->body = L->s + b0;
    fn->len = blen;
    fn->n_params = n_params;
    {
      int pi;
      for (pi = 0; pi < n_params; pi++)
        snprintf(fn->params[pi], sizeof fn->params[0], "%s", params[pi]);
    }
    if (kw(&L->cur,"END")) lex_next(L);
    if (vm->trace) fprintf(vm->trace, "# FN %s params=%d len=%zu\n", fname, n_params, blen);
    bump(vm); return 1;
  }
  /* CALL name [args] [OR|DEFAULT fallback] Â· TRYCALL|CALL SOFT â soft miss.
   * TRYCALL â¦ OR val / CALL â¦ OR val â miss â LAST=fallback, OK=1, CALL_OR=1
   * (GETF/ENV dual). Bare CALL still fatal without OR. Complements HASFN/FNINFO.
   * Usability: optional plugin hooks with default without IF HASFN glue. */
  if (kw(&L->cur,"CALL")||kw(&L->cur,"RUNFN")||kw(&L->cur,"DO")||
      kw(&L->cur,"CALLIF")||kw(&L->cur,"CALLNZ")||kw(&L->cur,"CALLZ")||
      kw(&L->cur,"CALLWHEN")||kw(&L->cur,"CALLUNLESS")||
      kw(&L->cur,"TRYCALL")||kw(&L->cur,"CALLSOFT")||kw(&L->cur,"SOFTCALL")||
      kw(&L->cur,"TRYFN")||kw(&L->cur,"TRYRUNFN")||
      kw(&L->cur,"TRYCALLFN")||kw(&L->cur,"CALLTRY")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    char fname[48];
    int mode = 0;
    long cond = 1;
    int do_call = 1;
    int soft = 0;
    FnDef *fn=NULL;
    int i;
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    if (strcmp(op,"TRYCALL")==0 || strcmp(op,"CALLSOFT")==0 ||
        strcmp(op,"SOFTCALL")==0 || strcmp(op,"TRYFN")==0 ||
        strcmp(op,"TRYRUNFN")==0 || strcmp(op,"TRYCALLFN")==0 ||
        strcmp(op,"CALLTRY")==0)
      soft = 1;
    lex_next(L);
    /* CALL SOFT|TRY|OPT name â two-token soft form (like SEND SOFT) */
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (strcmp(op,"CALLIF")==0 || strcmp(op,"CALLNZ")==0 || strcmp(op,"CALLWHEN")==0) mode = 1;
    else if (strcmp(op,"CALLZ")==0 || strcmp(op,"CALLUNLESS")==0) mode = 2;
    if (mode) cond = parse_expr(vm, L);
    if (L->cur.kind == TK_STR) {
      snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(fname, sizeof fname, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(fname, sizeof fname, "%s", vm->last_str);
      else
        snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail_at(vm, L, soft ? "TRYCALL needs FN name [args] â TRYCALL add 1 2" : "CALL needs FN name [args] â CALL add 1 2"); return -1;
    }
    if (mode == 1) do_call = (cond != 0);
    else if (mode == 2) do_call = (cond == 0);
    if (!do_call){
      int ai=0;
      while (ai<8 && (L->cur.kind==TK_NUM||L->cur.kind==TK_IDENT||L->cur.kind==TK_STR||L->cur.kind==TK_MINUS||L->cur.kind==TK_LPAREN)){
        if (oop_stmt_kw(L)) break;
        if (L->cur.kind==TK_STR){ lex_next(L); }
        else { (void)parse_expr(vm,L); }
        ai++;
      }
      var_set_num(vm,"CALLED",0);
      var_set_num(vm,"TRYCALL_N",0);
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    /* CALL obj method â OOP sugar when first name is a live object.
     * Peek method name without consuming unless method exists (so FN args stay). */
    {
      ObjInst *ob = oop_find_obj(vm, fname);
      if (ob && (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR)) {
        ClassDef *cd = &vm->classes[ob->class_idx];
        char mname[48];
        MethodDef *md;
        if (L->cur.kind == TK_STR)
          snprintf(mname, sizeof mname, "%s", L->cur.text);
        else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(mname, sizeof mname, "%s", vv->sval);
          else
            snprintf(mname, sizeof mname, "%s", L->cur.text);
        }
        md = oop_find_method(cd, mname);
        if (md) {
          int got, ag;
          char qname[96];
          lex_next(L);
          got = oop_bind_args(vm, L, md->params, md->n_params);
          snprintf(qname, sizeof qname, "%s.%s", cd->name, md->name);
          ag = oop_arity_gate(vm, "CALL", qname, md->params, md->n_params, got, soft);
          if (ag < 0) return -1;
          if (ag > 0) {
            var_set_num(vm, "CALLED", 0);
            var_set_num(vm, "TRYCALL_N", 0);
            bump(vm);
            return 1;
          }
          if (oop_run_method(vm, ob, md) < 0) return -1;
          var_set_num(vm, "CALLED", 1);
          var_set_num(vm, "TRYCALL_N", 1);
          var_set_num(vm, "OK", 1);
          bump(vm);
          return 1;
        }
        /* no method â fall through to FN lookup; leave token for args */
      }
    }
    for (i=0;i<vm->n_fns;i++) if (strcmp(vm->fns[i].name,fname)==0){ fn=&vm->fns[i]; break; }
    if (!fn){
      char sug[48], ebuf[160], base[96], fb[256];
      int or_rc;
      oop_suggest_fn(vm, fname, sug, sizeof sug);
      snprintf(base, sizeof base, "CALL unknown FN %s", fname);
      oop_err_suggest(ebuf, sizeof ebuf, base, sug);
      /* drain args but stop at OR/DEFAULT (oop_stmt_kw) */
      while (L->cur.kind == TK_NUM || L->cur.kind == TK_STR ||
             L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
        if (L->cur.kind == TK_STR) lex_next(L);
        else (void)parse_expr(vm, L);
      }
      or_rc = call_parse_or_fallback(vm, L, fb, sizeof fb);
      if (or_rc < 0) return -1;
      if (or_rc > 0) {
        /* CALL/TRYCALL â¦ OR fallback â soft miss with default (GETF OR twin) */
        if (fb[0] && (fb[0] == '-' || (fb[0] >= '0' && fb[0] <= '9'))) {
          long v = 0;
          char *end = NULL;
          v = strtol(fb, &end, 10);
          if (end && end != fb && !*end) {
            var_set_num(vm, "LAST_N", v);
            vm->last_n = v;
            var_set_str(vm, "LAST", fb);
            snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
          } else {
            var_set_str(vm, "LAST", fb);
            snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
            var_set_num(vm, "LAST_N", 0);
            vm->last_n = 0;
          }
        } else {
          var_set_str(vm, "LAST", fb);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
          var_set_num(vm, "LAST_N", 0);
          vm->last_n = 0;
        }
        var_set_num(vm, "CALLED", 0);
        var_set_num(vm, "TRYCALL_N", 0);
        var_set_num(vm, "CALL_OR", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "FN", fname);
        /* clear sticky err â fallback recovered */
        var_set_str(vm, "LAST_ERR", "");
        var_set_str(vm, "ERR", "");
        if (vm->trace)
          fprintf(vm->trace, "# CALL OR fallback after miss FN %s\n", fname);
        bump(vm);
        return 1;
      }
      if (soft) {
        var_set_num(vm, "CALLED", 0);
        var_set_num(vm, "TRYCALL_N", 0);
        var_set_num(vm, "CALL_OR", 0);
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
        var_set_str(vm, "FN", fname);
        bump(vm);
        return 1;
      }
      fail(vm, ebuf); return -1;
    }
    {
      int got = oop_bind_args(vm, L, fn->params, fn->n_params);
      int ag = oop_arity_gate(vm, "CALL", fn->name, fn->params, fn->n_params, got, soft);
      if (ag < 0) return -1;
      if (ag > 0) {
        char fb[256];
        int or_rc = call_parse_or_fallback(vm, L, fb, sizeof fb);
        if (or_rc < 0) return -1;
        if (or_rc > 0) {
          var_set_str(vm, "LAST", fb);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
          var_set_num(vm, "LAST_N", 0);
          vm->last_n = 0;
          var_set_num(vm, "CALLED", 0);
          var_set_num(vm, "TRYCALL_N", 0);
          var_set_num(vm, "CALL_OR", 1);
          var_set_num(vm, "OK", 1);
          var_set_str(vm, "FN", fn->name);
          var_set_str(vm, "LAST_ERR", "");
          var_set_str(vm, "ERR", "");
          bump(vm);
          return 1;
        }
        var_set_num(vm, "CALLED", 0);
        var_set_num(vm, "TRYCALL_N", 0);
        var_set_num(vm, "CALL_OR", 0);
        var_set_str(vm, "FN", fn->name);
        bump(vm);
        return 1;
      }
    }
    var_set_num(vm, "CALLED", 1);
    var_set_num(vm, "TRYCALL_N", 1);
    var_set_str(vm, "FN", fn->name);
    vm->return_fn = 0;
    {
      Lex fl; lex_init(&fl, fn->body, fn->len);
      if (exec_stmts_until(vm, &fl, "END", NULL)<0) return -1;
    }
    vm->return_fn = 0;
    /* discard unused OR fallback after successful CALL */
    {
      char fb[256];
      int or_rc = call_parse_or_fallback(vm, L, fb, sizeof fb);
      if (or_rc < 0) return -1;
      var_set_num(vm, "CALL_OR", 0);
    }
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* CALLANY|TRYCALLANY namesâ¦ [WITH argsâ¦] [OR|DEFAULT fb]
   * Soft TRYCALLANY: OK=0 if none defined (no OR). Hard CALLANY: fail-fast.
   * â¦ OR fb â none defined / soft arity miss â LAST=fb, OK=1, CALLANY_OR=1 (CALL OR twin).
   * Usability: multi-plugin hooks with default without HASFNANY+IF+CALL glue.
   * 0-arity when no WITH; WITH introduces shared args for the chosen FN. */
  if (kw(&L->cur, "CALLANY") || kw(&L->cur, "CALL_ANY") ||
      kw(&L->cur, "CALLFIRST") || kw(&L->cur, "FIRSTFN") ||
      kw(&L->cur, "CALLFNANY") || kw(&L->cur, "RUNFNANY") ||
      kw(&L->cur, "TRYCALLANY") || kw(&L->cur, "TRY_CALL_ANY") ||
      kw(&L->cur, "SOFTCALLANY") || kw(&L->cur, "CALLANYSOFT") ||
      kw(&L->cur, "NEEDCALLANY") || kw(&L->cur, "REQUIRECALLANY")) {
    int soft = kw(&L->cur, "TRYCALLANY") || kw(&L->cur, "TRY_CALL_ANY") ||
               kw(&L->cur, "SOFTCALLANY") || kw(&L->cur, "CALLANYSOFT");
    char names[32][48];
    char fname[48];
    int nname = 0, i, j, aln = L->cur.line;
    FnDef *fn = NULL;
    lex_next(L);
    if (kw(&L->cur, "OF") || kw(&L->cur, "FN") || kw(&L->cur, "FROM"))
      lex_next(L);
    while (nname < 32 &&
           (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur, "WITH") || kw(&L->cur, "ARGS") || kw(&L->cur, "USING") ||
           kw(&L->cur, "ASSERT") || kw(&L->cur, "LET") || kw(&L->cur, "PRINT") ||
           kw(&L->cur, "REQUIRE") || kw(&L->cur, "END") ||
           kw(&L->cur, "IF") || kw(&L->cur, "ELSE") || kw(&L->cur, "ELIF") ||
           kw(&L->cur, "PASS") || kw(&L->cur, "FAIL") || kw(&L->cur, "INCLUDE") ||
           kw(&L->cur, "SYS") || kw(&L->cur, "HELP") || kw(&L->cur, "CLEAR_ERR") ||
           kw(&L->cur, "NOTE") || kw(&L->cur, "EXIT") || kw(&L->cur, "DEFAULT") ||
           kw(&L->cur, "OR") || kw(&L->cur, "FALLBACK") ||
           kw(&L->cur, "VERSION") || kw(&L->cur, "STATUS") || kw(&L->cur, "FOR") ||
           kw(&L->cur, "WHILE") || kw(&L->cur, "LOOP") || kw(&L->cur, "EACH") ||
           kw(&L->cur, "CUBE") || kw(&L->cur, "PLUG") || kw(&L->cur, "HOLD_FLASH") ||
           kw(&L->cur, "CALL") || kw(&L->cur, "TRYCALL") || kw(&L->cur, "CALLANY") ||
           kw(&L->cur, "TRYCALLANY") || kw(&L->cur, "HASFN") || kw(&L->cur, "HASFNS") ||
           kw(&L->cur, "HASFNANY") || kw(&L->cur, "LISTFNS") || kw(&L->cur, "FNINFO") ||
           kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") || kw(&L->cur, "SEND") ||
           kw(&L->cur, "RET") || kw(&L->cur, "RETURN")))
        break;
      if (L->cur.kind == TK_STR) {
        const char *s = L->cur.text;
        if (strchr(s, '\n') || strchr(s, ',') || strchr(s, ' ')) {
          const char *p = s;
          while (*p && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
          continue;
        }
        snprintf(names[nname++], sizeof names[0], "%s", s);
        lex_next(L);
      } else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (strchr(vv->sval, '\n') || strchr(vv->sval, ',') ||
             strchr(vv->sval, ' '))) {
          const char *p = vv->sval;
          while (*p && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(names[nname++], sizeof names[0], "%s", vm->last_str);
          lex_next(L);
        } else if (vv && vv->is_str && vv->sval[0] &&
                   !strchr(vv->sval, '\n') && !strchr(vv->sval, ',') &&
                   !strchr(vv->sval, ' ')) {
          snprintf(names[nname++], sizeof names[0], "%s", vv->sval);
          lex_next(L);
        } else {
          snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
          lex_next(L);
        }
      }
    }
    if (kw(&L->cur, "WITH") || kw(&L->cur, "ARGS") || kw(&L->cur, "USING") ||
        kw(&L->cur, "ON"))
      lex_next(L);
    if (nname == 0) {
      char fb0[256];
      int or0 = call_parse_or_fallback(vm, L, fb0, sizeof fb0);
      if (or0 < 0) return -1;
      if (or0 > 0) {
        call_apply_or_last(vm, fb0);
        var_set_num(vm, "CALLED", 0);
        var_set_num(vm, "CALLANY_N", 0);
        var_set_num(vm, "TRYCALLANY_N", 0);
        var_set_num(vm, "CALLANY_OR", 1);
        var_set_str(vm, "CALLANY_FN", "");
        var_set_str(vm, "FN", "");
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST_ERR", "");
        var_set_str(vm, "ERR", "");
        bump(vm);
        return 1;
      }
      if (soft) {
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_num(vm, "CALLED", 0);
        var_set_num(vm, "CALLANY_N", 0);
        var_set_num(vm, "TRYCALLANY_N", 0);
        var_set_num(vm, "CALLANY_OR", 0);
        var_set_str(vm, "CALLANY_FN", "");
        var_set_str(vm, "FN", "");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "CALLANY: need FN names â CALLANY greeter hello WITH arg");
        var_set_str(vm, "ERR", "CALLANY: need FN names â CALLANY greeter hello WITH arg");
        bump(vm);
        return 1;
      }
      fail_at(vm, L, "CALLANY namesâ¦ [WITH args] [OR fb] â CALLANY greeter hello WITH x");
      return -1;
    }
    for (i = 0; i < nname; i++) {
      for (j = 0; j < vm->n_fns; j++) {
        if (strcmp(vm->fns[j].name, names[i]) == 0) {
          fn = &vm->fns[j];
          snprintf(fname, sizeof fname, "%s", names[i]);
          break;
        }
      }
      if (fn) break;
    }
    if (!fn) {
      char ebuf[220];
      size_t o = 0;
      ebuf[0] = 0;
      for (i = 0; i < nname && o + 50 < sizeof ebuf; i++) {
        int ln;
        if (i && o + 1 < sizeof ebuf) { ebuf[o++] = ','; ebuf[o] = 0; }
        ln = snprintf(ebuf + o, sizeof ebuf - o, "%s", names[i]);
        if (ln > 0) o += (size_t)ln;
      }
      {
        char msg[280];
        snprintf(msg, sizeof msg,
                 "CALLANY miss line %d: none of [%s] defined â DEFINE/INCLUDE Â· HASFNANY Â· LISTFNS",
                 aln, ebuf[0] ? ebuf : "?");
        /* drain args so soft/OR recovery can continue (stop at OR via oop_stmt_kw) */
        while (L->cur.kind == TK_NUM || L->cur.kind == TK_STR ||
               L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
               (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
          if (L->cur.kind == TK_STR) lex_next(L);
          else (void)parse_expr(vm, L);
        }
        {
          char fb[256];
          int or_rc = call_parse_or_fallback(vm, L, fb, sizeof fb);
          if (or_rc < 0) return -1;
          if (or_rc > 0) {
            call_apply_or_last(vm, fb);
            var_set_num(vm, "CALLED", 0);
            var_set_num(vm, "CALLANY_N", 0);
            var_set_num(vm, "TRYCALLANY_N", 0);
            var_set_num(vm, "CALLANY_OR", 1);
            var_set_str(vm, "CALLANY_FN", "");
            var_set_str(vm, "FN", "");
            var_set_num(vm, "OK", 1);
            var_set_str(vm, "LAST_ERR", "");
            var_set_str(vm, "ERR", "");
            if (vm->trace)
              fprintf(vm->trace, "# CALLANY OR fallback after miss [%s]\n",
                      ebuf[0] ? ebuf : "?");
            bump(vm);
            return 1;
          }
        }
        if (soft) {
          var_set_num(vm, "CALLED", 0);
          var_set_num(vm, "CALLANY_N", 0);
          var_set_num(vm, "TRYCALLANY_N", 0);
          var_set_num(vm, "CALLANY_OR", 0);
          var_set_num(vm, "LAST_N", 0);
          vm->last_n = 0;
          var_set_str(vm, "CALLANY_FN", "");
          var_set_str(vm, "FN", "");
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", msg);
          var_set_str(vm, "ERR", msg);
          bump(vm);
          return 1;
        }
        fail_at(vm, L, msg);
        return -1;
      }
    }
    {
      int got = oop_bind_args(vm, L, fn->params, fn->n_params);
      int ag = oop_arity_gate(vm, "CALLANY", fn->name, fn->params, fn->n_params, got, soft);
      if (ag < 0) return -1;
      if (ag > 0) {
        char fb[256];
        int or_rc = call_parse_or_fallback(vm, L, fb, sizeof fb);
        if (or_rc < 0) return -1;
        if (or_rc > 0) {
          call_apply_or_last(vm, fb);
          var_set_num(vm, "CALLED", 0);
          var_set_num(vm, "CALLANY_N", 0);
          var_set_num(vm, "TRYCALLANY_N", 0);
          var_set_num(vm, "CALLANY_OR", 1);
          var_set_str(vm, "FN", fn->name);
          var_set_str(vm, "CALLANY_FN", fn->name);
          var_set_num(vm, "OK", 1);
          var_set_str(vm, "LAST_ERR", "");
          var_set_str(vm, "ERR", "");
          bump(vm);
          return 1;
        }
        var_set_num(vm, "CALLED", 0);
        var_set_num(vm, "CALLANY_N", 0);
        var_set_num(vm, "TRYCALLANY_N", 0);
        var_set_num(vm, "CALLANY_OR", 0);
        var_set_str(vm, "FN", fn->name);
        var_set_str(vm, "CALLANY_FN", fn->name);
        bump(vm);
        return 1;
      }
    }
    var_set_num(vm, "CALLED", 1);
    var_set_num(vm, "CALLANY_N", 1);
    var_set_num(vm, "TRYCALLANY_N", 1);
    var_set_str(vm, "FN", fn->name);
    var_set_str(vm, "CALLANY_FN", fn->name);
    vm->return_fn = 0;
    {
      Lex fl;
      lex_init(&fl, fn->body, fn->len);
      if (exec_stmts_until(vm, &fl, "END", NULL) < 0) return -1;
    }
    vm->return_fn = 0;
    /* discard unused OR fallback after successful CALLANY */
    {
      char fb[256];
      int or_rc = call_parse_or_fallback(vm, L, fb, sizeof fb);
      if (or_rc < 0) return -1;
      var_set_num(vm, "CALLANY_OR", 0);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# CALLANY hit %s (of %d)\n", fn->name, nname);
    bump(vm);
    return 1;
  }

  /* RET [expr] â early return from FN (digit-4 control flow)
     RETIF cond [expr] / RETNZ / RETZ / RETUNLESS â conditional return (digit-1) */
  if (kw(&L->cur,"RET")||kw(&L->cur,"RETURN")||
      kw(&L->cur,"RETIF")||kw(&L->cur,"RETNZ")||kw(&L->cur,"RETZ")||
      kw(&L->cur,"RETUNLESS")||kw(&L->cur,"RETWHEN")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    int mode = 0; /* 0 always, 1 if nz, 2 if z */
    if (strcmp(op,"RETIF")==0 || strcmp(op,"RETNZ")==0 || strcmp(op,"RETWHEN")==0) mode = 1;
    else if (strcmp(op,"RETZ")==0 || strcmp(op,"RETUNLESS")==0) mode = 2;
    int do_ret = 1;
    if (mode){
      long cond = parse_expr(vm, L);
      if (mode == 1) do_ret = (cond != 0);
      else do_ret = (cond == 0);
    }
    /* optional return value when next looks like an expression start */
    int has_val = 0;
    long v = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        L->cur.kind==TK_STR ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"END") && !kw(&L->cur,"ELSE") &&
         !kw(&L->cur,"FN") && !kw(&L->cur,"CALL") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"ASSERT") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"RET") &&
         !kw(&L->cur,"RETURN") && !kw(&L->cur,"RETIF") && !kw(&L->cur,"RETZ") &&
         !kw(&L->cur,"RETNZ") && !kw(&L->cur,"RETUNLESS") && !kw(&L->cur,"RETWHEN") &&
         !kw(&L->cur,"WHEN") && !kw(&L->cur,"DEFAULT") &&
         !kw(&L->cur,"FOR") && !kw(&L->cur,"WHILE") && !kw(&L->cur,"LOOP") &&
         !kw(&L->cur,"IF") && !kw(&L->cur,"UNLESS") && !kw(&L->cur,"BREAK") &&
         !kw(&L->cur,"CONTINUE") && !kw(&L->cur,"CASE") && !kw(&L->cur,"CUBE") &&
         !kw(&L->cur,"SYS"))){
      v = parse_expr(vm, L);
      has_val = 1;
    }
    if (!do_ret){
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    if (has_val){
      var_set_num(vm, "LAST_N", v);
      vm->last_n = v;
      var_set_num(vm, "RETVAL", v);
    }
    vm->return_fn = 1;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }

  return 0;
}
