/* CubalC CLI snip — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_snip.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int cubalc_cmd_snip(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "snip";
  if (strcmp(cmd, "snip") == 0 || strcmp(cmd, "snippet") == 0 ||
      strcmp(cmd, "mini") == 0 || strcmp(cmd, "miniprog") == 0 ||
      strcmp(cmd, "codesnip") == 0 || strcmp(cmd, "starter-src") == 0) {
    /* Usability: curated mini runnable source by topic (dual of SNIP).
     *   cubalc snip · cubalc snip cap · cubalc snippet plate
     * Schema cubalc.snip.v1 · triad with tips + formsfor (prose / names / code). */
    static const struct { const char *topic; const char *src; } snips[] = {
      {"general",
       "VERSION\nSTATUS\nPRINT \"version\" VERSION\nPASS\n"},
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
      {"run",
       "EXPECT 1 == 1\nWHY\nPRINT WHY_HINT\nCLEAR_ERR\nPASS \"run probe ok\"\n"},
      {"lib",
       "LISTLIBS\nPRINT \"libs\" LAST_N\nHASLIB agent_boot\nASSERT LAST_N == 1\n"
       "RECIPE agent_boot\nPRINT LAST\nPASS\n"},
      {"protect",
       "HOLD_FLASH\nPRINT \"hold_flash\" LAST_N\nVERSION\nSTATUS\n"
       "TIPS protect\nPRINT LAST\nPASS\n"},
    };
    const char *topic = (argc > 2 && argv[2][0]) ? argv[2] : "general";
    char tup[32];
    char esc[2048];
    size_t k, o = 0;
    int i, nall = (int)(sizeof snips / sizeof snips[0]);
    const char *src = NULL;
    int lines = 0;
    const char *p;
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
    for (i = 0; i < nall; i++) {
      if (!strcmp(snips[i].topic, tup)) {
        src = snips[i].src;
        break;
      }
    }
    if (!src) {
      printf("{\"schema\":\"cubalc.snip.v1\",\"ok\":false,\"cmd\":\"snip\","
             "\"topic\":\"%s\",\"err\":\"unknown topic\","
             "\"topics\":[\"general\",\"cap\",\"fat\",\"plate\",\"p2p\",\"run\",\"lib\",\"protect\"],"
             "\"version\":\"%s\"}\n",
             tup, CUBALC_LANG_VERSION);
      return 1;
    }
    for (p = src; *p; p++) {
      if (*p == '\n') lines++;
      if (o + 2 >= sizeof esc) break;
      if (*p == '"' || *p == '\\') {
        esc[o++] = '\\';
        esc[o++] = *p;
      } else if (*p == '\n') {
        esc[o++] = '\\';
        esc[o++] = 'n';
      } else if (*p == '\r' || *p == '\t') {
        esc[o++] = ' ';
      } else if ((unsigned char)*p < 32) {
        continue;
      } else {
        esc[o++] = *p;
      }
    }
    esc[o] = 0;
    if (lines == 0 && src[0]) lines = 1;
    printf("{\"schema\":\"cubalc.snip.v1\",\"ok\":true,\"cmd\":\"snip\","
           "\"topic\":\"%s\",\"lines\":%d,\"version\":\"%s\","
           "\"note\":\"mini runnable source · dual of in-lang SNIP · triad with tips/formsfor\","
           "\"topics\":[\"general\",\"cap\",\"fat\",\"plate\",\"p2p\",\"run\",\"lib\",\"protect\"],"
           "\"source\":\"%s\"}\n",
           tup, lines, CUBALC_LANG_VERSION, esc);
    return 0;
  }
  return 2;
}
