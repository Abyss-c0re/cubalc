/* CubalC lang — lang_ops_flow_fstr_hasfield.c
 * FLOW fstr field-existence: HASFIELD · HASFIELDS · HASFIELDANY · FIELDINFO
 * Peel from fstr_b (B_fstr_b_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_hasfield.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_hasfield(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* HASFIELD obj|Class field â soft 0|1 probe before GETF/SETF.
   * Class/obj and field names expand string-vars (FIELD_ON / name bags).
   * Twin of HASMETHOD resolve. */
  if (kw(&L->cur, "HASFIELD") || kw(&L->cur, "HASF") ||
      kw(&L->cur, "FIELD?") || kw(&L->cur, "HASFILD") ||
      kw(&L->cur, "HAS_FIELD")) {
    char a[48], fname[48];
    ClassDef *cd = NULL;
    ObjInst *ob;
    int hit = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "HASFIELD obj|Class field"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text); lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
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
      fail(vm, "HASFIELD field"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(fname, sizeof fname, "%s", L->cur.text); lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      vv = var_get(vm, id, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(fname, sizeof fname, "%s", vv->sval);
      else
        snprintf(fname, sizeof fname, "%s", id);
    }
    ob = oop_find_obj(vm, a);
    if (ob) cd = &vm->classes[ob->class_idx];
    else cd = oop_find_class(vm, a);
    if (cd && oop_field_idx(cd, fname) >= 0) hit = 1;
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASFIELD_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASFIELDS|NEEDFIELDS Class|obj fieldâ¦ | Class.field bag â
   * multi field capability gate (all-or-FIELDMISS).
   * Soft: LAST_N 0|1 Â· FIELDMISS/FLDMISS bag Â· NEEDFIELDS fail-fast first miss.
   * Forms: HASFIELDS Cell x y Â· HASFIELDS bag_of_Class.field (multi-class).
   * Usability: agent GETF/SETF / field_guard contracts without NÃ HASFIELD soup. */
  if (kw(&L->cur, "HASFIELDS") || kw(&L->cur, "HAS_FIELDS") ||
      kw(&L->cur, "ALLFIELDS") || kw(&L->cur, "FIELDSOK") ||
      kw(&L->cur, "NEEDFIELDS") || kw(&L->cur, "NEED_FIELDS") ||
      kw(&L->cur, "REQUIREFIELDS") || kw(&L->cur, "MUSTFIELDS")) {
    int hard = kw(&L->cur, "NEEDFIELDS") || kw(&L->cur, "NEED_FIELDS") ||
               kw(&L->cur, "REQUIREFIELDS") || kw(&L->cur, "MUSTFIELDS");
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
        fail_at(vm, L, "NEEDFIELDS Class|obj fieldâ¦ â NEEDFIELDS Cell x Â· bag Class.field");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASFIELDS_N", 0);
      var_set_num(vm, "FIELDMISS_N", 0);
      var_set_num(vm, "FLDMISS_N", 0);
      var_set_str(vm, "FIELDMISS", "");
      var_set_str(vm, "FLDMISS", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASFIELDS: need Class|obj fieldâ¦ or Class.field bag");
      var_set_str(vm, "ERR", "HASFIELDS: need Class|obj fieldâ¦ or Class.field bag");
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
           kw(&L->cur, "HASFIELD") || kw(&L->cur, "HASFIELDS") ||
           kw(&L->cur, "HASFIELDANY") || kw(&L->cur, "NEEDFIELDS") ||
           kw(&L->cur, "NEEDFIELDANY") || kw(&L->cur, "LISTFIELDS") ||
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
          fail_at(vm, L, "NEEDFIELDS Class.fieldâ¦ â NEEDFIELDS Ticket.id Order.n");
          return -1;
        }
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "0");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "HASFIELDS_N", 0);
        var_set_num(vm, "FIELDMISS_N", 0);
        var_set_num(vm, "FLDMISS_N", 0);
        var_set_str(vm, "FIELDMISS", "");
        var_set_str(vm, "FLDMISS", "");
        var_set_str(vm, "FIELD_ON", "*");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "HASFIELDS: need Class.field bag");
        var_set_str(vm, "ERR", "HASFIELDS: need Class.field bag");
        bump(vm);
        return 1;
      }
      for (i = 0; i < nname; i++) {
        char cname[48], fname[48];
        const char *dot = strchr(names[i], '.');
        int hit = 0;
        cname[0] = 0;
        fname[0] = 0;
        if (dot && dot != names[i] && dot[1]) {
          size_t cl = (size_t)(dot - names[i]);
          if (cl + 1 < sizeof cname) {
            memcpy(cname, names[i], cl);
            cname[cl] = 0;
          }
          snprintf(fname, sizeof fname, "%s", dot + 1);
          ob = oop_find_obj(vm, cname);
          if (ob)
            cd = &vm->classes[ob->class_idx];
          else
            cd = oop_find_class(vm, cname);
          if (cd && oop_field_idx(cd, fname) >= 0)
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
                   "NEEDFIELDS miss line %d: '%s' (+%d more?) â CLASS FIELD Â· HASFIELD Â· LISTFIELDS",
                   aln, names[i], nname - i - 1);
          fail_at(vm, L, em);
          return -1;
        }
      }
      var_set_str(vm, "FIELDMISS", miss);
      var_set_str(vm, "FLDMISS", miss);
      var_set_num(vm, "FIELDMISS_N", nmiss);
      var_set_num(vm, "FLDMISS_N", nmiss);
      var_set_num(vm, "HASFIELDS_N", nname - nmiss);
      var_set_num(vm, "NEEDFIELDS_N", nname);
      var_set_num(vm, "FIELDS_N", nname);
      var_set_str(vm, "FIELD_ON", "*");
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
                 "HASFIELDS miss (%d/%d): %s â CLASS FIELD Â· NEEDFIELDS Â· LISTFIELDS",
                 nmiss, nname, miss);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", em);
        var_set_str(vm, "ERR", em);
      }
      if (vm->trace)
        fprintf(vm->trace, "# %s dotted n=%d miss=%d\n",
                hard ? "NEEDFIELDS" : "HASFIELDS", nname, nmiss);
      bump(vm);
      return 1;
    }
    /* classic Class|obj fieldâ¦ form */
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
           kw(&L->cur, "HASFIELD") || kw(&L->cur, "HASFIELDS") ||
           kw(&L->cur, "HASFIELDANY") || kw(&L->cur, "NEEDFIELDS") ||
           kw(&L->cur, "NEEDFIELDANY") || kw(&L->cur, "LISTFIELDS") ||
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
        fail_at(vm, L, "NEEDFIELDS Class|obj fieldâ¦ â NEEDFIELDS Cell x y");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASFIELDS_N", 0);
      var_set_num(vm, "FIELDMISS_N", 0);
      var_set_num(vm, "FLDMISS_N", 0);
      var_set_str(vm, "FIELDMISS", "");
      var_set_str(vm, "FLDMISS", "");
      var_set_str(vm, "FIELD_ON", target);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASFIELDS: need fields â HASFIELDS Cell x y");
      var_set_str(vm, "ERR", "HASFIELDS: need fields â HASFIELDS Cell x y");
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
      if (cd && oop_field_idx(cd, names[i]) >= 0)
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
                 "NEEDFIELDS miss line %d: '%s.%s' (+%d more?) â CLASS FIELD Â· HASFIELD Â· LISTFIELDS",
                 aln, target[0] ? target : "?", names[i], nname - i - 1);
        fail_at(vm, L, em);
        return -1;
      }
    }
    var_set_str(vm, "FIELDMISS", miss);
    var_set_str(vm, "FLDMISS", miss);
    var_set_num(vm, "FIELDMISS_N", nmiss);
    var_set_num(vm, "FLDMISS_N", nmiss);
    var_set_num(vm, "HASFIELDS_N", nname - nmiss);
    var_set_num(vm, "NEEDFIELDS_N", nname);
    var_set_num(vm, "FIELDS_N", nname);
    var_set_str(vm, "FIELD_ON", target);
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
               "HASFIELDS miss (%d/%d) on %s: %s â CLASS FIELD Â· NEEDFIELDS Â· LISTFIELDS",
               nmiss, nname, target[0] ? target : "?", miss);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s %s n=%d miss=%d\n",
              hard ? "NEEDFIELDS" : "HASFIELDS", target, nname, nmiss);
    bump(vm);
    return 1;
  }

  /* HASFIELDANY|NEEDFIELDANY Class|obj fieldâ¦ | Class.field bag â
   * multi field any-of gate (HASFIELDS twin).
   * Soft: LAST_N 1 if any match Â· FIELDHAVE/FLDHAVE Â· FIELDMISS/FLDMISS.
   * Forms: HASFIELDANY Cell alive Â· HASFIELDANY bag_of_Class.field.
   * NEEDFIELDANY fail-fast if none match. */
  if (kw(&L->cur, "HASFIELDANY") || kw(&L->cur, "HAS_FIELD_ANY") ||
      kw(&L->cur, "ANYFIELD") || kw(&L->cur, "FIELDANY") ||
      kw(&L->cur, "HASANYFIELD") || kw(&L->cur, "NEEDFIELDANY") ||
      kw(&L->cur, "NEED_FIELD_ANY") || kw(&L->cur, "REQUIREFIELDANY") ||
      kw(&L->cur, "MUSTFIELDANY")) {
    int hard = kw(&L->cur, "NEEDFIELDANY") || kw(&L->cur, "NEED_FIELD_ANY") ||
               kw(&L->cur, "REQUIREFIELDANY") || kw(&L->cur, "MUSTFIELDANY");
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
        fail_at(vm, L, "NEEDFIELDANY Class|obj fieldâ¦ â NEEDFIELDANY Cell alive Â· bag Class.field");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASFIELDANY_N", 0);
      var_set_num(vm, "FIELDMISS_N", 0);
      var_set_num(vm, "FIELDHAVE_N", 0);
      var_set_num(vm, "FLDMISS_N", 0);
      var_set_num(vm, "FLDHAVE_N", 0);
      var_set_str(vm, "FIELDMISS", "");
      var_set_str(vm, "FIELDHAVE", "");
      var_set_str(vm, "FLDMISS", "");
      var_set_str(vm, "FLDHAVE", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASFIELDANY: need Class|obj fieldâ¦ or Class.field bag");
      var_set_str(vm, "ERR", "HASFIELDANY: need Class|obj fieldâ¦ or Class.field bag");
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
           kw(&L->cur, "HASFIELD") || kw(&L->cur, "HASFIELDS") ||
           kw(&L->cur, "HASFIELDANY") || kw(&L->cur, "NEEDFIELDS") ||
           kw(&L->cur, "NEEDFIELDANY") || kw(&L->cur, "LISTFIELDS") ||
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
          fail_at(vm, L, "NEEDFIELDANY Class.fieldâ¦ â NEEDFIELDANY Ticket.id Order.n");
          return -1;
        }
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "0");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "HASFIELDANY_N", 0);
        var_set_num(vm, "FIELDMISS_N", 0);
        var_set_num(vm, "FIELDHAVE_N", 0);
        var_set_num(vm, "FLDMISS_N", 0);
        var_set_num(vm, "FLDHAVE_N", 0);
        var_set_str(vm, "FIELDMISS", "");
        var_set_str(vm, "FIELDHAVE", "");
        var_set_str(vm, "FLDMISS", "");
        var_set_str(vm, "FLDHAVE", "");
        var_set_str(vm, "FIELD_ON", "*");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "HASFIELDANY: need Class.field bag");
        var_set_str(vm, "ERR", "HASFIELDANY: need Class.field bag");
        bump(vm);
        return 1;
      }
      for (i = 0; i < nname; i++) {
        char cname[48], fname[48];
        const char *dot = strchr(names[i], '.');
        int hit = 0;
        if (dot && dot != names[i] && dot[1]) {
          size_t cl = (size_t)(dot - names[i]);
          if (cl + 1 < sizeof cname) {
            memcpy(cname, names[i], cl);
            cname[cl] = 0;
          } else
            cname[0] = 0;
          snprintf(fname, sizeof fname, "%s", dot + 1);
          ob = oop_find_obj(vm, cname);
          if (ob)
            cd = &vm->classes[ob->class_idx];
          else
            cd = oop_find_class(vm, cname);
          if (cd && oop_field_idx(cd, fname) >= 0)
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
      var_set_str(vm, "FIELDMISS", miss);
      var_set_str(vm, "FIELDHAVE", havebag);
      var_set_str(vm, "FLDMISS", miss);
      var_set_str(vm, "FLDHAVE", havebag);
      var_set_num(vm, "FIELDMISS_N", nmiss);
      var_set_num(vm, "FIELDHAVE_N", nhave);
      var_set_num(vm, "FLDMISS_N", nmiss);
      var_set_num(vm, "FLDHAVE_N", nhave);
      var_set_num(vm, "HASFIELDANY_N", nhave);
      var_set_num(vm, "NEEDFIELDANY_N", nname);
      var_set_num(vm, "FIELDS_ANY_N", nname);
      var_set_str(vm, "FIELD_ON", "*");
      if (nhave > 0) {
        var_set_num(vm, "LAST_N", 1);
        vm->last_n = 1;
        var_set_str(vm, "LAST", "1");
        snprintf(vm->last_str, sizeof vm->last_str, "1");
        var_set_num(vm, "OK", 1);
      } else if (hard) {
        char em[240];
        snprintf(em, sizeof em,
                 "NEEDFIELDANY miss line %d: need one of [%s] â CLASS FIELD Â· HASFIELDANY Â· LISTFIELDS",
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
                 "HASFIELDANY miss (0/%d): need one of %s â NEEDFIELDANY Â· LISTFIELDS",
                 nname, miss[0] ? miss : "?");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", em);
        var_set_str(vm, "ERR", em);
      }
      if (vm->trace)
        fprintf(vm->trace, "# %s dotted n=%d have=%d miss=%d\n",
                hard ? "NEEDFIELDANY" : "HASFIELDANY", nname, nhave, nmiss);
      bump(vm);
      return 1;
    }
    /* classic Class|obj fieldâ¦ form */
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
           kw(&L->cur, "HASFIELD") || kw(&L->cur, "HASFIELDS") ||
           kw(&L->cur, "HASFIELDANY") || kw(&L->cur, "NEEDFIELDS") ||
           kw(&L->cur, "NEEDFIELDANY") || kw(&L->cur, "LISTFIELDS") ||
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
        fail_at(vm, L, "NEEDFIELDANY Class|obj fieldâ¦ â NEEDFIELDANY Cell alive energy");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASFIELDANY_N", 0);
      var_set_num(vm, "FIELDMISS_N", 0);
      var_set_num(vm, "FIELDHAVE_N", 0);
      var_set_num(vm, "FLDMISS_N", 0);
      var_set_num(vm, "FLDHAVE_N", 0);
      var_set_str(vm, "FIELDMISS", "");
      var_set_str(vm, "FIELDHAVE", "");
      var_set_str(vm, "FLDMISS", "");
      var_set_str(vm, "FLDHAVE", "");
      var_set_str(vm, "FIELD_ON", target);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASFIELDANY: need fields â HASFIELDANY Cell alive ghost");
      var_set_str(vm, "ERR", "HASFIELDANY: need fields â HASFIELDANY Cell alive ghost");
      bump(vm);
      return 1;
    }
    ob = oop_find_obj(vm, target);
    if (ob)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, target);
    for (i = 0; i < nname; i++) {
      if (cd && oop_field_idx(cd, names[i]) >= 0) {
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
    var_set_str(vm, "FIELDMISS", miss);
    var_set_str(vm, "FIELDHAVE", havebag);
    var_set_str(vm, "FLDMISS", miss);
    var_set_str(vm, "FLDHAVE", havebag);
    var_set_num(vm, "FIELDMISS_N", nmiss);
    var_set_num(vm, "FIELDHAVE_N", nhave);
    var_set_num(vm, "FLDMISS_N", nmiss);
    var_set_num(vm, "FLDHAVE_N", nhave);
    var_set_num(vm, "HASFIELDANY_N", nhave);
    var_set_num(vm, "NEEDFIELDANY_N", nname);
    var_set_num(vm, "FIELDS_ANY_N", nname);
    var_set_str(vm, "FIELD_ON", target);
    if (nhave > 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else if (hard) {
      char em[240];
      snprintf(em, sizeof em,
               "NEEDFIELDANY miss line %d: need one of %s.%s â CLASS FIELD Â· HASFIELDANY Â· LISTFIELDS",
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
               "HASFIELDANY miss (0/%d) on %s: need one of %s â NEEDFIELDANY Â· LISTFIELDS",
               nname, target[0] ? target : "?", miss[0] ? miss : "?");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s %s n=%d have=%d miss=%d\n",
              hard ? "NEEDFIELDANY" : "HASFIELDANY", target, nname, nhave, nmiss);
    bump(vm);
    return 1;
  }

  /* LISTFIELDS Class|obj â newline bag of field names.
   * String-var expands (FIELD_ON / class name bags) Â· twin of LISTMETHODS. */
  if (kw(&L->cur, "LISTFIELDS") || kw(&L->cur, "FIELDS") ||
      kw(&L->cur, "FIELDLIST") || kw(&L->cur, "LISTF")) {
    char a[48], bag[2048];
    ClassDef *cd = NULL;
    ObjInst *ob;
    size_t o = 0;
    int i, n = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "LISTFIELDS Class|obj"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text); lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
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
    ob = oop_find_obj(vm, a);
    if (ob) cd = &vm->classes[ob->class_idx];
    else cd = oop_find_class(vm, a);
    bag[0] = 0;
    if (cd) {
      for (i = 0; i < cd->n_fields; i++) {
        size_t ln = strlen(cd->fields[i].name);
        if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) {
          memcpy(bag + o, cd->fields[i].name, ln);
          o += ln;
        }
        bag[o] = 0;
        n++;
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTFIELDS", bag);
    var_set_str(vm, "FIELDS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "NFIELDS", n);
    var_set_num(vm, "LISTFIELDS_N", n);
    var_set_num(vm, "OK", cd ? 1 : 0);
    if (!cd) {
      var_set_str(vm, "LAST_ERR", "LISTFIELDS: unknown class/obj");
      var_set_str(vm, "ERR", "LISTFIELDS: unknown class/obj");
    }
    bump(vm);
    return 1;
  }

  /* FIELDINFO|DUMPFIELD|DESCRIBEFIELD [JSON] Class|obj field
   * â field kind/default plate for agent SETF/NEW prep (twin of METHODINFO).
   * Default bag: name/class/kind/has_def/default[/value if live obj].
   * JSON â cubalc.field.v1. Soft OK=0 if class or field missing. */
  if (kw(&L->cur, "FIELDINFO") || kw(&L->cur, "DUMPFIELD") ||
      kw(&L->cur, "DESCRIBEFIELD") || kw(&L->cur, "FIELDDEF") ||
      kw(&L->cur, "INSPECTFIELD") || kw(&L->cur, "FIELDMETA") ||
      kw(&L->cur, "FIELDSCHEMA") || kw(&L->cur, "PROPINFO") ||
      kw(&L->cur, "ATTRINFO") || kw(&L->cur, "MEMBERINFO")) {
    char a[48], fname[48], bag[2048], defbuf[160], valbuf[160];
    ClassDef *cd = NULL;
    FieldDef *fd = NULL;
    ObjInst *ob = NULL;
    int fi = -1, as_json = 0, has_live = 0;
    const char *kind;
    lex_next(L);
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "FIELDINFO [JSON] Class|obj field"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    } else {
      if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      else
        snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    /* optional FIELD|PROP keyword (not NAME â conflicts with field id) */
    if (kw(&L->cur, "FIELD") || kw(&L->cur, "PROP") || kw(&L->cur, "ATTR") ||
        kw(&L->cur, "MEMBER") || kw(&L->cur, "OF"))
      lex_next(L);
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
      fail(vm, "FIELDINFO [JSON] Class|obj field"); return -1;
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    if (cd) {
      fi = oop_field_idx(cd, fname);
      if (fi >= 0) fd = &cd->fields[fi];
    }
    bag[0] = 0;
    defbuf[0] = 0;
    valbuf[0] = 0;
    if (!cd || !fd) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "FIELDINFO_N", 0);
      var_set_num(vm, "HAS_DEF", 0);
      var_set_num(vm, "IS_STR", 0);
      var_set_str(vm, "FIELDINFO", "");
      var_set_str(vm, "KIND", "");
      var_set_str(vm, "DEFAULT", "");
      var_set_str(vm, "FIELD", fname);
      if (cd) var_set_str(vm, "CLASS", cd->name);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  !cd ? "FIELDINFO: unknown class/obj"
                      : "FIELDINFO: unknown field");
      var_set_str(vm, "ERR",
                  !cd ? "FIELDINFO: unknown class/obj"
                      : "FIELDINFO: unknown field");
      bump(vm);
      return 1;
    }
    if (fd->is_str) kind = "str";
    else if (fd->has_def) kind = "num";
    else kind = "num"; /* unset defaults act as numeric 0 on NEW */
    if (fd->has_def) {
      if (fd->is_str)
        snprintf(defbuf, sizeof defbuf, "%s", fd->def_str);
      else
        snprintf(defbuf, sizeof defbuf, "%ld", fd->def_num);
    }
    if (ob && fi >= 0 && fi < CUBALC_MAX_FIELDS) {
      has_live = 1;
      if (ob->fis_str[fi])
        snprintf(valbuf, sizeof valbuf, "%s", ob->fstr[fi]);
      else
        snprintf(valbuf, sizeof valbuf, "%ld", ob->fnum[fi]);
    }
    if (as_json) {
      if (has_live) {
        if (ob->fis_str[fi])
          snprintf(bag, sizeof bag,
                   "{\"schema\":\"cubalc.field.v1\",\"class\":\"%s\","
                   "\"name\":\"%s\",\"kind\":\"%s\",\"has_def\":%d,"
                   "\"is_str\":%d,\"default\":\"%s\",\"value\":\"%s\"}",
                   cd->name, fd->name, kind, fd->has_def ? 1 : 0,
                   fd->is_str ? 1 : 0, defbuf, valbuf);
        else
          snprintf(bag, sizeof bag,
                   "{\"schema\":\"cubalc.field.v1\",\"class\":\"%s\","
                   "\"name\":\"%s\",\"kind\":\"%s\",\"has_def\":%d,"
                   "\"is_str\":%d,\"default\":%s,\"value\":%s}",
                   cd->name, fd->name, kind, fd->has_def ? 1 : 0,
                   fd->is_str ? 1 : 0,
                   fd->has_def ? defbuf : "null",
                   valbuf);
      } else if (fd->is_str) {
        snprintf(bag, sizeof bag,
                 "{\"schema\":\"cubalc.field.v1\",\"class\":\"%s\","
                 "\"name\":\"%s\",\"kind\":\"%s\",\"has_def\":%d,"
                 "\"is_str\":1,\"default\":\"%s\"}",
                 cd->name, fd->name, kind, fd->has_def ? 1 : 0, defbuf);
      } else {
        snprintf(bag, sizeof bag,
                 "{\"schema\":\"cubalc.field.v1\",\"class\":\"%s\","
                 "\"name\":\"%s\",\"kind\":\"%s\",\"has_def\":%d,"
                 "\"is_str\":0,\"default\":%s}",
                 cd->name, fd->name, kind, fd->has_def ? 1 : 0,
                 fd->has_def ? defbuf : "null");
      }
    } else {
      if (has_live)
        snprintf(bag, sizeof bag,
                 "name:%s\nclass:%s\nkind:%s\nhas_def:%d\nis_str:%d\n"
                 "default:%s\nvalue:%s",
                 fd->name, cd->name, kind, fd->has_def ? 1 : 0,
                 fd->is_str ? 1 : 0, defbuf, valbuf);
      else
        snprintf(bag, sizeof bag,
                 "name:%s\nclass:%s\nkind:%s\nhas_def:%d\nis_str:%d\n"
                 "default:%s",
                 fd->name, cd->name, kind, fd->has_def ? 1 : 0,
                 fd->is_str ? 1 : 0, defbuf);
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "FIELDINFO", bag);
    var_set_str(vm, "DUMPFIELD", bag);
    var_set_str(vm, "DESCRIBEFIELD", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = fd->has_def ? 1 : 0;
    var_set_num(vm, "LAST_N", fd->has_def ? 1 : 0);
    var_set_num(vm, "FIELDINFO_N", 1);
    var_set_num(vm, "HAS_DEF", fd->has_def ? 1 : 0);
    var_set_num(vm, "IS_STR", fd->is_str ? 1 : 0);
    var_set_str(vm, "FIELD", fd->name);
    var_set_str(vm, "CLASS", cd->name);
    var_set_str(vm, "KIND", kind);
    var_set_str(vm, "DEFAULT", defbuf);
    if (has_live) var_set_str(vm, "VALUE", valbuf);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }
  return 0;
}
