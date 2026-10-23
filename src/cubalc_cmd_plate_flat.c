/* CubalC — CLI plate FLAT/LEAF op plane (peeled from cubalc_cmd_plate).
 * B_cmd_plate_peel · leaves|flat|…|needflatn. Cube is SoT. Zero bloat.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_plate_flat.h"
#include "cubalc.h"
#include "cubalc_smx.h"
#include "cubalc_lang.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

/* Returns 0/1/2 like cubalc_cmd_plate when op handled; -1 if not a flat op. */
int cubalc_cmd_plate_try_flat(const char *op, const char *path, char *plate,
                              int *file_hit, int argc, char **argv, int *ai_io) {
  cubalc_host_result hr, keys, wr;
  char parent[512];
  if (!op || !path || !plate || !file_hit || !ai_io) return -1;
  (void)argc;
  memset(&hr, 0, sizeof hr);
  memset(&keys, 0, sizeof keys);
  memset(&wr, 0, sizeof wr);
  parent[0] = 0;
  const char *val = NULL;
  const char *fb = NULL;
  long delta = 1;
  int have_fb = 0;
  /* (*ai_io) is the argv cursor */

  /* gate: only flat/leaf family canonical ops */
  if (!(strcmp(op, "leaves") == 0 || strcmp(op, "flat") == 0 ||
        strcmp(op, "unflat") == 0 || strcmp(op, "grepf") == 0 ||
        strcmp(op, "grepvf") == 0 || strcmp(op, "grepfi") == 0 ||
        strcmp(op, "prune") == 0 || strcmp(op, "keeponly") == 0 ||
        strcmp(op, "mergeflat") == 0 || strcmp(op, "renameflat") == 0 ||
        strcmp(op, "setflat") == 0 || strcmp(op, "incflat") == 0 ||
        strcmp(op, "decflat") == 0 || strcmp(op, "sumflat") == 0 ||
        strcmp(op, "avgflat") == 0 || strcmp(op, "medianflat") == 0 ||
        strcmp(op, "toppath") == 0 || strcmp(op, "botpath") == 0 ||
        strcmp(op, "threshflat") == 0 || strcmp(op, "dropzeroflat") == 0 ||
        strcmp(op, "capflat") == 0 || strcmp(op, "scaleflat") == 0 ||
        strcmp(op, "hasflat") == 0 || strcmp(op, "countflat") == 0 ||
        strcmp(op, "hasflatn") == 0 || strcmp(op, "countflatn") == 0 ||
        strcmp(op, "pathsflat") == 0 || strcmp(op, "valsflat") == 0 ||
        strcmp(op, "pathsflatn") == 0 || strcmp(op, "valsflatn") == 0 ||
        strcmp(op, "getflat") == 0 || strcmp(op, "lastflat") == 0 ||
        strcmp(op, "nthflat") == 0 || strcmp(op, "needflat") == 0 ||
        strcmp(op, "getflatn") == 0 || strcmp(op, "lastflatn") == 0 ||
        strcmp(op, "nthflatn") == 0 || strcmp(op, "typeflat") == 0 ||
        strcmp(op, "freqflat") == 0 || strcmp(op, "modeflat") == 0 ||
        strcmp(op, "pathbyval") == 0 || strcmp(op, "uniqflat") == 0 ||
        strcmp(op, "pathsbyval") == 0 || strcmp(op, "countbyval") == 0 ||
        strcmp(op, "hasval") == 0 || strcmp(op, "setbyval") == 0 ||
        strcmp(op, "delbyval") == 0 || strcmp(op, "alleqflat") == 0 ||
        strcmp(op, "firstuneq") == 0 || strcmp(op, "uneqpaths") == 0 ||
        strcmp(op, "uniform") == 0 || strcmp(op, "needflatn") == 0))
    return -1;

    /* leaves|pathkeys [nest.path] — PATHKEYS dual: dotted leaf path bag.
     *   cubalc plate leaves agent.json
     *   cubalc plate leaves agent.json cfg
     *   cubalc plate pathkeys agent.json cfg.meta */
    if (strcmp(op, "leaves") == 0) {
      const char *nestp = NULL;
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t i, o = 0;

      if ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0])
        nestp = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_paths(plate, nestp, &pr) != 0) {
        pr.str[0] = 0;
        pr.n = 0;
      }
      flat[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof flat; i++) {
        char ch = pr.str[i];
        if (ch == '\n' || ch == '\r') {
          if (o > 0 && flat[o - 1] != ',')
            flat[o++] = ',';
        } else if (ch == '"' || ch == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = ch;
        }
      }
      flat[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"leaves\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"paths\":\"%s\",\"version\":\"%s\","
             "\"note\":\"PATHKEYS dual · dotted leaf paths · bag lines above plate\"}\n",
             path, nestp ? nestp : "", (*file_hit) ? "true" : "false",
             pr.n, flat, CUBALC_LANG_VERSION);
      return 0;
    }

    /* flat|flatkv [nest.path] — FLATKV dual: recursive path:value bag.
     *   cubalc plate flat agent.json
     *   cubalc plate flat agent.json cfg
     *   cubalc plate flatkv agent.json cfg.meta */
    if (strcmp(op, "flat") == 0) {
      const char *nestp = NULL;
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t i, o = 0;

      if ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0])
        nestp = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_kv(plate, nestp, &pr) != 0) {
        pr.str[0] = 0;
        pr.n = 0;
      }
      flat[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof flat; i++) {
        char ch = pr.str[i];
        if (ch == '\n' || ch == '\r') {
          if (o > 0 && flat[o - 1] != ',')
            flat[o++] = ',';
        } else if (ch == '"' || ch == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = ch;
        }
      }
      flat[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"flat\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"FLATKV dual · path:value bag · lines above plate\"}\n",
             path, nestp ? nestp : "", (*file_hit) ? "true" : "false",
             pr.n, flat, CUBALC_LANG_VERSION);
      return 0;
    }

    /* unflat|unflatkv [UNDER nest] bag|@file|k:v… — UNFLATKV dual: path:value → nest plate.
     *   cubalc plate unflat agent.json host:cubeA cfg.port:9
     *   cubalc plate unflat agent.json @flat.txt
     *   cubalc plate unflat agent.json UNDER cfg meta.role:worker port:8080
     * Creates plate file if missing. Empty val → {}. Writes plate back to path. */
    if (strcmp(op, "unflat") == 0) {
      const char *under = NULL;
      char bag[CUBALC_HOST_STR_MAX];
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr, wr, tr;
      size_t o = 0, i;
      long nlines = 0;

      bag[0] = 0;
      /* optional UNDER nest */
      if ((*ai_io) < argc && argv[(*ai_io)] &&
          (strcmp(argv[(*ai_io)], "UNDER") == 0 || strcmp(argv[(*ai_io)], "under") == 0 ||
           strcmp(argv[(*ai_io)], "AT") == 0 || strcmp(argv[(*ai_io)], "at") == 0 ||
           strcmp(argv[(*ai_io)], "IN") == 0 || strcmp(argv[(*ai_io)], "in") == 0)) {
        (*ai_io)++;
        if ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0])
          under = argv[(*ai_io)++];
      }

      if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"unflat\",\"path\":\"%s\","
               "\"err\":\"need bag lines, @file, or path:val args\",\"version\":\"%s\"}\n",
               path, CUBALC_LANG_VERSION);
        return 2;
      }

      /* single @file or readable file → bag body; else join remaining as lines */
      if (argv[(*ai_io)][0] == '@' ||
          ((*ai_io) + 1 == argc && argv[(*ai_io)][0] != '{' &&
           strchr(argv[(*ai_io)], ':') == NULL && strchr(argv[(*ai_io)], '=') == NULL &&
           access(argv[(*ai_io)], R_OK) == 0)) {
        const char *fp = (argv[(*ai_io)][0] == '@') ? argv[(*ai_io)] + 1 : argv[(*ai_io)];
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(fp, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"unflat\",\"err\":\"bag file read fail\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(bag, sizeof bag, "%s", tr.str);
        (*ai_io)++;
      } else {
        while ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0]) {
          size_t kl = strlen(argv[(*ai_io)]);
          if (o + kl + 2 >= sizeof bag) break;
          if (o > 0) bag[o++] = '\n';
          memcpy(bag + o, argv[(*ai_io)], kl);
          o += kl;
          bag[o] = 0;
          (*ai_io)++;
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_unflat_kv(plate, bag, under, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"unflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "unflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      nlines = pr.n;
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", pr.str);

      /* write-back file when path looks like file */
      if ((*file_hit) || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"unflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        (*file_hit) = 1;
      }

      flat[0] = 0;
      o = 0;
      for (i = 0; bag[i] && o + 2 < sizeof flat; i++) {
        char ch = bag[i];
        if (ch == '\n' || ch == '\r') {
          if (o > 0 && flat[o - 1] != ',')
            flat[o++] = ',';
        } else if (ch == '"' || ch == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = ch;
        }
      }
      flat[o] = 0;

      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"unflat\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"UNFLATKV dual · path:value → nested plate\"}\n",
             path, under ? under : "", (*file_hit) ? "true" : "false",
             nlines, flat, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* grepf|grepvf|grepfi [needle] — GREPFLAT duals: filter path:value bag by path needle.
     *   cubalc plate grepf  agent.json cfg
     *   cubalc plate grepvf agent.json tmp
     *   cubalc plate grepfi agent.json CFG.META
     * Bag lines above plate. Empty needle → all leaves. */
    if (strcmp(op, "grepf") == 0 || strcmp(op, "grepvf") == 0 ||
        strcmp(op, "grepfi") == 0) {
      const char *needle = "";
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t i, o = 0;
      int invert = (strcmp(op, "grepvf") == 0) ? 1 : 0;
      int icase = (strcmp(op, "grepfi") == 0) ? 1 : 0;

      if ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      cubalc_host_json_leaf_grep(plate, needle, invert, icase, &pr);
      flat[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof flat; i++) {
        char ch = pr.str[i];
        if (ch == '\n' || ch == '\r') {
          if (o > 0 && flat[o - 1] != ',')
            flat[o++] = ',';
        } else if (ch == '"' || ch == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = ch;
        }
      }
      flat[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"invert\":%s,\"icase\":%s,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"GREPFLAT dual · path needle filter · bag lines above plate\"}\n",
             op, path, needle, (*file_hit) ? "true" : "false",
             pr.n, invert ? "true" : "false", icase ? "true" : "false",
             flat, CUBALC_LANG_VERSION);
      return 0;
    }

    /* prune|keeponly [needle] — PRUNEFLAT / KEEONLYFLAT duals (write-back).
     *   cubalc plate prune    agent.json tmp
     *   cubalc plate keeponly agent.json cfg
     * Rewrites plate file. n = deleted (prune) or kept (keeponly). */
    if (strcmp(op, "prune") == 0 || strcmp(op, "keeponly") == 0) {
      const char *needle = "";
      cubalc_host_result pr, wr;
      int keep_only = (strcmp(op, "keeponly") == 0) ? 1 : 0;

      if ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_filter_write(plate, needle, keep_only, 0, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "filter write fail",
               CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", pr.str);
      if ((*file_hit) || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
          return 1;
        }
        (*file_hit) = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"keep_only\":%s,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"%s\"}\n",
             op, path, needle, (*file_hit) ? "true" : "false",
             pr.n, keep_only ? "true" : "false", CUBALC_LANG_VERSION, plate,
             keep_only
                 ? "KEEPONLYFLAT dual · project matching leaves"
                 : "PRUNEFLAT dual · drop matching leaves");
      return 0;
    }

    /* mergeflat <overlay.json> — MERGEFLAT dual: deep leaf overlay onto path plate.
     *   cubalc plate mergeflat base.json overlay.json
     * Writes base. n = overlay leaf count. Nest-aware (unlike plate merge top-level). */
    if (strcmp(op, "mergeflat") == 0) {
      const char *path2 = NULL;
      char plate2[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr, wr, tr;
      int file_hit2 = 0;
      const char *body2;

      plate2[0] = 0;
      if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"mergeflat\",\"path\":\"%s\",\"err\":\"need overlay path\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      path2 = argv[(*ai_io)++];
      memset(&tr, 0, sizeof tr);
      if (cubalc_host_read(path2, &tr) == 0 && tr.str[0]) {
        body2 = tr.str;
        while (*body2 == ' ' || *body2 == '\t' || *body2 == '\n' || *body2 == '\r')
          body2++;
        if (*body2 == '{') {
          snprintf(plate2, (CUBALC_HOST_STR_MAX * 2), "%s", body2);
          file_hit2 = 1;
        }
      }
      if (!file_hit2)
        snprintf(plate2, (CUBALC_HOST_STR_MAX * 2), "%s", "{}");

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_merge(plate, plate2, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"mergeflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "mergeflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", pr.str);
      if ((*file_hit) || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"mergeflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        (*file_hit) = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"mergeflat\",\"path\":\"%s\",\"path2\":\"%s\",\"file\":%s,"
             "\"file2\":%s,\"n\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"MERGEFLAT dual · deep leaf overlay · nest-aware\"}\n",
             path, path2, (*file_hit) ? "true" : "false",
             file_hit2 ? "true" : "false", pr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* renameflat <old_pfx> <new_pfx> — RENAMEFLAT dual: rewrite leaf path prefixes.
     *   cubalc plate renameflat agent.json cfg. config.
     * Writes plate. n = renamed leaf count. */
    if (strcmp(op, "renameflat") == 0) {
      const char *oldp = NULL, *newp = NULL;
      cubalc_host_result pr, wr;

      if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"renameflat\",\"path\":\"%s\",\"err\":\"need old_pfx new_pfx\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      oldp = argv[(*ai_io)++];
      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"renameflat\",\"path\":\"%s\",\"err\":\"need new_pfx\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      newp = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_rename_pfx(plate, oldp, newp, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"renameflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "renameflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", pr.str);
      if ((*file_hit) || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"renameflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        (*file_hit) = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"renameflat\",\"path\":\"%s\",\"old\":\"%s\",\"new\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"RENAMEFLAT dual · leaf path prefix rewrite\"}\n",
             path, oldp, newp, (*file_hit) ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* setflat <needle> <value> — SETFLAT dual: bulk set leaf values by path needle.
     *   cubalc plate setflat agent.json debug 0
     *   cubalc plate setflat agent.json cfg.port 9090
     * Writes plate. n = updated leaf count. Empty needle sets all leaves. */
    if (strcmp(op, "setflat") == 0) {
      const char *needle = "", *val = NULL;
      cubalc_host_result pr, wr;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setflat\",\"path\":\"%s\",\"err\":\"need needle value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      /* allow empty needle as "" */
      needle = argv[(*ai_io)++];
      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setflat\",\"path\":\"%s\",\"err\":\"need value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      val = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_set(plate, needle, val, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "setflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", pr.str);
      if ((*file_hit) || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"setflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        (*file_hit) = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"setflat\",\"path\":\"%s\",\"needle\":\"%s\",\"value\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"SETFLAT dual · bulk leaf value set by path needle\"}\n",
             path, needle, val, (*file_hit) ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* incflat|decflat <needle> [delta] — INCFLAT dual: bump pure-int leaves by path needle.
     *   cubalc plate incflat agent.json hits
     *   cubalc plate incflat agent.json cfg.meta.x 5
     *   cubalc plate decflat agent.json errs
     * Writes plate. n = bumped count. Non-numeric soft-skipped. */
    if (strcmp(op, "incflat") == 0 || strcmp(op, "decflat") == 0) {
      const char *needle = "";
      long delta = 1;
      cubalc_host_result pr, wr;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need needle [delta]\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[(*ai_io)++];
      if ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0]) {
        char *end = NULL;
        long d = strtol(argv[(*ai_io)], &end, 10);
        if (end && end != argv[(*ai_io)] && *end == 0) {
          delta = d;
          (*ai_io)++;
        }
      }
      if (strcmp(op, "decflat") == 0) {
        if (delta > 0) delta = -delta;
        else if (delta == 1) delta = -1;
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_inc(plate, needle, delta, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "incflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", pr.str);
      if ((*file_hit) || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
          return 1;
        }
        (*file_hit) = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\",\"delta\":%ld,"
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"INCFLAT dual · bump pure-int leaves by path needle\"}\n",
             op, path, needle, delta, (*file_hit) ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* sumflat <needle?> — SUMFLAT dual: sum pure-int leaves by path needle (read-only).
     *   cubalc plate sumflat agent.json hits
     *   cubalc plate sumflat agent.json          # all pure-int leaves
     * n = sum · count = pure-int leaves used · plate not written. */
    if (strcmp(op, "sumflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_sum(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"sumflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "sumflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"sumflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"count\":%ld,\"sum\":%ld,\"version\":\"%s\","
             "\"note\":\"SUMFLAT dual · sum pure-int leaves by path needle (read-only)\"}\n",
             path, needle, (*file_hit) ? "true" : "false",
             pr.n, (long)pr.code, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* avgflat [needle] — AVGFLAT dual: integer mean pure-int leaves (read-only).
     *   cubalc plate avgflat agent.json score
     *   cubalc plate avgflat agent.json          # all pure-int leaves
     * n = mean · count = pure-int leaves used · sum side · plate not written. */
    if (strcmp(op, "avgflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      long sum_side = 0;

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_avg(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"avgflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "avgflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.err[0]) {
        char *end = NULL;
        long s = strtol(pr.err, &end, 10);
        if (end && end != pr.err && *end == 0)
          sum_side = s;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"avgflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"count\":%ld,\"avg\":%ld,\"sum\":%ld,"
             "\"version\":\"%s\","
             "\"note\":\"AVGFLAT dual · mean pure-int leaves by path needle (read-only)\"}\n",
             path, needle, (*file_hit) ? "true" : "false",
             pr.n, (long)pr.code, pr.n, sum_side, CUBALC_LANG_VERSION);
      return 0;
    }

    /* medianflat [needle] — MEDIANFLAT dual: integer median pure-int leaves (read-only).
     *   cubalc plate medianflat agent.json score
     *   cubalc plate medianflat agent.json          # all pure-int leaves
     * n = median · count = pure-int leaves used · plate not written. */
    if (strcmp(op, "medianflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_median(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"medianflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "medianflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"medianflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"count\":%ld,\"median\":%ld,"
             "\"version\":\"%s\","
             "\"note\":\"MEDIANFLAT dual · median pure-int leaves by path needle (read-only)\"}\n",
             path, needle, (*file_hit) ? "true" : "false",
             pr.n, (long)pr.code, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* toppath|botpath [needle] — TOPPATHFLAT/BOTPATHFLAT dual (read-only).
     *   cubalc plate toppath agent.json score
     *   cubalc plate botpath agent.json latency
     * path = winning leaf path · n = value · count = pure-int leaves considered. */
    if (strcmp(op, "toppath") == 0 || strcmp(op, "botpath") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int want_min = (strcmp(op, "botpath") == 0);

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_toppath(plate, needle, want_min, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "toppath fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"leaf\":\"%s\",\"n\":%ld,\"count\":%ld,\"version\":\"%s\","
             "\"note\":\"%s dual · path of %s pure-int leaf by needle (read-only)\"}\n",
             op, path, needle, (*file_hit) ? "true" : "false",
             pr.str, pr.n, (long)pr.code, CUBALC_LANG_VERSION,
             want_min ? "BOTPATHFLAT" : "TOPPATHFLAT",
             want_min ? "min" : "max");
      return 0;
    }

    /* threshflat [needle] min | dropzeroflat [needle]
     * THRESHFLAT/DROPZEROFLAT duals: denoise pure-int leaves by value · write plate.
     *   cubalc plate threshflat agent.json hits 5
     *   cubalc plate threshflat agent.json 10          # empty needle
     *   cubalc plate dropzeroflat agent.json delta
     */
    if (strcmp(op, "threshflat") == 0 || strcmp(op, "dropzeroflat") == 0) {
      const char *needle = "";
      long minv = 0;
      int drop_zero = (strcmp(op, "dropzeroflat") == 0);
      int have_min = 0;
      cubalc_host_result pr, wr;

      if (drop_zero) {
        if ((*ai_io) < argc && argv[(*ai_io)])
          needle = argv[(*ai_io)++];
      } else {
        if ((*ai_io) >= argc || !argv[(*ai_io)]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"threshflat\",\"path\":\"%s\",\"err\":\"need [needle] min\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 2;
        }
        /* if only one arg and it's a number → empty needle + min */
        if ((*ai_io) + 1 >= argc) {
          char *end = NULL;
          long d = strtol(argv[(*ai_io)], &end, 10);
          if (end && end != argv[(*ai_io)] && *end == 0) {
            minv = d;
            have_min = 1;
            (*ai_io)++;
          } else {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"threshflat\",\"path\":\"%s\",\"err\":\"need min int\","
                   "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
            return 2;
          }
        } else {
          needle = argv[(*ai_io)++];
          {
            char *end = NULL;
            long d = strtol(argv[(*ai_io)], &end, 10);
            if (!(end && end != argv[(*ai_io)] && *end == 0)) {
              printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                     "\"op\":\"threshflat\",\"path\":\"%s\",\"err\":\"need min int\","
                     "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
              return 2;
            }
            minv = d;
            have_min = 1;
            (*ai_io)++;
          }
        }
        (void)have_min;
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_thresh(plate, needle, minv, drop_zero, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "threshflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", pr.str);
      if ((*file_hit) || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
          return 1;
        }
        (*file_hit) = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\",\"min\":%ld,"
             "\"file\":%s,\"n\":%ld,\"drop\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"%s dual · pure-int leaf value denoise by path needle\"}\n",
             op, path, needle, minv, (*file_hit) ? "true" : "false",
             pr.n, (long)pr.code, CUBALC_LANG_VERSION, plate,
             drop_zero ? "DROPZEROFLAT" : "THRESHFLAT");
      return 0;
    }

    /* capflat [needle] max — CAPFLAT dual: clamp pure-int leaves to max · write plate.
     *   cubalc plate capflat agent.json score 100
     *   cubalc plate capflat agent.json 50          # empty needle
     */
    if (strcmp(op, "capflat") == 0) {
      const char *needle = "";
      long maxv = 0;
      cubalc_host_result pr, wr;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"need [needle] max\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      if ((*ai_io) + 1 >= argc) {
        char *end = NULL;
        long d = strtol(argv[(*ai_io)], &end, 10);
        if (end && end != argv[(*ai_io)] && *end == 0) {
          maxv = d;
          (*ai_io)++;
        } else {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"need max int\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 2;
        }
      } else {
        needle = argv[(*ai_io)++];
        {
          char *end = NULL;
          long d = strtol(argv[(*ai_io)], &end, 10);
          if (!(end && end != argv[(*ai_io)] && *end == 0)) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"need max int\","
                   "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
            return 2;
          }
          maxv = d;
          (*ai_io)++;
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_cap(plate, needle, maxv, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "capflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", pr.str);
      if ((*file_hit) || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        (*file_hit) = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"capflat\",\"path\":\"%s\",\"needle\":\"%s\",\"max\":%ld,"
             "\"file\":%s,\"n\":%ld,\"match\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"CAPFLAT dual · clamp pure-int leaves to max by path needle\"}\n",
             path, needle, maxv, (*file_hit) ? "true" : "false",
             pr.n, (long)pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* scaleflat [needle] factor — SCALEFLAT dual: multiply pure-int leaves · write plate.
     *   cubalc plate scaleflat agent.json score 2
     *   cubalc plate scaleflat agent.json 10          # empty needle
     */
    if (strcmp(op, "scaleflat") == 0) {
      const char *needle = "";
      long factor = 1;
      cubalc_host_result pr, wr;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"need [needle] factor\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      if ((*ai_io) + 1 >= argc) {
        char *end = NULL;
        long d = strtol(argv[(*ai_io)], &end, 10);
        if (end && end != argv[(*ai_io)] && *end == 0) {
          factor = d;
          (*ai_io)++;
        } else {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"need factor int\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 2;
        }
      } else {
        needle = argv[(*ai_io)++];
        {
          char *end = NULL;
          long d = strtol(argv[(*ai_io)], &end, 10);
          if (!(end && end != argv[(*ai_io)] && *end == 0)) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"need factor int\","
                   "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
            return 2;
          }
          factor = d;
          (*ai_io)++;
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_scale(plate, needle, factor, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "scaleflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", pr.str);
      if ((*file_hit) || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        (*file_hit) = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"scaleflat\",\"path\":\"%s\",\"needle\":\"%s\",\"factor\":%ld,"
             "\"file\":%s,\"n\":%ld,\"match\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"SCALEFLAT dual · multiply pure-int leaves by factor\"}\n",
             path, needle, factor, (*file_hit) ? "true" : "false",
             pr.n, (long)pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* hasflat|countflat [needle] — HASFLAT/COUNTFLAT duals (read-only).
     *   cubalc plate hasflat agent.json error
     *   cubalc plate countflat agent.json score
     * hasflat: n = 0|1 hit · count = match count
     * countflat: n = match count
     */
    if (strcmp(op, "hasflat") == 0 || strcmp(op, "countflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int is_count = (strcmp(op, "countflat") == 0);

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_count(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "hasflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      if (is_count) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"countflat\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"COUNTFLAT dual · count leaf paths matching needle (read-only)\"}\n",
               path, needle, (*file_hit) ? "true" : "false",
               pr.n, CUBALC_LANG_VERSION);
      } else {
        long hit = (pr.n > 0) ? 1 : 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"hasflat\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"count\":%ld,\"hit\":%s,\"version\":\"%s\","
               "\"note\":\"HASFLAT dual · soft leaf-path presence by needle (read-only)\"}\n",
               path, needle, (*file_hit) ? "true" : "false",
               hit, pr.n, hit ? "true" : "false", CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* hasflatn|countflatn [needle] — HASFLATN/COUNTFLATN duals (pure-int, read-only).
     *   cubalc plate hasflatn agent.json port
     *   cubalc plate countflatn agent.json score
     * hasflatn: n=0|1 · count=pure-int matches
     * countflatn: n=pure-int match count
     */
    if (strcmp(op, "hasflatn") == 0 || strcmp(op, "countflatn") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int is_count = (strcmp(op, "countflatn") == 0);

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_countn(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "hasflatn fail", CUBALC_LANG_VERSION);
        return 1;
      }
      if (is_count) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"countflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"COUNTFLATN dual · pure-int leaf count by needle (read-only)\"}\n",
               path, needle, (*file_hit) ? "true" : "false",
               pr.n, CUBALC_LANG_VERSION);
      } else {
        long hit = (pr.n > 0) ? 1 : 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"hasflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"count\":%ld,\"hit\":%s,\"version\":\"%s\","
               "\"note\":\"HASFLATN dual · soft pure-int leaf presence by needle (read-only)\"}\n",
               path, needle, (*file_hit) ? "true" : "false",
               hit, pr.n, hit ? "true" : "false", CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* pathsflat|valsflat [needle] — PATHSFLAT/VALSFLAT duals (read-only bags).
     *   cubalc plate pathsflat agent.json score
     *   cubalc plate valsflat agent.json hits
     * bag = newline fields · n = count. */
    if (strcmp(op, "pathsflat") == 0 || strcmp(op, "valsflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int want_vals = (strcmp(op, "valsflat") == 0);
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_match_bag(plate, needle, want_vals, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "pathsflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      /* JSON-escape bag for plate field */
      esc[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') {
          esc[o++] = '\\';
          esc[o++] = c;
        } else if (c == '\n') {
          esc[o++] = '\\';
          esc[o++] = 'n';
        } else if (c == '\r') {
          /* skip */
        } else {
          esc[o++] = c;
        }
      }
      esc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"%s dual · matching leaf %s bag (read-only)\"}\n",
             op, path, needle, (*file_hit) ? "true" : "false",
             pr.n, esc, CUBALC_LANG_VERSION,
             want_vals ? "VALSFLAT" : "PATHSFLAT",
             want_vals ? "values" : "paths");
      return 0;
    }

    /* pathsflatn|valsflatn [needle] — PATHSFLATN/VALSFLATN duals (pure-int bags).
     *   cubalc plate pathsflatn agent.json score
     *   cubalc plate valsflatn agent.json hits
     * bag = newline pure-int path or value fields · n = count. */
    if (strcmp(op, "pathsflatn") == 0 || strcmp(op, "valsflatn") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int want_vals = (strcmp(op, "valsflatn") == 0);
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_match_bagn(plate, needle, want_vals, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "pathsflatn fail", CUBALC_LANG_VERSION);
        return 1;
      }
      esc[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') {
          esc[o++] = '\\';
          esc[o++] = c;
        } else if (c == '\n') {
          esc[o++] = '\\';
          esc[o++] = 'n';
        } else if (c == '\r') {
          /* skip */
        } else {
          esc[o++] = c;
        }
      }
      esc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"%s dual · pure-int leaf %s bag (read-only)\"}\n",
             op, path, needle, (*file_hit) ? "true" : "false",
             pr.n, esc, CUBALC_LANG_VERSION,
             want_vals ? "VALSFLATN" : "PATHSFLATN",
             want_vals ? "values" : "paths");
      return 0;
    }

    /* getflat <needle> [OR|DEFAULT def] — GETFLAT dual (read-only).
     *   cubalc plate getflat agent.json port
     *   cubalc plate getflat agent.json port OR 8080
     * value = first matching leaf · leaf = winning path · hit true/false. */
    if (strcmp(op, "getflat") == 0) {
      const char *needle = "", *fb = NULL;
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2], lesc[512];
      size_t i, o;
      const char *val, *leafp;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getflat\",\"path\":\"%s\",\"err\":\"need needle [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[(*ai_io)++];
      if ((*ai_io) < argc && argv[(*ai_io)] &&
          (strcmp(argv[(*ai_io)], "OR") == 0 || strcmp(argv[(*ai_io)], "or") == 0 ||
           strcmp(argv[(*ai_io)], "DEFAULT") == 0 || strcmp(argv[(*ai_io)], "default") == 0)) {
        (*ai_io)++;
        if ((*ai_io) < argc && argv[(*ai_io)])
          fb = argv[(*ai_io)++];
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_get(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getflat\",\"path\":\"%s\",\"err\":\"getflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.n == 0 && fb) {
        val = fb;
        leafp = "";
      } else {
        val = pr.str;
        leafp = pr.err;
      }
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      for (i = 0, o = 0; leafp[i] && o + 2 < sizeof lesc; i++) {
        char c = leafp[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"getflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"hit\":%s,\"fallback\":%s,\"leaf\":\"%s\",\"value\":\"%s\","
             "\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"GETFLAT dual · first matching leaf value by path needle\"}\n",
             path, needle, (*file_hit) ? "true" : "false",
             pr.n ? "true" : "false",
             (pr.n == 0 && fb) ? "true" : "false",
             lesc, vesc, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* lastflat <needle> [OR|DEFAULT def] — LASTFLAT dual (read-only).
     *   cubalc plate lastflat agent.json port
     *   cubalc plate lastflat agent.json port OR 8080
     * value = last matching leaf · leaf = winning path · hit true/false. */
    if (strcmp(op, "lastflat") == 0) {
      const char *needle = "", *fb = NULL;
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2], lesc[512];
      size_t i, o;
      const char *val, *leafp;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"lastflat\",\"path\":\"%s\",\"err\":\"need needle [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[(*ai_io)++];
      if ((*ai_io) < argc && argv[(*ai_io)] &&
          (strcmp(argv[(*ai_io)], "OR") == 0 || strcmp(argv[(*ai_io)], "or") == 0 ||
           strcmp(argv[(*ai_io)], "DEFAULT") == 0 || strcmp(argv[(*ai_io)], "default") == 0)) {
        (*ai_io)++;
        if ((*ai_io) < argc && argv[(*ai_io)])
          fb = argv[(*ai_io)++];
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_get_last(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"lastflat\",\"path\":\"%s\",\"err\":\"lastflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.n == 0 && fb) {
        val = fb;
        leafp = "";
      } else {
        val = pr.str;
        leafp = pr.err;
      }
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      for (i = 0, o = 0; leafp[i] && o + 2 < sizeof lesc; i++) {
        char c = leafp[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"lastflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"hit\":%s,\"fallback\":%s,\"leaf\":\"%s\",\"value\":\"%s\","
             "\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"LASTFLAT dual · last matching leaf value by path needle\"}\n",
             path, needle, (*file_hit) ? "true" : "false",
             pr.n ? "true" : "false",
             (pr.n == 0 && fb) ? "true" : "false",
             lesc, vesc, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* nthflat <needle> <i> [OR def] — NTHFLAT dual: 0-based Nth matching leaf (read-only).
     *   cubalc plate nthflat agent.json role 0
     *   cubalc plate nthflat agent.json role 1 OR worker
     * value · hit · leaf · i. */
    if (strcmp(op, "nthflat") == 0) {
      const char *needle = "", *fb = NULL;
      long idx = 0;
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2], lesc[512];
      size_t i, o;
      const char *val, *leafp;
      char *end = NULL;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflat\",\"path\":\"%s\",\"err\":\"need needle index [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[(*ai_io)++];
      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflat\",\"path\":\"%s\",\"err\":\"need needle index [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      end = NULL;
      idx = strtol(argv[(*ai_io)], &end, 10);
      if (!(end && end != argv[(*ai_io)] && *end == 0)) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflat\",\"path\":\"%s\",\"err\":\"bad index\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      (*ai_io)++;
      if ((*ai_io) < argc && argv[(*ai_io)] &&
          (strcmp(argv[(*ai_io)], "OR") == 0 || strcmp(argv[(*ai_io)], "or") == 0 ||
           strcmp(argv[(*ai_io)], "DEFAULT") == 0 || strcmp(argv[(*ai_io)], "default") == 0)) {
        (*ai_io)++;
        if ((*ai_io) < argc && argv[(*ai_io)])
          fb = argv[(*ai_io)++];
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_get_nth(plate, needle, idx, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflat\",\"path\":\"%s\",\"err\":\"nthflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.n == 0 && fb) {
        val = fb;
        leafp = "";
      } else {
        val = pr.str;
        leafp = pr.err;
      }
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      for (i = 0, o = 0; leafp[i] && o + 2 < sizeof lesc; i++) {
        char c = leafp[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"nthflat\",\"path\":\"%s\",\"needle\":\"%s\",\"i\":%ld,"
             "\"file\":%s,\"hit\":%s,\"fallback\":%s,\"leaf\":\"%s\",\"value\":\"%s\","
             "\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"NTHFLAT dual · 0-based Nth matching leaf by path needle\"}\n",
             path, needle, idx, (*file_hit) ? "true" : "false",
             pr.n ? "true" : "false",
             (pr.n == 0 && fb) ? "true" : "false",
             lesc, vesc, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* needflat <needle> [needle…] — NEEDFLAT dual: fail-fast leaf path contract.
     *   cubalc plate needflat agent.json tls agent_id
     * exit 0 if all needles match ≥1 leaf · exit 2 if any miss (miss bag). */
    if (strcmp(op, "needflat") == 0) {
      char miss_bag[CUBALC_HOST_STR_MAX];
      size_t molen = 0;
      int n_need = 0, n_miss = 0;
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"needflat\",\"path\":\"%s\",\"err\":\"need needle [needle…]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      miss_bag[0] = 0;
      while ((*ai_io) < argc && argv[(*ai_io)]) {
        const char *nd = argv[(*ai_io)++];
        size_t mlen;
        n_need++;
        memset(&pr, 0, sizeof pr);
        cubalc_host_json_leaf_count(plate, nd, &pr);
        if (pr.n <= 0) {
          mlen = strlen(nd);
          n_miss++;
          if (molen > 0 && molen + 1 < sizeof miss_bag)
            miss_bag[molen++] = '\n';
          if (molen + mlen < sizeof miss_bag) {
            memcpy(miss_bag + molen, nd, mlen);
            molen += mlen;
            miss_bag[molen] = 0;
          }
        }
      }
      for (i = 0, o = 0; miss_bag[i] && o + 2 < sizeof esc; i++) {
        char c = miss_bag[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      if (n_miss > 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"needflat\",\"path\":\"%s\",\"file\":%s,"
               "\"n\":%d,\"miss_n\":%d,\"miss\":\"%s\",\"version\":\"%s\","
               "\"note\":\"NEEDFLAT dual · missing leaf path needle(s)\"}\n",
               path, (*file_hit) ? "true" : "false",
               n_need, n_miss, esc, CUBALC_LANG_VERSION);
        return 2;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"needflat\",\"path\":\"%s\",\"file\":%s,"
             "\"n\":%d,\"miss_n\":0,\"version\":\"%s\","
             "\"note\":\"NEEDFLAT dual · all leaf path needles present\"}\n",
             path, (*file_hit) ? "true" : "false", n_need, CUBALC_LANG_VERSION);
      return 0;
    }

    /* getflatn <needle> [OR n] — GETFLATN dual: first pure-int leaf by needle (read-only).
     *   cubalc plate getflatn agent.json port
     *   cubalc plate getflatn agent.json port OR 8080
     * n = value · hit · leaf path. */
    if (strcmp(op, "getflatn") == 0) {
      const char *needle = "";
      long fb_n = 0;
      int have_fb = 0;
      cubalc_host_result pr;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"err\":\"need needle [OR n]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[(*ai_io)++];
      if ((*ai_io) < argc && argv[(*ai_io)] &&
          (strcmp(argv[(*ai_io)], "OR") == 0 || strcmp(argv[(*ai_io)], "or") == 0 ||
           strcmp(argv[(*ai_io)], "DEFAULT") == 0 || strcmp(argv[(*ai_io)], "default") == 0)) {
        (*ai_io)++;
        if ((*ai_io) < argc && argv[(*ai_io)]) {
          char *end = NULL;
          long d = strtol(argv[(*ai_io)], &end, 10);
          if (end && end != argv[(*ai_io)] && *end == 0) {
            fb_n = d;
            have_fb = 1;
            (*ai_io)++;
          }
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_getn(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"err\":\"getflatn fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.code == 0 && have_fb) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":false,\"fallback\":true,\"leaf\":\"\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"GETFLATN dual · pure-int leaf miss used OR fallback\"}\n",
               path, needle, (*file_hit) ? "true" : "false", fb_n, CUBALC_LANG_VERSION);
      } else if (pr.code == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":false,\"fallback\":false,\"leaf\":\"\","
               "\"n\":0,\"version\":\"%s\","
               "\"note\":\"GETFLATN dual · no pure-int leaf match\"}\n",
               path, needle, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
      } else {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":true,\"fallback\":false,\"leaf\":\"%s\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"GETFLATN dual · first pure-int leaf by path needle\"}\n",
               path, needle, (*file_hit) ? "true" : "false", pr.err, pr.n,
               CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* lastflatn <needle> [OR n] — LASTFLATN dual: last pure-int leaf by needle (read-only).
     *   cubalc plate lastflatn agent.json port
     *   cubalc plate lastflatn agent.json port OR 8080
     * n = value · hit · leaf path (last match). */
    if (strcmp(op, "lastflatn") == 0) {
      const char *needle = "";
      long fb_n = 0;
      int have_fb = 0;
      cubalc_host_result pr;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"err\":\"need needle [OR n]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[(*ai_io)++];
      if ((*ai_io) < argc && argv[(*ai_io)] &&
          (strcmp(argv[(*ai_io)], "OR") == 0 || strcmp(argv[(*ai_io)], "or") == 0 ||
           strcmp(argv[(*ai_io)], "DEFAULT") == 0 || strcmp(argv[(*ai_io)], "default") == 0)) {
        (*ai_io)++;
        if ((*ai_io) < argc && argv[(*ai_io)]) {
          char *end = NULL;
          long d = strtol(argv[(*ai_io)], &end, 10);
          if (end && end != argv[(*ai_io)] && *end == 0) {
            fb_n = d;
            have_fb = 1;
            (*ai_io)++;
          }
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_getn_last(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"err\":\"lastflatn fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.code == 0 && have_fb) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":false,\"fallback\":true,\"leaf\":\"\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"LASTFLATN dual · pure-int leaf miss used OR fallback\"}\n",
               path, needle, (*file_hit) ? "true" : "false", fb_n, CUBALC_LANG_VERSION);
      } else if (pr.code == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":false,\"fallback\":false,\"leaf\":\"\","
               "\"n\":0,\"version\":\"%s\","
               "\"note\":\"LASTFLATN dual · no pure-int leaf match\"}\n",
               path, needle, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
      } else {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":true,\"fallback\":false,\"leaf\":\"%s\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"LASTFLATN dual · last pure-int leaf by path needle\"}\n",
               path, needle, (*file_hit) ? "true" : "false", pr.err, pr.n,
               CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* nthflatn <needle> <i> [OR n] — NTHFLATN dual: 0-based Nth pure-int leaf (read-only).
     *   cubalc plate nthflatn agent.json score 0
     *   cubalc plate nthflatn agent.json score 1 OR 0
     * n = value · hit · leaf path · i = index. */
    if (strcmp(op, "nthflatn") == 0) {
      const char *needle = "";
      long fb_n = 0, idx = 0;
      int have_fb = 0;
      cubalc_host_result pr;
      char *end = NULL;

      if ((*ai_io) + 1 > argc || !argv[(*ai_io)] || !argv[(*ai_io) + 1]) {
        /* need needle and index: at least 2 args remaining if both present */
      }
      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"err\":\"need needle index [OR n]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[(*ai_io)++];
      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"err\":\"need needle index [OR n]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      end = NULL;
      idx = strtol(argv[(*ai_io)], &end, 10);
      if (!(end && end != argv[(*ai_io)] && *end == 0)) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"err\":\"bad index\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      (*ai_io)++;
      if ((*ai_io) < argc && argv[(*ai_io)] &&
          (strcmp(argv[(*ai_io)], "OR") == 0 || strcmp(argv[(*ai_io)], "or") == 0 ||
           strcmp(argv[(*ai_io)], "DEFAULT") == 0 || strcmp(argv[(*ai_io)], "default") == 0)) {
        (*ai_io)++;
        if ((*ai_io) < argc && argv[(*ai_io)]) {
          end = NULL;
          long d = strtol(argv[(*ai_io)], &end, 10);
          if (end && end != argv[(*ai_io)] && *end == 0) {
            fb_n = d;
            have_fb = 1;
            (*ai_io)++;
          }
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_getn_nth(plate, needle, idx, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"err\":\"nthflatn fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.code == 0 && have_fb) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"needle\":\"%s\",\"i\":%ld,"
               "\"file\":%s,\"hit\":false,\"fallback\":true,\"leaf\":\"\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"NTHFLATN dual · pure-int leaf miss used OR fallback\"}\n",
               path, needle, idx, (*file_hit) ? "true" : "false", fb_n, CUBALC_LANG_VERSION);
      } else if (pr.code == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"needle\":\"%s\",\"i\":%ld,"
               "\"file\":%s,\"hit\":false,\"fallback\":false,\"leaf\":\"\","
               "\"n\":0,\"version\":\"%s\","
               "\"note\":\"NTHFLATN dual · no pure-int leaf at index\"}\n",
               path, needle, idx, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
      } else {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"needle\":\"%s\",\"i\":%ld,"
               "\"file\":%s,\"hit\":true,\"fallback\":false,\"leaf\":\"%s\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"NTHFLATN dual · 0-based Nth pure-int leaf by path needle\"}\n",
               path, needle, idx, (*file_hit) ? "true" : "false", pr.err, pr.n,
               CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* typeflat <needle> — TYPEFLAT dual: first matching leaf kind (read-only).
     *   cubalc plate typeflat agent.json port
     * kind = missing|num|str|bool|null|obj|arr · n = kind code 0..6 · leaf = path. */
    if (strcmp(op, "typeflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char lesc[512];
      size_t i, o = 0;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"typeflat\",\"path\":\"%s\",\"err\":\"need needle\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_type(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"typeflat\",\"path\":\"%s\",\"err\":\"typeflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.err[i] && o + 2 < sizeof lesc; i++) {
        char c = pr.err[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"typeflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"hit\":%s,\"kind\":\"%s\",\"n\":%ld,\"leaf\":\"%s\","
             "\"version\":\"%s\","
             "\"note\":\"TYPEFLAT dual · first matching leaf kind by path needle\"}\n",
             path, needle, (*file_hit) ? "true" : "false",
             pr.code ? "true" : "false", pr.str, pr.n, lesc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* freqflat [needle] — FREQFLAT dual: value frequency of matching leaves (read-only).
     *   cubalc plate freqflat agent.json role
     *   cubalc plate freqflat agent.json          # all leaves
     * bag lines: val:count · n=uniques · total=matching leaves · plate not written. */
    if (strcmp(op, "freqflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_freq(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"freqflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "freqflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"freqflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"FREQFLAT dual · value frequency of matching leaves (read-only)\"}\n",
             path, needle, (*file_hit) ? "true" : "false",
             pr.n, (long)pr.code, esc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* modeflat [needle] — MODEFLAT dual: dominant matching leaf value (read-only).
     *   cubalc plate modeflat agent.json role
     *   cubalc plate modeflat agent.json          # all leaves
     * value = mode · n = winner count · total = matching leaves · plate not written. */
    if (strcmp(op, "modeflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_mode(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"modeflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "modeflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof vesc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      if (pr.str[0])
        printf("%s\n", pr.str);
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"modeflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"hit\":%s,\"value\":\"%s\",\"n\":%ld,\"total\":%ld,"
             "\"version\":\"%s\","
             "\"note\":\"MODEFLAT dual · dominant matching leaf value (read-only)\"}\n",
             path, needle, (*file_hit) ? "true" : "false",
             pr.str[0] ? "true" : "false", vesc, pr.n, (long)pr.code,
             CUBALC_LANG_VERSION);
      return 0;
    }

    /* pathbyval <value> [OR def] — PATHBYVAL dual: first leaf path by exact value (read-only).
     *   cubalc plate pathbyval agent.json worker
     *   cubalc plate pathbyval agent.json leader OR cfg.role
     * leaf = winning path · value = matched text · hit/fallback flags. */
    if (strcmp(op, "pathbyval") == 0) {
      const char *needle = "";
      const char *fb = NULL;
      cubalc_host_result pr;
      char lesc[512], vesc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;
      int have_fb = 0;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pathbyval\",\"path\":\"%s\",\"err\":\"need value [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[(*ai_io)++];
      if ((*ai_io) < argc && argv[(*ai_io)] &&
          (strcmp(argv[(*ai_io)], "OR") == 0 || strcmp(argv[(*ai_io)], "or") == 0 ||
           strcmp(argv[(*ai_io)], "DEFAULT") == 0 || strcmp(argv[(*ai_io)], "default") == 0)) {
        (*ai_io)++;
        if ((*ai_io) < argc && argv[(*ai_io)]) {
          fb = argv[(*ai_io)++];
          have_fb = 1;
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_path_by_val(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pathbyval\",\"path\":\"%s\",\"err\":\"pathbyval fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      {
        const char *outp = pr.n ? pr.str : (have_fb && fb ? fb : "");
        for (i = 0, o = 0; outp[i] && o + 2 < sizeof lesc; i++) {
          char c = outp[i];
          if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
          else lesc[o++] = c;
        }
        lesc[o] = 0;
        for (i = 0, o = 0; needle[i] && o + 2 < sizeof vesc; i++) {
          char c = needle[i];
          if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
          else vesc[o++] = c;
        }
        vesc[o] = 0;
        if (outp[0])
          printf("%s\n", outp);
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"pathbyval\",\"path\":\"%s\",\"value\":\"%s\","
               "\"file\":%s,\"hit\":%s,\"fallback\":%s,\"leaf\":\"%s\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"PATHBYVAL dual · first leaf path by exact value\"}\n",
               path, vesc, (*file_hit) ? "true" : "false",
               pr.n ? "true" : "false",
               (pr.n == 0 && have_fb) ? "true" : "false",
               lesc, pr.n, CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* uniqflat [needle] — UNIQFLAT dual: unique matching leaf values (read-only).
     *   cubalc plate uniqflat agent.json role
     *   cubalc plate uniqflat agent.json          # all leaves
     * bag lines: unique values · n=uniques · total=matching leaves. */
    if (strcmp(op, "uniqflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_uniq(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"uniqflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "uniqflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"uniqflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"UNIQFLAT dual · unique matching leaf values (read-only)\"}\n",
             path, needle, (*file_hit) ? "true" : "false",
             pr.n, (long)pr.code, esc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* pathsbyval <value> — PATHSBYVAL dual: all leaf paths by exact value (read-only).
     *   cubalc plate pathsbyval agent.json worker
     * bag lines: matching paths · n=count. */
    if (strcmp(op, "pathsbyval") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2], vesc[512];
      size_t i, o = 0;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pathsbyval\",\"path\":\"%s\",\"err\":\"need value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_paths_by_val(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pathsbyval\",\"path\":\"%s\",\"err\":\"pathsbyval fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      for (i = 0, o = 0; needle[i] && o + 2 < sizeof vesc; i++) {
        char c = needle[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"pathsbyval\",\"path\":\"%s\",\"value\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"PATHSBYVAL dual · all leaf paths by exact value (read-only)\"}\n",
             path, vesc, (*file_hit) ? "true" : "false",
             pr.n, esc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* countbyval|hasval <value> — COUNTBYVAL/HASVAL duals (read-only).
     *   cubalc plate countbyval agent.json worker
     *   cubalc plate hasval agent.json leader
     * n = count · hit = presence for hasval. */
    if (strcmp(op, "countbyval") == 0 || strcmp(op, "hasval") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int want_has = (strcmp(op, "hasval") == 0);

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need value\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_count_by_val(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s fail\","
               "\"version\":\"%s\"}\n", op, path, op, CUBALC_LANG_VERSION);
        return 1;
      }
      if (want_has) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"hasval\",\"path\":\"%s\",\"value\":\"%s\","
               "\"file\":%s,\"hit\":%s,\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"HASVAL dual · soft presence of exact leaf value\"}\n",
               path, needle, (*file_hit) ? "true" : "false",
               pr.code ? "true" : "false", pr.n, CUBALC_LANG_VERSION);
      } else {
        printf("%ld\n", pr.n);
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"countbyval\",\"path\":\"%s\",\"value\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"COUNTBYVAL dual · count leaves by exact value\"}\n",
               path, needle, (*file_hit) ? "true" : "false",
               pr.n, CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* setbyval <old> <new> — SETBYVAL dual: rewrite leaf values by exact match.
     *   cubalc plate setbyval agent.json worker standby
     * Writes plate. n = rewritten leaf count. */
    if (strcmp(op, "setbyval") == 0) {
      const char *oldv = "", *newv = NULL;
      cubalc_host_result pr, wr;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setbyval\",\"path\":\"%s\",\"err\":\"need old new\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      oldv = argv[(*ai_io)++];
      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setbyval\",\"path\":\"%s\",\"err\":\"need new value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      newv = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_set_by_val(plate, oldv, newv, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setbyval\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "setbyval fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", pr.str);
      if ((*file_hit) || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"setbyval\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        (*file_hit) = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"setbyval\",\"path\":\"%s\",\"old\":\"%s\",\"new\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"SETBYVAL dual · rewrite leaf values by exact match\"}\n",
             path, oldv, newv, (*file_hit) ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* delbyval <value> — DELBYVAL dual: drop leaves by exact value (write-back).
     *   cubalc plate delbyval agent.json standby
     * Writes plate. n = deleted leaf count · kept = remaining. */
    if (strcmp(op, "delbyval") == 0) {
      const char *needle = "";
      cubalc_host_result pr, wr;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"delbyval\",\"path\":\"%s\",\"err\":\"need value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_del_by_val(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"delbyval\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "delbyval fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", pr.str);
      if ((*file_hit) || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"delbyval\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        (*file_hit) = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"delbyval\",\"path\":\"%s\",\"value\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"kept\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"DELBYVAL dual · drop leaves by exact value\"}\n",
             path, needle, (*file_hit) ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* alleqflat [needle] — ALLEQFLAT dual: all matching leaf values identical (read-only).
     *   cubalc plate alleqflat agent.json role
     *   cubalc plate alleqflat agent.json          # all leaves
     * eq=true|false · value=common when eq · n=match count. */
    if (strcmp(op, "alleqflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_all_eq(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"alleqflat\",\"path\":\"%s\",\"err\":\"alleqflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof vesc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      if (pr.code && pr.str[0])
        printf("%s\n", pr.str);
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"alleqflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"eq\":%s,\"value\":\"%s\",\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"ALLEQFLAT dual · all matching leaf values identical\"}\n",
             path, needle, (*file_hit) ? "true" : "false",
             pr.code ? "true" : "false", vesc, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* firstuneq [needle] — FIRSTUNEQFLAT dual: first path that breaks uniformity (read-only).
     *   cubalc plate firstuneq agent.json role
     *   cubalc plate firstuneq agent.json
     * found=true|false · path · ref · value · i · n=match count. */
    if (strcmp(op, "firstuneq") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char pesc[1024], resc[512], vesc[512];
      char ref[256], val[256];
      long total = 0, found = 0, idx = 0;
      size_t i, o = 0;
      const char *ep;

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_first_uneq(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"firstuneq\",\"path\":\"%s\",\"err\":\"firstuneq fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      found = pr.code ? 1 : 0;
      ref[0] = 0;
      val[0] = 0;
      if (found) {
        ep = pr.err;
        {
          size_t k = 0;
          while (*ep && *ep != '\n' && *ep != '\r' && k + 1 < sizeof ref)
            ref[k++] = *ep++;
          ref[k] = 0;
        }
        while (*ep == '\n' || *ep == '\r') ep++;
        {
          size_t k = 0;
          while (*ep && *ep != '\n' && *ep != '\r' && k + 1 < sizeof val)
            val[k++] = *ep++;
          val[k] = 0;
        }
        while (*ep == '\n' || *ep == '\r') ep++;
        total = strtol(ep, NULL, 10);
        idx = pr.n;
        if (pr.str[0])
          printf("%s\n", pr.str);
      } else {
        total = strtol(pr.err, NULL, 10);
        idx = 0;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof pesc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { pesc[o++] = '\\'; pesc[o++] = c; }
        else if (c == '\n') { pesc[o++] = '\\'; pesc[o++] = 'n'; }
        else pesc[o++] = c;
      }
      pesc[o] = 0;
      for (i = 0, o = 0; ref[i] && o + 2 < sizeof resc; i++) {
        char c = ref[i];
        if (c == '"' || c == '\\') { resc[o++] = '\\'; resc[o++] = c; }
        else resc[o++] = c;
      }
      resc[o] = 0;
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"firstuneq\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"found\":%s,\"leaf\":\"%s\",\"ref\":\"%s\","
             "\"value\":\"%s\",\"i\":%ld,\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"FIRSTUNEQFLAT dual · first path that breaks uniformity\"}\n",
             path, needle, (*file_hit) ? "true" : "false",
             found ? "true" : "false", pesc, resc, vesc, idx, total,
             CUBALC_LANG_VERSION);
      return 0;
    }

    /* uneqpaths [needle] — UNEQPATHS dual: all paths that break uniformity (read-only).
     *   cubalc plate uneqpaths agent.json role
     * bag lines: diverge paths · n=diverge count · total=matches · ref=first value. */
    if (strcmp(op, "uneqpaths") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char besc[CUBALC_HOST_STR_MAX * 2], resc[512];
      size_t i, o = 0;

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_uneq_paths(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"uneqpaths\",\"path\":\"%s\",\"err\":\"uneqpaths fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.str[0])
        printf("%s\n", pr.str);
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof besc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { besc[o++] = '\\'; besc[o++] = c; }
        else if (c == '\n') { besc[o++] = '\\'; besc[o++] = 'n'; }
        else besc[o++] = c;
      }
      besc[o] = 0;
      for (i = 0, o = 0; pr.err[i] && o + 2 < sizeof resc; i++) {
        char c = pr.err[i];
        if (c == '"' || c == '\\') { resc[o++] = '\\'; resc[o++] = c; }
        else resc[o++] = c;
      }
      resc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"uneqpaths\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"ref\":\"%s\",\"bag\":\"%s\","
             "\"version\":\"%s\","
             "\"note\":\"UNEQPATHS dual · all paths that break uniformity\"}\n",
             path, needle, (*file_hit) ? "true" : "false",
             pr.n, (long)pr.code, resc, besc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* uniform [needle] — UNIFORMFLAT dual: one-shot eq + first path + all diverge paths.
     *   cubalc plate uniform agent.json role
     * eq · n · div · uniques · path · paths bag · ref · value · common when eq. */
    if (strcmp(op, "uniform") == 0) {
      const char *needle = "";
      cubalc_host_result heq, h1, hpaths;
      char pesc[1024], besc[CUBALC_HOST_STR_MAX * 2], resc[512], vesc[512], cesc[512];
      char ref[256], val[256], path1[512];
      long eq = 0, total = 0, divn = 0, uniques = 0, idx = 0;
      size_t i, o = 0;
      const char *ep;

      if ((*ai_io) < argc && argv[(*ai_io)])
        needle = argv[(*ai_io)++];

      memset(&heq, 0, sizeof heq);
      memset(&h1, 0, sizeof h1);
      memset(&hpaths, 0, sizeof hpaths);
      if (cubalc_host_json_leaf_all_eq(plate, needle, &heq) != 0 ||
          cubalc_host_json_leaf_first_uneq(plate, needle, &h1) != 0 ||
          cubalc_host_json_leaf_uneq_paths(plate, needle, &hpaths) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"uniform\",\"path\":\"%s\",\"err\":\"uniform fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      eq = heq.code ? 1 : 0;
      total = heq.n;
      divn = hpaths.n;
      uniques = (heq.err[0] && heq.err[0] >= '0' && heq.err[0] <= '9')
                    ? strtol(heq.err, NULL, 10)
                    : (eq ? (total > 0 ? 1 : 0) : 0);
      ref[0] = 0;
      val[0] = 0;
      path1[0] = 0;
      if (h1.code) {
        snprintf(path1, sizeof path1, "%s", h1.str);
        idx = h1.n;
        ep = h1.err;
        {
          size_t k = 0;
          while (*ep && *ep != '\n' && *ep != '\r' && k + 1 < sizeof ref)
            ref[k++] = *ep++;
          ref[k] = 0;
        }
        while (*ep == '\n' || *ep == '\r') ep++;
        {
          size_t k = 0;
          while (*ep && *ep != '\n' && *ep != '\r' && k + 1 < sizeof val)
            val[k++] = *ep++;
          val[k] = 0;
        }
      } else if (hpaths.err[0]) {
        snprintf(ref, sizeof ref, "%s", hpaths.err);
      }
      if (eq && heq.str[0])
        printf("%s\n", heq.str);
      else if (path1[0])
        printf("%s\n", path1);
      for (i = 0, o = 0; path1[i] && o + 2 < sizeof pesc; i++) {
        char c = path1[i];
        if (c == '"' || c == '\\') { pesc[o++] = '\\'; pesc[o++] = c; }
        else pesc[o++] = c;
      }
      pesc[o] = 0;
      for (i = 0, o = 0; hpaths.str[i] && o + 2 < sizeof besc; i++) {
        char c = hpaths.str[i];
        if (c == '"' || c == '\\') { besc[o++] = '\\'; besc[o++] = c; }
        else if (c == '\n') { besc[o++] = '\\'; besc[o++] = 'n'; }
        else besc[o++] = c;
      }
      besc[o] = 0;
      for (i = 0, o = 0; ref[i] && o + 2 < sizeof resc; i++) {
        char c = ref[i];
        if (c == '"' || c == '\\') { resc[o++] = '\\'; resc[o++] = c; }
        else resc[o++] = c;
      }
      resc[o] = 0;
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      for (i = 0, o = 0; heq.str[i] && o + 2 < sizeof cesc; i++) {
        char c = heq.str[i];
        if (c == '"' || c == '\\') { cesc[o++] = '\\'; cesc[o++] = c; }
        else cesc[o++] = c;
      }
      cesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"uniform\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"eq\":%s,\"n\":%ld,\"div\":%ld,\"uniques\":%ld,"
             "\"i\":%ld,\"path_first\":\"%s\",\"paths\":\"%s\","
             "\"ref\":\"%s\",\"value\":\"%s\",\"common\":\"%s\","
             "\"version\":\"%s\","
             "\"note\":\"UNIFORMFLAT dual · one-shot nest consistency\"}\n",
             path, needle, (*file_hit) ? "true" : "false",
             eq ? "true" : "false", total, divn, uniques, idx,
             pesc, besc, resc, vesc, cesc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* needflatn <needle> [needle…] — NEEDFLATN dual: fail-fast pure-int leaf contract.
     *   cubalc plate needflatn agent.json port timeout_ms
     * on ok: n = first needle pure-int value · needles = count · leaf = first path
     * exit 2 if any needle has no pure-int leaf match. */
    if (strcmp(op, "needflatn") == 0) {
      char miss_bag[CUBALC_HOST_STR_MAX];
      size_t molen = 0;
      int n_need = 0, n_miss = 0, have_first = 0;
      long first_val = 0;
      char first_path[512];
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2], lesc[1024];
      size_t i, o = 0;

      if ((*ai_io) >= argc || !argv[(*ai_io)]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"needflatn\",\"path\":\"%s\",\"err\":\"need needle [needle…]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      miss_bag[0] = 0;
      first_path[0] = 0;
      while ((*ai_io) < argc && argv[(*ai_io)]) {
        const char *nd = argv[(*ai_io)++];
        size_t mlen;
        n_need++;
        memset(&pr, 0, sizeof pr);
        cubalc_host_json_leaf_getn(plate, nd, &pr);
        if (pr.code == 0) {
          mlen = strlen(nd);
          n_miss++;
          if (molen > 0 && molen + 1 < sizeof miss_bag)
            miss_bag[molen++] = '\n';
          if (molen + mlen < sizeof miss_bag) {
            memcpy(miss_bag + molen, nd, mlen);
            molen += mlen;
            miss_bag[molen] = 0;
          }
        } else if (!have_first) {
          first_val = pr.n;
          snprintf(first_path, sizeof first_path, "%s", pr.err);
          have_first = 1;
        }
      }
      for (i = 0, o = 0; miss_bag[i] && o + 2 < sizeof esc; i++) {
        char c = miss_bag[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      for (i = 0, o = 0; first_path[i] && o + 2 < sizeof lesc; i++) {
        char c = first_path[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      if (n_miss > 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"needflatn\",\"path\":\"%s\",\"file\":%s,"
               "\"needles\":%d,\"miss_n\":%d,\"miss\":\"%s\",\"version\":\"%s\","
               "\"note\":\"NEEDFLATN dual · missing pure-int leaf needle(s)\"}\n",
               path, (*file_hit) ? "true" : "false",
               n_need, n_miss, esc, CUBALC_LANG_VERSION);
        return 2;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"needflatn\",\"path\":\"%s\",\"file\":%s,"
             "\"needles\":%d,\"miss_n\":0,\"n\":%ld,\"leaf\":\"%s\","
             "\"version\":\"%s\","
             "\"note\":\"NEEDFLATN dual · pure-int leaf contract + first peel\"}\n",
             path, (*file_hit) ? "true" : "false", n_need, first_val, lesc,
             CUBALC_LANG_VERSION);
      return 0;
    }

  return -1; /* should not reach — all flat ops return inside */
}
