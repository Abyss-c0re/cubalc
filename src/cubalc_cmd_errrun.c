/* CubalC CLI errrun — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_errrun.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include "cubalc_run_util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int cubalc_cmd_errrun(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "errrun";
  if (strcmp(cmd, "errrun") == 0 || strcmp(cmd, "recoversnip") == 0 ||
      strcmp(cmd, "fixrun") == 0 || strcmp(cmd, "err-run") == 0 ||
      strcmp(cmd, "errsnip") == 0 || strcmp(cmd, "fixsnip") == 0) {
    /* Usability: classify err + RUNSNIP topic (dual of ERRRUN).
     *   cubalc errrun "INCLUDE missing"
     * Schema cubalc.errrun.v1 · nested RUNSNIP via in-lang ERRRUN. */
    char msg[400], esc[480], src[720];
    cubalc_run_result rr;
    size_t o = 0, j, e;
    int i, ok;
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
    e = 0;
    for (j = 0; msg[j] && e + 2 < sizeof esc; j++) {
      char c = msg[j];
      if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
      else if ((unsigned char)c < 32) esc[e++] = ' ';
      else esc[e++] = c;
    }
    esc[e] = 0;
    snprintf(src, sizeof src,
             "ERRRUN \"%s\"\n"
             "PRINT ERRRUN_TOPIC\n"
             "IF OK == 0 THEN\n"
             "  FAIL \"errrun nested\"\n"
             "END\n"
             "PASS\n",
             esc);
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<cli-errrun>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    {
      char topicj[48], lastj[400];
      size_t k, lo = 0;
      const char *tp = rr.last_print[0] ? rr.last_print : "general";
      /* topic from first print — may be multi-line if PRINT wrong; take first line */
      k = 0;
      while (tp[k] && tp[k] != '\n' && k + 1 < sizeof topicj) {
        topicj[k] = tp[k];
        k++;
      }
      topicj[k] = 0;
      /* re-fetch last via second field: use ERRRUN_LAST by another run */
      {
        char src2[640];
        cubalc_run_result rr2;
        snprintf(src2, sizeof src2,
                 "ERRRUN \"%s\"\n"
                 "PRINT ERRRUN_TOPIC\n"
                 "PASS\n",
                 esc);
        memset(&rr2, 0, sizeof rr2);
        (void)cubalc_run_source(src2, strlen(src2), "<cli-errrun-topic>", &rr2, NULL);
        tp = rr2.last_print[0] ? rr2.last_print : topicj;
        k = 0;
        while (tp[k] && tp[k] != '\n' && k + 1 < sizeof topicj) {
          topicj[k] = tp[k];
          k++;
        }
        topicj[k] = 0;
        snprintf(src2, sizeof src2,
                 "ERRRUN \"%s\"\n"
                 "PRINT LAST\n"
                 "PASS\n",
                 esc);
        memset(&rr2, 0, sizeof rr2);
        (void)cubalc_run_source(src2, strlen(src2), "<cli-errrun-last>", &rr2, NULL);
        tp = rr2.last_print;
        lo = 0;
        for (k = 0; tp[k] && lo + 2 < sizeof lastj; k++) {
          char c = tp[k];
          if (c == '"' || c == '\\') { lastj[lo++] = '\\'; lastj[lo++] = c; }
          else if (c == '\n') { lastj[lo++] = '\\'; lastj[lo++] = 'n'; }
          else if ((unsigned char)c < 32) lastj[lo++] = ' ';
          else lastj[lo++] = c;
        }
        lastj[lo] = 0;
      }
      printf("{\"schema\":\"cubalc.errrun.v1\",\"ok\":%s,\"cmd\":\"errrun\","
             "\"topic\":\"%s\",\"asserts_ok\":%d,\"asserts_fail\":%d,\"stmts\":%d,"
             "\"last\":\"%s\",\"version\":\"%s\","
             "\"note\":\"ERRTIPS classify + RUNSNIP topic · dual of in-lang ERRRUN\","
             "\"err\":\"%s\"}\n",
             ok ? "true" : "false", topicj, rr.asserts_ok, rr.asserts_fail, rr.stmts,
             lastj, CUBALC_LANG_VERSION, esc);
    }
    return ok ? 0 : 1;
  }
  return -1;
}
