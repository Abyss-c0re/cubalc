/* CubalC lang — lang_ops_core_discover.c
 * RELATED · SNIP · TOPIC · GUIDE · DISCOVER · OPEN · ERRTIPS · START (core carve).
 */
#include "lang/cubalc_lang_ops_core_discover.h"
#include "lang/cubalc_lang_ops_core_help.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

int cubalc_lang_ops_core_try_discover(VM *vm, Lex *L) {
  /* RELATED|SEEALSO form — curated related form-name bag (dual of cubalc related).
   * Complements FORMHINT (one-line docs) and FORMSFOR (topic surface): form → neighbors.
   * LAST = newline related names · RELATED_N · RELATED_OF · soft miss OK=0. */
  if (kw(&L->cur,"RELATED")||kw(&L->cur,"SEEALSO")||kw(&L->cur,"FORMSLIKE")||
      kw(&L->cur,"NEARFORMS")||kw(&L->cur,"WITHFORMS")||kw(&L->cur,"FORMREL")||
      kw(&L->cur,"RELATEDFORMS")||kw(&L->cur,"NEIGHBORS")){
    static const struct { const char *form; const char *rel; } edges[] = {
      /* capability */
      {"HASFORM", "NEEDFORM"}, {"HASFORM", "HASFORMS"}, {"HASFORM", "NEEDFORMS"},
      {"HASFORM", "FORMHINT"}, {"HASFORM", "LISTFORMS"}, {"HASFORM", "COUNTFORMS"},
      {"HASFORM", "REQUIRE FORM"}, {"HASFORM", "FORMSFOR"}, {"HASFORM", "TIPS"},
      {"NEEDFORM", "HASFORM"}, {"NEEDFORM", "NEEDFORMS"}, {"NEEDFORM", "HASFORMS"},
      {"NEEDFORM", "FORMHINT"}, {"NEEDFORM", "REQUIRE FORM"},
      {"HASFORMS", "NEEDFORMS"}, {"HASFORMS", "HASFORM"}, {"HASFORMS", "FORMMISS"},
      {"HASFORMS", "HASFORMANY"}, {"HASFORMS", "NEEDFORMANY"},
      {"NEEDFORMS", "HASFORMS"}, {"NEEDFORMS", "NEEDFORM"}, {"NEEDFORMS", "HASFORM"},
      {"NEEDFORMS", "NEEDFORMANY"},
      {"HASFORMANY", "NEEDFORMANY"}, {"HASFORMANY", "HASFORMS"}, {"HASFORMANY", "HASFORM"},
      {"HASFORMANY", "FORMHAVE"}, {"HASFORMANY", "FORMMISS"}, {"HASFORMANY", "LISTFORMS"},
      {"NEEDFORMANY", "HASFORMANY"}, {"NEEDFORMANY", "NEEDFORMS"}, {"NEEDFORMANY", "NEEDFORM"},
      {"NEEDFORMANY", "FORMMISS"}, {"NEEDFORMANY", "FORMHAVE"},
      /* multi CLI flag contract (HASFORMS twin for --flags) */
      {"HASFLAG", "HASFLAGALL"}, {"HASFLAG", "NEEDFLAGS"}, {"HASFLAG", "REQUIRE FLAG"},
      {"HASFLAGALL", "NEEDFLAGS"}, {"HASFLAGALL", "HASFLAG"}, {"HASFLAGALL", "FLAGMISS"},
      {"HASFLAGALL", "HASFLAGS"}, {"HASFLAGALL", "LISTFLAGS"},
      {"NEEDFLAGS", "HASFLAGALL"}, {"NEEDFLAGS", "HASFLAG"}, {"NEEDFLAGS", "REQUIRE FLAG"},
      {"NEEDFLAGS", "USAGE"}, {"NEEDFLAGS", "FLAGMISS"},
      {"HASFLAGS", "HASFLAGALL"}, {"HASFLAGS", "LISTFLAGS"}, {"HASFLAGS", "HASFLAG"},
      {"LISTFLAGS", "HASFLAGS"}, {"LISTFLAGS", "HASFLAGALL"}, {"LISTFLAGS", "FLAGMAP"},
      /* any-of multi CLI flag contract (HASFLAGALL twin) */
      {"HASFLAGANY", "NEEDFLAGANY"}, {"HASFLAGANY", "HASFLAGALL"}, {"HASFLAGANY", "HASFLAG"},
      {"HASFLAGANY", "FLAGHAVE"}, {"HASFLAGANY", "LISTFLAGS"},
      {"NEEDFLAGANY", "HASFLAGANY"}, {"NEEDFLAGANY", "NEEDFLAGS"}, {"NEEDFLAGANY", "HASFLAG"},
      {"NEEDFLAGANY", "USAGE"}, {"NEEDFLAGANY", "FLAGMISS"},
      /* multi CLI arg contract (HASFLAGALL twin for CUBALC_ARGn / env names) */
      {"HASARG", "HASARGALL"}, {"HASARG", "NEEDARGS"}, {"HASARG", "REQUIRE ARG"},
      {"HASARG", "HASARGANY"}, {"HASARG", "NEEDARGANY"},
      {"HASARGALL", "NEEDARGS"}, {"HASARGALL", "HASARG"}, {"HASARGALL", "ARGMISS"},
      {"HASARGALL", "HASARGC"}, {"HASARGALL", "RESTARGS"}, {"HASARGALL", "HASARGANY"},
      {"NEEDARGS", "HASARGALL"}, {"NEEDARGS", "HASARG"}, {"NEEDARGS", "REQUIRE ARG"},
      {"NEEDARGS", "USAGE"}, {"NEEDARGS", "ARGMISS"}, {"NEEDARGS", "NEEDARGANY"},
      {"HASARGANY", "NEEDARGANY"}, {"HASARGANY", "HASARGALL"}, {"HASARGANY", "HASARG"},
      {"HASARGANY", "ARGHAVE"}, {"HASARGANY", "ARGMISS"},
      {"NEEDARGANY", "HASARGANY"}, {"NEEDARGANY", "NEEDARGS"}, {"NEEDARGANY", "HASARG"},
      {"NEEDARGANY", "USAGE"}, {"NEEDARGANY", "ARGMISS"},
      /* multi host-env contract (HASARGALL twin for process env keys) */
      {"HASENVALL", "NEEDENVS"}, {"HASENVALL", "SYS HASENV"}, {"HASENVALL", "REQUIRE ENV"},
      {"HASENVALL", "ENVMISS"}, {"HASENVALL", "ENVHAVE"}, {"HASENVALL", "HASARGALL"},
      {"HASENVALL", "HASENVANY"}, {"HASENVALL", "NEEDENVANY"},
      {"NEEDENVS", "HASENVALL"}, {"NEEDENVS", "REQUIRE ENV"}, {"NEEDENVS", "SYS HASENV"},
      {"NEEDENVS", "USAGE"}, {"NEEDENVS", "ENVMISS"}, {"NEEDENVS", "SYS ENV SET"},
      {"NEEDENVS", "NEEDENVANY"},
      {"HASENVANY", "NEEDENVANY"}, {"HASENVANY", "HASENVALL"}, {"HASENVANY", "SYS HASENV"},
      {"HASENVANY", "ENVHAVE"}, {"HASENVANY", "ENVMISS"}, {"HASENVANY", "HASARGANY"},
      {"NEEDENVANY", "HASENVANY"}, {"NEEDENVANY", "NEEDENVS"}, {"NEEDENVANY", "REQUIRE ENV"},
      {"NEEDENVANY", "USAGE"}, {"NEEDENVANY", "ENVMISS"}, {"NEEDENVANY", "SYS ENV SET"},
      {"HASARGC", "HASARGALL"}, {"HASARGC", "HASARG"}, {"HASARGC", "NEEDARGS"},
      {"CLIINFO", "LISTFLAGS"}, {"CLIINFO", "FLAGMAP"}, {"CLIINFO", "RESTARGS"},
      {"CLIINFO", "HASFLAGALL"}, {"CLIINFO", "HASARGALL"}, {"CLIINFO", "USAGE"},
      {"CLIINFO", "ARGMAP"}, {"DUMPCLI", "CLIINFO"}, {"DUMPCLI", "LISTFLAGS"},
      {"DUMPCLI", "FLAGMAP"}, {"DUMPCLI", "ARGMAP"},
      {"LISTFLAGS", "CLIINFO"}, {"FLAGMAP", "CLIINFO"}, {"RESTARGS", "CLIINFO"},
      {"ARGMAP", "FLAGMAP"}, {"ARGMAP", "CLIINFO"}, {"ARGMAP", "ARGS"},
      {"ARGMAP", "HASARG"}, {"ARGMAP", "NTHPOS"}, {"ARGMAP", "NTHARG"},
      {"FLAGMAP", "ARGMAP"},
      {"NTHARG", "ARGMAP"}, {"NTHARG", "HASARG"}, {"NTHARG", "NTHPOS"},
      {"NTHARG", "SYS ARG"}, {"NTHARG", "FINDARG"}, {"NTHPOS", "NTHARG"},
      {"GETARG", "NTHARG"},
      {"FINDARG", "NTHARG"}, {"FINDARG", "ARGMAP"}, {"FINDARG", "HASARG"},
      {"FINDARG", "SYS ARGS"}, {"FINDARG", "AFTERARG"}, {"ARGINDEX", "FINDARG"},
      {"ARGINDEX", "NTHARG"},
      {"AFTERARG", "FINDARG"}, {"AFTERARG", "NTHARG"}, {"AFTERARG", "GETFLAG"},
      {"AFTERARG", "ARGMAP"}, {"NEXTARG", "AFTERARG"}, {"NEXTARG", "FINDARG"},
      {"FIRSTARG", "LASTARG"}, {"FIRSTARG", "NTHARG"}, {"FIRSTARG", "ARGMAP"},
      {"FIRSTARG", "SYS ARGS"}, {"LASTARG", "FIRSTARG"}, {"LASTARG", "NTHARG"},
      {"LASTARG", "ARGMAP"}, {"HEADARG", "FIRSTARG"}, {"TAILARG", "LASTARG"},
      {"FORMHINT", "HASFORM"}, {"FORMHINT", "LISTFORMS"}, {"FORMHINT", "RELATED"},
      {"FORMHINT", "FORMSFOR"}, {"FORMHINT", "TIPS"},
      {"LISTFORMS", "COUNTFORMS"}, {"LISTFORMS", "HASFORM"}, {"LISTFORMS", "FORMHINT"},
      {"LISTFORMS", "FORMSFOR"}, {"COUNTFORMS", "LISTFORMS"}, {"COUNTFORMS", "HASFORM"},
      {"FORMSFOR", "TIPS"}, {"FORMSFOR", "RELATED"}, {"FORMSFOR", "FORMHINT"},
      {"FORMSFOR", "LISTFORMS"}, {"FORMSFOR", "HASFORM"},
      {"TIPS", "FORMSFOR"}, {"TIPS", "WHY"}, {"TIPS", "FORMHINT"}, {"TIPS", "RELATED"},
      /* topic discovery graph (RELATED twin for topics) */
      {"TOPICHINT", "RELATEDTOPIC"}, {"TOPICHINT", "LISTTOPICS"}, {"TOPICHINT", "TOPIC"},
      {"TOPICHINT", "HASTOPIC"}, {"TOPICHINT", "TIPS"},
      {"RELATEDTOPIC", "TOPICHINT"}, {"RELATEDTOPIC", "LISTTOPICS"}, {"RELATEDTOPIC", "TOPIC"},
      {"RELATEDTOPIC", "HASTOPIC"}, {"RELATEDTOPIC", "TIPS"}, {"RELATEDTOPIC", "FORMSFOR"},
      {"RELATEDTOPIC", "FORMTOPICS"},
      {"LISTTOPICS", "RELATEDTOPIC"}, {"LISTTOPICS", "TOPICHINT"}, {"LISTTOPICS", "HASTOPIC"},
      {"LISTTOPICS", "NEEDTOPIC"}, {"LISTTOPICS", "TOPIC"}, {"LISTTOPICS", "FORMTOPICS"},
      {"LISTTOPICS", "MATCHTOPICS"}, {"LISTTOPICS", "PICKTOPIC"}, {"LISTTOPICS", "DISCOVER"},
      {"DISCOVER", "MATCHTOPICS"}, {"DISCOVER", "GUIDE"}, {"DISCOVER", "RUNSNIP"}, {"DISCOVER", "OPEN"},
      {"OPEN", "DISCOVER"}, {"OPEN", "GUIDE"}, {"OPEN", "RUNSNIP"}, {"OPEN", "GUIDEMATCH"},
      {"DISCOVER", "TOPICHINT"}, {"DISCOVER", "LISTTOPICS"}, {"DISCOVER", "START"},
      {"DISCOVER", "GUIDEMATCH"}, {"DISCOVER", "RUNSNIPMATCH"},
      {"HASTOPIC", "NEEDTOPIC"}, {"HASTOPIC", "LISTTOPICS"}, {"HASTOPIC", "TOPICHINT"},
      {"NEEDTOPIC", "HASTOPIC"}, {"NEEDTOPIC", "LISTTOPICS"},
      {"MATCHTOPICS", "PICKTOPIC"}, {"MATCHTOPICS", "LISTTOPICS"}, {"MATCHTOPICS", "HASTOPIC"},
      {"MATCHTOPICS", "TOPICHINT"}, {"MATCHTOPICS", "GUIDE"},
      {"MATCHTOPICS", "HASMATCHTOPICS"}, {"MATCHTOPICS", "COUNTMATCHTOPICS"},
      {"PICKTOPIC", "MATCHTOPICS"}, {"PICKTOPIC", "GUIDE"}, {"PICKTOPIC", "TOPIC"},
      {"PICKTOPIC", "TOPICHINT"}, {"PICKTOPIC", "HASTOPIC"},
      {"HASMATCHTOPICS", "NEEDMATCHTOPICS"}, {"HASMATCHTOPICS", "MATCHTOPICS"},
      {"HASMATCHTOPICS", "COUNTMATCHTOPICS"}, {"HASMATCHTOPICS", "PICKTOPIC"},
      {"NEEDMATCHTOPICS", "HASMATCHTOPICS"}, {"NEEDMATCHTOPICS", "MATCHTOPICS"},
      {"COUNTMATCHTOPICS", "MATCHTOPICS"}, {"COUNTMATCHTOPICS", "HASMATCHTOPICS"},
      {"COUNTMATCHTOPICS", "PICKTOPIC"},
      {"NTHTOPIC", "PICKTOPIC"}, {"NTHTOPIC", "LASTTOPIC"}, {"NTHTOPIC", "MATCHTOPICS"},
      {"NTHTOPIC", "GUIDE"}, {"NTHTOPIC", "GUIDENTH"}, {"LASTTOPIC", "NTHTOPIC"}, {"LASTTOPIC", "PICKTOPIC"},
      {"LASTTOPIC", "MATCHTOPICS"}, {"LASTTOPIC", "GUIDE"}, {"LASTTOPIC", "GUIDELASTMATCH"},
      {"GUIDEMATCH", "GUIDENTH"}, {"GUIDEMATCH", "GUIDELASTMATCH"}, {"GUIDEMATCH", "MATCHTOPICS"},
      {"GUIDEMATCH", "GUIDE"}, {"GUIDEMATCH", "PICKTOPIC"},
      {"GUIDENTH", "GUIDEMATCH"}, {"GUIDENTH", "NTHTOPIC"}, {"GUIDENTH", "GUIDE"},
      {"GUIDELASTMATCH", "GUIDEMATCH"}, {"GUIDELASTMATCH", "LASTTOPIC"}, {"GUIDELASTMATCH", "GUIDE"},
      {"TOPIC", "RELATEDTOPIC"}, {"TOPIC", "TOPICHINT"}, {"TOPIC", "TIPS"},
      {"TOPIC", "FORMSFOR"}, {"TOPIC", "SNIP"}, {"TOPIC", "LISTTOPICS"},
      {"TOPIC", "FORMTOPICS"}, {"TOPIC", "GUIDE"},
      {"GUIDE", "TOPIC"}, {"GUIDE", "TOPICHINT"}, {"GUIDE", "RELATEDTOPIC"},
      {"GUIDE", "TIPS"}, {"GUIDE", "FORMSFOR"}, {"GUIDE", "SNIP"}, {"GUIDE", "RUNSNIP"},
      {"RUNSNIP", "GUIDE"}, {"RUNSNIP", "SNIP"}, {"RUNSNIP", "RUNSNIPMATCH"}, {"RUNSNIP", "MATCHTOPICS"},
      {"RUNSNIPMATCH", "RUNSNIPNTH"}, {"RUNSNIPMATCH", "RUNSNIPLASTMATCH"}, {"RUNSNIPMATCH", "RUNSNIP"},
      {"RUNSNIPMATCH", "GUIDEMATCH"}, {"RUNSNIPMATCH", "PICKTOPIC"},
      {"RUNSNIPNTH", "RUNSNIPMATCH"}, {"RUNSNIPNTH", "NTHTOPIC"}, {"RUNSNIPNTH", "RUNSNIP"},
      {"RUNSNIPLASTMATCH", "RUNSNIPMATCH"}, {"RUNSNIPLASTMATCH", "LASTTOPIC"}, {"RUNSNIPLASTMATCH", "RUNSNIP"},
      {"GUIDE", "GUIDEMATCH"}, {"GUIDE", "GUIDENTH"}, {"GUIDE", "GUIDELASTMATCH"}, {"GUIDE", "MATCHTOPICS"},
      {"GUIDE", "ERRGUIDE"}, {"ERRGUIDE", "GUIDE"}, {"ERRGUIDE", "ERRTIPS"},
      {"ERRGUIDE", "ERRRUN"}, {"ERRGUIDE", "WHY"}, {"ERRRUN", "ERRGUIDE"},
      {"ERRTIPS", "ERRGUIDE"},
      {"FORMTOPICS", "FORMSFOR"}, {"FORMTOPICS", "FORMHINT"}, {"FORMTOPICS", "RELATED"},
      {"FORMTOPICS", "TOPIC"}, {"FORMTOPICS", "TOPICHINT"}, {"FORMTOPICS", "LISTTOPICS"},
      {"FORMTOPICS", "GUIDE"}, {"FORMTOPICS", "FORMGUIDE"},
      {"FORMGUIDE", "FORMHINT"}, {"FORMGUIDE", "FORMTOPICS"}, {"FORMGUIDE", "GUIDE"},
      {"FORMGUIDE", "RELATED"}, {"FORMGUIDE", "TOPIC"},
      {"FORMHINT", "FORMGUIDE"}, {"FORMSFOR", "FORMTOPICS"}, {"FORMHINT", "FORMTOPICS"},
      {"RELATED", "FORMTOPICS"}, {"RELATED", "FORMGUIDE"},
      {"TOPICHINT", "GUIDE"}, {"RELATEDTOPIC", "GUIDE"}, {"TIPS", "GUIDE"},
      /* lib / include */
      {"INCLUDE", "LISTLIBS"}, {"INCLUDE", "HASLIB"}, {"INCLUDE", "MATCHLIBS"},
      {"INCLUDE", "PICKLIB"}, {"INCLUDE", "RECIPE"}, {"INCLUDE", "CHECKDEPS"},
      {"LISTLIBS", "HASLIB"}, {"LISTLIBS", "MATCHLIBS"}, {"LISTLIBS", "RECIPE"},
      {"LISTLIBS", "CATLIB"}, {"MATCHLIBS", "PICKLIB"}, {"MATCHLIBS", "NTHLIB"},
      {"MATCHLIBS", "SORTLIBS"}, {"MATCHLIBS", "FRESHLIBS"},
      {"PICKLIB", "MATCHLIBS"}, {"PICKLIB", "NTHLIB"}, {"PICKLIB", "INCLUDE"},
      {"RECIPE", "LIBDEPS"}, {"RECIPE", "LIBDEFAULTS"}, {"RECIPE", "CHECKDEPS"},
      {"RECIPE", "LIBINFO"}, {"CHECKDEPS", "HASDEPS"}, {"CHECKDEPS", "NEEDDEPS"},
      {"CHECKDEPS", "RECIPE"}, {"SORTLIBS", "NEWESTLIB"}, {"SORTLIBS", "OLDESTLIB"},
      {"SORTLIBS", "FRESHLIBS"}, {"FRESHLIBS", "STALELIBS"}, {"FRESHLIBS", "LIBAGE"},
      {"LIBAGE", "HASFRESH"}, {"LIBAGE", "NEEDFRESH"}, {"LIBAGE", "FRESHLIBS"},
      {"CATLIB", "HEADLIB"}, {"CATLIB", "TAILLIB"}, {"CATLIB", "GREPLIB"},
      /* plate */
      {"SETP", "GETP"}, {"SETP", "NEEDP"}, {"SETP", "DEFAULTP"}, {"SETP", "INCP"},
      {"SETP", "DELP"}, {"SETP", "SAVEPLATE"}, {"SETP", "LOADPLATE"},
      {"GETP", "SETP"}, {"GETP", "NEEDP"}, {"GETP", "GETPN"}, {"GETP", "HASP"},
      {"NEEDP", "SETP"}, {"NEEDP", "GETP"}, {"NEEDP", "HASP"}, {"NEEDP", "DEFAULTP"},
      {"DEFAULTP", "SETP"}, {"DEFAULTP", "NEEDP"}, {"INCP", "SETP"}, {"INCP", "GETPN"},
      {"DELP", "SETP"}, {"DELP", "GETP"}, {"PRETTYP", "DUMPP"}, {"PRETTYP", "SAVEPLATE"},
      {"DUMPP", "PRETTYP"}, {"DUMPP", "GETP"}, {"SAVEPLATE", "LOADPLATE"},
      {"SAVEPLATE", "SETP"}, {"LOADPLATE", "SAVEPLATE"}, {"LOADPLATE", "NEEDP"},
      /* fat / budget */
      {"VARROOM", "HASVARROOM"}, {"VARROOM", "NEEDVARROOM"}, {"VARROOM", "VARS"},
      {"HASVARROOM", "NEEDVARROOM"}, {"HASVARROOM", "VARROOM"},
      {"NEEDVARROOM", "HASVARROOM"}, {"NEEDVARROOM", "VARROOM"},
      {"REMAIN_MS", "HAS_TIME"}, {"REMAIN_MS", "NEEDTIME"}, {"REMAIN_MS", "WALL_MS"},
      {"HAS_TIME", "NEEDTIME"}, {"HAS_TIME", "REMAIN_MS"},
      {"NEEDTIME", "HAS_TIME"}, {"NEEDTIME", "REMAIN_MS"},
      /* run / status */
      {"ASSERT", "EXPECT"}, {"ASSERT", "WHY"}, {"ASSERT", "FAIL"}, {"ASSERT", "PASS"},
      {"EXPECT", "ASSERT"}, {"EXPECT", "WHY"}, {"EXPECT", "CLEAR_ERR"}, {"EXPECT", "FAIL"},
      {"FAIL", "PASS"}, {"FAIL", "WHY"}, {"FAIL", "CLEAR_ERR"}, {"FAIL", "HARDFAIL"},
      {"HARDFAIL", "FAIL"}, {"HARDFAIL", "EXIT"}, {"HARDFAIL", "CLEAR_ERR"}, {"HARDFAIL", "WHY"},
      {"PASS", "FAIL"}, {"PASS", "NOTE"}, {"WHY", "CLEAR_ERR"}, {"WHY", "STATUS"},
      {"WHY", "TIPS"}, {"CLEAR_ERR", "WHY"}, {"CLEAR_ERR", "STATUS"},
      {"STATUS", "IDENTITY"}, {"STATUS", "VERSION"}, {"STATUS", "VARS"}, {"STATUS", "DOCTOR"},
      {"DOCTOR", "STATUS"}, {"DOCTOR", "VERSION"}, {"DOCTOR", "OPEN"}, {"DOCTOR", "DISCOVER"},
      {"DOCTOR", "NEEDDOCTOR"}, {"NEEDDOCTOR", "DOCTOR"}, {"NEEDDOCTOR", "REQUIRE DOCTOR"},
      {"DOCTOR", "READY"}, {"READY", "DOCTOR"}, {"READY", "NEEDREADY"}, {"NEEDREADY", "READY"},
      {"VERSION", "REQUIRE VERSION"}, {"VERSION", "STATUS"},
      {"EXIT", "PASS"}, {"EXIT", "FAIL"}, {"EXIT", "STATUS"},
      {"NOTE", "PASS"}, {"NOTE", "TIPS"},
      {"START", "GUIDE"}, {"START", "TIPS"}, {"START", "LISTTOPICS"},
      {"START", "VERSION"}, {"START", "STATUS"}, {"START", "ERRGUIDE"},
      {"START", "FORMGUIDE"}, {"START", "TOPIC"},
      {"ONBOARD", "START"}, {"GUIDE", "START"}, {"TIPS", "START"},
      /* p2p */
      {"SMX", "SERVE"}, {"SMX", "DIAL"}, {"SMX", "TALK"}, {"SMX", "EXCHANGE"},
      {"SERVE", "DIAL"}, {"SERVE", "SMX"}, {"DIAL", "SERVE"}, {"DIAL", "SMX"},
      {"TALK", "SMX"}, {"TALK", "EXCHANGE"}, {"EXCHANGE", "SMX"}, {"EXCHANGE", "TALK"},
      /* protect / core */
      {"HOLD_FLASH", "STATUS"}, {"HOLD_FLASH", "VERSION"}, {"HOLD_FLASH", "TIPS"},
    };
    char name[96], nup[96], bag[4096];
    size_t k, o = 0;
    int i, n = 0, nall = (int)(sizeof edges / sizeof edges[0]);
    int seen = 0;
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
      var_set_num(vm, "RELATED_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "RELATED: need form — RELATED HASFORM · SEEALSO SETP");
      var_set_str(vm, "ERR",
                  "RELATED: need form — RELATED HASFORM · SEEALSO SETP");
      bump(vm);
      return 1;
    }
    /* normalize compare key: uppercase alnum, map space to space */
    for (k = 0; name[k] && k + 1 < sizeof nup; k++) {
      char c = name[k];
      if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
      nup[k] = c;
    }
    nup[k] = 0;
    bag[0] = 0;
    for (i = 0; i < nall; i++) {
      char fup[96];
      size_t j, ln;
      int match;
      for (j = 0; edges[i].form[j] && j + 1 < sizeof fup; j++) {
        char c = edges[i].form[j];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        fup[j] = c;
      }
      fup[j] = 0;
      match = !strcmp(fup, nup);
      if (!match) continue;
      seen = 1;
      ln = strlen(edges[i].rel);
      if (o && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, edges[i].rel, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "RELATED_OF", name);
    var_set_str(vm, "SEEALSO_OF", name);
    var_set_str(vm, "FORM", name);
    if (!seen || n == 0) {
      char em[192];
      snprintf(em, sizeof em,
               "RELATED miss: '%s' — FORMHINT · FORMSFOR · LISTFORMS · cubalc related",
               name);
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "RELATED", "");
      var_set_str(vm, "SEEALSO", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RELATED_N", 0);
      var_set_num(vm, "SEEALSO_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "RELATED", bag);
    var_set_str(vm, "SEEALSO", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "RELATED_N", n);
    var_set_num(vm, "SEEALSO_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SNIP|SNIPPET [topic] — curated mini runnable source by topic.
   * Completes agent discovery triad: TIPS (prose) · FORMSFOR (names) · SNIP (paste code).
   * Topics: general|cap|fat|plate|p2p|run|lib|protect. LAST=source · SNIP_N=lines · OK=1. */
  if (kw(&L->cur,"SNIP")||kw(&L->cur,"SNIPPET")||kw(&L->cur,"MINI")||
      kw(&L->cur,"SNIPPET_SRC")||kw(&L->cur,"STARTER_SRC")||kw(&L->cur,"MINIPROG")||
      kw(&L->cur,"CODESNIP")||kw(&L->cur,"SNIPCODE")){
    static const struct { const char *topic; const char *src; } snips[] = {
      {"general",
       "VERSION\n"
       "STATUS\n"
       "PRINT \"version\" VERSION\n"
       "PASS\n"},
      {"cap",
       "HASFORM SORTLIBS\n"
       "ASSERT LAST_N == 1 \"need SORTLIBS form\"\n"
       "FORMHINT SORTLIBS\n"
       "PRINT LAST\n"
       "RELATED HASFORM\n"
       "PRINT RELATED\n"
       "PASS\n"},
      {"fat",
       "VARROOM\n"
       "PRINT \"free_slots\" LAST_N\n"
       "HASVARROOM 8\n"
       "ASSERT LAST_N == 1\n"
       "REMAIN_MS\n"
       "PRINT \"remain_ms\" LAST_N\n"
       "PASS\n"},
      {"plate",
       "LET PLATE = \"{\\\"n\\\":0,\\\"ok\\\":true}\"\n"
       "SETP \"status\" \"ready\"\n"
       "INCP \"n\"\n"
       "NEEDP \"n\" \"ok\" \"status\"\n"
       "PRETTYP\n"
       "PRINT LAST\n"
       "PASS\n"},
      {"p2p",
       "NOTE \"set CUBALC_SMX_KEY before mesh\"\n"
       "TIPS p2p\n"
       "PRINT LAST\n"
       "FORMSFOR p2p\n"
       "PRINT LAST\n"
       "PASS\n"},
      {"run",
       "EXPECT 1 == 1\n"
       "WHY\n"
       "PRINT WHY_HINT\n"
       "CLEAR_ERR\n"
       "PASS \"run probe ok\"\n"},
      {"lib",
       "LISTLIBS\n"
       "PRINT \"libs\" LAST_N\n"
       "HASLIB agent_boot\n"
       "ASSERT LAST_N == 1\n"
       "RECIPE agent_boot\n"
       "PRINT LAST\n"
       "PASS\n"},
      {"protect",
       "HOLD_FLASH\n"
       "PRINT \"hold_flash\" LAST_N\n"
       "VERSION\n"
       "STATUS\n"
       "TIPS protect\n"
       "PRINT LAST\n"
       "PASS\n"},
    };
    char topic[32], tup[32];
    size_t k;
    int i, nall = (int)(sizeof snips / sizeof snips[0]);
    const char *src = NULL;
    int lines = 0;
    const char *p;
    lex_next(L);
    topic[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(topic, sizeof topic, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
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
    for (i = 0; i < nall; i++) {
      if (!strcmp(snips[i].topic, tup)) {
        src = snips[i].src;
        break;
      }
    }
    if (!src) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SNIP_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "SNIP: unknown topic — SNIP general|cap|fat|plate|p2p|run|lib|protect");
      var_set_str(vm, "ERR",
                  "SNIP: unknown topic — SNIP general|cap|fat|plate|p2p|run|lib|protect");
      bump(vm);
      return 1;
    }
    for (p = src; *p; p++)
      if (*p == '\n') lines++;
    if (lines == 0 && src[0]) lines = 1;
    var_set_str(vm, "LAST", src);
    var_set_str(vm, "SNIP", src);
    var_set_str(vm, "SNIPPET", src);
    var_set_str(vm, "SNIP_TOPIC", tup);
    var_set_str(vm, "SNIPPET_TOPIC", tup);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", src);
    vm->last_n = lines;
    var_set_num(vm, "LAST_N", lines);
    var_set_num(vm, "SNIP_N", lines);
    var_set_num(vm, "SNIPPET_N", lines);
    var_set_num(vm, "OK", 1);
    if (vm->res)
      snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", src);
    bump(vm);
    return 1;
  }

  /* RUNSNIP|SNIPRUN [topic] — execute curated SNIP mini program one-shot.
   * Usability: SNIP returns pasteable source; RUNSNIP runs it without temp files.
   * Nested cubalc_run_source · LAST from nested print · OK from nested plate.
   * Topics match SNIP: general|cap|fat|plate|p2p|run|lib|protect. */
  if (kw(&L->cur,"RUNSNIP")||kw(&L->cur,"SNIPRUN")||kw(&L->cur,"RUN_SNIP")||
      kw(&L->cur,"EXEC_SNIP")||kw(&L->cur,"APPLY_SNIP")||kw(&L->cur,"DOSNIP")||
      kw(&L->cur,"RUNMINI")||kw(&L->cur,"PLAYSNIP")||
      kw(&L->cur,"RUNSNIPMATCH")||kw(&L->cur,"MATCHRUNSNIP")||kw(&L->cur,"PICKRUNSNIP")||
      kw(&L->cur,"FIRSTRUNSNIP")||kw(&L->cur,"RUNSNIPPICK")||
      kw(&L->cur,"RUNSNIPNTH")||kw(&L->cur,"NTHRUNSNIP")||kw(&L->cur,"INDEXRUNSNIP")||
      kw(&L->cur,"RUNSNIPLASTMATCH")||kw(&L->cur,"LASTMATCHRUNSNIP")||
      kw(&L->cur,"RUNSNIPENDMATCH")||kw(&L->cur,"ENDMATCHRUNSNIP")){
    static const struct { const char *topic; const char *src; } snips[] = {
      {"general",
       "VERSION\n"
       "STATUS\n"
       "PRINT \"version\" VERSION\n"
       "PASS\n"},
      {"cap",
       "HASFORM SORTLIBS\n"
       "ASSERT LAST_N == 1 \"need SORTLIBS form\"\n"
       "FORMHINT SORTLIBS\n"
       "PRINT LAST\n"
       "RELATED HASFORM\n"
       "PRINT RELATED\n"
       "PASS\n"},
      {"fat",
       "VARROOM\n"
       "PRINT \"free_slots\" LAST_N\n"
       "HASVARROOM 8\n"
       "ASSERT LAST_N == 1\n"
       "REMAIN_MS\n"
       "PRINT \"remain_ms\" LAST_N\n"
       "PASS\n"},
      {"plate",
       "LET PLATE = \"{\\\"n\\\":0,\\\"ok\\\":true}\"\n"
       "SETP \"status\" \"ready\"\n"
       "INCP \"n\"\n"
       "NEEDP \"n\" \"ok\" \"status\"\n"
       "PRETTYP\n"
       "PRINT LAST\n"
       "PASS\n"},
      {"p2p",
       "NOTE \"set CUBALC_SMX_KEY before mesh\"\n"
       "TIPS p2p\n"
       "PRINT LAST\n"
       "FORMSFOR p2p\n"
       "PRINT LAST\n"
       "PASS\n"},
      {"run",
       "EXPECT 1 == 1\n"
       "WHY\n"
       "PRINT WHY_HINT\n"
       "CLEAR_ERR\n"
       "PASS \"run probe ok\"\n"},
      {"lib",
       "LISTLIBS\n"
       "PRINT \"libs\" LAST_N\n"
       "HASLIB agent_boot\n"
       "ASSERT LAST_N == 1\n"
       "RECIPE agent_boot\n"
       "PRINT LAST\n"
       "PASS\n"},
      {"protect",
       "HOLD_FLASH\n"
       "PRINT \"hold_flash\" LAST_N\n"
       "VERSION\n"
       "STATUS\n"
       "TIPS protect\n"
       "PRINT LAST\n"
       "PASS\n"},
    };
    static const char *topic_ids[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    char topic[32], tup[32];
    char m_needle[64], m_fallback[64], nlow[64];
    char matches[8][16];
    size_t k;
    int i, nall = (int)(sizeof snips / sizeof snips[0]);
    int n_topic_ids = (int)(sizeof topic_ids / sizeof topic_ids[0]);
    const char *src = NULL, *mpick = NULL;
    cubalc_run_result rr;
    int ok;
    int mode_filt = 0; /* 0 direct · 1 first · 2 nth · 3 lastmatch */
    int m_idx = 0, m_n = 0, m_have_or = 0;
    if (kw(&L->cur,"RUNSNIPMATCH")||kw(&L->cur,"MATCHRUNSNIP")||kw(&L->cur,"PICKRUNSNIP")||
        kw(&L->cur,"FIRSTRUNSNIP")||kw(&L->cur,"RUNSNIPPICK"))
      mode_filt = 1;
    else if (kw(&L->cur,"RUNSNIPNTH")||kw(&L->cur,"NTHRUNSNIP")||kw(&L->cur,"INDEXRUNSNIP"))
      mode_filt = 2;
    else if (kw(&L->cur,"RUNSNIPLASTMATCH")||kw(&L->cur,"LASTMATCHRUNSNIP")||
             kw(&L->cur,"RUNSNIPENDMATCH")||kw(&L->cur,"ENDMATCHRUNSNIP"))
      mode_filt = 3;
    lex_next(L);
    topic[0] = 0;
    m_needle[0] = 0;
    m_fallback[0] = 0;
    /* RUNSNIP MATCH|FIRST|PICK · NTH · LASTMATCH (not bare RUNSNIP LAST) */
    if (mode_filt == 0) {
      if (kw(&L->cur,"MATCH")||kw(&L->cur,"FIRST")||kw(&L->cur,"PICK")) {
        mode_filt = 1;
        lex_next(L);
      } else if (kw(&L->cur,"NTH")||kw(&L->cur,"INDEX")) {
        mode_filt = 2;
        lex_next(L);
      } else if (kw(&L->cur,"LASTMATCH")||kw(&L->cur,"ENDMATCH")||
                 kw(&L->cur,"MATCHLAST")) {
        mode_filt = 3;
        lex_next(L);
      }
    }
    if (mode_filt != 0) {
      if (mode_filt == 2) {
        long idx_l = 0;
        int have_idx = 0;
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
            (L->cur.kind == TK_IDENT && (strcmp(L->cur.text,"LAST_N")==0 ||
             strcmp(L->cur.text,"OK")==0 || strcmp(L->cur.text,"N")==0))) {
          idx_l = parse_expr(vm, L);
          have_idx = 1;
        }
        if (!have_idx) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_num(vm, "RUNSNIP_OK", 0);
          var_set_str(vm, "LAST_ERR",
                      "RUNSNIPNTH: need idx — RUNSNIP NTH 0 p · RUNSNIPNTH 1 plate");
          var_set_str(vm, "ERR",
                      "RUNSNIPNTH: need idx — RUNSNIP NTH 0 p · RUNSNIPNTH 1 plate");
          bump(vm);
          return 1;
        }
        m_idx = (int)idx_l;
      }
      if (L->cur.kind == TK_STR) {
        snprintf(m_needle, sizeof m_needle, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(m_needle, sizeof m_needle, "%s", vv->sval);
        else if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(m_needle, sizeof m_needle, "%s", vm->last_str);
        else
          snprintf(m_needle, sizeof m_needle, "%s", L->cur.text);
        lex_next(L);
      }
      if (kw(&L->cur,"OR") || kw(&L->cur,"ELSE") || kw(&L->cur,"DEFAULT")) {
        lex_next(L);
        m_have_or = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(m_fallback, sizeof m_fallback, "%s", L->cur.text);
          lex_next(L);
        } else if (L->cur.kind == TK_IDENT) {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str)
            snprintf(m_fallback, sizeof m_fallback, "%s", vv->sval);
          else
            snprintf(m_fallback, sizeof m_fallback, "%s", L->cur.text);
          lex_next(L);
        }
      }
      if (!m_needle[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "RUNSNIP_OK", 0);
        var_set_str(vm, "LAST_ERR",
                    "RUNSNIPMATCH: need needle — RUNSNIP MATCH p · RUNSNIPNTH 1 plate");
        var_set_str(vm, "ERR",
                    "RUNSNIPMATCH: need needle — RUNSNIP MATCH p · RUNSNIPNTH 1 plate");
        bump(vm);
        return 1;
      }
      for (k = 0; m_needle[k] && k + 1 < sizeof nlow; k++) {
        char ch = m_needle[k];
        if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
        nlow[k] = ch;
      }
      nlow[k] = 0;
      if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
        snprintf(nlow, sizeof nlow, "%s", "cap");
      if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
        snprintf(nlow, sizeof nlow, "%s", "p2p");
      if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
        snprintf(nlow, sizeof nlow, "%s", "fat");
      if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
        snprintf(nlow, sizeof nlow, "%s", "plate");
      if (!strcmp(nlow, "start") || !strcmp(nlow, "all") || !strcmp(nlow, "help") ||
          !strcmp(nlow, "default") || !strcmp(nlow, "*"))
        snprintf(nlow, sizeof nlow, "%s", "general");
      m_n = 0;
      for (i = 0; i < n_topic_ids && m_n < 8; i++) {
        if (!strstr(topic_ids[i], nlow)) continue;
        snprintf(matches[m_n], sizeof matches[0], "%s", topic_ids[i]);
        m_n++;
      }
      if (mode_filt == 3) {
        if (m_n > 0) { mpick = matches[m_n - 1]; m_idx = m_n - 1; }
      } else if (mode_filt == 1) {
        if (m_n > 0) { mpick = matches[0]; m_idx = 0; }
      } else {
        if (m_idx < 0) m_idx = 0;
        if (m_idx < m_n) mpick = matches[m_idx];
      }
      var_set_str(vm, "MATCHTOPICS_FILTER", nlow);
      var_set_str(vm, "RUNSNIP_FILTER", nlow);
      var_set_num(vm, "MATCHTOPICS_N", m_n);
      var_set_num(vm, "RUNSNIP_MATCH_N", m_n);
      var_set_num(vm, "RUNSNIP_MATCH_I", m_idx);
      if (!mpick) {
        if (m_have_or && m_fallback[0]) {
          snprintf(topic, sizeof topic, "%s", m_fallback);
        } else {
          char em[192];
          snprintf(em, sizeof em,
                   "RUNSNIPMATCH miss idx=%d filter='%s' n=%d — MATCHTOPICS · PICKTOPIC · OR fallback",
                   m_idx, nlow, m_n);
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_num(vm, "RUNSNIP_OK", 0);
          var_set_str(vm, "LAST_ERR", em);
          var_set_str(vm, "ERR", em);
          bump(vm);
          return 1;
        }
      } else {
        snprintf(topic, sizeof topic, "%s", mpick);
      }
      var_set_str(vm, "PICKTOPIC", topic);
      var_set_str(vm, "NTHTOPIC", topic);
      var_set_str(vm, "LASTTOPIC", topic);
      var_set_str(vm, "RUNSNIPMATCH", topic);
    } else {
      if (L->cur.kind == TK_STR) {
        snprintf(topic, sizeof topic, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
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
    }
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
    for (i = 0; i < nall; i++) {
      if (!strcmp(snips[i].topic, tup)) { src = snips[i].src; break; }
    }
    var_set_str(vm, "RUNSNIP_TOPIC", tup);
    var_set_str(vm, "TOPIC_NAME", tup);
    var_set_str(vm, "SNIPRUN_TOPIC", tup);
    if (!src) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "RUNSNIP_OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "RUNSNIP: unknown topic — RUNSNIP general|cap|fat|plate|p2p|run|lib|protect");
      var_set_str(vm, "ERR",
                  "RUNSNIP: unknown topic — RUNSNIP general|cap|fat|plate|p2p|run|lib|protect");
      bump(vm);
      return 1;
    }
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<runsnip>", &rr, vm->trace);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    var_set_str(vm, "RUNSNIP_SRC", src);
    var_set_num(vm, "RUNSNIP_OK", ok);
    var_set_num(vm, "RUNSNIP_ASSERTS_OK", rr.asserts_ok);
    var_set_num(vm, "RUNSNIP_ASSERTS_FAIL", rr.asserts_fail);
    var_set_num(vm, "RUNSNIP_STMTS", rr.stmts);
    if (rr.last_print[0]) {
      var_set_str(vm, "LAST", rr.last_print);
      var_set_str(vm, "RUNSNIP_LAST", rr.last_print);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", rr.last_print);
      vm->last_n = (long)strlen(rr.last_print);
    } else if (rr.last_err[0]) {
      var_set_str(vm, "LAST", rr.last_err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", rr.last_err);
      vm->last_n = (long)strlen(rr.last_err);
    } else if (rr.err[0]) {
      var_set_str(vm, "LAST", rr.err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", rr.err);
      vm->last_n = (long)strlen(rr.err);
    } else {
      var_set_str(vm, "LAST", ok ? "runsnip ok" : "runsnip fail");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", ok ? "runsnip ok" : "runsnip fail");
      vm->last_n = ok ? 10 : 12;
    }
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "OK", ok);
    if (!ok) {
      const char *em = rr.last_err[0] ? rr.last_err : (rr.err[0] ? rr.err : "RUNSNIP nested fail");
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->res) {
      vm->res->asserts_ok += rr.asserts_ok;
      /* do not accumulate nested asserts_fail into outer hard fail count unless soft */
      snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", vm->last_str);
    }
    if (vm->trace)
      fprintf(vm->trace, "# runsnip topic=%s ok=%d asserts_ok=%d fail=%d\n",
              tup, ok, rr.asserts_ok, rr.asserts_fail);
    bump(vm);
    return 1;
  }

  /* TOPIC|TOPICCARD [topic] — one discovery plate: tips + forms + snip.
   * Usability: agents avoid 3 calls (TIPS+FORMSFOR+SNIP). LAST=cubalc.topic.v1 JSON.
   * Bags: TOPIC_TIPS · TOPIC_FORMS · TOPIC_SNIP · counts · OK=1. */
  if (kw(&L->cur,"TOPIC")||kw(&L->cur,"TOPICCARD")||kw(&L->cur,"TOPICINFO")||
      kw(&L->cur,"PLAYCARD")||kw(&L->cur,"DISCOVERTOPIC")||kw(&L->cur,"AGENTTOPIC")||
      kw(&L->cur,"TOPIC_PLATE")||kw(&L->cur,"TOPICPACK")){
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
    static const struct { const char *topic; const char *form; } forms[] = {
      {"general", "VERSION"}, {"general", "STATUS"}, {"general", "IDENTITY"},
      {"general", "TIPS"}, {"general", "FORMSFOR"}, {"general", "SNIP"},
      {"general", "TOPIC"}, {"general", "TOPICHINT"}, {"general", "RELATEDTOPIC"},
      {"general", "FORMTOPICS"}, {"general", "GUIDE"}, {"general", "WHY"},
      {"general", "INCLUDE"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORM"}, {"cap", "HASFORMS"}, {"cap", "HASFORMANY"},
      {"cap", "NEEDFORMS"}, {"cap", "FORMHINT"}, {"cap", "LISTFORMS"},
      {"cap", "RELATED"}, {"cap", "RELATEDTOPIC"}, {"cap", "FORMTOPICS"},
      {"cap", "GUIDE"}, {"cap", "FORMSFOR"}, {"cap", "SNIP"}, {"cap", "TOPIC"},
      {"fat", "VARROOM"}, {"fat", "HASVARROOM"}, {"fat", "NEEDVARROOM"},
      {"fat", "REMAIN_MS"}, {"fat", "HAS_TIME"}, {"fat", "NEEDTIME"}, {"fat", "STATUS"},
      {"plate", "SETP"}, {"plate", "GETP"}, {"plate", "NEEDP"}, {"plate", "DEFAULTP"},
      {"plate", "INCP"}, {"plate", "PRETTYP"}, {"plate", "SAVEPLATE"}, {"plate", "LOADPLATE"},
      {"p2p", "SMX"}, {"p2p", "SERVE"}, {"p2p", "DIAL"}, {"p2p", "TALK"}, {"p2p", "EXCHANGE"},
      {"run", "ASSERT"}, {"run", "EXPECT"}, {"run", "WHY"}, {"run", "CLEAR_ERR"},
      {"run", "FAIL"}, {"run", "PASS"}, {"run", "EXIT"},
      {"lib", "LISTLIBS"}, {"lib", "HASLIB"}, {"lib", "MATCHLIBS"}, {"lib", "PICKLIB"},
      {"lib", "RECIPE"}, {"lib", "CHECKDEPS"}, {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"}, {"protect", "STATUS"}, {"protect", "VERSION"}, {"protect", "TIPS"},
    };
    static const struct { const char *topic; const char *src; } snips[] = {
      {"general", "VERSION\nSTATUS\nPRINT \"version\" VERSION\nPASS\n"},
      {"cap",
       "HASFORM SORTLIBS\nASSERT LAST_N == 1 \"need SORTLIBS form\"\n"
       "FORMHINT SORTLIBS\nPRINT LAST\nRELATED HASFORM\nPRINT RELATED\nPASS\n"},
      {"fat",
       "VARROOM\nPRINT \"free_slots\" LAST_N\nHASVARROOM 8\nASSERT LAST_N == 1\n"
       "REMAIN_MS\nPRINT \"remain_ms\" LAST_N\nPASS\n"},
      {"plate",
       "LET PLATE = \"{\\\"n\\\":0,\\\"ok\\\":true}\"\n"
       "SETP \"status\" \"ready\"\nINCP \"n\"\nNEEDP \"n\" \"ok\" \"status\"\n"
       "PRETTYP\nPRINT LAST\nPASS\n"},
      {"p2p",
       "NOTE \"set CUBALC_SMX_KEY before mesh\"\nTIPS p2p\nPRINT LAST\n"
       "FORMSFOR p2p\nPRINT LAST\nPASS\n"},
      {"run", "EXPECT 1 == 1\nWHY\nPRINT WHY_HINT\nCLEAR_ERR\nPASS \"run probe ok\"\n"},
      {"lib",
       "LISTLIBS\nPRINT \"libs\" LAST_N\nHASLIB agent_boot\nASSERT LAST_N == 1\n"
       "RECIPE agent_boot\nPRINT LAST\nPASS\n"},
      {"protect",
       "HOLD_FLASH\nPRINT \"hold_flash\" LAST_N\nVERSION\nSTATUS\n"
       "TIPS protect\nPRINT LAST\nPASS\n"},
    };
    char topic[32], tup[32];
    char tips_bag[2048], forms_bag[1024];
    char line[CUBALC_HOST_STR_MAX];
    char esc_tips[2400], esc_forms[1200], esc_snip[2400];
    const char *src = NULL;
    size_t k, o, eo;
    int i, nt = 0, nf = 0, ns = 0, lines = 0;
    const char *p;
    int n_tips = (int)(sizeof tips / sizeof tips[0]);
    int n_forms = (int)(sizeof forms / sizeof forms[0]);
    int n_snips = (int)(sizeof snips / sizeof snips[0]);
    lex_next(L);
    topic[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(topic, sizeof topic, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
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
    tips_bag[0] = 0; o = 0;
    for (i = 0; i < n_tips; i++) {
      size_t ln;
      if (strcmp(tips[i].topic, tup) != 0) continue;
      ln = strlen(tips[i].tip);
      if (o && o + 1 < sizeof tips_bag) tips_bag[o++] = '\n';
      if (o + ln < sizeof tips_bag) { memcpy(tips_bag + o, tips[i].tip, ln); o += ln; }
      tips_bag[o] = 0; nt++;
    }
    forms_bag[0] = 0; o = 0;
    for (i = 0; i < n_forms; i++) {
      size_t ln;
      if (strcmp(forms[i].topic, tup) != 0) continue;
      ln = strlen(forms[i].form);
      if (o && o + 1 < sizeof forms_bag) forms_bag[o++] = '\n';
      if (o + ln < sizeof forms_bag) { memcpy(forms_bag + o, forms[i].form, ln); o += ln; }
      forms_bag[o] = 0; nf++;
    }
    for (i = 0; i < n_snips; i++) {
      if (!strcmp(snips[i].topic, tup)) { src = snips[i].src; break; }
    }
    if (!src) src = "";
    for (p = src; *p; p++) if (*p == '\n') lines++;
    if (lines == 0 && src[0]) lines = 1;
    ns = lines;
#define CUBALC_TOPIC_ESC(dst, srcv) do { \
      eo = 0; \
      for (p = (srcv); *p && eo + 2 < sizeof(dst); p++) { \
        if (*p == '"' || *p == '\\') { (dst)[eo++] = '\\'; (dst)[eo++] = *p; } \
        else if (*p == '\n') { (dst)[eo++] = '\\'; (dst)[eo++] = 'n'; } \
        else if ((unsigned char)*p < 0x20) continue; \
        else (dst)[eo++] = *p; \
      } \
      (dst)[eo] = 0; \
    } while (0)
    CUBALC_TOPIC_ESC(esc_tips, tips_bag);
    CUBALC_TOPIC_ESC(esc_forms, forms_bag);
    CUBALC_TOPIC_ESC(esc_snip, src);
#undef CUBALC_TOPIC_ESC
    if (nt == 0 && nf == 0 && !src[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "TOPIC: unknown topic — TOPIC general|cap|fat|plate|p2p|run|lib|protect");
      var_set_str(vm, "ERR",
                  "TOPIC: unknown topic — TOPIC general|cap|fat|plate|p2p|run|lib|protect");
      bump(vm);
      return 1;
    }
    snprintf(line, sizeof line,
      "{\"schema\":\"cubalc.topic.v1\",\"ok\":true,\"topic\":\"%s\","
      "\"tips_n\":%d,\"forms_n\":%d,\"snip_lines\":%d,"
      "\"tips\":\"%s\",\"forms\":\"%s\",\"snip\":\"%s\","
      "\"version\":\"%s\","
      "\"note\":\"one plate triad · TIPS+FORMSFOR+SNIP · dual of cubalc topic\"}",
      tup, nt, nf, ns, esc_tips, esc_forms, esc_snip, CUBALC_LANG_VERSION);
    var_set_str(vm, "LAST", line);
    var_set_str(vm, "TOPIC", line);
    var_set_str(vm, "TOPICCARD", line);
    var_set_str(vm, "TOPIC_TIPS", tips_bag);
    var_set_str(vm, "TOPIC_FORMS", forms_bag);
    var_set_str(vm, "TOPIC_SNIP", src);
    var_set_str(vm, "TOPIC_NAME", tup);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", line);
    vm->last_n = nt + nf;
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "TOPIC_TIPS_N", nt);
    var_set_num(vm, "TOPIC_FORMS_N", nf);
    var_set_num(vm, "TOPIC_SNIP_N", ns);
    var_set_num(vm, "OK", 1);
    if (vm->res)
      snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", line);
    if (vm->trace)
      fprintf(vm->trace, "# topic %s tips=%d forms=%d snip_lines=%d\n", tup, nt, nf, ns);
    bump(vm);
    return 1;
  }

  /* GUIDE|PLAYGUIDE [topic] — full discovery playbook plate.
   * Usability: TOPIC triad + TOPICHINT + RELATEDTOPIC in one call so agents after
   * ERRTIPS/FORMTOPICS skip multi-form glue. LAST=cubalc.guide.v1 JSON.
   * Bags: GUIDE_HINT · GUIDE_RELATED · GUIDE_TIPS · GUIDE_FORMS · GUIDE_SNIP. */
  if (kw(&L->cur,"GUIDE")||kw(&L->cur,"PLAYGUIDE")||kw(&L->cur,"TOPICGUIDE")||
      kw(&L->cur,"AGENTGUIDE")||kw(&L->cur,"FULLTOPIC")||kw(&L->cur,"TOPICBOARD")||
      kw(&L->cur,"PLAYBOOKTOPIC")||kw(&L->cur,"GUIDE_TOPIC")||
      kw(&L->cur,"GUIDEMATCH")||kw(&L->cur,"MATCHGUIDE")||kw(&L->cur,"PICKGUIDE")||
      kw(&L->cur,"FIRSTGUIDE")||kw(&L->cur,"GUIDEPICK")||
      kw(&L->cur,"GUIDENTH")||kw(&L->cur,"NTHGUIDE")||kw(&L->cur,"INDEXGUIDE")||
      kw(&L->cur,"GUIDELASTMATCH")||kw(&L->cur,"LASTMATCHGUIDE")||
      kw(&L->cur,"GUIDEENDMATCH")||kw(&L->cur,"ENDMATCHGUIDE")){
    static const struct { const char *id; const char *hint; } hints[] = {
      {"general", "install/doctor/init surface · VERSION STATUS IDENTITY"},
      {"cap", "HASFORM/NEEDFORMS capability floor · FORMHINT · run -C"},
      {"fat", "VARROOM/REMAIN_MS fat nest budget · fat_session · run -T"},
      {"plate", "SETP/NEEDP plate agent JSON · plate_session · PRETTYP"},
      {"p2p", "SMX SERVE/DIAL mesh · CUBALC_SMX_KEY · P2P_SOFT/TIMEOUT"},
      {"run", "ASSERT/EXPECT/WHY run probes · ERRTIPS · CLEAR_ERR"},
      {"lib", "LISTLIBS/RECIPE INCLUDE discovery · MATCHLIBS · checkdeps"},
      {"protect", "HOLD_FLASH/protect status · CORE_PROTECT · device/mesh-join only"},
    };
    static const struct { const char *topic; const char *rel; } rels[] = {
      {"general", "cap"}, {"general", "lib"}, {"general", "run"}, {"general", "plate"},
      {"cap", "general"}, {"cap", "run"}, {"cap", "lib"}, {"cap", "fat"},
      {"fat", "plate"}, {"fat", "run"}, {"fat", "cap"}, {"fat", "general"},
      {"plate", "fat"}, {"plate", "run"}, {"plate", "general"}, {"plate", "lib"},
      {"p2p", "protect"}, {"p2p", "run"}, {"p2p", "general"}, {"p2p", "lib"},
      {"run", "general"}, {"run", "cap"}, {"run", "plate"}, {"run", "fat"},
      {"lib", "general"}, {"lib", "cap"}, {"lib", "plate"}, {"lib", "run"},
      {"protect", "p2p"}, {"protect", "general"}, {"protect", "run"}, {"protect", "lib"},
    };
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
    static const struct { const char *topic; const char *form; } forms[] = {
      {"general", "VERSION"}, {"general", "STATUS"}, {"general", "GUIDE"},
      {"general", "TOPIC"}, {"general", "TOPICHINT"}, {"general", "RELATEDTOPIC"},
      {"general", "TIPS"}, {"general", "FORMSFOR"}, {"general", "INCLUDE"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORMS"}, {"cap", "FORMHINT"},
      {"cap", "RELATED"}, {"cap", "GUIDE"}, {"cap", "FORMSFOR"},
      {"fat", "VARROOM"}, {"fat", "REMAIN_MS"}, {"fat", "NEEDVARROOM"}, {"fat", "GUIDE"},
      {"plate", "SETP"}, {"plate", "GETP"}, {"plate", "NEEDP"}, {"plate", "PRETTYP"},
      {"p2p", "SMX"}, {"p2p", "SERVE"}, {"p2p", "DIAL"},
      {"run", "ASSERT"}, {"run", "EXPECT"}, {"run", "WHY"}, {"run", "ERRTIPS"},
      {"lib", "LISTLIBS"}, {"lib", "RECIPE"}, {"lib", "CHECKDEPS"}, {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"}, {"protect", "STATUS"}, {"protect", "TIPS"},
    };
    static const struct { const char *topic; const char *src; } snips[] = {
      {"general", "VERSION\nSTATUS\nPRINT \"version\" VERSION\nPASS\n"},
      {"cap",
       "HASFORM SORTLIBS\nASSERT LAST_N == 1\nFORMHINT SORTLIBS\nPRINT LAST\nPASS\n"},
      {"fat", "VARROOM\nHASVARROOM 8\nASSERT LAST_N == 1\nREMAIN_MS\nPASS\n"},
      {"plate",
       "LET PLATE = \"{\\\"n\\\":0}\"\nSETP \"status\" \"ready\"\nNEEDP \"status\"\nPASS\n"},
      {"p2p", "TIPS p2p\nPRINT LAST\nFORMSFOR p2p\nPRINT LAST\nPASS\n"},
      {"run", "EXPECT 1 == 1\nWHY\nCLEAR_ERR\nPASS\n"},
      {"lib", "HASLIB agent_boot\nASSERT LAST_N == 1\nRECIPE agent_boot\nPASS\n"},
      {"protect", "HOLD_FLASH\nVERSION\nTIPS protect\nPASS\n"},
    };
    static const char *topic_ids[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    char topic[32], tup[32];
    char tips_bag[2048], forms_bag[1024], rel_bag[256];
    char line[CUBALC_HOST_STR_MAX];
    char esc_tips[2400], esc_forms[1200], esc_snip[1600], esc_hint[320], esc_rel[400];
    char m_needle[64], m_fallback[64], nlow[64];
    char matches[8][16];
    const char *src = NULL, *hint = NULL, *mpick = NULL;
    size_t k, o, eo;
    int i, nt = 0, nf = 0, nr = 0, ns = 0, lines = 0;
    int mode_filt = 0; /* 0 direct · 1 first · 2 nth · 3 lastmatch */
    int m_idx = 0, m_n = 0, m_have_or = 0;
    int n_topic_ids = (int)(sizeof topic_ids / sizeof topic_ids[0]);
    const char *p;
    int n_tips = (int)(sizeof tips / sizeof tips[0]);
    int n_forms = (int)(sizeof forms / sizeof forms[0]);
    int n_snips = (int)(sizeof snips / sizeof snips[0]);
    int n_hints = (int)(sizeof hints / sizeof hints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    if (kw(&L->cur,"GUIDEMATCH")||kw(&L->cur,"MATCHGUIDE")||kw(&L->cur,"PICKGUIDE")||
        kw(&L->cur,"FIRSTGUIDE")||kw(&L->cur,"GUIDEPICK"))
      mode_filt = 1;
    else if (kw(&L->cur,"GUIDENTH")||kw(&L->cur,"NTHGUIDE")||kw(&L->cur,"INDEXGUIDE"))
      mode_filt = 2;
    else if (kw(&L->cur,"GUIDELASTMATCH")||kw(&L->cur,"LASTMATCHGUIDE")||
             kw(&L->cur,"GUIDEENDMATCH")||kw(&L->cur,"ENDMATCHGUIDE"))
      mode_filt = 3;
    lex_next(L);
    topic[0] = 0;
    m_needle[0] = 0;
    m_fallback[0] = 0;
    /* GUIDE MATCH|FIRST|PICK · GUIDE NTH · GUIDE LASTMATCH (not bare GUIDE LAST) */
    if (mode_filt == 0) {
      if (kw(&L->cur,"MATCH")||kw(&L->cur,"FIRST")||kw(&L->cur,"PICK")) {
        mode_filt = 1;
        lex_next(L);
      } else if (kw(&L->cur,"NTH")||kw(&L->cur,"INDEX")) {
        mode_filt = 2;
        lex_next(L);
      } else if (kw(&L->cur,"LASTMATCH")||kw(&L->cur,"ENDMATCH")||
                 kw(&L->cur,"MATCHLAST")) {
        mode_filt = 3;
        lex_next(L);
      }
    }
    if (mode_filt != 0) {
      if (mode_filt == 2) {
        long idx_l = 0;
        int have_idx = 0;
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
            (L->cur.kind == TK_IDENT && (strcmp(L->cur.text,"LAST_N")==0 ||
             strcmp(L->cur.text,"OK")==0 || strcmp(L->cur.text,"N")==0))) {
          idx_l = parse_expr(vm, L);
          have_idx = 1;
        }
        if (!have_idx) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR",
                      "GUIDENTH: need idx — GUIDE NTH 0 p · GUIDENTH 1 plate");
          var_set_str(vm, "ERR",
                      "GUIDENTH: need idx — GUIDE NTH 0 p · GUIDENTH 1 plate");
          bump(vm);
          return 1;
        }
        m_idx = (int)idx_l;
      }
      if (L->cur.kind == TK_STR) {
        snprintf(m_needle, sizeof m_needle, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(m_needle, sizeof m_needle, "%s", vv->sval);
        else if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(m_needle, sizeof m_needle, "%s", vm->last_str);
        else
          snprintf(m_needle, sizeof m_needle, "%s", L->cur.text);
        lex_next(L);
      }
      if (kw(&L->cur,"OR") || kw(&L->cur,"ELSE") || kw(&L->cur,"DEFAULT")) {
        lex_next(L);
        m_have_or = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(m_fallback, sizeof m_fallback, "%s", L->cur.text);
          lex_next(L);
        } else if (L->cur.kind == TK_IDENT) {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str)
            snprintf(m_fallback, sizeof m_fallback, "%s", vv->sval);
          else
            snprintf(m_fallback, sizeof m_fallback, "%s", L->cur.text);
          lex_next(L);
        }
      }
      if (!m_needle[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR",
                    "GUIDEMATCH: need needle — GUIDE MATCH p · GUIDENTH 1 plate");
        var_set_str(vm, "ERR",
                    "GUIDEMATCH: need needle — GUIDE MATCH p · GUIDENTH 1 plate");
        bump(vm);
        return 1;
      }
      for (k = 0; m_needle[k] && k + 1 < sizeof nlow; k++) {
        char ch = m_needle[k];
        if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
        nlow[k] = ch;
      }
      nlow[k] = 0;
      if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
        snprintf(nlow, sizeof nlow, "%s", "cap");
      if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
        snprintf(nlow, sizeof nlow, "%s", "p2p");
      if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
        snprintf(nlow, sizeof nlow, "%s", "fat");
      if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
        snprintf(nlow, sizeof nlow, "%s", "plate");
      if (!strcmp(nlow, "start") || !strcmp(nlow, "all") || !strcmp(nlow, "help") ||
          !strcmp(nlow, "default") || !strcmp(nlow, "*"))
        snprintf(nlow, sizeof nlow, "%s", "general");
      m_n = 0;
      for (i = 0; i < n_topic_ids && m_n < 8; i++) {
        if (!strstr(topic_ids[i], nlow)) continue;
        snprintf(matches[m_n], sizeof matches[0], "%s", topic_ids[i]);
        m_n++;
      }
      if (mode_filt == 3) {
        if (m_n > 0) { mpick = matches[m_n - 1]; m_idx = m_n - 1; }
      } else if (mode_filt == 1) {
        if (m_n > 0) { mpick = matches[0]; m_idx = 0; }
      } else {
        if (m_idx < 0) m_idx = 0;
        if (m_idx < m_n) mpick = matches[m_idx];
      }
      var_set_str(vm, "MATCHTOPICS_FILTER", nlow);
      var_set_str(vm, "GUIDE_FILTER", nlow);
      var_set_num(vm, "MATCHTOPICS_N", m_n);
      var_set_num(vm, "GUIDE_MATCH_N", m_n);
      var_set_num(vm, "GUIDE_MATCH_I", m_idx);
      if (!mpick) {
        if (m_have_or && m_fallback[0]) {
          snprintf(topic, sizeof topic, "%s", m_fallback);
        } else {
          char em[192];
          snprintf(em, sizeof em,
                   "GUIDEMATCH miss idx=%d filter='%s' n=%d — MATCHTOPICS · PICKTOPIC · OR fallback",
                   m_idx, nlow, m_n);
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", em);
          var_set_str(vm, "ERR", em);
          bump(vm);
          return 1;
        }
      } else {
        snprintf(topic, sizeof topic, "%s", mpick);
      }
      var_set_str(vm, "PICKTOPIC", topic);
      var_set_str(vm, "NTHTOPIC", topic);
      var_set_str(vm, "LASTTOPIC", topic);
      var_set_str(vm, "GUIDEMATCH", topic);
    } else {
      /* direct topic name (preserve GUIDE LAST = use LAST string) */
      if (L->cur.kind == TK_STR) {
        snprintf(topic, sizeof topic, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
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
    }
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
    for (i = 0; i < n_hints; i++) {
      if (!strcmp(hints[i].id, tup)) { hint = hints[i].hint; break; }
    }
    rel_bag[0] = 0; o = 0;
    for (i = 0; i < n_rels; i++) {
      size_t ln;
      if (strcmp(rels[i].topic, tup) != 0) continue;
      ln = strlen(rels[i].rel);
      if (o && o + 1 < sizeof rel_bag) rel_bag[o++] = '\n';
      if (o + ln < sizeof rel_bag) { memcpy(rel_bag + o, rels[i].rel, ln); o += ln; }
      rel_bag[o] = 0; nr++;
    }
    tips_bag[0] = 0; o = 0;
    for (i = 0; i < n_tips; i++) {
      size_t ln;
      if (strcmp(tips[i].topic, tup) != 0) continue;
      ln = strlen(tips[i].tip);
      if (o && o + 1 < sizeof tips_bag) tips_bag[o++] = '\n';
      if (o + ln < sizeof tips_bag) { memcpy(tips_bag + o, tips[i].tip, ln); o += ln; }
      tips_bag[o] = 0; nt++;
    }
    forms_bag[0] = 0; o = 0;
    for (i = 0; i < n_forms; i++) {
      size_t ln;
      if (strcmp(forms[i].topic, tup) != 0) continue;
      ln = strlen(forms[i].form);
      if (o && o + 1 < sizeof forms_bag) forms_bag[o++] = '\n';
      if (o + ln < sizeof forms_bag) { memcpy(forms_bag + o, forms[i].form, ln); o += ln; }
      forms_bag[o] = 0; nf++;
    }
    for (i = 0; i < n_snips; i++) {
      if (!strcmp(snips[i].topic, tup)) { src = snips[i].src; break; }
    }
    if (!src) src = "";
    for (p = src; *p; p++) if (*p == '\n') lines++;
    if (lines == 0 && src[0]) lines = 1;
    ns = lines;
    if (!hint || (nt == 0 && nf == 0 && !src[0])) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "GUIDE: unknown topic — GUIDE general|cap|fat|plate|p2p|run|lib|protect");
      var_set_str(vm, "ERR",
                  "GUIDE: unknown topic — GUIDE general|cap|fat|plate|p2p|run|lib|protect");
      bump(vm);
      return 1;
    }
#define CUBALC_GUIDE_ESC(dst, srcv) do { \
      eo = 0; \
      for (p = (srcv); *p && eo + 2 < sizeof(dst); p++) { \
        if (*p == '"' || *p == '\\') { (dst)[eo++] = '\\'; (dst)[eo++] = *p; } \
        else if (*p == '\n') { (dst)[eo++] = '\\'; (dst)[eo++] = 'n'; } \
        else if ((unsigned char)*p < 0x20) continue; \
        else (dst)[eo++] = *p; \
      } \
      (dst)[eo] = 0; \
    } while (0)
    CUBALC_GUIDE_ESC(esc_hint, hint);
    CUBALC_GUIDE_ESC(esc_rel, rel_bag);
    CUBALC_GUIDE_ESC(esc_tips, tips_bag);
    CUBALC_GUIDE_ESC(esc_forms, forms_bag);
    CUBALC_GUIDE_ESC(esc_snip, src);
#undef CUBALC_GUIDE_ESC
    snprintf(line, sizeof line,
      "{\"schema\":\"cubalc.guide.v1\",\"ok\":true,\"topic\":\"%s\","
      "\"hint\":\"%s\",\"related_n\":%d,\"tips_n\":%d,\"forms_n\":%d,\"snip_lines\":%d,"
      "\"related\":\"%s\",\"tips\":\"%s\",\"forms\":\"%s\",\"snip\":\"%s\","
      "\"version\":\"%s\","
      "\"note\":\"full playbook · MATCH|NTH|LASTMATCH filter pick · dual of cubalc guide\"}",
      tup, esc_hint, nr, nt, nf, ns, esc_rel, esc_tips, esc_forms, esc_snip,
      CUBALC_LANG_VERSION);
    var_set_str(vm, "LAST", line);
    var_set_str(vm, "GUIDE", line);
    var_set_str(vm, "PLAYGUIDE", line);
    var_set_str(vm, "GUIDE_HINT", hint);
    var_set_str(vm, "GUIDE_RELATED", rel_bag);
    var_set_str(vm, "GUIDE_TIPS", tips_bag);
    var_set_str(vm, "GUIDE_FORMS", forms_bag);
    var_set_str(vm, "GUIDE_SNIP", src);
    var_set_str(vm, "TOPIC_NAME", tup);
    var_set_str(vm, "TOPIC_HINT", hint);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", line);
    vm->last_n = nt + nf + nr;
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "GUIDE_RELATED_N", nr);
    var_set_num(vm, "GUIDE_TIPS_N", nt);
    var_set_num(vm, "GUIDE_FORMS_N", nf);
    var_set_num(vm, "GUIDE_SNIP_N", ns);
    var_set_num(vm, "OK", 1);
    if (vm->res)
      snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", "cubalc.guide.v1");
    if (vm->trace)
      fprintf(vm->trace, "# guide %s related=%d tips=%d forms=%d\n", tup, nr, nt, nf);
    bump(vm);
    return 1;
  }

  /* LISTTOPICS|TOPICS — bag of curated discovery topics (dual of cubalc topics).
   * Complements TIPS/FORMSFOR/SNIP/TOPIC/RUNSNIP — agents discover topic names
   * without hardcoding. LAST=newline bag · LAST_N=count · OK=1.
   * HASTOPIC|NEEDTOPIC name — soft/hard membership gate (twin of HASFORM). */
  if (kw(&L->cur,"LISTTOPICS")||kw(&L->cur,"TOPICS")||kw(&L->cur,"TOPICLIST")||
      kw(&L->cur,"LIST_TOPICS")||kw(&L->cur,"DISCOVERY_TOPICS")){
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    char bag[256];
    size_t o = 0;
    int i, n = (int)(sizeof topics / sizeof topics[0]);
    lex_next(L);
    bag[0] = 0;
    for (i = 0; i < n; i++) {
      size_t ln = strlen(topics[i]);
      if (o && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) { memcpy(bag + o, topics[i], ln); o += ln; }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTTOPICS", bag);
    var_set_str(vm, "TOPICS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "LISTTOPICS_N", n);
    var_set_num(vm, "TOPICS_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }
  if (kw(&L->cur,"HASTOPIC")||kw(&L->cur,"KNOWNTOPIC")||kw(&L->cur,"ISTOPIC")||
      kw(&L->cur,"NEEDTOPIC")||kw(&L->cur,"REQUIRETOPIC")||kw(&L->cur,"REQUIRE TOPIC")){
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    int hard = kw(&L->cur,"NEEDTOPIC")||kw(&L->cur,"REQUIRETOPIC")||
               kw(&L->cur,"REQUIRE TOPIC");
    char name[32], nup[32], tup[32];
    size_t k;
    int i, nall = (int)(sizeof topics / sizeof topics[0]), hit = 0;
    int aln = L->cur.line;
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
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "HASTOPIC_N", 0);
      var_set_str(vm, "LAST_ERR",
                  "HASTOPIC: need name — HASTOPIC cap · NEEDTOPIC plate");
      var_set_str(vm, "ERR",
                  "HASTOPIC: need name — HASTOPIC cap · NEEDTOPIC plate");
      if (hard) { fail(vm, "NEEDTOPIC: need name"); return -1; }
      bump(vm);
      return 1;
    }
    for (k = 0; name[k] && k + 1 < sizeof tup; k++) {
      char c = name[k];
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      tup[k] = c;
    }
    tup[k] = 0;
    /* aliases match TIPS/TOPIC surface */
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
    for (i = 0; i < nall; i++) {
      if (!strcmp(topics[i], tup)) { hit = 1; break; }
    }
    var_set_str(vm, "TOPIC_NAME", tup);
    var_set_str(vm, "HASTOPIC_OF", tup);
    var_set_num(vm, "LAST_N", hit ? 1 : 0);
    vm->last_n = hit ? 1 : 0;
    var_set_num(vm, "HASTOPIC_N", hit ? 1 : 0);
    var_set_str(vm, "LAST", hit ? "1" : "0");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hit ? "1" : "0");
    var_set_num(vm, "OK", hit ? 1 : 0);
    if (!hit) {
      char em[160];
      snprintf(em, sizeof em,
               "%s miss: '%s' — LISTTOPICS · TOPIC · TIPS · cubalc topics",
               hard ? "NEEDTOPIC" : "HASTOPIC", name);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      if (hard) {
        fail(vm, em);
        return -1;
      }
    }
    (void)aln;
    bump(vm);
    return 1;
  }


  /* DISCOVER|EXPLORE [needle] — discovery open plate.
   * Usability: MATCHTOPICS bag + first-topic TOPICHINT + next-step commands in one
   * call so agents skip MATCHTOPICS+PICKTOPIC+TOPICHINT+GUIDE glue after onboard.
   * No needle → all topics, first=general. Soft empty OK=0.
   * LAST=cubalc.discover.v1 · bags DISCOVER_TOPICS · DISCOVER_NEXT · DISCOVER_FIRST. */
  if (kw(&L->cur,"DISCOVER")||kw(&L->cur,"EXPLORE")||kw(&L->cur,"DISCOVERTOPICS")||
      kw(&L->cur,"OPENDISCOVER")||kw(&L->cur,"TOPICDISCOVER")||kw(&L->cur,"SCOUT")){
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    static const struct { const char *id; const char *hint; } hints[] = {
      {"general", "install/doctor/init surface · VERSION STATUS IDENTITY"},
      {"cap", "HASFORM/NEEDFORMS capability floor · FORMHINT · run -C"},
      {"fat", "VARROOM/REMAIN_MS fat nest budget · fat_session · run -T"},
      {"plate", "SETP/NEEDP plate agent JSON · plate_session · PRETTYP"},
      {"p2p", "SMX SERVE/DIAL mesh · CUBALC_SMX_KEY · P2P_SOFT/TIMEOUT"},
      {"run", "ASSERT/EXPECT/WHY run probes · ERRTIPS · CLEAR_ERR"},
      {"lib", "LISTLIBS/RECIPE INCLUDE discovery · MATCHLIBS · checkdeps"},
      {"protect", "HOLD_FLASH/protect status · CORE_PROTECT · device/mesh-join only"},
    };
    static const struct { const char *topic; const char *rel; } rels[] = {
      {"general", "cap"}, {"general", "lib"}, {"general", "run"}, {"general", "plate"},
      {"cap", "general"}, {"cap", "run"}, {"cap", "lib"}, {"cap", "fat"},
      {"fat", "plate"}, {"fat", "run"}, {"fat", "cap"}, {"fat", "general"},
      {"plate", "fat"}, {"plate", "run"}, {"plate", "general"}, {"plate", "lib"},
      {"p2p", "protect"}, {"p2p", "run"}, {"p2p", "general"}, {"p2p", "lib"},
      {"run", "general"}, {"run", "cap"}, {"run", "plate"}, {"run", "fat"},
      {"lib", "general"}, {"lib", "cap"}, {"lib", "plate"}, {"lib", "run"},
      {"protect", "p2p"}, {"protect", "general"}, {"protect", "run"}, {"protect", "lib"},
    };
    char needle[64], nlow[64], bag[256], rel_bag[256], next_bag[768];
    char line[CUBALC_HOST_STR_MAX];
    char esc_hint[320], esc_bag[400], esc_rel[400], esc_next[900];
    char matches[8][16];
    const char *first = NULL, *hint = NULL;
    size_t k, o, eo;
    int i, n = 0, nr = 0, nn = 0;
    int nall = (int)(sizeof topics / sizeof topics[0]);
    int n_hints = (int)(sizeof hints / sizeof hints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    int have_filter = 0;
    const char *p;
    lex_next(L);
    needle[0] = 0;
    nlow[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
      have_filter = 1;
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0]) {
        snprintf(needle, sizeof needle, "%s", vv->sval);
        lex_next(L);
        have_filter = 1;
      } else if (strcmp(L->cur.text, "LAST") == 0) {
        snprintf(needle, sizeof needle, "%s", vm->last_str);
        lex_next(L);
        have_filter = 1;
      } else if (!(kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
                   kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"PASS") ||
                   kw(&L->cur,"FAIL") || kw(&L->cur,"NOTE") || kw(&L->cur,"STATUS") ||
                   kw(&L->cur,"GUIDE") || kw(&L->cur,"RUNSNIP"))) {
        snprintf(needle, sizeof needle, "%s", L->cur.text);
        lex_next(L);
        have_filter = 1;
      }
    }
    if (have_filter && needle[0]) {
      for (k = 0; needle[k] && k + 1 < sizeof nlow; k++) {
        char ch = needle[k];
        if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
        nlow[k] = ch;
      }
      nlow[k] = 0;
      if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
        snprintf(nlow, sizeof nlow, "%s", "cap");
      if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
        snprintf(nlow, sizeof nlow, "%s", "p2p");
      if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
        snprintf(nlow, sizeof nlow, "%s", "fat");
      if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
        snprintf(nlow, sizeof nlow, "%s", "plate");
      if (!strcmp(nlow, "start") || !strcmp(nlow, "all") || !strcmp(nlow, "help") ||
          !strcmp(nlow, "default") || !strcmp(nlow, "*"))
        snprintf(nlow, sizeof nlow, "%s", "general");
      for (i = 0; i < nall && n < 8; i++) {
        if (!strstr(topics[i], nlow)) continue;
        snprintf(matches[n], sizeof matches[0], "%s", topics[i]);
        n++;
      }
    } else {
      snprintf(nlow, sizeof nlow, "%s", "");
      for (i = 0; i < nall && n < 8; i++) {
        snprintf(matches[n], sizeof matches[0], "%s", topics[i]);
        n++;
      }
    }
    bag[0] = 0; o = 0;
    for (i = 0; i < n; i++) {
      size_t ln = strlen(matches[i]);
      if (o && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) { memcpy(bag + o, matches[i], ln); o += ln; }
      bag[o] = 0;
    }
    var_set_str(vm, "DISCOVER_FILTER", nlow);
    var_set_str(vm, "MATCHTOPICS_FILTER", nlow[0] ? nlow : "*");
    var_set_num(vm, "DISCOVER_N", n);
    var_set_num(vm, "MATCHTOPICS_N", n);
    var_set_str(vm, "DISCOVER_TOPICS", bag);
    var_set_str(vm, "MATCHTOPICS", bag);
    if (n == 0) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "DISCOVER", "");
      var_set_str(vm, "DISCOVER_FIRST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "DISCOVER: empty — LISTTOPICS · MATCHTOPICS · cubalc discover");
      var_set_str(vm, "ERR",
                  "DISCOVER: empty — LISTTOPICS · MATCHTOPICS · cubalc discover");
      bump(vm);
      return 1;
    }
    first = matches[0];
    for (i = 0; i < n_hints; i++) {
      if (!strcmp(hints[i].id, first)) { hint = hints[i].hint; break; }
    }
    if (!hint) hint = "";
    rel_bag[0] = 0; o = 0; nr = 0;
    for (i = 0; i < n_rels; i++) {
      size_t ln;
      if (strcmp(rels[i].topic, first) != 0) continue;
      ln = strlen(rels[i].rel);
      if (o && o + 1 < sizeof rel_bag) rel_bag[o++] = '\n';
      if (o + ln < sizeof rel_bag) { memcpy(rel_bag + o, rels[i].rel, ln); o += ln; }
      rel_bag[o] = 0; nr++;
    }
    /* next-step command bag for agents */
    next_bag[0] = 0; o = 0; nn = 0;
    {
      char cmds[6][96];
      int ci, nc;
      snprintf(cmds[0], sizeof cmds[0], "GUIDE %s", first);
      snprintf(cmds[1], sizeof cmds[1], "RUNSNIP %s", first);
      snprintf(cmds[2], sizeof cmds[2], "TOPIC %s", first);
      snprintf(cmds[3], sizeof cmds[3], "TOPICHINT %s", first);
      if (nlow[0]) {
        snprintf(cmds[4], sizeof cmds[4], "GUIDE MATCH %s", nlow);
        snprintf(cmds[5], sizeof cmds[5], "RUNSNIP MATCH %s", nlow);
        nc = 6;
      } else {
        snprintf(cmds[4], sizeof cmds[4], "cubalc discover");
        snprintf(cmds[5], sizeof cmds[5], "LISTTOPICS");
        nc = 6;
      }
      for (ci = 0; ci < nc; ci++) {
        size_t ln = strlen(cmds[ci]);
        if (o && o + 1 < sizeof next_bag) next_bag[o++] = '\n';
        if (o + ln < sizeof next_bag) { memcpy(next_bag + o, cmds[ci], ln); o += ln; }
        next_bag[o] = 0; nn++;
      }
    }
#define CUBALC_DISC_ESC(dst, srcv) do { \
      eo = 0; \
      for (p = (srcv); *p && eo + 2 < sizeof(dst); p++) { \
        if (*p == '"' || *p == '\\') { (dst)[eo++] = '\\'; (dst)[eo++] = *p; } \
        else if (*p == '\n') { (dst)[eo++] = '\\'; (dst)[eo++] = 'n'; } \
        else if ((unsigned char)*p < 0x20) continue; \
        else (dst)[eo++] = *p; \
      } \
      (dst)[eo] = 0; \
    } while (0)
    CUBALC_DISC_ESC(esc_hint, hint);
    CUBALC_DISC_ESC(esc_bag, bag);
    CUBALC_DISC_ESC(esc_rel, rel_bag);
    CUBALC_DISC_ESC(esc_next, next_bag);
#undef CUBALC_DISC_ESC
    snprintf(line, sizeof line,
      "{\"schema\":\"cubalc.discover.v1\",\"ok\":true,\"filter\":\"%s\","
      "\"n\":%d,\"first\":\"%s\",\"hint\":\"%s\",\"related_n\":%d,\"next_n\":%d,"
      "\"topics\":\"%s\",\"related\":\"%s\",\"next\":\"%s\","
      "\"version\":\"%s\","
      "\"note\":\"match bag + first open · chain GUIDE/RUNSNIP DISCOVER_FIRST · dual of cubalc discover\"}",
      nlow[0] ? nlow : "*", n, first, esc_hint, nr, nn,
      esc_bag, esc_rel, esc_next, CUBALC_LANG_VERSION);
    var_set_str(vm, "LAST", line);
    var_set_str(vm, "DISCOVER", line);
    var_set_str(vm, "EXPLORE", line);
    var_set_str(vm, "DISCOVER_FIRST", first);
    var_set_str(vm, "DISCOVER_HINT", hint);
    var_set_str(vm, "DISCOVER_RELATED", rel_bag);
    var_set_str(vm, "DISCOVER_NEXT", next_bag);
    var_set_str(vm, "TOPIC_NAME", first);
    var_set_str(vm, "TOPIC_HINT", hint);
    var_set_str(vm, "PICKTOPIC", first);
    var_set_str(vm, "TOPICHINT", hint);
    var_set_num(vm, "DISCOVER_RELATED_N", nr);
    var_set_num(vm, "DISCOVER_NEXT_N", nn);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", line);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "OK", 1);
    if (vm->res)
      snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", "cubalc.discover.v1");
    if (vm->trace)
      fprintf(vm->trace, "# discover filter=%s n=%d first=%s\n",
              nlow[0] ? nlow : "*", n, first);
    bump(vm);
    return 1;
  }


  /* OPEN|OPENPLAY [needle] — DISCOVER match bag + full GUIDE playbook for first.
   * Usability: open filter surface without DISCOVER then GUIDE DISCOVER_FIRST glue.
   * Soft empty OK=0. LAST=cubalc.open.v1 · sets DISCOVER_* and GUIDE_* bags. */
  if (kw(&L->cur,"OPEN")||kw(&L->cur,"OPENPLAY")||kw(&L->cur,"OPENGUIDE")||
      kw(&L->cur,"PLAYOPEN")||kw(&L->cur,"OPEN_TOPIC")||kw(&L->cur,"DISCOVERGUIDE")){
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    static const struct { const char *id; const char *hint; } hints[] = {
      {"general", "install/doctor/init surface · VERSION STATUS IDENTITY"},
      {"cap", "HASFORM/NEEDFORMS capability floor · FORMHINT · run -C"},
      {"fat", "VARROOM/REMAIN_MS fat nest budget · fat_session · run -T"},
      {"plate", "SETP/NEEDP plate agent JSON · plate_session · PRETTYP"},
      {"p2p", "SMX SERVE/DIAL mesh · CUBALC_SMX_KEY · P2P_SOFT/TIMEOUT"},
      {"run", "ASSERT/EXPECT/WHY run probes · ERRTIPS · CLEAR_ERR"},
      {"lib", "LISTLIBS/RECIPE INCLUDE discovery · MATCHLIBS · checkdeps"},
      {"protect", "HOLD_FLASH/protect status · CORE_PROTECT · device/mesh-join only"},
    };
    static const struct { const char *topic; const char *rel; } rels[] = {
      {"general", "cap"}, {"general", "lib"}, {"general", "run"}, {"general", "plate"},
      {"cap", "general"}, {"cap", "run"}, {"cap", "lib"}, {"cap", "fat"},
      {"fat", "plate"}, {"fat", "run"}, {"fat", "cap"}, {"fat", "general"},
      {"plate", "fat"}, {"plate", "run"}, {"plate", "general"}, {"plate", "lib"},
      {"p2p", "protect"}, {"p2p", "run"}, {"p2p", "general"}, {"p2p", "lib"},
      {"run", "general"}, {"run", "cap"}, {"run", "plate"}, {"run", "fat"},
      {"lib", "general"}, {"lib", "cap"}, {"lib", "plate"}, {"lib", "run"},
      {"protect", "p2p"}, {"protect", "general"}, {"protect", "run"}, {"protect", "lib"},
    };
    static const struct { const char *topic; const char *tip; } tips[] = {
      {"general", "cubalc doctor — install readiness plate"},
      {"general", "cubalc init --list — scaffold catalog"},
      {"general", "INCLUDE discover_boot — DISCOVER open plate"},
      {"general", "OPEN general — full playbook one-shot"},
      {"cap", "INCLUDE cap_boot — agent_boot + form_guard"},
      {"cap", "cubalc init --cap · HASFORM · NEEDFORMS"},
      {"fat", "INCLUDE fat_session durable nest"},
      {"fat", "cubalc run -T ms · REMAIN_MS · VARROOM"},
      {"plate", "INCLUDE plate_session · SETP NEEDP SAVEPLATE"},
      {"p2p", "export CUBALC_SMX_KEY hex64 · SMX SERVE/DIAL"},
      {"run", "cubalc run -q -e CODE · WHY after ASSERT"},
      {"lib", "cubalc libs|recipe|checkdeps · INCLUDE MATCH"},
      {"protect", "cubalc protect status · HOLD_FLASH device/mesh-join only"},
    };
    static const struct { const char *topic; const char *form; } forms[] = {
      {"general", "VERSION"}, {"general", "STATUS"}, {"general", "OPEN"},
      {"general", "DISCOVER"}, {"general", "GUIDE"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORMS"}, {"cap", "FORMHINT"},
      {"fat", "VARROOM"}, {"fat", "REMAIN_MS"},
      {"plate", "SETP"}, {"plate", "NEEDP"}, {"plate", "PRETTYP"},
      {"p2p", "SMX"}, {"p2p", "SERVE"}, {"p2p", "DIAL"},
      {"run", "ASSERT"}, {"run", "EXPECT"}, {"run", "WHY"},
      {"lib", "LISTLIBS"}, {"lib", "RECIPE"}, {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"}, {"protect", "STATUS"},
    };
    static const struct { const char *topic; const char *src; } snips[] = {
      {"general", "VERSION\nSTATUS\nPASS\n"},
      {"cap", "HASFORM SORTLIBS\nFORMHINT SORTLIBS\nPASS\n"},
      {"fat", "VARROOM\nHASVARROOM 8\nPASS\n"},
      {"plate", "SETP \"status\" \"ready\"\nNEEDP \"status\"\nPASS\n"},
      {"p2p", "TIPS p2p\nFORMSFOR p2p\nPASS\n"},
      {"run", "EXPECT 1 == 1\nWHY\nCLEAR_ERR\nPASS\n"},
      {"lib", "HASLIB agent_boot\nRECIPE agent_boot\nPASS\n"},
      {"protect", "HOLD_FLASH\nVERSION\nPASS\n"},
    };
    char needle[64], nlow[64], bag[256], rel_bag[256], tips_bag[2048], forms_bag[1024];
    char next_bag[768], line[CUBALC_HOST_STR_MAX];
    char esc_hint[320], esc_bag[400], esc_rel[400], esc_tips[2400], esc_forms[1200];
    char esc_snip[1600], esc_next[900];
    char matches[8][16];
    const char *first = NULL, *hint = NULL, *src = NULL, *pp;
    size_t k, o, eo;
    int i, n = 0, nr = 0, nt = 0, nf = 0, ns = 0, nn = 0, lines = 0, have_filter = 0;
    int nall = (int)(sizeof topics / sizeof topics[0]);
    int n_hints = (int)(sizeof hints / sizeof hints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    int n_tips = (int)(sizeof tips / sizeof tips[0]);
    int n_forms = (int)(sizeof forms / sizeof forms[0]);
    int n_snips = (int)(sizeof snips / sizeof snips[0]);
    lex_next(L);
    needle[0] = 0; nlow[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text); lex_next(L); have_filter = 1;
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0]) {
        snprintf(needle, sizeof needle, "%s", vv->sval); lex_next(L); have_filter = 1;
      } else if (strcmp(L->cur.text, "LAST") == 0) {
        snprintf(needle, sizeof needle, "%s", vm->last_str); lex_next(L); have_filter = 1;
      } else if (!(kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
                   kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"PASS") ||
                   kw(&L->cur,"FAIL") || kw(&L->cur,"NOTE") || kw(&L->cur,"STATUS"))) {
        snprintf(needle, sizeof needle, "%s", L->cur.text); lex_next(L); have_filter = 1;
      }
    }
    if (have_filter && needle[0]) {
      for (k = 0; needle[k] && k + 1 < sizeof nlow; k++) {
        char ch = needle[k];
        if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
        nlow[k] = ch;
      }
      nlow[k] = 0;
      if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
        snprintf(nlow, sizeof nlow, "%s", "cap");
      if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
        snprintf(nlow, sizeof nlow, "%s", "p2p");
      if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
        snprintf(nlow, sizeof nlow, "%s", "fat");
      if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
        snprintf(nlow, sizeof nlow, "%s", "plate");
      if (!strcmp(nlow, "start") || !strcmp(nlow, "all") || !strcmp(nlow, "help") ||
          !strcmp(nlow, "default") || !strcmp(nlow, "*"))
        snprintf(nlow, sizeof nlow, "%s", "general");
      for (i = 0; i < nall && n < 8; i++) {
        if (!strstr(topics[i], nlow)) continue;
        snprintf(matches[n], sizeof matches[0], "%s", topics[i]); n++;
      }
    } else {
      for (i = 0; i < nall && n < 8; i++) {
        snprintf(matches[n], sizeof matches[0], "%s", topics[i]); n++;
      }
    }
    bag[0] = 0; o = 0;
    for (i = 0; i < n; i++) {
      size_t ln = strlen(matches[i]);
      if (o && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) { memcpy(bag + o, matches[i], ln); o += ln; }
      bag[o] = 0;
    }
    var_set_str(vm, "DISCOVER_FILTER", nlow);
    var_set_str(vm, "OPEN_FILTER", nlow[0] ? nlow : "*");
    var_set_num(vm, "DISCOVER_N", n);
    var_set_num(vm, "OPEN_N", n);
    var_set_str(vm, "DISCOVER_TOPICS", bag);
    var_set_str(vm, "OPEN_TOPICS", bag);
    if (n == 0) {
      var_set_str(vm, "LAST", ""); vm->last_str[0] = 0; vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0); var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "OPEN: empty — DISCOVER · LISTTOPICS · cubalc open");
      var_set_str(vm, "ERR", "OPEN: empty — DISCOVER · LISTTOPICS · cubalc open");
      bump(vm); return 1;
    }
    first = matches[0];
    for (i = 0; i < n_hints; i++)
      if (!strcmp(hints[i].id, first)) { hint = hints[i].hint; break; }
    if (!hint) hint = "";
    rel_bag[0] = 0; o = 0; nr = 0;
    for (i = 0; i < n_rels; i++) {
      size_t ln; if (strcmp(rels[i].topic, first) != 0) continue;
      ln = strlen(rels[i].rel);
      if (o && o + 1 < sizeof rel_bag) rel_bag[o++] = '\n';
      if (o + ln < sizeof rel_bag) { memcpy(rel_bag + o, rels[i].rel, ln); o += ln; }
      rel_bag[o] = 0; nr++;
    }
    tips_bag[0] = 0; o = 0; nt = 0;
    for (i = 0; i < n_tips; i++) {
      size_t ln; if (strcmp(tips[i].topic, first) != 0) continue;
      ln = strlen(tips[i].tip);
      if (o && o + 1 < sizeof tips_bag) tips_bag[o++] = '\n';
      if (o + ln < sizeof tips_bag) { memcpy(tips_bag + o, tips[i].tip, ln); o += ln; }
      tips_bag[o] = 0; nt++;
    }
    forms_bag[0] = 0; o = 0; nf = 0;
    for (i = 0; i < n_forms; i++) {
      size_t ln; if (strcmp(forms[i].topic, first) != 0) continue;
      ln = strlen(forms[i].form);
      if (o && o + 1 < sizeof forms_bag) forms_bag[o++] = '\n';
      if (o + ln < sizeof forms_bag) { memcpy(forms_bag + o, forms[i].form, ln); o += ln; }
      forms_bag[o] = 0; nf++;
    }
    for (i = 0; i < n_snips; i++)
      if (!strcmp(snips[i].topic, first)) { src = snips[i].src; break; }
    if (!src) src = "";
    for (pp = src; *pp; pp++) if (*pp == '\n') lines++;
    if (lines == 0 && src[0]) lines = 1;
    ns = lines;
    next_bag[0] = 0; o = 0; nn = 0;
    {
      char cmds[4][96]; int ci;
      snprintf(cmds[0], sizeof cmds[0], "RUNSNIP %s", first);
      snprintf(cmds[1], sizeof cmds[1], "GUIDE %s", first);
      snprintf(cmds[2], sizeof cmds[2], "TOPIC %s", first);
      if (nlow[0]) snprintf(cmds[3], sizeof cmds[3], "DISCOVER %s", nlow);
      else snprintf(cmds[3], sizeof cmds[3], "DISCOVER");
      for (ci = 0; ci < 4; ci++) {
        size_t ln = strlen(cmds[ci]);
        if (o && o + 1 < sizeof next_bag) next_bag[o++] = '\n';
        if (o + ln < sizeof next_bag) { memcpy(next_bag + o, cmds[ci], ln); o += ln; }
        next_bag[o] = 0; nn++;
      }
    }
#define CUBALC_OPEN_ESC(dst, srcv) do { \
      eo = 0; for (pp = (srcv); *pp && eo + 2 < sizeof(dst); pp++) { \
        if (*pp == '"' || *pp == '\\') { (dst)[eo++] = '\\'; (dst)[eo++] = *pp; } \
        else if (*pp == '\n') { (dst)[eo++] = '\\'; (dst)[eo++] = 'n'; } \
        else if ((unsigned char)*pp < 0x20) continue; \
        else (dst)[eo++] = *pp; } (dst)[eo] = 0; } while (0)
    CUBALC_OPEN_ESC(esc_hint, hint);
    CUBALC_OPEN_ESC(esc_bag, bag);
    CUBALC_OPEN_ESC(esc_rel, rel_bag);
    CUBALC_OPEN_ESC(esc_tips, tips_bag);
    CUBALC_OPEN_ESC(esc_forms, forms_bag);
    CUBALC_OPEN_ESC(esc_snip, src);
    CUBALC_OPEN_ESC(esc_next, next_bag);
