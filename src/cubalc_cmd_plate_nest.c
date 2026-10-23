/* CubalC — CLI plate NEST op plane (peeled from cubalc_cmd_plate).
 * B_cmd_plate_nest · nestget…pluckobj. Cube is SoT. Zero bloat.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_plate_nest.h"
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

int cubalc_cmd_plate_try_nest(const char *op, const char *path, char *plate,
                              int *file_hit, int argc, char **argv, int *ai_io) {
  cubalc_host_result hr;
  char parent[512];
  const char *slash;
  if (!op || !path || !plate || !file_hit || !ai_io) return -1;
  (void)argc;
  memset(&hr, 0, sizeof hr);
  parent[0] = 0;

  if (!(strcmp(op, "nestget") == 0 || strcmp(op, "nestset") == 0 ||
        strcmp(op, "nestinc") == 0 || strcmp(op, "nestdel") == 0 ||
        strcmp(op, "nestkeys") == 0 || strcmp(op, "nesthas") == 0 ||
        strcmp(op, "nestpick") == 0 || strcmp(op, "nestomit") == 0 ||
        strcmp(op, "nestrename") == 0 || strcmp(op, "nestcopy") == 0 ||
        strcmp(op, "nestswap") == 0 || strcmp(op, "pluckobj") == 0 ||
        strcmp(op, "nestsum") == 0 || strcmp(op, "nestavg") == 0 ||
        strcmp(op, "nestmedian") == 0 || strcmp(op, "nesttop") == 0 ||
        strcmp(op, "nestbot") == 0 || strcmp(op, "nestsort") == 0 ||
        strcmp(op, "nestsortbag") == 0))
    return -1;

    /* nestget|nestset|nestinc|nestdel|nestkeys|nesthas — nested field CLI duals of
     * GETPOBJ/SETPOBJ/INCOBJ/DELPOBJ/KEYSOBJ/HASPOBJ. Agents one-shot nested config
     * without writing a .cubalc program.
     * Nest key may be dotted path (cfg.flags) via path_obj peel.
     * nestsum|nestavg|nestmedian|nesttop|nestbot — SUMNOBJ/TOPNOBJ CLI duals (no write).
     * nestsort|nestsortbag — SORTOBJ/SORTBAGOBJ CLI duals (sort write / bag no-write).
     * nestpick|nestomit — PICKOBJ/OMITOBJ duals (paths ok).
     * pluckobj|nestpluck — PLUCKOBJ dual: multi-key peel nest → value bag (paths ok). */
    if (strcmp(op, "nestget") == 0 || strcmp(op, "nestset") == 0 ||
        strcmp(op, "nestinc") == 0 || strcmp(op, "nestdel") == 0 ||
        strcmp(op, "nestkeys") == 0 || strcmp(op, "nesthas") == 0 ||
        strcmp(op, "nestpick") == 0 || strcmp(op, "nestomit") == 0 ||
        strcmp(op, "nestrename") == 0 || strcmp(op, "nestcopy") == 0 ||
        strcmp(op, "nestswap") == 0 ||
        strcmp(op, "pluckobj") == 0 ||
        strcmp(op, "nestsum") == 0 || strcmp(op, "nestavg") == 0 ||
        strcmp(op, "nestmedian") == 0 || strcmp(op, "nesttop") == 0 ||
        strcmp(op, "nestbot") == 0 ||
        strcmp(op, "nestsort") == 0 || strcmp(op, "nestsortbag") == 0) {
      const char *nestk = NULL, *field = NULL;
      char nest[CUBALC_HOST_STR_MAX];
      cubalc_host_result ngr, gr, wr, kr;
      int nest_hit = 0;

      if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need nest key\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      nestk = argv[(*ai_io)++];

      /* path-aware nest peel: "cfg.flags" · soft miss → {} */
      nest[0] = 0;
      memset(&ngr, 0, sizeof ngr);
      cubalc_host_json_path_obj(plate, nestk, &ngr);
      snprintf(nest, sizeof nest, "%s", ngr.str);
      nest_hit = (ngr.n != 0) ? 1 : 0;

      /* nestsum|nestavg|nestmedian — SUMNOBJ/AVGNOBJ/MEDIANOBJ duals (no write). */
      if (strcmp(op, "nestsum") == 0 || strcmp(op, "nestavg") == 0 ||
          strcmp(op, "nestmedian") == 0) {
        cubalc_host_result ar;
        long used = 0;
        memset(&ar, 0, sizeof ar);
        if (!nest_hit) {
          if (strcmp(op, "nestsum") == 0)
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                   "\"op\":\"nestsum\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,"
                   "\"nest_hit\":false,\"file\":%s,\"version\":\"%s\"}\n",
                   path, nestk, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
          else
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                   "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,\"used\":0,"
                   "\"nest_hit\":false,\"file\":%s,\"version\":\"%s\"}\n",
                   op, path, nestk, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
          return 0;
        }
        if (strcmp(op, "nestsum") == 0) {
          if (cubalc_host_json_sum(nest, &ar) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestsum\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, ar.err[0] ? ar.err : "json sum fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestsum\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
                 "\"nest_hit\":true,\"file\":%s,\"version\":\"%s\"}\n",
                 path, nestk, ar.n, (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
          return 0;
        }
        if (strcmp(op, "nestavg") == 0) {
          if (cubalc_host_json_avg(nest, &ar) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestavg\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, ar.err[0] ? ar.err : "json avg fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          used = (long)ar.code;
        } else {
          if (cubalc_host_json_median(nest, &ar) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestmedian\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, ar.err[0] ? ar.err : "json median fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          used = (long)ar.code;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,\"used\":%ld,"
               "\"nest_hit\":true,\"file\":%s,\"version\":\"%s\"}\n",
               op, path, nestk, ar.n, used, (*file_hit) ? "true" : "false",
               CUBALC_LANG_VERSION);
        return 0;
      }

      /* nesttop|nestbot [n] — TOPNOBJ/BOTNOBJ duals (read-only; no file write). */
      if (strcmp(op, "nesttop") == 0 || strcmp(op, "nestbot") == 0) {
        cubalc_host_result tr, tk;
        long ntake = 1;
        int want_bot = (strcmp(op, "nestbot") == 0) ? 1 : 0;
        char key1[256];
        long val1 = 0;
        if ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0]) {
          char *end = NULL;
          long vv = strtol(argv[(*ai_io)], &end, 10);
          if (end && end != argv[(*ai_io)] && *end == 0)
            ntake = vv;
        }
        if (ntake < 0) ntake = 0;
        if (!nest_hit) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,\"cand\":0,"
                 "\"take\":%ld,\"key\":\"\",\"value\":0,\"nest_hit\":false,"
                 "\"file\":%s,\"version\":\"%s\",\"plate\":{}}\n",
                 op, path, nestk, ntake, (*file_hit) ? "true" : "false",
                 CUBALC_LANG_VERSION);
          return 0;
        }
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_json_topn(nest, ntake, want_bot, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                 "\"version\":\"%s\"}\n",
                 op, path, nestk, tr.err[0] ? tr.err : "json topn fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }
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
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,\"cand\":%d,"
               "\"take\":%ld,\"key\":\"%s\",\"value\":%ld,\"nest_hit\":true,"
               "\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
               op, path, nestk, tr.n, tr.code, ntake, key1, val1,
               (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION, tr.str);
        return 0;
      }

      /* nestsort [ASC|DESC] — SORTOBJ dual: sort pure-int nest keys, write-back file.
       * nestsortbag [ASC|DESC] — SORTBAGOBJ dual: key:val bag, no file write.
       * Usability: agent nest FREQ full rank without a .cubalc program. */
      if (strcmp(op, "nestsort") == 0 || strcmp(op, "nestsortbag") == 0) {
        cubalc_host_result sr;
        int want_asc = 0;
        int as_bag = (strcmp(op, "nestsortbag") == 0) ? 1 : 0;
        char bag_esc[CUBALC_HOST_STR_MAX];
        size_t bi, bo;

        while ((*ai_io) < argc && argv[(*ai_io)] && argv[(*ai_io)][0]) {
          if (strcmp(argv[(*ai_io)], "ASC") == 0 || strcmp(argv[(*ai_io)], "asc") == 0 ||
              strcmp(argv[(*ai_io)], "UP") == 0 || strcmp(argv[(*ai_io)], "up") == 0 ||
              strcmp(argv[(*ai_io)], "LIGHT") == 0 || strcmp(argv[(*ai_io)], "light") == 0 ||
              strcmp(argv[(*ai_io)], "LOW") == 0 || strcmp(argv[(*ai_io)], "low") == 0) {
            want_asc = 1;
            (*ai_io)++;
            continue;
          }
          if (strcmp(argv[(*ai_io)], "DESC") == 0 || strcmp(argv[(*ai_io)], "desc") == 0 ||
              strcmp(argv[(*ai_io)], "DOWN") == 0 || strcmp(argv[(*ai_io)], "down") == 0 ||
              strcmp(argv[(*ai_io)], "HEAVY") == 0 || strcmp(argv[(*ai_io)], "heavy") == 0 ||
              strcmp(argv[(*ai_io)], "HIGH") == 0 || strcmp(argv[(*ai_io)], "high") == 0) {
            want_asc = 0;
            (*ai_io)++;
            continue;
          }
          break;
        }

        if (!nest_hit) {
          if (as_bag) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                   "\"op\":\"nestsortbag\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,"
                   "\"asc\":%s,\"nest_hit\":false,\"bag\":\"\","
                   "\"file\":%s,\"version\":\"%s\"}\n",
                   path, nestk, want_asc ? "true" : "false",
                   (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
          } else {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                   "\"op\":\"nestsort\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,"
                   "\"asc\":%s,\"nest_hit\":false,\"written\":false,"
                   "\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
                   path, nestk, want_asc ? "true" : "false",
                   (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION, plate);
          }
          return 0;
        }

        memset(&sr, 0, sizeof sr);
        if (cubalc_host_json_sortbyval(nest, want_asc, as_bag, &sr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                 "\"version\":\"%s\"}\n",
                 op, path, nestk, sr.err[0] ? sr.err : "json sort fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }

        if (as_bag) {
          bag_esc[0] = 0;
          bo = 0;
          for (bi = 0; sr.str[bi] && bo + 2 < sizeof bag_esc; bi++) {
            char c = sr.str[bi];
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
                 "\"op\":\"nestsortbag\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
                 "\"asc\":%s,\"nest_hit\":true,\"bag\":\"%s\","
                 "\"file\":%s,\"version\":\"%s\"}\n",
                 path, nestk, sr.n, want_asc ? "true" : "false", bag_esc,
                 (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
          return 0;
        }

        /* write-back sorted nest into outer plate file */
        snprintf(nest, sizeof nest, "%s", sr.str);
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, nestk, nest, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"nestsort\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                 "\"version\":\"%s\"}\n",
                 path, nestk, wr.err[0] ? wr.err : "outer set fail",
                 CUBALC_LANG_VERSION);
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
                 "\"op\":\"nestsort\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 path, nestk, CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nestsort\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
               "\"asc\":%s,\"nest_hit\":true,\"written\":true,\"bytes\":%ld,"
               "\"file\":%s,\"version\":\"%s\",\"nest_body\":%s,\"plate\":%s}\n",
               path, nestk, sr.n, want_asc ? "true" : "false", hr.n,
               (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION, nest, plate);
        return 0;
      }

      if (strcmp(op, "nestkeys") == 0) {
        char flatk[CUBALC_HOST_STR_MAX];
        size_t i, o = 0;
        memset(&kr, 0, sizeof kr);
        if (nest_hit)
          cubalc_host_json_keys(nest, &kr);
        flatk[0] = 0;
        for (i = 0; kr.str[i] && o + 2 < sizeof flatk; i++) {
          char c = kr.str[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flatk[o - 1] != ',') flatk[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flatk[o++] = '_';
          } else {
            flatk[o++] = c;
          }
        }
        flatk[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nestkeys\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
               "\"nest_hit\":%s,\"keys\":\"%s\",\"file\":%s,\"version\":\"%s\"}\n",
               path, nestk, nest_hit ? kr.n : 0L,
               nest_hit ? "true" : "false", flatk,
               (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }

      
      /* nestrename|nestcopy|nestswap — RENAMEPOBJ/COPYPOBJ/SWAPPOBJ duals (nest path ok).
       *   cubalc plate nestrename agent.json flags dbg debug
       *   cubalc plate nestcopy   agent.json flags debug mirror
       *   cubalc plate nestswap   agent.json flags debug trace */
      if (strcmp(op, "nestrename") == 0 || strcmp(op, "nestcopy") == 0 ||
          strcmp(op, "nestswap") == 0) {
        const char *a = NULL, *b = NULL;
        cubalc_host_result rr;
        if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"need src key\",\"version\":\"%s\"}\n",
                 op, path, nestk, CUBALC_LANG_VERSION);
          return 2;
        }
        a = argv[(*ai_io)++];
        if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"need dst key\",\"version\":\"%s\"}\n",
                 op, path, nestk, CUBALC_LANG_VERSION);
          return 2;
        }
        b = argv[(*ai_io)++];
        if (!nest_hit) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"src\":\"%s\","
                 "\"dst\":\"%s\",\"n\":0,\"nest_hit\":false,\"file\":%s,"
                 "\"version\":\"%s\",\"plate\":%s}\n",
                 op, path, nestk, a, b, (*file_hit) ? "true" : "false",
                 CUBALC_LANG_VERSION, plate);
          return 0;
        }
        memset(&rr, 0, sizeof rr);
        if (strcmp(op, "nestrename") == 0) {
          if (cubalc_host_json_rename(nest, a, b, &rr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestrename\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   rr.err[0] ? rr.err : "rename fail", CUBALC_LANG_VERSION);
            return 1;
          }
        } else if (strcmp(op, "nestcopy") == 0) {
          if (cubalc_host_json_copy_key(nest, a, b, &rr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestcopy\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   rr.err[0] ? rr.err : "copy fail", CUBALC_LANG_VERSION);
            return 1;
          }
        } else {
          if (cubalc_host_json_swap_keys(nest, a, b, &rr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestswap\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   rr.err[0] ? rr.err : "swap fail", CUBALC_LANG_VERSION);
            return 1;
          }
        }
        if (rr.n == 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"src\":\"%s\","
                 "\"dst\":\"%s\",\"n\":0,\"nest_hit\":true,\"file\":%s,"
                 "\"version\":\"%s\",\"plate\":%s}\n",
                 op, path, nestk, a, b, (*file_hit) ? "true" : "false",
                 CUBALC_LANG_VERSION, plate);
          return 0;
        }
        snprintf(nest, sizeof nest, "%s", rr.str);
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, nestk, nest, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 op, wr.err[0] ? wr.err : "outer set fail", CUBALC_LANG_VERSION);
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
                 "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"src\":\"%s\","
               "\"dst\":\"%s\",\"n\":%ld,\"nest_hit\":true,\"file\":%s,"
               "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s,"
               "\"note\":\"RENAMEPOBJ/COPYPOBJ/SWAPPOBJ dual · nest path ok\"}\n",
               op, path, nestk, a, b, rr.n, (*file_hit) ? "true" : "false",
               hr.n, CUBALC_LANG_VERSION, plate);
        return 0;
      }

      /* pluckobj — multi-key peel from nest → value bag (PLUCKOBJ dual; nest path ok).
       * Usability: agent one-shot nested extract without .cubalc:
       *   cubalc plate pluckobj agent.json cfg.meta role x zone
       * Bag lines above plate meta · empty field on miss · hit/miss counts. */
      if (strcmp(op, "pluckobj") == 0) {
        char keys_nl[CUBALC_HOST_STR_MAX];
        char flat_req[CUBALC_HOST_STR_MAX];
        char bag_esc[CUBALC_HOST_STR_MAX];
        cubalc_host_result pr;
        size_t o = 0, bi, bo;
        long nreq = 0;
        long hit = 0, miss = 0;

        keys_nl[0] = 0;
        flat_req[0] = 0;
        if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"pluckobj\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"need one or more fields\",\"version\":\"%s\"}\n",
                 path, nestk, CUBALC_LANG_VERSION);
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
        if (nest_hit) {
          if (cubalc_host_json_pluck(nest, keys_nl, &pr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"pluckobj\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, pr.err[0] ? pr.err : "pluck fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          hit = (long)pr.code;
          miss = pr.n - hit;
        } else {
          /* soft nest miss: nreq empty bag fields */
          {
            int i;
            pr.str[0] = 0;
            o = 0;
            for (i = 0; i < nreq; i++) {
              if (i > 0 && o + 1 < sizeof pr.str) pr.str[o++] = '\n';
            }
            pr.str[o] = 0;
            pr.n = nreq;
            pr.code = 0;
          }
          hit = 0;
          miss = nreq;
        }

        if (pr.str[0]) {
          fputs(pr.str, stdout);
          if (pr.str[strlen(pr.str) - 1] != '\n')
            fputc('\n', stdout);
        } else if (nreq > 0) {
          /* all-empty bag still emits blank lines for agent field count */
          {
            int i;
            for (i = 0; i < nreq; i++)
              fputc('\n', stdout);
          }
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
               "\"op\":\"pluckobj\",\"path\":\"%s\",\"nest\":\"%s\","
               "\"n\":%ld,\"hit\":%ld,\"miss\":%ld,\"keys\":\"%s\",\"bag\":\"%s\","
               "\"nest_hit\":%s,\"file\":%s,\"version\":\"%s\","
               "\"note\":\"bag lines above plate · PLUCKOBJ dual · nest path ok\"}\n",
               path, nestk, nreq, hit, miss, flat_req, bag_esc,
               nest_hit ? "true" : "false",
               (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }

      /* nestpick|nestomit — multi-key keep/drop inside nest (PICKOBJ/OMITOBJ duals). */
      if (strcmp(op, "nestpick") == 0 || strcmp(op, "nestomit") == 0) {
        char keys_nl[CUBALC_HOST_STR_MAX];
        char flat_req[CUBALC_HOST_STR_MAX];
        cubalc_host_result pr;
        size_t o = 0;
        long nreq = 0;
        int is_omit = (strcmp(op, "nestomit") == 0) ? 1 : 0;

        keys_nl[0] = 0;
        flat_req[0] = 0;
        if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"need one or more fields\",\"version\":\"%s\"}\n",
                 op, path, nestk, CUBALC_LANG_VERSION);
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
          if (cubalc_host_json_drop(nest, keys_nl, &pr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestomit\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, pr.err[0] ? pr.err : "json drop fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
        } else {
          if (cubalc_host_json_pick(nest, keys_nl, &pr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestpick\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, pr.err[0] ? pr.err : "json pick fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
        }
        snprintf(nest, sizeof nest, "%s", pr.str);

        /* path write-back: nest may be dotted (cfg.flags) */
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, nestk, nest, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 op, wr.err[0] ? wr.err : "outer set fail", CUBALC_LANG_VERSION);
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
                 "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
               "\"n_req\":%ld,\"keys\":\"%s\",\"nest_hit\":%s,\"file\":%s,"
               "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s,"
               "\"note\":\"PICKOBJ/OMITOBJ dual · nest path ok\"}\n",
               op, path, nestk, pr.n, nreq, flat_req,
               nest_hit ? "true" : "false",
               (*file_hit) ? "true" : "false", hr.n, CUBALC_LANG_VERSION, plate);
        return 0;
      }

if ((*ai_io) >= argc || !argv[(*ai_io)] || !argv[(*ai_io)][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"need field\","
               "\"version\":\"%s\"}\n", op, path, nestk, CUBALC_LANG_VERSION);
        return 2;
      }
      field = argv[(*ai_io)++];

      if (strcmp(op, "nesthas") == 0) {
        int hit = 0;
        if (nest_hit) {
          memset(&gr, 0, sizeof gr);
          if (cubalc_host_json_get_raw(nest, field, &gr) == 0)
            hit = 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nesthas\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
               "\"hit\":%s,\"nest_hit\":%s,\"file\":%s,\"version\":\"%s\"}\n",
               path, nestk, field, hit ? "true" : "false",
               nest_hit ? "true" : "false",
               (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
        return hit ? 0 : 1;
      }

      if (strcmp(op, "nestget") == 0) {
        int have_fb2 = 0;
        const char *fb2 = NULL;
        if ((*ai_io) < argc && (strcmp(argv[(*ai_io)], "OR") == 0 ||
                          strcmp(argv[(*ai_io)], "DEFAULT") == 0 ||
                          strcmp(argv[(*ai_io)], "ELSE") == 0)) {
          (*ai_io)++;
          if ((*ai_io) < argc) {
            fb2 = argv[(*ai_io)++];
            have_fb2 = 1;
          }
        }
        if (nest_hit) {
          memset(&gr, 0, sizeof gr);
          if (cubalc_host_json_get(nest, field, &gr) == 0) {
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
                   "\"op\":\"nestget\",\"path\":\"%s\",\"nest\":\"%s\","
                   "\"field\":\"%s\",\"hit\":true,\"value\":\"%s\",\"file\":%s,"
                   "\"version\":\"%s\"}\n",
                   path, nestk, field, esc,
                   (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
            return 0;
          }
        }
        if (have_fb2 && fb2) {
          char esc[512];
          size_t i, o = 0;
          for (i = 0; fb2[i] && o + 2 < sizeof esc; i++) {
            char c = fb2[i];
            if (c == '"' || c == '\\') {
              esc[o++] = '\\';
              esc[o++] = c;
            } else {
              esc[o++] = c;
            }
          }
          esc[o] = 0;
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestget\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"field\":\"%s\",\"hit\":false,\"or\":true,\"value\":\"%s\","
                 "\"file\":%s,\"version\":\"%s\"}\n",
                 path, nestk, field, esc,
                 (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
          return 0;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nestget\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
               "\"hit\":false,\"err\":\"field miss\",\"file\":%s,\"version\":\"%s\"}\n",
               path, nestk, field,
               (*file_hit) ? "true" : "false", CUBALC_LANG_VERSION);
        return 1;
      }

      {
        long delta_n = 1, cur = 0, nv = 0;
        int val_kind2 = 0;
        const char *val2 = NULL;
        char raw[32];

        if (strcmp(op, "nestset") == 0) {
          size_t i;
          int allnum = 1;
          if ((*ai_io) >= argc) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestset\",\"err\":\"need value\",\"version\":\"%s\"}\n",
                   CUBALC_LANG_VERSION);
            return 2;
          }
          val2 = argv[(*ai_io)++];
          if (!strcmp(val2, "true") || !strcmp(val2, "false") ||
              !strcmp(val2, "null") || val2[0] == '{' || val2[0] == '[')
            val_kind2 = 1;
          else {
            if (val2[0] == '-' && val2[1]) i = 1;
            else i = 0;
            if (!val2[i]) allnum = 0;
            for (; val2[i]; i++) {
              if (val2[i] < '0' || val2[i] > '9') {
                allnum = 0;
                break;
              }
            }
            if (allnum) val_kind2 = 1;
          }
          memset(&wr, 0, sizeof wr);
          if (cubalc_host_json_set(nest, field, val2, val_kind2, &wr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestset\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   wr.err[0] ? wr.err : "nest set fail", CUBALC_LANG_VERSION);
            return 1;
          }
          snprintf(nest, sizeof nest, "%s", wr.str);
        } else if (strcmp(op, "nestinc") == 0) {
          if ((*ai_io) < argc) {
            char *end = NULL;
            delta_n = strtol(argv[(*ai_io)], &end, 10);
            if (end == argv[(*ai_io)]) delta_n = 1;
            (*ai_io)++;
          }
          memset(&gr, 0, sizeof gr);
          if (cubalc_host_json_get(nest, field, &gr) == 0) {
            char *end = NULL;
            cur = strtol(gr.str, &end, 10);
            if (end == gr.str) cur = 0;
          }
          nv = cur + delta_n;
          snprintf(raw, sizeof raw, "%ld", nv);
          memset(&wr, 0, sizeof wr);
          if (cubalc_host_json_set(nest, field, raw, 1, &wr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestinc\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   wr.err[0] ? wr.err : "nest set fail", CUBALC_LANG_VERSION);
            return 1;
          }
          snprintf(nest, sizeof nest, "%s", wr.str);
        } else {
          memset(&wr, 0, sizeof wr);
          if (cubalc_host_json_del(nest, field, &wr) != 0) {
            wr.n = 0;
          } else {
            snprintf(nest, sizeof nest, "%s", wr.str);
          }
        }
        {
          long nest_removed = wr.n;

        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, nestk, nest, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 op, wr.err[0] ? wr.err : "outer set fail", CUBALC_LANG_VERSION);
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
                 "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        if (strcmp(op, "nestinc") == 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestinc\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
                 "\"value\":%ld,\"delta\":%ld,\"bytes\":%ld,\"version\":\"%s\","
                 "\"plate\":%s}\n",
                 path, nestk, field, nv, delta_n, hr.n, CUBALC_LANG_VERSION, plate);
        } else if (strcmp(op, "nestdel") == 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestdel\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
                 "\"removed\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
                 path, nestk, field, nest_removed, hr.n, CUBALC_LANG_VERSION, plate);
        } else {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestset\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
                 "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
                 path, nestk, field, hr.n, CUBALC_LANG_VERSION, plate);
        }
        return 0;
        }
      }
    }

  return -1;
}
