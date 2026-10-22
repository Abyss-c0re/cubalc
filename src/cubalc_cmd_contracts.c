/* CubalC CLI contracts — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_contracts.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

static void lib_count_recipe_meta(const char *path, int *ndep_out, int *nknob_out) {
  FILE *f;
  char *src = NULL;
  long sz = 0;
  size_t nr = 0;
  const char *lp;
  int ndep = 0, nknob = 0;
  if (ndep_out) *ndep_out = 0;
  if (nknob_out) *nknob_out = 0;
  if (!path || !path[0]) return;
  f = fopen(path, "rb");
  if (!f) return;
  fseek(f, 0, SEEK_END);
  sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (sz < 0) sz = 0;
  if (sz > 256 * 1024) sz = 256 * 1024;
  src = malloc((size_t)sz + 1);
  if (!src) { fclose(f); return; }
  nr = fread(src, 1, (size_t)sz, f);
  fclose(f);
  src[nr] = 0;
  lp = src;
  while (*lp) {
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
      if (*p && *p != '#' && *p != '\r')
        ndep++;
    } else if (p[0] == 'D' && p[1] == 'E' && p[2] == 'F' && p[3] == 'A' &&
               p[4] == 'U' && p[5] == 'L' && p[6] == 'T' &&
               (p[7] == ' ' || p[7] == '\t')) {
      nknob++;
    }
  }
  free(src);
  if (ndep_out) *ndep_out = ndep;
  if (nknob_out) *nknob_out = nknob;
}

int cubalc_cmd_contracts(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "contracts";
    /* Usability: catalog *guard* INCLUDE contracts for agents — NEED_* knobs,
     * soft flags, HARDFAIL hard path note. Twin of cubalc libs filtered to guards. */
    static const struct { const char *stem; const char *plane; const char *need; const char *soft; const char *okv; } known[] = {
      {"var_guard", "varroom", "NEED_VARROOM", "VAR_GUARD_SOFT", "TIME/VAR status"},
      {"time_guard", "time", "NEED_TIME", "TIME_GUARD_SOFT", "TIME_GUARD_OK"},
      {"form_guard", "forms", "NEED_FORMS / NEED_FORM_ANY", "FORM_GUARD_SOFT", "FORM_GUARD_OK"},
      {"cli_guard", "cli", "NEED_FLAGS / NEED_FLAG_ANY / NEED_REST_N", "CLI_GUARD_SOFT", "CLI_GUARD_OK"},
      {"arg_guard", "args", "NEED_ARGS / NEED_ARG_ANY", "ARG_GUARD_SOFT", "ARG_GUARD_OK"},
      {"env_guard", "env", "NEED_ENVS / NEED_ENV_ANY", "ENV_GUARD_SOFT", "ENV_GUARD_OK"},
      {"path_guard", "path", "NEED_PATHS / NEED_PATH_ANY", "PATH_GUARD_SOFT", "PATH_GUARD_OK"},
      {"rw_guard", "access", "NEED_READ_PATHS / NEED_WRITE_PATHS", "RW_GUARD_SOFT", "RW_GUARD_OK"},
      {"bin_guard", "bin", "NEED_BINS / NEED_BIN_ANY", "BIN_GUARD_SOFT", "BIN_GUARD_OK"},
      {"lib_guard", "lib", "NEED_LIBS / NEED_LIB_ANY", "LIB_GUARD_SOFT", "LIB_GUARD_OK"},
      {"plate_guard", "plate", "NEED_KEYS / NEED_KEY_ANY", "PLATE_GUARD_SOFT", "PLATE_GUARD_OK"},
      {"fn_guard", "fn", "NEED_FNALL / NEED_FNANY", "FN_GUARD_SOFT", "FN_GUARD_OK"},
      {"class_guard", "oop", "NEED_CLASSES / NEED_CLASS_ANY", "CLASS_GUARD_SOFT", "CLASS_GUARD_OK"},
      {"method_guard", "oop", "NEED_METHODS / NEED_METHOD_ANY Class.method", "METHOD_GUARD_SOFT", "METHOD_GUARD_OK"},
      {"field_guard", "oop", "NEED_FIELDS / NEED_FIELD_ANY Class.field", "FIELD_GUARD_SOFT", "FIELD_GUARD_OK"},
      {"obj_guard", "oop", "NEED_OBJS / NEED_OBJ_ANY live instances", "OBJ_GUARD_SOFT", "OBJ_GUARD_OK"},
    };
    const char *libdir = "programs/lib";
    const char *filter = "";
    char stems[48][64];
    char paths[48][160];
    char planes[48][24];
    char needs[48][96];
    char softs[48][40];
    char oks[48][40];
    int hardfail[48];
    int defaults_n[48];
    int keep[48];
    int n = 0, i, j, n_match = 0;
    DIR *d;
    for (i = 2; i < argc; i++) {
      if (!strcmp(argv[i], "--filter") || !strcmp(argv[i], "-q") ||
          !strcmp(argv[i], "--query") || !strcmp(argv[i], "--match") ||
          !strcmp(argv[i], "--plane") || !strcmp(argv[i], "--prefix")) {
        if (i + 1 < argc && argv[i + 1][0] != '-')
          filter = argv[++i];
      } else if (!strncmp(argv[i], "--filter=", 9) && argv[i][9]) {
        filter = argv[i] + 9;
      } else if (argv[i][0] != '-') {
        filter = argv[i];
      }
    }
    d = opendir(libdir);
    if (d) {
      struct dirent *de;
      while ((de = readdir(d)) != NULL && n < 48) {
        size_t len = strlen(de->d_name);
        size_t slen;
        char stem[64];
        int known_i = -1;
        FILE *f;
        char line[240];
        if (len < 8 || strcmp(de->d_name + len - 7, ".cubalc") != 0)
          continue;
        if (de->d_name[0] == '.') continue;
        if (!strstr(de->d_name, "guard"))
          continue;
        snprintf(stem, sizeof stem, "%s", de->d_name);
        slen = strlen(stem);
        if (slen > 7) stem[slen - 7] = 0;
        snprintf(stems[n], sizeof stems[n], "%s", stem);
        snprintf(paths[n], sizeof paths[n], "%s/%s", libdir, de->d_name);
        snprintf(planes[n], sizeof planes[n], "%s", "contract");
        snprintf(needs[n], sizeof needs[n], "%s", "DEFAULT NEED_*");
        snprintf(softs[n], sizeof softs[n], "%s", "*_GUARD_SOFT");
        snprintf(oks[n], sizeof oks[n], "%s", "*_GUARD_OK");
        hardfail[n] = 0;
        defaults_n[n] = 0;
        for (j = 0; j < (int)(sizeof known / sizeof known[0]); j++) {
          if (!strcmp(stem, known[j].stem)) {
            known_i = j;
            snprintf(planes[n], sizeof planes[n], "%s", known[j].plane);
            snprintf(needs[n], sizeof needs[n], "%s", known[j].need);
            snprintf(softs[n], sizeof softs[n], "%s", known[j].soft);
            snprintf(oks[n], sizeof oks[n], "%s", known[j].okv);
            break;
          }
        }
        (void)known_i;
        lib_count_recipe_meta(paths[n], &j, &defaults_n[n]);
        f = fopen(paths[n], "r");
        if (f) {
          while (fgets(line, sizeof line, f)) {
            if (strstr(line, "HARDFAIL"))
              hardfail[n] = 1;
          }
          fclose(f);
        }
        n++;
      }
      closedir(d);
    }
    /* sort by stem */
    for (i = 0; i < n; i++) {
      for (j = i + 1; j < n; j++) {
        if (strcmp(stems[j], stems[i]) < 0) {
          char ts[64], tp[160], tplane[24], tneed[96], tsoft[40], tok[40];
          int th, td;
          snprintf(ts, sizeof ts, "%s", stems[i]);
          snprintf(tp, sizeof tp, "%s", paths[i]);
          snprintf(tplane, sizeof tplane, "%s", planes[i]);
          snprintf(tneed, sizeof tneed, "%s", needs[i]);
          snprintf(tsoft, sizeof tsoft, "%s", softs[i]);
          snprintf(tok, sizeof tok, "%s", oks[i]);
          th = hardfail[i]; td = defaults_n[i];
          snprintf(stems[i], sizeof stems[i], "%s", stems[j]);
          snprintf(paths[i], sizeof paths[i], "%s", paths[j]);
          snprintf(planes[i], sizeof planes[i], "%s", planes[j]);
          snprintf(needs[i], sizeof needs[i], "%s", needs[j]);
          snprintf(softs[i], sizeof softs[i], "%s", softs[j]);
          snprintf(oks[i], sizeof oks[i], "%s", oks[j]);
          hardfail[i] = hardfail[j]; defaults_n[i] = defaults_n[j];
          snprintf(stems[j], sizeof stems[j], "%s", ts);
          snprintf(paths[j], sizeof paths[j], "%s", tp);
          snprintf(planes[j], sizeof planes[j], "%s", tplane);
          snprintf(needs[j], sizeof needs[j], "%s", tneed);
          snprintf(softs[j], sizeof softs[j], "%s", tsoft);
          snprintf(oks[j], sizeof oks[j], "%s", tok);
          hardfail[j] = th; defaults_n[j] = td;
        }
      }
    }
    n_match = 0;
    for (i = 0; i < n; i++) {
      keep[i] = 1;
      if (filter && filter[0]) {
        char fup[96], hay[320];
        size_t a, b;
        for (a = 0; filter[a] && a + 1 < sizeof fup; a++) {
          char c = filter[a];
          if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
          fup[a] = c;
        }
        fup[a] = 0;
        snprintf(hay, sizeof hay, "%s\n%s\n%s\n%s\n%s",
                 stems[i], planes[i], needs[i], softs[i], paths[i]);
        for (b = 0; hay[b]; b++)
          if (hay[b] >= 'A' && hay[b] <= 'Z')
            hay[b] = (char)(hay[b] - 'A' + 'a');
        keep[i] = (fup[0] && strstr(hay, fup)) ? 1 : 0;
      }
      if (keep[i]) n_match++;
    }
    if (filter && filter[0])
      printf("# CubalC guards filter=%s n=%d/%d version=%s\n",
             filter, n_match, n, CUBALC_LANG_VERSION);
    else
      printf("# CubalC contract guards n=%d version=%s\n", n, CUBALC_LANG_VERSION);
    printf("# cols: stem plane need soft ok hardfail defaults_n path\n");
    printf("# hard path: HARDFAIL (halt exit 1) · soft: *_GUARD_SOFT=1 · recipe stem\n");
    for (i = 0; i < n; i++) {
      if (!keep[i]) continue;
      printf("%s\t%s\t%s\t%s\t%s\t%s\t%d\t%s\n",
             stems[i], planes[i], needs[i], softs[i], oks[i],
             hardfail[i] ? "yes" : "no", defaults_n[i], paths[i]);
    }
    printf("{\"schema\":\"cubalc.guards.v1\",\"ok\":%s,\"cmd\":\"guards\","
           "\"n\":%d,\"n_total\":%d,\"filter\":\"%s\",\"version\":\"%s\","
           "\"hardfail_default\":true,"
           "\"note\":\"INCLUDE {stem} after DEFAULT NEED_* · HARDFAIL hard · SOFT=1 soft bags\","
           "\"guards\":[",
           n_match > 0 ? "true" : "false", n_match, n,
           filter && filter[0] ? filter : "", CUBALC_LANG_VERSION);
    {
      int first = 1;
      for (i = 0; i < n; i++) {
        if (!keep[i]) continue;
        printf("%s{\"stem\":\"%s\",\"plane\":\"%s\",\"need\":\"%s\","
               "\"soft\":\"%s\",\"ok\":\"%s\",\"hardfail\":%s,"
               "\"defaults_n\":%d,\"path\":\"%s\"}",
               first ? "" : ",", stems[i], planes[i], needs[i], softs[i], oks[i],
               hardfail[i] ? "true" : "false", defaults_n[i], paths[i]);
        first = 0;
      }
    }
    printf("]}\n");
    return n_match > 0 ? 0 : 1;
}
