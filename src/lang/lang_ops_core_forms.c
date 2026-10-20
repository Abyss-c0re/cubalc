/* CubalC lang — lang_ops_core_forms.c
 * HASFORM · FORMHINT · LISTFORMS · HELP (mesh vote core carve).
 */
#include "lang/cubalc_lang_ops_core_forms.h"
#include "lang/cubalc_lang_ops_core_help.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_core_try_forms(VM *vm, Lex *L) {
  if (kw(&L->cur,"HASFORM") || kw(&L->cur,"HAS_FORM") ||
      kw(&L->cur,"FORMEXISTS") || kw(&L->cur,"KNOWNFORM") ||
      kw(&L->cur,"NEEDFORM") || kw(&L->cur,"NEED_FORM") ||
      kw(&L->cur,"REQUIREFORM") || kw(&L->cur,"MUSTFORM")) {
    int hard = kw(&L->cur,"NEEDFORM") || kw(&L->cur,"NEED_FORM") ||
               kw(&L->cur,"REQUIREFORM") || kw(&L->cur,"MUSTFORM");
    char name[96];
    const char *hint = NULL;
    int hit = 0, aln = L->cur.line;
    lex_next(L);
    name[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(name, sizeof name, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(name, sizeof name, "%s", vm->last_str);
      else
        snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    }
    if (!name[0]) {
      if (hard) {
        fail_at(vm, L, "NEEDFORM name — NEEDFORM SORTLIBS · HASFORM LIBAGE");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASFORM_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASFORM: need name — HASFORM SORTLIBS");
      var_set_str(vm, "ERR", "HASFORM: need name — HASFORM SORTLIBS");
      bump(vm);
      return 1;
    }
    hit = cubalc_form_known(name, &hint) ? 1 : 0;
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASFORM_N", hit);
    var_set_str(vm, "FORM", name);
    if (hit && hint && hint[0])
      var_set_str(vm, "FORM_HINT", hint);
    if (hit) {
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    {
      char em[176];
      snprintf(em, sizeof em,
               "%s miss: '%s' — cubalc forms · HELP · REQUIRE FORM · upgrade runtime",
               hard ? "NEEDFORM" : "HASFORM", name);
      if (hard) {
        fail_at(vm, L, em);
        (void)aln;
        return -1;
      }
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
  }

  /* FORMHINT|DESCRIBEFORM name — peel HELP catalog one-line hint → LAST.
   * Usability: agent docs without HELP multi-match soup or shell cubalc forms.
   * Soft miss OK=0 · FORM_HINT/FORM set · dual of HASFORM with payload. */
  if (kw(&L->cur,"FORMHINT") || kw(&L->cur,"DESCRIBEFORM") ||
      kw(&L->cur,"FORMDOC") || kw(&L->cur,"FORMHELP") ||
      kw(&L->cur,"HINTFORM") || kw(&L->cur,"FORM_HINT") ||
      kw(&L->cur,"WHATISFORM") || kw(&L->cur,"EXPLAINFORM")) {
    char name[96];
    const char *hint = NULL;
    lex_next(L);
    name[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(name, sizeof name, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(name, sizeof name, "%s", vm->last_str);
      else
        snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    }
    if (!name[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "FORMHINT: need name — FORMHINT SORTLIBS · DESCRIBEFORM LIBAGE");
      var_set_str(vm, "ERR",
                  "FORMHINT: need name — FORMHINT SORTLIBS · DESCRIBEFORM LIBAGE");
      bump(vm);
      return 1;
    }
    if (!cubalc_form_known(name, &hint) || !hint || !hint[0]) {
      char em[176];
      snprintf(em, sizeof em,
               "FORMHINT miss: '%s' — LISTFORMS · HASFORM · cubalc forms · HELP",
               name);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "FORM", name);
      var_set_str(vm, "FORM_HINT", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", hint);
    var_set_str(vm, "FORM_HINT", hint);
    var_set_str(vm, "FORMHINT", hint);
    var_set_str(vm, "FORM", name);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hint);
    vm->last_n = (long)strlen(hint);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASFORMS|NEEDFORMS name… — multi form capability gate (all-or-FORMMISS).
   * Usability: agent boot contracts without N× HASFORM glue · bag/CSV/idents.
   * Soft: LAST_N 0|1 · FORMMISS bag · NEEDFORMS fail-fast first miss. */
  if (kw(&L->cur,"HASFORMS") || kw(&L->cur,"HAS_FORMS") ||
      kw(&L->cur,"ALLFORMS") || kw(&L->cur,"FORMSOK") ||
      kw(&L->cur,"NEEDFORMS") || kw(&L->cur,"NEED_FORMS") ||
      kw(&L->cur,"REQUIREFORMS") || kw(&L->cur,"MUSTFORMS")) {
    int hard = kw(&L->cur,"NEEDFORMS") || kw(&L->cur,"NEED_FORMS") ||
               kw(&L->cur,"REQUIREFORMS") || kw(&L->cur,"MUSTFORMS");
    char names[32][96];
    char miss[1024];
    int nname = 0, nmiss = 0, i, aln = L->cur.line;
    size_t mo = 0;
    lex_next(L);
    miss[0] = 0;
    /* gather idents/strings until statement boundary; one str may be bag/CSV */
    while (nname < 32 &&
           (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
      /* Stop only on control/statement heads — form names themselves (HASFORM…)
       * are valid capability tokens and must not cut the list short. */
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
           kw(&L->cur,"REQUIRE") || kw(&L->cur,"END") ||
           kw(&L->cur,"IF") || kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") ||
           kw(&L->cur,"PASS") || kw(&L->cur,"FAIL") || kw(&L->cur,"INCLUDE") ||
           kw(&L->cur,"SYS") || kw(&L->cur,"HELP") || kw(&L->cur,"CLEAR_ERR") ||
           kw(&L->cur,"NOTE") || kw(&L->cur,"EXIT") || kw(&L->cur,"DEFAULT") ||
           kw(&L->cur,"VERSION") || kw(&L->cur,"STATUS") || kw(&L->cur,"FOR") ||
           kw(&L->cur,"WHILE") || kw(&L->cur,"LOOP") || kw(&L->cur,"EACH") ||
           kw(&L->cur,"CUBE") || kw(&L->cur,"PLUG") || kw(&L->cur,"HOLD_FLASH")))
        break;
      if (L->cur.kind == TK_STR) {
        const char *s = L->cur.text;
        /* split bag on newline/comma/space if multi */
        if (strchr(s, '\n') || strchr(s, ',') || strchr(s, ' ')) {
          const char *p = s;
          while (*p && nname < 32) {
            char tok[96];
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
          /* expand bag var as multiple names */
          const char *p = vv->sval;
          while (*p && nname < 32) {
            char tok[96];
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
        fail_at(vm, L, "NEEDFORMS name… — NEEDFORMS SORTLIBS LIBAGE · HASFORMS bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASFORMS_N", 0);
      var_set_num(vm, "FORMMISS_N", 0);
      var_set_str(vm, "FORMMISS", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASFORMS: need names — HASFORMS SORTLIBS LIBAGE");
      var_set_str(vm, "ERR", "HASFORMS: need names — HASFORMS SORTLIBS LIBAGE");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      if (cubalc_form_known(names[i], NULL))
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
                 "NEEDFORMS miss line %d: '%s' (+%d more?) — cubalc forms · HASFORM · upgrade",
                 aln, names[i], nname - i - 1);
        fail_at(vm, L, em);
        return -1;
      }
    }
    var_set_str(vm, "FORMMISS", miss);
    var_set_num(vm, "FORMMISS_N", nmiss);
    var_set_num(vm, "HASFORMS_N", nname - nmiss);
    var_set_num(vm, "NEEDFORMS_N", nname);
    var_set_num(vm, "FORMS_N", nname);
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
               "HASFORMS miss (%d/%d): %s — cubalc forms · NEEDFORMS · upgrade runtime",
               nmiss, nname, miss);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    bump(vm);
    return 1;
  }

  /* HASFORMANY|NEEDFORMANY name… — multi form any-of capability gate (HASFORMS twin).
   * Soft: LAST_N 1 if any known · FORMHAVE bag · FORMMISS of unknowns.
   * Hard NEEDFORMANY fail-fast if none of the listed forms exist.
   * Usability: need SORTLIBS OR WALK OR PATHGLOB without N× HASFORM + IF glue. */
  if (kw(&L->cur,"HASFORMANY") || kw(&L->cur,"HAS_FORM_ANY") ||
      kw(&L->cur,"HASANYFORMS") || kw(&L->cur,"ANYFORMS_OK") ||
      kw(&L->cur,"FORMANY?") || kw(&L->cur,"NEEDFORMANY") ||
      kw(&L->cur,"NEED_FORM_ANY") || kw(&L->cur,"REQUIREFORMANY") ||
      kw(&L->cur,"MUSTFORMANY") || kw(&L->cur,"REQUIRE_FORM_ANY") ||
      kw(&L->cur,"NEEDANYFORMS") || kw(&L->cur,"REQUIREANYFORMS")) {
    int hard = kw(&L->cur,"NEEDFORMANY") || kw(&L->cur,"NEED_FORM_ANY") ||
               kw(&L->cur,"REQUIREFORMANY") || kw(&L->cur,"MUSTFORMANY") ||
               kw(&L->cur,"REQUIRE_FORM_ANY") || kw(&L->cur,"NEEDANYFORMS") ||
               kw(&L->cur,"REQUIREANYFORMS");
    char names[32][96];
    char miss[1024], havebag[1024];
    int nname = 0, nmiss = 0, nhave = 0, i, aln = L->cur.line;
    size_t mo = 0, ho = 0;
    lex_next(L);
    miss[0] = 0;
    havebag[0] = 0;
    while (nname < 32 &&
           (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
           kw(&L->cur,"REQUIRE") || kw(&L->cur,"END") ||
           kw(&L->cur,"IF") || kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") ||
           kw(&L->cur,"PASS") || kw(&L->cur,"FAIL") || kw(&L->cur,"INCLUDE") ||
           kw(&L->cur,"SYS") || kw(&L->cur,"HELP") || kw(&L->cur,"CLEAR_ERR") ||
           kw(&L->cur,"NOTE") || kw(&L->cur,"EXIT") || kw(&L->cur,"DEFAULT") ||
           kw(&L->cur,"VERSION") || kw(&L->cur,"STATUS") || kw(&L->cur,"FOR") ||
           kw(&L->cur,"WHILE") || kw(&L->cur,"LOOP") || kw(&L->cur,"EACH") ||
           kw(&L->cur,"CUBE") || kw(&L->cur,"PLUG") || kw(&L->cur,"HOLD_FLASH")))
        break;
      if (L->cur.kind == TK_STR) {
        const char *s = L->cur.text;
        if (strchr(s, '\n') || strchr(s, ',') || strchr(s, ' ')) {
          const char *p = s;
          while (*p && nname < 32) {
            char tok[96];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' || *p == ':' ||
                   *p == '|')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != ':' && p[tl] != '|' &&
                   tl + 1 < sizeof tok) {
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
             strchr(vv->sval, ' ') || strchr(vv->sval, '|'))) {
          const char *p = vv->sval;
          while (*p && nname < 32) {
            char tok[96];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' || *p == ':' ||
                   *p == '|')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != ':' && p[tl] != '|' &&
                   tl + 1 < sizeof tok) {
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
        fail_at(vm, L, "NEEDFORMANY name… — NEEDFORMANY SORTLIBS WALK · HASFORMANY bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASFORMANY_N", 0);
      var_set_num(vm, "FORMMISS_N", 0);
      var_set_num(vm, "NEEDFORMANY_N", 0);
      var_set_str(vm, "FORMMISS", "");
      var_set_str(vm, "FORMHAVE", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASFORMANY: need names — HASFORMANY SORTLIBS WALK PATHGLOB");
      var_set_str(vm, "ERR", "HASFORMANY: need names — HASFORMANY SORTLIBS WALK PATHGLOB");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      int hit = cubalc_form_known(names[i], NULL) ? 1 : 0;
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
    var_set_str(vm, "FORMMISS", miss);
    var_set_str(vm, "FORMHAVE", havebag);
    var_set_num(vm, "FORMMISS_N", nmiss);
    var_set_num(vm, "HASFORMANY_N", nhave);
    var_set_num(vm, "NEEDFORMANY_N", nname);
    var_set_num(vm, "FORMS_ANY_N", nname);
    if (nhave > 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else if (hard) {
      char em[240];
      snprintf(em, sizeof em,
               "NEEDFORMANY miss line %d: need one of [%s] — cubalc forms · HASFORMANY · upgrade",
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
               "HASFORMANY miss (0/%d): need one of %s — NEEDFORMANY · upgrade runtime",
               nname, miss[0] ? miss : "?");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# hasformany n=%d have=%d miss=%d hard=%d\n",
              nname, nhave, nmiss, hard);
    bump(vm);
    return 1;
  }


  /* LISTFORMS [prefix] — bag of HELP-catalog form names (dual of cubalc forms).
   * Usability: agents discover surface in-lang without shell · optional filter.
   * COUNTFORMS prefix → LAST_N only. Bare LISTFORMS = full catalog (capped bag).
   * LAST = newline names · LAST_N = count · soft empty OK=1. */
  if (kw(&L->cur,"LISTFORMS") || kw(&L->cur,"FORMSLIST") ||
      kw(&L->cur,"CATALOGFORMS") || kw(&L->cur,"FORMLIST") ||
      kw(&L->cur,"LIST_FORMS") || kw(&L->cur,"HELPLIST") ||
      kw(&L->cur,"COUNTFORMS") || kw(&L->cur,"NFORMS") ||
      kw(&L->cur,"FORMCOUNT") || kw(&L->cur,"COUNT_FORMS")) {
    int count_only = kw(&L->cur,"COUNTFORMS") || kw(&L->cur,"NFORMS") ||
                     kw(&L->cur,"FORMCOUNT") || kw(&L->cur,"COUNT_FORMS");
    char pref[96], pref_up[96], bag[8192];
    int nhelp = cubalc_help_catalog_n();
    int i, nmatch = 0;
    size_t o = 0, a;
    lex_next(L);
    pref[0] = 0;
    bag[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(pref, sizeof pref, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      /* Always lex_next after consuming a prefix token (var, LAST, or bare name).
       * Missing advance left LISTFORMS f → unknown form 'f' (realworld 298). */
      if (vv && vv->is_str && vv->sval[0]) {
        snprintf(pref, sizeof pref, "%s", vv->sval);
        lex_next(L);
      } else if (strcmp(L->cur.text, "LAST") == 0) {
        snprintf(pref, sizeof pref, "%s", vm->last_str);
        lex_next(L);
      } else if (!(kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
                 kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
                 kw(&L->cur,"ELIF") || kw(&L->cur,"PASS") || kw(&L->cur,"FAIL") ||
                 kw(&L->cur,"INCLUDE") || kw(&L->cur,"SYS") || kw(&L->cur,"HELP") ||
                 kw(&L->cur,"REQUIRE") || kw(&L->cur,"DEFAULT") ||
                 kw(&L->cur,"VERSION") || kw(&L->cur,"STATUS") ||
                 kw(&L->cur,"CLEAR_ERR") || kw(&L->cur,"NOTE") || kw(&L->cur,"EXIT") ||
                 kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") || kw(&L->cur,"LOOP") ||
                 kw(&L->cur,"EACH") || kw(&L->cur,"CUBE") || kw(&L->cur,"PLUG"))) {
        /* form names (HASFORM/SORTLIBS/…) are valid prefixes — do not stop on them */
        snprintf(pref, sizeof pref, "%s", L->cur.text);
        lex_next(L);
      }
    }
    for (a = 0; pref[a] && a + 1 < sizeof pref_up; a++) {
      char c = pref[a];
      if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
      pref_up[a] = c;
    }
    pref_up[a] = 0;
    for (i = 0; i < nhelp; i++) {
      char nm[96];
      size_t k;
      const char *hn = cubalc_help_catalog[i].name;
      int hit = 1;
      for (k = 0; hn[k] && k + 1 < sizeof nm; k++) {
        char c = hn[k];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        nm[k] = c;
      }
      nm[k] = 0;
      if (pref_up[0] && !strstr(nm, pref_up))
        hit = 0;
      if (!hit) continue;
      nmatch++;
      if (!count_only) {
        size_t ln = strlen(hn);
        if (o && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) {
          memcpy(bag + o, hn, ln);
          o += ln;
        }
        bag[o] = 0;
      }
    }
    var_set_num(vm, "LAST_N", nmatch);
    vm->last_n = nmatch;
    var_set_num(vm, "LISTFORMS_N", nmatch);
    var_set_num(vm, "COUNTFORMS_N", nmatch);
    var_set_num(vm, "FORMS_N", nmatch);
    var_set_str(vm, "LISTFORMS_FILTER", pref);
    var_set_str(vm, "FORMS_FILTER", pref);
    if (count_only) {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", nmatch);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    } else {
      var_set_str(vm, "LAST", bag);
      var_set_str(vm, "LISTFORMS", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HELP [prefix|form] — in-program form discovery for agents/humans
   * Sets LAST to first match "NAME: hint", LAST_N=len, OK=1 if found.
   * Bare HELP → catalog tip pointing at cubalc forms CLI. */
  if (kw(&L->cur,"HELP")||kw(&L->cur,"MAN")||kw(&L->cur,"DOC")){
    const CubalcHelpEnt *help = cubalc_help_catalog;
    int nhelp = cubalc_help_catalog_n();
    char q[64]; q[0]=0;
    lex_next(L);
    /* Topic may be a keyword (ASSERT, SMX, PLUG…) — always consume as help query. */
    if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
      snprintf(q, sizeof q, "%s", L->cur.text);
      lex_next(L);
    }
    /* uppercase query for match */
    {
      size_t i;
      for (i = 0; q[i]; i++)
        if (q[i] >= 'a' && q[i] <= 'z') q[i] = (char)(q[i] - 'a' + 'A');
    }
    char out[CUBALC_HOST_STR_MAX]; out[0]=0;
    int found = 0, hits = 0;
    if (!q[0]){
      snprintf(out, sizeof out,
               "HELP [form] · cubalc forms [prefix] · cookbook: docs/COOKBOOK.md · "
               "HOLD_FLASH device/mesh-join only · SMX binary mesh");
      found = 1; hits = nhelp;
    } else {
      size_t o = 0;
      int i;
      for (i = 0; i < nhelp; i++){
        char nm[48]; size_t k;
        for (k = 0; help[i].name[k] && k + 1 < sizeof nm; k++)
          nm[k] = (char)((help[i].name[k] >= 'a' && help[i].name[k] <= 'z')
                         ? help[i].name[k] - 'a' + 'A' : help[i].name[k]);
        nm[k] = 0;
        if (strstr(nm, q) || strstr(help[i].hint, q)){
          int n = snprintf(out + o, sizeof out - o, "%s%s: %s",
                           o ? " | " : "", help[i].name, help[i].hint);
          if (n > 0) o += (size_t)n;
          hits++; found = 1;
          if (o + 32 >= sizeof out) break;
        }
      }
      if (!found)
        snprintf(out, sizeof out, "no form match for '%s' — try cubalc forms %s", q, q);
    }
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = (long)strlen(out);
    var_set_str(vm, "LAST", out);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "OK", found ? 1 : 0);
    var_set_num(vm, "HELP_N", (long)hits);
    if (vm->trace) fprintf(vm->trace, "%s\n", out);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", out);
    bump(vm); return 1;
  }
  /* PRINT|SAY|ECHO [parts…] — space-join strings and numbers; LAST=line.
   * Usability: string vars print content (not strlen); multi-arg without CAT glue. */
  return 0;
}
