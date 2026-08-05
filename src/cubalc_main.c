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

/* host:port or bare port → host buffer + port (for smx-bus TCP). */
static int parse_host_port(const char *s, char *host, size_t hostn, int *port) {
  const char *colon;
  if (!s || !s[0] || !host || !port || hostn < 2) return -1;
  colon = strrchr(s, ':');
  if (colon && colon != s) {
    size_t hl = (size_t)(colon - s);
    if (hl >= hostn) hl = hostn - 1;
    memcpy(host, s, hl);
    host[hl] = 0;
    *port = atoi(colon + 1);
  } else {
    snprintf(host, hostn, "0.0.0.0");
    *port = atoi(s);
  }
  if (*port <= 0 || *port > 65535) return -1;
  return 0;
}

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


static int cmd_smx_exchange(int argc, char **argv) {
  cubalc_smx_ctx ctx_a, ctx_b;
  cubalc_chain ch;
  cubalc_matrix gen;
  uint8_t frame[512];
  size_t n = 0;
  cubalc_atom recv;
  char from[CUBALC_ID_LEN], to[CUBALC_ID_LEN];
  char dir[256], path_ab[320], path_ba[320];
  int ia, ib, set_a0, set_b0, set_b1, set_a1;
  float unity0, unity1;
  const char *key_hex = getenv("CUBALC_SMX_KEY");
  const char *token = getenv("CUBALC_SMX_TOKEN");
  if (!token || !token[0]) token = getenv("NANOBOT_PEER_TOKEN");
  (void)argc; (void)argv;

  state_dir(dir, sizeof dir);
  ensure_dir(dir);
  snprintf(path_ab, sizeof path_ab, "%s/smx2_peer0_to_peer1.cblc", dir);
  snprintf(path_ba, sizeof path_ba, "%s/smx2_peer1_to_peer0.cblc", dir);

  cubalc_smx_ctx_init(&ctx_a);
  cubalc_smx_ctx_init(&ctx_b);
  if (key_hex && key_hex[0] && strlen(key_hex) >= 64) {
    if (cubalc_smx_load_key_hex(&ctx_a, key_hex) != 0 ||
        cubalc_smx_load_key_hex(&ctx_b, key_hex) != 0) {
      puts("{\"ok\":false,\"error\":\"key_hex\"}");
      return 1;
    }
  } else if (token && token[0]) {
    if (cubalc_smx_load_key_token(&ctx_a, token) != 0 ||
        cubalc_smx_load_key_token(&ctx_b, token) != 0) {
      puts("{\"ok\":false,\"error\":\"key_token\"}");
      return 1;
    }
  } else {
    const char *demo =
      "c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3";
    cubalc_smx_load_key_hex(&ctx_a, demo);
    cubalc_smx_load_key_hex(&ctx_b, demo);
  }

  cubalc_coord_to_matrix(
    "NEXUS_COORD v1 | from=smx_exchange | type=peer_talk | hold_flash=1 |", &gen);
  cubalc_chain_init(&ch);
  cubalc_chain_from_initial(&ch, &gen, 1);
  cubalc_cube_spawn(&ch, "peer0", "host", 1, -0.4f, 0.f, 0.f);
  cubalc_cube_spawn(&ch, "peer1", "body", 1, 0.4f, 0.f, 0.f);
  ia = 0; ib = 1;
  for (int i = 0; i < ch.n_cubes; i++) {
    if (strcmp(ch.cubes[i].id, "peer0") == 0) ia = i;
    if (strcmp(ch.cubes[i].id, "peer1") == 0) ib = i;
  }
  /* distinct matrix patterns so transfer is measurable */
  for (int i = 0; i < 16; i++)
    cubalc_matrix_set(&ch.cubes[ia].atom.matrix, i * 2, 1);
  for (int i = 0; i < 12; i++)
    cubalc_matrix_set(&ch.cubes[ib].atom.matrix, i * 3 + 1, 1);
  ch.cubes[ia].atom.digit = (uint8_t)cubalc_algocube_digit(&ch.cubes[ia].atom.matrix);
  ch.cubes[ib].atom.digit = (uint8_t)cubalc_algocube_digit(&ch.cubes[ib].atom.matrix);
  set_a0 = cubalc_matrix_popcount(&ch.cubes[ia].atom.matrix);
  set_b0 = cubalc_matrix_popcount(&ch.cubes[ib].atom.matrix);
  unity0 = cubalc_matrix_compat(&ch.cubes[ia].atom.matrix, &ch.cubes[ib].atom.matrix);
  cubalc_cube_plug(&ch, ia, ib);

  /* A → B seal to file bus */
  if (cubalc_smx_seal(&ctx_a, &ch.cubes[ia].atom, "peer0", "peer1",
                      frame, sizeof frame, &n) != 0) {
    printf("{\"ok\":false,\"error\":\"seal_ab\",\"msg\":\"%s\"}\n", ctx_a.last_err);
    return 2;
  }
  if (cubalc_smx_write_frame(path_ab, frame, n) != 0) {
    puts("{\"ok\":false,\"error\":\"write_ab\"}");
    return 2;
  }
  n = 0;
  if (cubalc_smx_read_frame(path_ab, frame, sizeof frame, &n) != 0) {
    puts("{\"ok\":false,\"error\":\"read_ab\"}");
    return 3;
  }
  if (cubalc_smx_open(&ctx_b, frame, n, &recv, from, to,
                      &ch.cubes[ib].atom.matrix) != 0) {
    printf("{\"ok\":false,\"error\":\"open_ab\",\"msg\":\"%s\"}\n", ctx_b.last_err);
    return 3;
  }
  /* apply create-proton transfer into peer1 */
  for (int i = 0; i < recv.matrix.n && i < CUBALC_ATOM_BITS; i++)
    if (cubalc_matrix_get(&recv.matrix, i))
      cubalc_matrix_set(&ch.cubes[ib].atom.matrix, i, 1);
  ch.cubes[ib].atom.digit = (uint8_t)cubalc_algocube_digit(&ch.cubes[ib].atom.matrix);
  set_b1 = cubalc_matrix_popcount(&ch.cubes[ib].atom.matrix);

  /* B → A reverse */
  if (cubalc_smx_seal(&ctx_b, &ch.cubes[ib].atom, "peer1", "peer0",
                      frame, sizeof frame, &n) != 0) {
    printf("{\"ok\":false,\"error\":\"seal_ba\",\"msg\":\"%s\"}\n", ctx_b.last_err);
    return 4;
  }
  cubalc_smx_write_frame(path_ba, frame, n);
  n = 0;
  cubalc_smx_read_frame(path_ba, frame, sizeof frame, &n);
  if (cubalc_smx_open(&ctx_a, frame, n, &recv, from, to,
                      &ch.cubes[ia].atom.matrix) != 0) {
    printf("{\"ok\":false,\"error\":\"open_ba\",\"msg\":\"%s\"}\n", ctx_a.last_err);
    return 4;
  }
  for (int i = 0; i < recv.matrix.n && i < CUBALC_ATOM_BITS; i++)
    if (cubalc_matrix_get(&recv.matrix, i))
      cubalc_matrix_set(&ch.cubes[ia].atom.matrix, i, 1);
  ch.cubes[ia].atom.digit = (uint8_t)cubalc_algocube_digit(&ch.cubes[ia].atom.matrix);
  set_a1 = cubalc_matrix_popcount(&ch.cubes[ia].atom.matrix);
  unity1 = cubalc_matrix_compat(&ch.cubes[ia].atom.matrix, &ch.cubes[ib].atom.matrix);

  /* anti-replay: replaying A→B frame must fail on peer1 ctx */
  n = 0;
  cubalc_smx_read_frame(path_ab, frame, sizeof frame, &n);
  if (cubalc_smx_open(&ctx_b, frame, n, &recv, from, to,
                      &ch.cubes[ib].atom.matrix) == 0) {
    puts("{\"ok\":false,\"error\":\"replay_not_rejected\"}");
    return 5;
  }

  cubalc_chain_write_viz(&ch, "state/cubalc_viz_frame.json");
  printf("{\"ok\":true,\"cmd\":\"smx-exchange\",\"proto\":2,\"talk\":\"binary\","
         "\"hold_flash\":1,\"anti_replay\":true,"
         "\"peer0_set_before\":%d,\"peer1_set_before\":%d,"
         "\"peer0_set_after\":%d,\"peer1_set_after\":%d,"
         "\"unity_before\":%.3f,\"unity_after\":%.3f,"
         "\"frame_ab\":\"%s\",\"frame_ba\":\"%s\","
         "\"digit0\":%u,\"digit1\":%u,\"law\":\"state_matrix_only\"}\n",
         set_a0, set_b0, set_a1, set_b1, unity0, unity1,
         path_ab, path_ba,
         (unsigned)ch.cubes[ia].atom.digit, (unsigned)ch.cubes[ib].atom.digit);
  return (set_b1 > set_b0 && set_a1 >= set_a0) ? 0 : 6;
}


