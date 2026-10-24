/* CubalC CLI open/topics — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_open.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int cubalc_cmd_open(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "open";
  if (strcmp(cmd, "open") == 0 || strcmp(cmd, "openplay") == 0 ||
      strcmp(cmd, "openguide") == 0 || strcmp(cmd, "playopen") == 0 ||
      strcmp(cmd, "discoverguide") == 0) {
    /* Usability: DISCOVER bag + GUIDE playbook for first (dual of OPEN).
     *   cubalc open · cubalc open p · cubalc openplay cap
     * Schema cubalc.open.v1 */
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
      {"general", "cubalc doctor"}, {"general", "INCLUDE discover_boot"},
      {"general", "OPEN general playbook"}, {"cap", "INCLUDE cap_boot"},
      {"cap", "HASFORM · NEEDFORMS"}, {"fat", "INCLUDE fat_session"},
      {"plate", "INCLUDE plate_session"}, {"p2p", "CUBALC_SMX_KEY · SERVE/DIAL"},
      {"run", "WHY after ASSERT"}, {"lib", "cubalc libs|recipe"},
      {"protect", "cubalc protect status"},
    };
    static const struct { const char *topic; const char *form; } forms[] = {
      {"general", "OPEN"}, {"general", "DISCOVER"}, {"general", "GUIDE"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORMS"},
      {"fat", "VARROOM"}, {"plate", "SETP"}, {"plate", "NEEDP"},
      {"p2p", "SMX"}, {"run", "ASSERT"}, {"lib", "INCLUDE"}, {"protect", "HOLD_FLASH"},
    };
    const char *needle = (argc > 2 && argv[2] && argv[2][0]) ? argv[2] : "";
    char nlow[64], matches[8][16], topics_j[256], rel_j[256], tips_j[2400], forms_j[1200], hj[320];
    const char *first = NULL, *hint = NULL;
    size_t k, o, j;
    int i, n = 0, nr = 0, nt = 0, nf = 0;
    int nall = (int)(sizeof topics / sizeof topics[0]);
    int n_hints = (int)(sizeof hints / sizeof hints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    int n_tips = (int)(sizeof tips / sizeof tips[0]);
    int n_forms = (int)(sizeof forms / sizeof forms[0]);
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
        snprintf(matches[n], sizeof matches[0], "%s", topics[i]); n++;
      }
    } else {
      for (i = 0; i < nall && n < 8; i++) {
        snprintf(matches[n], sizeof matches[0], "%s", topics[i]); n++;
      }
    }
    if (n == 0) {
      printf("{\"schema\":\"cubalc.open.v1\",\"ok\":false,\"cmd\":\"open\","
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
    o = 0; tips_j[0] = 0;
    for (i = 0; i < n_tips; i++) {
      char esc[200]; size_t e = 0;
      if (strcmp(tips[i].topic, first) != 0) continue;
      for (j = 0; tips[i].tip[j] && e + 2 < sizeof esc; j++) {
        char c = tips[i].tip[j];
        if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
        else esc[e++] = c;
      }
      esc[e] = 0;
      if (o && o + 1 < sizeof tips_j) tips_j[o++] = ',';
      if (o + e + 3 < sizeof tips_j) {
        tips_j[o++] = '"'; memcpy(tips_j + o, esc, e); o += e;
        tips_j[o++] = '"'; tips_j[o] = 0;
      }
      nt++;
    }
    o = 0; forms_j[0] = 0;
    for (i = 0; i < n_forms; i++) {
      size_t ln;
      if (strcmp(forms[i].topic, first) != 0) continue;
      ln = strlen(forms[i].form);
      if (o && o + 1 < sizeof forms_j) forms_j[o++] = ',';
      if (o + ln + 3 < sizeof forms_j) {
        forms_j[o++] = '"'; memcpy(forms_j + o, forms[i].form, ln); o += ln;
        forms_j[o++] = '"'; forms_j[o] = 0;
      }
      nf++;
    }
    o = 0;
    for (j = 0; hint[j] && o + 2 < sizeof hj; j++) {
      char c = hint[j];
      if (c == '"' || c == '\\') { hj[o++] = '\\'; hj[o++] = c; }
      else hj[o++] = c;
    }
    hj[o] = 0;
    printf("{\"schema\":\"cubalc.open.v1\",\"ok\":true,\"cmd\":\"open\","
           "\"filter\":\"%s\",\"n\":%d,\"first\":\"%s\",\"hint\":\"%s\","
           "\"related_n\":%d,\"tips_n\":%d,\"forms_n\":%d,\"version\":\"%s\","
           "\"note\":\"DISCOVER bag + GUIDE first · dual of OPEN · chain runsnip\","
           "\"topics\":[%s],\"related\":[%s],\"tips\":[%s],\"forms\":[%s]}\n",
           nlow[0] ? nlow : "*", n, first, hj, nr, nt, nf, CUBALC_LANG_VERSION,
           topics_j, rel_j, tips_j, forms_j);
    return 0;
  }
  if (strcmp(cmd, "topics") == 0 || strcmp(cmd, "listtopics") == 0 ||
      strcmp(cmd, "topiclist") == 0 || strcmp(cmd, "discovery-topics") == 0 ||
      strcmp(cmd, "hastopic") == 0 || strcmp(cmd, "needtopic") == 0) {
    /* Usability: discovery topic catalog (dual of LISTTOPICS/HASTOPIC).
     *   cubalc topics · cubalc hastopic cap · cubalc needtopic plate
     * Schema cubalc.topics.v1 · agents list topics without hardcoding. */
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    static const char *hints[] = {
      "install/doctor/init surface",
      "HASFORM/NEEDFORMS capability floor",
      "VARROOM/REMAIN_MS fat nest budget",
      "SETP/NEEDP plate agent JSON",
      "SMX SERVE/DIAL mesh",
      "ASSERT/EXPECT/WHY run probes",
      "LISTLIBS/RECIPE INCLUDE discovery",
      "HOLD_FLASH/protect status"
    };
    int nall = (int)(sizeof topics / sizeof topics[0]);
    int i, is_has = (strcmp(cmd, "hastopic") == 0);
    int is_need = (strcmp(cmd, "needtopic") == 0);
    if (is_has || is_need) {
      const char *name = (argc > 2 && argv[2][0]) ? argv[2] : "";
      char tup[32];
      size_t k;
      int hit = 0;
      if (!name[0]) {
        fprintf(stderr, "usage: cubalc hastopic|needtopic <topic>\n");
        printf("{\"schema\":\"cubalc.topics.v1\",\"ok\":false,\"cmd\":\"%s\","
               "\"err\":\"need topic name\",\"version\":\"%s\"}\n",
               cmd, CUBALC_LANG_VERSION);
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
        if (!strcmp(topics[i], tup)) { hit = 1; break; }
      printf("{\"schema\":\"cubalc.topics.v1\",\"ok\":%s,\"cmd\":\"%s\","
             "\"topic\":\"%s\",\"known\":%s,\"mode\":\"%s\",\"version\":\"%s\","
             "\"note\":\"CLI dual of HASTOPIC/NEEDTOPIC · cubalc topics catalog\"}\n",
             hit ? "true" : "false", cmd, tup, hit ? "true" : "false",
             is_need ? "need" : "has", CUBALC_LANG_VERSION);
      if (is_need) return hit ? 0 : 1;
      return 0;
    }
    printf("{\"schema\":\"cubalc.topics.v1\",\"ok\":true,\"cmd\":\"topics\","
           "\"n\":%d,\"version\":\"%s\","
           "\"note\":\"discovery topic catalog · dual of LISTTOPICS · use with topic/tips/snip/runsnip\","
           "\"topics\":[",
           nall, CUBALC_LANG_VERSION);
    for (i = 0; i < nall; i++) {
      printf("%s{\"id\":\"%s\",\"hint\":\"%s\"}",
             i ? "," : "", topics[i], hints[i]);
    }
    printf("]}\n");
    return 0;
  }

  return 2;
}
