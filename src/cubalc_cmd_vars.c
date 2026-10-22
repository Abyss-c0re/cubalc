/* CubalC CLI env/vars/env-list — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_vars.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int cubalc_cmd_vars(int argc, char **argv) {
  (void)argc; (void)argv;
    /* Usability: host env contract for agents/humans — what CUBALC_* means,
     * default, and whether set (secrets preview only). Complements doctor. */
    static const struct {
      const char *name;
      const char *defv;
      int secret; /* 1 → never print full value */
      const char *hint;
    } catalog[] = {
      {"CUBALC_SMX_KEY", "", 1, "64-hex SMX2 shared secret (openssl rand -hex 32)"},
      {"CUBALC_SMX_KEY_FILE", "", 0, "path to file holding SMX key"},
      {"CUBALC_SMX_TOKEN", "", 1, "alt peer token if SMX_KEY unset"},
      {"CUBALC_P2P_BIND", "127.0.0.1:7733", 0, "SMX SERVE bind host:port"},
      {"CUBALC_P2P_PEER", "", 0, "SMX DIAL peer host:port"},
      {"CUBALC_P2P_SERVE", "", 0, "1 → program prefers SERVE path"},
      {"CUBALC_P2P_TIMEOUT", "30000", 0, "SERVE accept timeout ms (0=forever)"},
      {"CUBALC_P2P_SOFT", "", 0, "1/true → DIAL soft-fail SMX_OK=0"},
      {"CUBALC_PROTECT", "", 0, "1 → tight SPAWN/HTTP host policy"},
      {"CUBALC_STATE", "state", 0, "state plate directory"},
      {"CUBALC_ROOT", "", 0, "install root for INCLUDE resolution"},
      {"CUBALC_INCLUDE_PATH", "", 0, "colon dirs for INCLUDE short names (after programs/lib)"},
      {"CUBALC_PRELOAD", "", 0, "colon lib names auto-INCLUDE ONCE before run body (-I dual)"},
      {"CUBALC_PRELOAD_ACTIVE", "", 0, "set by run to effective -I/PRELOAD list · LISTPRELOAD in-lang"},
      {"CUBALC_DOTENV", "", 0, "colon KEY=VAL plate paths auto-load before run body (-F/--dotenv dual)"},
      {"CUBALC_DOTENV_ACTIVE", "", 0, "set by run to effective dotenv paths after successful load"},
      {"CUBALC_SETENV_N", "", 0, "count of -E/--setenv KEY=VAL injects applied by run"},
      {"CUBALC_SETENV_KEYS", "", 0, "newline bag of keys set by run -E/--setenv"},
      {"CUBALC_UNSETENV_N", "", 0, "count of -U/--unsetenv KEY clears applied by run"},
      {"CUBALC_UNSETENV_KEYS", "", 0, "newline bag of keys cleared by run -U/--unsetenv"},
      {"CUBALC_MKDIR_N", "", 0, "count of -M/--mkdir dirs created/ensured by run"},
      {"CUBALC_MKDIR_DIRS", "", 0, "newline bag of dirs from run -M/--mkdir"},
      {"CUBALC_CWD", "", 0, "chdir before run body (--cwd/--cd dual · relative plates)"},
      {"CUBALC_CWD_ACTIVE", "", 0, "set by run to absolute cwd after successful --cwd"},
      {"CUBALC_REQUIRE_VERSION", "", 0, "x.y[.z] floor for run (-R dual · fail if runtime older)"},
      {"CUBALC_REQUIRE_FORMS", "", 0, "comma form names for run (-C dual · NEEDFORMS floor before body)"},
      {"CUBALC_REQUIRE_FNS", "", 0, "comma FN names for run (-N dual · NEEDFNS after -I preload)"},
      {"CUBALC_REQUIRE_CLASSES", "", 0, "comma CLASS names for run (-K dual · NEEDCLASSES after -I preload)"},
      {"CUBALC_REQUIRE_DOCTOR", "", 0, "1 to require NEEDDOCTOR before run body (-D dual)"},
      {"CUBALC_REQUIRE_READY", "", 0, "1 to require NEEDREADY prove checklist before run body (-Y dual)"},
      {"CUBALC_SEED", "", 0, "RNG seed for reproducible runs"},
      {"CUBALC_QUIET", "", 0, "1 → run plate-only no board noise"},
      {"CUBALC_STRICT", "", 0, "1 → run: soft last_err fails exit + plate ok"},
      {"CUBALC_RUN_TIMEOUT", "", 0, "ms wall budget for run (-T dual · kill runaway loops)"},
      {"CUBALC_ARG0", "", 0, "SYS ARG 0 — script arg without shell glue"},
      {"CUBALC_ARG1", "", 0, "SYS ARG 1"},
      {"CUBALC_ARG2", "", 0, "SYS ARG 2"},
      {"CUBALC_MSG", "", 0, "SYS CHAT default message body"},
      {"CUBALC_HOLD_FLASH", "1", 0, "host default HOLD (language HOLD_FLASH wins)"},
    };
    const char *prefix = (argc > 2) ? argv[2] : "";
    int json_only = 0;
    int i, nmatch = 0, nall = (int)(sizeof catalog / sizeof catalog[0]);
    char pref_up[64];
    if (prefix && (!strcmp(prefix, "--json") || !strcmp(prefix, "-j"))) {
      json_only = 1;
      prefix = (argc > 3) ? argv[3] : "";
    } else if (argc > 3 && (!strcmp(argv[3], "--json") || !strcmp(argv[3], "-j"))) {
      json_only = 1;
    }
    pref_up[0] = 0;
    if (prefix && prefix[0]) {
      size_t k;
      for (k = 0; prefix[k] && k + 1 < sizeof pref_up; k++)
        pref_up[k] = (char)toupper((unsigned char)prefix[k]);
      pref_up[k] = 0;
    }
    if (!json_only) {
      printf("# CubalC host env catalog prefix=%s version=%s\n",
             pref_up[0] ? pref_up : "*", CUBALC_LANG_VERSION);
      printf("# name\tset\tvalue|default\thint\n");
    }
    /* count matches first */
    for (i = 0; i < nall; i++) {
      char name_up[48];
      size_t k;
      for (k = 0; catalog[i].name[k] && k + 1 < sizeof name_up; k++)
        name_up[k] = (char)toupper((unsigned char)catalog[i].name[k]);
      name_up[k] = 0;
      if (pref_up[0] && !strstr(name_up, pref_up))
        continue;
      nmatch++;
    }
    if (!json_only) {
      for (i = 0; i < nall; i++) {
        char name_up[48], show[96];
        size_t k;
        const char *ev;
        int is_set;
        for (k = 0; catalog[i].name[k] && k + 1 < sizeof name_up; k++)
          name_up[k] = (char)toupper((unsigned char)catalog[i].name[k]);
        name_up[k] = 0;
        if (pref_up[0] && !strstr(name_up, pref_up))
          continue;
        ev = getenv(catalog[i].name);
        is_set = (ev && ev[0]) ? 1 : 0;
        show[0] = 0;
        if (is_set) {
          if (catalog[i].secret) {
            if (strlen(ev) >= 8)
              snprintf(show, sizeof show, "%.8s…", ev);
            else
              snprintf(show, sizeof show, "(set)");
          } else {
            snprintf(show, sizeof show, "%s", ev);
          }
        } else if (catalog[i].defv[0]) {
          snprintf(show, sizeof show, "default=%s", catalog[i].defv);
        } else {
          snprintf(show, sizeof show, "(unset)");
        }
        printf("%s\t%s\t%s\t%s\n", catalog[i].name,
               is_set ? "yes" : "no", show, catalog[i].hint);
      }
    }
    printf("{\"schema\":\"cubalc.env.v1\",\"ok\":true,\"cmd\":\"env\","
           "\"prefix\":\"%s\",\"n\":%d,\"n_catalog\":%d,\"version\":\"%s\","
           "\"note\":\"host contract — use SYS ENV NAME OR fallback in programs\","
           "\"vars\":[",
           pref_up[0] ? pref_up : "", nmatch, nall, CUBALC_LANG_VERSION);
    {
      int first = 1;
      for (i = 0; i < nall; i++) {
        char name_up[48], preview[96];
        size_t k;
        const char *ev;
        int is_set;
        for (k = 0; catalog[i].name[k] && k + 1 < sizeof name_up; k++)
          name_up[k] = (char)toupper((unsigned char)catalog[i].name[k]);
        name_up[k] = 0;
        if (pref_up[0] && !strstr(name_up, pref_up))
          continue;
        ev = getenv(catalog[i].name);
        is_set = (ev && ev[0]) ? 1 : 0;
        preview[0] = 0;
        if (is_set) {
          if (catalog[i].secret) {
            if (strlen(ev) >= 8)
              snprintf(preview, sizeof preview, "%.8s…", ev);
            else
              snprintf(preview, sizeof preview, "(set)");
          } else {
            /* escape minimal: drop quotes/backslashes in preview */
            size_t p = 0;
            for (k = 0; ev[k] && p + 1 < sizeof preview; k++) {
              char c = ev[k];
              if (c == '"' || c == '\\' || c < 32) c = '_';
              preview[p++] = c;
            }
            preview[p] = 0;
          }
        }
        printf("%s{\"name\":\"%s\",\"set\":%s,\"preview\":\"%s\","
               "\"default\":\"%s\",\"secret\":%s,\"hint\":\"%s\"}",
               first ? "" : ",", catalog[i].name,
               is_set ? "true" : "false", preview, catalog[i].defv,
               catalog[i].secret ? "true" : "false", catalog[i].hint);
        first = 0;
      }
    }
    printf("]}\n");
    return 0;
}
