/* CubalC CLI hasflagall|needflags|hasargall|needargs — F_main_carve peel.
 * Peel from cubalc_main.c. Cube is SoT. Zero bloat.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_flaggate.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cubalc_cmd_flaggate(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "hasflagall";
  /* Usability: CLI dual of HASFLAGALL/NEEDFLAGS · HASFLAGANY/NEEDFLAGANY ·
   * HASARGALL/NEEDARGS · HASARGANY/NEEDARGANY.
   * Optional live argv after -- :
   *   cubalc needflags out verbose -- --out=x --verbose a.txt
   *   cubalc hasflagany json yaml -- --json a.txt
   *   cubalc needargs 0 1 -- file0 file1
   *   cubalc hasargany 0 FILE -- file0
   * Schema cubalc.flaggate.v1 / cubalc.arggate.v1 */
  int is_arg = (strcmp(cmd, "hasargall") == 0 || strcmp(cmd, "hasallargs") == 0 ||
                strcmp(cmd, "needargs") == 0 || strcmp(cmd, "requireargs") == 0 ||
                strcmp(cmd, "mustargs") == 0 ||
                strcmp(cmd, "hasargany") == 0 || strcmp(cmd, "hasanyargs") == 0 ||
                strcmp(cmd, "needargany") == 0 || strcmp(cmd, "requireargany") == 0 ||
                strcmp(cmd, "mustargany") == 0 || strcmp(cmd, "needanyargs") == 0);
  int is_any = (strcmp(cmd, "hasflagany") == 0 || strcmp(cmd, "hasanyflags") == 0 ||
                strcmp(cmd, "needflagany") == 0 || strcmp(cmd, "requireflagany") == 0 ||
                strcmp(cmd, "mustflagany") == 0 || strcmp(cmd, "needanyflags") == 0 ||
                strcmp(cmd, "hasargany") == 0 || strcmp(cmd, "hasanyargs") == 0 ||
                strcmp(cmd, "needargany") == 0 || strcmp(cmd, "requireargany") == 0 ||
                strcmp(cmd, "mustargany") == 0 || strcmp(cmd, "needanyargs") == 0);
  int hard = (strcmp(cmd, "needflags") == 0 || strcmp(cmd, "requireflags") == 0 ||
              strcmp(cmd, "mustflags") == 0 ||
              strcmp(cmd, "needflagany") == 0 || strcmp(cmd, "requireflagany") == 0 ||
              strcmp(cmd, "mustflagany") == 0 || strcmp(cmd, "needanyflags") == 0 ||
              strcmp(cmd, "needargs") == 0 || strcmp(cmd, "requireargs") == 0 ||
              strcmp(cmd, "mustargs") == 0 ||
              strcmp(cmd, "needargany") == 0 || strcmp(cmd, "requireargany") == 0 ||
              strcmp(cmd, "mustargany") == 0 || strcmp(cmd, "needanyargs") == 0);
  char src[1200];
  char names[32][96];
  int nname = 0, i, ai, nlive = 0, hit = 0;
  size_t o = 0;
  cubalc_run_result rr;
  char envn[32];
  const char *form;
  /* collect names until -- */
  for (i = 2; i < argc && nname < 32; i++) {
    if (!argv[i] || !argv[i][0]) continue;
    if (!strcmp(argv[i], "--")) {
      i++;
      break;
    }
    snprintf(names[nname++], sizeof names[0], "%s", argv[i]);
  }
  /* live args after -- → CUBALC_ARGn */
  for (; i < argc && nlive < 32; i++, nlive++) {
    snprintf(envn, sizeof envn, "CUBALC_ARG%d", nlive);
    setenv(envn, argv[i] ? argv[i] : "", 1);
  }
  if (nlive > 0) {
    snprintf(envn, sizeof envn, "%d", nlive);
    setenv("CUBALC_ARGC", envn, 1);
  }
  if (nname == 0) {
    fprintf(stderr,
            "usage: cubalc hasflagall|needflags <flag> [flag…] [-- args…]\n"
            "       cubalc hasflagany|needflagany <flag> [flag…] [-- args…]\n"
            "       cubalc hasargall|needargs <n|name> [n|name…] [-- args…]\n"
            "       cubalc hasargany|needargany <n|name> [n|name…] [-- args…]\n");
    printf("{\"schema\":\"cubalc.%sgate.v1\",\"ok\":false,\"cmd\":\"%s\","
           "\"err\":\"need names\",\"version\":\"%s\"}\n",
           is_arg ? "arg" : "flag", cmd, CUBALC_LANG_VERSION);
    return 2;
  }
  if (is_arg && is_any)
    form = hard ? "NEEDARGANY" : "HASARGANY";
  else if (is_arg)
    form = hard ? "NEEDARGS" : "HASARGALL";
  else if (is_any)
    form = hard ? "NEEDFLAGANY" : "HASFLAGANY";
  else
    form = hard ? "NEEDFLAGS" : "HASFLAGALL";
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
  /* PRINT LAST_N so soft has* plate can read 0|1 from last_print */
  o += (size_t)snprintf(src + o, sizeof src - o, "\nPRINT LAST_N\nPASS\n");
  (void)o;
  memset(&rr, 0, sizeof rr);
  (void)cubalc_run_source(src, strlen(src), is_arg ? "<cli-arggate>" : "<cli-flaggate>",
                          &rr, NULL);
  /* hard need*: fail if runtime err (NEEDFLAGS/NEEDFLAGANY/NEEDARGS fail_at).
   * soft has*: LAST_N printed as 0|1. */
  if (hard)
    hit = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
  else
    hit = (rr.ok && !rr.err[0] && rr.last_print[0] == '1') ? 1 : 0;
  printf("{\"schema\":\"cubalc.%sgate.v1\",\"ok\":%s,\"cmd\":\"%s\","
         "\"mode\":\"%s\",\"any\":%s,\"n\":%d,\"live_args\":%d,\"version\":\"%s\","
         "\"note\":\"CLI dual of %s · optional -- live argv\","
         "\"names\":[",
         is_arg ? "arg" : "flag",
         hit ? "true" : "false", cmd,
         hard ? "need" : "has", is_any ? "true" : "false", nname, nlive,
         CUBALC_LANG_VERSION, form);
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
