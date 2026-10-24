/* CubalC CLI tips/nthtopic — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_tips.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int cubalc_cmd_tips(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "tips";
  if (strcmp(cmd, "tips") == 0 || strcmp(cmd, "tip") == 0 ||
      strcmp(cmd, "howto") == 0 || strcmp(cmd, "next") == 0 ||
      strcmp(cmd, "playbook") == 0) {
    /* Usability: curated agent next-steps by topic without cookbook prose.
     *   cubalc tips · cubalc tips cap · cubalc tips run
     * Schema cubalc.tips.v1 · dual of in-lang TIPS. */
    static const struct {
      const char *topic;
      const char *tip;
    } tips[] = {
      {"general", "cubalc doctor — install readiness plate (libs, vars_max, nest)"},
      {"general", "cubalc init --list — scaffold catalog (agent_boot · plate · fat · cap)"},
      {"general", "cubalc search KEYWORD — forms/libs/examples/env discovery"},
      {"general", "HOLD_FLASH is device/mesh-join only · default 1 · not language PLUG"},
      {"general", "INCLUDE agent_boot — REQUIRE VERSION + VERSION plate one-shot"},
      {"cap", "INCLUDE cap_boot — agent_boot + form_guard capability contract"},
      {"cap", "cubalc init --cap path — scaffold capability starter"},
      {"cap", "HASFORM SORTLIBS · NEEDFORMS a b · FORMHINT name for docs"},
      {"cap", "cubalc run -C SORTLIBS,LIBAGE — host form floor (CUBALC_REQUIRE_FORMS)"},
      {"cap", "LISTFORMS SORT · COUNTFORMS FORM · cubalc listforms dual"},
      {"fat", "INCLUDE fat_session — durable nest (fat_boot + plate_boot)"},
      {"fat", "cubalc init --fat-session nest — scaffold durable fat nest"},
      {"fat", "DEFAULT NEED_VARROOM=48 · INCLUDE var_guard before nest specials"},
      {"fat", "cubalc run -T 5000 · INCLUDE time_guard · REMAIN_MS mid-run"},
      {"plate", "INCLUDE plate_session · SETP/NEEDP/SAVEPLATE agent JSON"},
      {"plate", "cubalc plate get|set|uniform path.json — CLI without .cubalc"},
      {"plate", "INCLUDE plate_uniform · PRETTYP for nest inspect without jq"},
      {"plate", "cubalc init --plate — plate_session + uniform + pretty starter"},
      {"p2p", "export CUBALC_SMX_KEY=$(openssl rand -hex 32) before mesh"},
      {"p2p", "SMX SERVE/DIAL · CUBALC_P2P_TIMEOUT · CUBALC_P2P_SOFT soft-fail"},
      {"p2p", "cubalc smx-bus prove-tcp · docs/P2P_SMX.md"},
      {"run", "cubalc run -q -e '…' — plate-only inline program"},
      {"run", "cubalc run -I lib -L dir -R 1.15 -C FORMS -T ms file"},
      {"run", "cubalc run -s / CUBALC_STRICT — soft last_err fails exit"},
      {"run", "WHY / why_hint on run plate after ASSERT or sticky LAST_ERR"},
      {"lib", "cubalc libs [filter] · recipe <lib> · checkdeps <lib>"},
      {"lib", "INCLUDE MATCH|NEWEST needle · NEWESTLIB · SORTLIBS mtime bag"},
      {"lib", "FRESHLIBS needle max_age · LIBAGE name · HASFRESH gate"},
      {"protect", "cubalc protect status — plate summary no board run"},
      {"protect", "cubalc protect all · docs/CORE_PROTECT.md · HOLD_FLASH"},
    };
    const char *topic = (argc > 2 && argv[2][0]) ? argv[2] : "general";
    char tup[32];
    size_t k;
    int i, n = 0, nall = (int)(sizeof tips / sizeof tips[0]);
    for (k = 0; topic[k] && k + 1 < sizeof tup; k++)
      tup[k] = (char)((topic[k] >= 'A' && topic[k] <= 'Z')
                          ? topic[k] - 'A' + 'a' : topic[k]);
    tup[k] = 0;
    /* aliases */
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
    printf("{\"schema\":\"cubalc.tips.v1\",\"ok\":true,\"cmd\":\"tips\","
           "\"topic\":\"%s\",\"version\":\"%s\","
           "\"note\":\"curated agent next-steps · dual of in-lang TIPS · no cookbook\","
           "\"topics\":[\"general\",\"cap\",\"fat\",\"plate\",\"p2p\",\"run\",\"lib\",\"protect\"],"
           "\"tips\":[",
           tup, CUBALC_LANG_VERSION);
    for (i = 0; i < nall; i++) {
      int hit = 0;
      if (!strcmp(tips[i].topic, tup) || !strcmp(tup, "general")) {
        /* general shows general only; other topics exact */
        if (!strcmp(tup, "general"))
          hit = !strcmp(tips[i].topic, "general");
        else
          hit = !strcmp(tips[i].topic, tup);
      }
      if (!hit) continue;
      /* JSON-escape tip */
      {
        char esc[240];
        size_t j, o = 0;
        const char *s = tips[i].tip;
        for (j = 0; s[j] && o + 2 < sizeof esc; j++) {
          char c = s[j];
          if (c == '"' || c == '\\') {
            esc[o++] = '\\';
            esc[o++] = c;
          } else if ((unsigned char)c < 32)
            esc[o++] = ' ';
          else
            esc[o++] = c;
        }
        esc[o] = 0;
        printf("%s\"%s\"", n ? "," : "", esc);
        n++;
      }
    }
    printf("],\"n\":%d}\n", n);
    return n > 0 ? 0 : 1;
  }
  if (strcmp(cmd, "nthtopic") == 0 || strcmp(cmd, "indextopic") == 0 ||
      strcmp(cmd, "topicnth") == 0 || strcmp(cmd, "lasttopic") == 0 ||
      strcmp(cmd, "endtopic") == 0 || strcmp(cmd, "topiclast") == 0) {
    /* Usability: Nth/last matching topic (dual of NTHTOPIC/LASTTOPIC).
     *   cubalc nthtopic 1 p · cubalc lasttopic p
     * Schema cubalc.topicmatch.v1 */
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    int mode_last = (strcmp(cmd, "lasttopic") == 0 || strcmp(cmd, "endtopic") == 0 ||
                     strcmp(cmd, "topiclast") == 0);
    int idx = 0, ai = 2, i, n = 0, nall = (int)(sizeof topics / sizeof topics[0]);
    const char *needle = "";
    const char *fallback = NULL;
    char nlow[64];
    char matches[8][16];
    size_t k;
    const char *pick = NULL;
    if (!mode_last) {
      if (argc <= 3 || !argv[2] || !argv[2][0]) {
        fprintf(stderr, "usage: cubalc nthtopic <idx> <needle> [OR fallback]\n");
        printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"nthtopic\","
               "\"err\":\"need idx and needle\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 2;
      }
      idx = atoi(argv[2]);
      needle = argv[3] ? argv[3] : "";
      ai = 4;
    } else {
      if (argc <= 2 || !argv[2] || !argv[2][0]) {
        fprintf(stderr, "usage: cubalc lasttopic <needle> [OR fallback]\n");
        printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"lasttopic\","
               "\"err\":\"need needle\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[2];
      ai = 3;
    }
    if (argc > ai + 1 && argv[ai] &&
        (!strcmp(argv[ai], "OR") || !strcmp(argv[ai], "or") ||
         !strcmp(argv[ai], "ELSE") || !strcmp(argv[ai], "DEFAULT")) &&
        argv[ai + 1] && argv[ai + 1][0])
      fallback = argv[ai + 1];
    if (!needle[0]) {
      printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need needle\",\"version\":\"%s\"}\n",
             mode_last ? "lasttopic" : "nthtopic", CUBALC_LANG_VERSION);
      return 2;
    }
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
    if (!strcmp(nlow, "start") || !strcmp(nlow, "all") || !strcmp(nlow, "help") ||
        !strcmp(nlow, "default") || !strcmp(nlow, "*"))
      snprintf(nlow, sizeof nlow, "%s", "general");
    for (i = 0; i < nall && n < 8; i++) {
      if (!strstr(topics[i], nlow)) continue;
      snprintf(matches[n], sizeof matches[0], "%s", topics[i]);
      n++;
    }
    if (mode_last) {
      if (n > 0) { pick = matches[n - 1]; idx = n - 1; }
    } else {
      if (idx < 0) idx = 0;
      if (idx < n) pick = matches[idx];
    }
    if (!pick) {
      if (fallback && fallback[0]) {
        printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":true,\"cmd\":\"%s\","
               "\"filter\":\"%s\",\"idx\":%d,\"topic\":\"%s\",\"fallback\":true,"
               "\"match_n\":%d,\"version\":\"%s\"}\n",
               mode_last ? "lasttopic" : "nthtopic",
               nlow, idx, fallback, n, CUBALC_LANG_VERSION);
        return 0;
      }
      printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"filter\":\"%s\",\"idx\":%d,\"match_n\":%d,\"err\":\"no match\","
             "\"version\":\"%s\"}\n",
             mode_last ? "lasttopic" : "nthtopic",
             nlow, idx, n, CUBALC_LANG_VERSION);
      return 1;
    }
    printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":true,\"cmd\":\"%s\","
           "\"filter\":\"%s\",\"idx\":%d,\"topic\":\"%s\",\"match_n\":%d,"
           "\"version\":\"%s\","
           "\"note\":\"Nth/last matching topic · dual of NTHTOPIC/LASTTOPIC\"}\n",
           mode_last ? "lasttopic" : "nthtopic",
           nlow, idx, pick, n, CUBALC_LANG_VERSION);
    return 0;
  }
  return -1;
}
