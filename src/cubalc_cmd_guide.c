/* CubalC CLI formguide/guide — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_guide.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int cubalc_cmd_guide(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "guide";
  if (strcmp(cmd, "formguide") == 0 || strcmp(cmd, "guideform") == 0 ||
      strcmp(cmd, "form-guide") == 0 || strcmp(cmd, "formcard") == 0 ||
      strcmp(cmd, "playform") == 0 || strcmp(cmd, "formboard") == 0) {
    /* Usability: FORMHINT + covering topics + first-topic GUIDE (dual of FORMGUIDE).
     *   cubalc formguide HASFORM · cubalc guideform SETP
     * Schema cubalc.formguide.v1 */
    static const struct { const char *topic; const char *form; } rows[] = {
      {"general", "VERSION"}, {"general", "STATUS"}, {"general", "IDENTITY"},
      {"general", "TIPS"}, {"general", "FORMSFOR"}, {"general", "LISTTOPICS"},
      {"general", "TOPICHINT"}, {"general", "RELATEDTOPIC"}, {"general", "FORMTOPICS"},
      {"general", "FORMGUIDE"}, {"general", "GUIDE"}, {"general", "WHY"},
      {"general", "INCLUDE"}, {"general", "TOPIC"}, {"general", "SNIP"},
      {"general", "ERRTIPS"}, {"general", "ERRGUIDE"}, {"general", "HASTOPIC"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORM"}, {"cap", "HASFORMS"},
      {"cap", "NEEDFORMS"}, {"cap", "FORMHINT"}, {"cap", "LISTFORMS"},
      {"cap", "COUNTFORMS"}, {"cap", "REQUIRE FORM"}, {"cap", "RELATEDTOPIC"},
      {"cap", "FORMTOPICS"}, {"cap", "FORMGUIDE"}, {"cap", "TIPS"},
      {"cap", "FORMSFOR"}, {"cap", "RELATED"}, {"cap", "GUIDE"}, {"cap", "TOPIC"},
      {"fat", "VARROOM"}, {"fat", "HASVARROOM"}, {"fat", "NEEDVARROOM"},
      {"fat", "REMAIN_MS"}, {"fat", "HAS_TIME"}, {"fat", "NEEDTIME"},
      {"fat", "STATUS"}, {"fat", "GUIDE"},
      {"plate", "SETP"}, {"plate", "GETP"}, {"plate", "NEEDP"},
      {"plate", "DEFAULTP"}, {"plate", "PRETTYP"}, {"plate", "SAVEPLATE"},
      {"plate", "LOADPLATE"},
      {"p2p", "SMX"}, {"p2p", "SERVE"}, {"p2p", "DIAL"}, {"p2p", "TALK"},
      {"p2p", "EXCHANGE"},
      {"run", "ASSERT"}, {"run", "EXPECT"}, {"run", "FAIL"}, {"run", "PASS"},
      {"run", "WHY"}, {"run", "CLEAR_ERR"}, {"run", "ERRTIPS"}, {"run", "ERRGUIDE"},
      {"lib", "LISTLIBS"}, {"lib", "HASLIB"}, {"lib", "MATCHLIBS"},
      {"lib", "PICKLIB"}, {"lib", "RECIPE"}, {"lib", "CHECKDEPS"},
      {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"}, {"protect", "STATUS"}, {"protect", "VERSION"},
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
    const char *name = (argc > 2 && argv[2][0]) ? argv[2] : "";
    char nup[96], topicsj[400], relj[256], thj[320], fhj[400], src[360];
    char seen_topics[16][32];
    char first_topic[32];
    const char *topic_hint = NULL;
    cubalc_run_result rr;
    size_t k, o = 0, j;
    int i, n = 0, nr = 0, nall = (int)(sizeof rows / sizeof rows[0]);
    int nseen = 0, first = 1;
    int n_th = (int)(sizeof thints / sizeof thints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    if (!name[0]) {
      fprintf(stderr, "usage: cubalc formguide|guideform <FormName>\n");
      printf("{\"schema\":\"cubalc.formguide.v1\",\"ok\":false,\"cmd\":\"formguide\","
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
    topicsj[0] = 0; first_topic[0] = 0; nseen = 0; n = 0; first = 1; o = 0;
    for (i = 0; i < nall; i++) {
      char fup[96];
      size_t jj;
      int match, already = 0, si;
      for (jj = 0; rows[i].form[jj] && jj + 1 < sizeof fup; jj++) {
        char c = rows[i].form[jj];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        fup[jj] = c;
      }
      fup[jj] = 0;
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
      if (!first && o + 1 < sizeof topicsj) topicsj[o++] = ',';
      o += (size_t)snprintf(topicsj + o, sizeof topicsj - o, "\"%s\"", rows[i].topic);
      first = 0; n++;
    }
    if (n == 0) {
      printf("{\"schema\":\"cubalc.formguide.v1\",\"ok\":false,\"cmd\":\"formguide\","
             "\"form\":\"%s\",\"err\":\"no covering topics\",\"version\":\"%s\","
             "\"note\":\"FORMHINT · FORMTOPICS · cubalc formtopics\"}\n",
             nup, CUBALC_LANG_VERSION);
      return 1;
    }
    for (i = 0; i < n_th; i++)
      if (!strcmp(thints[i].id, first_topic)) { topic_hint = thints[i].hint; break; }
    if (!topic_hint) topic_hint = "";
    relj[0] = 0; o = 0; nr = 0; first = 1;
    for (i = 0; i < n_rels; i++) {
      if (strcmp(rels[i].topic, first_topic) != 0) continue;
      if (!first && o + 1 < sizeof relj) relj[o++] = ',';
      o += (size_t)snprintf(relj + o, sizeof relj - o, "\"%s\"", rels[i].rel);
      first = 0; nr++;
    }
    o = 0;
    for (j = 0; topic_hint[j] && o + 2 < sizeof thj; j++) {
      char c = topic_hint[j];
      if (c == '"' || c == '\\') { thj[o++] = '\\'; thj[o++] = c; }
      else thj[o++] = c;
    }
    thj[o] = 0;
    /* form hint via nested FORMHINT */
    if (strchr(name, ' ') || strchr(name, '"'))
      snprintf(src, sizeof src, "FORMHINT \"%s\"\nPRINT LAST\nPASS\n", name);
    else
      snprintf(src, sizeof src, "FORMHINT %s\nPRINT LAST\nPASS\n", name);
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<formguide-hint>", &rr, NULL);
    {
      const char *h = (rr.ok && rr.last_print[0]) ? rr.last_print : nup;
      o = 0;
      for (j = 0; h[j] && o + 2 < sizeof fhj; j++) {
        char c = h[j];
        if (c == '"' || c == '\\') { fhj[o++] = '\\'; fhj[o++] = c; }
        else if ((unsigned char)c < 32) fhj[o++] = ' ';
        else fhj[o++] = c;
      }
      fhj[o] = 0;
    }
    printf("{\"schema\":\"cubalc.formguide.v1\",\"ok\":true,\"cmd\":\"formguide\","
           "\"form\":\"%s\",\"form_hint\":\"%s\",\"topic\":\"%s\",\"topic_hint\":\"%s\","
           "\"topics_n\":%d,\"related_n\":%d,\"version\":\"%s\","
           "\"note\":\"FORMHINT+FORMTOPICS+first GUIDE · dual of FORMGUIDE\","
           "\"topics\":[%s],\"related\":[%s]}\n",
           nup, fhj, first_topic, thj, n, nr, CUBALC_LANG_VERSION,
           topicsj, relj);
    return 0;
  }
  if (strcmp(cmd, "guide") == 0 || strcmp(cmd, "playguide") == 0 ||
      strcmp(cmd, "topicguide") == 0 || strcmp(cmd, "agentguide") == 0 ||
      strcmp(cmd, "fulltopic") == 0 || strcmp(cmd, "topicboard") == 0 ||
      strcmp(cmd, "guidematch") == 0 || strcmp(cmd, "matchguide") == 0 ||
      strcmp(cmd, "guidenth") == 0 || strcmp(cmd, "nthguide") == 0 ||
      strcmp(cmd, "guidelastmatch") == 0 || strcmp(cmd, "lastmatchguide") == 0) {
    /* Usability: full playbook plate (dual of GUIDE) — hint+related+tips+forms+snip.
     *   cubalc guide · cubalc guide cap · cubalc guide match p · cubalc guide nth 1 p
     *   cubalc guidematch p · cubalc guidenth 1 p · cubalc guidelastmatch p
     * Schema cubalc.guide.v1 · agents skip TOPICHINT+RELATEDTOPIC+TOPIC / NTHTOPIC+GUIDE glue. */
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
      {"cap", "INCLUDE cap_boot — agent_boot + form_guard"},
      {"cap", "cubalc init --cap · HASFORM · NEEDFORMS · FORMHINT"},
      {"cap", "cubalc run -C FORMS · CUBALC_REQUIRE_FORMS floor"},
      {"fat", "INCLUDE fat_session durable nest"},
      {"fat", "cubalc run -T ms · REMAIN_MS · VARROOM"},
      {"plate", "INCLUDE plate_session · SETP NEEDP SAVEPLATE"},
      {"plate", "cubalc plate get|set|uniform · PRETTYP"},
      {"p2p", "export CUBALC_SMX_KEY hex64 · SMX SERVE/DIAL"},
      {"run", "cubalc run -q -e CODE · -I -L -R -C -T -s"},
      {"run", "WHY / why_hint after ASSERT or sticky LAST_ERR"},
      {"lib", "cubalc libs|recipe|checkdeps · INCLUDE MATCH|NEWEST"},
      {"protect", "cubalc protect status · docs/CORE_PROTECT.md"},
    };
    static const struct { const char *topic; const char *form; } forms[] = {
      {"general", "VERSION"}, {"general", "GUIDE"}, {"general", "TOPIC"},
      {"general", "TOPICHINT"}, {"general", "RELATEDTOPIC"}, {"general", "TIPS"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORMS"}, {"cap", "FORMHINT"}, {"cap", "GUIDE"},
      {"fat", "VARROOM"}, {"fat", "REMAIN_MS"}, {"fat", "GUIDE"},
      {"plate", "SETP"}, {"plate", "GETP"}, {"plate", "NEEDP"},
      {"p2p", "SMX"}, {"p2p", "SERVE"}, {"p2p", "DIAL"},
      {"run", "ASSERT"}, {"run", "EXPECT"}, {"run", "WHY"},
      {"lib", "LISTLIBS"}, {"lib", "RECIPE"}, {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"}, {"protect", "STATUS"},
    };
    static const struct { const char *topic; const char *src; } snips[] = {
      {"general", "VERSION\\nSTATUS\\nPASS\\n"},
      {"cap", "HASFORM SORTLIBS\\nFORMHINT SORTLIBS\\nPASS\\n"},
      {"fat", "VARROOM\\nHASVARROOM 8\\nPASS\\n"},
      {"plate", "SETP \\\"status\\\" \\\"ready\\\"\\nNEEDP \\\"status\\\"\\nPASS\\n"},
      {"p2p", "TIPS p2p\\nFORMSFOR p2p\\nPASS\\n"},
      {"run", "EXPECT 1 == 1\\nWHY\\nCLEAR_ERR\\nPASS\\n"},
      {"lib", "HASLIB agent_boot\\nRECIPE agent_boot\\nPASS\\n"},
      {"protect", "HOLD_FLASH\\nVERSION\\nPASS\\n"},
    };
    static const char *topic_ids[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    const char *topic = "general";
    char tup[32], tips_j[2400], forms_j[1200], rel_j[256], hj[320];
    char nlow[64], matches[8][16];
    nlow[0] = 0;
    const char *hint = NULL, *src = "", *needle = NULL, *fallback = NULL, *mpick = NULL;
    size_t k, o, j;
    int i, nt = 0, nf = 0, nr = 0, lines = 0;
    int mode_filt = 0; /* 0 direct · 1 first · 2 nth · 3 lastmatch */
    int m_idx = 0, m_n = 0, ai = 2;
    int n_hints = (int)(sizeof hints / sizeof hints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    int n_tips = (int)(sizeof tips / sizeof tips[0]);
    int n_forms = (int)(sizeof forms / sizeof forms[0]);
    int n_snips = (int)(sizeof snips / sizeof snips[0]);
    int n_topic_ids = (int)(sizeof topic_ids / sizeof topic_ids[0]);
    if (strcmp(cmd, "guidematch") == 0 || strcmp(cmd, "matchguide") == 0)
      mode_filt = 1;
    else if (strcmp(cmd, "guidenth") == 0 || strcmp(cmd, "nthguide") == 0)
      mode_filt = 2;
    else if (strcmp(cmd, "guidelastmatch") == 0 || strcmp(cmd, "lastmatchguide") == 0)
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
          fprintf(stderr, "usage: cubalc guide nth <idx> <needle> [OR fallback]\n");
          printf("{\"schema\":\"cubalc.guide.v1\",\"ok\":false,\"cmd\":\"guidenth\","
                 "\"err\":\"need idx and needle\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 2;
        }
        m_idx = atoi(argv[ai]);
        ai++;
      }
      if (argc <= ai || !argv[ai] || !argv[ai][0]) {
        fprintf(stderr, "usage: cubalc guide match|lastmatch <needle> [OR fallback]\n");
        printf("{\"schema\":\"cubalc.guide.v1\",\"ok\":false,\"cmd\":\"guidematch\","
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
          printf("{\"schema\":\"cubalc.guide.v1\",\"ok\":false,\"cmd\":\"guidematch\","
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
    for (i = 0; i < n_hints; i++)
      if (!strcmp(hints[i].id, tup)) { hint = hints[i].hint; break; }
    if (!hint) {
      printf("{\"schema\":\"cubalc.guide.v1\",\"ok\":false,\"cmd\":\"guide\","
             "\"topic\":\"%s\",\"err\":\"unknown topic\",\"version\":\"%s\","
             "\"note\":\"LISTTOPICS · cubalc topics · HASTOPIC\"}\n",
             tup, CUBALC_LANG_VERSION);
      return 1;
    }
    o = 0; rel_j[0] = 0;
    for (i = 0; i < n_rels; i++) {
      if (strcmp(rels[i].topic, tup) != 0) continue;
      if (o && o + 1 < sizeof rel_j) rel_j[o++] = ',';
      o += (size_t)snprintf(rel_j + o, sizeof rel_j - o, "\"%s\"", rels[i].rel);
      nr++;
    }
    o = 0; tips_j[0] = 0;
    for (i = 0; i < n_tips; i++) {
      char esc[280];
      size_t e = 0;
      if (strcmp(tips[i].topic, tup) != 0) continue;
      for (j = 0; tips[i].tip[j] && e + 2 < sizeof esc; j++) {
        char c = tips[i].tip[j];
        if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
        else if ((unsigned char)c < 32) esc[e++] = ' ';
        else esc[e++] = c;
      }
      esc[e] = 0;
      if (o && o + 1 < sizeof tips_j) tips_j[o++] = ',';
      if (o + e + 3 < sizeof tips_j) {
        tips_j[o++] = '"';
        memcpy(tips_j + o, esc, e); o += e;
        tips_j[o++] = '"'; tips_j[o] = 0;
      }
      nt++;
    }
    o = 0; forms_j[0] = 0;
    for (i = 0; i < n_forms; i++) {
      size_t ln;
      if (strcmp(forms[i].topic, tup) != 0) continue;
      ln = strlen(forms[i].form);
      if (o && o + 1 < sizeof forms_j) forms_j[o++] = ',';
      if (o + ln + 3 < sizeof forms_j) {
        forms_j[o++] = '"';
        memcpy(forms_j + o, forms[i].form, ln); o += ln;
        forms_j[o++] = '"'; forms_j[o] = 0;
      }
      nf++;
    }
    for (i = 0; i < n_snips; i++) {
      if (!strcmp(snips[i].topic, tup)) { src = snips[i].src; break; }
    }
    for (j = 0; src[j]; j++) if (src[j] == 'n' && j > 0 && src[j-1] == '\\') lines++;
    if (lines == 0 && src[0]) lines = 1;
    o = 0;
    for (j = 0; hint[j] && o + 2 < sizeof hj; j++) {
      char c = hint[j];
      if (c == '"' || c == '\\') { hj[o++] = '\\'; hj[o++] = c; }
      else hj[o++] = c;
    }
    hj[o] = 0;
    printf("{\"schema\":\"cubalc.guide.v1\",\"ok\":true,\"cmd\":\"%s\","
           "\"topic\":\"%s\",\"hint\":\"%s\",\"related_n\":%d,\"tips_n\":%d,"
           "\"forms_n\":%d,\"snip_lines\":%d,\"version\":\"%s\","
           "\"filter\":\"%s\",\"match_i\":%d,\"match_n\":%d,\"filter_mode\":%d,"
           "\"note\":\"full playbook · MATCH|NTH|LASTMATCH filter pick · dual of GUIDE\","
           "\"related\":[%s],\"tips\":[%s],\"forms\":[%s],\"snip\":\"%s\"}\n",
           mode_filt == 1 ? "guidematch" : mode_filt == 2 ? "guidenth" :
           mode_filt == 3 ? "guidelastmatch" : "guide",
           tup, hj, nr, nt, nf, lines, CUBALC_LANG_VERSION,
           mode_filt ? nlow : "", m_idx, mode_filt ? m_n : 0, mode_filt,
           rel_j, tips_j, forms_j, src);
    return 0;
  }
  return -1; /* not this plane */
}
