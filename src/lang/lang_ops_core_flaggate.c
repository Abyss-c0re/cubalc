/* CubalC lang — lang_ops_core_flaggate.c (B_E restore)
 * HASFLAGALL/NEEDFLAGS · HASFLAGANY/NEEDFLAGANY · HASARGALL/NEEDARGS · HASARGANY/NEEDARGANY
 * HASENVALL/NEEDENVS (host-env twin · B_E_science restore)
 * Restored from broken_unlinked. Supports F_main CLI flaggate peel. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_core_flaggate.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include "cubalc_law.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cubalc_lang_ops_core_try_flaggate(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* HASFLAGALL|NEEDFLAGS name… — multi named-flag presence gate (all-or-FLAGMISS).
   * Soft: LAST_N 0|1 · FLAGMISS bag of missing names · HASFLAGALL_N have count.
   * Hard NEEDFLAGS|REQUIREFLAGS fail-fast first miss (USAGE tip when set).
   * Usability: CLI tool contracts without N× HASFLAG glue · bag/CSV/idents.
   * Twin of HASFORMS/NEEDFORMS for --flags · not HASFLAGS (count-only probe). */
  if (kw(&L->cur,"HASFLAGALL") || kw(&L->cur,"HAS_FLAG_ALL") ||
      kw(&L->cur,"HASALLFLAGS") || kw(&L->cur,"ALLFLAGS_OK") ||
      kw(&L->cur,"FLAGALL?") || kw(&L->cur,"NEEDFLAGS") ||
      kw(&L->cur,"NEED_FLAGS") || kw(&L->cur,"REQUIREFLAGS") ||
      kw(&L->cur,"MUSTFLAGS") || kw(&L->cur,"REQUIRE_FLAGS")) {
    int hard = kw(&L->cur,"NEEDFLAGS") || kw(&L->cur,"NEED_FLAGS") ||
               kw(&L->cur,"REQUIREFLAGS") || kw(&L->cur,"MUSTFLAGS") ||
               kw(&L->cur,"REQUIRE_FLAGS");
    char names[32][96];
    char miss[1024], havebag[1024];
    int nname = 0, nmiss = 0, nhave = 0, i, aln = L->cur.line;
    size_t mo = 0, ho = 0;
    char val[CUBALC_HOST_STR_MAX];
    lex_next(L);
    miss[0] = 0;
    havebag[0] = 0;
    while (nname < 32) {
      /* allow --name / -name tokens (strip dashes like GETFLAG) */
      while (L->cur.kind == TK_MINUS) lex_next(L);
      if (L->cur.kind != TK_STR && L->cur.kind != TK_IDENT)
        break;
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
           kw(&L->cur,"REQUIRE") || kw(&L->cur,"END") ||
           kw(&L->cur,"IF") || kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") ||
           kw(&L->cur,"PASS") || kw(&L->cur,"FAIL") || kw(&L->cur,"INCLUDE") ||
           kw(&L->cur,"SYS") || kw(&L->cur,"HELP") || kw(&L->cur,"CLEAR_ERR") ||
           kw(&L->cur,"NOTE") || kw(&L->cur,"EXIT") || kw(&L->cur,"DEFAULT") ||
           kw(&L->cur,"VERSION") || kw(&L->cur,"STATUS") || kw(&L->cur,"FOR") ||
           kw(&L->cur,"WHILE") || kw(&L->cur,"LOOP") || kw(&L->cur,"EACH") ||
           kw(&L->cur,"USAGE") || kw(&L->cur,"HELPFLAG") || kw(&L->cur,"CUBE") ||
           kw(&L->cur,"PLUG") || kw(&L->cur,"HOLD_FLASH") ||
           kw(&L->cur,"HASFLAG") || kw(&L->cur,"HASFLAGS") ||
           kw(&L->cur,"HASFLAGALL") || kw(&L->cur,"NEEDFLAGS") ||
           kw(&L->cur,"HASFLAGANY") || kw(&L->cur,"NEEDFLAGANY") ||
           kw(&L->cur,"LISTFLAGS") || kw(&L->cur,"GETFLAG") ||
           kw(&L->cur,"FLAGMAP") || kw(&L->cur,"HASRESTARGS")))
        break;
      if (L->cur.kind == TK_STR) {
        const char *s = L->cur.text;
        if (strchr(s, '\n') || strchr(s, ',') || strchr(s, ' ') || strchr(s, '|')) {
          const char *p = s;
          while (*p && nname < 32) {
            char tok[96];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' ||
                   *p == '|' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != '|' && p[tl] != ':' &&
                   tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            while (tok[0] == '-')
              memmove(tok, tok + 1, strlen(tok));
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
          continue;
        }
        {
          char tok[96];
          snprintf(tok, sizeof tok, "%s", s);
          while (tok[0] == '-')
            memmove(tok, tok + 1, strlen(tok));
          if (tok[0])
            snprintf(names[nname++], sizeof names[0], "%s", tok);
        }
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
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' ||
                   *p == '|' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != '|' && p[tl] != ':' &&
                   tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            while (tok[0] == '-')
              memmove(tok, tok + 1, strlen(tok));
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
        } else if (vv && vv->is_str && vv->sval[0]) {
          char tok[96];
          snprintf(tok, sizeof tok, "%s", vv->sval);
          while (tok[0] == '-')
            memmove(tok, tok + 1, strlen(tok));
          if (tok[0])
            snprintf(names[nname++], sizeof names[0], "%s", tok);
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          char tok[96];
          snprintf(tok, sizeof tok, "%s", vm->last_str);
          while (tok[0] == '-')
            memmove(tok, tok + 1, strlen(tok));
          if (tok[0])
            snprintf(names[nname++], sizeof names[0], "%s", tok);
          lex_next(L);
        } else {
          char tok[96];
          snprintf(tok, sizeof tok, "%s", L->cur.text);
          while (tok[0] == '-')
            memmove(tok, tok + 1, strlen(tok));
          if (tok[0])
            snprintf(names[nname++], sizeof names[0], "%s", tok);
          lex_next(L);
        }
      }
    }
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDFLAGS name… — NEEDFLAGS verbose out · HASFLAGALL bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASFLAGALL_N", 0);
      var_set_num(vm, "FLAGMISS_N", 0);
      var_set_num(vm, "NEEDFLAGS_N", 0);
      var_set_str(vm, "FLAGMISS", "");
      var_set_str(vm, "FLAGHAVE", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASFLAGALL: need names — HASFLAGALL verbose out workers");
      var_set_str(vm, "ERR", "HASFLAGALL: need names — HASFLAGALL verbose out workers");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      int hit;
      val[0] = 0;
      hit = cubalc_scan_cli_flag(names[i], val, sizeof val);
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
        continue;
      }
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
        char em[280];
        Var *vu = var_get(vm, "USAGE", 0);
        const char *use = (vu && vu->is_str && vu->sval[0]) ? vu->sval : "";
        if (use[0])
          snprintf(em, sizeof em,
                   "NEEDFLAGS miss line %d: '%s' (+%d more?) — pass --%s · usage: %s",
                   aln, names[i], nname - i - 1, names[i], use);
        else
          snprintf(em, sizeof em,
                   "NEEDFLAGS miss line %d: '%s' (+%d more?) — pass --%s · HASFLAGALL · USAGE",
                   aln, names[i], nname - i - 1, names[i]);
        var_set_str(vm, "FLAGMISS", miss);
        var_set_num(vm, "FLAGMISS_N", nmiss);
        var_set_str(vm, "FLAGHAVE", havebag);
        var_set_num(vm, "HASFLAGALL_N", nhave);
        fail_at(vm, L, em);
        return -1;
      }
    }
    var_set_str(vm, "FLAGMISS", miss);
    var_set_str(vm, "FLAGHAVE", havebag);
    var_set_num(vm, "FLAGMISS_N", nmiss);
    var_set_num(vm, "HASFLAGALL_N", nhave);
    var_set_num(vm, "NEEDFLAGS_N", nname);
    var_set_num(vm, "FLAGS_NEED_N", nname);
    if (nmiss == 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else {
      char em[240];
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      snprintf(em, sizeof em,
               "HASFLAGALL miss (%d/%d): %s — pass --flags · NEEDFLAGS · USAGE",
               nmiss, nname, miss);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# hasflagall n=%d have=%d miss=%d hard=%d\n",
              nname, nhave, nmiss, hard);
    bump(vm);
    return 1;
  }
  /* HASFLAGANY|NEEDFLAGANY name… — multi named-flag any-of gate (HASFLAGALL twin).
   * Soft: LAST_N 1 if any present · FLAGHAVE bag of hits · FLAGMISS of absences.
   * Hard NEEDFLAGANY fail-fast if none of the names present (USAGE tip when set).
   * Usability: --json OR --yaml OR --xml without N× HASFLAG + IF glue. */
  if (kw(&L->cur,"HASFLAGANY") || kw(&L->cur,"HAS_FLAG_ANY") ||
      kw(&L->cur,"HASANYFLAGS") || kw(&L->cur,"ANYFLAGS_OK") ||
      kw(&L->cur,"FLAGANY?") || kw(&L->cur,"NEEDFLAGANY") ||
      kw(&L->cur,"NEED_FLAG_ANY") || kw(&L->cur,"REQUIREFLAGANY") ||
      kw(&L->cur,"MUSTFLAGANY") || kw(&L->cur,"REQUIRE_FLAG_ANY") ||
      kw(&L->cur,"NEEDANYFLAGS") || kw(&L->cur,"REQUIREANYFLAGS")) {
    int hard = kw(&L->cur,"NEEDFLAGANY") || kw(&L->cur,"NEED_FLAG_ANY") ||
               kw(&L->cur,"REQUIREFLAGANY") || kw(&L->cur,"MUSTFLAGANY") ||
               kw(&L->cur,"REQUIRE_FLAG_ANY") || kw(&L->cur,"NEEDANYFLAGS") ||
               kw(&L->cur,"REQUIREANYFLAGS");
    char names[32][96];
    char miss[1024], havebag[1024];
    int nname = 0, nmiss = 0, nhave = 0, i, aln = L->cur.line;
    size_t mo = 0, ho = 0;
    char val[CUBALC_HOST_STR_MAX];
    lex_next(L);
    miss[0] = 0;
    havebag[0] = 0;
    while (nname < 32) {
      while (L->cur.kind == TK_MINUS) lex_next(L);
      if (L->cur.kind != TK_STR && L->cur.kind != TK_IDENT)
        break;
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
           kw(&L->cur,"REQUIRE") || kw(&L->cur,"END") ||
           kw(&L->cur,"IF") || kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") ||
           kw(&L->cur,"PASS") || kw(&L->cur,"FAIL") || kw(&L->cur,"INCLUDE") ||
           kw(&L->cur,"SYS") || kw(&L->cur,"HELP") || kw(&L->cur,"CLEAR_ERR") ||
           kw(&L->cur,"NOTE") || kw(&L->cur,"EXIT") || kw(&L->cur,"DEFAULT") ||
           kw(&L->cur,"VERSION") || kw(&L->cur,"STATUS") || kw(&L->cur,"FOR") ||
           kw(&L->cur,"WHILE") || kw(&L->cur,"LOOP") || kw(&L->cur,"EACH") ||
           kw(&L->cur,"USAGE") || kw(&L->cur,"HELPFLAG") || kw(&L->cur,"CUBE") ||
           kw(&L->cur,"PLUG") || kw(&L->cur,"HOLD_FLASH") ||
           kw(&L->cur,"HASFLAG") || kw(&L->cur,"HASFLAGS") ||
           kw(&L->cur,"HASFLAGALL") || kw(&L->cur,"NEEDFLAGS") ||
           kw(&L->cur,"HASFLAGANY") || kw(&L->cur,"NEEDFLAGANY") ||
           kw(&L->cur,"LISTFLAGS") || kw(&L->cur,"GETFLAG") ||
           kw(&L->cur,"FLAGMAP") || kw(&L->cur,"HASRESTARGS")))
        break;
      if (L->cur.kind == TK_STR) {
        const char *s = L->cur.text;
        if (strchr(s, '\n') || strchr(s, ',') || strchr(s, ' ') || strchr(s, '|')) {
          const char *p = s;
          while (*p && nname < 32) {
            char tok[96];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' ||
                   *p == '|' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != '|' && p[tl] != ':' &&
                   tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            while (tok[0] == '-')
              memmove(tok, tok + 1, strlen(tok));
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
          continue;
        }
        {
          char tok[96];
          snprintf(tok, sizeof tok, "%s", s);
          while (tok[0] == '-')
            memmove(tok, tok + 1, strlen(tok));
          if (tok[0])
            snprintf(names[nname++], sizeof names[0], "%s", tok);
        }
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
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' ||
                   *p == '|' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != '|' && p[tl] != ':' &&
                   tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            while (tok[0] == '-')
              memmove(tok, tok + 1, strlen(tok));
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
        } else if (vv && vv->is_str && vv->sval[0]) {
          char tok[96];
          snprintf(tok, sizeof tok, "%s", vv->sval);
          while (tok[0] == '-')
            memmove(tok, tok + 1, strlen(tok));
          if (tok[0])
            snprintf(names[nname++], sizeof names[0], "%s", tok);
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          char tok[96];
          snprintf(tok, sizeof tok, "%s", vm->last_str);
          while (tok[0] == '-')
            memmove(tok, tok + 1, strlen(tok));
          if (tok[0])
            snprintf(names[nname++], sizeof names[0], "%s", tok);
          lex_next(L);
        } else {
          char tok[96];
          snprintf(tok, sizeof tok, "%s", L->cur.text);
          while (tok[0] == '-')
            memmove(tok, tok + 1, strlen(tok));
          if (tok[0])
            snprintf(names[nname++], sizeof names[0], "%s", tok);
          lex_next(L);
        }
      }
    }
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDFLAGANY name… — NEEDFLAGANY json yaml · HASFLAGANY bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASFLAGANY_N", 0);
      var_set_num(vm, "FLAGMISS_N", 0);
      var_set_num(vm, "NEEDFLAGANY_N", 0);
      var_set_str(vm, "FLAGMISS", "");
      var_set_str(vm, "FLAGHAVE", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASFLAGANY: need names — HASFLAGANY json yaml xml");
      var_set_str(vm, "ERR", "HASFLAGANY: need names — HASFLAGANY json yaml xml");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      int hit;
      val[0] = 0;
      hit = cubalc_scan_cli_flag(names[i], val, sizeof val);
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
    var_set_str(vm, "FLAGMISS", miss);
    var_set_str(vm, "FLAGHAVE", havebag);
    var_set_num(vm, "FLAGMISS_N", nmiss);
    var_set_num(vm, "HASFLAGANY_N", nhave);
    var_set_num(vm, "NEEDFLAGANY_N", nname);
    var_set_num(vm, "FLAGS_ANY_N", nname);
    if (nhave > 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else if (hard) {
      char em[280];
      Var *vu = var_get(vm, "USAGE", 0);
      const char *use = (vu && vu->is_str && vu->sval[0]) ? vu->sval : "";
      if (use[0])
        snprintf(em, sizeof em,
                 "NEEDFLAGANY miss line %d: need one of %d flags — usage: %s",
                 aln, nname, use);
      else
        snprintf(em, sizeof em,
                 "NEEDFLAGANY miss line %d: need one of [%s] — pass any --flag · HASFLAGANY · USAGE",
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
               "HASFLAGANY miss (0/%d): need one of %s — NEEDFLAGANY · USAGE",
               nname, miss[0] ? miss : "?");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# hasflagany n=%d have=%d miss=%d hard=%d\n",
              nname, nhave, nmiss, hard);
    bump(vm);
    return 1;
  }
  /* HASARGALL|NEEDARGS n|name… — multi program-arg presence gate (all-or-ARGMISS).
   * Soft: LAST_N 0|1 · ARGMISS bag · HASARGALL_N have count.
   * Hard NEEDARGS|REQUIREARGS fail-fast first miss (USAGE tip when set).
   * Keys: numeric index → CUBALC_ARGn · digit str same · else getenv(name).
   * Usability: CLI contracts without N× HASARG glue · bag/CSV/idents.
   * Twin of HASFLAGALL for args · not HASARGS (HASARGC count alias). */
  if (kw(&L->cur,"HASARGALL") || kw(&L->cur,"HAS_ARG_ALL") ||
      kw(&L->cur,"HASALLARGS") || kw(&L->cur,"ALLARGS_OK") ||
      kw(&L->cur,"ARGALL?") || kw(&L->cur,"NEEDARGS") ||
      kw(&L->cur,"NEED_ARGS") || kw(&L->cur,"REQUIREARGS") ||
      kw(&L->cur,"MUSTARGS") || kw(&L->cur,"REQUIRE_ARGS")) {
    int hard = kw(&L->cur,"NEEDARGS") || kw(&L->cur,"NEED_ARGS") ||
               kw(&L->cur,"REQUIREARGS") || kw(&L->cur,"MUSTARGS") ||
               kw(&L->cur,"REQUIRE_ARGS");
    char keys[32][96];
    char envns[32][96];
    char miss[1024], havebag[1024];
    int nkey = 0, nmiss = 0, nhave = 0, i, aln = L->cur.line;
    size_t mo = 0, ho = 0;
    lex_next(L);
    miss[0] = 0;
    havebag[0] = 0;
    while (nkey < 32) {
      char key[96], envn[96];
      int all_digit = 1, pi;
      key[0] = 0;
      envn[0] = 0;
      if (L->cur.kind == TK_NUM) {
        snprintf(key, sizeof key, "%ld", L->cur.num);
        snprintf(envn, sizeof envn, "CUBALC_ARG%ld", L->cur.num);
        lex_next(L);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (L->cur.kind == TK_IDENT &&
            (kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
             kw(&L->cur,"REQUIRE") || kw(&L->cur,"END") ||
             kw(&L->cur,"IF") || kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") ||
             kw(&L->cur,"PASS") || kw(&L->cur,"FAIL") || kw(&L->cur,"INCLUDE") ||
             kw(&L->cur,"SYS") || kw(&L->cur,"HELP") || kw(&L->cur,"CLEAR_ERR") ||
             kw(&L->cur,"NOTE") || kw(&L->cur,"EXIT") || kw(&L->cur,"DEFAULT") ||
             kw(&L->cur,"VERSION") || kw(&L->cur,"STATUS") || kw(&L->cur,"FOR") ||
             kw(&L->cur,"WHILE") || kw(&L->cur,"LOOP") || kw(&L->cur,"EACH") ||
             kw(&L->cur,"USAGE") || kw(&L->cur,"HELPFLAG") || kw(&L->cur,"CUBE") ||
             kw(&L->cur,"PLUG") || kw(&L->cur,"HOLD_FLASH") ||
             kw(&L->cur,"HASARG") || kw(&L->cur,"HASARGC") ||
             kw(&L->cur,"HASARGALL") || kw(&L->cur,"NEEDARGS") ||
             kw(&L->cur,"HASARGANY") || kw(&L->cur,"NEEDARGANY") ||
             kw(&L->cur,"HASFLAG") || kw(&L->cur,"HASFLAGS") ||
             kw(&L->cur,"HASFLAGALL") || kw(&L->cur,"NEEDFLAGS") ||
             kw(&L->cur,"HASFLAGANY") || kw(&L->cur,"NEEDFLAGANY") ||
             kw(&L->cur,"LISTFLAGS") || kw(&L->cur,"GETFLAG") ||
             kw(&L->cur,"RESTARGS") || kw(&L->cur,"HASRESTARGS")))
          break;
        if (L->cur.kind == TK_STR &&
            (strchr(L->cur.text, '\n') || strchr(L->cur.text, ',') ||
             strchr(L->cur.text, ' ') || strchr(L->cur.text, '|'))) {
          const char *p = L->cur.text;
          while (*p && nkey < 32) {
            char tok[96];
            size_t tl = 0;
            int ad = 1;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' ||
                   *p == '|' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != '|' && p[tl] != ':' &&
                   tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (!tok[0]) continue;
            for (pi = 0; tok[pi]; pi++) {
              if (tok[pi] < '0' || tok[pi] > '9') { ad = 0; break; }
            }
            snprintf(keys[nkey], sizeof keys[0], "%s", tok);
            if (ad)
              snprintf(envns[nkey], sizeof envns[0], "CUBALC_ARG%s", tok);
            else if (strncmp(tok, "CUBALC_ARG", 10) == 0)
              snprintf(envns[nkey], sizeof envns[0], "%s", tok);
            else
              snprintf(envns[nkey], sizeof envns[0], "%s", tok);
            nkey++;
          }
          lex_next(L);
          continue;
        }
        {
          Var *vv = (L->cur.kind == TK_IDENT) ? var_get(vm, L->cur.text, 0) : NULL;
          if (vv && vv->is_str && vv->sval[0] &&
              (strchr(vv->sval, '\n') || strchr(vv->sval, ',') ||
               strchr(vv->sval, ' ') || strchr(vv->sval, '|'))) {
            const char *p = vv->sval;
            while (*p && nkey < 32) {
              char tok[96];
              size_t tl = 0;
              int ad = 1;
              while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' ||
                     *p == '|' || *p == ':')
                p++;
              if (!*p) break;
              while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                     p[tl] != '\t' && p[tl] != '|' && p[tl] != ':' &&
                     tl + 1 < sizeof tok) {
                tok[tl] = p[tl];
                tl++;
              }
              tok[tl] = 0;
              p += tl;
              if (!tok[0]) continue;
              for (pi = 0; tok[pi]; pi++) {
                if (tok[pi] < '0' || tok[pi] > '9') { ad = 0; break; }
              }
              snprintf(keys[nkey], sizeof keys[0], "%s", tok);
              if (ad)
                snprintf(envns[nkey], sizeof envns[0], "CUBALC_ARG%s", tok);
              else if (strncmp(tok, "CUBALC_ARG", 10) == 0)
                snprintf(envns[nkey], sizeof envns[0], "%s", tok);
              else
                snprintf(envns[nkey], sizeof envns[0], "%s", tok);
              nkey++;
            }
            lex_next(L);
            continue;
          }
          if (vv && vv->is_str && vv->sval[0])
            snprintf(key, sizeof key, "%s", vv->sval);
          else if (vv && !vv->is_str)
            snprintf(key, sizeof key, "%ld", vv->val);
          else if (strcmp(L->cur.text, "LAST") == 0)
            snprintf(key, sizeof key, "%s", vm->last_str);
          else
            snprintf(key, sizeof key, "%s", L->cur.text);
        }
        lex_next(L);
        for (pi = 0; key[pi]; pi++) {
          if (key[pi] < '0' || key[pi] > '9') { all_digit = 0; break; }
        }
        if (all_digit && key[0])
          snprintf(envn, sizeof envn, "CUBALC_ARG%s", key);
        else if (strncmp(key, "CUBALC_ARG", 10) == 0)
          snprintf(envn, sizeof envn, "%s", key);
        else
          snprintf(envn, sizeof envn, "%s", key);
      } else {
        break;
      }
      if (!key[0]) continue;
      snprintf(keys[nkey], sizeof keys[0], "%s", key);
      snprintf(envns[nkey], sizeof envns[0], "%s", envn);
      nkey++;
    }
    if (nkey == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDARGS n|name… — NEEDARGS 0 1 · HASARGALL bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASARGALL_N", 0);
      var_set_num(vm, "ARGMISS_N", 0);
      var_set_num(vm, "NEEDARGS_N", 0);
      var_set_str(vm, "ARGMISS", "");
      var_set_str(vm, "ARGHAVE", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASARGALL: need n|name… — HASARGALL 0 1 file");
      var_set_str(vm, "ERR", "HASARGALL: need n|name… — HASARGALL 0 1 file");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nkey; i++) {
      const char *val = getenv(envns[i]);
      int hit = (val && val[0]) ? 1 : 0;
      if (hit) {
        nhave++;
        if (ho && ho + 1 < sizeof havebag) havebag[ho++] = '\n';
        {
          size_t ln = strlen(keys[i]);
          if (ho + ln < sizeof havebag) {
            memcpy(havebag + ho, keys[i], ln);
            ho += ln;
            havebag[ho] = 0;
          }
        }
        continue;
      }
      nmiss++;
      if (mo && mo + 1 < sizeof miss) miss[mo++] = '\n';
      {
        size_t ln = strlen(keys[i]);
        if (mo + ln < sizeof miss) {
          memcpy(miss + mo, keys[i], ln);
          mo += ln;
          miss[mo] = 0;
        }
      }
      if (hard) {
        char em[280];
        Var *vu = var_get(vm, "USAGE", 0);
        const char *use = (vu && vu->is_str && vu->sval[0]) ? vu->sval : "";
        if (use[0])
          snprintf(em, sizeof em,
                   "NEEDARGS miss line %d: '%s' (+%d more?) — set arg/env · usage: %s",
                   aln, keys[i], nkey - i - 1, use);
        else
          snprintf(em, sizeof em,
                   "NEEDARGS miss line %d: '%s' (+%d more?) — set CUBALC_ARG/env · HASARGALL · USAGE",
                   aln, keys[i], nkey - i - 1);
        var_set_str(vm, "ARGMISS", miss);
        var_set_num(vm, "ARGMISS_N", nmiss);
        var_set_str(vm, "ARGHAVE", havebag);
        var_set_num(vm, "HASARGALL_N", nhave);
        fail_at(vm, L, em);
        return -1;
      }
    }
    var_set_str(vm, "ARGMISS", miss);
    var_set_str(vm, "ARGHAVE", havebag);
    var_set_num(vm, "ARGMISS_N", nmiss);
    var_set_num(vm, "HASARGALL_N", nhave);
    var_set_num(vm, "NEEDARGS_N", nkey);
    var_set_num(vm, "ARGS_NEED_N", nkey);
    if (nmiss == 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else {
      char em[240];
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      snprintf(em, sizeof em,
               "HASARGALL miss (%d/%d): %s — set args · NEEDARGS · USAGE",
               nmiss, nkey, miss);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# hasargall n=%d have=%d miss=%d hard=%d\n",
              nkey, nhave, nmiss, hard);
    bump(vm);
    return 1;
  }
  /* HASENVALL|NEEDENVS name… — multi host-env ALL gate (HASARGALL twin for process env).
   * Soft: LAST_N 0|1 · ENVMISS bag · ENVHAVE bag · HASENVALL_N have count.
   * Hard NEEDENVS|REQUIREENVS fail-fast first miss (USAGE tip when set).
   * Keys: bare idents/strings · bag/CSV · LAST. Empty name list soft → 0.
   * B_E_science restore — help/discover already listed; form was missing. */
  if (kw(&L->cur,"HASENVALL") || kw(&L->cur,"HAS_ENV_ALL") ||
      kw(&L->cur,"HASALLENV") || kw(&L->cur,"ALLENV_OK") ||
      kw(&L->cur,"ENVALL?") || kw(&L->cur,"NEEDENVS") ||
      kw(&L->cur,"NEED_ENVS") || kw(&L->cur,"REQUIREENVS") ||
      kw(&L->cur,"MUSTENVS") || kw(&L->cur,"REQUIRE_ENVS")) {
    int hard = kw(&L->cur,"NEEDENVS") || kw(&L->cur,"NEED_ENVS") ||
               kw(&L->cur,"REQUIREENVS") || kw(&L->cur,"MUSTENVS") ||
               kw(&L->cur,"REQUIRE_ENVS");
    char keys[32][96];
    char miss[1024], havebag[1024];
    int nkey = 0, nmiss = 0, nhave = 0, i, aln = L->cur.line;
    size_t mo = 0, ho = 0;
    lex_next(L);
    miss[0] = 0;
    havebag[0] = 0;
    while (nkey < 32) {
      char key[96];
      key[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (L->cur.kind == TK_IDENT &&
            (kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
             kw(&L->cur,"REQUIRE") || kw(&L->cur,"END") ||
             kw(&L->cur,"IF") || kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") ||
             kw(&L->cur,"PASS") || kw(&L->cur,"FAIL") || kw(&L->cur,"INCLUDE") ||
             kw(&L->cur,"SYS") || kw(&L->cur,"HELP") || kw(&L->cur,"CLEAR_ERR") ||
             kw(&L->cur,"NOTE") || kw(&L->cur,"EXIT") || kw(&L->cur,"DEFAULT") ||
             kw(&L->cur,"VERSION") || kw(&L->cur,"STATUS") || kw(&L->cur,"FOR") ||
             kw(&L->cur,"WHILE") || kw(&L->cur,"LOOP") || kw(&L->cur,"EACH") ||
             kw(&L->cur,"USAGE") || kw(&L->cur,"CUBE") || kw(&L->cur,"PLUG") ||
             kw(&L->cur,"HOLD_FLASH") ||
             kw(&L->cur,"HASENV") || kw(&L->cur,"HASENVALL") || kw(&L->cur,"NEEDENVS") ||
             kw(&L->cur,"HASENVANY") || kw(&L->cur,"NEEDENVANY") ||
             kw(&L->cur,"HASARG") || kw(&L->cur,"HASARGALL") || kw(&L->cur,"NEEDARGS") ||
             kw(&L->cur,"HASARGANY") || kw(&L->cur,"NEEDARGANY") ||
             kw(&L->cur,"HASFLAG") || kw(&L->cur,"HASFLAGALL") || kw(&L->cur,"NEEDFLAGS") ||
             kw(&L->cur,"HASFORM") || kw(&L->cur,"HASFORMS")))
          break;
        if (L->cur.kind == TK_STR &&
            (strchr(L->cur.text, '\n') || strchr(L->cur.text, ',') ||
             strchr(L->cur.text, ' ') || strchr(L->cur.text, '|'))) {
          const char *p = L->cur.text;
          while (*p && nkey < 32) {
            char tok[96];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' ||
                   *p == '|' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != '|' && p[tl] != ':' &&
                   tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (!tok[0]) continue;
            snprintf(keys[nkey], sizeof keys[0], "%s", tok);
            nkey++;
          }
          lex_next(L);
          continue;
        }
        {
          Var *vv = (L->cur.kind == TK_IDENT) ? var_get(vm, L->cur.text, 0) : NULL;
          if (vv && vv->is_str && vv->sval[0] &&
              (strchr(vv->sval, '\n') || strchr(vv->sval, ',') ||
               strchr(vv->sval, ' ') || strchr(vv->sval, '|'))) {
            const char *p = vv->sval;
            while (*p && nkey < 32) {
              char tok[96];
              size_t tl = 0;
              while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' ||
                     *p == '|' || *p == ':')
                p++;
              if (!*p) break;
              while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                     p[tl] != '\t' && p[tl] != '|' && p[tl] != ':' &&
                     tl + 1 < sizeof tok) {
                tok[tl] = p[tl];
                tl++;
              }
              tok[tl] = 0;
              p += tl;
              if (!tok[0]) continue;
              snprintf(keys[nkey], sizeof keys[0], "%s", tok);
              nkey++;
            }
            lex_next(L);
            continue;
          }
          if (vv && vv->is_str && vv->sval[0])
            snprintf(key, sizeof key, "%s", vv->sval);
          else if (strcmp(L->cur.text, "LAST") == 0)
            snprintf(key, sizeof key, "%s", vm->last_str);
          else
            snprintf(key, sizeof key, "%s", L->cur.text);
        }
        lex_next(L);
      } else {
        break;
      }
      if (!key[0]) continue;
      snprintf(keys[nkey], sizeof keys[0], "%s", key);
      nkey++;
    }
    if (nkey == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDENVS name… — NEEDENVS A B · HASENVALL bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASENVALL_N", 0);
      var_set_num(vm, "ENVMISS_N", 0);
      var_set_num(vm, "NEEDENVS_N", 0);
      var_set_str(vm, "ENVMISS", "");
      var_set_str(vm, "ENVHAVE", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASENVALL: need name… — HASENVALL A B");
      var_set_str(vm, "ERR", "HASENVALL: need name… — HASENVALL A B");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nkey; i++) {
      const char *val = getenv(keys[i]);
      int hit = (val && val[0]) ? 1 : 0;
      if (hit) {
        nhave++;
        if (ho && ho + 1 < sizeof havebag) havebag[ho++] = '\n';
        {
          size_t ln = strlen(keys[i]);
          if (ho + ln < sizeof havebag) {
            memcpy(havebag + ho, keys[i], ln);
            ho += ln;
            havebag[ho] = 0;
          }
        }
        continue;
      }
      nmiss++;
      if (mo && mo + 1 < sizeof miss) miss[mo++] = '\n';
      {
        size_t ln = strlen(keys[i]);
        if (mo + ln < sizeof miss) {
          memcpy(miss + mo, keys[i], ln);
          mo += ln;
          miss[mo] = 0;
        }
      }
      if (hard) {
        char em[280];
        Var *vu = var_get(vm, "USAGE", 0);
        const char *use = (vu && vu->is_str && vu->sval[0]) ? vu->sval : "";
        if (use[0])
          snprintf(em, sizeof em,
                   "NEEDENVS miss line %d: '%s' (+%d more?) — set env · usage: %s",
                   aln, keys[i], nkey - i - 1, use);
        else
          snprintf(em, sizeof em,
                   "NEEDENVS miss line %d: '%s' (+%d more?) — SYS ENV SET · HASENVALL · USAGE",
                   aln, keys[i], nkey - i - 1);
        var_set_str(vm, "ENVMISS", miss);
        var_set_num(vm, "ENVMISS_N", nmiss);
        var_set_str(vm, "ENVHAVE", havebag);
        var_set_num(vm, "HASENVALL_N", nhave);
        var_set_num(vm, "NEEDENVS_N", nkey);
        fail_at(vm, L, em);
        return -1;
      }
    }
    var_set_str(vm, "ENVMISS", miss);
    var_set_str(vm, "ENVHAVE", havebag);
    var_set_num(vm, "ENVMISS_N", nmiss);
    var_set_num(vm, "HASENVALL_N", nhave);
    var_set_num(vm, "NEEDENVS_N", nkey);
    var_set_num(vm, "ENVS_NEED_N", nkey);
    if (nmiss == 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else {
      char em[240];
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      snprintf(em, sizeof em,
               "HASENVALL miss (%d/%d): %s — set env · NEEDENVS · USAGE",
               nmiss, nkey, miss);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# hasenvall n=%d have=%d miss=%d hard=%d\n",
              nkey, nhave, nmiss, hard);
    bump(vm);
    return 1;
  }

  /* HASARGANY|NEEDARGANY n|name… — multi program-arg any-of gate (HASARGALL twin).
   * Soft: LAST_N 1 if any present · ARGHAVE bag of hits · ARGMISS of absences.
   * Hard NEEDARGANY fail-fast if none present (USAGE tip when set).
   * Usability: need ARG0 OR env FILE OR HOST without N× HASARG + IF glue. */
  if (kw(&L->cur,"HASARGANY") || kw(&L->cur,"HAS_ARG_ANY") ||
      kw(&L->cur,"HASANYARGS") || kw(&L->cur,"ANYARGS_OK") ||
      kw(&L->cur,"ARGANY?") || kw(&L->cur,"NEEDARGANY") ||
      kw(&L->cur,"NEED_ARG_ANY") || kw(&L->cur,"REQUIREARGANY") ||
      kw(&L->cur,"MUSTARGANY") || kw(&L->cur,"REQUIRE_ARG_ANY") ||
      kw(&L->cur,"NEEDANYARGS") || kw(&L->cur,"REQUIREANYARGS")) {
    int hard = kw(&L->cur,"NEEDARGANY") || kw(&L->cur,"NEED_ARG_ANY") ||
               kw(&L->cur,"REQUIREARGANY") || kw(&L->cur,"MUSTARGANY") ||
               kw(&L->cur,"REQUIRE_ARG_ANY") || kw(&L->cur,"NEEDANYARGS") ||
               kw(&L->cur,"REQUIREANYARGS");
    char keys[32][96];
    char envns[32][96];
    char miss[1024], havebag[1024];
    int nkey = 0, nmiss = 0, nhave = 0, i, aln = L->cur.line;
    size_t mo = 0, ho = 0;
    lex_next(L);
    miss[0] = 0;
    havebag[0] = 0;
    while (nkey < 32) {
      char key[96], envn[96];
      int all_digit = 1, pi;
      key[0] = 0;
      envn[0] = 0;
      if (L->cur.kind == TK_NUM) {
        snprintf(key, sizeof key, "%ld", L->cur.num);
        snprintf(envn, sizeof envn, "CUBALC_ARG%ld", L->cur.num);
        lex_next(L);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (L->cur.kind == TK_IDENT &&
            (kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
             kw(&L->cur,"REQUIRE") || kw(&L->cur,"END") ||
             kw(&L->cur,"IF") || kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") ||
             kw(&L->cur,"PASS") || kw(&L->cur,"FAIL") || kw(&L->cur,"INCLUDE") ||
             kw(&L->cur,"SYS") || kw(&L->cur,"HELP") || kw(&L->cur,"CLEAR_ERR") ||
             kw(&L->cur,"NOTE") || kw(&L->cur,"EXIT") || kw(&L->cur,"DEFAULT") ||
             kw(&L->cur,"VERSION") || kw(&L->cur,"STATUS") || kw(&L->cur,"FOR") ||
             kw(&L->cur,"WHILE") || kw(&L->cur,"LOOP") || kw(&L->cur,"EACH") ||
             kw(&L->cur,"USAGE") || kw(&L->cur,"HELPFLAG") || kw(&L->cur,"CUBE") ||
             kw(&L->cur,"PLUG") || kw(&L->cur,"HOLD_FLASH") ||
             kw(&L->cur,"HASARG") || kw(&L->cur,"HASARGC") ||
             kw(&L->cur,"HASARGALL") || kw(&L->cur,"NEEDARGS") ||
             kw(&L->cur,"HASARGANY") || kw(&L->cur,"NEEDARGANY") ||
             kw(&L->cur,"HASFLAG") || kw(&L->cur,"HASFLAGS") ||
             kw(&L->cur,"HASFLAGALL") || kw(&L->cur,"NEEDFLAGS") ||
             kw(&L->cur,"HASFLAGANY") || kw(&L->cur,"NEEDFLAGANY") ||
             kw(&L->cur,"LISTFLAGS") || kw(&L->cur,"GETFLAG") ||
             kw(&L->cur,"RESTARGS") || kw(&L->cur,"HASRESTARGS")))
          break;
        if (L->cur.kind == TK_STR &&
            (strchr(L->cur.text, '\n') || strchr(L->cur.text, ',') ||
             strchr(L->cur.text, ' ') || strchr(L->cur.text, '|'))) {
          const char *p = L->cur.text;
          while (*p && nkey < 32) {
            char tok[96];
            size_t tl = 0;
            int ad = 1;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' ||
                   *p == '|' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != '|' && p[tl] != ':' &&
                   tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (!tok[0]) continue;
            for (pi = 0; tok[pi]; pi++) {
              if (tok[pi] < '0' || tok[pi] > '9') { ad = 0; break; }
            }
            snprintf(keys[nkey], sizeof keys[0], "%s", tok);
            if (ad)
              snprintf(envns[nkey], sizeof envns[0], "CUBALC_ARG%s", tok);
            else if (strncmp(tok, "CUBALC_ARG", 10) == 0)
              snprintf(envns[nkey], sizeof envns[0], "%s", tok);
            else
              snprintf(envns[nkey], sizeof envns[0], "%s", tok);
            nkey++;
          }
          lex_next(L);
          continue;
        }
        {
          Var *vv = (L->cur.kind == TK_IDENT) ? var_get(vm, L->cur.text, 0) : NULL;
          if (vv && vv->is_str && vv->sval[0] &&
              (strchr(vv->sval, '\n') || strchr(vv->sval, ',') ||
               strchr(vv->sval, ' ') || strchr(vv->sval, '|'))) {
            const char *p = vv->sval;
            while (*p && nkey < 32) {
              char tok[96];
              size_t tl = 0;
              int ad = 1;
              while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' ||
                     *p == '|' || *p == ':')
                p++;
              if (!*p) break;
              while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                     p[tl] != '\t' && p[tl] != '|' && p[tl] != ':' &&
                     tl + 1 < sizeof tok) {
                tok[tl] = p[tl];
                tl++;
              }
              tok[tl] = 0;
              p += tl;
              if (!tok[0]) continue;
              for (pi = 0; tok[pi]; pi++) {
                if (tok[pi] < '0' || tok[pi] > '9') { ad = 0; break; }
              }
              snprintf(keys[nkey], sizeof keys[0], "%s", tok);
              if (ad)
                snprintf(envns[nkey], sizeof envns[0], "CUBALC_ARG%s", tok);
              else if (strncmp(tok, "CUBALC_ARG", 10) == 0)
                snprintf(envns[nkey], sizeof envns[0], "%s", tok);
              else
                snprintf(envns[nkey], sizeof envns[0], "%s", tok);
              nkey++;
            }
            lex_next(L);
            continue;
          }
          if (vv && vv->is_str && vv->sval[0])
            snprintf(key, sizeof key, "%s", vv->sval);
          else if (vv && !vv->is_str)
            snprintf(key, sizeof key, "%ld", vv->val);
          else if (strcmp(L->cur.text, "LAST") == 0)
            snprintf(key, sizeof key, "%s", vm->last_str);
          else
            snprintf(key, sizeof key, "%s", L->cur.text);
        }
        lex_next(L);
        for (pi = 0; key[pi]; pi++) {
          if (key[pi] < '0' || key[pi] > '9') { all_digit = 0; break; }
        }
        if (all_digit && key[0])
          snprintf(envn, sizeof envn, "CUBALC_ARG%s", key);
        else if (strncmp(key, "CUBALC_ARG", 10) == 0)
          snprintf(envn, sizeof envn, "%s", key);
        else
          snprintf(envn, sizeof envn, "%s", key);
      } else {
        break;
      }
      if (!key[0]) continue;
      snprintf(keys[nkey], sizeof keys[0], "%s", key);
      snprintf(envns[nkey], sizeof envns[0], "%s", envn);
      nkey++;
    }
    if (nkey == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDARGANY n|name… — NEEDARGANY 0 FILE · HASARGANY bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASARGANY_N", 0);
      var_set_num(vm, "ARGMISS_N", 0);
      var_set_num(vm, "NEEDARGANY_N", 0);
      var_set_str(vm, "ARGMISS", "");
      var_set_str(vm, "ARGHAVE", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASARGANY: need n|name… — HASARGANY 0 1 FILE");
      var_set_str(vm, "ERR", "HASARGANY: need n|name… — HASARGANY 0 1 FILE");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nkey; i++) {
      const char *val = getenv(envns[i]);
      int hit = (val && val[0]) ? 1 : 0;
      if (hit) {
        nhave++;
        if (ho && ho + 1 < sizeof havebag) havebag[ho++] = '\n';
        {
          size_t ln = strlen(keys[i]);
          if (ho + ln < sizeof havebag) {
            memcpy(havebag + ho, keys[i], ln);
            ho += ln;
            havebag[ho] = 0;
          }
        }
      } else {
        nmiss++;
        if (mo && mo + 1 < sizeof miss) miss[mo++] = '\n';
        {
          size_t ln = strlen(keys[i]);
          if (mo + ln < sizeof miss) {
            memcpy(miss + mo, keys[i], ln);
            mo += ln;
            miss[mo] = 0;
          }
        }
      }
    }
    var_set_str(vm, "ARGMISS", miss);
    var_set_str(vm, "ARGHAVE", havebag);
    var_set_num(vm, "ARGMISS_N", nmiss);
    var_set_num(vm, "HASARGANY_N", nhave);
    var_set_num(vm, "NEEDARGANY_N", nkey);
    var_set_num(vm, "ARGS_ANY_N", nkey);
    if (nhave > 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else if (hard) {
      char em[280];
      Var *vu = var_get(vm, "USAGE", 0);
      const char *use = (vu && vu->is_str && vu->sval[0]) ? vu->sval : "";
      if (use[0])
        snprintf(em, sizeof em,
                 "NEEDARGANY miss line %d: need one of %d args — usage: %s",
                 aln, nkey, use);
      else
        snprintf(em, sizeof em,
                 "NEEDARGANY miss line %d: need one of [%s] — set CUBALC_ARG/env · HASARGANY · USAGE",
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
               "HASARGANY miss (0/%d): need one of %s — NEEDARGANY · USAGE",
               nkey, miss[0] ? miss : "?");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# hasargany n=%d have=%d miss=%d hard=%d\n",
              nkey, nhave, nmiss, hard);
    bump(vm);
    return 1;
  }
  return 0;
}
