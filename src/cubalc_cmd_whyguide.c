/* CubalC CLI whyguide/errplayguide — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_whyguide.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>

int cubalc_cmd_whyguide(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "whyguide";
    /* Usability: classify err + GUIDE playbook (dual of ERRGUIDE / twin of errrun).
     *   cubalc errguide "INCLUDE missing"
     * Schema cubalc.errguide.v1 · pure C plate (no nested flakiness). */
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
    char msg[400], topic[32], tipsj[2400], formsj[800], relj[256], hj[320], errj[480];
    const char *hint = NULL;
    size_t o = 0, e, j, to = 0;
    int i, n = 0, nr = 0, nf = 0;
    int nall = (int)(sizeof tips / sizeof tips[0]);
    int n_hints = (int)(sizeof hints / sizeof hints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    int n_forms = (int)(sizeof forms / sizeof forms[0]);
    int first;
    msg[0] = 0;
    for (i = 2; i < argc; i++) {
      size_t ln;
      if (!argv[i]) continue;
      ln = strlen(argv[i]);
      if (o && o + 1 < sizeof msg) msg[o++] = ' ';
      if (o + ln < sizeof msg) { memcpy(msg + o, argv[i], ln); o += ln; }
      msg[o] = 0;
    }
    if (!msg[0]) snprintf(msg, sizeof msg, "%s", "ok");
    snprintf(topic, sizeof topic, "%s", "general");
    if (!strcmp(msg, "ok"))
      snprintf(topic, sizeof topic, "%s", "general");
    else if (strstr(msg, "NEEDFORM") || strstr(msg, "HASFORM") ||
             strstr(msg, "FORMMISS") || strstr(msg, "unknown form") ||
             strstr(msg, "did you mean") || strstr(msg, "REQUIRE FORM") ||
             strstr(msg, "LISTFORMS") || strstr(msg, "FORMHINT") ||
             strstr(msg, "NEEDFORMS") || strstr(msg, "HASFORMS"))
      snprintf(topic, sizeof topic, "%s", "cap");
    else if (strstr(msg, "NEEDMATCHLIBS") || strstr(msg, "HASMATCHLIBS") ||
             strstr(msg, "PICKLIB") || strstr(msg, "SORTLIBS") ||
             strstr(msg, "INCLUDE") || strstr(msg, "include") ||
             strstr(msg, "REQUIRE LIB") || strstr(msg, "lib missing") ||
             strstr(msg, "NEEDDEPS") || strstr(msg, "CHECKDEPS") ||
             strstr(msg, "HASLIB") || strstr(msg, "LISTLIBS"))
      snprintf(topic, sizeof topic, "%s", "lib");
    else if (strstr(msg, "VARROOM") || strstr(msg, "NEEDVARROOM") ||
             strstr(msg, "HASVARROOM") || strstr(msg, "REMAIN_MS") ||
             strstr(msg, "NEEDTIME") || strstr(msg, "HAS_TIME"))
      snprintf(topic, sizeof topic, "%s", "fat");
    else if (strstr(msg, "NEEDP") || strstr(msg, "NEEDFLAT") ||
             strstr(msg, "UNIFORM") || strstr(msg, "plate") ||
             strstr(msg, "PLATE") || strstr(msg, "SETP") || strstr(msg, "GETP"))
      snprintf(topic, sizeof topic, "%s", "plate");
    else if (strstr(msg, "DIAL") || strstr(msg, "SERVE") || strstr(msg, "SMX") ||
             strstr(msg, "TALK") || strstr(msg, "P2P") || strstr(msg, "timeout"))
      snprintf(topic, sizeof topic, "%s", "p2p");
    else if (strstr(msg, "HOLD_FLASH") || strstr(msg, "protect") ||
             strstr(msg, "CORE_PROTECT"))
      snprintf(topic, sizeof topic, "%s", "protect");
    else if (strstr(msg, "ASSERT") || strstr(msg, "EXPECT") ||
             strstr(msg, "is false") || strstr(msg, "FAIL") ||
             strstr(msg, "VERSION") || strstr(msg, "too old"))
      snprintf(topic, sizeof topic, "%s", "run");
    for (i = 0; i < n_hints; i++)
      if (!strcmp(hints[i].id, topic)) { hint = hints[i].hint; break; }
    if (!hint) hint = "cubalc topics · GUIDE general";
    tipsj[0] = 0; to = 0; n = 0; first = 1;
    for (i = 0; i < nall; i++) {
      char esc[280];
      size_t ee = 0;
      if (strcmp(tips[i].topic, topic) != 0) continue;
      for (j = 0; tips[i].tip[j] && ee + 2 < sizeof esc; j++) {
        char c = tips[i].tip[j];
        if (c == '"' || c == '\\') { esc[ee++] = '\\'; esc[ee++] = c; }
        else if ((unsigned char)c < 32) esc[ee++] = ' ';
        else esc[ee++] = c;
      }
      esc[ee] = 0;
      if (!first && to + 1 < sizeof tipsj) tipsj[to++] = ',';
      if (to + ee + 3 < sizeof tipsj) {
        tipsj[to++] = '"';
        memcpy(tipsj + to, esc, ee); to += ee;
        tipsj[to++] = '"'; tipsj[to] = 0;
      }
      first = 0; n++;
    }
    if (!strcmp(msg, "ok") && n == 0) {
      snprintf(tipsj, sizeof tipsj, "\"ok — no sticky LAST_ERR · GUIDE general playbook\"");
      n = 1;
    }
    relj[0] = 0; o = 0; nr = 0; first = 1;
    for (i = 0; i < n_rels; i++) {
      if (strcmp(rels[i].topic, topic) != 0) continue;
      if (!first && o + 1 < sizeof relj) relj[o++] = ',';
      o += (size_t)snprintf(relj + o, sizeof relj - o, "\"%s\"", rels[i].rel);
      first = 0; nr++;
    }
    formsj[0] = 0; o = 0; nf = 0; first = 1;
    for (i = 0; i < n_forms; i++) {
      size_t ln;
      if (strcmp(forms[i].topic, topic) != 0) continue;
      ln = strlen(forms[i].form);
      if (!first && o + 1 < sizeof formsj) formsj[o++] = ',';
      if (o + ln + 3 < sizeof formsj) {
        formsj[o++] = '"';
        memcpy(formsj + o, forms[i].form, ln); o += ln;
        formsj[o++] = '"'; formsj[o] = 0;
      }
      first = 0; nf++;
    }
    o = 0;
    for (j = 0; hint[j] && o + 2 < sizeof hj; j++) {
      char c = hint[j];
      if (c == '"' || c == '\\') { hj[o++] = '\\'; hj[o++] = c; }
      else hj[o++] = c;
    }
    hj[o] = 0;
    e = 0;
    for (j = 0; msg[j] && e + 2 < sizeof errj; j++) {
      char c = msg[j];
      if (c == '"' || c == '\\') { errj[e++] = '\\'; errj[e++] = c; }
      else if ((unsigned char)c < 32) errj[e++] = ' ';
      else errj[e++] = c;
    }
    errj[e] = 0;
    printf("{\"schema\":\"cubalc.errguide.v1\",\"ok\":true,\"cmd\":\"errguide\","
           "\"topic\":\"%s\",\"hint\":\"%s\",\"related_n\":%d,\"tips_n\":%d,"
           "\"forms_n\":%d,\"version\":\"%s\","
           "\"note\":\"ERRTIPS classify + GUIDE playbook · dual of ERRGUIDE · twin of errrun\","
           "\"related\":[%s],\"tips\":[%s],\"forms\":[%s],\"err\":\"%s\"}\n",
           topic, hj, nr, n, nf, CUBALC_LANG_VERSION,
           relj, tipsj, formsj, errj);
    return 0;
}
