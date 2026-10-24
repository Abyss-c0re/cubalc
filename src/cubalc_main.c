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
#include "cubalc_cmd_findarg.h"
#include "cubalc_cmd_envform.h"
#include "cubalc_cmd_pathver.h"
#include "cubalc_cmd_help.h"
#include "cubalc_cmd_flaggate.h"
#include "cubalc_cmd_libmatch.h"
#include "cubalc_cmd_recipe.h"
#include "cubalc_cmd_checkdeps.h"
#include "cubalc_cmd_freshlibs.h"
#include "cubalc_cmd_libage.h"
#include "cubalc_cmd_examples.h"
#include "cubalc_cmd_guide.h"
#include "cubalc_cmd_topic.h"
#include "cubalc_cmd_runsnip.h"
#include "cubalc_cmd_formsfor.h"
#include "cubalc_cmd_snip.h"
#include "cubalc_cmd_open.h"
#include "cubalc_cmd_matchtopics.h"
#include "cubalc_cmd_tips.h"
#include "cubalc_cmd_topichint.h"
#include "cubalc_cmd_formhints.h"
#include "cubalc_cmd_errrun.h"
#include "cubalc_cmd_cat.h"
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
      strcmp(cmd, "argvindex") == 0 ||
      strcmp(cmd, "afterarg") == 0 || strcmp(cmd, "nextarg") == 0 ||
      strcmp(cmd, "argafter") == 0 || strcmp(cmd, "followarg") == 0 ||
      strcmp(cmd, "argnext") == 0) {
    /* findarg/afterarg → cubalc_cmd_findarg peel (F_main_carve) */
    return cubalc_cmd_findarg(argc, argv);
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
    /* tips·howto → cubalc_cmd_tips peel (F_main_carve) */
    return cubalc_cmd_tips(argc, argv);
  }
  if (strcmp(cmd, "formsfor") == 0 || strcmp(cmd, "topicforms") == 0 ||
      strcmp(cmd, "formstopic") == 0 || strcmp(cmd, "surface") == 0 ||
      strcmp(cmd, "forms-for") == 0 || strcmp(cmd, "capforms") == 0) {
    /* formsfor·topicforms → cubalc_cmd_formsfor peel (F_main_carve) */
    return cubalc_cmd_formsfor(argc, argv);
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
    /* snip·snippet → cubalc_cmd_snip peel (F_main_carve) */
    return cubalc_cmd_snip(argc, argv);
  }
  if (strcmp(cmd, "topic") == 0 || strcmp(cmd, "topiccard") == 0 ||
      strcmp(cmd, "topicinfo") == 0 || strcmp(cmd, "playcard") == 0 ||
      strcmp(cmd, "discover-topic") == 0 || strcmp(cmd, "topicpack") == 0) {
    /* topic·topiccard → cubalc_cmd_topic peel (F_main_carve) */
    return cubalc_cmd_topic(argc, argv);
  }
  if (strcmp(cmd, "runsnip") == 0 || strcmp(cmd, "sniprun") == 0 ||
      strcmp(cmd, "snip-run") == 0 || strcmp(cmd, "run-snip") == 0 ||
      strcmp(cmd, "exec-snip") == 0 || strcmp(cmd, "play-snip") == 0 ||
      strcmp(cmd, "runsnipmatch") == 0 || strcmp(cmd, "matchrunsnip") == 0 ||
      strcmp(cmd, "runsnipnth") == 0 || strcmp(cmd, "nthrunsnip") == 0 ||
      strcmp(cmd, "runsniplastmatch") == 0 || strcmp(cmd, "lastmatchrunsnip") == 0) {
    /* runsnip·sniprun → cubalc_cmd_runsnip peel (F_main_carve) */
    return cubalc_cmd_runsnip(argc, argv);
  }
  if (strcmp(cmd, "discover") == 0 || strcmp(cmd, "explore") == 0 ||
      strcmp(cmd, "discovertopics") == 0 || strcmp(cmd, "scout") == 0 ||
      strcmp(cmd, "opendiscover") == 0) {
    /* discover·explore → cubalc_cmd_formsfor peel (F_main_carve) */
    return cubalc_cmd_formsfor(argc, argv);
  }
  if (strcmp(cmd, "open") == 0 || strcmp(cmd, "openplay") == 0 ||
      strcmp(cmd, "openguide") == 0 || strcmp(cmd, "playopen") == 0 ||
      strcmp(cmd, "discoverguide") == 0) {
    /* open·openplay → cubalc_cmd_open peel (F_main_carve) */
    return cubalc_cmd_open(argc, argv);
  }
  if (strcmp(cmd, "topics") == 0 || strcmp(cmd, "listtopics") == 0 ||
      strcmp(cmd, "topiclist") == 0 || strcmp(cmd, "discovery-topics") == 0 ||
      strcmp(cmd, "hastopic") == 0 || strcmp(cmd, "needtopic") == 0) {
    /* topics·listtopics → cubalc_cmd_open peel (F_main_carve) */
    return cubalc_cmd_open(argc, argv);
  }
  if (strcmp(cmd, "matchtopics") == 0 || strcmp(cmd, "filtertopics") == 0 ||
      strcmp(cmd, "topicmatch") == 0 || strcmp(cmd, "greptopics") == 0 ||
      strcmp(cmd, "picktopic") == 0 || strcmp(cmd, "firsttopic") == 0 ||
      strcmp(cmd, "topicpick") == 0) {
    /* matchtopics·picktopic → cubalc_cmd_matchtopics peel (F_main_carve) */
    return cubalc_cmd_matchtopics(argc, argv);
  }
  if (strcmp(cmd, "hasmatchtopics") == 0 || strcmp(cmd, "hastopicmatch") == 0 ||
      strcmp(cmd, "needmatchtopics") == 0 || strcmp(cmd, "needtopicmatch") == 0 ||
      strcmp(cmd, "countmatchtopics") == 0 || strcmp(cmd, "counttopics") == 0 ||
      strcmp(cmd, "ntopicmatch") == 0) {
    /* hasmatchtopics·counttopics → cubalc_cmd_matchtopics peel (F_main_carve) */
    return cubalc_cmd_matchtopics(argc, argv);
  }
  if (strcmp(cmd, "nthtopic") == 0 || strcmp(cmd, "indextopic") == 0 ||
      strcmp(cmd, "topicnth") == 0 || strcmp(cmd, "lasttopic") == 0 ||
      strcmp(cmd, "endtopic") == 0 || strcmp(cmd, "topiclast") == 0) {
    /* nthtopic·lasttopic → cubalc_cmd_tips peel (F_main_carve) */
    return cubalc_cmd_tips(argc, argv);
  }

  if (strcmp(cmd, "topichint") == 0 || strcmp(cmd, "describetopic") == 0 ||
      strcmp(cmd, "topicdoc") == 0 || strcmp(cmd, "topichelp") == 0 ||
      strcmp(cmd, "hinttopic") == 0 || strcmp(cmd, "topic-hint") == 0) {
    /* topichint·describetopic → cubalc_cmd_topichint peel (F_main_carve) */
    return cubalc_cmd_topichint(argc, argv);
  }
  if (strcmp(cmd, "relatedtopic") == 0 || strcmp(cmd, "seetopics") == 0 ||
      strcmp(cmd, "relatedtopics") == 0 || strcmp(cmd, "topicrel") == 0 ||
      strcmp(cmd, "neartopics") == 0 || strcmp(cmd, "topic-neighbors") == 0) {
    /* relatedtopic·seetopics → cubalc_cmd_topichint peel (F_main_carve) */
    return cubalc_cmd_topichint(argc, argv);
  }
  if (strcmp(cmd, "formtopics") == 0 || strcmp(cmd, "topicsof") == 0 ||
      strcmp(cmd, "topicsfor") == 0 || strcmp(cmd, "form-topics") == 0 ||
      strcmp(cmd, "covertopics") == 0 || strcmp(cmd, "topics-of") == 0) {
    /* formtopics·topicsof → cubalc_cmd_topichint peel (F_main_carve) */
    return cubalc_cmd_topichint(argc, argv);
  }
  if (strcmp(cmd, "formguide") == 0 || strcmp(cmd, "guideform") == 0 ||
      strcmp(cmd, "form-guide") == 0 || strcmp(cmd, "formcard") == 0 ||
      strcmp(cmd, "playform") == 0 || strcmp(cmd, "formboard") == 0 ||
      strcmp(cmd, "guide") == 0 || strcmp(cmd, "playguide") == 0 ||
      strcmp(cmd, "topicguide") == 0 || strcmp(cmd, "agentguide") == 0 ||
      strcmp(cmd, "fulltopic") == 0 || strcmp(cmd, "topicboard") == 0 ||
      strcmp(cmd, "guidematch") == 0 || strcmp(cmd, "matchguide") == 0 ||
      strcmp(cmd, "guidenth") == 0 || strcmp(cmd, "nthguide") == 0 ||
      strcmp(cmd, "guidelastmatch") == 0 || strcmp(cmd, "lastmatchguide") == 0) {
    /* formguide·guide → cubalc_cmd_guide peel (F_main_carve) */
    return cubalc_cmd_guide(argc, argv);
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
    /* errrun·recoversnip → cubalc_cmd_errrun peel (F_main_carve) */
    return cubalc_cmd_errrun(argc, argv);
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
      strcmp(cmd, "starters") == 0 || strcmp(cmd, "recipes") == 0)
    return cubalc_cmd_examples(argc, argv);
  if (strcmp(cmd, "cat") == 0 || strcmp(cmd, "type") == 0 ||
      strcmp(cmd, "read-lib") == 0 || strcmp(cmd, "source") == 0) {
    /* cat/type/source → cubalc_cmd_cat peel (F_main_carve) */
    return cubalc_cmd_cat(argc, argv);
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
  if (strcmp(cmd, "freshlibs") == 0 ||
      strcmp(cmd, "recentlibs") == 0 ||
      strcmp(cmd, "stalelibs") == 0 ||
      strcmp(cmd, "olderlibs") == 0 ||
      strcmp(cmd, "keepfreshlibs") == 0 ||
      strcmp(cmd, "keepstalelibs") == 0) {
    /* freshlibs/stalelibs → cubalc_cmd_freshlibs peel (F_main_carve) */
    return cubalc_cmd_freshlibs(argc, argv);
  }
  if (strcmp(cmd, "libage") == 0 ||
      strcmp(cmd, "ageoflib") == 0 ||
      strcmp(cmd, "hasfresh") == 0 ||
      strcmp(cmd, "isfresh") == 0 ||
      strcmp(cmd, "needfresh") == 0 ||
      strcmp(cmd, "requirefresh") == 0) {
    /* libage/hasfresh/needfresh → cubalc_cmd_libage peel (F_main_carve) */
    return cubalc_cmd_libage(argc, argv);
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
    /* hasflagall/needflags/hasargall/needargs → cubalc_cmd_flaggate peel (F_main_carve) */
    return cubalc_cmd_flaggate(argc, argv);
  }
  if (strcmp(cmd, "hasenvall") == 0 || strcmp(cmd, "hasallenv") == 0 ||
      strcmp(cmd, "needenvs") == 0 || strcmp(cmd, "requireenvs") == 0 ||
      strcmp(cmd, "mustenvs") == 0 || strcmp(cmd, "needenvall") == 0 ||
      strcmp(cmd, "requireenvall") == 0 ||
      strcmp(cmd, "hasenvany") == 0 || strcmp(cmd, "hasanyenv") == 0 ||
      strcmp(cmd, "needenvany") == 0 || strcmp(cmd, "requireenvany") == 0 ||
      strcmp(cmd, "mustenvany") == 0 || strcmp(cmd, "needanyenv") == 0 ||
      strcmp(cmd, "hasform") == 0 || strcmp(cmd, "formexists") == 0 ||
      strcmp(cmd, "needform") == 0 || strcmp(cmd, "requireform") == 0 ||
      strcmp(cmd, "knownform") == 0 ||
      strcmp(cmd, "hasforms") == 0 || strcmp(cmd, "needforms") == 0 ||
      strcmp(cmd, "requireforms") == 0 || strcmp(cmd, "allforms") == 0 ||
      strcmp(cmd, "hasformany") == 0 || strcmp(cmd, "hasanyforms") == 0 ||
      strcmp(cmd, "needformany") == 0 || strcmp(cmd, "requireformany") == 0 ||
      strcmp(cmd, "mustformany") == 0 || strcmp(cmd, "needanyforms") == 0) {
    /* hasenvall/hasform gates → cubalc_cmd_envform peel (F_main_carve) */
    return cubalc_cmd_envform(argc, argv);
  }
  if (strcmp(cmd, "plate") == 0 || strcmp(cmd, "jsonplate") == 0 ||
      strcmp(cmd, "platefile") == 0 || strcmp(cmd, "agent-plate") == 0) {
    return cubalc_cmd_plate(argc, argv);
  }
  if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0 ||
      strcmp(cmd, "-V") == 0 || strcmp(cmd, "ver") == 0 ||
      strcmp(cmd, "paths") == 0 || strcmp(cmd, "path") == 0 ||
      strcmp(cmd, "where") == 0 || strcmp(cmd, "layout") == 0) {
    /* version/paths → cubalc_cmd_pathver peel (F_main_carve) */
    return cubalc_cmd_pathver(argc, argv);
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
    /* help → cubalc_cmd_help peel (F_main_carve) */
    return cubalc_cmd_help(argc, argv);
  }
  fprintf(stderr, "unknown cmd %s — try help\n", cmd);
  return 2;
}
