/* CubalC CLI errtips/formhints/whytips — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_formhints.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int cubalc_cmd_formhints(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "errtips";
    /* Usability: classify error text → recovery tip bag + topic (dual of ERRTIPS).
     *   cubalc errtips "INCLUDE missing foo"
     * Schema cubalc.errtips.v1 · pure C (no nested PRINT flakiness). */
    static const struct { const char *topic; const char *tip; } tips[] = {
      {"general", "cubalc doctor — install readiness plate"},
      {"general", "STATUS · VARS · CLEAR_ERR after fix"},
      {"general", "cubalc topics · TOPIC name for tips+forms+snip triad"},
      {"general", "cubalc search KEYWORD — forms/libs/examples"},
      {"cap", "HASFORM name · NEEDFORMS a b · FORMHINT name"},
      {"cap", "LISTFORMS prefix · cubalc forms · cubalc run -C FORMS"},
      {"cap", "INCLUDE cap_boot · cubalc init --cap"},
      {"cap", "RELATED HASFORM · FORMSFOR cap · RUNSNIP cap"},
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
      {"run", "ERRTIPS after FAIL · TIPS run playbook"},
      {"lib", "cubalc libs [filter] · REQUIRE LIB · INCLUDE SOFT"},
      {"lib", "MATCHLIBS · CHECKDEPS · cubalc recipe name"},
      {"lib", "cubalc which name · HASLIB · LISTLIBS"},
      {"protect", "cubalc protect status · HOLD_FLASH device/mesh-join only"},
      {"protect", "docs/CORE_PROTECT.md · cubalc protect all"},
    };
    char msg[400], topic[32], tipsj[2400], errj[480];
    size_t o = 0, e, j, to = 0;
    int i, n = 0, nall = (int)(sizeof tips / sizeof tips[0]);
    int first = 1;
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
    tipsj[0] = 0; to = 0; n = 0; first = 1;
    if (!strcmp(msg, "ok")) {
      const char *oktip = "ok — no sticky LAST_ERR · STATUS · CLEAR_ERR idle";
      size_t ln = strlen(oktip);
      tipsj[to++] = '"';
      memcpy(tipsj + to, oktip, ln); to += ln;
      tipsj[to++] = '"'; tipsj[to] = 0;
      n = 1;
    } else {
      for (i = 0; i < nall; i++) {
        size_t ln, v = 0;
        char one[320];
        if (strcmp(tips[i].topic, topic) != 0) continue;
        ln = strlen(tips[i].tip);
        for (j = 0; j < ln && v + 2 < sizeof one; j++) {
          char c = tips[i].tip[j];
          if (c == '"' || c == '\\') { one[v++] = '\\'; one[v++] = c; }
          else one[v++] = c;
        }
        one[v] = 0;
        if (to + v + 4 < sizeof tipsj) {
          if (!first) tipsj[to++] = ',';
          tipsj[to++] = '"';
          memcpy(tipsj + to, one, v); to += v;
          tipsj[to++] = '"'; tipsj[to] = 0;
          first = 0; n++;
        }
      }
    }
    e = 0;
    for (j = 0; msg[j] && e + 2 < sizeof errj; j++) {
      char c = msg[j];
      if (c == '"' || c == '\\') { errj[e++] = '\\'; errj[e++] = c; }
      else if ((unsigned char)c < 32) errj[e++] = ' ';
      else errj[e++] = c;
    }
    errj[e] = 0;
    printf("{\"schema\":\"cubalc.errtips.v1\",\"ok\":true,\"cmd\":\"errtips\","
           "\"topic\":\"%s\",\"n\":%d,\"version\":\"%s\","
           "\"note\":\"recovery tip bag from err text · dual of ERRTIPS · chain cubalc topic\","
           "\"err\":\"%s\",\"tips\":[%s]}\n",
           topic, n, CUBALC_LANG_VERSION, errj, tipsj);
    return n > 0 ? 0 : 1;
}
