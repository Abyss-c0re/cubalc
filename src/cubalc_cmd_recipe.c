/* CubalC CLI recipe/libcard — F_main_carve peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_recipe.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include "cubalc_hostops.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

int cubalc_cmd_recipe(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "recipe";
  (void)cmd;

    /* Usability: one agent plate for a lib recipe — path + deps + defaults + head
     * without shelling CATLIB/LIBDEPS/LIBDEFAULTS/HEADLIB separately.
     *   cubalc recipe fat_session
     *   cubalc card plate_tick
     * Schema cubalc.recipe.v1 · soft miss JSON ok:false. */
    const char *arg = (argc > 2) ? argv[2] : "";
    char path[768], stem[160], tried[4][768];
    int ntry = 0, found = 0, i;
    FILE *f = NULL;
    long sz = 0;
    char *buf = NULL;
    size_t nr = 0;
    int lines = 0, ndep = 0, nknob = 0;
    char deps[32][96];
    char knobs[32][160];
    char head[8][256];
    int nhead = 0;
    const char *lp;
    if (!arg || !arg[0]) {
      fprintf(stderr, "usage: cubalc recipe <libname|path.cubalc>\n"
                      "       cubalc card fat_session · cubalc libcard plate_tick\n");
      printf("{\"schema\":\"cubalc.recipe.v1\",\"ok\":false,\"cmd\":\"recipe\","
             "\"err\":\"need libname\",\"version\":\"%s\"}\n",
             CUBALC_LANG_VERSION);
      return 2;
    }
    {
      const char *slash = strrchr(arg, '/');
      const char *leaf = slash ? slash + 1 : arg;
      size_t blen;
      snprintf(stem, sizeof stem, "%s", leaf);
      blen = strlen(stem);
      if (blen > 7 && strcmp(stem + blen - 7, ".cubalc") == 0)
        stem[blen - 7] = 0;
      snprintf(tried[ntry++], sizeof tried[0], "%s", arg);
      if (ntry < 4)
        snprintf(tried[ntry++], sizeof tried[0], "programs/lib/%s.cubalc", stem);
      if (ntry < 4)
        snprintf(tried[ntry++], sizeof tried[0], "programs/lib/%s", arg);
    }
    for (i = 0; i < ntry; i++) {
      if (access(tried[i], R_OK) == 0) {
        snprintf(path, sizeof path, "%s", tried[i]);
        found = 1;
        break;
      }
    }
    if (!found) {
      cubalc_host_result hr;
      if (cubalc_host_find_cubalc(arg, &hr) == 0 && hr.str[0] &&
          access(hr.str, R_OK) == 0) {
        snprintf(path, sizeof path, "%s", hr.str);
        found = 1;
      }
    }
    if (!found) {
      printf("{\"schema\":\"cubalc.recipe.v1\",\"ok\":false,\"cmd\":\"recipe\","
             "\"query\":\"%s\",\"err\":\"not found — cubalc libs|which|LIBINFO\","
             "\"version\":\"%s\"}\n",
             arg, CUBALC_LANG_VERSION);
      return 1;
    }
    f = fopen(path, "rb");
    if (!f) {
      printf("{\"schema\":\"cubalc.recipe.v1\",\"ok\":false,\"cmd\":\"recipe\","
             "\"path\":\"%s\",\"err\":\"open fail\",\"version\":\"%s\"}\n",
             path, CUBALC_LANG_VERSION);
      return 1;
    }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) sz = 0;
    if (sz > CUBALC_MAX_SRC) sz = CUBALC_MAX_SRC;
    buf = malloc((size_t)sz + 1);
    if (!buf) {
      fclose(f);
      printf("{\"schema\":\"cubalc.recipe.v1\",\"ok\":false,\"err\":\"oom\"}\n");
      return 2;
    }
    nr = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[nr] = 0;
    lp = buf;
    lines = 0;
    ndep = 0;
    nknob = 0;
    nhead = 0;
    while (*lp) {
      char raw[512];
      size_t ri = 0;
      while (*lp && *lp != '\n' && ri + 1 < sizeof raw)
        raw[ri++] = *lp++;
      raw[ri] = 0;
      if (*lp == '\n') lp++;
      lines++;
      if (nhead < 8) {
        snprintf(head[nhead], sizeof head[0], "%s", raw);
        nhead++;
      }
      {
        const char *p = raw;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || !*p) continue;
        if (p[0] == 'I' && p[1] == 'N' && p[2] == 'C' && p[3] == 'L' &&
            p[4] == 'U' && p[5] == 'D' && p[6] == 'E' &&
            (p[7] == ' ' || p[7] == '\t' || p[7] == '"' || p[7] == '\'')) {
          char tok[96];
          size_t ti = 0;
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
            while (*p && *p != ' ' && *p != '\t' && *p != '#' && ti + 1 < sizeof tok)
              tok[ti++] = *p++;
            tok[ti] = 0;
          }
          if (tok[0] && ndep < 32) {
            const char *sl = strrchr(tok, '/');
            const char *st = sl ? sl + 1 : tok;
            size_t slen;
            snprintf(deps[ndep], sizeof deps[0], "%s", st);
            slen = strlen(deps[ndep]);
            if (slen > 7 && strcmp(deps[ndep] + slen - 7, ".cubalc") == 0)
              deps[ndep][slen - 7] = 0;
            if (deps[ndep][0]) ndep++;
          }
        } else if (p[0] == 'D' && p[1] == 'E' && p[2] == 'F' && p[3] == 'A' &&
                   p[4] == 'U' && p[5] == 'L' && p[6] == 'T' &&
                   (p[7] == ' ' || p[7] == '\t')) {
          char key[64], val[128], kv[200];
          size_t ki = 0, vi = 0;
          p += 7;
          while (*p == ' ' || *p == '\t') p++;
          while (*p && *p != ' ' && *p != '\t' && *p != '=' && ki + 1 < sizeof key)
            key[ki++] = *p++;
          key[ki] = 0;
          while (*p == ' ' || *p == '\t') p++;
          if (*p == '=') { p++; while (*p == ' ' || *p == '\t') p++; }
          if (*p == '"' || *p == '\'') {
            char q = *p++;
            /* honor \\ and \" so JSON DEFAULT knobs round-trip (PLATE_SEED) */
            while (*p && vi + 1 < sizeof val) {
              if (*p == '\\' && p[1]) {
                p++;
                val[vi++] = *p++;
                continue;
              }
              if (*p == q) break;
              val[vi++] = *p++;
            }
            if (*p == q) p++;
            val[vi] = 0;
          } else {
            while (*p && *p != '#' && vi + 1 < sizeof val) val[vi++] = *p++;
            val[vi] = 0;
            while (vi > 0 && (val[vi - 1] == ' ' || val[vi - 1] == '\t'))
              val[--vi] = 0;
          }
          if (key[0] && nknob < 32) {
            snprintf(kv, sizeof kv, "%s=%s", key, val);
            snprintf(knobs[nknob++], sizeof knobs[0], "%s", kv);
          }
        }
      }
    }
    free(buf);
    {
      int j;
      printf("{\"schema\":\"cubalc.recipe.v1\",\"ok\":true,\"cmd\":\"recipe\","
             "\"stem\":\"%s\",\"path\":\"%s\",\"bytes\":%zu,\"lines\":%d,"
             "\"deps_n\":%d,\"defaults_n\":%d,\"version\":\"%s\","
             "\"note\":\"LIBINFO+LIBDEPS+LIBDEFAULTS+HEADLIB one plate · INCLUDE short name\","
             "\"deps\":[",
             stem, path, nr, lines, ndep, nknob, CUBALC_LANG_VERSION);
      for (j = 0; j < ndep; j++)
        printf("%s\"%s\"", j ? "," : "", deps[j]);
      printf("],\"defaults\":[");
      for (j = 0; j < nknob; j++) {
        char esc[200];
        size_t a = 0, b = 0;
        for (a = 0; knobs[j][a] && b + 2 < sizeof esc; a++) {
          if (knobs[j][a] == '\\' || knobs[j][a] == '"') {
            esc[b++] = '\\';
            esc[b++] = knobs[j][a];
          } else if ((unsigned char)knobs[j][a] >= 32) {
            esc[b++] = knobs[j][a];
          }
        }
        esc[b] = 0;
        printf("%s\"%s\"", j ? "," : "", esc);
      }
      printf("],\"head\":[");
      for (j = 0; j < nhead; j++) {
        char esc[300];
        size_t a = 0, b = 0;
        for (a = 0; head[j][a] && b + 2 < sizeof esc; a++) {
          if (head[j][a] == '\\' || head[j][a] == '"') {
            esc[b++] = '\\';
            esc[b++] = head[j][a];
          } else if (head[j][a] == '\t') {
            esc[b++] = ' ';
          } else if ((unsigned char)head[j][a] >= 32) {
            esc[b++] = head[j][a];
          }
        }
        esc[b] = 0;
        printf("%s\"%s\"", j ? "," : "", esc);
      }
      printf("]}\n");
    }
    return 0;
  
  return 1;
}
