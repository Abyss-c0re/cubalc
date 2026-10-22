/* CubalC CLI related/formrel/neighbors — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_formrel.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>

int cubalc_cmd_formrel(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "related";
    /* Usability: related form-name bag for one form (dual of RELATED / FORMSFOR).
     *   cubalc related HASFORM · cubalc seealso SETP
     * Schema cubalc.related.v1 · complements formhint + formsfor. */
    static const struct { const char *form; const char *rel; } edges[] = {
      {"HASFORM", "NEEDFORM"}, {"HASFORM", "HASFORMS"}, {"HASFORM", "NEEDFORMS"},
      {"HASFORM", "FORMHINT"}, {"HASFORM", "LISTFORMS"}, {"HASFORM", "COUNTFORMS"},
      {"HASFORM", "REQUIRE FORM"}, {"HASFORM", "FORMSFOR"}, {"HASFORM", "TIPS"},
      {"NEEDFORM", "HASFORM"}, {"NEEDFORM", "NEEDFORMS"}, {"NEEDFORM", "HASFORMS"},
      {"NEEDFORM", "FORMHINT"}, {"NEEDFORM", "REQUIRE FORM"},
      {"HASFORMS", "NEEDFORMS"}, {"HASFORMS", "HASFORM"},
      {"HASFORMS", "HASFORMANY"}, {"HASFORMS", "NEEDFORMANY"},
      {"NEEDFORMS", "HASFORMS"}, {"NEEDFORMS", "NEEDFORM"}, {"NEEDFORMS", "HASFORM"},
      {"NEEDFORMS", "NEEDFORMANY"},
      {"HASFORMANY", "NEEDFORMANY"}, {"HASFORMANY", "HASFORMS"}, {"HASFORMANY", "HASFORM"},
      {"HASFORMANY", "FORMHAVE"}, {"HASFORMANY", "FORMMISS"}, {"HASFORMANY", "LISTFORMS"},
      {"NEEDFORMANY", "HASFORMANY"}, {"NEEDFORMANY", "NEEDFORMS"}, {"NEEDFORMANY", "NEEDFORM"},
      {"NEEDFORMANY", "FORMMISS"}, {"NEEDFORMANY", "FORMHAVE"},
      {"FORMHINT", "HASFORM"}, {"FORMHINT", "LISTFORMS"}, {"FORMHINT", "RELATED"},
      {"FORMHINT", "FORMSFOR"}, {"FORMHINT", "TIPS"},
      {"LISTFORMS", "COUNTFORMS"}, {"LISTFORMS", "HASFORM"}, {"LISTFORMS", "FORMHINT"},
      {"LISTFORMS", "FORMSFOR"}, {"COUNTFORMS", "LISTFORMS"}, {"COUNTFORMS", "HASFORM"},
      {"FORMSFOR", "TIPS"}, {"FORMSFOR", "RELATED"}, {"FORMSFOR", "FORMHINT"},
      {"FORMSFOR", "LISTFORMS"}, {"FORMSFOR", "HASFORM"},
      {"TIPS", "FORMSFOR"}, {"TIPS", "WHY"}, {"TIPS", "FORMHINT"}, {"TIPS", "RELATED"},
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
      {"SETP", "GETP"}, {"SETP", "NEEDP"}, {"SETP", "DEFAULTP"}, {"SETP", "INCP"},
      {"SETP", "DELP"}, {"SETP", "SAVEPLATE"}, {"SETP", "LOADPLATE"},
      {"GETP", "SETP"}, {"GETP", "NEEDP"}, {"GETP", "GETPN"}, {"GETP", "HASP"},
      {"NEEDP", "SETP"}, {"NEEDP", "GETP"}, {"NEEDP", "HASP"}, {"NEEDP", "DEFAULTP"},
      {"DEFAULTP", "SETP"}, {"DEFAULTP", "NEEDP"}, {"INCP", "SETP"}, {"INCP", "GETPN"},
      {"DELP", "SETP"}, {"DELP", "GETP"}, {"PRETTYP", "DUMPP"}, {"PRETTYP", "SAVEPLATE"},
      {"DUMPP", "PRETTYP"}, {"DUMPP", "GETP"}, {"SAVEPLATE", "LOADPLATE"},
      {"SAVEPLATE", "SETP"}, {"LOADPLATE", "SAVEPLATE"}, {"LOADPLATE", "NEEDP"},
      {"VARROOM", "HASVARROOM"}, {"VARROOM", "NEEDVARROOM"}, {"VARROOM", "VARS"},
      {"HASVARROOM", "NEEDVARROOM"}, {"HASVARROOM", "VARROOM"},
      {"NEEDVARROOM", "HASVARROOM"}, {"NEEDVARROOM", "VARROOM"},
      {"REMAIN_MS", "HAS_TIME"}, {"REMAIN_MS", "NEEDTIME"}, {"REMAIN_MS", "WALL_MS"},
      {"HAS_TIME", "NEEDTIME"}, {"HAS_TIME", "REMAIN_MS"},
      {"NEEDTIME", "HAS_TIME"}, {"NEEDTIME", "REMAIN_MS"},
      {"ASSERT", "EXPECT"}, {"ASSERT", "WHY"}, {"ASSERT", "FAIL"}, {"ASSERT", "PASS"},
      {"EXPECT", "ASSERT"}, {"EXPECT", "WHY"}, {"EXPECT", "CLEAR_ERR"}, {"EXPECT", "FAIL"},
      {"FAIL", "PASS"}, {"FAIL", "WHY"}, {"FAIL", "CLEAR_ERR"}, {"FAIL", "EXPECT"},
      {"PASS", "FAIL"}, {"PASS", "NOTE"}, {"WHY", "CLEAR_ERR"}, {"WHY", "STATUS"},
      {"WHY", "TIPS"}, {"CLEAR_ERR", "WHY"}, {"CLEAR_ERR", "STATUS"},
      {"STATUS", "IDENTITY"}, {"STATUS", "VERSION"}, {"STATUS", "VARS"},
      {"VERSION", "REQUIRE VERSION"}, {"VERSION", "STATUS"},
      {"EXIT", "PASS"}, {"EXIT", "FAIL"}, {"EXIT", "STATUS"},
      {"NOTE", "PASS"}, {"NOTE", "TIPS"},
      {"SMX", "SERVE"}, {"SMX", "DIAL"}, {"SMX", "TALK"}, {"SMX", "EXCHANGE"},
      {"SERVE", "DIAL"}, {"SERVE", "SMX"}, {"DIAL", "SERVE"}, {"DIAL", "SMX"},
      {"TALK", "SMX"}, {"TALK", "EXCHANGE"}, {"EXCHANGE", "SMX"}, {"EXCHANGE", "TALK"},
      {"HOLD_FLASH", "STATUS"}, {"HOLD_FLASH", "VERSION"}, {"HOLD_FLASH", "TIPS"},
      /* multi CLI flag contract (HASFORMS twin for --flags) */
      {"HASFLAG", "HASFLAGALL"}, {"HASFLAG", "NEEDFLAGS"}, {"HASFLAG", "REQUIRE FLAG"},
      {"HASFLAGALL", "NEEDFLAGS"}, {"HASFLAGALL", "HASFLAG"}, {"HASFLAGALL", "FLAGMISS"},
      {"HASFLAGALL", "HASFLAGS"}, {"HASFLAGALL", "LISTFLAGS"}, {"HASFLAGALL", "HASFLAGANY"},
      {"NEEDFLAGS", "HASFLAGALL"}, {"NEEDFLAGS", "HASFLAG"}, {"NEEDFLAGS", "REQUIRE FLAG"},
      {"NEEDFLAGS", "USAGE"}, {"NEEDFLAGS", "FLAGMISS"}, {"NEEDFLAGS", "NEEDFLAGANY"},
      /* multi named-flag ANY gate (HASFLAGALL twin) */
      {"HASFLAGANY", "NEEDFLAGANY"}, {"HASFLAGANY", "HASFLAGALL"}, {"HASFLAGANY", "HASFLAG"},
      {"HASFLAGANY", "FLAGHAVE"}, {"HASFLAGANY", "LISTFLAGS"}, {"HASFLAGANY", "FLAGMISS"},
      {"NEEDFLAGANY", "HASFLAGANY"}, {"NEEDFLAGANY", "NEEDFLAGS"}, {"NEEDFLAGANY", "HASFLAG"},
      {"NEEDFLAGANY", "USAGE"}, {"NEEDFLAGANY", "FLAGMISS"},
      {"HASFLAGS", "HASFLAGALL"}, {"HASFLAGS", "LISTFLAGS"}, {"HASFLAGS", "HASFLAG"},
      {"LISTFLAGS", "HASFLAGS"}, {"LISTFLAGS", "HASFLAGALL"}, {"LISTFLAGS", "FLAGMAP"},
      /* multi CLI arg contract (HASFLAGALL twin for CUBALC_ARGn) */
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
      /* multi host-env ALL gate (HASARGALL twin for process env) */
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
    };
    const char *name = (argc > 2 && argv[2][0]) ? argv[2] : "";
    char nup[96];
    char forms_json[2048];
    size_t k, fo = 0;
    int i, n = 0, nall = (int)(sizeof edges / sizeof edges[0]), seen = 0;
    if (!name[0]) {
      fprintf(stderr, "usage: cubalc related|seealso <FormName>\n");
      printf("{\"schema\":\"cubalc.related.v1\",\"ok\":false,\"cmd\":\"related\","
             "\"err\":\"need form name\",\"version\":\"%s\"}\n",
             CUBALC_LANG_VERSION);
      return 2;
    }
    for (k = 0; name[k] && k + 1 < sizeof nup; k++) {
      char c = name[k];
      if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
      nup[k] = c;
    }
    nup[k] = 0;
    forms_json[0] = 0;
    for (i = 0; i < nall; i++) {
      char fup[96];
      size_t j, ln;
      for (j = 0; edges[i].form[j] && j + 1 < sizeof fup; j++) {
        char c = edges[i].form[j];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        fup[j] = c;
      }
      fup[j] = 0;
      if (strcmp(fup, nup) != 0) continue;
      seen = 1;
      ln = strlen(edges[i].rel);
      if (fo && fo + 1 < sizeof forms_json) forms_json[fo++] = ',';
      if (fo + ln + 3 < sizeof forms_json) {
        forms_json[fo++] = '"';
        memcpy(forms_json + fo, edges[i].rel, ln);
        fo += ln;
        forms_json[fo++] = '"';
        forms_json[fo] = 0;
      }
      n++;
    }
    printf("{\"schema\":\"cubalc.related.v1\",\"ok\":%s,\"cmd\":\"related\","
           "\"form\":\"%s\",\"n\":%d,\"version\":\"%s\","
           "\"note\":\"related form-name bag · dual of in-lang RELATED · complements formhint/formsfor\","
           "\"related\":[%s]}\n",
           (seen && n > 0) ? "true" : "false", name, n, CUBALC_LANG_VERSION,
           forms_json);
    return (seen && n > 0) ? 0 : 1;
}