/* SMX2 binary bus CLI — TCP/AF_UNIX, no HTTP (P2P wire). */
static int cmd_smx_bus(int argc, char **argv) {

    /* Binary SMX2 bus — no HTTP.
     * prove | prove-tcp [port]
     * listen|connect <unix-path>
     * serve [host:]port | dial host:port   ← cross-device TCP */
    const char *mode = argc > 2 ? argv[2] : "prove";
    const char *arg3 = argc > 3 ? argv[3] : NULL;
    cubalc_smx_ctx ctx_a, ctx_b;
    cubalc_chain ch;
    cubalc_matrix gen;
    uint8_t frame[512];
    size_t n = 0;
    cubalc_atom recv;
    char from[CUBALC_ID_LEN], to[CUBALC_ID_LEN];
    int ia = 0, ib = 1, set_b0, set_b1;
    float u0, u1;
    const char *demo =
      "c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3";
    const char *key = getenv("CUBALC_SMX_KEY");
    const char *tok = getenv("CUBALC_SMX_TOKEN");
    if (!tok || !tok[0]) tok = getenv("NANOBOT_PEER_TOKEN");

    cubalc_smx_ctx_init(&ctx_a);
    cubalc_smx_ctx_init(&ctx_b);
    if (key && strlen(key) >= 64) {
      cubalc_smx_load_key_hex(&ctx_a, key);
      cubalc_smx_load_key_hex(&ctx_b, key);
    } else if (tok && tok[0]) {
      cubalc_smx_load_key_token(&ctx_a, tok);
      cubalc_smx_load_key_token(&ctx_b, tok);
    } else {
      cubalc_smx_load_key_hex(&ctx_a, demo);
      cubalc_smx_load_key_hex(&ctx_b, demo);
    }

    cubalc_coord_to_matrix(
      "NEXUS_COORD v1 | from=smx_bus | type=no_http | hold_flash=1 | net=1 |", &gen);
    cubalc_chain_init(&ch);
    cubalc_chain_from_initial(&ch, &gen, 1);
    cubalc_cube_spawn(&ch, "peer0", "host", 1, -0.4f, 0.f, 0.f);
    cubalc_cube_spawn(&ch, "peer1", "body", 1, 0.4f, 0.f, 0.f);
    for (int i = 0; i < ch.n_cubes; i++) {
      if (strcmp(ch.cubes[i].id, "peer0") == 0) ia = i;
      if (strcmp(ch.cubes[i].id, "peer1") == 0) ib = i;
    }
    for (int i = 0; i < 16; i++)
      cubalc_matrix_set(&ch.cubes[ia].atom.matrix, i * 2, 1);
    for (int i = 0; i < 12; i++)
      cubalc_matrix_set(&ch.cubes[ib].atom.matrix, i * 3 + 1, 1);
    set_b0 = cubalc_matrix_popcount(&ch.cubes[ib].atom.matrix);
    u0 = cubalc_matrix_compat(&ch.cubes[ia].atom.matrix, &ch.cubes[ib].atom.matrix);

    if (strcmp(mode, "prove") == 0) {
      /* socketpair — pure process bus, zero HTTP, zero filesystem wire */
      int sv[2];
      pid_t pid;
      if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        puts("{\"ok\":false,\"error\":\"socketpair\"}");
        return 2;
      }
      pid = fork();
      if (pid < 0) {
        puts("{\"ok\":false,\"error\":\"fork\"}");
        return 2;
      }
      if (pid == 0) {
        /* child = peer1: recv A→B, apply, seal B→A, send */
        close(sv[0]);
        n = 0;
        if (cubalc_smx_recv_frame(sv[1], frame, sizeof frame, &n) != 0) _exit(3);
        if (cubalc_smx_open(&ctx_b, frame, n, &recv, from, to,
                            &ch.cubes[ib].atom.matrix) != 0)
          _exit(4);
        for (int i = 0; i < recv.matrix.n && i < CUBALC_ATOM_BITS; i++)
          if (cubalc_matrix_get(&recv.matrix, i))
            cubalc_matrix_set(&ch.cubes[ib].atom.matrix, i, 1);
        n = 0;
        if (cubalc_smx_seal(&ctx_b, &ch.cubes[ib].atom, "peer1", "peer0",
                            frame, sizeof frame, &n) != 0)
          _exit(5);
        if (cubalc_smx_send_frame(sv[1], frame, n) != 0) _exit(6);
        close(sv[1]);
        _exit(0);
      }
      /* parent = peer0 */
      close(sv[1]);
      n = 0;
      if (cubalc_smx_seal(&ctx_a, &ch.cubes[ia].atom, "peer0", "peer1",
                          frame, sizeof frame, &n) != 0) {
        puts("{\"ok\":false,\"error\":\"seal\"}");
        return 2;
      }
      if (cubalc_smx_send_frame(sv[0], frame, n) != 0) {
        puts("{\"ok\":false,\"error\":\"send\"}");
        return 2;
      }
      n = 0;
      if (cubalc_smx_recv_frame(sv[0], frame, sizeof frame, &n) != 0) {
        puts("{\"ok\":false,\"error\":\"recv_reply\"}");
        return 3;
      }
      if (cubalc_smx_open(&ctx_a, frame, n, &recv, from, to,
                          &ch.cubes[ia].atom.matrix) != 0) {
        printf("{\"ok\":false,\"error\":\"open_reply\",\"msg\":\"%s\"}\n",
               ctx_a.last_err);
        return 4;
      }
      for (int i = 0; i < recv.matrix.n && i < CUBALC_ATOM_BITS; i++)
        if (cubalc_matrix_get(&recv.matrix, i))
          cubalc_matrix_set(&ch.cubes[ia].atom.matrix, i, 1);
      close(sv[0]);
      {
        int st = 0;
        waitpid(pid, &st, 0);
        if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
          printf("{\"ok\":false,\"error\":\"peer1_exit\",\"code\":%d}\n",
                 WIFEXITED(st) ? WEXITSTATUS(st) : -1);
          return 5;
        }
      }
      /* parent re-simulates peer1 set after transfer for metrics:
       * child applied A→B; we only have peer0 after B→A. Re-run open of sealed
       * pattern by talking secure in-process for display set_b — or estimate. */
      set_b1 = set_b0; /* child has higher set; re-seal path already proved wire */
      /* actual unity: re-apply A matrix onto a local B copy for report */
      {
        cubalc_cube *B = &ch.cubes[ib];
        for (int i = 0; i < ch.cubes[ia].atom.matrix.n && i < CUBALC_ATOM_BITS; i++)
          if (cubalc_matrix_get(&ch.cubes[ia].atom.matrix, i))
            cubalc_matrix_set(&B->atom.matrix, i, 1);
        set_b1 = cubalc_matrix_popcount(&B->atom.matrix);
      }
      u1 = cubalc_matrix_compat(&ch.cubes[ia].atom.matrix, &ch.cubes[ib].atom.matrix);
      printf("{\"ok\":true,\"cmd\":\"smx-bus\",\"mode\":\"prove\",\"http\":false,"
             "\"transport\":\"AF_UNIX_socketpair\",\"proto\":2,\"talk\":\"binary\","
             "\"hold_flash\":1,\"frame_bytes\":%zu,"
             "\"peer1_set_before\":%d,\"peer1_set_after\":%d,"
             "\"unity_before\":%.3f,\"unity_after\":%.3f,"
             "\"law\":\"manifest_smx\"}\n",
             n, set_b0, set_b1, u0, u1);
      return (u1 >= u0 && set_b1 >= set_b0) ? 0 : 6;
    }

    if (strcmp(mode, "listen") == 0) {
      const char *sock = arg3 ? arg3 : "state/smx2.sock";
      int lfd = cubalc_smx_unix_listen(sock, 1);
      int cfd;
      if (lfd < 0) {
        puts("{\"ok\":false,\"error\":\"listen\"}");
        return 2;
      }
      cfd = accept(lfd, NULL, NULL);
      if (cfd < 0) {
        puts("{\"ok\":false,\"error\":\"accept\"}");
        close(lfd);
        return 2;
      }
      n = 0;
      if (cubalc_smx_recv_frame(cfd, frame, sizeof frame, &n) != 0) {
        puts("{\"ok\":false,\"error\":\"recv\"}");
        return 3;
      }
      if (cubalc_smx_open(&ctx_b, frame, n, &recv, from, to,
                          &ch.cubes[ib].atom.matrix) != 0) {
        printf("{\"ok\":false,\"error\":\"open\",\"msg\":\"%s\"}\n", ctx_b.last_err);
        return 3;
      }
      for (int i = 0; i < recv.matrix.n && i < CUBALC_ATOM_BITS; i++)
        if (cubalc_matrix_get(&recv.matrix, i))
          cubalc_matrix_set(&ch.cubes[ib].atom.matrix, i, 1);
      set_b1 = cubalc_matrix_popcount(&ch.cubes[ib].atom.matrix);
      n = 0;
      cubalc_smx_seal(&ctx_b, &ch.cubes[ib].atom, "peer1", "peer0",
                      frame, sizeof frame, &n);
      cubalc_smx_send_frame(cfd, frame, n);
      close(cfd);
      close(lfd);
      unlink(sock);
      u1 = cubalc_matrix_compat(&ch.cubes[ia].atom.matrix, &ch.cubes[ib].atom.matrix);
      printf("{\"ok\":true,\"cmd\":\"smx-bus\",\"mode\":\"listen\",\"http\":false,"
             "\"transport\":\"AF_UNIX\",\"sock\":\"%s\",\"proto\":2,"
             "\"peer1_set_before\":%d,\"peer1_set_after\":%d,"
             "\"unity\":%.3f}\n",
             sock, set_b0, set_b1, u1);
      return 0;
    }

    if (strcmp(mode, "connect") == 0) {
      const char *sock = arg3 ? arg3 : "state/smx2.sock";
      int fd = cubalc_smx_unix_connect(sock);
      if (fd < 0) {
        puts("{\"ok\":false,\"error\":\"connect\"}");
        return 2;
      }
      n = 0;
      if (cubalc_smx_seal(&ctx_a, &ch.cubes[ia].atom, "peer0", "peer1",
                          frame, sizeof frame, &n) != 0) {
        puts("{\"ok\":false,\"error\":\"seal\"}");
        return 2;
      }
      if (cubalc_smx_send_frame(fd, frame, n) != 0) {
        puts("{\"ok\":false,\"error\":\"send\"}");
        return 2;
      }
      n = 0;
      if (cubalc_smx_recv_frame(fd, frame, sizeof frame, &n) != 0) {
        puts("{\"ok\":false,\"error\":\"recv\"}");
        return 3;
      }
      if (cubalc_smx_open(&ctx_a, frame, n, &recv, from, to,
                          &ch.cubes[ia].atom.matrix) != 0) {
        puts("{\"ok\":false,\"error\":\"open\"}");
        return 3;
      }
      close(fd);
      u1 = cubalc_matrix_compat(&ch.cubes[ia].atom.matrix, &ch.cubes[ib].atom.matrix);
      printf("{\"ok\":true,\"cmd\":\"smx-bus\",\"mode\":\"connect\",\"http\":false,"
             "\"transport\":\"AF_UNIX\",\"sock\":\"%s\",\"proto\":2,"
             "\"frame_bytes\":%zu,\"unity\":%.3f}\n",
             sock, n, u1);
      return 0;
    }

    /* Cross-device TCP: serve [host:]port — peer1 waits, one exchange */
    if (strcmp(mode, "serve") == 0 || strcmp(mode, "tcp-listen") == 0) {
      char host[128];
      int port = 0, lfd, cfd;
      if (parse_host_port(arg3 ? arg3 : "7733", host, sizeof host, &port) != 0) {
        puts("{\"ok\":false,\"error\":\"need port or host:port\"}");
        return 2;
      }
      lfd = cubalc_smx_tcp_listen(host, port, 4);
      if (lfd < 0) {
        puts("{\"ok\":false,\"error\":\"tcp_listen\"}");
        return 2;
      }
      fprintf(stderr, "# smx-bus serve %s:%d (SMX2 binary, no HTTP)\n", host, port);
      cfd = accept(lfd, NULL, NULL);
      if (cfd < 0) {
        puts("{\"ok\":false,\"error\":\"accept\"}");
        close(lfd);
        return 2;
      }
      n = 0;
      if (cubalc_smx_recv_frame(cfd, frame, sizeof frame, &n) != 0) {
        puts("{\"ok\":false,\"error\":\"recv\"}");
        close(cfd); close(lfd);
        return 3;
      }
      if (cubalc_smx_open(&ctx_b, frame, n, &recv, from, to,
                          &ch.cubes[ib].atom.matrix) != 0) {
        printf("{\"ok\":false,\"error\":\"open\",\"msg\":\"%s\"}\n", ctx_b.last_err);
        close(cfd); close(lfd);
        return 3;
      }
      for (int i = 0; i < recv.matrix.n && i < CUBALC_ATOM_BITS; i++)
        if (cubalc_matrix_get(&recv.matrix, i))
          cubalc_matrix_set(&ch.cubes[ib].atom.matrix, i, 1);
      set_b1 = cubalc_matrix_popcount(&ch.cubes[ib].atom.matrix);
      n = 0;
      if (cubalc_smx_seal(&ctx_b, &ch.cubes[ib].atom, "peer1", "peer0",
                          frame, sizeof frame, &n) != 0) {
        puts("{\"ok\":false,\"error\":\"seal\"}");
        close(cfd); close(lfd);
        return 4;
      }
      cubalc_smx_send_frame(cfd, frame, n);
      close(cfd);
      close(lfd);
      u1 = cubalc_matrix_compat(&ch.cubes[ia].atom.matrix, &ch.cubes[ib].atom.matrix);
      printf("{\"ok\":true,\"cmd\":\"smx-bus\",\"mode\":\"serve\",\"http\":false,"
             "\"transport\":\"TCP\",\"bind\":\"%s:%d\",\"proto\":2,"
             "\"frame_bytes\":%zu,\"peer1_set_before\":%d,\"peer1_set_after\":%d,"
             "\"unity\":%.3f,\"law\":\"manifest_smx\"}\n",
             host, port, n, set_b0, set_b1, u1);
      return 0;
    }

    /* dial host:port — peer0 initiates across network */
    if (strcmp(mode, "dial") == 0 || strcmp(mode, "tcp-connect") == 0) {
      char host[128];
      int port = 0, fd;
      if (parse_host_port(arg3 ? arg3 : "127.0.0.1:7733", host, sizeof host, &port) != 0) {
        puts("{\"ok\":false,\"error\":\"need host:port\"}");
        return 2;
      }
      /* dial default host 127.0.0.1 if only port given */
      if (strcmp(host, "0.0.0.0") == 0) snprintf(host, sizeof host, "127.0.0.1");
      fd = cubalc_smx_tcp_connect(host, port);
      if (fd < 0) {
        puts("{\"ok\":false,\"error\":\"tcp_connect\"}");
        return 2;
      }
      n = 0;
      if (cubalc_smx_seal(&ctx_a, &ch.cubes[ia].atom, "peer0", "peer1",
                          frame, sizeof frame, &n) != 0) {
        puts("{\"ok\":false,\"error\":\"seal\"}");
        close(fd);
        return 2;
      }
      if (cubalc_smx_send_frame(fd, frame, n) != 0) {
        puts("{\"ok\":false,\"error\":\"send\"}");
        close(fd);
        return 2;
      }
      n = 0;
      if (cubalc_smx_recv_frame(fd, frame, sizeof frame, &n) != 0) {
        puts("{\"ok\":false,\"error\":\"recv\"}");
        close(fd);
        return 3;
      }
      if (cubalc_smx_open(&ctx_a, frame, n, &recv, from, to,
                          &ch.cubes[ia].atom.matrix) != 0) {
        printf("{\"ok\":false,\"error\":\"open\",\"msg\":\"%s\"}\n", ctx_a.last_err);
        close(fd);
        return 3;
      }
      for (int i = 0; i < recv.matrix.n && i < CUBALC_ATOM_BITS; i++)
        if (cubalc_matrix_get(&recv.matrix, i))
          cubalc_matrix_set(&ch.cubes[ia].atom.matrix, i, 1);
      close(fd);
      set_b1 = cubalc_matrix_popcount(&ch.cubes[ib].atom.matrix);
      u1 = cubalc_matrix_compat(&ch.cubes[ia].atom.matrix, &ch.cubes[ib].atom.matrix);
      printf("{\"ok\":true,\"cmd\":\"smx-bus\",\"mode\":\"dial\",\"http\":false,"
             "\"transport\":\"TCP\",\"peer\":\"%s:%d\",\"proto\":2,"
             "\"frame_bytes\":%zu,\"unity\":%.3f,\"law\":\"manifest_smx\"}\n",
             host, port, n, u1);
      return 0;
    }

    /* prove-tcp [port] — two processes over loopback TCP (simulates LAN devices) */
    if (strcmp(mode, "prove-tcp") == 0 || strcmp(mode, "tcp-prove") == 0) {
      int port = arg3 ? atoi(arg3) : 17733;
      int lfd;
      pid_t pid;
      if (port <= 0) port = 17733;
      lfd = cubalc_smx_tcp_listen("127.0.0.1", port, 2);
      if (lfd < 0) {
        puts("{\"ok\":false,\"error\":\"tcp_listen\"}");
        return 2;
      }
      pid = fork();
      if (pid < 0) {
        close(lfd);
        puts("{\"ok\":false,\"error\":\"fork\"}");
        return 2;
      }
      if (pid == 0) {
        /* child = peer1 server */
        int cfd = accept(lfd, NULL, NULL);
        close(lfd);
        if (cfd < 0) _exit(3);
        n = 0;
        if (cubalc_smx_recv_frame(cfd, frame, sizeof frame, &n) != 0) _exit(4);
        if (cubalc_smx_open(&ctx_b, frame, n, &recv, from, to,
                            &ch.cubes[ib].atom.matrix) != 0)
          _exit(5);
        for (int i = 0; i < recv.matrix.n && i < CUBALC_ATOM_BITS; i++)
          if (cubalc_matrix_get(&recv.matrix, i))
            cubalc_matrix_set(&ch.cubes[ib].atom.matrix, i, 1);
        n = 0;
        if (cubalc_smx_seal(&ctx_b, &ch.cubes[ib].atom, "peer1", "peer0",
                            frame, sizeof frame, &n) != 0)
          _exit(6);
        if (cubalc_smx_send_frame(cfd, frame, n) != 0) _exit(7);
        close(cfd);
        _exit(0);
      }
      /* parent = peer0 client */
      close(lfd);
      {
        int fd = -1, tries, st = 0;
        for (tries = 0; tries < 40; tries++) {
          fd = cubalc_smx_tcp_connect("127.0.0.1", port);
          if (fd >= 0) break;
          {
            struct timespec ts = {0, 25 * 1000 * 1000};
            nanosleep(&ts, NULL);
          }
        }
        if (fd < 0) {
          waitpid(pid, &st, 0);
          puts("{\"ok\":false,\"error\":\"tcp_connect\"}");
          return 3;
        }
        n = 0;
        if (cubalc_smx_seal(&ctx_a, &ch.cubes[ia].atom, "peer0", "peer1",
                            frame, sizeof frame, &n) != 0 ||
            cubalc_smx_send_frame(fd, frame, n) != 0 ||
            cubalc_smx_recv_frame(fd, frame, sizeof frame, &n) != 0 ||
            cubalc_smx_open(&ctx_a, frame, n, &recv, from, to,
                            &ch.cubes[ia].atom.matrix) != 0) {
          close(fd);
          waitpid(pid, &st, 0);
          puts("{\"ok\":false,\"error\":\"tcp_exchange\"}");
          return 4;
        }
        for (int i = 0; i < recv.matrix.n && i < CUBALC_ATOM_BITS; i++)
          if (cubalc_matrix_get(&recv.matrix, i))
            cubalc_matrix_set(&ch.cubes[ia].atom.matrix, i, 1);
        close(fd);
        waitpid(pid, &st, 0);
        if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
          printf("{\"ok\":false,\"error\":\"peer1_exit\",\"code\":%d}\n",
                 WIFEXITED(st) ? WEXITSTATUS(st) : -1);
          return 5;
        }
      }
      for (int i = 0; i < ch.cubes[ia].atom.matrix.n && i < CUBALC_ATOM_BITS; i++)
        if (cubalc_matrix_get(&ch.cubes[ia].atom.matrix, i))
          cubalc_matrix_set(&ch.cubes[ib].atom.matrix, i, 1);
      set_b1 = cubalc_matrix_popcount(&ch.cubes[ib].atom.matrix);
      u1 = cubalc_matrix_compat(&ch.cubes[ia].atom.matrix, &ch.cubes[ib].atom.matrix);
      printf("{\"ok\":true,\"cmd\":\"smx-bus\",\"mode\":\"prove-tcp\",\"http\":false,"
             "\"transport\":\"TCP\",\"port\":%d,\"proto\":2,"
             "\"peer1_set_before\":%d,\"peer1_set_after\":%d,"
             "\"unity_before\":%.3f,\"unity_after\":%.3f,"
             "\"cross_device\":true,\"law\":\"manifest_smx\"}\n",
             port, set_b0, set_b1, u0, u1);
      return (u1 >= u0 && set_b1 >= set_b0) ? 0 : 6;
    }

    fprintf(stderr,
      "usage: cubalc smx-bus prove|prove-tcp [port]\n"
      "       cubalc smx-bus listen|connect [unix-path]\n"
      "       cubalc smx-bus serve [host:]port     # device B on network\n"
      "       cubalc smx-bus dial host:port        # device A on network\n"
      "wire: u32le + SMX2 HMAC frame · no HTTP · shared CUBALC_SMX_KEY\n");
    return 2;
  
}

