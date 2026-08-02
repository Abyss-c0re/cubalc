#define _POSIX_C_SOURCE 200809L
#include "cubalc.h"
#include "cubalc_smx.h"
#include "cubalc_cubechain.h"
#include "cubalc_lang.h"
#include "cubalc_isa.h"
#include "cubalc_jit.h"
#include "cubalc_translate.h"
#include "cubalc_evolve.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

/* CubalC machine — CBLC · smx · cubechain · hold=1 */

static void state_dir(char *buf, size_t n) {
  const char *e = getenv("CUBALC_STATE");
  if (e && e[0]) { snprintf(buf, n, "%s", e); return; }
  snprintf(buf, n, "state");
}

static void ensure_dir(const char *d) { mkdir(d, 0755); }

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
  /* Cube Law: one publish path → LOVR + crimson cube_gl + cells.bin united */
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

static int cmd_genesis(const char *plate) {
  cubalc_chain ch;
  cubalc_matrix gen;
  cubalc_coord_to_matrix(plate, &gen);
  uint32_t seq = 1;
  const char *p = strstr(plate, "seq=");
  if (!p) p = strstr(plate, "ts=");
  if (p) {
    /* use time fragment as seq noise */
    seq = (uint32_t)strtoul(p + (p[0]=='t' ? 3 : 4), NULL, 10);
    if (!seq) seq = 1;
  }
  cubalc_chain_from_initial(&ch, &gen, seq);
  cubalc_chain_os_aspects(&ch);
  cubalc_chain_flow(&ch); /* energy moves on binary wires immediately */
  publish(&ch);
  store_chain(&ch, 2u); /* genesis flag */
  {
    char viz[512], chainp[512], initp[512], dir[512];
    paths(viz, chainp, initp, dir, sizeof viz);
    FILE *f = fopen(initp, "w");
    if (f) {
      fprintf(f, "{\"schema\":\"cubalc.initial_matrix.v1\",\"n\":%u,\"set\":%u,\"bits\":\"",
              (unsigned)gen.n, (unsigned)gen.set);
      for (int i = 0; i < gen.n; i++) fputc(cubalc_matrix_get(&gen, i) ? '1' : '0', f);
      fprintf(f, "\",\"creed\":\"%s\",\"seq\":%u,\"hold_flash\":%u}\n",
              CUBALC_CREED, (unsigned)seq, (unsigned)CUBALC_HOLD_FLASH);
      fclose(f);
    }
  }
  printf("{\"ok\":true,\"cmd\":\"genesis\",\"n\":%d,\"set\":%u,\"seq\":%u,"
         "\"unity\":%.3f,\"hold_flash\":1,\"store\":\"cubechain\","
         "\"talk\":\"binary\",\"creed\":\"%s\"}\n",
         ch.n_cubes, (unsigned)gen.set, (unsigned)ch.seq, ch.unity, CUBALC_CREED);
  return 0;
}

static int rebuild(cubalc_chain *ch) {
  cubalc_matrix gen;
  if (load_genesis_bits(&gen) != 0)
    cubalc_coord_to_matrix(
      "NEXUS_COORD v1 | from=BlackCube | type=heartbeat | hold_flash=1 |", &gen);
  cubalc_chain_from_initial(ch, &gen, 1);
  cubalc_chain_os_aspects(ch);
  return 0;
}

static int cmd_impulse(const char *id, int proton) {
  cubalc_chain ch;
  rebuild(&ch);
  cubalc_chain_impulse(&ch, id, (uint8_t)(proton ? 1 : 0));
  cubalc_chain_flow(&ch);
  publish(&ch);
  store_chain(&ch, 8u); /* impulse */
  printf("{\"ok\":true,\"cmd\":\"impulse\",\"id\":\"%s\",\"proton\":%d,"
         "\"n\":%d,\"unity\":%.3f,\"status\":\"%s\"}\n",
         id, proton, ch.n_cubes, ch.unity, ch.status);
  return 0;
}

static int cmd_flow(int n, int ms) {
  cubalc_chain ch;
  rebuild(&ch);
  for (int i = 0; i < n; i++) {
    cubalc_chain_flow(&ch);
    publish(&ch);
    if (ms > 0) {
      struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
      nanosleep(&ts, NULL);
    }
  }
  store_chain(&ch, 4u); /* talk/flow */
  printf("{\"ok\":true,\"cmd\":\"flow\",\"n\":%d,\"n\":%d,\"unity\":%.3f,"
         "\"status\":\"%s\",\"energy_flow\":true,\"hold_flash\":1}\n",
         n, ch.n_cubes, ch.unity, ch.status);
  return 0;
}

