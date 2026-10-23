/* CubalC lang — lang_ops_flow_fstr_hasmeth.c
 * FLOW fstr method-existence: HASMETHOD · HASMETHODS · HASMETHODANY
 * Peel from fstr_b (B_fstr_b_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_hasmeth.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_hasmeth(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* HASMETHOD obj|Class method â LAST_N 1|0 soft probe before SEND.
   * First arg object (live) or ClassName. String-vars expand (METHOD_ON / name bags).
   * Usability: agent IF without fatal Â· twin of SEND method-name resolve. */
  if (kw(&L->cur, "HASMETHOD") || kw(&L->cur, "HASMETH") ||
      kw(&L->cur, "CANCALL") || kw(&L->cur, "RESPONDS") ||
      kw(&L->cur, "RESPONDSTO") || kw(&L->cur, "METHOD?")) {
    char a[48], mname[48];
    ClassDef *cd = NULL;
    ObjInst *ob;
    MethodDef *md;
    int hit = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "HASMETHOD obj|Class method"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      /* prefer live obj / class literal; else expand string-var (METHOD_ON) */
      if (oop_find_obj(vm, id) || oop_find_class(vm, id)) {
        snprintf(a, sizeof a, "%s", id);
      } else {
        vv = var_get(vm, id, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(a, sizeof a, "%s", vv->sval);
        else
          snprintf(a, sizeof a, "%s", id);
      }
    }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "HASMETHOD method"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      vv = var_get(vm, id, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else
        snprintf(mname, sizeof mname, "%s", id);
    }
    ob = oop_find_obj(vm, a);
    if (ob)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    if (cd) {
      md = oop_find_method(cd, mname);
      if (md) hit = 1;
    }
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASMETHOD_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASMETHODS|NEEDMETHODS Class|obj methodâ¦ | Class.method bag â
   * multi method capability gate (all-or-METHODMISS).
   * Soft: LAST_N 0|1 Â· METHODMISS/METHMISS bag Â· NEEDMETHODS fail-fast first miss.
   * Forms: HASMETHODS Cell tick init Â· HASMETHODS bag_of_Class.method (multi-class).
   * Usability: agent SEND / method_guard contracts without NÃ HASMETHOD soup. */
  if (kw(&L->cur, "HASMETHODS") || kw(&L->cur, "HAS_METHODS") ||
      kw(&L->cur, "ALLMETHODS") || kw(&L->cur, "METHODSOK") ||
      kw(&L->cur, "NEEDMETHODS") || kw(&L->cur, "NEED_METHODS") ||
      kw(&L->cur, "REQUIREMETHODS") || kw(&L->cur, "MUSTMETHODS")) {
    int hard = kw(&L->cur, "NEEDMETHODS") || kw(&L->cur, "NEED_METHODS") ||
               kw(&L->cur, "REQUIREMETHODS") || kw(&L->cur, "MUSTMETHODS");
    char first[1024];
    char target[48];
    char names[32][48];
    char miss[1024];
    ClassDef *cd = NULL;
    ObjInst *ob;
    int nname = 0, nmiss = 0, i, aln = L->cur.line, dotted = 0;
    size_t mo = 0;
    lex_next(L);
    miss[0] = 0;
    first[0] = 0;
    target[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      if (hard) {
        fail_at(vm, L, "NEEDMETHODS Class|obj methodâ¦ â NEEDMETHODS Cell tick Â· bag Class.method");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASMETHODS_N", 0);
      var_set_num(vm, "METHODMISS_N", 0);
      var_set_num(vm, "METHMISS_N", 0);
      var_set_str(vm, "METHODMISS", "");
      var_set_str(vm, "METHMISS", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASMETHODS: need Class|obj methodâ¦ or Class.method bag");
      var_set_str(vm, "ERR", "HASMETHODS: need Class|obj methodâ¦ or Class.method bag");
      bump(vm);
      return 1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(first, sizeof first, "%s", L->cur.text);
      lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      if (oop_find_obj(vm, id) || oop_find_class(vm, id)) {
        snprintf(first, sizeof first, "%s", id);
      } else {
        vv = var_get(vm, id, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(first, sizeof first, "%s", vv->sval);
        else
          snprintf(first, sizeof first, "%s", id);
      }
    }
    /* Class.method bag mode when first has dots and is bag/CSV or lone Class.method */
    if (strchr(first, '.')) {
      int more = (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT);
      if (more && L->cur.kind == TK_IDENT &&
          (kw(&L->cur, "ASSERT") || kw(&L->cur, "LET") || kw(&L->cur, "PRINT") ||
           kw(&L->cur, "REQUIRE") || kw(&L->cur, "END") ||
           kw(&L->cur, "IF") || kw(&L->cur, "ELSE") || kw(&L->cur, "ELIF") ||
           kw(&L->cur, "PASS") || kw(&L->cur, "FAIL") || kw(&L->cur, "INCLUDE") ||
           kw(&L->cur, "SYS") || kw(&L->cur, "HELP") || kw(&L->cur, "CLEAR_ERR") ||
           kw(&L->cur, "NOTE") || kw(&L->cur, "EXIT") || kw(&L->cur, "DEFAULT") ||
           kw(&L->cur, "VERSION") || kw(&L->cur, "STATUS") || kw(&L->cur, "FOR") ||
           kw(&L->cur, "WHILE") || kw(&L->cur, "LOOP") || kw(&L->cur, "EACH") ||
           kw(&L->cur, "CUBE") || kw(&L->cur, "PLUG") || kw(&L->cur, "HOLD_FLASH") ||
           kw(&L->cur, "HASCLASS") || kw(&L->cur, "HASCLASSES") ||
           kw(&L->cur, "HASMETHOD") || kw(&L->cur, "HASMETHODS") ||
           kw(&L->cur, "HASMETHODANY") || kw(&L->cur, "NEEDMETHODS") ||
           kw(&L->cur, "NEEDMETHODANY") || kw(&L->cur, "NEEDCLASSES") ||
           kw(&L->cur, "LISTMETHODS") || kw(&L->cur, "LISTCLASSES") ||
           kw(&L->cur, "LISTOBJS") || kw(&L->cur, "HASFORM") ||
           kw(&L->cur, "HASOBJ")))
        more = 0;
      if (!more || strchr(first, '\n') || strchr(first, ',') || strchr(first, ' '))
        dotted = 1;
    }
    if (dotted) {
      const char *p = first;
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
      if (nname == 0) {
        if (hard) {
          fail_at(vm, L, "NEEDMETHODS Class.methodâ¦ â NEEDMETHODS Ticket.ping Order.pay");
          return -1;
        }
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "0");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "HASMETHODS_N", 0);
        var_set_num(vm, "METHODMISS_N", 0);
        var_set_num(vm, "METHMISS_N", 0);
        var_set_str(vm, "METHODMISS", "");
        var_set_str(vm, "METHMISS", "");
        var_set_str(vm, "METHOD_ON", "*");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "HASMETHODS: need Class.method bag");
        var_set_str(vm, "ERR", "HASMETHODS: need Class.method bag");
        bump(vm);
        return 1;
      }
      for (i = 0; i < nname; i++) {
        char cname[48], mname[48];
        const char *dot = strchr(names[i], '.');
        int hit = 0;
        cname[0] = 0;
        mname[0] = 0;
        if (dot && dot != names[i] && dot[1]) {
          size_t cl = (size_t)(dot - names[i]);
          if (cl + 1 < sizeof cname) {
            memcpy(cname, names[i], cl);
            cname[cl] = 0;
          }
          snprintf(mname, sizeof mname, "%s", dot + 1);
          ob = oop_find_obj(vm, cname);
          if (ob)
            cd = &vm->classes[ob->class_idx];
          else
            cd = oop_find_class(vm, cname);
          if (cd && oop_find_method(cd, mname))
            hit = 1;
        }
        if (hit)
          continue;
        nmiss++;
        if (mo && mo + 1 < sizeof miss) miss[mo++] = '\n';
        {
          size_t ln = strlen(names[i]);
          if (mo + ln < sizeof miss) {
            memcpy(miss + mo, names[i], ln);
            mo += ln;
            miss[mo] = 0;
          }
        }
        if (hard) {
          char em[220];
          snprintf(em, sizeof em,
                   "NEEDMETHODS miss line %d: '%s' (+%d more?) â CLASS METHOD Â· HASMETHOD Â· LISTMETHODS",
                   aln, names[i], nname - i - 1);
          fail_at(vm, L, em);
          return -1;
        }
      }
      var_set_str(vm, "METHODMISS", miss);
      var_set_str(vm, "METHMISS", miss);
      var_set_num(vm, "METHODMISS_N", nmiss);
      var_set_num(vm, "METHMISS_N", nmiss);
      var_set_num(vm, "HASMETHODS_N", nname - nmiss);
      var_set_num(vm, "NEEDMETHODS_N", nname);
      var_set_num(vm, "METHODS_N", nname);
      var_set_str(vm, "METHOD_ON", "*");
      if (nmiss == 0) {
        var_set_num(vm, "LAST_N", 1);
        vm->last_n = 1;
        var_set_str(vm, "LAST", "1");
        snprintf(vm->last_str, sizeof vm->last_str, "1");
        var_set_num(vm, "OK", 1);
      } else {
        char em[220];
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "0");
        snprintf(em, sizeof em,
                 "HASMETHODS miss (%d/%d): %s â CLASS METHOD Â· NEEDMETHODS Â· LISTMETHODS",
                 nmiss, nname, miss);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", em);
        var_set_str(vm, "ERR", em);
      }
      if (vm->trace)
        fprintf(vm->trace, "# %s dotted n=%d miss=%d\n",
                hard ? "NEEDMETHODS" : "HASMETHODS", nname, nmiss);
      bump(vm);
      return 1;
    }
    /* classic Class|obj methodâ¦ form */
    snprintf(target, sizeof target, "%s", first);
    while (nname < 32 &&
           (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur, "ASSERT") || kw(&L->cur, "LET") || kw(&L->cur, "PRINT") ||
           kw(&L->cur, "REQUIRE") || kw(&L->cur, "END") ||
           kw(&L->cur, "IF") || kw(&L->cur, "ELSE") || kw(&L->cur, "ELIF") ||
           kw(&L->cur, "PASS") || kw(&L->cur, "FAIL") || kw(&L->cur, "INCLUDE") ||
           kw(&L->cur, "SYS") || kw(&L->cur, "HELP") || kw(&L->cur, "CLEAR_ERR") ||
           kw(&L->cur, "NOTE") || kw(&L->cur, "EXIT") || kw(&L->cur, "DEFAULT") ||
           kw(&L->cur, "VERSION") || kw(&L->cur, "STATUS") || kw(&L->cur, "FOR") ||
           kw(&L->cur, "WHILE") || kw(&L->cur, "LOOP") || kw(&L->cur, "EACH") ||
           kw(&L->cur, "CUBE") || kw(&L->cur, "PLUG") || kw(&L->cur, "HOLD_FLASH") ||
           kw(&L->cur, "HASCLASS") || kw(&L->cur, "HASCLASSES") ||
           kw(&L->cur, "HASMETHOD") || kw(&L->cur, "HASMETHODS") ||
           kw(&L->cur, "HASMETHODANY") || kw(&L->cur, "NEEDMETHODS") ||
           kw(&L->cur, "NEEDMETHODANY") || kw(&L->cur, "NEEDCLASSES") ||
           kw(&L->cur, "LISTMETHODS") || kw(&L->cur, "LISTCLASSES") ||
           kw(&L->cur, "LISTOBJS") || kw(&L->cur, "HASFORM") ||
           kw(&L->cur, "HASOBJ") || kw(&L->cur, "ASSERT")))
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
                   strchr(vv->sval, '.') == NULL &&
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
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDMETHODS Class|obj methodâ¦ â NEEDMETHODS Cell tick init");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASMETHODS_N", 0);
      var_set_num(vm, "METHODMISS_N", 0);
      var_set_num(vm, "METHMISS_N", 0);
      var_set_str(vm, "METHODMISS", "");
      var_set_str(vm, "METHMISS", "");
      var_set_str(vm, "METHOD_ON", target);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASMETHODS: need methods â HASMETHODS Cell tick init");
      var_set_str(vm, "ERR", "HASMETHODS: need methods â HASMETHODS Cell tick init");
      bump(vm);
      return 1;
    }
    ob = oop_find_obj(vm, target);
    if (ob)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, target);
    for (i = 0; i < nname; i++) {
      int hit = 0;
      if (cd && oop_find_method(cd, names[i]))
        hit = 1;
      if (hit)
        continue;
      nmiss++;
      if (mo && mo + 1 < sizeof miss) miss[mo++] = '\n';
      {
        size_t ln = strlen(names[i]);
        if (mo + ln < sizeof miss) {
          memcpy(miss + mo, names[i], ln);
          mo += ln;
          miss[mo] = 0;
        }
      }
      if (hard) {
        char em[220];
        snprintf(em, sizeof em,
                 "NEEDMETHODS miss line %d: '%s.%s' (+%d more?) â CLASS METHOD Â· HASMETHOD Â· LISTMETHODS",
                 aln, target[0] ? target : "?", names[i], nname - i - 1);
        fail_at(vm, L, em);
        return -1;
      }
    }
    var_set_str(vm, "METHODMISS", miss);
    var_set_str(vm, "METHMISS", miss);
    var_set_num(vm, "METHODMISS_N", nmiss);
    var_set_num(vm, "METHMISS_N", nmiss);
    var_set_num(vm, "HASMETHODS_N", nname - nmiss);
    var_set_num(vm, "NEEDMETHODS_N", nname);
    var_set_num(vm, "METHODS_N", nname);
    var_set_str(vm, "METHOD_ON", target);
    if (nmiss == 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else {
      char em[220];
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      snprintf(em, sizeof em,
               "HASMETHODS miss (%d/%d) on %s: %s â CLASS METHOD Â· NEEDMETHODS Â· LISTMETHODS",
               nmiss, nname, target[0] ? target : "?", miss);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s %s n=%d miss=%d\n",
              hard ? "NEEDMETHODS" : "HASMETHODS", target, nname, nmiss);
    bump(vm);
    return 1;
  }

  /* HASMETHODANY|NEEDMETHODANY Class|obj methodâ¦ | Class.method bag â
   * multi method any-of gate (HASMETHODS twin).
   * Soft: LAST_N 1 if any match Â· METHODHAVE/METHHAVE Â· METHODMISS/METHMISS.
   * Forms: HASMETHODANY Cell tick die Â· HASMETHODANY bag_of_Class.method.
   * NEEDMETHODANY fail-fast if none match. */
  if (kw(&L->cur, "HASMETHODANY") || kw(&L->cur, "HAS_METHOD_ANY") ||
      kw(&L->cur, "ANYMETHOD") || kw(&L->cur, "METHODANY") ||
      kw(&L->cur, "HASANYMETHOD") || kw(&L->cur, "NEEDMETHODANY") ||
      kw(&L->cur, "NEED_METHOD_ANY") || kw(&L->cur, "REQUIREMETHODANY") ||
      kw(&L->cur, "MUSTMETHODANY")) {
    int hard = kw(&L->cur, "NEEDMETHODANY") || kw(&L->cur, "NEED_METHOD_ANY") ||
               kw(&L->cur, "REQUIREMETHODANY") || kw(&L->cur, "MUSTMETHODANY");
    char first[1024];
    char target[48];
    char names[32][48];
    char miss[1024], havebag[1024];
    ClassDef *cd = NULL;
    ObjInst *ob;
    int nname = 0, nmiss = 0, nhave = 0, i, aln = L->cur.line, dotted = 0;
    size_t mo = 0, ho = 0;
    lex_next(L);
    miss[0] = 0;
    havebag[0] = 0;
    first[0] = 0;
    target[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      if (hard) {
        fail_at(vm, L, "NEEDMETHODANY Class|obj methodâ¦ â NEEDMETHODANY Cell tick Â· bag Class.method");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASMETHODANY_N", 0);
      var_set_num(vm, "METHODMISS_N", 0);
      var_set_num(vm, "METHODHAVE_N", 0);
      var_set_num(vm, "METHMISS_N", 0);
      var_set_num(vm, "METHHAVE_N", 0);
      var_set_str(vm, "METHODMISS", "");
      var_set_str(vm, "METHODHAVE", "");
      var_set_str(vm, "METHMISS", "");
      var_set_str(vm, "METHHAVE", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASMETHODANY: need Class|obj methodâ¦ or Class.method bag");
      var_set_str(vm, "ERR", "HASMETHODANY: need Class|obj methodâ¦ or Class.method bag");
      bump(vm);
      return 1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(first, sizeof first, "%s", L->cur.text);
      lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      if (oop_find_obj(vm, id) || oop_find_class(vm, id)) {
        snprintf(first, sizeof first, "%s", id);
      } else {
        vv = var_get(vm, id, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(first, sizeof first, "%s", vv->sval);
        else
          snprintf(first, sizeof first, "%s", id);
      }
    }
    if (strchr(first, '.')) {
      int more = (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT);
      if (more && L->cur.kind == TK_IDENT &&
          (kw(&L->cur, "ASSERT") || kw(&L->cur, "LET") || kw(&L->cur, "PRINT") ||
           kw(&L->cur, "REQUIRE") || kw(&L->cur, "END") ||
           kw(&L->cur, "IF") || kw(&L->cur, "ELSE") || kw(&L->cur, "ELIF") ||
           kw(&L->cur, "PASS") || kw(&L->cur, "FAIL") || kw(&L->cur, "INCLUDE") ||
           kw(&L->cur, "SYS") || kw(&L->cur, "HELP") || kw(&L->cur, "CLEAR_ERR") ||
           kw(&L->cur, "NOTE") || kw(&L->cur, "EXIT") || kw(&L->cur, "DEFAULT") ||
           kw(&L->cur, "VERSION") || kw(&L->cur, "STATUS") || kw(&L->cur, "FOR") ||
           kw(&L->cur, "WHILE") || kw(&L->cur, "LOOP") || kw(&L->cur, "EACH") ||
           kw(&L->cur, "CUBE") || kw(&L->cur, "PLUG") || kw(&L->cur, "HOLD_FLASH") ||
           kw(&L->cur, "HASCLASS") || kw(&L->cur, "HASCLASSES") ||
           kw(&L->cur, "HASMETHOD") || kw(&L->cur, "HASMETHODS") ||
           kw(&L->cur, "HASMETHODANY") || kw(&L->cur, "NEEDMETHODS") ||
           kw(&L->cur, "NEEDMETHODANY") || kw(&L->cur, "NEEDCLASSES") ||
           kw(&L->cur, "LISTMETHODS") || kw(&L->cur, "LISTCLASSES") ||
           kw(&L->cur, "LISTOBJS") || kw(&L->cur, "HASFORM") ||
           kw(&L->cur, "HASOBJ")))
        more = 0;
      if (!more || strchr(first, '\n') || strchr(first, ',') || strchr(first, ' '))
        dotted = 1;
    }
    if (dotted) {
      const char *p = first;
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
      if (nname == 0) {
        if (hard) {
          fail_at(vm, L, "NEEDMETHODANY Class.methodâ¦ â NEEDMETHODANY Ticket.ping Order.pay");
          return -1;
        }
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "0");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "HASMETHODANY_N", 0);
        var_set_num(vm, "METHODMISS_N", 0);
        var_set_num(vm, "METHODHAVE_N", 0);
        var_set_num(vm, "METHMISS_N", 0);
        var_set_num(vm, "METHHAVE_N", 0);
        var_set_str(vm, "METHODMISS", "");
        var_set_str(vm, "METHODHAVE", "");
        var_set_str(vm, "METHMISS", "");
        var_set_str(vm, "METHHAVE", "");
        var_set_str(vm, "METHOD_ON", "*");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "HASMETHODANY: need Class.method bag");
        var_set_str(vm, "ERR", "HASMETHODANY: need Class.method bag");
        bump(vm);
        return 1;
      }
      for (i = 0; i < nname; i++) {
        char cname[48], mname[48];
        const char *dot = strchr(names[i], '.');
        int hit = 0;
        if (dot && dot != names[i] && dot[1]) {
          size_t cl = (size_t)(dot - names[i]);
          if (cl + 1 < sizeof cname) {
            memcpy(cname, names[i], cl);
            cname[cl] = 0;
          } else
            cname[0] = 0;
          snprintf(mname, sizeof mname, "%s", dot + 1);
          ob = oop_find_obj(vm, cname);
          if (ob)
            cd = &vm->classes[ob->class_idx];
          else
            cd = oop_find_class(vm, cname);
          if (cd && oop_find_method(cd, mname))
            hit = 1;
        }
        if (hit) {
          nhave++;
          if (ho && ho + 1 < sizeof havebag) havebag[ho++] = '\n';
          {
            size_t ln = strlen(names[i]);
            if (ho + ln < sizeof havebag) {
              memcpy(havebag + ho, names[i], ln);
              ho += ln;
              havebag[ho] = 0;
            }
          }
        } else {
          nmiss++;
          if (mo && mo + 1 < sizeof miss) miss[mo++] = '\n';
          {
            size_t ln = strlen(names[i]);
            if (mo + ln < sizeof miss) {
              memcpy(miss + mo, names[i], ln);
              mo += ln;
              miss[mo] = 0;
            }
          }
        }
      }
      var_set_str(vm, "METHODMISS", miss);
      var_set_str(vm, "METHODHAVE", havebag);
      var_set_str(vm, "METHMISS", miss);
      var_set_str(vm, "METHHAVE", havebag);
      var_set_num(vm, "METHODMISS_N", nmiss);
      var_set_num(vm, "METHODHAVE_N", nhave);
      var_set_num(vm, "METHMISS_N", nmiss);
      var_set_num(vm, "METHHAVE_N", nhave);
      var_set_num(vm, "HASMETHODANY_N", nhave);
      var_set_num(vm, "NEEDMETHODANY_N", nname);
      var_set_num(vm, "METHODS_ANY_N", nname);
      var_set_str(vm, "METHOD_ON", "*");
      if (nhave > 0) {
        var_set_num(vm, "LAST_N", 1);
        vm->last_n = 1;
        var_set_str(vm, "LAST", "1");
        snprintf(vm->last_str, sizeof vm->last_str, "1");
        var_set_num(vm, "OK", 1);
      } else if (hard) {
        char em[240];
        snprintf(em, sizeof em,
                 "NEEDMETHODANY miss line %d: need one of [%s] â CLASS METHOD Â· HASMETHODANY Â· LISTMETHODS",
                 aln, miss[0] ? miss : "?");
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        fail_at(vm, L, em);
        return -1;
      } else {
        char em[240];
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "0");
        snprintf(em, sizeof em,
                 "HASMETHODANY miss (0/%d): need one of %s â NEEDMETHODANY Â· LISTMETHODS",
                 nname, miss[0] ? miss : "?");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", em);
        var_set_str(vm, "ERR", em);
      }
      if (vm->trace)
        fprintf(vm->trace, "# %s dotted n=%d have=%d miss=%d\n",
                hard ? "NEEDMETHODANY" : "HASMETHODANY", nname, nhave, nmiss);
      bump(vm);
      return 1;
    }
    /* classic Class|obj methodâ¦ form */
    snprintf(target, sizeof target, "%s", first);
    while (nname < 32 &&
           (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur, "ASSERT") || kw(&L->cur, "LET") || kw(&L->cur, "PRINT") ||
           kw(&L->cur, "REQUIRE") || kw(&L->cur, "END") ||
           kw(&L->cur, "IF") || kw(&L->cur, "ELSE") || kw(&L->cur, "ELIF") ||
           kw(&L->cur, "PASS") || kw(&L->cur, "FAIL") || kw(&L->cur, "INCLUDE") ||
           kw(&L->cur, "SYS") || kw(&L->cur, "HELP") || kw(&L->cur, "CLEAR_ERR") ||
           kw(&L->cur, "NOTE") || kw(&L->cur, "EXIT") || kw(&L->cur, "DEFAULT") ||
           kw(&L->cur, "VERSION") || kw(&L->cur, "STATUS") || kw(&L->cur, "FOR") ||
           kw(&L->cur, "WHILE") || kw(&L->cur, "LOOP") || kw(&L->cur, "EACH") ||
           kw(&L->cur, "CUBE") || kw(&L->cur, "PLUG") || kw(&L->cur, "HOLD_FLASH") ||
           kw(&L->cur, "HASCLASS") || kw(&L->cur, "HASCLASSES") ||
           kw(&L->cur, "HASMETHOD") || kw(&L->cur, "HASMETHODS") ||
           kw(&L->cur, "HASMETHODANY") || kw(&L->cur, "NEEDMETHODS") ||
           kw(&L->cur, "NEEDMETHODANY") || kw(&L->cur, "NEEDCLASSES") ||
           kw(&L->cur, "LISTMETHODS") || kw(&L->cur, "LISTCLASSES") ||
           kw(&L->cur, "LISTOBJS") || kw(&L->cur, "HASFORM") ||
           kw(&L->cur, "HASOBJ")))
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
                   strchr(vv->sval, '.') == NULL &&
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
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDMETHODANY Class|obj methodâ¦ â NEEDMETHODANY Cell tick die");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASMETHODANY_N", 0);
      var_set_num(vm, "METHODMISS_N", 0);
      var_set_num(vm, "METHODHAVE_N", 0);
      var_set_num(vm, "METHMISS_N", 0);
      var_set_num(vm, "METHHAVE_N", 0);
      var_set_str(vm, "METHODMISS", "");
      var_set_str(vm, "METHODHAVE", "");
      var_set_str(vm, "METHMISS", "");
      var_set_str(vm, "METHHAVE", "");
      var_set_str(vm, "METHOD_ON", target);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASMETHODANY: need methods â HASMETHODANY Cell tick ghost");
      var_set_str(vm, "ERR", "HASMETHODANY: need methods â HASMETHODANY Cell tick ghost");
      bump(vm);
      return 1;
    }
    ob = oop_find_obj(vm, target);
    if (ob)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, target);
    for (i = 0; i < nname; i++) {
      if (cd && oop_find_method(cd, names[i])) {
        nhave++;
        if (ho && ho + 1 < sizeof havebag) havebag[ho++] = '\n';
        {
          size_t ln = strlen(names[i]);
          if (ho + ln < sizeof havebag) {
            memcpy(havebag + ho, names[i], ln);
            ho += ln;
            havebag[ho] = 0;
          }
        }
      } else {
        nmiss++;
        if (mo && mo + 1 < sizeof miss) miss[mo++] = '\n';
        {
          size_t ln = strlen(names[i]);
          if (mo + ln < sizeof miss) {
            memcpy(miss + mo, names[i], ln);
            mo += ln;
            miss[mo] = 0;
          }
        }
      }
    }
    var_set_str(vm, "METHODMISS", miss);
    var_set_str(vm, "METHODHAVE", havebag);
    var_set_str(vm, "METHMISS", miss);
    var_set_str(vm, "METHHAVE", havebag);
    var_set_num(vm, "METHODMISS_N", nmiss);
    var_set_num(vm, "METHODHAVE_N", nhave);
    var_set_num(vm, "METHMISS_N", nmiss);
    var_set_num(vm, "METHHAVE_N", nhave);
    var_set_num(vm, "HASMETHODANY_N", nhave);
    var_set_num(vm, "NEEDMETHODANY_N", nname);
    var_set_num(vm, "METHODS_ANY_N", nname);
    var_set_str(vm, "METHOD_ON", target);
    if (nhave > 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else if (hard) {
      char em[240];
      snprintf(em, sizeof em,
               "NEEDMETHODANY miss line %d: need one of %s.%s â CLASS METHOD Â· HASMETHODANY Â· LISTMETHODS",
               aln, target[0] ? target : "?", miss[0] ? miss : "?");
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      fail_at(vm, L, em);
      return -1;
    } else {
      char em[240];
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      snprintf(em, sizeof em,
               "HASMETHODANY miss (0/%d) on %s: need one of %s â NEEDMETHODANY Â· LISTMETHODS",
               nname, target[0] ? target : "?", miss[0] ? miss : "?");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s %s n=%d have=%d miss=%d\n",
              hard ? "NEEDMETHODANY" : "HASMETHODANY", target, nname, nhave, nmiss);
    bump(vm);
    return 1;
  }
  return 0;
}
