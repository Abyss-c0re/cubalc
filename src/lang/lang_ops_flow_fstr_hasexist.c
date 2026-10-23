/* CubalC lang — lang_ops_flow_fstr_hasexist.c
 * FLOW fstr existence: HASCLASS* · HASOBJ* · HASFN*
 * Peel from fstr_b (B_fstr_b_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_hasexist.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_hasexist(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* HASCLASS|KNOWNCLASS|CLASS? name â soft 0|1 probe if CLASS is defined.
   * Complements REQUIRE CLASS (fail-fast) and LISTCLASSES+HASLINE glue.
   * String-var class names expand. LAST = name when hit, else "".
   * Usability: IF before ENSURENEW/TRYNEW without bag materialize. */
  if (kw(&L->cur, "HASCLASS") || kw(&L->cur, "KNOWNCLASS") ||
      kw(&L->cur, "CLASS?") || kw(&L->cur, "HAS_CLASS") ||
      kw(&L->cur, "ISDEFINEDCLASS") || kw(&L->cur, "CLASSDEF?") ||
      kw(&L->cur, "TYPEDEFINED") || kw(&L->cur, "HASTYPE")) {
    char cname[48];
    int hit = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "HASCLASS name"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(cname, sizeof cname, "%s", L->cur.text);
      lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      if (oop_find_class(vm, id))
        snprintf(cname, sizeof cname, "%s", id);
      else {
        vv = var_get(vm, id, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(cname, sizeof cname, "%s", vv->sval);
        else
          snprintf(cname, sizeof cname, "%s", id);
      }
    }
    hit = oop_find_class(vm, cname) ? 1 : 0;
    if (hit) {
      var_set_str(vm, "LAST", cname);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", cname);
      var_set_str(vm, "CLASS", cname);
    } else {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
    }
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    var_set_num(vm, "HASCLASS_N", hit);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# HASCLASS %s -> %d\n", cname, hit);
    bump(vm);
    return 1;
  }

  /* HASCLASSES|NEEDCLASSES nameâ¦ â multi CLASS capability gate (all-or-CLASSMISS).
   * Soft: LAST_N 0|1 Â· CLASSMISS bag Â· NEEDCLASSES fail-fast first miss.
   * Usability: agent boot contracts without NÃ HASCLASS glue Â· bag/CSV/idents.
   * Twin of HASFORMS for OOP plane Â· dual REQUIRE CLASS multi. */
  if (kw(&L->cur, "HASCLASSES") || kw(&L->cur, "HAS_CLASSES") ||
      kw(&L->cur, "ALLCLASSES") || kw(&L->cur, "CLASSESOK") ||
      kw(&L->cur, "NEEDCLASSES") || kw(&L->cur, "NEED_CLASSES") ||
      kw(&L->cur, "REQUIRECLASSES") || kw(&L->cur, "MUSTCLASSES")) {
    int hard = kw(&L->cur, "NEEDCLASSES") || kw(&L->cur, "NEED_CLASSES") ||
               kw(&L->cur, "REQUIRECLASSES") || kw(&L->cur, "MUSTCLASSES");
    char names[32][48];
    char miss[1024];
    int nname = 0, nmiss = 0, i, aln = L->cur.line;
    size_t mo = 0;
    lex_next(L);
    miss[0] = 0;
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
           kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") || kw(&L->cur, "HASCLASS") ||
           kw(&L->cur, "LISTCLASSES") || kw(&L->cur, "HASFORM") ||
           oop_stmt_kw(L)))
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
        } else if (vv && vv->is_str && vv->sval[0] && !oop_find_class(vm, L->cur.text)) {
          snprintf(names[nname++], sizeof names[0], "%s", vv->sval);
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(names[nname++], sizeof names[0], "%s", vm->last_str);
          lex_next(L);
        } else {
          snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
          lex_next(L);
        }
      }
    }
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDCLASSES nameâ¦ â NEEDCLASSES Ticket Cell Â· HASCLASSES bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASCLASSES_N", 0);
      var_set_num(vm, "CLASSMISS_N", 0);
      var_set_str(vm, "CLASSMISS", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASCLASSES: need names â HASCLASSES Ticket Cell");
      var_set_str(vm, "ERR", "HASCLASSES: need names â HASCLASSES Ticket Cell");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      if (oop_find_class(vm, names[i]))
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
        char em[192];
        snprintf(em, sizeof em,
                 "NEEDCLASSES miss line %d: '%s' (+%d more?) â CLASS/INCLUDE Â· HASCLASS Â· LISTCLASSES",
                 aln, names[i], nname - i - 1);
        fail_at(vm, L, em);
        return -1;
      }
    }
    var_set_str(vm, "CLASSMISS", miss);
    var_set_num(vm, "CLASSMISS_N", nmiss);
    var_set_num(vm, "HASCLASSES_N", nname - nmiss);
    var_set_num(vm, "NEEDCLASSES_N", nname);
    var_set_num(vm, "CLASSES_N", nname);
    if (nmiss == 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else {
      char em[200];
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      snprintf(em, sizeof em,
               "HASCLASSES miss (%d/%d): %s â CLASS/INCLUDE Â· NEEDCLASSES Â· LISTCLASSES",
               nmiss, nname, miss);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%d miss=%d\n",
              hard ? "NEEDCLASSES" : "HASCLASSES", nname, nmiss);
    bump(vm);
    return 1;
  }

  /* HASCLASSANY|NEEDCLASSANY nameâ¦ â multi CLASS any-of gate (HASCLASSES twin).
   * Soft: LAST_N 1 if any CLASS defined Â· CLASSHAVE bag Â· CLASSMISS bag.
   * NEEDCLASSANY fail-fast if none of the listed classes exist.
   * Usability: optional plugin class OR without NÃ HASCLASS IF glue. */
  if (kw(&L->cur, "HASCLASSANY") || kw(&L->cur, "HAS_CLASS_ANY") ||
      kw(&L->cur, "ANYCLASS") || kw(&L->cur, "CLASSANY") ||
      kw(&L->cur, "HASANYCLASS") || kw(&L->cur, "NEEDCLASSANY") ||
      kw(&L->cur, "NEED_CLASS_ANY") || kw(&L->cur, "REQUIRECLASSANY") ||
      kw(&L->cur, "MUSTCLASSANY")) {
    int hard = kw(&L->cur, "NEEDCLASSANY") || kw(&L->cur, "NEED_CLASS_ANY") ||
               kw(&L->cur, "REQUIRECLASSANY") || kw(&L->cur, "MUSTCLASSANY");
    char names[32][48];
    char miss[1024], havebag[1024];
    int nname = 0, nmiss = 0, nhave = 0, i, aln = L->cur.line;
    size_t mo = 0, ho = 0;
    lex_next(L);
    miss[0] = 0;
    havebag[0] = 0;
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
           kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") || kw(&L->cur, "HASCLASS") ||
           kw(&L->cur, "HASCLASSES") || kw(&L->cur, "LISTCLASSES") ||
           kw(&L->cur, "HASFORM") || oop_stmt_kw(L)))
        break;
      if (L->cur.kind == TK_STR) {
        const char *s = L->cur.text;
        if (strchr(s, '\n') || strchr(s, ',') || strchr(s, ' ')) {
          const char *pp = s;
          while (*pp && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*pp == ' ' || *pp == '\t' || *pp == '\n' || *pp == ',' || *pp == ':')
              pp++;
            if (!*pp) break;
            while (pp[tl] && pp[tl] != '\n' && pp[tl] != ',' && pp[tl] != ' ' &&
                   pp[tl] != '\t' && pp[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = pp[tl];
              tl++;
            }
            tok[tl] = 0;
            pp += tl;
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
          const char *pp = vv->sval;
          while (*pp && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*pp == ' ' || *pp == '\t' || *pp == '\n' || *pp == ',' || *pp == ':')
              pp++;
            if (!*pp) break;
            while (pp[tl] && pp[tl] != '\n' && pp[tl] != ',' && pp[tl] != ' ' &&
                   pp[tl] != '\t' && pp[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = pp[tl];
              tl++;
            }
            tok[tl] = 0;
            pp += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
        } else if (vv && vv->is_str && vv->sval[0] && !oop_find_class(vm, L->cur.text)) {
          snprintf(names[nname++], sizeof names[0], "%s", vv->sval);
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(names[nname++], sizeof names[0], "%s", vm->last_str);
          lex_next(L);
        } else {
          snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
          lex_next(L);
        }
      }
    }
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDCLASSANY nameâ¦ â NEEDCLASSANY Ticket Cell Â· HASCLASSANY bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASCLASSANY_N", 0);
      var_set_num(vm, "CLASSMISS_N", 0);
      var_set_num(vm, "CLASSHAVE_N", 0);
      var_set_str(vm, "CLASSMISS", "");
      var_set_str(vm, "CLASSHAVE", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASCLASSANY: need names â HASCLASSANY Ticket Ghost");
      var_set_str(vm, "ERR", "HASCLASSANY: need names â HASCLASSANY Ticket Ghost");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      if (oop_find_class(vm, names[i])) {
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
    var_set_str(vm, "CLASSMISS", miss);
    var_set_str(vm, "CLASSHAVE", havebag);
    var_set_num(vm, "CLASSMISS_N", nmiss);
    var_set_num(vm, "CLASSHAVE_N", nhave);
    var_set_num(vm, "HASCLASSANY_N", nhave);
    var_set_num(vm, "NEEDCLASSANY_N", nname);
    var_set_num(vm, "CLASSES_ANY_N", nname);
    if (nhave > 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else if (hard) {
      char em[240];
      snprintf(em, sizeof em,
               "NEEDCLASSANY miss line %d: need one of [%s] â CLASS/INCLUDE Â· HASCLASSANY Â· LISTCLASSES",
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
               "HASCLASSANY miss (0/%d): need one of %s â NEEDCLASSANY Â· INCLUDE",
               nname, miss[0] ? miss : "?");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%d have=%d miss=%d\n",
              hard ? "NEEDCLASSANY" : "HASCLASSANY", nname, nhave, nmiss);
    bump(vm);
    return 1;
  }

/* HASOBJ|LIVES|ISOBJ|ALIVEOBJ name â soft probe if live object exists.
   * LAST_N 1|0. Usability: IF before SEND/GETF without fatal. */
  if (kw(&L->cur, "HASOBJ") || kw(&L->cur, "LIVES") || kw(&L->cur, "ISOBJ") ||
      kw(&L->cur, "ALIVEOBJ") || kw(&L->cur, "OBJ?") || kw(&L->cur, "HASOBJECT") ||
      kw(&L->cur, "OBJEXISTS") || kw(&L->cur, "EXISTS_OBJ")) {
    char oname[48];
    ObjInst *ob;
    int hit = 0;
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "HASOBJ name"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    hit = ob ? 1 : 0;
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASOBJ_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASOBJS|NEEDOBJS nameâ¦ â multi live-object capability gate (all-or-OBJMISS).
   * Soft: LAST_N 0|1 Â· OBJMISS bag Â· NEEDOBJS fail-fast first miss.
   * Usability: agent live_session contracts without NÃ HASOBJ glue Â· bag/CSV/idents.
   * Twin of HASCLASSES for live instances Â· dual of obj_guard ALL path. */
  if (kw(&L->cur, "HASOBJS") || kw(&L->cur, "HAS_OBJS") ||
      kw(&L->cur, "ALLOBJS") || kw(&L->cur, "OBJSOK") ||
      kw(&L->cur, "NEEDOBJS") || kw(&L->cur, "NEED_OBJS") ||
      kw(&L->cur, "REQUIREOBJS") || kw(&L->cur, "MUSTOBJS")) {
    int hard = kw(&L->cur, "NEEDOBJS") || kw(&L->cur, "NEED_OBJS") ||
               kw(&L->cur, "REQUIREOBJS") || kw(&L->cur, "MUSTOBJS");
    char names[32][48];
    char miss[1024];
    int nname = 0, nmiss = 0, i, aln = L->cur.line;
    size_t mo = 0;
    lex_next(L);
    miss[0] = 0;
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
           kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") || kw(&L->cur, "HASOBJ") ||
           kw(&L->cur, "HASOBJS") || kw(&L->cur, "NEEDOBJS") ||
           kw(&L->cur, "HASOBJANY") || kw(&L->cur, "NEEDOBJANY") ||
           kw(&L->cur, "LISTOBJS") || kw(&L->cur, "HASCLASS") ||
           kw(&L->cur, "HASFORM") || kw(&L->cur, "SEND") || kw(&L->cur, "GETF") ||
           kw(&L->cur, "SETF") || kw(&L->cur, "DELETEOBJ") ||
           kw(&L->cur, "TRYDELETE")))
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
        } else if (vv && vv->is_str && vv->sval[0] && !oop_find_obj(vm, L->cur.text)) {
          /* expand string-var object name when ident is not a live obj */
          snprintf(names[nname++], sizeof names[0], "%s", vv->sval);
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(names[nname++], sizeof names[0], "%s", vm->last_str);
          lex_next(L);
        } else {
          snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
          lex_next(L);
        }
      }
    }
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDOBJS nameâ¦ â NEEDOBJS a b Â· HASOBJS bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASOBJS_N", 0);
      var_set_num(vm, "OBJMISS_N", 0);
      var_set_str(vm, "OBJMISS", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASOBJS: need names â HASOBJS a b");
      var_set_str(vm, "ERR", "HASOBJS: need names â HASOBJS a b");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      if (oop_find_obj(vm, names[i]))
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
                 "NEEDOBJS miss line %d: '%s' (+%d more?) â NEW/ENSURENEW Â· HASOBJ Â· LISTOBJS",
                 aln, names[i], nname - i - 1);
        fail_at(vm, L, em);
        return -1;
      }
    }
    var_set_str(vm, "OBJMISS", miss);
    var_set_num(vm, "OBJMISS_N", nmiss);
    var_set_num(vm, "HASOBJS_N", nname - nmiss);
    var_set_num(vm, "NEEDOBJS_N", nname);
    var_set_num(vm, "OBJS_N", nname);
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
               "HASOBJS miss (%d/%d): %s â NEW/ENSURENEW Â· NEEDOBJS Â· LISTOBJS",
               nmiss, nname, miss);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%d miss=%d\n",
              hard ? "NEEDOBJS" : "HASOBJS", nname, nmiss);
    bump(vm);
    return 1;
  }

  /* HASOBJANY|NEEDOBJANY nameâ¦ â multi live-object any-of gate (HASOBJS twin).
   * Soft: LAST_N 1 if any live Â· OBJHAVE bag Â· OBJMISS bag.
   * NEEDOBJANY fail-fast if none of the listed objects exist.
   * Usability: optional peer/session OR without NÃ HASOBJ IF glue. */
  if (kw(&L->cur, "HASOBJANY") || kw(&L->cur, "HAS_OBJ_ANY") ||
      kw(&L->cur, "OBJANY") || kw(&L->cur, "HASANYOBJ") ||
      kw(&L->cur, "LIVESANY") || kw(&L->cur, "NEEDOBJANY") ||
      kw(&L->cur, "NEED_OBJ_ANY") || kw(&L->cur, "REQUIREOBJANY") ||
      kw(&L->cur, "MUSTOBJANY")) {
    int hard = kw(&L->cur, "NEEDOBJANY") || kw(&L->cur, "NEED_OBJ_ANY") ||
               kw(&L->cur, "REQUIREOBJANY") || kw(&L->cur, "MUSTOBJANY");
    char names[32][48];
    char miss[1024], havebag[1024];
    int nname = 0, nmiss = 0, nhave = 0, i, aln = L->cur.line;
    size_t mo = 0, ho = 0;
    lex_next(L);
    miss[0] = 0;
    havebag[0] = 0;
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
           kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") || kw(&L->cur, "HASOBJ") ||
           kw(&L->cur, "HASOBJS") || kw(&L->cur, "NEEDOBJS") ||
           kw(&L->cur, "HASOBJANY") || kw(&L->cur, "NEEDOBJANY") ||
           kw(&L->cur, "LISTOBJS") || kw(&L->cur, "HASCLASS") ||
           kw(&L->cur, "HASFORM") || kw(&L->cur, "SEND") || kw(&L->cur, "GETF") ||
           kw(&L->cur, "SETF") || kw(&L->cur, "DELETEOBJ") ||
           kw(&L->cur, "TRYDELETE")))
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
        } else if (vv && vv->is_str && vv->sval[0] && !oop_find_obj(vm, L->cur.text)) {
          snprintf(names[nname++], sizeof names[0], "%s", vv->sval);
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(names[nname++], sizeof names[0], "%s", vm->last_str);
          lex_next(L);
        } else {
          snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
          lex_next(L);
        }
      }
    }
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDOBJANY nameâ¦ â NEEDOBJANY a b Â· HASOBJANY bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASOBJANY_N", 0);
      var_set_num(vm, "OBJMISS_N", 0);
      var_set_num(vm, "OBJHAVE_N", 0);
      var_set_str(vm, "OBJMISS", "");
      var_set_str(vm, "OBJHAVE", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASOBJANY: need names â HASOBJANY a b");
      var_set_str(vm, "ERR", "HASOBJANY: need names â HASOBJANY a b");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      if (oop_find_obj(vm, names[i])) {
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
    var_set_str(vm, "OBJMISS", miss);
    var_set_str(vm, "OBJHAVE", havebag);
    var_set_num(vm, "OBJMISS_N", nmiss);
    var_set_num(vm, "OBJHAVE_N", nhave);
    var_set_num(vm, "HASOBJANY_N", nhave);
    var_set_num(vm, "NEEDOBJANY_N", nname);
    var_set_num(vm, "OBJS_ANY_N", nname);
    if (nhave > 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else if (hard) {
      char em[240];
      snprintf(em, sizeof em,
               "NEEDOBJANY miss line %d: need one of [%s] â NEW/ENSURENEW Â· HASOBJANY Â· LISTOBJS",
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
               "HASOBJANY miss (0/%d): need one of %s â NEEDOBJANY Â· NEW/ENSURENEW",
               nname, miss[0] ? miss : "?");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%d have=%d miss=%d\n",
              hard ? "NEEDOBJANY" : "HASOBJANY", nname, nhave, nmiss);
    bump(vm);
    return 1;
  }

  /* HASFN|HASFUNC name â soft 0|1 probe before CALL.
   * Name may be IDENT, "string", or string-var. Usability: IF without fatal. */
  if (kw(&L->cur, "HASFN") || kw(&L->cur, "HASFUNC") ||
      kw(&L->cur, "HASFUNCTION") || kw(&L->cur, "FN?") ||
      kw(&L->cur, "FUNC?") || kw(&L->cur, "FNEXISTS") ||
      kw(&L->cur, "FUNCEXISTS") || kw(&L->cur, "DEFINEDFN") ||
      kw(&L->cur, "ISFN") || kw(&L->cur, "CANCALLFN")) {
    char fname[48];
    int i, hit = 0;
    lex_next(L);
    /* optional FN|FUNC keyword before name (not NAME â conflicts with var id) */
    if (kw(&L->cur, "FN") || kw(&L->cur, "FUNC") || kw(&L->cur, "FUNCTION") ||
        kw(&L->cur, "OF"))
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
      fail(vm, "HASFN name"); return -1;
    }
    for (i = 0; i < vm->n_fns; i++) {
      if (strcmp(vm->fns[i].name, fname) == 0) {
        hit = 1;
        break;
      }
    }
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASFN_N", hit);
    var_set_str(vm, "FN", fname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }
  /* HASFNS|NEEDFNS nameâ¦ â multi FN capability gate (all-or-FNMISS).
   * Soft: LAST_N 0|1 Â· FNMISS bag Â· NEEDFNS fail-fast first miss.
   * Usability: agent CALL contracts without NÃ HASFN glue Â· bag/CSV/idents.
   * Twin of HASOBJS for FN plane Â· dual of single HASFN. */
  if (kw(&L->cur, "HASFNS") || kw(&L->cur, "HAS_FNS") ||
      kw(&L->cur, "ALLFNS") || kw(&L->cur, "FNSOK") ||
      kw(&L->cur, "HASFUNCS") || kw(&L->cur, "ALLFUNCS") ||
      kw(&L->cur, "NEEDFNS") || kw(&L->cur, "NEED_FNS") ||
      kw(&L->cur, "REQUIREFNS") || kw(&L->cur, "MUSTFNS") ||
      kw(&L->cur, "NEEDFUNCS") || kw(&L->cur, "REQUIREFUNCS")) {
    int hard = kw(&L->cur, "NEEDFNS") || kw(&L->cur, "NEED_FNS") ||
               kw(&L->cur, "REQUIREFNS") || kw(&L->cur, "MUSTFNS") ||
               kw(&L->cur, "NEEDFUNCS") || kw(&L->cur, "REQUIREFUNCS");
    char names[32][48];
    char miss[1024];
    int nname = 0, nmiss = 0, i, j, aln = L->cur.line;
    size_t mo = 0;
    lex_next(L);
    miss[0] = 0;
    if (kw(&L->cur, "FN") || kw(&L->cur, "FUNC") || kw(&L->cur, "FUNCTION") ||
        kw(&L->cur, "OF"))
      lex_next(L);
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
           kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") || kw(&L->cur, "FN") ||
           kw(&L->cur, "HASFN") || kw(&L->cur, "HASFNS") || kw(&L->cur, "NEEDFNS") ||
           kw(&L->cur, "HASFNANY") || kw(&L->cur, "NEEDFNANY") ||
           kw(&L->cur, "LISTFNS") || kw(&L->cur, "CALL") || kw(&L->cur, "FNINFO") ||
           kw(&L->cur, "SEND") || kw(&L->cur, "GETF") || kw(&L->cur, "SETF")))
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
        } else if (vv && vv->is_str && vv->sval[0]) {
          int already = 0;
          for (j = 0; j < vm->n_fns; j++) {
            if (strcmp(vm->fns[j].name, L->cur.text) == 0) {
              already = 1;
              break;
            }
          }
          if (!already) {
            snprintf(names[nname++], sizeof names[0], "%s", vv->sval);
            lex_next(L);
          } else {
            snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
            lex_next(L);
          }
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(names[nname++], sizeof names[0], "%s", vm->last_str);
          lex_next(L);
        } else {
          snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
          lex_next(L);
        }
      }
    }
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDFNS nameâ¦ â NEEDFNS add greet Â· HASFNS bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASFNS_N", 0);
      var_set_num(vm, "FNMISS_N", 0);
      var_set_str(vm, "FNMISS", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASFNS: need names â HASFNS add greet");
      var_set_str(vm, "ERR", "HASFNS: need names â HASFNS add greet");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      int hit = 0;
      for (j = 0; j < vm->n_fns; j++) {
        if (strcmp(vm->fns[j].name, names[i]) == 0) {
          hit = 1;
          break;
        }
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
                 "NEEDFNS miss line %d: '%s' (+%d more?) â DEFINE/INCLUDE Â· HASFN Â· LISTFNS",
                 aln, names[i], nname - i - 1);
        fail_at(vm, L, em);
        return -1;
      }
    }
    var_set_str(vm, "FNMISS", miss);
    var_set_num(vm, "FNMISS_N", nmiss);
    var_set_num(vm, "HASFNS_N", nname - nmiss);
    var_set_num(vm, "NEEDFNS_N", nname);
    var_set_num(vm, "FNS_N", nname);
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
               "HASFNS miss (%d/%d): %s â DEFINE/INCLUDE Â· NEEDFNS Â· LISTFNS",
               nmiss, nname, miss);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%d miss=%d\n",
              hard ? "NEEDFNS" : "HASFNS", nname, nmiss);
    bump(vm);
    return 1;
  }

  /* HASFNANY|NEEDFNANY nameâ¦ â multi FN any-of gate (HASFNS twin).
   * Soft: LAST_N 1 if any listed FN defined Â· FNHAVE bag Â· FNMISS bag.
   * NEEDFNANY fail-fast if none of the listed FNs exist.
   * Usability: optional CALL plugin FN OR without NÃ HASFN IF glue. */
  if (kw(&L->cur, "HASFNANY") || kw(&L->cur, "HAS_FN_ANY") ||
      kw(&L->cur, "ANYFN") || kw(&L->cur, "FNANY") ||
      kw(&L->cur, "HASANYFN") || kw(&L->cur, "ANYFUNC") ||
      kw(&L->cur, "NEEDFNANY") || kw(&L->cur, "NEED_FN_ANY") ||
      kw(&L->cur, "REQUIREFNANY") || kw(&L->cur, "MUSTFNANY") ||
      kw(&L->cur, "NEEDFUNCANY") || kw(&L->cur, "REQUIREFUNCANY")) {
    int hard = kw(&L->cur, "NEEDFNANY") || kw(&L->cur, "NEED_FN_ANY") ||
               kw(&L->cur, "REQUIREFNANY") || kw(&L->cur, "MUSTFNANY") ||
               kw(&L->cur, "NEEDFUNCANY") || kw(&L->cur, "REQUIREFUNCANY");
    char names[32][48];
    char miss[1024], havebag[1024];
    int nname = 0, nmiss = 0, nhave = 0, i, j, aln = L->cur.line;
    size_t mo = 0, ho = 0;
    lex_next(L);
    miss[0] = 0;
    havebag[0] = 0;
    if (kw(&L->cur, "FN") || kw(&L->cur, "FUNC") || kw(&L->cur, "FUNCTION") ||
        kw(&L->cur, "OF"))
      lex_next(L);
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
           kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") || kw(&L->cur, "FN") ||
           kw(&L->cur, "HASFN") || kw(&L->cur, "HASFNS") || kw(&L->cur, "NEEDFNS") ||
           kw(&L->cur, "HASFNANY") || kw(&L->cur, "NEEDFNANY") ||
           kw(&L->cur, "LISTFNS") || kw(&L->cur, "CALL") || kw(&L->cur, "FNINFO") ||
           kw(&L->cur, "SEND") || kw(&L->cur, "GETF") || kw(&L->cur, "SETF")))
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
        } else if (vv && vv->is_str && vv->sval[0]) {
          int already = 0;
          for (j = 0; j < vm->n_fns; j++) {
            if (strcmp(vm->fns[j].name, L->cur.text) == 0) {
              already = 1;
              break;
            }
          }
          if (!already) {
            snprintf(names[nname++], sizeof names[0], "%s", vv->sval);
            lex_next(L);
          } else {
            snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
            lex_next(L);
          }
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(names[nname++], sizeof names[0], "%s", vm->last_str);
          lex_next(L);
        } else {
          snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
          lex_next(L);
        }
      }
    }
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDFNANY nameâ¦ â NEEDFNANY add ghost Â· HASFNANY bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASFNANY_N", 0);
      var_set_num(vm, "FNMISS_N", 0);
      var_set_num(vm, "FNHAVE_N", 0);
      var_set_str(vm, "FNMISS", "");
      var_set_str(vm, "FNHAVE", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASFNANY: need names â HASFNANY add ghost");
      var_set_str(vm, "ERR", "HASFNANY: need names â HASFNANY add ghost");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      int hit = 0;
      for (j = 0; j < vm->n_fns; j++) {
        if (strcmp(vm->fns[j].name, names[i]) == 0) {
          hit = 1;
          break;
        }
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
    var_set_str(vm, "FNMISS", miss);
    var_set_str(vm, "FNHAVE", havebag);
    var_set_num(vm, "FNMISS_N", nmiss);
    var_set_num(vm, "FNHAVE_N", nhave);
    var_set_num(vm, "HASFNANY_N", nhave);
    var_set_num(vm, "NEEDFNANY_N", nname);
    var_set_num(vm, "FNS_ANY_N", nname);
    if (nhave > 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else if (hard) {
      char em[240];
      snprintf(em, sizeof em,
               "NEEDFNANY miss line %d: need one of [%s] â DEFINE/INCLUDE Â· HASFNANY Â· LISTFNS",
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
               "HASFNANY miss (0/%d): need one of %s â NEEDFNANY Â· DEFINE",
               nname, miss[0] ? miss : "?");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%d have=%d miss=%d\n",
              hard ? "NEEDFNANY" : "HASFNANY", nname, nhave, nmiss);
    bump(vm);
    return 1;
  }

  return 0;
}