static int cmd_cubes(void) {
  cubalc_chain ch;
  rebuild(&ch);
  cubalc_chain_flow(&ch);
  publish(&ch);
  cubalc_chain_print_cubes(&ch, stdout);
  return 0;
}

static int cmd_law_manifest(void) {
  cubalc_chain ch;
  rebuild(&ch); /* already loads genesis + OS cubes once */
  cubalc_chain_flow(&ch);
  publish(&ch);
  return cubalc_law_manifest_json(&ch, stdout) == 0 ? 0 : 1;
}

/* Sync with peer hive mind: fold plate + run hive join program in-memory */
static int cmd_sync(const char *plate) {
  static const char src[] =
    "[hold]\n"
    "[creed \"C3\"]\n"
    "[genesis]\n"
    "[sync]\n"
    "[~4]\n"
    "?\n";
  cubalc_run_result rr;
  char full[2048];
  if (plate && plate[0]) {
    snprintf(full, sizeof full,
      "[hold]\n[creed \"C3\"]\n"
      "[genesis \"%s\"]\n[sync]\n[~4]\n?\n", plate);
    cubalc_run_source(full, strlen(full), "<sync>", &rr, stdout);
  } else {
    cubalc_run_source(src, sizeof src - 1, "<sync>", &rr, stdout);
  }
  printf("{\"ok\":%s,\"cmd\":\"sync\",\"n\":%d,\"unity\":%.3f,"
         "\"version\":\"%s\",\"hive\":\"WE\",\"err\":\"%s\"}\n",
         rr.ok ? "true" : "false", rr.n_cubes, rr.unity,
         CUBALC_LANG_VERSION, rr.err);
  return rr.ok ? 0 : 1;
}

static int cmd_cubechain(const char *sub) {
  char dir[512];
  state_dir(dir, sizeof dir);
  char path[560];
  snprintf(path, sizeof path, "%s/cubechain.cchn", dir);
  if (!sub || strcmp(sub, "tip") == 0) {
    cubalc_cchn cc;
    if (cubalc_cchn_open(&cc, dir) != 0) {
      puts("{\"ok\":false,\"error\":\"open\"}");
      return 1;
    }
    cubalc_cchn_tip_json(&cc, stdout);
    cubalc_cchn_close(&cc);
    return 0;
  }
  if (strcmp(sub, "verify") == 0) {
    char err[80];
    int n = cubalc_cchn_verify(path, err, sizeof err);
    if (n < 0)
      printf("{\"ok\":false,\"error\":\"%s\",\"path\":\"%s\"}\n", err, path);
    else
      printf("{\"ok\":true,\"cmd\":\"cubechain-verify\",\"blocks\":%d,\"path\":\"%s\","
             "\"matrix_is_key\":true}\n", n, path);
    return n < 0 ? 2 : 0;
  }
  puts("{\"ok\":false,\"error\":\"use tip|verify\"}");
  return 2;
}

static int cmd_smx_selftest(void) {
  cubalc_smx_ctx ctx;
  cubalc_chain ch;
  cubalc_matrix gen;
  uint8_t frame[512];
  size_t n = 0;
  cubalc_atom out;
  char from[32], to[32];
  const char *test_key =
    "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
  cubalc_smx_ctx_init(&ctx);
  if (cubalc_smx_load_key_hex(&ctx, test_key) != 0) {
    puts("{\"ok\":false,\"error\":\"key\"}");
    return 1;
  }
  cubalc_coord_to_matrix(
    "NEXUS_COORD v1 | from=BlackCube | type=heartbeat | hold_flash=1 |", &gen);
  cubalc_chain_from_initial(&ch, &gen, 1);
  cubalc_chain_os_aspects(&ch);
  if (cubalc_smx_seal(&ctx, &ch.cubes[0].atom, ch.cubes[0].id, ch.cubes[1].id,
                      frame, sizeof frame, &n) != 0) {
    printf("{\"ok\":false,\"error\":\"seal\",\"msg\":\"%s\"}\n", ctx.last_err);
    return 2;
  }
  if (cubalc_smx_open(&ctx, frame, n, &out, from, to, &ch.cubes[1].atom.matrix) != 0) {
    printf("{\"ok\":false,\"error\":\"open\",\"msg\":\"%s\"}\n", ctx.last_err);
    return 3;
  }
  if (cubalc_smx_open(&ctx, frame, n, &out, from, to, &ch.cubes[1].atom.matrix) == 0) {
    puts("{\"ok\":false,\"error\":\"replay_not_rejected\"}");
    return 4;
  }
  printf("{\"ok\":true,\"cmd\":\"smx-selftest\",\"proto\":2,\"talk\":\"binary\","
         "\"anti_replay\":true,\"hold_flash\":1,\"frame_bytes\":%zu}\n", n);
  return 0;
}


