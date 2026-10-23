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
#include "cubalc_cmd_doctor.h"
#include "cubalc_cmd_selftest.h"
#include "cubalc_cmd_run.h"
#include "cubalc_cmd_stdlib.h"
#include "cubalc_cmd_vars.h"
#include "cubalc_cmd_formrel.h"
#include "cubalc_cmd_contracts.h"
#include "cubalc_cmd_whyguide.h"
#include "cubalc_cmd_cliinfo.h"
#include "cubalc_cmd_argmap.h"
#include "cubalc_cmd_endarg.h"
#include "cubalc_cmd_libmatch.h"
#include "cubalc_cmd_recipe.h"
#include "cubalc_cmd_checkdeps.h"
#include "cubalc_cmd_formhints.h"
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
    /* run/eval → cubalc_cmd_run peel (F_main_carve) */
    return cubalc_cmd_run(argc, argv);
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
  if (strcmp(cmd, "cliinfo") == 0 || strcmp(cmd, "dumpcli") == 0 ||
      strcmp(cmd, "clisnap") == 0 || strcmp(cmd, "cli-info") == 0 ||
      strcmp(cmd, "argvinfo") == 0 || strcmp(cmd, "cli-status") == 0) {
    /* cliinfo → cubalc_cmd_cliinfo peel (F_main_carve) */
    return cubalc_cmd_cliinfo(argc, argv);
  }
  if (strcmp(cmd, "argmap") == 0 || strcmp(cmd, "argkv") == 0 ||
      strcmp(cmd, "argvmap") == 0 || strcmp(cmd, "args-kv") == 0 ||
      strcmp(cmd, "cli-argmap") == 0) {
    /* argmap → cubalc_cmd_argmap peel (F_main_carve) */
    return cubalc_cmd_argmap(argc, argv);
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
    /* firstarg/lastarg → cubalc_cmd_endarg peel (F_main_carve) */
    return cubalc_cmd_endarg(argc, argv);
  }
  if (strcmp(cmd, "needdoctor") == 0 || strcmp(cmd, "need-doctor") == 0 ||
      strcmp(cmd, "require-doctor") == 0 || strcmp(cmd, "requiredoctor") == 0 ||
      strcmp(cmd, "needready") == 0 || strcmp(cmd, "need-ready") == 0 ||
      strcmp(cmd, "require-ready") == 0 || strcmp(cmd, "requireready") == 0 ||
      strcmp(cmd, "mustready") == 0 ||
      strcmp(cmd, "ready") == 0 || strcmp(cmd, "proveready") == 0 ||
      strcmp(cmd, "install-ready") == 0 || strcmp(cmd, "check-ready") == 0 ||
      strcmp(cmd, "doctor") == 0 || strcmp(cmd, "health") == 0) {
    /* doctor/ready/needdoctor/needready → cubalc_cmd_doctor peel (F_main_carve) */
    return cubalc_cmd_doctor(argc, argv);
  }
  if (strcmp(cmd, "selftest") == 0 || strcmp(cmd, "smoke") == 0 ||
      strcmp(cmd, "usability-test") == 0 || strcmp(cmd, "prove-usability") == 0) {
    /* selftest/smoke → cubalc_cmd_selftest peel (F_main_carve) */
    return cubalc_cmd_selftest(argc, argv);
  }
  if (strcmp(cmd, "forms") == 0 || strcmp(cmd, "ops") == 0 ||
      strcmp(cmd, "forms-list") == 0 ||
      strcmp(cmd, "listforms") == 0 || strcmp(cmd, "formlist") == 0 ||
      strcmp(cmd, "countforms") == 0 || strcmp(cmd, "nforms") == 0 ||
      strcmp(cmd, "formhint") == 0 || strcmp(cmd, "describeform") == 0 ||
      strcmp(cmd, "formdoc") == 0 || strcmp(cmd, "formhelp") == 0) {
    /* forms/listforms/formhint → cubalc_cmd_forms peel (F_main_carve) */
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
    /* related/formrel → cubalc_cmd_formrel peel (F_main_carve) */
    return cubalc_cmd_formrel(argc, argv);
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
    /* errtips/formhints → cubalc_cmd_formhints peel (F_main_carve) */
    return cubalc_cmd_formhints(argc, argv);
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
    /* whyguide → cubalc_cmd_whyguide peel (F_main_carve) */
    return cubalc_cmd_whyguide(argc, argv);
  }
  if (strcmp(cmd, "init") == 0 || strcmp(cmd, "new") == 0 ||
      strcmp(cmd, "scaffold") == 0 || strcmp(cmd, "create") == 0) {
    return cubalc_cmd_init(argc, argv);
  }
  if (strcmp(cmd, "libs") == 0 || strcmp(cmd, "lib") == 0 ||
      strcmp(cmd, "stdlib") == 0) {
    /* libs/lib/stdlib → cubalc_cmd_stdlib peel (F_main_carve) */
    return cubalc_cmd_stdlib(argc, argv);
  }
  
  if (strcmp(cmd, "guards") == 0 || strcmp(cmd, "guard") == 0 ||
      strcmp(cmd, "contracts") == 0 || strcmp(cmd, "contract") == 0) {
    /* contracts → cubalc_cmd_contracts peel (F_main_carve) */
    return cubalc_cmd_contracts(argc, argv);
  }
  if (strcmp(cmd, "env") == 0 || strcmp(cmd, "environ") == 0 ||
      strcmp(cmd, "vars") == 0 || strcmp(cmd, "env-list") == 0) {
    /* env/vars/env-list → cubalc_cmd_vars peel (F_main_carve) */
    return cubalc_cmd_vars(argc, argv);
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
    /* recipe/card → cubalc_cmd_recipe peel (F_main_carve) */
    return cubalc_cmd_recipe(argc, argv);
  }
  if (strcmp(cmd, "checkdeps") == 0 || strcmp(cmd, "deps") == 0 ||
      strcmp(cmd, "hasdeps") == 0 || strcmp(cmd, "needdeps") == 0 ||
      strcmp(cmd, "verifydeps") == 0 || strcmp(cmd, "libdeps-check") == 0) {
    /* checkdeps/hasdeps/needdeps → cubalc_cmd_checkdeps peel (F_main_carve) */
    return cubalc_cmd_checkdeps(argc, argv);
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
    /* picklib/sortlibs/match → cubalc_cmd_libmatch peel (F_main_carve) */
    return cubalc_cmd_libmatch(argc, argv);
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
