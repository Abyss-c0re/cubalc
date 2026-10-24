/* CubalC CLI findarg/afterarg — F_main_carve peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_findarg.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include "cubalc_hostops.h"
#include "cubalc_run_util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cubalc_cmd_findarg(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "findarg";
  if (strcmp(cmd, "findarg") == 0 || strcmp(cmd, "argindex") == 0 ||
      strcmp(cmd, "indexarg") == 0 || strcmp(cmd, "whicharg") == 0 ||
      strcmp(cmd, "argvindex") == 0) {
    /* Usability: CLI dual of FINDARG — reverse NTHARG (token → index) without .cubalc.
     *   cubalc findarg --verbose -- --verbose file.txt
     * Schema cubalc.findarg.v1 · hit + index · soft miss exit 0 with hit=false. */
    cubalc_run_result rr;
    int ok, ai, narg = 0, tok_i = -1;
    long idx = -1, hit = 0;
    char envn[32];
    char token[CUBALC_HOST_STR_MAX];
    char src[CUBALC_HOST_STR_MAX + 96];
    char esc_tok[CUBALC_HOST_STR_MAX];
    size_t e = 0, k;
    token[0] = 0;
    /* token is first non-option after cmd; optional live args after -- */
    for (ai = 2; ai < argc; ai++) {
      if (!strcmp(argv[ai], "--")) {
        ai++;
        break;
      }
      if (tok_i < 0) {
        tok_i = ai;
        snprintf(token, sizeof token, "%s", argv[ai]);
      }
    }
    if (tok_i < 0 || !token[0]) {
      fprintf(stderr, "usage: cubalc findarg|argindex <token> [-- args…]\n");
      printf("{\"schema\":\"cubalc.findarg.v1\",\"ok\":false,\"cmd\":\"findarg\","
             "\"version\":\"%s\",\"note\":\"token required · dual of FINDARG\","
             "\"err\":\"missing token\"}\n",
             CUBALC_LANG_VERSION);
      return 1;
    }
    if (ai < argc) {
      for (; ai < argc && narg < 32; ai++, narg++) {
        snprintf(envn, sizeof envn, "CUBALC_ARG%d", narg);
        setenv(envn, argv[ai], 1);
      }
      snprintf(envn, sizeof envn, "%d", narg);
      setenv("CUBALC_ARGC", envn, 1);
    }
    /* escape token for string literal in -e source */
    {
      char lit[CUBALC_HOST_STR_MAX];
      size_t li = 0;
      for (k = 0; token[k] && li + 2 < sizeof lit; k++) {
        char c = token[k];
        if (c == '"' || c == '\\') {
          lit[li++] = '\\';
          lit[li++] = c;
        } else if ((unsigned char)c < 32)
          lit[li++] = ' ';
        else
          lit[li++] = c;
      }
      lit[li] = 0;
      snprintf(src, sizeof src,
               "FINDARG \"%s\"\n"
               "PRINT LAST_N\n"
               "PASS\n",
               lit);
    }
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<cli-findarg>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    if (rr.last_print[0])
      idx = strtol(rr.last_print, NULL, 10);
    hit = (idx >= 0) ? 1 : 0;
    for (k = 0; token[k] && e + 2 < sizeof esc_tok; k++) {
      char c = token[k];
      if (c == '"' || c == '\\') {
        esc_tok[e++] = '\\';
        esc_tok[e++] = c;
      } else if ((unsigned char)c < 32)
        esc_tok[e++] = ' ';
      else
        esc_tok[e++] = c;
    }
    esc_tok[e] = 0;
    printf("{\"schema\":\"cubalc.findarg.v1\",\"ok\":%s,\"cmd\":\"findarg\","
           "\"hit\":%s,\"index\":%ld,\"token\":\"%s\",\"live_args\":%d,"
           "\"version\":\"%s\","
           "\"note\":\"CLI dual of FINDARG · reverse NTHARG token→index\"}\n",
           ok ? "true" : "false", hit ? "true" : "false", idx, esc_tok, narg,
           CUBALC_LANG_VERSION);
    return ok ? 0 : 1;
  }
  if (strcmp(cmd, "afterarg") == 0 || strcmp(cmd, "nextarg") == 0 ||
      strcmp(cmd, "argafter") == 0 || strcmp(cmd, "followarg") == 0 ||
      strcmp(cmd, "argnext") == 0) {
    /* Usability: CLI dual of AFTERARG — peel argv after exact token without .cubalc.
     *   cubalc afterarg --out -- --verbose --out x.txt
     * Schema cubalc.afterarg.v1 · value + hit · soft miss empty value. */
    cubalc_run_result rr;
    int ok, ai, narg = 0, tok_i = -1;
    long hit = 0;
    char envn[32];
    char token[CUBALC_HOST_STR_MAX];
    char src[CUBALC_HOST_STR_MAX + 128];
    char esc_tok[CUBALC_HOST_STR_MAX], esc_val[CUBALC_HOST_STR_MAX];
    size_t e = 0, k;
    token[0] = 0;
    for (ai = 2; ai < argc; ai++) {
      if (!strcmp(argv[ai], "--")) {
        ai++;
        break;
      }
      if (tok_i < 0) {
        tok_i = ai;
        snprintf(token, sizeof token, "%s", argv[ai]);
      }
    }
    if (tok_i < 0 || !token[0]) {
      fprintf(stderr, "usage: cubalc afterarg|nextarg <token> [-- args…]\n");
      printf("{\"schema\":\"cubalc.afterarg.v1\",\"ok\":false,\"cmd\":\"afterarg\","
             "\"version\":\"%s\",\"note\":\"token required · dual of AFTERARG\","
             "\"err\":\"missing token\"}\n",
             CUBALC_LANG_VERSION);
      return 1;
    }
    if (ai < argc) {
      for (; ai < argc && narg < 32; ai++, narg++) {
        snprintf(envn, sizeof envn, "CUBALC_ARG%d", narg);
        setenv(envn, argv[ai], 1);
      }
      snprintf(envn, sizeof envn, "%d", narg);
      setenv("CUBALC_ARGC", envn, 1);
    }
    {
      char lit[CUBALC_HOST_STR_MAX];
      size_t li = 0;
      for (k = 0; token[k] && li + 2 < sizeof lit; k++) {
        char c = token[k];
        if (c == '"' || c == '\\') {
          lit[li++] = '\\';
          lit[li++] = c;
        } else if ((unsigned char)c < 32)
          lit[li++] = ' ';
        else
          lit[li++] = c;
      }
      lit[li] = 0;
      snprintf(src, sizeof src,
               "AFTERARG \"%s\"\n"
               "PRINT LAST\n"
               "PASS\n",
               lit);
    }
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<cli-afterarg>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    hit = (rr.last_print[0] != 0) ? 1 : 0;
    for (k = 0; token[k] && e + 2 < sizeof esc_tok; k++) {
      char c = token[k];
      if (c == '"' || c == '\\') {
        esc_tok[e++] = '\\';
        esc_tok[e++] = c;
      } else if ((unsigned char)c < 32)
        esc_tok[e++] = ' ';
      else
        esc_tok[e++] = c;
    }
    esc_tok[e] = 0;
    e = 0;
    for (k = 0; rr.last_print[k] && e + 2 < sizeof esc_val; k++) {
      char c = rr.last_print[k];
      if (c == '"' || c == '\\') {
        esc_val[e++] = '\\';
        esc_val[e++] = c;
      } else if (c == '\n') {
        esc_val[e++] = '\\';
        esc_val[e++] = 'n';
      } else if ((unsigned char)c < 32)
        esc_val[e++] = ' ';
      else
        esc_val[e++] = c;
    }
    esc_val[e] = 0;
    printf("{\"schema\":\"cubalc.afterarg.v1\",\"ok\":%s,\"cmd\":\"afterarg\","
           "\"hit\":%s,\"token\":\"%s\",\"value\":\"%s\",\"live_args\":%d,"
           "\"version\":\"%s\","
           "\"note\":\"CLI dual of AFTERARG · peel argv after exact token\"}\n",
           ok ? "true" : "false", hit ? "true" : "false", esc_tok, esc_val, narg,
           CUBALC_LANG_VERSION);
    return ok ? 0 : 1;
  }
  fprintf(stderr, "cubalc findarg: unexpected\n");
  return 2;
}