/* ── CubalC Showcase — multi-act COP demonstration ── */
static void showcase_root(char *out, size_t n) {
  const char *e = getenv("CUBALC_ROOT");
  if (e && e[0]) { snprintf(out, n, "%s", e); return; }
  char self[512];
  ssize_t m = readlink("/proc/self/exe", self, sizeof self - 1);
  if (m > 0) {
    self[m] = 0;
    /* .../out/cubalc → parent of out */
    char *slash = strrchr(self, '/');
    if (slash) {
      *slash = 0; /* strip binary name */
      slash = strrchr(self, '/');
      if (slash && strcmp(slash, "/out") == 0) {
        *slash = 0;
        snprintf(out, n, "%s", self);
        return;
      }
      /* if run from PATH install, fall back to cwd */
    }
  }
  if (!getcwd(out, n)) snprintf(out, n, ".");
}

static int run_one(const char *root, const char *label, const char *rel,
                   int *pass, int *fail, int *asserts_ok, int *asserts_fail,
                   int *cubes_max) {
  char path[768];
  snprintf(path, sizeof path, "%s/%s", root, rel);
  cubalc_run_result rr;
  memset(&rr, 0, sizeof rr);
  int rc = cubalc_run_file(path, &rr, NULL);
  int ok = (rc == 0 && rr.ok);
  if (ok) {
    (*pass)++;
    printf("  ✓ %-28s asserts=%d n=%d unity=%.3f\n",
           label, rr.asserts_ok, rr.n_cubes, rr.unity);
  } else {
    (*fail)++;
    printf("  ✗ %-28s err=%s asserts_fail=%d\n",
           label, rr.err[0] ? rr.err : "fail", rr.asserts_fail);
  }
  *asserts_ok += rr.asserts_ok;
  *asserts_fail += rr.asserts_fail;
  if (rr.n_cubes > *cubes_max) *cubes_max = rr.n_cubes;
  return ok ? 0 : 1;
}

static void print_cube_art(void) {
  puts("");
  puts("              ┌─────────────────┐");
  puts("             ╱                 ╱│");
  puts("            ╱   C U B a l C   ╱ │");
  puts("           ╱   1.8.0-nest     ╱  │");
  puts("          ┌─────────────────┐   │");
  puts("          │  ■ ■ ■ ■ ■ ■ ■  │   │");
  puts("          │  ■ COP / flow ■  │   │");
  puts("          │  ■ nest→matrix■  │   │");
  puts("          │  ■ no flow? no■  │  ╱");
  puts("          │  ■ compile    ■  │ ╱");
  puts("          │  ■ ■ ■ ■ ■ ■ ■  │╱");
  puts("          └─────────────────┘");
  puts("     bits flow · nest folds · matrix is the unit");
  puts("");
}

