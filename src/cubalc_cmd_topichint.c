/* CubalC CLI topichint/relatedtopic/formtopics — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_topichint.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int cubalc_cmd_topichint(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "topichint";
  if (strcmp(cmd, "topichint") == 0 || strcmp(cmd, "describetopic") == 0 ||
      strcmp(cmd, "topicdoc") == 0 || strcmp(cmd, "topichelp") == 0 ||
      strcmp(cmd, "hinttopic") == 0 || strcmp(cmd, "topic-hint") == 0) {
    /* Usability: one-line topic hint (dual of TOPICHINT / FORMHINT for topics).
     *   cubalc topichint cap · cubalc describetopic plate
     * Schema cubalc.topichint.v1 */
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
    const char *name = (argc > 2 && argv[2][0]) ? argv[2] : "";
    char tup[32];
    size_t k;
    int i, nall = (int)(sizeof rows / sizeof rows[0]);
    const char *hint = NULL;
    if (!name[0]) {
      fprintf(stderr, "usage: cubalc topichint|describetopic <topic>\n");
      printf("{\"schema\":\"cubalc.topichint.v1\",\"ok\":false,\"cmd\":\"topichint\","
             "\"err\":\"need topic name\",\"version\":\"%s\"}\n",
             CUBALC_LANG_VERSION);
      return 2;
    }
    for (k = 0; name[k] && k + 1 < sizeof tup; k++)
      tup[k] = (char)((name[k] >= 'A' && name[k] <= 'Z')
                          ? name[k] - 'A' + 'a' : name[k]);
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
    for (i = 0; i < nall; i++)
      if (!strcmp(rows[i].id, tup)) { hint = rows[i].hint; break; }
    if (!hint) {
      printf("{\"schema\":\"cubalc.topichint.v1\",\"ok\":false,\"cmd\":\"topichint\","
             "\"topic\":\"%s\",\"err\":\"unknown topic\",\"version\":\"%s\","
             "\"note\":\"LISTTOPICS · cubalc topics · HASTOPIC\"}\n",
             tup, CUBALC_LANG_VERSION);
      return 1;
    }
    {
      char hj[280];
      size_t j, o = 0;
      for (j = 0; hint[j] && o + 2 < sizeof hj; j++) {
        char c = hint[j];
        if (c == '"' || c == '\\') { hj[o++] = '\\'; hj[o++] = c; }
        else hj[o++] = c;
      }
      hj[o] = 0;
      printf("{\"schema\":\"cubalc.topichint.v1\",\"ok\":true,\"cmd\":\"topichint\","
             "\"topic\":\"%s\",\"hint\":\"%s\",\"version\":\"%s\","
             "\"note\":\"one-line topic docs · dual of TOPICHINT · twin of formhint\"}\n",
             tup, hj, CUBALC_LANG_VERSION);
    }
    return 0;
  }
  if (strcmp(cmd, "relatedtopic") == 0 || strcmp(cmd, "seetopics") == 0 ||
      strcmp(cmd, "relatedtopics") == 0 || strcmp(cmd, "topicrel") == 0 ||
      strcmp(cmd, "neartopics") == 0 || strcmp(cmd, "topic-neighbors") == 0) {
    /* Usability: related discovery topic bag (dual of RELATEDTOPIC / RELATED for topics).
     *   cubalc relatedtopic cap · cubalc seetopics plate
     * Schema cubalc.relatedtopic.v1 */
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
    const char *name = (argc > 2 && argv[2][0]) ? argv[2] : "";
    char tup[32], relj[256];
    size_t k, o = 0;
    int i, n = 0, nall = (int)(sizeof edges / sizeof edges[0]);
    int first = 1, seen = 0;
    if (!name[0]) {
      fprintf(stderr, "usage: cubalc relatedtopic|seetopics <topic>\n");
      printf("{\"schema\":\"cubalc.relatedtopic.v1\",\"ok\":false,\"cmd\":\"relatedtopic\","
             "\"err\":\"need topic name\",\"version\":\"%s\"}\n",
             CUBALC_LANG_VERSION);
      return 2;
    }
    for (k = 0; name[k] && k + 1 < sizeof tup; k++)
      tup[k] = (char)((name[k] >= 'A' && name[k] <= 'Z')
                          ? name[k] - 'A' + 'a' : name[k]);
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
    relj[0] = 0;
    for (i = 0; i < nall; i++) {
      if (strcmp(edges[i].topic, tup) != 0) continue;
      seen = 1;
      if (o + 24 < sizeof relj) {
        if (!first) relj[o++] = ',';
        o += (size_t)snprintf(relj + o, sizeof relj - o, "\"%s\"", edges[i].rel);
        first = 0;
        n++;
      }
    }
    if (!seen || n == 0) {
      printf("{\"schema\":\"cubalc.relatedtopic.v1\",\"ok\":false,\"cmd\":\"relatedtopic\","
             "\"topic\":\"%s\",\"err\":\"unknown topic\",\"version\":\"%s\","
             "\"note\":\"LISTTOPICS · cubalc topics · HASTOPIC\"}\n",
             tup, CUBALC_LANG_VERSION);
      return 1;
    }
    printf("{\"schema\":\"cubalc.relatedtopic.v1\",\"ok\":true,\"cmd\":\"relatedtopic\","
           "\"topic\":\"%s\",\"n\":%d,\"version\":\"%s\","
           "\"note\":\"related topic bag · dual of RELATEDTOPIC · twin of related for topics\","
           "\"related\":[%s]}\n",
           tup, n, CUBALC_LANG_VERSION, relj);
    return 0;
  }
  if (strcmp(cmd, "formtopics") == 0 || strcmp(cmd, "topicsof") == 0 ||
      strcmp(cmd, "topicsfor") == 0 || strcmp(cmd, "form-topics") == 0 ||
      strcmp(cmd, "covertopics") == 0 || strcmp(cmd, "topics-of") == 0) {
    /* Usability: reverse FORMSFOR — form name → discovery topics covering it.
     *   cubalc formtopics HASFORM · cubalc topicsof SETP
     * Schema cubalc.formtopics.v1 · dual of FORMTOPICS / complement of formsfor. */
    static const struct { const char *topic; const char *form; } rows[] = {
      {"general", "VERSION"}, {"general", "STATUS"}, {"general", "IDENTITY"},
      {"general", "TIPS"}, {"general", "FORMSFOR"}, {"general", "LISTTOPICS"},
      {"general", "TOPICHINT"}, {"general", "RELATEDTOPIC"}, {"general", "FORMTOPICS"},
      {"general", "WHY"}, {"general", "CLEAR_ERR"}, {"general", "INCLUDE"},
      {"general", "REQUIRE VERSION"}, {"general", "VARS"}, {"general", "TOPIC"},
      {"general", "SNIP"}, {"general", "RUNSNIP"}, {"general", "ERRTIPS"},
      {"general", "ERRRUN"}, {"general", "HASTOPIC"}, {"general", "NEEDTOPIC"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORM"}, {"cap", "HASFORMS"},
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
    const char *name = (argc > 2 && argv[2][0]) ? argv[2] : "";
    char nup[96], relj[256];
    char seen_topics[16][32];
    size_t k, o = 0;
    int i, n = 0, nall = (int)(sizeof rows / sizeof rows[0]);
    int first = 1, nseen = 0;
    if (!name[0]) {
      fprintf(stderr, "usage: cubalc formtopics|topicsof <FormName>\n");
      printf("{\"schema\":\"cubalc.formtopics.v1\",\"ok\":false,\"cmd\":\"formtopics\","
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
    relj[0] = 0;
    for (i = 0; i < nall; i++) {
      char fup[96];
      size_t j;
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
      if (o + 24 < sizeof relj) {
        if (!first) relj[o++] = ',';
        o += (size_t)snprintf(relj + o, sizeof relj - o, "\"%s\"", rows[i].topic);
        first = 0;
        n++;
      }
    }
    if (n == 0) {
      printf("{\"schema\":\"cubalc.formtopics.v1\",\"ok\":false,\"cmd\":\"formtopics\","
             "\"form\":\"%s\",\"err\":\"no covering topics\",\"version\":\"%s\","
             "\"note\":\"FORMHINT · FORMSFOR · LISTFORMS · cubalc formsfor\"}\n",
             nup, CUBALC_LANG_VERSION);
      return 1;
    }
    printf("{\"schema\":\"cubalc.formtopics.v1\",\"ok\":true,\"cmd\":\"formtopics\","
           "\"form\":\"%s\",\"n\":%d,\"version\":\"%s\","
           "\"note\":\"topics covering form · dual of FORMTOPICS · reverse of formsfor\","
           "\"topics\":[%s]}\n",
           nup, n, CUBALC_LANG_VERSION, relj);
    return 0;
  }
  return -1;
}
