/* CubalC CLI run/eval — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_run.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include "cubalc_run_util.h"
#include "cubalc_isa.h"
#include "cubalc_jit.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

int cubalc_cmd_run(int argc, char **argv) {
    /* Real language entry: parse + evaluate a .cubalc source program.
     * Usability: cubalc run - | eval - reads program from stdin (agents pipe).
     * Usability: cubalc run -e CODE | --expr|--code|-c — inline source (no temp file).
     *   multiple -e join with newline; \n \t \\ escapes expanded in each chunk.
     * Quiet: -q|--quiet|--plate or CUBALC_QUIET=1 → plate-only (no board/# ok).
     * Strict: -s|--strict or CUBALC_STRICT=1 → soft last_err fails exit+plate ok.
     * Timeout: -T MS|--timeout MS or CUBALC_RUN_TIMEOUT — wall-clock kill runaway loops.
     * Preload: -I|--include|--preload LIB + CUBALC_PRELOAD=a:b → INCLUDE ONCE before body.
     * Path: -L|--include-path|--lib-path DIR prepends CUBALC_INCLUDE_PATH for this run.
     * Dotenv: -F|--dotenv|--env-file PATH + CUBALC_DOTENV=a:b → SYS DOTENV before body.
     * Setenv: -E|--setenv KEY=VAL (repeatable) — process env inject after dotenv (CLI wins).
     * Unsetenv: -U|--unsetenv KEY (repeatable) — clear keys after dotenv, before -E.
     * Mkdir: -M|--mkdir|--ensure-dir DIR (repeatable) — mkdir -p before cwd/body.
     * Cwd: --cwd|--cd|--chdir DIR + CUBALC_CWD — chdir before body (relative plates).
     * Floor: -R|--require-version X.Y + CUBALC_REQUIRE_VERSION — fail if runtime older.
     * Forms: -C|--need-forms F1,F2 + CUBALC_REQUIRE_FORMS — fail if HELP catalog misses.
     * FNs: -N|--require-fns F1,F2 + CUBALC_REQUIRE_FNS — fail if preload FN surface misses.
     * Classes: -K|--require-classes C1,C2 + CUBALC_REQUIRE_CLASSES — fail if CLASS surface misses.
     * Ready: -Y|--need-ready + CUBALC_REQUIRE_READY — fail if READY prove checklist fails. */
    int quiet = 0, strict = 0, req_doctor = 0, req_ready = 0, i, rc;
    int plate_ok;
    int have_expr = 0;
    int src_idx = -1;
    int ddash = 0;
    int n_parg = 0;
    int n_preload = 0;
    int n_ipath = 0;
    int n_dotenv = 0;
    int n_setenv = 0;
    int n_unsetenv = 0;
    int n_mkdir = 0;
    long run_timeout_ms = 0;
    int run_timeout_cli = 0;
    long dotenv_keys = 0;
    const char *parg[32];
    char preload[16][96];
    char ipaths[8][512];
    char dotenv_paths[8][512];
    char setenv_kvs[32][384];
    char unsetenv_keys[32][192];
    char mkdir_paths[8][512];
    char run_cwd[512];
    char src_abs[1024];
    char req_ver[64];
    char req_forms[512];
    char req_fns[512];
    char req_classes[512];
    const char *src_path = NULL;
    const char *src_label;
    const char *eq;
    char *expr_buf = NULL;
    size_t expr_len = 0, expr_cap = 0;
    FILE *trace;
    FILE *devnull = NULL;
    cubalc_run_result rr;
    (void)ddash;
    (void)src_idx;
    req_ver[0] = 0;
    req_forms[0] = 0;
    req_fns[0] = 0;
    req_classes[0] = 0;
    req_doctor = 0;
    req_ready = 0;
    n_dotenv = 0;
    n_setenv = 0;
    n_unsetenv = 0;
    n_mkdir = 0;
    dotenv_keys = 0;
    run_cwd[0] = 0;
    src_abs[0] = 0;
    eq = getenv("CUBALC_QUIET");
    if (eq && eq[0] && strcmp(eq, "0") != 0 && strcmp(eq, "false") != 0 &&
        strcmp(eq, "FALSE") != 0 && strcmp(eq, "no") != 0 && strcmp(eq, "NO") != 0)
      quiet = 1;
    eq = getenv("CUBALC_STRICT");
    if (eq && eq[0] && strcmp(eq, "0") != 0 && strcmp(eq, "false") != 0 &&
        strcmp(eq, "FALSE") != 0 && strcmp(eq, "no") != 0 && strcmp(eq, "NO") != 0)
      strict = 1;
    /* Env wall budget first; CLI -T overrides. */
    eq = getenv("CUBALC_RUN_TIMEOUT");
    if (eq && eq[0]) {
      char *end = NULL;
      long t = strtol(eq, &end, 10);
      if (end != eq && t > 0) {
        if (t > 86400000L) t = 86400000L;
        run_timeout_ms = t;
      }
    }
    /* Env version floor first; CLI -R overrides. */
    eq = getenv("CUBALC_REQUIRE_VERSION");
    if (eq && eq[0])
      snprintf(req_ver, sizeof req_ver, "%s", eq);
    /* Env form floor first; CLI -C appends/overrides. */
    eq = getenv("CUBALC_REQUIRE_FORMS");
    if (eq && eq[0])
      snprintf(req_forms, sizeof req_forms, "%s", eq);
    eq = getenv("CUBALC_REQUIRE_FNS");
    if (eq && eq[0])
      snprintf(req_fns, sizeof req_fns, "%s", eq);
    eq = getenv("CUBALC_REQUIRE_CLASSES");
    if (eq && eq[0])
      snprintf(req_classes, sizeof req_classes, "%s", eq);
    eq = getenv("CUBALC_REQUIRE_DOCTOR");
    if (eq && eq[0] && strcmp(eq, "0") != 0 && strcasecmp(eq, "false") != 0 &&
        strcasecmp(eq, "no") != 0 && strcasecmp(eq, "off") != 0)
      req_doctor = 1;
    eq = getenv("CUBALC_REQUIRE_READY");
    if (eq && eq[0] && strcmp(eq, "0") != 0 && strcasecmp(eq, "false") != 0 &&
        strcasecmp(eq, "no") != 0 && strcasecmp(eq, "off") != 0)
      req_ready = 1;
    /* Env preloads first; CLI -I appends (deduped). */
    run_preload_parse_env(preload, &n_preload, getenv("CUBALC_PRELOAD"));
    /* Env dotenv paths first; CLI -F/--dotenv appends (deduped). */
    run_dotenv_parse_env(dotenv_paths, &n_dotenv, getenv("CUBALC_DOTENV"));
    /* Env cwd first; CLI --cwd overrides. */
    eq = getenv("CUBALC_CWD");
    if (eq && eq[0])
      snprintf(run_cwd, sizeof run_cwd, "%s", eq);
    /* Scan from argv[1] so top-level cubalc -e CODE (cmd rewritten to run) works. */
    for (i = 1; i < argc; i++) {
      if (!strcmp(argv[i], "run") || !strcmp(argv[i], "eval"))
        continue;
      if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "--quiet") ||
          !strcmp(argv[i], "--plate") || !strcmp(argv[i], "--json-only")) {
        quiet = 1;
        continue;
      }
      if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--strict") ||
          !strcmp(argv[i], "--fail-soft") || !strcmp(argv[i], "--strict-err")) {
        strict = 1;
        continue;
      }
      /* -T ms · --timeout · CUBALC_RUN_TIMEOUT dual — wall-clock kill */
      if (!strcmp(argv[i], "-T") || !strcmp(argv[i], "--timeout") ||
          !strcmp(argv[i], "--time-limit") || !strcmp(argv[i], "--max-ms")) {
        long t;
        char *end = NULL;
        if (i + 1 >= argc) {
          fprintf(stderr, "cubalc run: %s needs milliseconds (e.g. 5000)\n", argv[i]);
          free(expr_buf);
          return 2;
        }
        t = strtol(argv[++i], &end, 10);
        if (end == argv[i] || t < 0) {
          fprintf(stderr, "cubalc run: bad timeout ms '%s'\n", argv[i]);
          free(expr_buf);
          return 2;
        }
        if (t > 86400000L) t = 86400000L;
        run_timeout_ms = t;
        run_timeout_cli = 1;
        continue;
      }
      if (!strncmp(argv[i], "--timeout=", 10) ||
          !strncmp(argv[i], "--time-limit=", 13) ||
          !strncmp(argv[i], "--max-ms=", 9)) {
        const char *v = argv[i];
        long t;
        char *end = NULL;
        if (!strncmp(v, "--timeout=", 10)) v += 10;
        else if (!strncmp(v, "--time-limit=", 13)) v += 13;
        else v += 9;
        t = strtol(v, &end, 10);
        if (end == v || t < 0) {
          fprintf(stderr, "cubalc run: bad timeout ms '%s'\n", v);
          free(expr_buf);
          return 2;
        }
        if (t > 86400000L) t = 86400000L;
        run_timeout_ms = t;
        run_timeout_cli = 1;
        continue;
      }
      /* -R x.y · --require-version · CUBALC_REQUIRE_VERSION dual */
      if (!strcmp(argv[i], "-R") || !strcmp(argv[i], "--require-version") ||
          !strcmp(argv[i], "--min-version")) {
        if (i + 1 >= argc) {
          fprintf(stderr, "cubalc run: %s needs a version like 1.15\n", argv[i]);
          free(expr_buf);
          return 2;
        }
        snprintf(req_ver, sizeof req_ver, "%s", argv[++i]);
        continue;
      }
      if (!strncmp(argv[i], "--require-version=", 18)) {
        snprintf(req_ver, sizeof req_ver, "%s", argv[i] + 18);
        continue;
      }
      if (!strncmp(argv[i], "--min-version=", 14)) {
        snprintf(req_ver, sizeof req_ver, "%s", argv[i] + 14);
        continue;
      }
      /* -C F1,F2 · --need-forms · --require-forms · CUBALC_REQUIRE_FORMS dual */
      if (!strcmp(argv[i], "-C") || !strcmp(argv[i], "--need-forms") ||
          !strcmp(argv[i], "--require-forms") || !strcmp(argv[i], "--forms-need")) {
        if (i + 1 >= argc) {
          fprintf(stderr, "cubalc run: %s needs form names (e.g. SORTLIBS,LIBAGE)\n",
                  argv[i]);
          free(expr_buf);
          return 2;
        }
        snprintf(req_forms, sizeof req_forms, "%s", argv[++i]);
        continue;
      }
      if (!strncmp(argv[i], "--need-forms=", 13)) {
        snprintf(req_forms, sizeof req_forms, "%s", argv[i] + 13);
        continue;
      }
      if (!strncmp(argv[i], "--require-forms=", 16)) {
        snprintf(req_forms, sizeof req_forms, "%s", argv[i] + 16);
        continue;
      }
      /* -N F1,F2 · --require-fns · CUBALC_REQUIRE_FNS dual (NEEDFNS after -I preload) */
      if (!strcmp(argv[i], "-N") || !strcmp(argv[i], "--require-fns") ||
          !strcmp(argv[i], "--need-fn") || !strcmp(argv[i], "--need-fns-run") ||
          !strcmp(argv[i], "--fns-need") || !strcmp(argv[i], "--require-fn")) {
        if (i + 1 >= argc) {
          fprintf(stderr,
                  "cubalc run: %s needs FN names (e.g. greet,hello) — use with -I lib\n",
                  argv[i]);
          free(expr_buf);
          return 2;
        }
        snprintf(req_fns, sizeof req_fns, "%s", argv[++i]);
        continue;
      }
      if (!strncmp(argv[i], "--require-fns=", 14)) {
        snprintf(req_fns, sizeof req_fns, "%s", argv[i] + 14);
        continue;
      }
      if (!strncmp(argv[i], "--need-fn=", 10)) {
        snprintf(req_fns, sizeof req_fns, "%s", argv[i] + 10);
        continue;
      }
      if (!strncmp(argv[i], "--require-fn=", 13)) {
        snprintf(req_fns, sizeof req_fns, "%s", argv[i] + 13);
        continue;
      }
      if (!strncmp(argv[i], "--fns-need=", 11)) {
        snprintf(req_fns, sizeof req_fns, "%s", argv[i] + 11);
        continue;
      }
      /* -K C1,C2 · --require-classes · CUBALC_REQUIRE_CLASSES dual (NEEDCLASSES after -I) */
      if (!strcmp(argv[i], "-K") || !strcmp(argv[i], "--require-classes") ||
          !strcmp(argv[i], "--need-classes") || !strcmp(argv[i], "--classes-need") ||
          !strcmp(argv[i], "--require-class") || !strcmp(argv[i], "--need-class")) {
        if (i + 1 >= argc) {
          fprintf(stderr,
                  "cubalc run: %s needs CLASS names (e.g. Cell,Ticket) — use with -I lib\n",
                  argv[i]);
          free(expr_buf);
          return 2;
        }
        snprintf(req_classes, sizeof req_classes, "%s", argv[++i]);
        continue;
      }
      if (!strncmp(argv[i], "--require-classes=", 18)) {
        snprintf(req_classes, sizeof req_classes, "%s", argv[i] + 18);
        continue;
      }
      if (!strncmp(argv[i], "--need-classes=", 15)) {
        snprintf(req_classes, sizeof req_classes, "%s", argv[i] + 15);
        continue;
      }
      if (!strncmp(argv[i], "--require-class=", 16)) {
        snprintf(req_classes, sizeof req_classes, "%s", argv[i] + 16);
        continue;
      }
      if (!strncmp(argv[i], "--classes-need=", 15)) {
        snprintf(req_classes, sizeof req_classes, "%s", argv[i] + 15);
        continue;
      }
      /* -D · --need-doctor · --require-doctor · CUBALC_REQUIRE_DOCTOR dual */
      if (!strcmp(argv[i], "-D") || !strcmp(argv[i], "--need-doctor") ||
          !strcmp(argv[i], "--require-doctor") || !strcmp(argv[i], "--doctor-gate")) {
        req_doctor = 1;
        continue;
      }
      /* -Y · --need-ready · --require-ready · CUBALC_REQUIRE_READY dual (NEEDREADY floor) */
      if (!strcmp(argv[i], "-Y") || !strcmp(argv[i], "--need-ready") ||
          !strcmp(argv[i], "--require-ready") || !strcmp(argv[i], "--ready-gate") ||
          !strcmp(argv[i], "--prove-ready")) {
        req_ready = 1;
        continue;
      }
      /* -I lib · --include lib · --include=lib · --preload */
      if (!strcmp(argv[i], "-I") || !strcmp(argv[i], "--include") ||
          !strcmp(argv[i], "--preload")) {
        if (i + 1 >= argc) {
          fprintf(stderr, "cubalc run: %s needs a lib/path name\n", argv[i]);
          free(expr_buf);
          return 2;
        }
        run_preload_add(preload, &n_preload, argv[++i]);
        continue;
      }
      if (!strncmp(argv[i], "--include=", 10)) {
        run_preload_add(preload, &n_preload, argv[i] + 10);
        continue;
      }
      if (!strncmp(argv[i], "--preload=", 10)) {
        run_preload_add(preload, &n_preload, argv[i] + 10);
        continue;
      }
      /* -L dir · --include-path · --lib-path — one-shot CUBALC_INCLUDE_PATH */
      if (!strcmp(argv[i], "-L") || !strcmp(argv[i], "--include-path") ||
          !strcmp(argv[i], "--lib-path")) {
        if (i + 1 >= argc) {
          fprintf(stderr, "cubalc run: %s needs a directory\n", argv[i]);
          free(expr_buf);
          return 2;
        }
        if (n_ipath < 8)
          snprintf(ipaths[n_ipath++], sizeof ipaths[0], "%s", argv[++i]);
        else
          i++;
        continue;
      }
      if (!strncmp(argv[i], "--include-path=", 15)) {
        if (n_ipath < 8)
          snprintf(ipaths[n_ipath++], sizeof ipaths[0], "%s", argv[i] + 15);
        continue;
      }
      if (!strncmp(argv[i], "--lib-path=", 11)) {
        if (n_ipath < 8)
          snprintf(ipaths[n_ipath++], sizeof ipaths[0], "%s", argv[i] + 11);
        continue;
      }
      /* -F path · --dotenv · --env-file · CUBALC_DOTENV dual — load KEY=VAL before body */
      if (!strcmp(argv[i], "-F") || !strcmp(argv[i], "--dotenv") ||
          !strcmp(argv[i], "--env-file") || !strcmp(argv[i], "--envfile") ||
          !strcmp(argv[i], "--load-env")) {
        if (i + 1 >= argc) {
          fprintf(stderr, "cubalc run: %s needs a KEY=VAL env plate path\n", argv[i]);
          free(expr_buf);
          return 2;
        }
        run_dotenv_add(dotenv_paths, &n_dotenv, argv[++i]);
        continue;
      }
      if (!strncmp(argv[i], "--dotenv=", 9)) {
        run_dotenv_add(dotenv_paths, &n_dotenv, argv[i] + 9);
        continue;
      }
      if (!strncmp(argv[i], "--env-file=", 11)) {
        run_dotenv_add(dotenv_paths, &n_dotenv, argv[i] + 11);
        continue;
      }
      if (!strncmp(argv[i], "--envfile=", 10)) {
        run_dotenv_add(dotenv_paths, &n_dotenv, argv[i] + 10);
        continue;
      }
      /* --cwd DIR · --cd · --chdir · CUBALC_CWD dual — process chdir before body */
      if (!strcmp(argv[i], "--cwd") || !strcmp(argv[i], "--cd") ||
          !strcmp(argv[i], "--chdir") || !strcmp(argv[i], "--workdir") ||
          !strcmp(argv[i], "--work-dir")) {
        if (i + 1 >= argc) {
          fprintf(stderr, "cubalc run: %s needs a directory\n", argv[i]);
          free(expr_buf);
          return 2;
        }
        snprintf(run_cwd, sizeof run_cwd, "%s", argv[++i]);
        continue;
      }
      if (!strncmp(argv[i], "--cwd=", 6)) {
        snprintf(run_cwd, sizeof run_cwd, "%s", argv[i] + 6);
        continue;
      }
      if (!strncmp(argv[i], "--cd=", 5)) {
        snprintf(run_cwd, sizeof run_cwd, "%s", argv[i] + 5);
        continue;
      }
      if (!strncmp(argv[i], "--chdir=", 8)) {
        snprintf(run_cwd, sizeof run_cwd, "%s", argv[i] + 8);
        continue;
      }
      if (!strncmp(argv[i], "--workdir=", 10)) {
        snprintf(run_cwd, sizeof run_cwd, "%s", argv[i] + 10);
        continue;
      }
      /* -M DIR · --mkdir · --ensure-dir · mkdir -p before cwd/body (SYS MKDIR dual) */
      if (!strcmp(argv[i], "-M") || !strcmp(argv[i], "--mkdir") ||
          !strcmp(argv[i], "--ensure-dir") || !strcmp(argv[i], "--ensuredir") ||
          !strcmp(argv[i], "--make-dir")) {
        if (i + 1 >= argc) {
          fprintf(stderr, "cubalc run: %s needs a directory path\n", argv[i]);
          free(expr_buf);
          return 2;
        }
        if (n_mkdir < 8)
          snprintf(mkdir_paths[n_mkdir++], sizeof mkdir_paths[0], "%s", argv[++i]);
        else
          i++;
        continue;
      }
      if (!strncmp(argv[i], "--mkdir=", 8)) {
        if (n_mkdir < 8)
          snprintf(mkdir_paths[n_mkdir++], sizeof mkdir_paths[0], "%s", argv[i] + 8);
        continue;
      }
      if (!strncmp(argv[i], "--ensure-dir=", 13)) {
        if (n_mkdir < 8)
          snprintf(mkdir_paths[n_mkdir++], sizeof mkdir_paths[0], "%s", argv[i] + 13);
        continue;
      }
      if (!strncmp(argv[i], "-M", 2) && argv[i][2] == '=' ) {
        if (n_mkdir < 8)
          snprintf(mkdir_paths[n_mkdir++], sizeof mkdir_paths[0], "%s", argv[i] + 3);
        continue;
      }
      /* -E KEY=VAL · --setenv · --env-set · process env inject (after dotenv) */
      if (!strcmp(argv[i], "-E") || !strcmp(argv[i], "--setenv") ||
          !strcmp(argv[i], "--env-set") || !strcmp(argv[i], "--export")) {
        if (i + 1 >= argc) {
          fprintf(stderr, "cubalc run: %s needs KEY=VAL\n", argv[i]);
          free(expr_buf);
          return 2;
        }
        if (n_setenv < 32)
          snprintf(setenv_kvs[n_setenv++], sizeof setenv_kvs[0], "%s", argv[++i]);
        else
          i++;
        continue;
      }
      if (!strncmp(argv[i], "--setenv=", 9)) {
        if (n_setenv < 32)
          snprintf(setenv_kvs[n_setenv++], sizeof setenv_kvs[0], "%s", argv[i] + 9);
        continue;
      }
      if (!strncmp(argv[i], "--env-set=", 10)) {
        if (n_setenv < 32)
          snprintf(setenv_kvs[n_setenv++], sizeof setenv_kvs[0], "%s", argv[i] + 10);
        continue;
      }
      if (!strncmp(argv[i], "-E", 2) && argv[i][2] == '=' ) {
        if (n_setenv < 32)
          snprintf(setenv_kvs[n_setenv++], sizeof setenv_kvs[0], "%s", argv[i] + 3);
        continue;
      }
      /* -U KEY · --unsetenv · --env-unset · clear process env after dotenv, before -E */
      if (!strcmp(argv[i], "-U") || !strcmp(argv[i], "--unsetenv") ||
          !strcmp(argv[i], "--env-unset") || !strcmp(argv[i], "--unexport")) {
        if (i + 1 >= argc) {
          fprintf(stderr, "cubalc run: %s needs KEY name\n", argv[i]);
          free(expr_buf);
          return 2;
        }
        if (n_unsetenv < 32)
          snprintf(unsetenv_keys[n_unsetenv++], sizeof unsetenv_keys[0], "%s", argv[++i]);
        else
          i++;
        continue;
      }
      if (!strncmp(argv[i], "--unsetenv=", 11)) {
        if (n_unsetenv < 32)
          snprintf(unsetenv_keys[n_unsetenv++], sizeof unsetenv_keys[0], "%s", argv[i] + 11);
        continue;
      }
      if (!strncmp(argv[i], "--env-unset=", 12)) {
        if (n_unsetenv < 32)
          snprintf(unsetenv_keys[n_unsetenv++], sizeof unsetenv_keys[0], "%s", argv[i] + 12);
        continue;
      }
      if (!strncmp(argv[i], "-U", 2) && argv[i][2] == '=' ) {
        if (n_unsetenv < 32)
          snprintf(unsetenv_keys[n_unsetenv++], sizeof unsetenv_keys[0], "%s", argv[i] + 3);
        continue;
      }
      if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--expr") ||
          !strcmp(argv[i], "--code") || !strcmp(argv[i], "-c")) {
        const char *chunk;
        char unesc[CUBALC_MAX_SRC];
        size_t ulen = 0, need;
        size_t j;
        if (i + 1 >= argc) {
          fprintf(stderr, "cubalc run: %s needs a code argument\n", argv[i]);
          free(expr_buf);
          return 2;
        }
        chunk = argv[++i];
        /* expand \n \t \r \\ in chunk for shell-friendly one-liners */
        for (j = 0; chunk[j] && ulen + 1 < sizeof unesc; j++) {
          if (chunk[j] == '\\' && chunk[j + 1]) {
            char n = chunk[++j];
            if (n == 'n') unesc[ulen++] = '\n';
            else if (n == 't') unesc[ulen++] = '\t';
            else if (n == 'r') unesc[ulen++] = '\r';
            else if (n == '0') unesc[ulen++] = '\0';
            else unesc[ulen++] = n;
          } else {
            unesc[ulen++] = chunk[j];
          }
        }
        unesc[ulen] = 0;
        need = expr_len + (expr_len ? 1 : 0) + ulen + 1;
        if (need > expr_cap) {
          size_t ncap = expr_cap ? expr_cap : 512;
          char *nb;
          while (ncap < need) ncap *= 2;
          if (ncap > (size_t)CUBALC_MAX_SRC + 8)
            ncap = (size_t)CUBALC_MAX_SRC + 8;
          nb = realloc(expr_buf, ncap);
          if (!nb) {
            free(expr_buf);
            printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"<expr>\",\"err\":\"oom\"}\n");
            return 2;
          }
          expr_buf = nb;
          expr_cap = ncap;
        }
        if (expr_len && expr_len + 1 < expr_cap)
          expr_buf[expr_len++] = '\n';
        if (ulen > expr_cap - expr_len - 1)
          ulen = expr_cap - expr_len - 1;
        memcpy(expr_buf + expr_len, unesc, ulen);
        expr_len += ulen;
        expr_buf[expr_len] = 0;
        have_expr = 1;
        continue;
      }
      if (!strcmp(argv[i], "--")) {
        ddash = 1;
        for (i = i + 1; i < argc && n_parg < 32; i++)
          parg[n_parg++] = argv[i];
        break;
      }
      if (argv[i][0] == '-')
        continue;
      /* First bare token is the program file when not using -e.
       * Every later bare token is a program arg (CUBALC_ARGn). */
      if (!have_expr && !src_path) {
        src_path = argv[i];
        src_idx = i;
      } else if (n_parg < 32) {
        parg[n_parg++] = argv[i];
      }
    }
    /* Apply -L dirs: prepend to CUBALC_INCLUDE_PATH for this process. */
    if (n_ipath > 0) {
      char joined[4096];
      size_t o = 0;
      int k;
      const char *old = getenv("CUBALC_INCLUDE_PATH");
      joined[0] = 0;
      for (k = 0; k < n_ipath; k++) {
        size_t L;
        if (!ipaths[k][0]) continue;
        if (o && o + 1 < sizeof joined) joined[o++] = ':';
        L = strlen(ipaths[k]);
        if (o + L >= sizeof joined) L = sizeof joined - o - 1;
        if (L > 0) {
          memcpy(joined + o, ipaths[k], L);
          o += L;
        }
      }
      if (old && old[0]) {
        size_t L = strlen(old);
        if (o && o + 1 < sizeof joined) joined[o++] = ':';
        if (o + L >= sizeof joined) L = sizeof joined - o - 1;
        if (L > 0) {
          memcpy(joined + o, old, L);
          o += L;
        }
      }
      joined[o] = 0;
      if (joined[0])
        setenv("CUBALC_INCLUDE_PATH", joined, 1);
    }
    /* Publish program args for SYS ARG / ARGC / ARGS */
    {
      char nm[32], ac[16];
      int k;
      for (k = 0; k < 32; k++) {
        snprintf(nm, sizeof nm, "CUBALC_ARG%d", k);
        if (k < n_parg && parg[k])
          setenv(nm, parg[k], 1);
        else
          unsetenv(nm);
      }
      snprintf(ac, sizeof ac, "%d", n_parg);
      setenv("CUBALC_ARGC", ac, 1);
    }
    if (!have_expr && !src_path) {
      fprintf(stderr,
              "usage: cubalc run [-q] [-s] [-T MS] [-R VER] [-C FORMS] [-N FNS] [-K CLASSES] [-I LIB]... [-L DIR]... [-F ENV]... [-e CODE]... <file.cubalc>|- [-- args…]\n"
              "       cubalc eval [-q] [-s] [-T MS] [-R VER] [-C FORMS] [-I LIB]... [-e CODE]... <file>|-\n"
              "       cubalc -e 'SYS DATE\\nPRINT LAST'   # top-level alias\n"
              "       cubalc -I agent_boot -e 'STATUS'  # preload INCLUDE ONCE\n"
              "       cubalc -F .env -e 'SYS ENV NAME'  # dotenv KEY=VAL before body\n"
              "       cubalc -E FOO=bar -e 'SYS ENV FOO'  # inject env (after dotenv)\n"
              "       cubalc -U SECRET -E SECRET=new -e '…'  # clear then inject\n"
              "       cubalc -M state/work --cwd state/work -e '…'  # mkdir -p then chdir\n"
              "       cubalc --cwd state -e 'SYS LIST .\\nPRINT LAST'  # chdir before body\n"
              "       multiple -e join with newline; \\n \\t \\\\ escapes\n"
              "       -I/--include/--preload LIB  · CUBALC_PRELOAD=a:b\n"
              "       -F/--dotenv/--env-file PATH · CUBALC_DOTENV=a:b (SYS DOTENV dual)\n"
              "       -E|--setenv KEY=VAL · inject process env after dotenv (SYS ENV SET dual)\n"
              "       -U|--unsetenv KEY · clear env after dotenv, before -E (SYS ENV UNSET dual)\n"
              "       -M|--mkdir DIR · mkdir -p before cwd/body (SYS MKDIR dual)\n"
              "       --cwd|--cd|--chdir DIR · CUBALC_CWD (SYS CHDIR dual before body)\n"
              "       -L/--include-path DIR · prepends CUBALC_INCLUDE_PATH\n"
              "       -R/--require-version X.Y · CUBALC_REQUIRE_VERSION floor\n"
              "       -C/--need-forms F1,F2 · CUBALC_REQUIRE_FORMS capability floor\n"
              "       -N/--require-fns F1,F2 · CUBALC_REQUIRE_FNS FN floor after -I\n"
              "       -K/--require-classes C1,C2 · CUBALC_REQUIRE_CLASSES CLASS floor after -I\n"
              "       -D/--need-doctor · CUBALC_REQUIRE_DOCTOR install readiness floor\n"
              "       -Y/--need-ready · CUBALC_REQUIRE_READY install prove checklist floor\n"
              "       -T/--timeout MS · CUBALC_RUN_TIMEOUT wall kill runaway loops\n"
              "       CUBALC_QUIET=1  → plate only · CUBALC_STRICT=1 → soft last_err fails\n");
      free(expr_buf);
      return 2;
    }
    /* Absolute-ize program path before chdir so relative .cubalc still opens.
     * Join cwd when not already absolute (avoid realpath feature-test fights). */
    if (src_path && src_path[0] && strcmp(src_path, "-") != 0) {
      if (src_path[0] == '/') {
        snprintf(src_abs, sizeof src_abs, "%s", src_path);
        src_path = src_abs;
      } else {
        char cur[768];
        if (getcwd(cur, sizeof cur)) {
          snprintf(src_abs, sizeof src_abs, "%s/%s", cur, src_path);
          src_path = src_abs;
        }
      }
    }
    /* mkdir -p before cwd so --cwd can land in a fresh workdir (SYS MKDIR dual). */
    if (n_mkdir > 0) {
      int mi;
      char merr[240];
      char mbag[2048];
      size_t mo = 0;
      mbag[0] = 0;
      for (mi = 0; mi < n_mkdir; mi++) {
        size_t ml;
        merr[0] = 0;
        if (run_mkdir_p(mkdir_paths[mi], merr, sizeof merr) != 0) {
          free(expr_buf);
          if (devnull) fclose(devnull);
          printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"%s\","
                 "\"err\":\"MKDIR failed: %s\","
                 "\"mkdir\":\"%s\",\"mkdir_n\":%d,\"version\":\"%s\","
                 "\"why_hint\":\"cubalc run -M|--mkdir DIR · SYS MKDIR in-lang\","
                 "\"exit_code\":1,\"halted\":false}\n",
                 have_expr ? "<expr>" : (src_path ? src_path : "?"),
                 merr[0] ? merr : "mkdir fail",
                 mkdir_paths[mi], n_mkdir, CUBALC_LANG_VERSION);
          return 1;
        }
        ml = strlen(mkdir_paths[mi]);
        if (mo && mo + 1 < sizeof mbag) mbag[mo++] = '\n';
        if (mo + ml < sizeof mbag) {
          memcpy(mbag + mo, mkdir_paths[mi], ml);
          mo += ml;
          mbag[mo] = 0;
        }
      }
      {
        char nb[16];
        snprintf(nb, sizeof nb, "%d", n_mkdir);
        setenv("CUBALC_MKDIR_N", nb, 1);
      }
      if (mbag[0])
        setenv("CUBALC_MKDIR_DIRS", mbag, 1);
    } else {
      unsetenv("CUBALC_MKDIR_N");
      unsetenv("CUBALC_MKDIR_DIRS");
    }
    /* Host chdir before dotenv/body (dual of SYS CHDIR). Hard-fail miss. */
    if (run_cwd[0]) {
      char cwd_now[1024];
      if (chdir(run_cwd) != 0) {
        free(expr_buf);
        if (devnull) fclose(devnull);
        printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"%s\","
               "\"err\":\"CWD failed: cannot chdir '%s'\","
               "\"cwd\":\"%s\",\"version\":\"%s\","
               "\"why_hint\":\"cubalc run --cwd DIR · CUBALC_CWD · SYS CHDIR in-lang\","
               "\"exit_code\":1,\"halted\":false}\n",
               have_expr ? "<expr>" : (src_path ? src_path : "?"),
               run_cwd, run_cwd, CUBALC_LANG_VERSION);
        return 1;
      }
      if (!getcwd(cwd_now, sizeof cwd_now))
        snprintf(cwd_now, sizeof cwd_now, "%s", run_cwd);
      setenv("CUBALC_CWD_ACTIVE", cwd_now, 1);
    } else {
      unsetenv("CUBALC_CWD_ACTIVE");
    }
    /* Load KEY=VAL env plates into this process before gates/body (SYS DOTENV dual).
     * Hard-fail missing/unreadable paths so agents see a clear host contract. */
    if (n_dotenv > 0) {
      int di;
      char derr[240];
      dotenv_keys = 0;
      for (di = 0; di < n_dotenv; di++) {
        derr[0] = 0;
        if (run_dotenv_load_one(dotenv_paths[di], &dotenv_keys, derr, sizeof derr) != 0) {
          free(expr_buf);
          if (devnull) fclose(devnull);
          printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"%s\","
                 "\"err\":\"DOTENV failed: %s\","
                 "\"dotenv\":\"%s\",\"dotenv_n\":%d,\"dotenv_keys\":%ld,"
                 "\"version\":\"%s\","
                 "\"why_hint\":\"cubalc run -F|--dotenv path · CUBALC_DOTENV · SYS DOTENV in-lang\","
                 "\"exit_code\":1,\"halted\":false}\n",
                 have_expr ? "<expr>" : (src_path ? src_path : "?"),
                 derr[0] ? derr : "load fail",
                 dotenv_paths[di], n_dotenv, dotenv_keys,
                 CUBALC_LANG_VERSION);
          return 1;
        }
      }
      {
        char joined[4096];
        size_t o = 0;
        int k;
        joined[0] = 0;
        for (k = 0; k < n_dotenv; k++) {
          size_t L = strlen(dotenv_paths[k]);
          if (k && o + 1 < sizeof joined) joined[o++] = ':';
          if (o + L >= sizeof joined) L = sizeof joined - o - 1;
          if (L > 0) {
            memcpy(joined + o, dotenv_paths[k], L);
            o += L;
          }
        }
        joined[o] = 0;
        if (joined[0])
          setenv("CUBALC_DOTENV_ACTIVE", joined, 1);
      }
      {
        char kb[32];
        snprintf(kb, sizeof kb, "%ld", dotenv_keys);
        setenv("CUBALC_DOTENV_KEYS", kb, 1);
      }
    } else {
      unsetenv("CUBALC_DOTENV_ACTIVE");
      unsetenv("CUBALC_DOTENV_KEYS");
    }
    /* Clear KEY names after dotenv, before -E (dual of SYS ENV UNSET).
     * Idempotent: unsetenv of missing keys is success. */
    if (n_unsetenv > 0) {
      int ui;
      char ubag[2048];
      size_t uo = 0;
      ubag[0] = 0;
      for (ui = 0; ui < n_unsetenv; ui++) {
        const char *k = unsetenv_keys[ui];
        size_t kl;
        if (!k || !k[0]) continue;
        unsetenv(k);
        kl = strlen(k);
        if (uo && uo + 1 < sizeof ubag) ubag[uo++] = '\n';
        if (uo + kl < sizeof ubag) {
          memcpy(ubag + uo, k, kl);
          uo += kl;
          ubag[uo] = 0;
        }
      }
      {
        char nb[16];
        snprintf(nb, sizeof nb, "%d", n_unsetenv);
        setenv("CUBALC_UNSETENV_N", nb, 1);
      }
      if (ubag[0])
        setenv("CUBALC_UNSETENV_KEYS", ubag, 1);
    } else {
      unsetenv("CUBALC_UNSETENV_N");
      unsetenv("CUBALC_UNSETENV_KEYS");
    }
    /* Inject KEY=VAL pairs after dotenv/unset so CLI -E wins (dual of SYS ENV SET). */
    if (n_setenv > 0) {
      int si;
      char serr[200];
      char keys_bag[2048];
      size_t ko = 0;
      keys_bag[0] = 0;
      for (si = 0; si < n_setenv; si++) {
        const char *eqp;
        serr[0] = 0;
        if (run_setenv_one(setenv_kvs[si], serr, sizeof serr) != 0) {
          free(expr_buf);
          if (devnull) fclose(devnull);
          printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"%s\","
                 "\"err\":\"SETENV failed: %s\","
                 "\"setenv\":\"%s\",\"setenv_n\":%d,\"version\":\"%s\","
                 "\"why_hint\":\"cubalc run -E|--setenv KEY=VAL · SYS ENV SET in-lang\","
                 "\"exit_code\":1,\"halted\":false}\n",
                 have_expr ? "<expr>" : (src_path ? src_path : "?"),
                 serr[0] ? serr : "bad KEY=VAL",
                 setenv_kvs[si], n_setenv, CUBALC_LANG_VERSION);
          return 1;
        }
        eqp = strchr(setenv_kvs[si], '=');
        if (eqp) {
          size_t kl = (size_t)(eqp - setenv_kvs[si]);
          if (ko && ko + 1 < sizeof keys_bag) keys_bag[ko++] = '\n';
          if (ko + kl < sizeof keys_bag) {
            memcpy(keys_bag + ko, setenv_kvs[si], kl);
            ko += kl;
            keys_bag[ko] = 0;
          }
        }
      }
      {
        char nb[16];
        snprintf(nb, sizeof nb, "%d", n_setenv);
        setenv("CUBALC_SETENV_N", nb, 1);
      }
      if (keys_bag[0])
        setenv("CUBALC_SETENV_KEYS", keys_bag, 1);
    } else {
      unsetenv("CUBALC_SETENV_N");
      unsetenv("CUBALC_SETENV_KEYS");
    }
    /* Publish wall budget for run_source (CLI -T wins over prior env). */
    if (run_timeout_cli || run_timeout_ms > 0) {
      char tb[32];
      if (run_timeout_ms > 0) {
        snprintf(tb, sizeof tb, "%ld", run_timeout_ms);
        setenv("CUBALC_RUN_TIMEOUT", tb, 1);
      } else if (run_timeout_cli) {
        /* explicit -T 0 clears budget */
        unsetenv("CUBALC_RUN_TIMEOUT");
      }
    }
    /* Publish effective preload list for in-lang LISTPRELOAD / agents. */
    if (n_preload > 0) {
      char joined[512];
      size_t o = 0;
      int k;
      joined[0] = 0;
      for (k = 0; k < n_preload; k++) {
        size_t L = strlen(preload[k]);
        if (k && o + 1 < sizeof joined) joined[o++] = ':';
        if (o + L >= sizeof joined) L = sizeof joined - o - 1;
        if (L > 0) {
          memcpy(joined + o, preload[k], L);
          o += L;
        }
      }
      joined[o] = 0;
      if (joined[0])
        setenv("CUBALC_PRELOAD_ACTIVE", joined, 1);
    } else {
      unsetenv("CUBALC_PRELOAD_ACTIVE");
    }
    /* Fail-fast version floor before parse/run (agent host contract). */
    if (req_ver[0] && !run_version_ge(CUBALC_LANG_VERSION, req_ver)) {
      char prej[512], pmiss[512];
      int pi, pmiss_n;
      size_t po = 0;
      free(expr_buf);
      if (devnull) fclose(devnull);
      prej[po++] = '[';
      for (pi = 0; pi < n_preload && po + 8 < sizeof prej; pi++) {
        size_t j, sl = strlen(preload[pi]);
        if (pi && po + 1 < sizeof prej) prej[po++] = ',';
        if (po + 1 < sizeof prej) prej[po++] = '"';
        for (j = 0; j < sl && po + 2 < sizeof prej; j++) {
          char c = preload[pi][j];
          if (c == '"' || c == '\\') { prej[po++] = '\\'; prej[po++] = c; }
          else if ((unsigned char)c < 32) prej[po++] = ' ';
          else prej[po++] = c;
        }
        if (po + 1 < sizeof prej) prej[po++] = '"';
      }
      if (po + 1 < sizeof prej) prej[po++] = ']';
      prej[po] = 0;
      /* No includes yet — all requested names miss (honest dual of PRELOADOK). */
      pmiss_n = run_preload_miss_json(preload, n_preload, "", pmiss, sizeof pmiss);
      printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"%s\","
             "\"err\":\"REQUIRE VERSION %s failed: have %s\","
             "\"require_version\":\"%s\",\"version\":\"%s\","
             "\"why_hint\":\"upgrade runtime · cubalc version · REQUIRE VERSION in-lang dual\","
             "\"preload_n\":%d,\"preload\":%s,"
             "\"preload_ok\":%s,\"preload_miss_n\":%d,\"preload_miss\":%s,"
             "\"include_path_n\":%d,"
             "\"includes_n\":0,\"includes\":[],"
             "\"include_stems_n\":0,\"include_stems\":[],"
             "\"vars_n\":0,\"vars_max\":%d,\"vars_full\":false,"
             "\"quiet\":%s,\"strict\":%s,"
             "\"exit_code\":1,\"halted\":false}\n",
             have_expr ? "<expr>" : (src_path ? src_path : "?"),
             req_ver, CUBALC_LANG_VERSION, req_ver, CUBALC_LANG_VERSION,
             n_preload, prej,
             pmiss_n == 0 ? "true" : "false", pmiss_n, pmiss,
             n_ipath,
             CUBALC_MAX_VARS,
             quiet ? "true" : "false", strict ? "true" : "false");
      return 1;
    }
    /* Fail-fast form capability floor before body (agent host contract · NEEDFORMS dual). */
    if (req_forms[0]) {
      char forms[512], src[700];
      size_t a, b;
      cubalc_run_result fr;
      int fhit;
      /* normalize , : ; | to spaces for NEEDFORMS */
      for (a = 0, b = 0; req_forms[a] && b + 1 < sizeof forms; a++) {
        char c = req_forms[a];
        if (c == ',' || c == ':' || c == ';' || c == '|')
          c = ' ';
        forms[b++] = c;
      }
      forms[b] = 0;
      snprintf(src, sizeof src, "NEEDFORMS %s\nPASS\n", forms);
      memset(&fr, 0, sizeof fr);
      (void)cubalc_run_source(src, strlen(src), "<require-forms>", &fr, NULL);
      fhit = (fr.ok && fr.asserts_fail == 0 && !fr.err[0]) ? 1 : 0;
      if (!fhit) {
        free(expr_buf);
        if (devnull) fclose(devnull);
        printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"%s\","
               "\"err\":\"REQUIRE FORMS failed: %s\","
               "\"require_forms\":\"%s\",\"version\":\"%s\","
               "\"why_hint\":\"cubalc hasforms · HASFORM · NEEDFORMS · upgrade runtime\","
               "\"preload_n\":%d,"
               "\"require_version\":\"%s\","
               "\"includes_n\":0,\"includes\":[],"
               "\"vars_n\":0,\"vars_max\":%d,\"vars_full\":false,"
               "\"quiet\":%s,\"strict\":%s,"
               "\"exit_code\":1,\"halted\":false}\n",
               have_expr ? "<expr>" : (src_path ? src_path : "?"),
               fr.err[0] ? fr.err : (fr.last_err[0] ? fr.last_err : "missing form"),
               req_forms, CUBALC_LANG_VERSION,
               n_preload,
               req_ver[0] ? req_ver : "",
               CUBALC_MAX_VARS,
               quiet ? "true" : "false", strict ? "true" : "false");
        return 1;
      }
    }
    /* Fail-fast FN catalog floor after preloads (NEEDFNS dual · agent host contract). */
    if (req_fns[0]) {
      char fns[512], need_body[700];
      size_t a, b, glen = 0;
      char *gate_src = NULL;
      cubalc_run_result nr;
      int nhit;
      for (a = 0, b = 0; req_fns[a] && b + 1 < sizeof fns; a++) {
        char c = req_fns[a];
        if (c == ',' || c == ':' || c == ';' || c == '|')
          c = ' ';
        fns[b++] = c;
      }
      fns[b] = 0;
      snprintf(need_body, sizeof need_body, "NEEDFNS %s\nPASS\n", fns);
      gate_src = run_build_with_preload(preload, n_preload, need_body,
                                        strlen(need_body), &glen);
      memset(&nr, 0, sizeof nr);
      if (gate_src) {
        (void)cubalc_run_source(gate_src, glen, "<require-fns>", &nr, NULL);
        free(gate_src);
      } else {
        (void)cubalc_run_source(need_body, strlen(need_body), "<require-fns>", &nr, NULL);
      }
      nhit = (nr.ok && nr.asserts_fail == 0 && !nr.err[0]) ? 1 : 0;
      if (!nhit) {
        free(expr_buf);
        if (devnull) fclose(devnull);
        printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"%s\","
               "\"err\":\"REQUIRE FNS failed: %s\","
               "\"require_fns\":\"%s\",\"version\":\"%s\","
               "\"why_hint\":\"cubalc run -I lib -N fns · NEEDFNS · HASFNS · DEFINE/INCLUDE\","
               "\"preload_n\":%d,"
               "\"require_version\":\"%s\","
               "\"require_forms\":\"%s\","
               "\"includes_n\":0,\"includes\":[],"
               "\"vars_n\":0,\"vars_max\":%d,\"vars_full\":false,"
               "\"quiet\":%s,\"strict\":%s,"
               "\"exit_code\":1,\"halted\":false}\n",
               have_expr ? "<expr>" : (src_path ? src_path : "?"),
               nr.err[0] ? nr.err : (nr.last_err[0] ? nr.last_err : "missing FN"),
               req_fns, CUBALC_LANG_VERSION,
               n_preload,
               req_ver[0] ? req_ver : "",
               req_forms[0] ? req_forms : "",
               CUBALC_MAX_VARS,
               quiet ? "true" : "false", strict ? "true" : "false");
        return 1;
      }
    }
    /* Fail-fast CLASS catalog floor after preloads (NEEDCLASSES dual · OOP host contract). */
    if (req_classes[0]) {
      char cls[512], need_body[700];
      size_t a, b, glen = 0;
      char *gate_src = NULL;
      cubalc_run_result nr;
      int nhit;
      for (a = 0, b = 0; req_classes[a] && b + 1 < sizeof cls; a++) {
        char c = req_classes[a];
        if (c == ',' || c == ':' || c == ';' || c == '|')
          c = ' ';
        cls[b++] = c;
      }
      cls[b] = 0;
      snprintf(need_body, sizeof need_body, "NEEDCLASSES %s\nPASS\n", cls);
      gate_src = run_build_with_preload(preload, n_preload, need_body,
                                        strlen(need_body), &glen);
      memset(&nr, 0, sizeof nr);
      if (gate_src) {
        (void)cubalc_run_source(gate_src, glen, "<require-classes>", &nr, NULL);
        free(gate_src);
      } else {
        (void)cubalc_run_source(need_body, strlen(need_body), "<require-classes>", &nr, NULL);
      }
      nhit = (nr.ok && nr.asserts_fail == 0 && !nr.err[0]) ? 1 : 0;
      if (!nhit) {
        free(expr_buf);
        if (devnull) fclose(devnull);
        printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"%s\","
               "\"err\":\"REQUIRE CLASSES failed: %s\","
               "\"require_classes\":\"%s\",\"version\":\"%s\","
               "\"why_hint\":\"cubalc run -I lib -K classes · NEEDCLASSES · HASCLASSES · CLASS/INCLUDE\","
               "\"preload_n\":%d,"
               "\"require_version\":\"%s\","
               "\"require_forms\":\"%s\","
               "\"require_fns\":\"%s\","
               "\"includes_n\":0,\"includes\":[],"
               "\"vars_n\":0,\"vars_max\":%d,\"vars_full\":false,"
               "\"quiet\":%s,\"strict\":%s,"
               "\"exit_code\":1,\"halted\":false}\n",
               have_expr ? "<expr>" : (src_path ? src_path : "?"),
               nr.err[0] ? nr.err : (nr.last_err[0] ? nr.last_err : "missing CLASS"),
               req_classes, CUBALC_LANG_VERSION,
               n_preload,
               req_ver[0] ? req_ver : "",
               req_forms[0] ? req_forms : "",
               req_fns[0] ? req_fns : "",
               CUBALC_MAX_VARS,
               quiet ? "true" : "false", strict ? "true" : "false");
        return 1;
      }
    }
    /* Fail-fast install readiness floor before body (NEEDDOCTOR dual). */
    if (req_doctor) {
      cubalc_run_result dr;
      int dhit;
      memset(&dr, 0, sizeof dr);
      (void)cubalc_run_source("NEEDDOCTOR\nPASS\n", 16, "<require-doctor>", &dr, NULL);
      dhit = (dr.ok && dr.asserts_fail == 0 && !dr.err[0]) ? 1 : 0;
      if (!dhit) {
        free(expr_buf);
        if (devnull) fclose(devnull);
        printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"%s\","
               "\"err\":\"REQUIRE DOCTOR failed: %s\","
               "\"require_doctor\":true,\"version\":\"%s\","
               "\"why_hint\":\"cubalc doctor · NEEDDOCTOR · programs/lib/agent_boot · HOLD_FLASH=1\","
               "\"preload_n\":%d,"
               "\"require_version\":\"%s\",\"require_forms\":\"%s\","
               "\"includes_n\":0,\"includes\":[],"
               "\"vars_n\":0,\"vars_max\":%d,\"vars_full\":false,"
               "\"quiet\":%s,\"strict\":%s,"
               "\"exit_code\":1,\"halted\":false}\n",
               have_expr ? "<expr>" : (src_path ? src_path : "?"),
               dr.err[0] ? dr.err : (dr.last_err[0] ? dr.last_err : "install not ready"),
               CUBALC_LANG_VERSION,
               n_preload,
               req_ver[0] ? req_ver : "",
               req_forms[0] ? req_forms : "",
               CUBALC_MAX_VARS,
               quiet ? "true" : "false", strict ? "true" : "false");
        return 1;
      }
    }
    /* Fail-fast install prove checklist floor before body (NEEDREADY dual). */
    if (req_ready) {
      cubalc_run_result yr;
      int yhit;
      memset(&yr, 0, sizeof yr);
      (void)cubalc_run_source("NEEDREADY\nPASS\n", 15, "<require-ready>", &yr, NULL);
      yhit = (yr.ok && yr.asserts_fail == 0 && !yr.err[0]) ? 1 : 0;
      if (!yhit) {
        free(expr_buf);
        if (devnull) fclose(devnull);
        printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"%s\","
               "\"err\":\"REQUIRE READY failed: %s\","
               "\"require_ready\":true,\"version\":\"%s\","
               "\"why_hint\":\"cubalc ready · NEEDREADY · READY_MISS · ready_boot · forms+libs\","
               "\"preload_n\":%d,"
               "\"require_version\":\"%s\",\"require_forms\":\"%s\","
               "\"require_doctor\":%s,"
               "\"includes_n\":0,\"includes\":[],"
               "\"vars_n\":0,\"vars_max\":%d,\"vars_full\":false,"
               "\"quiet\":%s,\"strict\":%s,"
               "\"exit_code\":1,\"halted\":false}\n",
               have_expr ? "<expr>" : (src_path ? src_path : "?"),
               yr.err[0] ? yr.err : (yr.last_err[0] ? yr.last_err : "install prove failed"),
               CUBALC_LANG_VERSION,
               n_preload,
               req_ver[0] ? req_ver : "",
               req_forms[0] ? req_forms : "",
               req_doctor ? "true" : "false",
               CUBALC_MAX_VARS,
               quiet ? "true" : "false", strict ? "true" : "false");
        return 1;
      }
    }
    if (quiet) {
      devnull = fopen("/dev/null", "w");
      trace = devnull ? devnull : NULL;
    } else {
      trace = stdout;
    }
    if (have_expr) {
      src_label = "<expr>";
      if (expr_len == 0) {
        free(expr_buf);
        if (devnull) fclose(devnull);
        printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"<expr>\","
               "\"err\":\"empty -e expression\"}\n");
        return 2;
      }
      if (n_preload > 0) {
        size_t clen = 0;
        char *comb = run_build_with_preload(preload, n_preload, expr_buf, expr_len, &clen);
        free(expr_buf);
        expr_buf = NULL;
        if (!comb) {
          if (devnull) fclose(devnull);
          printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"<expr>\",\"err\":\"oom\"}\n");
          return 2;
        }
        rc = cubalc_run_source(comb, clen, src_label, &rr, trace);
        free(comb);
      } else {
        rc = cubalc_run_source(expr_buf, expr_len, src_label, &rr, trace);
        free(expr_buf);
        expr_buf = NULL;
      }
    } else {
      src_label = src_path;
      if (!strcmp(src_path, "-") || !strcmp(src_path, "--stdin") ||
          !strcmp(src_path, "/dev/stdin")) {
        char *buf = malloc((size_t)CUBALC_MAX_SRC + 1);
        size_t n = 0;
        if (!buf) {
          if (devnull) fclose(devnull);
          printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"<stdin>\",\"err\":\"oom\"}\n");
          return 2;
        }
        n = fread(buf, 1, (size_t)CUBALC_MAX_SRC, stdin);
        buf[n] = 0;
        if (n == 0) {
          free(buf);
          if (devnull) fclose(devnull);
          printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"<stdin>\","
                 "\"err\":\"empty stdin — pipe a .cubalc program\"}\n");
          return 2;
        }
        src_label = "<stdin>";
        if (n_preload > 0) {
          size_t clen = 0;
          char *comb = run_build_with_preload(preload, n_preload, buf, n, &clen);
          free(buf);
          if (!comb) {
            if (devnull) fclose(devnull);
            printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"<stdin>\",\"err\":\"oom\"}\n");
            return 2;
          }
          rc = cubalc_run_source(comb, clen, src_label, &rr, trace);
          free(comb);
        } else {
          rc = cubalc_run_source(buf, n, src_label, &rr, trace);
          free(buf);
        }
      } else if (strstr(src_path, ".cblc")) {
        cubalc_image img;
        if (cubalc_isa_load(&img, src_path) != 0) {
          if (devnull) fclose(devnull);
          printf("{\"ok\":false,\"cmd\":\"run\",\"err\":\"bad cblc\"}\n");
          return 2;
        }
        /* ISA image has no INCLUDE surface — preload ignored for .cblc */
        rc = cubalc_jit_exec(&img, &rr, trace);
      } else if (n_preload > 0) {
        /* File + preload: read body, prepend INCLUDE ONCE, run as source. */
        FILE *f = fopen(src_path, "rb");
        char *buf = NULL;
        size_t n = 0;
        if (!f) {
          if (devnull) fclose(devnull);
          printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"%s\","
                 "\"err\":\"cannot open\"}\n", src_path);
          return 2;
        }
        fseek(f, 0, SEEK_END);
        {
          long sz = ftell(f);
          fseek(f, 0, SEEK_SET);
          if (sz < 0 || sz > CUBALC_MAX_SRC) {
            fclose(f);
            if (devnull) fclose(devnull);
            printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"%s\","
                   "\"err\":\"bad size\"}\n", src_path);
            return 2;
          }
          buf = malloc((size_t)sz + 1);
          if (!buf) {
            fclose(f);
            if (devnull) fclose(devnull);
            printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"%s\",\"err\":\"oom\"}\n",
                   src_path);
            return 2;
          }
          n = fread(buf, 1, (size_t)sz, f);
          buf[n] = 0;
        }
        fclose(f);
        {
          size_t clen = 0;
          char *comb = run_build_with_preload(preload, n_preload, buf, n, &clen);
          free(buf);
          if (!comb) {
            if (devnull) fclose(devnull);
            printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"%s\",\"err\":\"oom\"}\n",
                   src_path);
            return 2;
          }
          rc = cubalc_run_source(comb, clen, src_label, &rr, trace);
          free(comb);
        }
      } else {
        rc = cubalc_run_file(src_path, &rr, trace);
      }
    }
    if (devnull) fclose(devnull);
    /* Usability: strict mode treats sticky soft last_err as process failure
     * (FAIL/EXPECT/INCLUDE SOFT leave ok=true by design; agents/CI use -s). */
    plate_ok = rr.ok ? 1 : 0;
    if (strict && rr.last_err[0]) {
      plate_ok = 0;
      if (rc == 0) rc = 1;
    }
    /* Usability: err_line/err_src — source snippet when error cites line N.
     * why_hint — agent recovery tip from sticky last_err (dual of in-lang WHY). */
    {
      char esrc[220], whyh[240], whyesc[280];
      size_t k, o = 0;
      const char *e = rr.last_err[0] ? rr.last_err : rr.err;
      for (k = 0; rr.err_src[k] && o + 2 < sizeof esrc; k++) {
        char c = rr.err_src[k];
        if (c == '"' || c == '\\') esrc[o++] = '_';
        else if ((unsigned char)c < 32) esrc[o++] = ' ';
        else esrc[o++] = c;
      }
      esrc[o] = 0;
      whyh[0] = 0;
      if (!e[0] && plate_ok)
        snprintf(whyh, sizeof whyh, "ok — no sticky LAST_ERR");
      else if (strstr(e, "NEEDPRELOAD") || strstr(e, "PRELOADOK") ||
               strstr(e, "PRELOADMISS") || strstr(e, "preload"))
        snprintf(whyh, sizeof whyh,
                 "cubalc run -I name · LISTPRELOAD · PRELOADMISS · INCLUDESTEMS");
      else if (strstr(e, "NEEDINCLUDE") || strstr(e, "HASINCLUDEALL") ||
               strstr(e, "INCLUDE") || strstr(e, "include") ||
               strstr(e, "REQUIRE LIB") || strstr(e, "lib missing"))
        snprintf(whyh, sizeof whyh,
                 "INCLUDE / cubalc run -I · NEEDINCLUDE · LISTINCLUDES · cubalc libs");
      else if (strstr(e, "ASSERT") || strstr(e, "EXPECT") ||
               strstr(e, "is false") || strstr(e, "falsey"))
        snprintf(whyh, sizeof whyh,
                 "ASSERT_GOT/ASSERT_EXPECTED · EXPECT soft · CLEAR_ERR after fix");
      else if (strstr(e, "NEEDP") || strstr(e, "NEEDFLAT") ||
               strstr(e, "missing") || strstr(e, "NEED "))
        snprintf(whyh, sizeof whyh,
                 "NEEDP/NEEDFLAT · plate has/need · DEFAULTP seed · cubalc plate need");
      else if (strstr(e, "VERSION") || strstr(e, "version") || strstr(e, "too old"))
        snprintf(whyh, sizeof whyh,
                 "REQUIRE VERSION floor · cubalc version · upgrade runtime");
      else if (strstr(e, "DIAL") || strstr(e, "SERVE") || strstr(e, "SMX") ||
               strstr(e, "TALK") || strstr(e, "P2P") || strstr(e, "timeout"))
        snprintf(whyh, sizeof whyh,
                 "CUBALC_P2P_SOFT=1 · cubalc env · CUBALC_P2P_TIMEOUT ms");
      else if (strstr(e, "HOLD_FLASH") || strstr(e, "PLUG") || strstr(e, "hold_flash"))
        snprintf(whyh, sizeof whyh,
                 "HOLD_FLASH device/mesh-join only · docs/HOLD_FLASH.md");
      else if (strstr(e, "unknown form") || strstr(e, "did you mean") ||
               strstr(e, "unknown"))
        snprintf(whyh, sizeof whyh,
                 "HELP form · cubalc forms prefix · cubalc search keyword");
      else if (strstr(e, "cannot open") || strstr(e, "ENOENT") ||
               strstr(e, "No such") || strstr(e, "path"))
        snprintf(whyh, sizeof whyh,
                 "cubalc paths · SYS EXIST · REQUIRE PATH · check CUBALC_STATE");
      else if (e[0])
        snprintf(whyh, sizeof whyh,
                 "WHY in-program · STATUS · CLEAR_ERR · cubalc doctor · COOKBOOK");
      else
        snprintf(whyh, sizeof whyh, "STATUS · VARS · FAIL/EXPECT probe");
      o = 0;
      for (k = 0; whyh[k] && o + 2 < sizeof whyesc; k++) {
        char c = whyh[k];
        if (c == '"' || c == '\\') { whyesc[o++] = '\\'; whyesc[o++] = c; }
        else if ((unsigned char)c < 32) whyesc[o++] = ' ';
        else whyesc[o++] = c;
      }
      whyesc[o] = 0;
      /* includes JSON array from newline bag (LISTINCLUDES plate dual).
       * include_stems short-name array (INCLUDESTEMS dual · compare to preload[]).
       * preload JSON array of -I / CUBALC_PRELOAD names (requested, not paths).
       * preload_ok / preload_miss* dual of PRELOADOK/PRELOADMISS for agents. */
      {
        char incj[768], stemsj[512], prej[512], pmiss[512];
        size_t io = 0, po = 0;
        const char *ip = rr.includes;
        int pi, pmiss_n, stems_n;
        incj[io++] = '[';
        while (*ip && io + 8 < sizeof incj) {
          const char *seg = ip;
          size_t slen = 0;
          while (ip[slen] && ip[slen] != '\n') slen++;
          if (io > 1 && io + 1 < sizeof incj) incj[io++] = ',';
          if (io + 1 < sizeof incj) incj[io++] = '"';
          {
            size_t j;
            for (j = 0; j < slen && io + 2 < sizeof incj; j++) {
              char c = seg[j];
              if (c == '"' || c == '\\') { incj[io++] = '\\'; incj[io++] = c; }
              else if ((unsigned char)c < 32) incj[io++] = ' ';
              else incj[io++] = c;
            }
          }
          if (io + 1 < sizeof incj) incj[io++] = '"';
          ip += slen;
          if (*ip == '\n') ip++;
        }
        if (io + 1 < sizeof incj) incj[io++] = ']';
        incj[io] = 0;
        stems_n = run_includes_stems_json(rr.includes, stemsj, sizeof stemsj);
        prej[po++] = '[';
        for (pi = 0; pi < n_preload && po + 8 < sizeof prej; pi++) {
          size_t j, sl = strlen(preload[pi]);
          if (pi && po + 1 < sizeof prej) prej[po++] = ',';
          if (po + 1 < sizeof prej) prej[po++] = '"';
          for (j = 0; j < sl && po + 2 < sizeof prej; j++) {
            char c = preload[pi][j];
            if (c == '"' || c == '\\') { prej[po++] = '\\'; prej[po++] = c; }
            else if ((unsigned char)c < 32) prej[po++] = ' ';
            else prej[po++] = c;
          }
          if (po + 1 < sizeof prej) prej[po++] = '"';
        }
        if (po + 1 < sizeof prej) prej[po++] = ']';
        prej[po] = 0;
        pmiss_n = run_preload_miss_json(preload, n_preload, rr.includes,
                                        pmiss, sizeof pmiss);
        printf("{\"ok\":%s,\"cmd\":\"run\",\"file\":\"%s\",\"stmts\":%d,"
               "\"asserts_ok\":%d,\"asserts_fail\":%d,\"n\":%d,\"unity\":%.3f,"
               "\"language\":\"%s\",\"version\":\"%s\",\"err\":\"%s\","
               "\"last_err\":\"%s\",\"err_line\":%d,\"err_src\":\"%s\","
               "\"why_hint\":\"%s\","
               "\"quiet\":%s,\"strict\":%s,"
               "\"timeout_ms\":%d,\"timed_out\":%s,\"remain_ms\":%d,\"wall_ms\":%d,"
               "\"preload_n\":%d,\"preload\":%s,"
               "\"preload_ok\":%s,\"preload_miss_n\":%d,\"preload_miss\":%s,"
               "\"include_path_n\":%d,"
               "\"require_version\":\"%s\","
               "\"require_forms\":\"%s\","
               "\"require_fns\":\"%s\","
               "\"require_classes\":\"%s\","
               "\"require_doctor\":%s,"
               "\"require_ready\":%s,"
               "\"includes_n\":%d,\"includes\":%s,"
               "\"include_stems_n\":%d,\"include_stems\":%s,"
               "\"vars_n\":%d,\"vars_max\":%d,\"vars_full\":%s,"
               "\"exit_code\":%d,\"halted\":%s}\n",
               plate_ok ? "true" : "false", src_label, rr.stmts, rr.asserts_ok,
               rr.asserts_fail, rr.n_cubes, rr.unity, CUBALC_LANG_NAME,
               CUBALC_LANG_VERSION, rr.err, rr.last_err, rr.err_line, esrc,
               whyesc,
               quiet ? "true" : "false", strict ? "true" : "false",
               rr.timeout_ms > 0 ? rr.timeout_ms : (int)run_timeout_ms,
               rr.timed_out ? "true" : "false",
               rr.remain_ms, rr.wall_ms,
               n_preload, prej,
               pmiss_n == 0 ? "true" : "false", pmiss_n, pmiss,
               n_ipath,
               req_ver[0] ? req_ver : "",
               req_forms[0] ? req_forms : "",
               req_fns[0] ? req_fns : "",
               req_classes[0] ? req_classes : "",
               req_doctor ? "true" : "false",
               req_ready ? "true" : "false",
               rr.includes_n, incj,
               stems_n, stemsj,
               rr.vars_n, rr.vars_max > 0 ? rr.vars_max : CUBALC_MAX_VARS,
               rr.vars_full ? "true" : "false",
               rr.exit_code, rr.halted ? "true" : "false");
      }
    }
    return rc;
}
