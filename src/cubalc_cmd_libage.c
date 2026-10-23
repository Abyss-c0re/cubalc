/* CubalC CLI libage/hasfresh/needfresh — F_main_carve peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_libage.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

int cubalc_cmd_libage(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "libage";

    /* Usability: CLI dual of LIBAGE/HASFRESH/NEEDFRESH — single-lib age + freshness
     * gates without .cubalc. cubalc libage fat_session · hasfresh fat_session 86400
     * Schema cubalc.libmatch.v1 · mode age|has|need */
    const char *name = (argc > 2) ? argv[2] : "";
    long max_age = 86400;
    int mode_age = 0, mode_need = 0;
    char path[768], base[160];
    struct stat st;
    time_t now = time(NULL);
    long age = -1, mtime = 0;
    int ok = 0, fresh = 0;
    path[0] = 0;
    base[0] = 0;
    if (strcmp(cmd, "libage") == 0 || strcmp(cmd, "ageoflib") == 0)
      mode_age = 1;
    else if (strcmp(cmd, "needfresh") == 0 || strcmp(cmd, "requirefresh") == 0)
      mode_need = 1;
    if (!mode_age && argc > 3 && argv[3][0]) {
      char *end = NULL;
      long v = strtol(argv[3], &end, 10);
      if (end && end != argv[3] && !*end && v >= 0)
        max_age = v;
    }
    if (!name || !name[0]) {
      fprintf(stderr,
              "usage: cubalc libage|ageoflib <lib>\n"
              "       cubalc hasfresh|isfresh <lib> [max_age_sec]\n"
              "       cubalc needfresh|requirefresh <lib> [max_age_sec]\n"
              "  default max_age: 86400 (1 day)\n");
      printf("{\"schema\":\"cubalc.libmatch.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need lib name\",\"version\":\"%s\"}\n",
             cmd, CUBALC_LANG_VERSION);
      return 2;
    }
    /* resolve like INCLUDE short name */
    {
      size_t blen;
      const char *slash = strrchr(name, '/');
      const char *leaf = slash ? slash + 1 : name;
      const char *ip, *root;
      FILE *f = NULL;
      snprintf(base, sizeof base, "%s", leaf);
      blen = strlen(base);
      if (blen > 7 && strcmp(base + blen - 7, ".cubalc") == 0)
        base[blen - 7] = 0;
      if (name[0] == '/' || strchr(name, '/')) {
        f = fopen(name, "rb");
        if (f) { snprintf(path, sizeof path, "%s", name); fclose(f); }
      }
      if (!path[0]) {
        char p3[768];
        snprintf(p3, sizeof p3, "programs/lib/%s.cubalc", base);
        f = fopen(p3, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p3); fclose(f); }
      }
      root = getenv("CUBALC_ROOT");
      if (!path[0] && root && root[0]) {
        char p2[768];
        snprintf(p2, sizeof p2, "%s/programs/lib/%s.cubalc", root, base);
        f = fopen(p2, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p2); fclose(f); }
      }
      ip = getenv("CUBALC_INCLUDE_PATH");
      if (!path[0] && ip && ip[0]) {
        const char *seg = ip;
        while (*seg && !path[0]) {
          char dir[512], p3[768];
          size_t dlen = 0;
          while (*seg == ':') seg++;
          if (!*seg) break;
          while (seg[dlen] && seg[dlen] != ':' && dlen + 1 < sizeof dir)
            dir[dlen] = seg[dlen], dlen++;
          dir[dlen] = 0;
          seg += dlen;
          if (!dir[0]) continue;
          snprintf(p3, sizeof p3, "%s/%s.cubalc", dir, base);
          f = fopen(p3, "rb");
          if (f) { snprintf(path, sizeof path, "%s", p3); fclose(f); }
        }
      }
    }
    if (path[0] && stat(path, &st) == 0) {
      mtime = (long)st.st_mtime;
      age = (long)now - mtime;
      if (age < 0) age = 0;
      ok = 1;
      fresh = (age <= max_age) ? 1 : 0;
    }
    if (mode_age) {
      printf("{\"schema\":\"cubalc.libmatch.v1\",\"ok\":%s,\"cmd\":\"%s\","
             "\"mode\":\"age\",\"stem\":\"%s\",\"path\":\"%s\","
             "\"mtime\":%ld,\"age\":%ld,\"version\":\"%s\","
             "\"note\":\"CLI dual of LIBAGE · seconds since mtime\"}\n",
             ok ? "true" : "false", cmd, base[0] ? base : name,
             path, mtime, age, CUBALC_LANG_VERSION);
      return ok ? 0 : 1;
    }
    {
      int gate_ok = ok && fresh;
      printf("{\"schema\":\"cubalc.libmatch.v1\",\"ok\":%s,\"cmd\":\"%s\","
             "\"mode\":\"%s\",\"stem\":\"%s\",\"path\":\"%s\","
             "\"mtime\":%ld,\"age\":%ld,\"max_age\":%ld,\"fresh\":%s,"
             "\"version\":\"%s\","
             "\"note\":\"CLI dual of HASFRESH/NEEDFRESH · single-lib freshness gate\"}\n",
             gate_ok ? "true" : "false", cmd,
             mode_need ? "need" : "has",
             base[0] ? base : name, path, mtime, age, max_age,
             fresh ? "true" : "false", CUBALC_LANG_VERSION);
      if (mode_need)
        return gate_ok ? 0 : 1;
      /* hasfresh always exit 0 with ok reflecting freshness (agents parse plate) */
      return 0;
    }
  
  return 1;
}
