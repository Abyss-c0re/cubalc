/* CubalC — CLI plate VIEW plane (show/pretty/keys/pick/sum/top).
 * B_cmd_plate_view peel from cubalc_cmd_plate. Cube is SoT. Zero bloat.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_plate_view.h"
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

int cubalc_cmd_plate_try_view(const char *op, const char *path, char *plate,
                              int *file_hit, int argc, char **argv, int *ai_io) {
  cubalc_host_result hr, keys, wr;
  if (!op || !path || !plate || !file_hit || !ai_io) return -1;
  (void)argc;
  memset(&hr, 0, sizeof hr);
  memset(&keys, 0, sizeof keys);
  memset(&wr, 0, sizeof wr);

  if (!(strcmp(op, "show") == 0 || strcmp(op, "pretty") == 0 ||
        strcmp(op, "keys") == 0 || strcmp(op, "len") == 0 ||
        strcmp(op, "empty") == 0 || strcmp(op, "vals") == 0 ||
        strcmp(op, "pick") == 0 || strcmp(op, "omit") == 0 ||
        strcmp(op, "sum") == 0 || strcmp(op, "avg") == 0 ||
        strcmp(op, "median") == 0 || strcmp(op, "top") == 0 ||
        strcmp(op, "bot") == 0))
    return -1;


    if (strcmp(op, "show") == 0) {
      memset(&keys, 0, sizeof keys);
      cubalc_host_json_keys(plate, &keys);
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"show\",\"path\":\"%s\",\"file\":%s,\"keys_n\":%ld,"
             "\"version\":\"%s\",\"plate\":%s}\n",
             path, (*file_hit) ? "true" : "false", keys.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* pretty — PRETTYP dual: 2-space indented plate text (read-only, no jq).
     *   cubalc plate pretty agent.json
     * prints pretty body then JSON meta with n=bytes. */
    if (strcmp(op, "pretty") == 0) {
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_pretty(plate, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pretty\",\"path\":\"%s\",\"err\":\"pretty fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.str[0])
        printf("%s\n", pr.str);
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else if (c == '\r') { esc[o++] = '\\'; esc[o++] = 'r'; }
        else if (c == '\t') { esc[o++] = '\\'; esc[o++] = 't'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"pretty\",\"path\":\"%s\",\"file\":%s,\"n\":%ld,"
             "\"truncated\":%s,\"pretty\":\"%s\",\"version\":\"%s\","
             "\"note\":\"PRETTYP dual · 2-space indented plate (no jq)\"}\n",
             path, (*file_hit) ? "true" : "false", pr.n,
             pr.code ? "false" : "true", esc, CUBALC_LANG_VERSION);
      return 0;
    }
    /* keys|len|empty|vals [nest.path] — KEYSP/LENP/EMPTYP/VALSP duals (path ok).
     *   cubalc plate keys agent.json
     *   cubalc plate keys agent.json cfg.flags
     *   cubalc plate len  agent.json cfg
     *   cubalc plate empty agent.json cfg.flags
     *   cubalc plate vals agent.json cfg */
    if (strcmp(op, "keys") == 0 || strcmp(op, "len") == 0 ||
        strcmp(op, "empty") == 0 || strcmp(op, "vals") == 0) {
      const char *nestp = NULL;
      char sub[CUBALC_HOST_STR_MAX];
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result obj, pr;
      size_t i, o = 0;
      long nkeys = 0;
      int isempty;

      if ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0])
        nestp = argv[(*ai_io)++];

      memset(&obj, 0, sizeof obj);
      cubalc_host_json_path_obj(plate, nestp, &obj);
      snprintf(sub, sizeof sub, "%s", obj.str);

      if (strcmp(op, "keys") == 0) {
        memset(&keys, 0, sizeof keys);
        if (cubalc_host_json_keys(sub, &keys) != 0) {
          keys.str[0] = 0;
          keys.n = 0;
        }
        flat[0] = 0;
        for (i = 0; keys.str[i] && o + 2 < sizeof flat; i++) {
          char c = keys.str[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',')
              flat[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        if (keys.str[0]) {
          fputs(keys.str, stdout);
          if (keys.str[strlen(keys.str) - 1] != '\n')
            fputc('\n', stdout);
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"keys\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
               "\"n\":%ld,\"keys\":\"%s\",\"version\":\"%s\","
               "\"note\":\"KEYSP dual · nest path ok · bag lines above plate\"}\n",
               path, nestp ? nestp : "", (*file_hit) ? "true" : "false",
               keys.n, flat, CUBALC_LANG_VERSION);
        return 0;
      }

      if (strcmp(op, "vals") == 0) {
        memset(&pr, 0, sizeof pr);
        if (cubalc_host_json_values(sub, &pr) != 0) {
          pr.str[0] = 0;
          pr.n = 0;
        }
        flat[0] = 0;
        for (i = 0; pr.str[i] && o + 2 < sizeof flat; i++) {
          char c = pr.str[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',')
              flat[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        if (pr.str[0]) {
          fputs(pr.str, stdout);
          if (pr.str[strlen(pr.str) - 1] != '\n')
            fputc('\n', stdout);
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"vals\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
               "\"n\":%ld,\"vals\":\"%s\",\"version\":\"%s\","
               "\"note\":\"VALSP dual · nest path ok · bag lines above plate\"}\n",
               path, nestp ? nestp : "", (*file_hit) ? "true" : "false",
               pr.n, flat, CUBALC_LANG_VERSION);
        return 0;
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_len(sub, &pr) != 0)
        nkeys = 0;
      else
        nkeys = pr.n;
      isempty = (nkeys == 0) ? 1 : 0;

      if (strcmp(op, "len") == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"len\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"LENP dual · nest path ok\"}\n",
               path, nestp ? nestp : "", (*file_hit) ? "true" : "false",
               nkeys, CUBALC_LANG_VERSION);
        return 0;
      }

      /* empty — always exit 0; agents read empty bool (EMPTYP dual) */
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"empty\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
             "\"empty\":%s,\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"EMPTYP dual · nest path ok\"}\n",
             path, nestp ? nestp : "", (*file_hit) ? "true" : "false",
             isempty ? "true" : "false", nkeys, CUBALC_LANG_VERSION);
      return 0;
    }
    /* pick|omit path k1 [k2 …] — project plate keys without a .cubalc program.
     * Usability: shell dual of PICKP / OMITP for peer/log slim plates.
     *   cubalc plate pick agent.json host status n
     *   cubalc plate omit agent.json tmp debug
     * Soft-missing keys skipped. Does not write file (stdout plate only). */
    if (strcmp(op, "pick") == 0 || strcmp(op, "omit") == 0) {
      char keys_nl[CUBALC_HOST_STR_MAX];
      char flat_req[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t o = 0;
      long nreq = 0;
      int is_omit = (strcmp(op, "omit") == 0) ? 1 : 0;

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
      if (is_omit) {
        if (cubalc_host_json_drop(plate, keys_nl, &pr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"omit\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, pr.err[0] ? pr.err : "json drop fail", CUBALC_LANG_VERSION);
          return 1;
        }
      } else {
        if (cubalc_host_json_pick(plate, keys_nl, &pr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"pick\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, pr.err[0] ? pr.err : "json pick fail", CUBALC_LANG_VERSION);
          return 1;
        }
      }

      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"n\":%ld,\"n_req\":%ld,"
             "\"keys\":\"%s\",\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
             op, path, pr.n, nreq, flat_req,
             (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION, pr.str);
      return 0;
    }

    /* sum|avg|median path — numeric plate aggregates without a .cubalc program.
     * Usability: CLI duals of SUMNP / AVGNP / MEDIANP for agent one-shots.
     *   cubalc plate sum scores.json
     *   cubalc plate avg scores.json
     *   cubalc plate median|p50 scores.json
     * Soft 0 if no pure-int fields. Does not write file. */
    if (strcmp(op, "sum") == 0 || strcmp(op, "avg") == 0 ||
        strcmp(op, "median") == 0) {
      cubalc_host_result ar;
      long used = 0;
      memset(&ar, 0, sizeof ar);
      if (strcmp(op, "sum") == 0) {
        if (cubalc_host_json_sum(plate, &ar) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"sum\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, ar.err[0] ? ar.err : "json sum fail", CUBALC_LANG_VERSION);
          return 1;
        }
      } else if (strcmp(op, "avg") == 0) {
        if (cubalc_host_json_avg(plate, &ar) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"avg\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, ar.err[0] ? ar.err : "json avg fail", CUBALC_LANG_VERSION);
          return 1;
        }
        used = (long)ar.code;
      } else {
        if (cubalc_host_json_median(plate, &ar) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"median\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, ar.err[0] ? ar.err : "json median fail", CUBALC_LANG_VERSION);
          return 1;
        }
        used = (long)ar.code;
      }
      if (strcmp(op, "sum") == 0)
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"sum\",\"path\":\"%s\",\"n\":%ld,\"file\":%s,"
               "\"version\":\"%s\"}\n",
               path, ar.n, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
      else
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"n\":%ld,\"used\":%ld,\"file\":%s,"
               "\"version\":\"%s\"}\n",
               op, path, ar.n, used, (*file_hit) ? "true" : "false",
               CUBALC_LANG_VERSION);
      return 0;
    }

    /* top|bot path [n] — top/bottom N pure-int keys by value → plate.
     * Usability: CLI dual of TOPNP/BOTNP · default n=1.
     *   cubalc plate top freq.json 3
     *   cubalc plate bot scores.json
     * Soft empty plate if no ints. Does not write file. */
    if (strcmp(op, "top") == 0 || strcmp(op, "bot") == 0) {
      cubalc_host_result tr;
      cubalc_host_result tk;
      long ntake = 1;
      int want_bot = (strcmp(op, "bot") == 0) ? 1 : 0;
      char key1[256];
      long val1 = 0;
      if ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0]) {
        char *end = NULL;
        long v = strtol(argv[(*ai_io)], &end, 10);
        if (end && end != argv[(*ai_io)] && *end == 0)
          ntake = v;
      }
      if (ntake < 0) ntake = 0;
      memset(&tr, 0, sizeof tr);
      if (cubalc_host_json_topn(plate, ntake, want_bot, &tr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, tr.err[0] ? tr.err : "json topn fail", CUBALC_LANG_VERSION);
        return 1;
      }
      /* peel first key for agent convenience when n>=1 */
      key1[0] = 0;
      memset(&tk, 0, sizeof tk);
      if (tr.n > 0 && cubalc_host_json_keys(tr.str, &tk) == 0 && tk.str[0]) {
        size_t kn = 0;
        const char *kp = tk.str;
        while (*kp && *kp != '\n' && *kp != '\r' && kn + 1 < sizeof key1)
          key1[kn++] = *kp++;
        key1[kn] = 0;
        if (key1[0]) {
          cubalc_host_result gr2;
          char *end = NULL;
          memset(&gr2, 0, sizeof gr2);
          if (cubalc_host_json_get(tr.str, key1, &gr2) == 0 && gr2.str[0]) {
            val1 = strtol(gr2.str, &end, 10);
            if (!(end && end != gr2.str && *end == 0))
              val1 = 0;
          }
        }
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"n\":%ld,\"cand\":%d,\"take\":%ld,"
             "\"key\":\"%s\",\"value\":%ld,\"file\":%s,\"version\":\"%s\","
             "\"plate\":%s}\n",
             op, path, tr.n, tr.code, ntake, key1, val1,
             (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION, tr.str);
      return 0;
    }

  return -1;
}
