/* CubalC — CLI plate SYNC plane (fill/ensure/merge/eq/has/pluck).
 * B_cmd_plate_sync peel from cubalc_cmd_plate. Cube is SoT. Zero bloat.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_plate_sync.h"
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

int cubalc_cmd_plate_try_sync(const char *op, const char *path, char *plate,
                              int *file_hit, int argc, char **argv, int *ai_io,
                              int fill_strict) {
  cubalc_host_result hr, wr, gr, keys;
  char parent[512];
  const char *slash;
  const char *key = NULL;
  const char *val = NULL;
  const char *fb = NULL;
  long delta = 1;
  int have_fb = 0;
  const char *body;
  if (!op || !path || !plate || !file_hit || !ai_io) return -1;
  (void)delta; (void)have_fb; (void)fb; (void)key; (void)val;
  memset(&hr, 0, sizeof hr);
  memset(&wr, 0, sizeof wr);
  memset(&gr, 0, sizeof gr);
  memset(&keys, 0, sizeof keys);
  parent[0] = 0;

  if (!(strcmp(op, "fill") == 0 || strcmp(op, "fillkeys") == 0 ||
        strcmp(op, "ensure") == 0 || strcmp(op, "merge") == 0 ||
        strcmp(op, "eq") == 0 || strcmp(op, "ne") == 0 ||
        strcmp(op, "diff") == 0 || strcmp(op, "changelog") == 0 ||
        strcmp(op, "diffflat") == 0 || strcmp(op, "pathdiff") == 0 ||
        strcmp(op, "has") == 0 || strcmp(op, "need") == 0 ||
        strcmp(op, "pluck") == 0))
    return -1;

    /* fill|fillkeys — expand {{key}} from plate file without a .cubalc program.
     * Usability: one-shot agent status/path templates (CLI dual of FILLP/FILLPKEYS).
     *   cubalc plate fill agent.json "peer={{name}}@{{host}}"
     *   cubalc plate fill -s agent.json @tpl.txt out.txt
     *   cubalc plate fillkeys agent.json @tpl.txt
     * Template: literal (contains {), @file, or readable path. */
    if (strcmp(op, "fill") == 0 || strcmp(op, "fillkeys") == 0) {
      const char *tmpl_arg = NULL;
      const char *out_path = NULL;
      char tmpl[CUBALC_HOST_STR_MAX], body[CUBALC_HOST_STR_MAX];
      char miss_keys[CUBALC_HOST_STR_MAX], esc[CUBALC_HOST_STR_MAX];
      char flat[CUBALC_HOST_STR_MAX];
      long hits = 0, miss = 0, nk = 0;
      int is_strict = fill_strict ? 1 : 0;
      int is_keys = (strcmp(op, "fillkeys") == 0) ? 1 : 0;
      size_t i, o;
      cubalc_host_result tr, wrr;

      tmpl[0] = 0; body[0] = 0; miss_keys[0] = 0;
      /* optional -s|--strict before template */
      if ((*ai_io) < argc && argv[(*ai_io)] &&
          (!strcmp(argv[(*ai_io)], "--strict") || !strcmp(argv[(*ai_io)], "-s"))) {
        is_strict = 1;
        (*ai_io)++;
      }
      if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"err\":\"need template string or @file\","
               "\"version\":\"%s\"}\n", op, CUBALC_LANG_VERSION);
        return 2;
      }
      tmpl_arg = argv[(*ai_io)++];
      if ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0] &&
          strcmp(argv[(*ai_io)], "--strict") != 0 && strcmp(argv[(*ai_io)], "-s") != 0)
        out_path = argv[(*ai_io)++];
      if ((*ai_io) < argc && argv[(*ai_io)] &&
          (!strcmp(argv[(*ai_io)], "--strict") || !strcmp(argv[(*ai_io)], "-s"))) {
        is_strict = 1;
        (*ai_io)++;
        if (!out_path && (*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0])
          out_path = argv[(*ai_io)++];
      }

      if (tmpl_arg[0] == '@') {
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(tmpl_arg + 1, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"template read fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(tmpl, sizeof tmpl, "%s", tr.str);
      } else if (strchr(tmpl_arg, '{') == NULL && access(tmpl_arg, R_OK) == 0) {
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(tmpl_arg, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"template read fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(tmpl, sizeof tmpl, "%s", tr.str);
      } else {
        snprintf(tmpl, sizeof tmpl, "%s", tmpl_arg);
      }

      if (is_keys) {
        cubalc_host_fillp_keys(tmpl, body, sizeof body, &nk);
        o = 0; flat[0] = 0;
        for (i = 0; body[i] && o + 2 < sizeof flat; i++) {
          char c = body[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',') flat[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"fillkeys\",\"path\":\"%s\",\"file\":%s,\"n\":%ld,"
               "\"keys\":\"%s\",\"version\":\"%s\"}\n",
               path, (*file_hit) ? "true" : "false", nk, flat, CUBALC_LANG_VERSION);
        return 0;
      }

      cubalc_host_expand_fillp(plate, tmpl, body, sizeof body,
                               &hits, &miss, miss_keys, sizeof miss_keys);

      if (is_strict && miss > 0) {
        o = 0; flat[0] = 0;
        for (i = 0; miss_keys[i] && o + 2 < sizeof flat; i++) {
          char c = miss_keys[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',') {
              flat[o++] = ',';
              if (o + 1 < sizeof flat) flat[o++] = ' ';
            }
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"fill\",\"strict\":true,\"path\":\"%s\",\"hits\":%ld,"
               "\"miss\":%ld,\"miss_keys\":\"%s\","
               "\"err\":\"missing plate keys\",\"version\":\"%s\"}\n",
               path, hits, miss, flat, CUBALC_LANG_VERSION);
        return 1;
      }

      if (out_path && out_path[0]) {
        slash = strrchr(out_path, '/');
        if (slash && slash != out_path) {
          size_t n = (size_t)(slash - out_path);
          if (n >= sizeof parent) n = sizeof parent - 1;
          memcpy(parent, out_path, n);
          parent[n] = 0;
          memset(&hr, 0, sizeof hr);
          cubalc_host_mkdir(parent, &hr);
        }
        memset(&wrr, 0, sizeof wrr);
        if (cubalc_host_write(out_path, body, &wrr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"fill\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"fill\",\"path\":\"%s\",\"out\":\"%s\",\"hits\":%ld,"
               "\"miss\":%ld,\"bytes\":%ld,\"strict\":%s,\"file\":%s,"
               "\"version\":\"%s\"}\n",
               path, out_path, hits, miss, (long)strlen(body),
               is_strict ? "true" : "false",
               (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }

      fputs(body, stdout);
      if (!body[0] || body[strlen(body) - 1] != '\n')
        fputc('\n', stdout);
      o = 0; esc[0] = 0;
      for (i = 0; body[i] && o + 2 < sizeof esc; i++) {
        char c = body[i];
        if (c == '"' || c == '\\') {
          esc[o++] = '\\';
          esc[o++] = c;
        } else if (c == '\n') {
          esc[o++] = '\\';
          esc[o++] = 'n';
        } else if (c == '\r') {
        } else if ((unsigned char)c < 32) {
          esc[o++] = ' ';
        } else {
          esc[o++] = c;
        }
      }
      esc[o] = 0;
      o = 0; flat[0] = 0;
      for (i = 0; miss_keys[i] && o + 2 < sizeof flat; i++) {
        char c = miss_keys[i];
        if (c == '\n' || c == '\r') {
          if (o > 0 && flat[o - 1] != ',') flat[o++] = ',';
        } else if (c == '"' || c == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = c;
        }
      }
      flat[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"fill\",\"path\":\"%s\",\"hits\":%ld,\"miss\":%ld,"
             "\"miss_keys\":\"%s\",\"bytes\":%ld,\"strict\":%s,\"file\":%s,"
             "\"body\":\"%s\",\"version\":\"%s\","
             "\"note\":\"filled body above plate · FILLP dual for CLI agents\"}\n",
             path, hits, miss, flat, (long)strlen(body),
             is_strict ? "true" : "false",
             (*file_hit) ? "true" : "false", esc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* ensure|seed — create object plate only if missing/non-object (ENSUREPLATE dual).
     *   cubalc plate ensure agent.json
     *   cubalc plate ensure agent.json '{"n":0,"ok":true}'
     *   cubalc plate ensure agent.json @seed.json
     * Does not clobber an existing object plate (use set/merge to mutate). */
    if (strcmp(op, "ensure") == 0) {
      const char *seed_arg = NULL;
      char seed[CUBALC_HOST_STR_MAX];
      cubalc_host_result tr;

      seed[0] = 0;
      if ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0])
        seed_arg = argv[(*ai_io)++];
      if (seed_arg) {
        if (seed_arg[0] == '@') {
          memset(&tr, 0, sizeof tr);
          if (cubalc_host_read(seed_arg + 1, &tr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"ensure\",\"err\":\"seed read fail\",\"version\":\"%s\"}\n",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          snprintf(seed, sizeof seed, "%s", tr.str);
        } else if (seed_arg[0] != '{' && access(seed_arg, R_OK) == 0) {
          memset(&tr, 0, sizeof tr);
          if (cubalc_host_read(seed_arg, &tr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"ensure\",\"err\":\"seed read fail\",\"version\":\"%s\"}\n",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          snprintf(seed, sizeof seed, "%s", tr.str);
        } else {
          snprintf(seed, sizeof seed, "%s", seed_arg);
        }
      } else {
        snprintf(seed, sizeof seed, "%s", "{}");
      }
      {
        const char *sb = seed;
        while (*sb == ' ' || *sb == '\t' || *sb == '\n' || *sb == '\r') sb++;
        if (*sb != '{') {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"ensure\",\"err\":\"seed must be JSON object\","
                 "\"version\":\"%s\"}\n", CUBALC_LANG_VERSION);
          return 2;
        }
        if (sb != seed)
          memmove(seed, sb, strlen(sb) + 1);
      }

      if ((*file_hit)) {
        /* keep existing object plate — no write */
        memset(&keys, 0, sizeof keys);
        cubalc_host_json_keys(plate, &keys);
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"ensure\",\"path\":\"%s\",\"created\":false,\"file\":true,"
               "\"keys_n\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
               path, keys.n, (long)strlen(plate), CUBALC_LANG_VERSION, plate);
        return 0;
      }

      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", seed);
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"ensure\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, hr.err[0] ? hr.err : "write fail", CUBALC_LANG_VERSION);
        return 1;
      }
      memset(&keys, 0, sizeof keys);
      cubalc_host_json_keys(plate, &keys);
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"ensure\",\"path\":\"%s\",\"created\":true,\"file\":false,"
             "\"keys_n\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, keys.n, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* merge|patch|overlay — multi-key overlay onto plate file (JSONFILEMERGE dual).
     *   cubalc plate merge agent.json '{"status":"ready","n":3}'
     *   cubalc plate merge agent.json @patch.json
     * Creates {} base if missing; overlay wins on key clash. */
    if (strcmp(op, "merge") == 0) {
      const char *over_arg = NULL;
      char over[CUBALC_HOST_STR_MAX];
      cubalc_host_result tr, mer;

      over[0] = 0;
      if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"merge\",\"path\":\"%s\",\"err\":\"need overlay object or @file\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      over_arg = argv[(*ai_io)++];
      if (over_arg[0] == '@') {
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(over_arg + 1, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"merge\",\"err\":\"overlay read fail\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(over, sizeof over, "%s", tr.str);
      } else if (over_arg[0] != '{' && access(over_arg, R_OK) == 0) {
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(over_arg, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"merge\",\"err\":\"overlay read fail\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(over, sizeof over, "%s", tr.str);
      } else {
        snprintf(over, sizeof over, "%s", over_arg);
      }
      {
        const char *ob = over;
        while (*ob == ' ' || *ob == '\t' || *ob == '\n' || *ob == '\r') ob++;
        if (*ob != '{') {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"merge\",\"err\":\"overlay must be JSON object\","
                 "\"version\":\"%s\"}\n", CUBALC_LANG_VERSION);
          return 2;
        }
      }

      memset(&mer, 0, sizeof mer);
      if (cubalc_host_json_merge(plate, over, &mer) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"merge\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, mer.err[0] ? mer.err : "json merge fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", mer.str);
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"merge\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, hr.err[0] ? hr.err : "write fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"merge\",\"path\":\"%s\",\"applied\":%ld,\"file\":%s,"
             "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, mer.n, (*file_hit) ? "true" : "false", hr.n,
             CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* eq|ne|diff|changelog a.json b.json — compare two agent plates without .cubalc.
     * Usability: mesh/peer sync verify (JSONEQ / JSONCHANGED / JSONCHANGELOG duals).
     * Soft-missing files load as {}. eq exit 0 if equal else 1; ne inverted;
     * diff/changelog exit 0 if identical else 1 (keys/lines listed either way). */
    if (strcmp(op, "eq") == 0 || strcmp(op, "ne") == 0 || strcmp(op, "diff") == 0 ||
        strcmp(op, "changelog") == 0 ||
        strcmp(op, "diffflat") == 0 || strcmp(op, "pathdiff") == 0) {
      const char *path2 = NULL;
      char plate2[CUBALC_HOST_STR_MAX];
      int file_hit2 = 0, equal = 0;
      cubalc_host_result er, ch;
      char flat[CUBALC_HOST_STR_MAX];
      char esc[CUBALC_HOST_STR_MAX];
      size_t i, o;

      plate2[0] = 0;
      if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need second path\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      path2 = argv[(*ai_io)++];

      /* soft-load second plate (mirror path load above) */
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(path2, &hr) == 0 && hr.str[0]) {
        body = hr.str;
        while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
          body++;
        if (*body == '{') {
          memset(&keys, 0, sizeof keys);
          if (cubalc_host_json_keys(body, &keys) == 0) {
            snprintf(plate2, CUBALC_HOST_STR_MAX, "%s", body);
            file_hit2 = 1;
          }
        }
      }
      if (!file_hit2)
        snprintf(plate2, CUBALC_HOST_STR_MAX, "%s", "{}");

      memset(&er, 0, sizeof er);
      cubalc_host_json_eq(plate, plate2, &er);
      equal = (er.n == 1) ? 1 : 0;

      if (strcmp(op, "eq") == 0 || strcmp(op, "ne") == 0) {
        int want_ne = (strcmp(op, "ne") == 0);
        int pass = want_ne ? !equal : equal;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"path2\":\"%s\","
               "\"equal\":%s,\"file\":%s,\"file2\":%s,\"version\":\"%s\"}\n",
               op, path, path2,
               equal ? "true" : "false",
               (*file_hit) ? "true" : "false",
               file_hit2 ? "true" : "false",
               CUBALC_LANG_VERSION);
        return pass ? 0 : 1;
      }

      if (strcmp(op, "diff") == 0) {
        /* diff — changed keys bag (want_same=0) */
        memset(&ch, 0, sizeof ch);
        cubalc_host_json_changed_keys(plate, plate2, 0, &ch);
        o = 0; flat[0] = 0;
        for (i = 0; ch.str[i] && o + 2 < sizeof flat; i++) {
          char c = ch.str[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',') flat[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"diff\",\"path\":\"%s\",\"path2\":\"%s\","
               "\"equal\":%s,\"n\":%ld,\"keys\":\"%s\","
               "\"file\":%s,\"file2\":%s,\"version\":\"%s\"}\n",
               path, path2,
               equal ? "true" : "false", ch.n, flat,
               (*file_hit) ? "true" : "false",
               file_hit2 ? "true" : "false",
               CUBALC_LANG_VERSION);
        return equal ? 0 : 1;
      }

      /* diffflat|pathdiff — deep leaf-path compare (DIFFFLAT / PATHDIFF duals).
       * Nest-aware changelog / path bag above plate. */
      if (strcmp(op, "diffflat") == 0 || strcmp(op, "pathdiff") == 0) {
        int paths_only = (strcmp(op, "pathdiff") == 0) ? 1 : 0;
        memset(&ch, 0, sizeof ch);
        cubalc_host_json_leaf_diff(plate, plate2, paths_only, &ch);
        if (ch.str[0]) {
          fputs(ch.str, stdout);
          if (ch.str[strlen(ch.str) - 1] != '\n')
            fputc('\n', stdout);
        }
        o = 0; esc[0] = 0;
        for (i = 0; ch.str[i] && o + 2 < sizeof esc; i++) {
          char c = ch.str[i];
          if (c == '"' || c == '\\') {
            esc[o++] = '\\';
            esc[o++] = c;
          } else if (c == '\n') {
            esc[o++] = '\\';
            esc[o++] = 'n';
          } else if (c == '\r') {
          } else if ((unsigned char)c < 32) {
            esc[o++] = ' ';
          } else {
            esc[o++] = c;
          }
        }
        esc[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"path2\":\"%s\","
               "\"equal\":%s,\"n\":%ld,\"lines\":\"%s\","
               "\"file\":%s,\"file2\":%s,\"version\":\"%s\","
               "\"note\":\"%s\"}\n",
               op, path, path2,
               (ch.n == 0) ? "true" : "false", ch.n, esc,
               (*file_hit) ? "true" : "false",
               file_hit2 ? "true" : "false",
               CUBALC_LANG_VERSION,
               paths_only
                   ? "PATHDIFF dual · changed leaf paths · bag above plate"
                   : "DIFFFLAT dual · deep leaf path: old → new · bag above plate");
        return (ch.n == 0) ? 0 : 1;
      }

      /* changelog — human/agent "key: old → new" lines (JSONCHANGELOG dual).
       * Prints newline bag above the meta plate for greppable agent logs. */
      memset(&ch, 0, sizeof ch);
      cubalc_host_json_changelog(plate, plate2, &ch);
      if (ch.str[0]) {
        fputs(ch.str, stdout);
        if (ch.str[strlen(ch.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      o = 0; esc[0] = 0;
      for (i = 0; ch.str[i] && o + 2 < sizeof esc; i++) {
        char c = ch.str[i];
        if (c == '"' || c == '\\') {
          esc[o++] = '\\';
          esc[o++] = c;
        } else if (c == '\n') {
          esc[o++] = '\\';
          esc[o++] = 'n';
        } else if (c == '\r') {
        } else if ((unsigned char)c < 32) {
          esc[o++] = ' ';
        } else {
          esc[o++] = c;
        }
      }
      esc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"changelog\",\"path\":\"%s\",\"path2\":\"%s\","
             "\"equal\":%s,\"n\":%ld,\"lines\":\"%s\","
             "\"file\":%s,\"file2\":%s,\"version\":\"%s\","
             "\"note\":\"changelog lines above plate · JSONCHANGELOG dual\"}\n",
             path, path2,
             equal ? "true" : "false", ch.n, esc,
             (*file_hit) ? "true" : "false",
             file_hit2 ? "true" : "false",
             CUBALC_LANG_VERSION);
      return equal ? 0 : 1;
    }

    /* has|need path k1 [k2 …] — multi-key presence contract on a plate file.
     * Usability: shell dual of HASPALL / NEEDP without a .cubalc program.
     * Keys may be dotted nest paths: cubalc plate need agent.json host freq.error
     *   cubalc plate has  agent.json n status role
     *   cubalc plate need agent.json n ok
     * has: ok always true · has_all · miss bag · exit 0 iff all present
     * need: ok false + err when miss (hard gate for agent scripts) */
    if (strcmp(op, "has") == 0 || strcmp(op, "need") == 0) {
      char keys_nl[CUBALC_HOST_STR_MAX];
      char flat_miss[CUBALC_HOST_STR_MAX];
      char flat_req[CUBALC_HOST_STR_MAX];
      cubalc_host_result miss, hasr;
      size_t o = 0, i;
      long nreq = 0;
      int all_ok;

      keys_nl[0] = 0;
      flat_req[0] = 0;
      if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need one or more keys\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      while ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0]) {
        size_t kl = strlen(argv[(*ai_io)]);
        if (o + kl + 2 >= sizeof keys_nl) break;
        if (o > 0) keys_nl[o++] = '\n';
        memcpy(keys_nl + o, argv[(*ai_io)], kl);
        o += kl;
        keys_nl[o] = 0;
        /* flat req for plate meta */
        {
          size_t fo = strlen(flat_req);
          if (fo + kl + 2 < sizeof flat_req) {
            if (fo > 0) {
              flat_req[fo++] = ',';
              flat_req[fo] = 0;
            }
            memcpy(flat_req + fo, argv[(*ai_io)], kl);
            flat_req[fo + kl] = 0;
          }
        }
        nreq++;
        (*ai_io)++;
      }

      memset(&miss, 0, sizeof miss);
      cubalc_host_json_filter_req_keys(plate, keys_nl, 0 /* missing */, &miss);
      memset(&hasr, 0, sizeof hasr);
      cubalc_host_json_has_keys(plate, keys_nl, 1 /* all */, &hasr);
      all_ok = (hasr.n == 1) ? 1 : 0;

      o = 0; flat_miss[0] = 0;
      for (i = 0; miss.str[i] && o + 2 < sizeof flat_miss; i++) {
        char c = miss.str[i];
        if (c == '\n' || c == '\r') {
          if (o > 0 && flat_miss[o - 1] != ',') flat_miss[o++] = ',';
        } else if (c == '"' || c == '\\') {
          flat_miss[o++] = '_';
        } else {
          flat_miss[o++] = c;
        }
      }
      flat_miss[o] = 0;

      if (strcmp(op, "need") == 0 && !all_ok) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"need\",\"path\":\"%s\",\"has_all\":false,"
               "\"n_req\":%ld,\"n_hit\":%ld,\"n_miss\":%ld,\"miss\":\"%s\","
               "\"keys\":\"%s\",\"file\":%s,\"err\":\"missing plate keys\","
               "\"version\":\"%s\"}\n",
               path, nreq, (long)hasr.code, miss.n, flat_miss, flat_req,
               (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
        return 1;
      }

      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"has_all\":%s,"
             "\"n_req\":%ld,\"n_hit\":%ld,\"n_miss\":%ld,\"miss\":\"%s\","
             "\"keys\":\"%s\",\"file\":%s,\"version\":\"%s\"}\n",
             op, path, all_ok ? "true" : "false",
             nreq, (long)hasr.code, miss.n, flat_miss, flat_req,
             (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
      return all_ok ? 0 : 1;
    }

    /* pluck|getall path k1 [k2 …] — multi-key peel → value bag (PLUCKP dual).
     * Usability: agent one-shot extract including dotted nest paths without .cubalc:
     *   cubalc plate pluck agent.json host freq.error n
     * Prints bag lines above plate meta · empty field on miss · hit/miss counts. */
    if (strcmp(op, "pluck") == 0) {
      char keys_nl[CUBALC_HOST_STR_MAX];
      char flat_req[CUBALC_HOST_STR_MAX];
      char bag_esc[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t o = 0, bi, bo;
      long nreq = 0;

      keys_nl[0] = 0;
      flat_req[0] = 0;
      if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pluck\",\"path\":\"%s\",\"err\":\"need one or more keys\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      while ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0]) {
        size_t kl = strlen(argv[(*ai_io)]);
        if (o + kl + 2 >= sizeof keys_nl) break;
        if (o > 0) keys_nl[o++] = '\n';
        memcpy(keys_nl + o, argv[(*ai_io)], kl);
        o += kl;
        keys_nl[o] = 0;
        {
          size_t fo = strlen(flat_req);
          if (fo + kl + 2 < sizeof flat_req) {
            if (fo > 0) {
              flat_req[fo++] = ',';
              flat_req[fo] = 0;
            }
            memcpy(flat_req + fo, argv[(*ai_io)], kl);
            flat_req[fo + kl] = 0;
          }
        }
        nreq++;
        (*ai_io)++;
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_pluck(plate, keys_nl, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pluck\",\"path\":\"%s\",\"err\":\"%s\","
               "\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "pluck fail", CUBALC_LANG_VERSION);
        return 1;
      }

      /* bag lines for greppable agent pipes */
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }

      bag_esc[0] = 0;
      bo = 0;
      for (bi = 0; pr.str[bi] && bo + 2 < sizeof bag_esc; bi++) {
        char c = pr.str[bi];
        if (c == '"' || c == '\\') {
          bag_esc[bo++] = '\\';
          bag_esc[bo++] = c;
        } else if (c == '\n' || c == '\r') {
          bag_esc[bo++] = '\\';
          bag_esc[bo++] = (c == '\n') ? 'n' : 'r';
        } else if ((unsigned char)c < 32) {
          bag_esc[bo++] = ' ';
        } else {
          bag_esc[bo++] = c;
        }
      }
      bag_esc[bo] = 0;

      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"pluck\",\"path\":\"%s\",\"n\":%ld,\"hit\":%ld,\"miss\":%ld,"
             "\"keys\":\"%s\",\"bag\":\"%s\",\"file\":%s,\"version\":\"%s\","
             "\"note\":\"bag lines above plate · PLUCKP dual · paths ok\"}\n",
             path, pr.n, (long)pr.code, pr.n - (long)pr.code, flat_req, bag_esc,
             (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
      return 0;
    }

  return -1;
}
