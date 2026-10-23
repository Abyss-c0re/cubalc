/* CubalC CLI freshlibs/stalelibs — F_main_carve peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_freshlibs.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

int cubalc_cmd_freshlibs(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "freshlibs";

    /* Usability: CLI dual of FRESHLIBS/STALELIBS — age-filter lib stems without .cubalc.
     *   cubalc freshlibs fat 100000
     *   cubalc stalelibs plate 86400
     * Schema cubalc.libmatch.v1 · order newest · mode fresh|stale · age_lim · ages[] */
    const char *filter = (argc > 2) ? argv[2] : "";
    long age_lim = 86400;
    int want_stale = 0;
    char stems[96][96];
    char stem_paths[96][768];
    long mtimes[96], ages[96];
    int nstem = 0, i, j;
    char fup[96];
    size_t a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    time_t now = time(NULL);
    if (strcmp(cmd, "stalelibs") == 0 || strcmp(cmd, "olderlibs") == 0 ||
        strcmp(cmd, "keepstalelibs") == 0)
      want_stale = 1;
    if (argc > 3 && argv[3][0]) {
      char *end = NULL;
      long v = strtol(argv[3], &end, 10);
      if (end && end != argv[3] && !*end && v >= 0)
        age_lim = v;
    }
    if (!filter || !filter[0]) {
      fprintf(stderr,
              "usage: cubalc freshlibs|recentlibs <filter> [max_age_sec]\n"
              "       cubalc stalelibs|olderlibs <filter> [min_age_sec]\n"
              "  default age threshold: 86400 (1 day)\n");
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
          long age;
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
          snprintf(stem_paths[nstem], sizeof stem_paths[0], "%s/%s",
                   dirs[di], ent->d_name);
          if (stat(stem_paths[nstem], &st) != 0) continue;
          age = (long)now - (long)st.st_mtime;
          if (age < 0) age = 0;
          if (want_stale) {
            if (age < age_lim) continue;
          } else {
            if (age > age_lim) continue;
          }
          snprintf(stems[nstem], sizeof stems[0], "%s", stem);
          mtimes[nstem] = (long)st.st_mtime;
          ages[nstem] = age;
          nstem++;
        }
        closedir(d);
      }
    }
    /* newest-first */
    for (i = 1; i < nstem; i++) {
      char tmp[96], tp[768];
      long tm, ag;
      snprintf(tmp, sizeof tmp, "%s", stems[i]);
      snprintf(tp, sizeof tp, "%s", stem_paths[i]);
      tm = mtimes[i];
      ag = ages[i];
      j = i;
      while (j > 0) {
        int before = 0;
        if (mtimes[j - 1] < tm ||
            (mtimes[j - 1] == tm && strcmp(stems[j - 1], tmp) > 0))
          before = 1;
        if (!before) break;
        snprintf(stems[j], sizeof stems[0], "%s", stems[j - 1]);
        snprintf(stem_paths[j], sizeof stem_paths[0], "%s", stem_paths[j - 1]);
        mtimes[j] = mtimes[j - 1];
        ages[j] = ages[j - 1];
        j--;
      }
      snprintf(stems[j], sizeof stems[0], "%s", tmp);
      snprintf(stem_paths[j], sizeof stem_paths[0], "%s", tp);
      mtimes[j] = tm;
      ages[j] = ag;
    }
    {
      int ok = (nstem > 0);
      printf("{\"schema\":\"cubalc.libmatch.v1\",\"ok\":%s,\"cmd\":\"%s\","
             "\"filter\":\"%s\",\"n\":%d,\"mode\":\"%s\",\"age_lim\":%ld,"
             "\"stem\":\"%s\",\"mtime\":%ld,\"age\":%ld,\"path\":\"%s\","
             "\"order\":\"newest\",\"version\":\"%s\","
             "\"note\":\"CLI dual of FRESHLIBS/STALELIBS · age filter · cubalc libs\","
             "\"stems\":[",
             ok ? "true" : "false", cmd, filter, nstem,
             want_stale ? "stale" : "fresh", age_lim,
             nstem > 0 ? stems[0] : "",
             nstem > 0 ? mtimes[0] : 0L,
             nstem > 0 ? ages[0] : 0L,
             nstem > 0 ? stem_paths[0] : "",
             CUBALC_LANG_VERSION);
      for (i = 0; i < nstem; i++)
        printf("%s\"%s\"", i ? "," : "", stems[i]);
      printf("],\"ages\":[");
      for (i = 0; i < nstem; i++)
        printf("%s%ld", i ? "," : "", ages[i]);
      printf("],\"mtimes\":[");
      for (i = 0; i < nstem; i++)
        printf("%s%ld", i ? "," : "", mtimes[i]);
      printf("]}\n");
      return ok ? 0 : 1;
    }
  
  return 1;
}
