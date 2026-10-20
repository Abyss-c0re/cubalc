/* CubalC lang — lang_ops_flow_send.c
 * SEND · SENDSUPER · SENDANY · SENDALL · SENDWHERE* · SENDBETWEEN (A3 peel).
 * Mesh vote A3_carve_lang_ops_flow · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_flow_send.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_send(VM *vm, Lex *L) {
  /* SEND family plane. Returns 1 handled, 0 not this plane, -1 fail.
   */
  /* SENDSUPER|SUPERSEND|CALLSUPER obj method [args…]
   * Call METHOD on the EXTEND parent chain (skip instance-class overrides).
   * Multi-file link usability: child unit overrides hi; base unit keeps hi;
   * agent reaches parent via SENDSUPER without hollow dual-name glue.
   * Sets SUPER=1 · BASE/PARENTCLASS · SENDSUPER_N=1 · OK. */
  if (kw(&L->cur, "SENDSUPER") || kw(&L->cur, "SUPERSEND") ||
      kw(&L->cur, "CALLSUPER") || kw(&L->cur, "SUPERCALL") ||
      kw(&L->cur, "INVOKESUPER") || kw(&L->cur, "SUPERCALLMETHOD") ||
      kw(&L->cur, "PARENTCALL") || kw(&L->cur, "CALLPARENT")) {
    char oname[48], mname[48];
    ObjInst *ob;
    ClassDef *cd;
    ClassDef *pcd = NULL;
    MethodDef *md = NULL;
    int pi, guard;
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail_at(vm, L, "SENDSUPER needs object method [args] — SENDSUPER obj hi");
      return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      vv = var_get(vm, id, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else
        snprintf(mname, sizeof mname, "%s", id);
    } else {
      fail_at(vm, L, "SENDSUPER needs method — SENDSUPER obj hi");
      return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob || ob->class_idx < 0 || ob->class_idx >= vm->n_classes) {
      var_set_num(vm, "SENDSUPER_N", 0);
      var_set_num(vm, "SUPER", 0);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "SENDSUPER unknown object");
      var_set_str(vm, "ERR", "SENDSUPER unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    /* Prefer CLASS plate when nested inside method of a class on ISOF chain. */
    {
      Var *cv = var_get(vm, "CLASS", 0);
      if (cv && cv->is_str && cv->sval[0]) {
        int ci = oop_find_class_idx(vm, cv->sval);
        if (ci >= 0 && oop_isof_chain(vm, ob->class_idx, cv->sval))
          cd = &vm->classes[ci];
      }
    }
    pi = cd->parent_idx;
    guard = 0;
    while (pi >= 0 && pi < vm->n_classes && guard++ < CUBALC_MAX_CLASSES) {
      ClassDef *pw = &vm->classes[pi];
      MethodDef *found = oop_find_method(pw, mname);
      if (found) {
        /* Prefer method body that actually lives on parent plate:
         * With flatten inherit, child already has copy — look for method
         * defined on parent before child override by walking parents and
         * taking first match in parent chain (parent has own or inherited).
         * To skip override: use method from parent class def directly. */
        md = found;
        pcd = pw;
        break;
      }
      pi = pw->parent_idx;
    }
    if (!md || !pcd) {
      char ebuf[160];
      snprintf(ebuf, sizeof ebuf, "SENDSUPER no parent METHOD %s.%s",
               cd->name, mname);
      var_set_num(vm, "SENDSUPER_N", 0);
      var_set_num(vm, "SUPER", 0);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", ebuf);
      var_set_str(vm, "ERR", ebuf);
      bump(vm);
      return 1;
    }
    {
      int got = oop_bind_args(vm, L, md->params, md->n_params);
      if (got < 0) return -1;
      if (oop_run_method(vm, ob, md) < 0) return -1;
    }
    var_set_num(vm, "SENDSUPER_N", 1);
    var_set_num(vm, "SUPER", 1);
    var_set_num(vm, "SUPER_N", 1);
    var_set_num(vm, "HASPARENT_N", 1);
    var_set_str(vm, "BASE", pcd->name);
    var_set_str(vm, "PARENTCLASS", pcd->name);
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "ERR", "");
    if (vm->trace)
      fprintf(vm->trace, "# SENDSUPER %s.%s via %s ok\n", oname, mname, pcd->name);
    bump(vm);
    return 1;
  }

  /* SEND obj method [args] [OR|DEFAULT fallback]
   * TRYSEND|SEND SOFT â soft miss OK=0; â¦ OR fb â LAST=fb OK=1 SEND_OR=1 (CALL OR twin).
   * Method may be IDENT, "string", or string-var. Bare SEND still fatal without OR.
   * Usability: optional method hooks with default without HASMETHOD+IF glue. */
  if (kw(&L->cur, "SEND") || kw(&L->cur, "CALLMETHOD") || kw(&L->cur, "INVOKE") ||
      kw(&L->cur, "DOMETHOD") || kw(&L->cur, "MSG") || kw(&L->cur, "EMIT") ||
      kw(&L->cur, "TRYSEND") || kw(&L->cur, "SENDSOFT") ||
      kw(&L->cur, "SOFTSEND") || kw(&L->cur, "TRYCALLMETHOD") ||
      kw(&L->cur, "TRYINVOKE")) {
    char oname[48], mname[48], op[24], fb[256];
    ObjInst *ob;
    ClassDef *cd;
    MethodDef *md;
    int soft = 0;
    int or_rc;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "TRYSEND") == 0 || strcmp(op, "SENDSOFT") == 0 ||
        strcmp(op, "SOFTSEND") == 0 || strcmp(op, "TRYCALLMETHOD") == 0 ||
        strcmp(op, "TRYINVOKE") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail_at(vm, L, "SEND needs object method [args] â SEND obj bump"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      vv = var_get(vm, id, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else
        snprintf(mname, sizeof mname, "%s", id);
    } else {
      fail_at(vm, L, "SEND needs method name â SEND obj bump [args]"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      char sug[48], ebuf[160], base[96];
      oop_suggest_obj(vm, oname, sug, sizeof sug);
      snprintf(base, sizeof base, "SEND unknown object %s", oname);
      oop_err_suggest(ebuf, sizeof ebuf, base, sug);
      while (L->cur.kind == TK_NUM || L->cur.kind == TK_STR ||
             L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
        if (L->cur.kind == TK_STR) lex_next(L);
        else (void)parse_expr(vm, L);
      }
      or_rc = call_parse_or_fallback(vm, L, fb, sizeof fb);
      if (or_rc < 0) return -1;
      if (or_rc > 0) {
        call_apply_or_last(vm, fb);
        var_set_num(vm, "SEND_N", 0);
        var_set_num(vm, "TRYSEND_N", 0);
        var_set_num(vm, "SEND_OR", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST_ERR", "");
        var_set_str(vm, "ERR", "");
        bump(vm);
        return 1;
      }
      if (soft) {
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_num(vm, "SEND_N", 0);
        var_set_num(vm, "TRYSEND_N", 0);
        var_set_num(vm, "SEND_OR", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
        bump(vm);
        return 1;
      }
      fail(vm, ebuf); return -1;
    }
    cd = &vm->classes[ob->class_idx];
    md = oop_find_method(cd, mname);
    if (!md) {
      char sug[48], ebuf[160], base[96];
      oop_suggest_method(cd, mname, sug, sizeof sug);
      snprintf(base, sizeof base, "SEND unknown METHOD %s.%s", cd->name, mname);
      oop_err_suggest(ebuf, sizeof ebuf, base, sug);
      while (L->cur.kind == TK_NUM || L->cur.kind == TK_STR ||
             L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
        if (L->cur.kind == TK_STR) lex_next(L);
        else (void)parse_expr(vm, L);
      }
      or_rc = call_parse_or_fallback(vm, L, fb, sizeof fb);
      if (or_rc < 0) return -1;
      if (or_rc > 0) {
        call_apply_or_last(vm, fb);
        var_set_num(vm, "SEND_N", 0);
        var_set_num(vm, "TRYSEND_N", 0);
        var_set_num(vm, "SEND_OR", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "METHOD", mname);
        var_set_str(vm, "LAST_ERR", "");
        var_set_str(vm, "ERR", "");
        bump(vm);
        return 1;
      }
      if (soft) {
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_num(vm, "SEND_N", 0);
        var_set_num(vm, "TRYSEND_N", 0);
        var_set_num(vm, "SEND_OR", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
        bump(vm);
        return 1;
      }
      fail(vm, ebuf); return -1;
    }
    {
      int got = oop_bind_args(vm, L, md->params, md->n_params);
      char qname[96];
      int ag;
      snprintf(qname, sizeof qname, "%s.%s", cd->name, md->name);
      ag = oop_arity_gate(vm, "SEND", qname, md->params, md->n_params, got, soft);
      if (ag < 0) return -1;
      if (ag > 0) {
        or_rc = call_parse_or_fallback(vm, L, fb, sizeof fb);
        if (or_rc < 0) return -1;
        if (or_rc > 0) {
          call_apply_or_last(vm, fb);
          var_set_num(vm, "SEND_N", 0);
          var_set_num(vm, "TRYSEND_N", 0);
          var_set_num(vm, "SEND_OR", 1);
          var_set_num(vm, "OK", 1);
          var_set_str(vm, "LAST_ERR", "");
          var_set_str(vm, "ERR", "");
          bump(vm);
          return 1;
        }
        var_set_num(vm, "SEND_N", 0);
        var_set_num(vm, "TRYSEND_N", 0);
        var_set_num(vm, "SEND_OR", 0);
        bump(vm);
        return 1;
      }
    }
    if (oop_run_method(vm, ob, md) < 0) return -1;
    /* discard unused OR after successful SEND */
    or_rc = call_parse_or_fallback(vm, L, fb, sizeof fb);
    if (or_rc < 0) return -1;
    var_set_num(vm, "SEND_OR", 0);
    var_set_num(vm, "SEND_N", 1);
    var_set_num(vm, "TRYSEND_N", 1);
    var_set_str(vm, "METHOD", md->name);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SENDANY|TRYSENDANY obj methodsâ¦ [WITH argsâ¦] [OR|DEFAULT fb]
   * Soft TRYSENDANY: OK=0 if obj/method miss (no OR). Hard SENDANY: fail-fast.
   * â¦ OR fb â miss/soft arity â LAST=fb, OK=1, SENDANY_OR=1 (CALLANY OR twin).
   * Usability: multi-method hooks with default without HASMETHODANY+IF+SEND glue.
   * Twin of CALLANY OR for method plane. 0-arity when no WITH. */
  if (kw(&L->cur, "SENDANY") || kw(&L->cur, "SEND_ANY") ||
      kw(&L->cur, "SENDFIRST") || kw(&L->cur, "FIRSTMETHOD") ||
      kw(&L->cur, "INVOKEANY") || kw(&L->cur, "CALLMETHODANY") ||
      kw(&L->cur, "TRYSENDANY") || kw(&L->cur, "TRY_SEND_ANY") ||
      kw(&L->cur, "SOFTSENDANY") || kw(&L->cur, "SENDANYSOFT") ||
      kw(&L->cur, "NEEDSENDANY") || kw(&L->cur, "REQUIRESENDANY")) {
    int soft = kw(&L->cur, "TRYSENDANY") || kw(&L->cur, "TRY_SEND_ANY") ||
               kw(&L->cur, "SOFTSENDANY") || kw(&L->cur, "SENDANYSOFT");
    char oname[48];
    char names[32][48];
    char mname[48];
    int nname = 0, i, aln = L->cur.line;
    ObjInst *ob;
    ClassDef *cd = NULL;
    MethodDef *md = NULL;
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail_at(vm, L, "SENDANY needs object methodsâ¦ [WITH args] â SENDANY obj tick update");
      return -1;
    }
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
           kw(&L->cur, "SEND") || kw(&L->cur, "TRYSEND") || kw(&L->cur, "SENDANY") ||
           kw(&L->cur, "TRYSENDANY") || kw(&L->cur, "SENDALL") || kw(&L->cur, "CALL") ||
           kw(&L->cur, "HASMETHOD") || kw(&L->cur, "HASMETHODS") ||
           kw(&L->cur, "HASMETHODANY") || kw(&L->cur, "LISTMETHODS") ||
           kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") || kw(&L->cur, "GETF") ||
           kw(&L->cur, "SETF")))
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
        var_set_num(vm, "SEND_N", 0);
        var_set_num(vm, "SENDANY_N", 0);
        var_set_num(vm, "TRYSENDANY_N", 0);
        var_set_num(vm, "SENDANY_OR", 1);
        var_set_str(vm, "SENDANY_METHOD", "");
        var_set_str(vm, "METHOD", "");
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST_ERR", "");
        var_set_str(vm, "ERR", "");
        bump(vm);
        return 1;
      }
      if (soft) {
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_num(vm, "SEND_N", 0);
        var_set_num(vm, "SENDANY_N", 0);
        var_set_num(vm, "TRYSENDANY_N", 0);
        var_set_num(vm, "SENDANY_OR", 0);
        var_set_str(vm, "SENDANY_METHOD", "");
        var_set_str(vm, "METHOD", "");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "SENDANY: need methods â SENDANY obj tick update WITH arg");
        var_set_str(vm, "ERR", "SENDANY: need methods â SENDANY obj tick update WITH arg");
        bump(vm);
        return 1;
      }
      fail_at(vm, L, "SENDANY obj methodsâ¦ [WITH args] [OR fb] â SENDANY obj tick update");
      return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      char sug[48], ebuf[160], base[96];
      oop_suggest_obj(vm, oname, sug, sizeof sug);
      snprintf(base, sizeof base, "SENDANY unknown object %s", oname);
      oop_err_suggest(ebuf, sizeof ebuf, base, sug);
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
          var_set_num(vm, "SEND_N", 0);
          var_set_num(vm, "SENDANY_N", 0);
          var_set_num(vm, "TRYSENDANY_N", 0);
          var_set_num(vm, "SENDANY_OR", 1);
          var_set_str(vm, "SENDANY_METHOD", "");
          var_set_str(vm, "METHOD", "");
          var_set_num(vm, "OK", 1);
          var_set_str(vm, "LAST_ERR", "");
          var_set_str(vm, "ERR", "");
          if (vm->trace)
            fprintf(vm->trace, "# SENDANY OR fallback after obj miss %s\n", oname);
          bump(vm);
          return 1;
        }
      }
      if (soft) {
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_num(vm, "SEND_N", 0);
        var_set_num(vm, "SENDANY_N", 0);
        var_set_num(vm, "TRYSENDANY_N", 0);
        var_set_num(vm, "SENDANY_OR", 0);
        var_set_str(vm, "SENDANY_METHOD", "");
        var_set_str(vm, "METHOD", "");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
        bump(vm);
        return 1;
      }
      fail(vm, ebuf);
      return -1;
    }
    cd = &vm->classes[ob->class_idx];
    for (i = 0; i < nname; i++) {
      md = oop_find_method(cd, names[i]);
      if (md) {
        snprintf(mname, sizeof mname, "%s", names[i]);
        break;
      }
    }
    if (!md) {
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
        char msg[300];
        snprintf(msg, sizeof msg,
                 "SENDANY miss line %d: %s none of [%s] â CLASS METHOD Â· HASMETHODANY Â· LISTMETHODS",
                 aln, cd->name, ebuf[0] ? ebuf : "?");
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
            var_set_num(vm, "SEND_N", 0);
            var_set_num(vm, "SENDANY_N", 0);
            var_set_num(vm, "TRYSENDANY_N", 0);
            var_set_num(vm, "SENDANY_OR", 1);
            var_set_str(vm, "SENDANY_METHOD", "");
            var_set_str(vm, "METHOD", "");
            var_set_num(vm, "OK", 1);
            var_set_str(vm, "LAST_ERR", "");
            var_set_str(vm, "ERR", "");
            if (vm->trace)
              fprintf(vm->trace, "# SENDANY OR fallback after method miss %s\n",
                      ebuf[0] ? ebuf : "?");
            bump(vm);
            return 1;
          }
        }
        if (soft) {
          var_set_num(vm, "LAST_N", 0);
          vm->last_n = 0;
          var_set_num(vm, "SEND_N", 0);
          var_set_num(vm, "SENDANY_N", 0);
          var_set_num(vm, "TRYSENDANY_N", 0);
          var_set_num(vm, "SENDANY_OR", 0);
          var_set_str(vm, "SENDANY_METHOD", "");
          var_set_str(vm, "METHOD", "");
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
      int got = oop_bind_args(vm, L, md->params, md->n_params);
      char qname[96];
      int ag;
      snprintf(qname, sizeof qname, "%s.%s", cd->name, md->name);
      ag = oop_arity_gate(vm, "SENDANY", qname, md->params, md->n_params, got, soft);
      if (ag < 0) return -1;
      if (ag > 0) {
        char fb[256];
        int or_rc = call_parse_or_fallback(vm, L, fb, sizeof fb);
        if (or_rc < 0) return -1;
        if (or_rc > 0) {
          call_apply_or_last(vm, fb);
          var_set_num(vm, "SEND_N", 0);
          var_set_num(vm, "SENDANY_N", 0);
          var_set_num(vm, "TRYSENDANY_N", 0);
          var_set_num(vm, "SENDANY_OR", 1);
          var_set_str(vm, "SENDANY_METHOD", md->name);
          var_set_str(vm, "METHOD", md->name);
          var_set_num(vm, "OK", 1);
          var_set_str(vm, "LAST_ERR", "");
          var_set_str(vm, "ERR", "");
          bump(vm);
          return 1;
        }
        var_set_num(vm, "SEND_N", 0);
        var_set_num(vm, "SENDANY_N", 0);
        var_set_num(vm, "TRYSENDANY_N", 0);
        var_set_num(vm, "SENDANY_OR", 0);
        var_set_str(vm, "SENDANY_METHOD", md->name);
        var_set_str(vm, "METHOD", md->name);
        bump(vm);
        return 1;
      }
    }
    if (oop_run_method(vm, ob, md) < 0) return -1;
    var_set_num(vm, "SEND_N", 1);
    var_set_num(vm, "SENDANY_N", 1);
    var_set_num(vm, "TRYSENDANY_N", 1);
    var_set_str(vm, "SENDANY_METHOD", md->name);
    var_set_str(vm, "METHOD", md->name);
    var_set_str(vm, "OBJ", oname);
    /* discard unused OR after successful SENDANY */
    {
      char fb[256];
      int or_rc = call_parse_or_fallback(vm, L, fb, sizeof fb);
      if (or_rc < 0) return -1;
      var_set_num(vm, "SENDANY_OR", 0);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# SENDANY %s.%s (of %d)\n", oname, md->name, nname);
    bump(vm);
    return 1;
  }

  /* SENDALL|BROADCAST [Class] method [args]
   * â invoke method on every live object (optional class filter).
   * Skips objects missing the method (soft). LAST_N/SENDALL_N = call count.
   * Usability: fleet tick without EACH OBJ + SEND glue (beyond TICK). */
  if (kw(&L->cur, "SENDALL") || kw(&L->cur, "BROADCAST") ||
      kw(&L->cur, "INVOKEALL") || kw(&L->cur, "CALLALL") ||
      kw(&L->cur, "MAPSEND") || kw(&L->cur, "FORALLSEND")) {
    char filt[48], mname[48], tok1[48];
    int has_filt = 0, i, n = 0, n_skip = 0, advanced = 0;
    Lex args_start;
    lex_next(L);
    filt[0] = 0;
    mname[0] = 0;
    tok1[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "SENDALL [Class] method [args]"); return -1;
    }
    snprintf(tok1, sizeof tok1, "%s", L->cur.text);
    lex_next(L);
    /* Class method when tok1 names a known CLASS and next is method */
    if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
        oop_find_class(vm, tok1)) {
      snprintf(filt, sizeof filt, "%s", tok1);
      has_filt = 1;
      if (L->cur.kind == TK_STR) {
        snprintf(mname, sizeof mname, "%s", L->cur.text);
        lex_next(L);
      } else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(mname, sizeof mname, "%s", vv->sval);
        else
          snprintf(mname, sizeof mname, "%s", L->cur.text);
        lex_next(L);
      }
    } else {
      Var *vv = var_get(vm, tok1, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else
        snprintf(mname, sizeof mname, "%s", tok1);
    }
    args_start = *L;
    for (i = 0; i < vm->n_objs && !vm->fatal && !vm->halt; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      MethodDef *md;
      Lex bl;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      md = oop_find_method(cd, mname);
      if (!md) { n_skip++; continue; }
      bl = args_start;
      oop_bind_args(vm, &bl, md->params, md->n_params);
      *L = bl;
      advanced = 1;
      if (oop_run_method(vm, ob, md) < 0) return -1;
      n++;
    }
    if (!advanced) {
      Lex bl = args_start;
      char dummy[8][32];
      memset(dummy, 0, sizeof dummy);
      oop_bind_args(vm, &bl, dummy, 0);
      *L = bl;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "SENDALL_N", n);
    var_set_num(vm, "BROADCAST_N", n);
    var_set_num(vm, "SENDALL_SKIP", n_skip);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SENDWHERE|INVOKEWHERE [Class] field value method [args]
   * â call method on every live object whose field equals value.
   * Soft always; missing method â SENDWHERE_SKIP. LAST_N = call count.
   * Usability: WHEREOBJ + SENDALL one-shot Â· no EACH+GETF+IF+SEND glue. */
  if (kw(&L->cur, "SENDWHERE") || kw(&L->cur, "INVOKEWHERE") ||
      kw(&L->cur, "CALLWHERE") || kw(&L->cur, "BROADCASTWHERE") ||
      kw(&L->cur, "MAPSENDWHERE") || kw(&L->cur, "FORWHERE")) {
    char filt[48], fname[48], tok1[48], mname[48], sval[512];
    int has_filt = 0, is_str = 0, i, n = 0, n_skip = 0, advanced = 0;
    long nval = 0;
    Lex args_start;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    mname[0] = 0;
    sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SENDWHERE OF Class field value method"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "SENDWHERE [Class] field value method"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SENDWHERE [Class] field value method"); return -1;
      }
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(sval, sizeof sval, "%s", L->cur.text);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(sval, sizeof sval, "%s", vm->last_str);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      /* method name may follow immediately if value is bare ident â only
       * treat as string-var when next token can still be method. Prefer num
       * var via parse_expr if not pure string. */
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(sval, sizeof sval, "%s", sv->sval);
        is_str = 1;
        lex_next(L);
      } else {
        nval = parse_expr(vm, L);
        is_str = 0;
      }
    } else {
      nval = parse_expr(vm, L);
      is_str = 0;
    }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "SENDWHERE [Class] field value method"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else
        snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    }
    args_start = *L;
    for (i = 0; i < vm->n_objs && !vm->fatal && !vm->halt; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      MethodDef *md;
      Lex bl;
      int fi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) {
        if (is_str)
          hit = (strcmp(ob->fstr[fi], sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", nval);
          hit = (strcmp(ob->fstr[fi], nb) == 0);
        }
      } else {
        if (is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[fi]);
          hit = (strcmp(nb, sval) == 0);
        } else {
          hit = (ob->fnum[fi] == nval);
        }
      }
      if (!hit) continue;
      md = oop_find_method(cd, mname);
      if (!md) { n_skip++; continue; }
      bl = args_start;
      oop_bind_args(vm, &bl, md->params, md->n_params);
      *L = bl;
      advanced = 1;
      if (oop_run_method(vm, ob, md) < 0) return -1;
      n++;
    }
    if (!advanced) {
      Lex bl = args_start;
      char dummy[8][32];
      memset(dummy, 0, sizeof dummy);
      oop_bind_args(vm, &bl, dummy, 0);
      *L = bl;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "SENDWHERE_N", n);
    var_set_num(vm, "INVOKEWHERE_N", n);
    var_set_num(vm, "SENDWHERE_SKIP", n_skip);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "METHOD", mname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SENDWHEREGE|SENDWHERELE|SENDWHEREGT|SENDWHERELT [Class] field value method [args]
   * INVOKEWHEREGE|CALLWHEREGE|SENDATLEAST|SENDABOVE|SENDBELOW aliases.
   * â call method on every live object whose numeric field meets threshold.
   * Soft always; missing method / string fields â SENDWHEREGE_SKIP.
   * Usability: WHEREGE + SENDALL one-shot Â· tick high-energy cells / drain low
   * counters without EACH+GETF+IF+SEND. Completes threshold triad with
   * WHEREGE + DELETEWHEREGE. */
  if (kw(&L->cur, "SENDWHEREGE") || kw(&L->cur, "SENDWHEREGTE") ||
      kw(&L->cur, "INVOKEWHEREGE") || kw(&L->cur, "CALLWHEREGE") ||
      kw(&L->cur, "SENDATLEAST") || kw(&L->cur, "MAPSENDWHEREGE") ||
      kw(&L->cur, "SENDWHEREGT") || kw(&L->cur, "INVOKEWHEREGT") ||
      kw(&L->cur, "CALLWHEREGT") || kw(&L->cur, "SENDABOVE") ||
      kw(&L->cur, "INVOKEABOVE") ||
      kw(&L->cur, "SENDWHERELE") || kw(&L->cur, "SENDWHERELTE") ||
      kw(&L->cur, "INVOKEWHERELE") || kw(&L->cur, "CALLWHERELE") ||
      kw(&L->cur, "SENDATMOST") ||
      kw(&L->cur, "SENDWHERELT") || kw(&L->cur, "INVOKEWHERELT") ||
      kw(&L->cur, "CALLWHERELT") || kw(&L->cur, "SENDBELOW") ||
      kw(&L->cur, "INVOKEBELOW") ||
      kw(&L->cur, "THRESHSEND") || kw(&L->cur, "SENDTHRESH")) {
    char filt[48], fname[48], tok1[48], mname[48], op[32];
    int has_filt = 0, mode = 0, i, n = 0, n_skip = 0, advanced = 0;
    long thresh = 0;
    Lex args_start;
    /* mode: 0=GE 1=GT 2=LE 3=LT */
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "SENDWHEREGT") == 0 || strcmp(op, "INVOKEWHEREGT") == 0 ||
        strcmp(op, "CALLWHEREGT") == 0 || strcmp(op, "SENDABOVE") == 0 ||
        strcmp(op, "INVOKEABOVE") == 0)
      mode = 1;
    else if (strcmp(op, "SENDWHERELE") == 0 || strcmp(op, "SENDWHERELTE") == 0 ||
             strcmp(op, "INVOKEWHERELE") == 0 || strcmp(op, "CALLWHERELE") == 0 ||
             strcmp(op, "SENDATMOST") == 0)
      mode = 2;
    else if (strcmp(op, "SENDWHERELT") == 0 || strcmp(op, "INVOKEWHERELT") == 0 ||
             strcmp(op, "CALLWHERELT") == 0 || strcmp(op, "SENDBELOW") == 0 ||
             strcmp(op, "INVOKEBELOW") == 0)
      mode = 3;
    else
      mode = 0; /* GE / ATLEAST / THRESHSEND */
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    mname[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SENDWHEREGE OF Class field value method"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "SENDWHEREGE [Class] field value method"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SENDWHEREGE [Class] field value method"); return -1;
      }
    }
    if (kw(&L->cur, "GE") || kw(&L->cur, "GTE") || kw(&L->cur, "GT") ||
        kw(&L->cur, "LE") || kw(&L->cur, "LTE") || kw(&L->cur, "LT") ||
        kw(&L->cur, "MIN") || kw(&L->cur, "MAX"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      thresh = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      thresh = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        thresh = atol(sv->sval);
      else
        thresh = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      thresh = parse_expr(vm, L);
    }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "SENDWHEREGE [Class] field value method"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else
        snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    }
    args_start = *L;
    for (i = 0; i < vm->n_objs && !vm->fatal && !vm->halt; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      MethodDef *md;
      Lex bl;
      int fi, hit = 0;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (mode == 0) hit = (v >= thresh);
      else if (mode == 1) hit = (v > thresh);
      else if (mode == 2) hit = (v <= thresh);
      else hit = (v < thresh);
      if (!hit) continue;
      md = oop_find_method(cd, mname);
      if (!md) { n_skip++; continue; }
      bl = args_start;
      oop_bind_args(vm, &bl, md->params, md->n_params);
      *L = bl;
      advanced = 1;
      if (oop_run_method(vm, ob, md) < 0) return -1;
      n++;
    }
    if (!advanced) {
      Lex bl = args_start;
      char dummy[8][32];
      memset(dummy, 0, sizeof dummy);
      oop_bind_args(vm, &bl, dummy, 0);
      *L = bl;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "SENDWHEREGE_N", n);
    var_set_num(vm, "SENDWHERELE_N", n);
    var_set_num(vm, "SENDWHEREGT_N", n);
    var_set_num(vm, "SENDWHERELT_N", n);
    var_set_num(vm, "INVOKEWHEREGE_N", n);
    var_set_num(vm, "SENDATLEAST_N", n);
    var_set_num(vm, "SENDATMOST_N", n);
    var_set_num(vm, "SENDABOVE_N", n);
    var_set_num(vm, "SENDBELOW_N", n);
    var_set_num(vm, "SENDWHEREGE_SKIP", n_skip);
    var_set_num(vm, "SENDWHEREGE_THRESH", thresh);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "METHOD", mname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SENDBETWEEN|INVOKEBETWEEN|CALLBETWEEN|SENDBAND [Class] field lo [TO] hi method [args]
   * â call method on every live obj with lo <= numeric field <= hi.
   * Soft always; missing method / string fields â SENDBETWEEN_SKIP.
   * Completes BETWEEN triad: WHEREBETWEEN Â· DELETEBETWEEN Â· SENDBETWEEN.
   * Usability: band tick/drain without dual SENDWHEREGE+SENDWHERELE. */
  if (kw(&L->cur, "SENDBETWEEN") || kw(&L->cur, "INVOKEBETWEEN") ||
      kw(&L->cur, "CALLBETWEEN") || kw(&L->cur, "SENDBAND") ||
      kw(&L->cur, "MAPSENDBETWEEN") || kw(&L->cur, "FORBETWEEN") ||
      kw(&L->cur, "BROADCASTBETWEEN") || kw(&L->cur, "SENDINRANGE")) {
    char filt[48], fname[48], tok1[48], mname[48];
    int has_filt = 0, i, n = 0, n_skip = 0, advanced = 0;
    long lo = 0, hi = 0, t;
    Lex args_start;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    mname[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SENDBETWEEN OF Class field lo hi method"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "SENDBETWEEN [Class] field lo hi method"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SENDBETWEEN [Class] field lo hi method"); return -1;
      }
    }
    if (kw(&L->cur, "IN") || kw(&L->cur, "BETWEEN") || kw(&L->cur, "RANGE"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      lo = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      lo = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        lo = atol(sv->sval);
      else
        lo = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      lo = parse_expr(vm, L);
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "AND") || kw(&L->cur, "THRU") ||
        kw(&L->cur, "THROUGH") || kw(&L->cur, "UNTIL") ||
        (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "..") == 0))
      lex_next(L);
    else if (L->cur.kind == TK_COMMA)
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      hi = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      hi = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        hi = atol(sv->sval);
      else
        hi = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      hi = parse_expr(vm, L);
    }
    if (lo > hi) { t = lo; lo = hi; hi = t; }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "SENDBETWEEN [Class] field lo hi method"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else
        snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    }
    args_start = *L;
    for (i = 0; i < vm->n_objs && !vm->fatal && !vm->halt; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      MethodDef *md;
      Lex bl;
      int fi;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (v < lo || v > hi) continue;
      md = oop_find_method(cd, mname);
      if (!md) { n_skip++; continue; }
      bl = args_start;
      oop_bind_args(vm, &bl, md->params, md->n_params);
      *L = bl;
      advanced = 1;
      if (oop_run_method(vm, ob, md) < 0) return -1;
      n++;
    }
    if (!advanced) {
      Lex bl = args_start;
      char dummy[8][32];
      memset(dummy, 0, sizeof dummy);
      oop_bind_args(vm, &bl, dummy, 0);
      *L = bl;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "SENDBETWEEN_N", n);
    var_set_num(vm, "INVOKEBETWEEN_N", n);
    var_set_num(vm, "CALLBETWEEN_N", n);
    var_set_num(vm, "SENDBAND_N", n);
    var_set_num(vm, "SENDINRANGE_N", n);
    var_set_num(vm, "SENDBETWEEN_SKIP", n_skip);
    var_set_num(vm, "SENDBETWEEN_LO", lo);
    var_set_num(vm, "SENDBETWEEN_HI", hi);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "METHOD", mname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  return 0;
}
