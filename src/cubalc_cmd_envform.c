/* CubalC CLI hasenvall|needenvs|hasenvany · hasform|needform|hasforms — F_main_carve peel.
 * Peel from cubalc_main.c. Cube is SoT. Zero bloat.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_envform.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include "cubalc_run_util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cubalc_cmd_envform(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "hasenvall";
  if (strcmp(cmd, "hasenvall") == 0 || strcmp(cmd, "hasallenv") == 0 ||
      strcmp(cmd, "needenvs") == 0 || strcmp(cmd, "requireenvs") == 0 ||
      strcmp(cmd, "mustenvs") == 0 || strcmp(cmd, "needenvall") == 0 ||
      strcmp(cmd, "requireenvall") == 0 ||
      strcmp(cmd, "hasenvany") == 0 || strcmp(cmd, "hasanyenv") == 0 ||
      strcmp(cmd, "needenvany") == 0 || strcmp(cmd, "requireenvany") == 0 ||
      strcmp(cmd, "mustenvany") == 0 || strcmp(cmd, "needanyenv") == 0) {
    /* Usability: CLI dual of HASENVALL/NEEDENVS · HASENVANY/NEEDENVANY.
     *   cubalc hasenvall HOME USER PATH
     *   cubalc needenvs CUBALC_STATE HOME
     *   cubalc hasenvany DB_URL REDIS_URL
     *   cubalc needenvany HOST PORT
     * Schema cubalc.envgate.v1 */
    int is_any = (strcmp(cmd, "hasenvany") == 0 || strcmp(cmd, "hasanyenv") == 0 ||
                  strcmp(cmd, "needenvany") == 0 || strcmp(cmd, "requireenvany") == 0 ||
                  strcmp(cmd, "mustenvany") == 0 || strcmp(cmd, "needanyenv") == 0);
    int hard = (strcmp(cmd, "needenvs") == 0 || strcmp(cmd, "requireenvs") == 0 ||
                strcmp(cmd, "mustenvs") == 0 || strcmp(cmd, "needenvall") == 0 ||
                strcmp(cmd, "requireenvall") == 0 ||
                strcmp(cmd, "needenvany") == 0 || strcmp(cmd, "requireenvany") == 0 ||
                strcmp(cmd, "mustenvany") == 0 || strcmp(cmd, "needanyenv") == 0);
    char src[1200];
    char names[32][96];
    int nname = 0, i, ai, hit = 0;
    size_t o = 0;
    cubalc_run_result rr;
    const char *form;
    if (is_any)
      form = hard ? "NEEDENVANY" : "HASENVANY";
    else
      form = hard ? "NEEDENVS" : "HASENVALL";
    for (i = 2; i < argc && nname < 32; i++) {
      if (!argv[i] || !argv[i][0]) continue;
      if (!strcmp(argv[i], "--")) break;
      snprintf(names[nname++], sizeof names[0], "%s", argv[i]);
    }
    if (nname == 0) {
      fprintf(stderr,
              "usage: cubalc hasenvall|needenvs <ENV> [ENV…]\n"
              "       cubalc hasenvany|needenvany <ENV> [ENV…]\n");
      printf("{\"schema\":\"cubalc.envgate.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need names\",\"version\":\"%s\"}\n",
             cmd, CUBALC_LANG_VERSION);
      return 2;
    }
    o = (size_t)snprintf(src, sizeof src, "%s", form);
    for (ai = 0; ai < nname && o + 2 < sizeof src; ai++) {
      const char *nm = names[ai];
      int need_q = 0, c;
      for (c = 0; nm[c]; c++) {
        if (nm[c] == ' ' || nm[c] == '"' || nm[c] == '\\') {
          need_q = 1;
          break;
        }
      }
      if (need_q)
        o += (size_t)snprintf(src + o, sizeof src - o, " \"%s\"", nm);
      else
        o += (size_t)snprintf(src + o, sizeof src - o, " %s", nm);
    }
    o += (size_t)snprintf(src + o, sizeof src - o, "\nPRINT LAST_N\nPASS\n");
    (void)o;
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<cli-envgate>", &rr, NULL);
    if (hard)
      hit = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    else
      hit = (rr.ok && !rr.err[0] && rr.last_print[0] == '1') ? 1 : 0;
    printf("{\"schema\":\"cubalc.envgate.v1\",\"ok\":%s,\"cmd\":\"%s\","
           "\"mode\":\"%s\",\"any\":%s,\"n\":%d,\"version\":\"%s\","
           "\"note\":\"CLI dual of %s · multi host env %s gate\","
           "\"names\":[",
           hit ? "true" : "false", cmd,
           hard ? "need" : "has", is_any ? "true" : "false", nname,
           CUBALC_LANG_VERSION, form, is_any ? "ANY" : "ALL");
    for (ai = 0; ai < nname; ai++) {
      char esc[120];
      size_t e = 0, k;
      const char *nm = names[ai];
      for (k = 0; nm[k] && e + 2 < sizeof esc; k++) {
        char c = nm[k];
        if (c == '"' || c == '\\') {
          esc[e++] = '\\';
          esc[e++] = c;
        } else if ((unsigned char)c < 32)
          esc[e++] = ' ';
        else
          esc[e++] = c;
      }
      esc[e] = 0;
      printf("%s\"%s\"", ai ? "," : "", esc);
    }
    printf("]");
    if (!hit && rr.err[0]) {
      char eesc[200];
      size_t e = 0, k;
      for (k = 0; rr.err[k] && e + 2 < sizeof eesc; k++) {
        char c = rr.err[k];
        if (c == '"' || c == '\\') {
          eesc[e++] = '\\';
          eesc[e++] = c;
        } else if ((unsigned char)c < 32)
          eesc[e++] = ' ';
        else
          eesc[e++] = c;
      }
      eesc[e] = 0;
      printf(",\"err\":\"%s\"", eesc);
    }
    printf("}\n");
    if (hard)
      return hit ? 0 : 1;
    return 0;
  }
  if (strcmp(cmd, "hasform") == 0 || strcmp(cmd, "formexists") == 0 ||
      strcmp(cmd, "needform") == 0 || strcmp(cmd, "requireform") == 0 ||
      strcmp(cmd, "knownform") == 0 ||
      strcmp(cmd, "hasforms") == 0 || strcmp(cmd, "needforms") == 0 ||
      strcmp(cmd, "requireforms") == 0 || strcmp(cmd, "allforms") == 0 ||
      strcmp(cmd, "hasformany") == 0 || strcmp(cmd, "hasanyforms") == 0 ||
      strcmp(cmd, "needformany") == 0 || strcmp(cmd, "requireformany") == 0 ||
      strcmp(cmd, "mustformany") == 0 || strcmp(cmd, "needanyforms") == 0) {
    /* Usability: CLI dual of HASFORM/NEEDFORM/HASFORMS/NEEDFORMS/HASFORMANY/NEEDFORMANY.
     * cubalc hasform SORTLIBS · hasforms SORTLIBS LIBAGE · hasformany WALK PATHGLOB */
    int is_any = (strcmp(cmd, "hasformany") == 0 || strcmp(cmd, "hasanyforms") == 0 ||
                  strcmp(cmd, "needformany") == 0 || strcmp(cmd, "requireformany") == 0 ||
                  strcmp(cmd, "mustformany") == 0 || strcmp(cmd, "needanyforms") == 0);
    int multi = is_any ||
                (strcmp(cmd, "hasforms") == 0 || strcmp(cmd, "needforms") == 0 ||
                 strcmp(cmd, "requireforms") == 0 || strcmp(cmd, "allforms") == 0);
    int hard = (strcmp(cmd, "needform") == 0 || strcmp(cmd, "requireform") == 0 ||
                strcmp(cmd, "needforms") == 0 || strcmp(cmd, "requireforms") == 0 ||
                strcmp(cmd, "needformany") == 0 || strcmp(cmd, "requireformany") == 0 ||
                strcmp(cmd, "mustformany") == 0 || strcmp(cmd, "needanyforms") == 0);
    char src[800];
    cubalc_run_result rr;
    int hit = 0, i, narg = 0;
    size_t o = 0;
    const char *form;
    if (argc <= 2 || !argv[2] || !argv[2][0]) {
      fprintf(stderr,
              "usage: cubalc hasform|formexists <FormName>\n"
              "       cubalc needform|requireform <FormName>\n"
              "       cubalc hasforms|needforms <Form> [Form…]\n"
              "       cubalc hasformany|needformany <Form> [Form…]\n");
      printf("{\"schema\":\"cubalc.formgate.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need form name\",\"version\":\"%s\"}\n",
             cmd, CUBALC_LANG_VERSION);
      return 2;
    }
    if (is_any)
      form = hard ? "NEEDFORMANY" : "HASFORMANY";
    else if (multi || argc > 3)
      form = hard ? "NEEDFORMS" : "HASFORMS";
    else
      form = hard ? "NEEDFORM" : "HASFORM";
    if (multi || is_any || argc > 3) {
      o = (size_t)snprintf(src, sizeof src, "%s", form);
      for (i = 2; i < argc && o + 2 < sizeof src; i++) {
        if (!argv[i] || !argv[i][0]) continue;
        narg++;
        o += (size_t)snprintf(src + o, sizeof src - o, " %s", argv[i]);
      }
      /* soft any/all: PRINT LAST_N; hard: ASSERT succeeds only if pass */
      if (hard)
        o += (size_t)snprintf(src + o, sizeof src - o,
                              "\nASSERT LAST_N == 1\nPASS\n");
      else
        o += (size_t)snprintf(src + o, sizeof src - o,
                              "\nPRINT LAST_N\nPASS\n");
    } else {
      const char *name = argv[2];
      narg = 1;
      if (hard) {
        if (strchr(name, ' ') || strchr(name, '"'))
          snprintf(src, sizeof src,
                   "%s \"%s\"\nASSERT LAST_N == 1\nPASS\n", form, name);
        else
          snprintf(src, sizeof src,
                   "%s %s\nASSERT LAST_N == 1\nPASS\n", form, name);
      } else {
        if (strchr(name, ' ') || strchr(name, '"'))
          snprintf(src, sizeof src,
                   "%s \"%s\"\nPRINT LAST_N\nPASS\n", form, name);
        else
          snprintf(src, sizeof src,
                   "%s %s\nPRINT LAST_N\nPASS\n", form, name);
      }
    }
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<hasform>", &rr, NULL);
    if (hard)
      hit = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    else
      hit = (rr.ok && !rr.err[0] && rr.last_print[0] == '1') ? 1 : 0;
    printf("{\"schema\":\"cubalc.formgate.v1\",\"ok\":%s,\"cmd\":\"%s\","
           "\"n\":%d,\"known\":%s,\"mode\":\"%s\",\"any\":%s,"
           "\"version\":\"%s\","
           "\"note\":\"CLI dual of %s · HELP catalog capability gate\","
           "\"forms\":[",
           hit ? "true" : "false", cmd, narg,
           hit ? "true" : "false",
           hard ? "need" : "has",
           is_any ? "true" : "false",
           CUBALC_LANG_VERSION, form);
    for (i = 2; i < argc; i++) {
      if (!argv[i] || !argv[i][0]) continue;
      printf("%s\"%s\"", (i == 2) ? "" : ",", argv[i]);
    }
    printf("]}\n");
    if (hard)
      return hit ? 0 : 1;
    return 0;
  }
  return -1; /* not this plane */
}
