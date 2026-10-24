/* CubalC CLI cat/type/source — F_main_carve peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_cat.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include "cubalc_law.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int cubalc_cmd_cat(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "cat";
  if (strcmp(cmd, "cat") == 0 || strcmp(cmd, "type") == 0 ||
      strcmp(cmd, "read-lib") == 0 || strcmp(cmd, "source") == 0) {
    /* Usability: dump lib/program source for agents without a separate FS browser.
     * Resolve order mirrors INCLUDE: path · programs/lib/<name>.cubalc · programs/…
     * Note: "show" is already cubes alias — use cat|type|source. */
    const char *arg = (argc > 2) ? argv[2] : "";
    char path[768];
    char tried[3][768];
    int ntry = 0, found = 0;
    FILE *f = NULL;
    long sz;
    char *buf;
    size_t nr;
    int meta_only = 0;
    int i;
    if (!arg || !arg[0]) {
      fprintf(stderr, "usage: cubalc cat <libname|path.cubalc>\n"
                      "       cubalc cat hold_seed · cubalc type programs/hello_cube.cubalc\n");
      printf("{\"schema\":\"cubalc.cat.v1\",\"ok\":false,\"cmd\":\"cat\","
             "\"err\":\"need libname or path\",\"version\":\"%s\"}\n",
             CUBALC_LANG_VERSION);
      return 2;
    }
    if (argc > 3 && (!strcmp(argv[3], "--meta") || !strcmp(argv[3], "-m") ||
                     !strcmp(argv[2], "--meta"))) {
      meta_only = 1;
    }
    if (!strcmp(arg, "--meta") || !strcmp(arg, "-m")) {
      fprintf(stderr, "usage: cubalc cat <libname|path> [--meta]\n");
      return 2;
    }
    /* candidate paths (INCLUDE-style + CUBALC_INCLUDE_PATH via host find) */
    snprintf(tried[ntry++], sizeof tried[0], "%s", arg);
    {
      char base[256];
      const char *slash = strrchr(arg, '/');
      const char *leaf = slash ? slash + 1 : arg;
      size_t blen;
      snprintf(base, sizeof base, "%s", leaf);
      blen = strlen(base);
      if (blen > 7 && strcmp(base + blen - 7, ".cubalc") == 0)
        base[blen - 7] = 0;
      if (ntry < 3)
        snprintf(tried[ntry++], sizeof tried[0], "programs/lib/%s.cubalc", base);
      if (ntry < 3)
        snprintf(tried[ntry++], sizeof tried[0], "programs/%s", arg);
    }
    for (i = 0; i < ntry; i++) {
      if (access(tried[i], R_OK) == 0) {
        snprintf(path, sizeof path, "%s", tried[i]);
        found = 1;
        break;
      }
    }
    /* Usability: project libs on CUBALC_INCLUDE_PATH (same as INCLUDE/which). */
    if (!found) {
      cubalc_host_result hr;
      if (cubalc_host_find_cubalc(arg, &hr) == 0 && hr.str[0] &&
          access(hr.str, R_OK) == 0) {
        snprintf(path, sizeof path, "%s", hr.str);
        found = 1;
      }
    }
    if (!found) {
      printf("{\"schema\":\"cubalc.cat.v1\",\"ok\":false,\"cmd\":\"cat\","
             "\"query\":\"%s\",\"err\":\"not found — try cubalc libs|which|examples\","
             "\"version\":\"%s\",\"tried\":[",
             arg, CUBALC_LANG_VERSION);
      for (i = 0; i < ntry; i++)
        printf("%s\"%s\"", i ? "," : "", tried[i]);
      printf("],\"hint\":\"CUBALC_INCLUDE_PATH · cubalc which name\"}\n");
      return 1;
    }
    f = fopen(path, "rb");
    if (!f) {
      printf("{\"schema\":\"cubalc.cat.v1\",\"ok\":false,\"cmd\":\"cat\","
             "\"path\":\"%s\",\"err\":\"open fail\",\"version\":\"%s\"}\n",
             path, CUBALC_LANG_VERSION);
      return 1;
    }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0 || sz > CUBALC_MAX_SRC) {
      fclose(f);
      printf("{\"schema\":\"cubalc.cat.v1\",\"ok\":false,\"cmd\":\"cat\","
             "\"path\":\"%s\",\"err\":\"too large\",\"version\":\"%s\"}\n",
             path, CUBALC_LANG_VERSION);
      return 1;
    }
    buf = malloc((size_t)sz + 1);
    if (!buf) {
      fclose(f);
      printf("{\"schema\":\"cubalc.cat.v1\",\"ok\":false,\"err\":\"oom\"}\n");
      return 2;
    }
    nr = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[nr] = 0;
    if (!meta_only) {
      /* body first for humans/agents that want raw source; plate after */
      fwrite(buf, 1, nr, stdout);
      if (nr == 0 || buf[nr - 1] != '\n')
        fputc('\n', stdout);
    }
    /* one JSON plate (agents parse last line or schema cubalc.cat.v1) */
    {
      const char *base = strrchr(path, '/');
      int lines = 1;
      size_t k;
      for (k = 0; k < nr; k++)
        if (buf[k] == '\n') lines++;
      if (nr > 0 && buf[nr - 1] == '\n' && lines > 1) lines--;
      base = base ? base + 1 : path;
      printf("{\"schema\":\"cubalc.cat.v1\",\"ok\":true,\"cmd\":\"cat\","
             "\"path\":\"%s\",\"name\":\"%s\",\"bytes\":%zu,\"lines\":%d,"
             "\"version\":\"%s\",\"note\":\"source above plate · INCLUDE short name ok\"}\n",
             path, base, nr, lines, CUBALC_LANG_VERSION);
    }
    free(buf);
    return 0;
  }
  fprintf(stderr, "cubalc cat: unexpected\n");
  return 2;
}
