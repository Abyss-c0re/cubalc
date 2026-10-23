/* CubalC CLI picklib/nthlib/sortlibs/matchlibs — F_main_carve · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_libmatch.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

int cubalc_cmd_libmatch(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "picklib";

    /* Usability: CLI duals of PICKLIB/COUNTMATCHLIBS/HASMATCHLIBS/NEEDMATCHLIBS/
     * LASTLIB/NTHLIB/NEWESTLIB/OLDESTLIB/SORTLIBS — filter lib discovery without .cubalc.
     *   cubalc picklib plate · newestlib fat · sortlibs fat [oldest]
     * Schema cubalc.libmatch.v1 (+ mtime · stems ordered for sortlibs) */
    const char *filter = "";
    int want_idx = -1;
    char stems[96][96];
    char stem_paths[96][768];
    long mtimes[96];
    int nstem = 0, i, j;
    char fup[96];
    size_t a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    int mode_count = 0, mode_has = 0, mode_need = 0, mode_last = 0, mode_nth = 0;
    int mode_newest = 0, mode_oldest = 0, mode_sort = 0, sort_old = 0;
    if (strcmp(cmd, "countmatchlibs") == 0 || strcmp(cmd, "countlibs") == 0 ||
        strcmp(cmd, "libcount") == 0)
      mode_count = 1;
    else if (strcmp(cmd, "hasmatchlibs") == 0)
      mode_has = 1;
    else if (strcmp(cmd, "needmatchlibs") == 0)
      mode_need = 1;
    else if (strcmp(cmd, "lastlib") == 0)
      mode_last = 1;
    else if (strcmp(cmd, "nthlib") == 0)
      mode_nth = 1;
    else if (strcmp(cmd, "newestlib") == 0 || strcmp(cmd, "latestlib") == 0)
      mode_newest = 1;
    else if (strcmp(cmd, "oldestlib") == 0 || strcmp(cmd, "earliestlib") == 0)
      mode_oldest = 1;
    else if (strcmp(cmd, "sortlibs") == 0 || strcmp(cmd, "libsort") == 0 ||
             strcmp(cmd, "sortlib") == 0 || strcmp(cmd, "libsbymtime") == 0)
      mode_sort = 1;
    if (mode_nth) {
      if (argc > 2 && argv[2][0]) {
        char *end = NULL;
        long v = strtol(argv[2], &end, 10);
        if (end && end != argv[2] && !*end) {
          want_idx = (int)v;
          filter = (argc > 3) ? argv[3] : "";
        } else {
          filter = argv[2];
          want_idx = 0;
        }
      }
    } else if (mode_sort) {
      filter = (argc > 2) ? argv[2] : "";
      if (argc > 3 && argv[3][0]) {
        const char *ord = argv[3];
        if (strcasecmp(ord, "asc") == 0 || strcasecmp(ord, "oldest") == 0 ||
            strcasecmp(ord, "earliest") == 0 || strcasecmp(ord, "old") == 0 ||
            strcasecmp(ord, "stale") == 0)
          sort_old = 1;
      }
    } else {
      filter = (argc > 2) ? argv[2] : "";
    }
    if (!filter || !filter[0]) {
      fprintf(stderr,
              "usage: cubalc picklib|firstlib <filter>\n"
              "       cubalc countmatchlibs|countlibs <filter>\n"
              "       cubalc hasmatchlibs|needmatchlibs <filter>\n"
              "       cubalc lastlib|newestlib|oldestlib <filter>\n"
              "       cubalc sortlibs|libsort <filter> [oldest|asc]\n"
              "       cubalc nthlib <idx> <filter>\n");
      printf("{\"schema\":\"cubalc.libmatch.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need filter\",\"version\":\"%s\"}\n",
             cmd, CUBALC_LANG_VERSION);
      return 2;
    }
    for (a = 0; filter[a] && a + 1 < sizeof fup; a++) {
      char c = filter[a];
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      fup[a] = c;
    }
    fup[a] = 0;
    {
      char dirs[10][160];
      int nd = 0, di;
      snprintf(dirs[nd++], sizeof dirs[0], "%s", "programs/lib");
      ip = getenv("CUBALC_INCLUDE_PATH");
      if (ip && ip[0]) {
        const char *p = ip;
        while (*p && nd < 10) {
          char dir[160];
          size_t len = 0;
          while (*p == ':' || *p == ' ' || *p == '\t') p++;
          if (!*p) break;
          while (p[len] && p[len] != ':' && len + 1 < sizeof dir) {
            dir[len] = p[len];
            len++;
          }
          dir[len] = 0;
          p += len;
          if (dir[0])
            snprintf(dirs[nd++], sizeof dirs[0], "%s", dir);
        }
      }
      for (di = 0; di < nd; di++) {
        d = opendir(dirs[di]);
        if (!d) continue;
        while ((ent = readdir(d)) != NULL && nstem < 96) {
          size_t len = strlen(ent->d_name);
          char stem[96], hay[192];
          int dup = 0;
          size_t b;
          struct stat st;
          if (len < 8 || strcmp(ent->d_name + len - 7, ".cubalc") != 0)
            continue;
          if (ent->d_name[0] == '.') continue;
          if (len - 7 >= sizeof stem) continue;
          memcpy(stem, ent->d_name, len - 7);
          stem[len - 7] = 0;
          for (j = 0; j < nstem; j++) {
            if (strcmp(stems[j], stem) == 0) { dup = 1; break; }
          }
          if (dup) continue;
          snprintf(hay, sizeof hay, "%s/%s", dirs[di], ent->d_name);
          for (b = 0; hay[b]; b++)
            if (hay[b] >= 'A' && hay[b] <= 'Z')
              hay[b] = (char)(hay[b] - 'A' + 'a');
          {
            char stem_l[96];
            for (b = 0; stem[b] && b + 1 < sizeof stem_l; b++) {
              char c = stem[b];
              if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              stem_l[b] = c;
            }
            stem_l[b] = 0;
            if (!strstr(stem_l, fup) && !strstr(hay, fup))
              continue;
          }
          snprintf(stems[nstem], sizeof stems[0], "%s", stem);
          snprintf(stem_paths[nstem], sizeof stem_paths[0], "%s/%s",
                   dirs[di], ent->d_name);
          mtimes[nstem] = 0;
          if (stat(stem_paths[nstem], &st) == 0)
            mtimes[nstem] = (long)st.st_mtime;
          nstem++;
        }
        closedir(d);
      }
    }
    if (mode_sort) {
      /* mtime order: default newest-first · optional oldest/asc */
      for (i = 1; i < nstem; i++) {
        char tmp[96], tp[768];
        long tm;
        snprintf(tmp, sizeof tmp, "%s", stems[i]);
        snprintf(tp, sizeof tp, "%s", stem_paths[i]);
        tm = mtimes[i];
        j = i;
        while (j > 0) {
          int before = 0;
          if (sort_old) {
            if (mtimes[j - 1] > tm ||
                (mtimes[j - 1] == tm && strcmp(stems[j - 1], tmp) > 0))
              before = 1;
          } else {
            if (mtimes[j - 1] < tm ||
                (mtimes[j - 1] == tm && strcmp(stems[j - 1], tmp) > 0))
              before = 1;
          }
          if (!before) break;
          snprintf(stems[j], sizeof stems[0], "%s", stems[j - 1]);
          snprintf(stem_paths[j], sizeof stem_paths[0], "%s", stem_paths[j - 1]);
          mtimes[j] = mtimes[j - 1];
          j--;
        }
        snprintf(stems[j], sizeof stems[0], "%s", tmp);
        snprintf(stem_paths[j], sizeof stem_paths[0], "%s", tp);
        mtimes[j] = tm;
      }
    } else {
      for (i = 1; i < nstem; i++) {
        char tmp[96], tp[768];
        long tm;
        snprintf(tmp, sizeof tmp, "%s", stems[i]);
        snprintf(tp, sizeof tp, "%s", stem_paths[i]);
        tm = mtimes[i];
        j = i;
        while (j > 0 && strcmp(stems[j - 1], tmp) > 0) {
          snprintf(stems[j], sizeof stems[0], "%s", stems[j - 1]);
          snprintf(stem_paths[j], sizeof stem_paths[0], "%s", stem_paths[j - 1]);
          mtimes[j] = mtimes[j - 1];
          j--;
        }
        snprintf(stems[j], sizeof stems[0], "%s", tmp);
        snprintf(stem_paths[j], sizeof stem_paths[0], "%s", tp);
        mtimes[j] = tm;
      }
    }
    {
      const char *pick = "";
      int pick_i = -1;
      int ok = 0;
      long mtime = 0;
      if (mode_count || mode_has || mode_need) {
        ok = (nstem > 0);
      } else if (mode_sort) {
        if (nstem > 0) {
          pick_i = 0;
          pick = stems[0];
          mtime = mtimes[0];
          ok = 1;
        }
      } else if (mode_newest || mode_oldest) {
        long best_mt = 0;
        int have_best = 0;
        for (i = 0; i < nstem; i++) {
          if (!have_best ||
              (mode_oldest ? (mtimes[i] < best_mt) : (mtimes[i] > best_mt)) ||
              (mtimes[i] == best_mt && strcmp(stems[i], stems[pick_i < 0 ? 0 : pick_i]) < 0)) {
            best_mt = mtimes[i];
            have_best = 1;
            pick_i = i;
          }
        }
        if (have_best) {
          pick = stems[pick_i];
          mtime = best_mt;
          ok = 1;
        }
      } else if (mode_last) {
        if (nstem > 0) {
          pick_i = nstem - 1;
          pick = stems[pick_i];
          ok = 1;
        }
      } else if (mode_nth) {
        if (want_idx < 0) want_idx = 0;
        if (want_idx >= 0 && want_idx < nstem) {
          pick_i = want_idx;
          pick = stems[pick_i];
          ok = 1;
        }
      } else {
        if (nstem > 0) {
          pick_i = 0;
          pick = stems[0];
          ok = 1;
        }
      }
      printf("{\"schema\":\"cubalc.libmatch.v1\",\"ok\":%s,\"cmd\":\"%s\","
             "\"filter\":\"%s\",\"n\":%d,\"index\":%d,\"stem\":\"%s\","
             "\"mtime\":%ld,\"path\":\"%s\","
             "\"order\":\"%s\","
             "\"version\":\"%s\","
             "\"note\":\"CLI dual of PICKLIB/NEWESTLIB/SORTLIBS · cubalc libs [filter]\","
             "\"stems\":[",
             ok ? "true" : "false", cmd, filter, nstem, pick_i,
             pick[0] ? pick : "", mtime,
             (ok && pick_i >= 0) ? stem_paths[pick_i] : "",
             mode_sort ? (sort_old ? "oldest" : "newest") :
             (mode_oldest ? "oldest" : (mode_newest ? "newest" : "alpha")),
             CUBALC_LANG_VERSION);
      for (i = 0; i < nstem; i++)
        printf("%s\"%s\"", i ? "," : "", stems[i]);
      if (mode_sort) {
        printf("],\"mtimes\":[");
        for (i = 0; i < nstem; i++)
          printf("%s%ld", i ? "," : "", mtimes[i]);
        printf("]}\n");
      } else {
        printf("]}\n");
      }
      if (mode_need)
        return ok ? 0 : 1;
      if (mode_has || mode_count)
        return 0;
      /* sortlibs empty filter match is soft-ok plate with n=0 exit 1 */
      if (mode_sort)
        return ok ? 0 : 1;
      return ok ? 0 : 1;
    }
  
  return 1;
}
