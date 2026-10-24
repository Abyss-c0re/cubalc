/* CubalC CLI formsfor/discover — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_formsfor.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int cubalc_cmd_formsfor(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "formsfor";
  if (strcmp(cmd, "formsfor") == 0 || strcmp(cmd, "topicforms") == 0 ||
      strcmp(cmd, "formstopic") == 0 || strcmp(cmd, "surface") == 0 ||
      strcmp(cmd, "forms-for") == 0 || strcmp(cmd, "capforms") == 0) {
    /* Usability: curated form-name bags by topic (dual of FORMSFOR / TIPS).
     *   cubalc formsfor · cubalc formsfor cap · cubalc topicforms fat
     * Schema cubalc.formsfor.v1 · agents get capability surface without LISTFORMS soup. */
    static const struct {
      const char *topic;
      const char *form;
    } rows[] = {
      {"general", "VERSION"},
      {"general", "STATUS"},
      {"general", "IDENTITY"},
      {"general", "TIPS"},
      {"general", "FORMSFOR"},
      {"general", "WHY"},
      {"general", "CLEAR_ERR"},
      {"general", "INCLUDE"},
      {"general", "REQUIRE VERSION"},
      {"general", "VARS"},
      {"cap", "HASFORM"},
      {"cap", "NEEDFORM"},
      {"cap", "HASFORMS"},
      {"cap", "NEEDFORMS"},
      {"cap", "FORMHINT"},
      {"cap", "LISTFORMS"},
      {"cap", "COUNTFORMS"},
      {"cap", "REQUIRE FORM"},
      {"cap", "TIPS"},
      {"cap", "FORMSFOR"},
      {"fat", "VARROOM"},
      {"fat", "HASVARROOM"},
      {"fat", "NEEDVARROOM"},
      {"fat", "REMAIN_MS"},
      {"fat", "HAS_TIME"},
      {"fat", "NEEDTIME"},
      {"fat", "STATUS"},
      {"fat", "VARS"},
      {"fat", "TIPS"},
      {"plate", "SETP"},
      {"plate", "GETP"},
      {"plate", "NEEDP"},
      {"plate", "DEFAULTP"},
      {"plate", "INCP"},
      {"plate", "DELP"},
      {"plate", "PRETTYP"},
      {"plate", "DUMPP"},
      {"plate", "SAVEPLATE"},
      {"plate", "LOADPLATE"},
      {"p2p", "SMX"},
      {"p2p", "SERVE"},
      {"p2p", "DIAL"},
      {"p2p", "TALK"},
      {"p2p", "EXCHANGE"},
      {"p2p", "TIPS"},
      {"run", "ASSERT"},
      {"run", "EXPECT"},
      {"run", "FAIL"},
      {"run", "PASS"},
      {"run", "NOTE"},
      {"run", "EXIT"},
      {"run", "WHY"},
      {"run", "CLEAR_ERR"},
      {"run", "TIPS"},
      {"lib", "LISTLIBS"},
      {"lib", "HASLIB"},
      {"lib", "MATCHLIBS"},
      {"lib", "PICKLIB"},
      {"lib", "RECIPE"},
      {"lib", "CHECKDEPS"},
      {"lib", "SORTLIBS"},
      {"lib", "FRESHLIBS"},
      {"lib", "LIBAGE"},
      {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"},
      {"protect", "STATUS"},
      {"protect", "VERSION"},
      {"protect", "TIPS"},
    };
    const char *topic = (argc > 2 && argv[2][0]) ? argv[2] : "general";
    char tup[32];
    size_t k;
    int i, n = 0, nall = (int)(sizeof rows / sizeof rows[0]);
    for (k = 0; topic[k] && k + 1 < sizeof tup; k++)
      tup[k] = (char)((topic[k] >= 'A' && topic[k] <= 'Z')
                          ? topic[k] - 'A' + 'a' : topic[k]);
    tup[k] = 0;
    if (!strcmp(tup, "capability") || !strcmp(tup, "forms") || !strcmp(tup, "form"))
      snprintf(tup, sizeof tup, "%s", "cap");
    if (!strcmp(tup, "mesh") || !strcmp(tup, "smx") || !strcmp(tup, "peer"))
      snprintf(tup, sizeof tup, "%s", "p2p");
    if (!strcmp(tup, "nest") || !strcmp(tup, "var") || !strcmp(tup, "timeout"))
      snprintf(tup, sizeof tup, "%s", "fat");
    if (!strcmp(tup, "json") || !strcmp(tup, "agent"))
      snprintf(tup, sizeof tup, "%s", "plate");
    if (!strcmp(tup, "start") || !strcmp(tup, "help") || !strcmp(tup, "all") ||
        !strcmp(tup, "default") || !strcmp(tup, "*"))
      snprintf(tup, sizeof tup, "%s", "general");
    printf("{\"schema\":\"cubalc.formsfor.v1\",\"ok\":true,\"cmd\":\"formsfor\","
           "\"topic\":\"%s\",\"version\":\"%s\","
           "\"note\":\"curated form-name bag by topic · dual of in-lang FORMSFOR · TIPS twin\","
           "\"topics\":[\"general\",\"cap\",\"fat\",\"plate\",\"p2p\",\"run\",\"lib\",\"protect\"],"
           "\"forms\":[",
           tup, CUBALC_LANG_VERSION);
    for (i = 0; i < nall; i++) {
      if (strcmp(rows[i].topic, tup) != 0)
        continue;
      printf("%s\"%s\"", n ? "," : "", rows[i].form);
      n++;
    }
    printf("],\"n\":%d}\n", n);
    return n > 0 ? 0 : 1;
  }
  if (strcmp(cmd, "discover") == 0 || strcmp(cmd, "explore") == 0 ||
      strcmp(cmd, "discovertopics") == 0 || strcmp(cmd, "scout") == 0 ||
      strcmp(cmd, "opendiscover") == 0) {
    /* Usability: match bag + first-topic open plate (dual of DISCOVER).
     *   cubalc discover · cubalc discover p · cubalc explore cap
     * Schema cubalc.discover.v1 · agents skip MATCHTOPICS+TOPICHINT+GUIDE glue. */
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
    const char *needle = (argc > 2 && argv[2] && argv[2][0]) ? argv[2] : "";
    char nlow[64], matches[8][16], topics_j[256], rel_j[256], next_j[900], hj[320];
    const char *first = NULL, *hint = NULL;
    size_t k, o, j;
    int i, n = 0, nr = 0, nn = 0;
    int nall = (int)(sizeof topics / sizeof topics[0]);
    int n_hints = (int)(sizeof hints / sizeof hints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    nlow[0] = 0;
    if (needle[0]) {
      for (k = 0; needle[k] && k + 1 < sizeof nlow; k++)
        nlow[k] = (char)((needle[k] >= 'A' && needle[k] <= 'Z')
                             ? needle[k] - 'A' + 'a' : needle[k]);
      nlow[k] = 0;
      if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
        snprintf(nlow, sizeof nlow, "%s", "cap");
      if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
        snprintf(nlow, sizeof nlow, "%s", "p2p");
      if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
        snprintf(nlow, sizeof nlow, "%s", "fat");
      if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
        snprintf(nlow, sizeof nlow, "%s", "plate");
      if (!strcmp(nlow, "start") || !strcmp(nlow, "help") || !strcmp(nlow, "all") ||
          !strcmp(nlow, "default") || !strcmp(nlow, "*"))
        snprintf(nlow, sizeof nlow, "%s", "general");
      for (i = 0; i < nall && n < 8; i++) {
        if (!strstr(topics[i], nlow)) continue;
        snprintf(matches[n], sizeof matches[0], "%s", topics[i]);
        n++;
      }
    } else {
      for (i = 0; i < nall && n < 8; i++) {
        snprintf(matches[n], sizeof matches[0], "%s", topics[i]);
        n++;
      }
    }
    if (n == 0) {
      printf("{\"schema\":\"cubalc.discover.v1\",\"ok\":false,\"cmd\":\"discover\","
             "\"filter\":\"%s\",\"n\":0,\"err\":\"no match\",\"version\":\"%s\"}\n",
             nlow[0] ? nlow : "*", CUBALC_LANG_VERSION);
      return 1;
    }
    first = matches[0];
    for (i = 0; i < n_hints; i++)
      if (!strcmp(hints[i].id, first)) { hint = hints[i].hint; break; }
    if (!hint) hint = "";
    o = 0; topics_j[0] = 0;
    for (i = 0; i < n; i++) {
      if (o && o + 1 < sizeof topics_j) topics_j[o++] = ',';
      o += (size_t)snprintf(topics_j + o, sizeof topics_j - o, "\"%s\"", matches[i]);
    }
    o = 0; rel_j[0] = 0;
    for (i = 0; i < n_rels; i++) {
      if (strcmp(rels[i].topic, first) != 0) continue;
      if (o && o + 1 < sizeof rel_j) rel_j[o++] = ',';
      o += (size_t)snprintf(rel_j + o, sizeof rel_j - o, "\"%s\"", rels[i].rel);
      nr++;
    }
    o = 0; next_j[0] = 0;
    {
      char cmds[6][80];
      int ci, nc = 6;
      snprintf(cmds[0], sizeof cmds[0], "GUIDE %s", first);
      snprintf(cmds[1], sizeof cmds[1], "RUNSNIP %s", first);
      snprintf(cmds[2], sizeof cmds[2], "TOPIC %s", first);
      snprintf(cmds[3], sizeof cmds[3], "cubalc guide %s", first);
      if (nlow[0]) {
        snprintf(cmds[4], sizeof cmds[4], "GUIDE MATCH %s", nlow);
        snprintf(cmds[5], sizeof cmds[5], "RUNSNIP MATCH %s", nlow);
      } else {
        snprintf(cmds[4], sizeof cmds[4], "cubalc discover");
        snprintf(cmds[5], sizeof cmds[5], "LISTTOPICS");
      }
      for (ci = 0; ci < nc; ci++) {
        char esc[100];
        size_t e = 0;
        for (j = 0; cmds[ci][j] && e + 2 < sizeof esc; j++) {
          char c = cmds[ci][j];
          if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
          else esc[e++] = c;
        }
        esc[e] = 0;
        if (o && o + 1 < sizeof next_j) next_j[o++] = ',';
        if (o + e + 3 < sizeof next_j) {
          next_j[o++] = '"';
          memcpy(next_j + o, esc, e); o += e;
          next_j[o++] = '"'; next_j[o] = 0;
        }
        nn++;
      }
    }
    o = 0;
    for (j = 0; hint[j] && o + 2 < sizeof hj; j++) {
      char c = hint[j];
      if (c == '"' || c == '\\') { hj[o++] = '\\'; hj[o++] = c; }
      else hj[o++] = c;
    }
    hj[o] = 0;
    printf("{\"schema\":\"cubalc.discover.v1\",\"ok\":true,\"cmd\":\"discover\","
           "\"filter\":\"%s\",\"n\":%d,\"first\":\"%s\",\"hint\":\"%s\","
           "\"related_n\":%d,\"next_n\":%d,\"version\":\"%s\","
           "\"note\":\"match bag + first open · dual of DISCOVER · chain guide/runsnip\","
           "\"topics\":[%s],\"related\":[%s],\"next\":[%s]}\n",
           nlow[0] ? nlow : "*", n, first, hj, nr, nn, CUBALC_LANG_VERSION,
           topics_j, rel_j, next_j);
    return 0;
  }
  return 2;
}
