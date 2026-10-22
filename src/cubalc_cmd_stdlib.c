/* CubalC CLI libs/lib/stdlib — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_stdlib.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

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

int cubalc_cmd_stdlib(int argc, char **argv) {
  (void)argc; (void)argv;
    /* Usability: list programs/lib + CUBALC_INCLUDE_PATH project libs for agents.
     * Each entry includes stem + deps_n/defaults_n (recipe meta) so agents pick
     * composition without cubalc recipe per lib. */
    static const struct { const char *file; const char *hint; } known[] = {
      {"hold_seed.cubalc", "optional new-device / mesh-join HOLD_FLASH seed"},
      {"agent_boot.cubalc", "REQUIRE 1.15 + VERSION agent preamble (no HOLD_FLASH tax)"},
      {"plate_boot.cubalc", "ENSUREPLATE seed + PLATE var agent state preamble (DEFAULT path/seed)"},
      {"plate_tick.cubalc", "ENSURE+bump+ts+SAVE agent plate tick (DEFAULT path/seed/keys)"},
      {"plate_save.cubalc", "SAVEPLATE PLATE_PATH PLATE agent write-back (dual of plate_boot)"},
      {"plate_patch.cubalc", "MERGEP/JSONFILEMERGE PLATE_PATCH + SAVE (multi-key update INCLUDE)"},
      {"plate_session.cubalc", "agent_boot + plate_boot one INCLUDE (version + durable PLATE)"},
      {"plate_uniform.cubalc", "UNIFORMFLAT nest check via DEFAULT UNIFORM_NEEDLE on PLATE"},
      {"plate_peer.cubalc", "SEEDP INTO PEER multi-plate peer state (DEFAULT PEER_PATH/SEED)"},
      {"plate_peer_save.cubalc", "SAVEP FROM PEER PEER_PATH write-back (dual of plate_peer)"},
      {"plate_peer_session.cubalc", "plate_session + plate_peer one INCLUDE (PLATE + PEER)"},
      {"plate_peer_tick.cubalc", "SEEDP+INCP+ts+SAVEP PEER tick (dual of plate_tick)"},
      {"plate_both_save.cubalc", "plate_save + plate_peer_save one INCLUDE (multi-plate write-back)"},
      {"peer_decide.cubalc", "FOLDBITS/SETDIGIT peer0 then DECIDE brain"},
      {"mesh_exchange.cubalc", "SMX KEY + dual EXCHANGE peer0/peer1"},
      {"var_guard.cubalc", "NEEDVARROOM free-slot gate · DEFAULT NEED_VARROOM · soft VAR_GUARD_SOFT"},
      {"time_guard.cubalc", "NEEDTIME wall budget gate · TIME_GUARD_OK · soft TIME_GUARD_SOFT"},
      {"fat_boot.cubalc", "agent_boot + var_guard one-shot · fat nest preamble · init --fat"},
      {"fat_session.cubalc",
       "fat_boot + plate_boot durable nest · init --fat-session · cubalc recipe"},
      {"doctor_boot.cubalc", "agent_boot + NEEDDOCTOR hard install gate · init --doctor"},
      {"ready_boot.cubalc", "agent_boot + NEEDREADY prove checklist · init --ready · run -Y"},
      {"cli_boot.cubalc", "agent_boot + HELPFLAG + VERSIONFLAG + CLIINFO · init --cli"},
      {"cli_guard.cubalc", "NEED_FLAGS + NEED_REST_N CLI contract · soft CLI_GUARD_SOFT · form_guard twin"},
      {"cli_session.cubalc", "cli_boot + cli_guard one-shot · init --cli-session"},
      {"arg_guard.cubalc", "NEED_ARGS + NEED_ARG_ANY program-arg contract · soft ARG_GUARD_SOFT"},
      {"arg_boot.cubalc", "agent_boot + arg_guard one-shot · init --arg · positionals"},
      {"time_boot.cubalc", "agent_boot + time_guard one-shot · init --time · wall budget"},
      {"tool_session.cubalc", "cli_boot + cli_guard + arg_guard · init --full-cli full CLI"},
      {"product_session.cubalc", "tool_session + env_guard + time_guard · init --product product CLI"},
      {"path_guard.cubalc", "NEED_PATHS + NEED_PATH_ANY host path contract · soft PATH_GUARD_SOFT"},
      {"path_boot.cubalc", "agent_boot + path_guard one-shot · init --path · host layout"},
      {"rw_guard.cubalc", "NEED_READ_PATHS + NEED_WRITE_PATHS access · soft RW_GUARD_SOFT"},
      {"rw_boot.cubalc", "agent_boot + rw_guard one-shot · init --rw · host access"},
      {"bin_guard.cubalc", "NEED_BINS + NEED_BIN_ANY PATH tools · soft BIN_GUARD_SOFT"},
      {"bin_boot.cubalc", "agent_boot + bin_guard one-shot · init --bin · PATH tools"},
      {"lib_guard.cubalc", "NEED_LIBS + NEED_LIB_ANY module gate · soft LIB_GUARD_SOFT"},
      {"lib_boot.cubalc", "agent_boot + lib_guard one-shot · init --lib-boot · modules"},
      {"host_session.cubalc", "path+rw+bin+lib guards · HOST_SESSION_OK · product_session twin"},
      {"host_boot.cubalc", "agent_boot + host_session one-shot · init --host host readiness"},
      {"full_session.cubalc", "product_session + host_session · FULL_SESSION_OK fleet contract"},
      {"full_boot.cubalc", "full_session one-shot · init --fleet product+host starter"},
      {"plate_guard.cubalc", "NEED_KEYS + NEED_KEY_ANY plate schema · soft PLATE_GUARD_SOFT"},
      {"key_boot.cubalc", "agent_boot + plate_boot + plate_guard · init --key schema"},
      {"fn_guard.cubalc", "NEED_FNALL + NEED_FNANY FN catalog · soft FN_GUARD_SOFT"},
      {"fn_boot.cubalc", "agent_boot + fn_guard one-shot · init --fn FN catalog"},
      {"class_guard.cubalc", "NEED_CLASSES + NEED_CLASS_ANY OOP catalog · soft CLASS_GUARD_SOFT"},
      {"class_boot.cubalc", "agent_boot + class_guard one-shot · init --class CLASS catalog"},
      {"method_guard.cubalc", "NEED_METHODS + NEED_METHOD_ANY Class.method · soft METHOD_GUARD_SOFT"},
      {"method_boot.cubalc", "agent_boot + method_guard one-shot · init --method METHOD catalog"},
      {"field_guard.cubalc", "NEED_FIELDS + NEED_FIELD_ANY Class.field · soft FIELD_GUARD_SOFT"},
      {"field_boot.cubalc", "agent_boot + field_guard one-shot · init --field FIELD catalog"},
      {"oop_session.cubalc", "class+method+field guards · OOP_SESSION_OK · soft *_GUARD_SOFT"},
      {"oop_boot.cubalc", "agent_boot + oop_session one-shot · init --oop OOP catalog"},
      {"obj_guard.cubalc", "NEED_OBJS + NEED_OBJ_ANY live instances · soft OBJ_GUARD_SOFT"},
      {"obj_boot.cubalc", "agent_boot + obj_guard one-shot · init --obj live OBJ catalog"},
      {"live_session.cubalc", "oop_session + obj_guard · LIVE_SESSION_OK · soft *_GUARD_SOFT"},
      {"live_boot.cubalc", "agent_boot + live_session one-shot · init --live live OOP"},
      {"env_guard.cubalc", "NEED_ENVS + NEED_ENV_ANY host env contract · soft ENV_GUARD_SOFT"},
      {"env_boot.cubalc", "agent_boot + env_guard one-shot · init --env · host config"},
    };
    const char *libdir = "programs/lib";
    const char *ip = getenv("CUBALC_INCLUDE_PATH");
    const char *filter = "";
    char paths[64][160];
    char hints[64][120];
    char origins[64][24];
    char stems[64][96];
    int deps_n[64], defaults_n[64];
    int keep[64];
    char ipdirs[8][160];
    int n = 0, n_ip = 0, n_std = 0, i, j, n_total = 0, n_match = 0;
    DIR *d;
    /* optional stem/prefix filter: cubalc libs plate · libs --filter fat · libs -q tick */
    for (i = 2; i < argc; i++) {
      if (!strcmp(argv[i], "--filter") || !strcmp(argv[i], "-q") ||
          !strcmp(argv[i], "--query") || !strcmp(argv[i], "--match") ||
          !strcmp(argv[i], "--stem") || !strcmp(argv[i], "--prefix")) {
        if (i + 1 < argc && argv[i + 1][0] != '-')
          filter = argv[++i];
      } else if (!strncmp(argv[i], "--filter=", 9) && argv[i][9]) {
        filter = argv[i] + 9;
      } else if (!strncmp(argv[i], "--query=", 8) && argv[i][8]) {
        filter = argv[i] + 8;
      } else if (argv[i][0] != '-') {
        filter = argv[i];
      }
    }
    d = opendir(libdir);
    if (d) {
      struct dirent *de;
      while ((de = readdir(d)) != NULL && n < 64) {
        size_t len = strlen(de->d_name);
        size_t slen;
        if (len < 8 || strcmp(de->d_name + len - 7, ".cubalc") != 0)
          continue;
        if (de->d_name[0] == '.') continue;
        snprintf(paths[n], sizeof paths[n], "%s/%s", libdir, de->d_name);
        snprintf(origins[n], sizeof origins[n], "%s", "stdlib");
        snprintf(stems[n], sizeof stems[n], "%s", de->d_name);
        slen = strlen(stems[n]);
        if (slen > 7 && strcmp(stems[n] + slen - 7, ".cubalc") == 0)
          stems[n][slen - 7] = 0;
        hints[n][0] = 0;
        for (j = 0; j < (int)(sizeof known / sizeof known[0]); j++) {
          if (strcmp(de->d_name, known[j].file) == 0) {
            snprintf(hints[n], sizeof hints[n], "%s", known[j].hint);
            break;
          }
        }
        if (!hints[n][0])
          snprintf(hints[n], sizeof hints[n], "INCLUDE lib snippet");
        lib_count_recipe_meta(paths[n], &deps_n[n], &defaults_n[n]);
        n++;
        n_std++;
      }
      closedir(d);
    }
    /* CUBALC_INCLUDE_PATH dirs — project INCLUDE short names (after programs/lib). */
    if (ip && ip[0]) {
      const char *seg = ip;
      while (*seg && n < 64) {
        char dir[160];
        size_t dlen = 0;
        DIR *pd;
        while (*seg == ':') seg++;
        if (!*seg) break;
        while (seg[dlen] && seg[dlen] != ':' && dlen + 1 < sizeof dir)
          dir[dlen] = seg[dlen], dlen++;
        dir[dlen] = 0;
        seg += dlen;
        if (!dir[0]) continue;
        if (n_ip < 8)
          snprintf(ipdirs[n_ip++], sizeof ipdirs[0], "%s", dir);
        pd = opendir(dir);
        if (!pd) continue;
        {
          struct dirent *de;
          while ((de = readdir(pd)) != NULL && n < 64) {
            size_t len = strlen(de->d_name);
            size_t slen;
            int already = 0;
            if (len < 8 || strcmp(de->d_name + len - 7, ".cubalc") != 0)
              continue;
            if (de->d_name[0] == '.') continue;
            snprintf(paths[n], sizeof paths[n], "%s/%s", dir, de->d_name);
            for (j = 0; j < n; j++) {
              const char *b1 = strrchr(paths[j], '/');
              b1 = b1 ? b1 + 1 : paths[j];
              if (strcmp(b1, de->d_name) == 0) { already = 1; break; }
            }
            if (already) continue;
            snprintf(origins[n], sizeof origins[n], "%s", "include_path");
            snprintf(hints[n], sizeof hints[n],
                     "CUBALC_INCLUDE_PATH project lib");
            snprintf(stems[n], sizeof stems[n], "%s", de->d_name);
            slen = strlen(stems[n]);
            if (slen > 7 && strcmp(stems[n] + slen - 7, ".cubalc") == 0)
              stems[n][slen - 7] = 0;
            lib_count_recipe_meta(paths[n], &deps_n[n], &defaults_n[n]);
            n++;
          }
        }
        closedir(pd);
      }
    }
    /* stable sort by path */
    for (i = 0; i < n; i++) {
      for (j = i + 1; j < n; j++) {
        if (strcmp(paths[j], paths[i]) < 0) {
          char tp[160], th[120], to[24], ts[96];
          int td, tk;
          snprintf(tp, sizeof tp, "%s", paths[i]);
          snprintf(th, sizeof th, "%s", hints[i]);
          snprintf(to, sizeof to, "%s", origins[i]);
          snprintf(ts, sizeof ts, "%s", stems[i]);
          td = deps_n[i];
          tk = defaults_n[i];
          snprintf(paths[i], sizeof paths[i], "%s", paths[j]);
          snprintf(hints[i], sizeof hints[i], "%s", hints[j]);
          snprintf(origins[i], sizeof origins[i], "%s", origins[j]);
          snprintf(stems[i], sizeof stems[i], "%s", stems[j]);
          deps_n[i] = deps_n[j];
          defaults_n[i] = defaults_n[j];
          snprintf(paths[j], sizeof paths[j], "%s", tp);
          snprintf(hints[j], sizeof hints[j], "%s", th);
          snprintf(origins[j], sizeof origins[j], "%s", to);
          snprintf(stems[j], sizeof stems[j], "%s", ts);
          deps_n[j] = td;
          defaults_n[j] = tk;
        }
      }
    }
    n_total = n;
    /* filter by stem/name/path/hint (case-insensitive substring) */
    n_match = 0;
    for (i = 0; i < n; i++) {
      keep[i] = 1;
      if (filter && filter[0]) {
        char fup[96], hay[256];
        size_t a, b;
        int hit = 0;
        for (a = 0; filter[a] && a + 1 < sizeof fup; a++) {
          char c = filter[a];
          if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
          fup[a] = c;
        }
        fup[a] = 0;
        snprintf(hay, sizeof hay, "%s\n%s\n%s\n%s",
                 stems[i], paths[i], origins[i], hints[i]);
        for (b = 0; hay[b]; b++)
          if (hay[b] >= 'A' && hay[b] <= 'Z')
            hay[b] = (char)(hay[b] - 'A' + 'a');
        if (fup[0] && strstr(hay, fup))
          hit = 1;
        keep[i] = hit;
      }
      if (keep[i]) n_match++;
    }
    if (filter && filter[0])
      printf("# CubalC INCLUDE catalog filter=%s n=%d/%d stdlib=%d include_path_n=%d version=%s\n",
             filter, n_match, n_total, n_std, n_ip, CUBALC_LANG_VERSION);
    else
      printf("# CubalC INCLUDE catalog n=%d stdlib=%d include_path_n=%d version=%s\n",
             n_total, n_std, n_ip, CUBALC_LANG_VERSION);
    printf("# cols: path origin stem deps_n defaults_n hint · cubalc libs [filter] · recipe|checkdeps\n");
    for (i = 0; i < n; i++) {
      if (!keep[i]) continue;
      printf("%s\t%s\t%s\t%d\t%d\t%s\n",
             paths[i], origins[i], stems[i], deps_n[i], defaults_n[i], hints[i]);
    }
    printf("{\"schema\":\"cubalc.libs.v1\",\"ok\":%s,\"cmd\":\"libs\","
           "\"dir\":\"%s\",\"n\":%d,\"n_total\":%d,\"n_match\":%d,"
           "\"n_stdlib\":%d,\"n_include_path\":%d,"
           "\"filter\":\"%s\",\"include_path_set\":%s,\"version\":\"%s\","
           "\"note\":\"stem+deps_n+defaults_n · optional filter stem/path/hint\","
           "\"include_from\":\"programs/lib + CUBALC_INCLUDE_PATH\","
           "\"include_path_dirs\":[",
           n_match > 0 ? "true" : "false", libdir, n_match, n_total, n_match,
           n_std, n - n_std,
           filter && filter[0] ? filter : "",
           n_ip > 0 ? "true" : "false", CUBALC_LANG_VERSION);
    for (i = 0; i < n_ip; i++) {
      char pesc[180];
      size_t k, o = 0;
      for (k = 0; ipdirs[i][k] && o + 2 < sizeof pesc; k++) {
        char c = ipdirs[i][k];
        if (c == '"' || c == '\\') pesc[o++] = '_';
        else pesc[o++] = c;
      }
      pesc[o] = 0;
      printf("%s\"%s\"", i ? "," : "", pesc);
    }
    printf("],\"libs\":[");
    {
      int first = 1;
      for (i = 0; i < n; i++) {
        const char *base;
        char pesc[200];
        size_t k, o = 0;
        if (!keep[i]) continue;
        base = strrchr(paths[i], '/');
        base = base ? base + 1 : paths[i];
        for (k = 0; paths[i][k] && o + 2 < sizeof pesc; k++) {
          char c = paths[i][k];
          if (c == '"' || c == '\\') pesc[o++] = '_';
          else pesc[o++] = c;
        }
        pesc[o] = 0;
        printf("%s{\"path\":\"%s\",\"name\":\"%s\",\"stem\":\"%s\","
               "\"deps_n\":%d,\"defaults_n\":%d,\"origin\":\"%s\",\"hint\":\"%s\"}",
               first ? "" : ",", pesc, base, stems[i], deps_n[i], defaults_n[i],
               origins[i], hints[i]);
        first = 0;
      }
    }
    printf("]}\n");
    return n_match > 0 ? 0 : 1;
}
