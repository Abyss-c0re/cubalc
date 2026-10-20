#define _POSIX_C_SOURCE 200809L
#include "cubalc.h"
#include "cubalc_smx.h"
#include "cubalc_cubechain.h"
#include "cubalc_lang.h"
#include "cubalc_isa.h"
#include "cubalc_jit.h"
#include "cubalc_translate.h"
#include "cubalc_evolve.h"
#include "cubalc_science_loop.h"
#include "cubalc_cmd_smx_exchange.h"
#include "cubalc_cmd_smx_bus.h"
#include "cubalc_cmd_smx_selftest.h"
#include "cubalc_cmd_protect.h"
#include "cubalc_cmd_showcase.h"
#include "cubalc_cmd_genesis.h"
#include "cubalc_cmd_sync.h"
#include "cubalc_run_util.h"
#include "cubalc_cmd_forms.h"
#include "cubalc_cmd_which.h"
#include "cubalc_cmd_search.h"
#include "cubalc_cmd_init.h"
#include "cubalc_cmd_plate.h"
#include "cubalc_cmd_law.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

/* CubalC machine — CBLC · smx · cubechain · hold=1 */

static void state_dir(char *buf, size_t n) {
  const char *e = getenv("CUBALC_STATE");
  if (e && e[0]) { snprintf(buf, n, "%s", e); return; }
  snprintf(buf, n, "state");
}

static void ensure_dir(const char *d) { mkdir(d, 0755); }

/* Count INCLUDE targets + DEFAULT knobs in a lib recipe (agent libs catalog). */
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

/* host:port or bare port → host buffer + port (for smx-bus TCP). */

static void paths(char *viz, char *chain, char *init, char *dir, size_t n) {
  state_dir(dir, n);
  ensure_dir(dir);
  snprintf(viz, n, "%s/cubalc_viz_frame.json", dir);
  snprintf(chain, n, "%s/cubalc_chain.json", dir);
  snprintf(init, n, "%s/cubalc_initial_matrix.json", dir);
}

static void publish(const cubalc_chain *ch) {
  char viz[512], chainp[512], initp[512], dir[512];
  paths(viz, chainp, initp, dir, sizeof viz);
  /* Cube Law: one publish path → viz JSON + lattice + cells.bin united */
  cubalc_chain_publish_united(ch);
  cubalc_chain_write_json(ch, chainp);
}

static void store_chain(const cubalc_chain *ch, uint16_t flags) {
  cubalc_cchn cc;
  char dir[512];
  state_dir(dir, sizeof dir);
  if (cubalc_cchn_open(&cc, dir) != 0) return;
  cubalc_cchn_append_chain(&cc, ch, flags);
  cubalc_cchn_close(&cc);
}

static int load_genesis_bits(cubalc_matrix *gen) {
  char viz[512], chainp[512], initp[512], dir[512], bits[CUBALC_ATOM_BITS + 1];
  paths(viz, chainp, initp, dir, sizeof viz);
  bits[0] = 0;
  FILE *f = fopen(initp, "r");
  if (f) {
    char buf[4096];
    size_t nr = fread(buf, 1, sizeof buf - 1, f);
    buf[nr] = 0;
    fclose(f);
    char *b = strstr(buf, "\"bits\":\"");
    if (b) {
      b += 8;
      int i = 0;
      while (*b && *b != '"' && i < CUBALC_ATOM_BITS) bits[i++] = *b++;
      bits[i] = 0;
    }
  }
  if (bits[0]) {
    cubalc_matrix_from_ascii(gen, bits, (int)strlen(bits));
    return 0;
  }
  return -1;
}



/* Sync with peer hive mind: fold plate + run hive join program in-memory */





/* SMX2 binary bus CLI — TCP/AF_UNIX, no HTTP (P2P wire). */






int main(int argc, char **argv) {
  const char *cmd = argc > 1 ? argv[1] : "genesis";
  /* Usability: cubalc -e CODE ≡ cubalc run -e CODE (no subcommand required).
   * Also -I/--include/-L top-level → run so agents skip the subcommand. */
  if (strcmp(cmd, "-e") == 0 || strcmp(cmd, "--expr") == 0 ||
      strcmp(cmd, "--code") == 0 || strcmp(cmd, "-c") == 0 ||
      strcmp(cmd, "-I") == 0 || strcmp(cmd, "--include") == 0 ||
      strcmp(cmd, "--preload") == 0 ||
      strncmp(cmd, "--include=", 10) == 0 ||
      strncmp(cmd, "--preload=", 10) == 0 ||
      strcmp(cmd, "-L") == 0 || strcmp(cmd, "--include-path") == 0 ||
      strcmp(cmd, "--lib-path") == 0 ||
      strncmp(cmd, "--include-path=", 15) == 0 ||
      strncmp(cmd, "--lib-path=", 11) == 0 ||
      strcmp(cmd, "-R") == 0 || strcmp(cmd, "--require-version") == 0 ||
      strncmp(cmd, "--require-version=", 18) == 0)
    cmd = "run";
  if (strcmp(cmd, "genesis") == 0)
    return cubalc_cmd_genesis(argc > 2 ? argv[2] :
      "NEXUS_COORD v1 | from=BlackCube | type=heartbeat | hold_flash=1 |");
  if (strcmp(cmd, "impulse") == 0)
    return cubalc_cmd_impulse(argc > 2 ? argv[2] : "cube-create",
                       argc > 3 ? atoi(argv[3]) : 1);
  if (strcmp(cmd, "tick") == 0 || strcmp(cmd, "flow") == 0)
    return cubalc_cmd_flow(argc > 2 ? atoi(argv[2]) : 8,
                    argc > 3 ? atoi(argv[3]) : 50);
  if (strcmp(cmd, "cubes") == 0 || strcmp(cmd, "show") == 0 ||
      strcmp(cmd, "lego") == 0) /* lego alias retired — cubes only */
    return cubalc_cmd_cubes();
  if (strcmp(cmd, "cubechain") == 0 || strcmp(cmd, "store") == 0)
    return cubalc_cmd_cubechain(argc > 2 ? argv[2] : "tip");
  if (strcmp(cmd, "smx-selftest") == 0 || strcmp(cmd, "smx") == 0)
    return cubalc_cmd_smx_selftest();
  if (strcmp(cmd, "smx-exchange") == 0)
    return cubalc_cmd_smx_exchange(argc, argv);
  if (strcmp(cmd, "smx-bus") == 0 || strcmp(cmd, "smx-nohttp") == 0 ||
      strcmp(cmd, "smx-net") == 0)
    return cubalc_cmd_smx_bus(argc, argv);
  if (strcmp(cmd, "law") == 0 || strcmp(cmd, "manifest") == 0)
    return cubalc_cmd_law_manifest();
  if (strcmp(cmd, "science") == 0 || strcmp(cmd, "sci") == 0 ||
      strcmp(cmd, "pure-science") == 0) {
    /* cubalc_cmd_science expects argv[0]=cubalc argv[1]=science argv[2]=sub */
    return cubalc_cmd_science(argc, argv);
  }
  if (strcmp(cmd, "protect") == 0 || strcmp(cmd, "core-protect") == 0 ||
      strcmp(cmd, "core-guard") == 0 || strcmp(cmd, "guard") == 0)
    return cubalc_cmd_protect(argc, argv);
  if (strcmp(cmd, "sync") == 0 || strcmp(cmd, "hive") == 0)
    return cubalc_cmd_sync(argc > 2 ? argv[2] : NULL);
  if (strcmp(cmd, "boot") == 0 || strcmp(cmd, "os") == 0) {
    /* CubeOS way in CubalC — braincube core. OS is path only. */
    const char *prog = "programs/cubeos/boot.cubalc";
    if (strcmp(cmd, "os") == 0 && argc > 2) {
      if (strcmp(argv[2], "tick") == 0)
        prog = "programs/cubeos/tick.cubalc";
      else if (strcmp(argv[2], "boot") == 0)
        prog = "programs/cubeos/boot.cubalc";
      else if (argv[2][0] == '/' || strstr(argv[2], ".cubalc"))
        prog = argv[2];
      else {
        static char pbuf[256];
        snprintf(pbuf, sizeof pbuf, "programs/cubeos/%s.cubalc", argv[2]);
        prog = pbuf;
      }
    } else if (strcmp(cmd, "boot") == 0 && argc > 2)
      prog = argv[2];
    cubalc_run_result rr;
    int rc = cubalc_run_file(prog, &rr, stdout);
    printf("{\"ok\":%s,\"cmd\":\"%s\",\"file\":\"%s\",\"stmts\":%d,"
           "\"asserts_ok\":%d,\"asserts_fail\":%d,\"n\":%d,\"unity\":%.3f,"
           "\"language\":\"%s\",\"version\":\"%s\",\"core\":\"c3\","
           "\"last\":\"%s\",\"err\":\"%s\"}\n",
           rr.ok ? "true" : "false", cmd, prog, rr.stmts, rr.asserts_ok,
           rr.asserts_fail, rr.n_cubes, rr.unity, CUBALC_LANG_NAME,
           CUBALC_LANG_VERSION, rr.last_print, rr.err);
    return rc;
  }
  if (strcmp(cmd, "translate") == 0 || strcmp(cmd, "from") == 0 ||
      strcmp(cmd, "decide") == 0) {
    /* Anything → CubalC; decide = translate + run (braincube algocube) */
    int do_run = (strcmp(cmd, "decide") == 0);
    char *buf = NULL;
    size_t n = 0;
    if (argc < 3) {
      fprintf(stderr,
        "usage: cubalc translate <file|-|text…>\n"
        "       cubalc decide    <file|-|text…>  # translate + run\n");
      return 2;
    }
    if (strcmp(argv[2], "-") == 0) {
      size_t cap = 65536;
      buf = malloc(cap);
      if (!buf) return 2;
      n = fread(buf, 1, cap - 1, stdin);
      buf[n] = 0;
    } else {
      FILE *f = fopen(argv[2], "rb");
      if (f && argc == 3) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (sz < 0 || sz > CUBALC_MAX_SRC) { fclose(f); return 2; }
        buf = malloc((size_t)sz + 1);
        if (!buf) { fclose(f); return 2; }
        n = fread(buf, 1, (size_t)sz, f);
        fclose(f);
        buf[n] = 0;
      } else {
        if (f) fclose(f);
        /* free text (one or many argv words) → anything path */
        size_t cap = 8192;
        buf = malloc(cap);
        if (!buf) return 2;
        buf[0] = 0;
        for (int i = 2; i < argc; i++) {
          size_t L = strlen(buf), a = strlen(argv[i]);
          if (L + a + 2 < cap) {
            if (L) { buf[L] = ' '; buf[L + 1] = 0; L++; }
            memcpy(buf + L, argv[i], a + 1);
          }
        }
        n = strlen(buf);
      }
    }
    char out[CUBALC_MAX_SRC];
    char err[160];
    err[0] = 0;
    int tr = cubalc_translate(buf, n, out, sizeof out, err, sizeof err);
    free(buf);
    if (tr != 0) {
      printf("{\"ok\":false,\"cmd\":\"%s\",\"err\":\"%s\"}\n", cmd, err);
      return tr;
    }
    if (!do_run) {
      fputs(out, stdout);
      if (out[0] && out[strlen(out) - 1] != '\n') fputc('\n', stdout);
      return 0;
    }
    cubalc_run_result rr;
    int rc = cubalc_run_source(out, strlen(out), "<decide>", &rr, stdout);
    printf("{\"ok\":%s,\"cmd\":\"decide\",\"stmts\":%d,\"asserts_ok\":%d,"
           "\"asserts_fail\":%d,\"n\":%d,\"unity\":%.3f,"
           "\"language\":\"%s\",\"version\":\"%s\",\"last\":\"%s\",\"err\":\"%s\"}\n",
           rr.ok ? "true" : "false", rr.stmts, rr.asserts_ok, rr.asserts_fail,
           rr.n_cubes, rr.unity, CUBALC_LANG_NAME, CUBALC_LANG_VERSION,
           rr.last_print, rr.err);
    return rc;
  }

  if (strcmp(cmd, "compile") == 0) {
    if (argc < 3) {
      fprintf(stderr, "usage: cubalc compile <file.cubalc|.casm> [-o out.cblc]\n");
      return 2;
    }
    const char *srcp = argv[2];
    const char *outp = "out/a.cblc";
    for (int i = 3; i < argc; i++)
      if (!strcmp(argv[i], "-o") && i + 1 < argc) outp = argv[++i];
    FILE *f = fopen(srcp, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", srcp); return 2; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz < 0 || sz > CUBALC_MAX_SRC) { fclose(f); return 2; }
    char *buf = malloc((size_t)sz + 1);
    size_t nr = fread(buf, 1, (size_t)sz, f); fclose(f); buf[nr] = 0;
    cubalc_image img; char err[160];
    if (cubalc_isa_compile_source(buf, nr, &img, err, sizeof err) != 0) {
      printf("{\"ok\":false,\"cmd\":\"compile\",\"err\":\"%s\"}\n", err);
      free(buf); return 2;
    }
    free(buf);
    {
      char dir[512]; snprintf(dir, sizeof dir, "%s", outp);
      char *slash = strrchr(dir, '/');
      if (slash) { *slash = 0; mkdir(dir, 0755); }
    }
    if (cubalc_isa_save(&img, outp) != 0) {
      printf("{\"ok\":false,\"cmd\":\"compile\",\"err\":\"save\"}\n");
      return 2;
    }
    printf("{\"ok\":true,\"cmd\":\"compile\",\"in\":\"%s\",\"out\":\"%s\","
           "\"n_ins\":%u,\"n_str\":%u,\"jit\":\"%s\",\"version\":\"%s\"}\n",
           srcp, outp, img.n_ins, img.n_str, cubalc_jit_backend(), CUBALC_LANG_VERSION);
    return 0;
  }
  if (strcmp(cmd, "jit") == 0 || strcmp(cmd, "cflow") == 0) {
    if (argc < 3) {
      fprintf(stderr, "usage: cubalc jit <file.cblc|file.cubalc>\n");
      return 2;
    }
    const char *path = argv[2];
    cubalc_run_result rr;
    int rc;
    if (strstr(path, ".cblc")) {
      cubalc_image img;
      if (cubalc_isa_load(&img, path) != 0) {
        printf("{\"ok\":false,\"cmd\":\"jit\",\"err\":\"load cblc\"}\n");
        return 2;
      }
      rc = cubalc_jit_exec(&img, &rr, stdout);
    } else {
      char outc[512];
      mkdir("out", 0755);
      snprintf(outc, sizeof outc, "out/manifest_%ld.cblc", (long)time(NULL));
      rc = cubalc_flow_manifest(path, outc, &rr, stdout);
    }
    printf("{\"ok\":%s,\"cmd\":\"jit\",\"file\":\"%s\",\"stmts\":%d,"
           "\"asserts_ok\":%d,\"asserts_fail\":%d,\"n\":%d,\"unity\":%.3f,"
           "\"backend\":\"%s\",\"version\":\"%s\",\"err\":\"%s\"}\n",
           rr.ok ? "true" : "false", path, rr.stmts, rr.asserts_ok, rr.asserts_fail,
           rr.n_cubes, rr.unity, cubalc_jit_backend(), CUBALC_LANG_VERSION, rr.err);
    return rc;
  }
  if (strcmp(cmd, "disasm") == 0) {
    if (argc < 3) return 2;
    cubalc_image img;
    if (cubalc_isa_load(&img, argv[2]) != 0) return 2;
    cubalc_isa_disasm(&img, stdout);
    return 0;
  }
  if (strcmp(cmd, "run") == 0 || strcmp(cmd, "eval") == 0) {
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
  if (strcmp(cmd, "peers") == 0 || strcmp(cmd, "oversee") == 0) {
    /* Generic peer fold — paths/digits from env, not device layouts in the language */
    const char *prog = "programs/peer_fold.cubalc";
    if (argc > 2 && (argv[2][0] == '/' || strstr(argv[2], ".cubalc")))
      prog = argv[2];
    cubalc_run_result rr;
    int rc = cubalc_run_file(prog, &rr, stdout);
    printf("{\"ok\":%s,\"cmd\":\"peers\",\"file\":\"%s\",\"stmts\":%d,"
           "\"asserts_ok\":%d,\"asserts_fail\":%d,\"n\":%d,\"unity\":%.3f,"
           "\"language\":\"%s\",\"version\":\"%s\",\"role\":\"peer_fold\","
           "\"last\":\"%s\",\"err\":\"%s\"}\n",
           rr.ok ? "true" : "false", prog, rr.stmts, rr.asserts_ok,
           rr.asserts_fail, rr.n_cubes, rr.unity, CUBALC_LANG_NAME,
           CUBALC_LANG_VERSION, rr.last_print, rr.err);
    return rc;
  }
  if (strcmp(cmd, "showcase") == 0 || strcmp(cmd, "demo") == 0 ||
      strcmp(cmd, "symphony") == 0) {
    return cubalc_cmd_showcase();
  }
  if (strcmp(cmd, "evolve") == 0 || strcmp(cmd, "evolve-loop") == 0 ||
      strcmp(cmd, "self-evolve") == 0) {
    /* Pure C: braincube solves · algocube optimizes · emits .cubalc */
    return cubalc_cmd_evolve(argc - 1, argv + 1);
  }
  if (strcmp(cmd, "ready") == 0 || strcmp(cmd, "proveready") == 0 ||
      strcmp(cmd, "install-ready") == 0 || strcmp(cmd, "check-ready") == 0) {
    /* Usability: install prove checklist (dual of READY). Soft by default.
     *   cubalc ready · cubalc proveready
     * Schema cubalc.ready.v1 · agents skip multi HASFORM+HASLIB glue. */
    cubalc_run_result rr;
    int ok;
    static const char src[] = "READY\nPASS\n";
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, sizeof src - 1, "<cli-ready>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    /* Prefer nested LAST if it is ready plate; else synthesize. */
    if (rr.last_print[0] && strstr(rr.last_print, "cubalc.ready.v1")) {
      printf("%s\n", rr.last_print);
    } else {
      printf("{\"schema\":\"cubalc.ready.v1\",\"ok\":%s,\"cmd\":\"ready\","
             "\"version\":\"%s\","
             "\"note\":\"install prove checklist · dual of READY\","
             "\"err\":\"%s\"}\n",
             ok ? "true" : "false", CUBALC_LANG_VERSION,
             ok ? "" : (rr.err[0] ? rr.err : (rr.last_err[0] ? rr.last_err : "not ready")));
    }
    return ok ? 0 : 1;
  }
  if (strcmp(cmd, "cliinfo") == 0 || strcmp(cmd, "dumpcli") == 0 ||
      strcmp(cmd, "clisnap") == 0 || strcmp(cmd, "cli-info") == 0 ||
      strcmp(cmd, "argvinfo") == 0 || strcmp(cmd, "cli-status") == 0) {
    /* Usability: full CLI surface plate (dual of CLIINFO).
     * Forwards optional trailing args after -- as CUBALC_ARGn for live inspect.
     *   cubalc cliinfo -- --verbose --out x a.txt
     * Schema cubalc.cli.v1 · agents skip LISTFLAGS+FLAGMAP+RESTARGS glue. */
    cubalc_run_result rr;
    int ok, ai, narg = 0;
    char envn[32];
    static const char src[] = "CLIINFO\nPASS\n";
    /* optional live args after -- */
    for (ai = 2; ai < argc; ai++) {
      if (!strcmp(argv[ai], "--")) {
        ai++;
        break;
      }
    }
    if (ai < argc) {
      for (; ai < argc && narg < 32; ai++, narg++) {
        snprintf(envn, sizeof envn, "CUBALC_ARG%d", narg);
        setenv(envn, argv[ai], 1);
      }
      snprintf(envn, sizeof envn, "%d", narg);
      setenv("CUBALC_ARGC", envn, 1);
    }
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, sizeof src - 1, "<cli-cliinfo>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    if (rr.last_print[0] && strstr(rr.last_print, "cubalc.cli.v1")) {
      printf("%s\n", rr.last_print);
    } else if (rr.last_print[0]) {
      printf("%s\n", rr.last_print);
    } else {
      printf("{\"schema\":\"cubalc.cli.v1\",\"ok\":%s,\"cmd\":\"cliinfo\","
             "\"version\":\"%s\",\"note\":\"CLI surface · dual of CLIINFO\","
             "\"err\":\"%s\"}\n",
             ok ? "true" : "false", CUBALC_LANG_VERSION,
             ok ? "" : (rr.err[0] ? rr.err : "cliinfo fail"));
    }
    return ok ? 0 : 1;
  }
  if (strcmp(cmd, "argmap") == 0 || strcmp(cmd, "argkv") == 0 ||
      strcmp(cmd, "argvmap") == 0 || strcmp(cmd, "args-kv") == 0 ||
      strcmp(cmd, "cli-argmap") == 0) {
    /* Usability: CLI dual of ARGMAP — raw argv i=value bag without .cubalc.
     *   cubalc argmap -- --verbose file.txt
     * Schema cubalc.argmap.v1 · twin of flagmap for full argv. */
    cubalc_run_result rr, rr_n;
    int ok, ai, narg = 0;
    long n = 0;
    char envn[32];
    char esc[CUBALC_HOST_STR_MAX];
    size_t e = 0, k;
    static const char src_bag[] = "ARGMAP\nPRINT LAST\nPASS\n";
    static const char src_n[] = "ARGMAP\nPRINT LAST_N\nPASS\n";
    for (ai = 2; ai < argc; ai++) {
      if (!strcmp(argv[ai], "--")) {
        ai++;
        break;
      }
    }
    if (ai < argc) {
      for (; ai < argc && narg < 32; ai++, narg++) {
        snprintf(envn, sizeof envn, "CUBALC_ARG%d", narg);
        setenv(envn, argv[ai], 1);
      }
      snprintf(envn, sizeof envn, "%d", narg);
      setenv("CUBALC_ARGC", envn, 1);
    }
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src_bag, sizeof src_bag - 1, "<cli-argmap>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    memset(&rr_n, 0, sizeof rr_n);
    (void)cubalc_run_source(src_n, sizeof src_n - 1, "<cli-argmap-n>", &rr_n, NULL);
    if (rr_n.last_print[0])
      n = strtol(rr_n.last_print, NULL, 10);
    for (k = 0; rr.last_print[k] && e + 2 < sizeof esc; k++) {
      char c = rr.last_print[k];
      if (c == '"' || c == '\\') {
        esc[e++] = '\\';
        esc[e++] = c;
      } else if (c == '\n') {
        esc[e++] = '\\';
        esc[e++] = 'n';
      } else if ((unsigned char)c < 32)
        esc[e++] = ' ';
      else
        esc[e++] = c;
    }
    esc[e] = 0;
    printf("{\"schema\":\"cubalc.argmap.v1\",\"ok\":%s,\"cmd\":\"argmap\","
           "\"n\":%ld,\"live_args\":%d,\"version\":\"%s\","
           "\"note\":\"CLI dual of ARGMAP · i=value bag for CUBALC_ARGn\","
           "\"bag\":\"%s\"}\n",
           ok ? "true" : "false", n, narg, CUBALC_LANG_VERSION, esc);
    return ok ? 0 : 1;
  }
  if (strcmp(cmd, "findarg") == 0 || strcmp(cmd, "argindex") == 0 ||
      strcmp(cmd, "indexarg") == 0 || strcmp(cmd, "whicharg") == 0 ||
      strcmp(cmd, "argvindex") == 0) {
    /* Usability: CLI dual of FINDARG — reverse NTHARG (token → index) without .cubalc.
     *   cubalc findarg --verbose -- --verbose file.txt
     * Schema cubalc.findarg.v1 · hit + index · soft miss exit 0 with hit=false. */
    cubalc_run_result rr;
    int ok, ai, narg = 0, tok_i = -1;
    long idx = -1, hit = 0;
    char envn[32];
    char token[CUBALC_HOST_STR_MAX];
    char src[CUBALC_HOST_STR_MAX + 96];
    char esc_tok[CUBALC_HOST_STR_MAX];
    size_t e = 0, k;
    token[0] = 0;
    /* token is first non-option after cmd; optional live args after -- */
    for (ai = 2; ai < argc; ai++) {
      if (!strcmp(argv[ai], "--")) {
        ai++;
        break;
      }
      if (tok_i < 0) {
        tok_i = ai;
        snprintf(token, sizeof token, "%s", argv[ai]);
      }
    }
    if (tok_i < 0 || !token[0]) {
      fprintf(stderr, "usage: cubalc findarg|argindex <token> [-- args…]\n");
      printf("{\"schema\":\"cubalc.findarg.v1\",\"ok\":false,\"cmd\":\"findarg\","
             "\"version\":\"%s\",\"note\":\"token required · dual of FINDARG\","
             "\"err\":\"missing token\"}\n",
             CUBALC_LANG_VERSION);
      return 1;
    }
    if (ai < argc) {
      for (; ai < argc && narg < 32; ai++, narg++) {
        snprintf(envn, sizeof envn, "CUBALC_ARG%d", narg);
        setenv(envn, argv[ai], 1);
      }
      snprintf(envn, sizeof envn, "%d", narg);
      setenv("CUBALC_ARGC", envn, 1);
    }
    /* escape token for string literal in -e source */
    {
      char lit[CUBALC_HOST_STR_MAX];
      size_t li = 0;
      for (k = 0; token[k] && li + 2 < sizeof lit; k++) {
        char c = token[k];
        if (c == '"' || c == '\\') {
          lit[li++] = '\\';
          lit[li++] = c;
        } else if ((unsigned char)c < 32)
          lit[li++] = ' ';
        else
          lit[li++] = c;
      }
      lit[li] = 0;
      snprintf(src, sizeof src,
               "FINDARG \"%s\"\n"
               "PRINT LAST_N\n"
               "PASS\n",
               lit);
    }
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<cli-findarg>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    if (rr.last_print[0])
      idx = strtol(rr.last_print, NULL, 10);
    hit = (idx >= 0) ? 1 : 0;
    for (k = 0; token[k] && e + 2 < sizeof esc_tok; k++) {
      char c = token[k];
      if (c == '"' || c == '\\') {
        esc_tok[e++] = '\\';
        esc_tok[e++] = c;
      } else if ((unsigned char)c < 32)
        esc_tok[e++] = ' ';
      else
        esc_tok[e++] = c;
    }
    esc_tok[e] = 0;
    printf("{\"schema\":\"cubalc.findarg.v1\",\"ok\":%s,\"cmd\":\"findarg\","
           "\"hit\":%s,\"index\":%ld,\"token\":\"%s\",\"live_args\":%d,"
           "\"version\":\"%s\","
           "\"note\":\"CLI dual of FINDARG · reverse NTHARG token→index\"}\n",
           ok ? "true" : "false", hit ? "true" : "false", idx, esc_tok, narg,
           CUBALC_LANG_VERSION);
    return ok ? 0 : 1;
  }
  if (strcmp(cmd, "afterarg") == 0 || strcmp(cmd, "nextarg") == 0 ||
      strcmp(cmd, "argafter") == 0 || strcmp(cmd, "followarg") == 0 ||
      strcmp(cmd, "argnext") == 0) {
    /* Usability: CLI dual of AFTERARG — peel argv after exact token without .cubalc.
     *   cubalc afterarg --out -- --verbose --out x.txt
     * Schema cubalc.afterarg.v1 · value + hit · soft miss empty value. */
    cubalc_run_result rr;
    int ok, ai, narg = 0, tok_i = -1;
    long hit = 0;
    char envn[32];
    char token[CUBALC_HOST_STR_MAX];
    char src[CUBALC_HOST_STR_MAX + 128];
    char esc_tok[CUBALC_HOST_STR_MAX], esc_val[CUBALC_HOST_STR_MAX];
    size_t e = 0, k;
    token[0] = 0;
    for (ai = 2; ai < argc; ai++) {
      if (!strcmp(argv[ai], "--")) {
        ai++;
        break;
      }
      if (tok_i < 0) {
        tok_i = ai;
        snprintf(token, sizeof token, "%s", argv[ai]);
      }
    }
    if (tok_i < 0 || !token[0]) {
      fprintf(stderr, "usage: cubalc afterarg|nextarg <token> [-- args…]\n");
      printf("{\"schema\":\"cubalc.afterarg.v1\",\"ok\":false,\"cmd\":\"afterarg\","
             "\"version\":\"%s\",\"note\":\"token required · dual of AFTERARG\","
             "\"err\":\"missing token\"}\n",
             CUBALC_LANG_VERSION);
      return 1;
    }
    if (ai < argc) {
      for (; ai < argc && narg < 32; ai++, narg++) {
        snprintf(envn, sizeof envn, "CUBALC_ARG%d", narg);
        setenv(envn, argv[ai], 1);
      }
      snprintf(envn, sizeof envn, "%d", narg);
      setenv("CUBALC_ARGC", envn, 1);
    }
    {
      char lit[CUBALC_HOST_STR_MAX];
      size_t li = 0;
      for (k = 0; token[k] && li + 2 < sizeof lit; k++) {
        char c = token[k];
        if (c == '"' || c == '\\') {
          lit[li++] = '\\';
          lit[li++] = c;
        } else if ((unsigned char)c < 32)
          lit[li++] = ' ';
        else
          lit[li++] = c;
      }
      lit[li] = 0;
      snprintf(src, sizeof src,
               "AFTERARG \"%s\"\n"
               "PRINT LAST\n"
               "PASS\n",
               lit);
    }
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<cli-afterarg>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    hit = (rr.last_print[0] != 0) ? 1 : 0;
    for (k = 0; token[k] && e + 2 < sizeof esc_tok; k++) {
      char c = token[k];
      if (c == '"' || c == '\\') {
        esc_tok[e++] = '\\';
        esc_tok[e++] = c;
      } else if ((unsigned char)c < 32)
        esc_tok[e++] = ' ';
      else
        esc_tok[e++] = c;
    }
    esc_tok[e] = 0;
    e = 0;
    for (k = 0; rr.last_print[k] && e + 2 < sizeof esc_val; k++) {
      char c = rr.last_print[k];
      if (c == '"' || c == '\\') {
        esc_val[e++] = '\\';
        esc_val[e++] = c;
      } else if (c == '\n') {
        esc_val[e++] = '\\';
        esc_val[e++] = 'n';
      } else if ((unsigned char)c < 32)
        esc_val[e++] = ' ';
      else
        esc_val[e++] = c;
    }
    esc_val[e] = 0;
    printf("{\"schema\":\"cubalc.afterarg.v1\",\"ok\":%s,\"cmd\":\"afterarg\","
           "\"hit\":%s,\"token\":\"%s\",\"value\":\"%s\",\"live_args\":%d,"
           "\"version\":\"%s\","
           "\"note\":\"CLI dual of AFTERARG · peel argv after exact token\"}\n",
           ok ? "true" : "false", hit ? "true" : "false", esc_tok, esc_val, narg,
           CUBALC_LANG_VERSION);
    return ok ? 0 : 1;
  }
  if (strcmp(cmd, "firstarg") == 0 || strcmp(cmd, "headarg") == 0 ||
      strcmp(cmd, "argv0") == 0 || strcmp(cmd, "lastarg") == 0 ||
      strcmp(cmd, "tailarg") == 0 || strcmp(cmd, "argvlast") == 0) {
    /* Usability: CLI dual of FIRSTARG/LASTARG — end peels without .cubalc.
     *   cubalc firstarg -- --verbose file.txt
     *   cubalc lastarg -- a b c
     * Schema cubalc.endarg.v1 · which=first|last · value + hit. */
    cubalc_run_result rr;
    int ok, ai, narg = 0, want_last = 0;
    long hit = 0;
    char envn[32];
    char esc_val[CUBALC_HOST_STR_MAX];
    size_t e = 0, k;
    static const char src_first[] = "FIRSTARG\nPRINT LAST\nPASS\n";
    static const char src_last[] = "LASTARG\nPRINT LAST\nPASS\n";
    want_last = (strcmp(cmd, "lastarg") == 0 || strcmp(cmd, "tailarg") == 0 ||
                 strcmp(cmd, "argvlast") == 0);
    for (ai = 2; ai < argc; ai++) {
      if (!strcmp(argv[ai], "--")) {
        ai++;
        break;
      }
    }
    if (ai < argc) {
      for (; ai < argc && narg < 32; ai++, narg++) {
        snprintf(envn, sizeof envn, "CUBALC_ARG%d", narg);
        setenv(envn, argv[ai], 1);
      }
      snprintf(envn, sizeof envn, "%d", narg);
      setenv("CUBALC_ARGC", envn, 1);
    }
    memset(&rr, 0, sizeof rr);
    if (want_last)
      (void)cubalc_run_source(src_last, sizeof src_last - 1, "<cli-lastarg>", &rr, NULL);
    else
      (void)cubalc_run_source(src_first, sizeof src_first - 1, "<cli-firstarg>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    hit = (rr.last_print[0] != 0) ? 1 : 0;
    for (k = 0; rr.last_print[k] && e + 2 < sizeof esc_val; k++) {
      char c = rr.last_print[k];
      if (c == '"' || c == '\\') {
        esc_val[e++] = '\\';
        esc_val[e++] = c;
      } else if (c == '\n') {
        esc_val[e++] = '\\';
        esc_val[e++] = 'n';
      } else if ((unsigned char)c < 32)
        esc_val[e++] = ' ';
      else
        esc_val[e++] = c;
    }
    esc_val[e] = 0;
    printf("{\"schema\":\"cubalc.endarg.v1\",\"ok\":%s,\"cmd\":\"%s\","
           "\"which\":\"%s\",\"hit\":%s,\"value\":\"%s\",\"live_args\":%d,"
           "\"version\":\"%s\","
           "\"note\":\"CLI dual of FIRSTARG/LASTARG · raw argv end peels\"}\n",
           ok ? "true" : "false", want_last ? "lastarg" : "firstarg",
           want_last ? "last" : "first", hit ? "true" : "false", esc_val, narg,
           CUBALC_LANG_VERSION);
    return ok ? 0 : 1;
  }
  if (strcmp(cmd, "needdoctor") == 0 || strcmp(cmd, "need-doctor") == 0 ||
      strcmp(cmd, "require-doctor") == 0 || strcmp(cmd, "requiredoctor") == 0) {
    /* Usability: hard install gate (dual of NEEDDOCTOR). exit 1 if not ready. */
    cubalc_run_result rr;
    int ok;
    static const char src_nd[] = "NEEDDOCTOR\nPASS\n";
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src_nd, sizeof src_nd - 1, "<cli-needdoctor>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    {
      const char *em = ok ? "" : (rr.err[0] ? rr.err : (rr.last_err[0] ? rr.last_err : "not ready"));
      char esc[240];
      size_t e = 0, k;
      for (k = 0; em[k] && e + 2 < sizeof esc; k++) {
        char c = em[k];
        if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
        else if ((unsigned char)c < 32) esc[e++] = ' ';
        else esc[e++] = c;
      }
      esc[e] = 0;
      printf("{\"schema\":\"cubalc.doctor.v1\",\"ok\":%s,\"cmd\":\"needdoctor\","
             "\"version\":\"%s\","
             "\"note\":\"hard install gate · dual of NEEDDOCTOR · exit 1 if not ready\","
             "\"err\":\"%s\"}\n",
             ok ? "true" : "false", CUBALC_LANG_VERSION, esc);
    }
    return ok ? 0 : 1;
  }
    if (strcmp(cmd, "needready") == 0 || strcmp(cmd, "need-ready") == 0 ||
      strcmp(cmd, "require-ready") == 0 || strcmp(cmd, "requireready") == 0 ||
      strcmp(cmd, "mustready") == 0) {
    /* Usability: hard install prove gate (dual of NEEDREADY). exit 1 if checklist fails.
     * Completes ready triad: cubalc ready (soft) · cubalc needready (hard) · run -Y. */
    cubalc_run_result rr;
    int ok;
    static const char src_nr[] = "NEEDREADY\nPASS\n";
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src_nr, sizeof src_nr - 1, "<cli-needready>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    {
      const char *em = ok ? "" : (rr.err[0] ? rr.err : (rr.last_err[0] ? rr.last_err : "not ready"));
      char esc[240];
      size_t e = 0, k;
      for (k = 0; em[k] && e + 2 < sizeof esc; k++) {
        char c = em[k];
        if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
        else if ((unsigned char)c < 32) esc[e++] = ' ';
        else esc[e++] = c;
      }
      esc[e] = 0;
      printf("{\"schema\":\"cubalc.ready.v1\",\"ok\":%s,\"cmd\":\"needready\","
             "\"version\":\"%s\","
             "\"note\":\"hard install prove gate · dual of NEEDREADY · exit 1 if checklist fails\","
             "\"err\":\"%s\"}\n",
             ok ? "true" : "false", CUBALC_LANG_VERSION, esc);
    }
    return ok ? 0 : 1;
  }
if (strcmp(cmd, "doctor") == 0 || strcmp(cmd, "health") == 0) {
    /* Usability: one JSON plate for agents/humans — is this install ready?
     * Also surfaces INCLUDE lib layout + nest-check readiness (plate_uniform). */
    char dir[512], protect_path[640], key_preview[16];
    int modular = 0, smx_key = 0, protect_plate = 0, bin_ok = 1;
    int libs_n = 0, lib_agent_boot = 0, lib_plate_session = 0, lib_plate_uniform = 0;
    int lib_hold_seed = 0, lib_var_guard = 0, lib_time_guard = 0, lib_fat_boot = 0;
    int lib_fat_session = 0;
    int lib_form_guard = 0;
    int lib_cap_boot = 0;
    int lib_onboard_boot = 0;
    int lib_discover_boot = 0;
    int lib_open_boot = 0;
    int lib_doctor_boot = 0;
    int lib_ready_boot = 0;
    int lib_cli_boot = 0;
    int lib_cli_guard = 0;
    int lib_cli_session = 0;
    int lib_env_guard = 0;
    int lib_env_boot = 0;
    int lib_arg_guard = 0;
    int lib_arg_boot = 0;
    int lib_time_boot = 0;
    int lib_tool_session = 0;
    int lib_product_session = 0;
    int lib_path_guard = 0, lib_path_boot = 0;
    int lib_rw_guard = 0, lib_rw_boot = 0;
    int lib_bin_guard = 0, lib_bin_boot = 0;
    int lib_lib_guard = 0, lib_lib_boot = 0;
    int lib_host_session = 0, lib_host_boot = 0;
    int lib_full_session = 0, lib_full_boot = 0;
    int lib_plate_guard = 0, lib_key_boot = 0;
    int lib_fn_guard = 0, lib_fn_boot = 0;
    int lib_class_guard = 0, lib_class_boot = 0;
    int lib_method_guard = 0, lib_method_boot = 0;
    int lib_field_guard = 0, lib_field_boot = 0;
    int lib_oop_session = 0, lib_oop_boot = 0;
    int lib_obj_guard = 0, lib_obj_boot = 0;
    int lib_live_session = 0, lib_live_boot = 0;
    int lib_service_session = 0, lib_service_boot = 0;
    int cookbook_ok = 0, for_agents_ok = 0, libdir_ok = 0;
    int include_path_set = 0, preload_set = 0;
    const char *hx = getenv("CUBALC_SMX_KEY");
    const char *kf = getenv("CUBALC_SMX_KEY_FILE");
    const char *ipath = getenv("CUBALC_INCLUDE_PATH");
    const char *preload = getenv("CUBALC_PRELOAD");
    const char *libdir = "programs/lib";
    state_dir(dir, sizeof dir);
    if (ipath && ipath[0]) include_path_set = 1;
    if (preload && preload[0]) preload_set = 1;
    snprintf(protect_path, sizeof protect_path, "%s/CORE_PROTECT.json", dir);
    modular = (access("src/lang/lang_parse.c", R_OK) == 0 &&
               access("include/lang/cubalc_lang_internal.h", R_OK) == 0 &&
               access("src/lang/lang_ops_smx.c", R_OK) == 0);
    if (hx && strlen(hx) >= 64) smx_key = 1;
    else if (kf && kf[0] && access(kf, R_OK) == 0) smx_key = 1;
    protect_plate = (access(protect_path, R_OK) == 0);
    key_preview[0] = 0;
    if (hx && strlen(hx) >= 8) {
      snprintf(key_preview, sizeof key_preview, "%.8s…", hx);
    }
    libdir_ok = (access(libdir, R_OK) == 0);
    lib_agent_boot = (access("programs/lib/agent_boot.cubalc", R_OK) == 0);
    lib_plate_session = (access("programs/lib/plate_session.cubalc", R_OK) == 0);
    lib_plate_uniform = (access("programs/lib/plate_uniform.cubalc", R_OK) == 0);
    lib_hold_seed = (access("programs/lib/hold_seed.cubalc", R_OK) == 0);
    lib_var_guard = (access("programs/lib/var_guard.cubalc", R_OK) == 0);
    lib_time_guard = (access("programs/lib/time_guard.cubalc", R_OK) == 0);
    lib_fat_boot = (access("programs/lib/fat_boot.cubalc", R_OK) == 0);
    lib_fat_session = (access("programs/lib/fat_session.cubalc", R_OK) == 0);
    lib_form_guard = (access("programs/lib/form_guard.cubalc", R_OK) == 0);
    lib_cap_boot = (access("programs/lib/cap_boot.cubalc", R_OK) == 0);
    lib_onboard_boot = (access("programs/lib/onboard_boot.cubalc", R_OK) == 0);
    lib_discover_boot = (access("programs/lib/discover_boot.cubalc", R_OK) == 0);
    lib_open_boot = (access("programs/lib/open_boot.cubalc", R_OK) == 0);
    lib_doctor_boot = (access("programs/lib/doctor_boot.cubalc", R_OK) == 0);
    lib_ready_boot = (access("programs/lib/ready_boot.cubalc", R_OK) == 0);
    lib_cli_boot = (access("programs/lib/cli_boot.cubalc", R_OK) == 0);
    lib_cli_guard = (access("programs/lib/cli_guard.cubalc", R_OK) == 0);
    lib_cli_session = (access("programs/lib/cli_session.cubalc", R_OK) == 0);
    lib_env_guard = (access("programs/lib/env_guard.cubalc", R_OK) == 0);
    lib_env_boot = (access("programs/lib/env_boot.cubalc", R_OK) == 0);
    lib_arg_guard = (access("programs/lib/arg_guard.cubalc", R_OK) == 0);
    lib_arg_boot = (access("programs/lib/arg_boot.cubalc", R_OK) == 0);
    lib_time_boot = (access("programs/lib/time_boot.cubalc", R_OK) == 0);
    lib_tool_session = (access("programs/lib/tool_session.cubalc", R_OK) == 0);
    lib_product_session = (access("programs/lib/product_session.cubalc", R_OK) == 0);
    lib_path_guard = (access("programs/lib/path_guard.cubalc", R_OK) == 0);
    lib_path_boot = (access("programs/lib/path_boot.cubalc", R_OK) == 0);
    lib_rw_guard = (access("programs/lib/rw_guard.cubalc", R_OK) == 0);
    lib_rw_boot = (access("programs/lib/rw_boot.cubalc", R_OK) == 0);
    lib_bin_guard = (access("programs/lib/bin_guard.cubalc", R_OK) == 0);
    lib_bin_boot = (access("programs/lib/bin_boot.cubalc", R_OK) == 0);
    lib_lib_guard = (access("programs/lib/lib_guard.cubalc", R_OK) == 0);
    lib_lib_boot = (access("programs/lib/lib_boot.cubalc", R_OK) == 0);
    lib_host_session = (access("programs/lib/host_session.cubalc", R_OK) == 0);
    lib_host_boot = (access("programs/lib/host_boot.cubalc", R_OK) == 0);
    lib_full_session = (access("programs/lib/full_session.cubalc", R_OK) == 0);
    lib_full_boot = (access("programs/lib/full_boot.cubalc", R_OK) == 0);
    lib_plate_guard = (access("programs/lib/plate_guard.cubalc", R_OK) == 0);
    lib_key_boot = (access("programs/lib/key_boot.cubalc", R_OK) == 0);
    lib_fn_guard = (access("programs/lib/fn_guard.cubalc", R_OK) == 0);
    lib_fn_boot = (access("programs/lib/fn_boot.cubalc", R_OK) == 0);
    lib_class_guard = (access("programs/lib/class_guard.cubalc", R_OK) == 0);
    lib_class_boot = (access("programs/lib/class_boot.cubalc", R_OK) == 0);
    lib_method_guard = (access("programs/lib/method_guard.cubalc", R_OK) == 0);
    lib_method_boot = (access("programs/lib/method_boot.cubalc", R_OK) == 0);
    lib_field_guard = (access("programs/lib/field_guard.cubalc", R_OK) == 0);
    lib_field_boot = (access("programs/lib/field_boot.cubalc", R_OK) == 0);
    lib_oop_session = (access("programs/lib/oop_session.cubalc", R_OK) == 0);
    lib_oop_boot = (access("programs/lib/oop_boot.cubalc", R_OK) == 0);
    lib_obj_guard = (access("programs/lib/obj_guard.cubalc", R_OK) == 0);
    lib_obj_boot = (access("programs/lib/obj_boot.cubalc", R_OK) == 0);
    lib_live_session = (access("programs/lib/live_session.cubalc", R_OK) == 0);
    lib_live_boot = (access("programs/lib/live_boot.cubalc", R_OK) == 0);
    lib_service_session = (access("programs/lib/service_session.cubalc", R_OK) == 0);
    lib_service_boot = (access("programs/lib/service_boot.cubalc", R_OK) == 0);
    cookbook_ok = (access("docs/COOKBOOK.md", R_OK) == 0);
    for_agents_ok = (access("docs/FOR_AGENTS.md", R_OK) == 0);
    if (libdir_ok) {
      DIR *d = opendir(libdir);
      if (d) {
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
          size_t len = strlen(de->d_name);
          if (len < 8 || strcmp(de->d_name + len - 7, ".cubalc") != 0)
            continue;
          if (de->d_name[0] == '.') continue;
          libs_n++;
        }
        closedir(d);
      }
    }
    {
      int ok = modular && (CUBALC_HOLD_FLASH == 1) && lib_agent_boot;
      printf("{\"schema\":\"cubalc.doctor.v1\",\"ok\":%s,"
             "\"version\":\"%s\",\"paradigm\":\"%s\",\"creed\":\"%s\","
             "\"hold_flash\":%d,\"hold_flash_means\":\"device_firmware_connection_safeguard\","
             "\"share\":\"%s\",\"http_required\":false,"
             "\"modular_lang\":%s,\"smx_key_configured\":%s,"
             "\"smx_key_preview\":\"%s\","
             "\"state_dir\":\"%s\",\"core_protect_plate\":%s,"
             "\"core_protect_path\":\"%s\","
             "\"bin_ok\":%s,"
             "\"libs_dir\":\"%s\",\"libs_dir_ok\":%s,\"libs_n\":%d,"
             "\"lib_agent_boot\":%s,\"lib_plate_session\":%s,"
             "\"lib_plate_uniform\":%s,\"lib_hold_seed\":%s,"
             "\"lib_var_guard\":%s,\"lib_time_guard\":%s,\"lib_fat_boot\":%s,"
             "\"lib_fat_session\":%s,\"lib_form_guard\":%s,\"lib_cap_boot\":%s,"
             "\"lib_onboard_boot\":%s,\"lib_discover_boot\":%s,\"lib_open_boot\":%s,\"lib_doctor_boot\":%s,"
             "\"lib_ready_boot\":%s,\"lib_cli_boot\":%s,\"lib_cli_guard\":%s,"
             "\"lib_cli_session\":%s,\"lib_env_guard\":%s,\"lib_env_boot\":%s,"
             "\"lib_arg_guard\":%s,\"lib_arg_boot\":%s,\"lib_time_boot\":%s,"
             "\"lib_tool_session\":%s,\"lib_product_session\":%s,"
             "\"lib_path_guard\":%s,\"lib_path_boot\":%s,"
             "\"lib_rw_guard\":%s,\"lib_rw_boot\":%s,"
             "\"lib_bin_guard\":%s,\"lib_bin_boot\":%s,"
             "\"lib_lib_guard\":%s,\"lib_lib_boot\":%s,"
             "\"lib_host_session\":%s,\"lib_host_boot\":%s,"
             "\"lib_full_session\":%s,\"lib_full_boot\":%s,"
             "\"lib_plate_guard\":%s,\"lib_key_boot\":%s,"
             "\"lib_fn_guard\":%s,\"lib_fn_boot\":%s,"
             "\"lib_class_guard\":%s,\"lib_class_boot\":%s,"
             "\"lib_method_guard\":%s,\"lib_method_boot\":%s,\"lib_field_guard\":%s,\"lib_field_boot\":%s,\"lib_oop_session\":%s,\"lib_oop_boot\":%s,\"lib_obj_guard\":%s,\"lib_obj_boot\":%s,\"lib_live_session\":%s,\"lib_live_boot\":%s,\"lib_service_session\":%s,\"lib_service_boot\":%s,"
             "\"include_path_set\":%s,\"preload_set\":%s,"
             "\"docs_cookbook\":%s,\"docs_for_agents\":%s,"
             "\"vars_max\":%d,\"varroom_forms\":true,"
             "\"run_timeout\":true,\"remain_ms_forms\":true,"
             "\"var_budget\":\"STATUS/VARS/run plate vars_n|max|full · "
             "VARROOM · INCLUDE fat_boot|fat_session|var_guard\","
             "\"nest_check\":\"%s\","
             "\"hints\":["
             "\"HOLD_FLASH device/mesh-join only · default 1 · not language PLUG\","
             "\"export CUBALC_SMX_KEY=$(openssl rand -hex 32) for P2P\","
             "\"cubalc protect · cubalc smx-bus prove-tcp\","
             "\"cubalc selftest — live usability proofs\","
             "\"cubalc onboard · INCLUDE onboard_boot · cubalc init --onboard\","
             "\"cubalc discover · INCLUDE discover_boot · cubalc init --discover\","
             "\"cubalc libs · cubalc guards · cubalc checkdeps fat_session · recipe · INCLUDE plate_uniform\","
             "\"CUBALC_INCLUDE_PATH + cubalc which name · run -I / NEEDINCLUDE\","
             "\"cubalc plate uniform agent.json role — nest value consistency\","
             "\"INCLUDE fat_session · cubalc init --fat-session durable nest (vars_max=%d)\","
             "\"cubalc run -T MS · INCLUDE time_guard · REMAIN_MS/NEEDTIME\","
             "\"INCLUDE form_guard · NEED_FORMS / NEED_FORM_ANY form contract\","
             "\"INCLUDE env_guard · NEED_ENVS / NEED_ENV_ANY host env contract\","
             "\"INCLUDE env_boot · cubalc init --env host env starter\","
             "\"INCLUDE arg_guard · NEED_ARGS / NEED_ARG_ANY program-arg contract\","
             "\"INCLUDE arg_boot · cubalc init --arg program-arg starter\","
             "\"INCLUDE time_boot · cubalc init --time wall-budget starter\","
             "\"INCLUDE tool_session · cubalc init --full-cli flags+args CLI\","
             "\"INCLUDE product_session · cubalc init --product flags+args+env+time\","
             "\"INCLUDE path_guard · NEED_PATHS / NEED_PATH_ANY host path contract\","
             "\"INCLUDE path_boot · cubalc init --path host path starter\","
             "\"INCLUDE rw_guard · NEED_READ_PATHS / NEED_WRITE_PATHS access contract\","
             "\"INCLUDE rw_boot · cubalc init --rw host access starter\","
             "\"INCLUDE bin_guard · NEED_BINS / NEED_BIN_ANY PATH tools\","
             "\"INCLUDE bin_boot · cubalc init --bin PATH tool starter\","
             "\"INCLUDE lib_guard · NEED_LIBS / NEED_LIB_ANY module contract\","
             "\"INCLUDE lib_boot · cubalc init --lib-boot stdlib gate starter\","
             "\"INCLUDE host_session · path+rw+bin+lib host readiness\","
             "\"INCLUDE host_boot · cubalc init --host host readiness starter\","
             "\"INCLUDE full_session · product+host FULL_SESSION_OK one-shot\","
             "\"INCLUDE full_boot · cubalc init --fleet product+host starter\","
             "\"INCLUDE plate_guard · NEED_KEYS / NEED_KEY_ANY plate schema\","
             "\"INCLUDE key_boot · cubalc init --key plate schema starter\","
             "\"INCLUDE fn_guard · NEED_FNALL / NEED_FNANY FN catalog\","
             "\"INCLUDE fn_boot · cubalc init --fn FN catalog starter\","
             "\"INCLUDE class_guard · NEED_CLASSES / NEED_CLASS_ANY OOP catalog\","
             "\"INCLUDE class_boot · cubalc init --class CLASS catalog starter\","
             "\"INCLUDE method_guard · NEED_METHODS / NEED_METHOD_ANY Class.method\","
             "\"INCLUDE method_boot · cubalc init --method METHOD catalog starter\","
             "\"INCLUDE field_guard · NEED_FIELDS / NEED_FIELD_ANY Class.field\","
             "\"INCLUDE field_boot · cubalc init --field FIELD catalog starter\","
             "\"INCLUDE oop_session · class+method+field OOP catalog\","
             "\"INCLUDE oop_boot · cubalc init --oop OOP catalog starter\","
             "\"INCLUDE obj_guard · NEED_OBJS / NEED_OBJ_ANY live instances\","
             "\"INCLUDE obj_boot · cubalc init --obj OBJ catalog starter\","
             "\"INCLUDE live_session · oop_session+obj_guard live OOP plane\","
             "\"INCLUDE live_boot · cubalc init --live live OOP starter\","
             "\"INCLUDE service_session · full_session+live_session fleet service\","
             "\"INCLUDE service_boot · cubalc init --service product+host+OOP\","
             "\"cubalc env · docs/COOKBOOK.md · docs/FOR_AGENTS.md\""
             "],"
             "\"cookbook\":[\"docs/COOKBOOK.md\",\"docs/P2P_SMX.md\","
             "\"docs/HOLD_FLASH.md\",\"docs/CORE_PROTECT.md\",\"docs/FOR_AGENTS.md\","
             "\"programs/hello_cube.cubalc\",\"programs/p2p/mesh_local.cubalc\","
             "\"programs/lib/plate_uniform.cubalc\",\"programs/lib/var_guard.cubalc\","
             "\"programs/lib/time_guard.cubalc\",\"programs/lib/fat_boot.cubalc\","
             "\"programs/lib/fat_session.cubalc\",\"programs/lib/form_guard.cubalc\","
             "\"programs/lib/env_guard.cubalc\",\"programs/lib/env_boot.cubalc\","
             "\"programs/lib/time_boot.cubalc\",\"programs/lib/tool_session.cubalc\","
             "\"programs/lib/product_session.cubalc\",\"programs/lib/path_guard.cubalc\","
             "\"programs/lib/path_boot.cubalc\",\"programs/lib/rw_guard.cubalc\","
             "\"programs/lib/rw_boot.cubalc\",\"programs/lib/bin_guard.cubalc\","
             "\"programs/lib/bin_boot.cubalc\",\"programs/lib/lib_guard.cubalc\","
             "\"programs/lib/lib_boot.cubalc\",\"programs/lib/host_session.cubalc\","
             "\"programs/lib/host_boot.cubalc\","
             "\"programs/lib/full_session.cubalc\",\"programs/lib/full_boot.cubalc\",\"programs/lib/plate_guard.cubalc\","
             "\"programs/lib/key_boot.cubalc\","
             "\"programs/lib/onboard_boot.cubalc\",\"programs/lib/discover_boot.cubalc\","
             "\"cubalc init --onboard\",\"cubalc init --discover\",\"cubalc init --fat-session\",\"cubalc hasforms SORTLIBS LIBAGE\"]"
             "}\n",
             ok ? "true" : "false",
             CUBALC_LANG_VERSION, CUBALC_LANG_PARADIGM, CUBALC_CREED,
             CUBALC_HOLD_FLASH, CUBALC_SHARE,
             modular ? "true" : "false",
             smx_key ? "true" : "false",
             key_preview,
             dir,
             protect_plate ? "true" : "false",
             protect_path,
             bin_ok ? "true" : "false",
             libdir,
             libdir_ok ? "true" : "false",
             libs_n,
             lib_agent_boot ? "true" : "false",
             lib_plate_session ? "true" : "false",
             lib_plate_uniform ? "true" : "false",
             lib_hold_seed ? "true" : "false",
             lib_var_guard ? "true" : "false",
             lib_time_guard ? "true" : "false",
             lib_fat_boot ? "true" : "false",
             lib_fat_session ? "true" : "false",
             lib_form_guard ? "true" : "false",
             lib_cap_boot ? "true" : "false",
             lib_onboard_boot ? "true" : "false",
             lib_discover_boot ? "true" : "false",
             lib_open_boot ? "true" : "false",
             lib_doctor_boot ? "true" : "false",
             lib_ready_boot ? "true" : "false",
             lib_cli_boot ? "true" : "false",
             lib_cli_guard ? "true" : "false",
             lib_cli_session ? "true" : "false",
             lib_env_guard ? "true" : "false",
             lib_env_boot ? "true" : "false",
             lib_arg_guard ? "true" : "false",
             lib_arg_boot ? "true" : "false",
             lib_time_boot ? "true" : "false",
             lib_tool_session ? "true" : "false",
             lib_product_session ? "true" : "false",
             lib_path_guard ? "true" : "false",
             lib_path_boot ? "true" : "false",
             lib_rw_guard ? "true" : "false",
             lib_rw_boot ? "true" : "false",
             lib_bin_guard ? "true" : "false",
             lib_bin_boot ? "true" : "false",
             lib_lib_guard ? "true" : "false",
             lib_lib_boot ? "true" : "false",
             lib_host_session ? "true" : "false",
             lib_host_boot ? "true" : "false",
             lib_full_session ? "true" : "false",
             lib_full_boot ? "true" : "false",
             lib_plate_guard ? "true" : "false",
             lib_key_boot ? "true" : "false",
             lib_fn_guard ? "true" : "false",
             lib_fn_boot ? "true" : "false",
             lib_class_guard ? "true" : "false",
             lib_class_boot ? "true" : "false",
             lib_method_guard ? "true" : "false",
             lib_method_boot ? "true" : "false",
             lib_field_guard ? "true" : "false",
             lib_field_boot ? "true" : "false",
             lib_oop_session ? "true" : "false",
             lib_oop_boot ? "true" : "false",
             lib_obj_guard ? "true" : "false",
             lib_obj_boot ? "true" : "false",
             lib_live_session ? "true" : "false",
             lib_live_boot ? "true" : "false",
             lib_service_session ? "true" : "false",
             lib_service_boot ? "true" : "false",
             include_path_set ? "true" : "false",
             preload_set ? "true" : "false",
             cookbook_ok ? "true" : "false",
             for_agents_ok ? "true" : "false",
             CUBALC_MAX_VARS,
             lib_plate_uniform
                 ? "INCLUDE plate_uniform · cubalc plate uniform"
                 : "missing programs/lib/plate_uniform.cubalc",
             CUBALC_MAX_VARS);
      return ok ? 0 : 1;
    }
  }
  if (strcmp(cmd, "selftest") == 0 || strcmp(cmd, "smoke") == 0 ||
      strcmp(cmd, "usability-test") == 0 || strcmp(cmd, "prove-usability") == 0) {
    /* Usability: live curated proof runner — doctor is static; this executes.
     * Agents: cubalc selftest → cubalc.selftest.v1 plate (pass/fail per item).
     * Skips network P2P proofs; focuses INCLUDE/ASSERT/EXPECT/VERSION/WHICH. */
    static const struct {
      const char *id;
      const char *path;
      const char *hint;
    } tests[] = {
      {"hello", "programs/hello_cube.cubalc", "hold place plug short form"},
      {"hold", "programs/proof/12_hold_flash_plug.cubalc", "HOLD_FLASH before PLUG"},
      {"env_assert", "programs/proof/573_env_or_assert_msg.cubalc", "ENV OR + ASSERT msg"},
      {"include", "programs/proof/577_include_shortname.cubalc", "INCLUDE short lib name"},
      {"expect", "programs/proof/578_expect_soft.cubalc", "EXPECT soft assert"},
      {"fail_pass", "programs/proof/579_fail_pass.cubalc", "FAIL/PASS soft status"},
      {"version", "programs/proof/580_version.cubalc", "VERSION form plate"},
      {"which_lib", "programs/proof/582_sys_which_lib.cubalc", "SYS WHICH lib resolve"},
      {"require", "programs/proof/583_require_version.cubalc", "REQUIRE VERSION gate"},
      {"require_lib", "programs/proof/590_require_lib.cubalc", "REQUIRE LIB fail-fast gate"},
      {"cwd", "programs/proof/584_sys_cwd_state_root.cubalc", "SYS CWD/STATE/ROOT paths"},
      {"include_soft", "programs/proof/585_include_soft.cubalc", "INCLUDE OR soft miss"},
      {"vars", "programs/proof/586_vars.cubalc", "VARS full var table JSON"},
      {"agent_boot", "programs/proof/587_agent_boot.cubalc", "INCLUDE agent_boot preamble"},
      {"status", "programs/proof/588_status.cubalc", "STATUS agent health plate"},
      {"clear_err", "programs/proof/589_clear_err.cubalc", "CLEAR_ERR wipe sticky LAST_ERR"},
      {"sys_ms", "programs/proof/591_sys_ms.cubalc", "SYS MS wall milliseconds"},
      {"note", "programs/proof/592_note.cubalc", "NOTE agent breadcrumb"},
      {"exit", "programs/proof/593_exit.cubalc", "EXIT early halt with code"},
      {"include_once", "programs/proof/594_include_once.cubalc", "INCLUDE ONCE skip reload"},
      {"sys_pid_host", "programs/proof/595_sys_pid_hostname.cubalc", "SYS PID/HOSTNAME host identity"},
      {"sys_user", "programs/proof/596_sys_user_uid_home.cubalc", "SYS USER/UID/HOME user identity"},
      {"default", "programs/proof/597_default.cubalc", "DEFAULT set-if-unset for INCLUDE libs"},
      {"defined", "programs/proof/598_defined.cubalc", "DEFINED var existence probe"},
      {"typeof", "programs/proof/599_typeof.cubalc", "TYPEOF num|str|undef kind probe"},
      {"unset", "programs/proof/600_unset.cubalc", "UNSET remove var for DEFAULT re-apply"},
      {"identity", "programs/proof/601_identity.cubalc", "IDENTITY host/process plate"},
      {"sys_date", "programs/proof/602_sys_date.cubalc", "SYS DATE UTC ISO stamp"},
      {"sys_tmp", "programs/proof/603_sys_tmp.cubalc", "SYS TMP portable temp dir"},
      {"sys_mkdir", "programs/proof/604_sys_mkdir.cubalc", "SYS MKDIR mkdir -p for plates"},
      {"sys_basename", "programs/proof/605_sys_basename.cubalc", "SYS BASENAME/DIRNAME path split"},
      {"sys_extname", "programs/proof/606_sys_extname.cubalc", "SYS EXTNAME/STEM path peel"},
      {"sys_size", "programs/proof/607_sys_size.cubalc", "SYS SIZE/ISDIR/ISFILE path meta"},
      {"sys_read_soft", "programs/proof/608_sys_read_soft.cubalc", "SYS READ OR soft optional plate"},
      {"sys_rm_rename", "programs/proof/609_sys_rm_rename.cubalc", "SYS RM/RENAME plate lifecycle"},
      {"sys_copy", "programs/proof/610_sys_copy.cubalc", "SYS COPY duplicate plate files"},
      {"sys_realpath", "programs/proof/611_sys_realpath.cubalc", "SYS REALPATH/ABSPATH absolute paths"},
      {"sys_touch", "programs/proof/612_sys_touch.cubalc", "SYS TOUCH plate markers"},
      {"sys_sleep", "programs/proof/613_sys_sleep.cubalc", "SYS SLEEP agent backoff ms"},
      {"sys_list", "programs/proof/614_sys_list.cubalc", "SYS LIST/LS directory basenames"},
      {"sys_nth", "programs/proof/615_sys_nth.cubalc", "SYS NTH/LINE/HEAD/TAIL field peel"},
      {"sys_grep", "programs/proof/616_sys_grep.cubalc", "SYS GREP/FILTER/GREPV line filter"},
      {"each_line", "programs/proof/617_each_line.cubalc", "EACH LINE walk LIST/GREP fields"},
      {"sys_take", "programs/proof/618_sys_take.cubalc", "SYS TAKE/DROP line windows"},
      {"sys_split", "programs/proof/619_sys_split.cubalc", "SYS SPLIT sep→newline fields"},
      {"sys_sort", "programs/proof/620_sys_sort.cubalc", "SYS SORT/UNIQ line order"},
      {"sys_join", "programs/proof/621_sys_join.cubalc", "SYS JOIN/PATH plate path join"},
      {"sys_joinlines", "programs/proof/622_sys_joinlines.cubalc", "SYS JOINLINES paste lines"},
      {"sys_replaceall", "programs/proof/623_sys_replaceall.cubalc", "SYS REPLACEALL template subst"},
      {"sys_append", "programs/proof/624_sys_append.cubalc", "SYS APPEND/LOG plate history lines"},
      {"sys_grepi", "programs/proof/625_sys_grepi.cubalc", "SYS GREPI case-insensitive filter"},
      {"sys_str", "programs/proof/626_sys_str.cubalc", "SYS STR/ITOA num→decimal string"},
      {"sys_before", "programs/proof/627_sys_before.cubalc", "SYS BEFORE/AFTER peel at needle"},
      {"sys_push", "programs/proof/628_sys_push.cubalc", "SYS PUSH/ADDLINE field accumulate"},
      {"sys_eqsi", "programs/proof/629_sys_eqsi.cubalc", "SYS EQSI/HASI case-insensitive probes"},
      {"sys_lines", "programs/proof/630_sys_lines.cubalc", "SYS LINES/WC newline field count"},
      {"sys_cut", "programs/proof/631_sys_cut.cubalc", "SYS CUT/COLUMN field by separator"},
      {"sys_empty", "programs/proof/632_sys_empty.cubalc", "SYS EMPTY/BLANK soft plate probes"},
      {"sys_pop", "programs/proof/633_sys_pop.cubalc", "SYS POP/POPLINE bag field peel"},
      {"sys_mtime", "programs/proof/634_sys_mtime.cubalc", "SYS MTIME/AGE plate freshness"},
      {"sys_startsi", "programs/proof/635_sys_startsi.cubalc", "SYS STARTSI/ENDSI case-insensitive affix"},
      {"sys_between", "programs/proof/636_sys_between.cubalc", "SYS BETWEEN peel between delimiters"},
      {"sys_findi", "programs/proof/637_sys_findi.cubalc", "SYS FINDI case-insensitive index"},
      {"sys_env_set", "programs/proof/638_sys_env_set.cubalc", "SYS ENV SET/UNSET process env"},
      {"sys_revl", "programs/proof/639_sys_revl.cubalc", "SYS REVL reverse bag line order"},
      {"sys_words", "programs/proof/640_sys_words.cubalc", "SYS WORDS whitespace tokenize to lines"},
      {"sys_hasline", "programs/proof/641_sys_hasline.cubalc", "SYS HASLINE exact bag field membership"},
      {"sys_prepend", "programs/proof/642_sys_prepend.cubalc", "SYS PREPEND/POPHEAD FIFO bag front ops"},
      {"sys_removeline", "programs/proof/643_sys_removeline.cubalc", "SYS REMOVELINE drop exact bag field"},
      {"sys_findline", "programs/proof/644_sys_findline.cubalc", "SYS FINDLINE exact bag field index"},
      {"sys_setline", "programs/proof/645_sys_setline.cubalc", "SYS SETLINE set bag field by index"},
      {"sys_insertline", "programs/proof/646_sys_insertline.cubalc", "SYS INSERTLINE insert bag field by index"},
      {"sys_dropnth", "programs/proof/647_sys_dropnth.cubalc", "SYS DROPNTH drop bag field by index"},
      {"sys_moveline", "programs/proof/648_sys_moveline.cubalc", "SYS MOVELINE move bag field by index"},
      {"sys_countline", "programs/proof/649_sys_countline.cubalc", "SYS COUNTLINE count exact bag field matches"},
      {"sys_setmatch", "programs/proof/650_sys_setmatch.cubalc", "SYS SETMATCH replace first exact bag field"},
      {"run_expr", "cli:run -e", "cubalc run -e inline one-liner (no temp file)"},
      {"sys_chdir", "programs/proof/651_sys_chdir.cubalc", "SYS CHDIR/CD change process cwd"},
      {"sys_rand", "programs/proof/652_sys_rand.cubalc", "SYS RAND/RANDOM uniform int jitter"},
      {"sys_minmax", "programs/proof/653_sys_minmax.cubalc", "SYS MIN/MAX/CLAMP host bounds"},
      {"sys_cmp", "programs/proof/654_sys_cmp.cubalc", "SYS CMP/SCMP/IABS three-way compare"},
      {"sys_sum", "programs/proof/655_sys_sum.cubalc", "SYS SUM/PROD/AVG host aggregates"},
      {"sys_sortn", "programs/proof/656_sys_sortn.cubalc", "SYS SORTN numeric bag sort"},
      {"sys_range", "programs/proof/657_sys_range.cubalc", "SYS RANGE/SEQ/IOTA number bags"},
      {"sys_div", "programs/proof/658_sys_div.cubalc", "SYS SIGN/DIV/MOD host arithmetic"},
      {"sys_pick", "programs/proof/659_sys_pick.cubalc", "SYS PICK/CHOICE random bag field"},
      {"sys_gcd", "programs/proof/660_sys_gcd.cubalc", "SYS GCD/LCM multi-arg host math"},
      {"sys_in", "programs/proof/661_sys_in.cubalc", "SYS IN/WITHIN numeric range membership"},
      {"sys_pow", "programs/proof/662_sys_pow.cubalc", "SYS POW/ISQRT host power and sqrt"},
      {"sys_shuffle", "programs/proof/663_sys_shuffle.cubalc", "SYS SHUFFLE randomize bag field order"},
      {"sys_coalesce", "programs/proof/664_sys_coalesce.cubalc", "SYS COALESCE first non-empty string"},
      {"sys_union", "programs/proof/665_sys_union.cubalc", "SYS UNION/INTERSECT/DIFF bag set ops"},
      {"sys_zip", "programs/proof/666_sys_zip.cubalc", "SYS ZIP/KEYS/VALS bag pair and peel"},
      {"sys_prefixall", "programs/proof/667_sys_prefixall.cubalc", "SYS PREFIXALL/SUFFIXALL tag bag fields"},
      {"sys_fill", "programs/proof/668_sys_fill.cubalc", "SYS FILL n value bag of repeated fields"},
      {"sys_enumerate", "programs/proof/669_sys_enumerate.cubalc", "SYS ENUMERATE index-prefix bag fields"},
      {"sys_squeeze", "programs/proof/670_sys_squeeze.cubalc", "SYS SQUEEZE drop empty bag fields"},
      {"sys_median", "programs/proof/671_sys_median.cubalc", "SYS MEDIAN integer bag/args median"},
      {"sys_trimall", "programs/proof/672_sys_trimall.cubalc", "SYS TRIMALL trim every bag field"},
      {"sys_midlines", "programs/proof/673_sys_midlines.cubalc", "SYS MIDLINES bag field window slice"},
      {"sys_grepany", "programs/proof/674_sys_grepany.cubalc", "SYS GREPANY multi-needle bag filter"},
      {"sys_grepall", "programs/proof/675_sys_grepall.cubalc", "SYS GREPALL multi-needle AND bag filter"},
      {"require_env", "programs/proof/676_require_env.cubalc", "REQUIRE ENV fail-fast host config"},
      {"sys_upperall", "programs/proof/677_sys_upperall.cubalc", "SYS UPPERALL/LOWERALL case-map bag fields"},
      {"sys_mapreplace", "programs/proof/678_sys_mapreplace.cubalc", "SYS MAPREPLACE bag-wide GSUB"},
      {"sys_freq", "programs/proof/679_sys_freq.cubalc", "SYS FREQ bag field frequency histogram"},
      {"sys_cutall", "programs/proof/680_sys_cutall.cubalc", "SYS CUTALL peel column from every bag line"},
      {"sys_sortfreq", "programs/proof/681_sys_sortfreq.cubalc", "SYS SORTFREQ sort FREQ bag by count"},
      {"sys_beforeall", "programs/proof/682_sys_beforeall.cubalc", "SYS BEFOREALL/AFTERALL peel every bag field"},
      {"sys_firstmatch", "programs/proof/683_sys_firstmatch.cubalc", "SYS FIRSTMATCH/GREP1 first bag field hit"},
      {"sys_chunk", "programs/proof/684_sys_chunk.cubalc", "SYS CHUNK/BATCH group bag fields by n"},
      {"sys_window", "programs/proof/685_sys_window.cubalc", "SYS WINDOW/SLIDE overlapping bag field windows"},
      {"sys_stride", "programs/proof/686_sys_stride.cubalc", "SYS STRIDE/EVERY partition bag by step/offset"},
      {"sys_rotate", "programs/proof/687_sys_rotate.cubalc", "SYS ROTATE/ROTL/ROTR bag field round-robin"},
      {"sys_flatten", "programs/proof/688_sys_flatten.cubalc", "SYS FLATTEN/UNCHUNK expand joined bag fields"},
      {"sys_countmatch", "programs/proof/689_sys_countmatch.cubalc", "SYS COUNTMATCH/GREPCOUNT substring field count"},
      {"sys_lenall", "programs/proof/690_sys_lenall.cubalc", "SYS LENALL/MAPLEN per-field lengths bag"},
      {"sys_minmax_bag", "programs/proof/691_sys_minmax_bag.cubalc", "SYS MIN/MAX bag mode like SUM"},
      {"sys_argmax", "programs/proof/692_sys_argmax.cubalc", "SYS ARGMAX/ARGMIN index of extreme"},
      {"sys_padall", "programs/proof/693_sys_padall.cubalc", "SYS PADALL/LPADALL pad every bag field"},
      {"sys_sortlen", "programs/proof/694_sys_sortlen.cubalc", "SYS SORTLEN sort bag fields by length"},
      {"sys_lastmatch", "programs/proof/695_sys_lastmatch.cubalc", "SYS LASTMATCH/GREP1L last bag field hit"},
      {"sys_maxlen", "programs/proof/696_sys_maxlen.cubalc", "SYS MAXLEN/MINLEN max/min bag field length"},
      {"sys_truncall", "programs/proof/697_sys_truncall.cubalc", "SYS TRUNCALL/CLIPALL truncate bag fields"},
      {"sys_longest", "programs/proof/698_sys_longest.cubalc", "SYS LONGEST/SHORTEST pick bag field by length"},
      {"sys_commonprefix", "programs/proof/699_sys_commonprefix.cubalc", "SYS COMMONPREFIX/LCP shared bag prefix"},
      {"sys_stripprefix", "programs/proof/700_sys_stripprefix.cubalc", "SYS STRIPPREFIX/STRIPCOMMON relative bag paths"},
      {"sys_drawn", "programs/proof/701_sys_drawn.cubalc", "SYS DRAWN/SAMPLEK sample k unique bag fields"},
      {"sys_lookup", "programs/proof/702_sys_lookup.cubalc", "SYS LOOKUP/KVGET peel key:val bag value"},
      {"sys_kvset", "programs/proof/703_sys_kvset.cubalc", "SYS KVSET/SETKV set/update key:val bag field"},
      {"sys_kvinc", "programs/proof/704_sys_kvinc.cubalc", "SYS KVINC/INCKV bump key:val numeric counter"},
      {"sys_kvdel", "programs/proof/705_sys_kvdel.cubalc", "SYS KVDEL/DELKV drop key:val bag field"},
      {"sys_lookup_or", "programs/proof/706_sys_lookup_or.cubalc", "SYS LOOKUP bag key OR fallback default"},
      {"sys_mergekv", "programs/proof/707_sys_mergekv.cubalc", "SYS MERGEKV/KVADDALL merge key:val bags by sum"},
      {"sys_sumkv", "programs/proof/708_sys_sumkv.cubalc", "SYS SUMKV/TOTALKV sum key:val bag values"},
      {"sys_topkey", "programs/proof/709_sys_topkey.cubalc", "SYS TOPKEY/BOTKEY max/min key of key:val bag"},
      {"sys_lookupn", "programs/proof/710_sys_lookupn.cubalc", "SYS LOOKUPN/KVGETN peel key:val as int LAST_N"},
      {"sys_threshkv", "programs/proof/711_sys_threshkv.cubalc", "SYS THRESHKV/KEEPVAL keep key:val value>=min"},
      {"sys_pctkv", "programs/proof/712_sys_pctkv.cubalc", "SYS PCTKV/SHAREKV key:val integer percent of total"},
      {"sys_capkv", "programs/proof/713_sys_capkv.cubalc", "SYS CAPKV/CLAMPKV clamp key:val values to max"},
      {"sys_scalekv", "programs/proof/714_sys_scalekv.cubalc", "SYS SCALEKV/MULKV multiply key:val values by factor"},
      {"sys_diffkv", "programs/proof/715_sys_diffkv.cubalc", "SYS DIFFKV/SUBKV subtract key:val bags by key"},
      {"sys_addkv", "programs/proof/716_sys_addkv.cubalc", "SYS ADDKV/OFFSETKV add delta to key:val values"},
      {"sys_dropzero", "programs/proof/717_sys_dropzero.cubalc", "SYS DROPZERO/KEEPNZ drop key:val value==0"},
      {"sys_abskv", "programs/proof/718_sys_abskv.cubalc", "SYS ABSKV/MAGKV absolute key:val values"},
      {"sys_signkv", "programs/proof/719_sys_signkv.cubalc", "SYS SIGNKV/DIRKV map key:val to -1|0|1"},
      {"sys_divkv", "programs/proof/720_sys_divkv.cubalc", "SYS DIVKV/IDIVKV integer-divide key:val values"},
      {"sys_avgkv", "programs/proof/721_sys_avgkv.cubalc", "SYS AVGKV/MEANKV integer mean of key:val values"},
      {"sys_mediankv", "programs/proof/722_sys_mediankv.cubalc", "SYS MEDIANKV/P50KV median of key:val values"},
      {"sys_keepkey", "programs/proof/723_sys_keepkey.cubalc", "SYS KEEPKEY/GREPKEY filter key:val by key needle"},
      {"sys_glob", "programs/proof/724_sys_glob.cubalc", "SYS GLOB/MATCHFILES list basenames matching pattern"},
      {"sys_pathglob", "programs/proof/725_sys_pathglob.cubalc", "SYS PATHGLOB/PGLOB full paths matching shell pattern"},
      {"require_path", "programs/proof/726_require_path.cubalc", "REQUIRE PATH/DIR/REG fail-fast host path gates"},
      {"sys_filterglob", "programs/proof/727_sys_filterglob.cubalc", "SYS FILTERGLOB/MATCHBAG shell-pattern bag filter"},
      {"sys_basenameall", "programs/proof/728_sys_basenameall.cubalc", "SYS BASENAMEALL/DIRNAMEALL/EXTALL/STEMALL path bag maps"},
      {"sys_keepfiles", "programs/proof/729_sys_keepfiles.cubalc", "SYS KEEPFILES/KEEPDIRS/KEEPEXIST path bag kind filters"},
      {"sys_sizeall", "programs/proof/730_sys_sizeall.cubalc", "SYS SIZEALL/MAPSIZE path bag byte sizes + sum"},
      {"sys_mtimeall", "programs/proof/731_sys_mtimeall.cubalc", "SYS MTIMEALL/AGEALL path bag time maps + min/max"},
      {"sys_newest", "programs/proof/732_sys_newest.cubalc", "SYS NEWEST/OLDEST/LARGEST/SMALLEST path bag pick"},
      {"sys_sortmtime", "programs/proof/733_sys_sortmtime.cubalc", "SYS SORTMTIME/SORTSIZE order path bags by time/size"},
      {"sys_fresh", "programs/proof/734_sys_fresh.cubalc", "SYS FRESH/KEEPSTALE filter path bags by age threshold"},
      {"sys_keepnewer", "programs/proof/735_sys_keepnewer.cubalc", "SYS KEEPNEWER/KEEPOLDER filter path bags vs ref mtime"},
      {"sys_keepbigger", "programs/proof/736_sys_keepbigger.cubalc", "SYS KEEPBIGGER/KEEPSMALLER filter path bags by size"},
      {"sys_rmall", "programs/proof/737_sys_rmall.cubalc", "SYS RMALL/UNLINKALL bulk remove path bag"},
      {"sys_touchall", "programs/proof/738_sys_touchall.cubalc", "SYS TOUCHALL/ENSUREALL bulk create/refresh path bag"},
      {"sys_copyall", "programs/proof/739_sys_copyall.cubalc", "SYS COPYALL/CPALL bulk copy path bag into dest dir"},
      {"sys_mkdirall", "programs/proof/740_sys_mkdirall.cubalc", "SYS MKDIRALL/ENSUREDIRS bulk mkdir -p path bag"},
      {"sys_moveall", "programs/proof/741_sys_moveall.cubalc", "SYS MOVEALL/MVALL bulk move path bag into dest dir"},
      {"sys_walk", "programs/proof/742_sys_walk.cubalc", "SYS WALK/FINDALL recursive full-path discovery"},
      {"sys_eqfile", "programs/proof/743_sys_eqfile.cubalc", "SYS EQFILE/SAMEFILE identical file content probe"},
      {"sys_logall", "programs/proof/744_sys_logall.cubalc", "SYS LOGALL/APPENDFILES append line to path bag"},
      {"sys_grepfiles", "programs/proof/745_sys_grepfiles.cubalc", "SYS GREPFILES/SEARCHFILES filter paths by file content"},
      {"sys_readall", "programs/proof/746_sys_readall.cubalc", "SYS READALL/CATFILES concat path bag file contents"},
      {"sys_writeall", "programs/proof/747_sys_writeall.cubalc", "SYS WRITEALL/WRITEFILES overwrite path bag with data"},
      {"sys_replacefiles", "programs/proof/748_sys_replacefiles.cubalc", "SYS REPLACEFILES/SUBFILES bulk rewrite path bag content"},
      {"sys_countinfiles", "programs/proof/749_sys_countinfiles.cubalc", "SYS COUNTINFILES/GREPCOUNTFILES total needle hits in path bag"},
      {"sys_firstfile", "programs/proof/750_sys_firstfile.cubalc", "SYS FIRSTFILE/LASTFILE first/last path with content match"},
      {"sys_greplines", "programs/proof/751_sys_greplines.cubalc", "SYS GREPLINES/EXTRACTLINES collect matching lines from path bag"},
      {"sys_headfile", "programs/proof/752_sys_headfile.cubalc", "SYS HEADFILE/TAILFILE first/last n lines of a file"},
      {"sys_linecountall", "programs/proof/753_sys_linecountall.cubalc", "SYS LINECOUNTALL/WCALL path bag line counts + sum"},
      {"sys_midfile", "programs/proof/754_sys_midfile.cubalc", "SYS MIDFILE/LINESLICE file line window extract"},
      {"sys_lineat", "programs/proof/755_sys_lineat.cubalc", "SYS LINEAT/FILELINE single file line by index"},
      {"sys_setfileline", "programs/proof/756_sys_setfileline.cubalc", "SYS SETFILELINE/FILESETLINE replace file line by index"},
      {"sys_insertfileline", "programs/proof/757_sys_insertfileline.cubalc", "SYS INSERTFILELINE/FILEINSERTLINE insert file line by index"},
      {"sys_dropfileline", "programs/proof/758_sys_dropfileline.cubalc", "SYS DROPFILELINE/FILEDROPLINE drop file line by index"},
      {"sys_findfileline", "programs/proof/759_sys_findfileline.cubalc", "SYS FINDFILELINE/FILEFINDLINE first file line index by needle"},
      {"sys_lastfindfileline", "programs/proof/760_sys_lastfindfileline.cubalc", "SYS LASTFINDFILELINE/FILELASTFIND last file line by needle"},
      {"sys_countfileline", "programs/proof/761_sys_countfileline.cubalc", "SYS COUNTFILELINE/FILECOUNTLINE count matching file lines"},
      {"sys_grepfile", "programs/proof/762_sys_grepfile.cubalc", "SYS GREPFILE/FILEGREP keep matching lines from one file"},
      {"sys_setmatchfile", "programs/proof/763_sys_setmatchfile.cubalc", "SYS SETMATCHFILE/FILESETMATCH replace first matching file line"},
      {"sys_dropmatchfile", "programs/proof/764_sys_dropmatchfile.cubalc", "SYS DROPMATCHFILE/FILEDROPMATCH drop first matching file line"},
      {"sys_dropallmatchfile", "programs/proof/765_sys_dropallmatchfile.cubalc", "SYS DROPALLMATCHFILE/FILEDROPALLMATCH drop all matching file lines"},
      {"sys_setallmatchfile", "programs/proof/766_sys_setallmatchfile.cubalc", "SYS SETALLMATCHFILE/FILESETALLMATCH replace all matching file lines"},
      {"sys_insertmatchfile", "programs/proof/767_sys_insertmatchfile.cubalc", "SYS INSERTMATCHFILE/FILEINSERTMATCH insert line after first match"},
      {"sys_beforematchfile", "programs/proof/768_sys_beforematchfile.cubalc", "SYS BEFOREMATCHFILE/INSERTBEFOREMATCH insert line before first match"},
      {"sys_lookupfile", "programs/proof/769_sys_lookupfile.cubalc", "SYS LOOKUPFILE/FILELOOKUP peel key=val from plate file"},
      {"sys_kvfileset", "programs/proof/770_sys_kvfileset.cubalc", "SYS KVFILESET/FILEKVSET set/update key=val in plate file"},
      {"sys_kvfiledel", "programs/proof/771_sys_kvfiledel.cubalc", "SYS KVFILEDEL/FILEKVDEL drop key=val line from plate file"},
      {"sys_kvfileinc", "programs/proof/772_sys_kvfileinc.cubalc", "SYS KVFILEINC/INCKVFILE bump numeric key=val in plate file"},
      {"sys_ensureline", "programs/proof/773_sys_ensureline.cubalc", "SYS ENSURELINE/ENSUREFILELINE append line if exact missing"},
      {"sys_hasfileline", "programs/proof/774_sys_hasfileline.cubalc", "SYS HASFILELINE/FILEHASLINE exact file line membership"},
      {"sys_dropexactfile", "programs/proof/775_sys_dropexactfile.cubalc", "SYS DROPEXACTFILE/DROPFILEEXACT drop first exact file line"},
      {"sys_toggleline", "programs/proof/776_sys_toggleline.cubalc", "SYS TOGGLELINE/TOGGLEFILELINE flip exact file line presence"},
      {"sys_kvfileensure", "programs/proof/777_sys_kvfileensure.cubalc", "SYS KVFILEENSURE/ENSUREKVFILE set key=val only if missing"},
      {"sys_substenv", "programs/proof/778_sys_substenv.cubalc", "SYS SUBSTENV/ENVSUBST expand $VAR and ${VAR} templates"},
      {"sys_bidir_flow", "programs/proof/779_sys_bidir_flow.cubalc", "SYS SWAPFILES/DUPLEX/TCPXFER bidirectional any-data host flow"},
      {"sys_waitfile", "programs/proof/781_sys_waitfile.cubalc", "SYS WAITFILE/WAITPATH poll until path exists"},
      {"sys_substfile", "programs/proof/782_sys_substfile.cubalc", "SYS SUBSTFILE/EXPANDFILE materialize $VAR template file"},
      {"sys_hash", "programs/proof/783_sys_hash.cubalc", "SYS HASH/HASHFILE FNV-1a plate content fingerprint"},
      {"sys_waitmatch", "programs/proof/784_sys_waitmatch.cubalc", "SYS WAITMATCH/WAITCONTAINS poll until file contains needle"},
      {"sys_writeatomic", "programs/proof/785_sys_writeatomic.cubalc", "SYS WRITEATOMIC/SAFEWRITE temp+rename plate write"},
      {"sys_lockfile", "programs/proof/786_sys_lockfile.cubalc", "SYS LOCKFILE/UNLOCKFILE exclusive agent plate locks"},
      {"sys_waitgone", "programs/proof/787_sys_waitgone.cubalc", "SYS WAITGONE/WAITMISSING poll until path does not exist"},
      {"sys_waitstable", "programs/proof/788_sys_waitstable.cubalc", "SYS WAITSTABLE/WAITQUIET poll until size+mtime quiet"},
      {"sys_waitchanged", "programs/proof/789_sys_waitchanged.cubalc", "SYS WAITCHANGED/WAITMODIFIED poll until size/mtime change"},
      {"sys_mktemp", "programs/proof/790_sys_mktemp.cubalc", "SYS MKTEMP/TEMPFILE unique empty file under TMP"},
      {"sys_claim", "programs/proof/791_sys_claim.cubalc", "SYS CLAIM/CREATEEXCL exclusive path work-item claim"},
      {"sys_stat", "programs/proof/792_sys_stat.cubalc", "SYS STAT/FSTAT one-shot path metadata probe"},
      {"sys_mktempdir", "programs/proof/793_sys_mktempdir.cubalc", "SYS MKTEMPDIR/MKDTEMP unique empty dir under TMP"},
      {"sys_rmdir", "programs/proof/794_sys_rmdir.cubalc", "SYS RMDIR/REMOVEDIR remove empty directory"},
      {"sys_base64", "programs/proof/795_sys_base64.cubalc", "SYS BASE64/BASE64D encode/decode plate payloads"},
      {"sys_hex", "programs/proof/796_sys_hex.cubalc", "SYS HEX/HEXD encode/decode plate bytes"},
      {"sys_urlenc", "programs/proof/797_sys_urlenc.cubalc", "SYS URLENC/URLDEC percent-encode plate paths"},
      {"sys_uuid", "programs/proof/798_sys_uuid.cubalc", "SYS UUID/UUID0 RFC4122 v4 agent work ids"},
      {"sys_jsonesc", "programs/proof/799_sys_jsonesc.cubalc", "SYS JSONESC/JSONUNESC plate string escape"},
      {"sys_crc32", "programs/proof/800_sys_crc32.cubalc", "SYS CRC32/CRC32FILE IEEE integrity stamps"},
      {"sys_symlink", "programs/proof/801_sys_symlink.cubalc", "SYS SYMLINK/READLINK plate path aliases"},
      {"sys_humansize", "programs/proof/802_sys_humansize.cubalc", "SYS HUMANSIZE integer B/K/M/G size labels"},
      {"sys_islink", "programs/proof/803_sys_islink.cubalc", "SYS ISLINK probe symlink paths"},
      {"sys_mode", "programs/proof/804_sys_mode.cubalc", "SYS MODE/CHMOD plate permission probe/set"},
      {"sys_parsesize", "programs/proof/805_sys_parsesize.cubalc", "SYS PARSESIZE human size string to bytes"},
      {"sys_monotonic", "programs/proof/806_sys_monotonic.cubalc", "SYS MONOTONIC steady ms for elapsed timing"},
      {"sys_nproc", "programs/proof/807_sys_nproc.cubalc", "SYS NPROC/CPUS online processor count"},
      {"sys_uname", "programs/proof/808_sys_uname.cubalc", "SYS UNAME/OS/ARCH/PLATFORM host identity"},
      {"sys_local", "programs/proof/809_sys_local.cubalc", "SYS LOCAL/LOCALTIME local wall stamp"},
      {"require_bin", "programs/proof/810_require_bin.cubalc", "REQUIRE BIN/SYS WHICHBIN PATH tool gates (+ string-var names)"},
      {"sys_isatty", "programs/proof/811_sys_isatty.cubalc", "SYS ISATTY/TTY terminal stream probe"},
      {"sys_loadavg", "programs/proof/812_sys_loadavg.cubalc", "SYS LOADAVG/LOAD host load averages"},
      {"sys_uptime", "programs/proof/813_sys_uptime.cubalc", "SYS UPTIME/BOOTAGE seconds since boot"},
      {"sys_mem", "programs/proof/814_sys_mem.cubalc", "SYS MEM/MEMINFO host RAM total/avail kB"},
      {"sys_df", "programs/proof/815_sys_df.cubalc", "SYS DF/DISKFREE filesystem free kB"},
      {"sys_fsync", "programs/proof/816_sys_fsync.cubalc", "SYS FSYNC/SYNCFILE durable file flush"},
      {"sys_dotenv", "programs/proof/817_sys_dotenv.cubalc", "SYS DOTENV/LOADENV load KEY=VAL plate into env"},
      {"sys_dumpenv", "programs/proof/818_sys_dumpenv.cubalc", "SYS DUMPENV/ENVALL process env KEY=VAL bag"},
      {"sys_writeenv", "programs/proof/819_sys_writeenv.cubalc", "SYS WRITEENV/SAVENV persist env KEY=VAL plate"},
      {"sys_truncate", "programs/proof/820_sys_truncate.cubalc", "SYS TRUNCATE/FTRUNCATE resize plate files"},
      {"sys_dirsync", "programs/proof/821_sys_dirsync.cubalc", "SYS DIRSYNC/FSYNCDIR durable dir flush"},
      {"sys_hardlink", "programs/proof/822_sys_hardlink.cubalc", "SYS HARDLINK/NLINK shared plate inodes"},
      {"sys_sameinode", "programs/proof/823_sys_sameinode.cubalc", "SYS SAMEINODE/INODE hard-link identity"},
      {"sys_tee", "programs/proof/824_sys_tee.cubalc", "SYS TEE write keep content in LAST"},
      {"sys_fileuid", "programs/proof/825_sys_fileuid.cubalc", "SYS FILEUID/FILEGID/OWNEDBY ownership probes"},
      {"sys_canread", "programs/proof/826_sys_canread.cubalc", "SYS CANREAD/CANWRITE/CANEXEC access probes"},
      {"sys_ownername", "programs/proof/827_sys_ownername.cubalc", "SYS OWNERNAME/GROUPNAME login and group name probes"},
      {"sys_cancreate", "programs/proof/828_sys_cancreate.cubalc", "SYS CANCREATE pre-flight create/overwrite probe"},
      {"sys_umask", "programs/proof/829_sys_umask.cubalc", "SYS UMASK get/set process file-creation mask"},
      {"sys_pushd", "programs/proof/830_sys_pushd.cubalc", "SYS PUSHD/POPD/DIRSTACK cwd stack for temp chdir"},
      {"sys_kindstr", "programs/proof/831_sys_kindstr.cubalc", "SYS KINDSTR human path kind labels"},
      {"sys_relpath", "programs/proof/832_sys_relpath.cubalc", "SYS RELPATH base path relative plate paths"},
      {"sys_atime", "programs/proof/833_sys_atime.cubalc", "SYS ATIME/CTIME access and status-change times"},
      {"sys_hasenv", "programs/proof/834_sys_hasenv.cubalc", "SYS HASENV soft env presence probe"},
      {"sys_gid", "programs/proof/835_sys_gid.cubalc", "SYS GID/EGID/GROUP process group identity"},
      {"sys_groups", "programs/proof/836_sys_groups.cubalc", "SYS GROUPS/INGROUP process group membership"},
      {"sys_ppid", "programs/proof/837_sys_ppid.cubalc", "SYS PPID/PGID/SID process lineage and session"},
      {"sys_euid_isroot", "programs/proof/838_sys_euid_isroot.cubalc", "SYS EUID/ISROOT effective uid and root probe"},
      {"sys_shell", "programs/proof/839_sys_shell.cubalc", "SYS SHELL login shell path without shell"},
      {"sys_gecos", "programs/proof/840_sys_gecos.cubalc", "SYS GECOS/FULLNAME passwd display name"},
      {"sys_ttyname", "programs/proof/841_sys_ttyname.cubalc", "SYS TTYNAME/CTTY terminal device path"},
      {"sys_nice", "programs/proof/842_sys_nice.cubalc", "SYS NICE/RENICE process priority get/set"},
      {"sys_shquote", "programs/proof/843_sys_shquote.cubalc", "SYS SHQUOTE/SHELLQUOTE POSIX shell-safe wrap"},
      {"sys_shunquote", "programs/proof/844_sys_shunquote.cubalc", "SYS SHUNQUOTE/UNQUOTE peel shell quote wrap"},
      {"sys_getlogin", "programs/proof/845_sys_getlogin.cubalc", "SYS GETLOGIN/LOGINNAME session login name"},
      {"sys_entropy", "programs/proof/846_sys_entropy.cubalc", "SYS ENTROPY/URANDOM random bytes as hex"},
      {"sys_parsems", "programs/proof/847_sys_parsems.cubalc", "SYS PARSEMS/DURATION compound human duration to ms"},
      {"sys_humanms", "programs/proof/848_sys_humanms.cubalc", "SYS HUMANMS/FMTMS ms to human duration"},
      {"sys_fromtime", "programs/proof/849_sys_fromtime.cubalc", "SYS FROMTIME/EPOCHISO epoch to UTC ISO"},
      {"sys_parseiso", "programs/proof/850_sys_parseiso.cubalc", "SYS PARSEISO/TOEPOCH ISO stamp to epoch"},
      {"sys_timediff", "programs/proof/851_sys_timediff.cubalc", "SYS TIMEDIFF/ELAPSED epoch second deltas"},
      {"sys_envdefault", "programs/proof/852_sys_envdefault.cubalc", "SYS ENVDEFAULT setenv if missing/empty"},
      {"sys_addtime", "programs/proof/853_sys_addtime.cubalc", "SYS ADDTIME/DEADLINE epoch plus seconds"},
      {"sys_expired", "programs/proof/854_sys_expired.cubalc", "SYS EXPIRED/REMAINING lease due and TTL"},
      {"sys_ms2sec", "programs/proof/855_sys_ms2sec.cubalc", "SYS MS2SEC/SEC2MS ms-seconds unit bridge"},
      {"sys_parsems_compound", "programs/proof/856_sys_parsems_compound.cubalc", "SYS PARSEMS compound 1h30m HUMANMS round-trip"},
      {"sys_sleep_duration", "programs/proof/857_sys_sleep_duration.cubalc", "SYS SLEEP human duration string without PARSEMS glue"},
      {"sys_waitfile_duration", "programs/proof/858_sys_waitfile_duration.cubalc", "SYS WAITFILE timeout human duration string"},
      {"sys_deadline_duration", "programs/proof/859_sys_deadline_duration.cubalc", "SYS DEADLINE/ADDTIME human duration lease offsets"},
      {"oop_class", "programs/proof/862_oop_class_method.cubalc", "CLASS/METHOD/NEW/SEND OOP reusable objects"},
      {"cop_entity_tick", "programs/proof/863_cop_entity_tick.cubalc", "ENTITY/SPAWN/TICK COP game-engine plane"},
      {"life_engine", "programs/proof/865_life_engine_division.cubalc", "COP life engine cell division mitosis demo"},
      {"include_class_lib", "programs/proof/864_include_class_lib.cubalc", "INCLUDE retains CLASS/METHOD bodies from programs/lib"},
      {"sys_humansecs", "programs/proof/866_sys_humansecs.cubalc", "SYS HUMANSECS/HUMANAGE seconds to human duration labels"},
      {"oop_list_hasmethod", "programs/proof/867_oop_list_hasmethod.cubalc", "LISTCLASSES/LISTOBJS/HASMETHOD OOP introspection"},
      {"oop_destroy_hasobj", "programs/proof/868_oop_destroy_hasobj.cubalc", "HASOBJ/DESTROY/HASFIELD object lifecycle"},
      {"oop_dumpobj", "programs/proof/869_oop_dumpobj.cubalc", "DUMPOBJ/INSPECT object field snapshot bag/JSON"},
      {"oop_string_fields", "programs/proof/870_oop_string_fields.cubalc", "SETF/NEW string-from-var + string formals"},
      {"oop_getf_or", "programs/proof/871_oop_getf_or.cubalc", "GETF OR/TRYGETF soft field reads with defaults"},
      {"assert_str_eq", "programs/proof/872_assert_str_eq.cubalc", "ASSERT/IF string ==/!= content compare"},
      {"oop_cloneobj", "programs/proof/873_oop_cloneobj.cubalc", "CLONEOBJ/COPYOBJ shallow object field copy"},
      {"oop_renameobj", "programs/proof/874_oop_renameobj.cubalc", "RENAMEOBJ/MOVEOBJ rename live object slot"},
      {"oop_trysetf", "programs/proof/875_oop_trysetf.cubalc", "TRYSETF/SETF SOFT soft field writes"},
      {"oop_trysend", "programs/proof/876_oop_trysend.cubalc", "TRYSEND/SEND SOFT soft method dispatch"},
      {"oop_each_obj", "programs/proof/877_oop_each_obj.cubalc", "EACH OBJ walk live objects with class filter"},
      {"oop_sendall", "programs/proof/878_oop_sendall.cubalc", "SENDALL/BROADCAST method on every live object"},
      {"oop_deleteall", "programs/proof/879_oop_deleteall.cubalc", "DELETEALL/FREEALL bulk free live objects"},
      {"oop_getfall", "programs/proof/880_oop_getfall.cubalc", "GETFALL/COLLECTF bulk field collect from live objects"},
      {"oop_setfall", "programs/proof/881_oop_setfall.cubalc", "SETFALL/MAPSETF bulk field write on live objects"},
      {"oop_whereobj", "programs/proof/882_oop_whereobj.cubalc", "WHEREOBJ/FINDOBJ select live objects by field equality"},
      {"oop_deletewhere", "programs/proof/883_oop_deletewhere.cubalc", "DELETEWHERE/FREEWHERE free live objects by field equality"},
      {"oop_sendwhere", "programs/proof/884_oop_sendwhere.cubalc", "SENDWHERE/INVOKEWHERE method on field-matching objects"},
      {"oop_incfall", "programs/proof/885_oop_incfall.cubalc", "INCFALL/ADDFALL/DECFALL bulk numeric field bump"},
      {"oop_maxobj", "programs/proof/886_oop_maxobj.cubalc", "MAXOBJ/MINOBJ pick object by extreme field"},
      {"oop_sortbyf", "programs/proof/887_oop_sortbyf.cubalc", "SORTBYF/SORTOBJS bag of names ordered by field"},
      {"oop_sumf", "programs/proof/888_oop_sumf.cubalc", "SUMF/AVGF fleet field sum and integer mean"},
      {"oop_wherege", "programs/proof/889_oop_wherege.cubalc", "WHEREGE/WHERELE threshold select live objects"},
      {"oop_deletewherege", "programs/proof/890_oop_deletewherege.cubalc", "DELETEWHEREGE/FREEBELOW threshold free live objects"},
      {"oop_sendwherege", "programs/proof/891_oop_sendwherege.cubalc", "SENDWHEREGE/SENDBELOW method on threshold-matched objects"},
      {"oop_countwhere", "programs/proof/892_oop_countwhere.cubalc", "COUNTOBJ/COUNTWHERE/COUNTWHEREGE fleet tallies without bags"},
      {"oop_haswhere", "programs/proof/893_oop_haswhere.cubalc", "HASWHERE/HASWHEREGE soft existence probes for IF gates"},
      {"oop_wherebetween", "programs/proof/894_oop_wherebetween.cubalc", "WHEREBETWEEN/COUNTBETWEEN/HASBETWEEN closed-range fleet query"},
      {"oop_deletebetween", "programs/proof/895_oop_deletebetween.cubalc", "DELETEBETWEEN/FREEBAND free live objects in closed field range"},
      {"oop_sendbetween", "programs/proof/896_oop_sendbetween.cubalc", "SENDBETWEEN/SENDBAND method on closed-range matched objects"},
      {"oop_clampfall", "programs/proof/897_oop_clampfall.cubalc", "CLAMPFALL/CLAMPF bulk clamp numeric fields to [lo,hi]"},
      {"oop_medianf", "programs/proof/898_oop_medianf.cubalc", "MEDIANF/P50F integer median of fleet numeric field"},
      {"oop_mulfall", "programs/proof/899_oop_mulfall.cubalc", "MULFALL/SCALEFALL bulk multiply numeric fields by factor"},
      {"oop_freqf", "programs/proof/900_oop_freqf.cubalc", "FREQF/HISTF fleet field value histogram key:count"},
      {"oop_modef", "programs/proof/901_oop_modef.cubalc", "MODEF/TOPF most frequent fleet field value"},
      {"oop_uniquf", "programs/proof/902_oop_uniquf.cubalc", "UNIQUF/DISTINCTF unique fleet field values bag"},
      {"oop_copyf", "programs/proof/903_oop_copyf.cubalc", "COPYF/COPYFALL copy field to field across live objects"},
      {"oop_swapf", "programs/proof/904_oop_swapf.cubalc", "SWAPF/SWAPFALL exchange two fields on live objects"},
      {"oop_setfwhere", "programs/proof/905_oop_setfwhere.cubalc", "SETFWHERE conditional field write on matching live objects"},
      {"oop_incfwhere", "programs/proof/906_oop_incfwhere.cubalc", "INCFWHERE/DECFWHERE bump field on matching live objects"},
      {"oop_getfwhere", "programs/proof/907_oop_getfwhere.cubalc", "GETFWHERE filtered field collect from matching live objects"},
      {"oop_sumwhere", "programs/proof/908_oop_sumwhere.cubalc", "SUMWHERE/AVGWHERE filtered fleet sum/mean on match"},
      {"oop_minwhere", "programs/proof/909_oop_minwhere.cubalc", "MINWHERE/MAXWHERE filtered fleet min/max on match"},
      {"oop_medianwhere", "programs/proof/910_oop_medianwhere.cubalc", "MEDIANWHERE/P50WHERE filtered fleet median on match"},
      {"oop_freqwhere", "programs/proof/911_oop_freqwhere.cubalc", "FREQWHERE/HISTWHERE filtered fleet field histogram on match"},
      {"oop_modewhere", "programs/proof/912_oop_modewhere.cubalc", "MODEWHERE/DOMINANTWHERE filtered fleet mode on match"},
      {"oop_uniquwhere", "programs/proof/913_oop_uniquwhere.cubalc", "UNIQUWHERE/DISTINCTWHERE filtered unique field values on match"},
      {"oop_clampwhere", "programs/proof/914_oop_clampwhere.cubalc", "CLAMPWHERE/CLIPWHERE selective clamp field on match"},
      {"oop_mulwhere", "programs/proof/915_oop_mulwhere.cubalc", "MULWHERE/SCALEWHERE selective multiply field on match"},
      {"oop_copyfwhere", "programs/proof/916_oop_copyfwhere.cubalc", "COPYFWHERE/SNAPSHOTWHERE selective field copy on match"},
      {"oop_swapfwhere", "programs/proof/917_oop_swapfwhere.cubalc", "SWAPFWHERE/EXCHANGEWHERE selective field swap on match"},
      {"oop_maxobjwhere", "programs/proof/918_oop_maxobjwhere.cubalc", "MAXOBJWHERE/MINOBJWHERE pick extreme obj among matches"},
      {"oop_sortbyfwhere", "programs/proof/919_oop_sortbyfwhere.cubalc", "SORTBYFWHERE/ORDERWHERE rank matching objects by field"},
      {"oop_topnbyf", "programs/proof/920_oop_topnbyf.cubalc", "TOPNBYF/HEADBYF top-n object names by numeric field"},
      {"oop_topnbyfwhere", "programs/proof/921_oop_topnbyfwhere.cubalc", "TOPNBYFWHERE/HEADWHERE top-n matching objects by field"},
      {"oop_nthbyf", "programs/proof/922_oop_nthbyf.cubalc", "NTHBYF/ATBYF 0-based rank pick object by numeric field"},
      {"oop_nthbyfwhere", "programs/proof/923_oop_nthbyfwhere.cubalc", "NTHBYFWHERE/ATWHERE kth match by numeric field rank"},
      {"oop_slicebyf", "programs/proof/924_oop_slicebyf.cubalc", "SLICEBYF/MIDBYF ranked object name window by field"},
      {"oop_pickobj", "programs/proof/925_oop_pickobj.cubalc", "PICKOBJ/RANDOBJ random live object sample"},
      {"oop_drawnobj", "programs/proof/926_oop_drawnobj.cubalc", "DRAWNOBJ/SAMPLEKOBJ sample k unique live objects"},
      {"oop_shuffleobjs", "programs/proof/927_oop_shuffleobjs.cubalc", "SHUFFLEOBJS/SHUFOBJS shuffle live object name bag"},
      {"oop_classinfo", "programs/proof/928_oop_classinfo.cubalc", "CLASSINFO/DUMPCLASS class schema plate for agents"},
      {"oop_methodinfo", "programs/proof/929_oop_methodinfo.cubalc", "METHODINFO/DUMPMETHOD method arity/params plate"},
      {"sys_addiso", "programs/proof/860_sys_addiso.cubalc", "SYS ADDISO shift ISO stamp by secs or duration"},
      {"sys_diffiso", "programs/proof/861_sys_diffiso.cubalc", "SYS DIFFISO/ISOAGE ISO stamp second deltas"},
      {"plate_session", "programs/proof/1117_plate_session.cubalc", "INCLUDE plate_session agent_boot+plate_boot"},
      {"plate_save", "programs/proof/1109_plate_save.cubalc", "INCLUDE plate_save PLATE write-back"},
      {"setp", "programs/proof/1110_setp.cubalc", "SETP/INCP/DELP/GETP PLATE mutate without LET"},
      {"setp_from", "programs/proof/1130_setp_from.cubalc", "SETP/GETP/INCP/DELP FROM multi-plate mutate"},
      {"mergep_from", "programs/proof/1131_mergep_from.cubalc", "MERGEP/DEFAULTP/TOGGLEP FROM multi-plate"},
      {"needp_from", "programs/proof/1132_needp_from.cubalc", "NEEDP/HASP/KEYSP FROM multi-plate probes"},
      {"dumpp_from", "programs/proof/1133_dumpp_from.cubalc", "DUMPP FROM multi-plate plate_info snapshot"},
      {"loadplate_into", "programs/proof/1134_loadplate_into.cubalc", "LOADPLATE/ENSUREPLATE INTO multi-plate disk bind"},
      {"savep", "programs/proof/1135_savep.cubalc", "SAVEP/SAVEPLATE FROM multi-plate disk persist"},
      {"loadp", "programs/proof/1136_loadp.cubalc", "LOADP/ENSUREP INTO multi-plate no-SYS disk boot"},
      {"plate_peer", "programs/proof/1137_plate_peer.cubalc", "INCLUDE plate_peer multi-plate PEER boot+save"},
      {"init_peer", "programs/proof/1138_init_peer.sh", "cubalc init --peer multi-plate scaffold"},
      {"plate_peer_tick", "programs/proof/1139_plate_peer_tick.cubalc", "INCLUDE plate_peer_tick multi-plate PEER tick"},
      {"plate_both_save", "programs/proof/1143_plate_both_save.cubalc", "INCLUDE plate_both_save PLATE+PEER one-shot write-back"},
      {"eqp", "programs/proof/1145_eqp.cubalc", "EQP/NEQP/DIFFP/CHANGELOGP multi-plate compare no SYS"},
      {"require_eqp", "programs/proof/1146_require_eqp.cubalc", "REQUIRE EQP/NEQP fail-fast multi-plate · soft twin EQP"},
      {"subsetp", "programs/proof/1147_subsetp.cubalc", "SUBSETP/COVERSP multi-plate required-field match"},
      {"pluckp", "programs/proof/1148_pluckp.cubalc", "PLUCKP/GETPALL multi-key peel value bag multi-plate"},
      {"deltap", "programs/proof/1149_deltap.cubalc", "DELTAP changed-key plate payload multi-plate sync"},
      {"stablep", "programs/proof/1150_stablep.cubalc", "STABLEP unchanged key bag multi-plate dual DIFFP"},
      {"pickp", "programs/proof/1151_pickp.cubalc", "PICKP/OMITP multi-plate keep/drop key projection"},
      {"renamep", "programs/proof/1152_renamep.cubalc", "RENAMEP multi-plate key rename write-back"},
      {"keydiffp", "programs/proof/1153_keydiffp.cubalc", "KEYDIFFP/KEYCOMMP multi-plate key-set bags"},
      {"copyp", "programs/proof/1154_copyp.cubalc", "COPYP/SWAPP multi-plate key copy/swap write-back"},
      {"lenp", "programs/proof/1155_lenp.cubalc", "LENP/EMPTYP/VALSP multi-plate size empty values probes"},
      {"typep", "programs/proof/1156_typep.cubalc", "TYPEP multi-plate field kind probe no SYS JSONTYPE"},
      {"tokvp", "programs/proof/1157_tokvp.cubalc", "TOKVP/FROMKVP multi-plate bag↔plate convert"},
      {"cli_plate_pick", "programs/proof/1158_cli_plate_pick.sh", "cubalc plate pick/omit key projection duals"},
      {"sumnp", "programs/proof/1159_sumnp.cubalc", "SUMNP/TOPKEYP/MAXNP multi-plate numeric aggregates"},
      {"threshp", "programs/proof/1160_threshp.cubalc", "THRESHP/DROPZEROP/CAPP multi-plate value denoise"},
      {"pctp", "programs/proof/1161_pctp.cubalc", "PCTP/SCALEP/ADDP multi-plate value share weight offset"},
      {"summergep", "programs/proof/1162_summergep.cubalc", "SUMMERGEP/SUBP/DIVP multi-plate FREQ combine delta mean"},
      {"absp", "programs/proof/1163_absp.cubalc", "ABSP/SIGNP multi-plate magnitude and polarity after SUBP"},
      {"keepkeyp", "programs/proof/1164_keepkeyp.cubalc", "KEEPKEYP/DROPKEYP multi-plate key needle filter"},
      {"medianp", "programs/proof/1165_medianp.cubalc", "MEDIANP multi-plate integer median robust mid vs AVGNP"},
      {"topnp", "programs/proof/1166_topnp.cubalc", "TOPNP/BOTNP multi-plate top/bottom N keys by value"},
      {"sortp", "programs/proof/1167_sortp.cubalc", "SORTP/SORTBAGP multi-plate sort keys by value"},
      {"cli_plate_agg", "programs/proof/1168_cli_plate_agg.sh", "cubalc plate sum/avg/median/top/bot aggregate duals"},
      {"each_key", "programs/proof/1169_each_key.cubalc", "EACH KEY OF plate walk keys+VALUE without KEYSP+EACH LINE"},
      {"each_keynest", "programs/proof/1175_each_keynest.cubalc", "EACH KEYNEST / KEY OF NEST walk nested keys+VALUE"},
      {"each_key_path", "programs/proof/1203_each_key_path.cubalc", "EACH KEYNEST/OF plate dotted nest path walk"},
      {"pluckobj", "programs/proof/1176_pluckobj.cubalc", "PLUCKOBJ multi-key peel nested value bag multi-plate"},
      {"pickobj", "programs/proof/1177_pickobj.cubalc", "PICKOBJ/OMITOBJ nest key keep/drop write-back multi-plate"},
      {"lenobj", "programs/proof/1179_lenobj.cubalc", "LENOBJ/EMPTYOBJ/VALSOBJ nested size empty values probes"},
      {"renamepobj", "programs/proof/1180_renamepobj.cubalc", "RENAMEPOBJ nest key rename write-back multi-plate RENAMEP dual"},
      {"copypobj", "programs/proof/1181_copypobj.cubalc", "COPYPOBJ/SWAPPOBJ nest key copy/swap write-back multi-plate COPYP/SWAPP duals"},
      {"tokvobj", "programs/proof/1182_tokvobj.cubalc", "TOKVOBJ/FROMKVOBJ nest bag bridge multi-plate TOKVP/FROMKVP duals"},
      {"sumnobj", "programs/proof/1183_sumnobj.cubalc", "SUMNOBJ/TOPKEYOBJ nest numeric aggregates multi-plate SUMNP duals"},
      {"threshobj", "programs/proof/1184_threshobj.cubalc", "THRESHOBJ/DROPZEROOBJ/CAPOBJ nest value denoise write-back multi-plate"},
      {"pctobj", "programs/proof/1185_pctobj.cubalc", "PCTOBJ/SCALEPOBJ/ADDPOBJ nest value map write-back multi-plate PCTP duals"},
      {"keepkeyobj", "programs/proof/1186_keepkeyobj.cubalc", "KEEPKEYOBJ/DROPKEYOBJ nest key needle filter write-back multi-plate"},
      {"topnobj", "programs/proof/1187_topnobj.cubalc", "TOPNOBJ/BOTNOBJ nest top/bottom N keys write-back multi-plate TOPNP duals"},
      {"cli_plate_nestagg", "programs/proof/1188_cli_plate_nestagg.sh", "cubalc plate nestsum/nestavg/nesttop/nestbot SUMNOBJ duals"},
      {"sortobj", "programs/proof/1189_sortobj.cubalc", "SORTOBJ/SORTBAGOBJ nest sort pure-int keys multi-plate SORTP duals"},
      {"cli_plate_nestsort", "programs/proof/1190_cli_plate_nestsort.sh", "cubalc plate nestsort/nestsortbag SORTOBJ duals"},
      {"summergeobj", "programs/proof/1191_summergeobj.cubalc", "SUMMERGEOBJ/SUBVALOBJ nest FREQ sum-merge delta multi-plate"},
      {"pathp", "programs/proof/1192_pathp.cubalc", "GETP/SETP/INCP/DELP/HASP dotted path nest sugar multi-plate"},
      {"pathp_more", "programs/proof/1193_pathp_more.cubalc", "TYPEP/DEFAULTP/TOGGLEP path + cubalc plate get/set/inc path"},
      {"cli_plate_path", "programs/proof/1193b_cli_plate_path.sh", "cubalc plate get/set/inc/del dotted path duals"},
      {"needp_path", "programs/proof/1194_needp_path.cubalc", "NEEDP/HASPALL dotted path nest contracts multi-plate"},
      {"cli_plate_need_path", "programs/proof/1194b_cli_plate_need_path.sh", "cubalc plate has/need dotted path contracts"},
      {"pluckp_path", "programs/proof/1195_pluckp_path.cubalc", "PLUCKP dotted path multi-key peel nest fields multi-plate"},
      {"fillp_path", "programs/proof/1196_fillp_path.cubalc", "FILLP {{freq.error}} dotted path nest templates multi-plate"},
      {"cli_plate_pluck", "programs/proof/1197_cli_plate_pluck.sh", "cubalc plate pluck multi-key path peel PLUCKP dual"},
      {"cli_plate_type", "programs/proof/1198_cli_plate_type.sh", "cubalc plate type TYPEP dual kind probe paths"},
      {"cli_plate_default_toggle", "programs/proof/1199_cli_plate_default_toggle.sh", "cubalc plate default/toggle DEFAULTP/TOGGLEP duals paths"},
      {"path_rename_copy_swap", "programs/proof/1200_path_rename_copy_swap.cubalc", "RENAMEP/COPYP/SWAPP dotted paths"},
      {"cli_plate_rename_copy_swap", "programs/proof/1200_cli_plate_rename_copy_swap.sh", "cubalc plate rename/copy/swap duals paths"},
      {"path_keysp_lenp", "programs/proof/1201_path_keysp_lenp.cubalc", "KEYSP/LENP/EMPTYP/VALSP nest path"},
      {"cli_plate_keys_len", "programs/proof/1201_cli_plate_keys_len.sh", "cubalc plate keys/len/empty/vals nest path"},
      {"pathkeys", "programs/proof/1210_pathkeys.cubalc", "PATHKEYS/LEAFKEYS dotted leaf path bag multi-plate"},
      {"cli_plate_leaves", "programs/proof/1210_cli_plate_leaves.sh", "cubalc plate leaves/pathkeys PATHKEYS dual"},
      {"flatkv", "programs/proof/1211_flatkv.cubalc", "FLATKV/LEAFKV recursive path:value bag multi-plate"},
      {"cli_plate_flat", "programs/proof/1211_cli_plate_flat.sh", "cubalc plate flat/flatkv FLATKV dual"},
      {"unflatkv", "programs/proof/1212_unflatkv.cubalc", "UNFLATKV/FROMFLAT path:value bag to nested plate"},
      {"cli_plate_unflat", "programs/proof/1212_cli_plate_unflat.sh", "cubalc plate unflat UNFLATKV dual"},
      {"diffflat", "programs/proof/1213_diffflat.cubalc", "DIFFFLAT/PATHDIFF deep leaf plate changelog multi-plate"},
      {"cli_plate_diffflat", "programs/proof/1213_cli_plate_diffflat.sh", "cubalc plate diffflat/pathdiff DIFFFLAT dual"},
      {"grepflat", "programs/proof/1214_grepflat.cubalc", "GREPFLAT/GREPVFLAT/GREPFLATI path needle filter multi-plate"},
      {"cli_plate_grepf", "programs/proof/1214_cli_plate_grepf.sh", "cubalc plate grepf/grepvf GREPFLAT dual"},
      {"pruneflat", "programs/proof/1215_pruneflat.cubalc", "PRUNEFLAT/KEEPONLYFLAT leaf path scrub project write-back"},
      {"cli_plate_prune", "programs/proof/1215_cli_plate_prune.sh", "cubalc plate prune/keeponly PRUNEFLAT dual"},
      {"mergeflat", "programs/proof/1216_mergeflat.cubalc", "MERGEFLAT deep leaf overlay multi-plate nest-aware MERGEP"},
      {"cli_plate_mergeflat", "programs/proof/1216_cli_plate_mergeflat.sh", "cubalc plate mergeflat MERGEFLAT dual"},
      {"renameflat", "programs/proof/1217_renameflat.cubalc", "RENAMEFLAT leaf path prefix rewrite multi-plate"},
      {"cli_plate_renameflat", "programs/proof/1217_cli_plate_renameflat.sh", "cubalc plate renameflat RENAMEFLAT dual"},
      {"setflat", "programs/proof/1218_setflat.cubalc", "SETFLAT/MAPFLAT bulk leaf value set multi-plate"},
      {"cli_plate_setflat", "programs/proof/1218_cli_plate_setflat.sh", "cubalc plate setflat SETFLAT dual"},
      {"incflat", "programs/proof/1219_incflat.cubalc", "INCFLAT/BUMPFLAT bulk pure-int leaf bump multi-plate"},
      {"cli_plate_incflat", "programs/proof/1219_cli_plate_incflat.sh", "cubalc plate incflat INCFLAT dual"},
      {"sumflat", "programs/proof/1220_sumflat.cubalc", "SUMFLAT/TOTALFLAT sum pure-int leaves by path needle multi-plate"},
      {"cli_plate_sumflat", "programs/proof/1220_cli_plate_sumflat.sh", "cubalc plate sumflat SUMFLAT dual"},
      {"toppathflat", "programs/proof/1221_toppathflat.cubalc", "TOPPATHFLAT/BOTPATHFLAT extreme pure-int leaf path multi-plate"},
      {"cli_plate_toppathflat", "programs/proof/1221_cli_plate_toppathflat.sh", "cubalc plate toppath|botpath TOPPATHFLAT dual"},
      {"threshflat", "programs/proof/1222_threshflat.cubalc", "THRESHFLAT/DROPZEROFLAT pure-int leaf value denoise multi-plate"},
      {"cli_plate_threshflat", "programs/proof/1222_cli_plate_threshflat.sh", "cubalc plate threshflat|dropzeroflat dual"},
      {"capflat", "programs/proof/1223_capflat.cubalc", "CAPFLAT/CLAMPFLAT clamp pure-int leaves by path needle multi-plate"},
      {"cli_plate_capflat", "programs/proof/1223_cli_plate_capflat.sh", "cubalc plate capflat CAPFLAT dual"},
      {"scaleflat", "programs/proof/1224_scaleflat.cubalc", "SCALEFLAT/MULFLAT multiply pure-int leaves by factor multi-plate"},
      {"cli_plate_scaleflat", "programs/proof/1224_cli_plate_scaleflat.sh", "cubalc plate scaleflat SCALEFLAT dual"},
      {"hasflat", "programs/proof/1225_hasflat.cubalc", "HASFLAT/COUNTFLAT leaf path presence and count multi-plate"},
      {"cli_plate_hasflat", "programs/proof/1225_cli_plate_hasflat.sh", "cubalc plate hasflat|countflat dual"},
      {"pathsflat", "programs/proof/1226_pathsflat.cubalc", "PATHSFLAT/VALSFLAT matching leaf path/value bags multi-plate"},
      {"cli_plate_pathsflat", "programs/proof/1226_cli_plate_pathsflat.sh", "cubalc plate pathsflat|valsflat dual"},
      {"getflat", "programs/proof/1227_getflat.cubalc", "GETFLAT first matching leaf value OR fallback multi-plate"},
      {"cli_plate_getflat", "programs/proof/1227_cli_plate_getflat.sh", "cubalc plate getflat GETFLAT dual"},
      {"needflat", "programs/proof/1228_needflat.cubalc", "NEEDFLAT fail-fast leaf path needle contract multi-plate"},
      {"cli_plate_needflat", "programs/proof/1228_cli_plate_needflat.sh", "cubalc plate needflat NEEDFLAT dual"},
      {"getflatn", "programs/proof/1229_getflatn.cubalc", "GETFLATN first pure-int leaf by needle OR fallback multi-plate"},
      {"cli_plate_getflatn", "programs/proof/1229_cli_plate_getflatn.sh", "cubalc plate getflatn GETFLATN dual"},
      {"avgflat", "programs/proof/1230_avgflat.cubalc", "AVGFLAT/MEANFLAT integer mean pure-int leaves by path needle multi-plate"},
      {"cli_plate_avgflat", "programs/proof/1230_cli_plate_avgflat.sh", "cubalc plate avgflat AVGFLAT dual"},
      {"medianflat", "programs/proof/1231_medianflat.cubalc", "MEDIANFLAT/P50FLAT integer median pure-int leaves by path needle multi-plate"},
      {"cli_plate_medianflat", "programs/proof/1231_cli_plate_medianflat.sh", "cubalc plate medianflat MEDIANFLAT dual"},
      {"needflatn", "programs/proof/1232_needflatn.cubalc", "NEEDFLATN fail-fast pure-int leaf path contract + peel multi-plate"},
      {"cli_plate_needflatn", "programs/proof/1232_cli_plate_needflatn.sh", "cubalc plate needflatn NEEDFLATN dual"},
      {"hasflatn", "programs/proof/1233_hasflatn.cubalc", "HASFLATN/COUNTFLATN pure-int leaf presence and count multi-plate"},
      {"cli_plate_hasflatn", "programs/proof/1233_cli_plate_hasflatn.sh", "cubalc plate hasflatn|countflatn dual"},
      {"pathsflatn", "programs/proof/1234_pathsflatn.cubalc", "PATHSFLATN/VALSFLATN pure-int leaf path/value bags multi-plate"},
      {"cli_plate_pathsflatn", "programs/proof/1234_cli_plate_pathsflatn.sh", "cubalc plate pathsflatn|valsflatn dual"},
      {"lastflatn", "programs/proof/1235_lastflatn.cubalc", "LASTFLATN last pure-int leaf by needle OR fallback multi-plate"},
      {"cli_plate_lastflatn", "programs/proof/1235_cli_plate_lastflatn.sh", "cubalc plate lastflatn LASTFLATN dual"},
      {"typeflat", "programs/proof/1236_typeflat.cubalc", "TYPEFLAT first matching leaf kind by path needle multi-plate"},
      {"cli_plate_typeflat", "programs/proof/1236_cli_plate_typeflat.sh", "cubalc plate typeflat TYPEFLAT dual"},
      {"lastflat", "programs/proof/1237_lastflat.cubalc", "LASTFLAT last matching leaf value OR fallback multi-plate"},
      {"cli_plate_lastflat", "programs/proof/1237_cli_plate_lastflat.sh", "cubalc plate lastflat LASTFLAT dual"},
      {"nthflatn", "programs/proof/1238_nthflatn.cubalc", "NTHFLATN 0-based Nth pure-int leaf by needle OR fallback multi-plate"},
      {"cli_plate_nthflatn", "programs/proof/1238_cli_plate_nthflatn.sh", "cubalc plate nthflatn NTHFLATN dual"},
      {"nthflat", "programs/proof/1239_nthflat.cubalc", "NTHFLAT 0-based Nth matching leaf OR fallback multi-plate"},
      {"cli_plate_nthflat", "programs/proof/1239_cli_plate_nthflat.sh", "cubalc plate nthflat NTHFLAT dual"},
      {"each_flat", "programs/proof/1240_each_flat.cubalc", "EACH FLAT/LEAF walk matching nest leaves PATH+VALUE without PATHSFLAT+EACH LINE+GETP"},
      {"freqflat", "programs/proof/1241_freqflat.cubalc", "FREQFLAT/HISTFLAT value frequency of matching leaves multi-plate"},
      {"cli_plate_freqflat", "programs/proof/1241_cli_plate_freqflat.sh", "cubalc plate freqflat FREQFLAT dual"},
      {"modeflat", "programs/proof/1242_modeflat.cubalc", "MODEFLAT/TOPVALFLAT dominant matching leaf value multi-plate"},
      {"cli_plate_modeflat", "programs/proof/1242_cli_plate_modeflat.sh", "cubalc plate modeflat MODEFLAT dual"},
      {"pathbyval", "programs/proof/1243_pathbyval.cubalc", "PATHBYVAL/VALPATH first leaf path by exact value multi-plate"},
      {"cli_plate_pathbyval", "programs/proof/1243_cli_plate_pathbyval.sh", "cubalc plate pathbyval PATHBYVAL dual"},
      {"uniqflat", "programs/proof/1244_uniqflat.cubalc", "UNIQFLAT/DISTINCTFLAT unique matching leaf values multi-plate"},
      {"cli_plate_uniqflat", "programs/proof/1244_cli_plate_uniqflat.sh", "cubalc plate uniqflat UNIQFLAT dual"},
      {"pathsbyval", "programs/proof/1245_pathsbyval.cubalc", "PATHSBYVAL/ALLPATHSBYVAL all leaf paths by exact value multi-plate"},
      {"cli_plate_pathsbyval", "programs/proof/1245_cli_plate_pathsbyval.sh", "cubalc plate pathsbyval PATHSBYVAL dual"},
      {"countbyval", "programs/proof/1246_countbyval.cubalc", "COUNTBYVAL/HASVAL count/presence by exact leaf value multi-plate"},
      {"cli_plate_countbyval", "programs/proof/1246_cli_plate_countbyval.sh", "cubalc plate countbyval|hasval dual"},
      {"setbyval", "programs/proof/1247_setbyval.cubalc", "SETBYVAL/REPLACEVAL rewrite leaf values by exact match multi-plate"},
      {"cli_plate_setbyval", "programs/proof/1247_cli_plate_setbyval.sh", "cubalc plate setbyval SETBYVAL dual"},
      {"delbyval", "programs/proof/1248_delbyval.cubalc", "DELBYVAL/DROPVAL drop leaves by exact value multi-plate"},
      {"cli_plate_delbyval", "programs/proof/1248_cli_plate_delbyval.sh", "cubalc plate delbyval DELBYVAL dual"},
      {"alleqflat", "programs/proof/1249_alleqflat.cubalc", "ALLEQFLAT/SAMEVALFLAT all matching leaf values identical multi-plate"},
      {"cli_plate_alleqflat", "programs/proof/1249_cli_plate_alleqflat.sh", "cubalc plate alleqflat ALLEQFLAT dual"},
      {"firstuneqflat", "programs/proof/1250_firstuneqflat.cubalc", "FIRSTUNEQFLAT/PATHUNEQ first diverge leaf path multi-plate"},
      {"cli_plate_firstuneq", "programs/proof/1250_cli_plate_firstuneq.sh", "cubalc plate firstuneq FIRSTUNEQFLAT dual"},
      {"uneqpaths", "programs/proof/1251_uneqpaths.cubalc", "UNEQPATHS/DIVERGEPATHS all diverge leaf paths multi-plate"},
      {"cli_plate_uneqpaths", "programs/proof/1251_cli_plate_uneqpaths.sh", "cubalc plate uneqpaths UNEQPATHS dual"},
      {"uniformflat", "programs/proof/1252_uniformflat.cubalc", "UNIFORMFLAT/CHECKFLAT one-shot nest consistency multi-plate"},
      {"cli_plate_uniform", "programs/proof/1252_cli_plate_uniform.sh", "cubalc plate uniform UNIFORMFLAT dual"},
      {"plate_uniform", "programs/proof/1253_plate_uniform.cubalc", "INCLUDE plate_uniform nest consistency DEFAULT needle"},
      {"cli_doctor_libs", "programs/proof/1254_cli_doctor_libs.sh", "cubalc doctor lib/nest-check readiness plate"},
      {"prettyp", "programs/proof/1255_prettyp.cubalc", "PRETTYP/JSONPRETTY indented plate multi-plate"},
      {"cli_plate_pretty", "programs/proof/1255_cli_plate_pretty.sh", "cubalc plate pretty PRETTYP dual"},
      {"cli_init_list", "programs/proof/1256_cli_init_list.sh", "cubalc init --list scaffold catalog + plate uniform starter"},
      {"why", "programs/proof/1257_why.cubalc", "WHY/EXPLAIN recovery plate from LAST_ERR + run why_hint"},
      {"cli_why_hint", "programs/proof/1257_cli_why_hint.sh", "run plate why_hint agent recovery tip"},
      {"cli_protect_status", "programs/proof/1258_cli_protect_status.sh", "cubalc protect status agent readiness plate"},
      {"include_path_soft", "programs/proof/1259_include_path_soft.cubalc", "CUBALC_INCLUDE_PATH + INCLUDE SOFT MISS/SUGGEST"},
      {"cli_include_path", "programs/proof/1259_cli_include_path.sh", "CUBALC_INCLUDE_PATH private lib resolve"},
      {"cli_run_preload", "programs/proof/1260_cli_run_preload.sh", "run -I/CUBALC_PRELOAD + -L include-path preload"},
      {"cli_run_dotenv", "programs/proof/1374_cli_run_dotenv.sh", "run -F/--dotenv/CUBALC_DOTENV KEY=VAL before body"},
      {"cli_run_cwd", "programs/proof/1375_cli_run_cwd.sh", "run --cwd/CUBALC_CWD chdir before body"},
      {"cli_run_setenv", "programs/proof/1377_cli_run_setenv.sh", "run -E/--setenv KEY=VAL inject after dotenv"},
      {"cli_run_unsetenv", "programs/proof/1378_cli_run_unsetenv.sh", "run -U/--unsetenv KEY clear before -E"},
      {"cli_run_mkdir", "programs/proof/1379_cli_run_mkdir.sh", "run -M/--mkdir DIR mkdir -p before cwd"},
      {"listincludes", "programs/proof/1261_listincludes.cubalc", "LISTINCLUDES/HASINCLUDE loaded module audit"},
      {"cli_listincludes", "programs/proof/1261_cli_listincludes.sh", "LISTINCLUDES after run -I preload"},
      {"cli_run_includes", "programs/proof/1262_cli_run_includes.sh", "run plate includes_n/includes LISTINCLUDES dual"},
      {"needinclude", "programs/proof/1263_needinclude.cubalc", "NEEDINCLUDE/HASINCLUDEALL loaded module gates"},
      {"cli_needinclude", "programs/proof/1263_cli_needinclude.sh", "NEEDINCLUDE after run -I preload"},
      {"cli_which_include_path", "programs/proof/1264_cli_which_include_path.sh", "which/SYS WHICH CUBALC_INCLUDE_PATH project libs"},
      {"cli_cat_libs_include_path", "programs/proof/1265_cli_cat_libs_include_path.sh", "cat/libs CUBALC_INCLUDE_PATH project lib discovery"},
      {"cli_require_version", "programs/proof/1266_cli_require_version.sh", "run -R / CUBALC_REQUIRE_VERSION host version floor"},
      {"cli_require_forms", "programs/proof/1326_cli_require_forms.sh", "run -C / CUBALC_REQUIRE_FORMS host form capability floor"},
      {"cli_require_fns", "programs/proof/1440_cli_require_fns.sh", "run -N / CUBALC_REQUIRE_FNS host FN floor after preload"},
      {"cli_require_classes", "programs/proof/1445_cli_require_classes.sh", "run -K / CUBALC_REQUIRE_CLASSES host CLASS floor after preload"},
      {"listforms", "programs/proof/1327_listforms.cubalc", "LISTFORMS/COUNTFORMS in-lang dual of cubalc forms"},
      {"cli_listforms", "programs/proof/1327_cli_listforms.sh", "LISTFORMS bag + COUNTFORMS + forms CLI dual"},
      {"formhint", "programs/proof/1328_formhint.cubalc", "FORMHINT/DESCRIBEFORM HELP one-line peel"},
      {"cli_formhint", "programs/proof/1328_cli_formhint.sh", "cubalc formhint CLI dual + forms"},
      {"tips", "programs/proof/1329_tips.cubalc", "TIPS topic bag dual of cubalc tips"},
      {"cli_tips", "programs/proof/1329_cli_tips.sh", "cubalc tips plate + topics + forms"},
      {"formsfor", "programs/proof/1330_formsfor.cubalc", "FORMSFOR/TOPICFORMS topic form-name bags"},
      {"cli_formsfor", "programs/proof/1330_cli_formsfor.sh", "cubalc formsfor plate + topics + forms"},
      {"related", "programs/proof/1331_related.cubalc", "RELATED/SEEALSO form neighbor bag"},
      {"cli_related", "programs/proof/1331_cli_related.sh", "cubalc related plate + forms"},
      {"snip", "programs/proof/1332_snip.cubalc", "SNIP/SNIPPET topic mini runnable source"},
      {"cli_snip", "programs/proof/1332_cli_snip.sh", "cubalc snip plate + topics + forms"},
      {"topic", "programs/proof/1333_topic.cubalc", "TOPIC/TOPICCARD triad plate tips+forms+snip"},
      {"cli_topic", "programs/proof/1333_cli_topic.sh", "cubalc topic plate + forms"},
      {"runsnip", "programs/proof/1334_runsnip.cubalc", "RUNSNIP/SNIPRUN execute topic mini"},
      {"cli_runsnip", "programs/proof/1334_cli_runsnip.sh", "cubalc runsnip plate + topics"},
      {"listtopics", "programs/proof/1335_listtopics.cubalc", "LISTTOPICS/HASTOPIC/NEEDTOPIC topic catalog"},
      {"cli_listtopics", "programs/proof/1335_cli_listtopics.sh", "cubalc topics plate + hastopic"},
      {"errtips", "programs/proof/1336_errtips.cubalc", "ERRTIPS recovery tip bag from LAST_ERR"},
      {"cli_errtips", "programs/proof/1336_cli_errtips.sh", "cubalc errtips classify + plate"},
      {"errrun", "programs/proof/1337_errrun.cubalc", "ERRRUN classify+RUNSNIP recovery one-shot"},
      {"cli_errrun", "programs/proof/1337_cli_errrun.sh", "cubalc errrun plate + topics"},
      {"errguide", "programs/proof/1343_errguide.cubalc", "ERRGUIDE classify+GUIDE recovery playbook"},
      {"cli_errguide", "programs/proof/1343_cli_errguide.sh", "cubalc errguide plate + forms"},
      {"formguide", "programs/proof/1344_formguide.cubalc", "FORMGUIDE form docs+topics+first playbook"},
      {"cli_formguide", "programs/proof/1344_cli_formguide.sh", "cubalc formguide plate + forms"},
      {"start", "programs/proof/1345_start.cubalc", "START/ONBOARD agent onboarding plate"},
      {"cli_onboard", "programs/proof/1345_cli_onboard.sh", "cubalc onboard plate + forms"},
      {"onboard_boot", "programs/proof/1346_onboard_boot.cubalc", "INCLUDE onboard_boot agent_boot+START"},
      {"cli_init_onboard", "programs/proof/1346_cli_init_onboard.sh", "cubalc init --onboard scaffold + doctor"},
      {"matchtopics", "programs/proof/1347_matchtopics.cubalc", "MATCHTOPICS/PICKTOPIC topic filter duals"},
      {"cli_matchtopics", "programs/proof/1347_cli_matchtopics.sh", "cubalc matchtopics|picktopic plate + forms"},
      {"hasmatchtopics", "programs/proof/1348_hasmatchtopics.cubalc", "HAS/NEED/COUNT MATCHTOPICS filter gates"},
      {"cli_hasmatchtopics", "programs/proof/1348_cli_hasmatchtopics.sh", "cubalc has|need|count matchtopics CLI"},
      {"nthtopic", "programs/proof/1349_nthtopic.cubalc", "NTHTOPIC/LASTTOPIC topic filter index duals"},
      {"cli_nthtopic", "programs/proof/1349_cli_nthtopic.sh", "cubalc nthtopic|lasttopic plate + forms"},
      {"guidematch", "programs/proof/1350_guidematch.cubalc", "GUIDE MATCH|NTH|LASTMATCH filter pick+playbook"},
      {"cli_guidematch", "programs/proof/1350_cli_guidematch.sh", "cubalc guide match|nth|lastmatch + forms"},
      {"runsnipmatch", "programs/proof/1351_runsnipmatch.cubalc", "RUNSNIP MATCH|NTH|LASTMATCH filter pick+execute"},
      {"cli_runsnipmatch", "programs/proof/1351_cli_runsnipmatch.sh", "cubalc runsnip match|nth|lastmatch + forms"},
      {"discover", "programs/proof/1352_discover.cubalc", "DISCOVER match bag + first-topic open plate"},
      {"cli_discover", "programs/proof/1352_cli_discover.sh", "cubalc discover plate + forms"},
      {"discover_boot", "programs/proof/1353_discover_boot.cubalc", "INCLUDE discover_boot agent_boot+DISCOVER"},
      {"cli_init_discover", "programs/proof/1353_cli_init_discover.sh", "cubalc init --discover scaffold + doctor"},
      {"open", "programs/proof/1354_open.cubalc", "OPEN discover bag + GUIDE playbook one-shot"},
      {"cli_open", "programs/proof/1354_cli_open.sh", "cubalc open plate + forms"},
      {"open_boot", "programs/proof/1355_open_boot.cubalc", "INCLUDE open_boot agent_boot+OPEN"},
      {"cli_init_open", "programs/proof/1355_cli_init_open.sh", "cubalc init --open scaffold + doctor"},
      {"doctor_form", "programs/proof/1356_doctor.cubalc", "DOCTOR in-lang install readiness plate"},
      {"cli_doctor_form", "programs/proof/1356_cli_doctor.sh", "DOCTOR form + cubalc doctor dual smoke"},
      {"needdoctor", "programs/proof/1357_needdoctor.cubalc", "NEEDDOCTOR hard install gate twin of DOCTOR"},
      {"cli_needdoctor", "programs/proof/1357_cli_needdoctor.sh", "cubalc needdoctor + run -D preflight"},
      {"doctor_boot", "programs/proof/1358_doctor_boot.cubalc", "INCLUDE doctor_boot agent_boot+NEEDDOCTOR"},
      {"cli_init_doctor", "programs/proof/1358_cli_init_doctor.sh", "cubalc init --doctor scaffold + doctor"},
      {"ready", "programs/proof/1359_ready.cubalc", "READY install prove checklist plate"},
      {"cli_ready", "programs/proof/1359_cli_ready.sh", "cubalc ready plate + NEEDREADY forms"},
      {"cli_require_ready", "programs/proof/1360_cli_require_ready.sh", "run -Y / CUBALC_REQUIRE_READY host prove floor"},
      {"hasflags", "programs/proof/1359_hasflags.cubalc", "HASFLAGS soft flag-count probe"},
      {"cli_hasflags", "programs/proof/1359_cli_hasflags.sh", "HASFLAGS/HASFLAGC forms + -e smoke"},
            {"ready_boot", "programs/proof/1361_ready_boot.cubalc", "INCLUDE ready_boot agent_boot+NEEDREADY"},
      {"needready", "programs/proof/1362_needready.cubalc", "NEEDREADY hard prove gate twin of READY"},
      {"cli_needready", "programs/proof/1362_cli_needready.sh", "cubalc needready + NEEDREADY forms"},
      {"cli_init_ready", "programs/proof/1361_cli_init_ready.sh", "cubalc init --ready scaffold + doctor"},
      {"hasflagall", "programs/proof/1362_hasflagall.cubalc", "HASFLAGALL multi named-flag soft gate"},
      {"cli_hasflagall", "programs/proof/1362_cli_hasflagall.sh", "HASFLAGALL/NEEDFLAGS forms + CLI --args"},
      {"hasflagany", "programs/proof/1376_hasflagany.cubalc", "HASFLAGANY multi named-flag any-of gate"},
      {"cli_hasflagany", "programs/proof/1376_cli_hasflagany.sh", "HASFLAGANY/NEEDFLAGANY forms + CLI"},
      {"hasargall", "programs/proof/1363_hasargall.cubalc", "HASARGALL multi arg soft gate"},
      {"cli_hasargall", "programs/proof/1363_cli_hasargall.sh", "HASARGALL/NEEDARGS forms + CLI args"},
      {"hasargany", "programs/proof/1380_hasargany.cubalc", "HASARGANY multi arg any-of gate"},
      {"cli_hasargany", "programs/proof/1380_cli_hasargany.sh", "HASARGANY/NEEDARGANY forms + CLI"},
      {"hasenvall", "programs/proof/1381_hasenvall.cubalc", "HASENVALL multi host-env ALL gate"},
      {"cli_hasenvall", "programs/proof/1381_cli_hasenvall.sh", "HASENVALL/NEEDENVS forms + CLI"},
      {"cli_envgate", "programs/proof/1381_cli_envgate.sh", "cubalc hasenvall|needenvs CLI duals"},
      {"hasenvany", "programs/proof/1382_hasenvany.cubalc", "HASENVANY multi host-env any-of gate"},
      {"cli_hasenvany", "programs/proof/1382_cli_hasenvany.sh", "HASENVANY/NEEDENVANY forms + CLI"},
      {"env_guard", "programs/proof/1383_env_guard.cubalc", "INCLUDE env_guard NEED_ENVS+NEED_ENV_ANY"},
      {"cli_env_guard", "programs/proof/1383_cli_env_guard.sh", "env_guard soft/hard + doctor lib"},
      {"env_boot", "programs/proof/1384_env_boot.cubalc", "INCLUDE env_boot agent_boot+env_guard"},
      {"cli_init_env", "programs/proof/1384_cli_init_env.sh", "cubalc init --env scaffold + doctor"},
      {"cliinfo", "programs/proof/1364_cliinfo.cubalc", "CLIINFO cubalc.cli.v1 CLI surface plate"},
      {"cli_cliinfo", "programs/proof/1364_cli_cliinfo.sh", "cubalc cliinfo plate + CLIINFO forms"},
      {"cli_boot", "programs/proof/1365_cli_boot.cubalc", "INCLUDE cli_boot agent HELPFLAG+CLIINFO"},
      {"cli_init_cli", "programs/proof/1365_cli_init_cli.sh", "cubalc init --cli scaffold + doctor"},
      {"cli_flaggate", "programs/proof/1366_cli_flaggate.sh", "cubalc hasflagall|needflags CLI duals"},
      {"cli_arggate", "programs/proof/1366_cli_arggate.sh", "cubalc hasargall|needargs CLI duals"},
      {"cli_guard", "programs/proof/1367_cli_guard.cubalc", "INCLUDE cli_guard NEED_FLAGS+NEED_REST_N"},
      {"cli_cli_guard", "programs/proof/1367_cli_cli_guard.sh", "cli_guard soft/hard + doctor lib"},
      {"cli_session", "programs/proof/1368_cli_session.cubalc", "INCLUDE cli_session cli_boot+cli_guard"},
      {"cli_init_cli_session", "programs/proof/1368_cli_init_cli_session.sh", "cubalc init --cli-session scaffold"},
      {"argmap", "programs/proof/1369_argmap.cubalc", "ARGMAP raw argv i=value bag twin FLAGMAP"},
      {"cli_argmap", "programs/proof/1369_cli_argmap.sh", "cubalc argmap plate + ARGMAP forms"},
      {"ntharg", "programs/proof/1370_ntharg.cubalc", "NTHARG raw argv peel with OR fallback"},
      {"cli_ntharg", "programs/proof/1370_cli_ntharg.sh", "NTHARG/GETARG forms + -e smoke"},
      {"findarg", "programs/proof/1371_findarg.cubalc", "FINDARG token→index reverse of NTHARG"},
      {"cli_findarg", "programs/proof/1371_cli_findarg.sh", "cubalc findarg plate + FINDARG forms"},
      {"afterarg", "programs/proof/1372_afterarg.cubalc", "AFTERARG peel argv after exact token"},
      {"cli_afterarg", "programs/proof/1372_cli_afterarg.sh", "cubalc afterarg plate + AFTERARG forms"},
      {"firstarg", "programs/proof/1373_firstarg.cubalc", "FIRSTARG/LASTARG raw argv end peels"},
      {"cli_firstarg", "programs/proof/1373_cli_firstarg.sh", "cubalc firstarg|lastarg + end peels"},
      {"each_topic", "programs/proof/1338_each_topic.cubalc", "EACH TOPIC walk discovery topics"},
      {"cli_each_topic", "programs/proof/1338_cli_each_topic.sh", "EACH TOPIC forms + -e smoke"},
      {"topichint", "programs/proof/1339_topichint.cubalc", "TOPICHINT one-line topic docs"},
      {"cli_topichint", "programs/proof/1339_cli_topichint.sh", "cubalc topichint plate + forms"},
      {"relatedtopic", "programs/proof/1340_relatedtopic.cubalc", "RELATEDTOPIC related discovery topic bag"},
      {"cli_relatedtopic", "programs/proof/1340_cli_relatedtopic.sh", "cubalc relatedtopic plate + forms"},
      {"formtopics", "programs/proof/1341_formtopics.cubalc", "FORMTOPICS reverse FORMSFOR form→topics"},
      {"cli_formtopics", "programs/proof/1341_cli_formtopics.sh", "cubalc formtopics plate + forms"},
      {"guide", "programs/proof/1342_guide.cubalc", "GUIDE full playbook plate hint+related+tips+forms+snip"},
      {"cli_guide", "programs/proof/1342_cli_guide.sh", "cubalc guide plate + forms"},
      {"cli_run_preload_plate", "programs/proof/1267_cli_run_preload_plate.sh", "run plate preload JSON array of -I names"},
      {"includestems", "programs/proof/1268_includestems.cubalc", "INCLUDESTEMS short-name bag from loaded modules"},
      {"cli_includestems", "programs/proof/1268_cli_includestems.sh", "INCLUDESTEMS after run -I preload"},
      {"listpreload", "programs/proof/1269_listpreload.cubalc", "LISTPRELOAD effective -I/PRELOAD bag"},
      {"cli_listpreload", "programs/proof/1269_cli_listpreload.sh", "LISTPRELOAD after run -I · PRELOAD_ACTIVE"},
      {"preloadmiss", "programs/proof/1270_preloadmiss.cubalc", "PRELOADMISS/PRELOADOK -I vs loaded audit"},
      {"cli_preloadmiss", "programs/proof/1270_cli_preloadmiss.sh", "PRELOADMISS/NEEDPRELOAD after run -I"},
      {"cli_preload_ok_plate", "programs/proof/1271_cli_preload_ok_plate.sh", "run plate preload_ok/preload_miss dual of PRELOADOK"},
      {"haspreload", "programs/proof/1272_haspreload.cubalc", "HASPRELOAD soft -I request bag probe"},
      {"cli_haspreload", "programs/proof/1272_cli_haspreload.sh", "HASPRELOAD after run -I"},
      {"cli_include_stems_plate", "programs/proof/1273_cli_include_stems_plate.sh", "run plate include_stems dual of INCLUDESTEMS"},
      {"vars_max", "programs/proof/1274_vars_max.cubalc", "CUBALC_MAX_VARS 256 + VARS/STATUS pressure"},
      {"cli_vars_plate", "programs/proof/1275_cli_vars_plate.sh", "run plate vars_n/max/full dual of STATUS"},
      {"varroom", "programs/proof/1276_varroom.cubalc", "VARROOM/NEEDVARROOM free-slot agent budget"},
      {"cli_varroom", "programs/proof/1276_cli_varroom.sh", "VARROOM after fat LETs + forms catalog"},
      {"hasvarroom", "programs/proof/1277_hasvarroom.cubalc", "HASVARROOM soft free-slot probe"},
      {"cli_hasvarroom", "programs/proof/1277_cli_hasvarroom.sh", "HASVARROOM soft miss + forms"},
      {"cli_doctor_vars_max", "programs/proof/1278_cli_doctor_vars_max.sh", "cubalc doctor vars_max/var budget plate"},
      {"var_guard", "programs/proof/1279_var_guard.cubalc", "INCLUDE var_guard NEED_VARROOM free-slot gate"},
      {"cli_var_guard", "programs/proof/1279_cli_var_guard.sh", "var_guard lib + doctor lib_var_guard"},
      {"run_timeout", "programs/proof/1280_run_timeout.cubalc", "CUBALC_RUN_TIMEOUT wall kill + TIMEOUT_MS specials"},
      {"cli_run_timeout", "programs/proof/1280_cli_run_timeout.sh", "cubalc run -T / timed_out plate dual"},
      {"remain_ms", "programs/proof/1281_remain_ms.cubalc", "REMAIN_MS/HAS_TIME/NEEDTIME wall budget probes"},
      {"cli_remain_ms", "programs/proof/1281_cli_remain_ms.sh", "REMAIN_MS mid-run budget + forms + doctor"},
      {"time_guard", "programs/proof/1282_time_guard.cubalc", "INCLUDE time_guard NEED_TIME wall budget gate"},
      {"cli_time_guard", "programs/proof/1282_cli_time_guard.sh", "time_guard lib + doctor lib_time_guard"},
      {"fat_boot", "programs/proof/1283_fat_boot.cubalc", "INCLUDE fat_boot agent_boot+var_guard one-shot"},
      {"cli_fat_boot", "programs/proof/1283_cli_fat_boot.sh", "fat_boot lib + doctor lib_fat_boot"},
      {"cli_init_fat", "programs/proof/1284_cli_init_fat.sh", "cubalc init --fat fat_boot nest scaffold"},
      {"cli_init_fat_session", "programs/proof/1296_cli_init_fat_session.sh", "cubalc init --fat-session durable nest scaffold"},
      {"cli_remain_ms_plate", "programs/proof/1285_cli_remain_ms_plate.sh", "run plate remain_ms dual of REMAIN_MS + STATUS"},
      {"wall_ms", "programs/proof/1286_wall_ms.cubalc", "WALL_MS/ELAPSED mono elapsed since run start"},
      {"cli_wall_ms_plate", "programs/proof/1286_cli_wall_ms_plate.sh", "run plate wall_ms + WALL_MS form + STATUS"},
      {"fat_session", "programs/proof/1287_fat_session.cubalc", "INCLUDE fat_session fat_boot+plate_boot durable nest"},
      {"cli_fat_session", "programs/proof/1287_cli_fat_session.sh", "fat_session lib + doctor lib_fat_session"},
      {"listlibs", "programs/proof/1288_listlibs.cubalc", "LISTLIBS/HASLIB in-lang dual of cubalc libs"},
      {"cli_listlibs", "programs/proof/1288_cli_listlibs.sh", "LISTLIBS bag + HASLIB soft + forms"},
      {"catlib", "programs/proof/1289_catlib.cubalc", "CATLIB/READLIB soft dump lib source dual of cubalc cat"},
      {"cli_catlib", "programs/proof/1289_cli_catlib.sh", "CATLIB path/bytes + soft miss + forms"},
      {"greplib", "programs/proof/1290_greplib.cubalc", "GREPLIB/SEARCHLIBS lib content discovery dual of shell grep"},
      {"cli_greplib", "programs/proof/1290_cli_greplib.sh", "GREPLIB lines + SEARCHLIBS stems + soft miss + forms"},
      {"headlib", "programs/proof/1291_headlib.cubalc", "HEADLIB/TAILLIB peek lib recipe lines dual of HEADFILE"},
      {"cli_headlib", "programs/proof/1291_cli_headlib.sh", "HEADLIB default/n + TAILLIB soft miss + forms"},
      {"libdeps", "programs/proof/1292_libdeps.cubalc", "LIBDEPS bag of INCLUDE stems from lib recipe"},
      {"cli_libdeps", "programs/proof/1292_cli_libdeps.sh", "LIBDEPS composition + soft miss + forms"},
      {"libinfo", "programs/proof/1293_libinfo.cubalc", "LIBINFO one-shot lib metadata dual of cat --meta"},
      {"cli_libinfo", "programs/proof/1293_cli_libinfo.sh", "LIBINFO plate + soft miss + forms"},
      {"libtree", "programs/proof/1294_libtree.cubalc", "LIBTREE transitive INCLUDE closure of lib recipe"},
      {"cli_libtree", "programs/proof/1294_cli_libtree.sh", "LIBTREE BFS deps + soft miss + forms"},
      {"libdefaults", "programs/proof/1295_libdefaults.cubalc", "LIBDEFAULTS DEFAULT knobs bag before INCLUDE"},
      {"cli_libdefaults", "programs/proof/1295_cli_libdefaults.sh", "LIBDEFAULTS knobs + soft miss + forms"},
      {"cli_recipe", "programs/proof/1297_cli_recipe.sh", "cubalc recipe one plate path+deps+defaults+head"},
      {"cli_examples_fat", "programs/proof/1298_cli_examples_fat.sh", "examples/search fat_session recipe discovery polish"},
      {"cli_help_surface", "programs/proof/1299_cli_help_surface.sh", "cubalc help lists recipe fat-session run -T discovery forms"},
      {"recipe_form", "programs/proof/1300_recipe.cubalc", "RECIPE/LIBCARD in-lang dual of cubalc recipe plate"},
      {"cli_recipe_form", "programs/proof/1300_cli_recipe.sh", "RECIPE plate + soft miss + forms + CLI parity"},
      {"cli_init_from", "programs/proof/1301_cli_init_from.sh", "cubalc init --from lib recipe scaffold knobs+INCLUDE"},
      {"checkdeps", "programs/proof/1302_checkdeps.cubalc", "CHECKDEPS/HASDEPS/NEEDDEPS install gate for lib+tree"},
      {"cli_checkdeps", "programs/proof/1302_cli_checkdeps.sh", "CHECKDEPS plate + soft miss + forms + NEEDDEPS fail"},
      {"cli_checkdeps_cmd", "programs/proof/1303_cli_checkdeps_cmd.sh", "cubalc checkdeps/hasdeps/needdeps CLI dual of CHECKDEPS"},
      {"cli_libs_recipe_meta", "programs/proof/1304_cli_libs_recipe_meta.sh", "cubalc libs stem+deps_n+defaults_n recipe meta plate"},
      {"cli_libs_filter", "programs/proof/1305_cli_libs_filter.sh", "cubalc libs [filter] stem/path/hint substring match"},
      {"matchlibs", "programs/proof/1306_matchlibs.cubalc", "MATCHLIBS/FILTERLIBS in-lang dual of cubalc libs filter"},
      {"cli_matchlibs", "programs/proof/1306_cli_matchlibs.sh", "MATCHLIBS bag + soft empty + forms"},
      {"picklib", "programs/proof/1307_picklib.cubalc", "PICKLIB/FIRSTLIB first match stem OR fallback"},
      {"cli_picklib", "programs/proof/1307_cli_picklib.sh", "PICKLIB plate + OR fallback + forms"},
      {"include_var", "programs/proof/1308_include_var.cubalc", "INCLUDE LAST/str-var after PICKLIB/LET"},
      {"cli_include_var", "programs/proof/1308_cli_include_var.sh", "INCLUDE var/LAST resolution smoke"},
      {"hasmatchlibs", "programs/proof/1309_hasmatchlibs.cubalc", "HASMATCHLIBS/NEEDMATCHLIBS filter presence gates"},
      {"cli_hasmatchlibs", "programs/proof/1309_cli_hasmatchlibs.sh", "HASMATCHLIBS soft + NEEDMATCHLIBS fail + forms"},
      {"nthlib", "programs/proof/1310_nthlib.cubalc", "NTHLIB/LASTLIB index and last match stem for INCLUDE"},
      {"cli_nthlib", "programs/proof/1310_cli_nthlib.sh", "NTHLIB/LASTLIB plate + OR fallback + forms"},
      {"include_match", "programs/proof/1311_include_match.cubalc", "INCLUDE MATCH|FIRST filter pick+include one-shot"},
      {"cli_include_match", "programs/proof/1311_cli_include_match.sh", "INCLUDE MATCH plate + DEFAULT + SOFT + forms"},
      {"include_nth_last", "programs/proof/1312_include_nth_last.cubalc", "INCLUDE NTH/LASTMATCH filter index+last pick+include"},
      {"cli_include_nth_last", "programs/proof/1312_cli_include_nth_last.sh", "INCLUDE NTH/LASTMATCH plate + DEFAULT + forms"},
      {"countmatchlibs", "programs/proof/1313_countmatchlibs.cubalc", "COUNTMATCHLIBS count without bag materialize"},
      {"cli_libmatch", "programs/proof/1313_cli_libmatch.sh", "cubalc picklib/countmatchlibs/hasmatchlibs/needmatchlibs CLI"},
      {"hascountmatchlibs", "programs/proof/1314_hascountmatchlibs.cubalc", "HASCOUNTMATCHLIBS/NEEDCOUNTMATCHLIBS min count gates"},
      {"cli_hascountmatchlibs", "programs/proof/1314_cli_hascountmatchlibs.sh", "min count gates soft/hard + forms"},
      {"each_lib", "programs/proof/1315_each_lib.cubalc", "EACH LIB MATCH walk sorted lib stems without MATCHLIBS+EACH LINE"},
      {"cli_each_lib", "programs/proof/1315_cli_each_lib.sh", "EACH LIB MATCH plate/fat + forms"},
      {"include_all_match", "programs/proof/1316_include_all_match.cubalc", "INCLUDE ALL MATCH bulk include filtered libs"},
      {"cli_include_all_match", "programs/proof/1316_cli_include_all_match.sh", "INCLUDE ALL MATCH guard/fat + DEFAULT + SOFT"},
      {"newestlib", "programs/proof/1317_newestlib.cubalc", "NEWESTLIB/OLDESTLIB mtime pick among filter matches"},
      {"cli_newestlib", "programs/proof/1317_cli_newestlib.sh", "NEWESTLIB/OLDESTLIB plate + OR fallback + forms"},
      {"include_newest", "programs/proof/1318_include_newest.cubalc", "INCLUDE NEWEST/OLDEST mtime pick+include one-shot"},
      {"cli_include_newest", "programs/proof/1318_cli_include_newest.sh", "INCLUDE NEWEST fat + OLDEST + DEFAULT + forms"},
      {"cli_newestlib_cmd", "programs/proof/1319_cli_newestlib_cmd.sh", "cubalc newestlib/oldestlib CLI dual of NEWESTLIB"},
      {"sortlibs", "programs/proof/1320_sortlibs.cubalc", "SORTLIBS mtime-ordered match bag · dual of alpha MATCHLIBS"},
      {"cli_sortlibs", "programs/proof/1320_cli_sortlibs.sh", "cubalc sortlibs CLI dual of SORTLIBS + forms"},
      {"freshlibs", "programs/proof/1321_freshlibs.cubalc", "FRESHLIBS/STALELIBS age-filter match bags"},
      {"cli_freshlibs", "programs/proof/1321_cli_freshlibs.sh", "cubalc freshlibs/stalelibs CLI dual + forms"},
      {"libage", "programs/proof/1322_libage.cubalc", "LIBAGE/HASFRESH/NEEDFRESH single-lib freshness gates"},
      {"cli_libage", "programs/proof/1322_cli_libage.sh", "cubalc libage/hasfresh/needfresh CLI dual + forms"},
      {"hasform", "programs/proof/1323_hasform.cubalc", "HASFORM/NEEDFORM/REQUIRE FORM capability gates"},
      {"cli_hasform", "programs/proof/1323_cli_hasform.sh", "cubalc hasform/needform CLI dual + forms"},
      {"hasforms", "programs/proof/1324_hasforms.cubalc", "HASFORMS/NEEDFORMS multi form gates + form_guard"},
      {"cli_hasforms", "programs/proof/1324_cli_hasforms.sh", "cubalc hasforms/needforms CLI + form_guard lib"},
      {"hasformany", "programs/proof/1385_hasformany.cubalc", "HASFORMANY multi form any-of gate"},
      {"cli_hasformany", "programs/proof/1385_cli_hasformany.sh", "HASFORMANY/NEEDFORMANY forms + CLI"},
      {"form_guard_any", "programs/proof/1386_form_guard.cubalc", "form_guard NEED_FORMS+NEED_FORM_ANY ALL/ANY"},
      {"cli_form_guard_any", "programs/proof/1386_cli_form_guard.sh", "form_guard soft/hard ALL+ANY + doctor"},
      {"arg_guard", "programs/proof/1388_arg_guard.cubalc", "INCLUDE arg_guard NEED_ARGS+NEED_ARG_ANY"},
      {"cli_arg_guard", "programs/proof/1388_cli_arg_guard.sh", "arg_guard soft/hard + doctor lib"},
      {"arg_boot", "programs/proof/1389_arg_boot.cubalc", "INCLUDE arg_boot agent_boot+arg_guard"},
      {"cli_init_arg", "programs/proof/1389_cli_init_arg.sh", "cubalc init --arg scaffold + doctor"},
      {"time_boot", "programs/proof/1390_time_boot.cubalc", "INCLUDE time_boot agent_boot+time_guard"},
      {"cli_init_time", "programs/proof/1390_cli_init_time.sh", "cubalc init --time scaffold + doctor"},
      {"tool_session", "programs/proof/1391_tool_session.cubalc", "INCLUDE tool_session cli+arg guards"},
      {"cli_init_tool_session", "programs/proof/1391_cli_init_tool_session.sh", "cubalc init --full-cli scaffold"},
      {"product_session", "programs/proof/1392_product_session.cubalc", "INCLUDE product_session tool+env+time"},
      {"cli_init_product_session", "programs/proof/1392_cli_init_product_session.sh", "cubalc init --product scaffold"},
      {"time_guard_ok", "programs/proof/1393_time_guard_ok.cubalc", "TIME_GUARD_OK + product_session soft status"},
      {"path_guard", "programs/proof/1394_path_guard.cubalc", "INCLUDE path_guard NEED_PATHS+NEED_PATH_ANY"},
      {"cli_path_guard", "programs/proof/1394_cli_path_guard.sh", "path_guard soft/hard + doctor lib"},
      {"path_boot", "programs/proof/1394_path_boot.cubalc", "INCLUDE path_boot agent_boot+path_guard"},
      {"cli_init_path", "programs/proof/1394_cli_init_path.sh", "cubalc init --path scaffold + doctor"},
      {"rw_guard", "programs/proof/1395_rw_guard.cubalc", "INCLUDE rw_guard NEED_READ/WRITE_PATHS"},
      {"cli_rw_guard", "programs/proof/1395_cli_rw_guard.sh", "rw_guard soft/hard + doctor lib"},
      {"rw_boot", "programs/proof/1395_rw_boot.cubalc", "INCLUDE rw_boot agent_boot+rw_guard"},
      {"cli_init_rw", "programs/proof/1395_cli_init_rw.sh", "cubalc init --rw scaffold + doctor"},
      {"bin_guard", "programs/proof/1396_bin_guard.cubalc", "INCLUDE bin_guard NEED_BINS+NEED_BIN_ANY"},
      {"cli_bin_guard", "programs/proof/1396_cli_bin_guard.sh", "bin_guard soft/hard + doctor lib"},
      {"bin_boot", "programs/proof/1396_bin_boot.cubalc", "INCLUDE bin_boot agent_boot+bin_guard"},
      {"cli_init_bin", "programs/proof/1396_cli_init_bin.sh", "cubalc init --bin scaffold + doctor"},
      {"lib_guard", "programs/proof/1397_lib_guard.cubalc", "INCLUDE lib_guard NEED_LIBS+NEED_LIB_ANY"},
      {"cli_lib_guard", "programs/proof/1397_cli_lib_guard.sh", "lib_guard soft/hard + doctor + REQUIRE LIB var"},
      {"lib_boot", "programs/proof/1397_lib_boot.cubalc", "INCLUDE lib_boot agent_boot+lib_guard"},
      {"cli_init_lib", "programs/proof/1397_cli_init_lib.sh", "cubalc init --lib-boot scaffold + doctor"},
      {"host_session", "programs/proof/1398_host_session.cubalc", "INCLUDE host_session path+rw+bin+lib"},
      {"cli_host_session", "programs/proof/1398_cli_host_session.sh", "host_session soft/hard + doctor"},
      {"host_boot", "programs/proof/1398_host_boot.cubalc", "INCLUDE host_boot agent_boot+host_session"},
      {"cli_init_host", "programs/proof/1398_cli_init_host.sh", "cubalc init --host scaffold + doctor"},
      {"full_session", "programs/proof/1399_full_session.cubalc", "INCLUDE full_session product+host"},
      {"cli_full_session", "programs/proof/1399_cli_full_session.sh", "full_session soft/hard + doctor"},
      {"full_boot", "programs/proof/1399_full_boot.cubalc", "INCLUDE full_boot product+host one-shot"},
      {"cli_init_full", "programs/proof/1399_cli_init_full.sh", "cubalc init --fleet scaffold + doctor"},
      {"plate_guard", "programs/proof/1400_plate_guard.cubalc", "INCLUDE plate_guard NEED_KEYS+NEED_KEY_ANY"},
      {"cli_plate_guard", "programs/proof/1400_cli_plate_guard.sh", "plate_guard soft/hard + doctor"},
      {"key_boot", "programs/proof/1400_key_boot.cubalc", "INCLUDE key_boot plate_boot+plate_guard"},
      {"cli_init_key", "programs/proof/1400_cli_init_key.sh", "cubalc init --key scaffold + doctor"},
      {"fn_guard", "programs/proof/1400_fn_guard.cubalc", "INCLUDE fn_guard NEED_FNALL+NEED_FNANY"},
      {"cli_fn_guard", "programs/proof/1400_cli_fn_guard.sh", "fn_guard soft/hard + doctor"},
      {"fn_boot", "programs/proof/1400_fn_boot.cubalc", "INCLUDE fn_boot agent_boot+fn_guard"},
      {"cli_init_fn", "programs/proof/1400_cli_init_fn.sh", "cubalc init --fn scaffold + doctor"},
      {"class_guard", "programs/proof/1401_class_guard.cubalc", "INCLUDE class_guard NEED_CLASSES"},
      {"cli_class_guard", "programs/proof/1401_cli_class_guard.sh", "class_guard soft/hard + doctor"},
      {"class_boot", "programs/proof/1401_class_boot.cubalc", "INCLUDE class_boot agent_boot+class_guard"},
      {"cli_init_class", "programs/proof/1401_cli_init_class.sh", "cubalc init --class scaffold + doctor"},
      {"method_guard", "programs/proof/1402_method_guard.cubalc", "INCLUDE method_guard NEED_METHODS Class.method"},
      {"cli_method_guard", "programs/proof/1402_cli_method_guard.sh", "method_guard soft/hard + doctor"},
      {"method_boot", "programs/proof/1402_method_boot.cubalc", "INCLUDE method_boot agent_boot+method_guard"},
      {"cli_init_method", "programs/proof/1402_cli_init_method.sh", "cubalc init --method scaffold + doctor"},
      {"field_guard", "programs/proof/1403_field_guard.cubalc", "INCLUDE field_guard NEED_FIELDS Class.field"},
      {"cli_field_guard", "programs/proof/1403_cli_field_guard.sh", "field_guard soft/hard + doctor"},
      {"field_boot", "programs/proof/1403_field_boot.cubalc", "INCLUDE field_boot agent_boot+field_guard"},
      {"cli_init_field", "programs/proof/1403_cli_init_field.sh", "cubalc init --field scaffold + doctor"},
      {"oop_session", "programs/proof/1404_oop_session.cubalc", "INCLUDE oop_session class+method+field"},
      {"cli_oop_session", "programs/proof/1404_cli_oop_session.sh", "oop_session soft/hard + doctor"},
      {"oop_boot", "programs/proof/1404_oop_boot.cubalc", "INCLUDE oop_boot agent_boot+oop_session"},
      {"cli_init_oop", "programs/proof/1404_cli_init_oop.sh", "cubalc init --oop scaffold + doctor"},
      {"obj_guard", "programs/proof/1405_obj_guard.cubalc", "INCLUDE obj_guard NEED_OBJS live instances"},
      {"cli_obj_guard", "programs/proof/1405_cli_obj_guard.sh", "obj_guard soft/hard + doctor"},
      {"obj_boot", "programs/proof/1405_obj_boot.cubalc", "INCLUDE obj_boot agent_boot+obj_guard"},
      {"cli_init_obj", "programs/proof/1405_cli_init_obj.sh", "cubalc init --obj scaffold + doctor"},
      {"live_session", "programs/proof/1406_live_session.cubalc", "INCLUDE live_session oop+obj LIVE_SESSION_OK"},
      {"cli_live_session", "programs/proof/1406_cli_live_session.sh", "live_session soft/hard + doctor"},
      {"live_boot", "programs/proof/1406_live_boot.cubalc", "INCLUDE live_boot agent_boot+live_session"},
      {"cli_init_live", "programs/proof/1406_cli_init_live.sh", "cubalc init --live scaffold + doctor"},
      {"hardfail", "programs/proof/1407_hardfail.cubalc", "HARDFAIL halt exit 1 sticky LAST_ERR"},
      {"cli_hardfail", "programs/proof/1407_cli_hardfail.sh", "HARDFAIL rc + guard hard without -s"},
      {"cli_guards", "programs/proof/1408_cli_guards.sh", "cubalc guards contract catalog JSON"},
      {"service_session", "programs/proof/1409_service_session.cubalc", "INCLUDE service_session full+live"},
      {"cli_service_session", "programs/proof/1409_cli_service_session.sh", "service_session soft/hard + doctor"},
      {"service_boot", "programs/proof/1409_service_boot.cubalc", "INCLUDE service_boot full+live one-shot"},
      {"cli_init_service", "programs/proof/1409_cli_init_service.sh", "cubalc init --service scaffold + doctor"},
      {"trynew", "programs/proof/1410_trynew.cubalc", "TRYNEW/NEW SOFT soft miss class"},
      {"cli_trynew", "programs/proof/1410_cli_trynew.sh", "TRYNEW soft + hard NEW dual"},
      {"ensurenew", "programs/proof/1414_ensurenew.cubalc", "ENSURENEW/NEW OR keep-or-create"},
      {"cli_ensurenew", "programs/proof/1414_cli_ensurenew.sh", "ENSURENEW soft miss + forms dual"},
      {"isclass", "programs/proof/1415_isclass.cubalc", "ISCLASS/ISA soft instance-of probe"},
      {"cli_isclass", "programs/proof/1415_cli_isclass.sh", "ISCLASS + NEEDISA dual"},
      {"ensureclone", "programs/proof/1416_ensureclone.cubalc", "ENSURECLONE/CLONE OR keep-or-clone"},
      {"cli_ensureclone", "programs/proof/1416_cli_ensureclone.sh", "ENSURECLONE soft miss + forms dual"},
      {"bindthis", "programs/proof/1417_bindthis.cubalc", "BINDTHIS/USEOBJ THIS bind without SEND"},
      {"cli_bindthis", "programs/proof/1417_cli_bindthis.sh", "BINDTHIS soft miss + forms dual"},
      {"trydelete", "programs/proof/1418_trydelete.cubalc", "TRYDELETE/DELETEOBJ SOFT free slot"},
      {"cli_trydelete", "programs/proof/1418_cli_trydelete.sh", "TRYDELETE soft miss + forms dual"},
      {"hasclass", "programs/proof/1419_hasclass.cubalc", "HASCLASS soft CLASS defined probe"},
      {"cli_hasclass", "programs/proof/1419_cli_hasclass.sh", "HASCLASS forms + soft dual"},
      {"hasclasses", "programs/proof/1420_hasclasses.cubalc", "HASCLASSES multi soft CLASSMISS bag"},
      {"cli_hasclasses", "programs/proof/1420_cli_hasclasses.sh", "HASCLASSES + NEEDCLASSES dual"},
      {"hasclassany", "programs/proof/1421_hasclassany.cubalc", "HASCLASSANY soft any-of CLASS gate"},
      {"cli_hasclassany", "programs/proof/1421_cli_hasclassany.sh", "HASCLASSANY + NEEDCLASSANY dual"},
      {"hasmethods", "programs/proof/1422_hasmethods.cubalc", "HASMETHODS multi soft METHODMISS bag"},
      {"cli_hasmethods", "programs/proof/1422_cli_hasmethods.sh", "HASMETHODS + NEEDMETHODS dual"},
      {"hasmethodany", "programs/proof/1423_hasmethodany.cubalc", "HASMETHODANY soft any-of method gate"},
      {"cli_hasmethodany", "programs/proof/1423_cli_hasmethodany.sh", "HASMETHODANY + NEEDMETHODANY dual"},
      {"hasfields", "programs/proof/1424_hasfields.cubalc", "HASFIELDS multi soft FIELDMISS bag"},
      {"cli_hasfields", "programs/proof/1424_cli_hasfields.sh", "HASFIELDS + NEEDFIELDS dual"},
      {"hasfieldany", "programs/proof/1425_hasfieldany.cubalc", "HASFIELDANY soft any-of field gate"},
      {"cli_hasfieldany", "programs/proof/1425_cli_hasfieldany.sh", "HASFIELDANY + NEEDFIELDANY dual"},
      {"hasobjs", "programs/proof/1426_hasobjs.cubalc", "HASOBJS multi soft OBJMISS bag"},
      {"cli_hasobjs", "programs/proof/1426_cli_hasobjs.sh", "HASOBJS + NEEDOBJS dual"},
      {"hasobjany", "programs/proof/1427_hasobjany.cubalc", "HASOBJANY soft any-of live-obj gate"},
      {"cli_hasobjany", "programs/proof/1427_cli_hasobjany.sh", "HASOBJANY + NEEDOBJANY dual"},
      {"method_guard_native", "programs/proof/1428_method_guard_native.cubalc", "method_guard via HASCLASS/HASMETHOD"},
      {"cli_method_guard_native", "programs/proof/1428_cli_method_guard_native.sh", "method_guard native + 1402 regression"},
      {"field_guard_native", "programs/proof/1430_field_guard_native.cubalc", "field_guard via HASCLASS/HASFIELD"},
      {"cli_field_guard_native", "programs/proof/1430_cli_field_guard_native.sh", "field_guard native + 1403 regression"},
      {"class_guard_native", "programs/proof/1431_class_guard_native.cubalc", "class_guard via HASCLASS"},
      {"cli_class_guard_native", "programs/proof/1431_cli_class_guard_native.sh", "class_guard native + 1401 regression"},
      {"class_guard_native", "programs/proof/1431_class_guard_native.cubalc", "class_guard via HASCLASS (no LISTCLASSES)"},
      {"cli_class_guard_native", "programs/proof/1431_cli_class_guard_native.sh", "class_guard native + 1401 regression"},
      {"hasfnany", "programs/proof/1432_hasfnany.cubalc", "HASFNANY soft any-of FN gate"},
      {"cli_hasfnany", "programs/proof/1432_cli_hasfnany.sh", "HASFNANY + NEEDFNANY dual"},
      {"fn_guard_native", "programs/proof/1432_fn_guard_native.cubalc", "fn_guard ALL via HASFNS/NEEDFNS"},
      {"cli_fn_guard_native", "programs/proof/1432_cli_fn_guard_native.sh", "fn_guard native ALL + 1400 regression"},
      {"fn_guard_any_native", "programs/proof/1433_fn_guard_any_native.cubalc", "fn_guard ANY via HASFNANY/NEEDFNANY"},
      {"cli_fn_guard_any_native", "programs/proof/1433_cli_fn_guard_any_native.sh", "fn_guard native ANY + hard miss"},
      {"tryclone", "programs/proof/1411_tryclone.cubalc", "TRYCLONE/CLONE SOFT soft redefine/miss"},
      {"cli_tryclone", "programs/proof/1411_cli_tryclone.sh", "TRYCLONE soft + hard CLONE dual"},
      {"tryrename", "programs/proof/1412_tryrename.cubalc", "TRYRENAME/RENAMEOBJ SOFT soft redefine/miss"},
      {"cli_tryrename", "programs/proof/1412_cli_tryrename.sh", "TRYRENAME soft + hard RENAME dual"},
      {"tryswap", "programs/proof/1413_tryswap.cubalc", "TRYSWAP/SWAPOBJ soft role flip names"},
      {"cli_tryswap", "programs/proof/1413_cli_tryswap.sh", "TRYSWAP soft miss + forms dual"},
      {"cli_form_guard", "programs/proof/1386_cli_form_guard.sh", "form_guard lib + CLI + formgate any"},
      {"cap_boot", "programs/proof/1325_cap_boot.cubalc", "INCLUDE cap_boot agent_boot+form_guard"},
      {"cli_init_cap", "programs/proof/1325_cli_init_cap.sh", "cubalc init --cap scaffold + doctor lib_cap_boot"},
      {"getpn_path", "programs/proof/1202_getpn_path.cubalc", "GETPN + path SYS JSONN numeric peel"},
      {"cli_plate_getn", "programs/proof/1202_cli_plate_getn.sh", "cubalc plate getn GETPN dual paths"},
      {"getobj", "programs/proof/1170_getobj.cubalc", "GETOBJ/SETOBJ peel and nest nested plate objects multi-plate"},
      {"mergeobj", "programs/proof/1171_mergeobj.cubalc", "MERGEOBJ/DEFAULTOBJ nested plate merge one-shot multi-plate"},
      {"getpobj", "programs/proof/1172_getpobj.cubalc", "GETPOBJ/SETPOBJ/INCOBJ/DELPOBJ nested scalar field plane multi-plate"},
      {"haspobj", "programs/proof/1173_haspobj.cubalc", "HASPOBJ/TYPEPOBJ/KEYSOBJ/NEEDPOBJ nested probes and contract multi-plate"},
      {"cli_plate_nest", "programs/proof/1174_cli_plate_nest.sh", "cubalc plate nestget/set/inc/del/keys/has nested CLI duals"},
      {"cli_plate_nestpick", "programs/proof/1178_cli_plate_nestpick.sh", "cubalc plate nestpick/nestomit PICKOBJ/OMITOBJ duals"},
      {"pickobj_path", "programs/proof/1204_pickobj_path.cubalc", "PICKOBJ/OMITOBJ dotted nest path write-back"},
      {"cli_plate_nestpick_path", "programs/proof/1204_cli_plate_nestpick_path.sh", "cubalc plate nestpick/omit deep nest path"},
      {"renamepobj_path", "programs/proof/1205_renamepobj_path.cubalc", "RENAMEPOBJ/COPYPOBJ/SWAPPOBJ dotted nest path"},
      {"cli_plate_nestrename", "programs/proof/1205_cli_plate_nestrename.sh", "cubalc plate nestrename/copy/swap duals paths"},
      {"setp_autoraw", "programs/proof/1206_setp_autoraw.cubalc", "SETP/DEFAULTP JSON-shaped string auto-raw object nest"},
      {"getobj_path", "programs/proof/1207_getobj_path.cubalc", "GETOBJ/SETOBJ/MERGEOBJ dotted path peel/nest"},
      {"cli_plate_getobj", "programs/proof/1208_cli_plate_getobj.sh", "cubalc plate getobj/setobj/mergeobj/defaultobj duals paths"},
      {"dumpp", "programs/proof/1116_dumpp.cubalc", "DUMPP cubalc.plate_info.v1 PLATE snapshot"},
      {"fillp", "programs/proof/1120_fillp.cubalc", "FILLP/SUBSTPLATE expand {{key}} from PLATE templates"},
      {"fillpfile", "programs/proof/1121_fillpfile.cubalc", "FILLPFILE materialize {{key}} template file from PLATE"},
      {"cat_wide", "programs/proof/1122_cat_wide.cubalc", "SYS CAT multi-arg host-wide concat for plate messages"},
      {"str_wide", "programs/proof/1123_str_wide.cubalc", "EQS/HAS/STARTS/ENDS/FIND/MID/REPLACE host-wide multi-KB"},
      {"fillp_strict", "programs/proof/1124_fillp_strict.cubalc", "NEEDFILLP/FILLP STRICT + FILLPKEYS template contract"},
      {"fillpfile_strict", "programs/proof/1125_fillpfile_strict.cubalc", "NEEDFILLPFILE fail-fast plate template materialize"},
      {"fillp_from", "programs/proof/1127_fillp_from.cubalc", "FILLP FROM other plate/var/LAST multi-plate templates"},
      {"norm_wide", "programs/proof/1128_norm_wide.cubalc", "TRIM/UPPER/LOWER/REVS host-wide multi-KB normalize"},
      {"fillpfile_from", "programs/proof/1129_fillpfile_from.cubalc", "FILLPFILE FROM multi-plate disk materialize"},
    };
    int i, n = (int)(sizeof tests / sizeof tests[0]);
    int n_pass = 0, n_fail = 0, n_miss = 0, aok = 0, afail = 0;
    int json_only = 0;
    struct {
      char id[32];
      char path[96];
      int ok;
      int missing;
      int asserts_ok;
      int asserts_fail;
      char err[120];
    } rows[1024];
    int nrow = 0;
    if (argc > 2 && (!strcmp(argv[2], "--json") || !strcmp(argv[2], "-j")))
      json_only = 1;
    if (!json_only) {
      printf("# CubalC selftest usability proofs n=%d version=%s\n",
             n, CUBALC_LANG_VERSION);
      printf("# id\tok\tasserts\thint\n");
    }
    /* Cap at rows[] size — must stay ≥ curated tests[] or tail proofs are skipped. */
    if (n > (int)(sizeof rows / sizeof rows[0])) {
      fprintf(stderr,
              "# selftest WARNING: tests[] n=%d > rows cap %d — tail skipped\n",
              n, (int)(sizeof rows / sizeof rows[0]));
    }
    for (i = 0; i < n && nrow < (int)(sizeof rows / sizeof rows[0]); i++) {
      cubalc_run_result rr;
      int is_cli = (strncmp(tests[i].path, "cli:", 4) == 0);
      int missing = is_cli ? 0 : (access(tests[i].path, R_OK) != 0);
      int ok = 0;
      memset(&rr, 0, sizeof rr);
      snprintf(rows[nrow].id, sizeof rows[0].id, "%s", tests[i].id);
      snprintf(rows[nrow].path, sizeof rows[0].path, "%s", tests[i].path);
      rows[nrow].missing = missing;
      rows[nrow].err[0] = 0;
      if (missing) {
        n_miss++;
        n_fail++;
        snprintf(rows[nrow].err, sizeof rows[0].err, "missing file");
        rows[nrow].ok = 0;
        rows[nrow].asserts_ok = 0;
        rows[nrow].asserts_fail = 0;
        if (!json_only)
          printf("%s\tFAIL\t-\t%s (missing)\n", tests[i].id, tests[i].hint);
      } else if (is_cli && !strcmp(tests[i].id, "run_expr")) {
        /* Live CLI smoke: cubalc run -e (no temp file). Uses argv[0] binary. */
        char cmd[768], line[512];
        FILE *fp;
        int saw_ok = 0, saw_asserts = 0;
        int prc;
        snprintf(cmd, sizeof cmd,
                 "'%s' run -q -e 'HOLD_FLASH 1\\nASSERT 1 == 1\\nASSERT 2 == 2' 2>/dev/null",
                 argv[0]);
        fp = popen(cmd, "r");
        if (!fp) {
          n_fail++;
          rows[nrow].ok = 0;
          snprintf(rows[nrow].err, sizeof rows[0].err, "popen failed");
          if (!json_only)
            printf("%s\tFAIL\tcli\t%s — popen\n", tests[i].id, tests[i].hint);
        } else {
          while (fgets(line, sizeof line, fp)) {
            if (strstr(line, "\"ok\":true") && strstr(line, "<expr>"))
              saw_ok = 1;
            if (strstr(line, "asserts_ok\":2"))
              saw_asserts = 1;
          }
          prc = pclose(fp);
          ok = (prc == 0 && saw_ok && saw_asserts);
          rows[nrow].ok = ok ? 1 : 0;
          rows[nrow].asserts_ok = ok ? 2 : 0;
          rows[nrow].asserts_fail = ok ? 0 : 1;
          if (ok) {
            n_pass++;
            aok += 2;
            if (!json_only)
              printf("%s\tPASS\t2\t%s\n", tests[i].id, tests[i].hint);
          } else {
            n_fail++;
            afail += 1;
            snprintf(rows[nrow].err, sizeof rows[0].err, "run -e plate not ok");
            if (!json_only)
              printf("%s\tFAIL\tcli\t%s — %s\n", tests[i].id, tests[i].hint,
                     rows[nrow].err);
          }
        }
      } else {
        int rc = cubalc_run_file(tests[i].path, &rr, NULL);
        ok = (rc == 0 && rr.ok);
        rows[nrow].ok = ok ? 1 : 0;
        rows[nrow].asserts_ok = rr.asserts_ok;
        rows[nrow].asserts_fail = rr.asserts_fail;
        aok += rr.asserts_ok;
        afail += rr.asserts_fail;
        if (ok) {
          n_pass++;
          if (!json_only)
            printf("%s\tPASS\t%d\t%s\n", tests[i].id, rr.asserts_ok,
                   tests[i].hint);
        } else {
          n_fail++;
          snprintf(rows[nrow].err, sizeof rows[0].err, "%s",
                   rr.err[0] ? rr.err : (rr.last_err[0] ? rr.last_err : "fail"));
          if (!json_only)
            printf("%s\tFAIL\t%d/%d\t%s — %s\n", tests[i].id, rr.asserts_ok,
                   rr.asserts_fail, tests[i].hint, rows[nrow].err);
        }
      }
      nrow++;
    }
    printf("{\"schema\":\"cubalc.selftest.v1\",\"ok\":%s,\"cmd\":\"selftest\","
           "\"n\":%d,\"n_pass\":%d,\"n_fail\":%d,\"n_missing\":%d,"
           "\"asserts_ok\":%d,\"asserts_fail\":%d,\"version\":\"%s\","
           "\"note\":\"curated usability proofs — not full ISA suite; "
           "skip P2P net; doctor=static selftest=live\","
           "\"tests\":[",
           n_fail == 0 ? "true" : "false", n, n_pass, n_fail, n_miss, aok,
           afail, CUBALC_LANG_VERSION);
    for (i = 0; i < nrow; i++) {
      char eesc[140];
      size_t k, o = 0;
      for (k = 0; rows[i].err[k] && o + 2 < sizeof eesc; k++) {
        char c = rows[i].err[k];
        if (c == '"' || c == '\\') eesc[o++] = '_';
        else if ((unsigned char)c < 32) eesc[o++] = ' ';
        else eesc[o++] = c;
      }
      eesc[o] = 0;
      printf("%s{\"id\":\"%s\",\"path\":\"%s\",\"ok\":%s,\"missing\":%s,"
             "\"asserts_ok\":%d,\"asserts_fail\":%d,\"err\":\"%s\"}",
             i ? "," : "", rows[i].id, rows[i].path,
             rows[i].ok ? "true" : "false",
             rows[i].missing ? "true" : "false", rows[i].asserts_ok,
             rows[i].asserts_fail, eesc);
    }
    printf("]}\n");
    return n_fail == 0 ? 0 : 1;
  }
  if (strcmp(cmd, "forms") == 0 || strcmp(cmd, "ops") == 0 ||
      strcmp(cmd, "forms-list") == 0) {
    return cubalc_cmd_forms(argc, argv);
  }
  if (strcmp(cmd, "onboard") == 0 || strcmp(cmd, "welcome") == 0 ||
      strcmp(cmd, "begin") == 0 || strcmp(cmd, "agent-start") == 0 ||
      strcmp(cmd, "hello-agent") == 0 || strcmp(cmd, "bootstrap") == 0) {
    /* Usability: agent onboarding JSON (dual of START/ONBOARD).
     *   cubalc onboard · cubalc welcome
     * Schema cubalc.start.v1 · (cubalc start remains cookbook alias). */
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    static const char *nexts[] = {
      "cubalc doctor",
      "cubalc guide general",
      "cubalc topics",
      "cubalc init --list",
      "cubalc search KEYWORD",
      "INCLUDE agent_boot",
      "cubalc formguide HASFORM",
      "cubalc errguide LAST_ERR",
      "HOLD_FLASH device/mesh-join only",
    };
    static const char *hint =
      "install/doctor/init surface · VERSION STATUS IDENTITY · GUIDE general";
    int i, nt = (int)(sizeof topics / sizeof topics[0]);
    int nn = (int)(sizeof nexts / sizeof nexts[0]);
    char hj[320];
    size_t o = 0, j;
    for (j = 0; hint[j] && o + 2 < sizeof hj; j++) {
      char c = hint[j];
      if (c == '"' || c == '\\') { hj[o++] = '\\'; hj[o++] = c; }
      else hj[o++] = c;
    }
    hj[o] = 0;
    printf("{\"schema\":\"cubalc.start.v1\",\"ok\":true,\"cmd\":\"onboard\","
           "\"version\":\"%s\",\"topics_n\":%d,\"next_n\":%d,"
           "\"hint\":\"%s\","
           "\"note\":\"agent onboarding · dual of START/ONBOARD · chain guide/doctor/init\","
           "\"topics\":[",
           CUBALC_LANG_VERSION, nt, nn, hj);
    for (i = 0; i < nt; i++)
      printf("%s\"%s\"", i ? "," : "", topics[i]);
    printf("],\"next\":[");
    for (i = 0; i < nn; i++) {
      char esc[160];
      size_t e = 0;
      for (j = 0; nexts[i][j] && e + 2 < sizeof esc; j++) {
        char c = nexts[i][j];
        if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
        else esc[e++] = c;
      }
      esc[e] = 0;
      printf("%s\"%s\"", i ? "," : "", esc);
    }
    printf("]}\n");
    return 0;
  }
  if (strcmp(cmd, "cookbook") == 0 || strcmp(cmd, "start") == 0) {
    printf("CubalC cookbook paths (read these first):\n"
           "  docs/COOKBOOK.md          # hold → place → plug → decide → smx\n"
           "  docs/HOLD_FLASH.md        # user permission before plug-in\n"
           "  docs/P2P_SMX.md           # SERVE/DIAL mesh\n"
           "  docs/CORE_PROTECT.md      # Core protection\n"
           "  programs/lib/             # INCLUDE snippets (agent_boot · hold_seed)\n"
           "  programs/hello_cube.cubalc\n"
           "  programs/proof/12_hold_flash_plug.cubalc\n"
           "  programs/p2p/mesh_local.cubalc\n"
           "  programs/protect/core_protect.cubalc\n"
           "Commands: cubalc onboard · cubalc doctor · cubalc guide · cubalc formguide · cubalc topics · cubalc init\n"
           "Agent first plate: cubalc onboard  (in-lang START|ONBOARD → cubalc.start.v1)\n");
    return 0;
  }
  if (strcmp(cmd, "tips") == 0 || strcmp(cmd, "tip") == 0 ||
      strcmp(cmd, "howto") == 0 || strcmp(cmd, "next") == 0 ||
      strcmp(cmd, "playbook") == 0) {
    /* Usability: curated agent next-steps by topic without cookbook prose.
     *   cubalc tips · cubalc tips cap · cubalc tips run
     * Schema cubalc.tips.v1 · dual of in-lang TIPS. */
    static const struct {
      const char *topic;
      const char *tip;
    } tips[] = {
      {"general", "cubalc doctor — install readiness plate (libs, vars_max, nest)"},
      {"general", "cubalc init --list — scaffold catalog (agent_boot · plate · fat · cap)"},
      {"general", "cubalc search KEYWORD — forms/libs/examples/env discovery"},
      {"general", "HOLD_FLASH is device/mesh-join only · default 1 · not language PLUG"},
      {"general", "INCLUDE agent_boot — REQUIRE VERSION + VERSION plate one-shot"},
      {"cap", "INCLUDE cap_boot — agent_boot + form_guard capability contract"},
      {"cap", "cubalc init --cap path — scaffold capability starter"},
      {"cap", "HASFORM SORTLIBS · NEEDFORMS a b · FORMHINT name for docs"},
      {"cap", "cubalc run -C SORTLIBS,LIBAGE — host form floor (CUBALC_REQUIRE_FORMS)"},
      {"cap", "LISTFORMS SORT · COUNTFORMS FORM · cubalc listforms dual"},
      {"fat", "INCLUDE fat_session — durable nest (fat_boot + plate_boot)"},
      {"fat", "cubalc init --fat-session nest — scaffold durable fat nest"},
      {"fat", "DEFAULT NEED_VARROOM=48 · INCLUDE var_guard before nest specials"},
      {"fat", "cubalc run -T 5000 · INCLUDE time_guard · REMAIN_MS mid-run"},
      {"plate", "INCLUDE plate_session · SETP/NEEDP/SAVEPLATE agent JSON"},
      {"plate", "cubalc plate get|set|uniform path.json — CLI without .cubalc"},
      {"plate", "INCLUDE plate_uniform · PRETTYP for nest inspect without jq"},
      {"plate", "cubalc init --plate — plate_session + uniform + pretty starter"},
      {"p2p", "export CUBALC_SMX_KEY=$(openssl rand -hex 32) before mesh"},
      {"p2p", "SMX SERVE/DIAL · CUBALC_P2P_TIMEOUT · CUBALC_P2P_SOFT soft-fail"},
      {"p2p", "cubalc smx-bus prove-tcp · docs/P2P_SMX.md"},
      {"run", "cubalc run -q -e '…' — plate-only inline program"},
      {"run", "cubalc run -I lib -L dir -R 1.15 -C FORMS -T ms file"},
      {"run", "cubalc run -s / CUBALC_STRICT — soft last_err fails exit"},
      {"run", "WHY / why_hint on run plate after ASSERT or sticky LAST_ERR"},
      {"lib", "cubalc libs [filter] · recipe <lib> · checkdeps <lib>"},
      {"lib", "INCLUDE MATCH|NEWEST needle · NEWESTLIB · SORTLIBS mtime bag"},
      {"lib", "FRESHLIBS needle max_age · LIBAGE name · HASFRESH gate"},
      {"protect", "cubalc protect status — plate summary no board run"},
      {"protect", "cubalc protect all · docs/CORE_PROTECT.md · HOLD_FLASH"},
    };
    const char *topic = (argc > 2 && argv[2][0]) ? argv[2] : "general";
    char tup[32];
    size_t k;
    int i, n = 0, nall = (int)(sizeof tips / sizeof tips[0]);
    for (k = 0; topic[k] && k + 1 < sizeof tup; k++)
      tup[k] = (char)((topic[k] >= 'A' && topic[k] <= 'Z')
                          ? topic[k] - 'A' + 'a' : topic[k]);
    tup[k] = 0;
    /* aliases */
    if (!strcmp(tup, "capability") || !strcmp(tup, "forms") || !strcmp(tup, "form"))
      snprintf(tup, sizeof tup, "%s", "cap");
    if (!strcmp(tup, "mesh") || !strcmp(tup, "smx") || !strcmp(tup, "peer"))
      snprintf(tup, sizeof tup, "%s", "p2p");
    if (!strcmp(tup, "nest") || !strcmp(tup, "var") || !strcmp(tup, "timeout"))
      snprintf(tup, sizeof tup, "%s", "fat");
    if (!strcmp(tup, "json") || !strcmp(tup, "agent"))
      snprintf(tup, sizeof tup, "%s", "plate");
    if (!strcmp(tup, "start") || !strcmp(tup, "help") || !strcmp(tup, "all") ||
        !strcmp(tup, "default") || !strcmp(tup, "*"))
      snprintf(tup, sizeof tup, "%s", "general");
    printf("{\"schema\":\"cubalc.tips.v1\",\"ok\":true,\"cmd\":\"tips\","
           "\"topic\":\"%s\",\"version\":\"%s\","
           "\"note\":\"curated agent next-steps · dual of in-lang TIPS · no cookbook\","
           "\"topics\":[\"general\",\"cap\",\"fat\",\"plate\",\"p2p\",\"run\",\"lib\",\"protect\"],"
           "\"tips\":[",
           tup, CUBALC_LANG_VERSION);
    for (i = 0; i < nall; i++) {
      int hit = 0;
      if (!strcmp(tips[i].topic, tup) || !strcmp(tup, "general")) {
        /* general shows general only; other topics exact */
        if (!strcmp(tup, "general"))
          hit = !strcmp(tips[i].topic, "general");
        else
          hit = !strcmp(tips[i].topic, tup);
      }
      if (!hit) continue;
      /* JSON-escape tip */
      {
        char esc[240];
        size_t j, o = 0;
        const char *s = tips[i].tip;
        for (j = 0; s[j] && o + 2 < sizeof esc; j++) {
          char c = s[j];
          if (c == '"' || c == '\\') {
            esc[o++] = '\\';
            esc[o++] = c;
          } else if ((unsigned char)c < 32)
            esc[o++] = ' ';
          else
            esc[o++] = c;
        }
        esc[o] = 0;
        printf("%s\"%s\"", n ? "," : "", esc);
        n++;
      }
    }
    printf("],\"n\":%d}\n", n);
    return n > 0 ? 0 : 1;
  }
  if (strcmp(cmd, "formsfor") == 0 || strcmp(cmd, "topicforms") == 0 ||
      strcmp(cmd, "formstopic") == 0 || strcmp(cmd, "surface") == 0 ||
      strcmp(cmd, "forms-for") == 0 || strcmp(cmd, "capforms") == 0) {
    /* Usability: curated form-name bags by topic (dual of FORMSFOR / TIPS).
     *   cubalc formsfor · cubalc formsfor cap · cubalc topicforms fat
     * Schema cubalc.formsfor.v1 · agents get capability surface without LISTFORMS soup. */
    static const struct {
      const char *topic;
      const char *form;
    } rows[] = {
      {"general", "VERSION"},
      {"general", "STATUS"},
      {"general", "IDENTITY"},
      {"general", "TIPS"},
      {"general", "FORMSFOR"},
      {"general", "WHY"},
      {"general", "CLEAR_ERR"},
      {"general", "INCLUDE"},
      {"general", "REQUIRE VERSION"},
      {"general", "VARS"},
      {"cap", "HASFORM"},
      {"cap", "NEEDFORM"},
      {"cap", "HASFORMS"},
      {"cap", "NEEDFORMS"},
      {"cap", "FORMHINT"},
      {"cap", "LISTFORMS"},
      {"cap", "COUNTFORMS"},
      {"cap", "REQUIRE FORM"},
      {"cap", "TIPS"},
      {"cap", "FORMSFOR"},
      {"fat", "VARROOM"},
      {"fat", "HASVARROOM"},
      {"fat", "NEEDVARROOM"},
      {"fat", "REMAIN_MS"},
      {"fat", "HAS_TIME"},
      {"fat", "NEEDTIME"},
      {"fat", "STATUS"},
      {"fat", "VARS"},
      {"fat", "TIPS"},
      {"plate", "SETP"},
      {"plate", "GETP"},
      {"plate", "NEEDP"},
      {"plate", "DEFAULTP"},
      {"plate", "INCP"},
      {"plate", "DELP"},
      {"plate", "PRETTYP"},
      {"plate", "DUMPP"},
      {"plate", "SAVEPLATE"},
      {"plate", "LOADPLATE"},
      {"p2p", "SMX"},
      {"p2p", "SERVE"},
      {"p2p", "DIAL"},
      {"p2p", "TALK"},
      {"p2p", "EXCHANGE"},
      {"p2p", "TIPS"},
      {"run", "ASSERT"},
      {"run", "EXPECT"},
      {"run", "FAIL"},
      {"run", "PASS"},
      {"run", "NOTE"},
      {"run", "EXIT"},
      {"run", "WHY"},
      {"run", "CLEAR_ERR"},
      {"run", "TIPS"},
      {"lib", "LISTLIBS"},
      {"lib", "HASLIB"},
      {"lib", "MATCHLIBS"},
      {"lib", "PICKLIB"},
      {"lib", "RECIPE"},
      {"lib", "CHECKDEPS"},
      {"lib", "SORTLIBS"},
      {"lib", "FRESHLIBS"},
      {"lib", "LIBAGE"},
      {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"},
      {"protect", "STATUS"},
      {"protect", "VERSION"},
      {"protect", "TIPS"},
    };
    const char *topic = (argc > 2 && argv[2][0]) ? argv[2] : "general";
    char tup[32];
    size_t k;
    int i, n = 0, nall = (int)(sizeof rows / sizeof rows[0]);
    for (k = 0; topic[k] && k + 1 < sizeof tup; k++)
      tup[k] = (char)((topic[k] >= 'A' && topic[k] <= 'Z')
                          ? topic[k] - 'A' + 'a' : topic[k]);
    tup[k] = 0;
    if (!strcmp(tup, "capability") || !strcmp(tup, "forms") || !strcmp(tup, "form"))
      snprintf(tup, sizeof tup, "%s", "cap");
    if (!strcmp(tup, "mesh") || !strcmp(tup, "smx") || !strcmp(tup, "peer"))
      snprintf(tup, sizeof tup, "%s", "p2p");
    if (!strcmp(tup, "nest") || !strcmp(tup, "var") || !strcmp(tup, "timeout"))
      snprintf(tup, sizeof tup, "%s", "fat");
    if (!strcmp(tup, "json") || !strcmp(tup, "agent"))
      snprintf(tup, sizeof tup, "%s", "plate");
    if (!strcmp(tup, "start") || !strcmp(tup, "help") || !strcmp(tup, "all") ||
        !strcmp(tup, "default") || !strcmp(tup, "*"))
      snprintf(tup, sizeof tup, "%s", "general");
    printf("{\"schema\":\"cubalc.formsfor.v1\",\"ok\":true,\"cmd\":\"formsfor\","
           "\"topic\":\"%s\",\"version\":\"%s\","
           "\"note\":\"curated form-name bag by topic · dual of in-lang FORMSFOR · TIPS twin\","
           "\"topics\":[\"general\",\"cap\",\"fat\",\"plate\",\"p2p\",\"run\",\"lib\",\"protect\"],"
           "\"forms\":[",
           tup, CUBALC_LANG_VERSION);
    for (i = 0; i < nall; i++) {
      if (strcmp(rows[i].topic, tup) != 0)
        continue;
      printf("%s\"%s\"", n ? "," : "", rows[i].form);
      n++;
    }
    printf("],\"n\":%d}\n", n);
    return n > 0 ? 0 : 1;
  }
  if (strcmp(cmd, "related") == 0 || strcmp(cmd, "seealso") == 0 ||
      strcmp(cmd, "formslike") == 0 || strcmp(cmd, "nearforms") == 0 ||
      strcmp(cmd, "formrel") == 0 || strcmp(cmd, "neighbors") == 0) {
    /* Usability: related form-name bag for one form (dual of RELATED / FORMSFOR).
     *   cubalc related HASFORM · cubalc seealso SETP
     * Schema cubalc.related.v1 · complements formhint + formsfor. */
    static const struct { const char *form; const char *rel; } edges[] = {
      {"HASFORM", "NEEDFORM"}, {"HASFORM", "HASFORMS"}, {"HASFORM", "NEEDFORMS"},
      {"HASFORM", "FORMHINT"}, {"HASFORM", "LISTFORMS"}, {"HASFORM", "COUNTFORMS"},
      {"HASFORM", "REQUIRE FORM"}, {"HASFORM", "FORMSFOR"}, {"HASFORM", "TIPS"},
      {"NEEDFORM", "HASFORM"}, {"NEEDFORM", "NEEDFORMS"}, {"NEEDFORM", "HASFORMS"},
      {"NEEDFORM", "FORMHINT"}, {"NEEDFORM", "REQUIRE FORM"},
      {"HASFORMS", "NEEDFORMS"}, {"HASFORMS", "HASFORM"},
      {"HASFORMS", "HASFORMANY"}, {"HASFORMS", "NEEDFORMANY"},
      {"NEEDFORMS", "HASFORMS"}, {"NEEDFORMS", "NEEDFORM"}, {"NEEDFORMS", "HASFORM"},
      {"NEEDFORMS", "NEEDFORMANY"},
      {"HASFORMANY", "NEEDFORMANY"}, {"HASFORMANY", "HASFORMS"}, {"HASFORMANY", "HASFORM"},
      {"HASFORMANY", "FORMHAVE"}, {"HASFORMANY", "FORMMISS"}, {"HASFORMANY", "LISTFORMS"},
      {"NEEDFORMANY", "HASFORMANY"}, {"NEEDFORMANY", "NEEDFORMS"}, {"NEEDFORMANY", "NEEDFORM"},
      {"NEEDFORMANY", "FORMMISS"}, {"NEEDFORMANY", "FORMHAVE"},
      {"FORMHINT", "HASFORM"}, {"FORMHINT", "LISTFORMS"}, {"FORMHINT", "RELATED"},
      {"FORMHINT", "FORMSFOR"}, {"FORMHINT", "TIPS"},
      {"LISTFORMS", "COUNTFORMS"}, {"LISTFORMS", "HASFORM"}, {"LISTFORMS", "FORMHINT"},
      {"LISTFORMS", "FORMSFOR"}, {"COUNTFORMS", "LISTFORMS"}, {"COUNTFORMS", "HASFORM"},
      {"FORMSFOR", "TIPS"}, {"FORMSFOR", "RELATED"}, {"FORMSFOR", "FORMHINT"},
      {"FORMSFOR", "LISTFORMS"}, {"FORMSFOR", "HASFORM"},
      {"TIPS", "FORMSFOR"}, {"TIPS", "WHY"}, {"TIPS", "FORMHINT"}, {"TIPS", "RELATED"},
      {"INCLUDE", "LISTLIBS"}, {"INCLUDE", "HASLIB"}, {"INCLUDE", "MATCHLIBS"},
      {"INCLUDE", "PICKLIB"}, {"INCLUDE", "RECIPE"}, {"INCLUDE", "CHECKDEPS"},
      {"LISTLIBS", "HASLIB"}, {"LISTLIBS", "MATCHLIBS"}, {"LISTLIBS", "RECIPE"},
      {"LISTLIBS", "CATLIB"}, {"MATCHLIBS", "PICKLIB"}, {"MATCHLIBS", "NTHLIB"},
      {"MATCHLIBS", "SORTLIBS"}, {"MATCHLIBS", "FRESHLIBS"},
      {"PICKLIB", "MATCHLIBS"}, {"PICKLIB", "NTHLIB"}, {"PICKLIB", "INCLUDE"},
      {"RECIPE", "LIBDEPS"}, {"RECIPE", "LIBDEFAULTS"}, {"RECIPE", "CHECKDEPS"},
      {"RECIPE", "LIBINFO"}, {"CHECKDEPS", "HASDEPS"}, {"CHECKDEPS", "NEEDDEPS"},
      {"CHECKDEPS", "RECIPE"}, {"SORTLIBS", "NEWESTLIB"}, {"SORTLIBS", "OLDESTLIB"},
      {"SORTLIBS", "FRESHLIBS"}, {"FRESHLIBS", "STALELIBS"}, {"FRESHLIBS", "LIBAGE"},
      {"LIBAGE", "HASFRESH"}, {"LIBAGE", "NEEDFRESH"}, {"LIBAGE", "FRESHLIBS"},
      {"CATLIB", "HEADLIB"}, {"CATLIB", "TAILLIB"}, {"CATLIB", "GREPLIB"},
      {"SETP", "GETP"}, {"SETP", "NEEDP"}, {"SETP", "DEFAULTP"}, {"SETP", "INCP"},
      {"SETP", "DELP"}, {"SETP", "SAVEPLATE"}, {"SETP", "LOADPLATE"},
      {"GETP", "SETP"}, {"GETP", "NEEDP"}, {"GETP", "GETPN"}, {"GETP", "HASP"},
      {"NEEDP", "SETP"}, {"NEEDP", "GETP"}, {"NEEDP", "HASP"}, {"NEEDP", "DEFAULTP"},
      {"DEFAULTP", "SETP"}, {"DEFAULTP", "NEEDP"}, {"INCP", "SETP"}, {"INCP", "GETPN"},
      {"DELP", "SETP"}, {"DELP", "GETP"}, {"PRETTYP", "DUMPP"}, {"PRETTYP", "SAVEPLATE"},
      {"DUMPP", "PRETTYP"}, {"DUMPP", "GETP"}, {"SAVEPLATE", "LOADPLATE"},
      {"SAVEPLATE", "SETP"}, {"LOADPLATE", "SAVEPLATE"}, {"LOADPLATE", "NEEDP"},
      {"VARROOM", "HASVARROOM"}, {"VARROOM", "NEEDVARROOM"}, {"VARROOM", "VARS"},
      {"HASVARROOM", "NEEDVARROOM"}, {"HASVARROOM", "VARROOM"},
      {"NEEDVARROOM", "HASVARROOM"}, {"NEEDVARROOM", "VARROOM"},
      {"REMAIN_MS", "HAS_TIME"}, {"REMAIN_MS", "NEEDTIME"}, {"REMAIN_MS", "WALL_MS"},
      {"HAS_TIME", "NEEDTIME"}, {"HAS_TIME", "REMAIN_MS"},
      {"NEEDTIME", "HAS_TIME"}, {"NEEDTIME", "REMAIN_MS"},
      {"ASSERT", "EXPECT"}, {"ASSERT", "WHY"}, {"ASSERT", "FAIL"}, {"ASSERT", "PASS"},
      {"EXPECT", "ASSERT"}, {"EXPECT", "WHY"}, {"EXPECT", "CLEAR_ERR"}, {"EXPECT", "FAIL"},
      {"FAIL", "PASS"}, {"FAIL", "WHY"}, {"FAIL", "CLEAR_ERR"}, {"FAIL", "EXPECT"},
      {"PASS", "FAIL"}, {"PASS", "NOTE"}, {"WHY", "CLEAR_ERR"}, {"WHY", "STATUS"},
      {"WHY", "TIPS"}, {"CLEAR_ERR", "WHY"}, {"CLEAR_ERR", "STATUS"},
      {"STATUS", "IDENTITY"}, {"STATUS", "VERSION"}, {"STATUS", "VARS"},
      {"VERSION", "REQUIRE VERSION"}, {"VERSION", "STATUS"},
      {"EXIT", "PASS"}, {"EXIT", "FAIL"}, {"EXIT", "STATUS"},
      {"NOTE", "PASS"}, {"NOTE", "TIPS"},
      {"SMX", "SERVE"}, {"SMX", "DIAL"}, {"SMX", "TALK"}, {"SMX", "EXCHANGE"},
      {"SERVE", "DIAL"}, {"SERVE", "SMX"}, {"DIAL", "SERVE"}, {"DIAL", "SMX"},
      {"TALK", "SMX"}, {"TALK", "EXCHANGE"}, {"EXCHANGE", "SMX"}, {"EXCHANGE", "TALK"},
      {"HOLD_FLASH", "STATUS"}, {"HOLD_FLASH", "VERSION"}, {"HOLD_FLASH", "TIPS"},
      /* multi CLI flag contract (HASFORMS twin for --flags) */
      {"HASFLAG", "HASFLAGALL"}, {"HASFLAG", "NEEDFLAGS"}, {"HASFLAG", "REQUIRE FLAG"},
      {"HASFLAGALL", "NEEDFLAGS"}, {"HASFLAGALL", "HASFLAG"}, {"HASFLAGALL", "FLAGMISS"},
      {"HASFLAGALL", "HASFLAGS"}, {"HASFLAGALL", "LISTFLAGS"}, {"HASFLAGALL", "HASFLAGANY"},
      {"NEEDFLAGS", "HASFLAGALL"}, {"NEEDFLAGS", "HASFLAG"}, {"NEEDFLAGS", "REQUIRE FLAG"},
      {"NEEDFLAGS", "USAGE"}, {"NEEDFLAGS", "FLAGMISS"}, {"NEEDFLAGS", "NEEDFLAGANY"},
      /* multi named-flag ANY gate (HASFLAGALL twin) */
      {"HASFLAGANY", "NEEDFLAGANY"}, {"HASFLAGANY", "HASFLAGALL"}, {"HASFLAGANY", "HASFLAG"},
      {"HASFLAGANY", "FLAGHAVE"}, {"HASFLAGANY", "LISTFLAGS"}, {"HASFLAGANY", "FLAGMISS"},
      {"NEEDFLAGANY", "HASFLAGANY"}, {"NEEDFLAGANY", "NEEDFLAGS"}, {"NEEDFLAGANY", "HASFLAG"},
      {"NEEDFLAGANY", "USAGE"}, {"NEEDFLAGANY", "FLAGMISS"},
      {"HASFLAGS", "HASFLAGALL"}, {"HASFLAGS", "LISTFLAGS"}, {"HASFLAGS", "HASFLAG"},
      {"LISTFLAGS", "HASFLAGS"}, {"LISTFLAGS", "HASFLAGALL"}, {"LISTFLAGS", "FLAGMAP"},
      /* multi CLI arg contract (HASFLAGALL twin for CUBALC_ARGn) */
      {"HASARG", "HASARGALL"}, {"HASARG", "NEEDARGS"}, {"HASARG", "REQUIRE ARG"},
      {"HASARG", "HASARGANY"}, {"HASARG", "NEEDARGANY"},
      {"HASARGALL", "NEEDARGS"}, {"HASARGALL", "HASARG"}, {"HASARGALL", "ARGMISS"},
      {"HASARGALL", "HASARGC"}, {"HASARGALL", "RESTARGS"}, {"HASARGALL", "HASARGANY"},
      {"NEEDARGS", "HASARGALL"}, {"NEEDARGS", "HASARG"}, {"NEEDARGS", "REQUIRE ARG"},
      {"NEEDARGS", "USAGE"}, {"NEEDARGS", "ARGMISS"}, {"NEEDARGS", "NEEDARGANY"},
      {"HASARGANY", "NEEDARGANY"}, {"HASARGANY", "HASARGALL"}, {"HASARGANY", "HASARG"},
      {"HASARGANY", "ARGHAVE"}, {"HASARGANY", "ARGMISS"},
      {"NEEDARGANY", "HASARGANY"}, {"NEEDARGANY", "NEEDARGS"}, {"NEEDARGANY", "HASARG"},
      {"NEEDARGANY", "USAGE"}, {"NEEDARGANY", "ARGMISS"},
      /* multi host-env ALL gate (HASARGALL twin for process env) */
      {"HASENVALL", "NEEDENVS"}, {"HASENVALL", "SYS HASENV"}, {"HASENVALL", "REQUIRE ENV"},
      {"HASENVALL", "ENVMISS"}, {"HASENVALL", "ENVHAVE"}, {"HASENVALL", "HASARGALL"},
      {"HASENVALL", "HASENVANY"}, {"HASENVALL", "NEEDENVANY"},
      {"NEEDENVS", "HASENVALL"}, {"NEEDENVS", "REQUIRE ENV"}, {"NEEDENVS", "SYS HASENV"},
      {"NEEDENVS", "USAGE"}, {"NEEDENVS", "ENVMISS"}, {"NEEDENVS", "SYS ENV SET"},
      {"NEEDENVS", "NEEDENVANY"},
      {"HASENVANY", "NEEDENVANY"}, {"HASENVANY", "HASENVALL"}, {"HASENVANY", "SYS HASENV"},
      {"HASENVANY", "ENVHAVE"}, {"HASENVANY", "ENVMISS"}, {"HASENVANY", "HASARGANY"},
      {"NEEDENVANY", "HASENVANY"}, {"NEEDENVANY", "NEEDENVS"}, {"NEEDENVANY", "REQUIRE ENV"},
      {"NEEDENVANY", "USAGE"}, {"NEEDENVANY", "ENVMISS"}, {"NEEDENVANY", "SYS ENV SET"},
      {"HASARGC", "HASARGALL"}, {"HASARGC", "HASARG"}, {"HASARGC", "NEEDARGS"},
    };
    const char *name = (argc > 2 && argv[2][0]) ? argv[2] : "";
    char nup[96];
    char forms_json[2048];
    size_t k, fo = 0;
    int i, n = 0, nall = (int)(sizeof edges / sizeof edges[0]), seen = 0;
    if (!name[0]) {
      fprintf(stderr, "usage: cubalc related|seealso <FormName>\n");
      printf("{\"schema\":\"cubalc.related.v1\",\"ok\":false,\"cmd\":\"related\","
             "\"err\":\"need form name\",\"version\":\"%s\"}\n",
             CUBALC_LANG_VERSION);
      return 2;
    }
    for (k = 0; name[k] && k + 1 < sizeof nup; k++) {
      char c = name[k];
      if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
      nup[k] = c;
    }
    nup[k] = 0;
    forms_json[0] = 0;
    for (i = 0; i < nall; i++) {
      char fup[96];
      size_t j, ln;
      for (j = 0; edges[i].form[j] && j + 1 < sizeof fup; j++) {
        char c = edges[i].form[j];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        fup[j] = c;
      }
      fup[j] = 0;
      if (strcmp(fup, nup) != 0) continue;
      seen = 1;
      ln = strlen(edges[i].rel);
      if (fo && fo + 1 < sizeof forms_json) forms_json[fo++] = ',';
      if (fo + ln + 3 < sizeof forms_json) {
        forms_json[fo++] = '"';
        memcpy(forms_json + fo, edges[i].rel, ln);
        fo += ln;
        forms_json[fo++] = '"';
        forms_json[fo] = 0;
      }
      n++;
    }
    printf("{\"schema\":\"cubalc.related.v1\",\"ok\":%s,\"cmd\":\"related\","
           "\"form\":\"%s\",\"n\":%d,\"version\":\"%s\","
           "\"note\":\"related form-name bag · dual of in-lang RELATED · complements formhint/formsfor\","
           "\"related\":[%s]}\n",
           (seen && n > 0) ? "true" : "false", name, n, CUBALC_LANG_VERSION,
           forms_json);
    return (seen && n > 0) ? 0 : 1;
  }
  if (strcmp(cmd, "snip") == 0 || strcmp(cmd, "snippet") == 0 ||
      strcmp(cmd, "mini") == 0 || strcmp(cmd, "miniprog") == 0 ||
      strcmp(cmd, "codesnip") == 0 || strcmp(cmd, "starter-src") == 0) {
    /* Usability: curated mini runnable source by topic (dual of SNIP).
     *   cubalc snip · cubalc snip cap · cubalc snippet plate
     * Schema cubalc.snip.v1 · triad with tips + formsfor (prose / names / code). */
    static const struct { const char *topic; const char *src; } snips[] = {
      {"general",
       "VERSION\nSTATUS\nPRINT \"version\" VERSION\nPASS\n"},
      {"cap",
       "HASFORM SORTLIBS\nASSERT LAST_N == 1 \"need SORTLIBS form\"\n"
       "FORMHINT SORTLIBS\nPRINT LAST\nRELATED HASFORM\nPRINT RELATED\nPASS\n"},
      {"fat",
       "VARROOM\nPRINT \"free_slots\" LAST_N\nHASVARROOM 8\nASSERT LAST_N == 1\n"
       "REMAIN_MS\nPRINT \"remain_ms\" LAST_N\nPASS\n"},
      {"plate",
       "LET PLATE = \"{\\\"n\\\":0,\\\"ok\\\":true}\"\n"
       "SETP \"status\" \"ready\"\nINCP \"n\"\nNEEDP \"n\" \"ok\" \"status\"\n"
       "PRETTYP\nPRINT LAST\nPASS\n"},
      {"p2p",
       "NOTE \"set CUBALC_SMX_KEY before mesh\"\nTIPS p2p\nPRINT LAST\n"
       "FORMSFOR p2p\nPRINT LAST\nPASS\n"},
      {"run",
       "EXPECT 1 == 1\nWHY\nPRINT WHY_HINT\nCLEAR_ERR\nPASS \"run probe ok\"\n"},
      {"lib",
       "LISTLIBS\nPRINT \"libs\" LAST_N\nHASLIB agent_boot\nASSERT LAST_N == 1\n"
       "RECIPE agent_boot\nPRINT LAST\nPASS\n"},
      {"protect",
       "HOLD_FLASH\nPRINT \"hold_flash\" LAST_N\nVERSION\nSTATUS\n"
       "TIPS protect\nPRINT LAST\nPASS\n"},
    };
    const char *topic = (argc > 2 && argv[2][0]) ? argv[2] : "general";
    char tup[32];
    char esc[2048];
    size_t k, o = 0;
    int i, nall = (int)(sizeof snips / sizeof snips[0]);
    const char *src = NULL;
    int lines = 0;
    const char *p;
    for (k = 0; topic[k] && k + 1 < sizeof tup; k++)
      tup[k] = (char)((topic[k] >= 'A' && topic[k] <= 'Z')
                          ? topic[k] - 'A' + 'a' : topic[k]);
    tup[k] = 0;
    if (!strcmp(tup, "capability") || !strcmp(tup, "forms") || !strcmp(tup, "form"))
      snprintf(tup, sizeof tup, "%s", "cap");
    if (!strcmp(tup, "mesh") || !strcmp(tup, "smx") || !strcmp(tup, "peer"))
      snprintf(tup, sizeof tup, "%s", "p2p");
    if (!strcmp(tup, "nest") || !strcmp(tup, "var") || !strcmp(tup, "timeout"))
      snprintf(tup, sizeof tup, "%s", "fat");
    if (!strcmp(tup, "json") || !strcmp(tup, "agent"))
      snprintf(tup, sizeof tup, "%s", "plate");
    if (!strcmp(tup, "start") || !strcmp(tup, "help") || !strcmp(tup, "all") ||
        !strcmp(tup, "default") || !strcmp(tup, "*"))
      snprintf(tup, sizeof tup, "%s", "general");
    for (i = 0; i < nall; i++) {
      if (!strcmp(snips[i].topic, tup)) {
        src = snips[i].src;
        break;
      }
    }
    if (!src) {
      printf("{\"schema\":\"cubalc.snip.v1\",\"ok\":false,\"cmd\":\"snip\","
             "\"topic\":\"%s\",\"err\":\"unknown topic\","
             "\"topics\":[\"general\",\"cap\",\"fat\",\"plate\",\"p2p\",\"run\",\"lib\",\"protect\"],"
             "\"version\":\"%s\"}\n",
             tup, CUBALC_LANG_VERSION);
      return 1;
    }
    for (p = src; *p; p++) {
      if (*p == '\n') lines++;
      if (o + 2 >= sizeof esc) break;
      if (*p == '"' || *p == '\\') {
        esc[o++] = '\\';
        esc[o++] = *p;
      } else if (*p == '\n') {
        esc[o++] = '\\';
        esc[o++] = 'n';
      } else if (*p == '\r' || *p == '\t') {
        esc[o++] = ' ';
      } else if ((unsigned char)*p < 32) {
        continue;
      } else {
        esc[o++] = *p;
      }
    }
    esc[o] = 0;
    if (lines == 0 && src[0]) lines = 1;
    printf("{\"schema\":\"cubalc.snip.v1\",\"ok\":true,\"cmd\":\"snip\","
           "\"topic\":\"%s\",\"lines\":%d,\"version\":\"%s\","
           "\"note\":\"mini runnable source · dual of in-lang SNIP · triad with tips/formsfor\","
           "\"topics\":[\"general\",\"cap\",\"fat\",\"plate\",\"p2p\",\"run\",\"lib\",\"protect\"],"
           "\"source\":\"%s\"}\n",
           tup, lines, CUBALC_LANG_VERSION, esc);
    return 0;
  }
  if (strcmp(cmd, "topic") == 0 || strcmp(cmd, "topiccard") == 0 ||
      strcmp(cmd, "topicinfo") == 0 || strcmp(cmd, "playcard") == 0 ||
      strcmp(cmd, "discover-topic") == 0 || strcmp(cmd, "topicpack") == 0) {
    /* Usability: one plate triad tips+forms+snip (dual of TOPIC / TOPICCARD).
     *   cubalc topic · cubalc topic cap · cubalc topiccard fat
     * Schema cubalc.topic.v1 · agents skip three separate discovery calls. */
    static const struct { const char *topic; const char *tip; } tips[] = {
      {"general", "cubalc doctor — install readiness plate"},
      {"general", "cubalc init --list — scaffold catalog"},
      {"general", "cubalc search KEYWORD — forms/libs/examples"},
      {"general", "INCLUDE agent_boot — REQUIRE VERSION + VERSION plate"},
      {"general", "HOLD_FLASH device/mesh-join only · default 1"},
      {"cap", "INCLUDE cap_boot — agent_boot + form_guard"},
      {"cap", "cubalc init --cap · HASFORM · NEEDFORMS · FORMHINT"},
      {"cap", "cubalc run -C FORMS · CUBALC_REQUIRE_FORMS floor"},
      {"ready", "cubalc run -Y · CUBALC_REQUIRE_READY prove floor"},
      {"cap", "LISTFORMS prefix · COUNTFORMS · cubalc listforms"},
      {"fat", "INCLUDE fat_session durable nest"},
      {"fat", "cubalc init --fat-session · NEED_VARROOM · time_guard"},
      {"fat", "cubalc run -T ms · REMAIN_MS · VARROOM"},
      {"plate", "INCLUDE plate_session · SETP NEEDP SAVEPLATE"},
      {"plate", "cubalc plate get|set|uniform · PRETTYP"},
      {"plate", "cubalc init --plate plate_uniform starter"},
      {"p2p", "export CUBALC_SMX_KEY hex64 · SMX SERVE/DIAL"},
      {"p2p", "CUBALC_P2P_TIMEOUT · CUBALC_P2P_SOFT · smx-bus prove-tcp"},
      {"run", "cubalc run -q -e CODE · -I -L -R -C -T -s"},
      {"run", "WHY / why_hint after ASSERT or sticky LAST_ERR"},
      {"lib", "cubalc libs|recipe|checkdeps · INCLUDE MATCH|NEWEST"},
      {"lib", "SORTLIBS · FRESHLIBS · LIBAGE · HASFRESH"},
      {"protect", "cubalc protect status · docs/CORE_PROTECT.md"},
    };
    static const struct { const char *topic; const char *form; } forms[] = {
      {"general", "VERSION"}, {"general", "STATUS"}, {"general", "IDENTITY"},
      {"general", "TIPS"}, {"general", "FORMSFOR"}, {"general", "SNIP"},
      {"general", "TOPIC"}, {"general", "WHY"}, {"general", "INCLUDE"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORM"}, {"cap", "HASFORMS"},
      {"cap", "NEEDFORMS"}, {"cap", "FORMHINT"}, {"cap", "LISTFORMS"},
      {"cap", "RELATED"}, {"cap", "FORMSFOR"}, {"cap", "SNIP"}, {"cap", "TOPIC"},
      {"fat", "VARROOM"}, {"fat", "HASVARROOM"}, {"fat", "NEEDVARROOM"},
      {"fat", "REMAIN_MS"}, {"fat", "HAS_TIME"}, {"fat", "NEEDTIME"}, {"fat", "STATUS"},
      {"plate", "SETP"}, {"plate", "GETP"}, {"plate", "NEEDP"}, {"plate", "DEFAULTP"},
      {"plate", "INCP"}, {"plate", "PRETTYP"}, {"plate", "SAVEPLATE"}, {"plate", "LOADPLATE"},
      {"p2p", "SMX"}, {"p2p", "SERVE"}, {"p2p", "DIAL"}, {"p2p", "TALK"}, {"p2p", "EXCHANGE"},
      {"run", "ASSERT"}, {"run", "EXPECT"}, {"run", "WHY"}, {"run", "CLEAR_ERR"},
      {"run", "FAIL"}, {"run", "PASS"}, {"run", "EXIT"},
      {"lib", "LISTLIBS"}, {"lib", "HASLIB"}, {"lib", "MATCHLIBS"}, {"lib", "PICKLIB"},
      {"lib", "RECIPE"}, {"lib", "CHECKDEPS"}, {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"}, {"protect", "STATUS"}, {"protect", "VERSION"}, {"protect", "TIPS"},
    };
    static const struct { const char *topic; const char *src; } snips[] = {
      {"general", "VERSION\nSTATUS\nPRINT \"version\" VERSION\nPASS\n"},
      {"cap",
       "HASFORM SORTLIBS\nASSERT LAST_N == 1 \"need SORTLIBS form\"\n"
       "FORMHINT SORTLIBS\nPRINT LAST\nRELATED HASFORM\nPRINT RELATED\nPASS\n"},
      {"fat",
       "VARROOM\nPRINT \"free_slots\" LAST_N\nHASVARROOM 8\nASSERT LAST_N == 1\n"
       "REMAIN_MS\nPRINT \"remain_ms\" LAST_N\nPASS\n"},
      {"plate",
       "LET PLATE = \"{\\\"n\\\":0,\\\"ok\\\":true}\"\n"
       "SETP \"status\" \"ready\"\nINCP \"n\"\nNEEDP \"n\" \"ok\" \"status\"\n"
       "PRETTYP\nPRINT LAST\nPASS\n"},
      {"p2p",
       "NOTE \"set CUBALC_SMX_KEY before mesh\"\nTIPS p2p\nPRINT LAST\n"
       "FORMSFOR p2p\nPRINT LAST\nPASS\n"},
      {"run", "EXPECT 1 == 1\nWHY\nPRINT WHY_HINT\nCLEAR_ERR\nPASS \"run probe ok\"\n"},
      {"lib",
       "LISTLIBS\nPRINT \"libs\" LAST_N\nHASLIB agent_boot\nASSERT LAST_N == 1\n"
       "RECIPE agent_boot\nPRINT LAST\nPASS\n"},
      {"protect",
       "HOLD_FLASH\nPRINT \"hold_flash\" LAST_N\nVERSION\nSTATUS\n"
       "TIPS protect\nPRINT LAST\nPASS\n"},
    };
    const char *topic = (argc > 2 && argv[2][0]) ? argv[2] : "general";
    char tup[32];
    char tips_j[2400], forms_j[1200], snip_j[2400];
    size_t k, o;
    int i, nt = 0, nf = 0, lines = 0;
    const char *src = NULL, *p;
    int n_tips = (int)(sizeof tips / sizeof tips[0]);
    int n_forms = (int)(sizeof forms / sizeof forms[0]);
    int n_snips = (int)(sizeof snips / sizeof snips[0]);
    for (k = 0; topic[k] && k + 1 < sizeof tup; k++)
      tup[k] = (char)((topic[k] >= 'A' && topic[k] <= 'Z')
                          ? topic[k] - 'A' + 'a' : topic[k]);
    tup[k] = 0;
    if (!strcmp(tup, "capability") || !strcmp(tup, "forms") || !strcmp(tup, "form"))
      snprintf(tup, sizeof tup, "%s", "cap");
    if (!strcmp(tup, "mesh") || !strcmp(tup, "smx") || !strcmp(tup, "peer"))
      snprintf(tup, sizeof tup, "%s", "p2p");
    if (!strcmp(tup, "nest") || !strcmp(tup, "var") || !strcmp(tup, "timeout"))
      snprintf(tup, sizeof tup, "%s", "fat");
    if (!strcmp(tup, "json") || !strcmp(tup, "agent"))
      snprintf(tup, sizeof tup, "%s", "plate");
    if (!strcmp(tup, "start") || !strcmp(tup, "help") || !strcmp(tup, "all") ||
        !strcmp(tup, "default") || !strcmp(tup, "*"))
      snprintf(tup, sizeof tup, "%s", "general");
    /* tips JSON array */
    o = 0; tips_j[0] = 0;
    for (i = 0; i < n_tips; i++) {
      size_t j, e;
      char esc[280];
      if (strcmp(tips[i].topic, tup) != 0) continue;
      e = 0;
      for (j = 0; tips[i].tip[j] && e + 2 < sizeof esc; j++) {
        char c = tips[i].tip[j];
        if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
        else if ((unsigned char)c < 32) esc[e++] = ' ';
        else esc[e++] = c;
      }
      esc[e] = 0;
      if (o && o + 1 < sizeof tips_j) tips_j[o++] = ',';
      if (o + e + 3 < sizeof tips_j) {
        tips_j[o++] = '"';
        memcpy(tips_j + o, esc, e); o += e;
        tips_j[o++] = '"'; tips_j[o] = 0;
      }
      nt++;
    }
    o = 0; forms_j[0] = 0;
    for (i = 0; i < n_forms; i++) {
      size_t ln;
      if (strcmp(forms[i].topic, tup) != 0) continue;
      ln = strlen(forms[i].form);
      if (o && o + 1 < sizeof forms_j) forms_j[o++] = ',';
      if (o + ln + 3 < sizeof forms_j) {
        forms_j[o++] = '"';
        memcpy(forms_j + o, forms[i].form, ln); o += ln;
        forms_j[o++] = '"'; forms_j[o] = 0;
      }
      nf++;
    }
    for (i = 0; i < n_snips; i++) {
      if (!strcmp(snips[i].topic, tup)) { src = snips[i].src; break; }
    }
    if (!src) {
      printf("{\"schema\":\"cubalc.topic.v1\",\"ok\":false,\"cmd\":\"topic\","
             "\"topic\":\"%s\",\"err\":\"unknown topic\","
             "\"topics\":[\"general\",\"cap\",\"fat\",\"plate\",\"p2p\",\"run\",\"lib\",\"protect\"],"
             "\"version\":\"%s\"}\n",
             tup, CUBALC_LANG_VERSION);
      return 1;
    }
    o = 0; snip_j[0] = 0;
    for (p = src; *p && o + 2 < sizeof snip_j; p++) {
      if (*p == '"' || *p == '\\') { snip_j[o++] = '\\'; snip_j[o++] = *p; }
      else if (*p == '\n') { snip_j[o++] = '\\'; snip_j[o++] = 'n'; lines++; }
      else if ((unsigned char)*p < 32) continue;
      else snip_j[o++] = *p;
    }
    snip_j[o] = 0;
    if (lines == 0 && src[0]) lines = 1;
    printf("{\"schema\":\"cubalc.topic.v1\",\"ok\":true,\"cmd\":\"topic\","
           "\"topic\":\"%s\",\"tips_n\":%d,\"forms_n\":%d,\"snip_lines\":%d,"
           "\"version\":\"%s\","
           "\"note\":\"one plate triad · TIPS+FORMSFOR+SNIP · dual of in-lang TOPIC\","
           "\"topics\":[\"general\",\"cap\",\"fat\",\"plate\",\"p2p\",\"run\",\"lib\",\"protect\"],"
           "\"tips\":[%s],\"forms\":[%s],\"snip\":\"%s\"}\n",
           tup, nt, nf, lines, CUBALC_LANG_VERSION, tips_j, forms_j, snip_j);
    return 0;
  }
  if (strcmp(cmd, "runsnip") == 0 || strcmp(cmd, "sniprun") == 0 ||
      strcmp(cmd, "snip-run") == 0 || strcmp(cmd, "run-snip") == 0 ||
      strcmp(cmd, "exec-snip") == 0 || strcmp(cmd, "play-snip") == 0 ||
      strcmp(cmd, "runsnipmatch") == 0 || strcmp(cmd, "matchrunsnip") == 0 ||
      strcmp(cmd, "runsnipnth") == 0 || strcmp(cmd, "nthrunsnip") == 0 ||
      strcmp(cmd, "runsniplastmatch") == 0 || strcmp(cmd, "lastmatchrunsnip") == 0) {
    /* Usability: execute curated SNIP mini without temp files (dual of RUNSNIP).
     *   cubalc runsnip · cubalc runsnip match p · cubalc runsnip nth 1 p
     *   cubalc runsnipmatch p · cubalc runsnipnth 1 p · cubalc runsniplastmatch p
     * Schema cubalc.runsnip.v1 · nested run plate fields + topic. */
    static const char *topic_ids[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    char src[640];
    cubalc_run_result rr;
    const char *topic = "general";
    const char *needle = NULL, *fallback = NULL, *mpick = NULL;
    char tup[32], nlow[64], matches[8][16];
    size_t k;
    int ok, i, m_idx = 0, m_n = 0, mode_filt = 0, ai = 2;
    int n_topic_ids = (int)(sizeof topic_ids / sizeof topic_ids[0]);
    nlow[0] = 0;
    if (strcmp(cmd, "runsnipmatch") == 0 || strcmp(cmd, "matchrunsnip") == 0)
      mode_filt = 1;
    else if (strcmp(cmd, "runsnipnth") == 0 || strcmp(cmd, "nthrunsnip") == 0)
      mode_filt = 2;
    else if (strcmp(cmd, "runsniplastmatch") == 0 || strcmp(cmd, "lastmatchrunsnip") == 0)
      mode_filt = 3;
    if (mode_filt == 0 && argc > 2 && argv[2] && argv[2][0]) {
      if (!strcmp(argv[2], "match") || !strcmp(argv[2], "first") ||
          !strcmp(argv[2], "pick")) {
        mode_filt = 1;
        ai = 3;
      } else if (!strcmp(argv[2], "nth") || !strcmp(argv[2], "index")) {
        mode_filt = 2;
        ai = 3;
      } else if (!strcmp(argv[2], "lastmatch") || !strcmp(argv[2], "endmatch") ||
                 !strcmp(argv[2], "matchlast")) {
        mode_filt = 3;
        ai = 3;
      } else {
        topic = argv[2];
      }
    }
    if (mode_filt != 0) {
      if (mode_filt == 2) {
        if (argc <= ai || !argv[ai] || !argv[ai][0]) {
          fprintf(stderr, "usage: cubalc runsnip nth <idx> <needle> [OR fallback]\n");
          printf("{\"schema\":\"cubalc.runsnip.v1\",\"ok\":false,\"cmd\":\"runsnipnth\","
                 "\"err\":\"need idx and needle\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 2;
        }
        m_idx = atoi(argv[ai]);
        ai++;
      }
      if (argc <= ai || !argv[ai] || !argv[ai][0]) {
        fprintf(stderr, "usage: cubalc runsnip match|lastmatch <needle> [OR fallback]\n");
        printf("{\"schema\":\"cubalc.runsnip.v1\",\"ok\":false,\"cmd\":\"runsnipmatch\","
               "\"err\":\"need needle\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai];
      ai++;
      if (argc > ai + 1 && argv[ai] &&
          (!strcmp(argv[ai], "OR") || !strcmp(argv[ai], "or") ||
           !strcmp(argv[ai], "ELSE") || !strcmp(argv[ai], "DEFAULT")) &&
          argv[ai + 1] && argv[ai + 1][0])
        fallback = argv[ai + 1];
      for (k = 0; needle[k] && k + 1 < sizeof nlow; k++)
        nlow[k] = (char)((needle[k] >= 'A' && needle[k] <= 'Z')
                             ? needle[k] - 'A' + 'a' : needle[k]);
      nlow[k] = 0;
      if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
        snprintf(nlow, sizeof nlow, "%s", "cap");
      if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
        snprintf(nlow, sizeof nlow, "%s", "p2p");
      if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
        snprintf(nlow, sizeof nlow, "%s", "fat");
      if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
        snprintf(nlow, sizeof nlow, "%s", "plate");
      if (!strcmp(nlow, "start") || !strcmp(nlow, "help") || !strcmp(nlow, "all") ||
          !strcmp(nlow, "default") || !strcmp(nlow, "*"))
        snprintf(nlow, sizeof nlow, "%s", "general");
      m_n = 0;
      for (i = 0; i < n_topic_ids && m_n < 8; i++) {
        if (!strstr(topic_ids[i], nlow)) continue;
        snprintf(matches[m_n], sizeof matches[0], "%s", topic_ids[i]);
        m_n++;
      }
      if (mode_filt == 3) {
        if (m_n > 0) { mpick = matches[m_n - 1]; m_idx = m_n - 1; }
      } else if (mode_filt == 1) {
        if (m_n > 0) { mpick = matches[0]; m_idx = 0; }
      } else {
        if (m_idx < 0) m_idx = 0;
        if (m_idx < m_n) mpick = matches[m_idx];
      }
      if (!mpick) {
        if (fallback && fallback[0]) {
          topic = fallback;
        } else {
          printf("{\"schema\":\"cubalc.runsnip.v1\",\"ok\":false,\"cmd\":\"runsnipmatch\","
                 "\"filter\":\"%s\",\"idx\":%d,\"match_n\":%d,\"err\":\"no match\","
                 "\"version\":\"%s\"}\n",
                 nlow, m_idx, m_n, CUBALC_LANG_VERSION);
          return 1;
        }
      } else {
        topic = mpick;
      }
    }
    for (k = 0; topic[k] && k + 1 < sizeof tup; k++)
      tup[k] = (char)((topic[k] >= 'A' && topic[k] <= 'Z')
                          ? topic[k] - 'A' + 'a' : topic[k]);
    tup[k] = 0;
    if (!strcmp(tup, "capability") || !strcmp(tup, "forms") || !strcmp(tup, "form"))
      snprintf(tup, sizeof tup, "%s", "cap");
    if (!strcmp(tup, "mesh") || !strcmp(tup, "smx") || !strcmp(tup, "peer"))
      snprintf(tup, sizeof tup, "%s", "p2p");
    if (!strcmp(tup, "nest") || !strcmp(tup, "var") || !strcmp(tup, "timeout"))
      snprintf(tup, sizeof tup, "%s", "fat");
    if (!strcmp(tup, "json") || !strcmp(tup, "agent"))
      snprintf(tup, sizeof tup, "%s", "plate");
    if (!strcmp(tup, "start") || !strcmp(tup, "help") || !strcmp(tup, "all") ||
        !strcmp(tup, "default") || !strcmp(tup, "*"))
      snprintf(tup, sizeof tup, "%s", "general");
    /* Prefer in-lang RUNSNIP so catalog stays single-source with language. */
    if (strchr(tup, '"') || strchr(tup, ' ') || strchr(tup, '\n')) {
      printf("{\"schema\":\"cubalc.runsnip.v1\",\"ok\":false,\"cmd\":\"runsnip\","
             "\"err\":\"bad topic\",\"version\":\"%s\"}\n", CUBALC_LANG_VERSION);
      return 2;
    }
    snprintf(src, sizeof src,
             "RUNSNIP %s\n"
             "PRINT LAST\n"
             "IF OK == 0 THEN\n"
             "  FAIL \"runsnip nested\"\n"
             "END\n"
             "PASS\n",
             tup);
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<cli-runsnip>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    {
      char lastj[400];
      size_t i2, o = 0;
      const char *h = rr.last_print[0] ? rr.last_print : (rr.last_err[0] ? rr.last_err : "");
      const char *cmd_out = mode_filt == 1 ? "runsnipmatch" :
                            mode_filt == 2 ? "runsnipnth" :
                            mode_filt == 3 ? "runsniplastmatch" : "runsnip";
      for (i2 = 0; h[i2] && o + 2 < sizeof lastj; i2++) {
        char c = h[i2];
        if (c == '"' || c == '\\') { lastj[o++] = '\\'; lastj[o++] = c; }
        else if (c == '\n') { lastj[o++] = '\\'; lastj[o++] = 'n'; }
        else if ((unsigned char)c < 32) lastj[o++] = ' ';
        else lastj[o++] = c;
      }
      lastj[o] = 0;
      printf("{\"schema\":\"cubalc.runsnip.v1\",\"ok\":%s,\"cmd\":\"%s\","
             "\"topic\":\"%s\",\"asserts_ok\":%d,\"asserts_fail\":%d,\"stmts\":%d,"
             "\"filter\":\"%s\",\"match_i\":%d,\"match_n\":%d,\"filter_mode\":%d,"
             "\"last\":\"%s\",\"version\":\"%s\","
             "\"note\":\"execute curated SNIP · MATCH|NTH|LASTMATCH filter pick · dual of RUNSNIP\","
             "\"topics\":[\"general\",\"cap\",\"fat\",\"plate\",\"p2p\",\"run\",\"lib\",\"protect\"]}\n",
             ok ? "true" : "false", cmd_out, tup, rr.asserts_ok, rr.asserts_fail, rr.stmts,
             mode_filt ? nlow : "", m_idx, mode_filt ? m_n : 0, mode_filt,
             lastj, CUBALC_LANG_VERSION);
    }
    return ok ? 0 : 1;
  }
  if (strcmp(cmd, "discover") == 0 || strcmp(cmd, "explore") == 0 ||
      strcmp(cmd, "discovertopics") == 0 || strcmp(cmd, "scout") == 0 ||
      strcmp(cmd, "opendiscover") == 0) {
    /* Usability: match bag + first-topic open plate (dual of DISCOVER).
     *   cubalc discover · cubalc discover p · cubalc explore cap
     * Schema cubalc.discover.v1 · agents skip MATCHTOPICS+TOPICHINT+GUIDE glue. */
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    static const struct { const char *id; const char *hint; } hints[] = {
      {"general", "install/doctor/init surface · VERSION STATUS IDENTITY"},
      {"cap", "HASFORM/NEEDFORMS capability floor · FORMHINT · run -C"},
      {"fat", "VARROOM/REMAIN_MS fat nest budget · fat_session · run -T"},
      {"plate", "SETP/NEEDP plate agent JSON · plate_session · PRETTYP"},
      {"p2p", "SMX SERVE/DIAL mesh · CUBALC_SMX_KEY · P2P_SOFT/TIMEOUT"},
      {"run", "ASSERT/EXPECT/WHY run probes · ERRTIPS · CLEAR_ERR"},
      {"lib", "LISTLIBS/RECIPE INCLUDE discovery · MATCHLIBS · checkdeps"},
      {"protect", "HOLD_FLASH/protect status · CORE_PROTECT · device/mesh-join only"},
    };
    static const struct { const char *topic; const char *rel; } rels[] = {
      {"general", "cap"}, {"general", "lib"}, {"general", "run"}, {"general", "plate"},
      {"cap", "general"}, {"cap", "run"}, {"cap", "lib"}, {"cap", "fat"},
      {"fat", "plate"}, {"fat", "run"}, {"fat", "cap"}, {"fat", "general"},
      {"plate", "fat"}, {"plate", "run"}, {"plate", "general"}, {"plate", "lib"},
      {"p2p", "protect"}, {"p2p", "run"}, {"p2p", "general"}, {"p2p", "lib"},
      {"run", "general"}, {"run", "cap"}, {"run", "plate"}, {"run", "fat"},
      {"lib", "general"}, {"lib", "cap"}, {"lib", "plate"}, {"lib", "run"},
      {"protect", "p2p"}, {"protect", "general"}, {"protect", "run"}, {"protect", "lib"},
    };
    const char *needle = (argc > 2 && argv[2] && argv[2][0]) ? argv[2] : "";
    char nlow[64], matches[8][16], topics_j[256], rel_j[256], next_j[900], hj[320];
    const char *first = NULL, *hint = NULL;
    size_t k, o, j;
    int i, n = 0, nr = 0, nn = 0;
    int nall = (int)(sizeof topics / sizeof topics[0]);
    int n_hints = (int)(sizeof hints / sizeof hints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    nlow[0] = 0;
    if (needle[0]) {
      for (k = 0; needle[k] && k + 1 < sizeof nlow; k++)
        nlow[k] = (char)((needle[k] >= 'A' && needle[k] <= 'Z')
                             ? needle[k] - 'A' + 'a' : needle[k]);
      nlow[k] = 0;
      if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
        snprintf(nlow, sizeof nlow, "%s", "cap");
      if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
        snprintf(nlow, sizeof nlow, "%s", "p2p");
      if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
        snprintf(nlow, sizeof nlow, "%s", "fat");
      if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
        snprintf(nlow, sizeof nlow, "%s", "plate");
      if (!strcmp(nlow, "start") || !strcmp(nlow, "help") || !strcmp(nlow, "all") ||
          !strcmp(nlow, "default") || !strcmp(nlow, "*"))
        snprintf(nlow, sizeof nlow, "%s", "general");
      for (i = 0; i < nall && n < 8; i++) {
        if (!strstr(topics[i], nlow)) continue;
        snprintf(matches[n], sizeof matches[0], "%s", topics[i]);
        n++;
      }
    } else {
      for (i = 0; i < nall && n < 8; i++) {
        snprintf(matches[n], sizeof matches[0], "%s", topics[i]);
        n++;
      }
    }
    if (n == 0) {
      printf("{\"schema\":\"cubalc.discover.v1\",\"ok\":false,\"cmd\":\"discover\","
             "\"filter\":\"%s\",\"n\":0,\"err\":\"no match\",\"version\":\"%s\"}\n",
             nlow[0] ? nlow : "*", CUBALC_LANG_VERSION);
      return 1;
    }
    first = matches[0];
    for (i = 0; i < n_hints; i++)
      if (!strcmp(hints[i].id, first)) { hint = hints[i].hint; break; }
    if (!hint) hint = "";
    o = 0; topics_j[0] = 0;
    for (i = 0; i < n; i++) {
      if (o && o + 1 < sizeof topics_j) topics_j[o++] = ',';
      o += (size_t)snprintf(topics_j + o, sizeof topics_j - o, "\"%s\"", matches[i]);
    }
    o = 0; rel_j[0] = 0;
    for (i = 0; i < n_rels; i++) {
      if (strcmp(rels[i].topic, first) != 0) continue;
      if (o && o + 1 < sizeof rel_j) rel_j[o++] = ',';
      o += (size_t)snprintf(rel_j + o, sizeof rel_j - o, "\"%s\"", rels[i].rel);
      nr++;
    }
    o = 0; next_j[0] = 0;
    {
      char cmds[6][80];
      int ci, nc = 6;
      snprintf(cmds[0], sizeof cmds[0], "GUIDE %s", first);
      snprintf(cmds[1], sizeof cmds[1], "RUNSNIP %s", first);
      snprintf(cmds[2], sizeof cmds[2], "TOPIC %s", first);
      snprintf(cmds[3], sizeof cmds[3], "cubalc guide %s", first);
      if (nlow[0]) {
        snprintf(cmds[4], sizeof cmds[4], "GUIDE MATCH %s", nlow);
        snprintf(cmds[5], sizeof cmds[5], "RUNSNIP MATCH %s", nlow);
      } else {
        snprintf(cmds[4], sizeof cmds[4], "cubalc discover");
        snprintf(cmds[5], sizeof cmds[5], "LISTTOPICS");
      }
      for (ci = 0; ci < nc; ci++) {
        char esc[100];
        size_t e = 0;
        for (j = 0; cmds[ci][j] && e + 2 < sizeof esc; j++) {
          char c = cmds[ci][j];
          if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
          else esc[e++] = c;
        }
        esc[e] = 0;
        if (o && o + 1 < sizeof next_j) next_j[o++] = ',';
        if (o + e + 3 < sizeof next_j) {
          next_j[o++] = '"';
          memcpy(next_j + o, esc, e); o += e;
          next_j[o++] = '"'; next_j[o] = 0;
        }
        nn++;
      }
    }
    o = 0;
    for (j = 0; hint[j] && o + 2 < sizeof hj; j++) {
      char c = hint[j];
      if (c == '"' || c == '\\') { hj[o++] = '\\'; hj[o++] = c; }
      else hj[o++] = c;
    }
    hj[o] = 0;
    printf("{\"schema\":\"cubalc.discover.v1\",\"ok\":true,\"cmd\":\"discover\","
           "\"filter\":\"%s\",\"n\":%d,\"first\":\"%s\",\"hint\":\"%s\","
           "\"related_n\":%d,\"next_n\":%d,\"version\":\"%s\","
           "\"note\":\"match bag + first open · dual of DISCOVER · chain guide/runsnip\","
           "\"topics\":[%s],\"related\":[%s],\"next\":[%s]}\n",
           nlow[0] ? nlow : "*", n, first, hj, nr, nn, CUBALC_LANG_VERSION,
           topics_j, rel_j, next_j);
    return 0;
  }
  if (strcmp(cmd, "open") == 0 || strcmp(cmd, "openplay") == 0 ||
      strcmp(cmd, "openguide") == 0 || strcmp(cmd, "playopen") == 0 ||
      strcmp(cmd, "discoverguide") == 0) {
    /* Usability: DISCOVER bag + GUIDE playbook for first (dual of OPEN).
     *   cubalc open · cubalc open p · cubalc openplay cap
     * Schema cubalc.open.v1 */
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    static const struct { const char *id; const char *hint; } hints[] = {
      {"general", "install/doctor/init surface · VERSION STATUS IDENTITY"},
      {"cap", "HASFORM/NEEDFORMS capability floor · FORMHINT · run -C"},
      {"fat", "VARROOM/REMAIN_MS fat nest budget · fat_session · run -T"},
      {"plate", "SETP/NEEDP plate agent JSON · plate_session · PRETTYP"},
      {"p2p", "SMX SERVE/DIAL mesh · CUBALC_SMX_KEY · P2P_SOFT/TIMEOUT"},
      {"run", "ASSERT/EXPECT/WHY run probes · ERRTIPS · CLEAR_ERR"},
      {"lib", "LISTLIBS/RECIPE INCLUDE discovery · MATCHLIBS · checkdeps"},
      {"protect", "HOLD_FLASH/protect status · CORE_PROTECT · device/mesh-join only"},
    };
    static const struct { const char *topic; const char *rel; } rels[] = {
      {"general", "cap"}, {"general", "lib"}, {"general", "run"}, {"general", "plate"},
      {"cap", "general"}, {"cap", "run"}, {"cap", "lib"}, {"cap", "fat"},
      {"fat", "plate"}, {"fat", "run"}, {"fat", "cap"}, {"fat", "general"},
      {"plate", "fat"}, {"plate", "run"}, {"plate", "general"}, {"plate", "lib"},
      {"p2p", "protect"}, {"p2p", "run"}, {"p2p", "general"}, {"p2p", "lib"},
      {"run", "general"}, {"run", "cap"}, {"run", "plate"}, {"run", "fat"},
      {"lib", "general"}, {"lib", "cap"}, {"lib", "plate"}, {"lib", "run"},
      {"protect", "p2p"}, {"protect", "general"}, {"protect", "run"}, {"protect", "lib"},
    };
    static const struct { const char *topic; const char *tip; } tips[] = {
      {"general", "cubalc doctor"}, {"general", "INCLUDE discover_boot"},
      {"general", "OPEN general playbook"}, {"cap", "INCLUDE cap_boot"},
      {"cap", "HASFORM · NEEDFORMS"}, {"fat", "INCLUDE fat_session"},
      {"plate", "INCLUDE plate_session"}, {"p2p", "CUBALC_SMX_KEY · SERVE/DIAL"},
      {"run", "WHY after ASSERT"}, {"lib", "cubalc libs|recipe"},
      {"protect", "cubalc protect status"},
    };
    static const struct { const char *topic; const char *form; } forms[] = {
      {"general", "OPEN"}, {"general", "DISCOVER"}, {"general", "GUIDE"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORMS"},
      {"fat", "VARROOM"}, {"plate", "SETP"}, {"plate", "NEEDP"},
      {"p2p", "SMX"}, {"run", "ASSERT"}, {"lib", "INCLUDE"}, {"protect", "HOLD_FLASH"},
    };
    const char *needle = (argc > 2 && argv[2] && argv[2][0]) ? argv[2] : "";
    char nlow[64], matches[8][16], topics_j[256], rel_j[256], tips_j[2400], forms_j[1200], hj[320];
    const char *first = NULL, *hint = NULL;
    size_t k, o, j;
    int i, n = 0, nr = 0, nt = 0, nf = 0;
    int nall = (int)(sizeof topics / sizeof topics[0]);
    int n_hints = (int)(sizeof hints / sizeof hints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    int n_tips = (int)(sizeof tips / sizeof tips[0]);
    int n_forms = (int)(sizeof forms / sizeof forms[0]);
    nlow[0] = 0;
    if (needle[0]) {
      for (k = 0; needle[k] && k + 1 < sizeof nlow; k++)
        nlow[k] = (char)((needle[k] >= 'A' && needle[k] <= 'Z')
                             ? needle[k] - 'A' + 'a' : needle[k]);
      nlow[k] = 0;
      if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
        snprintf(nlow, sizeof nlow, "%s", "cap");
      if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
        snprintf(nlow, sizeof nlow, "%s", "p2p");
      if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
        snprintf(nlow, sizeof nlow, "%s", "fat");
      if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
        snprintf(nlow, sizeof nlow, "%s", "plate");
      if (!strcmp(nlow, "start") || !strcmp(nlow, "help") || !strcmp(nlow, "all") ||
          !strcmp(nlow, "default") || !strcmp(nlow, "*"))
        snprintf(nlow, sizeof nlow, "%s", "general");
      for (i = 0; i < nall && n < 8; i++) {
        if (!strstr(topics[i], nlow)) continue;
        snprintf(matches[n], sizeof matches[0], "%s", topics[i]); n++;
      }
    } else {
      for (i = 0; i < nall && n < 8; i++) {
        snprintf(matches[n], sizeof matches[0], "%s", topics[i]); n++;
      }
    }
    if (n == 0) {
      printf("{\"schema\":\"cubalc.open.v1\",\"ok\":false,\"cmd\":\"open\","
             "\"filter\":\"%s\",\"n\":0,\"err\":\"no match\",\"version\":\"%s\"}\n",
             nlow[0] ? nlow : "*", CUBALC_LANG_VERSION);
      return 1;
    }
    first = matches[0];
    for (i = 0; i < n_hints; i++)
      if (!strcmp(hints[i].id, first)) { hint = hints[i].hint; break; }
    if (!hint) hint = "";
    o = 0; topics_j[0] = 0;
    for (i = 0; i < n; i++) {
      if (o && o + 1 < sizeof topics_j) topics_j[o++] = ',';
      o += (size_t)snprintf(topics_j + o, sizeof topics_j - o, "\"%s\"", matches[i]);
    }
    o = 0; rel_j[0] = 0;
    for (i = 0; i < n_rels; i++) {
      if (strcmp(rels[i].topic, first) != 0) continue;
      if (o && o + 1 < sizeof rel_j) rel_j[o++] = ',';
      o += (size_t)snprintf(rel_j + o, sizeof rel_j - o, "\"%s\"", rels[i].rel);
      nr++;
    }
    o = 0; tips_j[0] = 0;
    for (i = 0; i < n_tips; i++) {
      char esc[200]; size_t e = 0;
      if (strcmp(tips[i].topic, first) != 0) continue;
      for (j = 0; tips[i].tip[j] && e + 2 < sizeof esc; j++) {
        char c = tips[i].tip[j];
        if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
        else esc[e++] = c;
      }
      esc[e] = 0;
      if (o && o + 1 < sizeof tips_j) tips_j[o++] = ',';
      if (o + e + 3 < sizeof tips_j) {
        tips_j[o++] = '"'; memcpy(tips_j + o, esc, e); o += e;
        tips_j[o++] = '"'; tips_j[o] = 0;
      }
      nt++;
    }
    o = 0; forms_j[0] = 0;
    for (i = 0; i < n_forms; i++) {
      size_t ln;
      if (strcmp(forms[i].topic, first) != 0) continue;
      ln = strlen(forms[i].form);
      if (o && o + 1 < sizeof forms_j) forms_j[o++] = ',';
      if (o + ln + 3 < sizeof forms_j) {
        forms_j[o++] = '"'; memcpy(forms_j + o, forms[i].form, ln); o += ln;
        forms_j[o++] = '"'; forms_j[o] = 0;
      }
      nf++;
    }
    o = 0;
    for (j = 0; hint[j] && o + 2 < sizeof hj; j++) {
      char c = hint[j];
      if (c == '"' || c == '\\') { hj[o++] = '\\'; hj[o++] = c; }
      else hj[o++] = c;
    }
    hj[o] = 0;
    printf("{\"schema\":\"cubalc.open.v1\",\"ok\":true,\"cmd\":\"open\","
           "\"filter\":\"%s\",\"n\":%d,\"first\":\"%s\",\"hint\":\"%s\","
           "\"related_n\":%d,\"tips_n\":%d,\"forms_n\":%d,\"version\":\"%s\","
           "\"note\":\"DISCOVER bag + GUIDE first · dual of OPEN · chain runsnip\","
           "\"topics\":[%s],\"related\":[%s],\"tips\":[%s],\"forms\":[%s]}\n",
           nlow[0] ? nlow : "*", n, first, hj, nr, nt, nf, CUBALC_LANG_VERSION,
           topics_j, rel_j, tips_j, forms_j);
    return 0;
  }
  if (strcmp(cmd, "topics") == 0 || strcmp(cmd, "listtopics") == 0 ||
      strcmp(cmd, "topiclist") == 0 || strcmp(cmd, "discovery-topics") == 0 ||
      strcmp(cmd, "hastopic") == 0 || strcmp(cmd, "needtopic") == 0) {
    /* Usability: discovery topic catalog (dual of LISTTOPICS/HASTOPIC).
     *   cubalc topics · cubalc hastopic cap · cubalc needtopic plate
     * Schema cubalc.topics.v1 · agents list topics without hardcoding. */
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    static const char *hints[] = {
      "install/doctor/init surface",
      "HASFORM/NEEDFORMS capability floor",
      "VARROOM/REMAIN_MS fat nest budget",
      "SETP/NEEDP plate agent JSON",
      "SMX SERVE/DIAL mesh",
      "ASSERT/EXPECT/WHY run probes",
      "LISTLIBS/RECIPE INCLUDE discovery",
      "HOLD_FLASH/protect status"
    };
    int nall = (int)(sizeof topics / sizeof topics[0]);
    int i, is_has = (strcmp(cmd, "hastopic") == 0);
    int is_need = (strcmp(cmd, "needtopic") == 0);
    if (is_has || is_need) {
      const char *name = (argc > 2 && argv[2][0]) ? argv[2] : "";
      char tup[32];
      size_t k;
      int hit = 0;
      if (!name[0]) {
        fprintf(stderr, "usage: cubalc hastopic|needtopic <topic>\n");
        printf("{\"schema\":\"cubalc.topics.v1\",\"ok\":false,\"cmd\":\"%s\","
               "\"err\":\"need topic name\",\"version\":\"%s\"}\n",
               cmd, CUBALC_LANG_VERSION);
        return 2;
      }
      for (k = 0; name[k] && k + 1 < sizeof tup; k++)
        tup[k] = (char)((name[k] >= 'A' && name[k] <= 'Z')
                            ? name[k] - 'A' + 'a' : name[k]);
      tup[k] = 0;
      if (!strcmp(tup, "capability") || !strcmp(tup, "forms") || !strcmp(tup, "form"))
        snprintf(tup, sizeof tup, "%s", "cap");
      if (!strcmp(tup, "mesh") || !strcmp(tup, "smx") || !strcmp(tup, "peer"))
        snprintf(tup, sizeof tup, "%s", "p2p");
      if (!strcmp(tup, "nest") || !strcmp(tup, "var") || !strcmp(tup, "timeout"))
        snprintf(tup, sizeof tup, "%s", "fat");
      if (!strcmp(tup, "json") || !strcmp(tup, "agent"))
        snprintf(tup, sizeof tup, "%s", "plate");
      if (!strcmp(tup, "start") || !strcmp(tup, "all") || !strcmp(tup, "help") ||
          !strcmp(tup, "default") || !strcmp(tup, "*"))
        snprintf(tup, sizeof tup, "%s", "general");
      for (i = 0; i < nall; i++)
        if (!strcmp(topics[i], tup)) { hit = 1; break; }
      printf("{\"schema\":\"cubalc.topics.v1\",\"ok\":%s,\"cmd\":\"%s\","
             "\"topic\":\"%s\",\"known\":%s,\"mode\":\"%s\",\"version\":\"%s\","
             "\"note\":\"CLI dual of HASTOPIC/NEEDTOPIC · cubalc topics catalog\"}\n",
             hit ? "true" : "false", cmd, tup, hit ? "true" : "false",
             is_need ? "need" : "has", CUBALC_LANG_VERSION);
      if (is_need) return hit ? 0 : 1;
      return 0;
    }
    printf("{\"schema\":\"cubalc.topics.v1\",\"ok\":true,\"cmd\":\"topics\","
           "\"n\":%d,\"version\":\"%s\","
           "\"note\":\"discovery topic catalog · dual of LISTTOPICS · use with topic/tips/snip/runsnip\","
           "\"topics\":[",
           nall, CUBALC_LANG_VERSION);
    for (i = 0; i < nall; i++) {
      printf("%s{\"id\":\"%s\",\"hint\":\"%s\"}",
             i ? "," : "", topics[i], hints[i]);
    }
    printf("]}\n");
    return 0;
  }

  if (strcmp(cmd, "matchtopics") == 0 || strcmp(cmd, "filtertopics") == 0 ||
      strcmp(cmd, "topicmatch") == 0 || strcmp(cmd, "greptopics") == 0 ||
      strcmp(cmd, "picktopic") == 0 || strcmp(cmd, "firsttopic") == 0 ||
      strcmp(cmd, "topicpick") == 0) {
    /* Usability: filter/first discovery topic (dual of MATCHTOPICS/PICKTOPIC).
     * Schema cubalc.topicmatch.v1 */
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    const char *needle = (argc > 2 && argv[2][0]) ? argv[2] : "";
    const char *fallback = NULL;
    char nlow[64], bagj[256];
    size_t k, o = 0;
    int i, n = 0, nall = (int)(sizeof topics / sizeof topics[0]);
    int is_pick = (strcmp(cmd, "picktopic") == 0 || strcmp(cmd, "firsttopic") == 0 ||
                   strcmp(cmd, "topicpick") == 0);
    int first = 1;
    const char *pick = NULL;
    if (!needle[0]) {
      fprintf(stderr, "usage: cubalc matchtopics|picktopic <needle> [OR fallback]\n");
      printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need needle\",\"version\":\"%s\"}\n",
             is_pick ? "picktopic" : "matchtopics", CUBALC_LANG_VERSION);
      return 2;
    }
    if (is_pick && argc > 3 &&
        (!strcmp(argv[3], "OR") || !strcmp(argv[3], "or") ||
         !strcmp(argv[3], "ELSE") || !strcmp(argv[3], "DEFAULT")) &&
        argc > 4 && argv[4][0])
      fallback = argv[4];
    for (k = 0; needle[k] && k + 1 < sizeof nlow; k++)
      nlow[k] = (char)((needle[k] >= 'A' && needle[k] <= 'Z')
                           ? needle[k] - 'A' + 'a' : needle[k]);
    nlow[k] = 0;
    if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
      snprintf(nlow, sizeof nlow, "%s", "cap");
    if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
      snprintf(nlow, sizeof nlow, "%s", "p2p");
    if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
      snprintf(nlow, sizeof nlow, "%s", "fat");
    if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
      snprintf(nlow, sizeof nlow, "%s", "plate");
    if (!strcmp(nlow, "start") || !strcmp(nlow, "all") || !strcmp(nlow, "help") ||
        !strcmp(nlow, "default") || !strcmp(nlow, "*"))
      snprintf(nlow, sizeof nlow, "%s", "general");
    bagj[0] = 0;
    for (i = 0; i < nall; i++) {
      if (!strstr(topics[i], nlow)) continue;
      if (!pick) pick = topics[i];
      if (!first && o + 1 < sizeof bagj) bagj[o++] = ',';
      o += (size_t)snprintf(bagj + o, sizeof bagj - o, "\"%s\"", topics[i]);
      first = 0;
      n++;
    }
    if (is_pick) {
      if (!pick) {
        if (fallback && fallback[0]) {
          printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":true,\"cmd\":\"picktopic\","
                 "\"filter\":\"%s\",\"topic\":\"%s\",\"fallback\":true,\"n\":1,"
                 "\"version\":\"%s\"}\n",
                 nlow, fallback, CUBALC_LANG_VERSION);
          return 0;
        }
        printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"picktopic\","
               "\"filter\":\"%s\",\"err\":\"no match\",\"version\":\"%s\"}\n",
               nlow, CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":true,\"cmd\":\"picktopic\","
             "\"filter\":\"%s\",\"topic\":\"%s\",\"n\":1,\"version\":\"%s\"}\n",
             nlow, pick, CUBALC_LANG_VERSION);
      return 0;
    }
    if (n == 0) {
      printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"matchtopics\","
             "\"filter\":\"%s\",\"n\":0,\"err\":\"empty\",\"version\":\"%s\"}\n",
             nlow, CUBALC_LANG_VERSION);
      return 1;
    }
    printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":true,\"cmd\":\"matchtopics\","
           "\"filter\":\"%s\",\"n\":%d,\"version\":\"%s\",\"topics\":[%s]}\n",
           nlow, n, CUBALC_LANG_VERSION, bagj);
    return 0;
  }
  if (strcmp(cmd, "hasmatchtopics") == 0 || strcmp(cmd, "hastopicmatch") == 0 ||
      strcmp(cmd, "needmatchtopics") == 0 || strcmp(cmd, "needtopicmatch") == 0 ||
      strcmp(cmd, "countmatchtopics") == 0 || strcmp(cmd, "counttopics") == 0 ||
      strcmp(cmd, "ntopicmatch") == 0) {
    /* Usability: topic filter gates (dual of HASMATCHTOPICS/NEED/COUNT).
     * Schema cubalc.topicmatch.v1 · mode has|need|count */
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    const char *needle = (argc > 2 && argv[2][0]) ? argv[2] : "";
    char nlow[64];
    size_t k;
    int i, n = 0, nall = (int)(sizeof topics / sizeof topics[0]);
    int mode_need = (strcmp(cmd, "needmatchtopics") == 0 ||
                     strcmp(cmd, "needtopicmatch") == 0);
    int mode_count = (strcmp(cmd, "countmatchtopics") == 0 ||
                      strcmp(cmd, "counttopics") == 0 ||
                      strcmp(cmd, "ntopicmatch") == 0);
    if (!needle[0]) {
      fprintf(stderr,
              "usage: cubalc hasmatchtopics|needmatchtopics|countmatchtopics <needle>\n");
      printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need needle\",\"version\":\"%s\"}\n",
             cmd, CUBALC_LANG_VERSION);
      return 2;
    }
    for (k = 0; needle[k] && k + 1 < sizeof nlow; k++)
      nlow[k] = (char)((needle[k] >= 'A' && needle[k] <= 'Z')
                           ? needle[k] - 'A' + 'a' : needle[k]);
    nlow[k] = 0;
    if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
      snprintf(nlow, sizeof nlow, "%s", "cap");
    if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
      snprintf(nlow, sizeof nlow, "%s", "p2p");
    if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
      snprintf(nlow, sizeof nlow, "%s", "fat");
    if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
      snprintf(nlow, sizeof nlow, "%s", "plate");
    if (!strcmp(nlow, "start") || !strcmp(nlow, "all") || !strcmp(nlow, "help") ||
        !strcmp(nlow, "default") || !strcmp(nlow, "*"))
      snprintf(nlow, sizeof nlow, "%s", "general");
    for (i = 0; i < nall; i++)
      if (strstr(topics[i], nlow)) n++;
    if (mode_count) {
      printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":true,\"cmd\":\"countmatchtopics\","
             "\"filter\":\"%s\",\"n\":%d,\"mode\":\"count\",\"version\":\"%s\","
             "\"note\":\"COUNTMATCHTOPICS dual · count without bag\"}\n",
             nlow, n, CUBALC_LANG_VERSION);
      return 0;
    }
    printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":%s,\"cmd\":\"%s\","
           "\"filter\":\"%s\",\"n\":%d,\"hit\":%s,\"mode\":\"%s\",\"version\":\"%s\","
           "\"note\":\"HASMATCHTOPICS/NEEDMATCHTOPICS dual · topic filter gate\"}\n",
           n > 0 ? "true" : "false",
           mode_need ? "needmatchtopics" : "hasmatchtopics",
           nlow, n, n > 0 ? "true" : "false",
           mode_need ? "need" : "has", CUBALC_LANG_VERSION);
    if (mode_need) return n > 0 ? 0 : 1;
    return 0;
  }


  if (strcmp(cmd, "nthtopic") == 0 || strcmp(cmd, "indextopic") == 0 ||
      strcmp(cmd, "topicnth") == 0 || strcmp(cmd, "lasttopic") == 0 ||
      strcmp(cmd, "endtopic") == 0 || strcmp(cmd, "topiclast") == 0) {
    /* Usability: Nth/last matching topic (dual of NTHTOPIC/LASTTOPIC).
     *   cubalc nthtopic 1 p · cubalc lasttopic p
     * Schema cubalc.topicmatch.v1 */
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    int mode_last = (strcmp(cmd, "lasttopic") == 0 || strcmp(cmd, "endtopic") == 0 ||
                     strcmp(cmd, "topiclast") == 0);
    int idx = 0, ai = 2, i, n = 0, nall = (int)(sizeof topics / sizeof topics[0]);
    const char *needle = "";
    const char *fallback = NULL;
    char nlow[64];
    char matches[8][16];
    size_t k;
    const char *pick = NULL;
    if (!mode_last) {
      if (argc <= 3 || !argv[2] || !argv[2][0]) {
        fprintf(stderr, "usage: cubalc nthtopic <idx> <needle> [OR fallback]\n");
        printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"nthtopic\","
               "\"err\":\"need idx and needle\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 2;
      }
      idx = atoi(argv[2]);
      needle = argv[3] ? argv[3] : "";
      ai = 4;
    } else {
      if (argc <= 2 || !argv[2] || !argv[2][0]) {
        fprintf(stderr, "usage: cubalc lasttopic <needle> [OR fallback]\n");
        printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"lasttopic\","
               "\"err\":\"need needle\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[2];
      ai = 3;
    }
    if (argc > ai + 1 && argv[ai] &&
        (!strcmp(argv[ai], "OR") || !strcmp(argv[ai], "or") ||
         !strcmp(argv[ai], "ELSE") || !strcmp(argv[ai], "DEFAULT")) &&
        argv[ai + 1] && argv[ai + 1][0])
      fallback = argv[ai + 1];
    if (!needle[0]) {
      printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need needle\",\"version\":\"%s\"}\n",
             mode_last ? "lasttopic" : "nthtopic", CUBALC_LANG_VERSION);
      return 2;
    }
    for (k = 0; needle[k] && k + 1 < sizeof nlow; k++)
      nlow[k] = (char)((needle[k] >= 'A' && needle[k] <= 'Z')
                           ? needle[k] - 'A' + 'a' : needle[k]);
    nlow[k] = 0;
    if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
      snprintf(nlow, sizeof nlow, "%s", "cap");
    if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
      snprintf(nlow, sizeof nlow, "%s", "p2p");
    if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
      snprintf(nlow, sizeof nlow, "%s", "fat");
    if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
      snprintf(nlow, sizeof nlow, "%s", "plate");
    if (!strcmp(nlow, "start") || !strcmp(nlow, "all") || !strcmp(nlow, "help") ||
        !strcmp(nlow, "default") || !strcmp(nlow, "*"))
      snprintf(nlow, sizeof nlow, "%s", "general");
    for (i = 0; i < nall && n < 8; i++) {
      if (!strstr(topics[i], nlow)) continue;
      snprintf(matches[n], sizeof matches[0], "%s", topics[i]);
      n++;
    }
    if (mode_last) {
      if (n > 0) { pick = matches[n - 1]; idx = n - 1; }
    } else {
      if (idx < 0) idx = 0;
      if (idx < n) pick = matches[idx];
    }
    if (!pick) {
      if (fallback && fallback[0]) {
        printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":true,\"cmd\":\"%s\","
               "\"filter\":\"%s\",\"idx\":%d,\"topic\":\"%s\",\"fallback\":true,"
               "\"match_n\":%d,\"version\":\"%s\"}\n",
               mode_last ? "lasttopic" : "nthtopic",
               nlow, idx, fallback, n, CUBALC_LANG_VERSION);
        return 0;
      }
      printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"filter\":\"%s\",\"idx\":%d,\"match_n\":%d,\"err\":\"no match\","
             "\"version\":\"%s\"}\n",
             mode_last ? "lasttopic" : "nthtopic",
             nlow, idx, n, CUBALC_LANG_VERSION);
      return 1;
    }
    printf("{\"schema\":\"cubalc.topicmatch.v1\",\"ok\":true,\"cmd\":\"%s\","
           "\"filter\":\"%s\",\"idx\":%d,\"topic\":\"%s\",\"match_n\":%d,"
           "\"version\":\"%s\","
           "\"note\":\"Nth/last matching topic · dual of NTHTOPIC/LASTTOPIC\"}\n",
           mode_last ? "lasttopic" : "nthtopic",
           nlow, idx, pick, n, CUBALC_LANG_VERSION);
    return 0;
  }

  if (strcmp(cmd, "topichint") == 0 || strcmp(cmd, "describetopic") == 0 ||
      strcmp(cmd, "topicdoc") == 0 || strcmp(cmd, "topichelp") == 0 ||
      strcmp(cmd, "hinttopic") == 0 || strcmp(cmd, "topic-hint") == 0) {
    /* Usability: one-line topic hint (dual of TOPICHINT / FORMHINT for topics).
     *   cubalc topichint cap · cubalc describetopic plate
     * Schema cubalc.topichint.v1 */
    static const struct { const char *id; const char *hint; } rows[] = {
      {"general", "install/doctor/init surface · VERSION STATUS IDENTITY"},
      {"cap", "HASFORM/NEEDFORMS capability floor · FORMHINT · run -C"},
      {"fat", "VARROOM/REMAIN_MS fat nest budget · fat_session · run -T"},
      {"plate", "SETP/NEEDP plate agent JSON · plate_session · PRETTYP"},
      {"p2p", "SMX SERVE/DIAL mesh · CUBALC_SMX_KEY · P2P_SOFT/TIMEOUT"},
      {"run", "ASSERT/EXPECT/WHY run probes · ERRTIPS · CLEAR_ERR"},
      {"lib", "LISTLIBS/RECIPE INCLUDE discovery · MATCHLIBS · checkdeps"},
      {"protect", "HOLD_FLASH/protect status · CORE_PROTECT · device/mesh-join only"},
    };
    const char *name = (argc > 2 && argv[2][0]) ? argv[2] : "";
    char tup[32];
    size_t k;
    int i, nall = (int)(sizeof rows / sizeof rows[0]);
    const char *hint = NULL;
    if (!name[0]) {
      fprintf(stderr, "usage: cubalc topichint|describetopic <topic>\n");
      printf("{\"schema\":\"cubalc.topichint.v1\",\"ok\":false,\"cmd\":\"topichint\","
             "\"err\":\"need topic name\",\"version\":\"%s\"}\n",
             CUBALC_LANG_VERSION);
      return 2;
    }
    for (k = 0; name[k] && k + 1 < sizeof tup; k++)
      tup[k] = (char)((name[k] >= 'A' && name[k] <= 'Z')
                          ? name[k] - 'A' + 'a' : name[k]);
    tup[k] = 0;
    if (!strcmp(tup, "capability") || !strcmp(tup, "forms") || !strcmp(tup, "form"))
      snprintf(tup, sizeof tup, "%s", "cap");
    if (!strcmp(tup, "mesh") || !strcmp(tup, "smx") || !strcmp(tup, "peer"))
      snprintf(tup, sizeof tup, "%s", "p2p");
    if (!strcmp(tup, "nest") || !strcmp(tup, "var") || !strcmp(tup, "timeout"))
      snprintf(tup, sizeof tup, "%s", "fat");
    if (!strcmp(tup, "json") || !strcmp(tup, "agent"))
      snprintf(tup, sizeof tup, "%s", "plate");
    if (!strcmp(tup, "start") || !strcmp(tup, "all") || !strcmp(tup, "help") ||
        !strcmp(tup, "default") || !strcmp(tup, "*"))
      snprintf(tup, sizeof tup, "%s", "general");
    for (i = 0; i < nall; i++)
      if (!strcmp(rows[i].id, tup)) { hint = rows[i].hint; break; }
    if (!hint) {
      printf("{\"schema\":\"cubalc.topichint.v1\",\"ok\":false,\"cmd\":\"topichint\","
             "\"topic\":\"%s\",\"err\":\"unknown topic\",\"version\":\"%s\","
             "\"note\":\"LISTTOPICS · cubalc topics · HASTOPIC\"}\n",
             tup, CUBALC_LANG_VERSION);
      return 1;
    }
    {
      char hj[280];
      size_t j, o = 0;
      for (j = 0; hint[j] && o + 2 < sizeof hj; j++) {
        char c = hint[j];
        if (c == '"' || c == '\\') { hj[o++] = '\\'; hj[o++] = c; }
        else hj[o++] = c;
      }
      hj[o] = 0;
      printf("{\"schema\":\"cubalc.topichint.v1\",\"ok\":true,\"cmd\":\"topichint\","
             "\"topic\":\"%s\",\"hint\":\"%s\",\"version\":\"%s\","
             "\"note\":\"one-line topic docs · dual of TOPICHINT · twin of formhint\"}\n",
             tup, hj, CUBALC_LANG_VERSION);
    }
    return 0;
  }
  if (strcmp(cmd, "relatedtopic") == 0 || strcmp(cmd, "seetopics") == 0 ||
      strcmp(cmd, "relatedtopics") == 0 || strcmp(cmd, "topicrel") == 0 ||
      strcmp(cmd, "neartopics") == 0 || strcmp(cmd, "topic-neighbors") == 0) {
    /* Usability: related discovery topic bag (dual of RELATEDTOPIC / RELATED for topics).
     *   cubalc relatedtopic cap · cubalc seetopics plate
     * Schema cubalc.relatedtopic.v1 */
    static const struct { const char *topic; const char *rel; } edges[] = {
      {"general", "cap"}, {"general", "lib"}, {"general", "run"}, {"general", "plate"},
      {"cap", "general"}, {"cap", "run"}, {"cap", "lib"}, {"cap", "fat"},
      {"fat", "plate"}, {"fat", "run"}, {"fat", "cap"}, {"fat", "general"},
      {"plate", "fat"}, {"plate", "run"}, {"plate", "general"}, {"plate", "lib"},
      {"p2p", "protect"}, {"p2p", "run"}, {"p2p", "general"}, {"p2p", "lib"},
      {"run", "general"}, {"run", "cap"}, {"run", "plate"}, {"run", "fat"},
      {"lib", "general"}, {"lib", "cap"}, {"lib", "plate"}, {"lib", "run"},
      {"protect", "p2p"}, {"protect", "general"}, {"protect", "run"}, {"protect", "lib"},
    };
    const char *name = (argc > 2 && argv[2][0]) ? argv[2] : "";
    char tup[32], relj[256];
    size_t k, o = 0;
    int i, n = 0, nall = (int)(sizeof edges / sizeof edges[0]);
    int first = 1, seen = 0;
    if (!name[0]) {
      fprintf(stderr, "usage: cubalc relatedtopic|seetopics <topic>\n");
      printf("{\"schema\":\"cubalc.relatedtopic.v1\",\"ok\":false,\"cmd\":\"relatedtopic\","
             "\"err\":\"need topic name\",\"version\":\"%s\"}\n",
             CUBALC_LANG_VERSION);
      return 2;
    }
    for (k = 0; name[k] && k + 1 < sizeof tup; k++)
      tup[k] = (char)((name[k] >= 'A' && name[k] <= 'Z')
                          ? name[k] - 'A' + 'a' : name[k]);
    tup[k] = 0;
    if (!strcmp(tup, "capability") || !strcmp(tup, "forms") || !strcmp(tup, "form"))
      snprintf(tup, sizeof tup, "%s", "cap");
    if (!strcmp(tup, "mesh") || !strcmp(tup, "smx") || !strcmp(tup, "peer"))
      snprintf(tup, sizeof tup, "%s", "p2p");
    if (!strcmp(tup, "nest") || !strcmp(tup, "var") || !strcmp(tup, "timeout"))
      snprintf(tup, sizeof tup, "%s", "fat");
    if (!strcmp(tup, "json") || !strcmp(tup, "agent"))
      snprintf(tup, sizeof tup, "%s", "plate");
    if (!strcmp(tup, "start") || !strcmp(tup, "all") || !strcmp(tup, "help") ||
        !strcmp(tup, "default") || !strcmp(tup, "*"))
      snprintf(tup, sizeof tup, "%s", "general");
    relj[0] = 0;
    for (i = 0; i < nall; i++) {
      if (strcmp(edges[i].topic, tup) != 0) continue;
      seen = 1;
      if (o + 24 < sizeof relj) {
        if (!first) relj[o++] = ',';
        o += (size_t)snprintf(relj + o, sizeof relj - o, "\"%s\"", edges[i].rel);
        first = 0;
        n++;
      }
    }
    if (!seen || n == 0) {
      printf("{\"schema\":\"cubalc.relatedtopic.v1\",\"ok\":false,\"cmd\":\"relatedtopic\","
             "\"topic\":\"%s\",\"err\":\"unknown topic\",\"version\":\"%s\","
             "\"note\":\"LISTTOPICS · cubalc topics · HASTOPIC\"}\n",
             tup, CUBALC_LANG_VERSION);
      return 1;
    }
    printf("{\"schema\":\"cubalc.relatedtopic.v1\",\"ok\":true,\"cmd\":\"relatedtopic\","
           "\"topic\":\"%s\",\"n\":%d,\"version\":\"%s\","
           "\"note\":\"related topic bag · dual of RELATEDTOPIC · twin of related for topics\","
           "\"related\":[%s]}\n",
           tup, n, CUBALC_LANG_VERSION, relj);
    return 0;
  }
  if (strcmp(cmd, "formtopics") == 0 || strcmp(cmd, "topicsof") == 0 ||
      strcmp(cmd, "topicsfor") == 0 || strcmp(cmd, "form-topics") == 0 ||
      strcmp(cmd, "covertopics") == 0 || strcmp(cmd, "topics-of") == 0) {
    /* Usability: reverse FORMSFOR — form name → discovery topics covering it.
     *   cubalc formtopics HASFORM · cubalc topicsof SETP
     * Schema cubalc.formtopics.v1 · dual of FORMTOPICS / complement of formsfor. */
    static const struct { const char *topic; const char *form; } rows[] = {
      {"general", "VERSION"}, {"general", "STATUS"}, {"general", "IDENTITY"},
      {"general", "TIPS"}, {"general", "FORMSFOR"}, {"general", "LISTTOPICS"},
      {"general", "TOPICHINT"}, {"general", "RELATEDTOPIC"}, {"general", "FORMTOPICS"},
      {"general", "WHY"}, {"general", "CLEAR_ERR"}, {"general", "INCLUDE"},
      {"general", "REQUIRE VERSION"}, {"general", "VARS"}, {"general", "TOPIC"},
      {"general", "SNIP"}, {"general", "RUNSNIP"}, {"general", "ERRTIPS"},
      {"general", "ERRRUN"}, {"general", "HASTOPIC"}, {"general", "NEEDTOPIC"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORM"}, {"cap", "HASFORMS"},
      {"cap", "NEEDFORMS"}, {"cap", "FORMHINT"}, {"cap", "LISTFORMS"},
      {"cap", "COUNTFORMS"}, {"cap", "REQUIRE FORM"}, {"cap", "RELATEDTOPIC"},
      {"cap", "FORMTOPICS"}, {"cap", "TIPS"}, {"cap", "FORMSFOR"},
      {"cap", "RELATED"}, {"cap", "SNIP"}, {"cap", "TOPIC"},
      {"fat", "VARROOM"}, {"fat", "HASVARROOM"}, {"fat", "NEEDVARROOM"},
      {"fat", "REMAIN_MS"}, {"fat", "HAS_TIME"}, {"fat", "NEEDTIME"},
      {"fat", "STATUS"}, {"fat", "VARS"}, {"fat", "TIPS"},
      {"plate", "SETP"}, {"plate", "GETP"}, {"plate", "NEEDP"},
      {"plate", "DEFAULTP"}, {"plate", "INCP"}, {"plate", "DELP"},
      {"plate", "PRETTYP"}, {"plate", "DUMPP"}, {"plate", "SAVEPLATE"},
      {"plate", "LOADPLATE"},
      {"p2p", "SMX"}, {"p2p", "SERVE"}, {"p2p", "DIAL"}, {"p2p", "TALK"},
      {"p2p", "EXCHANGE"}, {"p2p", "TIPS"},
      {"run", "ASSERT"}, {"run", "EXPECT"}, {"run", "FAIL"}, {"run", "PASS"},
      {"run", "NOTE"}, {"run", "EXIT"}, {"run", "WHY"}, {"run", "CLEAR_ERR"},
      {"run", "TIPS"}, {"run", "ERRTIPS"}, {"run", "ERRRUN"},
      {"lib", "LISTLIBS"}, {"lib", "HASLIB"}, {"lib", "MATCHLIBS"},
      {"lib", "PICKLIB"}, {"lib", "RECIPE"}, {"lib", "CHECKDEPS"},
      {"lib", "SORTLIBS"}, {"lib", "FRESHLIBS"}, {"lib", "LIBAGE"},
      {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"}, {"protect", "STATUS"}, {"protect", "VERSION"},
      {"protect", "TIPS"},
    };
    const char *name = (argc > 2 && argv[2][0]) ? argv[2] : "";
    char nup[96], relj[256];
    char seen_topics[16][32];
    size_t k, o = 0;
    int i, n = 0, nall = (int)(sizeof rows / sizeof rows[0]);
    int first = 1, nseen = 0;
    if (!name[0]) {
      fprintf(stderr, "usage: cubalc formtopics|topicsof <FormName>\n");
      printf("{\"schema\":\"cubalc.formtopics.v1\",\"ok\":false,\"cmd\":\"formtopics\","
             "\"err\":\"need form name\",\"version\":\"%s\"}\n",
             CUBALC_LANG_VERSION);
      return 2;
    }
    for (k = 0; name[k] && k + 1 < sizeof nup; k++) {
      char c = name[k];
      if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
      nup[k] = c;
    }
    nup[k] = 0;
    relj[0] = 0;
    for (i = 0; i < nall; i++) {
      char fup[96];
      size_t j;
      int match, already = 0, si;
      for (j = 0; rows[i].form[j] && j + 1 < sizeof fup; j++) {
        char c = rows[i].form[j];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        fup[j] = c;
      }
      fup[j] = 0;
      match = !strcmp(fup, nup);
      if (!match) continue;
      for (si = 0; si < nseen; si++) {
        if (!strcmp(seen_topics[si], rows[i].topic)) { already = 1; break; }
      }
      if (already) continue;
      if (nseen < (int)(sizeof seen_topics / sizeof seen_topics[0])) {
        snprintf(seen_topics[nseen], sizeof seen_topics[0], "%s", rows[i].topic);
        nseen++;
      }
      if (o + 24 < sizeof relj) {
        if (!first) relj[o++] = ',';
        o += (size_t)snprintf(relj + o, sizeof relj - o, "\"%s\"", rows[i].topic);
        first = 0;
        n++;
      }
    }
    if (n == 0) {
      printf("{\"schema\":\"cubalc.formtopics.v1\",\"ok\":false,\"cmd\":\"formtopics\","
             "\"form\":\"%s\",\"err\":\"no covering topics\",\"version\":\"%s\","
             "\"note\":\"FORMHINT · FORMSFOR · LISTFORMS · cubalc formsfor\"}\n",
             nup, CUBALC_LANG_VERSION);
      return 1;
    }
    printf("{\"schema\":\"cubalc.formtopics.v1\",\"ok\":true,\"cmd\":\"formtopics\","
           "\"form\":\"%s\",\"n\":%d,\"version\":\"%s\","
           "\"note\":\"topics covering form · dual of FORMTOPICS · reverse of formsfor\","
           "\"topics\":[%s]}\n",
           nup, n, CUBALC_LANG_VERSION, relj);
    return 0;
  }
  if (strcmp(cmd, "formguide") == 0 || strcmp(cmd, "guideform") == 0 ||
      strcmp(cmd, "form-guide") == 0 || strcmp(cmd, "formcard") == 0 ||
      strcmp(cmd, "playform") == 0 || strcmp(cmd, "formboard") == 0) {
    /* Usability: FORMHINT + covering topics + first-topic GUIDE (dual of FORMGUIDE).
     *   cubalc formguide HASFORM · cubalc guideform SETP
     * Schema cubalc.formguide.v1 */
    static const struct { const char *topic; const char *form; } rows[] = {
      {"general", "VERSION"}, {"general", "STATUS"}, {"general", "IDENTITY"},
      {"general", "TIPS"}, {"general", "FORMSFOR"}, {"general", "LISTTOPICS"},
      {"general", "TOPICHINT"}, {"general", "RELATEDTOPIC"}, {"general", "FORMTOPICS"},
      {"general", "FORMGUIDE"}, {"general", "GUIDE"}, {"general", "WHY"},
      {"general", "INCLUDE"}, {"general", "TOPIC"}, {"general", "SNIP"},
      {"general", "ERRTIPS"}, {"general", "ERRGUIDE"}, {"general", "HASTOPIC"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORM"}, {"cap", "HASFORMS"},
      {"cap", "NEEDFORMS"}, {"cap", "FORMHINT"}, {"cap", "LISTFORMS"},
      {"cap", "COUNTFORMS"}, {"cap", "REQUIRE FORM"}, {"cap", "RELATEDTOPIC"},
      {"cap", "FORMTOPICS"}, {"cap", "FORMGUIDE"}, {"cap", "TIPS"},
      {"cap", "FORMSFOR"}, {"cap", "RELATED"}, {"cap", "GUIDE"}, {"cap", "TOPIC"},
      {"fat", "VARROOM"}, {"fat", "HASVARROOM"}, {"fat", "NEEDVARROOM"},
      {"fat", "REMAIN_MS"}, {"fat", "HAS_TIME"}, {"fat", "NEEDTIME"},
      {"fat", "STATUS"}, {"fat", "GUIDE"},
      {"plate", "SETP"}, {"plate", "GETP"}, {"plate", "NEEDP"},
      {"plate", "DEFAULTP"}, {"plate", "PRETTYP"}, {"plate", "SAVEPLATE"},
      {"plate", "LOADPLATE"},
      {"p2p", "SMX"}, {"p2p", "SERVE"}, {"p2p", "DIAL"}, {"p2p", "TALK"},
      {"p2p", "EXCHANGE"},
      {"run", "ASSERT"}, {"run", "EXPECT"}, {"run", "FAIL"}, {"run", "PASS"},
      {"run", "WHY"}, {"run", "CLEAR_ERR"}, {"run", "ERRTIPS"}, {"run", "ERRGUIDE"},
      {"lib", "LISTLIBS"}, {"lib", "HASLIB"}, {"lib", "MATCHLIBS"},
      {"lib", "PICKLIB"}, {"lib", "RECIPE"}, {"lib", "CHECKDEPS"},
      {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"}, {"protect", "STATUS"}, {"protect", "VERSION"},
    };
    static const struct { const char *id; const char *hint; } thints[] = {
      {"general", "install/doctor/init surface · VERSION STATUS IDENTITY"},
      {"cap", "HASFORM/NEEDFORMS capability floor · FORMHINT · run -C"},
      {"fat", "VARROOM/REMAIN_MS fat nest budget · fat_session · run -T"},
      {"plate", "SETP/NEEDP plate agent JSON · plate_session · PRETTYP"},
      {"p2p", "SMX SERVE/DIAL mesh · CUBALC_SMX_KEY · P2P_SOFT/TIMEOUT"},
      {"run", "ASSERT/EXPECT/WHY run probes · ERRTIPS · CLEAR_ERR"},
      {"lib", "LISTLIBS/RECIPE INCLUDE discovery · MATCHLIBS · checkdeps"},
      {"protect", "HOLD_FLASH/protect status · CORE_PROTECT · device/mesh-join only"},
    };
    static const struct { const char *topic; const char *rel; } rels[] = {
      {"general", "cap"}, {"general", "lib"}, {"general", "run"}, {"general", "plate"},
      {"cap", "general"}, {"cap", "run"}, {"cap", "lib"}, {"cap", "fat"},
      {"fat", "plate"}, {"fat", "run"}, {"fat", "cap"}, {"fat", "general"},
      {"plate", "fat"}, {"plate", "run"}, {"plate", "general"}, {"plate", "lib"},
      {"p2p", "protect"}, {"p2p", "run"}, {"p2p", "general"}, {"p2p", "lib"},
      {"run", "general"}, {"run", "cap"}, {"run", "plate"}, {"run", "fat"},
      {"lib", "general"}, {"lib", "cap"}, {"lib", "plate"}, {"lib", "run"},
      {"protect", "p2p"}, {"protect", "general"}, {"protect", "run"}, {"protect", "lib"},
    };
    const char *name = (argc > 2 && argv[2][0]) ? argv[2] : "";
    char nup[96], topicsj[400], relj[256], thj[320], fhj[400], src[360];
    char seen_topics[16][32];
    char first_topic[32];
    const char *topic_hint = NULL;
    cubalc_run_result rr;
    size_t k, o = 0, j;
    int i, n = 0, nr = 0, nall = (int)(sizeof rows / sizeof rows[0]);
    int nseen = 0, first = 1;
    int n_th = (int)(sizeof thints / sizeof thints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    if (!name[0]) {
      fprintf(stderr, "usage: cubalc formguide|guideform <FormName>\n");
      printf("{\"schema\":\"cubalc.formguide.v1\",\"ok\":false,\"cmd\":\"formguide\","
             "\"err\":\"need form name\",\"version\":\"%s\"}\n",
             CUBALC_LANG_VERSION);
      return 2;
    }
    for (k = 0; name[k] && k + 1 < sizeof nup; k++) {
      char c = name[k];
      if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
      nup[k] = c;
    }
    nup[k] = 0;
    topicsj[0] = 0; first_topic[0] = 0; nseen = 0; n = 0; first = 1; o = 0;
    for (i = 0; i < nall; i++) {
      char fup[96];
      size_t jj;
      int match, already = 0, si;
      for (jj = 0; rows[i].form[jj] && jj + 1 < sizeof fup; jj++) {
        char c = rows[i].form[jj];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        fup[jj] = c;
      }
      fup[jj] = 0;
      match = !strcmp(fup, nup);
      if (!match) continue;
      for (si = 0; si < nseen; si++) {
        if (!strcmp(seen_topics[si], rows[i].topic)) { already = 1; break; }
      }
      if (already) continue;
      if (nseen < (int)(sizeof seen_topics / sizeof seen_topics[0])) {
        snprintf(seen_topics[nseen], sizeof seen_topics[0], "%s", rows[i].topic);
        nseen++;
      }
      if (!first_topic[0])
        snprintf(first_topic, sizeof first_topic, "%s", rows[i].topic);
      if (!first && o + 1 < sizeof topicsj) topicsj[o++] = ',';
      o += (size_t)snprintf(topicsj + o, sizeof topicsj - o, "\"%s\"", rows[i].topic);
      first = 0; n++;
    }
    if (n == 0) {
      printf("{\"schema\":\"cubalc.formguide.v1\",\"ok\":false,\"cmd\":\"formguide\","
             "\"form\":\"%s\",\"err\":\"no covering topics\",\"version\":\"%s\","
             "\"note\":\"FORMHINT · FORMTOPICS · cubalc formtopics\"}\n",
             nup, CUBALC_LANG_VERSION);
      return 1;
    }
    for (i = 0; i < n_th; i++)
      if (!strcmp(thints[i].id, first_topic)) { topic_hint = thints[i].hint; break; }
    if (!topic_hint) topic_hint = "";
    relj[0] = 0; o = 0; nr = 0; first = 1;
    for (i = 0; i < n_rels; i++) {
      if (strcmp(rels[i].topic, first_topic) != 0) continue;
      if (!first && o + 1 < sizeof relj) relj[o++] = ',';
      o += (size_t)snprintf(relj + o, sizeof relj - o, "\"%s\"", rels[i].rel);
      first = 0; nr++;
    }
    o = 0;
    for (j = 0; topic_hint[j] && o + 2 < sizeof thj; j++) {
      char c = topic_hint[j];
      if (c == '"' || c == '\\') { thj[o++] = '\\'; thj[o++] = c; }
      else thj[o++] = c;
    }
    thj[o] = 0;
    /* form hint via nested FORMHINT */
    if (strchr(name, ' ') || strchr(name, '"'))
      snprintf(src, sizeof src, "FORMHINT \"%s\"\nPRINT LAST\nPASS\n", name);
    else
      snprintf(src, sizeof src, "FORMHINT %s\nPRINT LAST\nPASS\n", name);
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<formguide-hint>", &rr, NULL);
    {
      const char *h = (rr.ok && rr.last_print[0]) ? rr.last_print : nup;
      o = 0;
      for (j = 0; h[j] && o + 2 < sizeof fhj; j++) {
        char c = h[j];
        if (c == '"' || c == '\\') { fhj[o++] = '\\'; fhj[o++] = c; }
        else if ((unsigned char)c < 32) fhj[o++] = ' ';
        else fhj[o++] = c;
      }
      fhj[o] = 0;
    }
    printf("{\"schema\":\"cubalc.formguide.v1\",\"ok\":true,\"cmd\":\"formguide\","
           "\"form\":\"%s\",\"form_hint\":\"%s\",\"topic\":\"%s\",\"topic_hint\":\"%s\","
           "\"topics_n\":%d,\"related_n\":%d,\"version\":\"%s\","
           "\"note\":\"FORMHINT+FORMTOPICS+first GUIDE · dual of FORMGUIDE\","
           "\"topics\":[%s],\"related\":[%s]}\n",
           nup, fhj, first_topic, thj, n, nr, CUBALC_LANG_VERSION,
           topicsj, relj);
    return 0;
  }
  if (strcmp(cmd, "guide") == 0 || strcmp(cmd, "playguide") == 0 ||
      strcmp(cmd, "topicguide") == 0 || strcmp(cmd, "agentguide") == 0 ||
      strcmp(cmd, "fulltopic") == 0 || strcmp(cmd, "topicboard") == 0 ||
      strcmp(cmd, "guidematch") == 0 || strcmp(cmd, "matchguide") == 0 ||
      strcmp(cmd, "guidenth") == 0 || strcmp(cmd, "nthguide") == 0 ||
      strcmp(cmd, "guidelastmatch") == 0 || strcmp(cmd, "lastmatchguide") == 0) {
    /* Usability: full playbook plate (dual of GUIDE) — hint+related+tips+forms+snip.
     *   cubalc guide · cubalc guide cap · cubalc guide match p · cubalc guide nth 1 p
     *   cubalc guidematch p · cubalc guidenth 1 p · cubalc guidelastmatch p
     * Schema cubalc.guide.v1 · agents skip TOPICHINT+RELATEDTOPIC+TOPIC / NTHTOPIC+GUIDE glue. */
    static const struct { const char *id; const char *hint; } hints[] = {
      {"general", "install/doctor/init surface · VERSION STATUS IDENTITY"},
      {"cap", "HASFORM/NEEDFORMS capability floor · FORMHINT · run -C"},
      {"fat", "VARROOM/REMAIN_MS fat nest budget · fat_session · run -T"},
      {"plate", "SETP/NEEDP plate agent JSON · plate_session · PRETTYP"},
      {"p2p", "SMX SERVE/DIAL mesh · CUBALC_SMX_KEY · P2P_SOFT/TIMEOUT"},
      {"run", "ASSERT/EXPECT/WHY run probes · ERRTIPS · CLEAR_ERR"},
      {"lib", "LISTLIBS/RECIPE INCLUDE discovery · MATCHLIBS · checkdeps"},
      {"protect", "HOLD_FLASH/protect status · CORE_PROTECT · device/mesh-join only"},
    };
    static const struct { const char *topic; const char *rel; } rels[] = {
      {"general", "cap"}, {"general", "lib"}, {"general", "run"}, {"general", "plate"},
      {"cap", "general"}, {"cap", "run"}, {"cap", "lib"}, {"cap", "fat"},
      {"fat", "plate"}, {"fat", "run"}, {"fat", "cap"}, {"fat", "general"},
      {"plate", "fat"}, {"plate", "run"}, {"plate", "general"}, {"plate", "lib"},
      {"p2p", "protect"}, {"p2p", "run"}, {"p2p", "general"}, {"p2p", "lib"},
      {"run", "general"}, {"run", "cap"}, {"run", "plate"}, {"run", "fat"},
      {"lib", "general"}, {"lib", "cap"}, {"lib", "plate"}, {"lib", "run"},
      {"protect", "p2p"}, {"protect", "general"}, {"protect", "run"}, {"protect", "lib"},
    };
    static const struct { const char *topic; const char *tip; } tips[] = {
      {"general", "cubalc doctor — install readiness plate"},
      {"general", "cubalc init --list — scaffold catalog"},
      {"general", "cubalc search KEYWORD — forms/libs/examples"},
      {"general", "INCLUDE agent_boot — REQUIRE VERSION + VERSION plate"},
      {"cap", "INCLUDE cap_boot — agent_boot + form_guard"},
      {"cap", "cubalc init --cap · HASFORM · NEEDFORMS · FORMHINT"},
      {"cap", "cubalc run -C FORMS · CUBALC_REQUIRE_FORMS floor"},
      {"fat", "INCLUDE fat_session durable nest"},
      {"fat", "cubalc run -T ms · REMAIN_MS · VARROOM"},
      {"plate", "INCLUDE plate_session · SETP NEEDP SAVEPLATE"},
      {"plate", "cubalc plate get|set|uniform · PRETTYP"},
      {"p2p", "export CUBALC_SMX_KEY hex64 · SMX SERVE/DIAL"},
      {"run", "cubalc run -q -e CODE · -I -L -R -C -T -s"},
      {"run", "WHY / why_hint after ASSERT or sticky LAST_ERR"},
      {"lib", "cubalc libs|recipe|checkdeps · INCLUDE MATCH|NEWEST"},
      {"protect", "cubalc protect status · docs/CORE_PROTECT.md"},
    };
    static const struct { const char *topic; const char *form; } forms[] = {
      {"general", "VERSION"}, {"general", "GUIDE"}, {"general", "TOPIC"},
      {"general", "TOPICHINT"}, {"general", "RELATEDTOPIC"}, {"general", "TIPS"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORMS"}, {"cap", "FORMHINT"}, {"cap", "GUIDE"},
      {"fat", "VARROOM"}, {"fat", "REMAIN_MS"}, {"fat", "GUIDE"},
      {"plate", "SETP"}, {"plate", "GETP"}, {"plate", "NEEDP"},
      {"p2p", "SMX"}, {"p2p", "SERVE"}, {"p2p", "DIAL"},
      {"run", "ASSERT"}, {"run", "EXPECT"}, {"run", "WHY"},
      {"lib", "LISTLIBS"}, {"lib", "RECIPE"}, {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"}, {"protect", "STATUS"},
    };
    static const struct { const char *topic; const char *src; } snips[] = {
      {"general", "VERSION\\nSTATUS\\nPASS\\n"},
      {"cap", "HASFORM SORTLIBS\\nFORMHINT SORTLIBS\\nPASS\\n"},
      {"fat", "VARROOM\\nHASVARROOM 8\\nPASS\\n"},
      {"plate", "SETP \\\"status\\\" \\\"ready\\\"\\nNEEDP \\\"status\\\"\\nPASS\\n"},
      {"p2p", "TIPS p2p\\nFORMSFOR p2p\\nPASS\\n"},
      {"run", "EXPECT 1 == 1\\nWHY\\nCLEAR_ERR\\nPASS\\n"},
      {"lib", "HASLIB agent_boot\\nRECIPE agent_boot\\nPASS\\n"},
      {"protect", "HOLD_FLASH\\nVERSION\\nPASS\\n"},
    };
    static const char *topic_ids[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    const char *topic = "general";
    char tup[32], tips_j[2400], forms_j[1200], rel_j[256], hj[320];
    char nlow[64], matches[8][16];
    nlow[0] = 0;
    const char *hint = NULL, *src = "", *needle = NULL, *fallback = NULL, *mpick = NULL;
    size_t k, o, j;
    int i, nt = 0, nf = 0, nr = 0, lines = 0;
    int mode_filt = 0; /* 0 direct · 1 first · 2 nth · 3 lastmatch */
    int m_idx = 0, m_n = 0, ai = 2;
    int n_hints = (int)(sizeof hints / sizeof hints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    int n_tips = (int)(sizeof tips / sizeof tips[0]);
    int n_forms = (int)(sizeof forms / sizeof forms[0]);
    int n_snips = (int)(sizeof snips / sizeof snips[0]);
    int n_topic_ids = (int)(sizeof topic_ids / sizeof topic_ids[0]);
    if (strcmp(cmd, "guidematch") == 0 || strcmp(cmd, "matchguide") == 0)
      mode_filt = 1;
    else if (strcmp(cmd, "guidenth") == 0 || strcmp(cmd, "nthguide") == 0)
      mode_filt = 2;
    else if (strcmp(cmd, "guidelastmatch") == 0 || strcmp(cmd, "lastmatchguide") == 0)
      mode_filt = 3;
    if (mode_filt == 0 && argc > 2 && argv[2] && argv[2][0]) {
      if (!strcmp(argv[2], "match") || !strcmp(argv[2], "first") ||
          !strcmp(argv[2], "pick")) {
        mode_filt = 1;
        ai = 3;
      } else if (!strcmp(argv[2], "nth") || !strcmp(argv[2], "index")) {
        mode_filt = 2;
        ai = 3;
      } else if (!strcmp(argv[2], "lastmatch") || !strcmp(argv[2], "endmatch") ||
                 !strcmp(argv[2], "matchlast")) {
        mode_filt = 3;
        ai = 3;
      } else {
        topic = argv[2];
      }
    }
    if (mode_filt != 0) {
      if (mode_filt == 2) {
        if (argc <= ai || !argv[ai] || !argv[ai][0]) {
          fprintf(stderr, "usage: cubalc guide nth <idx> <needle> [OR fallback]\n");
          printf("{\"schema\":\"cubalc.guide.v1\",\"ok\":false,\"cmd\":\"guidenth\","
                 "\"err\":\"need idx and needle\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 2;
        }
        m_idx = atoi(argv[ai]);
        ai++;
      }
      if (argc <= ai || !argv[ai] || !argv[ai][0]) {
        fprintf(stderr, "usage: cubalc guide match|lastmatch <needle> [OR fallback]\n");
        printf("{\"schema\":\"cubalc.guide.v1\",\"ok\":false,\"cmd\":\"guidematch\","
               "\"err\":\"need needle\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai];
      ai++;
      if (argc > ai + 1 && argv[ai] &&
          (!strcmp(argv[ai], "OR") || !strcmp(argv[ai], "or") ||
           !strcmp(argv[ai], "ELSE") || !strcmp(argv[ai], "DEFAULT")) &&
          argv[ai + 1] && argv[ai + 1][0])
        fallback = argv[ai + 1];
      for (k = 0; needle[k] && k + 1 < sizeof nlow; k++)
        nlow[k] = (char)((needle[k] >= 'A' && needle[k] <= 'Z')
                             ? needle[k] - 'A' + 'a' : needle[k]);
      nlow[k] = 0;
      if (!strcmp(nlow, "capability") || !strcmp(nlow, "forms") || !strcmp(nlow, "form"))
        snprintf(nlow, sizeof nlow, "%s", "cap");
      if (!strcmp(nlow, "mesh") || !strcmp(nlow, "smx") || !strcmp(nlow, "peer"))
        snprintf(nlow, sizeof nlow, "%s", "p2p");
      if (!strcmp(nlow, "nest") || !strcmp(nlow, "var") || !strcmp(nlow, "timeout"))
        snprintf(nlow, sizeof nlow, "%s", "fat");
      if (!strcmp(nlow, "json") || !strcmp(nlow, "agent"))
        snprintf(nlow, sizeof nlow, "%s", "plate");
      if (!strcmp(nlow, "start") || !strcmp(nlow, "help") || !strcmp(nlow, "all") ||
          !strcmp(nlow, "default") || !strcmp(nlow, "*"))
        snprintf(nlow, sizeof nlow, "%s", "general");
      m_n = 0;
      for (i = 0; i < n_topic_ids && m_n < 8; i++) {
        if (!strstr(topic_ids[i], nlow)) continue;
        snprintf(matches[m_n], sizeof matches[0], "%s", topic_ids[i]);
        m_n++;
      }
      if (mode_filt == 3) {
        if (m_n > 0) { mpick = matches[m_n - 1]; m_idx = m_n - 1; }
      } else if (mode_filt == 1) {
        if (m_n > 0) { mpick = matches[0]; m_idx = 0; }
      } else {
        if (m_idx < 0) m_idx = 0;
        if (m_idx < m_n) mpick = matches[m_idx];
      }
      if (!mpick) {
        if (fallback && fallback[0]) {
          topic = fallback;
        } else {
          printf("{\"schema\":\"cubalc.guide.v1\",\"ok\":false,\"cmd\":\"guidematch\","
                 "\"filter\":\"%s\",\"idx\":%d,\"match_n\":%d,\"err\":\"no match\","
                 "\"version\":\"%s\"}\n",
                 nlow, m_idx, m_n, CUBALC_LANG_VERSION);
          return 1;
        }
      } else {
        topic = mpick;
      }
    }
    for (k = 0; topic[k] && k + 1 < sizeof tup; k++)
      tup[k] = (char)((topic[k] >= 'A' && topic[k] <= 'Z')
                          ? topic[k] - 'A' + 'a' : topic[k]);
    tup[k] = 0;
    if (!strcmp(tup, "capability") || !strcmp(tup, "forms") || !strcmp(tup, "form"))
      snprintf(tup, sizeof tup, "%s", "cap");
    if (!strcmp(tup, "mesh") || !strcmp(tup, "smx") || !strcmp(tup, "peer"))
      snprintf(tup, sizeof tup, "%s", "p2p");
    if (!strcmp(tup, "nest") || !strcmp(tup, "var") || !strcmp(tup, "timeout"))
      snprintf(tup, sizeof tup, "%s", "fat");
    if (!strcmp(tup, "json") || !strcmp(tup, "agent"))
      snprintf(tup, sizeof tup, "%s", "plate");
    if (!strcmp(tup, "start") || !strcmp(tup, "help") || !strcmp(tup, "all") ||
        !strcmp(tup, "default") || !strcmp(tup, "*"))
      snprintf(tup, sizeof tup, "%s", "general");
    for (i = 0; i < n_hints; i++)
      if (!strcmp(hints[i].id, tup)) { hint = hints[i].hint; break; }
    if (!hint) {
      printf("{\"schema\":\"cubalc.guide.v1\",\"ok\":false,\"cmd\":\"guide\","
             "\"topic\":\"%s\",\"err\":\"unknown topic\",\"version\":\"%s\","
             "\"note\":\"LISTTOPICS · cubalc topics · HASTOPIC\"}\n",
             tup, CUBALC_LANG_VERSION);
      return 1;
    }
    o = 0; rel_j[0] = 0;
    for (i = 0; i < n_rels; i++) {
      if (strcmp(rels[i].topic, tup) != 0) continue;
      if (o && o + 1 < sizeof rel_j) rel_j[o++] = ',';
      o += (size_t)snprintf(rel_j + o, sizeof rel_j - o, "\"%s\"", rels[i].rel);
      nr++;
    }
    o = 0; tips_j[0] = 0;
    for (i = 0; i < n_tips; i++) {
      char esc[280];
      size_t e = 0;
      if (strcmp(tips[i].topic, tup) != 0) continue;
      for (j = 0; tips[i].tip[j] && e + 2 < sizeof esc; j++) {
        char c = tips[i].tip[j];
        if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
        else if ((unsigned char)c < 32) esc[e++] = ' ';
        else esc[e++] = c;
      }
      esc[e] = 0;
      if (o && o + 1 < sizeof tips_j) tips_j[o++] = ',';
      if (o + e + 3 < sizeof tips_j) {
        tips_j[o++] = '"';
        memcpy(tips_j + o, esc, e); o += e;
        tips_j[o++] = '"'; tips_j[o] = 0;
      }
      nt++;
    }
    o = 0; forms_j[0] = 0;
    for (i = 0; i < n_forms; i++) {
      size_t ln;
      if (strcmp(forms[i].topic, tup) != 0) continue;
      ln = strlen(forms[i].form);
      if (o && o + 1 < sizeof forms_j) forms_j[o++] = ',';
      if (o + ln + 3 < sizeof forms_j) {
        forms_j[o++] = '"';
        memcpy(forms_j + o, forms[i].form, ln); o += ln;
        forms_j[o++] = '"'; forms_j[o] = 0;
      }
      nf++;
    }
    for (i = 0; i < n_snips; i++) {
      if (!strcmp(snips[i].topic, tup)) { src = snips[i].src; break; }
    }
    for (j = 0; src[j]; j++) if (src[j] == 'n' && j > 0 && src[j-1] == '\\') lines++;
    if (lines == 0 && src[0]) lines = 1;
    o = 0;
    for (j = 0; hint[j] && o + 2 < sizeof hj; j++) {
      char c = hint[j];
      if (c == '"' || c == '\\') { hj[o++] = '\\'; hj[o++] = c; }
      else hj[o++] = c;
    }
    hj[o] = 0;
    printf("{\"schema\":\"cubalc.guide.v1\",\"ok\":true,\"cmd\":\"%s\","
           "\"topic\":\"%s\",\"hint\":\"%s\",\"related_n\":%d,\"tips_n\":%d,"
           "\"forms_n\":%d,\"snip_lines\":%d,\"version\":\"%s\","
           "\"filter\":\"%s\",\"match_i\":%d,\"match_n\":%d,\"filter_mode\":%d,"
           "\"note\":\"full playbook · MATCH|NTH|LASTMATCH filter pick · dual of GUIDE\","
           "\"related\":[%s],\"tips\":[%s],\"forms\":[%s],\"snip\":\"%s\"}\n",
           mode_filt == 1 ? "guidematch" : mode_filt == 2 ? "guidenth" :
           mode_filt == 3 ? "guidelastmatch" : "guide",
           tup, hj, nr, nt, nf, lines, CUBALC_LANG_VERSION,
           mode_filt ? nlow : "", m_idx, mode_filt ? m_n : 0, mode_filt,
           rel_j, tips_j, forms_j, src);
    return 0;
  }
  if (strcmp(cmd, "errtips") == 0 || strcmp(cmd, "fixtips") == 0 ||
      strcmp(cmd, "recovertips") == 0 || strcmp(cmd, "err-tips") == 0 ||
      strcmp(cmd, "fixhints") == 0 || strcmp(cmd, "whytips") == 0) {
    /* Usability: classify error text → recovery tip bag + topic (dual of ERRTIPS).
     *   cubalc errtips "INCLUDE missing foo"
     * Schema cubalc.errtips.v1 · pure C (no nested PRINT flakiness). */
    static const struct { const char *topic; const char *tip; } tips[] = {
      {"general", "cubalc doctor — install readiness plate"},
      {"general", "STATUS · VARS · CLEAR_ERR after fix"},
      {"general", "cubalc topics · TOPIC name for tips+forms+snip triad"},
      {"general", "cubalc search KEYWORD — forms/libs/examples"},
      {"cap", "HASFORM name · NEEDFORMS a b · FORMHINT name"},
      {"cap", "LISTFORMS prefix · cubalc forms · cubalc run -C FORMS"},
      {"cap", "INCLUDE cap_boot · cubalc init --cap"},
      {"cap", "RELATED HASFORM · FORMSFOR cap · RUNSNIP cap"},
      {"fat", "VARROOM · HASVARROOM n · NEEDVARROOM n"},
      {"fat", "REMAIN_MS · HAS_TIME · NEEDTIME · cubalc run -T ms"},
      {"fat", "INCLUDE fat_session · DEFAULT NEED_VARROOM"},
      {"plate", "NEEDP/DEFAULTP seed · GETP OR fallback"},
      {"plate", "cubalc plate has|need path.json keys"},
      {"plate", "PRETTYP · SAVEPLATE · INCLUDE plate_session"},
      {"p2p", "export CUBALC_SMX_KEY hex64 before mesh"},
      {"p2p", "CUBALC_P2P_SOFT=1 · CUBALC_P2P_TIMEOUT ms"},
      {"p2p", "cubalc smx-bus prove-tcp · docs/P2P_SMX.md"},
      {"run", "ASSERT_GOT/ASSERT_EXPECTED · EXPECT soft · CLEAR_ERR"},
      {"run", "WHY · cubalc run -s / CUBALC_STRICT"},
      {"run", "ERRTIPS after FAIL · TIPS run playbook"},
      {"lib", "cubalc libs [filter] · REQUIRE LIB · INCLUDE SOFT"},
      {"lib", "MATCHLIBS · CHECKDEPS · cubalc recipe name"},
      {"lib", "cubalc which name · HASLIB · LISTLIBS"},
      {"protect", "cubalc protect status · HOLD_FLASH device/mesh-join only"},
      {"protect", "docs/CORE_PROTECT.md · cubalc protect all"},
    };
    char msg[400], topic[32], tipsj[2400], errj[480];
    size_t o = 0, e, j, to = 0;
    int i, n = 0, nall = (int)(sizeof tips / sizeof tips[0]);
    int first = 1;
    msg[0] = 0;
    for (i = 2; i < argc; i++) {
      size_t ln;
      if (!argv[i]) continue;
      ln = strlen(argv[i]);
      if (o && o + 1 < sizeof msg) msg[o++] = ' ';
      if (o + ln < sizeof msg) { memcpy(msg + o, argv[i], ln); o += ln; }
      msg[o] = 0;
    }
    if (!msg[0]) snprintf(msg, sizeof msg, "%s", "ok");
    snprintf(topic, sizeof topic, "%s", "general");
    if (!strcmp(msg, "ok"))
      snprintf(topic, sizeof topic, "%s", "general");
    else if (strstr(msg, "NEEDFORM") || strstr(msg, "HASFORM") ||
             strstr(msg, "FORMMISS") || strstr(msg, "unknown form") ||
             strstr(msg, "did you mean") || strstr(msg, "REQUIRE FORM") ||
             strstr(msg, "LISTFORMS") || strstr(msg, "FORMHINT") ||
             strstr(msg, "NEEDFORMS") || strstr(msg, "HASFORMS"))
      snprintf(topic, sizeof topic, "%s", "cap");
    else if (strstr(msg, "NEEDMATCHLIBS") || strstr(msg, "HASMATCHLIBS") ||
             strstr(msg, "PICKLIB") || strstr(msg, "SORTLIBS") ||
             strstr(msg, "INCLUDE") || strstr(msg, "include") ||
             strstr(msg, "REQUIRE LIB") || strstr(msg, "lib missing") ||
             strstr(msg, "NEEDDEPS") || strstr(msg, "CHECKDEPS") ||
             strstr(msg, "HASLIB") || strstr(msg, "LISTLIBS"))
      snprintf(topic, sizeof topic, "%s", "lib");
    else if (strstr(msg, "VARROOM") || strstr(msg, "NEEDVARROOM") ||
             strstr(msg, "HASVARROOM") || strstr(msg, "REMAIN_MS") ||
             strstr(msg, "NEEDTIME") || strstr(msg, "HAS_TIME"))
      snprintf(topic, sizeof topic, "%s", "fat");
    else if (strstr(msg, "NEEDP") || strstr(msg, "NEEDFLAT") ||
             strstr(msg, "UNIFORM") || strstr(msg, "plate") ||
             strstr(msg, "PLATE") || strstr(msg, "SETP") || strstr(msg, "GETP"))
      snprintf(topic, sizeof topic, "%s", "plate");
    else if (strstr(msg, "DIAL") || strstr(msg, "SERVE") || strstr(msg, "SMX") ||
             strstr(msg, "TALK") || strstr(msg, "P2P") || strstr(msg, "timeout"))
      snprintf(topic, sizeof topic, "%s", "p2p");
    else if (strstr(msg, "HOLD_FLASH") || strstr(msg, "protect") ||
             strstr(msg, "CORE_PROTECT"))
      snprintf(topic, sizeof topic, "%s", "protect");
    else if (strstr(msg, "ASSERT") || strstr(msg, "EXPECT") ||
             strstr(msg, "is false") || strstr(msg, "FAIL") ||
             strstr(msg, "VERSION") || strstr(msg, "too old"))
      snprintf(topic, sizeof topic, "%s", "run");
    tipsj[0] = 0; to = 0; n = 0; first = 1;
    if (!strcmp(msg, "ok")) {
      const char *oktip = "ok — no sticky LAST_ERR · STATUS · CLEAR_ERR idle";
      size_t ln = strlen(oktip);
      tipsj[to++] = '"';
      memcpy(tipsj + to, oktip, ln); to += ln;
      tipsj[to++] = '"'; tipsj[to] = 0;
      n = 1;
    } else {
      for (i = 0; i < nall; i++) {
        size_t ln, v = 0;
        char one[320];
        if (strcmp(tips[i].topic, topic) != 0) continue;
        ln = strlen(tips[i].tip);
        for (j = 0; j < ln && v + 2 < sizeof one; j++) {
          char c = tips[i].tip[j];
          if (c == '"' || c == '\\') { one[v++] = '\\'; one[v++] = c; }
          else one[v++] = c;
        }
        one[v] = 0;
        if (to + v + 4 < sizeof tipsj) {
          if (!first) tipsj[to++] = ',';
          tipsj[to++] = '"';
          memcpy(tipsj + to, one, v); to += v;
          tipsj[to++] = '"'; tipsj[to] = 0;
          first = 0; n++;
        }
      }
    }
    e = 0;
    for (j = 0; msg[j] && e + 2 < sizeof errj; j++) {
      char c = msg[j];
      if (c == '"' || c == '\\') { errj[e++] = '\\'; errj[e++] = c; }
      else if ((unsigned char)c < 32) errj[e++] = ' ';
      else errj[e++] = c;
    }
    errj[e] = 0;
    printf("{\"schema\":\"cubalc.errtips.v1\",\"ok\":true,\"cmd\":\"errtips\","
           "\"topic\":\"%s\",\"n\":%d,\"version\":\"%s\","
           "\"note\":\"recovery tip bag from err text · dual of ERRTIPS · chain cubalc topic\","
           "\"err\":\"%s\",\"tips\":[%s]}\n",
           topic, n, CUBALC_LANG_VERSION, errj, tipsj);
    return n > 0 ? 0 : 1;
  }
  if (strcmp(cmd, "errrun") == 0 || strcmp(cmd, "recoversnip") == 0 ||
      strcmp(cmd, "fixrun") == 0 || strcmp(cmd, "err-run") == 0 ||
      strcmp(cmd, "errsnip") == 0 || strcmp(cmd, "fixsnip") == 0) {
    /* Usability: classify err + RUNSNIP topic (dual of ERRRUN).
     *   cubalc errrun "INCLUDE missing"
     * Schema cubalc.errrun.v1 · nested RUNSNIP via in-lang ERRRUN. */
    char msg[400], esc[480], src[720];
    cubalc_run_result rr;
    size_t o = 0, j, e;
    int i, ok;
    msg[0] = 0;
    for (i = 2; i < argc; i++) {
      size_t ln;
      if (!argv[i]) continue;
      ln = strlen(argv[i]);
      if (o && o + 1 < sizeof msg) msg[o++] = ' ';
      if (o + ln < sizeof msg) { memcpy(msg + o, argv[i], ln); o += ln; }
      msg[o] = 0;
    }
    if (!msg[0]) snprintf(msg, sizeof msg, "%s", "ok");
    e = 0;
    for (j = 0; msg[j] && e + 2 < sizeof esc; j++) {
      char c = msg[j];
      if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
      else if ((unsigned char)c < 32) esc[e++] = ' ';
      else esc[e++] = c;
    }
    esc[e] = 0;
    snprintf(src, sizeof src,
             "ERRRUN \"%s\"\n"
             "PRINT ERRRUN_TOPIC\n"
             "IF OK == 0 THEN\n"
             "  FAIL \"errrun nested\"\n"
             "END\n"
             "PASS\n",
             esc);
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<cli-errrun>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    {
      char topicj[48], lastj[400];
      size_t k, lo = 0;
      const char *tp = rr.last_print[0] ? rr.last_print : "general";
      /* topic from first print — may be multi-line if PRINT wrong; take first line */
      k = 0;
      while (tp[k] && tp[k] != '\n' && k + 1 < sizeof topicj) {
        topicj[k] = tp[k];
        k++;
      }
      topicj[k] = 0;
      /* re-fetch last via second field: use ERRRUN_LAST by another run */
      {
        char src2[640];
        cubalc_run_result rr2;
        snprintf(src2, sizeof src2,
                 "ERRRUN \"%s\"\n"
                 "PRINT ERRRUN_TOPIC\n"
                 "PASS\n",
                 esc);
        memset(&rr2, 0, sizeof rr2);
        (void)cubalc_run_source(src2, strlen(src2), "<cli-errrun-topic>", &rr2, NULL);
        tp = rr2.last_print[0] ? rr2.last_print : topicj;
        k = 0;
        while (tp[k] && tp[k] != '\n' && k + 1 < sizeof topicj) {
          topicj[k] = tp[k];
          k++;
        }
        topicj[k] = 0;
        snprintf(src2, sizeof src2,
                 "ERRRUN \"%s\"\n"
                 "PRINT LAST\n"
                 "PASS\n",
                 esc);
        memset(&rr2, 0, sizeof rr2);
        (void)cubalc_run_source(src2, strlen(src2), "<cli-errrun-last>", &rr2, NULL);
        tp = rr2.last_print;
        lo = 0;
        for (k = 0; tp[k] && lo + 2 < sizeof lastj; k++) {
          char c = tp[k];
          if (c == '"' || c == '\\') { lastj[lo++] = '\\'; lastj[lo++] = c; }
          else if (c == '\n') { lastj[lo++] = '\\'; lastj[lo++] = 'n'; }
          else if ((unsigned char)c < 32) lastj[lo++] = ' ';
          else lastj[lo++] = c;
        }
        lastj[lo] = 0;
      }
      printf("{\"schema\":\"cubalc.errrun.v1\",\"ok\":%s,\"cmd\":\"errrun\","
             "\"topic\":\"%s\",\"asserts_ok\":%d,\"asserts_fail\":%d,\"stmts\":%d,"
             "\"last\":\"%s\",\"version\":\"%s\","
             "\"note\":\"ERRTIPS classify + RUNSNIP topic · dual of in-lang ERRRUN\","
             "\"err\":\"%s\"}\n",
             ok ? "true" : "false", topicj, rr.asserts_ok, rr.asserts_fail, rr.stmts,
             lastj, CUBALC_LANG_VERSION, esc);
    }
    return ok ? 0 : 1;
  }
  if (strcmp(cmd, "errguide") == 0 || strcmp(cmd, "recoverguide") == 0 ||
      strcmp(cmd, "fixguide") == 0 || strcmp(cmd, "err-guide") == 0 ||
      strcmp(cmd, "whyguide") == 0 || strcmp(cmd, "errplayguide") == 0) {
    /* Usability: classify err + GUIDE playbook (dual of ERRGUIDE / twin of errrun).
     *   cubalc errguide "INCLUDE missing"
     * Schema cubalc.errguide.v1 · pure C plate (no nested flakiness). */
    static const struct { const char *topic; const char *tip; } tips[] = {
      {"general", "cubalc doctor — install readiness plate"},
      {"general", "STATUS · VARS · CLEAR_ERR after fix"},
      {"general", "cubalc guide · TOPIC name for full playbook"},
      {"general", "cubalc search KEYWORD — forms/libs/examples"},
      {"cap", "HASFORM name · NEEDFORMS a b · FORMHINT name"},
      {"cap", "LISTFORMS prefix · cubalc forms · cubalc run -C FORMS"},
      {"cap", "INCLUDE cap_boot · cubalc init --cap"},
      {"cap", "RELATED HASFORM · GUIDE cap · RUNSNIP cap"},
      {"fat", "VARROOM · HASVARROOM n · NEEDVARROOM n"},
      {"fat", "REMAIN_MS · HAS_TIME · NEEDTIME · cubalc run -T ms"},
      {"fat", "INCLUDE fat_session · DEFAULT NEED_VARROOM"},
      {"plate", "NEEDP/DEFAULTP seed · GETP OR fallback"},
      {"plate", "cubalc plate has|need path.json keys"},
      {"plate", "PRETTYP · SAVEPLATE · INCLUDE plate_session"},
      {"p2p", "export CUBALC_SMX_KEY hex64 before mesh"},
      {"p2p", "CUBALC_P2P_SOFT=1 · CUBALC_P2P_TIMEOUT ms"},
      {"p2p", "cubalc smx-bus prove-tcp · docs/P2P_SMX.md"},
      {"run", "ASSERT_GOT/ASSERT_EXPECTED · EXPECT soft · CLEAR_ERR"},
      {"run", "WHY · cubalc run -s / CUBALC_STRICT"},
      {"run", "ERRGUIDE after FAIL · GUIDE run playbook"},
      {"lib", "cubalc libs [filter] · REQUIRE LIB · INCLUDE SOFT"},
      {"lib", "MATCHLIBS · CHECKDEPS · cubalc recipe name"},
      {"lib", "cubalc which name · HASLIB · LISTLIBS"},
      {"protect", "cubalc protect status · HOLD_FLASH device/mesh-join only"},
      {"protect", "docs/CORE_PROTECT.md · cubalc protect all"},
    };
    static const struct { const char *id; const char *hint; } hints[] = {
      {"general", "install/doctor/init surface · VERSION STATUS IDENTITY"},
      {"cap", "HASFORM/NEEDFORMS capability floor · FORMHINT · run -C"},
      {"fat", "VARROOM/REMAIN_MS fat nest budget · fat_session · run -T"},
      {"plate", "SETP/NEEDP plate agent JSON · plate_session · PRETTYP"},
      {"p2p", "SMX SERVE/DIAL mesh · CUBALC_SMX_KEY · P2P_SOFT/TIMEOUT"},
      {"run", "ASSERT/EXPECT/WHY run probes · ERRTIPS · CLEAR_ERR"},
      {"lib", "LISTLIBS/RECIPE INCLUDE discovery · MATCHLIBS · checkdeps"},
      {"protect", "HOLD_FLASH/protect status · CORE_PROTECT · device/mesh-join only"},
    };
    static const struct { const char *topic; const char *rel; } rels[] = {
      {"general", "cap"}, {"general", "lib"}, {"general", "run"}, {"general", "plate"},
      {"cap", "general"}, {"cap", "run"}, {"cap", "lib"}, {"cap", "fat"},
      {"fat", "plate"}, {"fat", "run"}, {"fat", "cap"}, {"fat", "general"},
      {"plate", "fat"}, {"plate", "run"}, {"plate", "general"}, {"plate", "lib"},
      {"p2p", "protect"}, {"p2p", "run"}, {"p2p", "general"}, {"p2p", "lib"},
      {"run", "general"}, {"run", "cap"}, {"run", "plate"}, {"run", "fat"},
      {"lib", "general"}, {"lib", "cap"}, {"lib", "plate"}, {"lib", "run"},
      {"protect", "p2p"}, {"protect", "general"}, {"protect", "run"}, {"protect", "lib"},
    };
    static const struct { const char *topic; const char *form; } forms[] = {
      {"general", "VERSION"}, {"general", "GUIDE"}, {"general", "STATUS"},
      {"general", "ERRGUIDE"}, {"general", "TIPS"},
      {"cap", "HASFORM"}, {"cap", "NEEDFORMS"}, {"cap", "FORMHINT"}, {"cap", "GUIDE"},
      {"fat", "VARROOM"}, {"fat", "REMAIN_MS"}, {"fat", "GUIDE"},
      {"plate", "SETP"}, {"plate", "GETP"}, {"plate", "NEEDP"},
      {"p2p", "SMX"}, {"p2p", "SERVE"}, {"p2p", "DIAL"},
      {"run", "ASSERT"}, {"run", "EXPECT"}, {"run", "WHY"}, {"run", "ERRGUIDE"},
      {"lib", "LISTLIBS"}, {"lib", "RECIPE"}, {"lib", "INCLUDE"},
      {"protect", "HOLD_FLASH"}, {"protect", "STATUS"},
    };
    char msg[400], topic[32], tipsj[2400], formsj[800], relj[256], hj[320], errj[480];
    const char *hint = NULL;
    size_t o = 0, e, j, to = 0;
    int i, n = 0, nr = 0, nf = 0;
    int nall = (int)(sizeof tips / sizeof tips[0]);
    int n_hints = (int)(sizeof hints / sizeof hints[0]);
    int n_rels = (int)(sizeof rels / sizeof rels[0]);
    int n_forms = (int)(sizeof forms / sizeof forms[0]);
    int first;
    msg[0] = 0;
    for (i = 2; i < argc; i++) {
      size_t ln;
      if (!argv[i]) continue;
      ln = strlen(argv[i]);
      if (o && o + 1 < sizeof msg) msg[o++] = ' ';
      if (o + ln < sizeof msg) { memcpy(msg + o, argv[i], ln); o += ln; }
      msg[o] = 0;
    }
    if (!msg[0]) snprintf(msg, sizeof msg, "%s", "ok");
    snprintf(topic, sizeof topic, "%s", "general");
    if (!strcmp(msg, "ok"))
      snprintf(topic, sizeof topic, "%s", "general");
    else if (strstr(msg, "NEEDFORM") || strstr(msg, "HASFORM") ||
             strstr(msg, "FORMMISS") || strstr(msg, "unknown form") ||
             strstr(msg, "did you mean") || strstr(msg, "REQUIRE FORM") ||
             strstr(msg, "LISTFORMS") || strstr(msg, "FORMHINT") ||
             strstr(msg, "NEEDFORMS") || strstr(msg, "HASFORMS"))
      snprintf(topic, sizeof topic, "%s", "cap");
    else if (strstr(msg, "NEEDMATCHLIBS") || strstr(msg, "HASMATCHLIBS") ||
             strstr(msg, "PICKLIB") || strstr(msg, "SORTLIBS") ||
             strstr(msg, "INCLUDE") || strstr(msg, "include") ||
             strstr(msg, "REQUIRE LIB") || strstr(msg, "lib missing") ||
             strstr(msg, "NEEDDEPS") || strstr(msg, "CHECKDEPS") ||
             strstr(msg, "HASLIB") || strstr(msg, "LISTLIBS"))
      snprintf(topic, sizeof topic, "%s", "lib");
    else if (strstr(msg, "VARROOM") || strstr(msg, "NEEDVARROOM") ||
             strstr(msg, "HASVARROOM") || strstr(msg, "REMAIN_MS") ||
             strstr(msg, "NEEDTIME") || strstr(msg, "HAS_TIME"))
      snprintf(topic, sizeof topic, "%s", "fat");
    else if (strstr(msg, "NEEDP") || strstr(msg, "NEEDFLAT") ||
             strstr(msg, "UNIFORM") || strstr(msg, "plate") ||
             strstr(msg, "PLATE") || strstr(msg, "SETP") || strstr(msg, "GETP"))
      snprintf(topic, sizeof topic, "%s", "plate");
    else if (strstr(msg, "DIAL") || strstr(msg, "SERVE") || strstr(msg, "SMX") ||
             strstr(msg, "TALK") || strstr(msg, "P2P") || strstr(msg, "timeout"))
      snprintf(topic, sizeof topic, "%s", "p2p");
    else if (strstr(msg, "HOLD_FLASH") || strstr(msg, "protect") ||
             strstr(msg, "CORE_PROTECT"))
      snprintf(topic, sizeof topic, "%s", "protect");
    else if (strstr(msg, "ASSERT") || strstr(msg, "EXPECT") ||
             strstr(msg, "is false") || strstr(msg, "FAIL") ||
             strstr(msg, "VERSION") || strstr(msg, "too old"))
      snprintf(topic, sizeof topic, "%s", "run");
    for (i = 0; i < n_hints; i++)
      if (!strcmp(hints[i].id, topic)) { hint = hints[i].hint; break; }
    if (!hint) hint = "cubalc topics · GUIDE general";
    tipsj[0] = 0; to = 0; n = 0; first = 1;
    for (i = 0; i < nall; i++) {
      char esc[280];
      size_t ee = 0;
      if (strcmp(tips[i].topic, topic) != 0) continue;
      for (j = 0; tips[i].tip[j] && ee + 2 < sizeof esc; j++) {
        char c = tips[i].tip[j];
        if (c == '"' || c == '\\') { esc[ee++] = '\\'; esc[ee++] = c; }
        else if ((unsigned char)c < 32) esc[ee++] = ' ';
        else esc[ee++] = c;
      }
      esc[ee] = 0;
      if (!first && to + 1 < sizeof tipsj) tipsj[to++] = ',';
      if (to + ee + 3 < sizeof tipsj) {
        tipsj[to++] = '"';
        memcpy(tipsj + to, esc, ee); to += ee;
        tipsj[to++] = '"'; tipsj[to] = 0;
      }
      first = 0; n++;
    }
    if (!strcmp(msg, "ok") && n == 0) {
      snprintf(tipsj, sizeof tipsj, "\"ok — no sticky LAST_ERR · GUIDE general playbook\"");
      n = 1;
    }
    relj[0] = 0; o = 0; nr = 0; first = 1;
    for (i = 0; i < n_rels; i++) {
      if (strcmp(rels[i].topic, topic) != 0) continue;
      if (!first && o + 1 < sizeof relj) relj[o++] = ',';
      o += (size_t)snprintf(relj + o, sizeof relj - o, "\"%s\"", rels[i].rel);
      first = 0; nr++;
    }
    formsj[0] = 0; o = 0; nf = 0; first = 1;
    for (i = 0; i < n_forms; i++) {
      size_t ln;
      if (strcmp(forms[i].topic, topic) != 0) continue;
      ln = strlen(forms[i].form);
      if (!first && o + 1 < sizeof formsj) formsj[o++] = ',';
      if (o + ln + 3 < sizeof formsj) {
        formsj[o++] = '"';
        memcpy(formsj + o, forms[i].form, ln); o += ln;
        formsj[o++] = '"'; formsj[o] = 0;
      }
      first = 0; nf++;
    }
    o = 0;
    for (j = 0; hint[j] && o + 2 < sizeof hj; j++) {
      char c = hint[j];
      if (c == '"' || c == '\\') { hj[o++] = '\\'; hj[o++] = c; }
      else hj[o++] = c;
    }
    hj[o] = 0;
    e = 0;
    for (j = 0; msg[j] && e + 2 < sizeof errj; j++) {
      char c = msg[j];
      if (c == '"' || c == '\\') { errj[e++] = '\\'; errj[e++] = c; }
      else if ((unsigned char)c < 32) errj[e++] = ' ';
      else errj[e++] = c;
    }
    errj[e] = 0;
    printf("{\"schema\":\"cubalc.errguide.v1\",\"ok\":true,\"cmd\":\"errguide\","
           "\"topic\":\"%s\",\"hint\":\"%s\",\"related_n\":%d,\"tips_n\":%d,"
           "\"forms_n\":%d,\"version\":\"%s\","
           "\"note\":\"ERRTIPS classify + GUIDE playbook · dual of ERRGUIDE · twin of errrun\","
           "\"related\":[%s],\"tips\":[%s],\"forms\":[%s],\"err\":\"%s\"}\n",
           topic, hj, nr, n, nf, CUBALC_LANG_VERSION,
           relj, tipsj, formsj, errj);
    return 0;
  }
  if (strcmp(cmd, "init") == 0 || strcmp(cmd, "new") == 0 ||
      strcmp(cmd, "scaffold") == 0 || strcmp(cmd, "create") == 0) {
    return cubalc_cmd_init(argc, argv);
  }
  if (strcmp(cmd, "libs") == 0 || strcmp(cmd, "lib") == 0 ||
      strcmp(cmd, "stdlib") == 0) {
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
  
  if (strcmp(cmd, "guards") == 0 || strcmp(cmd, "guard") == 0 ||
      strcmp(cmd, "contracts") == 0 || strcmp(cmd, "contract") == 0) {
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
if (strcmp(cmd, "env") == 0 || strcmp(cmd, "environ") == 0 ||
      strcmp(cmd, "vars") == 0 || strcmp(cmd, "env-list") == 0) {
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
  if (strcmp(cmd, "examples") == 0 || strcmp(cmd, "example") == 0 ||
      strcmp(cmd, "starters") == 0 || strcmp(cmd, "recipes") == 0) {
    /* Usability: curated runnable starters for agents/humans — what to run
     * first without reading full COOKBOOK prose. Filter by path/tag prefix. */
    static const struct {
      const char *path;
      const char *tag;
      const char *hint;
    } catalog[] = {
      {"programs/hello_cube.cubalc", "hello",
       "hold → place → plug short form"},
      {"programs/proof/12_hold_flash_plug.cubalc", "hold",
       "HOLD_FLASH user permission before PLUG"},
      {"programs/proof/573_env_or_assert_msg.cubalc", "assert",
       "SYS ENV OR fallback + ASSERT message strings"},
      {"programs/proof/577_include_shortname.cubalc", "include",
       "INCLUDE short lib name → programs/lib"},
      {"programs/proof/help_form.cubalc", "help",
       "in-language HELP [form] discovery"},
      {"programs/p2p/mesh_local.cubalc", "smx",
       "in-process SMX EXCHANGE mesh"},
      {"programs/p2p/peer_dial.cubalc", "p2p",
       "SMX DIAL · CUBALC_P2P_SOFT soft-fail"},
      {"programs/protect/core_protect.cubalc", "protect",
       "Core protect board proof"},
      {"programs/proof/sys_arg_or.cubalc", "arg",
       "SYS ARG n OR fallback script defaults"},
      {"programs/lib/hold_seed.cubalc", "lib",
       "INCLUDE hold_seed seed snippet (not a full program)"},
      {"programs/lib/agent_boot.cubalc", "boot",
       "INCLUDE agent_boot REQUIRE+VERSION (HOLD_FLASH default)"},
      {"programs/lib/plate_boot.cubalc", "plate",
       "INCLUDE plate_boot ENSUREPLATE + PLATE state preamble"},
      {"programs/lib/plate_tick.cubalc", "plate",
       "INCLUDE plate_tick ENSURE + bump + ts stamp + SAVE"},
      {"programs/lib/plate_save.cubalc", "plate",
       "INCLUDE plate_save SAVEPLATE PLATE_PATH PLATE write-back"},
      {"programs/lib/plate_patch.cubalc", "plate",
       "INCLUDE plate_patch overlay PLATE_PATCH + SAVE multi-key update"},
      {"programs/lib/plate_session.cubalc", "plate",
       "INCLUDE plate_session agent_boot + plate_boot one-shot"},
      {"programs/lib/plate_uniform.cubalc", "plate",
       "INCLUDE plate_uniform UNIFORMFLAT DEFAULT needle nest check"},
      {"programs/lib/plate_peer.cubalc", "plate",
       "INCLUDE plate_peer SEEDP INTO PEER multi-plate state"},
      {"programs/lib/plate_peer_save.cubalc", "plate",
       "INCLUDE plate_peer_save SAVEP FROM PEER write-back"},
      {"programs/lib/plate_both_save.cubalc", "plate",
       "INCLUDE plate_both_save PLATE+PEER dual write-back"},
      {"programs/lib/plate_peer_session.cubalc", "peer",
       "INCLUDE plate_peer_session PLATE+PEER multi-plate one-shot"},
      {"programs/lib/var_guard.cubalc", "fat",
       "INCLUDE var_guard NEEDVARROOM free-slot gate · DEFAULT NEED_VARROOM"},
      {"programs/lib/time_guard.cubalc", "fat",
       "INCLUDE time_guard NEEDTIME wall budget gate · DEFAULT NEED_TIME"},
      {"programs/lib/fat_boot.cubalc", "fat",
       "INCLUDE fat_boot agent_boot+var_guard nest room one-shot"},
      {"programs/lib/fat_session.cubalc", "fat",
       "INCLUDE fat_session fat_boot+plate_boot durable nest · cubalc recipe"},
      {"programs/proof/1283_fat_boot.cubalc", "fat",
       "fat_boot stdlib proof · cubalc init --fat"},
      {"programs/proof/1287_fat_session.cubalc", "fat",
       "fat_session durable nest proof · cubalc init --fat-session"},
      {"programs/proof/1279_var_guard.cubalc", "fat",
       "var_guard free-slot gate proof"},
      {"programs/proof/1282_time_guard.cubalc", "fat",
       "time_guard wall budget gate proof"},
      {"programs/proof/587_agent_boot.cubalc", "agent",
       "agent_boot stdlib preamble proof"},
      {"programs/proof/1099_plate_boot.cubalc", "plate_boot",
       "plate_boot stdlib state plate preamble proof"},
      {"programs/proof/1105_plate_tick.cubalc", "plate_tick",
       "plate_tick stdlib agent plate tick proof"},
      {"programs/proof/1109_plate_save.cubalc", "plate_save",
       "plate_save stdlib agent plate write-back proof"},
      {"programs/proof/1115_plate_patch.cubalc", "plate_patch",
       "plate_patch stdlib multi-key overlay+save proof"},
      {"programs/proof/1117_plate_session.cubalc", "plate_session",
       "plate_session agent_boot+plate_boot one INCLUDE proof"},
      {"programs/proof/1253_plate_uniform.cubalc", "plate_uniform",
       "plate_uniform INCLUDE nest consistency DEFAULT needle proof"},
      {"programs/proof/588_status.cubalc", "status",
       "STATUS cubalc.status.v1 agent health plate"},
      {"programs/proof/589_clear_err.cubalc", "clear_err",
       "CLEAR_ERR wipe sticky LAST_ERR after soft recovery"},
      {"programs/proof/590_require_lib.cubalc", "require_lib",
       "REQUIRE LIB fail-fast if module missing"},
      {"programs/proof/676_require_env.cubalc", "require_env",
       "REQUIRE ENV fail-fast if host env missing"},
      {"programs/proof/726_require_path.cubalc", "require_path",
       "REQUIRE PATH/DIR/REG fail-fast host path gates"},
      {"programs/proof/677_sys_upperall.cubalc", "sys_upperall",
       "SYS UPPERALL/LOWERALL case-map bag fields"},
      {"programs/proof/678_sys_mapreplace.cubalc", "sys_mapreplace",
       "SYS MAPREPLACE bag-wide GSUB path/tag rewrite"},
      {"programs/proof/679_sys_freq.cubalc", "sys_freq",
       "SYS FREQ bag field frequency histogram"},
      {"programs/proof/680_sys_cutall.cubalc", "sys_cutall",
       "SYS CUTALL peel column from every bag line"},
      {"programs/proof/681_sys_sortfreq.cubalc", "sys_sortfreq",
       "SYS SORTFREQ sort FREQ bag by count"},
      {"programs/proof/682_sys_beforeall.cubalc", "sys_beforeall",
       "SYS BEFOREALL/AFTERALL peel every bag field"},
      {"programs/proof/683_sys_firstmatch.cubalc", "sys_firstmatch",
       "SYS FIRSTMATCH/GREP1 first bag field hit"},
      {"programs/proof/684_sys_chunk.cubalc", "sys_chunk",
       "SYS CHUNK/BATCH group bag fields by n"},
      {"programs/proof/685_sys_window.cubalc", "sys_window",
       "SYS WINDOW/SLIDE overlapping bag field windows"},
      {"programs/proof/686_sys_stride.cubalc", "sys_stride",
       "SYS STRIDE/EVERY partition bag by step/offset"},
      {"programs/proof/687_sys_rotate.cubalc", "sys_rotate",
       "SYS ROTATE/ROTL/ROTR bag field round-robin"},
      {"programs/proof/688_sys_flatten.cubalc", "sys_flatten",
       "SYS FLATTEN/UNCHUNK expand joined bag fields"},
      {"programs/proof/689_sys_countmatch.cubalc", "sys_countmatch",
       "SYS COUNTMATCH/GREPCOUNT substring field count"},
      {"programs/proof/690_sys_lenall.cubalc", "sys_lenall",
       "SYS LENALL/MAPLEN per-field lengths bag"},
      {"programs/proof/691_sys_minmax_bag.cubalc", "sys_minmax_bag",
       "SYS MIN/MAX bag mode like SUM"},
      {"programs/proof/692_sys_argmax.cubalc", "sys_argmax",
       "SYS ARGMAX/ARGMIN index of extreme"},
      {"programs/proof/693_sys_padall.cubalc", "sys_padall",
       "SYS PADALL/LPADALL pad every bag field"},
      {"programs/proof/694_sys_sortlen.cubalc", "sys_sortlen",
       "SYS SORTLEN sort bag fields by length"},
      {"programs/proof/695_sys_lastmatch.cubalc", "sys_lastmatch",
       "SYS LASTMATCH/GREP1L last bag field hit"},
      {"programs/proof/696_sys_maxlen.cubalc", "sys_maxlen",
       "SYS MAXLEN/MINLEN max/min bag field length"},
      {"programs/proof/697_sys_truncall.cubalc", "sys_truncall",
       "SYS TRUNCALL/CLIPALL truncate bag fields"},
      {"programs/proof/698_sys_longest.cubalc", "sys_longest",
       "SYS LONGEST/SHORTEST pick bag field by length"},
      {"programs/proof/699_sys_commonprefix.cubalc", "sys_commonprefix",
       "SYS COMMONPREFIX/LCP shared bag prefix"},
      {"programs/proof/700_sys_stripprefix.cubalc", "sys_stripprefix",
       "SYS STRIPPREFIX/STRIPCOMMON relative bag paths"},
      {"programs/proof/701_sys_drawn.cubalc", "sys_drawn",
       "SYS DRAWN/SAMPLEK sample k unique bag fields"},
      {"programs/proof/702_sys_lookup.cubalc", "sys_lookup",
       "SYS LOOKUP/KVGET peel key:val bag value"},
      {"programs/proof/703_sys_kvset.cubalc", "sys_kvset",
       "SYS KVSET/SETKV set/update key:val bag field"},
      {"programs/proof/704_sys_kvinc.cubalc", "sys_kvinc",
       "SYS KVINC/INCKV bump key:val numeric counter"},
      {"programs/proof/705_sys_kvdel.cubalc", "sys_kvdel",
       "SYS KVDEL/DELKV drop key:val bag field"},
      {"programs/proof/706_sys_lookup_or.cubalc", "sys_lookup_or",
       "SYS LOOKUP bag key OR fallback default"},
      {"programs/proof/707_sys_mergekv.cubalc", "sys_mergekv",
       "SYS MERGEKV/KVADDALL merge key:val bags by sum"},
      {"programs/proof/708_sys_sumkv.cubalc", "sys_sumkv",
       "SYS SUMKV/TOTALKV sum key:val bag values"},
      {"programs/proof/709_sys_topkey.cubalc", "sys_topkey",
       "SYS TOPKEY/BOTKEY max/min key of key:val bag"},
      {"programs/proof/710_sys_lookupn.cubalc", "sys_lookupn",
       "SYS LOOKUPN/KVGETN peel key:val as int LAST_N"},
      {"programs/proof/711_sys_threshkv.cubalc", "sys_threshkv",
       "SYS THRESHKV/KEEPVAL keep key:val value>=min"},
      {"programs/proof/712_sys_pctkv.cubalc", "sys_pctkv",
       "SYS PCTKV/SHAREKV key:val integer percent of total"},
      {"programs/proof/713_sys_capkv.cubalc", "sys_capkv",
       "SYS CAPKV/CLAMPKV clamp key:val values to max"},
      {"programs/proof/714_sys_scalekv.cubalc", "sys_scalekv",
       "SYS SCALEKV/MULKV multiply key:val values by factor"},
      {"programs/proof/715_sys_diffkv.cubalc", "sys_diffkv",
       "SYS DIFFKV/SUBKV subtract key:val bags by key"},
      {"programs/proof/716_sys_addkv.cubalc", "sys_addkv",
       "SYS ADDKV/OFFSETKV add delta to key:val values"},
      {"programs/proof/717_sys_dropzero.cubalc", "sys_dropzero",
       "SYS DROPZERO/KEEPNZ drop key:val value==0"},
      {"programs/proof/718_sys_abskv.cubalc", "sys_abskv",
       "SYS ABSKV/MAGKV absolute key:val values"},
      {"programs/proof/719_sys_signkv.cubalc", "sys_signkv",
       "SYS SIGNKV/DIRKV map key:val to -1|0|1"},
      {"programs/proof/720_sys_divkv.cubalc", "sys_divkv",
       "SYS DIVKV/IDIVKV integer-divide key:val values"},
      {"programs/proof/721_sys_avgkv.cubalc", "sys_avgkv",
       "SYS AVGKV/MEANKV integer mean of key:val values"},
      {"programs/proof/722_sys_mediankv.cubalc", "sys_mediankv",
       "SYS MEDIANKV/P50KV median of key:val values"},
      {"programs/proof/723_sys_keepkey.cubalc", "sys_keepkey",
       "SYS KEEPKEY/GREPKEY filter key:val by key needle"},
      {"programs/proof/724_sys_glob.cubalc", "sys_glob",
       "SYS GLOB/MATCHFILES list basenames matching pattern"},
      {"programs/proof/725_sys_pathglob.cubalc", "sys_pathglob",
       "SYS PATHGLOB/PGLOB full paths matching shell pattern"},
      {"programs/proof/726_require_path.cubalc", "require_path",
       "REQUIRE PATH/DIR/REG fail-fast host path gates"},
      {"programs/proof/727_sys_filterglob.cubalc", "sys_filterglob",
       "SYS FILTERGLOB/MATCHBAG shell-pattern bag filter"},
      {"programs/proof/728_sys_basenameall.cubalc", "sys_basenameall",
       "SYS BASENAMEALL/DIRNAMEALL/EXTALL/STEMALL path bag maps"},
      {"programs/proof/729_sys_keepfiles.cubalc", "sys_keepfiles",
       "SYS KEEPFILES/KEEPDIRS/KEEPEXIST path bag kind filters"},
      {"programs/proof/730_sys_sizeall.cubalc", "sys_sizeall",
       "SYS SIZEALL/MAPSIZE path bag byte sizes + sum"},
      {"programs/proof/731_sys_mtimeall.cubalc", "sys_mtimeall",
       "SYS MTIMEALL/AGEALL path bag time maps + min/max"},
      {"programs/proof/732_sys_newest.cubalc", "sys_newest",
       "SYS NEWEST/OLDEST/LARGEST/SMALLEST path bag pick"},
      {"programs/proof/733_sys_sortmtime.cubalc", "sys_sortmtime",
       "SYS SORTMTIME/SORTSIZE order path bags by time/size"},
      {"programs/proof/734_sys_fresh.cubalc", "sys_fresh",
       "SYS FRESH/KEEPSTALE filter path bags by age threshold"},
      {"programs/proof/735_sys_keepnewer.cubalc", "sys_keepnewer",
       "SYS KEEPNEWER/KEEPOLDER filter path bags vs ref mtime"},
      {"programs/proof/736_sys_keepbigger.cubalc", "sys_keepbigger",
       "SYS KEEPBIGGER/KEEPSMALLER filter path bags by size"},
      {"programs/proof/737_sys_rmall.cubalc", "sys_rmall",
       "SYS RMALL/UNLINKALL bulk remove path bag"},
      {"programs/proof/738_sys_touchall.cubalc", "sys_touchall",
       "SYS TOUCHALL/ENSUREALL bulk create/refresh path bag"},
      {"programs/proof/739_sys_copyall.cubalc", "sys_copyall",
       "SYS COPYALL/CPALL bulk copy path bag into dest dir"},
      {"programs/proof/740_sys_mkdirall.cubalc", "sys_mkdirall",
       "SYS MKDIRALL/ENSUREDIRS bulk mkdir -p path bag"},
      {"programs/proof/741_sys_moveall.cubalc", "sys_moveall",
       "SYS MOVEALL/MVALL bulk move path bag into dest dir"},
      {"programs/proof/742_sys_walk.cubalc", "sys_walk",
       "SYS WALK/FINDALL recursive full-path discovery"},
      {"programs/proof/743_sys_eqfile.cubalc", "sys_eqfile",
       "SYS EQFILE/SAMEFILE identical file content probe"},
      {"programs/proof/744_sys_logall.cubalc", "sys_logall",
       "SYS LOGALL/APPENDFILES append line to path bag"},
      {"programs/proof/745_sys_grepfiles.cubalc", "sys_grepfiles",
       "SYS GREPFILES/SEARCHFILES filter paths by file content"},
      {"programs/proof/746_sys_readall.cubalc", "sys_readall",
       "SYS READALL/CATFILES concat path bag file contents"},
      {"programs/proof/747_sys_writeall.cubalc", "sys_writeall",
       "SYS WRITEALL/WRITEFILES overwrite path bag with data"},
      {"programs/proof/748_sys_replacefiles.cubalc", "sys_replacefiles",
       "SYS REPLACEFILES/SUBFILES bulk rewrite path bag content"},
      {"programs/proof/749_sys_countinfiles.cubalc", "sys_countinfiles",
       "SYS COUNTINFILES/GREPCOUNTFILES total needle hits in path bag"},
      {"programs/proof/750_sys_firstfile.cubalc", "sys_firstfile",
       "SYS FIRSTFILE/LASTFILE first/last path with content match"},
      {"programs/proof/751_sys_greplines.cubalc", "sys_greplines",
       "SYS GREPLINES/EXTRACTLINES collect matching lines from path bag"},
      {"programs/proof/752_sys_headfile.cubalc", "sys_headfile",
       "SYS HEADFILE/TAILFILE first/last n lines of a file"},
      {"programs/proof/753_sys_linecountall.cubalc", "sys_linecountall",
       "SYS LINECOUNTALL/WCALL path bag line counts + sum"},
      {"programs/proof/754_sys_midfile.cubalc", "sys_midfile",
       "SYS MIDFILE/LINESLICE file line window extract"},
      {"programs/proof/755_sys_lineat.cubalc", "sys_lineat",
       "SYS LINEAT/FILELINE single file line by index"},
      {"programs/proof/756_sys_setfileline.cubalc", "sys_setfileline",
       "SYS SETFILELINE/FILESETLINE replace file line by index"},
      {"programs/proof/757_sys_insertfileline.cubalc", "sys_insertfileline",
       "SYS INSERTFILELINE/FILEINSERTLINE insert file line by index"},
      {"programs/proof/758_sys_dropfileline.cubalc", "sys_dropfileline",
       "SYS DROPFILELINE/FILEDROPLINE drop file line by index"},
      {"programs/proof/759_sys_findfileline.cubalc", "sys_findfileline",
       "SYS FINDFILELINE/FILEFINDLINE first file line index by needle"},
      {"programs/proof/760_sys_lastfindfileline.cubalc", "sys_lastfindfileline",
       "SYS LASTFINDFILELINE/FILELASTFIND last file line by needle"},
      {"programs/proof/761_sys_countfileline.cubalc", "sys_countfileline",
       "SYS COUNTFILELINE/FILECOUNTLINE count matching file lines"},
      {"programs/proof/762_sys_grepfile.cubalc", "sys_grepfile",
       "SYS GREPFILE/FILEGREP keep matching lines from one file"},
      {"programs/proof/763_sys_setmatchfile.cubalc", "sys_setmatchfile",
       "SYS SETMATCHFILE/FILESETMATCH replace first matching file line"},
      {"programs/proof/764_sys_dropmatchfile.cubalc", "sys_dropmatchfile",
       "SYS DROPMATCHFILE/FILEDROPMATCH drop first matching file line"},
      {"programs/proof/765_sys_dropallmatchfile.cubalc", "sys_dropallmatchfile",
       "SYS DROPALLMATCHFILE/FILEDROPALLMATCH drop all matching file lines"},
      {"programs/proof/766_sys_setallmatchfile.cubalc", "sys_setallmatchfile",
       "SYS SETALLMATCHFILE/FILESETALLMATCH replace all matching file lines"},
      {"programs/proof/767_sys_insertmatchfile.cubalc", "sys_insertmatchfile",
       "SYS INSERTMATCHFILE/FILEINSERTMATCH insert line after first match"},
      {"programs/proof/768_sys_beforematchfile.cubalc", "sys_beforematchfile",
       "SYS BEFOREMATCHFILE/INSERTBEFOREMATCH insert line before first match"},
      {"programs/proof/769_sys_lookupfile.cubalc", "sys_lookupfile",
       "SYS LOOKUPFILE/FILELOOKUP peel key=val from plate file"},
      {"programs/proof/770_sys_kvfileset.cubalc", "sys_kvfileset",
       "SYS KVFILESET/FILEKVSET set/update key=val in plate file"},
      {"programs/proof/771_sys_kvfiledel.cubalc", "sys_kvfiledel",
       "SYS KVFILEDEL/FILEKVDEL drop key=val line from plate file"},
      {"programs/proof/772_sys_kvfileinc.cubalc", "sys_kvfileinc",
       "SYS KVFILEINC/INCKVFILE bump numeric key=val in plate file"},
      {"programs/proof/773_sys_ensureline.cubalc", "sys_ensureline",
       "SYS ENSURELINE/ENSUREFILELINE append line if exact missing"},
      {"programs/proof/774_sys_hasfileline.cubalc", "sys_hasfileline",
       "SYS HASFILELINE/FILEHASLINE exact file line membership"},
      {"programs/proof/775_sys_dropexactfile.cubalc", "sys_dropexactfile",
       "SYS DROPEXACTFILE/DROPFILEEXACT drop first exact file line"},
      {"programs/proof/776_sys_toggleline.cubalc", "sys_toggleline",
       "SYS TOGGLELINE/TOGGLEFILELINE flip exact file line presence"},
      {"programs/proof/777_sys_kvfileensure.cubalc", "sys_kvfileensure",
       "SYS KVFILEENSURE/ENSUREKVFILE set key=val only if missing"},
      {"programs/proof/778_sys_substenv.cubalc", "sys_substenv",
       "SYS SUBSTENV/ENVSUBST expand $VAR and ${VAR} templates"},
      {"programs/proof/781_sys_waitfile.cubalc", "sys_waitfile",
       "SYS WAITFILE/WAITPATH poll until path exists"},
      {"programs/proof/782_sys_substfile.cubalc", "sys_substfile",
       "SYS SUBSTFILE/EXPANDFILE materialize $VAR template file"},
      {"programs/proof/783_sys_hash.cubalc", "sys_hash",
       "SYS HASH/HASHFILE FNV-1a plate content fingerprint"},
      {"programs/proof/784_sys_waitmatch.cubalc", "sys_waitmatch",
       "SYS WAITMATCH/WAITCONTAINS poll until file contains needle"},
      {"programs/proof/785_sys_writeatomic.cubalc", "sys_writeatomic",
       "SYS WRITEATOMIC/SAFEWRITE temp+rename plate write"},
      {"programs/proof/786_sys_lockfile.cubalc", "sys_lockfile",
       "SYS LOCKFILE/UNLOCKFILE exclusive agent plate locks"},
      {"programs/proof/787_sys_waitgone.cubalc", "sys_waitgone",
       "SYS WAITGONE/WAITMISSING poll until path does not exist"},
      {"programs/proof/788_sys_waitstable.cubalc", "sys_waitstable",
       "SYS WAITSTABLE/WAITQUIET poll until size+mtime quiet"},
      {"programs/proof/789_sys_waitchanged.cubalc", "sys_waitchanged",
       "SYS WAITCHANGED/WAITMODIFIED poll until size/mtime change"},
      {"programs/proof/790_sys_mktemp.cubalc", "sys_mktemp",
       "SYS MKTEMP/TEMPFILE unique empty file under TMP"},
      {"programs/proof/791_sys_claim.cubalc", "sys_claim",
       "SYS CLAIM/CREATEEXCL exclusive path work-item claim"},
      {"programs/proof/792_sys_stat.cubalc", "sys_stat",
       "SYS STAT/FSTAT one-shot path metadata probe"},
      {"programs/proof/793_sys_mktempdir.cubalc", "sys_mktempdir",
       "SYS MKTEMPDIR/MKDTEMP unique empty dir under TMP"},
      {"programs/proof/794_sys_rmdir.cubalc", "sys_rmdir",
       "SYS RMDIR/REMOVEDIR remove empty directory"},
      {"programs/proof/795_sys_base64.cubalc", "sys_base64",
       "SYS BASE64/BASE64D encode/decode plate payloads"},
      {"programs/proof/796_sys_hex.cubalc", "sys_hex",
       "SYS HEX/HEXD encode/decode plate bytes"},
      {"programs/proof/797_sys_urlenc.cubalc", "sys_urlenc",
       "SYS URLENC/URLDEC percent-encode plate paths"},
      {"programs/proof/798_sys_uuid.cubalc", "sys_uuid",
       "SYS UUID/UUID0 RFC4122 v4 agent work ids"},
      {"programs/proof/799_sys_jsonesc.cubalc", "sys_jsonesc",
       "SYS JSONESC/JSONUNESC plate string escape"},
      {"programs/proof/800_sys_crc32.cubalc", "sys_crc32",
       "SYS CRC32/CRC32FILE IEEE integrity stamps"},
      {"programs/proof/801_sys_symlink.cubalc", "sys_symlink",
       "SYS SYMLINK/READLINK plate path aliases"},
      {"programs/proof/802_sys_humansize.cubalc", "sys_humansize",
       "SYS HUMANSIZE integer B/K/M/G size labels"},
      {"programs/proof/803_sys_islink.cubalc", "sys_islink",
       "SYS ISLINK probe symlink paths"},
      {"programs/proof/804_sys_mode.cubalc", "sys_mode",
       "SYS MODE/CHMOD plate permission probe/set"},
      {"programs/proof/805_sys_parsesize.cubalc", "sys_parsesize",
       "SYS PARSESIZE human size string to bytes"},
      {"programs/proof/806_sys_monotonic.cubalc", "sys_monotonic",
       "SYS MONOTONIC steady ms for elapsed timing"},
      {"programs/proof/807_sys_nproc.cubalc", "sys_nproc",
       "SYS NPROC/CPUS online processor count"},
      {"programs/proof/808_sys_uname.cubalc", "sys_uname",
       "SYS UNAME/OS/ARCH/PLATFORM host identity"},
      {"programs/proof/809_sys_local.cubalc", "sys_local",
       "SYS LOCAL/LOCALTIME local wall stamp"},
      {"programs/proof/810_require_bin.cubalc", "require_bin",
       "REQUIRE BIN/SYS WHICHBIN PATH tool gates (+ string-var names)"},
      {"programs/proof/811_sys_isatty.cubalc", "sys_isatty",
       "SYS ISATTY/TTY terminal stream probe"},
      {"programs/proof/812_sys_loadavg.cubalc", "sys_loadavg",
       "SYS LOADAVG/LOAD host load averages"},
      {"programs/proof/813_sys_uptime.cubalc", "sys_uptime",
       "SYS UPTIME/BOOTAGE seconds since boot"},
      {"programs/proof/814_sys_mem.cubalc", "sys_mem",
       "SYS MEM/MEMINFO host RAM total/avail kB"},
      {"programs/proof/815_sys_df.cubalc", "sys_df",
       "SYS DF/DISKFREE filesystem free kB"},
      {"programs/proof/816_sys_fsync.cubalc", "sys_fsync",
       "SYS FSYNC/SYNCFILE durable file flush"},
      {"programs/proof/817_sys_dotenv.cubalc", "sys_dotenv",
       "SYS DOTENV/LOADENV load KEY=VAL plate into env"},
      {"programs/proof/818_sys_dumpenv.cubalc", "sys_dumpenv",
       "SYS DUMPENV/ENVALL process env KEY=VAL bag"},
      {"programs/proof/819_sys_writeenv.cubalc", "sys_writeenv",
       "SYS WRITEENV/SAVENV persist env KEY=VAL plate"},
      {"programs/proof/820_sys_truncate.cubalc", "sys_truncate",
       "SYS TRUNCATE/FTRUNCATE resize plate files"},
      {"programs/proof/821_sys_dirsync.cubalc", "sys_dirsync",
       "SYS DIRSYNC/FSYNCDIR durable dir flush"},
      {"programs/proof/822_sys_hardlink.cubalc", "sys_hardlink",
       "SYS HARDLINK/NLINK shared plate inodes"},
      {"programs/proof/823_sys_sameinode.cubalc", "sys_sameinode",
       "SYS SAMEINODE/INODE hard-link identity"},
      {"programs/proof/824_sys_tee.cubalc", "sys_tee",
       "SYS TEE write keep content in LAST"},
      {"programs/proof/825_sys_fileuid.cubalc", "sys_fileuid",
       "SYS FILEUID/FILEGID/OWNEDBY ownership probes"},
      {"programs/proof/826_sys_canread.cubalc", "sys_canread",
       "SYS CANREAD/CANWRITE/CANEXEC access probes"},
      {"programs/proof/827_sys_ownername.cubalc", "sys_ownername",
       "SYS OWNERNAME/GROUPNAME login and group name probes"},
      {"programs/proof/828_sys_cancreate.cubalc", "sys_cancreate",
       "SYS CANCREATE pre-flight create/overwrite probe"},
      {"programs/proof/829_sys_umask.cubalc", "sys_umask",
       "SYS UMASK get/set process file-creation mask"},
      {"programs/proof/830_sys_pushd.cubalc", "sys_pushd",
       "SYS PUSHD/POPD/DIRSTACK cwd stack for temp chdir"},
      {"programs/proof/831_sys_kindstr.cubalc", "sys_kindstr",
       "SYS KINDSTR human path kind labels"},
      {"programs/proof/832_sys_relpath.cubalc", "sys_relpath",
       "SYS RELPATH base path relative plate paths"},
      {"programs/proof/833_sys_atime.cubalc", "sys_atime",
       "SYS ATIME/CTIME access and status-change times"},
      {"programs/proof/834_sys_hasenv.cubalc", "sys_hasenv",
       "SYS HASENV soft env presence probe"},
      {"programs/proof/835_sys_gid.cubalc", "sys_gid",
       "SYS GID/EGID/GROUP process group identity"},
      {"programs/proof/836_sys_groups.cubalc", "sys_groups",
       "SYS GROUPS/INGROUP process group membership"},
      {"programs/proof/837_sys_ppid.cubalc", "sys_ppid",
       "SYS PPID/PGID/SID process lineage and session"},
      {"programs/proof/838_sys_euid_isroot.cubalc", "sys_euid_isroot",
       "SYS EUID/ISROOT effective uid and root probe"},
      {"programs/proof/839_sys_shell.cubalc", "sys_shell",
       "SYS SHELL login shell path without shell"},
      {"programs/proof/840_sys_gecos.cubalc", "sys_gecos",
       "SYS GECOS/FULLNAME passwd display name"},
      {"programs/proof/841_sys_ttyname.cubalc", "sys_ttyname",
       "SYS TTYNAME/CTTY terminal device path"},
      {"programs/proof/842_sys_nice.cubalc", "sys_nice",
       "SYS NICE/RENICE process priority get/set"},
      {"programs/proof/843_sys_shquote.cubalc", "sys_shquote",
       "SYS SHQUOTE/SHELLQUOTE POSIX shell-safe wrap"},
      {"programs/proof/844_sys_shunquote.cubalc", "sys_shunquote",
       "SYS SHUNQUOTE/UNQUOTE peel shell quote wrap"},
      {"programs/proof/845_sys_getlogin.cubalc", "sys_getlogin",
       "SYS GETLOGIN/LOGINNAME session login name"},
      {"programs/proof/846_sys_entropy.cubalc", "sys_entropy",
       "SYS ENTROPY/URANDOM random bytes as hex"},
      {"programs/proof/847_sys_parsems.cubalc", "sys_parsems",
       "SYS PARSEMS/DURATION compound human duration to ms"},
      {"programs/proof/848_sys_humanms.cubalc", "sys_humanms",
       "SYS HUMANMS/FMTMS ms to human duration"},
      {"programs/proof/849_sys_fromtime.cubalc", "sys_fromtime",
       "SYS FROMTIME/EPOCHISO epoch to UTC ISO"},
      {"programs/proof/850_sys_parseiso.cubalc", "sys_parseiso",
       "SYS PARSEISO/TOEPOCH ISO stamp to epoch"},
      {"programs/proof/851_sys_timediff.cubalc", "sys_timediff",
       "SYS TIMEDIFF/ELAPSED epoch second deltas"},
      {"programs/proof/852_sys_envdefault.cubalc", "sys_envdefault",
       "SYS ENVDEFAULT setenv if missing/empty"},
      {"programs/proof/853_sys_addtime.cubalc", "sys_addtime",
       "SYS ADDTIME/DEADLINE epoch plus seconds"},
      {"programs/proof/854_sys_expired.cubalc", "sys_expired",
       "SYS EXPIRED/REMAINING lease due and TTL"},
      {"programs/proof/855_sys_ms2sec.cubalc", "sys_ms2sec",
       "SYS MS2SEC/SEC2MS ms-seconds unit bridge"},
      {"programs/proof/856_sys_parsems_compound.cubalc", "sys_parsems_compound",
       "SYS PARSEMS compound 1h30m HUMANMS round-trip"},
      {"programs/proof/857_sys_sleep_duration.cubalc", "sys_sleep_duration",
       "SYS SLEEP human duration string without PARSEMS glue"},
      {"programs/proof/858_sys_waitfile_duration.cubalc", "sys_waitfile_duration",
       "SYS WAITFILE timeout human duration string"},
      {"programs/proof/859_sys_deadline_duration.cubalc", "sys_deadline_duration",
       "SYS DEADLINE/ADDTIME human duration lease offsets"},
      {"programs/proof/860_sys_addiso.cubalc", "sys_addiso",
       "SYS ADDISO shift ISO stamp by secs or duration"},
      {"programs/proof/861_sys_diffiso.cubalc", "sys_diffiso",
       "SYS DIFFISO/ISOAGE ISO stamp second deltas"},
      {"programs/proof/591_sys_ms.cubalc", "sys_ms",
       "SYS MS wall milliseconds for agent timing"},
      {"programs/proof/592_note.cubalc", "note",
       "NOTE agent breadcrumb without OK/ERR change"},
      {"programs/proof/593_exit.cubalc", "exit",
       "EXIT [code] early program halt"},
      {"programs/proof/594_include_once.cubalc", "include_once",
       "INCLUDE ONCE skips already-loaded module"},
      {"programs/proof/595_sys_pid_hostname.cubalc", "sys_pid_host",
       "SYS PID and HOSTNAME host identity"},
      {"programs/proof/596_sys_user_uid_home.cubalc", "sys_user",
       "SYS USER UID HOME user identity without shell"},
      {"programs/proof/597_default.cubalc", "default",
       "DEFAULT name = value set-only-if-unset"},
      {"programs/proof/598_defined.cubalc", "defined",
       "DEFINED name var existence probe"},
      {"programs/proof/599_typeof.cubalc", "typeof",
       "TYPEOF name num|str|undef kind probe"},
      {"programs/proof/600_unset.cubalc", "unset",
       "UNSET name remove var for DEFAULT re-apply"},
      {"programs/proof/601_identity.cubalc", "identity",
       "IDENTITY cubalc.identity.v1 host plate"},
      {"programs/proof/602_sys_date.cubalc", "sys_date",
       "SYS DATE UTC ISO plate stamp"},
      {"programs/proof/603_sys_tmp.cubalc", "sys_tmp",
       "SYS TMP portable temp directory"},
      {"programs/proof/604_sys_mkdir.cubalc", "sys_mkdir",
       "SYS MKDIR mkdir -p for agent plate dirs"},
      {"programs/proof/605_sys_basename.cubalc", "sys_basename",
       "SYS BASENAME/DIRNAME path split"},
      {"programs/proof/606_sys_extname.cubalc", "sys_extname",
       "SYS EXTNAME/STEM extension and stem peel"},
      {"programs/proof/607_sys_size.cubalc", "sys_size",
       "SYS SIZE/ISDIR/ISFILE path metadata probes"},
      {"programs/proof/608_sys_read_soft.cubalc", "sys_read_soft",
       "SYS READ OR soft optional plate + fallback"},
      {"programs/proof/609_sys_rm_rename.cubalc", "sys_rm_rename",
       "SYS RM/RENAME plate file lifecycle"},
      {"programs/proof/610_sys_copy.cubalc", "sys_copy",
       "SYS COPY duplicate plate files"},
      {"programs/proof/611_sys_realpath.cubalc", "sys_realpath",
       "SYS REALPATH/ABSPATH absolute paths"},
      {"programs/proof/612_sys_touch.cubalc", "sys_touch",
       "SYS TOUCH create plate markers"},
      {"programs/proof/613_sys_sleep.cubalc", "sys_sleep",
       "SYS SLEEP agent backoff milliseconds"},
      {"programs/proof/614_sys_list.cubalc", "sys_list",
       "SYS LIST/LS directory basenames"},
      {"programs/proof/615_sys_nth.cubalc", "sys_nth",
       "SYS NTH/LINE/HEAD/TAIL newline field peel"},
      {"programs/proof/616_sys_grep.cubalc", "sys_grep",
       "SYS GREP/FILTER/GREPV newline field filter"},
      {"programs/proof/617_each_line.cubalc", "each_line",
       "EACH LINE walk newline fields after LIST/GREP"},
      {"programs/proof/618_sys_take.cubalc", "sys_take",
       "SYS TAKE/DROP newline field windows"},
      {"programs/proof/619_sys_split.cubalc", "sys_split",
       "SYS SPLIT sep into newline fields for EACH LINE"},
      {"programs/proof/620_sys_sort.cubalc", "sys_sort",
       "SYS SORT/UNIQ stable line order for LIST walks"},
      {"programs/proof/621_sys_join.cubalc", "sys_join",
       "SYS JOIN/PATH portable plate path join"},
      {"programs/proof/622_sys_joinlines.cubalc", "sys_joinlines",
       "SYS JOINLINES/PASTE join newline fields with sep"},
      {"programs/proof/623_sys_replaceall.cubalc", "sys_replaceall",
       "SYS REPLACEALL/GSUB all-occurrence string subst"},
      {"programs/proof/624_sys_append.cubalc", "sys_append",
       "SYS APPEND/LOG append line for plate history"},
      {"programs/proof/625_sys_grepi.cubalc", "sys_grepi",
       "SYS GREPI case-insensitive newline filter"},
      {"programs/proof/626_sys_str.cubalc", "sys_str",
       "SYS STR/ITOA integer to decimal string for templates"},
      {"programs/proof/627_sys_before.cubalc", "sys_before",
       "SYS BEFORE/AFTER peel at first needle for log/kv"},
      {"programs/proof/628_sys_push.cubalc", "sys_push",
       "SYS PUSH/ADDLINE newline field accumulate"},
      {"programs/proof/629_sys_eqsi.cubalc", "sys_eqsi",
       "SYS EQSI/HASI case-insensitive string probes"},
      {"programs/proof/630_sys_lines.cubalc", "sys_lines",
       "SYS LINES/WC count newline fields"},
      {"programs/proof/631_sys_cut.cubalc", "sys_cut",
       "SYS CUT/COLUMN peel field by separator"},
      {"programs/proof/632_sys_empty.cubalc", "sys_empty",
       "SYS EMPTY/BLANK/NONEMPTY soft plate probes"},
      {"programs/proof/633_sys_pop.cubalc", "sys_pop",
       "SYS POP/POPLINE peel last bag field"},
      {"programs/proof/634_sys_mtime.cubalc", "sys_mtime",
       "SYS MTIME/AGE plate freshness without shell"},
      {"programs/proof/635_sys_startsi.cubalc", "sys_startsi",
       "SYS STARTSI/ENDSI case-insensitive prefix/suffix"},
      {"programs/proof/636_sys_between.cubalc", "sys_between",
       "SYS BETWEEN/MIDOF/EXTRACT peel between delimiters"},
      {"programs/proof/637_sys_findi.cubalc", "sys_findi",
       "SYS FINDI/INDEXI case-insensitive string index"},
      {"programs/proof/638_sys_env_set.cubalc", "sys_env_set",
       "SYS ENV SET/UNSET process env without shell"},
      {"programs/proof/639_sys_revl.cubalc", "sys_revl",
       "SYS REVL/REVLINES reverse bag line order"},
      {"programs/proof/640_sys_words.cubalc", "sys_words",
       "SYS WORDS/TOKENIZE whitespace to newline fields"},
      {"programs/proof/641_sys_hasline.cubalc", "sys_hasline",
       "SYS HASLINE exact bag field membership"},
      {"programs/proof/642_sys_prepend.cubalc", "sys_prepend",
       "SYS PREPEND/POPHEAD FIFO bag front ops"},
      {"programs/proof/643_sys_removeline.cubalc", "sys_removeline",
       "SYS REMOVELINE drop first exact bag field"},
      {"programs/proof/644_sys_findline.cubalc", "sys_findline",
       "SYS FINDLINE exact bag field index"},
      {"programs/proof/645_sys_setline.cubalc", "sys_setline",
       "SYS SETLINE set bag field by index"},
      {"programs/proof/646_sys_insertline.cubalc", "sys_insertline",
       "SYS INSERTLINE insert bag field by index"},
      {"programs/proof/647_sys_dropnth.cubalc", "sys_dropnth",
       "SYS DROPNTH drop bag field by index"},
      {"programs/proof/648_sys_moveline.cubalc", "sys_moveline",
       "SYS MOVELINE move bag field by index"},
      {"programs/proof/649_sys_countline.cubalc", "sys_countline",
       "SYS COUNTLINE count exact bag field matches"},
      {"programs/proof/650_sys_setmatch.cubalc", "sys_setmatch",
       "SYS SETMATCH replace first exact bag field"},
    };
    const char *prefix = (argc > 2) ? argv[2] : "";
    int json_only = 0;
    int i, nmatch = 0, nall = (int)(sizeof catalog / sizeof catalog[0]);
    int present = 0;
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
      printf("# CubalC starter examples prefix=%s version=%s\n",
             pref_up[0] ? pref_up : "*", CUBALC_LANG_VERSION);
      printf("# run: cubalc run <path>\n");
      printf("# tag\tpath\thint\n");
    }
    for (i = 0; i < nall; i++) {
      char path_up[160], tag_up[32];
      size_t k;
      int hit = 1, exists;
      for (k = 0; catalog[i].path[k] && k + 1 < sizeof path_up; k++)
        path_up[k] = (char)toupper((unsigned char)catalog[i].path[k]);
      path_up[k] = 0;
      for (k = 0; catalog[i].tag[k] && k + 1 < sizeof tag_up; k++)
        tag_up[k] = (char)toupper((unsigned char)catalog[i].tag[k]);
      tag_up[k] = 0;
      if (pref_up[0] && !strstr(path_up, pref_up) && !strstr(tag_up, pref_up) &&
          !strstr(catalog[i].hint, prefix))
        hit = 0;
      if (!hit) continue;
      nmatch++;
      exists = (access(catalog[i].path, R_OK) == 0);
      if (exists) present++;
      if (!json_only)
        printf("%s\t%s\t%s%s\n", catalog[i].tag, catalog[i].path,
               catalog[i].hint, exists ? "" : " (missing)");
    }
    printf("{\"schema\":\"cubalc.examples.v1\",\"ok\":%s,\"cmd\":\"examples\","
           "\"prefix\":\"%s\",\"n\":%d,\"n_catalog\":%d,\"n_present\":%d,"
           "\"version\":\"%s\","
           "\"note\":\"curated starters — cubalc run <path>; see docs/COOKBOOK.md\","
           "\"examples\":[",
           (nmatch > 0 && present > 0) ? "true" : "false",
           pref_up[0] ? pref_up : "", nmatch, nall, present,
           CUBALC_LANG_VERSION);
    {
      int first = 1;
      for (i = 0; i < nall; i++) {
        char path_up[160], tag_up[32];
        size_t k;
        int hit = 1, exists;
        for (k = 0; catalog[i].path[k] && k + 1 < sizeof path_up; k++)
          path_up[k] = (char)toupper((unsigned char)catalog[i].path[k]);
        path_up[k] = 0;
        for (k = 0; catalog[i].tag[k] && k + 1 < sizeof tag_up; k++)
          tag_up[k] = (char)toupper((unsigned char)catalog[i].tag[k]);
        tag_up[k] = 0;
        if (pref_up[0] && !strstr(path_up, pref_up) && !strstr(tag_up, pref_up) &&
            !strstr(catalog[i].hint, prefix))
          hit = 0;
        if (!hit) continue;
        exists = (access(catalog[i].path, R_OK) == 0);
        printf("%s{\"path\":\"%s\",\"tag\":\"%s\",\"hint\":\"%s\",\"present\":%s}",
               first ? "" : ",", catalog[i].path, catalog[i].tag,
               catalog[i].hint, exists ? "true" : "false");
        first = 0;
      }
    }
    printf("]}\n");
    return (nmatch > 0 && present > 0) ? 0 : 1;
  }
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
  if (strcmp(cmd, "recipe") == 0 || strcmp(cmd, "card") == 0 ||
      strcmp(cmd, "libcard") == 0 || strcmp(cmd, "lib-card") == 0 ||
      strcmp(cmd, "describe-lib") == 0) {
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
  }
  if (strcmp(cmd, "checkdeps") == 0 || strcmp(cmd, "deps") == 0 ||
      strcmp(cmd, "hasdeps") == 0 || strcmp(cmd, "needdeps") == 0 ||
      strcmp(cmd, "verifydeps") == 0 || strcmp(cmd, "libdeps-check") == 0) {
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
  }
  if (strcmp(cmd, "picklib") == 0 || strcmp(cmd, "firstlib") == 0 ||
      strcmp(cmd, "countmatchlibs") == 0 || strcmp(cmd, "countlibs") == 0 ||
      strcmp(cmd, "hasmatchlibs") == 0 || strcmp(cmd, "needmatchlibs") == 0 ||
      strcmp(cmd, "lastlib") == 0 || strcmp(cmd, "nthlib") == 0 ||
      strcmp(cmd, "newestlib") == 0 || strcmp(cmd, "latestlib") == 0 ||
      strcmp(cmd, "oldestlib") == 0 || strcmp(cmd, "earliestlib") == 0 ||
      strcmp(cmd, "sortlibs") == 0 || strcmp(cmd, "libsort") == 0 ||
      strcmp(cmd, "sortlib") == 0 || strcmp(cmd, "libsbymtime") == 0 ||
      strcmp(cmd, "libpick") == 0 || strcmp(cmd, "libcount") == 0) {
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
  }
  if (strcmp(cmd, "freshlibs") == 0 || strcmp(cmd, "recentlibs") == 0 ||
      strcmp(cmd, "stalelibs") == 0 || strcmp(cmd, "olderlibs") == 0 ||
      strcmp(cmd, "keepfreshlibs") == 0 || strcmp(cmd, "keepstalelibs") == 0) {
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
  }
  if (strcmp(cmd, "libage") == 0 || strcmp(cmd, "ageoflib") == 0 ||
      strcmp(cmd, "hasfresh") == 0 || strcmp(cmd, "isfresh") == 0 ||
      strcmp(cmd, "needfresh") == 0 || strcmp(cmd, "requirefresh") == 0) {
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
  }
  if (strcmp(cmd, "hasflagall") == 0 || strcmp(cmd, "hasallflags") == 0 ||
      strcmp(cmd, "needflags") == 0 || strcmp(cmd, "requireflags") == 0 ||
      strcmp(cmd, "mustflags") == 0 ||
      strcmp(cmd, "hasflagany") == 0 || strcmp(cmd, "hasanyflags") == 0 ||
      strcmp(cmd, "needflagany") == 0 || strcmp(cmd, "requireflagany") == 0 ||
      strcmp(cmd, "mustflagany") == 0 || strcmp(cmd, "needanyflags") == 0 ||
      strcmp(cmd, "hasargall") == 0 || strcmp(cmd, "hasallargs") == 0 ||
      strcmp(cmd, "needargs") == 0 || strcmp(cmd, "requireargs") == 0 ||
      strcmp(cmd, "mustargs") == 0 ||
      strcmp(cmd, "hasargany") == 0 || strcmp(cmd, "hasanyargs") == 0 ||
      strcmp(cmd, "needargany") == 0 || strcmp(cmd, "requireargany") == 0 ||
      strcmp(cmd, "mustargany") == 0 || strcmp(cmd, "needanyargs") == 0) {
    /* Usability: CLI dual of HASFLAGALL/NEEDFLAGS · HASFLAGANY/NEEDFLAGANY ·
     * HASARGALL/NEEDARGS · HASARGANY/NEEDARGANY.
     * Optional live argv after -- :
     *   cubalc needflags out verbose -- --out=x --verbose a.txt
     *   cubalc hasflagany json yaml -- --json a.txt
     *   cubalc needargs 0 1 -- file0 file1
     *   cubalc hasargany 0 FILE -- file0
     * Schema cubalc.flaggate.v1 / cubalc.arggate.v1 */
    int is_arg = (strcmp(cmd, "hasargall") == 0 || strcmp(cmd, "hasallargs") == 0 ||
                  strcmp(cmd, "needargs") == 0 || strcmp(cmd, "requireargs") == 0 ||
                  strcmp(cmd, "mustargs") == 0 ||
                  strcmp(cmd, "hasargany") == 0 || strcmp(cmd, "hasanyargs") == 0 ||
                  strcmp(cmd, "needargany") == 0 || strcmp(cmd, "requireargany") == 0 ||
                  strcmp(cmd, "mustargany") == 0 || strcmp(cmd, "needanyargs") == 0);
    int is_any = (strcmp(cmd, "hasflagany") == 0 || strcmp(cmd, "hasanyflags") == 0 ||
                  strcmp(cmd, "needflagany") == 0 || strcmp(cmd, "requireflagany") == 0 ||
                  strcmp(cmd, "mustflagany") == 0 || strcmp(cmd, "needanyflags") == 0 ||
                  strcmp(cmd, "hasargany") == 0 || strcmp(cmd, "hasanyargs") == 0 ||
                  strcmp(cmd, "needargany") == 0 || strcmp(cmd, "requireargany") == 0 ||
                  strcmp(cmd, "mustargany") == 0 || strcmp(cmd, "needanyargs") == 0);
    int hard = (strcmp(cmd, "needflags") == 0 || strcmp(cmd, "requireflags") == 0 ||
                strcmp(cmd, "mustflags") == 0 ||
                strcmp(cmd, "needflagany") == 0 || strcmp(cmd, "requireflagany") == 0 ||
                strcmp(cmd, "mustflagany") == 0 || strcmp(cmd, "needanyflags") == 0 ||
                strcmp(cmd, "needargs") == 0 || strcmp(cmd, "requireargs") == 0 ||
                strcmp(cmd, "mustargs") == 0 ||
                strcmp(cmd, "needargany") == 0 || strcmp(cmd, "requireargany") == 0 ||
                strcmp(cmd, "mustargany") == 0 || strcmp(cmd, "needanyargs") == 0);
    char src[1200];
    char names[32][96];
    int nname = 0, i, ai, nlive = 0, hit = 0;
    size_t o = 0;
    cubalc_run_result rr;
    char envn[32];
    const char *form;
    /* collect names until -- */
    for (i = 2; i < argc && nname < 32; i++) {
      if (!argv[i] || !argv[i][0]) continue;
      if (!strcmp(argv[i], "--")) {
        i++;
        break;
      }
      snprintf(names[nname++], sizeof names[0], "%s", argv[i]);
    }
    /* live args after -- → CUBALC_ARGn */
    for (; i < argc && nlive < 32; i++, nlive++) {
      snprintf(envn, sizeof envn, "CUBALC_ARG%d", nlive);
      setenv(envn, argv[i] ? argv[i] : "", 1);
    }
    if (nlive > 0) {
      snprintf(envn, sizeof envn, "%d", nlive);
      setenv("CUBALC_ARGC", envn, 1);
    }
    if (nname == 0) {
      fprintf(stderr,
              "usage: cubalc hasflagall|needflags <flag> [flag…] [-- args…]\n"
              "       cubalc hasflagany|needflagany <flag> [flag…] [-- args…]\n"
              "       cubalc hasargall|needargs <n|name> [n|name…] [-- args…]\n"
              "       cubalc hasargany|needargany <n|name> [n|name…] [-- args…]\n");
      printf("{\"schema\":\"cubalc.%sgate.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need names\",\"version\":\"%s\"}\n",
             is_arg ? "arg" : "flag", cmd, CUBALC_LANG_VERSION);
      return 2;
    }
    if (is_arg && is_any)
      form = hard ? "NEEDARGANY" : "HASARGANY";
    else if (is_arg)
      form = hard ? "NEEDARGS" : "HASARGALL";
    else if (is_any)
      form = hard ? "NEEDFLAGANY" : "HASFLAGANY";
    else
      form = hard ? "NEEDFLAGS" : "HASFLAGALL";
    o = (size_t)snprintf(src, sizeof src, "%s", form);
    for (ai = 0; ai < nname && o + 2 < sizeof src; ai++) {
      const char *nm = names[ai];
      int need_q = 0, c;
      for (c = 0; nm[c]; c++) {
        if (nm[c] == ' ' || nm[c] == '"' || nm[c] == '\\') {
          need_q = 1;
          break;
        }
      }
      if (need_q)
        o += (size_t)snprintf(src + o, sizeof src - o, " \"%s\"", nm);
      else
        o += (size_t)snprintf(src + o, sizeof src - o, " %s", nm);
    }
    /* PRINT LAST_N so soft has* plate can read 0|1 from last_print */
    o += (size_t)snprintf(src + o, sizeof src - o, "\nPRINT LAST_N\nPASS\n");
    (void)o;
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), is_arg ? "<cli-arggate>" : "<cli-flaggate>",
                            &rr, NULL);
    /* hard need*: fail if runtime err (NEEDFLAGS/NEEDFLAGANY/NEEDARGS fail_at).
     * soft has*: LAST_N printed as 0|1. */
    if (hard)
      hit = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    else
      hit = (rr.ok && !rr.err[0] && rr.last_print[0] == '1') ? 1 : 0;
    printf("{\"schema\":\"cubalc.%sgate.v1\",\"ok\":%s,\"cmd\":\"%s\","
           "\"mode\":\"%s\",\"any\":%s,\"n\":%d,\"live_args\":%d,\"version\":\"%s\","
           "\"note\":\"CLI dual of %s · optional -- live argv\","
           "\"names\":[",
           is_arg ? "arg" : "flag",
           hit ? "true" : "false", cmd,
           hard ? "need" : "has", is_any ? "true" : "false", nname, nlive,
           CUBALC_LANG_VERSION, form);
    for (ai = 0; ai < nname; ai++) {
      char esc[120];
      size_t e = 0, k;
      const char *nm = names[ai];
      for (k = 0; nm[k] && e + 2 < sizeof esc; k++) {
        char c = nm[k];
        if (c == '"' || c == '\\') {
          esc[e++] = '\\';
          esc[e++] = c;
        } else if ((unsigned char)c < 32)
          esc[e++] = ' ';
        else
          esc[e++] = c;
      }
      esc[e] = 0;
      printf("%s\"%s\"", ai ? "," : "", esc);
    }
    printf("]");
    if (!hit && rr.err[0]) {
      char eesc[200];
      size_t e = 0, k;
      for (k = 0; rr.err[k] && e + 2 < sizeof eesc; k++) {
        char c = rr.err[k];
        if (c == '"' || c == '\\') {
          eesc[e++] = '\\';
          eesc[e++] = c;
        } else if ((unsigned char)c < 32)
          eesc[e++] = ' ';
        else
          eesc[e++] = c;
      }
      eesc[e] = 0;
      printf(",\"err\":\"%s\"", eesc);
    }
    printf("}\n");
    if (hard)
      return hit ? 0 : 1;
    return 0;
  }
  if (strcmp(cmd, "hasenvall") == 0 || strcmp(cmd, "hasallenv") == 0 ||
      strcmp(cmd, "needenvs") == 0 || strcmp(cmd, "requireenvs") == 0 ||
      strcmp(cmd, "mustenvs") == 0 || strcmp(cmd, "needenvall") == 0 ||
      strcmp(cmd, "requireenvall") == 0 ||
      strcmp(cmd, "hasenvany") == 0 || strcmp(cmd, "hasanyenv") == 0 ||
      strcmp(cmd, "needenvany") == 0 || strcmp(cmd, "requireenvany") == 0 ||
      strcmp(cmd, "mustenvany") == 0 || strcmp(cmd, "needanyenv") == 0) {
    /* Usability: CLI dual of HASENVALL/NEEDENVS · HASENVANY/NEEDENVANY.
     *   cubalc hasenvall HOME USER PATH
     *   cubalc needenvs CUBALC_STATE HOME
     *   cubalc hasenvany DB_URL REDIS_URL
     *   cubalc needenvany HOST PORT
     * Schema cubalc.envgate.v1 */
    int is_any = (strcmp(cmd, "hasenvany") == 0 || strcmp(cmd, "hasanyenv") == 0 ||
                  strcmp(cmd, "needenvany") == 0 || strcmp(cmd, "requireenvany") == 0 ||
                  strcmp(cmd, "mustenvany") == 0 || strcmp(cmd, "needanyenv") == 0);
    int hard = (strcmp(cmd, "needenvs") == 0 || strcmp(cmd, "requireenvs") == 0 ||
                strcmp(cmd, "mustenvs") == 0 || strcmp(cmd, "needenvall") == 0 ||
                strcmp(cmd, "requireenvall") == 0 ||
                strcmp(cmd, "needenvany") == 0 || strcmp(cmd, "requireenvany") == 0 ||
                strcmp(cmd, "mustenvany") == 0 || strcmp(cmd, "needanyenv") == 0);
    char src[1200];
    char names[32][96];
    int nname = 0, i, ai, hit = 0;
    size_t o = 0;
    cubalc_run_result rr;
    const char *form;
    if (is_any)
      form = hard ? "NEEDENVANY" : "HASENVANY";
    else
      form = hard ? "NEEDENVS" : "HASENVALL";
    for (i = 2; i < argc && nname < 32; i++) {
      if (!argv[i] || !argv[i][0]) continue;
      if (!strcmp(argv[i], "--")) break;
      snprintf(names[nname++], sizeof names[0], "%s", argv[i]);
    }
    if (nname == 0) {
      fprintf(stderr,
              "usage: cubalc hasenvall|needenvs <ENV> [ENV…]\n"
              "       cubalc hasenvany|needenvany <ENV> [ENV…]\n");
      printf("{\"schema\":\"cubalc.envgate.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need names\",\"version\":\"%s\"}\n",
             cmd, CUBALC_LANG_VERSION);
      return 2;
    }
    o = (size_t)snprintf(src, sizeof src, "%s", form);
    for (ai = 0; ai < nname && o + 2 < sizeof src; ai++) {
      const char *nm = names[ai];
      int need_q = 0, c;
      for (c = 0; nm[c]; c++) {
        if (nm[c] == ' ' || nm[c] == '"' || nm[c] == '\\') {
          need_q = 1;
          break;
        }
      }
      if (need_q)
        o += (size_t)snprintf(src + o, sizeof src - o, " \"%s\"", nm);
      else
        o += (size_t)snprintf(src + o, sizeof src - o, " %s", nm);
    }
    o += (size_t)snprintf(src + o, sizeof src - o, "\nPRINT LAST_N\nPASS\n");
    (void)o;
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<cli-envgate>", &rr, NULL);
    if (hard)
      hit = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    else
      hit = (rr.ok && !rr.err[0] && rr.last_print[0] == '1') ? 1 : 0;
    printf("{\"schema\":\"cubalc.envgate.v1\",\"ok\":%s,\"cmd\":\"%s\","
           "\"mode\":\"%s\",\"any\":%s,\"n\":%d,\"version\":\"%s\","
           "\"note\":\"CLI dual of %s · multi host env %s gate\","
           "\"names\":[",
           hit ? "true" : "false", cmd,
           hard ? "need" : "has", is_any ? "true" : "false", nname,
           CUBALC_LANG_VERSION, form, is_any ? "ANY" : "ALL");
    for (ai = 0; ai < nname; ai++) {
      char esc[120];
      size_t e = 0, k;
      const char *nm = names[ai];
      for (k = 0; nm[k] && e + 2 < sizeof esc; k++) {
        char c = nm[k];
        if (c == '"' || c == '\\') {
          esc[e++] = '\\';
          esc[e++] = c;
        } else if ((unsigned char)c < 32)
          esc[e++] = ' ';
        else
          esc[e++] = c;
      }
      esc[e] = 0;
      printf("%s\"%s\"", ai ? "," : "", esc);
    }
    printf("]");
    if (!hit && rr.err[0]) {
      char eesc[200];
      size_t e = 0, k;
      for (k = 0; rr.err[k] && e + 2 < sizeof eesc; k++) {
        char c = rr.err[k];
        if (c == '"' || c == '\\') {
          eesc[e++] = '\\';
          eesc[e++] = c;
        } else if ((unsigned char)c < 32)
          eesc[e++] = ' ';
        else
          eesc[e++] = c;
      }
      eesc[e] = 0;
      printf(",\"err\":\"%s\"", eesc);
    }
    printf("}\n");
    if (hard)
      return hit ? 0 : 1;
    return 0;
  }
  if (strcmp(cmd, "hasform") == 0 || strcmp(cmd, "formexists") == 0 ||
      strcmp(cmd, "needform") == 0 || strcmp(cmd, "requireform") == 0 ||
      strcmp(cmd, "knownform") == 0 ||
      strcmp(cmd, "hasforms") == 0 || strcmp(cmd, "needforms") == 0 ||
      strcmp(cmd, "requireforms") == 0 || strcmp(cmd, "allforms") == 0 ||
      strcmp(cmd, "hasformany") == 0 || strcmp(cmd, "hasanyforms") == 0 ||
      strcmp(cmd, "needformany") == 0 || strcmp(cmd, "requireformany") == 0 ||
      strcmp(cmd, "mustformany") == 0 || strcmp(cmd, "needanyforms") == 0) {
    /* Usability: CLI dual of HASFORM/NEEDFORM/HASFORMS/NEEDFORMS/HASFORMANY/NEEDFORMANY.
     * cubalc hasform SORTLIBS · hasforms SORTLIBS LIBAGE · hasformany WALK PATHGLOB */
    int is_any = (strcmp(cmd, "hasformany") == 0 || strcmp(cmd, "hasanyforms") == 0 ||
                  strcmp(cmd, "needformany") == 0 || strcmp(cmd, "requireformany") == 0 ||
                  strcmp(cmd, "mustformany") == 0 || strcmp(cmd, "needanyforms") == 0);
    int multi = is_any ||
                (strcmp(cmd, "hasforms") == 0 || strcmp(cmd, "needforms") == 0 ||
                 strcmp(cmd, "requireforms") == 0 || strcmp(cmd, "allforms") == 0);
    int hard = (strcmp(cmd, "needform") == 0 || strcmp(cmd, "requireform") == 0 ||
                strcmp(cmd, "needforms") == 0 || strcmp(cmd, "requireforms") == 0 ||
                strcmp(cmd, "needformany") == 0 || strcmp(cmd, "requireformany") == 0 ||
                strcmp(cmd, "mustformany") == 0 || strcmp(cmd, "needanyforms") == 0);
    char src[800];
    cubalc_run_result rr;
    int hit = 0, i, narg = 0;
    size_t o = 0;
    const char *form;
    if (argc <= 2 || !argv[2] || !argv[2][0]) {
      fprintf(stderr,
              "usage: cubalc hasform|formexists <FormName>\n"
              "       cubalc needform|requireform <FormName>\n"
              "       cubalc hasforms|needforms <Form> [Form…]\n"
              "       cubalc hasformany|needformany <Form> [Form…]\n");
      printf("{\"schema\":\"cubalc.formgate.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need form name\",\"version\":\"%s\"}\n",
             cmd, CUBALC_LANG_VERSION);
      return 2;
    }
    if (is_any)
      form = hard ? "NEEDFORMANY" : "HASFORMANY";
    else if (multi || argc > 3)
      form = hard ? "NEEDFORMS" : "HASFORMS";
    else
      form = hard ? "NEEDFORM" : "HASFORM";
    if (multi || is_any || argc > 3) {
      o = (size_t)snprintf(src, sizeof src, "%s", form);
      for (i = 2; i < argc && o + 2 < sizeof src; i++) {
        if (!argv[i] || !argv[i][0]) continue;
        narg++;
        o += (size_t)snprintf(src + o, sizeof src - o, " %s", argv[i]);
      }
      /* soft any/all: PRINT LAST_N; hard: ASSERT succeeds only if pass */
      if (hard)
        o += (size_t)snprintf(src + o, sizeof src - o,
                              "\nASSERT LAST_N == 1\nPASS\n");
      else
        o += (size_t)snprintf(src + o, sizeof src - o,
                              "\nPRINT LAST_N\nPASS\n");
    } else {
      const char *name = argv[2];
      narg = 1;
      if (hard) {
        if (strchr(name, ' ') || strchr(name, '"'))
          snprintf(src, sizeof src,
                   "%s \"%s\"\nASSERT LAST_N == 1\nPASS\n", form, name);
        else
          snprintf(src, sizeof src,
                   "%s %s\nASSERT LAST_N == 1\nPASS\n", form, name);
      } else {
        if (strchr(name, ' ') || strchr(name, '"'))
          snprintf(src, sizeof src,
                   "%s \"%s\"\nPRINT LAST_N\nPASS\n", form, name);
        else
          snprintf(src, sizeof src,
                   "%s %s\nPRINT LAST_N\nPASS\n", form, name);
      }
    }
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<hasform>", &rr, NULL);
    if (hard)
      hit = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    else
      hit = (rr.ok && !rr.err[0] && rr.last_print[0] == '1') ? 1 : 0;
    printf("{\"schema\":\"cubalc.formgate.v1\",\"ok\":%s,\"cmd\":\"%s\","
           "\"n\":%d,\"known\":%s,\"mode\":\"%s\",\"any\":%s,"
           "\"version\":\"%s\","
           "\"note\":\"CLI dual of %s · HELP catalog capability gate\","
           "\"forms\":[",
           hit ? "true" : "false", cmd, narg,
           hit ? "true" : "false",
           hard ? "need" : "has",
           is_any ? "true" : "false",
           CUBALC_LANG_VERSION, form);
    for (i = 2; i < argc; i++) {
      if (!argv[i] || !argv[i][0]) continue;
      printf("%s\"%s\"", (i == 2) ? "" : ",", argv[i]);
    }
    printf("]}\n");
    if (hard)
      return hit ? 0 : 1;
    return 0;
  }
  if (strcmp(cmd, "listforms") == 0 || strcmp(cmd, "formlist") == 0 ||
      strcmp(cmd, "countforms") == 0 || strcmp(cmd, "nforms") == 0) {
    /* Usability: CLI dual of LISTFORMS/COUNTFORMS — form catalog bag/count without .cubalc. */
    const char *pref = (argc > 2) ? argv[2] : "";
    int count_only = (strcmp(cmd, "countforms") == 0 || strcmp(cmd, "nforms") == 0);
    char src[480];
    cubalc_run_result rr;
    long n = 0;
    int ok;
    if (pref && pref[0])
      snprintf(src, sizeof src, "COUNTFORMS %s\nPRINT LAST_N\nPASS\n", pref);
    else
      snprintf(src, sizeof src, "COUNTFORMS\nPRINT LAST_N\nPASS\n");
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<listforms>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    if (rr.last_print[0])
      n = strtol(rr.last_print, NULL, 10);
    printf("{\"schema\":\"cubalc.formlist.v1\",\"ok\":%s,\"cmd\":\"%s\","
           "\"prefix\":\"%s\",\"n\":%ld,\"count_only\":%s,"
           "\"version\":\"%s\","
           "\"note\":\"CLI dual of LISTFORMS/COUNTFORMS · cubalc forms for table\","
           "\"hint\":\"in-lang LISTFORMS %s · HASFORM name · run -C forms\"}\n",
           ok ? "true" : "false", cmd, pref ? pref : "", n,
           count_only ? "true" : "false", CUBALC_LANG_VERSION,
           (pref && pref[0]) ? pref : "");
    return ok ? 0 : 1;
  }
  if (strcmp(cmd, "formhint") == 0 || strcmp(cmd, "describeform") == 0 ||
      strcmp(cmd, "formdoc") == 0 || strcmp(cmd, "formhelp") == 0) {
    /* Usability: CLI dual of FORMHINT — one-line HELP peel without .cubalc. */
    const char *name = (argc > 2) ? argv[2] : "";
    char src[320];
    cubalc_run_result rr;
    if (!name || !name[0]) {
      fprintf(stderr, "usage: cubalc formhint|describeform <FormName>\n");
      printf("{\"schema\":\"cubalc.formhint.v1\",\"ok\":false,\"cmd\":\"%s\","
             "\"err\":\"need form name\",\"version\":\"%s\"}\n",
             cmd, CUBALC_LANG_VERSION);
      return 2;
    }
    if (strchr(name, ' ') || strchr(name, '"'))
      snprintf(src, sizeof src, "FORMHINT \"%s\"\nPRINT LAST\nPASS\n", name);
    else
      snprintf(src, sizeof src, "FORMHINT %s\nPRINT LAST\nPASS\n", name);
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, strlen(src), "<formhint>", &rr, NULL);
    {
      int ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0] && rr.last_print[0]) ? 1 : 0;
      /* escape hint for JSON */
      char hintj[400];
      size_t i, o = 0;
      const char *h = rr.last_print;
      for (i = 0; h[i] && o + 2 < sizeof hintj; i++) {
        char c = h[i];
        if (c == '"' || c == '\\') { hintj[o++] = '\\'; hintj[o++] = c; }
        else if ((unsigned char)c < 32) hintj[o++] = ' ';
        else hintj[o++] = c;
      }
      hintj[o] = 0;
      printf("{\"schema\":\"cubalc.formhint.v1\",\"ok\":%s,\"cmd\":\"%s\","
             "\"form\":\"%s\",\"hint\":\"%s\",\"version\":\"%s\","
             "\"note\":\"CLI dual of FORMHINT · HELP one-line · cubalc forms\"}\n",
             ok ? "true" : "false", cmd, name, hintj, CUBALC_LANG_VERSION);
      return ok ? 0 : 1;
    }
  }
  if (strcmp(cmd, "plate") == 0 || strcmp(cmd, "jsonplate") == 0 ||
      strcmp(cmd, "platefile") == 0 || strcmp(cmd, "agent-plate") == 0) {
    return cubalc_cmd_plate(argc, argv);
  }
  if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0 ||
      strcmp(cmd, "-V") == 0 || strcmp(cmd, "ver") == 0) {
    /* Usability: machine-readable version plate (agents skip parsing help text). */
    printf("{\"schema\":\"cubalc.version.v1\",\"ok\":true,\"cmd\":\"version\","
           "\"version\":\"%s\",\"name\":\"%s\",\"paradigm\":\"%s\",\"creed\":\"%s\","
           "\"share\":\"%s\",\"hold_flash\":%d,"
           "\"hold_flash_means\":\"device_firmware_connection_safeguard\","
           "\"http_required\":false,\"wire\":\"smx2\","
           "\"note\":\"in-language: VERSION → LAST/VERSION string\"}\n",
           CUBALC_LANG_VERSION, CUBALC_LANG_NAME, CUBALC_LANG_PARADIGM,
           CUBALC_CREED, CUBALC_SHARE, CUBALC_HOLD_FLASH);
    return 0;
  }
  if (strcmp(cmd, "paths") == 0 || strcmp(cmd, "path") == 0 ||
      strcmp(cmd, "where") == 0 || strcmp(cmd, "layout") == 0) {
    /* Usability: install/workspace path plate — agents locate lib/docs/state. */
    static const struct { const char *name; const char *path; const char *hint; } fixed[] = {
      {"bin", "out/cubalc", "built CLI"},
      {"lib", "programs/lib", "INCLUDE short-name stdlib"},
      {"programs", "programs", "runnable .cubalc tree"},
      {"proof", "programs/proof", "usability + ISA proofs"},
      {"p2p", "programs/p2p", "SMX SERVE/DIAL samples"},
      {"protect", "programs/protect", "core protect samples"},
      {"cookbook", "docs/COOKBOOK.md", "hold → plug → smx recipes"},
      {"agents", "docs/FOR_AGENTS.md", "agent prompt snippet"},
      {"hold_flash", "docs/HOLD_FLASH.md", "device/mesh-join safeguard only"},
      {"p2p_doc", "docs/P2P_SMX.md", "binary mesh wire"},
      {"lang_src", "src/lang", "modular language planes"},
      {"hello", "programs/hello_cube.cubalc", "minimal starter"},
    };
    char cwd[512], sdir[512], root[512];
    int i, nfixed = (int)(sizeof fixed / sizeof fixed[0]);
    int present = 0, n = 0;
    if (!getcwd(cwd, sizeof cwd))
      snprintf(cwd, sizeof cwd, ".");
    state_dir(sdir, sizeof sdir);
    {
      const char *r = getenv("CUBALC_ROOT");
      if (r && r[0]) snprintf(root, sizeof root, "%s", r);
      else snprintf(root, sizeof root, "%s", cwd);
    }
    printf("# CubalC paths version=%s\n", CUBALC_LANG_VERSION);
    printf("# name\tpath\tpresent\thint\n");
    /* dynamic: cwd, state, root */
    {
      struct { const char *name; const char *path; const char *hint; } dyn[3];
      int di;
      dyn[0].name = "cwd"; dyn[0].path = cwd; dyn[0].hint = "process working directory";
      dyn[1].name = "state"; dyn[1].path = sdir; dyn[1].hint = "CUBALC_STATE plates";
      dyn[2].name = "root"; dyn[2].path = root; dyn[2].hint = "CUBALC_ROOT or cwd";
      for (di = 0; di < 3; di++) {
        int ok = (access(dyn[di].path, F_OK) == 0);
        if (ok) present++;
        n++;
        printf("%s\t%s\t%s\t%s\n", dyn[di].name, dyn[di].path,
               ok ? "yes" : "no", dyn[di].hint);
      }
    }
    for (i = 0; i < nfixed; i++) {
      int ok = (access(fixed[i].path, F_OK) == 0);
      if (ok) present++;
      n++;
      printf("%s\t%s\t%s\t%s\n", fixed[i].name, fixed[i].path,
             ok ? "yes" : "no", fixed[i].hint);
    }
    printf("{\"schema\":\"cubalc.paths.v1\",\"ok\":%s,\"cmd\":\"paths\","
           "\"n\":%d,\"n_present\":%d,\"version\":\"%s\","
           "\"cwd\":\"%s\",\"state\":\"%s\",\"root\":\"%s\","
           "\"note\":\"layout for INCLUDE/run/cat — relative to cwd unless absolute\","
           "\"paths\":[",
           present > 0 ? "true" : "false", n, present, CUBALC_LANG_VERSION,
           cwd, sdir, root);
    {
      int first = 1;
      struct { const char *name; const char *path; const char *hint; } dyn[3];
      int di;
      dyn[0].name = "cwd"; dyn[0].path = cwd; dyn[0].hint = "process working directory";
      dyn[1].name = "state"; dyn[1].path = sdir; dyn[1].hint = "CUBALC_STATE plates";
      dyn[2].name = "root"; dyn[2].path = root; dyn[2].hint = "CUBALC_ROOT or cwd";
      for (di = 0; di < 3; di++) {
        int ok = (access(dyn[di].path, F_OK) == 0);
        /* escape path lightly */
        char pesc[520];
        size_t k, o = 0;
        for (k = 0; dyn[di].path[k] && o + 2 < sizeof pesc; k++) {
          char c = dyn[di].path[k];
          if (c == '"' || c == '\\') pesc[o++] = '_';
          else pesc[o++] = c;
        }
        pesc[o] = 0;
        printf("%s{\"name\":\"%s\",\"path\":\"%s\",\"present\":%s,\"hint\":\"%s\"}",
               first ? "" : ",", dyn[di].name, pesc, ok ? "true" : "false",
               dyn[di].hint);
        first = 0;
      }
      for (i = 0; i < nfixed; i++) {
        int ok = (access(fixed[i].path, F_OK) == 0);
        printf("%s{\"name\":\"%s\",\"path\":\"%s\",\"present\":%s,\"hint\":\"%s\"}",
               first ? "" : ",", fixed[i].name, fixed[i].path,
               ok ? "true" : "false", fixed[i].hint);
        first = 0;
      }
    }
    printf("]}\n");
    return present > 0 ? 0 : 1;
  }
  if (strcmp(cmd, "which") == 0 || strcmp(cmd, "locate") == 0 ||
      strcmp(cmd, "resolve") == 0) {
    return cubalc_cmd_which(argc, argv);
  }
  if (strcmp(cmd, "search") == 0 || strcmp(cmd, "find") == 0 ||
      strcmp(cmd, "query") == 0 || strcmp(cmd, "look") == 0) {
    return cubalc_cmd_search(argc, argv);
  }
  if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0) {
    /* Usability: keep help in sync with product surface agents actually use
     * (recipe, init --fat-session, run -T/-I, discovery forms). */
    fprintf(stderr,
      "CubalC %s — pure-C COP/flow (matrix SoT · SMX2 · no HTTP required)\n"
      "\n"
      "  Run & learn\n"
      "    doctor|health|needdoctor|ready|needready  install readiness / prove checklist JSON\n"
      "    cliinfo|dumpcli [-- args…]  CLI surface plate (cubalc.cli.v1 · flags+restargs)\n"
      "    hasflagall|needflags names… [-- args]  multi --flag ALL gate (cubalc.flaggate.v1)\n"
      "    hasflagany|needflagany names… [-- args]  multi --flag ANY gate (cubalc.flaggate.v1)\n"
      "    hasargall|needargs n|name… [-- args]  multi arg ALL gate (cubalc.arggate.v1)\n"
      "    hasargany|needargany n|name… [-- args]  multi arg ANY gate (cubalc.arggate.v1)\n"
      "    hasenvall|needenvs ENV…          multi host-env ALL gate (cubalc.envgate.v1)\n"
      "    hasenvany|needenvany ENV…        multi host-env ANY gate (cubalc.envgate.v1)\n"
      "    hasform|needform FORM            single form capability gate (cubalc.formgate.v1)\n"
      "    hasforms|needforms FORM…         multi form ALL gate (cubalc.formgate.v1)\n"
      "    hasformany|needformany FORM…     multi form ANY gate (cubalc.formgate.v1)\n"
      "    argmap|argkv [-- args…]     raw argv i=value bag (cubalc.argmap.v1)\n"
      "    findarg|argindex <tok> [-- args…]  token→index reverse NTHARG (cubalc.findarg.v1)\n"
      "    afterarg|nextarg <tok> [-- args…]  peel argv after token (cubalc.afterarg.v1)\n"
      "    firstarg|lastarg [-- args…]  raw argv end peels (cubalc.endarg.v1)\n"
      "    selftest|smoke         live curated usability proofs JSON\n"
      "    version|ver|-V         language version JSON plate\n"
      "    paths|where|layout     install/workspace paths JSON\n"
      "    which|locate|resolve   resolve name → path/kind (lib/form/bin)\n"
      "    search|find|query      keyword search forms/libs/examples/env\n"
      "    cookbook|start         paths to starters (legacy; agents prefer onboard)\n"
      "    onboard|welcome|begin  agent onboarding plate (cubalc.start.v1 · dual of START)\n"
      "    tips|howto|next [topic]  curated agent next-steps JSON (cap|fat|plate|run…)\n"
      "    formsfor|topicforms [topic]  form-name bag by topic (cubalc.formsfor.v1)\n"
      "    related|seealso <form>     related form-name bag (cubalc.related.v1)\n"
      "    snip|snippet [topic]       mini runnable source JSON (cubalc.snip.v1)\n"
      "    topic|topiccard [topic]    tips+forms+snip one plate (cubalc.topic.v1)\n"
      "    runsnip|sniprun [topic]|match|nth|lastmatch  execute SNIP · filter pick (cubalc.runsnip.v1)\n"
      "    topics|listtopics          discovery topic catalog (cubalc.topics.v1)\n    discover|explore [q]       match bag + first open plate (cubalc.discover.v1)\n    open|openplay [q]           DISCOVER bag + GUIDE first (cubalc.open.v1)\n"
      "    hastopic|needtopic <t>     soft/hard topic membership gates\n"
      "    matchtopics|picktopic <q>  filter/first discovery topic (cubalc.topicmatch.v1)\n"
      "    hasmatchtopics|needmatchtopics|countmatchtopics <q>  topic filter gates\n"
      "    nthtopic|lasttopic …        Nth/last matching topic (cubalc.topicmatch.v1)\n"
      "    topichint|describetopic <t>  one-line topic docs (cubalc.topichint.v1)\n"
      "    relatedtopic|seetopics <t> related topic bag (cubalc.relatedtopic.v1)\n"
      "    formtopics|topicsof <form> topics covering form (cubalc.formtopics.v1)\n"
      "    formguide|guideform <form> form docs+topics+playbook (cubalc.formguide.v1)\n"
      "    guide|playguide [topic]|match|nth|lastmatch  full playbook · filter pick (cubalc.guide.v1)\n"
      "    errtips|fixtips <err…>     recovery tip bag + topic (cubalc.errtips.v1)\n"
      "    errrun|recoversnip <err…>  classify + RUNSNIP topic (cubalc.errrun.v1)\n"
      "    errguide|recoverguide <err…> classify + GUIDE playbook (cubalc.errguide.v1)\n"
      "    init|new|scaffold [f]  --list · --plate · --peer · --fat · --fat-session · --cap · --onboard · --discover · --open · --doctor · --ready · --cli · --cli-session · --full-cli · --product · --path · --rw · --bin · --lib-boot · --host · --fleet · --key · --fn · --class · --method · --field · --oop · --obj · --live · --service · --env · --arg · --time · --from lib\n"
      "    examples|starters [p]  curated runnable programs (JSON · examples fat)\n"
      "    cat|type|source <lib>  dump lib/program source + meta plate\n"
      "    recipe|card <lib>      path+deps+defaults+head one plate (cubalc.recipe.v1)\n"
      "    checkdeps|hasdeps|needdeps <lib>  root+LIBTREE disk gate (cubalc.checkdeps.v1)\n"
      "    picklib|listforms|formhint|topichint|relatedtopic|formtopics|formguide|guide|guidematch|guidenth|guidelastmatch|runsnipmatch|runsnipnth|runsniplastmatch|discover|explore|ready|proveready|onboard|matchtopics|picktopic|hasmatchtopics|nthtopic|lasttopic|formsfor|related|snip|topic|runsnip|topics|errtips|errrun|errguide\n"
      "    plate|jsonplate …      agent plate get/set/fill/ensure/merge/eq/has/need (JSON)\n"
      "    forms|ops [prefix]     list play forms (filterable; JSON plate)\n"
      "    libs|lib|stdlib [q]    list INCLUDE libs (+stem/deps_n/defaults_n) · filter q\n"
      "    guards|contracts [q]  *guard* contracts NEED_*/SOFT/HARDFAIL · cubalc.guards.v1\n"
      "    env|environ|vars [pfx] host CUBALC_* env contract (JSON)\n"
      "    run|eval [-q][-s][-T ms][-I lib][-L dir][-R ver][-C forms][-D][-Y][-e CODE] <file|->\n"
      "                          -q plate · -s strict · -T wall · -C form floor · -D doctor · -Y ready floor (NEEDREADY)\n"
      "    help|-h                this text\n"
      "\n"
      "  Law & Core safety\n"
      "    law|manifest           law plate JSON\n"
      "    protect|core-guard     Core protect checks → state/CORE_PROTECT.json\n"
      "    protect status         JSON summary only · why_hint/next/docs (no board)\n"
      "    HOLD_FLASH             device/mesh-join only · default 1 · not language PLUG\n"
      "\n"
      "  P2P / SMX2 (binary wire)\n"
      "    smx|smx-selftest       seal/open/anti-replay\n"
      "    smx-exchange           file-bus two-peer proof\n"
      "    smx-bus prove|prove-tcp|serve|dial\n"
      "    env: CUBALC_SMX_KEY · CUBALC_P2P_BIND/PEER · CUBALC_P2P_TIMEOUT · CUBALC_P2P_SOFT\n"
      "\n"
      "  Machine / OS\n"
      "    boot|os|genesis|impulse|flow|cubes|cubechain\n"
      "    compile|jit|cflow|disasm|translate|decide\n"
      "    sync|hive|peers|showcase\n"
      "    evolve|evolve-loop     pure-C self-improve (optional)\n"
      "\n"
      "  Language surface (in .cubalc)\n"
      "    CUBE PLUG FLOW IMPULSE SETBIT SETDIGIT FOLDBITS DECIDE\n"
      "    SMX KEY|TALK|EXCHANGE|SERVE|DIAL · SYS … · INCLUDE [ONCE][SOFT]|MATCH|ALL MATCH\n"
      "    ASSERT|EXPECT|FAIL|PASS|NOTE|TIPS|ERRTIPS|ERRRUN|FORMSFOR|RELATED|SNIP|RUNSNIP|TOPIC|LISTTOPICS|EXIT|CLEAR_ERR|WHY\n"
      "    LISTLIBS|LISTFORMS|LISTTOPICS|FORMHINT|TOPICHINT|FORMSFOR|RELATED|SNIP|RUNSNIP|TOPIC|ERRTIPS|ERRRUN|HASTOPIC\n"
      "    DEFAULT|DEFINED|TYPEOF|UNSET · PRINT_JSON · VARS · REQUIRE LIB|VERSION|ENV\n"
      "\n"
      "  Agents: cubalc doctor · checkdeps fat_session · init --from plate_tick · RECIPE\n"
      "  hold=%d share=%s tok=%s paradigm=%s\n",
      CUBALC_LANG_VERSION, CUBALC_HOLD_FLASH, CUBALC_SHARE, CUBALC_CREED,
      CUBALC_LANG_PARADIGM);
    return 0;
  }
  fprintf(stderr, "unknown cmd %s — try help\n", cmd);
  return 2;
}
