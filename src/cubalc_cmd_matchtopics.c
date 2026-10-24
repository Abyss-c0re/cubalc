/* CubalC CLI matchtopics/hasmatchtopics — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_matchtopics.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int cubalc_cmd_matchtopics(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "matchtopics";
  if (strcmp(cmd, "matchtopics") == 0 || strcmp(cmd, "filtertopics") == 0 ||
      strcmp(cmd, "topicmatch") == 0 || strcmp(cmd, "greptopics") == 0 ||
      strcmp(cmd, "picktopic") == 0 || strcmp(cmd, "firsttopic") == 0 ||
      strcmp(cmd, "topicpick") == 0) {
    /* Usability: filter/first discovery topic (dual of MATCHTOPICS/PICKTOPIC).
     * Schema cubalc.topicmatch.v1 */
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    const char *needle = (argc > 2 && argv[2][0]) ? argv[2] : "";
    const char *fallback = NULL;
    char nlow[64], bagj[256];
    size_t k, o = 0;
    int i, n = 0, nall = (int)(sizeof topics / sizeof topics[0]);
    int is_pick = (strcmp(cmd, "picktopic") == 0 || strcmp(cmd, "firsttopic") == 0 ||
                   strcmp(cmd, "topicpick") == 0);
    int first = 1;
    const char *pick = NULL;
    if (!needle[0]) {
      fprintf(stderr, "usage: cubalc matchtopics|picktopic <needle> [OR fallback]\n");
      printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need needle\",\"version\":\"%s\"}\n",
             is_pick ? "picktopic" : "matchtopics", CUBALC_LANG_VERSION);
      return 2;
    }
    if (is_pick && argc > 3 &&
        (!strcmp(argv[3], "OR") || !strcmp(argv[3], "or") ||
         !strcmp(argv[3], "ELSE") || !strcmp(argv[3], "DEFAULT")) &&
        argc > 4 && argv[4][0])
      fallback = argv[4];
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
    bagj[0] = 0;
    for (i = 0; i < nall; i++) {
      if (!strstr(topics[i], nlow)) continue;
      if (!pick) pick = topics[i];
      if (!first && o + 1 < sizeof bagj) bagj[o++] = ',';
      o += (size_t)snprintf(bagj + o, sizeof bagj - o, "\"%s\"", topics[i]);
      first = 0;
      n++;
    }
    if (is_pick) {
      if (!pick) {
        if (fallback && fallback[0]) {
          printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":true,\"cmd\":\"picktopic\","
                 "\"filter\":\"%s\",\"topic\":\"%s\",\"fallback\":true,\"n\":1,"
                 "\"version\":\"%s\"}\n",
                 nlow, fallback, CUBALC_LANG_VERSION);
          return 0;
        }
        printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"picktopic\","
               "\"filter\":\"%s\",\"err\":\"no match\",\"version\":\"%s\"}\n",
               nlow, CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":true,\"cmd\":\"picktopic\","
             "\"filter\":\"%s\",\"topic\":\"%s\",\"n\":1,\"version\":\"%s\"}\n",
             nlow, pick, CUBALC_LANG_VERSION);
      return 0;
    }
    if (n == 0) {
      printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"matchtopics\","
             "\"filter\":\"%s\",\"n\":0,\"err\":\"empty\",\"version\":\"%s\"}\n",
             nlow, CUBALC_LANG_VERSION);
      return 1;
    }
    printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":true,\"cmd\":\"matchtopics\","
           "\"filter\":\"%s\",\"n\":%d,\"version\":\"%s\",\"topics\":[%s]}\n",
           nlow, n, CUBALC_LANG_VERSION, bagj);
    return 0;
  }
  if (strcmp(cmd, "hasmatchtopics") == 0 || strcmp(cmd, "hastopicmatch") == 0 ||
      strcmp(cmd, "needmatchtopics") == 0 || strcmp(cmd, "needtopicmatch") == 0 ||
      strcmp(cmd, "countmatchtopics") == 0 || strcmp(cmd, "counttopics") == 0 ||
      strcmp(cmd, "ntopicmatch") == 0) {
    /* Usability: topic filter gates (dual of HASMATCHTOPICS/NEED/COUNT).
     * Schema cubalc.topicmatch.v1 · mode has|need|count */
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    const char *needle = (argc > 2 && argv[2][0]) ? argv[2] : "";
    char nlow[64];
    size_t k;
    int i, n = 0, nall = (int)(sizeof topics / sizeof topics[0]);
    int mode_need = (strcmp(cmd, "needmatchtopics") == 0 ||
                     strcmp(cmd, "needtopicmatch") == 0);
    int mode_count = (strcmp(cmd, "countmatchtopics") == 0 ||
                      strcmp(cmd, "counttopics") == 0 ||
                      strcmp(cmd, "ntopicmatch") == 0);
    if (!needle[0]) {
      fprintf(stderr,
              "usage: cubalc hasmatchtopics|needmatchtopics|countmatchtopics <needle>\n");
      printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need needle\",\"version\":\"%s\"}\n",
             cmd, CUBALC_LANG_VERSION);
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
    for (i = 0; i < nall; i++)
      if (strstr(topics[i], nlow)) n++;
    if (mode_count) {
      printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":true,\"cmd\":\"countmatchtopics\","
             "\"filter\":\"%s\",\"n\":%d,\"mode\":\"count\",\"version\":\"%s\","
             "\"note\":\"COUNTMATCHTOPICS dual · count without bag\"}\n",
             nlow, n, CUBALC_LANG_VERSION);
      return 0;
    }
    printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":%s,\"cmd\":\"%s\","
           "\"filter\":\"%s\",\"n\":%d,\"hit\":%s,\"mode\":\"%s\",\"version\":\"%s\","
           "\"note\":\"HASMATCHTOPICS/NEEDMATCHTOPICS dual · topic filter gate\"}\n",
           n > 0 ? "true" : "false",
           mode_need ? "needmatchtopics" : "hasmatchtopics",
           nlow, n, n > 0 ? "true" : "false",
           mode_need ? "need" : "has", CUBALC_LANG_VERSION);
    if (mode_need) return n > 0 ? 0 : 1;
    return 0;
  }


  return 2;
}