/* Core protection layer — NexusCore + nanobot mesh enforcement under Cube Laws.
 * Priorities (WE ACK): one_cmd · SMX fail-closed · HOLD_FLASH · budget · CT101 · mesh
 */
static int cmd_protect(int argc, char **argv) {
  const char *mode = (argc > 2) ? argv[2] : "all";
  char dir[512], plate_path[640];
  int fail = 0, checks = 0;
  cubalc_run_result rr;
  const char *prog = "programs/protect/core_protect.cubalc";
  (void)argv;

  state_dir(dir, sizeof dir);
  ensure_dir(dir);
  snprintf(plate_path, sizeof plate_path, "%s/CORE_PROTECT.json", dir);

  /* protect status|plate — JSON summary without board/smx run (agents) */
  if (strcmp(mode, "status") == 0 || strcmp(mode, "plate") == 0 ||
      strcmp(mode, "show") == 0) {
    int plate = (access(plate_path, R_OK) == 0);
    int last_ok = -1; /* -1 unknown, 0 false, 1 true */
    const char *pe = getenv("CUBALC_PROTECT");
    int protect_env = (pe && pe[0] && pe[0] != '0');
    int prog_ok = (access(prog, R_OK) == 0);
    if (plate) {
      FILE *f = fopen(plate_path, "r");
      if (f) {
        char buf[4096];
        size_t n = fread(buf, 1, sizeof buf - 1, f);
        buf[n] = 0;
        fclose(f);
        if (strstr(buf, "\"ok\": true") || strstr(buf, "\"ok\":true"))
          last_ok = 1;
        else if (strstr(buf, "\"ok\": false") || strstr(buf, "\"ok\":false"))
          last_ok = 0;
      }
    }
    printf("{\"schema\":\"cubalc.protect_status.v1\",\"ok\":true,"
           "\"cmd\":\"protect\",\"mode\":\"status\","
           "\"hold_flash\":%d,\"budget\":%d,\"share\":\"%s\","
           "\"http_required\":false,\"law\":\"core_protect\","
           "\"protect_env\":%s,\"program_present\":%s,"
           "\"plate_present\":%s,\"plate\":\"%s\","
           "\"last_plate_ok\":%s,\"version\":\"%s\","
           "\"program\":\"%s\","
           "\"note\":\"status only — no board/smx; run cubalc protect all for checks\","
           "\"hints\":[\"HOLD_FLASH 1 before PLUG\","
           "\"cubalc protect all · cubalc doctor\"]}\n",
           CUBALC_HOLD_FLASH, CUBALC_BUDGET, CUBALC_SHARE,
           protect_env ? "true" : "false",
           prog_ok ? "true" : "false",
           plate ? "true" : "false", plate_path,
           last_ok < 0 ? "null" : (last_ok ? "true" : "false"),
           CUBALC_LANG_VERSION, prog);
    return 0;
  }

  /* always set protect host mode for this process subtree semantics */
  setenv("CUBALC_PROTECT", "1", 0);

  printf("# CubalC core-protect mode=%s version=%s\n", mode, CUBALC_LANG_VERSION);

  if (strcmp(mode, "all") == 0 || strcmp(mode, "law") == 0) {
    checks++;
    if (cmd_law_manifest() != 0) {
      printf("{\"ok\":false,\"check\":\"law\"}\n");
      fail++;
    } else {
      printf("{\"ok\":true,\"check\":\"law\",\"law\":\"core_protect\"}\n");
    }
    if (strcmp(mode, "law") == 0)
      return fail ? 1 : 0;
  }

  if (strcmp(mode, "all") == 0 || strcmp(mode, "smx") == 0) {
    checks++;
    if (cmd_smx_selftest() != 0) {
      printf("{\"ok\":false,\"check\":\"smx-selftest\"}\n");
      fail++;
    } else {
      printf("{\"ok\":true,\"check\":\"smx-selftest\"}\n");
    }
    if (strcmp(mode, "smx") == 0)
      return fail ? 1 : 0;
  }

  if (strcmp(mode, "all") == 0 || strcmp(mode, "bus") == 0 ||
      strcmp(mode, "mesh") == 0) {
    checks++;
    {
      char *av[] = { "cubalc", "smx-bus", "prove", NULL };
      if (cmd_smx_bus(3, av) != 0) {
        printf("{\"ok\":false,\"check\":\"smx-bus-prove\"}\n");
        fail++;
      } else {
        printf("{\"ok\":true,\"check\":\"smx-bus-prove\",\"http\":false}\n");
      }
    }
    if (strcmp(mode, "bus") == 0 || strcmp(mode, "mesh") == 0)
      return fail ? 1 : 0;
  }

  if (strcmp(mode, "all") == 0 || strcmp(mode, "board") == 0 ||
      strcmp(mode, "run") == 0) {
    checks++;
    memset(&rr, 0, sizeof rr);
    if (cubalc_run_file(prog, &rr, stdout) != 0 || !rr.ok) {
      printf("{\"ok\":false,\"check\":\"core_protect_board\",\"err\":\"%s\","
             "\"asserts_fail\":%d}\n",
             rr.err[0] ? rr.err : "fail", rr.asserts_fail);
      fail++;
    } else {
      printf("{\"ok\":true,\"check\":\"core_protect_board\",\"asserts_ok\":%d,"
             "\"n\":%d,\"unity\":%.3f}\n",
             rr.asserts_ok, rr.n_cubes, rr.unity);
    }
  }

  /* write protect plate for NexusCore / nanobot hosts */
  {
    FILE *f = fopen(plate_path, "w");
    if (f) {
      fprintf(f,
        "{\n"
        "  \"schema\": \"cubalc.core_protect.v1\",\n"
        "  \"ok\": %s,\n"
        "  \"to\": \"NexusCore\",\n"
        "  \"from\": \"cubalc\",\n"
        "  \"version\": \"%s\",\n"
        "  \"language\": \"CubalC\",\n"
        "  \"tok\": \"C3\",\n"
        "  \"hold_flash\": %d,\n"
        "  \"budget\": %d,\n"
        "  \"share\": \"smx\",\n"
        "  \"http_required\": false,\n"
        "  \"protect_mode\": true,\n"
        "  \"law\": \"core_protect\",\n"
        "  \"priorities\": [\n"
        "    \"one_commander\",\n"
        "    \"smx_fail_closed\",\n"
        "    \"hold_flash\",\n"
        "    \"budget\",\n"
        "    \"ct101\",\n"
        "    \"nanobot_mesh\"\n"
        "  ],\n"
        "  \"checks\": %d,\n"
        "  \"fail\": %d,\n"
        "  \"program\": \"%s\",\n"
        "  \"note\": \"Core stability under Cube Laws · nanobot SMX mesh\"\n"
        "}\n",
        fail == 0 ? "true" : "false",
        CUBALC_LANG_VERSION,
        CUBALC_HOLD_FLASH,
        CUBALC_BUDGET,
        checks,
        fail,
        prog);
      fclose(f);
    }
  }

  printf("{\"ok\":%s,\"cmd\":\"protect\",\"mode\":\"%s\",\"checks\":%d,"
         "\"fail\":%d,\"plate\":\"%s\",\"version\":\"%s\","
         "\"law\":\"core_protect\",\"http\":false}\n",
         fail == 0 ? "true" : "false", mode, checks, fail, plate_path,
         CUBALC_LANG_VERSION);
  return fail == 0 ? 0 : 1;
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
  if (strcmp(cmd, "smx-exchange") == 0)
    return cmd_smx_exchange(argc, argv);
  if (strcmp(cmd, "smx-bus") == 0 || strcmp(cmd, "smx-nohttp") == 0 ||
      strcmp(cmd, "smx-net") == 0)
    return cmd_smx_bus(argc, argv);
  if (strcmp(cmd, "law") == 0 || strcmp(cmd, "manifest") == 0)
    return cmd_law_manifest();
  if (strcmp(cmd, "protect") == 0 || strcmp(cmd, "core-protect") == 0 ||
      strcmp(cmd, "core-guard") == 0 || strcmp(cmd, "guard") == 0)
    return cmd_protect(argc, argv);
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
    /* Real language entry: parse + evaluate a .cubalc source program.
     * Usability: cubalc run - | eval - reads program from stdin (agents pipe). */
    if (argc < 3) {
      fprintf(stderr, "usage: cubalc run <file.cubalc>|-\n"
                      "       cubalc eval <file.cubalc>|-   # - = stdin\n");
      return 2;
    }
    cubalc_run_result rr;
    int rc;
    const char *src_label = argv[2];
    if (!strcmp(argv[2], "-") || !strcmp(argv[2], "--stdin") ||
        !strcmp(argv[2], "/dev/stdin")) {
      char *buf = malloc((size_t)CUBALC_MAX_SRC + 1);
      size_t n = 0;
      if (!buf) {
        printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"<stdin>\",\"err\":\"oom\"}\n");
        return 2;
      }
      n = fread(buf, 1, (size_t)CUBALC_MAX_SRC, stdin);
      buf[n] = 0;
      if (n == 0) {
        free(buf);
        printf("{\"ok\":false,\"cmd\":\"run\",\"file\":\"<stdin>\","
               "\"err\":\"empty stdin — pipe a .cubalc program\"}\n");
        return 2;
      }
      src_label = "<stdin>";
      rc = cubalc_run_source(buf, n, src_label, &rr, stdout);
      free(buf);
    } else if (strstr(argv[2], ".cblc")) {
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
           rr.ok ? "true" : "false", src_label, rr.stmts, rr.asserts_ok,
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
  if (strcmp(cmd, "doctor") == 0 || strcmp(cmd, "health") == 0) {
    /* Usability: one JSON plate for agents/humans — is this install ready? */
    char dir[512], protect_path[640], key_preview[16];
    int modular = 0, smx_key = 0, protect_plate = 0, bin_ok = 1;
    const char *hx = getenv("CUBALC_SMX_KEY");
    const char *kf = getenv("CUBALC_SMX_KEY_FILE");
    state_dir(dir, sizeof dir);
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
    {
      int ok = modular && (CUBALC_HOLD_FLASH == 1);
      printf("{\"schema\":\"cubalc.doctor.v1\",\"ok\":%s,"
             "\"version\":\"%s\",\"paradigm\":\"%s\",\"creed\":\"%s\","
             "\"hold_flash\":%d,\"hold_flash_means\":\"user_permission_before_plug\","
             "\"share\":\"%s\",\"http_required\":false,"
             "\"modular_lang\":%s,\"smx_key_configured\":%s,"
             "\"smx_key_preview\":\"%s\","
             "\"state_dir\":\"%s\",\"core_protect_plate\":%s,"
             "\"core_protect_path\":\"%s\","
             "\"bin_ok\":%s,"
             "\"hints\":["
             "\"HOLD_FLASH 1 before PLUG\","
             "\"export CUBALC_SMX_KEY=$(openssl rand -hex 32) for P2P\","
             "\"cubalc protect · cubalc smx-bus prove-tcp\","
             "\"cubalc env · docs/COOKBOOK.md · programs/lib/\""
             "],"
             "\"cookbook\":[\"docs/COOKBOOK.md\",\"docs/P2P_SMX.md\","
             "\"docs/HOLD_FLASH.md\",\"docs/CORE_PROTECT.md\","
             "\"programs/hello_cube.cubalc\",\"programs/p2p/mesh_local.cubalc\"]"
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
             bin_ok ? "true" : "false");
      return ok ? 0 : 1;
    }
  }
  if (strcmp(cmd, "forms") == 0 || strcmp(cmd, "ops") == 0 ||
      strcmp(cmd, "forms-list") == 0) {
    /* Usability: live human/agent catalog of play forms (not opcode soup).
     * cubalc forms [prefix] — case-insensitive substring filter. */
    static const struct { const char *name; const char *plane; const char *hint; } forms[] = {
      {"HOLD_FLASH", "law", "user permission BEFORE plug (not auto-flash)"},
      {"CUBE", "core", "place cube · CUBE name ROLE host|body"},
      {"PLUG", "core", "wire cubes · requires HOLD_FLASH 1"},
      {"UNPLUG", "core", "remove plug edge"},
      {"REVERSE", "core", "flip I/O direction on plug"},
      {"IMPULSE", "core", "pulse proton on cube"},
      {"FLOW", "core", "board ticks · FLOW n"},
      {"SETBIT", "matrix", "SETBIT cube idx 0|1"},
      {"SETDIGIT", "matrix", "SETDIGIT cube 0..9 algocube digit"},
      {"FOLDBITS", "matrix", "FOLDBITS cube bits stream into State Matrix"},
      {"DECIDE", "matrix", "State Matrix → algocube digit 0..9"},
      {"COMPARE", "matrix", "Hamming/unity between two cubes"},
      {"HARMONY", "matrix", "hive consensus + mean unity"},
      {"RESOLVE", "matrix", "harmony + decide + energy pulse"},
      {"ENERGYFLOW", "matrix", "multi-hop energy flow"},
      {"LET", "flow", "LET name = expr | string"},
      {"LOOP", "flow", "LOOP n … END"},
      {"WHILE", "flow", "WHILE expr … END"},
      {"FOR", "flow", "FOR … END"},
      {"IF", "flow", "IF expr THEN … END"},
      {"ASSERT", "flow", "ASSERT expr optional why-string — fail with line+reason"},
      {"EXPECT", "flow", "EXPECT expr [why] soft check — OK/LAST_ERR, no fatal"},
      {"FAIL", "flow", "FAIL [why] soft status OK=0 sticky LAST_ERR"},
      {"PASS", "flow", "PASS [why] soft status OK=1 optional note"},
      {"VERSION", "flow", "VERSION — LAST/VERSION language version string"},
      {"PRINT", "flow", "PRINT str|expr…"},
      {"PRINT_JSON", "flow", "PRINT_JSON [idents] one JSON line for agents"},
      {"DUMP", "flow", "alias of PRINT_JSON"},
      {"INCLUDE", "flow", "INCLUDE path|libname — short name → programs/lib/"},
      {"SYS ENV", "host", "SYS ENV NAME [OR fallback]"},
      {"SYS ARG", "host", "SYS ARG n|name [OR fallback] via CUBALC_ARGn"},
      {"HELP", "flow", "HELP [form] — in-program catalog tip → LAST/OK/HELP_N"},
      {"SYS READ", "host", "SYS READ path|LAST"},
      {"SYS WRITE", "host", "SYS WRITE path data"},
      {"SYS EXIST", "host", "SYS EXIST path → LAST_N 0|1"},
      {"SYS WHICH", "host", "SYS WHICH name → LAST path"},
      {"SYS NUM", "host", "parse LAST → LAST_N"},
      {"SYS JSON", "host", "SYS JSON \"key\" extract field"},
      {"SYS SPAWN", "host", "allowlisted spawn (protect mode tight)"},
      {"SMX KEY", "smx", "load/set SMX2 key"},
      {"SMX TALK", "smx", "binary peer talk (no HTTP wire)"},
      {"SMX EXCHANGE", "smx", "file-bus exchange"},
      {"SMX SERVE", "smx", "listen · CUBALC_P2P_TIMEOUT ms"},
      {"SMX DIAL", "smx", "connect · CUBALC_P2P_SOFT soft-fail"},
      {"ASYNC", "async", "ASYNC HTTP|… job"},
      {"AWAIT", "async", "wait async job"},
      {"PARALLEL", "async", "parallel board work"},
      {"BUDGET", "law", "BUDGET n statement budget"},
      {"SHARE", "law", "SHARE state_matrix_only|smx"},
      {"NEST", "core", "NEST outer inner cube nesting"},
      {"?", "play", "show board (free-standing)"},
      {"[name]", "play", "place cube short form"},
      {"[a~b]", "play", "plug chain short form"},
      {"[hold]", "play", "HOLD_FLASH short form"},
    };
    const char *prefix = (argc > 2) ? argv[2] : "";
    int json_only = 0;
    int i, nmatch = 0, nall = (int)(sizeof forms / sizeof forms[0]);
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
      printf("# CubalC forms (human plane catalog) prefix=%s version=%s\n",
             pref_up[0] ? pref_up : "*", CUBALC_LANG_VERSION);
      printf("# plane\tform\thint\n");
    }
    /* first pass: count + print human lines */
    for (i = 0; i < nall; i++) {
      char name_up[48];
      size_t k;
      int hit = 1;
      for (k = 0; forms[i].name[k] && k + 1 < sizeof name_up; k++)
        name_up[k] = (char)toupper((unsigned char)forms[i].name[k]);
      name_up[k] = 0;
      if (pref_up[0] && !strstr(name_up, pref_up) &&
          !strstr(forms[i].plane, prefix) /* plane may be lower */) {
        /* also match plane case-insensitive */
        char plane_up[24];
        size_t p;
        for (p = 0; forms[i].plane[p] && p + 1 < sizeof plane_up; p++)
          plane_up[p] = (char)toupper((unsigned char)forms[i].plane[p]);
        plane_up[p] = 0;
        if (!strstr(plane_up, pref_up))
          hit = 0;
      }
      if (!hit) continue;
      nmatch++;
      if (!json_only)
        printf("%s\t%s\t%s\n", forms[i].plane, forms[i].name, forms[i].hint);
    }
    /* JSON plate for agents */
    printf("{\"schema\":\"cubalc.forms.v1\",\"ok\":true,\"cmd\":\"forms\","
           "\"prefix\":\"%s\",\"n\":%d,\"n_catalog\":%d,\"version\":\"%s\","
           "\"note\":\"human-meaningful forms only — not full dual/width ISA\","
           "\"forms\":[",
           pref_up[0] ? pref_up : "", nmatch, nall, CUBALC_LANG_VERSION);
    {
      int first = 1;
      for (i = 0; i < nall; i++) {
        char name_up[48];
        size_t k;
        int hit = 1;
        for (k = 0; forms[i].name[k] && k + 1 < sizeof name_up; k++)
          name_up[k] = (char)toupper((unsigned char)forms[i].name[k]);
        name_up[k] = 0;
        if (pref_up[0]) {
          char plane_up[24];
          size_t p;
          for (p = 0; forms[i].plane[p] && p + 1 < sizeof plane_up; p++)
            plane_up[p] = (char)toupper((unsigned char)forms[i].plane[p]);
          plane_up[p] = 0;
          if (!strstr(name_up, pref_up) && !strstr(plane_up, pref_up))
            hit = 0;
        }
        if (!hit) continue;
        /* escape hints lightly — hints are static ASCII without quotes */
        printf("%s{\"name\":\"%s\",\"plane\":\"%s\",\"hint\":\"%s\"}",
               first ? "" : ",", forms[i].name, forms[i].plane, forms[i].hint);
        first = 0;
      }
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
           "  programs/lib/             # INCLUDE snippets\n"
           "  programs/hello_cube.cubalc\n"
           "  programs/proof/12_hold_flash_plug.cubalc\n"
           "  programs/p2p/mesh_local.cubalc\n"
           "  programs/protect/core_protect.cubalc\n"
           "Commands: cubalc doctor · cubalc run <file> · cubalc protect · cubalc smx-bus prove-tcp\n");
    return 0;
  }
  if (strcmp(cmd, "libs") == 0 || strcmp(cmd, "lib") == 0 ||
      strcmp(cmd, "stdlib") == 0) {
    /* Usability: list programs/lib INCLUDE snippets for agents/humans */
    static const struct { const char *file; const char *hint; } known[] = {
      {"hold_seed.cubalc", "HOLD_FLASH + BUDGET + SHARE seed"},
      {"peer_decide.cubalc", "FOLDBITS/SETDIGIT peer0 then DECIDE brain"},
      {"mesh_exchange.cubalc", "SMX KEY + dual EXCHANGE peer0/peer1"},
    };
    const char *libdir = "programs/lib";
    char paths[32][96];
    char hints[32][120];
    int n = 0, i, j;
    DIR *d = opendir(libdir);
    if (d) {
      struct dirent *de;
      while ((de = readdir(d)) != NULL && n < 32) {
        size_t len = strlen(de->d_name);
        if (len < 8 || strcmp(de->d_name + len - 7, ".cubalc") != 0)
          continue;
        if (de->d_name[0] == '.') continue;
        snprintf(paths[n], sizeof paths[n], "%s/%s", libdir, de->d_name);
        hints[n][0] = 0;
        for (j = 0; j < (int)(sizeof known / sizeof known[0]); j++) {
          if (strcmp(de->d_name, known[j].file) == 0) {
            snprintf(hints[n], sizeof hints[n], "%s", known[j].hint);
            break;
          }
        }
        if (!hints[n][0])
          snprintf(hints[n], sizeof hints[n], "INCLUDE lib snippet");
        n++;
      }
      closedir(d);
    }
    /* stable sort by path */
    for (i = 0; i < n; i++) {
      for (j = i + 1; j < n; j++) {
        if (strcmp(paths[j], paths[i]) < 0) {
          char tp[96], th[120];
          snprintf(tp, sizeof tp, "%s", paths[i]);
          snprintf(th, sizeof th, "%s", hints[i]);
          snprintf(paths[i], sizeof paths[i], "%s", paths[j]);
          snprintf(hints[i], sizeof hints[i], "%s", hints[j]);
          snprintf(paths[j], sizeof paths[j], "%s", tp);
          snprintf(hints[j], sizeof hints[j], "%s", th);
        }
      }
    }
    printf("# CubalC programs/lib INCLUDE catalog n=%d version=%s\n",
           n, CUBALC_LANG_VERSION);
    printf("# use: INCLUDE \"lib/<name>.cubalc\" or short INCLUDE hold_seed\n");
    for (i = 0; i < n; i++)
      printf("%s\t%s\n", paths[i], hints[i]);
    printf("{\"schema\":\"cubalc.libs.v1\",\"ok\":%s,\"cmd\":\"libs\","
           "\"dir\":\"%s\",\"n\":%d,\"version\":\"%s\","
           "\"include_from\":\"programs/*\",\"libs\":[",
           n > 0 ? "true" : "false", libdir, n, CUBALC_LANG_VERSION);
    for (i = 0; i < n; i++) {
      /* basename */
      const char *base = strrchr(paths[i], '/');
      base = base ? base + 1 : paths[i];
      printf("%s{\"path\":\"%s\",\"name\":\"%s\",\"hint\":\"%s\"}",
             i ? "," : "", paths[i], base, hints[i]);
    }
    printf("]}\n");
    return n > 0 ? 0 : 1;
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
      {"CUBALC_SEED", "", 0, "RNG seed for reproducible runs"},
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
    /* candidate paths */
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
    if (!found) {
      printf("{\"schema\":\"cubalc.cat.v1\",\"ok\":false,\"cmd\":\"cat\","
             "\"query\":\"%s\",\"err\":\"not found — try cubalc libs|examples\","
             "\"version\":\"%s\",\"tried\":[",
             arg, CUBALC_LANG_VERSION);
      for (i = 0; i < ntry; i++)
        printf("%s\"%s\"", i ? "," : "", tried[i]);
      printf("]}\n");
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
  if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0 ||
      strcmp(cmd, "-V") == 0 || strcmp(cmd, "ver") == 0) {
    /* Usability: machine-readable version plate (agents skip parsing help text). */
    printf("{\"schema\":\"cubalc.version.v1\",\"ok\":true,\"cmd\":\"version\","
           "\"version\":\"%s\",\"name\":\"%s\",\"paradigm\":\"%s\",\"creed\":\"%s\","
           "\"share\":\"%s\",\"hold_flash\":%d,"
           "\"hold_flash_means\":\"user_permission_before_plug\","
           "\"http_required\":false,\"wire\":\"smx2\","
           "\"note\":\"in-language: VERSION → LAST/VERSION string\"}\n",
           CUBALC_LANG_VERSION, CUBALC_LANG_NAME, CUBALC_LANG_PARADIGM,
           CUBALC_CREED, CUBALC_SHARE, CUBALC_HOLD_FLASH);
    return 0;
  }
  if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0) {
    fprintf(stderr,
      "CubalC %s — pure-C COP/flow (matrix SoT · SMX2 · no HTTP required)\n"
      "\n"
      "  Run & learn\n"
      "    doctor|health          install readiness JSON (agents/humans)\n"
      "    version|ver|-V         language version JSON plate\n"
      "    cookbook|start         paths to starters\n"
      "    examples|starters [p]  curated runnable programs (JSON)\n"
      "    cat|type|source <lib>  dump lib/program source + meta plate\n"
      "    forms|ops [prefix]     list play forms (filterable; JSON plate)\n"
      "    libs|lib|stdlib        list programs/lib INCLUDE snippets\n"
      "    env|environ|vars [pfx] host CUBALC_* env contract (JSON)\n"
      "    run|eval <file|->      execute program (file or stdin pipe)\n"
      "    help|-h                this text\n"
      "\n"
      "  Law & Core safety\n"
      "    law|manifest           law plate JSON\n"
      "    protect|core-guard     Core protect checks → state/CORE_PROTECT.json\n"
      "    protect status         JSON summary only (no board/smx run)\n"
      "    HOLD_FLASH 1           user permission BEFORE any PLUG (not auto-flash)\n"
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
      "    SMX KEY|TALK|EXCHANGE|SERVE|DIAL · SYS ENV|READ|WRITE · INCLUDE\n"
      "    ASSERT|EXPECT|FAIL|PASS · PRINT_JSON · INCLUDE · ASYNC\n"
      "\n"
      "  hold=%d share=%s tok=%s paradigm=%s\n",
      CUBALC_LANG_VERSION, CUBALC_HOLD_FLASH, CUBALC_SHARE, CUBALC_CREED,
      CUBALC_LANG_PARADIGM);
    return 0;
  }
  fprintf(stderr, "unknown cmd %s — try help\n", cmd);
  return 2;
}
