/* CubalC CLI checkdeps/hasdeps/needdeps — F_main_carve peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_checkdeps.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

int cubalc_cmd_checkdeps(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "checkdeps";

    /* Usability: CLI dual of CHECKDEPS/HASDEPS/NEEDDEPS — install gate for a lib
     * + transitive INCLUDE tree without writing a .cubalc program.
     *   cubalc checkdeps fat_session
     *   cubalc hasdeps plate_tick
     *   cubalc needdeps fat_session   # exit 1 if any miss
     * Schema cubalc.checkdeps.v1 · ok true only when all on disk. */
    const char *arg = (argc > 2) ? argv[2] : "";
    char stems[64][96];
    char paths_hit[64][768];
    int present[64];
    int nstem = 0, nmiss = 0, nok = 0, qi = 0, i, j;
    int hard = (strcmp(cmd, "needdeps") == 0);
    char root_path[768], root_stem[160];
    int root_found = 0;
    if (!arg || !arg[0]) {
      fprintf(stderr,
              "usage: cubalc checkdeps <libname|path.cubalc>\n"
              "       cubalc hasdeps fat_session · cubalc needdeps plate_tick\n");
      printf("{\"schema\":\"cubalc.checkdeps.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need libname\",\"version\":\"%s\"}\n",
             cmd, CUBALC_LANG_VERSION);
      return 2;
    }
    /* stem + resolve root (mirror recipe / INCLUDE short-name) */
    {
      const char *slash = strrchr(arg, '/');
      const char *leaf = slash ? slash + 1 : arg;
      size_t blen;
      char tried[5][768];
      int ntry = 0;
      snprintf(root_stem, sizeof root_stem, "%s", leaf);
      blen = strlen(root_stem);
      if (blen > 7 && strcmp(root_stem + blen - 7, ".cubalc") == 0)
        root_stem[blen - 7] = 0;
      root_path[0] = 0;
      snprintf(tried[ntry++], sizeof tried[0], "%s", arg);
      if (ntry < 5)
        snprintf(tried[ntry++], sizeof tried[0], "programs/lib/%s.cubalc", root_stem);
      if (ntry < 5)
        snprintf(tried[ntry++], sizeof tried[0], "programs/lib/%s", arg);
      for (i = 0; i < ntry; i++) {
        if (access(tried[i], R_OK) == 0) {
          snprintf(root_path, sizeof root_path, "%s", tried[i]);
          root_found = 1;
          break;
        }
      }
      if (!root_found) {
        cubalc_host_result hr;
        if (cubalc_host_find_cubalc(arg, &hr) == 0 && hr.str[0] &&
            access(hr.str, R_OK) == 0) {
          snprintf(root_path, sizeof root_path, "%s", hr.str);
          root_found = 1;
        }
      }
      if (!root_found) {
        const char *ip = getenv("CUBALC_INCLUDE_PATH");
        if (ip && ip[0]) {
          const char *seg = ip;
          while (*seg && !root_found) {
            char dir[512], p3[768];
            size_t dlen = 0;
            while (*seg == ':') seg++;
            if (!*seg) break;
            while (seg[dlen] && seg[dlen] != ':' && dlen + 1 < sizeof dir)
              dir[dlen] = seg[dlen], dlen++;
            dir[dlen] = 0;
            seg += dlen;
            if (!dir[0]) continue;
            snprintf(p3, sizeof p3, "%s/%s.cubalc", dir, root_stem);
            if (access(p3, R_OK) == 0) {
              snprintf(root_path, sizeof root_path, "%s", p3);
              root_found = 1;
              break;
            }
          }
        }
      }
    }
    memset(present, 0, sizeof present);
    nstem = 0;
    snprintf(stems[nstem], sizeof stems[0], "%s", root_stem);
    present[nstem] = root_found;
    if (root_found)
      snprintf(paths_hit[nstem], sizeof paths_hit[0], "%s", root_path);
    else
      paths_hit[nstem][0] = 0;
    nstem = 1;
    /* BFS INCLUDE stems from files that exist */
    qi = 0;
    while (qi < nstem && nstem < 64) {
      if (present[qi] && paths_hit[qi][0]) {
        FILE *f = fopen(paths_hit[qi], "rb");
        char *src = NULL;
        long sz = 0;
        size_t nr = 0;
        const char *lp;
        if (f) {
          fseek(f, 0, SEEK_END);
          sz = ftell(f);
          fseek(f, 0, SEEK_SET);
          if (sz < 0) sz = 0;
          if (sz > 256 * 1024) sz = 256 * 1024;
          src = malloc((size_t)sz + 1);
          if (src) {
            nr = fread(src, 1, (size_t)sz, f);
            src[nr] = 0;
            lp = src;
            while (*lp && nstem < 64) {
              char raw[512];
              size_t ri = 0;
              const char *p;
              while (*lp && *lp != '\n' && ri + 1 < sizeof raw)
                raw[ri++] = *lp++;
              raw[ri] = 0;
              if (*lp == '\n') lp++;
              p = raw;
              while (*p == ' ' || *p == '\t') p++;
              if (*p == '#' || !*p) continue;
              if (p[0] == 'I' && p[1] == 'N' && p[2] == 'C' && p[3] == 'L' &&
                  p[4] == 'U' && p[5] == 'D' && p[6] == 'E' &&
                  (p[7] == ' ' || p[7] == '\t' || p[7] == '"' || p[7] == '\'')) {
                char tok[96];
                size_t ti = 0;
                int dup = 0;
                p += 7;
                while (*p == ' ' || *p == '\t') p++;
                for (;;) {
                  if (p[0] == 'O' && p[1] == 'N' && p[2] == 'C' && p[3] == 'E' &&
                      (p[4] == ' ' || p[4] == '\t' || !p[4])) {
                    p += 4; while (*p == ' ' || *p == '\t') p++; continue;
                  }
                  if (p[0] == 'S' && p[1] == 'O' && p[2] == 'F' && p[3] == 'T' &&
                      (p[4] == ' ' || p[4] == '\t' || !p[4])) {
                    p += 4; while (*p == ' ' || *p == '\t') p++; continue;
                  }
                  if (p[0] == 'O' && p[1] == 'R' &&
                      (p[2] == ' ' || p[2] == '\t' || !p[2])) {
                    p += 2; while (*p == ' ' || *p == '\t') p++; continue;
                  }
                  break;
                }
                if (*p == '"' || *p == '\'') {
                  char q = *p++;
                  while (*p && *p != q && ti + 1 < sizeof tok) tok[ti++] = *p++;
                  tok[ti] = 0;
                } else {
                  while (*p && *p != ' ' && *p != '\t' && *p != '#' &&
                         ti + 1 < sizeof tok)
                    tok[ti++] = *p++;
                  tok[ti] = 0;
                }
                if (tok[0]) {
                  const char *sl = strrchr(tok, '/');
                  const char *st = sl ? sl + 1 : tok;
                  size_t slen;
                  char stem[96];
                  snprintf(stem, sizeof stem, "%s", st);
                  slen = strlen(stem);
                  if (slen > 7 && strcmp(stem + slen - 7, ".cubalc") == 0)
                    stem[slen - 7] = 0;
                  for (j = 0; j < nstem; j++)
                    if (strcmp(stems[j], stem) == 0) { dup = 1; break; }
                  if (!dup && stem[0] && nstem < 64) {
                    char pth[768];
                    int hit = 0;
                    snprintf(stems[nstem], sizeof stems[0], "%s", stem);
                    snprintf(pth, sizeof pth, "programs/lib/%s.cubalc", stem);
                    if (access(pth, R_OK) == 0) {
                      hit = 1;
                      snprintf(paths_hit[nstem], sizeof paths_hit[0], "%s", pth);
                    } else {
                      cubalc_host_result hr;
                      if (cubalc_host_find_cubalc(stem, &hr) == 0 && hr.str[0] &&
                          access(hr.str, R_OK) == 0) {
                        hit = 1;
                        snprintf(paths_hit[nstem], sizeof paths_hit[0], "%s",
                                 hr.str);
                      } else {
                        paths_hit[nstem][0] = 0;
                      }
                    }
                    present[nstem] = hit;
                    nstem++;
                  }
                }
              }
            }
            free(src);
          }
          fclose(f);
        }
      }
      qi++;
    }
    for (i = 0; i < nstem; i++) {
      if (present[i]) nok++;
      else nmiss++;
    }
    {
      int ok_all = (nmiss == 0 && nstem > 0);
      printf("{\"schema\":\"cubalc.checkdeps.v1\",\"ok\":%s,\"cmd\":\"%s\","
             "\"query\":\"%s\",\"stem\":\"%s\",\"path\":\"%s\","
             "\"deps_n\":%d,\"ok_n\":%d,\"miss_n\":%d,"
             "\"version\":\"%s\","
             "\"note\":\"CLI dual of CHECKDEPS/HASDEPS/NEEDDEPS · root+LIBTREE disk\","
             "\"deps\":[",
             ok_all ? "true" : "false", cmd, arg, root_stem,
             root_found ? root_path : "",
             nstem, nok, nmiss, CUBALC_LANG_VERSION);
      for (i = 0; i < nstem; i++)
        printf("%s\"%s\"", i ? "," : "", stems[i]);
      printf("],\"ok_stems\":[");
      {
        int first = 1;
        for (i = 0; i < nstem; i++) {
          if (!present[i]) continue;
          printf("%s\"%s\"", first ? "" : ",", stems[i]);
          first = 0;
        }
      }
      printf("],\"miss\":[");
      {
        int first = 1;
        for (i = 0; i < nstem; i++) {
          if (present[i]) continue;
          printf("%s\"%s\"", first ? "" : ",", stems[i]);
          first = 0;
        }
      }
      printf("]}\n");
      (void)hard;
      /* exit 1 on any miss so shell agents can gate installs */
      return ok_all ? 0 : 1;
    }
  
  return 1;
}