#undef CUBALC_OPEN_ESC
    snprintf(line, sizeof line,
      "{\"schema\":\"cubalc.open.v1\",\"ok\":true,\"filter\":\"%s\","
      "\"n\":%d,\"first\":\"%s\",\"hint\":\"%s\","
      "\"related_n\":%d,\"tips_n\":%d,\"forms_n\":%d,\"snip_lines\":%d,\"next_n\":%d,"
      "\"topics\":\"%s\",\"related\":\"%s\",\"tips\":\"%s\",\"forms\":\"%s\","
      "\"snip\":\"%s\",\"next\":\"%s\",\"version\":\"%s\","
      "\"note\":\"DISCOVER bag + GUIDE playbook for first · dual of cubalc open\"}",
      nlow[0] ? nlow : "*", n, first, esc_hint, nr, nt, nf, ns, nn,
      esc_bag, esc_rel, esc_tips, esc_forms, esc_snip, esc_next, CUBALC_LANG_VERSION);
    var_set_str(vm, "LAST", line);
    var_set_str(vm, "OPEN", line);
    var_set_str(vm, "OPENPLAY", line);
    var_set_str(vm, "DISCOVER_FIRST", first);
    var_set_str(vm, "OPEN_FIRST", first);
    var_set_str(vm, "DISCOVER_HINT", hint);
    var_set_str(vm, "DISCOVER_RELATED", rel_bag);
    var_set_str(vm, "DISCOVER_NEXT", next_bag);
    var_set_str(vm, "GUIDE_HINT", hint);
    var_set_str(vm, "GUIDE_RELATED", rel_bag);
    var_set_str(vm, "GUIDE_TIPS", tips_bag);
    var_set_str(vm, "GUIDE_FORMS", forms_bag);
    var_set_str(vm, "GUIDE_SNIP", src);
    var_set_str(vm, "TOPIC_NAME", first);
    var_set_str(vm, "TOPIC_HINT", hint);
    var_set_str(vm, "PICKTOPIC", first);
    var_set_str(vm, "TOPICHINT", hint);
    var_set_num(vm, "DISCOVER_RELATED_N", nr);
    var_set_num(vm, "DISCOVER_NEXT_N", nn);
    var_set_num(vm, "GUIDE_RELATED_N", nr);
    var_set_num(vm, "GUIDE_TIPS_N", nt);
    var_set_num(vm, "GUIDE_FORMS_N", nf);
    var_set_num(vm, "GUIDE_SNIP_N", ns);
    var_set_num(vm, "OPEN_TIPS_N", nt);
    var_set_num(vm, "OPEN_FORMS_N", nf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", line);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "OK", 1);
    if (vm->res)
      snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", "cubalc.open.v1");
    if (vm->trace)
      fprintf(vm->trace, "# open filter=%s n=%d first=%s\n", nlow[0] ? nlow : "*", n, first);
    bump(vm);
    return 1;
  }

  /* MATCHTOPICS|FILTERTOPICS needle — bag of discovery topics matching filter.
   * Dual of MATCHLIBS for the fixed topic catalog. Soft empty OK=0.
   * Dual of cubalc matchtopics. */
  if (kw(&L->cur,"MATCHTOPICS")||kw(&L->cur,"FILTERTOPICS")||kw(&L->cur,"TOPICMATCH")||
      kw(&L->cur,"MATCH_TOPICS")||kw(&L->cur,"TOPICFILTER")||kw(&L->cur,"GREPTOPICS")){
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    char needle[64], nlow[64], bag[256];
    size_t k, o = 0;
    int i, n = 0, nall = (int)(sizeof topics / sizeof topics[0]);
    lex_next(L);
    needle[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(needle, sizeof needle, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
      else
        snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MATCHTOPICS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "MATCHTOPICS: need needle — MATCHTOPICS cap · FILTERTOPICS p");
      var_set_str(vm, "ERR",
                  "MATCHTOPICS: need needle — MATCHTOPICS cap · FILTERTOPICS p");
      bump(vm);
      return 1;
    }
    for (k = 0; needle[k] && k + 1 < sizeof nlow; k++) {
      char ch = needle[k];
      if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
      nlow[k] = ch;
    }
    nlow[k] = 0;
    if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
      snprintf(nlow, sizeof nlow, "%s", "cap");
    if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
      snprintf(nlow, sizeof nlow, "%s", "p2p");
    if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
      snprintf(nlow, sizeof nlow, "%s", "fat");
    if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
      snprintf(nlow, sizeof nlow, "%s", "plate");
    if (!strcmp(nlow, "start") || !strcmp(nlow, "all") || !strcmp(nlow, "help") ||
        !strcmp(nlow, "default") || !strcmp(nlow, "*"))
      snprintf(nlow, sizeof nlow, "%s", "general");
    bag[0] = 0;
    for (i = 0; i < nall; i++) {
      size_t ln;
      if (!strstr(topics[i], nlow)) continue;
      ln = strlen(topics[i]);
      if (o && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) { memcpy(bag + o, topics[i], ln); o += ln; }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "MATCHTOPICS_FILTER", nlow);
    if (n == 0) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "MATCHTOPICS", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MATCHTOPICS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "MATCHTOPICS: empty — LISTTOPICS · HASTOPIC · cubalc topics");
      var_set_str(vm, "ERR",
                  "MATCHTOPICS: empty — LISTTOPICS · HASTOPIC · cubalc topics");
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "MATCHTOPICS", bag);
    var_set_str(vm, "FILTERTOPICS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "MATCHTOPICS_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* PICKTOPIC|FIRSTTOPIC needle [OR fallback] — first matching discovery topic.
   * Dual of PICKLIB. Soft miss OK=0 · OR fallback. Dual of cubalc picktopic. */
  if (kw(&L->cur,"PICKTOPIC")||kw(&L->cur,"FIRSTTOPIC")||kw(&L->cur,"TOPICPICK")||
      kw(&L->cur,"PICK_TOPIC")||kw(&L->cur,"CHOOSETOPIC")||kw(&L->cur,"SELECTTOPIC")){
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    char needle[64], nlow[64], fallback[64];
    const char *pick = NULL;
    size_t k;
    int i, nall = (int)(sizeof topics / sizeof topics[0]);
    int have_or = 0;
    lex_next(L);
    needle[0] = 0;
    fallback[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(needle, sizeof needle, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
      else
        snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    }
    if (kw(&L->cur,"OR") || kw(&L->cur,"ELSE") || kw(&L->cur,"DEFAULT")) {
      lex_next(L);
      have_or = 1;
      if (L->cur.kind == TK_STR) {
        snprintf(fallback, sizeof fallback, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str)
          snprintf(fallback, sizeof fallback, "%s", vv->sval);
        else
          snprintf(fallback, sizeof fallback, "%s", L->cur.text);
        lex_next(L);
      }
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "PICKTOPIC: need needle — PICKTOPIC cap · FIRSTTOPIC plate OR general");
      var_set_str(vm, "ERR",
                  "PICKTOPIC: need needle — PICKTOPIC cap · FIRSTTOPIC plate OR general");
      bump(vm);
      return 1;
    }
    for (k = 0; needle[k] && k + 1 < sizeof nlow; k++) {
      char ch = needle[k];
      if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
      nlow[k] = ch;
    }
    nlow[k] = 0;
    if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
      snprintf(nlow, sizeof nlow, "%s", "cap");
    if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
      snprintf(nlow, sizeof nlow, "%s", "p2p");
    if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
      snprintf(nlow, sizeof nlow, "%s", "fat");
    if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
      snprintf(nlow, sizeof nlow, "%s", "plate");
    if (!strcmp(nlow, "start") || !strcmp(nlow, "all") || !strcmp(nlow, "help") ||
        !strcmp(nlow, "default") || !strcmp(nlow, "*"))
      snprintf(nlow, sizeof nlow, "%s", "general");
    for (i = 0; i < nall; i++) {
      if (strstr(topics[i], nlow)) { pick = topics[i]; break; }
    }
    var_set_str(vm, "PICKTOPIC_FILTER", nlow);
    if (!pick) {
      if (have_or && fallback[0]) {
        var_set_str(vm, "LAST", fallback);
        var_set_str(vm, "PICKTOPIC", fallback);
        var_set_str(vm, "TOPIC_NAME", fallback);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", fallback);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_num(vm, "OK", 1);
        bump(vm);
        return 1;
      }
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "PICKTOPIC", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "PICKTOPIC miss — MATCHTOPICS · LISTTOPICS · PICKTOPIC x OR general");
      var_set_str(vm, "ERR",
                  "PICKTOPIC miss — MATCHTOPICS · LISTTOPICS · PICKTOPIC x OR general");
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", pick);
    var_set_str(vm, "PICKTOPIC", pick);
    var_set_str(vm, "FIRSTTOPIC", pick);
    var_set_str(vm, "TOPIC_NAME", pick);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
    vm->last_n = 1;
    var_set_num(vm, "LAST_N", 1);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }


  /* HASMATCHTOPICS / NEEDMATCHTOPICS / COUNTMATCHTOPICS needle —
   * soft 0|1 presence · fail-fast · count without bag (dual of MATCHLIBS triad gates).
   * Dual of cubalc hasmatchtopics|needmatchtopics|countmatchtopics. */
  if (kw(&L->cur,"HASMATCHTOPICS")||kw(&L->cur,"HASTOPICMATCH")||kw(&L->cur,"TOPICSMATCH")||
      kw(&L->cur,"NEEDMATCHTOPICS")||kw(&L->cur,"NEEDTOPICMATCH")||
      kw(&L->cur,"COUNTMATCHTOPICS")||kw(&L->cur,"COUNTTOPICS")||
      kw(&L->cur,"COUNTTOPICMATCH")||kw(&L->cur,"NTOPICMATCH")){
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    int hard = kw(&L->cur,"NEEDMATCHTOPICS")||kw(&L->cur,"NEEDTOPICMATCH");
    int mode_count = kw(&L->cur,"COUNTMATCHTOPICS")||kw(&L->cur,"COUNTTOPICS")||
                     kw(&L->cur,"COUNTTOPICMATCH")||kw(&L->cur,"NTOPICMATCH");
    char needle[64], nlow[64];
    size_t k;
    int i, n = 0, nall = (int)(sizeof topics / sizeof topics[0]);
    int aln = L->cur.line;
    lex_next(L);
    needle[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(needle, sizeof needle, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
      else
        snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    }
    if (!needle[0]) {
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "HASMATCHTOPICS_N", 0);
      var_set_num(vm, "COUNTMATCHTOPICS_N", 0);
      var_set_str(vm, "LAST_ERR",
                  "HASMATCHTOPICS: need needle — HASMATCHTOPICS p · NEEDMATCHTOPICS cap");
      var_set_str(vm, "ERR",
                  "HASMATCHTOPICS: need needle — HASMATCHTOPICS p · NEEDMATCHTOPICS cap");
      if (hard) { fail(vm, "NEEDMATCHTOPICS: need needle"); return -1; }
      bump(vm);
      return 1;
    }
    for (k = 0; needle[k] && k + 1 < sizeof nlow; k++) {
      char ch = needle[k];
      if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
      nlow[k] = ch;
    }
    nlow[k] = 0;
    if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
      snprintf(nlow, sizeof nlow, "%s", "cap");
    if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
      snprintf(nlow, sizeof nlow, "%s", "p2p");
    if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
      snprintf(nlow, sizeof nlow, "%s", "fat");
    if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
      snprintf(nlow, sizeof nlow, "%s", "plate");
    if (!strcmp(nlow, "start") || !strcmp(nlow, "all") || !strcmp(nlow, "help") ||
        !strcmp(nlow, "default") || !strcmp(nlow, "*"))
      snprintf(nlow, sizeof nlow, "%s", "general");
    for (i = 0; i < nall; i++) {
      if (strstr(topics[i], nlow)) n++;
    }
    var_set_str(vm, "MATCHTOPICS_FILTER", nlow);
    var_set_str(vm, "HASMATCHTOPICS_FILTER", nlow);
    var_set_num(vm, "MATCHTOPICS_N", n);
    var_set_num(vm, "COUNTMATCHTOPICS_N", n);
    var_set_num(vm, "HASMATCHTOPICS_N", n > 0 ? 1 : 0);
    if (mode_count) {
      char nb[24];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    /* has / need presence */
    var_set_str(vm, "LAST", n > 0 ? "1" : "0");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", n > 0 ? "1" : "0");
    vm->last_n = n > 0 ? 1 : 0;
    var_set_num(vm, "LAST_N", n > 0 ? 1 : 0);
    var_set_num(vm, "OK", n > 0 ? 1 : 0);
    if (n == 0) {
      char em[176];
      snprintf(em, sizeof em,
               "%s miss: '%s' — MATCHTOPICS · LISTTOPICS · cubalc matchtopics",
               hard ? "NEEDMATCHTOPICS" : "HASMATCHTOPICS", needle);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      if (hard) {
        fail(vm, em);
        return -1;
      }
    }
    (void)aln;
    bump(vm);
    return 1;
  }


  /* NTHTOPIC idx needle [OR fallback] · LASTTOPIC|ENDTOPIC needle [OR fallback]
   * 0-based index / last matching discovery topic (dual of NTHLIB/LASTLIB).
   * Usability: no MATCHTOPICS+NTH/POP glue. Soft miss OK=0. Dual of cubalc nthtopic|lasttopic. */
  if (kw(&L->cur,"NTHTOPIC")||kw(&L->cur,"INDEXTOPIC")||kw(&L->cur,"TOPICNTH")||
      kw(&L->cur,"LASTTOPIC")||kw(&L->cur,"ENDTOPIC")||kw(&L->cur,"TOPICLAST")){
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    int mode_last = kw(&L->cur,"LASTTOPIC")||kw(&L->cur,"ENDTOPIC")||kw(&L->cur,"TOPICLAST");
    char needle[64], nlow[64], fallback[64];
    char matches[8][16];
    const char *pick = NULL;
    size_t k;
    int i, n = 0, nall = (int)(sizeof topics / sizeof topics[0]);
    int idx = 0, have_or = 0, have_idx = 0;
    long idx_l = 0;
    lex_next(L);
    needle[0] = 0;
    fallback[0] = 0;
    if (!mode_last) {
      /* NTHTOPIC idx needle */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          (L->cur.kind == TK_IDENT && (strcmp(L->cur.text,"LAST_N")==0 ||
           strcmp(L->cur.text,"OK")==0 || strcmp(L->cur.text,"N")==0))) {
        idx_l = parse_expr(vm, L);
        have_idx = 1;
      }
      if (!have_idx) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR",
                    "NTHTOPIC: need idx — NTHTOPIC 0 p · NTHTOPIC 1 plate");
        var_set_str(vm, "ERR",
                    "NTHTOPIC: need idx — NTHTOPIC 0 p · NTHTOPIC 1 plate");
        bump(vm);
        return 1;
      }
      idx = (int)idx_l;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(needle, sizeof needle, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
      else
        snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    }
    if (kw(&L->cur,"OR") || kw(&L->cur,"ELSE") || kw(&L->cur,"DEFAULT")) {
      lex_next(L);
      have_or = 1;
      if (L->cur.kind == TK_STR) {
        snprintf(fallback, sizeof fallback, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str)
          snprintf(fallback, sizeof fallback, "%s", vv->sval);
        else
          snprintf(fallback, sizeof fallback, "%s", L->cur.text);
        lex_next(L);
      }
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  mode_last
                    ? "LASTTOPIC: need needle — LASTTOPIC p · ENDTOPIC plate OR general"
                    : "NTHTOPIC: need needle — NTHTOPIC 0 p · NTHTOPIC 1 plate");
      var_set_str(vm, "ERR",
                  mode_last
                    ? "LASTTOPIC: need needle — LASTTOPIC p · ENDTOPIC plate OR general"
                    : "NTHTOPIC: need needle — NTHTOPIC 0 p · NTHTOPIC 1 plate");
      bump(vm);
      return 1;
    }
    for (k = 0; needle[k] && k + 1 < sizeof nlow; k++) {
      char ch = needle[k];
      if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
      nlow[k] = ch;
    }
    nlow[k] = 0;
    if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
      snprintf(nlow, sizeof nlow, "%s", "cap");
    if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
      snprintf(nlow, sizeof nlow, "%s", "p2p");
    if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
      snprintf(nlow, sizeof nlow, "%s", "fat");
    if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
      snprintf(nlow, sizeof nlow, "%s", "plate");
    if (!strcmp(nlow, "start") || !strcmp(nlow, "all") || !strcmp(nlow, "help") ||
        !strcmp(nlow, "default") || !strcmp(nlow, "*"))
      snprintf(nlow, sizeof nlow, "%s", "general");
    for (i = 0; i < nall && n < 8; i++) {
      if (!strstr(topics[i], nlow)) continue;
      snprintf(matches[n], sizeof matches[0], "%s", topics[i]);
      n++;
    }
    var_set_str(vm, "MATCHTOPICS_FILTER", nlow);
    var_set_num(vm, "MATCHTOPICS_N", n);
    var_set_num(vm, "NTHTOPIC_N", n);
    if (mode_last) {
      if (n > 0) pick = matches[n - 1];
      idx = n > 0 ? n - 1 : 0;
    } else {
      if (idx < 0) idx = 0;
      if (idx < n) pick = matches[idx];
    }
    var_set_num(vm, "NTHTOPIC_I", idx);
    if (!pick) {
      if (have_or && fallback[0]) {
        var_set_str(vm, "LAST", fallback);
        var_set_str(vm, "NTHTOPIC", fallback);
        var_set_str(vm, "LASTTOPIC", fallback);
        var_set_str(vm, "TOPIC_NAME", fallback);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", fallback);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_num(vm, "OK", 1);
        bump(vm);
        return 1;
      }
      {
        char em[192];
        snprintf(em, sizeof em,
                 "%s miss idx=%d filter='%s' n=%d — MATCHTOPICS · PICKTOPIC · OR fallback",
                 mode_last ? "LASTTOPIC" : "NTHTOPIC", idx, nlow, n);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "NTHTOPIC", "");
        var_set_str(vm, "LASTTOPIC", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", em);
        var_set_str(vm, "ERR", em);
        bump(vm);
        return 1;
      }
    }
    var_set_str(vm, "LAST", pick);
    var_set_str(vm, "NTHTOPIC", pick);
    var_set_str(vm, "LASTTOPIC", pick);
    var_set_str(vm, "ENDTOPIC", pick);
    var_set_str(vm, "TOPIC_NAME", pick);
    var_set_str(vm, "PICKTOPIC", pick);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
    vm->last_n = 1;
    var_set_num(vm, "LAST_N", 1);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* TOPICHINT|DESCRIBETOPIC name — one-line discovery topic hint → LAST.
   * Twin of FORMHINT for topics. Soft miss OK=0 · TOPIC_HINT/TOPIC_NAME set.
   * Dual of cubalc topichint. Complements LISTTOPICS/HASTOPIC/TOPIC plate. */
  if (kw(&L->cur,"TOPICHINT")||kw(&L->cur,"DESCRIBETOPIC")||kw(&L->cur,"TOPICDOC")||
      kw(&L->cur,"TOPICHELP")||kw(&L->cur,"HINTTOPIC")||kw(&L->cur,"TOPIC_HINT")||
      kw(&L->cur,"WHATISTOPIC")||kw(&L->cur,"EXPLAINTOPIC")){
    static const struct { const char *id; const char *hint; } rows[] = {
      {"general", "install/doctor/init surface · VERSION STATUS IDENTITY"},
      {"cap", "HASFORM/NEEDFORMS capability floor · FORMHINT · run -C"},
      {"fat", "VARROOM/REMAIN_MS fat nest budget · fat_session · run -T"},
      {"plate", "SETP/NEEDP plate agent JSON · plate_session · PRETTYP"},
      {"p2p", "SMX SERVE/DIAL mesh · CUBALC_SMX_KEY · P2P_SOFT/TIMEOUT"},
      {"run", "ASSERT/EXPECT/WHY run probes · ERRTIPS · CLEAR_ERR"},
      {"lib", "LISTLIBS/RECIPE INCLUDE discovery · MATCHLIBS · checkdeps"},
      {"protect", "HOLD_FLASH/protect status · CORE_PROTECT · device/mesh-join only"},
    };
    char name[32], tup[32];
    const char *hint = NULL;
    size_t k;
    int i, nall = (int)(sizeof rows / sizeof rows[0]);
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
                  "TOPICHINT: need name — TOPICHINT cap · DESCRIBETOPIC plate");
      var_set_str(vm, "ERR",
                  "TOPICHINT: need name — TOPICHINT cap · DESCRIBETOPIC plate");
      bump(vm);
      return 1;
    }
    for (k = 0; name[k] && k + 1 < sizeof tup; k++) {
      char c = name[k];
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
    for (i = 0; i < nall; i++) {
      if (!strcmp(rows[i].id, tup)) { hint = rows[i].hint; break; }
    }
    var_set_str(vm, "TOPIC_NAME", tup);
    var_set_str(vm, "TOPIC", tup);
    if (!hint) {
      char em[176];
      snprintf(em, sizeof em,
               "TOPICHINT miss: '%s' — LISTTOPICS · HASTOPIC · cubalc topics",
               name);
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "TOPIC_HINT", "");
      var_set_str(vm, "TOPICHINT", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", hint);
    var_set_str(vm, "TOPIC_HINT", hint);
    var_set_str(vm, "TOPICHINT", hint);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hint);
    vm->last_n = (long)strlen(hint);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* RELATEDTOPIC|SEETOPICS name — related discovery topic bag (twin of RELATED for topics).
   * Complements TOPICHINT (one-line docs) and LISTTOPICS (catalog): topic → neighbors.
   * LAST = newline related topics · RELATEDTOPIC_N · RELATEDTOPIC_OF · soft miss OK=0.
   * Dual of cubalc relatedtopic. */
  if (kw(&L->cur,"RELATEDTOPIC")||kw(&L->cur,"SEETOPICS")||kw(&L->cur,"TOPICREL")||
      kw(&L->cur,"NEARTOPICS")||kw(&L->cur,"TOPICSNEAR")||kw(&L->cur,"RELATEDTOPICS")||
      kw(&L->cur,"TOPICNEIGHBORS")||kw(&L->cur,"TOPICSEEALSO")){
    static const struct { const char *topic; const char *rel; } edges[] = {
      {"general", "cap"}, {"general", "lib"}, {"general", "run"}, {"general", "plate"},
      {"cap", "general"}, {"cap", "run"}, {"cap", "lib"}, {"cap", "fat"},
      {"fat", "plate"}, {"fat", "run"}, {"fat", "cap"}, {"fat", "general"},
      {"plate", "fat"}, {"plate", "run"}, {"plate", "general"}, {"plate", "lib"},
      {"p2p", "protect"}, {"p2p", "run"}, {"p2p", "general"}, {"p2p", "lib"},
      {"run", "general"}, {"run", "cap"}, {"run", "plate"}, {"run", "fat"},
      {"lib", "general"}, {"lib", "cap"}, {"lib", "plate"}, {"lib", "run"},
      {"protect", "p2p"}, {"protect", "general"}, {"protect", "run"}, {"protect", "lib"},
    };
    char name[32], tup[32], bag[512];
    size_t k, o = 0;
    int i, n = 0, nall = (int)(sizeof edges / sizeof edges[0]);
    int seen = 0;
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
      var_set_num(vm, "RELATEDTOPIC_N", 0);
      var_set_num(vm, "SEETOPICS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "RELATEDTOPIC: need topic — RELATEDTOPIC cap · SEETOPICS plate");
      var_set_str(vm, "ERR",
                  "RELATEDTOPIC: need topic — RELATEDTOPIC cap · SEETOPICS plate");
      bump(vm);
      return 1;
    }
    for (k = 0; name[k] && k + 1 < sizeof tup; k++) {
      char c = name[k];
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
      size_t ln;
      if (strcmp(edges[i].topic, tup) != 0) continue;
      seen = 1;
      ln = strlen(edges[i].rel);
      if (o && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, edges[i].rel, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "RELATEDTOPIC_OF", tup);
    var_set_str(vm, "SEETOPICS_OF", tup);
    var_set_str(vm, "TOPIC_NAME", tup);
    var_set_str(vm, "TOPIC", tup);
    if (!seen || n == 0) {
      char em[192];
      snprintf(em, sizeof em,
               "RELATEDTOPIC miss: '%s' — LISTTOPICS · HASTOPIC · cubalc topics",
               name);
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "RELATEDTOPIC", "");
      var_set_str(vm, "SEETOPICS", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RELATEDTOPIC_N", 0);
      var_set_num(vm, "SEETOPICS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "RELATEDTOPIC", bag);
    var_set_str(vm, "SEETOPICS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "RELATEDTOPIC_N", n);
    var_set_num(vm, "SEETOPICS_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* FORMTOPICS|TOPICSOF form — reverse of FORMSFOR: form → topics that list it.
   * Usability: after FORMHINT/RELATED, agents learn which discovery topics cover a form
   * without scanning FORMSFOR for every topic. LAST=newline topics · FORMTOPICS_N ·
   * FORMTOPICS_OF · soft miss OK=0. Dual of cubalc formtopics. */
  if (kw(&L->cur,"FORMTOPICS")||kw(&L->cur,"TOPICSOF")||kw(&L->cur,"TOPICSFOR")||
      kw(&L->cur,"FORMTOPIC")||kw(&L->cur,"WHERETOPIC")||kw(&L->cur,"COVERTOPICS")||
      kw(&L->cur,"TOPICCOVER")||kw(&L->cur,"FORM_TOPICS")||kw(&L->cur,"TOPICSOFFORM")){
    /* Keep in sync with FORMSFOR rows (+ discovery extras for reverse lookup). */
    static const struct { const char *topic; const char *form; } rows[] = {
      {"general", "VERSION"}, {"general", "STATUS"}, {"general", "IDENTITY"},
      {"general", "TIPS"}, {"general", "FORMSFOR"}, {"general", "LISTTOPICS"},
      {"general", "TOPICHINT"}, {"general", "RELATEDTOPIC"}, {"general", "FORMTOPICS"},
      {"general", "WHY"}, {"general", "CLEAR_ERR"}, {"general", "INCLUDE"},
      {"general", "REQUIRE VERSION"}, {"general", "VARS"}, {"general", "TOPIC"},
      {"general", "SNIP"}, {"general", "RUNSNIP"}, {"general", "ERRTIPS"},
      {"general", "ERRRUN"}, {"general", "HASTOPIC"}, {"general", "NEEDTOPIC"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORM"}, {"cap", "HASFORMS"}, {"cap", "HASFORMANY"},
      {"cap", "NEEDFORMS"}, {"cap", "FORMHINT"}, {"cap", "LISTFORMS"},
      {"cap", "COUNTFORMS"}, {"cap", "REQUIRE FORM"}, {"cap", "RELATEDTOPIC"},
      {"cap", "FORMTOPICS"}, {"cap", "TIPS"}, {"cap", "FORMSFOR"},
      {"cap", "RELATED"}, {"cap", "SNIP"}, {"cap", "TOPIC"},
      {"fat", "VARROOM"}, {"fat", "HASVARROOM"}, {"fat", "NEEDVARROOM"},
      {"fat", "REMAIN_MS"}, {"fat", "HAS_TIME"}, {"fat", "NEEDTIME"},
      {"fat", "STATUS"}, {"fat", "VARS"}, {"fat", "TIPS"},
      {"plate", "SETP"}, {"plate", "GETP"}, {"plate", "NEEDP"},
      {"plate", "DEFAULTP"}, {"plate", "INCP"}, {"plate", "DELP"},
      {"plate", "PRETTYP"}, {"plate", "DUMPP"}, {"plate", "SAVEPLATE"},
      {"plate", "LOADPLATE"},
      {"p2p", "SMX"}, {"p2p", "SERVE"}, {"p2p", "DIAL"}, {"p2p", "TALK"},
      {"p2p", "EXCHANGE"}, {"p2p", "TIPS"},
      {"run", "ASSERT"}, {"run", "EXPECT"}, {"run", "FAIL"}, {"run", "PASS"},
      {"run", "NOTE"}, {"run", "EXIT"}, {"run", "WHY"}, {"run", "CLEAR_ERR"},
      {"run", "TIPS"}, {"run", "ERRTIPS"}, {"run", "ERRRUN"},
      {"lib", "LISTLIBS"}, {"lib", "HASLIB"}, {"lib", "MATCHLIBS"},
      {"lib", "PICKLIB"}, {"lib", "RECIPE"}, {"lib", "CHECKDEPS"},
      {"lib", "SORTLIBS"}, {"lib", "FRESHLIBS"}, {"lib", "LIBAGE"},
      {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"}, {"protect", "STATUS"}, {"protect", "VERSION"},
      {"protect", "TIPS"},
    };
    char name[96], nup[96], bag[512];
    char seen_topics[16][32];
    size_t k, o = 0;
    int i, n = 0, nall = (int)(sizeof rows / sizeof rows[0]);
    int nseen = 0;
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
      var_set_num(vm, "FORMTOPICS_N", 0);
      var_set_num(vm, "TOPICSOF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "FORMTOPICS: need form — FORMTOPICS HASFORM · TOPICSOF SETP");
      var_set_str(vm, "ERR",
                  "FORMTOPICS: need form — FORMTOPICS HASFORM · TOPICSOF SETP");
      bump(vm);
      return 1;
    }
    for (k = 0; name[k] && k + 1 < sizeof nup; k++) {
      char c = name[k];
      if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
      nup[k] = c;
    }
    nup[k] = 0;
    bag[0] = 0;
    for (i = 0; i < nall; i++) {
      char fup[96];
      size_t j, ln;
      int match, already = 0, si;
      for (j = 0; rows[i].form[j] && j + 1 < sizeof fup; j++) {
        char c = rows[i].form[j];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        fup[j] = c;
      }
      fup[j] = 0;
      match = !strcmp(fup, nup);
      if (!match) continue;
      for (si = 0; si < nseen; si++) {
        if (!strcmp(seen_topics[si], rows[i].topic)) { already = 1; break; }
      }
      if (already) continue;
      if (nseen < (int)(sizeof seen_topics / sizeof seen_topics[0])) {
        snprintf(seen_topics[nseen], sizeof seen_topics[0], "%s", rows[i].topic);
        nseen++;
      }
      ln = strlen(rows[i].topic);
      if (o && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, rows[i].topic, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "FORMTOPICS_OF", name);
    var_set_str(vm, "TOPICSOF_OF", name);
    var_set_str(vm, "FORM", name);
    if (n == 0) {
      char em[192];
      snprintf(em, sizeof em,
               "FORMTOPICS miss: '%s' — FORMHINT · FORMSFOR · LISTFORMS · cubalc formtopics",
               name);
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "FORMTOPICS", "");
      var_set_str(vm, "TOPICSOF", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "FORMTOPICS_N", 0);
      var_set_num(vm, "TOPICSOF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "FORMTOPICS", bag);
    var_set_str(vm, "TOPICSOF", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "FORMTOPICS_N", n);
    var_set_num(vm, "TOPICSOF_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* FORMGUIDE|GUIDEFORM form — FORMHINT + FORMTOPICS + first-topic GUIDE one-shot.
   * Usability: agents with a form name get docs + covering topics + playbook without
   * FORMHINT+FORMTOPICS+GUIDE glue. LAST=cubalc.formguide.v1 · dual of cubalc formguide. */
  if (kw(&L->cur,"FORMGUIDE")||kw(&L->cur,"GUIDEFORM")||kw(&L->cur,"FORM_GUIDE")||
      kw(&L->cur,"PLAYFORM")||kw(&L->cur,"FORMBOARD")||kw(&L->cur,"DOCFORMGUIDE")||
      kw(&L->cur,"FORMCARD")||kw(&L->cur,"GUIDE_FORM")){
    static const struct { const char *topic; const char *form; } rows[] = {
      {"general", "VERSION"}, {"general", "STATUS"}, {"general", "IDENTITY"},
      {"general", "TIPS"}, {"general", "FORMSFOR"}, {"general", "LISTTOPICS"},
      {"general", "TOPICHINT"}, {"general", "RELATEDTOPIC"}, {"general", "FORMTOPICS"},
      {"general", "FORMGUIDE"}, {"general", "GUIDE"}, {"general", "WHY"},
      {"general", "CLEAR_ERR"}, {"general", "INCLUDE"}, {"general", "REQUIRE VERSION"},
      {"general", "VARS"}, {"general", "TOPIC"}, {"general", "SNIP"},
      {"general", "RUNSNIP"}, {"general", "ERRTIPS"}, {"general", "ERRRUN"},
      {"general", "ERRGUIDE"}, {"general", "HASTOPIC"}, {"general", "NEEDTOPIC"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORM"}, {"cap", "HASFORMS"}, {"cap", "HASFORMANY"},
      {"cap", "NEEDFORMS"}, {"cap", "FORMHINT"}, {"cap", "LISTFORMS"},
      {"cap", "COUNTFORMS"}, {"cap", "REQUIRE FORM"}, {"cap", "RELATEDTOPIC"},
      {"cap", "FORMTOPICS"}, {"cap", "FORMGUIDE"}, {"cap", "TIPS"},
      {"cap", "FORMSFOR"}, {"cap", "RELATED"}, {"cap", "SNIP"}, {"cap", "TOPIC"},
      {"cap", "GUIDE"},
      {"fat", "VARROOM"}, {"fat", "HASVARROOM"}, {"fat", "NEEDVARROOM"},
      {"fat", "REMAIN_MS"}, {"fat", "HAS_TIME"}, {"fat", "NEEDTIME"},
      {"fat", "STATUS"}, {"fat", "VARS"}, {"fat", "TIPS"}, {"fat", "GUIDE"},
      {"plate", "SETP"}, {"plate", "GETP"}, {"plate", "NEEDP"},
      {"plate", "DEFAULTP"}, {"plate", "INCP"}, {"plate", "DELP"},
      {"plate", "PRETTYP"}, {"plate", "DUMPP"}, {"plate", "SAVEPLATE"},
      {"plate", "LOADPLATE"},
      {"p2p", "SMX"}, {"p2p", "SERVE"}, {"p2p", "DIAL"}, {"p2p", "TALK"},
      {"p2p", "EXCHANGE"}, {"p2p", "TIPS"},
      {"run", "ASSERT"}, {"run", "EXPECT"}, {"run", "FAIL"}, {"run", "PASS"},
      {"run", "NOTE"}, {"run", "EXIT"}, {"run", "WHY"}, {"run", "CLEAR_ERR"},
      {"run", "TIPS"}, {"run", "ERRTIPS"}, {"run", "ERRRUN"}, {"run", "ERRGUIDE"},
      {"lib", "LISTLIBS"}, {"lib", "HASLIB"}, {"lib", "MATCHLIBS"},
      {"lib", "PICKLIB"}, {"lib", "RECIPE"}, {"lib", "CHECKDEPS"},
      {"lib", "SORTLIBS"}, {"lib", "FRESHLIBS"}, {"lib", "LIBAGE"},
      {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"}, {"protect", "STATUS"}, {"protect", "VERSION"},
      {"protect", "TIPS"},
    };
    static const struct { const char *id; const char *hint; } thints[] = {
      {"general", "install/doctor/init surface · VERSION STATUS IDENTITY"},
      {"cap", "HASFORM/NEEDFORMS capability floor · FORMHINT · run -C"},
      {"fat", "VARROOM/REMAIN_MS fat nest budget · fat_session · run -T"},
      {"plate", "SETP/NEEDP plate agent JSON · plate_session · PRETTYP"},
      {"p2p", "SMX SERVE/DIAL mesh · CUBALC_SMX_KEY · P2P_SOFT/TIMEOUT"},
      {"run", "ASSERT/EXPECT/WHY run probes · ERRTIPS · CLEAR_ERR"},
      {"lib", "LISTLIBS/RECIPE INCLUDE discovery · MATCHLIBS · checkdeps"},
      {"protect", "HOLD_FLASH/protect status · CORE_PROTECT · device/mesh-join only"},
    };
    static const struct { const char *topic; const char *rel; } rels[] = {
      {"general", "cap"}, {"general", "lib"}, {"general", "run"}, {"general", "plate"},
      {"cap", "general"}, {"cap", "run"}, {"cap", "lib"}, {"cap", "fat"},
      {"fat", "plate"}, {"fat", "run"}, {"fat", "cap"}, {"fat", "general"},
      {"plate", "fat"}, {"plate", "run"}, {"plate", "general"}, {"plate", "lib"},
      {"p2p", "protect"}, {"p2p", "run"}, {"p2p", "general"}, {"p2p", "lib"},
      {"run", "general"}, {"run", "cap"}, {"run", "plate"}, {"run", "fat"},
      {"lib", "general"}, {"lib", "cap"}, {"lib", "plate"}, {"lib", "run"},
      {"protect", "p2p"}, {"protect", "general"}, {"protect", "run"}, {"protect", "lib"},
    };
    char name[96], nup[96], topics_bag[512], rel_bag[256];
    char seen_topics[16][32];
    char line[CUBALC_HOST_STR_MAX];
    char esc_fhint[400], esc_thint[320], esc_topics[600], esc_rel[400];
    const char *form_hint = NULL, *topic_hint = NULL;
    char first_topic[32];
    size_t k, o, eo;
    int i, n = 0, nr = 0, nall = (int)(sizeof rows / sizeof rows[0]);
    int nseen = 0, n_th = (int)(sizeof thints / sizeof thints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    const char *p;
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
                  "FORMGUIDE: need form — FORMGUIDE HASFORM · GUIDEFORM SETP");
      var_set_str(vm, "ERR",
                  "FORMGUIDE: need form — FORMGUIDE HASFORM · GUIDEFORM SETP");
      bump(vm);
      return 1;
    }
    for (k = 0; name[k] && k + 1 < sizeof nup; k++) {
      char c = name[k];
      if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
      nup[k] = c;
    }
    nup[k] = 0;
    topics_bag[0] = 0; o = 0; n = 0; nseen = 0; first_topic[0] = 0;
    for (i = 0; i < nall; i++) {
      char fup[96];
      size_t j, ln;
      int match, already = 0, si;
      for (j = 0; rows[i].form[j] && j + 1 < sizeof fup; j++) {
        char c = rows[i].form[j];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        fup[j] = c;
      }
      fup[j] = 0;
      match = !strcmp(fup, nup);
      if (!match) continue;
      for (si = 0; si < nseen; si++) {
        if (!strcmp(seen_topics[si], rows[i].topic)) { already = 1; break; }
      }
      if (already) continue;
      if (nseen < (int)(sizeof seen_topics / sizeof seen_topics[0])) {
        snprintf(seen_topics[nseen], sizeof seen_topics[0], "%s", rows[i].topic);
        nseen++;
      }
      if (!first_topic[0])
        snprintf(first_topic, sizeof first_topic, "%s", rows[i].topic);
      ln = strlen(rows[i].topic);
      if (o && o + 1 < sizeof topics_bag) topics_bag[o++] = '\n';
      if (o + ln < sizeof topics_bag) {
        memcpy(topics_bag + o, rows[i].topic, ln);
        o += ln;
      }
      topics_bag[o] = 0;
      n++;
    }
    if (n == 0) {
      char em[192];
      snprintf(em, sizeof em,
               "FORMGUIDE miss: '%s' — FORMHINT · FORMTOPICS · LISTFORMS · cubalc formguide",
               name);
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "FORMGUIDE", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "FORMGUIDE_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    if (!cubalc_form_known(name, &form_hint) || !form_hint || !form_hint[0])
      form_hint = name;
    for (i = 0; i < n_th; i++) {
      if (!strcmp(thints[i].id, first_topic)) { topic_hint = thints[i].hint; break; }
    }
    if (!topic_hint) topic_hint = "";
    rel_bag[0] = 0; o = 0; nr = 0;
    for (i = 0; i < n_rels; i++) {
      size_t ln;
      if (strcmp(rels[i].topic, first_topic) != 0) continue;
      ln = strlen(rels[i].rel);
      if (o && o + 1 < sizeof rel_bag) rel_bag[o++] = '\n';
      if (o + ln < sizeof rel_bag) { memcpy(rel_bag + o, rels[i].rel, ln); o += ln; }
      rel_bag[o] = 0; nr++;
    }
#define CUBALC_FG_ESC(dst, srcv) do { \
      eo = 0; \
      for (p = (srcv); *p && eo + 2 < sizeof(dst); p++) { \
        if (*p == '"' || *p == '\\') { (dst)[eo++] = '\\'; (dst)[eo++] = *p; } \
        else if (*p == '\n') { (dst)[eo++] = '\\'; (dst)[eo++] = 'n'; } \
        else if ((unsigned char)*p < 0x20) continue; \
        else (dst)[eo++] = *p; \
      } \
      (dst)[eo] = 0; \
    } while (0)
    CUBALC_FG_ESC(esc_fhint, form_hint);
    CUBALC_FG_ESC(esc_thint, topic_hint);
    CUBALC_FG_ESC(esc_topics, topics_bag);
    CUBALC_FG_ESC(esc_rel, rel_bag);
#undef CUBALC_FG_ESC
    snprintf(line, sizeof line,
      "{\"schema\":\"cubalc.formguide.v1\",\"ok\":true,\"form\":\"%s\","
      "\"form_hint\":\"%s\",\"topic\":\"%s\",\"topic_hint\":\"%s\","
      "\"topics_n\":%d,\"related_n\":%d,"
      "\"topics\":\"%s\",\"related\":\"%s\","
      "\"version\":\"%s\","
      "\"note\":\"FORMHINT+FORMTOPICS+first GUIDE · dual of cubalc formguide\"}",
      nup, esc_fhint, first_topic, esc_thint, n, nr, esc_topics, esc_rel,
      CUBALC_LANG_VERSION);
    var_set_str(vm, "LAST", line);
    var_set_str(vm, "FORMGUIDE", line);
    var_set_str(vm, "GUIDEFORM", line);
    var_set_str(vm, "FORM", name);
    var_set_str(vm, "FORMGUIDE_OF", name);
    var_set_str(vm, "FORM_HINT", form_hint);
    var_set_str(vm, "FORMHINT", form_hint);
    var_set_str(vm, "FORMTOPICS", topics_bag);
    var_set_str(vm, "TOPIC_NAME", first_topic);
    var_set_str(vm, "FORMGUIDE_TOPIC", first_topic);
    var_set_str(vm, "TOPIC_HINT", topic_hint);
    var_set_str(vm, "GUIDE_HINT", topic_hint);
    var_set_str(vm, "GUIDE_RELATED", rel_bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", line);
    vm->last_n = n + nr;
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "FORMTOPICS_N", n);
    var_set_num(vm, "FORMGUIDE_N", n);
    var_set_num(vm, "GUIDE_RELATED_N", nr);
    var_set_num(vm, "OK", 1);
    if (vm->res)
      snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", "cubalc.formguide.v1");
    if (vm->trace)
      fprintf(vm->trace, "# formguide form=%s topic=%s topics=%d\n", name, first_topic, n);
    bump(vm);
    return 1;
  }

  /* ERRTIPS|FIXTIPS [err] — multi-line recovery tips from sticky LAST_ERR (or arg).
   * Usability: WHY = one hint; ERRTIPS = tip bag + inferred discovery topic so agents
   * can chain TIPS/TOPIC/RUNSNIP after soft fail without cookbook.
   * LAST=newline tips · ERRTIPS_TOPIC · ERRTIPS_N · report-only (does not clear ERR). */
  if (kw(&L->cur,"ERRTIPS")||kw(&L->cur,"FIXTIPS")||kw(&L->cur,"RECOVERTIPS")||
      kw(&L->cur,"ERR_TIPS")||kw(&L->cur,"FIXHINTS")||kw(&L->cur,"RECOVERY_TIPS")||
      kw(&L->cur,"WHYTIPS")||kw(&L->cur,"ERRPLAYBOOK")){
    static const struct { const char *topic; const char *tip; } tips[] = {
      {"general", "cubalc doctor — install readiness plate"},
      {"general", "STATUS · VARS · CLEAR_ERR after fix"},
      {"general", "cubalc topics · TOPIC name for tips+forms+snip triad"},
      {"general", "cubalc search KEYWORD — forms/libs/examples"},
      {"cap", "HASFORM name · NEEDFORMS a b · FORMHINT name"},
      {"cap", "LISTFORMS prefix · cubalc forms · cubalc run -C FORMS"},
      {"cap", "INCLUDE cap_boot · cubalc init --cap"},
      {"cap", "RELATED HASFORM · FORMSFOR cap · RUNSNIP cap"},
      {"fat", "VARROOM · HASVARROOM n · NEEDVARROOM n"},
      {"fat", "REMAIN_MS · HAS_TIME · NEEDTIME · cubalc run -T ms"},
      {"fat", "INCLUDE fat_session · DEFAULT NEED_VARROOM"},
      {"plate", "NEEDP/DEFAULTP seed · GETP OR fallback"},
      {"plate", "cubalc plate has|need path.json keys"},
      {"plate", "PRETTYP · SAVEPLATE · INCLUDE plate_session"},
      {"p2p", "export CUBALC_SMX_KEY hex64 before mesh"},
      {"p2p", "CUBALC_P2P_SOFT=1 · CUBALC_P2P_TIMEOUT ms"},
      {"p2p", "cubalc smx-bus prove-tcp · docs/P2P_SMX.md"},
      {"run", "ASSERT_GOT/ASSERT_EXPECTED · EXPECT soft · CLEAR_ERR"},
      {"run", "WHY · cubalc run -s / CUBALC_STRICT"},
      {"run", "ERRTIPS after FAIL · TIPS run playbook"},
      {"lib", "cubalc libs [filter] · REQUIRE LIB · INCLUDE SOFT"},
      {"lib", "MATCHLIBS · CHECKDEPS · cubalc recipe name"},
      {"lib", "cubalc which name · HASLIB · LISTLIBS"},
      {"protect", "cubalc protect status · HOLD_FLASH device/mesh-join only"},
      {"protect", "docs/CORE_PROTECT.md · cubalc protect all"},
    };
    char topic[32], bag[4096], argerr[240];
    const char *err = "";
    size_t o = 0;
    int i, n = 0, nall = (int)(sizeof tips / sizeof tips[0]);
    Var *vle;
    lex_next(L);
    argerr[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(argerr, sizeof argerr, "%s", L->cur.text);
      lex_next(L);
      err = argerr;
    } else if (L->cur.kind == TK_IDENT) {
      if (strcmp(L->cur.text, "LAST") == 0 || strcmp(L->cur.text, "LAST_ERR") == 0 ||
          strcmp(L->cur.text, "ERR") == 0) {
        if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(argerr, sizeof argerr, "%s", vm->last_str);
        else {
          Var *vx = var_get(vm, L->cur.text, 0);
          if (vx && vx->is_str) snprintf(argerr, sizeof argerr, "%s", vx->sval);
        }
        lex_next(L);
        err = argerr;
      } else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            !(kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
              kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"PASS") ||
              kw(&L->cur,"FAIL") || kw(&L->cur,"NOTE") || kw(&L->cur,"STATUS"))) {
          snprintf(argerr, sizeof argerr, "%s", vv->sval);
          lex_next(L);
          err = argerr;
        }
      }
    }
    if (!err[0]) {
      vle = var_get(vm, "LAST_ERR", 0);
      if (vle && vle->is_str && vle->sval[0]) err = vle->sval;
      else if (vm->err[0]) err = vm->err;
      else {
        Var *ve2 = var_get(vm, "ERR", 0);
        if (ve2 && ve2->is_str && ve2->sval[0]) err = ve2->sval;
      }
    }
    /* classify → discovery topic (cap before lib: NEEDFORMS/SORTLIBS overlap) */
    snprintf(topic, sizeof topic, "%s", "general");
    if (!err[0])
      snprintf(topic, sizeof topic, "%s", "general");
    else if (strstr(err, "NEEDFORM") || strstr(err, "HASFORM") ||
             strstr(err, "FORMMISS") || strstr(err, "unknown form") ||
             strstr(err, "did you mean") || strstr(err, "REQUIRE FORM") ||
             strstr(err, "LISTFORMS") || strstr(err, "FORMHINT") ||
             strstr(err, "NEEDFORMS") || strstr(err, "HASFORMS"))
      snprintf(topic, sizeof topic, "%s", "cap");
    else if (strstr(err, "NEEDMATCHLIBS") || strstr(err, "HASMATCHLIBS") ||
             strstr(err, "PICKLIB") || strstr(err, "SORTLIBS") ||
             strstr(err, "INCLUDE") || strstr(err, "include") ||
             strstr(err, "REQUIRE LIB") || strstr(err, "lib missing") ||
             strstr(err, "NEEDDEPS") || strstr(err, "CHECKDEPS") ||
             strstr(err, "HASLIB") || strstr(err, "LISTLIBS"))
      snprintf(topic, sizeof topic, "%s", "lib");
    else if (strstr(err, "VARROOM") || strstr(err, "NEEDVARROOM") ||
             strstr(err, "HASVARROOM") || strstr(err, "REMAIN_MS") ||
             strstr(err, "NEEDTIME") || strstr(err, "HAS_TIME") ||
             strstr(err, "timed_out") || strstr(err, "timeout_ms"))
      snprintf(topic, sizeof topic, "%s", "fat");
    else if (strstr(err, "NEEDP") || strstr(err, "NEEDFLAT") ||
             strstr(err, "UNIFORM") || strstr(err, "roles must") ||
             strstr(err, "plate") || strstr(err, "PLATE") ||
             strstr(err, "SETP") || strstr(err, "GETP"))
      snprintf(topic, sizeof topic, "%s", "plate");
    else if (strstr(err, "DIAL") || strstr(err, "SERVE") || strstr(err, "SMX") ||
             strstr(err, "TALK") || strstr(err, "P2P") || strstr(err, "timeout") ||
             strstr(err, "peer"))
      snprintf(topic, sizeof topic, "%s", "p2p");
    else if (strstr(err, "HOLD_FLASH") || strstr(err, "hold_flash") ||
             strstr(err, "CORE_PROTECT") || strstr(err, "protect"))
      snprintf(topic, sizeof topic, "%s", "protect");
    else if (strstr(err, "ASSERT") || strstr(err, "EXPECT") ||
             strstr(err, "is false") || strstr(err, "falsey") ||
             strstr(err, "FAIL") || strstr(err, "EXIT"))
      snprintf(topic, sizeof topic, "%s", "run");
    else if (strstr(err, "VERSION") || strstr(err, "too old"))
      snprintf(topic, sizeof topic, "%s", "run");
    else if (strstr(err, "cannot open") || strstr(err, "ENOENT") ||
             strstr(err, "No such") || strstr(err, "path"))
      snprintf(topic, sizeof topic, "%s", "general");

    bag[0] = 0;
    if (!err[0]) {
      const char *oktip = "ok — no sticky LAST_ERR · STATUS · CLEAR_ERR idle";
      size_t ln = strlen(oktip);
      memcpy(bag, oktip, ln);
      bag[ln] = 0;
      n = 1;
      snprintf(topic, sizeof topic, "%s", "general");
    } else {
      for (i = 0; i < nall; i++) {
        size_t ln;
        if (strcmp(tips[i].topic, topic) != 0) continue;
        ln = strlen(tips[i].tip);
        if (o && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) { memcpy(bag + o, tips[i].tip, ln); o += ln; }
        bag[o] = 0;
        n++;
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "ERRTIPS", bag);
    var_set_str(vm, "FIXTIPS", bag);
    var_set_str(vm, "ERRTIPS_TOPIC", topic);
    var_set_str(vm, "FIXTIPS_TOPIC", topic);
    var_set_str(vm, "TOPIC_NAME", topic);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "ERRTIPS_N", n);
    var_set_num(vm, "FIXTIPS_N", n);
    /* report-only: do not rewrite OK / sticky LAST_ERR */
    if (vm->trace)
      fprintf(vm->trace, "# errtips topic=%s n=%d\n", topic, n);
    bump(vm);
    return 1;
  }

  /* ERRRUN|RECOVERSNIP [err] — ERRTIPS classify + RUNSNIP on inferred topic.
   * Usability: soft-fail → tip bag + prove recovery surface without two forms.
   * Sets ERRTIPS_* then nested RUNSNIP <topic> · OK from nested · LAST nested print. */
  if (kw(&L->cur,"ERRRUN")||kw(&L->cur,"RECOVERSNIP")||kw(&L->cur,"FIXRUN")||
      kw(&L->cur,"ERR_RUN")||kw(&L->cur,"RECOVER_RUN")||kw(&L->cur,"AUTOFIX_SNIP")||
      kw(&L->cur,"ERRSNIP")||kw(&L->cur,"FIXSNIP")){
    static const struct { const char *topic; const char *tip; } tips[] = {
      {"general", "cubalc doctor — install readiness plate"},
      {"general", "STATUS · VARS · CLEAR_ERR after fix"},
      {"general", "cubalc topics · TOPIC name for tips+forms+snip triad"},
      {"cap", "HASFORM name · NEEDFORMS a b · FORMHINT name"},
      {"cap", "RELATED HASFORM · FORMSFOR cap · RUNSNIP cap"},
      {"fat", "VARROOM · HASVARROOM n · NEEDVARROOM n"},
      {"fat", "REMAIN_MS · HAS_TIME · NEEDTIME · cubalc run -T ms"},
      {"plate", "NEEDP/DEFAULTP seed · GETP OR fallback"},
      {"plate", "PRETTYP · SAVEPLATE · INCLUDE plate_session"},
      {"p2p", "export CUBALC_SMX_KEY hex64 before mesh"},
      {"p2p", "CUBALC_P2P_SOFT=1 · CUBALC_P2P_TIMEOUT ms"},
      {"run", "ASSERT_GOT/ASSERT_EXPECTED · EXPECT soft · CLEAR_ERR"},
      {"run", "WHY · ERRTIPS after FAIL · TIPS run"},
      {"lib", "cubalc libs [filter] · REQUIRE LIB · INCLUDE SOFT"},
      {"lib", "MATCHLIBS · CHECKDEPS · cubalc recipe name"},
      {"protect", "cubalc protect status · HOLD_FLASH device/mesh-join only"},
    };
    char topic[32], bag[2048], argerr[240], nested[96];
    const char *err = "";
    size_t o = 0;
    int i, n = 0, nall = (int)(sizeof tips / sizeof tips[0]);
    cubalc_run_result rr;
    int ok;
    Var *vle;
    lex_next(L);
    argerr[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(argerr, sizeof argerr, "%s", L->cur.text);
      lex_next(L);
      err = argerr;
    } else if (L->cur.kind == TK_IDENT) {
      if (strcmp(L->cur.text, "LAST") == 0 || strcmp(L->cur.text, "LAST_ERR") == 0 ||
          strcmp(L->cur.text, "ERR") == 0) {
        if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(argerr, sizeof argerr, "%s", vm->last_str);
        else {
          Var *vx = var_get(vm, L->cur.text, 0);
          if (vx && vx->is_str) snprintf(argerr, sizeof argerr, "%s", vx->sval);
        }
        lex_next(L);
        err = argerr;
      } else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            !(kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
              kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"PASS") ||
              kw(&L->cur,"FAIL") || kw(&L->cur,"NOTE") || kw(&L->cur,"STATUS"))) {
          snprintf(argerr, sizeof argerr, "%s", vv->sval);
          lex_next(L);
          err = argerr;
        }
      }
    }
    if (!err[0]) {
      vle = var_get(vm, "LAST_ERR", 0);
      if (vle && vle->is_str && vle->sval[0]) err = vle->sval;
      else if (vm->err[0]) err = vm->err;
      else {
        Var *ve2 = var_get(vm, "ERR", 0);
        if (ve2 && ve2->is_str && ve2->sval[0]) err = ve2->sval;
      }
    }
    snprintf(topic, sizeof topic, "%s", "general");
    if (err[0]) {
      if (strstr(err, "NEEDFORM") || strstr(err, "HASFORM") ||
          strstr(err, "FORMMISS") || strstr(err, "unknown form") ||
          strstr(err, "did you mean") || strstr(err, "REQUIRE FORM") ||
          strstr(err, "LISTFORMS") || strstr(err, "FORMHINT") ||
          strstr(err, "NEEDFORMS") || strstr(err, "HASFORMS"))
        snprintf(topic, sizeof topic, "%s", "cap");
      else if (strstr(err, "NEEDMATCHLIBS") || strstr(err, "HASMATCHLIBS") ||
               strstr(err, "PICKLIB") || strstr(err, "SORTLIBS") ||
               strstr(err, "INCLUDE") || strstr(err, "include") ||
               strstr(err, "REQUIRE LIB") || strstr(err, "lib missing") ||
               strstr(err, "NEEDDEPS") || strstr(err, "CHECKDEPS") ||
               strstr(err, "HASLIB") || strstr(err, "LISTLIBS"))
        snprintf(topic, sizeof topic, "%s", "lib");
      else if (strstr(err, "VARROOM") || strstr(err, "NEEDVARROOM") ||
               strstr(err, "HASVARROOM") || strstr(err, "REMAIN_MS") ||
               strstr(err, "NEEDTIME") || strstr(err, "HAS_TIME"))
        snprintf(topic, sizeof topic, "%s", "fat");
      else if (strstr(err, "NEEDP") || strstr(err, "NEEDFLAT") ||
               strstr(err, "UNIFORM") || strstr(err, "plate") ||
               strstr(err, "PLATE") || strstr(err, "SETP") || strstr(err, "GETP"))
        snprintf(topic, sizeof topic, "%s", "plate");
      else if (strstr(err, "DIAL") || strstr(err, "SERVE") || strstr(err, "SMX") ||
               strstr(err, "TALK") || strstr(err, "P2P") || strstr(err, "timeout"))
        snprintf(topic, sizeof topic, "%s", "p2p");
      else if (strstr(err, "HOLD_FLASH") || strstr(err, "protect") ||
               strstr(err, "CORE_PROTECT"))
        snprintf(topic, sizeof topic, "%s", "protect");
      else if (strstr(err, "ASSERT") || strstr(err, "EXPECT") ||
               strstr(err, "is false") || strstr(err, "FAIL") ||
               strstr(err, "VERSION") || strstr(err, "too old"))
        snprintf(topic, sizeof topic, "%s", "run");
    }
    bag[0] = 0; o = 0; n = 0;
    if (!err[0]) {
      const char *oktip = "ok — no sticky LAST_ERR · RUNSNIP general probe";
      size_t ln = strlen(oktip);
      memcpy(bag, oktip, ln); bag[ln] = 0; n = 1;
    } else {
      for (i = 0; i < nall; i++) {
        size_t ln;
        if (strcmp(tips[i].topic, topic) != 0) continue;
        ln = strlen(tips[i].tip);
        if (o && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) { memcpy(bag + o, tips[i].tip, ln); o += ln; }
        bag[o] = 0; n++;
      }
    }
    var_set_str(vm, "ERRTIPS", bag);
    var_set_str(vm, "ERRTIPS_TOPIC", topic);
    var_set_str(vm, "ERRRUN_TOPIC", topic);
    var_set_str(vm, "TOPIC_NAME", topic);
    var_set_num(vm, "ERRTIPS_N", n);
    var_set_num(vm, "ERRRUN_TIPS_N", n);
    /* nested RUNSNIP on inferred topic */
    snprintf(nested, sizeof nested, "RUNSNIP %s\nPASS\n", topic);
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(nested, strlen(nested), "<errrun>", &rr, vm->trace);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    var_set_num(vm, "ERRRUN_OK", ok);
    var_set_num(vm, "RUNSNIP_OK", ok);
    var_set_num(vm, "ERRRUN_ASSERTS_OK", rr.asserts_ok);
    var_set_num(vm, "ERRRUN_ASSERTS_FAIL", rr.asserts_fail);
    if (rr.last_print[0]) {
      var_set_str(vm, "LAST", rr.last_print);
      var_set_str(vm, "ERRRUN_LAST", rr.last_print);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", rr.last_print);
      vm->last_n = (long)strlen(rr.last_print);
    } else {
      var_set_str(vm, "LAST", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      vm->last_n = n;
    }
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "OK", ok);
    if (!ok) {
      const char *em = rr.last_err[0] ? rr.last_err : (rr.err[0] ? rr.err : "ERRRUN nested fail");
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->res) {
      vm->res->asserts_ok += rr.asserts_ok;
      snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", vm->last_str);
    }
    if (vm->trace)
      fprintf(vm->trace, "# errrun topic=%s tips=%d ok=%d\n", topic, n, ok);
    bump(vm);
    return 1;
  }

  /* ERRGUIDE|RECOVERGUIDE [err] — ERRTIPS classify + GUIDE playbook one-shot.
   * Usability: soft-fail → tip bag + full recovery plate (hint/related/forms/snip)
   * without ERRRUN-only snip or multi-form glue. Dual of cubalc errguide.
   * LAST=cubalc.errguide.v1 · ERRGUIDE_TOPIC · ERRTIPS · GUIDE_* bags · does not clear ERR. */
  if (kw(&L->cur,"ERRGUIDE")||kw(&L->cur,"RECOVERGUIDE")||kw(&L->cur,"FIXGUIDE")||
      kw(&L->cur,"ERR_GUIDE")||kw(&L->cur,"RECOVER_GUIDE")||kw(&L->cur,"ERRPLAYGUIDE")||
      kw(&L->cur,"WHYGUIDE")||kw(&L->cur,"AUTOFIX_GUIDE")){
    static const struct { const char *topic; const char *tip; } tips[] = {
      {"general", "cubalc doctor — install readiness plate"},
      {"general", "STATUS · VARS · CLEAR_ERR after fix"},
      {"general", "cubalc guide · TOPIC name for full playbook"},
      {"general", "cubalc search KEYWORD — forms/libs/examples"},
      {"cap", "HASFORM name · NEEDFORMS a b · FORMHINT name"},
      {"cap", "LISTFORMS prefix · cubalc forms · cubalc run -C FORMS"},
      {"cap", "INCLUDE cap_boot · cubalc init --cap"},
      {"cap", "RELATED HASFORM · GUIDE cap · RUNSNIP cap"},
      {"fat", "VARROOM · HASVARROOM n · NEEDVARROOM n"},
      {"fat", "REMAIN_MS · HAS_TIME · NEEDTIME · cubalc run -T ms"},
      {"fat", "INCLUDE fat_session · DEFAULT NEED_VARROOM"},
      {"plate", "NEEDP/DEFAULTP seed · GETP OR fallback"},
      {"plate", "cubalc plate has|need path.json keys"},
      {"plate", "PRETTYP · SAVEPLATE · INCLUDE plate_session"},
      {"p2p", "export CUBALC_SMX_KEY hex64 before mesh"},
      {"p2p", "CUBALC_P2P_SOFT=1 · CUBALC_P2P_TIMEOUT ms"},
      {"p2p", "cubalc smx-bus prove-tcp · docs/P2P_SMX.md"},
      {"run", "ASSERT_GOT/ASSERT_EXPECTED · EXPECT soft · CLEAR_ERR"},
      {"run", "WHY · cubalc run -s / CUBALC_STRICT"},
      {"run", "ERRGUIDE after FAIL · GUIDE run playbook"},
      {"lib", "cubalc libs [filter] · REQUIRE LIB · INCLUDE SOFT"},
      {"lib", "MATCHLIBS · CHECKDEPS · cubalc recipe name"},
      {"lib", "cubalc which name · HASLIB · LISTLIBS"},
      {"protect", "cubalc protect status · HOLD_FLASH device/mesh-join only"},
      {"protect", "docs/CORE_PROTECT.md · cubalc protect all"},
    };
    static const struct { const char *id; const char *hint; } hints[] = {
      {"general", "install/doctor/init surface · VERSION STATUS IDENTITY"},
      {"cap", "HASFORM/NEEDFORMS capability floor · FORMHINT · run -C"},
      {"fat", "VARROOM/REMAIN_MS fat nest budget · fat_session · run -T"},
      {"plate", "SETP/NEEDP plate agent JSON · plate_session · PRETTYP"},
      {"p2p", "SMX SERVE/DIAL mesh · CUBALC_SMX_KEY · P2P_SOFT/TIMEOUT"},
      {"run", "ASSERT/EXPECT/WHY run probes · ERRTIPS · CLEAR_ERR"},
      {"lib", "LISTLIBS/RECIPE INCLUDE discovery · MATCHLIBS · checkdeps"},
      {"protect", "HOLD_FLASH/protect status · CORE_PROTECT · device/mesh-join only"},
    };
    static const struct { const char *topic; const char *rel; } rels[] = {
      {"general", "cap"}, {"general", "lib"}, {"general", "run"}, {"general", "plate"},
      {"cap", "general"}, {"cap", "run"}, {"cap", "lib"}, {"cap", "fat"},
      {"fat", "plate"}, {"fat", "run"}, {"fat", "cap"}, {"fat", "general"},
      {"plate", "fat"}, {"plate", "run"}, {"plate", "general"}, {"plate", "lib"},
      {"p2p", "protect"}, {"p2p", "run"}, {"p2p", "general"}, {"p2p", "lib"},
      {"run", "general"}, {"run", "cap"}, {"run", "plate"}, {"run", "fat"},
      {"lib", "general"}, {"lib", "cap"}, {"lib", "plate"}, {"lib", "run"},
      {"protect", "p2p"}, {"protect", "general"}, {"protect", "run"}, {"protect", "lib"},
    };
    static const struct { const char *topic; const char *form; } forms[] = {
      {"general", "VERSION"}, {"general", "GUIDE"}, {"general", "STATUS"},
      {"general", "ERRGUIDE"}, {"general", "TIPS"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORMS"}, {"cap", "FORMHINT"}, {"cap", "GUIDE"},
      {"fat", "VARROOM"}, {"fat", "REMAIN_MS"}, {"fat", "GUIDE"},
      {"plate", "SETP"}, {"plate", "GETP"}, {"plate", "NEEDP"},
      {"p2p", "SMX"}, {"p2p", "SERVE"}, {"p2p", "DIAL"},
      {"run", "ASSERT"}, {"run", "EXPECT"}, {"run", "WHY"}, {"run", "ERRGUIDE"},
      {"lib", "LISTLIBS"}, {"lib", "RECIPE"}, {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"}, {"protect", "STATUS"},
    };
    char topic[32], bag[4096], argerr[240], forms_bag[512], rel_bag[256];
    char line[CUBALC_HOST_STR_MAX];
    char esc_tips[2400], esc_forms[800], esc_hint[320], esc_rel[400], esc_err[480];
    const char *err = "";
    const char *hint = NULL;
    size_t o = 0, eo;
    int i, n = 0, nr = 0, nf = 0;
    int nall = (int)(sizeof tips / sizeof tips[0]);
    int n_hints = (int)(sizeof hints / sizeof hints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    int n_forms = (int)(sizeof forms / sizeof forms[0]);
    const char *p;
    Var *vle;
    lex_next(L);
    argerr[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(argerr, sizeof argerr, "%s", L->cur.text);
      lex_next(L);
      err = argerr;
    } else if (L->cur.kind == TK_IDENT) {
      if (strcmp(L->cur.text, "LAST") == 0 || strcmp(L->cur.text, "LAST_ERR") == 0 ||
          strcmp(L->cur.text, "ERR") == 0) {
        if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(argerr, sizeof argerr, "%s", vm->last_str);
        else {
          Var *vx = var_get(vm, L->cur.text, 0);
          if (vx && vx->is_str) snprintf(argerr, sizeof argerr, "%s", vx->sval);
        }
        lex_next(L);
        err = argerr;
      } else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            !(kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
              kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"PASS") ||
              kw(&L->cur,"FAIL") || kw(&L->cur,"NOTE") || kw(&L->cur,"STATUS"))) {
          snprintf(argerr, sizeof argerr, "%s", vv->sval);
          lex_next(L);
          err = argerr;
        }
      }
    }
    if (!err[0]) {
      vle = var_get(vm, "LAST_ERR", 0);
      if (vle && vle->is_str && vle->sval[0]) err = vle->sval;
      else if (vm->err[0]) err = vm->err;
      else {
        Var *ve2 = var_get(vm, "ERR", 0);
        if (ve2 && ve2->is_str && ve2->sval[0]) err = ve2->sval;
      }
    }
    snprintf(topic, sizeof topic, "%s", "general");
    if (err[0]) {
      if (strstr(err, "NEEDFORM") || strstr(err, "HASFORM") ||
          strstr(err, "FORMMISS") || strstr(err, "unknown form") ||
          strstr(err, "did you mean") || strstr(err, "REQUIRE FORM") ||
          strstr(err, "LISTFORMS") || strstr(err, "FORMHINT") ||
          strstr(err, "NEEDFORMS") || strstr(err, "HASFORMS"))
        snprintf(topic, sizeof topic, "%s", "cap");
      else if (strstr(err, "NEEDMATCHLIBS") || strstr(err, "HASMATCHLIBS") ||
               strstr(err, "PICKLIB") || strstr(err, "SORTLIBS") ||
               strstr(err, "INCLUDE") || strstr(err, "include") ||
               strstr(err, "REQUIRE LIB") || strstr(err, "lib missing") ||
               strstr(err, "NEEDDEPS") || strstr(err, "CHECKDEPS") ||
               strstr(err, "HASLIB") || strstr(err, "LISTLIBS"))
        snprintf(topic, sizeof topic, "%s", "lib");
      else if (strstr(err, "VARROOM") || strstr(err, "NEEDVARROOM") ||
               strstr(err, "HASVARROOM") || strstr(err, "REMAIN_MS") ||
               strstr(err, "NEEDTIME") || strstr(err, "HAS_TIME"))
        snprintf(topic, sizeof topic, "%s", "fat");
      else if (strstr(err, "NEEDP") || strstr(err, "NEEDFLAT") ||
               strstr(err, "UNIFORM") || strstr(err, "plate") ||
               strstr(err, "PLATE") || strstr(err, "SETP") || strstr(err, "GETP"))
        snprintf(topic, sizeof topic, "%s", "plate");
      else if (strstr(err, "DIAL") || strstr(err, "SERVE") || strstr(err, "SMX") ||
               strstr(err, "TALK") || strstr(err, "P2P") || strstr(err, "timeout"))
        snprintf(topic, sizeof topic, "%s", "p2p");
      else if (strstr(err, "HOLD_FLASH") || strstr(err, "protect") ||
               strstr(err, "CORE_PROTECT"))
        snprintf(topic, sizeof topic, "%s", "protect");
      else if (strstr(err, "ASSERT") || strstr(err, "EXPECT") ||
               strstr(err, "is false") || strstr(err, "FAIL") ||
               strstr(err, "VERSION") || strstr(err, "too old"))
        snprintf(topic, sizeof topic, "%s", "run");
    }
    bag[0] = 0; o = 0; n = 0;
    if (!err[0]) {
      const char *oktip = "ok — no sticky LAST_ERR · GUIDE general playbook";
      size_t ln = strlen(oktip);
      memcpy(bag, oktip, ln); bag[ln] = 0; n = 1;
    } else {
      for (i = 0; i < nall; i++) {
        size_t ln;
        if (strcmp(tips[i].topic, topic) != 0) continue;
        ln = strlen(tips[i].tip);
        if (o && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) { memcpy(bag + o, tips[i].tip, ln); o += ln; }
        bag[o] = 0; n++;
      }
    }
    for (i = 0; i < n_hints; i++) {
      if (!strcmp(hints[i].id, topic)) { hint = hints[i].hint; break; }
    }
    if (!hint) hint = "cubalc topics · GUIDE general";
    rel_bag[0] = 0; o = 0; nr = 0;
    for (i = 0; i < n_rels; i++) {
      size_t ln;
      if (strcmp(rels[i].topic, topic) != 0) continue;
      ln = strlen(rels[i].rel);
      if (o && o + 1 < sizeof rel_bag) rel_bag[o++] = '\n';
      if (o + ln < sizeof rel_bag) { memcpy(rel_bag + o, rels[i].rel, ln); o += ln; }
      rel_bag[o] = 0; nr++;
    }
    forms_bag[0] = 0; o = 0; nf = 0;
    for (i = 0; i < n_forms; i++) {
      size_t ln;
      if (strcmp(forms[i].topic, topic) != 0) continue;
      ln = strlen(forms[i].form);
      if (o && o + 1 < sizeof forms_bag) forms_bag[o++] = '\n';
      if (o + ln < sizeof forms_bag) { memcpy(forms_bag + o, forms[i].form, ln); o += ln; }
      forms_bag[o] = 0; nf++;
    }
#define CUBALC_EG_ESC(dst, srcv) do { \
      eo = 0; \
      for (p = (srcv); *p && eo + 2 < sizeof(dst); p++) { \
        if (*p == '"' || *p == '\\') { (dst)[eo++] = '\\'; (dst)[eo++] = *p; } \
        else if (*p == '\n') { (dst)[eo++] = '\\'; (dst)[eo++] = 'n'; } \
        else if ((unsigned char)*p < 0x20) continue; \
        else (dst)[eo++] = *p; \
      } \
      (dst)[eo] = 0; \
    } while (0)
    CUBALC_EG_ESC(esc_err, err[0] ? err : "ok");
    CUBALC_EG_ESC(esc_hint, hint);
    CUBALC_EG_ESC(esc_rel, rel_bag);
    CUBALC_EG_ESC(esc_tips, bag);
    CUBALC_EG_ESC(esc_forms, forms_bag);
#undef CUBALC_EG_ESC
    snprintf(line, sizeof line,
      "{\"schema\":\"cubalc.errguide.v1\",\"ok\":true,\"topic\":\"%s\","
      "\"hint\":\"%s\",\"related_n\":%d,\"tips_n\":%d,\"forms_n\":%d,"
      "\"related\":\"%s\",\"tips\":\"%s\",\"forms\":\"%s\","
      "\"err\":\"%s\",\"version\":\"%s\","
      "\"note\":\"ERRTIPS classify + GUIDE playbook · dual of cubalc errguide · twin of ERRRUN\"}",
      topic, esc_hint, nr, n, nf, esc_rel, esc_tips, esc_forms, esc_err,
      CUBALC_LANG_VERSION);
    var_set_str(vm, "LAST", line);
    var_set_str(vm, "ERRGUIDE", line);
    var_set_str(vm, "RECOVERGUIDE", line);
    var_set_str(vm, "ERRTIPS", bag);
    var_set_str(vm, "ERRTIPS_TOPIC", topic);
    var_set_str(vm, "ERRGUIDE_TOPIC", topic);
    var_set_str(vm, "TOPIC_NAME", topic);
    var_set_str(vm, "GUIDE_HINT", hint);
    var_set_str(vm, "TOPIC_HINT", hint);
    var_set_str(vm, "GUIDE_RELATED", rel_bag);
    var_set_str(vm, "GUIDE_FORMS", forms_bag);
    var_set_str(vm, "GUIDE_TIPS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", line);
    vm->last_n = n + nr + nf;
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "ERRTIPS_N", n);
    var_set_num(vm, "ERRGUIDE_TIPS_N", n);
    var_set_num(vm, "GUIDE_RELATED_N", nr);
    var_set_num(vm, "GUIDE_FORMS_N", nf);
    var_set_num(vm, "GUIDE_TIPS_N", n);
    var_set_num(vm, "OK", 1);
    if (vm->res)
      snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", "cubalc.errguide.v1");
    if (vm->trace)
      fprintf(vm->trace, "# errguide topic=%s tips=%d related=%d forms=%d\n",
              topic, n, nr, nf);
    bump(vm);
    return 1;
  }

  /* START|ONBOARD|WELCOME — agent onboarding plate (dual of cubalc onboard).
   * Usability: first-call surface without cookbook — topics catalog + next commands
   * + general guide hint. LAST=cubalc.start.v1 · START_NEXT · START_TOPICS bags. */
  if (kw(&L->cur,"START")||kw(&L->cur,"ONBOARD")||kw(&L->cur,"WELCOME")||
      kw(&L->cur,"BEGIN")||kw(&L->cur,"AGENTSTART")||kw(&L->cur,"BOOTSTRAP")||
      kw(&L->cur,"HELLO_AGENT")||kw(&L->cur,"START_HERE")){
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    static const char *nexts[] = {
      "cubalc doctor",
      "cubalc onboard",
      "cubalc guide general",
      "cubalc topics",
      "cubalc init --list",
      "cubalc search KEYWORD",
      "INCLUDE agent_boot",
      "cubalc formguide HASFORM",
      "cubalc errguide LAST_ERR",
      "HOLD_FLASH device/mesh-join only",
    };
    static const char *hint =
      "install/doctor/init surface · VERSION STATUS IDENTITY · GUIDE general";
    char topics_bag[256], next_bag[1024], line[CUBALC_HOST_STR_MAX];
    char esc_topics[400], esc_next[1400], esc_hint[320];
    size_t o, eo;
    int i, nt = (int)(sizeof topics / sizeof topics[0]);
    int nn = (int)(sizeof nexts / sizeof nexts[0]);
    const char *p;
    lex_next(L);
    topics_bag[0] = 0; o = 0;
    for (i = 0; i < nt; i++) {
      size_t ln = strlen(topics[i]);
      if (o && o + 1 < sizeof topics_bag) topics_bag[o++] = '\n';
      if (o + ln < sizeof topics_bag) { memcpy(topics_bag + o, topics[i], ln); o += ln; }
      topics_bag[o] = 0;
    }
    next_bag[0] = 0; o = 0;
    for (i = 0; i < nn; i++) {
      size_t ln = strlen(nexts[i]);
      if (o && o + 1 < sizeof next_bag) next_bag[o++] = '\n';
      if (o + ln < sizeof next_bag) { memcpy(next_bag + o, nexts[i], ln); o += ln; }
      next_bag[o] = 0;
    }
#define CUBALC_ST_ESC(dst, srcv) do { \
      eo = 0; \
      for (p = (srcv); *p && eo + 2 < sizeof(dst); p++) { \
        if (*p == '"' || *p == '\\') { (dst)[eo++] = '\\'; (dst)[eo++] = *p; } \
        else if (*p == '\n') { (dst)[eo++] = '\\'; (dst)[eo++] = 'n'; } \
        else if ((unsigned char)*p < 0x20) continue; \
        else (dst)[eo++] = *p; \
      } \
      (dst)[eo] = 0; \
    } while (0)
    CUBALC_ST_ESC(esc_topics, topics_bag);
    CUBALC_ST_ESC(esc_next, next_bag);
    CUBALC_ST_ESC(esc_hint, hint);
#undef CUBALC_ST_ESC
    snprintf(line, sizeof line,
      "{\"schema\":\"cubalc.start.v1\",\"ok\":true,\"cmd\":\"start\","
      "\"version\":\"%s\",\"topics_n\":%d,\"next_n\":%d,"
      "\"hint\":\"%s\",\"topics\":\"%s\",\"next\":\"%s\","
      "\"note\":\"agent onboarding · dual of cubalc onboard · chain GUIDE/TIPS/doctor\"}",
      CUBALC_LANG_VERSION, nt, nn, esc_hint, esc_topics, esc_next);
    var_set_str(vm, "LAST", line);
    var_set_str(vm, "START", line);
    var_set_str(vm, "ONBOARD", line);
    var_set_str(vm, "WELCOME", line);
    var_set_str(vm, "START_TOPICS", topics_bag);
    var_set_str(vm, "START_NEXT", next_bag);
    var_set_str(vm, "START_HINT", hint);
    var_set_str(vm, "GUIDE_HINT", hint);
    var_set_str(vm, "LISTTOPICS", topics_bag);
    var_set_str(vm, "TOPIC_NAME", "general");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", line);
    vm->last_n = nt + nn;
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "START_TOPICS_N", nt);
    var_set_num(vm, "START_NEXT_N", nn);
    var_set_num(vm, "LISTTOPICS_N", nt);
    var_set_num(vm, "OK", 1);
    if (vm->res)
      snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", "cubalc.start.v1");
    if (vm->trace)
      fprintf(vm->trace, "# start onboard topics=%d next=%d\n", nt, nn);
    bump(vm);
    return 1;
  }

  /* NOTE ["text"] — agent breadcrumb / step log. Sets LAST + NOTE, does not
   * change OK, EXPECT_OK, or sticky LAST_ERR (unlike PASS/FAIL). Trace: # note: */
  /* NOTE hz ms is a schedule note. A text or single-ident NOTE stays a breadcrumb. */
  if ((kw(&L->cur,"NOTE")||kw(&L->cur,"REMARK")||kw(&L->cur,"LOG_NOTE")||
       kw(&L->cur,"BREADCRUMB")||kw(&L->cur,"STEP_NOTE")) &&
      !cubalc_lang_sched_note_ahead(L)){
    int aln = L->cur.line;
    char msg[160];
    lex_next(L);
    msg[0] = 0;
    if (L->cur.kind == TK_STR){
      snprintf(msg, sizeof msg, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"IF") && !kw(&L->cur,"END") && !kw(&L->cur,"STATUS")){
      /* NOTE LAST or NOTE varname */
      if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(msg, sizeof msg, "%s", vm->last_str);
      else {
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && v->is_str) snprintf(msg, sizeof msg, "%s", v->sval);
        else if (v) snprintf(msg, sizeof msg, "%ld", v->val);
        else snprintf(msg, sizeof msg, "%s", L->cur.text);
      }
      lex_next(L);
    }
    if (!msg[0])
      snprintf(msg, sizeof msg, "note line %d", aln);
    var_set_str(vm, "NOTE", msg);
    var_set_str(vm, "LAST", msg);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", msg);
    vm->last_n = (long)strlen(msg);
    var_set_num(vm, "LAST_N", vm->last_n);
    /* preserve OK — do not rewrite soft status */
    if (vm->trace) fprintf(vm->trace, "# note: %s\n", msg);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", msg);
    bump(vm); return 1;
  }
  /* SETP|PUTP|PLATE_SET [FROM plate] key value — mutate PLATE or any plate var.
   * INCP|BUMPP|PLATE_INC [FROM plate] key [delta] — bump numeric key.
   * DELP|DROPP|PLATE_DEL [FROM plate] key — drop key (soft miss LAST_N=0).
   * GETP|PEEKP|PLATE_GET [FROM plate] key [OR|DEFAULT fb] — peel value.
   * Dotted/slash paths for nested fields (no GETPOBJ glue):
   *   GETP "freq.error" · SETP "freq.error" 9 · INCP "stats.hits" · DELP "cfg/tmp"
   * FROM: multi-plate agents without LET PLATE = other (twin of FILLP FROM):
   *   SETP FROM peer "host" "cubeB"
   *   GETP "n" FROM peer
   *   INCP "ticks" 1 FROM session
   *   SETP "role" "worker" FROM peer
   * Named FROM write-back updates that var; conventional PLATE stays untouched.
   * SETP_FROM/GETP_FROM/INCP_FROM/DELP_FROM = 0|1 · PATH_DEPTH when dotted.
   * Missing default PLATE → {}. GETP hit → LAST=value LAST_N=1; miss+OR → fb. */
  return 0;
}
