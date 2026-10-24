/* CubalC CLI runsnip — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_runsnip.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int cubalc_cmd_runsnip(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "runsnip";
  if (strcmp(cmd, "runsnip") == 0 || strcmp(cmd, "sniprun") == 0 ||
      strcmp(cmd, "snip-run") == 0 || strcmp(cmd, "run-snip") == 0 ||
      strcmp(cmd, "exec-snip") == 0 || strcmp(cmd, "play-snip") == 0 ||
      strcmp(cmd, "runsnipmatch") == 0 || strcmp(cmd, "matchrunsnip") == 0 ||
      strcmp(cmd, "runsnipnth") == 0 || strcmp(cmd, "nthrunsnip") == 0 ||
      strcmp(cmd, "runsniplastmatch") == 0 || strcmp(cmd, "lastmatchrunsnip") == 0) {
    /* Usability: execute curated SNIP mini without temp files (dual of RUNSNIP).
     *   cubalc runsnip · cubalc runsnip match p · cubalc runsnip nth 1 p
     *   cubalc runsnipmatch p · cubalc runsnipnth 1 p · cubalc runsniplastmatch p
     * Schema cubalc.runsnip.v1 · nested run plate fields + topic. */
    static const char *topic_ids[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    char src[640];
    cubalc_run_result rr;
    const char *topic = "general";
    const char *needle = NULL, *fallback = NULL, *mpick = NULL;
    char tup[32], nlow[64], matches[8][16];
    size_t k;
    int ok, i, m_idx = 0, m_n = 0, mode_filt = 0, ai = 2;
    int n_topic_ids = (int)(sizeof topic_ids / sizeof topic_ids[0]);
    nlow[0] = 0;
    if (strcmp(cmd, "runsnipmatch") == 0 || strcmp(cmd, "matchrunsnip") == 0)
      mode_filt = 1;
    else if (strcmp(cmd, "runsnipnth") == 0 || strcmp(cmd, "nthrunsnip") == 0)
      mode_filt = 2;
    else if (strcmp(cmd, "runsniplastmatch") == 0 || strcmp(cmd, "lastmatchrunsnip") == 0)
      mode_filt = 3;
    if (mode_filt == 0 && argc > 2 && argv[2] && argv[2][0]) {
      if (!strcmp(argv[2], "match") || !strcmp(argv[2], "first") ||
          !strcmp(argv[2], "pick")) {
        mode_filt = 1;
        ai = 3;
      } else if (!strcmp(argv[2], "nth") || !strcmp(argv[2], "index")) {
        mode_filt = 2;
        ai = 3;
      } else if (!strcmp(argv[2], "lastmatch") || !strcmp(argv[2], "endmatch") ||
                 !strcmp(argv[2], "matchlast")) {
        mode_filt = 3;
        ai = 3;
      } else {
        topic = argv[2];
      }
    }
    if (mode_filt != 0) {
      if (mode_filt == 2) {
        if (argc <= ai || !argv[ai] || !argv[ai][0]) {
          fprintf(stderr, "usage: cubalc runsnip nth <idx> <needle> [OR fallback]\n");
          printf("{\"schema\":\"cubalc.runsnip.v1\",\"ok\":false,\"cmd\":\"runsnipnth\","
                 "\"err\":\"need idx and needle\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 2;
        }
        m_idx = atoi(argv[ai]);
        ai++;
      }
      if (argc <= ai || !argv[ai] || !argv[ai][0]) {
        fprintf(stderr, "usage: cubalc runsnip match|lastmatch <needle> [OR fallback]\n");
        printf("{\"schema\":\"cubalc.runsnip.v1\",\"ok\":false,\"cmd\":\"runsnipmatch\","
               "\"err\":\"need needle\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai];
      ai++;
      if (argc > ai + 1 && argv[ai] &&
          (!strcmp(argv[ai], "OR") || !strcmp(argv[ai], "or") ||
           !strcmp(argv[ai], "ELSE") || !strcmp(argv[ai], "DEFAULT")) &&
          argv[ai + 1] && argv[ai + 1][0])
        fallback = argv[ai + 1];
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
      if (!mpick) {
        if (fallback && fallback[0]) {
          topic = fallback;
        } else {
          printf("{\"schema\":\"cubalc.runsnip.v1\",\"ok\":false,\"cmd\":\"runsnipmatch\","
                 "\"filter\":\"%s\",\"idx\":%d,\"match_n\":%d,\"err\":\"no match\","
                 "\"version\":\"%s\"}\n",
                 nlow, m_idx, m_n, CUBALC_LANG_VERSION);
          return 1;
        }
      } else {
        topic = mpick;
      }
    }
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
    /* Prefer in-lang RUNSNIP so catalog stays single-source with language. */
    if (strchr(tup, '"') || strchr(tup, ' ') || strchr(tup, '\n')) {
      printf("{\"schema\":\"cubalc.runsnip.v1\",\"ok\":false,\"cmd\":\"runsnip\","
             "\"err\":\"bad topic\",\"version\":\"%s\"}\n", CUBALC_LANG_VERSION);
      return 2;
    }
    snprintf(src, sizeof src,
             "RUNSNIP %s\n"
             "PRINT LAST\n"
             "IF OK == 0 THEN\n"
             "  FAIL \"runsnip nested\"\n"
             "END\n"
             "PASS\n",
             tup);
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<cli-runsnip>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    {
      char lastj[400];
      size_t i2, o = 0;
      const char *h = rr.last_print[0] ? rr.last_print : (rr.last_err[0] ? rr.last_err : "");
      const char *cmd_out = mode_filt == 1 ? "runsnipmatch" :
                            mode_filt == 2 ? "runsnipnth" :
                            mode_filt == 3 ? "runsniplastmatch" : "runsnip";
      for (i2 = 0; h[i2] && o + 2 < sizeof lastj; i2++) {
        char c = h[i2];
        if (c == '"' || c == '\\') { lastj[o++] = '\\'; lastj[o++] = c; }
        else if (c == '\n') { lastj[o++] = '\\'; lastj[o++] = 'n'; }
        else if ((unsigned char)c < 32) lastj[o++] = ' ';
        else lastj[o++] = c;
      }
      lastj[o] = 0;
      printf("{\"schema\":\"cubalc.runsnip.v1\",\"ok\":%s,\"cmd\":\"%s\","
             "\"topic\":\"%s\",\"asserts_ok\":%d,\"asserts_fail\":%d,\"stmts\":%d,"
             "\"filter\":\"%s\",\"match_i\":%d,\"match_n\":%d,\"filter_mode\":%d,"
             "\"last\":\"%s\",\"version\":\"%s\","
             "\"note\":\"execute curated SNIP · MATCH|NTH|LASTMATCH filter pick · dual of RUNSNIP\","
             "\"topics\":[\"general\",\"cap\",\"fat\",\"plate\",\"p2p\",\"run\",\"lib\",\"protect\"]}\n",
             ok ? "true" : "false", cmd_out, tup, rr.asserts_ok, rr.asserts_fail, rr.stmts,
             mode_filt ? nlow : "", m_idx, mode_filt ? m_n : 0, mode_filt,
             lastj, CUBALC_LANG_VERSION);
    }
    return ok ? 0 : 1;
  }
  return 0;
}