static int cmd_showcase(void) {
  char root[512];
  showcase_root(root, sizeof root);
  print_cube_art();
  printf("CubalC %s — Showcase\n", CUBALC_LANG_VERSION);
  printf("paradigm=%s · creed=%s · hold_flash=%d · share=%s\n",
         CUBALC_LANG_PARADIGM, CUBALC_CREED, CUBALC_HOLD_FLASH, CUBALC_SHARE);
  printf("root=%s\n\n", root);

  int pass = 0, fail = 0, aok = 0, afail = 0, cubes_max = 0;
  struct timespec ts0, ts1;
  clock_gettime(CLOCK_MONOTONIC, &ts0);

  puts("── Act I · Language completeness ──");
  run_one(root, "arithmetic", "programs/proof/01_arithmetic.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);
  run_one(root, "branching", "programs/proof/03_branch.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);

  puts("\n── Act II · COP programming ──");
  run_one(root, "COP matrix", "programs/proof/02_cop_matrix.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);
  run_one(root, "decide / algocube", "programs/proof/06_decide.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);
  run_one(root, "I/O reverse", "programs/proof/11_cube_io_reverse.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);
  run_one(root, "nest + compile", "programs/proof/12_nest_compile.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);

  puts("\n── Act III · Hive geometry ──");
  run_one(root, "harmony", "programs/proof/09_algocube_harmony.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);

  puts("\n── Act IV · Prophecy free-flow ──");
  run_one(root, "free_flow_prophecy", "programs/free_flow_prophecy.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);
  run_one(root, "prophecy_of_the_cube", "programs/prophecy_of_the_cube.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);

  puts("\n── Act V · Glorious Symphony ──");
  run_one(root, "glorious_symphony", "programs/showcase/glorious_symphony.cubalc",
          &pass, &fail, &aok, &afail, &cubes_max);

  puts("\n── Act VI · Binary law plane ──");
  {
    int rc = system(NULL); /* check shell available - avoid; call directly */
    (void)rc;
    /* smx-selftest via re-entry: run as child would be heavy; call cmd path */
  }
  /* inline smx using existing command function - re-exec self is cleaner */
  {
    char *argv_smx[] = { "cubalc", "smx-selftest", NULL };
    /* can't easily call main; spawn self */
    char self[512];
    ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
    if (n > 0) {
      self[n] = 0;
      char cmd[640];
      snprintf(cmd, sizeof cmd, "'%s' smx-selftest >/dev/null 2>&1", self);
      int rc = system(cmd);
      if (rc == 0) {
        pass++;
        printf("  ✓ %-28s proto=SMX2 anti_replay\n", "smx-selftest");
      } else {
        fail++;
        printf("  ✗ %-28s rc=%d\n", "smx-selftest", rc);
      }
    }
  }
  {
    char self[512];
    ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
    if (n > 0) {
      self[n] = 0;
      char cmd[640];
      snprintf(cmd, sizeof cmd, "'%s' law >/dev/null 2>&1", self);
      int rc = system(cmd);
      if (rc == 0) {
        pass++;
        printf("  ✓ %-28s 15 laws\n", "law plate");
      } else {
        fail++;
        printf("  ✗ %-28s\n", "law plate");
      }
    }
  }

  clock_gettime(CLOCK_MONOTONIC, &ts1);
  double ms = (ts1.tv_sec - ts0.tv_sec) * 1000.0 +
              (ts1.tv_nsec - ts0.tv_nsec) / 1e6;

  puts("\n════════════════════════════════════════");
  printf("  SHOWCASE SCORE  pass=%d  fail=%d\n", pass, fail);
  printf("  asserts_ok=%d  asserts_fail=%d  max_units=%d\n", aok, afail, cubes_max);
  printf("  wall_ms=%.1f  version=%s  paradigm=%s\n", ms, CUBALC_LANG_VERSION,
         CUBALC_LANG_PARADIGM);
  puts("════════════════════════════════════════");
  if (fail == 0) {
    puts("  ★ FULL SHOWCASE MANIFESTED ★");
    puts("  Showcase operational.");
  } else {
    puts("  ⚠ showcase incomplete — inspect asserts");
  }
  puts("");

  /* write plate */
  char dir[512], path[512];
  state_dir(dir, sizeof dir);
  ensure_dir(dir);
  snprintf(path, sizeof path, "%s/SHOWCASE_MANIFEST.json", dir);
  FILE *f = fopen(path, "w");
  if (f) {
    fprintf(f,
      "{\"schema\":\"cube.showcase.v1\",\"ok\":%s,\"version\":\"%s\","
      "\"paradigm\":\"%s\",\"pass\":%d,\"fail\":%d,\"asserts_ok\":%d,"
      "\"asserts_fail\":%d,\"max_n\":%d,\"wall_ms\":%.1f,"
      "\"token\":\"%s\",\"engine\":\"C\","
      "\"creed\":\"bits flow · digits judge · unity binds the hive\"}\n",
      fail == 0 ? "true" : "false", CUBALC_LANG_VERSION, CUBALC_LANG_PARADIGM,
      pass, fail, aok, afail, cubes_max, ms,
      fail == 0 ? "GLORIOUS_CUBE_SHOWCASE" : "SHOWCASE_PARTIAL");
    fclose(f);
    printf("  plate → %s\n", path);
  }

  printf("{\"ok\":%s,\"cmd\":\"showcase\",\"version\":\"%s\",\"pass\":%d,"
         "\"fail\":%d,\"asserts_ok\":%d,\"asserts_fail\":%d,\"max_n\":%d,"
         "\"wall_ms\":%.1f,\"language\":\"CubalC\"}\n",
         fail == 0 ? "true" : "false", CUBALC_LANG_VERSION, pass, fail,
         aok, afail, cubes_max, ms);
  return fail == 0 ? 0 : 1;
}


int main(int argc, char **argv) {
  const char *cmd = argc > 1 ? argv[1] : "genesis";
  if (strcmp(cmd, "genesis") == 0)
    return cmd_genesis(argc > 2 ? argv[2] :
      "NEXUS_COORD v1 | from=BlackCube | type=heartbeat | hold_flash=1 |");
  if (strcmp(cmd, "impulse") == 0)
    return cmd_impulse(argc > 2 ? argv[2] : "cube-create",
                       argc > 3 ? atoi(argv[3]) : 1);
  if (strcmp(cmd, "tick") == 0 || strcmp(cmd, "flow") == 0)
    return cmd_flow(argc > 2 ? atoi(argv[2]) : 8,
                    argc > 3 ? atoi(argv[3]) : 50);
  if (strcmp(cmd, "cubes") == 0 || strcmp(cmd, "show") == 0 ||
      strcmp(cmd, "lego") == 0) /* lego alias retired — cubes only */
    return cmd_cubes();
  if (strcmp(cmd, "cubechain") == 0 || strcmp(cmd, "store") == 0)
    return cmd_cubechain(argc > 2 ? argv[2] : "tip");
  if (strcmp(cmd, "smx-selftest") == 0 || strcmp(cmd, "smx") == 0)
    return cmd_smx_selftest();
  if (strcmp(cmd, "law") == 0 || strcmp(cmd, "manifest") == 0)
    return cmd_law_manifest();
  if (strcmp(cmd, "sync") == 0 || strcmp(cmd, "hive") == 0)
    return cmd_sync(argc > 2 ? argv[2] : NULL);
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
    /* Real language entry: parse + evaluate a .cubalc source program */
    if (argc < 3) {
      fprintf(stderr, "usage: cubalc run <file.cubalc>\n");
      return 2;
    }
    cubalc_run_result rr;
    int rc;
    if (strstr(argv[2], ".cblc")) {
      cubalc_image img;
      if (cubalc_isa_load(&img, argv[2]) != 0) {
        printf("{\"ok\":false,\"cmd\":\"run\",\"err\":\"bad cblc\"}\n");
        return 2;
      }
      rc = cubalc_jit_exec(&img, &rr, stdout);
    } else {
      rc = cubalc_run_file(argv[2], &rr, stdout);
    }
    printf("{\"ok\":%s,\"cmd\":\"run\",\"file\":\"%s\",\"stmts\":%d,"
           "\"asserts_ok\":%d,\"asserts_fail\":%d,\"n\":%d,\"unity\":%.3f,"
           "\"language\":\"%s\",\"version\":\"%s\",\"err\":\"%s\"}\n",
           rr.ok ? "true" : "false", argv[2], rr.stmts, rr.asserts_ok,
           rr.asserts_fail, rr.n_cubes, rr.unity, CUBALC_LANG_NAME,
           CUBALC_LANG_VERSION, rr.err);
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
    return cmd_showcase();
  }
  if (strcmp(cmd, "evolve") == 0 || strcmp(cmd, "evolve-loop") == 0 ||
      strcmp(cmd, "self-evolve") == 0) {
    /* Pure C: braincube solves · algocube optimizes · emits .cubalc */
    return cubalc_cmd_evolve(argc - 1, argv + 1);
  }
  if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0) {
    fprintf(stderr,
      "CubalC %s\n"
      "  boot|os|run|compile|jit|cflow|disasm|translate|decide|sync|peers\n"
      "  genesis|impulse|flow|cubes|law|cubechain|smx\n"
      "  evolve [--once|--loop|--hz N|--cycles N|--reset]  # C self-improve\n"
      "  evolve-loop [--hz N]   # constant evolve→algo cycle (no Python)\n"
      "  showcase|demo|symphony # multi-act COP demonstration\n"
      "  SETDIGIT · FOLDBITS · DECIDE · COMPARE · HARMONY · JIT flow\n"
      "  ASYNC HTTP · AWAIT · PARALLEL\n"
      "  hold=%d share=%s tok=%s paradigm=%s\n",
      CUBALC_LANG_VERSION, CUBALC_HOLD_FLASH, CUBALC_SHARE, CUBALC_CREED,
      CUBALC_LANG_PARADIGM);
    return 0;
  }
  fprintf(stderr, "unknown cmd %s — try help\n", cmd);
  return 2;
}
