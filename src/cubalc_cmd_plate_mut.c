/* CubalC — CLI plate MUT core (get/set/inc/del/obj/rename).
 * B_cmd_plate_core peel from cubalc_cmd_plate. Cube is SoT. Zero bloat.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_plate_mut.h"
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

int cubalc_cmd_plate_try_mut(const char *op, const char *path, char *plate,
                             int *file_hit, int argc, char **argv, int *ai_io) {
  cubalc_host_result hr, wr, gr, keys;
  char parent[512];
  const char *slash;
  const char *key = NULL;
  const char *val = NULL;
  const char *fb = NULL;
  long delta = 1;
  int have_fb = 0;
  int val_kind = 0;
  int allnum = 1;
  size_t i;
  if (!op || !path || !plate || !file_hit || !ai_io) return -1;
  (void)argc;
  memset(&hr, 0, sizeof hr);
  memset(&wr, 0, sizeof wr);
  memset(&gr, 0, sizeof gr);
  memset(&keys, 0, sizeof keys);
  parent[0] = 0;

  if (!(strcmp(op, "rename") == 0 || strcmp(op, "copy") == 0 ||
        strcmp(op, "swap") == 0 || strcmp(op, "get") == 0 ||
        strcmp(op, "getn") == 0 || strcmp(op, "getobj") == 0 ||
        strcmp(op, "setobj") == 0 || strcmp(op, "mergeobj") == 0 ||
        strcmp(op, "defaultobj") == 0 || strcmp(op, "type") == 0 ||
        strcmp(op, "set") == 0 || strcmp(op, "default") == 0 ||
        strcmp(op, "toggle") == 0 || strcmp(op, "inc") == 0 ||
        strcmp(op, "del") == 0))
    return -1;

    /* rename|copy|swap path src dst — RENAMEP/COPYP/SWAPP duals (paths ok).
     * Usability: restructure nest fields without a .cubalc program:
     *   cubalc plate rename agent.json cfg.port net.listen
     *   cubalc plate copy   agent.json flags.debug flags.trace
     *   cubalc plate swap   agent.json a.x a.y
     * Soft miss: rename/copy n=0 plate unchanged · swap both-miss n=0. */
    if (strcmp(op, "rename") == 0 || strcmp(op, "copy") == 0 ||
        strcmp(op, "swap") == 0) {
      const char *src = NULL, *dst = NULL;
      cubalc_host_result rr;
      if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need src key\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      src = argv[(*ai_io)++];
      if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need dst key\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      dst = argv[(*ai_io)++];
      memset(&rr, 0, sizeof rr);
      if (strcmp(op, "rename") == 0) {
        if (cubalc_host_json_rename(plate, src, dst, &rr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"rename\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
                 "\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, src, dst, rr.err[0] ? rr.err : "rename fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }
      } else if (strcmp(op, "copy") == 0) {
        if (cubalc_host_json_copy_key(plate, src, dst, &rr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"copy\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
                 "\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, src, dst, rr.err[0] ? rr.err : "copy fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }
      } else {
        if (cubalc_host_json_swap_keys(plate, src, dst, &rr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"swap\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
                 "\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, src, dst, rr.err[0] ? rr.err : "swap fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", rr.str);
      if (rr.n) {
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
                 "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
               "\"n\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
               op, path, src, dst, rr.n, hr.n, CUBALC_LANG_VERSION, plate);
        return 0;
      }
      /* soft miss / no-op write skipped */
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
             "\"n\":0,\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
             op, path, src, dst, (*file_hit) ? "true" : "false",
             CUBALC_LANG_VERSION, plate);
      return 0;
    }

        /* remaining ops need key */
    if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need key\",\"version\":\"%s\"}\n",
             op, path, CUBALC_LANG_VERSION);
      return 2;
    }
    key = argv[(*ai_io)++];

    if (strcmp(op, "get") == 0) {
      if ((*ai_io) < argc && (strcmp(argv[(*ai_io)], "OR") == 0 || strcmp(argv[(*ai_io)], "DEFAULT") == 0 ||
                        strcmp(argv[(*ai_io)], "ELSE") == 0)) {
        (*ai_io)++;
        if ((*ai_io) < argc) {
          fb = argv[(*ai_io)++];
          have_fb = 1;
        }
      }
      memset(&gr, 0, sizeof gr);
      /* dotted/slash path: plate get agent.json freq.error */
      if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
        /* escape value lightly for JSON string field */
        char esc[CUBALC_HOST_STR_MAX];
        size_t i, o = 0;
        for (i = 0; gr.str[i] && o + 2 < sizeof esc; i++) {
          char c = gr.str[i];
          if (c == '"' || c == '\\') {
            esc[o++] = '\\';
            esc[o++] = c;
          } else if ((unsigned char)c < 32) {
            esc[o++] = ' ';
          } else {
            esc[o++] = c;
          }
        }
        esc[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"get\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":true,"
               "\"value\":\"%s\",\"file\":%s,\"version\":\"%s\"}\n",
               path, key, esc, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }
      if (have_fb) {
        char esc[512];
        size_t i, o = 0;
        for (i = 0; fb[i] && o + 2 < sizeof esc; i++) {
          char c = fb[i];
          if (c == '"' || c == '\\') {
            esc[o++] = '\\';
            esc[o++] = c;
          } else {
            esc[o++] = c;
          }
        }
        esc[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"get\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
               "\"or\":true,\"value\":\"%s\",\"file\":%s,\"version\":\"%s\"}\n",
               path, key, esc, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
             "\"op\":\"get\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
             "\"err\":\"key miss\",\"file\":%s,\"version\":\"%s\"}\n",
             path, key, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
      return 1;
    }

    /* getn|num path key [OR n] — GETPN dual: numeric peel (paths ok).
     * Usability: shell dual of GETPN / SYS JSONN for nest counters without .cubalc:
     *   cubalc plate getn agent.json cfg.port
     *   cubalc plate getn agent.json freq.error OR 0
     * n = numeric value · hit true|false · exit 0 on hit or OR, 1 on hard miss. */
    if (strcmp(op, "getn") == 0) {
      long nval = 0, fb_n = 0;
      char *end = NULL;
      if ((*ai_io) < argc && (strcmp(argv[(*ai_io)], "OR") == 0 || strcmp(argv[(*ai_io)], "DEFAULT") == 0 ||
                        strcmp(argv[(*ai_io)], "ELSE") == 0)) {
        (*ai_io)++;
        if ((*ai_io) < argc) {
          fb = argv[(*ai_io)++];
          have_fb = 1;
          fb_n = strtol(fb, &end, 10);
          if (end == fb) fb_n = 0;
        }
      }
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
        end = NULL;
        nval = strtol(gr.str, &end, 10);
        if (end == gr.str) nval = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getn\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":true,"
               "\"n\":%ld,\"value\":\"%s\",\"file\":%s,\"version\":\"%s\","
               "\"note\":\"GETPN dual · LAST_N-style numeric peel · paths ok\"}\n",
               path, key, nval, gr.str, (*file_hit) ? "true" : "false",
               CUBALC_LANG_VERSION);
        return 0;
      }
      if (have_fb) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getn\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
               "\"or\":true,\"n\":%ld,\"value\":\"%ld\",\"file\":%s,"
               "\"version\":\"%s\"}\n",
               path, key, fb_n, fb_n, (*file_hit) ? "true" : "false",
               CUBALC_LANG_VERSION);
        return 0;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
             "\"op\":\"getn\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
             "\"n\":0,\"err\":\"key miss\",\"file\":%s,\"version\":\"%s\"}\n",
             path, key, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
      return 1;
    }

    /* getobj path key [OR '{}'] — GETOBJ dual: peel nested object (paths ok).
     * setobj path key obj|@file — SETOBJ dual: nest object write-back (paths ok).
     * mergeobj|defaultobj path key obj|@file — MERGEOBJ/DEFAULTOBJ duals.
     * Usability: deep nest object peel/patch without a .cubalc program:
     *   cubalc plate getobj agent.json cfg.meta
     *   cubalc plate setobj agent.json net.peer '{"host":"B"}'
     *   cubalc plate mergeobj agent.json cfg.meta '{"zone":"A"}'
     */
    if (strcmp(op, "getobj") == 0 || strcmp(op, "setobj") == 0 ||
        strcmp(op, "mergeobj") == 0 || strcmp(op, "defaultobj") == 0) {
      char objbuf[CUBALC_HOST_STR_MAX];
      char nest[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr, mr;
      const char *v;
      int hit = 0;

      if (strcmp(op, "getobj") == 0) {
        if ((*ai_io) < argc && (strcmp(argv[(*ai_io)], "OR") == 0 || strcmp(argv[(*ai_io)], "DEFAULT") == 0 ||
                          strcmp(argv[(*ai_io)], "ELSE") == 0)) {
          (*ai_io)++;
          if ((*ai_io) < argc) {
            fb = argv[(*ai_io)++];
            have_fb = 1;
          }
        }
        memset(&gr, 0, sizeof gr);
        if (cubalc_host_json_path_get_raw(plate, key, &gr) == 0) {
          v = gr.str;
          while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
          if (*v == '{') {
            hit = 1;
            if (v != gr.str) {
              size_t n = strlen(v);
              memmove(gr.str, v, n + 1);
            }
          }
        }
        if (hit) {
          fputs(gr.str, stdout);
          if (!gr.str[0] || gr.str[strlen(gr.str) - 1] != '\n')
            fputc('\n', stdout);
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"getobj\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":true,"
                 "\"file\":%s,\"version\":\"%s\",\"plate\":%s,"
                 "\"note\":\"GETOBJ dual · object body above plate · paths ok\"}\n",
                 path, key, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION, gr.str);
          return 0;
        }
        if (have_fb) {
          const char *body_out = fb;
          while (*body_out == ' ' || *body_out == '\t') body_out++;
          if (*body_out != '{')
            body_out = "{}";
          fputs(body_out, stdout);
          if (body_out[strlen(body_out) - 1] != '\n')
            fputc('\n', stdout);
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"getobj\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
                 "\"or\":true,\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
                 path, key, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION, body_out);
          return 0;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getobj\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
               "\"err\":\"key miss or not object\",\"file\":%s,\"version\":\"%s\"}\n",
               path, key, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
        return 1;
      }

      /* setobj|mergeobj|defaultobj need object value */
      if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"err\":\"need object value or @file\","
               "\"version\":\"%s\"}\n", op, CUBALC_LANG_VERSION);
        return 2;
      }
      val = argv[(*ai_io)++];
      objbuf[0] = 0;
      if (val[0] == '@') {
        memset(&pr, 0, sizeof pr);
        if (cubalc_host_read(val + 1, &pr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"object read fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(objbuf, sizeof objbuf, "%s", pr.str);
      } else if (strchr(val, '{') == NULL && access(val, R_OK) == 0) {
        memset(&pr, 0, sizeof pr);
        if (cubalc_host_read(val, &pr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"object read fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(objbuf, sizeof objbuf, "%s", pr.str);
      } else {
        snprintf(objbuf, sizeof objbuf, "%s", val);
      }
      v = objbuf;
      while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
      if (*v != '{') {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"err\":\"value not object\",\"version\":\"%s\"}\n",
               op, CUBALC_LANG_VERSION);
        return 1;
      }
      if (v != objbuf) {
        size_t n = strlen(v);
        memmove(objbuf, v, n + 1);
      }

      if (strcmp(op, "setobj") == 0) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, key, objbuf, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"setobj\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 wr.err[0] ? wr.err : "set fail", CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(plate, CUBALC_HOST_STR_MAX, "%s", wr.str);
      } else {
        /* mergeobj / defaultobj: peel nest, merge/defaults, path_set */
        memset(&pr, 0, sizeof pr);
        cubalc_host_json_path_obj(plate, key, &pr);
        snprintf(nest, sizeof nest, "%s", pr.str);
        memset(&mr, 0, sizeof mr);
        if (strcmp(op, "defaultobj") == 0) {
          if (cubalc_host_json_defaults(nest, objbuf, &mr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"defaultobj\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   mr.err[0] ? mr.err : "defaults fail", CUBALC_LANG_VERSION);
            return 1;
          }
        } else {
          if (cubalc_host_json_merge(nest, objbuf, &mr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"mergeobj\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   mr.err[0] ? mr.err : "merge fail", CUBALC_LANG_VERSION);
            return 1;
          }
        }
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, key, mr.str, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 op, wr.err[0] ? wr.err : "set fail", CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(plate, CUBALC_HOST_STR_MAX, "%s", wr.str);
        /* report applied count in wr.n via mr.n after write */
        wr.n = mr.n;
      }

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
               "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               op, CUBALC_LANG_VERSION);
        return 1;
      }
      if (strcmp(op, "setobj") == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"setobj\",\"path\":\"%s\",\"key\":\"%s\",\"bytes\":%ld,"
               "\"file\":%s,\"version\":\"%s\",\"plate\":%s,"
               "\"note\":\"SETOBJ dual · paths ok\"}\n",
               path, key, hr.n, (*file_hit) ? "true" : "false",
               CUBALC_LANG_VERSION, plate);
      } else {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"key\":\"%s\",\"n\":%ld,"
               "\"bytes\":%ld,\"file\":%s,\"version\":\"%s\",\"plate\":%s,"
               "\"note\":\"MERGEOBJ/DEFAULTOBJ dual · paths ok\"}\n",
               op, path, key, wr.n, hr.n, (*file_hit) ? "true" : "false",
               CUBALC_LANG_VERSION, plate);
      }
      return 0;
    }

    /* type|kind path key — TYPEP dual: field kind probe (paths ok).
     * Usability: choose num/str/obj peel without a .cubalc program:
     *   cubalc plate type agent.json freq.error  → num
     *   cubalc plate kind agent.json meta        → obj
     * Soft miss → kind=missing · hit=false · exit 1. */
    if (strcmp(op, "type") == 0) {
      const char *kind_s = "missing";
      long kind_n = 0;
      int hit = 0;
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get_raw(plate, key, &gr) == 0) {
        const char *v = gr.str;
        hit = 1;
        while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
        if (*v == '"') {
          kind_s = "str";
          kind_n = 2;
        } else if (*v == '{') {
          kind_s = "obj";
          kind_n = 5;
        } else if (*v == '[') {
          kind_s = "arr";
          kind_n = 6;
        } else if (strncmp(v, "true", 4) == 0 || strncmp(v, "false", 5) == 0) {
          kind_s = "bool";
          kind_n = 3;
        } else if (strncmp(v, "null", 4) == 0) {
          kind_s = "null";
          kind_n = 4;
        } else if (*v == '-' || (*v >= '0' && *v <= '9')) {
          kind_s = "num";
          kind_n = 1;
        } else {
          kind_s = "str";
          kind_n = 2;
        }
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"type\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":%s,"
             "\"kind\":\"%s\",\"n\":%ld,\"file\":%s,\"version\":\"%s\","
             "\"note\":\"TYPEP dual · missing|num|str|bool|null|obj|arr · paths ok\"}\n",
             path, key, hit ? "true" : "false", kind_s, kind_n,
             (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
      return hit ? 0 : 1;
    }

    if (strcmp(op, "set") == 0) {
      size_t i;
      int allnum = 1;
      if ((*ai_io) >= argc) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"set\",\"err\":\"need value\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 2;
      }
      val = argv[(*ai_io)++];
      /* raw if integer / true / false / null / starts with { or [ */
      if (!strcmp(val, "true") || !strcmp(val, "false") || !strcmp(val, "null") ||
          val[0] == '{' || val[0] == '[')
        val_kind = 1;
      else {
        if (val[0] == '-' && val[1]) i = 1;
        else i = 0;
        if (!val[i]) allnum = 0;
        for (; val[i]; i++) {
          if (val[i] < '0' || val[i] > '9') {
            allnum = 0;
            break;
          }
        }
        if (allnum) val_kind = 1;
      }
      memset(&wr, 0, sizeof wr);
      /* dotted/slash path: plate set agent.json freq.error 9 */
      if (cubalc_host_json_path_set(plate, key, val, val_kind, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"set\",\"path\":\"%s\",\"key\":\"%s\",\"err\":\"%s\","
               "\"version\":\"%s\"}\n",
               path, key, wr.err[0] ? wr.err : "json set fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", wr.str);
      /* mkdir parent + write */
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
               "\"op\":\"set\",\"path\":\"%s\",\"key\":\"%s\",\"err\":\"%s\","
               "\"version\":\"%s\"}\n",
               path, key, hr.err[0] ? hr.err : "write fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"set\",\"path\":\"%s\",\"key\":\"%s\",\"updated\":%ld,"
             "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, wr.n, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* default path key value — DEFAULTP dual: set-if-missing (paths ok).
     *   cubalc plate default agent.json cfg.port 8080
     * kept=true when key already present (no clobber). */
    if (strcmp(op, "default") == 0) {
      size_t i;
      int allnum = 1, present = 0;
      cubalc_host_result hasr;
      if ((*ai_io) >= argc) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"default\",\"err\":\"need value\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 2;
      }
      val = argv[(*ai_io)++];
      memset(&hasr, 0, sizeof hasr);
      present = (cubalc_host_json_path_has(plate, key, &hasr) == 0 && hasr.n) ? 1 : 0;
      if (present) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"default\",\"path\":\"%s\",\"key\":\"%s\",\"kept\":true,"
               "\"inserted\":false,\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
               path, key, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION, plate);
        return 0;
      }
      if (!strcmp(val, "true") || !strcmp(val, "false") || !strcmp(val, "null") ||
          val[0] == '{' || val[0] == '[')
        val_kind = 1;
      else {
        if (val[0] == '-' && val[1]) i = 1;
        else i = 0;
        if (!val[i]) allnum = 0;
        for (; val[i]; i++) {
          if (val[i] < '0' || val[i] > '9') {
            allnum = 0;
            break;
          }
        }
        if (allnum) val_kind = 1;
      }
      memset(&wr, 0, sizeof wr);
      if (cubalc_host_json_path_set(plate, key, val, val_kind, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"default\",\"path\":\"%s\",\"key\":\"%s\",\"err\":\"%s\","
               "\"version\":\"%s\"}\n",
               path, key, wr.err[0] ? wr.err : "json set fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", wr.str);
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
               "\"op\":\"default\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"default\",\"path\":\"%s\",\"key\":\"%s\",\"kept\":false,"
             "\"inserted\":true,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* toggle path key — TOGGLEP dual: flip 0↔1 flag (paths ok; miss→1).
     *   cubalc plate toggle agent.json flags.debug */
    if (strcmp(op, "toggle") == 0) {
      long cur = 0, nv;
      char raw[32];
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
        char *end = NULL;
        if (!gr.str[0]) cur = 0;
        else {
          cur = strtol(gr.str, &end, 10);
          if (end == gr.str) cur = 1; /* non-numeric non-empty → truthy */
        }
      } else {
        cur = 0;
      }
      nv = cur ? 0 : 1;
      snprintf(raw, sizeof raw, "%ld", nv);
      memset(&wr, 0, sizeof wr);
      if (cubalc_host_json_path_set(plate, key, raw, 1, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"toggle\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               wr.err[0] ? wr.err : "json set fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", wr.str);
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
               "\"op\":\"toggle\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"toggle\",\"path\":\"%s\",\"key\":\"%s\",\"value\":%ld,"
             "\"prev\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, nv, cur ? 1L : 0L, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    if (strcmp(op, "inc") == 0) {
      long cur = 0, nv;
      char raw[32];
      if ((*ai_io) < argc) {
        char *end = NULL;
        delta = strtol(argv[(*ai_io)], &end, 10);
        if (end == argv[(*ai_io)]) delta = 1;
        (*ai_io)++;
      }
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
        char *end = NULL;
        cur = strtol(gr.str, &end, 10);
        if (end == gr.str) cur = 0;
      }
      nv = cur + delta;
      snprintf(raw, sizeof raw, "%ld", nv);
      memset(&wr, 0, sizeof wr);
      if (cubalc_host_json_path_set(plate, key, raw, 1, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"inc\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               wr.err[0] ? wr.err : "json set fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", wr.str);
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
               "\"op\":\"inc\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"inc\",\"path\":\"%s\",\"key\":\"%s\",\"value\":%ld,"
             "\"delta\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, nv, delta, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    if (strcmp(op, "del") == 0) {
      memset(&wr, 0, sizeof wr);
      if (cubalc_host_json_path_del(plate, key, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"del\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               wr.err[0] ? wr.err : "json del fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, CUBALC_HOST_STR_MAX, "%s", wr.str);
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
               "\"op\":\"del\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"del\",\"path\":\"%s\",\"key\":\"%s\",\"removed\":%ld,"
             "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, wr.n, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

  return -1;
}
