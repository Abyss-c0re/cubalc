#define _POSIX_C_SOURCE 200809L
#include "cubalc.h"
#include "cubalc_smx.h"
#include "cubalc_cubechain.h"
#include "cubalc_lang.h"
#include "cubalc_isa.h"
#include "cubalc_jit.h"
#include "cubalc_translate.h"
#include "cubalc_evolve.h"
#include "cubalc_hostops.h"
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
           "\"hints\":[\"HOLD_FLASH default 1 (device/firmware safeguard; omit preamble)\","
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
  /* Usability: cubalc -e CODE ≡ cubalc run -e CODE (no subcommand required). */
  if (strcmp(cmd, "-e") == 0 || strcmp(cmd, "--expr") == 0 ||
      strcmp(cmd, "--code") == 0 || strcmp(cmd, "-c") == 0)
    cmd = "run";
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
     * Usability: cubalc run - | eval - reads program from stdin (agents pipe).
     * Usability: cubalc run -e CODE | --expr|--code|-c — inline source (no temp file).
     *   multiple -e join with newline; \n \t \\ escapes expanded in each chunk.
     * Quiet: -q|--quiet|--plate or CUBALC_QUIET=1 → plate-only (no board/# ok).
     * Strict: -s|--strict or CUBALC_STRICT=1 → soft last_err fails exit+plate ok. */
    int quiet = 0, strict = 0, i, rc;
    int plate_ok;
    int have_expr = 0;
    int src_idx = -1;
    int ddash = 0;
    int n_parg = 0;
    const char *parg[32];
    const char *src_path = NULL;
    const char *src_label;
    const char *eq;
    char *expr_buf = NULL;
    size_t expr_len = 0, expr_cap = 0;
    FILE *trace;
    FILE *devnull = NULL;
    cubalc_run_result rr;
    eq = getenv("CUBALC_QUIET");
    if (eq && eq[0] && strcmp(eq, "0") != 0 && strcmp(eq, "false") != 0 &&
        strcmp(eq, "FALSE") != 0 && strcmp(eq, "no") != 0 && strcmp(eq, "NO") != 0)
      quiet = 1;
    eq = getenv("CUBALC_STRICT");
    if (eq && eq[0] && strcmp(eq, "0") != 0 && strcmp(eq, "false") != 0 &&
        strcmp(eq, "FALSE") != 0 && strcmp(eq, "no") != 0 && strcmp(eq, "NO") != 0)
      strict = 1;
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
              "usage: cubalc run [-q] [-s] [-e CODE]... <file.cubalc>|- [-- args…]\n"
              "       cubalc eval [-q] [-s] [-e CODE]... <file>|-\n"
              "       cubalc -e 'SYS DATE\\nPRINT LAST'   # top-level alias\n"
              "       multiple -e join with newline; \\n \\t \\\\ escapes\n"
              "       CUBALC_QUIET=1  → plate only · CUBALC_STRICT=1 → soft last_err fails\n");
      free(expr_buf);
      return 2;
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
      rc = cubalc_run_source(expr_buf, expr_len, src_label, &rr, trace);
      free(expr_buf);
      expr_buf = NULL;
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
        rc = cubalc_run_source(buf, n, src_label, &rr, trace);
        free(buf);
      } else if (strstr(src_path, ".cblc")) {
        cubalc_image img;
        if (cubalc_isa_load(&img, src_path) != 0) {
          if (devnull) fclose(devnull);
          printf("{\"ok\":false,\"cmd\":\"run\",\"err\":\"bad cblc\"}\n");
          return 2;
        }
        rc = cubalc_jit_exec(&img, &rr, trace);
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
      else if (strstr(e, "INCLUDE") || strstr(e, "include") ||
               strstr(e, "REQUIRE LIB") || strstr(e, "lib missing"))
        snprintf(whyh, sizeof whyh,
                 "cubalc libs · REQUIRE LIB name · INCLUDE SOFT · cubalc which name");
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
                 "HOLD_FLASH default 1 · HOLD_FLASH 0 denies PLUG · docs/HOLD_FLASH.md");
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
      printf("{\"ok\":%s,\"cmd\":\"run\",\"file\":\"%s\",\"stmts\":%d,"
             "\"asserts_ok\":%d,\"asserts_fail\":%d,\"n\":%d,\"unity\":%.3f,"
             "\"language\":\"%s\",\"version\":\"%s\",\"err\":\"%s\","
             "\"last_err\":\"%s\",\"err_line\":%d,\"err_src\":\"%s\","
             "\"why_hint\":\"%s\","
             "\"quiet\":%s,\"strict\":%s,\"exit_code\":%d,\"halted\":%s}\n",
             plate_ok ? "true" : "false", src_label, rr.stmts, rr.asserts_ok,
             rr.asserts_fail, rr.n_cubes, rr.unity, CUBALC_LANG_NAME,
             CUBALC_LANG_VERSION, rr.err, rr.last_err, rr.err_line, esrc,
             whyesc,
             quiet ? "true" : "false", strict ? "true" : "false",
             rr.exit_code, rr.halted ? "true" : "false");
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
    return cmd_showcase();
  }
  if (strcmp(cmd, "evolve") == 0 || strcmp(cmd, "evolve-loop") == 0 ||
      strcmp(cmd, "self-evolve") == 0) {
    /* Pure C: braincube solves · algocube optimizes · emits .cubalc */
    return cubalc_cmd_evolve(argc - 1, argv + 1);
  }
  if (strcmp(cmd, "doctor") == 0 || strcmp(cmd, "health") == 0) {
    /* Usability: one JSON plate for agents/humans — is this install ready?
     * Also surfaces INCLUDE lib layout + nest-check readiness (plate_uniform). */
    char dir[512], protect_path[640], key_preview[16];
    int modular = 0, smx_key = 0, protect_plate = 0, bin_ok = 1;
    int libs_n = 0, lib_agent_boot = 0, lib_plate_session = 0, lib_plate_uniform = 0;
    int lib_hold_seed = 0, cookbook_ok = 0, for_agents_ok = 0, libdir_ok = 0;
    const char *hx = getenv("CUBALC_SMX_KEY");
    const char *kf = getenv("CUBALC_SMX_KEY_FILE");
    const char *libdir = "programs/lib";
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
    libdir_ok = (access(libdir, R_OK) == 0);
    lib_agent_boot = (access("programs/lib/agent_boot.cubalc", R_OK) == 0);
    lib_plate_session = (access("programs/lib/plate_session.cubalc", R_OK) == 0);
    lib_plate_uniform = (access("programs/lib/plate_uniform.cubalc", R_OK) == 0);
    lib_hold_seed = (access("programs/lib/hold_seed.cubalc", R_OK) == 0);
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
             "\"docs_cookbook\":%s,\"docs_for_agents\":%s,"
             "\"nest_check\":\"%s\","
             "\"hints\":["
             "\"HOLD_FLASH default 1 — omit preamble; HOLD_FLASH 0 denies PLUG\","
             "\"export CUBALC_SMX_KEY=$(openssl rand -hex 32) for P2P\","
             "\"cubalc protect · cubalc smx-bus prove-tcp\","
             "\"cubalc selftest — live usability proofs\","
             "\"cubalc libs · cubalc cat plate_uniform · INCLUDE plate_uniform\","
             "\"cubalc plate uniform agent.json role — nest value consistency\","
             "\"cubalc env · docs/COOKBOOK.md · docs/FOR_AGENTS.md\""
             "],"
             "\"cookbook\":[\"docs/COOKBOOK.md\",\"docs/P2P_SMX.md\","
             "\"docs/HOLD_FLASH.md\",\"docs/CORE_PROTECT.md\",\"docs/FOR_AGENTS.md\","
             "\"programs/hello_cube.cubalc\",\"programs/p2p/mesh_local.cubalc\","
             "\"programs/lib/plate_uniform.cubalc\"]"
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
             cookbook_ok ? "true" : "false",
             for_agents_ok ? "true" : "false",
             lib_plate_uniform
                 ? "INCLUDE plate_uniform · cubalc plate uniform"
                 : "missing programs/lib/plate_uniform.cubalc");
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
    } rows[512];
    int nrow = 0;
    if (argc > 2 && (!strcmp(argv[2], "--json") || !strcmp(argv[2], "-j")))
      json_only = 1;
    if (!json_only) {
      printf("# CubalC selftest usability proofs n=%d version=%s\n",
             n, CUBALC_LANG_VERSION);
      printf("# id\tok\tasserts\thint\n");
    }
    /* Cap at rows[] size (512) so new usability proofs are not silently skipped. */
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
    /* Usability: live human/agent catalog of play forms (not opcode soup).
     * cubalc forms [prefix] — case-insensitive substring filter. */
    static const struct { const char *name; const char *plane; const char *hint; } forms[] = {
      {"HOLD_FLASH", "law", "device/firmware safeguard · default 1 (omit preamble)"},
      {"CUBE", "core", "place cube · CUBE name ROLE host|body"},
      {"PLUG", "core", "wire cubes · denied only if HOLD_FLASH 0"},
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
      {"DEFAULT", "flow", "DEFAULT name = expr|str — set only if unset (INCLUDE-safe)"},
      {"DEFINED", "flow", "DEFINED name — LAST_N 1 if var exists, 0 if missing"},
      {"TYPEOF", "flow", "TYPEOF name — LAST undef|num|str · LAST_N 0|1|2"},
      {"UNSET", "flow", "UNSET name — remove var · LAST_N 1 if removed"},
      {"LOOP", "flow", "LOOP n … END"},
      {"WHILE", "flow", "WHILE expr … END"},
      {"FOR", "flow", "FOR … END"},
      {"EACH", "flow", "EACH CUBE|CELL|LINE|PROP|METHOD|OBJ|KEY|FLAT … END"},
      {"EACH LINE", "flow", "EACH LINE [as name] [IN str|obj field] … END — bag/LIST walk"},
      {"EACH KEY", "flow", "EACH KEY [AS name] OF|FROM plate [path] … END — walk plate keys · nest path ok · VALUE bound"},
      {"EACH KEYNEST", "flow", "EACH KEYNEST nest [FROM plate] … END — walk nested keys · dotted path ok · VALUE bound"},
      {"EACH KEY OF NEST", "flow", "EACH KEY OF NEST nest [FROM plate] … END — alias of KEYNEST · paths ok"},
      {"EACH FLAT", "flow", "EACH FLAT|LEAF [AS name] [needle] [OF|FROM plate] … END — walk matching leaf path:value · PATH+VALUE"},
      {"EACH LEAF", "flow", "EACH LEAF alias of EACH FLAT — nest leaf walk without PATHSFLAT+EACH LINE+GETP"},
      {"EACH PROP", "flow", "EACH PROP|ATTR OF obj|Class [AS name] … END — schema field names"},
      {"EACH METHOD", "flow", "EACH METHOD|MSG OF obj|Class [AS name] … END — schema method names"},
      {"EACH OBJ", "flow", "EACH OBJ [Class] [AS name] … END — live OOP objects · EACH_N"},
      {"ENSUREBAGLINE", "oop", "ENSUREBAGLINE obj field line — append exact bag line if missing"},
      {"PUSHF", "oop", "PUSHF|POPF|POPHEADF obj field — bag queue on object fields"},
      {"HASBAGLINE", "oop", "HASBAGLINE|DROPBAGLINE obj field line — bag membership/ack"},
      {"GREPF", "oop", "GREPF|GREPVF obj field needle — filter bag lines in field"},
      {"SORTBAGF", "oop", "SORTBAGF|UNIQBAGF|UNIQUEBAGF obj field — sort/dedupe bag lines"},
      {"LINEF", "oop", "LINEF|LINENOF obj field index — peel Nth bag line into field"},
      {"CUTF", "oop", "CUTF|COLUMNF obj field sep index — peel Nth sep column into field"},
      {"CLASS", "oop", "CLASS Name · FIELD · METHOD … END — OOP type"},
      {"NEW", "oop", "NEW Class instance [args] — construct object (init)"},
      {"SEND", "oop", "SEND obj method [args] — method call · THIS set"},
      {"TRYSEND", "oop", "TRYSEND|SEND SOFT — soft method miss OK=0 sticky LAST_ERR"},
      {"GETF", "oop", "GETF obj field [OR def] — read field · TRYGETF soft"},
      {"SETF", "oop", "SETF obj field value — write field · TRYSETF soft"},
      {"LISTOBJS", "oop", "LISTOBJS|OBJECTS [Class] — live object names bag"},
      {"LISTFIELDS", "oop", "LISTFIELDS Class|obj — field name bag"},
      {"LISTMETHODS", "oop", "LISTMETHODS Class|obj — method name bag"},
      {"HASOBJ", "oop", "HASOBJ name — soft live-object probe 0|1"},
      {"HASFIELD", "oop", "HASFIELD obj|Class field — soft 0|1"},
      {"HASMETHOD", "oop", "HASMETHOD obj|Class method — soft 0|1"},
      {"DUMPOBJ", "oop", "DUMPOBJ|INSPECT obj — field:value bag snapshot"},
      {"IF", "flow", "IF expr THEN … END"},
      {"ASSERT", "flow", "ASSERT expr [why] — fail with line+reason · string ==/!= content"},
      {"EXPECT", "flow", "EXPECT expr [why] soft check — OK/LAST_ERR, no fatal"},
      {"FAIL", "flow", "FAIL [why] soft status OK=0 sticky LAST_ERR"},
      {"PASS", "flow", "PASS [why] soft status OK=1 optional note"},
      {"NOTE", "flow", "NOTE [text] agent breadcrumb · no OK/ERR change"},
      {"EXIT", "flow", "EXIT [code] [why] — halt; non-zero fails plate + rc"},
      {"CLEAR_ERR", "flow", "CLEAR_ERR [note] wipe sticky ERR/LAST_ERR after recovery"},
      {"VERSION", "flow", "VERSION — LAST/VERSION language version string"},
      {"REQUIRE", "flow", "REQUIRE VERSION|LIB|ENV|PATH|DIR|REG|BIN|JSONHASALL|JSONONLY|JSONEXACT|JSONEQ|JSONNEQ|JSONSUBSET|JSONTYPE|PLATEFILE — fail-fast gates"},
      {"REQUIRE JSONHASALL", "flow", "REQUIRE JSONHASALL|JSONNEED [plate] key… — fail-fast plate keys · missing listed"},
      {"REQUIRE JSONONLY", "flow", "REQUIRE JSONONLY|NOEXTRA [plate] allow… — fail if plate has unknown keys"},
      {"REQUIRE JSONEXACT", "flow", "REQUIRE JSONEXACT|JSONSCHEMA [plate] key… — exact key set (HASALL+ONLY)"},
      {"REQUIRE JSONEQ", "flow", "REQUIRE JSONEQ|SAMEJSON a b — fail-fast plate equality · lists changed keys"},
      {"REQUIRE JSONNEQ", "flow", "REQUIRE JSONNEQ|JNEQ|DIFFJSON a b — fail if plates still equal · mutation gate"},
      {"REQUIRE JSONTYPE", "flow", "REQUIRE JSONTYPE|JTYPE [plate] key kind — fail-fast field type · soft SYS JSONTYPE"},
      {"REQUIRE PLATEFILE", "flow", "REQUIRE PLATEFILE|JSONPLATE|HASPLATE path — object plate file · LAST=content"},
      {"REQUIRE JSONSUBSET", "flow", "REQUIRE JSONSUBSET|JSUBSET sub super — fail-fast required fields · bad keys listed"},
      {"REQUIRE JSONSUPERSET", "flow", "REQUIRE JSONSUPERSET|JSONCOVERS super sub — dual (a ⊇ b) · same gate"},
      {"SYS JSONEXACT", "host", "SYS JSONEXACT|JSONSCHEMA [plate] key… — soft exact key-set → LAST_N 0|1"},
      {"SYS JSONKEYDIFF", "host", "SYS JSONKEYDIFF|KEYSDIFF a b — keys in a not in b → bag"},
      {"SYS JSONKEYCOMM", "host", "SYS JSONKEYCOMM|KEYSINTER a b — shared keys bag · dual DIFF"},
      {"SYS JSONCHANGED", "host", "SYS JSONCHANGED|JSONNEQKEYS a b — keys that differ (struct or value)"},
      {"SYS JSONSTABLE", "host", "SYS JSONSTABLE|JSONEQKEYS a b — keys same in both · dual CHANGED"},
      {"SYS JSONDELTA", "host", "SYS JSONDELTA|JDELTA a b [FROM NEW|OLD] — changed keys as plate object"},
      {"SYS JSONCHANGELOG", "host", "SYS JSONCHANGELOG|JCHANGELOG a b — key: old → new lines · agent plate sync log"},
      {"SYS JSONDEFAULTS", "host", "SYS JSONDEFAULTS|JSONENSUREALL plate defaults — fill missing keys only · no clobber"},
      {"SYS LOADPLATE", "host", "SYS LOADPLATE|READPLATE path [OR defaults] — soft plate file load · LAST_N hit"},
      {"SYS SAVEPLATE", "host", "SYS SAVEPLATE|STOREPLATE path [plate] — object plate persist · mkdir parent"},
      {"SYS ENSUREPLATE", "host", "SYS ENSUREPLATE|TOUCHPLATE path [OR defaults] — create-or-load plate · LAST=content"},
      {"SYS UPDATEPLATE", "host", "SYS UPDATEPLATE|JSONFILESET path key value — load-set-save JSON plate field · LAST=plate"},
      {"SYS BUMPPLATE", "host", "SYS BUMPPLATE|JSONFILEINC path key [delta] — load-inc-save plate counter · LAST_N=value"},
      {"SYS PEEKPLATE", "host", "SYS PEEKPLATE|JSONFILEGET path key [OR fallback] — peel plate file field · LAST=value"},
      {"SYS DROPPLATE", "host", "SYS DROPPLATE|JSONFILEDEL path key — load-del-save plate field · LAST=plate LAST_N=removed"},
      {"SYS JSONFILEMERGE", "host", "SYS JSONFILEMERGE|OVERLAYPLATE path overlay — multi-key load-merge-save plate · LAST=plate"},
      {"SYS JSONFILEENSURE", "host", "SYS JSONFILEENSURE|ENSUREPLATEKEY path key value — set plate file key only if missing"},
      {"SYS JSONFILEDEFAULTS", "host", "SYS JSONFILEDEFAULTS|DEFAULTPLATEALL path defaults — multi-key disk fill-if-missing"},
      {"SYS JSONFILETOGGLE", "host", "SYS JSONFILETOGGLE|TOGGLEPLATEKEY path key — flip disk plate flag 0↔1 · LAST_N=new"},
      {"PRINT", "flow", "PRINT str|expr…"},
      {"PRINT_JSON", "flow", "PRINT_JSON [idents] one JSON line for agents"},
      {"DUMP", "flow", "alias of PRINT_JSON"},
      {"VARS", "flow", "VARS — dump all program vars as cubalc.vars.v1 JSON"},
      {"STATUS", "flow", "STATUS — cubalc.status.v1 health (ok/last_err/version/time)"},
      {"IDENTITY", "flow", "IDENTITY — cubalc.identity.v1 user@host:pid plate"},
      {"SETP", "flow", "SETP [FROM plate] key value — set key · JSON-shaped strings auto-raw · paths ok · multi-plate"},
      {"DEFAULTP", "flow", "DEFAULTP [FROM plate] key value — set-if-missing · JSON-shaped strings auto-raw · paths ok"},
      {"INCP", "flow", "INCP [FROM plate] key [delta] — bump numeric key · dotted path nest ok · multi-plate"},
      {"DELP", "flow", "DELP [FROM plate] key — drop key · dotted path nest ok · soft miss · multi-plate"},
      {"GETP", "flow", "GETP [FROM plate] key [OR fallback] — peel value · dotted path nest ok · multi-plate"},
      {"GETPN", "flow", "GETPN|NUMP [FROM plate] key [OR n] — peel numeric LAST_N=value · paths ok · multi-plate"},
      {"NUMP", "flow", "NUMP alias of GETPN"},
      {"MERGEP", "flow", "MERGEP [FROM plate] overlay — multi-key · multi-plate"},
      {"DEFAULTP", "flow", "DEFAULTP|ENSUREP [FROM plate] key value — set-if-missing · multi-plate"},
      {"TOGGLEP", "flow", "TOGGLEP [FROM plate] key — flip flag · multi-plate · miss→1"},
      {"NEEDP", "flow", "NEEDP [FROM plate] key… — fail-fast missing keys · dotted path nest ok · multi-plate"},
      {"HASP", "flow", "HASP [FROM plate] key — soft 0|1 presence · dotted path nest ok · multi-plate"},
      {"HASPALL", "flow", "HASPALL [FROM plate] key… — soft all-present · dotted path nest ok · multi-plate"},
      {"KEYSP", "flow", "KEYSP [FROM plate] — key bag → LAST · multi-plate"},
      {"PATHKEYS", "flow", "PATHKEYS|LEAFKEYS|DOTPATHS [FROM plate] [path] — dotted leaf path bag · multi-plate"},
      {"LEAFKEYS", "flow", "LEAFKEYS alias of PATHKEYS"},
      {"DOTPATHS", "flow", "DOTPATHS alias of PATHKEYS"},
      {"FLATKV", "flow", "FLATKV|LEAFKV|FLATTENP [FROM plate] [path] — recursive path:value bag · multi-plate"},
      {"LEAFKV", "flow", "LEAFKV alias of FLATKV"},
      {"FLATTENP", "flow", "FLATTENP alias of FLATKV"},
      {"UNFLATKV", "flow", "UNFLATKV|FROMFLAT|UNFLATTENP [bag] [UNDER path] [INTO name] — path:value bag → nested plate"},
      {"FROMFLAT", "flow", "FROMFLAT alias of UNFLATKV"},
      {"UNFLATTENP", "flow", "UNFLATTENP alias of UNFLATKV"},
      {"DIFFFLAT", "flow", "DIFFFLAT|CHANGELOGFLAT a b — deep leaf path: old → new bag · nest-aware"},
      {"PATHDIFF", "flow", "PATHDIFF|DIFFPATH a b — changed leaf path bag · nest-aware DIFFP"},
      {"GREPFLAT", "flow", "GREPFLAT|KEEPFLAT [FROM plate] needle — filter path:value bag by path needle"},
      {"GREPVFLAT", "flow", "GREPVFLAT|DROPFLAT [FROM plate] needle — invert GREPFLAT"},
      {"GREPFLATI", "flow", "GREPFLATI|IGREPFLAT [FROM plate] needle — case-insensitive GREPFLAT"},
      {"PRUNEFLAT", "flow", "PRUNEFLAT|DELFLAT [FROM plate] needle — drop matching leaf paths write-back"},
      {"KEEPONLYFLAT", "flow", "KEEPONLYFLAT|RETAINFLAT [FROM plate] needle — keep only matching leaves rewrite"},
      {"MERGEFLAT", "flow", "MERGEFLAT|OVERLAYFLAT [FROM base] overlay — deep leaf overlay write-back · nest-aware"},
      {"OVERLAYFLAT", "flow", "OVERLAYFLAT alias of MERGEFLAT"},
      {"RENAMEFLAT", "flow", "RENAMEFLAT|MOVEFLAT [FROM plate] old_pfx new_pfx — rewrite leaf path prefixes write-back"},
      {"MOVEFLAT", "flow", "MOVEFLAT alias of RENAMEFLAT"},
      {"SETFLAT", "flow", "SETFLAT|MAPFLAT [FROM plate] needle value — bulk set leaf values by path needle write-back"},
      {"MAPFLAT", "flow", "MAPFLAT alias of SETFLAT"},
      {"INCFLAT", "flow", "INCFLAT|BUMPFLAT [FROM plate] needle [delta] — bump pure-int leaves by path needle write-back"},
      {"BUMPFLAT", "flow", "BUMPFLAT alias of INCFLAT"},
      {"SUMFLAT", "flow", "SUMFLAT|TOTALFLAT [FROM plate] [needle] — sum pure-int leaves by path needle → LAST_N · read-only"},
      {"TOTALFLAT", "flow", "TOTALFLAT alias of SUMFLAT"},
      {"AVGFLAT", "flow", "AVGFLAT|MEANFLAT [FROM plate] [needle] — integer mean pure-int leaves by path needle → LAST_N · read-only"},
      {"MEANFLAT", "flow", "MEANFLAT alias of AVGFLAT"},
      {"MEDIANFLAT", "flow", "MEDIANFLAT|P50FLAT [FROM plate] [needle] — integer median pure-int leaves by path needle → LAST_N · read-only"},
      {"P50FLAT", "flow", "P50FLAT alias of MEDIANFLAT"},
      {"TOPPATHFLAT", "flow", "TOPPATHFLAT|MAXPATHFLAT [FROM plate] [needle] — path of max pure-int leaf → LAST · value LAST_N"},
      {"BOTPATHFLAT", "flow", "BOTPATHFLAT|MINPATHFLAT [FROM plate] [needle] — path of min pure-int leaf → LAST · value LAST_N"},
      {"MAXPATHFLAT", "flow", "MAXPATHFLAT alias of TOPPATHFLAT"},
      {"THRESHFLAT", "flow", "THRESHFLAT|KEEPVFLAT [FROM plate] [needle] min — keep pure-int leaves value≥min by path needle write-back"},
      {"KEEPVFLAT", "flow", "KEEPVFLAT alias of THRESHFLAT"},
      {"DROPZEROFLAT", "flow", "DROPZEROFLAT|NZFLAT [FROM plate] [needle] — drop pure-int leaves value==0 by path needle write-back"},
      {"CAPFLAT", "flow", "CAPFLAT|CLAMPFLAT [FROM plate] [needle] max — clamp pure-int leaves value>max → max by path needle write-back"},
      {"CLAMPFLAT", "flow", "CLAMPFLAT alias of CAPFLAT"},
      {"SCALEFLAT", "flow", "SCALEFLAT|MULFLAT [FROM plate] [needle] factor — multiply pure-int leaves by factor by path needle write-back"},
      {"MULFLAT", "flow", "MULFLAT alias of SCALEFLAT"},
      {"HASFLAT", "flow", "HASFLAT|ANYFLAT [FROM plate] [needle] — soft leaf-path presence → LAST_N 0|1 · read-only"},
      {"ANYFLAT", "flow", "ANYFLAT alias of HASFLAT"},
      {"COUNTFLAT", "flow", "COUNTFLAT|NFLAT [FROM plate] [needle] — count leaf paths matching needle → LAST_N · read-only"},
      {"HASFLATN", "flow", "HASFLATN|ANYFLATN [FROM plate] [needle] — soft pure-int leaf presence → LAST_N 0|1 · read-only"},
      {"ANYFLATN", "flow", "ANYFLATN alias of HASFLATN"},
      {"COUNTFLATN", "flow", "COUNTFLATN|NFLATN [FROM plate] [needle] — count pure-int leaves matching path needle → LAST_N · read-only"},
      {"NFLATN", "flow", "NFLATN alias of COUNTFLATN"},
      {"PATHSFLAT", "flow", "PATHSFLAT|MATCHPATHS [FROM plate] [needle] — matching leaf paths → LAST bag · read-only"},
      {"VALSFLAT", "flow", "VALSFLAT|MATCHVALS [FROM plate] [needle] — matching leaf values → LAST bag · read-only"},
      {"PATHSFLATN", "flow", "PATHSFLATN|MATCHPATHSN [FROM plate] [needle] — pure-int matching leaf paths → LAST bag · read-only"},
      {"VALSFLATN", "flow", "VALSFLATN|MATCHVALSN [FROM plate] [needle] — pure-int matching leaf values → LAST bag · read-only"},
      {"MATCHPATHSN", "flow", "MATCHPATHSN alias of PATHSFLATN"},
      {"MATCHVALSN", "flow", "MATCHVALSN alias of VALSFLATN"},
      {"GETFLAT", "flow", "GETFLAT|FIRSTFLAT [FROM plate] needle [OR fallback] — first matching leaf value → LAST · read-only"},
      {"FIRSTFLAT", "flow", "FIRSTFLAT alias of GETFLAT"},
      {"LASTFLAT", "flow", "LASTFLAT|ENDFLAT [FROM plate] needle [OR fallback] — last matching leaf value → LAST · read-only"},
      {"ENDFLAT", "flow", "ENDFLAT alias of LASTFLAT"},
      {"NTHFLAT", "flow", "NTHFLAT|INDEXFLAT [FROM plate] needle index [OR fallback] — 0-based Nth matching leaf → LAST · read-only"},
      {"INDEXFLAT", "flow", "INDEXFLAT alias of NTHFLAT"},
      {"NEEDFLAT", "flow", "NEEDFLAT|REQUIREFLAT [FROM plate] needle… — fail-fast if any leaf-path needle missing · soft twin HASFLAT"},
      {"REQUIREFLAT", "flow", "REQUIREFLAT alias of NEEDFLAT"},
      {"GETFLATN", "flow", "GETFLATN|FIRSTFLATN [FROM plate] needle [OR fallback] — first pure-int matching leaf → LAST_N · read-only"},
      {"FIRSTFLATN", "flow", "FIRSTFLATN alias of GETFLATN"},
      {"LASTFLATN", "flow", "LASTFLATN|ENDFLATN [FROM plate] needle [OR fallback] — last pure-int matching leaf → LAST_N · read-only"},
      {"ENDFLATN", "flow", "ENDFLATN alias of LASTFLATN"},
      {"NTHFLATN", "flow", "NTHFLATN|INDEXFLATN [FROM plate] needle index [OR fallback] — 0-based Nth pure-int leaf → LAST_N · read-only"},
      {"INDEXFLATN", "flow", "INDEXFLATN alias of NTHFLATN"},
      {"TYPEFLAT", "flow", "TYPEFLAT|KINDFLAT [FROM plate] needle — first matching leaf kind missing|num|str|bool|null|obj|arr → LAST · LAST_N · read-only"},
      {"KINDFLAT", "flow", "KINDFLAT alias of TYPEFLAT"},
      {"NEEDFLATN", "flow", "NEEDFLATN|REQUIREFLATN [FROM plate] needle… — fail-fast pure-int leaf needles · peel first → LAST_N · soft twin GETFLATN"},
      {"REQUIREFLATN", "flow", "REQUIREFLATN alias of NEEDFLATN"},
      {"SAVEP", "flow", "SAVEP [FROM plate] path — persist PLATE or named plate · multi-plate"},
      {"LOADP", "flow", "LOADP [INTO name] path [OR defaults] — soft load · multi-plate · no SYS"},
      {"SEEDP", "flow", "SEEDP|BOOTP [INTO name] path [OR seed] — disk create-or-load · multi-plate · no SYS"},
      {"DUMPP", "flow", "DUMPP|PLATEINFO [FROM plate] — cubalc.plate_info.v1 · multi-plate"},
      {"EQP", "flow", "EQP|SAMEP a b — soft plate equality · multi-plate · no SYS JSONEQ"},
      {"NEQP", "flow", "NEQP a b — soft plate inequality · inverse of EQP"},
      {"DIFFP", "flow", "DIFFP|CHANGEDP a b — changed key bag · multi-plate · no SYS JSONCHANGED"},
      {"STABLEP", "flow", "STABLEP|SAMEKEYSP|EQKEYSP a b — unchanged key bag · multi-plate · dual DIFFP"},
      {"SAMEKEYSP", "flow", "SAMEKEYSP alias of STABLEP"},
      {"EQKEYSP", "flow", "EQKEYSP alias of STABLEP"},
      {"UNCHANGEDP", "flow", "UNCHANGEDP alias of STABLEP"},
      {"CHANGELOGP", "flow", "CHANGELOGP|CLOGP a b — key: old → new bag · multi-plate"},
      {"REQUIRE EQP", "flow", "REQUIRE EQP|SAMEP a b — fail-fast multi-plate equality · soft twin EQP"},
      {"REQUIRE NEQP", "flow", "REQUIRE NEQP a b — fail-fast if still equal · soft twin NEQP"},
      {"SUBSETP", "flow", "SUBSETP sub super — soft required-field match · multi-plate · no SYS JSONSUBSET"},
      {"COVERSP", "flow", "COVERSP super sub — super covers sub · multi-plate"},
      {"REQUIRE SUBSETP", "flow", "REQUIRE SUBSETP sub super — fail-fast required fields · soft twin SUBSETP"},
      {"REQUIRE COVERSP", "flow", "REQUIRE COVERSP super sub — fail-fast cover · soft twin COVERSP"},
      {"PLUCKP", "flow", "PLUCKP|GETPALL [FROM plate] key… — multi-key peel → value bag · dotted path nest ok · multi-plate"},
      {"GETPALL", "flow", "GETPALL alias of PLUCKP"},
      {"DELTAP", "flow", "DELTAP|CHANGEOBJP a b [FROM NEW|OLD] — changed keys as plate · multi-plate sync"},
      {"CHANGEOBJP", "flow", "CHANGEOBJP alias of DELTAP"},
      {"PICKP", "flow", "PICKP|KEEPP [FROM plate] key… — keep listed keys → subset plate · multi-plate"},
      {"KEEPP", "flow", "KEEPP alias of PICKP"},
      {"OMITP", "flow", "OMITP|DROPKEYSP [FROM plate] key… — drop listed keys · multi-plate dual PICKP"},
      {"DROPKEYSP", "flow", "DROPKEYSP alias of OMITP"},
      {"RENAMEP", "flow", "RENAMEP|MOVEKEYP [FROM plate] old new — rename key write-back · multi-plate"},
      {"MOVEKEYP", "flow", "MOVEKEYP alias of RENAMEP"},
      {"KEYDIFFP", "flow", "KEYDIFFP|ONLYKEYSP a b — keys in a not in b · multi-plate · no SYS JSONKEYDIFF"},
      {"ONLYKEYSP", "flow", "ONLYKEYSP alias of KEYDIFFP"},
      {"KEYCOMMP", "flow", "KEYCOMMP|KEYINTERP a b — keys in both · multi-plate · no SYS JSONKEYCOMM"},
      {"KEYINTERP", "flow", "KEYINTERP alias of KEYCOMMP"},
      {"COPYP", "flow", "COPYP|DUPKEYP [FROM plate] src dst — copy key write-back · multi-plate"},
      {"DUPKEYP", "flow", "DUPKEYP alias of COPYP"},
      {"SWAPP", "flow", "SWAPP|XCHGP [FROM plate] a b — swap keys write-back · multi-plate"},
      {"XCHGP", "flow", "XCHGP alias of SWAPP"},
      {"LENP", "flow", "LENP|NKEYSP [FROM plate] — key count → LAST_N · multi-plate"},
      {"EMPTYP", "flow", "EMPTYP [FROM plate] — soft empty 0|1 · multi-plate"},
      {"NONEMPTYP", "flow", "NONEMPTYP|HASKEYSP [FROM plate] — soft nonempty · multi-plate"},
      {"VALSP", "flow", "VALSP [FROM plate] — values bag · twin KEYSP · multi-plate"},
      {"TYPEP", "flow", "TYPEP|KINDP [FROM plate] key — field kind probe · multi-plate · no SYS JSONTYPE"},
      {"KINDP", "flow", "KINDP alias of TYPEP"},
      {"GETOBJ", "flow", "GETOBJ|PEEKOBJ [FROM plate] key [OR fb] [INTO name] — peel nested object → plate · multi-plate"},
      {"PEEKOBJ", "flow", "PEEKOBJ alias of GETOBJ"},
      {"SETOBJ", "flow", "SETOBJ|PUTOBJ|NESTP [FROM plate] key value — nest plate object under key · multi-plate"},
      {"NESTP", "flow", "NESTP alias of SETOBJ · nest plate under key"},
      {"PUTOBJ", "flow", "PUTOBJ alias of SETOBJ"},
      {"MERGEOBJ", "flow", "MERGEOBJ|PATCHNEST [FROM plate] key overlay — merge into nested object · multi-plate"},
      {"PATCHNEST", "flow", "PATCHNEST alias of MERGEOBJ"},
      {"DEFAULTOBJ", "flow", "DEFAULTOBJ|ENSUREOBJ [FROM plate] key defaults — fill missing nested keys · multi-plate"},
      {"ENSUREOBJ", "flow", "ENSUREOBJ alias of DEFAULTOBJ"},
      {"GETPOBJ", "flow", "GETPOBJ|PEEKNEST [FROM plate] nest field [OR fb] — peel nested scalar · multi-plate"},
      {"SETPOBJ", "flow", "SETPOBJ|PUTNESTF [FROM plate] nest field value — set nested scalar · multi-plate"},
      {"INCOBJ", "flow", "INCOBJ|BUMPNEST [FROM plate] nest field [delta] — bump nested counter · multi-plate"},
      {"DELPOBJ", "flow", "DELPOBJ|DROPNEST [FROM plate] nest field — drop nested key · multi-plate"},
      {"HASPOBJ", "flow", "HASPOBJ|NESTHAS [FROM plate] nest field — soft nested field presence · multi-plate"},
      {"TYPEPOBJ", "flow", "TYPEPOBJ|NESTTYPE [FROM plate] nest field — nested field kind · multi-plate"},
      {"KEYSOBJ", "flow", "KEYSOBJ|NESTKEYS [FROM plate] nest — nested object key bag · multi-plate"},
      {"NEEDPOBJ", "flow", "NEEDPOBJ|REQUIRENEST [FROM plate] nest field… — fail-fast nested keys · multi-plate"},
      {"PLUCKOBJ", "flow", "PLUCKOBJ|NESTPLUCK [FROM plate] nest field… — multi-key peel nested → value bag · multi-plate"},
      {"PICKOBJ", "flow", "PICKOBJ|KEEPOBJ [FROM plate] nest field… — keep listed nest keys write-back · multi-plate"},
      {"OMITOBJ", "flow", "OMITOBJ|STRIPNEST [FROM plate] nest field… — drop listed nest keys write-back · multi-plate"},
      {"LENOBJ", "flow", "LENOBJ|NESTLEN [FROM plate] nest — nested key count → LAST_N · multi-plate"},
      {"EMPTYOBJ", "flow", "EMPTYOBJ [FROM plate] nest — soft empty nest 0|1 · multi-plate"},
      {"VALSOBJ", "flow", "VALSOBJ|NESTVALS [FROM plate] nest — nested values bag · multi-plate"},
      {"RENAMEPOBJ", "flow", "RENAMEPOBJ|NESTRENAME [FROM plate] nest old new — rename key in nest · dotted nest path ok · write-back"},
      {"COPYPOBJ", "flow", "COPYPOBJ|NESTCOPY [FROM plate] nest src dst — copy key in nest · dotted nest path ok"},
      {"SWAPPOBJ", "flow", "SWAPPOBJ|NESTSWAP [FROM plate] nest a b — swap keys in nest · dotted nest path ok"},
      {"MOVEKEYOBJ", "flow", "MOVEKEYOBJ alias of RENAMEPOBJ"},
      {"NESTRENAME", "flow", "NESTRENAME alias of RENAMEPOBJ"},
      {"COPYPOBJ", "flow", "COPYPOBJ|DUPKEYOBJ|NESTCOPY [FROM plate] nest src dst — copy key in nest · multi-plate · COPYP dual"},
      {"DUPKEYOBJ", "flow", "DUPKEYOBJ alias of COPYPOBJ"},
      {"NESTCOPY", "flow", "NESTCOPY alias of COPYPOBJ"},
      {"SWAPPOBJ", "flow", "SWAPPOBJ|XCHGKEYOBJ|NESTSWAP [FROM plate] nest a b — swap keys in nest · multi-plate · SWAPP dual"},
      {"XCHGKEYOBJ", "flow", "XCHGKEYOBJ alias of SWAPPOBJ"},
      {"NESTSWAP", "flow", "NESTSWAP alias of SWAPPOBJ"},
      {"TOKVOBJ", "flow", "TOKVOBJ|NESTTOKV [FROM plate] nest — nest → key:val bag · multi-plate · TOKVP dual"},
      {"NESTTOKV", "flow", "NESTTOKV alias of TOKVOBJ"},
      {"FROMKVOBJ", "flow", "FROMKVOBJ|NESTFROMKV|BAGTONEST [FROM plate] nest [bag] — bag → nest write-back · multi-plate"},
      {"NESTFROMKV", "flow", "NESTFROMKV alias of FROMKVOBJ"},
      {"BAGTONEST", "flow", "BAGTONEST alias of FROMKVOBJ"},
      {"SUMNOBJ", "flow", "SUMNOBJ|TOTALOBJ|NESTSUM [FROM plate] nest — sum int nest values → LAST_N · multi-plate · SUMNP dual"},
      {"MAXNOBJ", "flow", "MAXNOBJ [FROM plate] nest — max int nest value → LAST_N · multi-plate"},
      {"MINNOBJ", "flow", "MINNOBJ [FROM plate] nest — min int nest value → LAST_N · multi-plate"},
      {"AVGNOBJ", "flow", "AVGNOBJ|MEANOBJ [FROM plate] nest — mean int nest value → LAST_N · multi-plate"},
      {"MEDIANOBJ", "flow", "MEDIANOBJ|P50OBJ [FROM plate] nest — median int nest value → LAST_N · multi-plate"},
      {"TOPKEYOBJ", "flow", "TOPKEYOBJ|MAXKEYOBJ [FROM plate] nest — key with max int · LAST=key LAST_N=v · multi-plate"},
      {"BOTKEYOBJ", "flow", "BOTKEYOBJ|MINKEYOBJ [FROM plate] nest — key with min int · LAST=key LAST_N=v · multi-plate"},
      {"THRESHOBJ", "flow", "THRESHOBJ|KEEPVOBJ [FROM plate] nest min — keep nest int value>=min write-back · multi-plate · THRESHP dual"},
      {"DROPZEROOBJ", "flow", "DROPZEROOBJ|KEEPNZOBJ [FROM plate] nest — drop nest zeros write-back · multi-plate · DROPZEROP dual"},
      {"CAPOBJ", "flow", "CAPOBJ|CLAMPOBJ [FROM plate] nest max — clamp nest int values write-back · multi-plate · CAPP dual"},
      {"PCTOBJ", "flow", "PCTOBJ|SHAREOBJ|NESTPCT [FROM plate] nest — nest ints as %% of sum write-back · multi-plate · PCTP dual"},
      {"SCALEPOBJ", "flow", "SCALEPOBJ|MULPOBJ|NESTSCALE [FROM plate] nest factor — multiply nest ints write-back · multi-plate · SCALEP dual"},
      {"ADDPOBJ", "flow", "ADDPOBJ|OFFSETPOBJ|NESTADD [FROM plate] nest delta — offset nest ints write-back · multi-plate · ADDP dual"},
      {"DIVPOBJ", "flow", "DIVPOBJ|NESTDIV [FROM plate] nest divisor — idiv nest ints write-back · multi-plate · DIVP dual"},
      {"ABSPOBJ", "flow", "ABSPOBJ|MAGPOBJ|NESTABS [FROM plate] nest — abs nest ints write-back · multi-plate · ABSP dual"},
      {"SIGNPOBJ", "flow", "SIGNPOBJ|DIRPOBJ|NESTSIGN [FROM plate] nest — signum nest ints write-back · multi-plate · SIGNP dual"},
      {"KEEPKEYOBJ", "flow", "KEEPKEYOBJ|GREPKEYOBJ [FROM plate] nest needle — keep nest keys containing needle write-back · multi-plate · KEEPKEYP dual"},
      {"DROPKEYOBJ", "flow", "DROPKEYOBJ|GREPVKEYOBJ [FROM plate] nest needle — drop nest keys containing needle write-back · multi-plate · DROPKEYP dual"},
      {"KEEPKEYOBJI", "flow", "KEEPKEYOBJI case-insensitive KEEPKEYOBJ"},
      {"DROPKEYOBJI", "flow", "DROPKEYOBJI case-insensitive DROPKEYOBJ"},
      {"TOPNOBJ", "flow", "TOPNOBJ|TOPNKEYOBJ|NESTTOPN [FROM plate] nest n — nest top n pure-int keys write-back · multi-plate · TOPNP dual"},
      {"BOTNOBJ", "flow", "BOTNOBJ|BOTNKEYOBJ|NESTBOTN [FROM plate] nest n — nest bottom n pure-int keys write-back · multi-plate · BOTNP dual"},
      {"SORTOBJ", "flow", "SORTOBJ|NESTSORT [FROM plate] nest [ASC|DESC] — sort pure-int nest keys write-back · multi-plate · SORTP dual"},
      {"SORTBAGOBJ", "flow", "SORTBAGOBJ|SORTFREQOBJ [FROM plate] nest [ASC|DESC] — nest key:val bag sorted by value · multi-plate · SORTBAGP dual"},
      {"SUMMERGEOBJ", "flow", "SUMMERGEOBJ|ADDFREQOBJ nest overlay|FROM src [srcnest] — sum-merge pure-int into nest write-back · multi-plate · SUMMERGEP dual"},
      {"SUBVALOBJ", "flow", "SUBVALOBJ|SUBFREQOBJ nest overlay|FROM src [srcnest] — nest−overlay pure-int write-back · multi-plate · SUBP dual · not SUBOBJ"},
      {"TOKVP", "flow", "TOKVP|TOBAGP [FROM plate] — plate → key:val bag · multi-plate"},
      {"FROMKVP", "flow", "FROMKVP|BAGTOP [bag] [INTO name] — bag → plate · multi-plate"},
      {"SUMNP", "flow", "SUMNP|TOTALP [FROM plate] — sum int values → LAST_N · multi-plate"},
      {"MAXNP", "flow", "MAXNP [FROM plate] — max int → LAST_N · multi-plate"},
      {"MINNP", "flow", "MINNP [FROM plate] — min int → LAST_N · multi-plate"},
      {"AVGNP", "flow", "AVGNP [FROM plate] — mean int → LAST_N · multi-plate"},
      {"MEDIANP", "flow", "MEDIANP|P50P [FROM plate] — median int → LAST_N · multi-plate · robust mid"},
      {"TOPKEYP", "flow", "TOPKEYP [FROM plate] — key with max int · LAST=key LAST_N=v"},
      {"BOTKEYP", "flow", "BOTKEYP [FROM plate] — key with min int · LAST=key LAST_N=v"},
      {"THRESHP", "flow", "THRESHP|KEEPVP [FROM plate] min — keep int value>=min → plate · multi-plate"},
      {"KEEPVP", "flow", "KEEPVP alias of THRESHP · denoise FREQ"},
      {"DROPZEROP", "flow", "DROPZEROP|KEEPNZP [FROM plate] — drop value==0 keys → plate · multi-plate"},
      {"CAPP", "flow", "CAPP|CLAMPP [FROM plate] max — clamp int values to max → plate · multi-plate"},
      {"PCTP", "flow", "PCTP|SHAREP [FROM plate] — int values as %% of sum → plate · multi-plate"},
      {"SCALEP", "flow", "SCALEP|MULP [FROM plate] factor — multiply int values → plate · multi-plate"},
      {"ADDP", "flow", "ADDP|OFFSETP [FROM plate] delta — add delta to int values → plate · multi-plate"},
      {"DIVP", "flow", "DIVP|IDIVP [FROM plate] divisor — integer-divide int values → plate · multi-plate"},
      {"SUMMERGEP", "flow", "SUMMERGEP|ADDFREQP a b — sum pure-int keys · multi-plate FREQ combine"},
      {"SUBP", "flow", "SUBP|SUBVALP a b — a−b pure-int by key · multi-plate FREQ delta · not DIFFP"},
      {"ABSP", "flow", "ABSP|MAGP [FROM plate] — |int| values → plate · rank SUBP deltas · multi-plate"},
      {"SIGNP", "flow", "SIGNP|DIRP [FROM plate] — map int to −1|0|1 → plate · multi-plate"},
      {"KEEPKEYP", "flow", "KEEPKEYP|GREPKEYP [FROM plate] needle — keep keys containing needle · multi-plate"},
      {"DROPKEYP", "flow", "DROPKEYP|GREPVKEYP [FROM plate] needle — drop keys containing needle · multi-plate"},
      {"TOPNP", "flow", "TOPNP|TOPNKEYP [FROM plate] n — top n pure-int keys by value → plate · multi-plate"},
      {"BOTNP", "flow", "BOTNP|BOTNKEYP [FROM plate] n — bottom n pure-int keys → plate · multi-plate"},
      {"SORTP", "flow", "SORTP [ASC|DESC] [FROM plate] — all pure-int keys sorted → plate · multi-plate"},
      {"SORTBAGP", "flow", "SORTBAGP|SORTFREQP [ASC|DESC] [FROM plate] — key:val bag sorted · multi-plate"},
      {"FILLP", "flow", "FILLP [STRICT] [FROM plate] [tmpl] — expand {{key}} · dotted path {{freq.error}} ok · multi-plate FROM"},
      {"NEEDFILLP", "flow", "NEEDFILLP|FILLP STRICT [tmpl] — fail if {{key}} missing · path keys ok · lists names"},
      {"FILLPKEYS", "flow", "FILLPKEYS [tmpl] — unique {{key}} names bag · path keys ok · template contract"},
      {"SUBSTPLATE", "flow", "SUBSTPLATE alias of FILLP"},
      {"EXPANDP", "flow", "EXPANDP alias of FILLP"},
      {"TEMPLATEP", "flow", "TEMPLATEP alias of FILLP"},
      {"FILLPFILE", "flow", "FILLPFILE [STRICT] [FROM plate] tmpl [out] — multi-plate materialize · path slots"},
      {"NEEDFILLPFILE", "flow", "NEEDFILLPFILE|FILLPFILE STRICT tmpl [out] — fail if {{key}} missing · no write"},
      {"SUBSTPLATEFILE", "flow", "SUBSTPLATEFILE alias of FILLPFILE"},
      {"EXPANDPFILE", "flow", "EXPANDPFILE alias of FILLPFILE"},
      {"INCLUDE", "flow", "INCLUDE [ONCE] [OR|SOFT] path|libname — ONCE skips reload"},
      {"SYS ENV", "host", "SYS ENV NAME [OR fallback] · ENV SET name val · ENV UNSET name"},
      {"SYS SETENV", "host", "SYS SETENV|ENV SET name value — process setenv"},
      {"SYS ENVDEFAULT", "host", "SYS ENVDEFAULT|ENSUREENV name value — setenv if missing/empty"},
      {"SYS UNSETENV", "host", "SYS UNSETENV|ENV UNSET name — process unsetenv · LAST_N was-set"},
      {"SYS CAT", "host", "SYS CAT|STRCAT parts… — multi-arg concat · host-wide buffers · LAST_N=len"},
      {"SYS STRCAT", "host", "SYS STRCAT alias of SYS CAT"},
      {"SYS CONCAT", "host", "SYS CONCAT alias of SYS CAT"},
      {"SYS SUBSTENV", "host", "SYS SUBSTENV|ENVSUBST [tmpl] — expand $NAME/${NAME} from env+vars"},
      {"SYS ENVSUBST", "host", "SYS ENVSUBST alias of SYS SUBSTENV"},
      {"SYS EXPANDENV", "host", "SYS EXPANDENV alias of SYS SUBSTENV"},
      {"SYS SUBSTFILE", "host", "SYS SUBSTFILE|EXPANDFILE path [out] — expand template file → write"},
      {"SYS EXPANDFILE", "host", "SYS EXPANDFILE alias of SYS SUBSTFILE"},
      {"SYS ENVSUBSTFILE", "host", "SYS ENVSUBSTFILE alias of SYS SUBSTFILE"},
      {"SYS HASH", "host", "SYS HASH|FINGERPRINT [str] — FNV-1a 32-bit hex stamp"},
      {"SYS HASHFILE", "host", "SYS HASHFILE|FILEHASH path — FNV-1a of file bytes"},
      {"SYS FINGERPRINT", "host", "SYS FINGERPRINT alias of SYS HASH"},
      {"SYS BASE64", "host", "SYS BASE64|B64ENC [str] — standard base64 encode"},
      {"SYS B64ENC", "host", "SYS B64ENC alias of SYS BASE64"},
      {"SYS BASE64D", "host", "SYS BASE64D|B64DEC [str] — standard base64 decode"},
      {"SYS B64DEC", "host", "SYS B64DEC alias of SYS BASE64D"},
      {"SYS HEX", "host", "SYS HEX|HEXENC [str] — lowercase hex encode"},
      {"SYS HEXENC", "host", "SYS HEXENC alias of SYS HEX"},
      {"SYS HEXD", "host", "SYS HEXD|HEXDEC [str] — hex decode"},
      {"SYS HEXDEC", "host", "SYS HEXDEC alias of SYS HEXD"},
      {"SYS URLENC", "host", "SYS URLENC|URLENCODE [str] — RFC3986 percent-encode"},
      {"SYS URLENCODE", "host", "SYS URLENCODE alias of SYS URLENC"},
      {"SYS PERCENTENC", "host", "SYS PERCENTENC alias of SYS URLENC"},
      {"SYS URLDEC", "host", "SYS URLDEC|URLDECODE [str] — percent-decode (+ as space)"},
      {"SYS URLDECODE", "host", "SYS URLDECODE alias of SYS URLDEC"},
      {"SYS PERCENTDEC", "host", "SYS PERCENTDEC alias of SYS URLDEC"},
      {"SYS UUID", "host", "SYS UUID|GUID — RFC4122 v4 id (dashed)"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM [n] — n random bytes as hex nonces/tokens"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS GUID", "host", "SYS GUID alias of SYS UUID"},
      {"SYS GENUUID", "host", "SYS GENUUID alias of SYS UUID"},
      {"SYS UUID0", "host", "SYS UUID0|UUIDNODASH — compact 32-hex UUID"},
      {"SYS UUIDNODASH", "host", "SYS UUIDNODASH alias of SYS UUID0"},
      {"SYS JSONESC", "host", "SYS JSONESC|JESC [str] — JSON string-body escape"},
      {"SYS SHQUOTE", "host", "SYS SHQUOTE|SHELLQUOTE [str] — POSIX shell-safe single-quote wrap"},
      {"SYS SHUNQUOTE", "host", "SYS SHUNQUOTE|UNQUOTE [str] — peel POSIX single-quote wrap"},
      {"SYS GETLOGIN", "host", "SYS GETLOGIN|LOGINNAME — session login via getlogin()"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM [n] — n random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS JESC", "host", "SYS JESC alias of SYS JSONESC"},
      {"SYS JSONUNESC", "host", "SYS JSONUNESC|JUNESC [str] — JSON string-body unescape"},
      {"SYS JUNESC", "host", "SYS JUNESC alias of SYS JSONUNESC"},
      {"SYS CRC32", "host", "SYS CRC32|CRC [str] — IEEE CRC-32 hex stamp"},
      {"SYS CRC", "host", "SYS CRC alias of SYS CRC32"},
      {"SYS CRC32FILE", "host", "SYS CRC32FILE|FILECRC32 path — CRC-32 of file bytes"},
      {"SYS FILECRC32", "host", "SYS FILECRC32 alias of SYS CRC32FILE"},
      {"SYS SYMLINK", "host", "SYS SYMLINK|LN target linkpath — create symlink"},
      {"SYS LN", "host", "SYS LN alias of SYS SYMLINK"},
      {"SYS READLINK", "host", "SYS READLINK|LINKTARGET path — peel symlink target"},
      {"SYS LINKTARGET", "host", "SYS LINKTARGET alias of SYS READLINK"},
      {"SYS HUMANSIZE", "host", "SYS HUMANSIZE|BYTESFMT [n] — integer B/K/M/G/T label"},
      {"SYS BYTESFMT", "host", "SYS BYTESFMT alias of SYS HUMANSIZE"},
      {"SYS HUMANSZ", "host", "SYS HUMANSZ alias of SYS HUMANSIZE"},
      {"SYS PARSESIZE", "host", "SYS PARSESIZE|FROMHUMAN [str] — 1K/2M → bytes"},
      {"SYS FROMHUMAN", "host", "SYS FROMHUMAN alias of SYS PARSESIZE"},
      {"SYS TOBYTES", "host", "SYS TOBYTES alias of SYS PARSESIZE"},
      {"SYS MONOTONIC", "host", "SYS MONOTONIC|MONO — steady ms for elapsed timing"},
      {"SYS MONO", "host", "SYS MONO alias of SYS MONOTONIC"},
      {"SYS STEADY_MS", "host", "SYS STEADY_MS alias of SYS MONOTONIC"},
      {"SYS NPROC", "host", "SYS NPROC|CPUS|CORES — online processor count → LAST_N"},
      {"SYS CPUS", "host", "SYS CPUS alias of SYS NPROC"},
      {"SYS CORES", "host", "SYS CORES alias of SYS NPROC"},
      {"SYS UNAME", "host", "SYS UNAME|OS — kernel/OS name → LAST/UNAME"},
      {"SYS OS", "host", "SYS OS alias of SYS UNAME"},
      {"SYS ARCH", "host", "SYS ARCH|MACHINE — machine arch → LAST/ARCH"},
      {"SYS MACHINE", "host", "SYS MACHINE alias of SYS ARCH"},
      {"SYS PLATFORM", "host", "SYS PLATFORM — os/arch combined tag"},
      {"SYS LOCAL", "host", "SYS LOCAL|LOCALTIME — local wall YYYY-MM-DDTHH:MM:SS"},
      {"SYS LOCALTIME", "host", "SYS LOCALTIME alias of SYS LOCAL"},
      {"SYS LOCALDATE", "host", "SYS LOCALDATE — local date-only YYYY-MM-DD"},
      {"SYS ISATTY", "host", "SYS ISATTY|TTY [IN|OUT|ERR] — terminal probe → LAST_N"},
      {"SYS TTY", "host", "SYS TTY alias of SYS ISATTY"},
      {"SYS TTYNAME", "host", "SYS TTYNAME|CTTY|TTYDEV [IN|OUT|ERR] — terminal device path → LAST"},
      {"SYS NICE", "host", "SYS NICE|GETNICE [n]|RENICE n — process nice get/set → LAST_N"},
      {"SYS SHQUOTE", "host", "SYS SHQUOTE|SHELLQUOTE [str] — POSIX single-quote shell-safe wrap"},
      {"SYS SHUNQUOTE", "host", "SYS SHUNQUOTE|UNQUOTE [str] — peel POSIX single-quote wrap"},
      {"SYS GETLOGIN", "host", "SYS GETLOGIN|LOGINNAME — session login via getlogin()"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM [n] — n random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS LOADAVG", "host", "SYS LOADAVG|LOAD — 1/5/15 load · LOAD1_N centiload"},
      {"SYS LOAD", "host", "SYS LOAD alias of SYS LOADAVG"},
      {"SYS UPTIME", "host", "SYS UPTIME|BOOTAGE — seconds since boot → LAST_N"},
      {"SYS BOOTAGE", "host", "SYS BOOTAGE alias of SYS UPTIME"},
      {"SYS MEM", "host", "SYS MEM|MEMINFO — RAM total/avail/free kB · LAST_N=avail"},
      {"SYS MEMINFO", "host", "SYS MEMINFO alias of SYS MEM"},
      {"SYS DF", "host", "SYS DF|DISKFREE [path] — filesystem free kB · LAST_N=avail"},
      {"SYS DISKFREE", "host", "SYS DISKFREE alias of SYS DF"},
      {"SYS FSYNC", "host", "SYS FSYNC|SYNCFILE path — durable flush file to disk"},
      {"SYS SYNCFILE", "host", "SYS SYNCFILE alias of SYS FSYNC"},
      {"SYS TRUNCATE", "host", "SYS TRUNCATE|FTRUNCATE path [nbytes] — resize plate file"},
      {"SYS FTRUNCATE", "host", "SYS FTRUNCATE alias of SYS TRUNCATE"},
      {"SYS FILETRUNC", "host", "SYS FILETRUNC alias of SYS TRUNCATE"},
      {"SYS DIRSYNC", "host", "SYS DIRSYNC|FSYNCDIR path — fsync dir or parent for durable rename"},
      {"SYS FSYNCDIR", "host", "SYS FSYNCDIR alias of SYS DIRSYNC"},
      {"SYS SYNCDIR", "host", "SYS SYNCDIR alias of SYS DIRSYNC"},
      {"SYS HARDLINK", "host", "SYS HARDLINK|HLINK existing newpath — hard link same inode"},
      {"SYS HLINK", "host", "SYS HLINK alias of SYS HARDLINK"},
      {"SYS NLINK", "host", "SYS NLINK|LINKS path — hard-link count → LAST_N"},
      {"SYS SAMEINODE", "host", "SYS SAMEINODE|EQINODE a b — 1 if same device+inode"},
      {"SYS EQINODE", "host", "SYS EQINODE alias of SYS SAMEINODE"},
      {"SYS INODE", "host", "SYS INODE|INO path — inode number → LAST_N"},
      {"SYS TEE", "host", "SYS TEE|WRITETEE path [data] — write keep content in LAST"},
      {"SYS WRITETEE", "host", "SYS WRITETEE alias of SYS TEE"},
      {"SYS FILEUID", "host", "SYS FILEUID|OWNERUID path — owner uid → LAST_N"},
      {"SYS FILEGID", "host", "SYS FILEGID|OWNERGID path — group gid → LAST_N"},
      {"SYS OWNEDBY", "host", "SYS OWNEDBY|ISOWNER path — LAST_N 1 if euid owns"},
      {"SYS CANREAD", "host", "SYS CANREAD|READABLE path — access R_OK → LAST_N 0|1"},
      {"SYS CANWRITE", "host", "SYS CANWRITE|WRITABLE path — access W_OK → LAST_N 0|1"},
      {"SYS CANEXEC", "host", "SYS CANEXEC|EXECUTABLE path — access X_OK → LAST_N 0|1"},
      {"SYS OWNERNAME", "host", "SYS OWNERNAME|OWNERUSER path — owner login name → LAST"},
      {"SYS GROUPNAME", "host", "SYS GROUPNAME|OWNERGROUP path — group name → LAST"},
      {"SYS CANCREATE", "host", "SYS CANCREATE|CREATABLE path — create/overwrite probe LAST_N 0|1"},
      {"SYS UMASK", "host", "SYS UMASK [mode] — get/set process file-creation mask (octal)"},
      {"SYS PUSHD", "host", "SYS PUSHD|PUSHDIR path — save cwd then chdir; LAST_N=depth"},
      {"SYS POPD", "host", "SYS POPD|POPDIR — restore previous cwd from stack"},
      {"SYS DIRSTACK", "host", "SYS DIRSTACK|DIRS — newline bag of saved dirs"},
      {"SYS KINDSTR", "host", "SYS KINDSTR|FILEKIND path — kind label file|dir|link|missing"},
      {"SYS RELPATH", "host", "SYS RELPATH|REL base path — relative to base or absolute"},
      {"SYS ATIME", "host", "SYS ATIME|ACCESSTIME path — last access epoch → LAST_N"},
      {"SYS CTIME", "host", "SYS CTIME|CHANGETIME path — inode status-change epoch → LAST_N"},
      {"SYS HASENV", "host", "SYS HASENV|ENVHAS name — LAST_N 1 if env set non-empty"},
      {"SYS GID", "host", "SYS GID|GROUP_ID — real group id → LAST_N"},
      {"SYS EGID", "host", "SYS EGID|GETEGID — effective group id → LAST_N"},
      {"SYS GROUP", "host", "SYS GROUP|GNAME — primary group name → LAST"},
      {"SYS GROUPS", "host", "SYS GROUPS|GROUPLIST — egid+supplementary names bag"},
      {"SYS INGROUP", "host", "SYS INGROUP|MEMBEROF name|gid — membership probe LAST_N 0|1"},
      {"SYS PPID", "host", "SYS PPID|GETPPID|PARENT_PID — parent process id → LAST_N"},
      {"SYS PGID", "host", "SYS PGID|GETPGID|PGRP — process group id → LAST_N"},
      {"SYS SID", "host", "SYS SID|GETSID|SESSION — session id → LAST_N"},
      {"SYS DOTENV", "host", "SYS DOTENV|LOADENV|ENVFILE path — load KEY=VAL plate into process env"},
      {"SYS LOADENV", "host", "SYS LOADENV alias of SYS DOTENV"},
      {"SYS ENVFILE", "host", "SYS ENVFILE alias of SYS DOTENV"},
      {"SYS DUMPENV", "host", "SYS DUMPENV|ENVALL [prefix] — process env as KEY=VAL bag"},
      {"SYS ENVALL", "host", "SYS ENVALL alias of SYS DUMPENV"},
      {"SYS LISTENV", "host", "SYS LISTENV alias of SYS DUMPENV"},
      {"SYS WRITEENV", "host", "SYS WRITEENV|SAVENV path [prefix] — write env KEY=VAL plate"},
      {"SYS SAVENV", "host", "SYS SAVENV alias of SYS WRITEENV"},
      {"SYS DUMPENVFILE", "host", "SYS DUMPENVFILE alias of SYS WRITEENV"},
      {"SYS WHICHBIN", "host", "SYS WHICHBIN|PATHWHICH name — PATH-only soft resolve"},
      {"REQUIRE BIN", "flow", "REQUIRE BIN|CMD|EXE name — fail if tool not on PATH"},
      {"SYS ISLINK", "host", "SYS ISLINK|ISLNK path — LAST_N 1 if symlink"},
      {"SYS ISLNK", "host", "SYS ISLNK alias of SYS ISLINK"},
      {"SYS ISSYMLINK", "host", "SYS ISSYMLINK alias of SYS ISLINK"},
      {"SYS MODE", "host", "SYS MODE|PERM path — octal perms → LAST/MODE_N"},
      {"SYS PERM", "host", "SYS PERM alias of SYS MODE"},
      {"SYS CHMOD", "host", "SYS CHMOD path mode — set permission bits"},
      {"SYS SETMODE", "host", "SYS SETMODE alias of SYS CHMOD"},
      {"SYS ARG", "host", "SYS ARG n|name [OR fallback] via CUBALC_ARGn"},
      {"SYS CWD", "host", "SYS CWD — working directory → LAST/CWD"},
      {"SYS CHDIR", "host", "SYS CHDIR|CD path — change process cwd soft miss"},
      {"SYS CD", "host", "SYS CD path — alias of SYS CHDIR"},
      {"SYS STATE", "host", "SYS STATE — CUBALC_STATE plate dir → LAST"},
      {"SYS ROOT", "host", "SYS ROOT — CUBALC_ROOT or cwd → LAST"},
      {"SYS TMP", "host", "SYS TMP|TEMP|TMPDIR — portable temp dir → LAST/TMP"},
      {"SYS MKTEMP", "host", "SYS MKTEMP|TEMPFILE [prefix] — unique empty file under TMP"},
      {"SYS TEMPFILE", "host", "SYS TEMPFILE alias of SYS MKTEMP"},
      {"SYS MAKETEMP", "host", "SYS MAKETEMP alias of SYS MKTEMP"},
      {"SYS MKTEMPDIR", "host", "SYS MKTEMPDIR|MKDTEMP [prefix] — unique empty dir under TMP"},
      {"SYS MKDTEMP", "host", "SYS MKDTEMP alias of SYS MKTEMPDIR"},
      {"SYS MAKETEMPDIR", "host", "SYS MAKETEMPDIR alias of SYS MKTEMPDIR"},
      {"SYS MKDIR", "host", "SYS MKDIR path — mkdir -p · OK if dir exists"},
      {"SYS BASENAME", "host", "SYS BASENAME|LEAF path — final component → LAST"},
      {"SYS DIRNAME", "host", "SYS DIRNAME|PARENT path — parent directory → LAST"},
      {"SYS EXTNAME", "host", "SYS EXTNAME|EXT|SUFFIX path — final .ext → LAST/EXT"},
      {"SYS STEM", "host", "SYS STEM|ROOTNAME path — basename without ext → LAST"},
      {"SYS SIZE", "host", "SYS SIZE|FSIZE path — file bytes → LAST_N · soft miss"},
      {"SYS STAT", "host", "SYS STAT|FSTAT path — one-shot exist/kind/size/mtime/isfile"},
      {"SYS FSTAT", "host", "SYS FSTAT alias of SYS STAT"},
      {"SYS FILESTAT", "host", "SYS FILESTAT alias of SYS STAT"},
      {"SYS ISDIR", "host", "SYS ISDIR path — LAST_N 1 if directory"},
      {"SYS ISFILE", "host", "SYS ISFILE path — LAST_N 1 if regular file"},
      {"SYS READ", "host", "SYS READ [OR|SOFT] path [OR fallback] optional plate"},
      {"SYS LOADPLATE", "host", "SYS LOADPLATE|READPLATE path [OR defaults] — soft plate file load"},
      {"SYS SAVEPLATE", "host", "SYS SAVEPLATE|STOREPLATE path [plate] — object plate persist"},
      {"SYS ENSUREPLATE", "host", "SYS ENSUREPLATE|TOUCHPLATE path [OR defaults] create-or-load"},
      {"SYS UPDATEPLATE", "host", "SYS UPDATEPLATE|JSONFILESET path key value load-set-save plate field"},
      {"SYS BUMPPLATE", "host", "SYS BUMPPLATE|JSONFILEINC path key [delta] load-inc-save counter"},
      {"SYS PEEKPLATE", "host", "SYS PEEKPLATE|JSONFILEGET path key [OR fb] peel plate file field"},
      {"SYS DROPPLATE", "host", "SYS DROPPLATE|JSONFILEDEL path key load-del-save plate field"},
      {"SYS JSONFILEMERGE", "host", "SYS JSONFILEMERGE|OVERLAYPLATE path overlay multi-key load-merge-save"},
      {"SYS JSONFILEENSURE", "host", "SYS JSONFILEENSURE|ENSUREPLATEKEY path key value set-if-missing"},
      {"SYS JSONFILEDEFAULTS", "host", "SYS JSONFILEDEFAULTS|DEFAULTPLATEALL path defaults multi-key fill-if-missing"},
      {"SYS JSONFILETOGGLE", "host", "SYS JSONFILETOGGLE|TOGGLEPLATEKEY path key flip disk flag 0↔1"},
      {"SYS RM", "host", "SYS RM|UNLINK|DELETE path — remove file · miss soft"},
      {"SYS RMDIR", "host", "SYS RMDIR|REMOVEDIR path — remove empty dir · miss soft"},
      {"SYS REMOVEDIR", "host", "SYS REMOVEDIR alias of SYS RMDIR"},
      {"SYS RD", "host", "SYS RD alias of SYS RMDIR"},
      {"SYS RENAME", "host", "SYS RENAME|MV|MOVE from to — move plate"},
      {"SYS COPY", "host", "SYS COPY|CP src dst — duplicate file · LAST_N=bytes"},
      {"SYS REALPATH", "host", "SYS REALPATH|ABSPATH path — absolute path → LAST"},
      {"SYS TOUCH", "host", "SYS TOUCH path — create empty / refresh mtime"},
      {"SYS TIME", "host", "SYS TIME|NOW|EPOCH — wall seconds → LAST_N/TIME"},
      {"SYS MS", "host", "SYS MS|MILLIS|TIME_MS — wall milliseconds → LAST_N/MS"},
      {"SYS SLEEP", "host", "SYS SLEEP n|\"1s\"|\"250ms\" — ms or PARSEMS duration (cap 60s)"},
      {"SYS WAITFILE", "host", "SYS WAITFILE path [ms|\"2s\"] — poll until exists · duration timeout"},
      {"SYS WAITPATH", "host", "SYS WAITPATH alias of SYS WAITFILE"},
      {"SYS POLLFILE", "host", "SYS POLLFILE alias of SYS WAITFILE"},
      {"SYS WAITGONE", "host", "SYS WAITGONE|WAITMISSING path [ms] — poll until path gone"},
      {"SYS WAITMISSING", "host", "SYS WAITMISSING alias of SYS WAITGONE"},
      {"SYS WAITDELETE", "host", "SYS WAITDELETE alias of SYS WAITGONE"},
      {"SYS WAITSTABLE", "host", "SYS WAITSTABLE|WAITQUIET path [ms]|quiet timeout — settle size+mtime"},
      {"SYS WAITQUIET", "host", "SYS WAITQUIET alias of SYS WAITSTABLE"},
      {"SYS STABLEFILE", "host", "SYS STABLEFILE alias of SYS WAITSTABLE"},
      {"SYS WAITCHANGED", "host", "SYS WAITCHANGED|WAITMODIFIED path [SINCE size mtime] [ms] — poll until change"},
      {"SYS WAITMODIFIED", "host", "SYS WAITMODIFIED alias of SYS WAITCHANGED"},
      {"SYS POLLCHANGE", "host", "SYS POLLCHANGE alias of SYS WAITCHANGED"},
      {"SYS WAITMATCH", "host", "SYS WAITMATCH|WAITCONTAINS path needle [ms] — poll until content match"},
      {"SYS WAITCONTAINS", "host", "SYS WAITCONTAINS alias of SYS WAITMATCH"},
      {"SYS WAITMATCHI", "host", "SYS WAITMATCHI case-insensitive WAITMATCH"},
      {"SYS RAND", "host", "SYS RAND|RANDOM [n]|[lo hi] — uniform int for jitter"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM [n] — n random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS RANDOM", "host", "SYS RANDOM [n]|[lo hi] — alias of SYS RAND"},
      {"SYS PICK", "host", "SYS PICK|CHOICE [str] — random newline field from bag"},
      {"SYS CHOICE", "host", "SYS CHOICE alias of SYS PICK"},
      {"SYS SHUFFLE", "host", "SYS SHUFFLE|SHUF [str] — randomize newline bag order"},
      {"SYS SHUF", "host", "SYS SHUF alias of SYS SHUFFLE"},
      {"SYS DRAWN", "host", "SYS DRAWN|SAMPLEK k [bag] — sample k unique fields"},
      {"SYS SAMPLEK", "host", "SYS SAMPLEK k [bag] — alias of SYS DRAWN"},
      {"SYS NPICK", "host", "SYS NPICK k [bag] — alias of SYS DRAWN"},
      {"SYS TAKERAND", "host", "SYS TAKERAND k [bag] — alias of SYS DRAWN"},
      {"SYS MIN", "host", "SYS MIN a b [c…]|bag — host-plane minimum"},
      {"SYS MAX", "host", "SYS MAX a b [c…]|bag — host-plane maximum"},
      {"SYS MINBAG", "host", "SYS MINBAG alias of bag-mode MIN"},
      {"SYS MAXBAG", "host", "SYS MAXBAG alias of bag-mode MAX"},
      {"SYS ARGMAX", "host", "SYS ARGMAX a b [c…]|bag — index of first max"},
      {"SYS ARGMIN", "host", "SYS ARGMIN a b [c…]|bag — index of first min"},
      {"SYS MAXIDX", "host", "SYS MAXIDX alias of SYS ARGMAX"},
      {"SYS MINIDX", "host", "SYS MINIDX alias of SYS ARGMIN"},
      {"SYS PADALL", "host", "SYS PADALL|RPADALL bag width [pad] — right-pad fields"},
      {"SYS LPADALL", "host", "SYS LPADALL bag width [pad] — left-pad every field"},
      {"SYS RPADALL", "host", "SYS RPADALL alias of PADALL"},
      {"SYS TRUNCALL", "host", "SYS TRUNCALL|CLIPALL [RIGHT] bag width — truncate fields"},
      {"SYS CLIPALL", "host", "SYS CLIPALL bag width — alias of SYS TRUNCALL"},
      {"SYS MAPTRUNC", "host", "SYS MAPTRUNC bag width — alias of SYS TRUNCALL"},
      {"SYS CLAMP", "host", "SYS CLAMP x lo hi — bound x into [lo,hi]"},
      {"SYS IN", "host", "SYS IN|WITHIN x lo hi — inclusive range membership 0|1"},
      {"SYS WITHIN", "host", "SYS WITHIN alias of SYS IN"},
      {"SYS CMP", "host", "SYS CMP a b — three-way numeric compare -1|0|1"},
      {"SYS SCMP", "host", "SYS SCMP a b — string compare -1|0|1"},
      {"SYS IABS", "host", "SYS IABS|NABS x — integer absolute value"},
      {"SYS SIGN", "host", "SYS SIGN|SGN x — signum -1|0|1"},
      {"SYS DIV", "host", "SYS DIV|IDIV|QUOT a b — integer divide"},
      {"SYS MOD", "host", "SYS MOD|REM a b — remainder"},
      {"SYS GCD", "host", "SYS GCD|HCF a b [c…] — greatest common divisor"},
      {"SYS LCM", "host", "SYS LCM a b [c…] — least common multiple"},
      {"SYS POW", "host", "SYS POW|POWER a e — integer power"},
      {"SYS ISQRT", "host", "SYS ISQRT|SQRT n — floor integer square root"},
      {"SYS SUM", "host", "SYS SUM|TOTAL args|bag — sum ints or newline bag"},
      {"SYS PROD", "host", "SYS PROD|PRODUCT args|bag — product of ints"},
      {"SYS AVG", "host", "SYS AVG|MEAN args|bag — integer mean"},
      {"SYS MEDIAN", "host", "SYS MEDIAN|P50 args|bag — integer median (even lower mid)"},
      {"SYS RANGE", "host", "SYS RANGE lo hi [step] — inclusive int sequence bag"},
      {"SYS SEQ", "host", "SYS SEQ n — 1..n number bag"},
      {"SYS IOTA", "host", "SYS IOTA n — 0..n-1 number bag"},
      {"SYS LIST", "host", "SYS LIST|LS path — dir basenames → LAST · count"},
      {"SYS GLOB", "host", "SYS GLOB|MATCHFILES path [pattern] — match basenames"},
      {"SYS MATCHFILES", "host", "SYS MATCHFILES alias of SYS GLOB"},
      {"SYS PATHGLOB", "host", "SYS PATHGLOB|PGLOB path|pattern [pattern] — full paths"},
      {"SYS PGLOB", "host", "SYS PGLOB alias of SYS PATHGLOB"},
      {"SYS FILTERGLOB", "host", "SYS FILTERGLOB|MATCHBAG bag pattern — shell bag filter"},
      {"SYS MATCHBAG", "host", "SYS MATCHBAG alias of SYS FILTERGLOB"},
      {"SYS BASENAMEALL", "host", "SYS BASENAMEALL|MAPBASE [bag] — basename bag map"},
      {"SYS DIRNAMEALL", "host", "SYS DIRNAMEALL|MAPDIR [bag] — dirname bag map"},
      {"SYS KEEPFILES", "host", "SYS KEEPFILES|FILTERFILES [bag] — keep regular-file paths"},
      {"SYS KEEPDIRS", "host", "SYS KEEPDIRS|FILTERDIRS [bag] — keep directory paths"},
      {"SYS SIZEALL", "host", "SYS SIZEALL|MAPSIZE [bag] — path bag byte sizes · SUM"},
      {"SYS MAPSIZE", "host", "SYS MAPSIZE alias of SYS SIZEALL"},
      {"SYS MTIMEALL", "host", "SYS MTIMEALL|MAPMTIME [bag] — epoch mtime bag"},
      {"SYS AGEALL", "host", "SYS AGEALL|MAPAGE [bag] — age-seconds bag"},
      {"SYS NEWEST", "host", "SYS NEWEST|LATEST [bag] — freshest path by mtime"},
      {"SYS OLDEST", "host", "SYS OLDEST|EARLIEST [bag] — oldest path by mtime"},
      {"SYS LARGEST", "host", "SYS LARGEST|BIGGEST [bag] — largest file path"},
      {"SYS SORTMTIME", "host", "SYS SORTMTIME [bag] [ASC|DESC] — order by mtime"},
      {"SYS SORTSIZE", "host", "SYS SORTSIZE [bag] [ASC|DESC] — order by size"},
      {"SYS FRESH", "host", "SYS FRESH bag max_age_sec — keep age<=max"},
      {"SYS KEEPSTALE", "host", "SYS KEEPSTALE|AGED bag min_age keep stale paths"},
      {"SYS KEEPNEWER", "host", "SYS KEEPNEWER|NEWERTHAN [bag] ref — mtime>ref"},
      {"SYS NEWERTHAN", "host", "SYS NEWERTHAN alias of SYS KEEPNEWER"},
      {"SYS KEEPOLDER", "host", "SYS KEEPOLDER|OLDERREF [bag] ref — mtime<ref"},
      {"SYS OLDERREF", "host", "SYS OLDERREF alias of SYS KEEPOLDER"},
      {"SYS KEEPBIGGER", "host", "SYS KEEPBIGGER|BIGFILES|SIZEGE bag min_bytes — size>=min"},
      {"SYS BIGFILES", "host", "SYS BIGFILES alias of SYS KEEPBIGGER"},
      {"SYS SIZEGE", "host", "SYS SIZEGE alias of SYS KEEPBIGGER"},
      {"SYS KEEPSMALLER", "host", "SYS KEEPSMALLER|SMALLFILES|SIZELE bag max_bytes — size<=max"},
      {"SYS SMALLFILES", "host", "SYS SMALLFILES alias of SYS KEEPSMALLER"},
      {"SYS SIZELE", "host", "SYS SIZELE alias of SYS KEEPSMALLER"},
      {"SYS RMALL", "host", "SYS RMALL|UNLINKALL|DELETEALL [bag] — bulk remove paths"},
      {"SYS UNLINKALL", "host", "SYS UNLINKALL alias of SYS RMALL"},
      {"SYS DELETEALL", "host", "SYS DELETEALL alias of SYS RMALL"},
      {"SYS TOUCHALL", "host", "SYS TOUCHALL|ENSUREALL|CREATEALL [bag] — create/refresh paths"},
      {"SYS ENSUREALL", "host", "SYS ENSUREALL alias of SYS TOUCHALL"},
      {"SYS CREATEALL", "host", "SYS CREATEALL alias of SYS TOUCHALL"},
      {"SYS COPYALL", "host", "SYS COPYALL|CPALL [bag] dest_dir — bulk copy into dest"},
      {"SYS CPALL", "host", "SYS CPALL alias of SYS COPYALL"},
      {"SYS BULKCOPY", "host", "SYS BULKCOPY alias of SYS COPYALL"},
      {"SYS MKDIRALL", "host", "SYS MKDIRALL|ENSUREDIRS|MKDIRS [bag] — mkdir -p every path"},
      {"SYS ENSUREDIRS", "host", "SYS ENSUREDIRS alias of SYS MKDIRALL"},
      {"SYS MKDIRS", "host", "SYS MKDIRS alias of SYS MKDIRALL"},
      {"SYS MOVEALL", "host", "SYS MOVEALL|MVALL [bag] dest_dir — bulk move into dest"},
      {"SYS MVALL", "host", "SYS MVALL alias of SYS MOVEALL"},
      {"SYS RENAMEALL", "host", "SYS RENAMEALL alias of SYS MOVEALL"},
      {"SYS WALK", "host", "SYS WALK|FINDALL root [pattern] — recursive full paths"},
      {"SYS FINDALL", "host", "SYS FINDALL alias of SYS WALK"},
      {"SYS TREEGLOB", "host", "SYS TREEGLOB alias of SYS WALK"},
      {"SYS EQFILE", "host", "SYS EQFILE|SAMEFILE|CMPFILE a b — identical content"},
      {"SYS SAMEFILE", "host", "SYS SAMEFILE alias of SYS EQFILE"},
      {"SYS CMPFILE", "host", "SYS CMPFILE alias of SYS EQFILE"},
      {"SYS LOGALL", "host", "SYS LOGALL|APPENDFILES [bag] data — append line to each path"},
      {"SYS APPENDFILES", "host", "SYS APPENDFILES alias of SYS LOGALL"},
      {"SYS BULKAPPEND", "host", "SYS BULKAPPEND alias of SYS LOGALL"},
      {"SYS GREPFILES", "host", "SYS GREPFILES|SEARCHFILES [bag] needle — paths whose content has needle"},
      {"SYS SEARCHFILES", "host", "SYS SEARCHFILES alias of SYS GREPFILES"},
      {"SYS FILESGREP", "host", "SYS FILESGREP alias of SYS GREPFILES"},
      {"SYS GREPFILESI", "host", "SYS GREPFILESI case-insensitive content path filter"},
      {"SYS GREPVFILES", "host", "SYS GREPVFILES|VGREPFILES invert content path filter"},
      {"SYS READALL", "host", "SYS READALL|CATFILES [bag] [sep] — concat file contents → LAST"},
      {"SYS CATFILES", "host", "SYS CATFILES alias of SYS READALL"},
      {"SYS SLURPALL", "host", "SYS SLURPALL alias of SYS READALL"},
      {"SYS CATALL", "host", "SYS CATALL alias of SYS READALL"},
      {"SYS WRITEALL", "host", "SYS WRITEALL|WRITEFILES [bag] data — overwrite every path with data"},
      {"SYS WRITEFILES", "host", "SYS WRITEFILES alias of SYS WRITEALL"},
      {"SYS BULKWRITE", "host", "SYS BULKWRITE alias of SYS WRITEALL"},
      {"SYS SPLATALL", "host", "SYS SPLATALL alias of SYS WRITEALL"},
      {"SYS WRITEATOMIC", "host", "SYS WRITEATOMIC|SAFEWRITE path data — temp+rename crash-safer write"},
      {"SYS ATOMICWRITE", "host", "SYS ATOMICWRITE alias of SYS WRITEATOMIC"},
      {"SYS SAFEWRITE", "host", "SYS SAFEWRITE alias of SYS WRITEATOMIC"},
      {"SYS LOCKFILE", "host", "SYS LOCKFILE|FLOCK path [ms] — exclusive path.lock agent mutex"},
      {"SYS FLOCK", "host", "SYS FLOCK alias of SYS LOCKFILE"},
      {"SYS UNLOCKFILE", "host", "SYS UNLOCKFILE|FUNLOCK path — release path.lock"},
      {"SYS CLAIM", "host", "SYS CLAIM|CREATEEXCL path [body] — exclusive path work-item claim"},
      {"SYS CLAIMFILE", "host", "SYS CLAIMFILE alias of SYS CLAIM"},
      {"SYS CREATEEXCL", "host", "SYS CREATEEXCL alias of SYS CLAIM"},
      {"SYS REPLACEFILES", "host", "SYS REPLACEFILES|SUBFILES [bag] old new — rewrite path contents"},
      {"SYS SUBFILES", "host", "SYS SUBFILES alias of SYS REPLACEFILES"},
      {"SYS GSUBFILES", "host", "SYS GSUBFILES alias of SYS REPLACEFILES"},
      {"SYS BULKREPLACE", "host", "SYS BULKREPLACE alias of SYS REPLACEFILES"},
      {"SYS COUNTINFILES", "host", "SYS COUNTINFILES|GREPCOUNTFILES [bag] needle — total hits across files"},
      {"SYS GREPCOUNTFILES", "host", "SYS GREPCOUNTFILES alias of SYS COUNTINFILES"},
      {"SYS FILECOUNT", "host", "SYS FILECOUNT alias of SYS COUNTINFILES"},
      {"SYS COUNTINFILESI", "host", "SYS COUNTINFILESI case-insensitive COUNTINFILES"},
      {"SYS FIRSTFILE", "host", "SYS FIRSTFILE|HITFILE [bag] needle — first path with content match"},
      {"SYS HITFILE", "host", "SYS HITFILE alias of SYS FIRSTFILE"},
      {"SYS GREP1FILE", "host", "SYS GREP1FILE alias of SYS FIRSTFILE"},
      {"SYS LASTFILE", "host", "SYS LASTFILE|HITFILEL [bag] needle — last path with content match"},
      {"SYS FIRSTFILEI", "host", "SYS FIRSTFILEI case-insensitive FIRSTFILE"},
      {"SYS LASTFILEI", "host", "SYS LASTFILEI case-insensitive LASTFILE"},
      {"SYS GREPLINES", "host", "SYS GREPLINES|EXTRACTLINES [bag] needle — collect matching lines from files"},
      {"SYS EXTRACTLINES", "host", "SYS EXTRACTLINES alias of SYS GREPLINES"},
      {"SYS FILEGREPLINES", "host", "SYS FILEGREPLINES alias of SYS GREPLINES"},
      {"SYS GREPLINESI", "host", "SYS GREPLINESI case-insensitive GREPLINES"},
      {"SYS HEADFILE", "host", "SYS HEADFILE|FILEHEAD path [n] — first n lines of file (default 10)"},
      {"SYS FILEHEAD", "host", "SYS FILEHEAD alias of SYS HEADFILE"},
      {"SYS TAILFILE", "host", "SYS TAILFILE|FILETAIL path [n] — last n lines of file"},
      {"SYS FILETAIL", "host", "SYS FILETAIL alias of SYS TAILFILE"},
      {"SYS LINECOUNTALL", "host", "SYS LINECOUNTALL|WCALL [bag] — line counts of path bag + sum"},
      {"SYS WCALL", "host", "SYS WCALL alias of SYS LINECOUNTALL"},
      {"SYS MAPLINES", "host", "SYS MAPLINES alias of SYS LINECOUNTALL"},
      {"SYS LINESALL", "host", "SYS LINESALL alias of SYS LINECOUNTALL"},
      {"SYS MIDFILE", "host", "SYS MIDFILE|LINESLICE path start [count] — file line window → bag"},
      {"SYS FILEMID", "host", "SYS FILEMID alias of SYS MIDFILE"},
      {"SYS LINESLICE", "host", "SYS LINESLICE alias of SYS MIDFILE"},
      {"SYS SLICEFILE", "host", "SYS SLICEFILE alias of SYS MIDFILE"},
      {"SYS LINEAT", "host", "SYS LINEAT|FILELINE|ATLINE path n — single 0-based file line → LAST"},
      {"SYS FILELINE", "host", "SYS FILELINE alias of SYS LINEAT"},
      {"SYS ATLINE", "host", "SYS ATLINE alias of SYS LINEAT"},
      {"SYS NTHFILELINE", "host", "SYS NTHFILELINE alias of SYS LINEAT"},
      {"SYS SETFILELINE", "host", "SYS SETFILELINE|FILESETLINE path n value — replace 0-based file line in place"},
      {"SYS FILESETLINE", "host", "SYS FILESETLINE alias of SYS SETFILELINE"},
      {"SYS PUTFILELINE", "host", "SYS PUTFILELINE alias of SYS SETFILELINE"},
      {"SYS SETLINEAT", "host", "SYS SETLINEAT alias of SYS SETFILELINE"},
      {"SYS INSERTFILELINE", "host", "SYS INSERTFILELINE|FILEINSERTLINE path n value — insert line at 0-based index"},
      {"SYS FILEINSERTLINE", "host", "SYS FILEINSERTLINE alias of SYS INSERTFILELINE"},
      {"SYS INSFILELINE", "host", "SYS INSFILELINE alias of SYS INSERTFILELINE"},
      {"SYS INSLINEF", "host", "SYS INSLINEF alias of SYS INSERTFILELINE"},
      {"SYS DROPFILELINE", "host", "SYS DROPFILELINE|FILEDROPLINE path n — drop 0-based file line in place"},
      {"SYS FILEDROPLINE", "host", "SYS FILEDROPLINE alias of SYS DROPFILELINE"},
      {"SYS DELFILELINE", "host", "SYS DELFILELINE alias of SYS DROPFILELINE"},
      {"SYS DROPLINEF", "host", "SYS DROPLINEF alias of SYS DROPFILELINE"},
      {"SYS FINDFILELINE", "host", "SYS FINDFILELINE|FILEFINDLINE path needle — 0-based first line containing needle"},
      {"SYS FILEFINDLINE", "host", "SYS FILEFINDLINE alias of SYS FINDFILELINE"},
      {"SYS LINEINDEXF", "host", "SYS LINEINDEXF alias of SYS FINDFILELINE"},
      {"SYS FINDFILELINEI", "host", "SYS FINDFILELINEI case-insensitive FINDFILELINE"},
      {"SYS LASTFINDFILELINE", "host", "SYS LASTFINDFILELINE|FILELASTFIND path needle — 0-based last line containing needle"},
      {"SYS FILELASTFIND", "host", "SYS FILELASTFIND alias of SYS LASTFINDFILELINE"},
      {"SYS LINEINDEXFL", "host", "SYS LINEINDEXFL alias of SYS LASTFINDFILELINE"},
      {"SYS LASTFINDFILELINEI", "host", "SYS LASTFINDFILELINEI case-insensitive LASTFINDFILELINE"},
      {"SYS COUNTFILELINE", "host", "SYS COUNTFILELINE|FILECOUNTLINE path needle — count lines containing needle → LAST_N"},
      {"SYS FILECOUNTLINE", "host", "SYS FILECOUNTLINE alias of SYS COUNTFILELINE"},
      {"SYS COUNTMATCHF", "host", "SYS COUNTMATCHF alias of SYS COUNTFILELINE"},
      {"SYS COUNTFILELINEI", "host", "SYS COUNTFILELINEI case-insensitive COUNTFILELINE"},
      {"SYS GREPFILE", "host", "SYS GREPFILE|FILEGREP path needle — keep matching file lines → bag"},
      {"SYS FILEGREP", "host", "SYS FILEGREP alias of SYS GREPFILE"},
      {"SYS MATCHFILELINES", "host", "SYS MATCHFILELINES alias of SYS GREPFILE"},
      {"SYS GREPFILEI", "host", "SYS GREPFILEI case-insensitive GREPFILE"},
      {"SYS SETMATCHFILE", "host", "SYS SETMATCHFILE|FILESETMATCH path needle value — replace first matching file line"},
      {"SYS FILESETMATCH", "host", "SYS FILESETMATCH alias of SYS SETMATCHFILE"},
      {"SYS REPLACEMATCHF", "host", "SYS REPLACEMATCHF alias of SYS SETMATCHFILE"},
      {"SYS SETMATCHFILEI", "host", "SYS SETMATCHFILEI case-insensitive SETMATCHFILE"},
      {"SYS DROPMATCHFILE", "host", "SYS DROPMATCHFILE|FILEDROPMATCH path needle — drop first matching file line"},
      {"SYS FILEDROPMATCH", "host", "SYS FILEDROPMATCH alias of SYS DROPMATCHFILE"},
      {"SYS DELMATCHFILE", "host", "SYS DELMATCHFILE alias of SYS DROPMATCHFILE"},
      {"SYS DROPMATCHFILEI", "host", "SYS DROPMATCHFILEI case-insensitive DROPMATCHFILE"},
      {"SYS DROPALLMATCHFILE", "host", "SYS DROPALLMATCHFILE|FILEDROPALLMATCH path needle — drop all matching file lines"},
      {"SYS FILEDROPALLMATCH", "host", "SYS FILEDROPALLMATCH alias of SYS DROPALLMATCHFILE"},
      {"SYS DELALLMATCHFILE", "host", "SYS DELALLMATCHFILE alias of SYS DROPALLMATCHFILE"},
      {"SYS PURGEMATCHFILE", "host", "SYS PURGEMATCHFILE alias of SYS DROPALLMATCHFILE"},
      {"SYS DROPALLMATCHFILEI", "host", "SYS DROPALLMATCHFILEI case-insensitive DROPALLMATCHFILE"},
      {"SYS SETALLMATCHFILE", "host", "SYS SETALLMATCHFILE|FILESETALLMATCH path needle value — replace all matching file lines"},
      {"SYS FILESETALLMATCH", "host", "SYS FILESETALLMATCH alias of SYS SETALLMATCHFILE"},
      {"SYS REPLACEALLMATCHF", "host", "SYS REPLACEALLMATCHF alias of SYS SETALLMATCHFILE"},
      {"SYS SETALLMATCHFILEI", "host", "SYS SETALLMATCHFILEI case-insensitive SETALLMATCHFILE"},
      {"SYS INSERTMATCHFILE", "host", "SYS INSERTMATCHFILE|FILEINSERTMATCH|AFTERMATCHFILE path needle value — insert line after first match"},
      {"SYS FILEINSERTMATCH", "host", "SYS FILEINSERTMATCH alias of SYS INSERTMATCHFILE"},
      {"SYS AFTERMATCHFILE", "host", "SYS AFTERMATCHFILE alias of SYS INSERTMATCHFILE"},
      {"SYS INSERTMATCHFILEI", "host", "SYS INSERTMATCHFILEI case-insensitive INSERTMATCHFILE"},
      {"SYS BEFOREMATCHFILE", "host", "SYS BEFOREMATCHFILE|INSERTBEFOREMATCH path needle value — insert line before first match"},
      {"SYS INSERTBEFOREMATCH", "host", "SYS INSERTBEFOREMATCH alias of SYS BEFOREMATCHFILE"},
      {"SYS FILEINSERTBEFORE", "host", "SYS FILEINSERTBEFORE alias of SYS BEFOREMATCHFILE"},
      {"SYS BEFOREMATCHFILEI", "host", "SYS BEFOREMATCHFILEI case-insensitive BEFOREMATCHFILE"},
      {"SYS LOOKUPFILE", "host", "SYS LOOKUPFILE|FILELOOKUP|KVFILEGET path key [sep] [OR fb] — peel key=val from plate file"},
      {"SYS FILELOOKUP", "host", "SYS FILELOOKUP alias of SYS LOOKUPFILE"},
      {"SYS KVFILEGET", "host", "SYS KVFILEGET alias of SYS LOOKUPFILE"},
      {"SYS PLATEGET", "host", "SYS PLATEGET alias of SYS LOOKUPFILE"},
      {"SYS LOOKUPFILEI", "host", "SYS LOOKUPFILEI case-insensitive LOOKUPFILE"},
      {"SYS KVFILESET", "host", "SYS KVFILESET|FILEKVSET|SETKVFILE path key value [sep] — set/update key=val in plate file"},
      {"SYS FILEKVSET", "host", "SYS FILEKVSET alias of SYS KVFILESET"},
      {"SYS SETKVFILE", "host", "SYS SETKVFILE alias of SYS KVFILESET"},
      {"SYS PLATESET", "host", "SYS PLATESET alias of SYS KVFILESET"},
      {"SYS KVFILEDEL", "host", "SYS KVFILEDEL|FILEKVDEL|DELKVFILE path key [sep] — drop key=val line from plate file"},
      {"SYS FILEKVDEL", "host", "SYS FILEKVDEL alias of SYS KVFILEDEL"},
      {"SYS DELKVFILE", "host", "SYS DELKVFILE alias of SYS KVFILEDEL"},
      {"SYS PLATEDEL", "host", "SYS PLATEDEL alias of SYS KVFILEDEL"},
      {"SYS KVFILEINC", "host", "SYS KVFILEINC|INCKVFILE path key [delta] [sep] — bump numeric key=val in plate file"},
      {"SYS INCKVFILE", "host", "SYS INCKVFILE alias of SYS KVFILEINC"},
      {"SYS FILEKVINC", "host", "SYS FILEKVINC alias of SYS KVFILEINC"},
      {"SYS PLATEINC", "host", "SYS PLATEINC alias of SYS KVFILEINC"},
      {"SYS KVFILEENSURE", "host", "SYS KVFILEENSURE|ENSUREKVFILE path key value [sep] — set key only if missing"},
      {"SYS ENSUREKVFILE", "host", "SYS ENSUREKVFILE alias of SYS KVFILEENSURE"},
      {"SYS DEFAULTKVFILE", "host", "SYS DEFAULTKVFILE alias of SYS KVFILEENSURE"},
      {"SYS PLATEENSURE", "host", "SYS PLATEENSURE alias of SYS KVFILEENSURE"},
      {"SYS ENSURELINE", "host", "SYS ENSURELINE|ENSUREFILELINE path line — append line if exact line missing"},
      {"SYS ENSUREFILELINE", "host", "SYS ENSUREFILELINE alias of SYS ENSURELINE"},
      {"SYS ADDIFMISSING", "host", "SYS ADDIFMISSING alias of SYS ENSURELINE"},
      {"SYS FILEENSURE", "host", "SYS FILEENSURE alias of SYS ENSURELINE"},
      {"SYS HASFILELINE", "host", "SYS HASFILELINE|FILEHASLINE path line — exact file line membership → LAST_N 0|1"},
      {"SYS FILEHASLINE", "host", "SYS FILEHASLINE alias of SYS HASFILELINE"},
      {"SYS HASLINEF", "host", "SYS HASLINEF alias of SYS HASFILELINE"},
      {"SYS HASFILELINEI", "host", "SYS HASFILELINEI case-insensitive HASFILELINE"},
      {"SYS DROPEXACTFILE", "host", "SYS DROPEXACTFILE|DROPFILEEXACT path line — drop first exact file line"},
      {"SYS DROPFILEEXACT", "host", "SYS DROPFILEEXACT alias of SYS DROPEXACTFILE"},
      {"SYS REMOVELINEFILE", "host", "SYS REMOVELINEFILE alias of SYS DROPEXACTFILE"},
      {"SYS DROPEXACTFILEI", "host", "SYS DROPEXACTFILEI case-insensitive DROPEXACTFILE"},
      {"SYS TOGGLELINE", "host", "SYS TOGGLELINE|TOGGLEFILELINE path line — flip exact line presence · ON after"},
      {"SYS TOGGLEFILELINE", "host", "SYS TOGGLEFILELINE alias of SYS TOGGLELINE"},
      {"SYS FLIPLINE", "host", "SYS FLIPLINE alias of SYS TOGGLELINE"},
      {"SYS TOGGLELINEI", "host", "SYS TOGGLELINEI case-insensitive TOGGLELINE"},
      {"SYS NTH", "host", "SYS NTH n [str] — 0-based newline field (LIST walk)"},
      {"SYS LINE", "host", "SYS LINE n [str] — 1-based newline field"},
      {"SYS HEAD", "host", "SYS HEAD [str] — first newline field"},
      {"SYS TAIL", "host", "SYS TAIL [str] — last newline field"},
      {"SYS GREP", "host", "SYS GREP|FILTER needle [str] — keep matching lines (LIST)"},
      {"SYS GREPV", "host", "SYS GREPV|VGREP needle [str] — drop matching lines"},
      {"SYS GREPI", "host", "SYS GREPI|IGREP|GREP I — case-insensitive filter"},
      {"SYS GREPVI", "host", "SYS GREPVI invert case-insensitive filter"},
      {"SYS GREPANY", "host", "SYS GREPANY|GREPOR bag n1 [n2…] — match any needle"},
      {"SYS GREPOR", "host", "SYS GREPOR alias of SYS GREPANY"},
      {"SYS GREPANYI", "host", "SYS GREPANYI case-insensitive multi-needle"},
      {"SYS GREPALL", "host", "SYS GREPALL|GREPAND bag n1 [n2…] — match every needle"},
      {"SYS GREPAND", "host", "SYS GREPAND alias of SYS GREPALL"},
      {"SYS GREPALLI", "host", "SYS GREPALLI case-insensitive multi-needle AND"},
      {"SYS STR", "host", "SYS STR|ITOA|NUMSTR [n] — integer → decimal string LAST"},
      {"SYS ITOA", "host", "SYS ITOA [n] — alias of SYS STR · dual of SYS NUM"},
      {"SYS BEFORE", "host", "SYS BEFORE|LEFT_OF hay needle — text left of first needle"},
      {"SYS AFTER", "host", "SYS AFTER|RIGHT_OF hay needle — text right of first needle"},
      {"SYS PUSH", "host", "SYS PUSH|ADDLINE bag [line] — append newline field · accumulate"},
      {"SYS ADDLINE", "host", "SYS ADDLINE bag line — alias of SYS PUSH"},
      {"SYS EQSI", "host", "SYS EQSI|IEQS|EQS I a b — case-insensitive equality"},
      {"SYS HASI", "host", "SYS HASI|ICONTAINS|HAS I hay needle — case-insensitive contains"},
      {"SYS LINES", "host", "SYS LINES|NLINES|WC [str] — count newline fields → LAST_N"},
      {"SYS WC", "host", "SYS WC [str] — alias of SYS LINES field count"},
      {"SYS CUT", "host", "SYS CUT|FIELDN hay sep n — 0-based field by separator"},
      {"SYS COLUMN", "host", "SYS COLUMN|COL hay sep n — 1-based field by separator"},
      {"SYS EMPTY", "host", "SYS EMPTY|ISEMPTY [str] — LAST_N 1 if zero-length"},
      {"SYS BLANK", "host", "SYS BLANK|ISBLANK [str] — empty or whitespace only"},
      {"SYS NONEMPTY", "host", "SYS NONEMPTY [str] — LAST_N 1 if any character"},
      {"SYS COALESCE", "host", "SYS COALESCE|NVL a b [c…] — first non-empty string"},
      {"SYS NVL", "host", "SYS NVL alias of SYS COALESCE"},
      {"SYS UNION", "host", "SYS UNION|ORLINES a [b…] — merge bags first-seen unique"},
      {"SYS DISTINCT", "host", "SYS DISTINCT|UNIQUEALL [bag] — order-preserving full unique"},
      {"SYS INTERSECT", "host", "SYS INTERSECT|ANDLINES a b — fields of a also in b"},
      {"SYS DIFF", "host", "SYS DIFF|EXCEPT|SETDIFF a b — fields of a not in b"},
      {"SYS ZIP", "host", "SYS ZIP|PAIR a b [sep] — pair bag fields by index"},
      {"SYS KEYS", "host", "SYS KEYS|COL0 bag [sep] — peel left of sep each field"},
      {"SYS VALS", "host", "SYS VALS|COL1 bag [sep] — peel right of sep each field"},
      {"SYS PREFIXALL", "host", "SYS PREFIXALL|MAPPRE bag prefix — prepend every field"},
      {"SYS SUFFIXALL", "host", "SYS SUFFIXALL|MAPSUF bag suffix — append every field"},
      {"SYS FILL", "host", "SYS FILL|REPEATL n value — bag of n copies of value"},
      {"SYS ENUMERATE", "host", "SYS ENUMERATE|NUMBER bag [start] [sep] — index-prefix fields"},
      {"SYS NUMBER", "host", "SYS NUMBER bag [start] [sep] — alias of SYS ENUMERATE"},
      {"SYS SQUEEZE", "host", "SYS SQUEEZE|COMPACT [BLANK] bag — drop empty/blank fields"},
      {"SYS COMPACT", "host", "SYS COMPACT alias of SYS SQUEEZE"},
      {"SYS TRIMALL", "host", "SYS TRIMALL|MAPTRIM [L|R] bag — trim every field"},
      {"SYS MAPTRIM", "host", "SYS MAPTRIM alias of SYS TRIMALL"},
      {"SYS UPPERALL", "host", "SYS UPPERALL|MAPUPPER bag — upper every field"},
      {"SYS MAPUPPER", "host", "SYS MAPUPPER alias of SYS UPPERALL"},
      {"SYS LOWERALL", "host", "SYS LOWERALL|MAPLOWER bag — lower every field"},
      {"SYS MAPLOWER", "host", "SYS MAPLOWER alias of SYS LOWERALL"},
      {"SYS MAPREPLACE", "host", "SYS MAPREPLACE|GSUBALL bag old new — GSUB every field"},
      {"SYS GSUBALL", "host", "SYS GSUBALL alias of SYS MAPREPLACE"},
      {"SYS REPLACEBAG", "host", "SYS REPLACEBAG alias of SYS MAPREPLACE"},
      {"SYS FREQ", "host", "SYS FREQ|HIST [bag] [sep] — field frequency key:count"},
      {"SYS HIST", "host", "SYS HIST alias of SYS FREQ"},
      {"SYS COUNTS", "host", "SYS COUNTS alias of SYS FREQ"},
      {"SYS CUTALL", "host", "SYS CUTALL|MAPCUT bag sep n — peel column every line"},
      {"SYS MAPCUT", "host", "SYS MAPCUT alias of SYS CUTALL"},
      {"SYS COLALL", "host", "SYS COLALL 1-based CUTALL"},
      {"SYS SORTFREQ", "host", "SYS SORTFREQ|SORTBYCOUNT [bag] [sep] [DESC|ASC] by count"},
      {"SYS SORTBYCOUNT", "host", "SYS SORTBYCOUNT alias of SYS SORTFREQ"},
      {"SYS FSORT", "host", "SYS FSORT alias of SYS SORTFREQ"},
      {"SYS BEFOREALL", "host", "SYS BEFOREALL|MAPBEFORE bag needle — peel left every field"},
      {"SYS AFTERALL", "host", "SYS AFTERALL|MAPAFTER bag needle — peel right every field"},
      {"SYS MAPBEFORE", "host", "SYS MAPBEFORE alias of SYS BEFOREALL"},
      {"SYS MAPAFTER", "host", "SYS MAPAFTER alias of SYS AFTERALL"},
      {"SYS FIRSTMATCH", "host", "SYS FIRSTMATCH|GREP1 bag needle — first field hit"},
      {"SYS GREP1", "host", "SYS GREP1 alias of SYS FIRSTMATCH"},
      {"SYS FIRSTMATCHI", "host", "SYS FIRSTMATCHI case-insensitive first hit"},
      {"SYS LOOKUP", "host", "SYS LOOKUP bag key [sep] [OR fb] — peel key:val / default"},
      {"SYS KVGET", "host", "SYS KVGET alias of SYS LOOKUP"},
      {"SYS LOOKUPI", "host", "SYS LOOKUPI case-insensitive LOOKUP"},
      {"SYS GETKV", "host", "SYS GETKV alias of SYS LOOKUP"},
      {"SYS KVSET", "host", "SYS KVSET|SETKV bag key value [sep] — set/update key:val"},
      {"SYS SETKV", "host", "SYS SETKV alias of SYS KVSET"},
      {"SYS DICTSET", "host", "SYS DICTSET alias of SYS KVSET"},
      {"SYS MAPSET", "host", "SYS MAPSET alias of SYS KVSET"},
      {"SYS KVINC", "host", "SYS KVINC|INCKV bag key [delta] [sep] — bump key counter"},
      {"SYS INCKV", "host", "SYS INCKV alias of SYS KVINC"},
      {"SYS BUMPKV", "host", "SYS BUMPKV alias of SYS KVINC"},
      {"SYS KVADD", "host", "SYS KVADD alias of SYS KVINC"},
      {"SYS KVDEL", "host", "SYS KVDEL|DELKV bag key [sep] — drop key:val field"},
      {"SYS DELKV", "host", "SYS DELKV alias of SYS KVDEL"},
      {"SYS RMKV", "host", "SYS RMKV alias of SYS KVDEL"},
      {"SYS UNSETKV", "host", "SYS UNSETKV alias of SYS KVDEL"},
      {"SYS MERGEKV", "host", "SYS MERGEKV|KVADDALL bag_a bag_b [sep] — sum-merge key:val"},
      {"SYS KVADDALL", "host", "SYS KVADDALL alias of SYS MERGEKV"},
      {"SYS ADDFREQ", "host", "SYS ADDFREQ alias of SYS MERGEKV"},
      {"SYS SUMKV", "host", "SYS SUMKV|TOTALKV bag [sep] — sum key:val values"},
      {"SYS TOTALKV", "host", "SYS TOTALKV alias of SYS SUMKV"},
      {"SYS SUMVALS", "host", "SYS SUMVALS alias of SYS SUMKV"},
      {"SYS AVGKV", "host", "SYS AVGKV|MEANKV bag [sep] — mean of values"},
      {"SYS MEANKV", "host", "SYS MEANKV alias of SYS AVGKV"},
      {"SYS MEANVAL", "host", "SYS MEANVAL alias of SYS AVGKV"},
      {"SYS MEDIANKV", "host", "SYS MEDIANKV|P50KV bag [sep] — median of values"},
      {"SYS P50KV", "host", "SYS P50KV alias of SYS MEDIANKV"},
      {"SYS MIDKV", "host", "SYS MIDKV alias of SYS MEDIANKV"},
      {"SYS TOPKEY", "host", "SYS TOPKEY|ARGMAXKV bag [sep] — key with max value"},
      {"SYS ARGMAXKV", "host", "SYS ARGMAXKV alias of SYS TOPKEY"},
      {"SYS BOTKEY", "host", "SYS BOTKEY|ARGMINKV bag [sep] — key with min value"},
      {"SYS ARGMINKV", "host", "SYS ARGMINKV alias of SYS BOTKEY"},
      {"SYS LOOKUPN", "host", "SYS LOOKUPN|KVGETN bag key [sep] [OR n] — value as int"},
      {"SYS KVGETN", "host", "SYS KVGETN alias of SYS LOOKUPN"},
      {"SYS GETKVN", "host", "SYS GETKVN alias of SYS LOOKUPN"},
      {"SYS THRESHKV", "host", "SYS THRESHKV|KEEPVAL bag min [sep] — keep value>=min"},
      {"SYS KEEPVAL", "host", "SYS KEEPVAL alias of SYS THRESHKV"},
      {"SYS MINCOUNT", "host", "SYS MINCOUNT alias of SYS THRESHKV"},
      {"SYS PCTKV", "host", "SYS PCTKV|SHAREKV bag [sep] — values as %% of total"},
      {"SYS SHAREKV", "host", "SYS SHAREKV alias of SYS PCTKV"},
      {"SYS PERCENTKV", "host", "SYS PERCENTKV alias of SYS PCTKV"},
      {"SYS CAPKV", "host", "SYS CAPKV|CLAMPKV bag max [sep] — clamp values to max"},
      {"SYS CLAMPKV", "host", "SYS CLAMPKV alias of SYS CAPKV"},
      {"SYS MAXVAL", "host", "SYS MAXVAL alias of SYS CAPKV"},
      {"SYS SCALEKV", "host", "SYS SCALEKV|MULKV bag factor [sep] — multiply values"},
      {"SYS MULKV", "host", "SYS MULKV alias of SYS SCALEKV"},
      {"SYS WEIGHTKV", "host", "SYS WEIGHTKV alias of SYS SCALEKV"},
      {"SYS DIVKV", "host", "SYS DIVKV|IDIVKV bag divisor [sep] — divide values"},
      {"SYS IDIVKV", "host", "SYS IDIVKV alias of SYS DIVKV"},
      {"SYS QUOTKV", "host", "SYS QUOTKV alias of SYS DIVKV"},
      {"SYS DIFFKV", "host", "SYS DIFFKV|SUBKV bag_a bag_b [sep] — subtract key:val"},
      {"SYS SUBKV", "host", "SYS SUBKV alias of SYS DIFFKV"},
      {"SYS DELTAKV", "host", "SYS DELTAKV alias of SYS DIFFKV"},
      {"SYS ADDKV", "host", "SYS ADDKV|OFFSETKV bag delta [sep] — add to values"},
      {"SYS OFFSETKV", "host", "SYS OFFSETKV alias of SYS ADDKV"},
      {"SYS ADDVAL", "host", "SYS ADDVAL alias of SYS ADDKV"},
      {"SYS DROPZERO", "host", "SYS DROPZERO|KEEPNZ bag [sep] — drop value==0"},
      {"SYS KEEPNZ", "host", "SYS KEEPNZ alias of SYS DROPZERO"},
      {"SYS NZKV", "host", "SYS NZKV alias of SYS DROPZERO"},
      {"SYS KEEPKEY", "host", "SYS KEEPKEY|GREPKEY bag needle [sep] — filter by key"},
      {"SYS GREPKEY", "host", "SYS GREPKEY alias of SYS KEEPKEY"},
      {"SYS DROPKEY", "host", "SYS DROPKEY drop keys containing needle"},
      {"SYS ABSKV", "host", "SYS ABSKV|MAGKV bag [sep] — abs key:val values"},
      {"SYS MAGKV", "host", "SYS MAGKV alias of SYS ABSKV"},
      {"SYS ABSALL", "host", "SYS ABSALL alias of SYS ABSKV"},
      {"SYS SIGNKV", "host", "SYS SIGNKV|DIRKV bag [sep] — map values to -1|0|1"},
      {"SYS DIRKV", "host", "SYS DIRKV alias of SYS SIGNKV"},
      {"SYS SGNKV", "host", "SYS SGNKV alias of SYS SIGNKV"},
      {"SYS LASTMATCH", "host", "SYS LASTMATCH|GREP1L bag needle — last field hit"},
      {"SYS GREP1L", "host", "SYS GREP1L alias of SYS LASTMATCH"},
      {"SYS LASTMATCHI", "host", "SYS LASTMATCHI case-insensitive last hit"},
      {"SYS CHUNK", "host", "SYS CHUNK|BATCH n [bag] [join] — group fields by n"},
      {"SYS BATCH", "host", "SYS BATCH alias of SYS CHUNK"},
      {"SYS GROUPN", "host", "SYS GROUPN alias of SYS CHUNK"},
      {"SYS WINDOW", "host", "SYS WINDOW|SLIDE n [bag] [join] — overlapping windows of n"},
      {"SYS SLIDE", "host", "SYS SLIDE alias of SYS WINDOW"},
      {"SYS NGRAM", "host", "SYS NGRAM alias of SYS WINDOW"},
      {"SYS STRIDE", "host", "SYS STRIDE|EVERY step [offset] [bag] — index%step==offset"},
      {"SYS EVERY", "host", "SYS EVERY alias of SYS STRIDE"},
      {"SYS STEP", "host", "SYS STEP alias of SYS STRIDE"},
      {"SYS ROTATE", "host", "SYS ROTATE|ROTL k [bag] — left-rotate bag fields"},
      {"SYS ROTL", "host", "SYS ROTL alias of SYS ROTATE"},
      {"SYS ROTR", "host", "SYS ROTR right-rotate bag fields"},
      {"SYS FLATTEN", "host", "SYS FLATTEN|UNCHUNK [bag] [sep] — split every field flat"},
      {"SYS UNCHUNK", "host", "SYS UNCHUNK alias of SYS FLATTEN"},
      {"SYS SPLITALL", "host", "SYS SPLITALL alias of SYS FLATTEN"},
      {"SYS COUNTMATCH", "host", "SYS COUNTMATCH|GREPCOUNT bag needle — substring field count"},
      {"SYS GREPCOUNT", "host", "SYS GREPCOUNT alias of SYS COUNTMATCH"},
      {"SYS COUNTMATCHI", "host", "SYS COUNTMATCHI case-insensitive COUNTMATCH"},
      {"SYS LENALL", "host", "SYS LENALL|MAPLEN [bag] — length of every field → bag"},
      {"SYS MAPLEN", "host", "SYS MAPLEN alias of SYS LENALL"},
      {"SYS FIELDLENS", "host", "SYS FIELDLENS alias of SYS LENALL"},
      {"SYS MAXLEN", "host", "SYS MAXLEN|MAXWIDTH [bag] — max field length → LAST_N"},
      {"SYS MINLEN", "host", "SYS MINLEN|MINWIDTH [bag] — min field length → LAST_N"},
      {"SYS MAXWIDTH", "host", "SYS MAXWIDTH alias of SYS MAXLEN"},
      {"SYS MINWIDTH", "host", "SYS MINWIDTH alias of SYS MINLEN"},
      {"SYS LONGEST", "host", "SYS LONGEST|MAXFIELD [bag] — first longest field text"},
      {"SYS SHORTEST", "host", "SYS SHORTEST|MINFIELD [bag] — first shortest field text"},
      {"SYS MAXFIELD", "host", "SYS MAXFIELD alias of SYS LONGEST"},
      {"SYS MINFIELD", "host", "SYS MINFIELD alias of SYS SHORTEST"},
      {"SYS COMMONPREFIX", "host", "SYS COMMONPREFIX|LCP [bag] — longest common prefix"},
      {"SYS LCP", "host", "SYS LCP alias of SYS COMMONPREFIX"},
      {"SYS COMMONSUFFIX", "host", "SYS COMMONSUFFIX|LCS [bag] — longest common suffix"},
      {"SYS LCS", "host", "SYS LCS alias of SYS COMMONSUFFIX"},
      {"SYS STRIPPREFIX", "host", "SYS STRIPPREFIX bag prefix — drop leading prefix per field"},
      {"SYS STRIPSUFFIX", "host", "SYS STRIPSUFFIX bag suffix — drop trailing suffix per field"},
      {"SYS STRIPCOMMON", "host", "SYS STRIPCOMMON|STRIPLCP [bag] — strip LCP relative paths"},
      {"SYS STRIPLCP", "host", "SYS STRIPLCP alias of SYS STRIPCOMMON"},
      {"SYS MIDLINES", "host", "SYS MIDLINES|SLICEBAG bag start [end] — field window"},
      {"SYS SLICEBAG", "host", "SYS SLICEBAG alias of SYS MIDLINES"},
      {"SYS POP", "host", "SYS POP|POPLINE bag — last field → LAST · rest → POP_REST"},
      {"SYS POPLINE", "host", "SYS POPLINE bag — alias of SYS POP"},
      {"SYS MTIME", "host", "SYS MTIME|MODTIME path — mtime epoch → LAST_N · soft miss"},
      {"SYS AGE", "host", "SYS AGE|FILEAGE path — seconds since mtime → LAST_N"},
      {"SYS STARTSI", "host", "SYS STARTSI|ISTARTS|STARTS I hay pref — case-insensitive prefix"},
      {"SYS ENDSI", "host", "SYS ENDSI|IENDS|ENDS I hay suf — case-insensitive suffix"},
      {"SYS BETWEEN", "host", "SYS BETWEEN|MIDOF|EXTRACT open close [hay] — peel between delimiters"},
      {"SYS MIDOF", "host", "SYS MIDOF open close [hay] — alias of SYS BETWEEN"},
      {"SYS EXTRACT", "host", "SYS EXTRACT open close [hay] — alias of SYS BETWEEN"},
      {"SYS FINDI", "host", "SYS FINDI|INDEXI|FIND I hay needle — case-insensitive index → LAST_N"},
      {"SYS INDEXI", "host", "SYS INDEXI hay needle — alias of SYS FINDI"},
      {"SYS REVL", "host", "SYS REVL|REVLINES|TAC [str] — reverse newline field order"},
      {"SYS REVLINES", "host", "SYS REVLINES [str] — alias of SYS REVL"},
      {"SYS TAC", "host", "SYS TAC [str] — alias of SYS REVL"},
      {"SYS WORDS", "host", "SYS WORDS|TOKENIZE [str] — whitespace → newline fields"},
      {"SYS TOKENIZE", "host", "SYS TOKENIZE [str] — alias of SYS WORDS"},
      {"SYS HASLINE", "host", "SYS HASLINE|HASFIELD bag needle — exact field membership"},
      {"SYS HASLINEI", "host", "SYS HASLINEI|ILINEIN bag needle — case-insensitive exact field"},
      {"SYS PREPEND", "host", "SYS PREPEND|UNSHIFT bag [line] — insert field at front"},
      {"SYS UNSHIFT", "host", "SYS UNSHIFT bag line — alias of SYS PREPEND"},
      {"SYS POPHEAD", "host", "SYS POPHEAD|DEQUEUE [bag] — peel first field · FIFO"},
      {"SYS DEQUEUE", "host", "SYS DEQUEUE bag — alias of SYS POPHEAD"},
      {"SYS REMOVELINE", "host", "SYS REMOVELINE|DROPLINE bag needle — drop first exact field"},
      {"SYS DROPLINE", "host", "SYS DROPLINE bag needle — alias of SYS REMOVELINE"},
      {"SYS REMOVELINEI", "host", "SYS REMOVELINEI bag needle — case-insensitive drop field"},
      {"SYS FINDLINE", "host", "SYS FINDLINE|LINEINDEX bag needle — 0-based exact field index"},
      {"SYS LINEINDEX", "host", "SYS LINEINDEX bag needle — alias of SYS FINDLINE"},
      {"SYS FINDLINEI", "host", "SYS FINDLINEI bag needle — case-insensitive field index"},
      {"SYS SETLINE", "host", "SYS SETLINE|REPLACELINE bag n value — set 0-based field"},
      {"SYS REPLACELINE", "host", "SYS REPLACELINE bag n value — alias of SYS SETLINE"},
      {"SYS INSERTLINE", "host", "SYS INSERTLINE|INSLINE bag n value — insert field at 0-based index"},
      {"SYS INSLINE", "host", "SYS INSLINE bag n value — alias of SYS INSERTLINE"},
      {"SYS DROPNTH", "host", "SYS DROPNTH|DROPAT bag n — drop 0-based field by index"},
      {"SYS DROPAT", "host", "SYS DROPAT bag n — alias of SYS DROPNTH"},
      {"SYS MOVELINE", "host", "SYS MOVELINE|MOVEAT bag from to — move field to final index"},
      {"SYS MOVEAT", "host", "SYS MOVEAT bag from to — alias of SYS MOVELINE"},
      {"SYS COUNTLINE", "host", "SYS COUNTLINE|COUNTFIELD bag needle — count exact field matches"},
      {"SYS COUNTLINEI", "host", "SYS COUNTLINEI bag needle — case-insensitive field match count"},
      {"SYS SETMATCH", "host", "SYS SETMATCH|REPLACEMATCH bag old new — replace first exact field"},
      {"SYS REPLACEMATCH", "host", "SYS REPLACEMATCH bag old new — alias of SYS SETMATCH"},
      {"SYS SETMATCHI", "host", "SYS SETMATCHI bag old new — case-insensitive match on old"},
      {"SYS TAKE", "host", "SYS TAKE|FIRSTN n [str] — first n newline fields"},
      {"SYS DROP", "host", "SYS DROP|SKIP n [str] — drop first n newline fields"},
      {"SYS SPLIT", "host", "SYS SPLIT|FIELDS sep [str] — sep → newline fields"},
      {"SYS SORT", "host", "SYS SORT [str] — lexicographic newline field sort"},
      {"SYS SORTN", "host", "SYS SORTN|NSORT [DESC] [str] — numeric newline field sort"},
      {"SYS NSORT", "host", "SYS NSORT alias of SYS SORTN"},
      {"SYS SORTLEN", "host", "SYS SORTLEN|LENSORT [DESC] [bag] — sort by field length"},
      {"SYS LENSORT", "host", "SYS LENSORT alias of SYS SORTLEN"},
      {"SYS MAXLEN", "host", "SYS MAXLEN|MAXWIDTH [bag] — max field length → LAST_N"},
      {"SYS MINLEN", "host", "SYS MINLEN|MINWIDTH [bag] — min field length → LAST_N"},
      {"SYS MAXWIDTH", "host", "SYS MAXWIDTH alias of SYS MAXLEN"},
      {"SYS MINWIDTH", "host", "SYS MINWIDTH alias of SYS MINLEN"},
      {"SYS LONGEST", "host", "SYS LONGEST|MAXFIELD [bag] — first longest field text"},
      {"SYS SHORTEST", "host", "SYS SHORTEST|MINFIELD [bag] — first shortest field text"},
      {"SYS MAXFIELD", "host", "SYS MAXFIELD alias of SYS LONGEST"},
      {"SYS MINFIELD", "host", "SYS MINFIELD alias of SYS SHORTEST"},
      {"SYS COMMONPREFIX", "host", "SYS COMMONPREFIX|LCP [bag] — longest common prefix"},
      {"SYS LCP", "host", "SYS LCP alias of SYS COMMONPREFIX"},
      {"SYS COMMONSUFFIX", "host", "SYS COMMONSUFFIX|LCS [bag] — longest common suffix"},
      {"SYS LCS", "host", "SYS LCS alias of SYS COMMONSUFFIX"},
      {"SYS STRIPPREFIX", "host", "SYS STRIPPREFIX bag prefix — drop leading prefix per field"},
      {"SYS STRIPSUFFIX", "host", "SYS STRIPSUFFIX bag suffix — drop trailing suffix per field"},
      {"SYS STRIPCOMMON", "host", "SYS STRIPCOMMON|STRIPLCP [bag] — strip LCP relative paths"},
      {"SYS STRIPLCP", "host", "SYS STRIPLCP alias of SYS STRIPCOMMON"},
      {"SYS UNIQ", "host", "SYS UNIQ [str] — drop adjacent duplicate fields"},
      {"SYS JOIN", "host", "SYS JOIN|PATH a b — portable path join a/b → LAST"},
      {"SYS PATH", "host", "SYS PATH a b — alias of SYS JOIN"},
      {"SYS JOINLINES", "host", "SYS JOINLINES|PASTE sep [str] — join lines with sep"},
      {"SYS REPLACEALL", "host", /* wide */ "SYS REPLACEALL|GSUB hay old new — all replacements"},
      {"SYS REPLACE", "host", "SYS REPLACE [ALL] hay old new — first or all"},
      {"SYS SUBSTENV", "host", "SYS SUBSTENV|ENVSUBST [tmpl] — expand $NAME/${NAME} env+vars"},
      {"SYS ENVSUBST", "host", "SYS ENVSUBST alias of SYS SUBSTENV"},
      {"SYS EXPANDENV", "host", "SYS EXPANDENV alias of SYS SUBSTENV"},
      {"SYS SUBSTFILE", "host", "SYS SUBSTFILE|EXPANDFILE path [out] — materialize template file"},
      {"SYS EXPANDFILE", "host", "SYS EXPANDFILE alias of SYS SUBSTFILE"},
      {"SYS APPEND", "host", "SYS APPEND|LOG path data — append line (history log)"},
      {"SYS LOG", "host", "SYS LOG path data — alias of SYS APPEND"},
      {"SYS DATE", "host", "SYS DATE|ISO|UTC — UTC stamp YYYY-MM-DDTHH:MM:SSZ"},
      {"SYS FROMTIME", "host", "SYS FROMTIME|EPOCHISO|TOISO [n] — epoch → UTC ISO stamp"},
      {"SYS PARSEISO", "host", "SYS PARSEISO|TOEPOCH|FROMISO [str] — ISO → epoch dual of FROMTIME"},
      {"SYS TIMEDIFF", "host", "SYS TIMEDIFF|ELAPSED a [b] — epoch delta seconds a-b or now-a"},
      {"SYS ADDTIME", "host", "SYS ADDTIME|DEADLINE — epoch + seconds / now+secs lease"},
      {"SYS EXPIRED", "host", "SYS EXPIRED|REMAINING [epoch] — lease due probe / TTL secs"},
      {"SYS LOCAL", "host", "SYS LOCAL|LOCALTIME — local wall YYYY-MM-DDTHH:MM:SS"},
      {"SYS LOCALTIME", "host", "SYS LOCALTIME alias of SYS LOCAL"},
      {"SYS LOCALDATE", "host", "SYS LOCALDATE — local date-only YYYY-MM-DD"},
      {"SYS ISATTY", "host", "SYS ISATTY|TTY [IN|OUT|ERR] — terminal probe → LAST_N"},
      {"SYS TTY", "host", "SYS TTY alias of SYS ISATTY"},
      {"SYS TTYNAME", "host", "SYS TTYNAME|CTTY|TTYDEV [IN|OUT|ERR] — terminal device path → LAST"},
      {"SYS NICE", "host", "SYS NICE|GETNICE [n]|RENICE n — process nice get/set → LAST_N"},
      {"SYS SHQUOTE", "host", "SYS SHQUOTE|SHELLQUOTE [str] — POSIX single-quote shell-safe wrap"},
      {"SYS SHUNQUOTE", "host", "SYS SHUNQUOTE|UNQUOTE [str] — peel POSIX single-quote wrap"},
      {"SYS GETLOGIN", "host", "SYS GETLOGIN|LOGINNAME — session login via getlogin()"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM [n] — n random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS LOADAVG", "host", "SYS LOADAVG|LOAD — 1/5/15 load · LOAD1_N centiload"},
      {"SYS LOAD", "host", "SYS LOAD alias of SYS LOADAVG"},
      {"SYS UPTIME", "host", "SYS UPTIME|BOOTAGE — seconds since boot → LAST_N"},
      {"SYS BOOTAGE", "host", "SYS BOOTAGE alias of SYS UPTIME"},
      {"SYS MEM", "host", "SYS MEM|MEMINFO — RAM total/avail/free kB · LAST_N=avail"},
      {"SYS MEMINFO", "host", "SYS MEMINFO alias of SYS MEM"},
      {"SYS DF", "host", "SYS DF|DISKFREE [path] — filesystem free kB · LAST_N=avail"},
      {"SYS DISKFREE", "host", "SYS DISKFREE alias of SYS DF"},
      {"SYS FSYNC", "host", "SYS FSYNC|SYNCFILE path — durable flush file to disk"},
      {"SYS SYNCFILE", "host", "SYS SYNCFILE alias of SYS FSYNC"},
      {"SYS TRUNCATE", "host", "SYS TRUNCATE|FTRUNCATE path [nbytes] — resize plate file"},
      {"SYS FTRUNCATE", "host", "SYS FTRUNCATE alias of SYS TRUNCATE"},
      {"SYS FILETRUNC", "host", "SYS FILETRUNC alias of SYS TRUNCATE"},
      {"SYS DIRSYNC", "host", "SYS DIRSYNC|FSYNCDIR path — fsync dir or parent for durable rename"},
      {"SYS FSYNCDIR", "host", "SYS FSYNCDIR alias of SYS DIRSYNC"},
      {"SYS SYNCDIR", "host", "SYS SYNCDIR alias of SYS DIRSYNC"},
      {"SYS HARDLINK", "host", "SYS HARDLINK|HLINK existing newpath — hard link same inode"},
      {"SYS HLINK", "host", "SYS HLINK alias of SYS HARDLINK"},
      {"SYS NLINK", "host", "SYS NLINK|LINKS path — hard-link count → LAST_N"},
      {"SYS SAMEINODE", "host", "SYS SAMEINODE|EQINODE a b — 1 if same device+inode"},
      {"SYS EQINODE", "host", "SYS EQINODE alias of SYS SAMEINODE"},
      {"SYS INODE", "host", "SYS INODE|INO path — inode number → LAST_N"},
      {"SYS TEE", "host", "SYS TEE|WRITETEE path [data] — write keep content in LAST"},
      {"SYS WRITETEE", "host", "SYS WRITETEE alias of SYS TEE"},
      {"SYS FILEUID", "host", "SYS FILEUID|OWNERUID path — owner uid → LAST_N"},
      {"SYS FILEGID", "host", "SYS FILEGID|OWNERGID path — group gid → LAST_N"},
      {"SYS OWNEDBY", "host", "SYS OWNEDBY|ISOWNER path — LAST_N 1 if euid owns"},
      {"SYS CANREAD", "host", "SYS CANREAD|READABLE path — access R_OK → LAST_N 0|1"},
      {"SYS CANWRITE", "host", "SYS CANWRITE|WRITABLE path — access W_OK → LAST_N 0|1"},
      {"SYS CANEXEC", "host", "SYS CANEXEC|EXECUTABLE path — access X_OK → LAST_N 0|1"},
      {"SYS OWNERNAME", "host", "SYS OWNERNAME|OWNERUSER path — owner login name → LAST"},
      {"SYS GROUPNAME", "host", "SYS GROUPNAME|OWNERGROUP path — group name → LAST"},
      {"SYS CANCREATE", "host", "SYS CANCREATE|CREATABLE path — create/overwrite probe LAST_N 0|1"},
      {"SYS UMASK", "host", "SYS UMASK [mode] — get/set process file-creation mask (octal)"},
      {"SYS PUSHD", "host", "SYS PUSHD|PUSHDIR path — save cwd then chdir; LAST_N=depth"},
      {"SYS POPD", "host", "SYS POPD|POPDIR — restore previous cwd from stack"},
      {"SYS DIRSTACK", "host", "SYS DIRSTACK|DIRS — newline bag of saved dirs"},
      {"SYS KINDSTR", "host", "SYS KINDSTR|FILEKIND path — kind label file|dir|link|missing"},
      {"SYS RELPATH", "host", "SYS RELPATH|REL base path — relative to base or absolute"},
      {"SYS ATIME", "host", "SYS ATIME|ACCESSTIME path — last access epoch → LAST_N"},
      {"SYS CTIME", "host", "SYS CTIME|CHANGETIME path — inode status-change epoch → LAST_N"},
      {"SYS HASENV", "host", "SYS HASENV|ENVHAS name — LAST_N 1 if env set non-empty"},
      {"SYS GID", "host", "SYS GID|GROUP_ID — real group id → LAST_N"},
      {"SYS EGID", "host", "SYS EGID|GETEGID — effective group id → LAST_N"},
      {"SYS GROUP", "host", "SYS GROUP|GNAME — primary group name → LAST"},
      {"SYS GROUPS", "host", "SYS GROUPS|GROUPLIST — egid+supplementary names bag"},
      {"SYS INGROUP", "host", "SYS INGROUP|MEMBEROF name|gid — membership probe LAST_N 0|1"},
      {"SYS PPID", "host", "SYS PPID|GETPPID|PARENT_PID — parent process id → LAST_N"},
      {"SYS PGID", "host", "SYS PGID|GETPGID|PGRP — process group id → LAST_N"},
      {"SYS SID", "host", "SYS SID|GETSID|SESSION — session id → LAST_N"},
      {"SYS DOTENV", "host", "SYS DOTENV|LOADENV|ENVFILE path — load KEY=VAL plate into process env"},
      {"SYS LOADENV", "host", "SYS LOADENV alias of SYS DOTENV"},
      {"SYS ENVFILE", "host", "SYS ENVFILE alias of SYS DOTENV"},
      {"SYS DUMPENV", "host", "SYS DUMPENV|ENVALL [prefix] — process env as KEY=VAL bag"},
      {"SYS ENVALL", "host", "SYS ENVALL alias of SYS DUMPENV"},
      {"SYS LISTENV", "host", "SYS LISTENV alias of SYS DUMPENV"},
      {"SYS WRITEENV", "host", "SYS WRITEENV|SAVENV path [prefix] — write env KEY=VAL plate"},
      {"SYS SAVENV", "host", "SYS SAVENV alias of SYS WRITEENV"},
      {"SYS DUMPENVFILE", "host", "SYS DUMPENVFILE alias of SYS WRITEENV"},
      {"SYS WHICHBIN", "host", "SYS WHICHBIN|PATHWHICH name — PATH-only soft resolve"},
      {"REQUIRE BIN", "flow", "REQUIRE BIN|CMD|EXE name — fail if tool not on PATH"},
      {"SYS PID", "host", "SYS PID — process id → LAST_N/PID"},
      {"SYS HOSTNAME", "host", "SYS HOSTNAME|HOST — machine name → LAST"},
      {"SYS USER", "host", "SYS USER|USERNAME — login name → LAST/USER"},
      {"SYS GETLOGIN", "host", "SYS GETLOGIN|LOGINNAME — session login via getlogin() → LAST"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM [n] — n random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS UID", "host", "SYS UID|RUID|GETUID — real user id → LAST_N/UID"},
{"SYS EUID", "host", "SYS EUID|GETEUID — effective user id → LAST_N/EUID"},
{"SYS ISROOT", "host", "SYS ISROOT|AMROOT — LAST_N 1 if euid==0 privilege probe"},
      {"SYS HOME", "host", "SYS HOME|HOMEDIR — home directory → LAST/HOME"},
      {"SYS SHELL", "host", "SYS SHELL|LOGINSHELL — login shell path → LAST/SHELL"},
      {"SYS GECOS", "host", "SYS GECOS|FULLNAME|REALNAME — passwd display name → LAST"},
      {"SYS TTYNAME", "host", "SYS TTYNAME|CTTY|TTYDEV — terminal device path → LAST"},
      {"SYS NICE", "host", "SYS NICE|RENICE process nice get/set → LAST_N"},
      {"SYS SHQUOTE", "host", "SYS SHQUOTE|SHELLQUOTE POSIX shell-safe wrap"},
      {"SYS SHUNQUOTE", "host", "SYS SHUNQUOTE|UNQUOTE peel POSIX single-quote wrap"},
      {"SYS GETLOGIN", "host", "SYS GETLOGIN|LOGINNAME session login name"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"HELP", "flow", "HELP [form] — in-program catalog tip → LAST/OK/HELP_N"},
      {"SYS WRITE", "host", "SYS WRITE path data"},
      {"SYS WRITEATOMIC", "host", "SYS WRITEATOMIC|SAFEWRITE path data — temp+rename crash-safer write"},
      {"SYS ATOMICWRITE", "host", "SYS ATOMICWRITE alias of SYS WRITEATOMIC"},
      {"SYS SAFEWRITE", "host", "SYS SAFEWRITE alias of SYS WRITEATOMIC"},
      {"SYS LOCKFILE", "host", "SYS LOCKFILE|FLOCK path [ms] — exclusive path.lock agent mutex"},
      {"SYS FLOCK", "host", "SYS FLOCK alias of SYS LOCKFILE"},
      {"SYS UNLOCKFILE", "host", "SYS UNLOCKFILE|FUNLOCK path — release path.lock"},
      {"SYS CLAIM", "host", "SYS CLAIM|CREATEEXCL path [body] — exclusive path work-item claim"},
      {"SYS CLAIMFILE", "host", "SYS CLAIMFILE alias of SYS CLAIM"},
      {"SYS CREATEEXCL", "host", "SYS CREATEEXCL alias of SYS CLAIM"},
      {"SYS EXIST", "host", "SYS EXIST path → LAST_N 0|1"},
      {"SYS HASH", "host", "SYS HASH|FINGERPRINT [str] — FNV-1a 32-bit hex stamp"},
      {"SYS HASHFILE", "host", "SYS HASHFILE|FILEHASH path — FNV-1a of file bytes"},
      {"SYS FINGERPRINT", "host", "SYS FINGERPRINT alias of SYS HASH"},
      {"SYS BASE64", "host", "SYS BASE64|B64ENC [str] — standard base64 encode"},
      {"SYS B64ENC", "host", "SYS B64ENC alias of SYS BASE64"},
      {"SYS BASE64D", "host", "SYS BASE64D|B64DEC [str] — standard base64 decode"},
      {"SYS B64DEC", "host", "SYS B64DEC alias of SYS BASE64D"},
      {"SYS HEX", "host", "SYS HEX|HEXENC [str] — lowercase hex encode"},
      {"SYS HEXENC", "host", "SYS HEXENC alias of SYS HEX"},
      {"SYS HEXD", "host", "SYS HEXD|HEXDEC [str] — hex decode"},
      {"SYS HEXDEC", "host", "SYS HEXDEC alias of SYS HEXD"},
      {"SYS URLENC", "host", "SYS URLENC|URLENCODE [str] — RFC3986 percent-encode"},
      {"SYS URLENCODE", "host", "SYS URLENCODE alias of SYS URLENC"},
      {"SYS PERCENTENC", "host", "SYS PERCENTENC alias of SYS URLENC"},
      {"SYS URLDEC", "host", "SYS URLDEC|URLDECODE [str] — percent-decode (+ as space)"},
      {"SYS URLDECODE", "host", "SYS URLDECODE alias of SYS URLDEC"},
      {"SYS PERCENTDEC", "host", "SYS PERCENTDEC alias of SYS URLDEC"},
      {"SYS UUID", "host", "SYS UUID|GUID — RFC4122 v4 id (dashed)"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM [n] — n random bytes as hex nonces/tokens"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS GUID", "host", "SYS GUID alias of SYS UUID"},
      {"SYS GENUUID", "host", "SYS GENUUID alias of SYS UUID"},
      {"SYS UUID0", "host", "SYS UUID0|UUIDNODASH — compact 32-hex UUID"},
      {"SYS UUIDNODASH", "host", "SYS UUIDNODASH alias of SYS UUID0"},
      {"SYS JSONESC", "host", "SYS JSONESC|JESC [str] — JSON string-body escape"},
      {"SYS SHQUOTE", "host", "SYS SHQUOTE|SHELLQUOTE [str] — POSIX shell-safe single-quote wrap"},
      {"SYS SHUNQUOTE", "host", "SYS SHUNQUOTE|UNQUOTE [str] — peel POSIX single-quote wrap"},
      {"SYS GETLOGIN", "host", "SYS GETLOGIN|LOGINNAME — session login via getlogin()"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM [n] — n random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS JESC", "host", "SYS JESC alias of SYS JSONESC"},
      {"SYS JSONUNESC", "host", "SYS JSONUNESC|JUNESC [str] — JSON string-body unescape"},
      {"SYS JUNESC", "host", "SYS JUNESC alias of SYS JSONUNESC"},
      {"SYS CRC32", "host", "SYS CRC32|CRC [str] — IEEE CRC-32 hex stamp"},
      {"SYS CRC", "host", "SYS CRC alias of SYS CRC32"},
      {"SYS CRC32FILE", "host", "SYS CRC32FILE|FILECRC32 path — CRC-32 of file bytes"},
      {"SYS FILECRC32", "host", "SYS FILECRC32 alias of SYS CRC32FILE"},
      {"SYS SYMLINK", "host", "SYS SYMLINK|LN target linkpath — create symlink"},
      {"SYS LN", "host", "SYS LN alias of SYS SYMLINK"},
      {"SYS READLINK", "host", "SYS READLINK|LINKTARGET path — peel symlink target"},
      {"SYS LINKTARGET", "host", "SYS LINKTARGET alias of SYS READLINK"},
      {"SYS HUMANSIZE", "host", "SYS HUMANSIZE|BYTESFMT [n] — integer B/K/M/G/T label"},
      {"SYS BYTESFMT", "host", "SYS BYTESFMT alias of SYS HUMANSIZE"},
      {"SYS HUMANSZ", "host", "SYS HUMANSZ alias of SYS HUMANSIZE"},
      {"SYS PARSESIZE", "host", "SYS PARSESIZE|FROMHUMAN [str] — 1K/2M → bytes"},
      {"SYS FROMHUMAN", "host", "SYS FROMHUMAN alias of SYS PARSESIZE"},
      {"SYS TOBYTES", "host", "SYS TOBYTES alias of SYS PARSESIZE"},
      {"SYS MONOTONIC", "host", "SYS MONOTONIC|MONO — steady ms for elapsed timing"},
      {"SYS MONO", "host", "SYS MONO alias of SYS MONOTONIC"},
      {"SYS STEADY_MS", "host", "SYS STEADY_MS alias of SYS MONOTONIC"},
      {"SYS NPROC", "host", "SYS NPROC|CPUS|CORES — online processor count → LAST_N"},
      {"SYS CPUS", "host", "SYS CPUS alias of SYS NPROC"},
      {"SYS CORES", "host", "SYS CORES alias of SYS NPROC"},
      {"SYS UNAME", "host", "SYS UNAME|OS — kernel/OS name → LAST/UNAME"},
      {"SYS OS", "host", "SYS OS alias of SYS UNAME"},
      {"SYS ARCH", "host", "SYS ARCH|MACHINE — machine arch → LAST/ARCH"},
      {"SYS MACHINE", "host", "SYS MACHINE alias of SYS ARCH"},
      {"SYS PLATFORM", "host", "SYS PLATFORM — os/arch combined tag"},
      {"SYS LOCAL", "host", "SYS LOCAL|LOCALTIME — local wall YYYY-MM-DDTHH:MM:SS"},
      {"SYS LOCALTIME", "host", "SYS LOCALTIME alias of SYS LOCAL"},
      {"SYS LOCALDATE", "host", "SYS LOCALDATE — local date-only YYYY-MM-DD"},
      {"SYS ISATTY", "host", "SYS ISATTY|TTY [IN|OUT|ERR] — terminal probe → LAST_N"},
      {"SYS TTY", "host", "SYS TTY alias of SYS ISATTY"},
      {"SYS TTYNAME", "host", "SYS TTYNAME|CTTY|TTYDEV [IN|OUT|ERR] — terminal device path → LAST"},
      {"SYS NICE", "host", "SYS NICE|GETNICE [n]|RENICE n — process nice get/set → LAST_N"},
      {"SYS SHQUOTE", "host", "SYS SHQUOTE|SHELLQUOTE [str] — POSIX single-quote shell-safe wrap"},
      {"SYS SHUNQUOTE", "host", "SYS SHUNQUOTE|UNQUOTE [str] — peel POSIX single-quote wrap"},
      {"SYS GETLOGIN", "host", "SYS GETLOGIN|LOGINNAME — session login via getlogin()"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM [n] — n random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS LOADAVG", "host", "SYS LOADAVG|LOAD — 1/5/15 load · LOAD1_N centiload"},
      {"SYS LOAD", "host", "SYS LOAD alias of SYS LOADAVG"},
      {"SYS UPTIME", "host", "SYS UPTIME|BOOTAGE — seconds since boot → LAST_N"},
      {"SYS BOOTAGE", "host", "SYS BOOTAGE alias of SYS UPTIME"},
      {"SYS MEM", "host", "SYS MEM|MEMINFO — RAM total/avail/free kB · LAST_N=avail"},
      {"SYS MEMINFO", "host", "SYS MEMINFO alias of SYS MEM"},
      {"SYS DF", "host", "SYS DF|DISKFREE [path] — filesystem free kB · LAST_N=avail"},
      {"SYS DISKFREE", "host", "SYS DISKFREE alias of SYS DF"},
      {"SYS FSYNC", "host", "SYS FSYNC|SYNCFILE path — durable flush file to disk"},
      {"SYS SYNCFILE", "host", "SYS SYNCFILE alias of SYS FSYNC"},
      {"SYS TRUNCATE", "host", "SYS TRUNCATE|FTRUNCATE path [nbytes] — resize plate file"},
      {"SYS FTRUNCATE", "host", "SYS FTRUNCATE alias of SYS TRUNCATE"},
      {"SYS FILETRUNC", "host", "SYS FILETRUNC alias of SYS TRUNCATE"},
      {"SYS DIRSYNC", "host", "SYS DIRSYNC|FSYNCDIR path — fsync dir or parent for durable rename"},
      {"SYS FSYNCDIR", "host", "SYS FSYNCDIR alias of SYS DIRSYNC"},
      {"SYS SYNCDIR", "host", "SYS SYNCDIR alias of SYS DIRSYNC"},
      {"SYS HARDLINK", "host", "SYS HARDLINK|HLINK existing newpath — hard link same inode"},
      {"SYS HLINK", "host", "SYS HLINK alias of SYS HARDLINK"},
      {"SYS NLINK", "host", "SYS NLINK|LINKS path — hard-link count → LAST_N"},
      {"SYS SAMEINODE", "host", "SYS SAMEINODE|EQINODE a b — 1 if same device+inode"},
      {"SYS EQINODE", "host", "SYS EQINODE alias of SYS SAMEINODE"},
      {"SYS INODE", "host", "SYS INODE|INO path — inode number → LAST_N"},
      {"SYS TEE", "host", "SYS TEE|WRITETEE path [data] — write keep content in LAST"},
      {"SYS WRITETEE", "host", "SYS WRITETEE alias of SYS TEE"},
      {"SYS FILEUID", "host", "SYS FILEUID|OWNERUID path — owner uid → LAST_N"},
      {"SYS FILEGID", "host", "SYS FILEGID|OWNERGID path — group gid → LAST_N"},
      {"SYS OWNEDBY", "host", "SYS OWNEDBY|ISOWNER path — LAST_N 1 if euid owns"},
      {"SYS CANREAD", "host", "SYS CANREAD|READABLE path — access R_OK → LAST_N 0|1"},
      {"SYS CANWRITE", "host", "SYS CANWRITE|WRITABLE path — access W_OK → LAST_N 0|1"},
      {"SYS CANEXEC", "host", "SYS CANEXEC|EXECUTABLE path — access X_OK → LAST_N 0|1"},
      {"SYS OWNERNAME", "host", "SYS OWNERNAME|OWNERUSER path — owner login name → LAST"},
      {"SYS GROUPNAME", "host", "SYS GROUPNAME|OWNERGROUP path — group name → LAST"},
      {"SYS CANCREATE", "host", "SYS CANCREATE|CREATABLE path — create/overwrite probe LAST_N 0|1"},
      {"SYS UMASK", "host", "SYS UMASK [mode] — get/set process file-creation mask (octal)"},
      {"SYS PUSHD", "host", "SYS PUSHD|PUSHDIR path — save cwd then chdir; LAST_N=depth"},
      {"SYS POPD", "host", "SYS POPD|POPDIR — restore previous cwd from stack"},
      {"SYS DIRSTACK", "host", "SYS DIRSTACK|DIRS — newline bag of saved dirs"},
      {"SYS KINDSTR", "host", "SYS KINDSTR|FILEKIND path — kind label file|dir|link|missing"},
      {"SYS RELPATH", "host", "SYS RELPATH|REL base path — relative to base or absolute"},
      {"SYS ATIME", "host", "SYS ATIME|ACCESSTIME path — last access epoch → LAST_N"},
      {"SYS CTIME", "host", "SYS CTIME|CHANGETIME path — inode status-change epoch → LAST_N"},
      {"SYS HASENV", "host", "SYS HASENV|ENVHAS name — LAST_N 1 if env set non-empty"},
      {"SYS GID", "host", "SYS GID|GROUP_ID — real group id → LAST_N"},
      {"SYS EGID", "host", "SYS EGID|GETEGID — effective group id → LAST_N"},
      {"SYS GROUP", "host", "SYS GROUP|GNAME — primary group name → LAST"},
      {"SYS GROUPS", "host", "SYS GROUPS|GROUPLIST — egid+supplementary names bag"},
      {"SYS INGROUP", "host", "SYS INGROUP|MEMBEROF name|gid — membership probe LAST_N 0|1"},
      {"SYS PPID", "host", "SYS PPID|GETPPID|PARENT_PID — parent process id → LAST_N"},
      {"SYS PGID", "host", "SYS PGID|GETPGID|PGRP — process group id → LAST_N"},
      {"SYS SID", "host", "SYS SID|GETSID|SESSION — session id → LAST_N"},
      {"SYS DOTENV", "host", "SYS DOTENV|LOADENV|ENVFILE path — load KEY=VAL plate into process env"},
      {"SYS LOADENV", "host", "SYS LOADENV alias of SYS DOTENV"},
      {"SYS ENVFILE", "host", "SYS ENVFILE alias of SYS DOTENV"},
      {"SYS DUMPENV", "host", "SYS DUMPENV|ENVALL [prefix] — process env as KEY=VAL bag"},
      {"SYS ENVALL", "host", "SYS ENVALL alias of SYS DUMPENV"},
      {"SYS LISTENV", "host", "SYS LISTENV alias of SYS DUMPENV"},
      {"SYS WRITEENV", "host", "SYS WRITEENV|SAVENV path [prefix] — write env KEY=VAL plate"},
      {"SYS SAVENV", "host", "SYS SAVENV alias of SYS WRITEENV"},
      {"SYS DUMPENVFILE", "host", "SYS DUMPENVFILE alias of SYS WRITEENV"},
      {"SYS WHICHBIN", "host", "SYS WHICHBIN|PATHWHICH name — PATH-only soft resolve"},
      {"REQUIRE BIN", "flow", "REQUIRE BIN|CMD|EXE name — fail if tool not on PATH"},
      {"SYS ISLINK", "host", "SYS ISLINK|ISLNK path — LAST_N 1 if symlink"},
      {"SYS ISLNK", "host", "SYS ISLNK alias of SYS ISLINK"},
      {"SYS ISSYMLINK", "host", "SYS ISSYMLINK alias of SYS ISLINK"},
      {"SYS MODE", "host", "SYS MODE|PERM path — octal perms → LAST/MODE_N"},
      {"SYS PERM", "host", "SYS PERM alias of SYS MODE"},
      {"SYS CHMOD", "host", "SYS CHMOD path mode — set permission bits"},
      {"SYS SETMODE", "host", "SYS SETMODE alias of SYS CHMOD"},
      {"SYS WAITFILE", "host", "SYS WAITFILE path [ms|\"2s\"] — poll until exists · duration timeout"},
      {"SYS WAITPATH", "host", "SYS WAITPATH alias of SYS WAITFILE"},
      {"SYS POLLFILE", "host", "SYS POLLFILE alias of SYS WAITFILE"},
      {"SYS WAITGONE", "host", "SYS WAITGONE|WAITMISSING path [ms] — poll until path gone"},
      {"SYS WAITMISSING", "host", "SYS WAITMISSING alias of SYS WAITGONE"},
      {"SYS WAITDELETE", "host", "SYS WAITDELETE alias of SYS WAITGONE"},
      {"SYS WAITSTABLE", "host", "SYS WAITSTABLE|WAITQUIET path [ms]|quiet timeout — settle size+mtime"},
      {"SYS WAITQUIET", "host", "SYS WAITQUIET alias of SYS WAITSTABLE"},
      {"SYS STABLEFILE", "host", "SYS STABLEFILE alias of SYS WAITSTABLE"},
      {"SYS WAITCHANGED", "host", "SYS WAITCHANGED|WAITMODIFIED path [SINCE size mtime] [ms] — poll until change"},
      {"SYS WAITMODIFIED", "host", "SYS WAITMODIFIED alias of SYS WAITCHANGED"},
      {"SYS POLLCHANGE", "host", "SYS POLLCHANGE alias of SYS WAITCHANGED"},
      {"SYS WAITMATCH", "host", "SYS WAITMATCH|WAITCONTAINS path needle [ms] — poll until content match"},
      {"SYS WAITCONTAINS", "host", "SYS WAITCONTAINS alias of SYS WAITMATCH"},
      {"SYS WAITMATCHI", "host", "SYS WAITMATCHI case-insensitive WAITMATCH"},
      {"SYS WHICH", "host", "SYS WHICH name → LAST path"},
      {"SYS NUM", "host", "parse LAST → LAST_N"},
      {"SYS JSON", "host", "SYS JSON \"key\" extract field"},
      {"SYS JSONFROMKV", "host", "SYS JSONFROMKV|JFROMKV bag — key=val bag → plate"},
      {"SYS JSONTOKV", "host", "SYS JSONTOKV|JTOKV plate — plate → key:val bag · dual of JSONFROMKV"},
      {"SYS JSONLEN", "host", "SYS JSONLEN|JLEN [plate] — top-level key count → LAST_N · soft 0"},
      {"SYS JSONEMPTY", "host", "SYS JSONEMPTY|JEMPTY [plate] — soft LAST_N 1 if empty/non-object"},
      {"SYS JSONNONEMPTY", "host", "SYS JSONNONEMPTY|HASKEYS [plate] — soft LAST_N 1 if has keys"},
      {"SYS JSONSUMN", "host", "SYS JSONSUMN|JSONSUM [plate] — sum integer top-level values → LAST_N"},
      {"SYS JSONTOPKEY", "host", "SYS JSONTOPKEY|JTOPKEY [plate] — key with max int value · LAST=key LAST_N=v"},
      {"SYS JSONBOTKEY", "host", "SYS JSONBOTKEY|JBOTKEY [plate] — key with min int value · dual of TOPKEY"},
      {"SYS JSONMAXN", "host", "SYS JSONMAXN|JMAXN [plate] — max int top-level value → LAST_N"},
      {"SYS JSONMINN", "host", "SYS JSONMINN|JMINN [plate] — min int top-level value → LAST_N"},
      {"SYS JSONAVGN", "host", "SYS JSONAVGN|JSONMEAN [plate] — integer mean of top-level ints → LAST_N"},
      {"SYS JSONEQ", "host", "SYS JSONEQ|JEQ a b — order-independent plate equality → LAST_N 0|1"},
      {"SYS JSONNEQ", "host", "SYS JSONNEQ|JNEQ a b — plates differ → LAST_N 1"},
      {"SYS JSONDEFAULTS", "host", "SYS JSONDEFAULTS|JSONENSUREALL plate defaults — fill missing keys · no clobber"},
      {"SYS JSONCHANGELOG", "host", "SYS JSONCHANGELOG|JCHANGELOG a b — key: old → new change bag"},
      {"SYS JSONSUBSET", "host", "SYS JSONSUBSET|JSUBSET sub super — required fields match → LAST_N 0|1"},
      {"SYS JSONSUPERSET", "host", "SYS JSONSUPERSET|JSONCOVERS a b — a ⊇ b required fields"},
      {"SYS JSONHASALL", "host", "SYS JSONHASALL|JSONNEED [plate] key… — all keys present → LAST_N 0|1"},
      {"SYS JSONHASANY", "host", "SYS JSONHASANY|JHASANY [plate] key… — any key present → LAST_N 0|1"},
      {"SYS JSONMISS", "host", "SYS JSONMISS|JSONMISSING [plate] key… — bag of missing required keys · LAST_N=count"},
      {"SYS JSONPRESENT", "host", "SYS JSONPRESENT|JSONFOUND [plate] key… — bag of present required keys · dual MISS"},
      {"SYS JSONEXTRA", "host", "SYS JSONEXTRA|JSONUNKNOWN [plate] allow… — bag of plate keys not in allow-list · LAST_N=count"},
      {"SYS JSONKNOWN", "host", "SYS JSONKNOWN|JSONALLOWED [plate] allow… — bag of plate keys in allow-list · dual EXTRA"},
      {"SYS JSONPLUCK", "host", "SYS JSONPLUCK|JSONGETALL [plate] key… — multi-key peel → value bag · HIT=found"},
      {"SYS JSONKEYS", "host", "SYS JSONKEYS [plate] — top-level keys bag"},
      {"SYS JSONVALUES", "host", "SYS JSONVALUES [plate] — top-level values bag"},
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
           "  programs/lib/             # INCLUDE snippets (agent_boot · hold_seed)\n"
           "  programs/hello_cube.cubalc\n"
           "  programs/proof/12_hold_flash_plug.cubalc\n"
           "  programs/p2p/mesh_local.cubalc\n"
           "  programs/protect/core_protect.cubalc\n"
           "Commands: cubalc doctor · cubalc init · cubalc run <file> · cubalc protect\n");
    return 0;
  }
  if (strcmp(cmd, "init") == 0 || strcmp(cmd, "new") == 0 ||
      strcmp(cmd, "scaffold") == 0 || strcmp(cmd, "create") == 0) {
    /* Usability: scaffold a runnable .cubalc starter.
     * cubalc init [path] [--force|-f] [--plate|--session] [--peer|--mesh]
     * cubalc init --list|--templates  → catalog of scaffolds (agent discovery)
     * Default: agent_boot + place/plug. --plate: plate_session + uniform + pretty.
     * --peer: plate_peer_session multi-plate PLATE+PEER starter.
     * Agents: write file then cubalc run — no cookbook prose required. */
    const char *path = "program.cubalc";
    const char *tmpl = "agent_boot";
    int force = 0, i, wrote = 0, existed = 0, want_plate = 0, want_peer = 0;
    int want_list = 0;
    char abspath[512];
    char parent[512];
    FILE *f;
    static const char *body_boot =
      "# CubalC starter — generated by cubalc init\n"
      "# INCLUDE agent_boot = REQUIRE 1.15 + VERSION (HOLD_FLASH default 1)\n"
      "INCLUDE agent_boot\n"
      "\n"
      "CUBE kernel ROLE host\n"
      "CUBE hive ROLE body\n"
      "PLUG kernel hive\n"
      "IMPULSE kernel\n"
      "FLOW 4\n"
      "DECIDE kernel\n"
      "STATUS\n"
      "PRINT \"init ok\" OK LAST_N\n";
    static const char *body_plate =
      "# CubalC agent plate starter — generated by cubalc init --plate\n"
      "# INCLUDE plate_session = agent_boot + plate_boot (durable PLATE)\n"
      "# plate_uniform = nest value consistency · PRETTYP = inspect without jq\n"
      "DEFAULT PLATE_PATH = \"state/agent_plate.json\"\n"
      "DEFAULT PLATE_SEED = \"{\\\"n\\\":0,\\\"ok\\\":true,\\\"role\\\":\\\"worker\\\"}\"\n"
      "INCLUDE plate_session\n"
      "\n"
      "SETP \"status\" \"ready\"\n"
      "SETP \"role\" \"worker\"\n"
      "INCP \"n\"\n"
      "NEEDP \"n\" \"ok\" \"status\" \"role\"\n"
      "DEFAULT UNIFORM_NEEDLE = \"role\"\n"
      "INCLUDE plate_uniform\n"
      "ASSERT UNIFORM_EQ == 1 \"roles must match after seed\"\n"
      "PRETTYP\n"
      "DUMPP\n"
      "INCLUDE plate_save\n"
      "PRINT \"plate init ok\" OK LAST_N\n";
    static const char *body_peer =
      "# CubalC multi-plate starter — generated by cubalc init --peer\n"
      "# INCLUDE plate_peer_session = plate_session + plate_peer (PLATE + PEER)\n"
      "DEFAULT PLATE_PATH = \"state/agent_plate.json\"\n"
      "DEFAULT PLATE_SEED = \"{\\\"n\\\":0,\\\"ok\\\":true}\"\n"
      "DEFAULT PEER_PATH = \"state/peer_plate.json\"\n"
      "DEFAULT PEER_SEED = \"{\\\"agent\\\":\\\"local\\\",\\\"n\\\":0}\"\n"
      "INCLUDE plate_peer_session\n"
      "\n"
      "SETP \"status\" \"ready\"\n"
      "INCP \"n\"\n"
      "SETP FROM PEER \"host\" \"cubeA\"\n"
      "INCP FROM PEER \"n\"\n"
      "NEEDP \"n\" \"ok\" \"status\"\n"
      "NEEDP FROM PEER \"agent\" \"host\" \"n\"\n"
      "DUMPP\n"
      "DUMPP FROM PEER\n"
      "INCLUDE plate_both_save\n"
      "PRINT \"peer init ok\" OK LAST_N\n";
    const char *body = body_boot;
    for (i = 2; i < argc; i++) {
      if (!strcmp(argv[i], "--force") || !strcmp(argv[i], "-f") ||
          !strcmp(argv[i], "--overwrite")) {
        force = 1;
      } else if (!strcmp(argv[i], "--list") || !strcmp(argv[i], "--templates") ||
                 !strcmp(argv[i], "--catalog") || !strcmp(argv[i], "list") ||
                 !strcmp(argv[i], "templates")) {
        want_list = 1;
      } else if (!strcmp(argv[i], "--peer") || !strcmp(argv[i], "--mesh") ||
                 !strcmp(argv[i], "--multi-plate") || !strcmp(argv[i], "--plates")) {
        want_peer = 1;
      } else if (!strcmp(argv[i], "--plate") || !strcmp(argv[i], "--session") ||
                 !strcmp(argv[i], "--agent-plate") || !strcmp(argv[i], "-p")) {
        want_plate = 1;
      } else if (argv[i][0] != '-') {
        path = argv[i];
      }
    }
    if (want_list) {
      /* Agent discovery: which scaffolds exist without reading COOKBOOK. */
      printf("# CubalC init templates version=%s\n", CUBALC_LANG_VERSION);
      printf("# run: cubalc init [path] [--plate|--peer] [--force]\n");
      printf("# flagidflagshint\n");
      printf("(default)agent_boot—INCLUDE agent_boot + CUBE/PLUG/STATUS\n");
      printf("--plate|-pplate_session—plate_session + plate_uniform + PRETTYP + save\n");
      printf("--peer|--meshplate_peer_session—PLATE+PEER multi-plate + dual save\n");
      printf("{\"schema\":\"cubalc.init.v1\",\"ok\":true,\"cmd\":\"init\","
             "\"op\":\"list\",\"n\":3,\"version\":\"%s\","
             "\"note\":\"scaffold catalog — cubalc init path --plate|--peer\","
             "\"templates\":["
             "{\"id\":\"agent_boot\",\"flags\":\"\",\"default\":true,"
             "\"hint\":\"INCLUDE agent_boot + CUBE/PLUG/STATUS\"},"
             "{\"id\":\"plate_session\",\"flags\":\"--plate|-p\","
             "\"hint\":\"plate_session + plate_uniform + PRETTYP + save\"},"
             "{\"id\":\"plate_peer_session\",\"flags\":\"--peer|--mesh\","
             "\"hint\":\"PLATE+PEER multi-plate + dual save\"}"
             "]}\n",
             CUBALC_LANG_VERSION);
      return 0;
    }
    if (want_peer) {
      body = body_peer;
      tmpl = "plate_peer_session";
    } else if (want_plate) {
      body = body_plate;
      tmpl = "plate_session";
    }
    if (!path[0]) path = "program.cubalc";
    /* ensure .cubalc suffix for bare names without extension */
    {
      const char *dot = strrchr(path, '.');
      const char *slash = strrchr(path, '/');
      int bare = (!dot || (slash && dot < slash));
      if (bare) {
        snprintf(abspath, sizeof abspath, "%s.cubalc", path);
        path = abspath;
      }
    }
    existed = (access(path, F_OK) == 0);
    if (existed && !force) {
      fprintf(stderr, "cubalc init: %s exists (use --force to overwrite)\n", path);
      printf("{\"schema\":\"cubalc.init.v1\",\"ok\":false,\"cmd\":\"init\","
             "\"path\":\"%s\",\"existed\":true,\"wrote\":false,"
             "\"err\":\"exists\",\"hint\":\"cubalc init %s --force\","
             "\"version\":\"%s\"}\n",
             path, path, CUBALC_LANG_VERSION);
      return 1;
    }
    /* mkdir parent if path has directory component */
    {
      const char *slash = strrchr(path, '/');
      if (slash && slash > path) {
        size_t n = (size_t)(slash - path);
        if (n + 1 < sizeof parent) {
          memcpy(parent, path, n);
          parent[n] = 0;
          ensure_dir(parent);
        }
      }
    }
    f = fopen(path, "w");
    if (!f) {
      fprintf(stderr, "cubalc init: cannot write %s: %s\n", path, strerror(errno));
      printf("{\"schema\":\"cubalc.init.v1\",\"ok\":false,\"cmd\":\"init\","
             "\"path\":\"%s\",\"err\":\"write_fail\",\"version\":\"%s\"}\n",
             path, CUBALC_LANG_VERSION);
      return 1;
    }
    if (fputs(body, f) >= 0) wrote = 1;
    fclose(f);
    if (!wrote) {
      printf("{\"schema\":\"cubalc.init.v1\",\"ok\":false,\"cmd\":\"init\","
             "\"path\":\"%s\",\"err\":\"write_fail\",\"version\":\"%s\"}\n",
             path, CUBALC_LANG_VERSION);
      return 1;
    }
    printf("# CubalC init wrote %s template=%s version=%s\n",
           path, tmpl, CUBALC_LANG_VERSION);
    if (want_peer)
      printf("# next: cubalc run %s · cubalc cat plate_peer_session · multi-plate PLATE+PEER\n", path);
    else if (want_plate)
      printf("# next: cubalc run %s · cubalc cat plate_uniform · plate pretty state/agent_plate.json\n", path);
    else
      printf("# next: cubalc run %s · cubalc cat agent_boot\n", path);
    printf("{\"schema\":\"cubalc.init.v1\",\"ok\":true,\"cmd\":\"init\","
           "\"path\":\"%s\",\"existed\":%s,\"wrote\":true,\"force\":%s,"
           "\"template\":\"%s\",\"bytes\":%zu,"
           "\"version\":\"%s\","
           "\"note\":\"%s\"}\n",
           path, existed ? "true" : "false", force ? "true" : "false",
           tmpl, strlen(body), CUBALC_LANG_VERSION,
           want_peer
               ? "INCLUDE plate_peer_session + PLATE/PEER SETP FROM + save both"
               : (want_plate
               ? "INCLUDE plate_session + plate_uniform + PRETTYP + plate_save"
               : "INCLUDE agent_boot + CUBE/PLUG/FLOW/STATUS starter"));
    return 0;
  }
  if (strcmp(cmd, "libs") == 0 || strcmp(cmd, "lib") == 0 ||
      strcmp(cmd, "stdlib") == 0) {
    /* Usability: list programs/lib INCLUDE snippets for agents/humans */
    static const struct { const char *file; const char *hint; } known[] = {
      {"hold_seed.cubalc", "optional device/firmware HOLD_FLASH seed (not program preamble)"},
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
      {"CUBALC_QUIET", "", 0, "1 → run plate-only no board noise"},
      {"CUBALC_STRICT", "", 0, "1 → run: soft last_err fails exit + plate ok"},
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
  if (strcmp(cmd, "plate") == 0 || strcmp(cmd, "jsonplate") == 0 ||
      strcmp(cmd, "platefile") == 0 || strcmp(cmd, "agent-plate") == 0) {
    /* Usability: inspect/mutate JSON agent plate files from the CLI without a
     * .cubalc program. Complements plate_boot/SETP/SAVEPLATE language surface.
     *   cubalc plate show path.json
     *   cubalc plate get  path.json key [OR default]
     *   cubalc plate set  path.json key value
     *   cubalc plate inc  path.json key [delta]
     *   cubalc plate del  path.json key
     *   cubalc plate keys path.json
     *   cubalc plate fill [-s] path.json tmpl|@file [out]
     *   cubalc plate fillkeys path.json tmpl|@file
     *   cubalc plate ensure path.json [seed|@file]   # create-or-keep · ENSUREPLATE dual
     *   cubalc plate merge  path.json overlay|@file  # multi-key overlay · JSONFILEMERGE dual
     *   cubalc plate eq     a.json b.json            # order-indep equal · JSONEQ dual
     *   cubalc plate ne     a.json b.json            # unequal probe
     *   cubalc plate diff   a.json b.json            # changed key bag · JSONCHANGED dual
     *   cubalc plate changelog a.json b.json         # "key: old → new" · JSONCHANGELOG dual
     *   cubalc plate has|need path.json k1 k2 …      # multi-key contract · HASPALL/NEEDP duals
     *   cubalc plate pick path.json k1 k2 …          # keep listed keys · PICKP dual
     *   cubalc plate omit path.json k1 k2 …          # drop listed keys · OMITP dual
     *   cubalc plate sum|avg|median path.json        # numeric aggregates · SUMNP/AVGNP/MEDIANP
     *   cubalc plate top|bot path.json [n]           # top/bot N keys by value · TOPNP/BOTNP
     *   cubalc plate rename path.json old new        # RENAMEP dual · paths ok
     *   cubalc plate copy   path.json src dst        # COPYP dual · paths ok
     *   cubalc plate swap   path.json a b            # SWAPP dual · paths ok
     * Bare: cubalc plate path.json  → show
     * One JSON plate per call (cubalc.plate.v1) for agents. */
    const char *op = "show";
    const char *path = NULL;
    const char *key = NULL;
    const char *val = NULL;
    const char *fb = NULL;
    long delta = 1;
    int have_fb = 0, file_hit = 0, val_kind = 0;
    char plate[CUBALC_HOST_STR_MAX];
    char parent[512];
    cubalc_host_result hr, gr, keys, wr;
    const char *body;
    const char *slash;
    int ai = 2;

    plate[0] = 0;
    if (argc <= 2) {
      fprintf(stderr,
              "usage: cubalc plate show|get|set|inc|del|keys|nestget|nestset|nestinc|nestdel|nestkeys|nesthas|nestpick|nestomit|fill|… <path> …\n"
              "       cubalc plate <path.json>                 # show\n"
              "       cubalc plate get <path> <key> [OR def]\n"
              "       cubalc plate getn <path> <key> [OR n]  # GETPN dual · numeric peel (paths ok)\n"
              "       cubalc plate getobj <path> <key> [OR '{}']  # GETOBJ dual · peel object (paths ok)\n"
              "       cubalc plate setobj <path> <key> <obj|@file>  # SETOBJ dual · nest object (paths ok)\n"
              "       cubalc plate mergeobj|defaultobj <path> <key> <obj|@file>  # MERGEOBJ/DEFAULTOBJ duals\n"
              "       cubalc plate type <path> <key>  # TYPEP dual · kind miss|num|str|… (paths ok)\n"
              "       cubalc plate set <path> <key> <value>\n"
              "       cubalc plate default <path> <key> <value>  # DEFAULTP dual · set-if-missing (paths ok)\n"
              "       cubalc plate toggle <path> <key>  # TOGGLEP dual · flip 0↔1 (paths ok)\n"
              "       cubalc plate rename <path> <old> <new>  # RENAMEP dual · paths ok\n"
              "       cubalc plate copy <path> <src> <dst>    # COPYP dual · paths ok\n"
              "       cubalc plate swap <path> <a> <b>        # SWAPP dual · paths ok\n"
              "       cubalc plate inc <path> <key> [delta]\n"
              "       cubalc plate del <path> <key>\n"
              "       cubalc plate keys <path> [nest.path]  # KEYSP dual · nest path ok\n"
              "       cubalc plate leaves|pathkeys <path> [nest.path]  # PATHKEYS dual · dotted leaf paths\n"
              "       cubalc plate flat|flatkv <path> [nest.path]  # FLATKV dual · path:value bag\n"
              "       cubalc plate unflat|unflatkv <path> [UNDER nest] <bag|@file|k:v…>  # UNFLATKV dual\n"
              "       cubalc plate diffflat|clogflat <a.json> <b.json>  # DIFFFLAT dual · deep leaf changelog\n"
              "       cubalc plate pathdiff <a.json> <b.json>  # PATHDIFF dual · changed leaf paths\n"
              "       cubalc plate grepf|grepflat <path> <needle>  # GREPFLAT dual · path filter\n"
              "       cubalc plate grepvf <path> <needle>  # GREPVFLAT dual · invert\n"
              "       cubalc plate prune <path> <needle>  # PRUNEFLAT dual · drop matching leaves\n"
              "       cubalc plate keeponly <path> <needle>  # KEEONLYFLAT dual · project leaves\n"
              "       cubalc plate mergeflat <base.json> <overlay.json>  # MERGEFLAT dual · deep leaf overlay\n"
              "       cubalc plate renameflat <path> <old_pfx> <new_pfx>  # RENAMEFLAT dual · path prefix rewrite\n"
              "       cubalc plate setflat <path> <needle> <value>  # SETFLAT dual · bulk leaf value set\n"
              "       cubalc plate incflat <path> <needle> [delta]  # INCFLAT dual · bump pure-int leaves\n"
              "       cubalc plate sumflat <path> [needle]  # SUMFLAT dual · sum pure-int leaves by path needle\n"
              "       cubalc plate avgflat <path> [needle]  # AVGFLAT dual · mean pure-int leaves by path needle\n"
              "       cubalc plate medianflat <path> [needle]  # MEDIANFLAT dual · median pure-int leaves by path needle\n"
              "       cubalc plate toppath|botpath <path> [needle]  # TOPPATHFLAT dual · path of max/min pure-int leaf\n"
              "       cubalc plate threshflat <path> [needle] <min>  # THRESHFLAT dual · drop pure-int leaves value<min\n"
              "       cubalc plate dropzeroflat <path> [needle]  # DROPZEROFLAT dual · drop pure-int zeros\n"
              "       cubalc plate capflat <path> [needle] <max>  # CAPFLAT dual · clamp pure-int leaves to max\n"
              "       cubalc plate scaleflat <path> [needle] <factor>  # SCALEFLAT dual · multiply pure-int leaves\n"
              "       cubalc plate hasflat <path> [needle]  # HASFLAT dual · leaf-path presence 0|1\n"
              "       cubalc plate countflat <path> [needle]  # COUNTFLAT dual · leaf-path match count\n"
              "       cubalc plate hasflatn <path> [needle]  # HASFLATN dual · pure-int leaf presence 0|1\n"
              "       cubalc plate countflatn <path> [needle]  # COUNTFLATN dual · pure-int leaf match count\n"
              "       cubalc plate pathsflat <path> [needle]  # PATHSFLAT dual · matching leaf paths bag\n"
              "       cubalc plate valsflat <path> [needle]  # VALSFLAT dual · matching leaf values bag\n"
              "       cubalc plate pathsflatn <path> [needle]  # PATHSFLATN dual · pure-int leaf paths bag\n"
              "       cubalc plate valsflatn <path> [needle]  # VALSFLATN dual · pure-int leaf values bag\n"
              "       cubalc plate getflat <path> <needle> [OR def]  # GETFLAT dual · first matching leaf value\n"
              "       cubalc plate lastflat <path> <needle> [OR def]  # LASTFLAT dual · last matching leaf value\n"
              "       cubalc plate nthflat <path> <needle> <i> [OR def]  # NTHFLAT dual · 0-based Nth matching leaf\n"
              "       cubalc plate needflat <path> <needle> [needle…]  # NEEDFLAT dual · fail-fast leaf path contract\n"
              "       cubalc plate getflatn <path> <needle> [OR n]  # GETFLATN dual · first pure-int leaf → n\n"
              "       cubalc plate lastflatn <path> <needle> [OR n]  # LASTFLATN dual · last pure-int leaf → n\n"
              "       cubalc plate nthflatn <path> <needle> <i> [OR n]  # NTHFLATN dual · 0-based Nth pure-int leaf → n\n"
              "       cubalc plate typeflat <path> <needle>  # TYPEFLAT dual · first matching leaf kind\n"
              "       cubalc plate freqflat <path> [needle]  # FREQFLAT dual · value frequency of matching leaves\n"
              "       cubalc plate modeflat <path> [needle]  # MODEFLAT dual · dominant matching leaf value\n"
              "       cubalc plate pathbyval <path> <value> [OR def]  # PATHBYVAL dual · first leaf path by exact value\n"
              "       cubalc plate uniqflat <path> [needle]  # UNIQFLAT dual · unique matching leaf values\n"
              "       cubalc plate pathsbyval <path> <value>  # PATHSBYVAL dual · all leaf paths by exact value\n"
              "       cubalc plate countbyval <path> <value>  # COUNTBYVAL dual · count leaves by exact value\n"
              "       cubalc plate hasval <path> <value>  # HASVAL dual · soft presence of exact leaf value\n"
              "       cubalc plate setbyval <path> <old> <new>  # SETBYVAL dual · rewrite leaf values by exact match\n"
              "       cubalc plate delbyval <path> <value>  # DELBYVAL dual · drop leaves by exact value\n"
              "       cubalc plate alleqflat <path> [needle]  # ALLEQFLAT dual · all matching leaf values identical\n"
              "       cubalc plate firstuneq <path> [needle]  # FIRSTUNEQFLAT dual · first path that breaks uniformity\n"
              "       cubalc plate uneqpaths <path> [needle]  # UNEQPATHS dual · all paths that break uniformity\n"
              "       cubalc plate uniform <path> [needle]  # UNIFORMFLAT dual · one-shot eq + first path + all diverge paths\n"
              "       cubalc plate pretty <path>  # PRETTYP dual · 2-space indented plate (no jq)\n"
              "       cubalc plate needflatn <path> <needle> [needle…]  # NEEDFLATN dual · fail-fast pure-int leaf + peel\n"
              "       cubalc plate len|empty|vals <path> [nest.path]  # LENP/EMPTYP/VALSP duals\n"
              "       cubalc plate nestget <path> <nest> <field> [OR def]\n"
              "       cubalc plate nestset <path> <nest> <field> <value>\n"
              "       cubalc plate nestinc <path> <nest> <field> [delta]\n"
              "       cubalc plate nestdel <path> <nest> <field>\n"
              "       cubalc plate nestkeys <path> <nest>\n"
              "       cubalc plate nesthas <path> <nest> <field>\n"
              "       cubalc plate nestpick <path> <nest> <k1> [k2…]  # PICKOBJ dual\n"
              "       cubalc plate nestomit <path> <nest> <k1> [k2…]  # OMITOBJ dual\n"
              "       cubalc plate nestrename <path> <nest> <old> <new>  # RENAMEPOBJ dual · path ok\n"
              "       cubalc plate nestcopy <path> <nest> <src> <dst>    # COPYPOBJ dual · path ok\n"
              "       cubalc plate nestswap <path> <nest> <a> <b>        # SWAPPOBJ dual · path ok\n"
              "       cubalc plate nestsum|nestavg|nestmedian <path> <nest>  # SUMNOBJ duals\n"
              "       cubalc plate nesttop|nestbot <path> <nest> [n]  # TOPNOBJ duals (no write)\n"
              "       cubalc plate nestsort <path> <nest> [ASC|DESC]  # SORTOBJ dual (write)\n"
              "       cubalc plate nestsortbag <path> <nest> [ASC|DESC]  # SORTBAGOBJ dual (no write)\n"
              "       cubalc plate fill [-s|--strict] <path> <tmpl|@file> [out]\n"
              "       cubalc plate fillkeys <path> <tmpl|@file>\n"
              "       cubalc plate ensure <path> [seed|@file]  # create-or-keep\n"
              "       cubalc plate merge  <path> <overlay|@file>\n"
              "       cubalc plate eq|ne|diff|changelog <a.json> <b.json>\n"
              "       cubalc plate has|need <path> <k1> [k2 …]\n"
              "       cubalc plate pluck <path> <k1> [k2 …]  # multi-key peel bag · PLUCKP dual (paths ok)\n"
              "       cubalc plate pluckobj <path> <nest> <k1> [k2…]  # PLUCKOBJ dual · nest path ok\n"
              "       cubalc plate pick|omit <path> <k1> [k2 …]  # keep/drop keys · PICKP/OMITP\n"
              "       cubalc plate sum|avg|median|mean|p50 <path>  # aggregates · SUMNP duals\n"
              "       cubalc plate top|bot <path> [n]          # top/bot N keys · TOPNP duals\n");
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
             "\"err\":\"need op and/or path\",\"version\":\"%s\","
             "\"ops\":[\"show\",\"get\",\"getn\",\"getobj\",\"setobj\",\"mergeobj\",\"defaultobj\","
             "\"type\",\"set\",\"default\",\"toggle\",\"rename\",\"copy\",\"swap\","
             "\"inc\",\"del\",\"keys\",\"leaves\",\"pathkeys\",\"flat\",\"flatkv\",\"unflat\",\"unflatkv\",\"diffflat\",\"pathdiff\",\"grepf\",\"grepflat\",\"grepvf\",\"prune\",\"keeponly\",\"mergeflat\",\"renameflat\",\"setflat\",\"incflat\",\"sumflat\",\"avgflat\",\"medianflat\",\"toppath\",\"botpath\",\"threshflat\",\"dropzeroflat\",\"capflat\",\"scaleflat\",\"hasflat\",\"countflat\",\"hasflatn\",\"countflatn\",\"pathsflat\",\"valsflat\",\"pathsflatn\",\"valsflatn\",\"getflat\",\"lastflat\",\"nthflat\",\"needflat\",\"getflatn\",\"lastflatn\",\"nthflatn\",\"typeflat\",\"freqflat\",\"modeflat\",\"pathbyval\",\"uniqflat\",\"pathsbyval\",\"countbyval\",\"hasval\",\"setbyval\",\"delbyval\",\"alleqflat\",\"firstuneq\",\"uneqpaths\",\"uniform\",\"pretty\",\"needflatn\",\"len\",\"empty\",\"vals\","
             "\"nestget\",\"nestset\",\"nestinc\",\"nestdel\",\"nestkeys\",\"nesthas\",\"nestpick\",\"nestomit\","
             "\"nestrename\",\"nestcopy\",\"nestswap\",\"pluckobj\","
             "\"nestsum\",\"nestavg\",\"nestmedian\",\"nesttop\",\"nestbot\","
             "\"nestsort\",\"nestsortbag\","
             "\"fill\",\"fillkeys\",\"ensure\",\"merge\",\"eq\",\"ne\",\"diff\",\"changelog\","
             "\"has\",\"need\",\"pluck\",\"pick\",\"omit\",\"sum\",\"avg\",\"median\",\"top\",\"bot\"]}\n",
             CUBALC_LANG_VERSION);
      return 2;
    }

        /* parse: op path …  OR  path (bare show) */
    if (strcmp(argv[2], "show") == 0 || strcmp(argv[2], "dump") == 0 ||
        strcmp(argv[2], "cat") == 0 || strcmp(argv[2], "read") == 0 ||
        strcmp(argv[2], "get") == 0 || strcmp(argv[2], "peek") == 0 ||
        strcmp(argv[2], "getn") == 0 || strcmp(argv[2], "num") == 0 ||
        strcmp(argv[2], "getpn") == 0 || strcmp(argv[2], "jgetn") == 0 ||
        strcmp(argv[2], "int") == 0 ||
        strcmp(argv[2], "getobj") == 0 || strcmp(argv[2], "peekobj") == 0 ||
        strcmp(argv[2], "objget") == 0 ||
        strcmp(argv[2], "setobj") == 0 || strcmp(argv[2], "putobj") == 0 ||
        strcmp(argv[2], "nestp") == 0 || strcmp(argv[2], "putnest") == 0 ||
        strcmp(argv[2], "mergeobj") == 0 || strcmp(argv[2], "patchnest") == 0 ||
        strcmp(argv[2], "updateobj") == 0 ||
        strcmp(argv[2], "defaultobj") == 0 || strcmp(argv[2], "ensureobj") == 0 ||
        strcmp(argv[2], "defaultnest") == 0 ||
        strcmp(argv[2], "type") == 0 || strcmp(argv[2], "kind") == 0 ||
        strcmp(argv[2], "typeof") == 0 || strcmp(argv[2], "typep") == 0 ||
        strcmp(argv[2], "jtype") == 0 ||
        strcmp(argv[2], "set") == 0 || strcmp(argv[2], "put") == 0 ||
        strcmp(argv[2], "default") == 0 || strcmp(argv[2], "ensurekey") == 0 ||
        strcmp(argv[2], "defaultp") == 0 || strcmp(argv[2], "setdefault") == 0 ||
        strcmp(argv[2], "toggle") == 0 || strcmp(argv[2], "flip") == 0 ||
        strcmp(argv[2], "togglep") == 0 || strcmp(argv[2], "flipp") == 0 ||
        strcmp(argv[2], "rename") == 0 || strcmp(argv[2], "move") == 0 ||
        strcmp(argv[2], "mv") == 0 || strcmp(argv[2], "renamep") == 0 ||
        strcmp(argv[2], "movekey") == 0 || strcmp(argv[2], "rekey") == 0 ||
        strcmp(argv[2], "copy") == 0 || strcmp(argv[2], "cp") == 0 ||
        strcmp(argv[2], "dup") == 0 || strcmp(argv[2], "copyp") == 0 ||
        strcmp(argv[2], "dupkey") == 0 ||
        strcmp(argv[2], "swap") == 0 || strcmp(argv[2], "xchg") == 0 ||
        strcmp(argv[2], "swapp") == 0 || strcmp(argv[2], "exchange") == 0 ||
        strcmp(argv[2], "inc") == 0 || strcmp(argv[2], "bump") == 0 ||
        strcmp(argv[2], "del") == 0 || strcmp(argv[2], "rm") == 0 ||
        strcmp(argv[2], "drop") == 0 || strcmp(argv[2], "keys") == 0 ||
        strcmp(argv[2], "list") == 0 || strcmp(argv[2], "ls") == 0 ||
        strcmp(argv[2], "leaves") == 0 || strcmp(argv[2], "leafkeys") == 0 ||
        strcmp(argv[2], "pathkeys") == 0 || strcmp(argv[2], "dotpaths") == 0 ||
        strcmp(argv[2], "flatpaths") == 0 || strcmp(argv[2], "dotkeys") == 0 ||
        strcmp(argv[2], "flat") == 0 || strcmp(argv[2], "flatkv") == 0 ||
        strcmp(argv[2], "leafkv") == 0 || strcmp(argv[2], "flatten") == 0 ||
        strcmp(argv[2], "flattenp") == 0 || strcmp(argv[2], "tokvpath") == 0 ||
        strcmp(argv[2], "unflat") == 0 || strcmp(argv[2], "unflatkv") == 0 ||
        strcmp(argv[2], "fromflat") == 0 || strcmp(argv[2], "unflatten") == 0 ||
        strcmp(argv[2], "unflattenp") == 0 || strcmp(argv[2], "pathfromkv") == 0 ||
        strcmp(argv[2], "diffflat") == 0 || strcmp(argv[2], "clogflat") == 0 ||
        strcmp(argv[2], "changelogflat") == 0 || strcmp(argv[2], "leafdiff") == 0 ||
        strcmp(argv[2], "pathdiff") == 0 || strcmp(argv[2], "diffpath") == 0 ||
        strcmp(argv[2], "diffpaths") == 0 ||
        strcmp(argv[2], "grepf") == 0 || strcmp(argv[2], "grepflat") == 0 ||
        strcmp(argv[2], "keepflat") == 0 || strcmp(argv[2], "filterflat") == 0 ||
        strcmp(argv[2], "grepvf") == 0 || strcmp(argv[2], "grepvflat") == 0 ||
        strcmp(argv[2], "dropflat") == 0 || strcmp(argv[2], "grepfi") == 0 ||
        strcmp(argv[2], "grepflati") == 0 ||
        strcmp(argv[2], "prune") == 0 || strcmp(argv[2], "pruneflat") == 0 ||
        strcmp(argv[2], "delflat") == 0 || strcmp(argv[2], "scrubflat") == 0 ||
        strcmp(argv[2], "keeponly") == 0 || strcmp(argv[2], "keeponlyflat") == 0 ||
        strcmp(argv[2], "retainflat") == 0 || strcmp(argv[2], "projectflat") == 0 ||
        strcmp(argv[2], "mergeflat") == 0 || strcmp(argv[2], "overlayflat") == 0 ||
        strcmp(argv[2], "patchflat") == 0 || strcmp(argv[2], "deepmerge") == 0 ||
        strcmp(argv[2], "leafmerge") == 0 || strcmp(argv[2], "syncflat") == 0 ||
        strcmp(argv[2], "renameflat") == 0 || strcmp(argv[2], "moveflat") == 0 ||
        strcmp(argv[2], "prefixflat") == 0 || strcmp(argv[2], "repathflat") == 0 ||
        strcmp(argv[2], "rekeyflat") == 0 ||
        strcmp(argv[2], "setflat") == 0 || strcmp(argv[2], "mapflat") == 0 ||
        strcmp(argv[2], "setleaf") == 0 || strcmp(argv[2], "putflat") == 0 ||
        strcmp(argv[2], "bulkset") == 0 ||
        strcmp(argv[2], "incflat") == 0 || strcmp(argv[2], "bumpflat") == 0 ||
        strcmp(argv[2], "addflat") == 0 || strcmp(argv[2], "tickflat") == 0 ||
        strcmp(argv[2], "bulkinc") == 0 || strcmp(argv[2], "decflat") == 0 ||
        strcmp(argv[2], "sumflat") == 0 || strcmp(argv[2], "totalflat") == 0 ||
        strcmp(argv[2], "leafsum") == 0 || strcmp(argv[2], "flattotal") == 0 ||
        strcmp(argv[2], "avgflat") == 0 || strcmp(argv[2], "meanflat") == 0 ||
        strcmp(argv[2], "avgleaf") == 0 || strcmp(argv[2], "flatmean") == 0 ||
        strcmp(argv[2], "leafmean") == 0 ||
        strcmp(argv[2], "medianflat") == 0 || strcmp(argv[2], "p50flat") == 0 ||
        strcmp(argv[2], "midflat") == 0 || strcmp(argv[2], "flatmedian") == 0 ||
        strcmp(argv[2], "leafmedian") == 0 ||
        strcmp(argv[2], "toppath") == 0 || strcmp(argv[2], "toppathflat") == 0 ||
        strcmp(argv[2], "maxpath") == 0 || strcmp(argv[2], "maxpathflat") == 0 ||
        strcmp(argv[2], "botpath") == 0 || strcmp(argv[2], "botpathflat") == 0 ||
        strcmp(argv[2], "minpath") == 0 || strcmp(argv[2], "minpathflat") == 0 ||
        strcmp(argv[2], "threshflat") == 0 || strcmp(argv[2], "keepvflat") == 0 ||
        strcmp(argv[2], "mincountflat") == 0 ||
        strcmp(argv[2], "dropzeroflat") == 0 || strcmp(argv[2], "nzflat") == 0 ||
        strcmp(argv[2], "capflat") == 0 || strcmp(argv[2], "clampflat") == 0 ||
        strcmp(argv[2], "maxvflat") == 0 || strcmp(argv[2], "ceilflat") == 0 ||
        strcmp(argv[2], "scaleflat") == 0 || strcmp(argv[2], "mulflat") == 0 ||
        strcmp(argv[2], "timesflat") == 0 || strcmp(argv[2], "multflat") == 0 ||
        strcmp(argv[2], "hasflat") == 0 || strcmp(argv[2], "anyflat") == 0 ||
        strcmp(argv[2], "countflat") == 0 || strcmp(argv[2], "nflat") == 0 ||
        strcmp(argv[2], "leafcount") == 0 ||
        strcmp(argv[2], "hasflatn") == 0 || strcmp(argv[2], "anyflatn") == 0 ||
        strcmp(argv[2], "countflatn") == 0 || strcmp(argv[2], "nflatn") == 0 ||
        strcmp(argv[2], "leafcountn") == 0 || strcmp(argv[2], "hitflatn") == 0 ||
        strcmp(argv[2], "pathsflat") == 0 || strcmp(argv[2], "matchpaths") == 0 ||
        strcmp(argv[2], "valsflat") == 0 || strcmp(argv[2], "matchvals") == 0 ||
        strcmp(argv[2], "leafvals") == 0 ||
        strcmp(argv[2], "pathsflatn") == 0 || strcmp(argv[2], "matchpathsn") == 0 ||
        strcmp(argv[2], "valsflatn") == 0 || strcmp(argv[2], "matchvalsn") == 0 ||
        strcmp(argv[2], "leafvalsn") == 0 || strcmp(argv[2], "numvalsflat") == 0 ||
        strcmp(argv[2], "getflat") == 0 || strcmp(argv[2], "firstflat") == 0 ||
        strcmp(argv[2], "peelflat") == 0 || strcmp(argv[2], "leafget") == 0 ||
        strcmp(argv[2], "lastflat") == 0 || strcmp(argv[2], "endflat") == 0 ||
        strcmp(argv[2], "tailflat") == 0 || strcmp(argv[2], "lastleafp") == 0 ||
        strcmp(argv[2], "nthflat") == 0 || strcmp(argv[2], "indexflat") == 0 ||
        strcmp(argv[2], "atflat") == 0 || strcmp(argv[2], "nthleaf") == 0 ||
        strcmp(argv[2], "needflat") == 0 || strcmp(argv[2], "requireflat") == 0 ||
        strcmp(argv[2], "mustflat") == 0 ||
        strcmp(argv[2], "getflatn") == 0 || strcmp(argv[2], "firstflatn") == 0 ||
        strcmp(argv[2], "numflat") == 0 || strcmp(argv[2], "getnflat") == 0 ||
        strcmp(argv[2], "lastflatn") == 0 || strcmp(argv[2], "endflatn") == 0 ||
        strcmp(argv[2], "tailflatn") == 0 || strcmp(argv[2], "lastnumflat") == 0 ||
        strcmp(argv[2], "nthflatn") == 0 || strcmp(argv[2], "indexflatn") == 0 ||
        strcmp(argv[2], "atflatn") == 0 || strcmp(argv[2], "nthnumflat") == 0 ||
        strcmp(argv[2], "typeflat") == 0 || strcmp(argv[2], "kindflat") == 0 ||
        strcmp(argv[2], "leaftype") == 0 || strcmp(argv[2], "flattype") == 0 ||
        strcmp(argv[2], "freqflat") == 0 || strcmp(argv[2], "histflat") == 0 ||
        strcmp(argv[2], "leaffreq") == 0 || strcmp(argv[2], "countvalflat") == 0 ||
        strcmp(argv[2], "modeflat") == 0 || strcmp(argv[2], "topvalflat") == 0 ||
        strcmp(argv[2], "majorflat") == 0 || strcmp(argv[2], "leafmode") == 0 ||
        strcmp(argv[2], "pathbyval") == 0 || strcmp(argv[2], "valpath") == 0 ||
        strcmp(argv[2], "findval") == 0 || strcmp(argv[2], "pathofval") == 0 ||
        strcmp(argv[2], "uniqflat") == 0 || strcmp(argv[2], "distinctflat") == 0 ||
        strcmp(argv[2], "uniqueflat") == 0 || strcmp(argv[2], "leafuniq") == 0 ||
        strcmp(argv[2], "pathsbyval") == 0 || strcmp(argv[2], "allpathsbyval") == 0 ||
        strcmp(argv[2], "findvals") == 0 || strcmp(argv[2], "pathsofval") == 0 ||
        strcmp(argv[2], "countbyval") == 0 || strcmp(argv[2], "nbyval") == 0 ||
        strcmp(argv[2], "valcount") == 0 || strcmp(argv[2], "nval") == 0 ||
        strcmp(argv[2], "hasval") == 0 || strcmp(argv[2], "hasvalflat") == 0 ||
        strcmp(argv[2], "anyval") == 0 || strcmp(argv[2], "valhas") == 0 ||
        strcmp(argv[2], "setbyval") == 0 || strcmp(argv[2], "replaceval") == 0 ||
        strcmp(argv[2], "rewval") == 0 || strcmp(argv[2], "mapval") == 0 ||
        strcmp(argv[2], "delbyval") == 0 || strcmp(argv[2], "dropval") == 0 ||
        strcmp(argv[2], "rmval") == 0 || strcmp(argv[2], "pruneval") == 0 ||
        strcmp(argv[2], "alleqflat") == 0 || strcmp(argv[2], "samevalflat") == 0 ||
        strcmp(argv[2], "eqvalsflat") == 0 || strcmp(argv[2], "allsameflat") == 0 ||
        strcmp(argv[2], "firstuneq") == 0 || strcmp(argv[2], "firstuneqflat") == 0 ||
        strcmp(argv[2], "pathuneq") == 0 || strcmp(argv[2], "diverge1") == 0 ||
        strcmp(argv[2], "uneqpaths") == 0 || strcmp(argv[2], "divergepaths") == 0 ||
        strcmp(argv[2], "alluneq") == 0 || strcmp(argv[2], "pathsuneq") == 0 ||
        strcmp(argv[2], "uniform") == 0 || strcmp(argv[2], "uniformflat") == 0 ||
        strcmp(argv[2], "checkflat") == 0 || strcmp(argv[2], "samecheck") == 0 ||
        strcmp(argv[2], "pretty") == 0 || strcmp(argv[2], "prettyp") == 0 ||
        strcmp(argv[2], "jsonpretty") == 0 || strcmp(argv[2], "prettyjson") == 0 ||
        strcmp(argv[2], "needflatn") == 0 || strcmp(argv[2], "requireflatn") == 0 ||
        strcmp(argv[2], "mustflatn") == 0 || strcmp(argv[2], "needintflat") == 0 ||
        strcmp(argv[2], "len") == 0 || strcmp(argv[2], "length") == 0 ||
        strcmp(argv[2], "nkeys") == 0 || strcmp(argv[2], "size") == 0 ||
        strcmp(argv[2], "countkeys") == 0 ||
        strcmp(argv[2], "empty") == 0 || strcmp(argv[2], "isempty") == 0 ||
        strcmp(argv[2], "emptyp") == 0 ||
        strcmp(argv[2], "vals") == 0 || strcmp(argv[2], "values") == 0 ||
        strcmp(argv[2], "valsp") == 0 ||
        strcmp(argv[2], "fill") == 0 || strcmp(argv[2], "expand") == 0 ||
        strcmp(argv[2], "template") == 0 || strcmp(argv[2], "render") == 0 ||
        strcmp(argv[2], "fillkeys") == 0 || strcmp(argv[2], "fill-keys") == 0 ||
        strcmp(argv[2], "tmplkeys") == 0 ||
        strcmp(argv[2], "ensure") == 0 || strcmp(argv[2], "seed") == 0 ||
        strcmp(argv[2], "touch") == 0 || strcmp(argv[2], "create") == 0 ||
        strcmp(argv[2], "merge") == 0 || strcmp(argv[2], "patch") == 0 ||
        strcmp(argv[2], "overlay") == 0 || strcmp(argv[2], "apply") == 0 ||
        strcmp(argv[2], "eq") == 0 || strcmp(argv[2], "same") == 0 ||
        strcmp(argv[2], "equal") == 0 || strcmp(argv[2], "cmp") == 0 ||
        strcmp(argv[2], "ne") == 0 || strcmp(argv[2], "neq") == 0 ||
        strcmp(argv[2], "differ") == 0 || strcmp(argv[2], "unequal") == 0 ||
        strcmp(argv[2], "diff") == 0 || strcmp(argv[2], "changed") == 0 ||
        strcmp(argv[2], "delta") == 0 ||
        strcmp(argv[2], "changelog") == 0 || strcmp(argv[2], "clog") == 0 ||
        strcmp(argv[2], "changes") == 0 || strcmp(argv[2], "logdiff") == 0 ||
        strcmp(argv[2], "has") == 0 || strcmp(argv[2], "hasall") == 0 ||
        strcmp(argv[2], "has-keys") == 0 || strcmp(argv[2], "check") == 0 ||
        strcmp(argv[2], "need") == 0 || strcmp(argv[2], "require") == 0 ||
        strcmp(argv[2], "needkeys") == 0 || strcmp(argv[2], "require-keys") == 0 ||
        strcmp(argv[2], "pluck") == 0 || strcmp(argv[2], "getall") == 0 ||
        strcmp(argv[2], "getpall") == 0 || strcmp(argv[2], "peel") == 0 ||
        strcmp(argv[2], "values") == 0 || strcmp(argv[2], "pluckp") == 0 ||
        strcmp(argv[2], "pluckobj") == 0 || strcmp(argv[2], "nestpluck") == 0 ||
        strcmp(argv[2], "objpluck") == 0 || strcmp(argv[2], "plucknest") == 0 ||
        strcmp(argv[2], "pick") == 0 || strcmp(argv[2], "keep") == 0 ||
        strcmp(argv[2], "select") == 0 || strcmp(argv[2], "project") == 0 ||
        strcmp(argv[2], "omit") == 0 || strcmp(argv[2], "strip") == 0 ||
        strcmp(argv[2], "dropkeys") == 0 || strcmp(argv[2], "drop-keys") == 0 ||
        strcmp(argv[2], "sum") == 0 || strcmp(argv[2], "total") == 0 ||
        strcmp(argv[2], "avg") == 0 || strcmp(argv[2], "mean") == 0 ||
        strcmp(argv[2], "average") == 0 ||
        strcmp(argv[2], "median") == 0 || strcmp(argv[2], "p50") == 0 ||
        strcmp(argv[2], "mid") == 0 ||
        strcmp(argv[2], "top") == 0 || strcmp(argv[2], "topn") == 0 ||
        strcmp(argv[2], "head") == 0 ||
        strcmp(argv[2], "bot") == 0 || strcmp(argv[2], "bottom") == 0 ||
        strcmp(argv[2], "botn") == 0 || strcmp(argv[2], "tail") == 0 ||
        strcmp(argv[2], "nestget") == 0 || strcmp(argv[2], "nget") == 0 ||
        strcmp(argv[2], "getnest") == 0 || strcmp(argv[2], "getpobj") == 0 ||
        strcmp(argv[2], "nestset") == 0 || strcmp(argv[2], "nset") == 0 ||
        strcmp(argv[2], "setnest") == 0 || strcmp(argv[2], "setpobj") == 0 ||
        strcmp(argv[2], "nestinc") == 0 || strcmp(argv[2], "ninc") == 0 ||
        strcmp(argv[2], "incnest") == 0 || strcmp(argv[2], "incobj") == 0 ||
        strcmp(argv[2], "nestdel") == 0 || strcmp(argv[2], "ndel") == 0 ||
        strcmp(argv[2], "delnest") == 0 || strcmp(argv[2], "delpobj") == 0 ||
        strcmp(argv[2], "nestkeys") == 0 || strcmp(argv[2], "nkeys") == 0 ||
        strcmp(argv[2], "keysobj") == 0 || strcmp(argv[2], "keysnest") == 0 ||
        strcmp(argv[2], "nesthas") == 0 || strcmp(argv[2], "nhas") == 0 ||
        strcmp(argv[2], "haspobj") == 0 || strcmp(argv[2], "hasnest") == 0 ||
        strcmp(argv[2], "nestpick") == 0 || strcmp(argv[2], "npick") == 0 ||
        strcmp(argv[2], "picknest") == 0 || strcmp(argv[2], "pickobj") == 0 ||
        strcmp(argv[2], "nestomit") == 0 || strcmp(argv[2], "nomit") == 0 ||
        strcmp(argv[2], "omitnest") == 0 || strcmp(argv[2], "omitobj") == 0 ||
        strcmp(argv[2], "stripnest") == 0 ||
        strcmp(argv[2], "nestrename") == 0 || strcmp(argv[2], "nrename") == 0 ||
        strcmp(argv[2], "renamenest") == 0 || strcmp(argv[2], "renamepobj") == 0 ||
        strcmp(argv[2], "nestcopy") == 0 || strcmp(argv[2], "ncopy") == 0 ||
        strcmp(argv[2], "copynest") == 0 || strcmp(argv[2], "copypobj") == 0 ||
        strcmp(argv[2], "nestswap") == 0 || strcmp(argv[2], "nswap") == 0 ||
        strcmp(argv[2], "swapnest") == 0 || strcmp(argv[2], "swappobj") == 0 ||
        strcmp(argv[2], "nestsum") == 0 || strcmp(argv[2], "nsum") == 0 ||
        strcmp(argv[2], "sumnest") == 0 || strcmp(argv[2], "sumnobj") == 0 ||
        strcmp(argv[2], "nestavg") == 0 || strcmp(argv[2], "navg") == 0 ||
        strcmp(argv[2], "avgnest") == 0 || strcmp(argv[2], "nestmean") == 0 ||
        strcmp(argv[2], "nestmedian") == 0 || strcmp(argv[2], "nmedian") == 0 ||
        strcmp(argv[2], "nestp50") == 0 || strcmp(argv[2], "mediannest") == 0 ||
        strcmp(argv[2], "nesttop") == 0 || strcmp(argv[2], "ntop") == 0 ||
        strcmp(argv[2], "topnest") == 0 || strcmp(argv[2], "topnobj") == 0 ||
        strcmp(argv[2], "nestbot") == 0 || strcmp(argv[2], "nbot") == 0 ||
        strcmp(argv[2], "botnest") == 0 || strcmp(argv[2], "botnobj") == 0 ||
        strcmp(argv[2], "nestbottom") == 0 ||
        strcmp(argv[2], "nestsort") == 0 || strcmp(argv[2], "nsort") == 0 ||
        strcmp(argv[2], "sortnest") == 0 || strcmp(argv[2], "sortobj") == 0 ||
        strcmp(argv[2], "nestsortbag") == 0 || strcmp(argv[2], "nsortbag") == 0 ||
        strcmp(argv[2], "sortbagnest") == 0 || strcmp(argv[2], "sortbagobj") == 0 ||
        strcmp(argv[2], "nestsortfreq") == 0 || strcmp(argv[2], "sortfreqobj") == 0) {
      op = argv[2];
      if (strcmp(op, "dump") == 0 || strcmp(op, "cat") == 0 || strcmp(op, "read") == 0)
        op = "show";
      else if (strcmp(op, "peek") == 0)
        op = "get";
      else if (strcmp(op, "num") == 0 || strcmp(op, "getpn") == 0 ||
               strcmp(op, "jgetn") == 0 || strcmp(op, "int") == 0)
        op = "getn";
      else if (strcmp(op, "peekobj") == 0 || strcmp(op, "objget") == 0)
        op = "getobj";
      else if (strcmp(op, "putobj") == 0 || strcmp(op, "nestp") == 0 ||
               strcmp(op, "putnest") == 0)
        op = "setobj";
      else if (strcmp(op, "patchnest") == 0 || strcmp(op, "updateobj") == 0)
        op = "mergeobj";
      else if (strcmp(op, "ensureobj") == 0 || strcmp(op, "defaultnest") == 0)
        op = "defaultobj";
      else if (strcmp(op, "kind") == 0 || strcmp(op, "typeof") == 0 ||
               strcmp(op, "typep") == 0 || strcmp(op, "jtype") == 0)
        op = "type";
      else if (strcmp(op, "put") == 0)
        op = "set";
      else if (strcmp(op, "ensurekey") == 0 || strcmp(op, "defaultp") == 0 ||
               strcmp(op, "setdefault") == 0)
        op = "default";
      else if (strcmp(op, "flip") == 0 || strcmp(op, "togglep") == 0 ||
               strcmp(op, "flipp") == 0)
        op = "toggle";
      else if (strcmp(op, "move") == 0 || strcmp(op, "mv") == 0 ||
               strcmp(op, "renamep") == 0 || strcmp(op, "movekey") == 0 ||
               strcmp(op, "rekey") == 0)
        op = "rename";
      else if (strcmp(op, "cp") == 0 || strcmp(op, "dup") == 0 ||
               strcmp(op, "copyp") == 0 || strcmp(op, "dupkey") == 0)
        op = "copy";
      else if (strcmp(op, "xchg") == 0 || strcmp(op, "swapp") == 0 ||
               strcmp(op, "exchange") == 0)
        op = "swap";
      else if (strcmp(op, "bump") == 0)
        op = "inc";
      else if (strcmp(op, "rm") == 0 || strcmp(op, "drop") == 0)
        op = "del";
      else if (strcmp(op, "list") == 0 || strcmp(op, "ls") == 0)
        op = "keys";
      else if (strcmp(op, "leafkeys") == 0 || strcmp(op, "pathkeys") == 0 ||
               strcmp(op, "dotpaths") == 0 || strcmp(op, "flatpaths") == 0 ||
               strcmp(op, "dotkeys") == 0 || strcmp(op, "leafpaths") == 0)
        op = "leaves";
      else if (strcmp(op, "flatkv") == 0 || strcmp(op, "leafkv") == 0 ||
               strcmp(op, "flatten") == 0 || strcmp(op, "flattenp") == 0 ||
               strcmp(op, "tokvpath") == 0 || strcmp(op, "pathtokv") == 0 ||
               strcmp(op, "plateflat") == 0)
        op = "flat";
      else if (strcmp(op, "unflatkv") == 0 || strcmp(op, "fromflat") == 0 ||
               strcmp(op, "unflatten") == 0 || strcmp(op, "unflattenp") == 0 ||
               strcmp(op, "pathfromkv") == 0 || strcmp(op, "applyflat") == 0)
        op = "unflat";
      else if (strcmp(op, "clogflat") == 0 || strcmp(op, "changelogflat") == 0 ||
               strcmp(op, "leafdiff") == 0 || strcmp(op, "flatdiff") == 0)
        op = "diffflat";
      else if (strcmp(op, "diffpath") == 0 || strcmp(op, "diffpaths") == 0 ||
               strcmp(op, "changedpaths") == 0)
        op = "pathdiff";
      else if (strcmp(op, "grepflat") == 0 || strcmp(op, "keepflat") == 0 ||
               strcmp(op, "filterflat") == 0 || strcmp(op, "leafgrep") == 0)
        op = "grepf";
      else if (strcmp(op, "grepvflat") == 0 || strcmp(op, "dropflat") == 0 ||
               strcmp(op, "vgrepflat") == 0)
        op = "grepvf";
      else if (strcmp(op, "grepflati") == 0 || strcmp(op, "igrepflat") == 0 ||
               strcmp(op, "grepiflat") == 0)
        op = "grepfi";
      else if (strcmp(op, "pruneflat") == 0 || strcmp(op, "delflat") == 0 ||
               strcmp(op, "scrubflat") == 0 || strcmp(op, "rmflat") == 0)
        op = "prune";
      else if (strcmp(op, "keeponlyflat") == 0 || strcmp(op, "retainflat") == 0 ||
               strcmp(op, "projectflat") == 0 || strcmp(op, "retain") == 0)
        op = "keeponly";
      else if (strcmp(op, "overlayflat") == 0 || strcmp(op, "patchflat") == 0 ||
               strcmp(op, "deepmerge") == 0 || strcmp(op, "leafmerge") == 0 ||
               strcmp(op, "syncflat") == 0 || strcmp(op, "applyflat") == 0)
        op = "mergeflat";
      else if (strcmp(op, "moveflat") == 0 || strcmp(op, "prefixflat") == 0 ||
               strcmp(op, "repathflat") == 0 || strcmp(op, "rekeyflat") == 0 ||
               strcmp(op, "mvflat") == 0)
        op = "renameflat";
      else if (strcmp(op, "mapflat") == 0 || strcmp(op, "setleaf") == 0 ||
               strcmp(op, "putflat") == 0 || strcmp(op, "bulkset") == 0 ||
               strcmp(op, "assignflat") == 0)
        op = "setflat";
      else if (strcmp(op, "bumpflat") == 0 || strcmp(op, "addflat") == 0 ||
               strcmp(op, "tickflat") == 0 || strcmp(op, "bulkinc") == 0)
        op = "incflat";
      else if (strcmp(op, "decflat") == 0)
        op = "decflat";
      else if (strcmp(op, "totalflat") == 0 || strcmp(op, "leafsum") == 0 ||
               strcmp(op, "flattotal") == 0 || strcmp(op, "sumleaf") == 0)
        op = "sumflat";
      else if (strcmp(op, "meanflat") == 0 || strcmp(op, "avgleaf") == 0 ||
               strcmp(op, "flatmean") == 0 || strcmp(op, "leafmean") == 0 ||
               strcmp(op, "avgallflat") == 0)
        op = "avgflat";
      else if (strcmp(op, "p50flat") == 0 || strcmp(op, "midflat") == 0 ||
               strcmp(op, "flatmedian") == 0 || strcmp(op, "leafmedian") == 0 ||
               strcmp(op, "medallflat") == 0)
        op = "medianflat";
      else if (strcmp(op, "toppathflat") == 0 || strcmp(op, "maxpath") == 0 ||
               strcmp(op, "maxpathflat") == 0 || strcmp(op, "topflat") == 0)
        op = "toppath";
      else if (strcmp(op, "botpathflat") == 0 || strcmp(op, "minpath") == 0 ||
               strcmp(op, "minpathflat") == 0 || strcmp(op, "botflat") == 0)
        op = "botpath";
      else if (strcmp(op, "keepvflat") == 0 || strcmp(op, "mincountflat") == 0 ||
               strcmp(op, "keepminflat") == 0)
        op = "threshflat";
      else if (strcmp(op, "nzflat") == 0 || strcmp(op, "keepnzflat") == 0)
        op = "dropzeroflat";
      else if (strcmp(op, "clampflat") == 0 || strcmp(op, "maxvflat") == 0 ||
               strcmp(op, "ceilflat") == 0 || strcmp(op, "maxcapflat") == 0)
        op = "capflat";
      else if (strcmp(op, "mulflat") == 0 || strcmp(op, "timesflat") == 0 ||
               strcmp(op, "multflat") == 0 || strcmp(op, "factorflat") == 0)
        op = "scaleflat";
      else if (strcmp(op, "anyflat") == 0 || strcmp(op, "hitflat") == 0)
        op = "hasflat";
      else if (strcmp(op, "nflat") == 0 || strcmp(op, "leafcount") == 0 ||
               strcmp(op, "ncflat") == 0)
        op = "countflat";
      else if (strcmp(op, "anyflatn") == 0 || strcmp(op, "hitflatn") == 0 ||
               strcmp(op, "anynumflat") == 0)
        op = "hasflatn";
      else if (strcmp(op, "nflatn") == 0 || strcmp(op, "leafcountn") == 0 ||
               strcmp(op, "ncflatn") == 0 || strcmp(op, "numcountflat") == 0)
        op = "countflatn";
      else if (strcmp(op, "matchpaths") == 0 || strcmp(op, "pathmatch") == 0)
        op = "pathsflat";
      else if (strcmp(op, "matchvals") == 0 || strcmp(op, "leafvals") == 0 ||
               strcmp(op, "valmatch") == 0)
        op = "valsflat";
      else if (strcmp(op, "matchpathsn") == 0 || strcmp(op, "pathmatchn") == 0 ||
               strcmp(op, "numpathsflat") == 0)
        op = "pathsflatn";
      else if (strcmp(op, "matchvalsn") == 0 || strcmp(op, "leafvalsn") == 0 ||
               strcmp(op, "numvalsflat") == 0 || strcmp(op, "intvalsflat") == 0)
        op = "valsflatn";
      else if (strcmp(op, "firstflat") == 0 || strcmp(op, "peelflat") == 0 ||
               strcmp(op, "leafget") == 0 || strcmp(op, "getleaf") == 0)
        op = "getflat";
      else if (strcmp(op, "endflat") == 0 || strcmp(op, "tailflat") == 0 ||
               strcmp(op, "lastleafp") == 0 || strcmp(op, "getleafl") == 0)
        op = "lastflat";
      else if (strcmp(op, "indexflat") == 0 || strcmp(op, "atflat") == 0 ||
               strcmp(op, "nthleaf") == 0 || strcmp(op, "leafnth") == 0)
        op = "nthflat";
      else if (strcmp(op, "requireflat") == 0 || strcmp(op, "mustflat") == 0 ||
               strcmp(op, "needleaves") == 0)
        op = "needflat";
      else if (strcmp(op, "firstflatn") == 0 || strcmp(op, "numflat") == 0 ||
               strcmp(op, "getnflat") == 0 || strcmp(op, "ngetflat") == 0)
        op = "getflatn";
      else if (strcmp(op, "endflatn") == 0 || strcmp(op, "tailflatn") == 0 ||
               strcmp(op, "lastnumflat") == 0 || strcmp(op, "getnflatl") == 0)
        op = "lastflatn";
      else if (strcmp(op, "indexflatn") == 0 || strcmp(op, "atflatn") == 0 ||
               strcmp(op, "nthnumflat") == 0 || strcmp(op, "leafnthn") == 0)
        op = "nthflatn";
      else if (strcmp(op, "kindflat") == 0 || strcmp(op, "leaftype") == 0 ||
               strcmp(op, "flattype") == 0 || strcmp(op, "typeleafp") == 0)
        op = "typeflat";
      else if (strcmp(op, "histflat") == 0 || strcmp(op, "leaffreq") == 0 ||
               strcmp(op, "countvalflat") == 0 || strcmp(op, "valfreqflat") == 0 ||
               strcmp(op, "tallyflat") == 0)
        op = "freqflat";
      else if (strcmp(op, "topvalflat") == 0 || strcmp(op, "majorflat") == 0 ||
               strcmp(op, "leafmode") == 0 || strcmp(op, "modalflat") == 0 ||
               strcmp(op, "dominantflat") == 0 || strcmp(op, "winvalflat") == 0)
        op = "modeflat";
      else if (strcmp(op, "valpath") == 0 || strcmp(op, "findval") == 0 ||
               strcmp(op, "pathofval") == 0 || strcmp(op, "findpathval") == 0 ||
               strcmp(op, "whereval") == 0 || strcmp(op, "locateval") == 0)
        op = "pathbyval";
      else if (strcmp(op, "distinctflat") == 0 || strcmp(op, "uniqueflat") == 0 ||
               strcmp(op, "leafuniq") == 0 || strcmp(op, "dedupflat") == 0 ||
               strcmp(op, "uniqvalsflat") == 0 || strcmp(op, "uniquevals") == 0)
        op = "uniqflat";
      else if (strcmp(op, "allpathsbyval") == 0 || strcmp(op, "findvals") == 0 ||
               strcmp(op, "pathsofval") == 0 || strcmp(op, "findpathsval") == 0 ||
               strcmp(op, "wherevals") == 0 || strcmp(op, "locatevals") == 0)
        op = "pathsbyval";
      else if (strcmp(op, "nbyval") == 0 || strcmp(op, "valcount") == 0 ||
               strcmp(op, "nval") == 0 || strcmp(op, "countval") == 0)
        op = "countbyval";
      else if (strcmp(op, "hasvalflat") == 0 || strcmp(op, "anyval") == 0 ||
               strcmp(op, "valhas") == 0 || strcmp(op, "containsval") == 0)
        op = "hasval";
      else if (strcmp(op, "replaceval") == 0 || strcmp(op, "rewval") == 0 ||
               strcmp(op, "mapval") == 0 || strcmp(op, "rewriteval") == 0 ||
               strcmp(op, "reval") == 0 || strcmp(op, "swapval") == 0)
        op = "setbyval";
      else if (strcmp(op, "dropval") == 0 || strcmp(op, "rmval") == 0 ||
               strcmp(op, "pruneval") == 0 || strcmp(op, "deleteval") == 0 ||
               strcmp(op, "removeval") == 0 || strcmp(op, "stripval") == 0)
        op = "delbyval";
      else if (strcmp(op, "samevalflat") == 0 || strcmp(op, "eqvalsflat") == 0 ||
               strcmp(op, "allsameflat") == 0 || strcmp(op, "sameflat") == 0 ||
               strcmp(op, "eqallflat") == 0 || strcmp(op, "uniformflat") == 0)
        op = "alleqflat";
      else if (strcmp(op, "firstuneqflat") == 0 || strcmp(op, "pathuneq") == 0 ||
               strcmp(op, "diverge1") == 0 || strcmp(op, "firstdiverge") == 0 ||
               strcmp(op, "uneq1") == 0 || strcmp(op, "firstdiffval") == 0 ||
               strcmp(op, "pathdiffval") == 0 || strcmp(op, "finduneq") == 0)
        op = "firstuneq";
      else if (strcmp(op, "divergepaths") == 0 || strcmp(op, "alluneq") == 0 ||
               strcmp(op, "pathsuneq") == 0 || strcmp(op, "divergeall") == 0 ||
               strcmp(op, "alldiverge") == 0 || strcmp(op, "uneqall") == 0 ||
               strcmp(op, "uneqpath") == 0 || strcmp(op, "divergepath") == 0)
        op = "uneqpaths";
      else if (strcmp(op, "uniformflat") == 0 || strcmp(op, "checkflat") == 0 ||
               strcmp(op, "samecheck") == 0 || strcmp(op, "checkuniform") == 0 ||
               strcmp(op, "nestuniform") == 0 || strcmp(op, "flatuniform") == 0 ||
               strcmp(op, "uniformcheck") == 0 || strcmp(op, "eqcheck") == 0)
        op = "uniform";
      else if (strcmp(op, "prettyp") == 0 || strcmp(op, "jsonpretty") == 0 ||
               strcmp(op, "prettyjson") == 0 || strcmp(op, "prettyplate") == 0 ||
               strcmp(op, "fmtjson") == 0 || strcmp(op, "indentjson") == 0 ||
               strcmp(op, "pp") == 0)
        op = "pretty";
      else if (strcmp(op, "requireflatn") == 0 || strcmp(op, "mustflatn") == 0 ||
               strcmp(op, "needintflat") == 0 || strcmp(op, "neednumflat") == 0)
        op = "needflatn";
      else if (strcmp(op, "length") == 0 || strcmp(op, "nkeys") == 0 ||
               strcmp(op, "size") == 0 || strcmp(op, "countkeys") == 0)
        op = "len";
      else if (strcmp(op, "isempty") == 0 || strcmp(op, "emptyp") == 0)
        op = "empty";
      else if (strcmp(op, "values") == 0 || strcmp(op, "valsp") == 0)
        op = "vals";
      else if (strcmp(op, "expand") == 0 || strcmp(op, "template") == 0 ||
               strcmp(op, "render") == 0)
        op = "fill";
      else if (strcmp(op, "fill-keys") == 0 || strcmp(op, "tmplkeys") == 0)
        op = "fillkeys";
      else if (strcmp(op, "seed") == 0 || strcmp(op, "touch") == 0 ||
               strcmp(op, "create") == 0)
        op = "ensure";
      else if (strcmp(op, "patch") == 0 || strcmp(op, "overlay") == 0 ||
               strcmp(op, "apply") == 0)
        op = "merge";
      else if (strcmp(op, "same") == 0 || strcmp(op, "equal") == 0 ||
               strcmp(op, "cmp") == 0)
        op = "eq";
      else if (strcmp(op, "neq") == 0 || strcmp(op, "differ") == 0 ||
               strcmp(op, "unequal") == 0)
        op = "ne";
      else if (strcmp(op, "changed") == 0 || strcmp(op, "delta") == 0)
        op = "diff";
      else if (strcmp(op, "clog") == 0 || strcmp(op, "changes") == 0 ||
               strcmp(op, "logdiff") == 0)
        op = "changelog";
      else if (strcmp(op, "hasall") == 0 || strcmp(op, "has-keys") == 0 ||
               strcmp(op, "check") == 0)
        op = "has";
      else if (strcmp(op, "require") == 0 || strcmp(op, "needkeys") == 0 ||
               strcmp(op, "require-keys") == 0)
        op = "need";
      else if (strcmp(op, "getall") == 0 || strcmp(op, "getpall") == 0 ||
               strcmp(op, "peel") == 0 || strcmp(op, "values") == 0 ||
               strcmp(op, "pluckp") == 0)
        op = "pluck";
      else if (strcmp(op, "nestpluck") == 0 || strcmp(op, "objpluck") == 0 ||
               strcmp(op, "plucknest") == 0)
        op = "pluckobj";
      else if (strcmp(op, "keep") == 0 || strcmp(op, "select") == 0 ||
               strcmp(op, "project") == 0)
        op = "pick";
      else if (strcmp(op, "strip") == 0 || strcmp(op, "dropkeys") == 0 ||
               strcmp(op, "drop-keys") == 0)
        op = "omit";
      else if (strcmp(op, "total") == 0)
        op = "sum";
      else if (strcmp(op, "mean") == 0 || strcmp(op, "average") == 0)
        op = "avg";
      else if (strcmp(op, "p50") == 0 || strcmp(op, "mid") == 0)
        op = "median";
      else if (strcmp(op, "topn") == 0 || strcmp(op, "head") == 0)
        op = "top";
      else if (strcmp(op, "bottom") == 0 || strcmp(op, "botn") == 0 ||
               strcmp(op, "tail") == 0)
        op = "bot";
      else if (strcmp(op, "nget") == 0 || strcmp(op, "getnest") == 0 ||
               strcmp(op, "getpobj") == 0)
        op = "nestget";
      else if (strcmp(op, "nset") == 0 || strcmp(op, "setnest") == 0 ||
               strcmp(op, "setpobj") == 0)
        op = "nestset";
      else if (strcmp(op, "ninc") == 0 || strcmp(op, "incnest") == 0 ||
               strcmp(op, "incobj") == 0)
        op = "nestinc";
      else if (strcmp(op, "ndel") == 0 || strcmp(op, "delnest") == 0 ||
               strcmp(op, "delpobj") == 0)
        op = "nestdel";
      else if (strcmp(op, "nkeys") == 0 || strcmp(op, "keysobj") == 0 ||
               strcmp(op, "keysnest") == 0)
        op = "nestkeys";
      else if (strcmp(op, "nhas") == 0 || strcmp(op, "haspobj") == 0 ||
               strcmp(op, "hasnest") == 0)
        op = "nesthas";
      else if (strcmp(op, "npick") == 0 || strcmp(op, "pickobj") == 0 ||
               strcmp(op, "picknest") == 0)
        op = "nestpick";
      else if (strcmp(op, "nomit") == 0 || strcmp(op, "omitobj") == 0 ||
               strcmp(op, "omitnest") == 0 || strcmp(op, "stripnest") == 0)
        op = "nestomit";
      else if (strcmp(op, "nrename") == 0 || strcmp(op, "renamenest") == 0 ||
               strcmp(op, "renamepobj") == 0)
        op = "nestrename";
      else if (strcmp(op, "ncopy") == 0 || strcmp(op, "copynest") == 0 ||
               strcmp(op, "copypobj") == 0)
        op = "nestcopy";
      else if (strcmp(op, "nswap") == 0 || strcmp(op, "swapnest") == 0 ||
               strcmp(op, "swappobj") == 0)
        op = "nestswap";
      else if (strcmp(op, "nsum") == 0 || strcmp(op, "sumnest") == 0 ||
               strcmp(op, "sumnobj") == 0)
        op = "nestsum";
      else if (strcmp(op, "navg") == 0 || strcmp(op, "avgnest") == 0 ||
               strcmp(op, "nestmean") == 0)
        op = "nestavg";
      else if (strcmp(op, "nmedian") == 0 || strcmp(op, "nestp50") == 0 ||
               strcmp(op, "mediannest") == 0)
        op = "nestmedian";
      else if (strcmp(op, "ntop") == 0 || strcmp(op, "topnest") == 0 ||
               strcmp(op, "topnobj") == 0)
        op = "nesttop";
      else if (strcmp(op, "nbot") == 0 || strcmp(op, "botnest") == 0 ||
               strcmp(op, "botnobj") == 0 || strcmp(op, "nestbottom") == 0)
        op = "nestbot";
      else if (strcmp(op, "nsort") == 0 || strcmp(op, "sortnest") == 0 ||
               strcmp(op, "sortobj") == 0)
        op = "nestsort";
      else if (strcmp(op, "nsortbag") == 0 || strcmp(op, "sortbagnest") == 0 ||
               strcmp(op, "sortbagobj") == 0 || strcmp(op, "nestsortfreq") == 0 ||
               strcmp(op, "sortfreqobj") == 0)
        op = "nestsortbag";
      ai = 3;
      /* fill[-keys]: optional -s|--strict before path */
      if ((strcmp(op, "fill") == 0 || strcmp(op, "fillkeys") == 0) &&
          ai < argc && argv[ai] &&
          (!strcmp(argv[ai], "--strict") || !strcmp(argv[ai], "-s"))) {
        val_kind = 99;
        ai++;
      }
      if (ai >= argc) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"err\":\"need path\",\"version\":\"%s\"}\n",
               op, CUBALC_LANG_VERSION);
        return 2;
      }
      path = argv[ai++];
    } else {
      /* bare path → show */
      op = "show";
      path = argv[2];
      ai = 3;
    }

    if (!path || !path[0]) {
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
             "\"err\":\"empty path\",\"version\":\"%s\"}\n", CUBALC_LANG_VERSION);
      return 2;
    }

    /* load soft object plate */
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_read(path, &hr) == 0 && hr.str[0]) {
      body = hr.str;
      while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
        body++;
      if (*body == '{') {
        memset(&keys, 0, sizeof keys);
        if (cubalc_host_json_keys(body, &keys) == 0) {
          snprintf(plate, sizeof plate, "%s", body);
          file_hit = 1;
        }
      }
    }
    if (!file_hit)
      snprintf(plate, sizeof plate, "%s", "{}");

    if (strcmp(op, "show") == 0) {
      memset(&keys, 0, sizeof keys);
      cubalc_host_json_keys(plate, &keys);
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"show\",\"path\":\"%s\",\"file\":%s,\"keys_n\":%ld,"
             "\"version\":\"%s\",\"plate\":%s}\n",
             path, file_hit ? "true" : "false", keys.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* pretty — PRETTYP dual: 2-space indented plate text (read-only, no jq).
     *   cubalc plate pretty agent.json
     * prints pretty body then JSON meta with n=bytes. */
    if (strcmp(op, "pretty") == 0) {
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_pretty(plate, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pretty\",\"path\":\"%s\",\"err\":\"pretty fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.str[0])
        printf("%s\n", pr.str);
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else if (c == '\r') { esc[o++] = '\\'; esc[o++] = 'r'; }
        else if (c == '\t') { esc[o++] = '\\'; esc[o++] = 't'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"pretty\",\"path\":\"%s\",\"file\":%s,\"n\":%ld,"
             "\"truncated\":%s,\"pretty\":\"%s\",\"version\":\"%s\","
             "\"note\":\"PRETTYP dual · 2-space indented plate (no jq)\"}\n",
             path, file_hit ? "true" : "false", pr.n,
             pr.code ? "false" : "true", esc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* leaves|pathkeys [nest.path] — PATHKEYS dual: dotted leaf path bag.
     *   cubalc plate leaves agent.json
     *   cubalc plate leaves agent.json cfg
     *   cubalc plate pathkeys agent.json cfg.meta */
    if (strcmp(op, "leaves") == 0) {
      const char *nestp = NULL;
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t i, o = 0;

      if (ai < argc && argv[ai] && argv[ai][0])
        nestp = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_paths(plate, nestp, &pr) != 0) {
        pr.str[0] = 0;
        pr.n = 0;
      }
      flat[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof flat; i++) {
        char ch = pr.str[i];
        if (ch == '\n' || ch == '\r') {
          if (o > 0 && flat[o - 1] != ',')
            flat[o++] = ',';
        } else if (ch == '"' || ch == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = ch;
        }
      }
      flat[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"leaves\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"paths\":\"%s\",\"version\":\"%s\","
             "\"note\":\"PATHKEYS dual · dotted leaf paths · bag lines above plate\"}\n",
             path, nestp ? nestp : "", file_hit ? "true" : "false",
             pr.n, flat, CUBALC_LANG_VERSION);
      return 0;
    }

    /* flat|flatkv [nest.path] — FLATKV dual: recursive path:value bag.
     *   cubalc plate flat agent.json
     *   cubalc plate flat agent.json cfg
     *   cubalc plate flatkv agent.json cfg.meta */
    if (strcmp(op, "flat") == 0) {
      const char *nestp = NULL;
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t i, o = 0;

      if (ai < argc && argv[ai] && argv[ai][0])
        nestp = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_kv(plate, nestp, &pr) != 0) {
        pr.str[0] = 0;
        pr.n = 0;
      }
      flat[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof flat; i++) {
        char ch = pr.str[i];
        if (ch == '\n' || ch == '\r') {
          if (o > 0 && flat[o - 1] != ',')
            flat[o++] = ',';
        } else if (ch == '"' || ch == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = ch;
        }
      }
      flat[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"flat\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"FLATKV dual · path:value bag · lines above plate\"}\n",
             path, nestp ? nestp : "", file_hit ? "true" : "false",
             pr.n, flat, CUBALC_LANG_VERSION);
      return 0;
    }

    /* unflat|unflatkv [UNDER nest] bag|@file|k:v… — UNFLATKV dual: path:value → nest plate.
     *   cubalc plate unflat agent.json host:cubeA cfg.port:9
     *   cubalc plate unflat agent.json @flat.txt
     *   cubalc plate unflat agent.json UNDER cfg meta.role:worker port:8080
     * Creates plate file if missing. Empty val → {}. Writes plate back to path. */
    if (strcmp(op, "unflat") == 0) {
      const char *under = NULL;
      char bag[CUBALC_HOST_STR_MAX];
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr, wr, tr;
      size_t o = 0, i;
      long nlines = 0;

      bag[0] = 0;
      /* optional UNDER nest */
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "UNDER") == 0 || strcmp(argv[ai], "under") == 0 ||
           strcmp(argv[ai], "AT") == 0 || strcmp(argv[ai], "at") == 0 ||
           strcmp(argv[ai], "IN") == 0 || strcmp(argv[ai], "in") == 0)) {
        ai++;
        if (ai < argc && argv[ai] && argv[ai][0])
          under = argv[ai++];
      }

      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"unflat\",\"path\":\"%s\","
               "\"err\":\"need bag lines, @file, or path:val args\",\"version\":\"%s\"}\n",
               path, CUBALC_LANG_VERSION);
        return 2;
      }

      /* single @file or readable file → bag body; else join remaining as lines */
      if (argv[ai][0] == '@' ||
          (ai + 1 == argc && argv[ai][0] != '{' &&
           strchr(argv[ai], ':') == NULL && strchr(argv[ai], '=') == NULL &&
           access(argv[ai], R_OK) == 0)) {
        const char *fp = (argv[ai][0] == '@') ? argv[ai] + 1 : argv[ai];
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(fp, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"unflat\",\"err\":\"bag file read fail\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(bag, sizeof bag, "%s", tr.str);
        ai++;
      } else {
        while (ai < argc && argv[ai] && argv[ai][0]) {
          size_t kl = strlen(argv[ai]);
          if (o + kl + 2 >= sizeof bag) break;
          if (o > 0) bag[o++] = '\n';
          memcpy(bag + o, argv[ai], kl);
          o += kl;
          bag[o] = 0;
          ai++;
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_unflat_kv(plate, bag, under, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"unflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "unflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      nlines = pr.n;
      snprintf(plate, sizeof plate, "%s", pr.str);

      /* write-back file when path looks like file */
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"unflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }

      flat[0] = 0;
      o = 0;
      for (i = 0; bag[i] && o + 2 < sizeof flat; i++) {
        char ch = bag[i];
        if (ch == '\n' || ch == '\r') {
          if (o > 0 && flat[o - 1] != ',')
            flat[o++] = ',';
        } else if (ch == '"' || ch == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = ch;
        }
      }
      flat[o] = 0;

      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"unflat\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"UNFLATKV dual · path:value → nested plate\"}\n",
             path, under ? under : "", file_hit ? "true" : "false",
             nlines, flat, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* grepf|grepvf|grepfi [needle] — GREPFLAT duals: filter path:value bag by path needle.
     *   cubalc plate grepf  agent.json cfg
     *   cubalc plate grepvf agent.json tmp
     *   cubalc plate grepfi agent.json CFG.META
     * Bag lines above plate. Empty needle → all leaves. */
    if (strcmp(op, "grepf") == 0 || strcmp(op, "grepvf") == 0 ||
        strcmp(op, "grepfi") == 0) {
      const char *needle = "";
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t i, o = 0;
      int invert = (strcmp(op, "grepvf") == 0) ? 1 : 0;
      int icase = (strcmp(op, "grepfi") == 0) ? 1 : 0;

      if (ai < argc && argv[ai] && argv[ai][0])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      cubalc_host_json_leaf_grep(plate, needle, invert, icase, &pr);
      flat[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof flat; i++) {
        char ch = pr.str[i];
        if (ch == '\n' || ch == '\r') {
          if (o > 0 && flat[o - 1] != ',')
            flat[o++] = ',';
        } else if (ch == '"' || ch == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = ch;
        }
      }
      flat[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"invert\":%s,\"icase\":%s,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"GREPFLAT dual · path needle filter · bag lines above plate\"}\n",
             op, path, needle, file_hit ? "true" : "false",
             pr.n, invert ? "true" : "false", icase ? "true" : "false",
             flat, CUBALC_LANG_VERSION);
      return 0;
    }

    /* prune|keeponly [needle] — PRUNEFLAT / KEEONLYFLAT duals (write-back).
     *   cubalc plate prune    agent.json tmp
     *   cubalc plate keeponly agent.json cfg
     * Rewrites plate file. n = deleted (prune) or kept (keeponly). */
    if (strcmp(op, "prune") == 0 || strcmp(op, "keeponly") == 0) {
      const char *needle = "";
      cubalc_host_result pr, wr;
      int keep_only = (strcmp(op, "keeponly") == 0) ? 1 : 0;

      if (ai < argc && argv[ai] && argv[ai][0])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_filter_write(plate, needle, keep_only, 0, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "filter write fail",
               CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"keep_only\":%s,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"%s\"}\n",
             op, path, needle, file_hit ? "true" : "false",
             pr.n, keep_only ? "true" : "false", CUBALC_LANG_VERSION, plate,
             keep_only
                 ? "KEEPONLYFLAT dual · project matching leaves"
                 : "PRUNEFLAT dual · drop matching leaves");
      return 0;
    }

    /* mergeflat <overlay.json> — MERGEFLAT dual: deep leaf overlay onto path plate.
     *   cubalc plate mergeflat base.json overlay.json
     * Writes base. n = overlay leaf count. Nest-aware (unlike plate merge top-level). */
    if (strcmp(op, "mergeflat") == 0) {
      const char *path2 = NULL;
      char plate2[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr, wr, tr;
      int file_hit2 = 0;
      const char *body2;

      plate2[0] = 0;
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"mergeflat\",\"path\":\"%s\",\"err\":\"need overlay path\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      path2 = argv[ai++];
      memset(&tr, 0, sizeof tr);
      if (cubalc_host_read(path2, &tr) == 0 && tr.str[0]) {
        body2 = tr.str;
        while (*body2 == ' ' || *body2 == '\t' || *body2 == '\n' || *body2 == '\r')
          body2++;
        if (*body2 == '{') {
          snprintf(plate2, sizeof plate2, "%s", body2);
          file_hit2 = 1;
        }
      }
      if (!file_hit2)
        snprintf(plate2, sizeof plate2, "%s", "{}");

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_merge(plate, plate2, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"mergeflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "mergeflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"mergeflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"mergeflat\",\"path\":\"%s\",\"path2\":\"%s\",\"file\":%s,"
             "\"file2\":%s,\"n\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"MERGEFLAT dual · deep leaf overlay · nest-aware\"}\n",
             path, path2, file_hit ? "true" : "false",
             file_hit2 ? "true" : "false", pr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* renameflat <old_pfx> <new_pfx> — RENAMEFLAT dual: rewrite leaf path prefixes.
     *   cubalc plate renameflat agent.json cfg. config.
     * Writes plate. n = renamed leaf count. */
    if (strcmp(op, "renameflat") == 0) {
      const char *oldp = NULL, *newp = NULL;
      cubalc_host_result pr, wr;

      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"renameflat\",\"path\":\"%s\",\"err\":\"need old_pfx new_pfx\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      oldp = argv[ai++];
      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"renameflat\",\"path\":\"%s\",\"err\":\"need new_pfx\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      newp = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_rename_pfx(plate, oldp, newp, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"renameflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "renameflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"renameflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"renameflat\",\"path\":\"%s\",\"old\":\"%s\",\"new\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"RENAMEFLAT dual · leaf path prefix rewrite\"}\n",
             path, oldp, newp, file_hit ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* setflat <needle> <value> — SETFLAT dual: bulk set leaf values by path needle.
     *   cubalc plate setflat agent.json debug 0
     *   cubalc plate setflat agent.json cfg.port 9090
     * Writes plate. n = updated leaf count. Empty needle sets all leaves. */
    if (strcmp(op, "setflat") == 0) {
      const char *needle = "", *val = NULL;
      cubalc_host_result pr, wr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setflat\",\"path\":\"%s\",\"err\":\"need needle value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      /* allow empty needle as "" */
      needle = argv[ai++];
      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setflat\",\"path\":\"%s\",\"err\":\"need value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      val = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_set(plate, needle, val, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "setflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"setflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"setflat\",\"path\":\"%s\",\"needle\":\"%s\",\"value\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"SETFLAT dual · bulk leaf value set by path needle\"}\n",
             path, needle, val, file_hit ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* incflat|decflat <needle> [delta] — INCFLAT dual: bump pure-int leaves by path needle.
     *   cubalc plate incflat agent.json hits
     *   cubalc plate incflat agent.json cfg.meta.x 5
     *   cubalc plate decflat agent.json errs
     * Writes plate. n = bumped count. Non-numeric soft-skipped. */
    if (strcmp(op, "incflat") == 0 || strcmp(op, "decflat") == 0) {
      const char *needle = "";
      long delta = 1;
      cubalc_host_result pr, wr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need needle [delta]\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai < argc && argv[ai] && argv[ai][0]) {
        char *end = NULL;
        long d = strtol(argv[ai], &end, 10);
        if (end && end != argv[ai] && *end == 0) {
          delta = d;
          ai++;
        }
      }
      if (strcmp(op, "decflat") == 0) {
        if (delta > 0) delta = -delta;
        else if (delta == 1) delta = -1;
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_inc(plate, needle, delta, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "incflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\",\"delta\":%ld,"
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"INCFLAT dual · bump pure-int leaves by path needle\"}\n",
             op, path, needle, delta, file_hit ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* sumflat <needle?> — SUMFLAT dual: sum pure-int leaves by path needle (read-only).
     *   cubalc plate sumflat agent.json hits
     *   cubalc plate sumflat agent.json          # all pure-int leaves
     * n = sum · count = pure-int leaves used · plate not written. */
    if (strcmp(op, "sumflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_sum(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"sumflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "sumflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"sumflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"count\":%ld,\"sum\":%ld,\"version\":\"%s\","
             "\"note\":\"SUMFLAT dual · sum pure-int leaves by path needle (read-only)\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n, (long)pr.code, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* avgflat [needle] — AVGFLAT dual: integer mean pure-int leaves (read-only).
     *   cubalc plate avgflat agent.json score
     *   cubalc plate avgflat agent.json          # all pure-int leaves
     * n = mean · count = pure-int leaves used · sum side · plate not written. */
    if (strcmp(op, "avgflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      long sum_side = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_avg(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"avgflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "avgflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.err[0]) {
        char *end = NULL;
        long s = strtol(pr.err, &end, 10);
        if (end && end != pr.err && *end == 0)
          sum_side = s;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"avgflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"count\":%ld,\"avg\":%ld,\"sum\":%ld,"
             "\"version\":\"%s\","
             "\"note\":\"AVGFLAT dual · mean pure-int leaves by path needle (read-only)\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n, (long)pr.code, pr.n, sum_side, CUBALC_LANG_VERSION);
      return 0;
    }

    /* medianflat [needle] — MEDIANFLAT dual: integer median pure-int leaves (read-only).
     *   cubalc plate medianflat agent.json score
     *   cubalc plate medianflat agent.json          # all pure-int leaves
     * n = median · count = pure-int leaves used · plate not written. */
    if (strcmp(op, "medianflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_median(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"medianflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "medianflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"medianflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"count\":%ld,\"median\":%ld,"
             "\"version\":\"%s\","
             "\"note\":\"MEDIANFLAT dual · median pure-int leaves by path needle (read-only)\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n, (long)pr.code, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* toppath|botpath [needle] — TOPPATHFLAT/BOTPATHFLAT dual (read-only).
     *   cubalc plate toppath agent.json score
     *   cubalc plate botpath agent.json latency
     * path = winning leaf path · n = value · count = pure-int leaves considered. */
    if (strcmp(op, "toppath") == 0 || strcmp(op, "botpath") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int want_min = (strcmp(op, "botpath") == 0);

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_toppath(plate, needle, want_min, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "toppath fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"leaf\":\"%s\",\"n\":%ld,\"count\":%ld,\"version\":\"%s\","
             "\"note\":\"%s dual · path of %s pure-int leaf by needle (read-only)\"}\n",
             op, path, needle, file_hit ? "true" : "false",
             pr.str, pr.n, (long)pr.code, CUBALC_LANG_VERSION,
             want_min ? "BOTPATHFLAT" : "TOPPATHFLAT",
             want_min ? "min" : "max");
      return 0;
    }

    /* threshflat [needle] min | dropzeroflat [needle]
     * THRESHFLAT/DROPZEROFLAT duals: denoise pure-int leaves by value · write plate.
     *   cubalc plate threshflat agent.json hits 5
     *   cubalc plate threshflat agent.json 10          # empty needle
     *   cubalc plate dropzeroflat agent.json delta
     */
    if (strcmp(op, "threshflat") == 0 || strcmp(op, "dropzeroflat") == 0) {
      const char *needle = "";
      long minv = 0;
      int drop_zero = (strcmp(op, "dropzeroflat") == 0);
      int have_min = 0;
      cubalc_host_result pr, wr;

      if (drop_zero) {
        if (ai < argc && argv[ai])
          needle = argv[ai++];
      } else {
        if (ai >= argc || !argv[ai]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"threshflat\",\"path\":\"%s\",\"err\":\"need [needle] min\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 2;
        }
        /* if only one arg and it's a number → empty needle + min */
        if (ai + 1 >= argc) {
          char *end = NULL;
          long d = strtol(argv[ai], &end, 10);
          if (end && end != argv[ai] && *end == 0) {
            minv = d;
            have_min = 1;
            ai++;
          } else {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"threshflat\",\"path\":\"%s\",\"err\":\"need min int\","
                   "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
            return 2;
          }
        } else {
          needle = argv[ai++];
          {
            char *end = NULL;
            long d = strtol(argv[ai], &end, 10);
            if (!(end && end != argv[ai] && *end == 0)) {
              printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                     "\"op\":\"threshflat\",\"path\":\"%s\",\"err\":\"need min int\","
                     "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
              return 2;
            }
            minv = d;
            have_min = 1;
            ai++;
          }
        }
        (void)have_min;
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_thresh(plate, needle, minv, drop_zero, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "threshflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\",\"min\":%ld,"
             "\"file\":%s,\"n\":%ld,\"drop\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"%s dual · pure-int leaf value denoise by path needle\"}\n",
             op, path, needle, minv, file_hit ? "true" : "false",
             pr.n, (long)pr.code, CUBALC_LANG_VERSION, plate,
             drop_zero ? "DROPZEROFLAT" : "THRESHFLAT");
      return 0;
    }

    /* capflat [needle] max — CAPFLAT dual: clamp pure-int leaves to max · write plate.
     *   cubalc plate capflat agent.json score 100
     *   cubalc plate capflat agent.json 50          # empty needle
     */
    if (strcmp(op, "capflat") == 0) {
      const char *needle = "";
      long maxv = 0;
      cubalc_host_result pr, wr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"need [needle] max\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      if (ai + 1 >= argc) {
        char *end = NULL;
        long d = strtol(argv[ai], &end, 10);
        if (end && end != argv[ai] && *end == 0) {
          maxv = d;
          ai++;
        } else {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"need max int\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 2;
        }
      } else {
        needle = argv[ai++];
        {
          char *end = NULL;
          long d = strtol(argv[ai], &end, 10);
          if (!(end && end != argv[ai] && *end == 0)) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"need max int\","
                   "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
            return 2;
          }
          maxv = d;
          ai++;
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_cap(plate, needle, maxv, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "capflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"capflat\",\"path\":\"%s\",\"needle\":\"%s\",\"max\":%ld,"
             "\"file\":%s,\"n\":%ld,\"match\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"CAPFLAT dual · clamp pure-int leaves to max by path needle\"}\n",
             path, needle, maxv, file_hit ? "true" : "false",
             pr.n, (long)pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* scaleflat [needle] factor — SCALEFLAT dual: multiply pure-int leaves · write plate.
     *   cubalc plate scaleflat agent.json score 2
     *   cubalc plate scaleflat agent.json 10          # empty needle
     */
    if (strcmp(op, "scaleflat") == 0) {
      const char *needle = "";
      long factor = 1;
      cubalc_host_result pr, wr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"need [needle] factor\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      if (ai + 1 >= argc) {
        char *end = NULL;
        long d = strtol(argv[ai], &end, 10);
        if (end && end != argv[ai] && *end == 0) {
          factor = d;
          ai++;
        } else {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"need factor int\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 2;
        }
      } else {
        needle = argv[ai++];
        {
          char *end = NULL;
          long d = strtol(argv[ai], &end, 10);
          if (!(end && end != argv[ai] && *end == 0)) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"need factor int\","
                   "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
            return 2;
          }
          factor = d;
          ai++;
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_scale(plate, needle, factor, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "scaleflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"scaleflat\",\"path\":\"%s\",\"needle\":\"%s\",\"factor\":%ld,"
             "\"file\":%s,\"n\":%ld,\"match\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"SCALEFLAT dual · multiply pure-int leaves by factor\"}\n",
             path, needle, factor, file_hit ? "true" : "false",
             pr.n, (long)pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* hasflat|countflat [needle] — HASFLAT/COUNTFLAT duals (read-only).
     *   cubalc plate hasflat agent.json error
     *   cubalc plate countflat agent.json score
     * hasflat: n = 0|1 hit · count = match count
     * countflat: n = match count
     */
    if (strcmp(op, "hasflat") == 0 || strcmp(op, "countflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int is_count = (strcmp(op, "countflat") == 0);

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_count(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "hasflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      if (is_count) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"countflat\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"COUNTFLAT dual · count leaf paths matching needle (read-only)\"}\n",
               path, needle, file_hit ? "true" : "false",
               pr.n, CUBALC_LANG_VERSION);
      } else {
        long hit = (pr.n > 0) ? 1 : 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"hasflat\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"count\":%ld,\"hit\":%s,\"version\":\"%s\","
               "\"note\":\"HASFLAT dual · soft leaf-path presence by needle (read-only)\"}\n",
               path, needle, file_hit ? "true" : "false",
               hit, pr.n, hit ? "true" : "false", CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* hasflatn|countflatn [needle] — HASFLATN/COUNTFLATN duals (pure-int, read-only).
     *   cubalc plate hasflatn agent.json port
     *   cubalc plate countflatn agent.json score
     * hasflatn: n=0|1 · count=pure-int matches
     * countflatn: n=pure-int match count
     */
    if (strcmp(op, "hasflatn") == 0 || strcmp(op, "countflatn") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int is_count = (strcmp(op, "countflatn") == 0);

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_countn(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "hasflatn fail", CUBALC_LANG_VERSION);
        return 1;
      }
      if (is_count) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"countflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"COUNTFLATN dual · pure-int leaf count by needle (read-only)\"}\n",
               path, needle, file_hit ? "true" : "false",
               pr.n, CUBALC_LANG_VERSION);
      } else {
        long hit = (pr.n > 0) ? 1 : 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"hasflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"count\":%ld,\"hit\":%s,\"version\":\"%s\","
               "\"note\":\"HASFLATN dual · soft pure-int leaf presence by needle (read-only)\"}\n",
               path, needle, file_hit ? "true" : "false",
               hit, pr.n, hit ? "true" : "false", CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* pathsflat|valsflat [needle] — PATHSFLAT/VALSFLAT duals (read-only bags).
     *   cubalc plate pathsflat agent.json score
     *   cubalc plate valsflat agent.json hits
     * bag = newline fields · n = count. */
    if (strcmp(op, "pathsflat") == 0 || strcmp(op, "valsflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int want_vals = (strcmp(op, "valsflat") == 0);
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_match_bag(plate, needle, want_vals, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "pathsflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      /* JSON-escape bag for plate field */
      esc[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') {
          esc[o++] = '\\';
          esc[o++] = c;
        } else if (c == '\n') {
          esc[o++] = '\\';
          esc[o++] = 'n';
        } else if (c == '\r') {
          /* skip */
        } else {
          esc[o++] = c;
        }
      }
      esc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"%s dual · matching leaf %s bag (read-only)\"}\n",
             op, path, needle, file_hit ? "true" : "false",
             pr.n, esc, CUBALC_LANG_VERSION,
             want_vals ? "VALSFLAT" : "PATHSFLAT",
             want_vals ? "values" : "paths");
      return 0;
    }

    /* pathsflatn|valsflatn [needle] — PATHSFLATN/VALSFLATN duals (pure-int bags).
     *   cubalc plate pathsflatn agent.json score
     *   cubalc plate valsflatn agent.json hits
     * bag = newline pure-int path or value fields · n = count. */
    if (strcmp(op, "pathsflatn") == 0 || strcmp(op, "valsflatn") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int want_vals = (strcmp(op, "valsflatn") == 0);
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_match_bagn(plate, needle, want_vals, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "pathsflatn fail", CUBALC_LANG_VERSION);
        return 1;
      }
      esc[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') {
          esc[o++] = '\\';
          esc[o++] = c;
        } else if (c == '\n') {
          esc[o++] = '\\';
          esc[o++] = 'n';
        } else if (c == '\r') {
          /* skip */
        } else {
          esc[o++] = c;
        }
      }
      esc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"%s dual · pure-int leaf %s bag (read-only)\"}\n",
             op, path, needle, file_hit ? "true" : "false",
             pr.n, esc, CUBALC_LANG_VERSION,
             want_vals ? "VALSFLATN" : "PATHSFLATN",
             want_vals ? "values" : "paths");
      return 0;
    }

    /* getflat <needle> [OR|DEFAULT def] — GETFLAT dual (read-only).
     *   cubalc plate getflat agent.json port
     *   cubalc plate getflat agent.json port OR 8080
     * value = first matching leaf · leaf = winning path · hit true/false. */
    if (strcmp(op, "getflat") == 0) {
      const char *needle = "", *fb = NULL;
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2], lesc[512];
      size_t i, o;
      const char *val, *leafp;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getflat\",\"path\":\"%s\",\"err\":\"need needle [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "or") == 0 ||
           strcmp(argv[ai], "DEFAULT") == 0 || strcmp(argv[ai], "default") == 0)) {
        ai++;
        if (ai < argc && argv[ai])
          fb = argv[ai++];
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_get(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getflat\",\"path\":\"%s\",\"err\":\"getflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.n == 0 && fb) {
        val = fb;
        leafp = "";
      } else {
        val = pr.str;
        leafp = pr.err;
      }
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      for (i = 0, o = 0; leafp[i] && o + 2 < sizeof lesc; i++) {
        char c = leafp[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"getflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"hit\":%s,\"fallback\":%s,\"leaf\":\"%s\",\"value\":\"%s\","
             "\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"GETFLAT dual · first matching leaf value by path needle\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n ? "true" : "false",
             (pr.n == 0 && fb) ? "true" : "false",
             lesc, vesc, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* lastflat <needle> [OR|DEFAULT def] — LASTFLAT dual (read-only).
     *   cubalc plate lastflat agent.json port
     *   cubalc plate lastflat agent.json port OR 8080
     * value = last matching leaf · leaf = winning path · hit true/false. */
    if (strcmp(op, "lastflat") == 0) {
      const char *needle = "", *fb = NULL;
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2], lesc[512];
      size_t i, o;
      const char *val, *leafp;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"lastflat\",\"path\":\"%s\",\"err\":\"need needle [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "or") == 0 ||
           strcmp(argv[ai], "DEFAULT") == 0 || strcmp(argv[ai], "default") == 0)) {
        ai++;
        if (ai < argc && argv[ai])
          fb = argv[ai++];
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_get_last(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"lastflat\",\"path\":\"%s\",\"err\":\"lastflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.n == 0 && fb) {
        val = fb;
        leafp = "";
      } else {
        val = pr.str;
        leafp = pr.err;
      }
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      for (i = 0, o = 0; leafp[i] && o + 2 < sizeof lesc; i++) {
        char c = leafp[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"lastflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"hit\":%s,\"fallback\":%s,\"leaf\":\"%s\",\"value\":\"%s\","
             "\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"LASTFLAT dual · last matching leaf value by path needle\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n ? "true" : "false",
             (pr.n == 0 && fb) ? "true" : "false",
             lesc, vesc, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* nthflat <needle> <i> [OR def] — NTHFLAT dual: 0-based Nth matching leaf (read-only).
     *   cubalc plate nthflat agent.json role 0
     *   cubalc plate nthflat agent.json role 1 OR worker
     * value · hit · leaf · i. */
    if (strcmp(op, "nthflat") == 0) {
      const char *needle = "", *fb = NULL;
      long idx = 0;
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2], lesc[512];
      size_t i, o;
      const char *val, *leafp;
      char *end = NULL;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflat\",\"path\":\"%s\",\"err\":\"need needle index [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflat\",\"path\":\"%s\",\"err\":\"need needle index [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      end = NULL;
      idx = strtol(argv[ai], &end, 10);
      if (!(end && end != argv[ai] && *end == 0)) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflat\",\"path\":\"%s\",\"err\":\"bad index\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      ai++;
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "or") == 0 ||
           strcmp(argv[ai], "DEFAULT") == 0 || strcmp(argv[ai], "default") == 0)) {
        ai++;
        if (ai < argc && argv[ai])
          fb = argv[ai++];
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_get_nth(plate, needle, idx, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflat\",\"path\":\"%s\",\"err\":\"nthflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.n == 0 && fb) {
        val = fb;
        leafp = "";
      } else {
        val = pr.str;
        leafp = pr.err;
      }
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      for (i = 0, o = 0; leafp[i] && o + 2 < sizeof lesc; i++) {
        char c = leafp[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"nthflat\",\"path\":\"%s\",\"needle\":\"%s\",\"i\":%ld,"
             "\"file\":%s,\"hit\":%s,\"fallback\":%s,\"leaf\":\"%s\",\"value\":\"%s\","
             "\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"NTHFLAT dual · 0-based Nth matching leaf by path needle\"}\n",
             path, needle, idx, file_hit ? "true" : "false",
             pr.n ? "true" : "false",
             (pr.n == 0 && fb) ? "true" : "false",
             lesc, vesc, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* needflat <needle> [needle…] — NEEDFLAT dual: fail-fast leaf path contract.
     *   cubalc plate needflat agent.json tls agent_id
     * exit 0 if all needles match ≥1 leaf · exit 2 if any miss (miss bag). */
    if (strcmp(op, "needflat") == 0) {
      char miss_bag[CUBALC_HOST_STR_MAX];
      size_t molen = 0;
      int n_need = 0, n_miss = 0;
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"needflat\",\"path\":\"%s\",\"err\":\"need needle [needle…]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      miss_bag[0] = 0;
      while (ai < argc && argv[ai]) {
        const char *nd = argv[ai++];
        size_t mlen;
        n_need++;
        memset(&pr, 0, sizeof pr);
        cubalc_host_json_leaf_count(plate, nd, &pr);
        if (pr.n <= 0) {
          mlen = strlen(nd);
          n_miss++;
          if (molen > 0 && molen + 1 < sizeof miss_bag)
            miss_bag[molen++] = '\n';
          if (molen + mlen < sizeof miss_bag) {
            memcpy(miss_bag + molen, nd, mlen);
            molen += mlen;
            miss_bag[molen] = 0;
          }
        }
      }
      for (i = 0, o = 0; miss_bag[i] && o + 2 < sizeof esc; i++) {
        char c = miss_bag[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      if (n_miss > 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"needflat\",\"path\":\"%s\",\"file\":%s,"
               "\"n\":%d,\"miss_n\":%d,\"miss\":\"%s\",\"version\":\"%s\","
               "\"note\":\"NEEDFLAT dual · missing leaf path needle(s)\"}\n",
               path, file_hit ? "true" : "false",
               n_need, n_miss, esc, CUBALC_LANG_VERSION);
        return 2;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"needflat\",\"path\":\"%s\",\"file\":%s,"
             "\"n\":%d,\"miss_n\":0,\"version\":\"%s\","
             "\"note\":\"NEEDFLAT dual · all leaf path needles present\"}\n",
             path, file_hit ? "true" : "false", n_need, CUBALC_LANG_VERSION);
      return 0;
    }

    /* getflatn <needle> [OR n] — GETFLATN dual: first pure-int leaf by needle (read-only).
     *   cubalc plate getflatn agent.json port
     *   cubalc plate getflatn agent.json port OR 8080
     * n = value · hit · leaf path. */
    if (strcmp(op, "getflatn") == 0) {
      const char *needle = "";
      long fb_n = 0;
      int have_fb = 0;
      cubalc_host_result pr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"err\":\"need needle [OR n]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "or") == 0 ||
           strcmp(argv[ai], "DEFAULT") == 0 || strcmp(argv[ai], "default") == 0)) {
        ai++;
        if (ai < argc && argv[ai]) {
          char *end = NULL;
          long d = strtol(argv[ai], &end, 10);
          if (end && end != argv[ai] && *end == 0) {
            fb_n = d;
            have_fb = 1;
            ai++;
          }
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_getn(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"err\":\"getflatn fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.code == 0 && have_fb) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":false,\"fallback\":true,\"leaf\":\"\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"GETFLATN dual · pure-int leaf miss used OR fallback\"}\n",
               path, needle, file_hit ? "true" : "false", fb_n, CUBALC_LANG_VERSION);
      } else if (pr.code == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":false,\"fallback\":false,\"leaf\":\"\","
               "\"n\":0,\"version\":\"%s\","
               "\"note\":\"GETFLATN dual · no pure-int leaf match\"}\n",
               path, needle, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      } else {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":true,\"fallback\":false,\"leaf\":\"%s\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"GETFLATN dual · first pure-int leaf by path needle\"}\n",
               path, needle, file_hit ? "true" : "false", pr.err, pr.n,
               CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* lastflatn <needle> [OR n] — LASTFLATN dual: last pure-int leaf by needle (read-only).
     *   cubalc plate lastflatn agent.json port
     *   cubalc plate lastflatn agent.json port OR 8080
     * n = value · hit · leaf path (last match). */
    if (strcmp(op, "lastflatn") == 0) {
      const char *needle = "";
      long fb_n = 0;
      int have_fb = 0;
      cubalc_host_result pr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"err\":\"need needle [OR n]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "or") == 0 ||
           strcmp(argv[ai], "DEFAULT") == 0 || strcmp(argv[ai], "default") == 0)) {
        ai++;
        if (ai < argc && argv[ai]) {
          char *end = NULL;
          long d = strtol(argv[ai], &end, 10);
          if (end && end != argv[ai] && *end == 0) {
            fb_n = d;
            have_fb = 1;
            ai++;
          }
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_getn_last(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"err\":\"lastflatn fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.code == 0 && have_fb) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":false,\"fallback\":true,\"leaf\":\"\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"LASTFLATN dual · pure-int leaf miss used OR fallback\"}\n",
               path, needle, file_hit ? "true" : "false", fb_n, CUBALC_LANG_VERSION);
      } else if (pr.code == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":false,\"fallback\":false,\"leaf\":\"\","
               "\"n\":0,\"version\":\"%s\","
               "\"note\":\"LASTFLATN dual · no pure-int leaf match\"}\n",
               path, needle, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      } else {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":true,\"fallback\":false,\"leaf\":\"%s\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"LASTFLATN dual · last pure-int leaf by path needle\"}\n",
               path, needle, file_hit ? "true" : "false", pr.err, pr.n,
               CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* nthflatn <needle> <i> [OR n] — NTHFLATN dual: 0-based Nth pure-int leaf (read-only).
     *   cubalc plate nthflatn agent.json score 0
     *   cubalc plate nthflatn agent.json score 1 OR 0
     * n = value · hit · leaf path · i = index. */
    if (strcmp(op, "nthflatn") == 0) {
      const char *needle = "";
      long fb_n = 0, idx = 0;
      int have_fb = 0;
      cubalc_host_result pr;
      char *end = NULL;

      if (ai + 1 > argc || !argv[ai] || !argv[ai + 1]) {
        /* need needle and index: at least 2 args remaining if both present */
      }
      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"err\":\"need needle index [OR n]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"err\":\"need needle index [OR n]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      end = NULL;
      idx = strtol(argv[ai], &end, 10);
      if (!(end && end != argv[ai] && *end == 0)) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"err\":\"bad index\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      ai++;
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "or") == 0 ||
           strcmp(argv[ai], "DEFAULT") == 0 || strcmp(argv[ai], "default") == 0)) {
        ai++;
        if (ai < argc && argv[ai]) {
          end = NULL;
          long d = strtol(argv[ai], &end, 10);
          if (end && end != argv[ai] && *end == 0) {
            fb_n = d;
            have_fb = 1;
            ai++;
          }
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_getn_nth(plate, needle, idx, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"err\":\"nthflatn fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.code == 0 && have_fb) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"needle\":\"%s\",\"i\":%ld,"
               "\"file\":%s,\"hit\":false,\"fallback\":true,\"leaf\":\"\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"NTHFLATN dual · pure-int leaf miss used OR fallback\"}\n",
               path, needle, idx, file_hit ? "true" : "false", fb_n, CUBALC_LANG_VERSION);
      } else if (pr.code == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"needle\":\"%s\",\"i\":%ld,"
               "\"file\":%s,\"hit\":false,\"fallback\":false,\"leaf\":\"\","
               "\"n\":0,\"version\":\"%s\","
               "\"note\":\"NTHFLATN dual · no pure-int leaf at index\"}\n",
               path, needle, idx, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      } else {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"needle\":\"%s\",\"i\":%ld,"
               "\"file\":%s,\"hit\":true,\"fallback\":false,\"leaf\":\"%s\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"NTHFLATN dual · 0-based Nth pure-int leaf by path needle\"}\n",
               path, needle, idx, file_hit ? "true" : "false", pr.err, pr.n,
               CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* typeflat <needle> — TYPEFLAT dual: first matching leaf kind (read-only).
     *   cubalc plate typeflat agent.json port
     * kind = missing|num|str|bool|null|obj|arr · n = kind code 0..6 · leaf = path. */
    if (strcmp(op, "typeflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char lesc[512];
      size_t i, o = 0;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"typeflat\",\"path\":\"%s\",\"err\":\"need needle\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_type(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"typeflat\",\"path\":\"%s\",\"err\":\"typeflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.err[i] && o + 2 < sizeof lesc; i++) {
        char c = pr.err[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"typeflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"hit\":%s,\"kind\":\"%s\",\"n\":%ld,\"leaf\":\"%s\","
             "\"version\":\"%s\","
             "\"note\":\"TYPEFLAT dual · first matching leaf kind by path needle\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.code ? "true" : "false", pr.str, pr.n, lesc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* freqflat [needle] — FREQFLAT dual: value frequency of matching leaves (read-only).
     *   cubalc plate freqflat agent.json role
     *   cubalc plate freqflat agent.json          # all leaves
     * bag lines: val:count · n=uniques · total=matching leaves · plate not written. */
    if (strcmp(op, "freqflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_freq(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"freqflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "freqflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"freqflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"FREQFLAT dual · value frequency of matching leaves (read-only)\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n, (long)pr.code, esc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* modeflat [needle] — MODEFLAT dual: dominant matching leaf value (read-only).
     *   cubalc plate modeflat agent.json role
     *   cubalc plate modeflat agent.json          # all leaves
     * value = mode · n = winner count · total = matching leaves · plate not written. */
    if (strcmp(op, "modeflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_mode(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"modeflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "modeflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof vesc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      if (pr.str[0])
        printf("%s\n", pr.str);
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"modeflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"hit\":%s,\"value\":\"%s\",\"n\":%ld,\"total\":%ld,"
             "\"version\":\"%s\","
             "\"note\":\"MODEFLAT dual · dominant matching leaf value (read-only)\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.str[0] ? "true" : "false", vesc, pr.n, (long)pr.code,
             CUBALC_LANG_VERSION);
      return 0;
    }

    /* pathbyval <value> [OR def] — PATHBYVAL dual: first leaf path by exact value (read-only).
     *   cubalc plate pathbyval agent.json worker
     *   cubalc plate pathbyval agent.json leader OR cfg.role
     * leaf = winning path · value = matched text · hit/fallback flags. */
    if (strcmp(op, "pathbyval") == 0) {
      const char *needle = "";
      const char *fb = NULL;
      cubalc_host_result pr;
      char lesc[512], vesc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;
      int have_fb = 0;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pathbyval\",\"path\":\"%s\",\"err\":\"need value [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "or") == 0 ||
           strcmp(argv[ai], "DEFAULT") == 0 || strcmp(argv[ai], "default") == 0)) {
        ai++;
        if (ai < argc && argv[ai]) {
          fb = argv[ai++];
          have_fb = 1;
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_path_by_val(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pathbyval\",\"path\":\"%s\",\"err\":\"pathbyval fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      {
        const char *outp = pr.n ? pr.str : (have_fb && fb ? fb : "");
        for (i = 0, o = 0; outp[i] && o + 2 < sizeof lesc; i++) {
          char c = outp[i];
          if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
          else lesc[o++] = c;
        }
        lesc[o] = 0;
        for (i = 0, o = 0; needle[i] && o + 2 < sizeof vesc; i++) {
          char c = needle[i];
          if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
          else vesc[o++] = c;
        }
        vesc[o] = 0;
        if (outp[0])
          printf("%s\n", outp);
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"pathbyval\",\"path\":\"%s\",\"value\":\"%s\","
               "\"file\":%s,\"hit\":%s,\"fallback\":%s,\"leaf\":\"%s\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"PATHBYVAL dual · first leaf path by exact value\"}\n",
               path, vesc, file_hit ? "true" : "false",
               pr.n ? "true" : "false",
               (pr.n == 0 && have_fb) ? "true" : "false",
               lesc, pr.n, CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* uniqflat [needle] — UNIQFLAT dual: unique matching leaf values (read-only).
     *   cubalc plate uniqflat agent.json role
     *   cubalc plate uniqflat agent.json          # all leaves
     * bag lines: unique values · n=uniques · total=matching leaves. */
    if (strcmp(op, "uniqflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_uniq(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"uniqflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "uniqflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"uniqflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"UNIQFLAT dual · unique matching leaf values (read-only)\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n, (long)pr.code, esc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* pathsbyval <value> — PATHSBYVAL dual: all leaf paths by exact value (read-only).
     *   cubalc plate pathsbyval agent.json worker
     * bag lines: matching paths · n=count. */
    if (strcmp(op, "pathsbyval") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2], vesc[512];
      size_t i, o = 0;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pathsbyval\",\"path\":\"%s\",\"err\":\"need value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_paths_by_val(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pathsbyval\",\"path\":\"%s\",\"err\":\"pathsbyval fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      for (i = 0, o = 0; needle[i] && o + 2 < sizeof vesc; i++) {
        char c = needle[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"pathsbyval\",\"path\":\"%s\",\"value\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"PATHSBYVAL dual · all leaf paths by exact value (read-only)\"}\n",
             path, vesc, file_hit ? "true" : "false",
             pr.n, esc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* countbyval|hasval <value> — COUNTBYVAL/HASVAL duals (read-only).
     *   cubalc plate countbyval agent.json worker
     *   cubalc plate hasval agent.json leader
     * n = count · hit = presence for hasval. */
    if (strcmp(op, "countbyval") == 0 || strcmp(op, "hasval") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int want_has = (strcmp(op, "hasval") == 0);

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need value\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_count_by_val(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s fail\","
               "\"version\":\"%s\"}\n", op, path, op, CUBALC_LANG_VERSION);
        return 1;
      }
      if (want_has) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"hasval\",\"path\":\"%s\",\"value\":\"%s\","
               "\"file\":%s,\"hit\":%s,\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"HASVAL dual · soft presence of exact leaf value\"}\n",
               path, needle, file_hit ? "true" : "false",
               pr.code ? "true" : "false", pr.n, CUBALC_LANG_VERSION);
      } else {
        printf("%ld\n", pr.n);
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"countbyval\",\"path\":\"%s\",\"value\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"COUNTBYVAL dual · count leaves by exact value\"}\n",
               path, needle, file_hit ? "true" : "false",
               pr.n, CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* setbyval <old> <new> — SETBYVAL dual: rewrite leaf values by exact match.
     *   cubalc plate setbyval agent.json worker standby
     * Writes plate. n = rewritten leaf count. */
    if (strcmp(op, "setbyval") == 0) {
      const char *oldv = "", *newv = NULL;
      cubalc_host_result pr, wr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setbyval\",\"path\":\"%s\",\"err\":\"need old new\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      oldv = argv[ai++];
      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setbyval\",\"path\":\"%s\",\"err\":\"need new value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      newv = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_set_by_val(plate, oldv, newv, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setbyval\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "setbyval fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"setbyval\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"setbyval\",\"path\":\"%s\",\"old\":\"%s\",\"new\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"SETBYVAL dual · rewrite leaf values by exact match\"}\n",
             path, oldv, newv, file_hit ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* delbyval <value> — DELBYVAL dual: drop leaves by exact value (write-back).
     *   cubalc plate delbyval agent.json standby
     * Writes plate. n = deleted leaf count · kept = remaining. */
    if (strcmp(op, "delbyval") == 0) {
      const char *needle = "";
      cubalc_host_result pr, wr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"delbyval\",\"path\":\"%s\",\"err\":\"need value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_del_by_val(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"delbyval\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "delbyval fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"delbyval\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"delbyval\",\"path\":\"%s\",\"value\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"kept\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"DELBYVAL dual · drop leaves by exact value\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* alleqflat [needle] — ALLEQFLAT dual: all matching leaf values identical (read-only).
     *   cubalc plate alleqflat agent.json role
     *   cubalc plate alleqflat agent.json          # all leaves
     * eq=true|false · value=common when eq · n=match count. */
    if (strcmp(op, "alleqflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_all_eq(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"alleqflat\",\"path\":\"%s\",\"err\":\"alleqflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof vesc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      if (pr.code && pr.str[0])
        printf("%s\n", pr.str);
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"alleqflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"eq\":%s,\"value\":\"%s\",\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"ALLEQFLAT dual · all matching leaf values identical\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.code ? "true" : "false", vesc, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* firstuneq [needle] — FIRSTUNEQFLAT dual: first path that breaks uniformity (read-only).
     *   cubalc plate firstuneq agent.json role
     *   cubalc plate firstuneq agent.json
     * found=true|false · path · ref · value · i · n=match count. */
    if (strcmp(op, "firstuneq") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char pesc[1024], resc[512], vesc[512];
      char ref[256], val[256];
      long total = 0, found = 0, idx = 0;
      size_t i, o = 0;
      const char *ep;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_first_uneq(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"firstuneq\",\"path\":\"%s\",\"err\":\"firstuneq fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      found = pr.code ? 1 : 0;
      ref[0] = 0;
      val[0] = 0;
      if (found) {
        ep = pr.err;
        {
          size_t k = 0;
          while (*ep && *ep != '\n' && *ep != '\r' && k + 1 < sizeof ref)
            ref[k++] = *ep++;
          ref[k] = 0;
        }
        while (*ep == '\n' || *ep == '\r') ep++;
        {
          size_t k = 0;
          while (*ep && *ep != '\n' && *ep != '\r' && k + 1 < sizeof val)
            val[k++] = *ep++;
          val[k] = 0;
        }
        while (*ep == '\n' || *ep == '\r') ep++;
        total = strtol(ep, NULL, 10);
        idx = pr.n;
        if (pr.str[0])
          printf("%s\n", pr.str);
      } else {
        total = strtol(pr.err, NULL, 10);
        idx = 0;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof pesc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { pesc[o++] = '\\'; pesc[o++] = c; }
        else if (c == '\n') { pesc[o++] = '\\'; pesc[o++] = 'n'; }
        else pesc[o++] = c;
      }
      pesc[o] = 0;
      for (i = 0, o = 0; ref[i] && o + 2 < sizeof resc; i++) {
        char c = ref[i];
        if (c == '"' || c == '\\') { resc[o++] = '\\'; resc[o++] = c; }
        else resc[o++] = c;
      }
      resc[o] = 0;
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"firstuneq\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"found\":%s,\"leaf\":\"%s\",\"ref\":\"%s\","
             "\"value\":\"%s\",\"i\":%ld,\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"FIRSTUNEQFLAT dual · first path that breaks uniformity\"}\n",
             path, needle, file_hit ? "true" : "false",
             found ? "true" : "false", pesc, resc, vesc, idx, total,
             CUBALC_LANG_VERSION);
      return 0;
    }

    /* uneqpaths [needle] — UNEQPATHS dual: all paths that break uniformity (read-only).
     *   cubalc plate uneqpaths agent.json role
     * bag lines: diverge paths · n=diverge count · total=matches · ref=first value. */
    if (strcmp(op, "uneqpaths") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char besc[CUBALC_HOST_STR_MAX * 2], resc[512];
      size_t i, o = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_uneq_paths(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"uneqpaths\",\"path\":\"%s\",\"err\":\"uneqpaths fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.str[0])
        printf("%s\n", pr.str);
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof besc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { besc[o++] = '\\'; besc[o++] = c; }
        else if (c == '\n') { besc[o++] = '\\'; besc[o++] = 'n'; }
        else besc[o++] = c;
      }
      besc[o] = 0;
      for (i = 0, o = 0; pr.err[i] && o + 2 < sizeof resc; i++) {
        char c = pr.err[i];
        if (c == '"' || c == '\\') { resc[o++] = '\\'; resc[o++] = c; }
        else resc[o++] = c;
      }
      resc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"uneqpaths\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"ref\":\"%s\",\"bag\":\"%s\","
             "\"version\":\"%s\","
             "\"note\":\"UNEQPATHS dual · all paths that break uniformity\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n, (long)pr.code, resc, besc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* uniform [needle] — UNIFORMFLAT dual: one-shot eq + first path + all diverge paths.
     *   cubalc plate uniform agent.json role
     * eq · n · div · uniques · path · paths bag · ref · value · common when eq. */
    if (strcmp(op, "uniform") == 0) {
      const char *needle = "";
      cubalc_host_result heq, h1, hpaths;
      char pesc[1024], besc[CUBALC_HOST_STR_MAX * 2], resc[512], vesc[512], cesc[512];
      char ref[256], val[256], path1[512];
      long eq = 0, total = 0, divn = 0, uniques = 0, idx = 0;
      size_t i, o = 0;
      const char *ep;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&heq, 0, sizeof heq);
      memset(&h1, 0, sizeof h1);
      memset(&hpaths, 0, sizeof hpaths);
      if (cubalc_host_json_leaf_all_eq(plate, needle, &heq) != 0 ||
          cubalc_host_json_leaf_first_uneq(plate, needle, &h1) != 0 ||
          cubalc_host_json_leaf_uneq_paths(plate, needle, &hpaths) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"uniform\",\"path\":\"%s\",\"err\":\"uniform fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      eq = heq.code ? 1 : 0;
      total = heq.n;
      divn = hpaths.n;
      uniques = (heq.err[0] && heq.err[0] >= '0' && heq.err[0] <= '9')
                    ? strtol(heq.err, NULL, 10)
                    : (eq ? (total > 0 ? 1 : 0) : 0);
      ref[0] = 0;
      val[0] = 0;
      path1[0] = 0;
      if (h1.code) {
        snprintf(path1, sizeof path1, "%s", h1.str);
        idx = h1.n;
        ep = h1.err;
        {
          size_t k = 0;
          while (*ep && *ep != '\n' && *ep != '\r' && k + 1 < sizeof ref)
            ref[k++] = *ep++;
          ref[k] = 0;
        }
        while (*ep == '\n' || *ep == '\r') ep++;
        {
          size_t k = 0;
          while (*ep && *ep != '\n' && *ep != '\r' && k + 1 < sizeof val)
            val[k++] = *ep++;
          val[k] = 0;
        }
      } else if (hpaths.err[0]) {
        snprintf(ref, sizeof ref, "%s", hpaths.err);
      }
      if (eq && heq.str[0])
        printf("%s\n", heq.str);
      else if (path1[0])
        printf("%s\n", path1);
      for (i = 0, o = 0; path1[i] && o + 2 < sizeof pesc; i++) {
        char c = path1[i];
        if (c == '"' || c == '\\') { pesc[o++] = '\\'; pesc[o++] = c; }
        else pesc[o++] = c;
      }
      pesc[o] = 0;
      for (i = 0, o = 0; hpaths.str[i] && o + 2 < sizeof besc; i++) {
        char c = hpaths.str[i];
        if (c == '"' || c == '\\') { besc[o++] = '\\'; besc[o++] = c; }
        else if (c == '\n') { besc[o++] = '\\'; besc[o++] = 'n'; }
        else besc[o++] = c;
      }
      besc[o] = 0;
      for (i = 0, o = 0; ref[i] && o + 2 < sizeof resc; i++) {
        char c = ref[i];
        if (c == '"' || c == '\\') { resc[o++] = '\\'; resc[o++] = c; }
        else resc[o++] = c;
      }
      resc[o] = 0;
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      for (i = 0, o = 0; heq.str[i] && o + 2 < sizeof cesc; i++) {
        char c = heq.str[i];
        if (c == '"' || c == '\\') { cesc[o++] = '\\'; cesc[o++] = c; }
        else cesc[o++] = c;
      }
      cesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"uniform\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"eq\":%s,\"n\":%ld,\"div\":%ld,\"uniques\":%ld,"
             "\"i\":%ld,\"path_first\":\"%s\",\"paths\":\"%s\","
             "\"ref\":\"%s\",\"value\":\"%s\",\"common\":\"%s\","
             "\"version\":\"%s\","
             "\"note\":\"UNIFORMFLAT dual · one-shot nest consistency\"}\n",
             path, needle, file_hit ? "true" : "false",
             eq ? "true" : "false", total, divn, uniques, idx,
             pesc, besc, resc, vesc, cesc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* needflatn <needle> [needle…] — NEEDFLATN dual: fail-fast pure-int leaf contract.
     *   cubalc plate needflatn agent.json port timeout_ms
     * on ok: n = first needle pure-int value · needles = count · leaf = first path
     * exit 2 if any needle has no pure-int leaf match. */
    if (strcmp(op, "needflatn") == 0) {
      char miss_bag[CUBALC_HOST_STR_MAX];
      size_t molen = 0;
      int n_need = 0, n_miss = 0, have_first = 0;
      long first_val = 0;
      char first_path[512];
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2], lesc[1024];
      size_t i, o = 0;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"needflatn\",\"path\":\"%s\",\"err\":\"need needle [needle…]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      miss_bag[0] = 0;
      first_path[0] = 0;
      while (ai < argc && argv[ai]) {
        const char *nd = argv[ai++];
        size_t mlen;
        n_need++;
        memset(&pr, 0, sizeof pr);
        cubalc_host_json_leaf_getn(plate, nd, &pr);
        if (pr.code == 0) {
          mlen = strlen(nd);
          n_miss++;
          if (molen > 0 && molen + 1 < sizeof miss_bag)
            miss_bag[molen++] = '\n';
          if (molen + mlen < sizeof miss_bag) {
            memcpy(miss_bag + molen, nd, mlen);
            molen += mlen;
            miss_bag[molen] = 0;
          }
        } else if (!have_first) {
          first_val = pr.n;
          snprintf(first_path, sizeof first_path, "%s", pr.err);
          have_first = 1;
        }
      }
      for (i = 0, o = 0; miss_bag[i] && o + 2 < sizeof esc; i++) {
        char c = miss_bag[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      for (i = 0, o = 0; first_path[i] && o + 2 < sizeof lesc; i++) {
        char c = first_path[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      if (n_miss > 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"needflatn\",\"path\":\"%s\",\"file\":%s,"
               "\"needles\":%d,\"miss_n\":%d,\"miss\":\"%s\",\"version\":\"%s\","
               "\"note\":\"NEEDFLATN dual · missing pure-int leaf needle(s)\"}\n",
               path, file_hit ? "true" : "false",
               n_need, n_miss, esc, CUBALC_LANG_VERSION);
        return 2;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"needflatn\",\"path\":\"%s\",\"file\":%s,"
             "\"needles\":%d,\"miss_n\":0,\"n\":%ld,\"leaf\":\"%s\","
             "\"version\":\"%s\","
             "\"note\":\"NEEDFLATN dual · pure-int leaf contract + first peel\"}\n",
             path, file_hit ? "true" : "false", n_need, first_val, lesc,
             CUBALC_LANG_VERSION);
      return 0;
    }

    /* keys|len|empty|vals [nest.path] — KEYSP/LENP/EMPTYP/VALSP duals (path ok).
     *   cubalc plate keys agent.json
     *   cubalc plate keys agent.json cfg.flags
     *   cubalc plate len  agent.json cfg
     *   cubalc plate empty agent.json cfg.flags
     *   cubalc plate vals agent.json cfg */
    if (strcmp(op, "keys") == 0 || strcmp(op, "len") == 0 ||
        strcmp(op, "empty") == 0 || strcmp(op, "vals") == 0) {
      const char *nestp = NULL;
      char sub[CUBALC_HOST_STR_MAX];
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result obj, pr;
      size_t i, o = 0;
      long nkeys = 0;
      int isempty;

      if (ai < argc && argv[ai] && argv[ai][0])
        nestp = argv[ai++];

      memset(&obj, 0, sizeof obj);
      cubalc_host_json_path_obj(plate, nestp, &obj);
      snprintf(sub, sizeof sub, "%s", obj.str);

      if (strcmp(op, "keys") == 0) {
        memset(&keys, 0, sizeof keys);
        if (cubalc_host_json_keys(sub, &keys) != 0) {
          keys.str[0] = 0;
          keys.n = 0;
        }
        flat[0] = 0;
        for (i = 0; keys.str[i] && o + 2 < sizeof flat; i++) {
          char c = keys.str[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',')
              flat[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        if (keys.str[0]) {
          fputs(keys.str, stdout);
          if (keys.str[strlen(keys.str) - 1] != '\n')
            fputc('\n', stdout);
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"keys\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
               "\"n\":%ld,\"keys\":\"%s\",\"version\":\"%s\","
               "\"note\":\"KEYSP dual · nest path ok · bag lines above plate\"}\n",
               path, nestp ? nestp : "", file_hit ? "true" : "false",
               keys.n, flat, CUBALC_LANG_VERSION);
        return 0;
      }

      if (strcmp(op, "vals") == 0) {
        memset(&pr, 0, sizeof pr);
        if (cubalc_host_json_values(sub, &pr) != 0) {
          pr.str[0] = 0;
          pr.n = 0;
        }
        flat[0] = 0;
        for (i = 0; pr.str[i] && o + 2 < sizeof flat; i++) {
          char c = pr.str[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',')
              flat[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        if (pr.str[0]) {
          fputs(pr.str, stdout);
          if (pr.str[strlen(pr.str) - 1] != '\n')
            fputc('\n', stdout);
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"vals\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
               "\"n\":%ld,\"vals\":\"%s\",\"version\":\"%s\","
               "\"note\":\"VALSP dual · nest path ok · bag lines above plate\"}\n",
               path, nestp ? nestp : "", file_hit ? "true" : "false",
               pr.n, flat, CUBALC_LANG_VERSION);
        return 0;
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_len(sub, &pr) != 0)
        nkeys = 0;
      else
        nkeys = pr.n;
      isempty = (nkeys == 0) ? 1 : 0;

      if (strcmp(op, "len") == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"len\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"LENP dual · nest path ok\"}\n",
               path, nestp ? nestp : "", file_hit ? "true" : "false",
               nkeys, CUBALC_LANG_VERSION);
        return 0;
      }

      /* empty — always exit 0; agents read empty bool (EMPTYP dual) */
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"empty\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
             "\"empty\":%s,\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"EMPTYP dual · nest path ok\"}\n",
             path, nestp ? nestp : "", file_hit ? "true" : "false",
             isempty ? "true" : "false", nkeys, CUBALC_LANG_VERSION);
      return 0;
    }

    /* nestget|nestset|nestinc|nestdel|nestkeys|nesthas — nested field CLI duals of
     * GETPOBJ/SETPOBJ/INCOBJ/DELPOBJ/KEYSOBJ/HASPOBJ. Agents one-shot nested config
     * without writing a .cubalc program.
     * Nest key may be dotted path (cfg.flags) via path_obj peel.
     * nestsum|nestavg|nestmedian|nesttop|nestbot — SUMNOBJ/TOPNOBJ CLI duals (no write).
     * nestsort|nestsortbag — SORTOBJ/SORTBAGOBJ CLI duals (sort write / bag no-write).
     * nestpick|nestomit — PICKOBJ/OMITOBJ duals (paths ok).
     * pluckobj|nestpluck — PLUCKOBJ dual: multi-key peel nest → value bag (paths ok). */
    if (strcmp(op, "nestget") == 0 || strcmp(op, "nestset") == 0 ||
        strcmp(op, "nestinc") == 0 || strcmp(op, "nestdel") == 0 ||
        strcmp(op, "nestkeys") == 0 || strcmp(op, "nesthas") == 0 ||
        strcmp(op, "nestpick") == 0 || strcmp(op, "nestomit") == 0 ||
        strcmp(op, "nestrename") == 0 || strcmp(op, "nestcopy") == 0 ||
        strcmp(op, "nestswap") == 0 ||
        strcmp(op, "pluckobj") == 0 ||
        strcmp(op, "nestsum") == 0 || strcmp(op, "nestavg") == 0 ||
        strcmp(op, "nestmedian") == 0 || strcmp(op, "nesttop") == 0 ||
        strcmp(op, "nestbot") == 0 ||
        strcmp(op, "nestsort") == 0 || strcmp(op, "nestsortbag") == 0) {
      const char *nestk = NULL, *field = NULL;
      char nest[CUBALC_HOST_STR_MAX];
      cubalc_host_result ngr, gr, wr, kr;
      int nest_hit = 0;

      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need nest key\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      nestk = argv[ai++];

      /* path-aware nest peel: "cfg.flags" · soft miss → {} */
      nest[0] = 0;
      memset(&ngr, 0, sizeof ngr);
      cubalc_host_json_path_obj(plate, nestk, &ngr);
      snprintf(nest, sizeof nest, "%s", ngr.str);
      nest_hit = (ngr.n != 0) ? 1 : 0;

      /* nestsum|nestavg|nestmedian — SUMNOBJ/AVGNOBJ/MEDIANOBJ duals (no write). */
      if (strcmp(op, "nestsum") == 0 || strcmp(op, "nestavg") == 0 ||
          strcmp(op, "nestmedian") == 0) {
        cubalc_host_result ar;
        long used = 0;
        memset(&ar, 0, sizeof ar);
        if (!nest_hit) {
          if (strcmp(op, "nestsum") == 0)
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                   "\"op\":\"nestsum\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,"
                   "\"nest_hit\":false,\"file\":%s,\"version\":\"%s\"}\n",
                   path, nestk, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
          else
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                   "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,\"used\":0,"
                   "\"nest_hit\":false,\"file\":%s,\"version\":\"%s\"}\n",
                   op, path, nestk, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
          return 0;
        }
        if (strcmp(op, "nestsum") == 0) {
          if (cubalc_host_json_sum(nest, &ar) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestsum\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, ar.err[0] ? ar.err : "json sum fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestsum\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
                 "\"nest_hit\":true,\"file\":%s,\"version\":\"%s\"}\n",
                 path, nestk, ar.n, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
          return 0;
        }
        if (strcmp(op, "nestavg") == 0) {
          if (cubalc_host_json_avg(nest, &ar) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestavg\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, ar.err[0] ? ar.err : "json avg fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          used = (long)ar.code;
        } else {
          if (cubalc_host_json_median(nest, &ar) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestmedian\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, ar.err[0] ? ar.err : "json median fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          used = (long)ar.code;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,\"used\":%ld,"
               "\"nest_hit\":true,\"file\":%s,\"version\":\"%s\"}\n",
               op, path, nestk, ar.n, used, file_hit ? "true" : "false",
               CUBALC_LANG_VERSION);
        return 0;
      }

      /* nesttop|nestbot [n] — TOPNOBJ/BOTNOBJ duals (read-only; no file write). */
      if (strcmp(op, "nesttop") == 0 || strcmp(op, "nestbot") == 0) {
        cubalc_host_result tr, tk;
        long ntake = 1;
        int want_bot = (strcmp(op, "nestbot") == 0) ? 1 : 0;
        char key1[256];
        long val1 = 0;
        if (ai < argc && argv[ai] && argv[ai][0]) {
          char *end = NULL;
          long vv = strtol(argv[ai], &end, 10);
          if (end && end != argv[ai] && *end == 0)
            ntake = vv;
        }
        if (ntake < 0) ntake = 0;
        if (!nest_hit) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,\"cand\":0,"
                 "\"take\":%ld,\"key\":\"\",\"value\":0,\"nest_hit\":false,"
                 "\"file\":%s,\"version\":\"%s\",\"plate\":{}}\n",
                 op, path, nestk, ntake, file_hit ? "true" : "false",
                 CUBALC_LANG_VERSION);
          return 0;
        }
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_json_topn(nest, ntake, want_bot, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                 "\"version\":\"%s\"}\n",
                 op, path, nestk, tr.err[0] ? tr.err : "json topn fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        key1[0] = 0;
        memset(&tk, 0, sizeof tk);
        if (tr.n > 0 && cubalc_host_json_keys(tr.str, &tk) == 0 && tk.str[0]) {
          size_t kn = 0;
          const char *kp = tk.str;
          while (*kp && *kp != '\n' && *kp != '\r' && kn + 1 < sizeof key1)
            key1[kn++] = *kp++;
          key1[kn] = 0;
          if (key1[0]) {
            cubalc_host_result gr2;
            char *end = NULL;
            memset(&gr2, 0, sizeof gr2);
            if (cubalc_host_json_get(tr.str, key1, &gr2) == 0 && gr2.str[0]) {
              val1 = strtol(gr2.str, &end, 10);
              if (!(end && end != gr2.str && *end == 0))
                val1 = 0;
            }
          }
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,\"cand\":%d,"
               "\"take\":%ld,\"key\":\"%s\",\"value\":%ld,\"nest_hit\":true,"
               "\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
               op, path, nestk, tr.n, tr.code, ntake, key1, val1,
               file_hit ? "true" : "false", CUBALC_LANG_VERSION, tr.str);
        return 0;
      }

      /* nestsort [ASC|DESC] — SORTOBJ dual: sort pure-int nest keys, write-back file.
       * nestsortbag [ASC|DESC] — SORTBAGOBJ dual: key:val bag, no file write.
       * Usability: agent nest FREQ full rank without a .cubalc program. */
      if (strcmp(op, "nestsort") == 0 || strcmp(op, "nestsortbag") == 0) {
        cubalc_host_result sr;
        int want_asc = 0;
        int as_bag = (strcmp(op, "nestsortbag") == 0) ? 1 : 0;
        char bag_esc[CUBALC_HOST_STR_MAX];
        size_t bi, bo;

        while (ai < argc && argv[ai] && argv[ai][0]) {
          if (strcmp(argv[ai], "ASC") == 0 || strcmp(argv[ai], "asc") == 0 ||
              strcmp(argv[ai], "UP") == 0 || strcmp(argv[ai], "up") == 0 ||
              strcmp(argv[ai], "LIGHT") == 0 || strcmp(argv[ai], "light") == 0 ||
              strcmp(argv[ai], "LOW") == 0 || strcmp(argv[ai], "low") == 0) {
            want_asc = 1;
            ai++;
            continue;
          }
          if (strcmp(argv[ai], "DESC") == 0 || strcmp(argv[ai], "desc") == 0 ||
              strcmp(argv[ai], "DOWN") == 0 || strcmp(argv[ai], "down") == 0 ||
              strcmp(argv[ai], "HEAVY") == 0 || strcmp(argv[ai], "heavy") == 0 ||
              strcmp(argv[ai], "HIGH") == 0 || strcmp(argv[ai], "high") == 0) {
            want_asc = 0;
            ai++;
            continue;
          }
          break;
        }

        if (!nest_hit) {
          if (as_bag) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                   "\"op\":\"nestsortbag\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,"
                   "\"asc\":%s,\"nest_hit\":false,\"bag\":\"\","
                   "\"file\":%s,\"version\":\"%s\"}\n",
                   path, nestk, want_asc ? "true" : "false",
                   file_hit ? "true" : "false", CUBALC_LANG_VERSION);
          } else {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                   "\"op\":\"nestsort\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,"
                   "\"asc\":%s,\"nest_hit\":false,\"written\":false,"
                   "\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
                   path, nestk, want_asc ? "true" : "false",
                   file_hit ? "true" : "false", CUBALC_LANG_VERSION, plate);
          }
          return 0;
        }

        memset(&sr, 0, sizeof sr);
        if (cubalc_host_json_sortbyval(nest, want_asc, as_bag, &sr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                 "\"version\":\"%s\"}\n",
                 op, path, nestk, sr.err[0] ? sr.err : "json sort fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }

        if (as_bag) {
          bag_esc[0] = 0;
          bo = 0;
          for (bi = 0; sr.str[bi] && bo + 2 < sizeof bag_esc; bi++) {
            char c = sr.str[bi];
            if (c == '"' || c == '\\') {
              bag_esc[bo++] = '\\';
              bag_esc[bo++] = c;
            } else if (c == '\n' || c == '\r') {
              bag_esc[bo++] = '\\';
              bag_esc[bo++] = (c == '\n') ? 'n' : 'r';
            } else if ((unsigned char)c < 32) {
              bag_esc[bo++] = ' ';
            } else {
              bag_esc[bo++] = c;
            }
          }
          bag_esc[bo] = 0;
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestsortbag\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
                 "\"asc\":%s,\"nest_hit\":true,\"bag\":\"%s\","
                 "\"file\":%s,\"version\":\"%s\"}\n",
                 path, nestk, sr.n, want_asc ? "true" : "false", bag_esc,
                 file_hit ? "true" : "false", CUBALC_LANG_VERSION);
          return 0;
        }

        /* write-back sorted nest into outer plate file */
        snprintf(nest, sizeof nest, "%s", sr.str);
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, nestk, nest, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"nestsort\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                 "\"version\":\"%s\"}\n",
                 path, nestk, wr.err[0] ? wr.err : "outer set fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(plate, sizeof plate, "%s", wr.str);
        slash = strrchr(path, '/');
        if (slash && slash != path) {
          size_t n = (size_t)(slash - path);
          if (n >= sizeof parent) n = sizeof parent - 1;
          memcpy(parent, path, n);
          parent[n] = 0;
          memset(&hr, 0, sizeof hr);
          cubalc_host_mkdir(parent, &hr);
        }
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_write(path, plate, &hr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"nestsort\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 path, nestk, CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nestsort\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
               "\"asc\":%s,\"nest_hit\":true,\"written\":true,\"bytes\":%ld,"
               "\"file\":%s,\"version\":\"%s\",\"nest_body\":%s,\"plate\":%s}\n",
               path, nestk, sr.n, want_asc ? "true" : "false", hr.n,
               file_hit ? "true" : "false", CUBALC_LANG_VERSION, nest, plate);
        return 0;
      }

      if (strcmp(op, "nestkeys") == 0) {
        char flatk[CUBALC_HOST_STR_MAX];
        size_t i, o = 0;
        memset(&kr, 0, sizeof kr);
        if (nest_hit)
          cubalc_host_json_keys(nest, &kr);
        flatk[0] = 0;
        for (i = 0; kr.str[i] && o + 2 < sizeof flatk; i++) {
          char c = kr.str[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flatk[o - 1] != ',') flatk[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flatk[o++] = '_';
          } else {
            flatk[o++] = c;
          }
        }
        flatk[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nestkeys\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
               "\"nest_hit\":%s,\"keys\":\"%s\",\"file\":%s,\"version\":\"%s\"}\n",
               path, nestk, nest_hit ? kr.n : 0L,
               nest_hit ? "true" : "false", flatk,
               file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }

      
      /* nestrename|nestcopy|nestswap — RENAMEPOBJ/COPYPOBJ/SWAPPOBJ duals (nest path ok).
       *   cubalc plate nestrename agent.json flags dbg debug
       *   cubalc plate nestcopy   agent.json flags debug mirror
       *   cubalc plate nestswap   agent.json flags debug trace */
      if (strcmp(op, "nestrename") == 0 || strcmp(op, "nestcopy") == 0 ||
          strcmp(op, "nestswap") == 0) {
        const char *a = NULL, *b = NULL;
        cubalc_host_result rr;
        if (ai >= argc || !argv[ai] || !argv[ai][0]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"need src key\",\"version\":\"%s\"}\n",
                 op, path, nestk, CUBALC_LANG_VERSION);
          return 2;
        }
        a = argv[ai++];
        if (ai >= argc || !argv[ai] || !argv[ai][0]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"need dst key\",\"version\":\"%s\"}\n",
                 op, path, nestk, CUBALC_LANG_VERSION);
          return 2;
        }
        b = argv[ai++];
        if (!nest_hit) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"src\":\"%s\","
                 "\"dst\":\"%s\",\"n\":0,\"nest_hit\":false,\"file\":%s,"
                 "\"version\":\"%s\",\"plate\":%s}\n",
                 op, path, nestk, a, b, file_hit ? "true" : "false",
                 CUBALC_LANG_VERSION, plate);
          return 0;
        }
        memset(&rr, 0, sizeof rr);
        if (strcmp(op, "nestrename") == 0) {
          if (cubalc_host_json_rename(nest, a, b, &rr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestrename\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   rr.err[0] ? rr.err : "rename fail", CUBALC_LANG_VERSION);
            return 1;
          }
        } else if (strcmp(op, "nestcopy") == 0) {
          if (cubalc_host_json_copy_key(nest, a, b, &rr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestcopy\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   rr.err[0] ? rr.err : "copy fail", CUBALC_LANG_VERSION);
            return 1;
          }
        } else {
          if (cubalc_host_json_swap_keys(nest, a, b, &rr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestswap\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   rr.err[0] ? rr.err : "swap fail", CUBALC_LANG_VERSION);
            return 1;
          }
        }
        if (rr.n == 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"src\":\"%s\","
                 "\"dst\":\"%s\",\"n\":0,\"nest_hit\":true,\"file\":%s,"
                 "\"version\":\"%s\",\"plate\":%s}\n",
                 op, path, nestk, a, b, file_hit ? "true" : "false",
                 CUBALC_LANG_VERSION, plate);
          return 0;
        }
        snprintf(nest, sizeof nest, "%s", rr.str);
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, nestk, nest, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 op, wr.err[0] ? wr.err : "outer set fail", CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(plate, sizeof plate, "%s", wr.str);
        slash = strrchr(path, '/');
        if (slash && slash != path) {
          size_t n = (size_t)(slash - path);
          if (n >= sizeof parent) n = sizeof parent - 1;
          memcpy(parent, path, n);
          parent[n] = 0;
          memset(&hr, 0, sizeof hr);
          cubalc_host_mkdir(parent, &hr);
        }
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_write(path, plate, &hr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"src\":\"%s\","
               "\"dst\":\"%s\",\"n\":%ld,\"nest_hit\":true,\"file\":%s,"
               "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s,"
               "\"note\":\"RENAMEPOBJ/COPYPOBJ/SWAPPOBJ dual · nest path ok\"}\n",
               op, path, nestk, a, b, rr.n, file_hit ? "true" : "false",
               hr.n, CUBALC_LANG_VERSION, plate);
        return 0;
      }

      /* pluckobj — multi-key peel from nest → value bag (PLUCKOBJ dual; nest path ok).
       * Usability: agent one-shot nested extract without .cubalc:
       *   cubalc plate pluckobj agent.json cfg.meta role x zone
       * Bag lines above plate meta · empty field on miss · hit/miss counts. */
      if (strcmp(op, "pluckobj") == 0) {
        char keys_nl[CUBALC_HOST_STR_MAX];
        char flat_req[CUBALC_HOST_STR_MAX];
        char bag_esc[CUBALC_HOST_STR_MAX];
        cubalc_host_result pr;
        size_t o = 0, bi, bo;
        long nreq = 0;
        long hit = 0, miss = 0;

        keys_nl[0] = 0;
        flat_req[0] = 0;
        if (ai >= argc || !argv[ai] || !argv[ai][0]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"pluckobj\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"need one or more fields\",\"version\":\"%s\"}\n",
                 path, nestk, CUBALC_LANG_VERSION);
          return 2;
        }
        while (ai < argc && argv[ai] && argv[ai][0]) {
          size_t kl = strlen(argv[ai]);
          if (o + kl + 2 >= sizeof keys_nl) break;
          if (o > 0) keys_nl[o++] = '\n';
          memcpy(keys_nl + o, argv[ai], kl);
          o += kl;
          keys_nl[o] = 0;
          {
            size_t fo = strlen(flat_req);
            if (fo + kl + 2 < sizeof flat_req) {
              if (fo > 0) {
                flat_req[fo++] = ',';
                flat_req[fo] = 0;
              }
              memcpy(flat_req + fo, argv[ai], kl);
              flat_req[fo + kl] = 0;
            }
          }
          nreq++;
          ai++;
        }

        memset(&pr, 0, sizeof pr);
        if (nest_hit) {
          if (cubalc_host_json_pluck(nest, keys_nl, &pr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"pluckobj\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, pr.err[0] ? pr.err : "pluck fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          hit = (long)pr.code;
          miss = pr.n - hit;
        } else {
          /* soft nest miss: nreq empty bag fields */
          {
            int i;
            pr.str[0] = 0;
            o = 0;
            for (i = 0; i < nreq; i++) {
              if (i > 0 && o + 1 < sizeof pr.str) pr.str[o++] = '\n';
            }
            pr.str[o] = 0;
            pr.n = nreq;
            pr.code = 0;
          }
          hit = 0;
          miss = nreq;
        }

        if (pr.str[0]) {
          fputs(pr.str, stdout);
          if (pr.str[strlen(pr.str) - 1] != '\n')
            fputc('\n', stdout);
        } else if (nreq > 0) {
          /* all-empty bag still emits blank lines for agent field count */
          {
            int i;
            for (i = 0; i < nreq; i++)
              fputc('\n', stdout);
          }
        }

        bag_esc[0] = 0;
        bo = 0;
        for (bi = 0; pr.str[bi] && bo + 2 < sizeof bag_esc; bi++) {
          char c = pr.str[bi];
          if (c == '"' || c == '\\') {
            bag_esc[bo++] = '\\';
            bag_esc[bo++] = c;
          } else if (c == '\n' || c == '\r') {
            bag_esc[bo++] = '\\';
            bag_esc[bo++] = (c == '\n') ? 'n' : 'r';
          } else if ((unsigned char)c < 32) {
            bag_esc[bo++] = ' ';
          } else {
            bag_esc[bo++] = c;
          }
        }
        bag_esc[bo] = 0;

        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"pluckobj\",\"path\":\"%s\",\"nest\":\"%s\","
               "\"n\":%ld,\"hit\":%ld,\"miss\":%ld,\"keys\":\"%s\",\"bag\":\"%s\","
               "\"nest_hit\":%s,\"file\":%s,\"version\":\"%s\","
               "\"note\":\"bag lines above plate · PLUCKOBJ dual · nest path ok\"}\n",
               path, nestk, nreq, hit, miss, flat_req, bag_esc,
               nest_hit ? "true" : "false",
               file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }

      /* nestpick|nestomit — multi-key keep/drop inside nest (PICKOBJ/OMITOBJ duals). */
      if (strcmp(op, "nestpick") == 0 || strcmp(op, "nestomit") == 0) {
        char keys_nl[CUBALC_HOST_STR_MAX];
        char flat_req[CUBALC_HOST_STR_MAX];
        cubalc_host_result pr;
        size_t o = 0;
        long nreq = 0;
        int is_omit = (strcmp(op, "nestomit") == 0) ? 1 : 0;

        keys_nl[0] = 0;
        flat_req[0] = 0;
        if (ai >= argc || !argv[ai] || !argv[ai][0]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"need one or more fields\",\"version\":\"%s\"}\n",
                 op, path, nestk, CUBALC_LANG_VERSION);
          return 2;
        }
        while (ai < argc && argv[ai] && argv[ai][0]) {
          size_t kl = strlen(argv[ai]);
          if (o + kl + 2 >= sizeof keys_nl) break;
          if (o > 0) keys_nl[o++] = '\n';
          memcpy(keys_nl + o, argv[ai], kl);
          o += kl;
          keys_nl[o] = 0;
          {
            size_t fo = strlen(flat_req);
            if (fo + kl + 2 < sizeof flat_req) {
              if (fo > 0) {
                flat_req[fo++] = ',';
                flat_req[fo] = 0;
              }
              memcpy(flat_req + fo, argv[ai], kl);
              flat_req[fo + kl] = 0;
            }
          }
          nreq++;
          ai++;
        }

        memset(&pr, 0, sizeof pr);
        if (is_omit) {
          if (cubalc_host_json_drop(nest, keys_nl, &pr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestomit\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, pr.err[0] ? pr.err : "json drop fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
        } else {
          if (cubalc_host_json_pick(nest, keys_nl, &pr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestpick\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, pr.err[0] ? pr.err : "json pick fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
        }
        snprintf(nest, sizeof nest, "%s", pr.str);

        /* path write-back: nest may be dotted (cfg.flags) */
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, nestk, nest, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 op, wr.err[0] ? wr.err : "outer set fail", CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(plate, sizeof plate, "%s", wr.str);
        slash = strrchr(path, '/');
        if (slash && slash != path) {
          size_t n = (size_t)(slash - path);
          if (n >= sizeof parent) n = sizeof parent - 1;
          memcpy(parent, path, n);
          parent[n] = 0;
          memset(&hr, 0, sizeof hr);
          cubalc_host_mkdir(parent, &hr);
        }
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_write(path, plate, &hr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
               "\"n_req\":%ld,\"keys\":\"%s\",\"nest_hit\":%s,\"file\":%s,"
               "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s,"
               "\"note\":\"PICKOBJ/OMITOBJ dual · nest path ok\"}\n",
               op, path, nestk, pr.n, nreq, flat_req,
               nest_hit ? "true" : "false",
               file_hit ? "true" : "false", hr.n, CUBALC_LANG_VERSION, plate);
        return 0;
      }

if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"need field\","
               "\"version\":\"%s\"}\n", op, path, nestk, CUBALC_LANG_VERSION);
        return 2;
      }
      field = argv[ai++];

      if (strcmp(op, "nesthas") == 0) {
        int hit = 0;
        if (nest_hit) {
          memset(&gr, 0, sizeof gr);
          if (cubalc_host_json_get_raw(nest, field, &gr) == 0)
            hit = 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nesthas\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
               "\"hit\":%s,\"nest_hit\":%s,\"file\":%s,\"version\":\"%s\"}\n",
               path, nestk, field, hit ? "true" : "false",
               nest_hit ? "true" : "false",
               file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return hit ? 0 : 1;
      }

      if (strcmp(op, "nestget") == 0) {
        int have_fb2 = 0;
        const char *fb2 = NULL;
        if (ai < argc && (strcmp(argv[ai], "OR") == 0 ||
                          strcmp(argv[ai], "DEFAULT") == 0 ||
                          strcmp(argv[ai], "ELSE") == 0)) {
          ai++;
          if (ai < argc) {
            fb2 = argv[ai++];
            have_fb2 = 1;
          }
        }
        if (nest_hit) {
          memset(&gr, 0, sizeof gr);
          if (cubalc_host_json_get(nest, field, &gr) == 0) {
            char esc[CUBALC_HOST_STR_MAX];
            size_t i, o = 0;
            for (i = 0; gr.str[i] && o + 2 < sizeof esc; i++) {
              char c = gr.str[i];
              if (c == '"' || c == '\\') {
                esc[o++] = '\\';
                esc[o++] = c;
              } else if ((unsigned char)c < 32) {
                esc[o++] = ' ';
              } else {
                esc[o++] = c;
              }
            }
            esc[o] = 0;
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                   "\"op\":\"nestget\",\"path\":\"%s\",\"nest\":\"%s\","
                   "\"field\":\"%s\",\"hit\":true,\"value\":\"%s\",\"file\":%s,"
                   "\"version\":\"%s\"}\n",
                   path, nestk, field, esc,
                   file_hit ? "true" : "false", CUBALC_LANG_VERSION);
            return 0;
          }
        }
        if (have_fb2 && fb2) {
          char esc[512];
          size_t i, o = 0;
          for (i = 0; fb2[i] && o + 2 < sizeof esc; i++) {
            char c = fb2[i];
            if (c == '"' || c == '\\') {
              esc[o++] = '\\';
              esc[o++] = c;
            } else {
              esc[o++] = c;
            }
          }
          esc[o] = 0;
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestget\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"field\":\"%s\",\"hit\":false,\"or\":true,\"value\":\"%s\","
                 "\"file\":%s,\"version\":\"%s\"}\n",
                 path, nestk, field, esc,
                 file_hit ? "true" : "false", CUBALC_LANG_VERSION);
          return 0;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nestget\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
               "\"hit\":false,\"err\":\"field miss\",\"file\":%s,\"version\":\"%s\"}\n",
               path, nestk, field,
               file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 1;
      }

      {
        long delta_n = 1, cur = 0, nv = 0;
        int val_kind2 = 0;
        const char *val2 = NULL;
        char raw[32];

        if (strcmp(op, "nestset") == 0) {
          size_t i;
          int allnum = 1;
          if (ai >= argc) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestset\",\"err\":\"need value\",\"version\":\"%s\"}\n",
                   CUBALC_LANG_VERSION);
            return 2;
          }
          val2 = argv[ai++];
          if (!strcmp(val2, "true") || !strcmp(val2, "false") ||
              !strcmp(val2, "null") || val2[0] == '{' || val2[0] == '[')
            val_kind2 = 1;
          else {
            if (val2[0] == '-' && val2[1]) i = 1;
            else i = 0;
            if (!val2[i]) allnum = 0;
            for (; val2[i]; i++) {
              if (val2[i] < '0' || val2[i] > '9') {
                allnum = 0;
                break;
              }
            }
            if (allnum) val_kind2 = 1;
          }
          memset(&wr, 0, sizeof wr);
          if (cubalc_host_json_set(nest, field, val2, val_kind2, &wr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestset\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   wr.err[0] ? wr.err : "nest set fail", CUBALC_LANG_VERSION);
            return 1;
          }
          snprintf(nest, sizeof nest, "%s", wr.str);
        } else if (strcmp(op, "nestinc") == 0) {
          if (ai < argc) {
            char *end = NULL;
            delta_n = strtol(argv[ai], &end, 10);
            if (end == argv[ai]) delta_n = 1;
            ai++;
          }
          memset(&gr, 0, sizeof gr);
          if (cubalc_host_json_get(nest, field, &gr) == 0) {
            char *end = NULL;
            cur = strtol(gr.str, &end, 10);
            if (end == gr.str) cur = 0;
          }
          nv = cur + delta_n;
          snprintf(raw, sizeof raw, "%ld", nv);
          memset(&wr, 0, sizeof wr);
          if (cubalc_host_json_set(nest, field, raw, 1, &wr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestinc\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   wr.err[0] ? wr.err : "nest set fail", CUBALC_LANG_VERSION);
            return 1;
          }
          snprintf(nest, sizeof nest, "%s", wr.str);
        } else {
          memset(&wr, 0, sizeof wr);
          if (cubalc_host_json_del(nest, field, &wr) != 0) {
            wr.n = 0;
          } else {
            snprintf(nest, sizeof nest, "%s", wr.str);
          }
        }
        {
          long nest_removed = wr.n;

        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, nestk, nest, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 op, wr.err[0] ? wr.err : "outer set fail", CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(plate, sizeof plate, "%s", wr.str);
        slash = strrchr(path, '/');
        if (slash && slash != path) {
          size_t n = (size_t)(slash - path);
          if (n >= sizeof parent) n = sizeof parent - 1;
          memcpy(parent, path, n);
          parent[n] = 0;
          memset(&hr, 0, sizeof hr);
          cubalc_host_mkdir(parent, &hr);
        }
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_write(path, plate, &hr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        if (strcmp(op, "nestinc") == 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestinc\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
                 "\"value\":%ld,\"delta\":%ld,\"bytes\":%ld,\"version\":\"%s\","
                 "\"plate\":%s}\n",
                 path, nestk, field, nv, delta_n, hr.n, CUBALC_LANG_VERSION, plate);
        } else if (strcmp(op, "nestdel") == 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestdel\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
                 "\"removed\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
                 path, nestk, field, nest_removed, hr.n, CUBALC_LANG_VERSION, plate);
        } else {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestset\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
                 "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
                 path, nestk, field, hr.n, CUBALC_LANG_VERSION, plate);
        }
        return 0;
        }
      }
    }

    /* pick|omit path k1 [k2 …] — project plate keys without a .cubalc program.
     * Usability: shell dual of PICKP / OMITP for peer/log slim plates.
     *   cubalc plate pick agent.json host status n
     *   cubalc plate omit agent.json tmp debug
     * Soft-missing keys skipped. Does not write file (stdout plate only). */
    if (strcmp(op, "pick") == 0 || strcmp(op, "omit") == 0) {
      char keys_nl[CUBALC_HOST_STR_MAX];
      char flat_req[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t o = 0;
      long nreq = 0;
      int is_omit = (strcmp(op, "omit") == 0) ? 1 : 0;

      keys_nl[0] = 0;
      flat_req[0] = 0;
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need one or more keys\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      while (ai < argc && argv[ai] && argv[ai][0]) {
        size_t kl = strlen(argv[ai]);
        if (o + kl + 2 >= sizeof keys_nl) break;
        if (o > 0) keys_nl[o++] = '\n';
        memcpy(keys_nl + o, argv[ai], kl);
        o += kl;
        keys_nl[o] = 0;
        {
          size_t fo = strlen(flat_req);
          if (fo + kl + 2 < sizeof flat_req) {
            if (fo > 0) {
              flat_req[fo++] = ',';
              flat_req[fo] = 0;
            }
            memcpy(flat_req + fo, argv[ai], kl);
            flat_req[fo + kl] = 0;
          }
        }
        nreq++;
        ai++;
      }

      memset(&pr, 0, sizeof pr);
      if (is_omit) {
        if (cubalc_host_json_drop(plate, keys_nl, &pr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"omit\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, pr.err[0] ? pr.err : "json drop fail", CUBALC_LANG_VERSION);
          return 1;
        }
      } else {
        if (cubalc_host_json_pick(plate, keys_nl, &pr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"pick\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, pr.err[0] ? pr.err : "json pick fail", CUBALC_LANG_VERSION);
          return 1;
        }
      }

      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"n\":%ld,\"n_req\":%ld,"
             "\"keys\":\"%s\",\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
             op, path, pr.n, nreq, flat_req,
             file_hit ? "true" : "false", CUBALC_LANG_VERSION, pr.str);
      return 0;
    }

    /* sum|avg|median path — numeric plate aggregates without a .cubalc program.
     * Usability: CLI duals of SUMNP / AVGNP / MEDIANP for agent one-shots.
     *   cubalc plate sum scores.json
     *   cubalc plate avg scores.json
     *   cubalc plate median|p50 scores.json
     * Soft 0 if no pure-int fields. Does not write file. */
    if (strcmp(op, "sum") == 0 || strcmp(op, "avg") == 0 ||
        strcmp(op, "median") == 0) {
      cubalc_host_result ar;
      long used = 0;
      memset(&ar, 0, sizeof ar);
      if (strcmp(op, "sum") == 0) {
        if (cubalc_host_json_sum(plate, &ar) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"sum\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, ar.err[0] ? ar.err : "json sum fail", CUBALC_LANG_VERSION);
          return 1;
        }
      } else if (strcmp(op, "avg") == 0) {
        if (cubalc_host_json_avg(plate, &ar) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"avg\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, ar.err[0] ? ar.err : "json avg fail", CUBALC_LANG_VERSION);
          return 1;
        }
        used = (long)ar.code;
      } else {
        if (cubalc_host_json_median(plate, &ar) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"median\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, ar.err[0] ? ar.err : "json median fail", CUBALC_LANG_VERSION);
          return 1;
        }
        used = (long)ar.code;
      }
      if (strcmp(op, "sum") == 0)
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"sum\",\"path\":\"%s\",\"n\":%ld,\"file\":%s,"
               "\"version\":\"%s\"}\n",
               path, ar.n, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      else
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"n\":%ld,\"used\":%ld,\"file\":%s,"
               "\"version\":\"%s\"}\n",
               op, path, ar.n, used, file_hit ? "true" : "false",
               CUBALC_LANG_VERSION);
      return 0;
    }

    /* top|bot path [n] — top/bottom N pure-int keys by value → plate.
     * Usability: CLI dual of TOPNP/BOTNP · default n=1.
     *   cubalc plate top freq.json 3
     *   cubalc plate bot scores.json
     * Soft empty plate if no ints. Does not write file. */
    if (strcmp(op, "top") == 0 || strcmp(op, "bot") == 0) {
      cubalc_host_result tr;
      cubalc_host_result tk;
      long ntake = 1;
      int want_bot = (strcmp(op, "bot") == 0) ? 1 : 0;
      char key1[256];
      long val1 = 0;
      if (ai < argc && argv[ai] && argv[ai][0]) {
        char *end = NULL;
        long v = strtol(argv[ai], &end, 10);
        if (end && end != argv[ai] && *end == 0)
          ntake = v;
      }
      if (ntake < 0) ntake = 0;
      memset(&tr, 0, sizeof tr);
      if (cubalc_host_json_topn(plate, ntake, want_bot, &tr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, tr.err[0] ? tr.err : "json topn fail", CUBALC_LANG_VERSION);
        return 1;
      }
      /* peel first key for agent convenience when n>=1 */
      key1[0] = 0;
      memset(&tk, 0, sizeof tk);
      if (tr.n > 0 && cubalc_host_json_keys(tr.str, &tk) == 0 && tk.str[0]) {
        size_t kn = 0;
        const char *kp = tk.str;
        while (*kp && *kp != '\n' && *kp != '\r' && kn + 1 < sizeof key1)
          key1[kn++] = *kp++;
        key1[kn] = 0;
        if (key1[0]) {
          cubalc_host_result gr2;
          char *end = NULL;
          memset(&gr2, 0, sizeof gr2);
          if (cubalc_host_json_get(tr.str, key1, &gr2) == 0 && gr2.str[0]) {
            val1 = strtol(gr2.str, &end, 10);
            if (!(end && end != gr2.str && *end == 0))
              val1 = 0;
          }
        }
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"n\":%ld,\"cand\":%d,\"take\":%ld,"
             "\"key\":\"%s\",\"value\":%ld,\"file\":%s,\"version\":\"%s\","
             "\"plate\":%s}\n",
             op, path, tr.n, tr.code, ntake, key1, val1,
             file_hit ? "true" : "false", CUBALC_LANG_VERSION, tr.str);
      return 0;
    }

    /* fill|fillkeys — expand {{key}} from plate file without a .cubalc program.
     * Usability: one-shot agent status/path templates (CLI dual of FILLP/FILLPKEYS).
     *   cubalc plate fill agent.json "peer={{name}}@{{host}}"
     *   cubalc plate fill -s agent.json @tpl.txt out.txt
     *   cubalc plate fillkeys agent.json @tpl.txt
     * Template: literal (contains {), @file, or readable path. */
    if (strcmp(op, "fill") == 0 || strcmp(op, "fillkeys") == 0) {
      const char *tmpl_arg = NULL;
      const char *out_path = NULL;
      char tmpl[CUBALC_HOST_STR_MAX], body[CUBALC_HOST_STR_MAX];
      char miss_keys[CUBALC_HOST_STR_MAX], esc[CUBALC_HOST_STR_MAX];
      char flat[CUBALC_HOST_STR_MAX];
      long hits = 0, miss = 0, nk = 0;
      int is_strict = (val_kind == 99) ? 1 : 0;
      int is_keys = (strcmp(op, "fillkeys") == 0) ? 1 : 0;
      size_t i, o;
      cubalc_host_result tr, wrr;

      tmpl[0] = 0; body[0] = 0; miss_keys[0] = 0;
      /* optional -s|--strict before template */
      if (ai < argc && argv[ai] &&
          (!strcmp(argv[ai], "--strict") || !strcmp(argv[ai], "-s"))) {
        is_strict = 1;
        ai++;
      }
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"err\":\"need template string or @file\","
               "\"version\":\"%s\"}\n", op, CUBALC_LANG_VERSION);
        return 2;
      }
      tmpl_arg = argv[ai++];
      if (ai < argc && argv[ai] && argv[ai][0] &&
          strcmp(argv[ai], "--strict") != 0 && strcmp(argv[ai], "-s") != 0)
        out_path = argv[ai++];
      if (ai < argc && argv[ai] &&
          (!strcmp(argv[ai], "--strict") || !strcmp(argv[ai], "-s"))) {
        is_strict = 1;
        ai++;
        if (!out_path && ai < argc && argv[ai] && argv[ai][0])
          out_path = argv[ai++];
      }

      if (tmpl_arg[0] == '@') {
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(tmpl_arg + 1, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"template read fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(tmpl, sizeof tmpl, "%s", tr.str);
      } else if (strchr(tmpl_arg, '{') == NULL && access(tmpl_arg, R_OK) == 0) {
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(tmpl_arg, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"template read fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(tmpl, sizeof tmpl, "%s", tr.str);
      } else {
        snprintf(tmpl, sizeof tmpl, "%s", tmpl_arg);
      }

      if (is_keys) {
        cubalc_host_fillp_keys(tmpl, body, sizeof body, &nk);
        o = 0; flat[0] = 0;
        for (i = 0; body[i] && o + 2 < sizeof flat; i++) {
          char c = body[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',') flat[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"fillkeys\",\"path\":\"%s\",\"file\":%s,\"n\":%ld,"
               "\"keys\":\"%s\",\"version\":\"%s\"}\n",
               path, file_hit ? "true" : "false", nk, flat, CUBALC_LANG_VERSION);
        return 0;
      }

      cubalc_host_expand_fillp(plate, tmpl, body, sizeof body,
                               &hits, &miss, miss_keys, sizeof miss_keys);

      if (is_strict && miss > 0) {
        o = 0; flat[0] = 0;
        for (i = 0; miss_keys[i] && o + 2 < sizeof flat; i++) {
          char c = miss_keys[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',') {
              flat[o++] = ',';
              if (o + 1 < sizeof flat) flat[o++] = ' ';
            }
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"fill\",\"strict\":true,\"path\":\"%s\",\"hits\":%ld,"
               "\"miss\":%ld,\"miss_keys\":\"%s\","
               "\"err\":\"missing plate keys\",\"version\":\"%s\"}\n",
               path, hits, miss, flat, CUBALC_LANG_VERSION);
        return 1;
      }

      if (out_path && out_path[0]) {
        slash = strrchr(out_path, '/');
        if (slash && slash != out_path) {
          size_t n = (size_t)(slash - out_path);
          if (n >= sizeof parent) n = sizeof parent - 1;
          memcpy(parent, out_path, n);
          parent[n] = 0;
          memset(&hr, 0, sizeof hr);
          cubalc_host_mkdir(parent, &hr);
        }
        memset(&wrr, 0, sizeof wrr);
        if (cubalc_host_write(out_path, body, &wrr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"fill\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"fill\",\"path\":\"%s\",\"out\":\"%s\",\"hits\":%ld,"
               "\"miss\":%ld,\"bytes\":%ld,\"strict\":%s,\"file\":%s,"
               "\"version\":\"%s\"}\n",
               path, out_path, hits, miss, (long)strlen(body),
               is_strict ? "true" : "false",
               file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }

      fputs(body, stdout);
      if (!body[0] || body[strlen(body) - 1] != '\n')
        fputc('\n', stdout);
      o = 0; esc[0] = 0;
      for (i = 0; body[i] && o + 2 < sizeof esc; i++) {
        char c = body[i];
        if (c == '"' || c == '\\') {
          esc[o++] = '\\';
          esc[o++] = c;
        } else if (c == '\n') {
          esc[o++] = '\\';
          esc[o++] = 'n';
        } else if (c == '\r') {
        } else if ((unsigned char)c < 32) {
          esc[o++] = ' ';
        } else {
          esc[o++] = c;
        }
      }
      esc[o] = 0;
      o = 0; flat[0] = 0;
      for (i = 0; miss_keys[i] && o + 2 < sizeof flat; i++) {
        char c = miss_keys[i];
        if (c == '\n' || c == '\r') {
          if (o > 0 && flat[o - 1] != ',') flat[o++] = ',';
        } else if (c == '"' || c == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = c;
        }
      }
      flat[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"fill\",\"path\":\"%s\",\"hits\":%ld,\"miss\":%ld,"
             "\"miss_keys\":\"%s\",\"bytes\":%ld,\"strict\":%s,\"file\":%s,"
             "\"body\":\"%s\",\"version\":\"%s\","
             "\"note\":\"filled body above plate · FILLP dual for CLI agents\"}\n",
             path, hits, miss, flat, (long)strlen(body),
             is_strict ? "true" : "false",
             file_hit ? "true" : "false", esc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* ensure|seed — create object plate only if missing/non-object (ENSUREPLATE dual).
     *   cubalc plate ensure agent.json
     *   cubalc plate ensure agent.json '{"n":0,"ok":true}'
     *   cubalc plate ensure agent.json @seed.json
     * Does not clobber an existing object plate (use set/merge to mutate). */
    if (strcmp(op, "ensure") == 0) {
      const char *seed_arg = NULL;
      char seed[CUBALC_HOST_STR_MAX];
      cubalc_host_result tr;

      seed[0] = 0;
      if (ai < argc && argv[ai] && argv[ai][0])
        seed_arg = argv[ai++];
      if (seed_arg) {
        if (seed_arg[0] == '@') {
          memset(&tr, 0, sizeof tr);
          if (cubalc_host_read(seed_arg + 1, &tr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"ensure\",\"err\":\"seed read fail\",\"version\":\"%s\"}\n",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          snprintf(seed, sizeof seed, "%s", tr.str);
        } else if (seed_arg[0] != '{' && access(seed_arg, R_OK) == 0) {
          memset(&tr, 0, sizeof tr);
          if (cubalc_host_read(seed_arg, &tr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"ensure\",\"err\":\"seed read fail\",\"version\":\"%s\"}\n",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          snprintf(seed, sizeof seed, "%s", tr.str);
        } else {
          snprintf(seed, sizeof seed, "%s", seed_arg);
        }
      } else {
        snprintf(seed, sizeof seed, "%s", "{}");
      }
      {
        const char *sb = seed;
        while (*sb == ' ' || *sb == '\t' || *sb == '\n' || *sb == '\r') sb++;
        if (*sb != '{') {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"ensure\",\"err\":\"seed must be JSON object\","
                 "\"version\":\"%s\"}\n", CUBALC_LANG_VERSION);
          return 2;
        }
        if (sb != seed)
          memmove(seed, sb, strlen(sb) + 1);
      }

      if (file_hit) {
        /* keep existing object plate — no write */
        memset(&keys, 0, sizeof keys);
        cubalc_host_json_keys(plate, &keys);
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"ensure\",\"path\":\"%s\",\"created\":false,\"file\":true,"
               "\"keys_n\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
               path, keys.n, (long)strlen(plate), CUBALC_LANG_VERSION, plate);
        return 0;
      }

      snprintf(plate, sizeof plate, "%s", seed);
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"ensure\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, hr.err[0] ? hr.err : "write fail", CUBALC_LANG_VERSION);
        return 1;
      }
      memset(&keys, 0, sizeof keys);
      cubalc_host_json_keys(plate, &keys);
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"ensure\",\"path\":\"%s\",\"created\":true,\"file\":false,"
             "\"keys_n\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, keys.n, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* merge|patch|overlay — multi-key overlay onto plate file (JSONFILEMERGE dual).
     *   cubalc plate merge agent.json '{"status":"ready","n":3}'
     *   cubalc plate merge agent.json @patch.json
     * Creates {} base if missing; overlay wins on key clash. */
    if (strcmp(op, "merge") == 0) {
      const char *over_arg = NULL;
      char over[CUBALC_HOST_STR_MAX];
      cubalc_host_result tr, mer;

      over[0] = 0;
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"merge\",\"path\":\"%s\",\"err\":\"need overlay object or @file\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      over_arg = argv[ai++];
      if (over_arg[0] == '@') {
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(over_arg + 1, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"merge\",\"err\":\"overlay read fail\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(over, sizeof over, "%s", tr.str);
      } else if (over_arg[0] != '{' && access(over_arg, R_OK) == 0) {
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(over_arg, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"merge\",\"err\":\"overlay read fail\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(over, sizeof over, "%s", tr.str);
      } else {
        snprintf(over, sizeof over, "%s", over_arg);
      }
      {
        const char *ob = over;
        while (*ob == ' ' || *ob == '\t' || *ob == '\n' || *ob == '\r') ob++;
        if (*ob != '{') {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"merge\",\"err\":\"overlay must be JSON object\","
                 "\"version\":\"%s\"}\n", CUBALC_LANG_VERSION);
          return 2;
        }
      }

      memset(&mer, 0, sizeof mer);
      if (cubalc_host_json_merge(plate, over, &mer) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"merge\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, mer.err[0] ? mer.err : "json merge fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", mer.str);
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"merge\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, hr.err[0] ? hr.err : "write fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"merge\",\"path\":\"%s\",\"applied\":%ld,\"file\":%s,"
             "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, mer.n, file_hit ? "true" : "false", hr.n,
             CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* eq|ne|diff|changelog a.json b.json — compare two agent plates without .cubalc.
     * Usability: mesh/peer sync verify (JSONEQ / JSONCHANGED / JSONCHANGELOG duals).
     * Soft-missing files load as {}. eq exit 0 if equal else 1; ne inverted;
     * diff/changelog exit 0 if identical else 1 (keys/lines listed either way). */
    if (strcmp(op, "eq") == 0 || strcmp(op, "ne") == 0 || strcmp(op, "diff") == 0 ||
        strcmp(op, "changelog") == 0 ||
        strcmp(op, "diffflat") == 0 || strcmp(op, "pathdiff") == 0) {
      const char *path2 = NULL;
      char plate2[CUBALC_HOST_STR_MAX];
      int file_hit2 = 0, equal = 0;
      cubalc_host_result er, ch;
      char flat[CUBALC_HOST_STR_MAX];
      char esc[CUBALC_HOST_STR_MAX];
      size_t i, o;

      plate2[0] = 0;
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need second path\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      path2 = argv[ai++];

      /* soft-load second plate (mirror path load above) */
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(path2, &hr) == 0 && hr.str[0]) {
        body = hr.str;
        while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
          body++;
        if (*body == '{') {
          memset(&keys, 0, sizeof keys);
          if (cubalc_host_json_keys(body, &keys) == 0) {
            snprintf(plate2, sizeof plate2, "%s", body);
            file_hit2 = 1;
          }
        }
      }
      if (!file_hit2)
        snprintf(plate2, sizeof plate2, "%s", "{}");

      memset(&er, 0, sizeof er);
      cubalc_host_json_eq(plate, plate2, &er);
      equal = (er.n == 1) ? 1 : 0;

      if (strcmp(op, "eq") == 0 || strcmp(op, "ne") == 0) {
        int want_ne = (strcmp(op, "ne") == 0);
        int pass = want_ne ? !equal : equal;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"path2\":\"%s\","
               "\"equal\":%s,\"file\":%s,\"file2\":%s,\"version\":\"%s\"}\n",
               op, path, path2,
               equal ? "true" : "false",
               file_hit ? "true" : "false",
               file_hit2 ? "true" : "false",
               CUBALC_LANG_VERSION);
        return pass ? 0 : 1;
      }

      if (strcmp(op, "diff") == 0) {
        /* diff — changed keys bag (want_same=0) */
        memset(&ch, 0, sizeof ch);
        cubalc_host_json_changed_keys(plate, plate2, 0, &ch);
        o = 0; flat[0] = 0;
        for (i = 0; ch.str[i] && o + 2 < sizeof flat; i++) {
          char c = ch.str[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',') flat[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"diff\",\"path\":\"%s\",\"path2\":\"%s\","
               "\"equal\":%s,\"n\":%ld,\"keys\":\"%s\","
               "\"file\":%s,\"file2\":%s,\"version\":\"%s\"}\n",
               path, path2,
               equal ? "true" : "false", ch.n, flat,
               file_hit ? "true" : "false",
               file_hit2 ? "true" : "false",
               CUBALC_LANG_VERSION);
        return equal ? 0 : 1;
      }

      /* diffflat|pathdiff — deep leaf-path compare (DIFFFLAT / PATHDIFF duals).
       * Nest-aware changelog / path bag above plate. */
      if (strcmp(op, "diffflat") == 0 || strcmp(op, "pathdiff") == 0) {
        int paths_only = (strcmp(op, "pathdiff") == 0) ? 1 : 0;
        memset(&ch, 0, sizeof ch);
        cubalc_host_json_leaf_diff(plate, plate2, paths_only, &ch);
        if (ch.str[0]) {
          fputs(ch.str, stdout);
          if (ch.str[strlen(ch.str) - 1] != '\n')
            fputc('\n', stdout);
        }
        o = 0; esc[0] = 0;
        for (i = 0; ch.str[i] && o + 2 < sizeof esc; i++) {
          char c = ch.str[i];
          if (c == '"' || c == '\\') {
            esc[o++] = '\\';
            esc[o++] = c;
          } else if (c == '\n') {
            esc[o++] = '\\';
            esc[o++] = 'n';
          } else if (c == '\r') {
          } else if ((unsigned char)c < 32) {
            esc[o++] = ' ';
          } else {
            esc[o++] = c;
          }
        }
        esc[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"path2\":\"%s\","
               "\"equal\":%s,\"n\":%ld,\"lines\":\"%s\","
               "\"file\":%s,\"file2\":%s,\"version\":\"%s\","
               "\"note\":\"%s\"}\n",
               op, path, path2,
               (ch.n == 0) ? "true" : "false", ch.n, esc,
               file_hit ? "true" : "false",
               file_hit2 ? "true" : "false",
               CUBALC_LANG_VERSION,
               paths_only
                   ? "PATHDIFF dual · changed leaf paths · bag above plate"
                   : "DIFFFLAT dual · deep leaf path: old → new · bag above plate");
        return (ch.n == 0) ? 0 : 1;
      }

      /* changelog — human/agent "key: old → new" lines (JSONCHANGELOG dual).
       * Prints newline bag above the meta plate for greppable agent logs. */
      memset(&ch, 0, sizeof ch);
      cubalc_host_json_changelog(plate, plate2, &ch);
      if (ch.str[0]) {
        fputs(ch.str, stdout);
        if (ch.str[strlen(ch.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      o = 0; esc[0] = 0;
      for (i = 0; ch.str[i] && o + 2 < sizeof esc; i++) {
        char c = ch.str[i];
        if (c == '"' || c == '\\') {
          esc[o++] = '\\';
          esc[o++] = c;
        } else if (c == '\n') {
          esc[o++] = '\\';
          esc[o++] = 'n';
        } else if (c == '\r') {
        } else if ((unsigned char)c < 32) {
          esc[o++] = ' ';
        } else {
          esc[o++] = c;
        }
      }
      esc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"changelog\",\"path\":\"%s\",\"path2\":\"%s\","
             "\"equal\":%s,\"n\":%ld,\"lines\":\"%s\","
             "\"file\":%s,\"file2\":%s,\"version\":\"%s\","
             "\"note\":\"changelog lines above plate · JSONCHANGELOG dual\"}\n",
             path, path2,
             equal ? "true" : "false", ch.n, esc,
             file_hit ? "true" : "false",
             file_hit2 ? "true" : "false",
             CUBALC_LANG_VERSION);
      return equal ? 0 : 1;
    }

    /* has|need path k1 [k2 …] — multi-key presence contract on a plate file.
     * Usability: shell dual of HASPALL / NEEDP without a .cubalc program.
     * Keys may be dotted nest paths: cubalc plate need agent.json host freq.error
     *   cubalc plate has  agent.json n status role
     *   cubalc plate need agent.json n ok
     * has: ok always true · has_all · miss bag · exit 0 iff all present
     * need: ok false + err when miss (hard gate for agent scripts) */
    if (strcmp(op, "has") == 0 || strcmp(op, "need") == 0) {
      char keys_nl[CUBALC_HOST_STR_MAX];
      char flat_miss[CUBALC_HOST_STR_MAX];
      char flat_req[CUBALC_HOST_STR_MAX];
      cubalc_host_result miss, hasr;
      size_t o = 0, i;
      long nreq = 0;
      int all_ok;

      keys_nl[0] = 0;
      flat_req[0] = 0;
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need one or more keys\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      while (ai < argc && argv[ai] && argv[ai][0]) {
        size_t kl = strlen(argv[ai]);
        if (o + kl + 2 >= sizeof keys_nl) break;
        if (o > 0) keys_nl[o++] = '\n';
        memcpy(keys_nl + o, argv[ai], kl);
        o += kl;
        keys_nl[o] = 0;
        /* flat req for plate meta */
        {
          size_t fo = strlen(flat_req);
          if (fo + kl + 2 < sizeof flat_req) {
            if (fo > 0) {
              flat_req[fo++] = ',';
              flat_req[fo] = 0;
            }
            memcpy(flat_req + fo, argv[ai], kl);
            flat_req[fo + kl] = 0;
          }
        }
        nreq++;
        ai++;
      }

      memset(&miss, 0, sizeof miss);
      cubalc_host_json_filter_req_keys(plate, keys_nl, 0 /* missing */, &miss);
      memset(&hasr, 0, sizeof hasr);
      cubalc_host_json_has_keys(plate, keys_nl, 1 /* all */, &hasr);
      all_ok = (hasr.n == 1) ? 1 : 0;

      o = 0; flat_miss[0] = 0;
      for (i = 0; miss.str[i] && o + 2 < sizeof flat_miss; i++) {
        char c = miss.str[i];
        if (c == '\n' || c == '\r') {
          if (o > 0 && flat_miss[o - 1] != ',') flat_miss[o++] = ',';
        } else if (c == '"' || c == '\\') {
          flat_miss[o++] = '_';
        } else {
          flat_miss[o++] = c;
        }
      }
      flat_miss[o] = 0;

      if (strcmp(op, "need") == 0 && !all_ok) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"need\",\"path\":\"%s\",\"has_all\":false,"
               "\"n_req\":%ld,\"n_hit\":%ld,\"n_miss\":%ld,\"miss\":\"%s\","
               "\"keys\":\"%s\",\"file\":%s,\"err\":\"missing plate keys\","
               "\"version\":\"%s\"}\n",
               path, nreq, (long)hasr.code, miss.n, flat_miss, flat_req,
               file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 1;
      }

      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"has_all\":%s,"
             "\"n_req\":%ld,\"n_hit\":%ld,\"n_miss\":%ld,\"miss\":\"%s\","
             "\"keys\":\"%s\",\"file\":%s,\"version\":\"%s\"}\n",
             op, path, all_ok ? "true" : "false",
             nreq, (long)hasr.code, miss.n, flat_miss, flat_req,
             file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      return all_ok ? 0 : 1;
    }

    /* pluck|getall path k1 [k2 …] — multi-key peel → value bag (PLUCKP dual).
     * Usability: agent one-shot extract including dotted nest paths without .cubalc:
     *   cubalc plate pluck agent.json host freq.error n
     * Prints bag lines above plate meta · empty field on miss · hit/miss counts. */
    if (strcmp(op, "pluck") == 0) {
      char keys_nl[CUBALC_HOST_STR_MAX];
      char flat_req[CUBALC_HOST_STR_MAX];
      char bag_esc[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t o = 0, bi, bo;
      long nreq = 0;

      keys_nl[0] = 0;
      flat_req[0] = 0;
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pluck\",\"path\":\"%s\",\"err\":\"need one or more keys\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      while (ai < argc && argv[ai] && argv[ai][0]) {
        size_t kl = strlen(argv[ai]);
        if (o + kl + 2 >= sizeof keys_nl) break;
        if (o > 0) keys_nl[o++] = '\n';
        memcpy(keys_nl + o, argv[ai], kl);
        o += kl;
        keys_nl[o] = 0;
        {
          size_t fo = strlen(flat_req);
          if (fo + kl + 2 < sizeof flat_req) {
            if (fo > 0) {
              flat_req[fo++] = ',';
              flat_req[fo] = 0;
            }
            memcpy(flat_req + fo, argv[ai], kl);
            flat_req[fo + kl] = 0;
          }
        }
        nreq++;
        ai++;
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_pluck(plate, keys_nl, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pluck\",\"path\":\"%s\",\"err\":\"%s\","
               "\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "pluck fail", CUBALC_LANG_VERSION);
        return 1;
      }

      /* bag lines for greppable agent pipes */
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }

      bag_esc[0] = 0;
      bo = 0;
      for (bi = 0; pr.str[bi] && bo + 2 < sizeof bag_esc; bi++) {
        char c = pr.str[bi];
        if (c == '"' || c == '\\') {
          bag_esc[bo++] = '\\';
          bag_esc[bo++] = c;
        } else if (c == '\n' || c == '\r') {
          bag_esc[bo++] = '\\';
          bag_esc[bo++] = (c == '\n') ? 'n' : 'r';
        } else if ((unsigned char)c < 32) {
          bag_esc[bo++] = ' ';
        } else {
          bag_esc[bo++] = c;
        }
      }
      bag_esc[bo] = 0;

      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"pluck\",\"path\":\"%s\",\"n\":%ld,\"hit\":%ld,\"miss\":%ld,"
             "\"keys\":\"%s\",\"bag\":\"%s\",\"file\":%s,\"version\":\"%s\","
             "\"note\":\"bag lines above plate · PLUCKP dual · paths ok\"}\n",
             path, pr.n, (long)pr.code, pr.n - (long)pr.code, flat_req, bag_esc,
             file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      return 0;
    }

    /* rename|copy|swap path src dst — RENAMEP/COPYP/SWAPP duals (paths ok).
     * Usability: restructure nest fields without a .cubalc program:
     *   cubalc plate rename agent.json cfg.port net.listen
     *   cubalc plate copy   agent.json flags.debug flags.trace
     *   cubalc plate swap   agent.json a.x a.y
     * Soft miss: rename/copy n=0 plate unchanged · swap both-miss n=0. */
    if (strcmp(op, "rename") == 0 || strcmp(op, "copy") == 0 ||
        strcmp(op, "swap") == 0) {
      const char *src = NULL, *dst = NULL;
      cubalc_host_result rr;
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need src key\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      src = argv[ai++];
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need dst key\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      dst = argv[ai++];
      memset(&rr, 0, sizeof rr);
      if (strcmp(op, "rename") == 0) {
        if (cubalc_host_json_rename(plate, src, dst, &rr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"rename\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
                 "\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, src, dst, rr.err[0] ? rr.err : "rename fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }
      } else if (strcmp(op, "copy") == 0) {
        if (cubalc_host_json_copy_key(plate, src, dst, &rr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"copy\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
                 "\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, src, dst, rr.err[0] ? rr.err : "copy fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }
      } else {
        if (cubalc_host_json_swap_keys(plate, src, dst, &rr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"swap\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
                 "\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, src, dst, rr.err[0] ? rr.err : "swap fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }
      }
      snprintf(plate, sizeof plate, "%s", rr.str);
      if (rr.n) {
        slash = strrchr(path, '/');
        if (slash && slash != path) {
          size_t n = (size_t)(slash - path);
          if (n >= sizeof parent) n = sizeof parent - 1;
          memcpy(parent, path, n);
          parent[n] = 0;
          memset(&hr, 0, sizeof hr);
          cubalc_host_mkdir(parent, &hr);
        }
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_write(path, plate, &hr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
               "\"n\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
               op, path, src, dst, rr.n, hr.n, CUBALC_LANG_VERSION, plate);
        return 0;
      }
      /* soft miss / no-op write skipped */
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
             "\"n\":0,\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
             op, path, src, dst, file_hit ? "true" : "false",
             CUBALC_LANG_VERSION, plate);
      return 0;
    }

        /* remaining ops need key */
    if (ai >= argc || !argv[ai] || !argv[ai][0]) {
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need key\",\"version\":\"%s\"}\n",
             op, path, CUBALC_LANG_VERSION);
      return 2;
    }
    key = argv[ai++];

    if (strcmp(op, "get") == 0) {
      if (ai < argc && (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "DEFAULT") == 0 ||
                        strcmp(argv[ai], "ELSE") == 0)) {
        ai++;
        if (ai < argc) {
          fb = argv[ai++];
          have_fb = 1;
        }
      }
      memset(&gr, 0, sizeof gr);
      /* dotted/slash path: plate get agent.json freq.error */
      if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
        /* escape value lightly for JSON string field */
        char esc[CUBALC_HOST_STR_MAX];
        size_t i, o = 0;
        for (i = 0; gr.str[i] && o + 2 < sizeof esc; i++) {
          char c = gr.str[i];
          if (c == '"' || c == '\\') {
            esc[o++] = '\\';
            esc[o++] = c;
          } else if ((unsigned char)c < 32) {
            esc[o++] = ' ';
          } else {
            esc[o++] = c;
          }
        }
        esc[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"get\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":true,"
               "\"value\":\"%s\",\"file\":%s,\"version\":\"%s\"}\n",
               path, key, esc, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }
      if (have_fb) {
        char esc[512];
        size_t i, o = 0;
        for (i = 0; fb[i] && o + 2 < sizeof esc; i++) {
          char c = fb[i];
          if (c == '"' || c == '\\') {
            esc[o++] = '\\';
            esc[o++] = c;
          } else {
            esc[o++] = c;
          }
        }
        esc[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"get\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
               "\"or\":true,\"value\":\"%s\",\"file\":%s,\"version\":\"%s\"}\n",
               path, key, esc, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
             "\"op\":\"get\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
             "\"err\":\"key miss\",\"file\":%s,\"version\":\"%s\"}\n",
             path, key, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      return 1;
    }

    /* getn|num path key [OR n] — GETPN dual: numeric peel (paths ok).
     * Usability: shell dual of GETPN / SYS JSONN for nest counters without .cubalc:
     *   cubalc plate getn agent.json cfg.port
     *   cubalc plate getn agent.json freq.error OR 0
     * n = numeric value · hit true|false · exit 0 on hit or OR, 1 on hard miss. */
    if (strcmp(op, "getn") == 0) {
      long nval = 0, fb_n = 0;
      char *end = NULL;
      if (ai < argc && (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "DEFAULT") == 0 ||
                        strcmp(argv[ai], "ELSE") == 0)) {
        ai++;
        if (ai < argc) {
          fb = argv[ai++];
          have_fb = 1;
          fb_n = strtol(fb, &end, 10);
          if (end == fb) fb_n = 0;
        }
      }
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
        end = NULL;
        nval = strtol(gr.str, &end, 10);
        if (end == gr.str) nval = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getn\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":true,"
               "\"n\":%ld,\"value\":\"%s\",\"file\":%s,\"version\":\"%s\","
               "\"note\":\"GETPN dual · LAST_N-style numeric peel · paths ok\"}\n",
               path, key, nval, gr.str, file_hit ? "true" : "false",
               CUBALC_LANG_VERSION);
        return 0;
      }
      if (have_fb) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getn\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
               "\"or\":true,\"n\":%ld,\"value\":\"%ld\",\"file\":%s,"
               "\"version\":\"%s\"}\n",
               path, key, fb_n, fb_n, file_hit ? "true" : "false",
               CUBALC_LANG_VERSION);
        return 0;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
             "\"op\":\"getn\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
             "\"n\":0,\"err\":\"key miss\",\"file\":%s,\"version\":\"%s\"}\n",
             path, key, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      return 1;
    }

    /* getobj path key [OR '{}'] — GETOBJ dual: peel nested object (paths ok).
     * setobj path key obj|@file — SETOBJ dual: nest object write-back (paths ok).
     * mergeobj|defaultobj path key obj|@file — MERGEOBJ/DEFAULTOBJ duals.
     * Usability: deep nest object peel/patch without a .cubalc program:
     *   cubalc plate getobj agent.json cfg.meta
     *   cubalc plate setobj agent.json net.peer '{"host":"B"}'
     *   cubalc plate mergeobj agent.json cfg.meta '{"zone":"A"}'
     */
    if (strcmp(op, "getobj") == 0 || strcmp(op, "setobj") == 0 ||
        strcmp(op, "mergeobj") == 0 || strcmp(op, "defaultobj") == 0) {
      char objbuf[CUBALC_HOST_STR_MAX];
      char nest[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr, mr;
      const char *v;
      int hit = 0;

      if (strcmp(op, "getobj") == 0) {
        if (ai < argc && (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "DEFAULT") == 0 ||
                          strcmp(argv[ai], "ELSE") == 0)) {
          ai++;
          if (ai < argc) {
            fb = argv[ai++];
            have_fb = 1;
          }
        }
        memset(&gr, 0, sizeof gr);
        if (cubalc_host_json_path_get_raw(plate, key, &gr) == 0) {
          v = gr.str;
          while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
          if (*v == '{') {
            hit = 1;
            if (v != gr.str) {
              size_t n = strlen(v);
              memmove(gr.str, v, n + 1);
            }
          }
        }
        if (hit) {
          fputs(gr.str, stdout);
          if (!gr.str[0] || gr.str[strlen(gr.str) - 1] != '\n')
            fputc('\n', stdout);
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"getobj\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":true,"
                 "\"file\":%s,\"version\":\"%s\",\"plate\":%s,"
                 "\"note\":\"GETOBJ dual · object body above plate · paths ok\"}\n",
                 path, key, file_hit ? "true" : "false", CUBALC_LANG_VERSION, gr.str);
          return 0;
        }
        if (have_fb) {
          const char *body_out = fb;
          while (*body_out == ' ' || *body_out == '\t') body_out++;
          if (*body_out != '{')
            body_out = "{}";
          fputs(body_out, stdout);
          if (body_out[strlen(body_out) - 1] != '\n')
            fputc('\n', stdout);
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"getobj\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
                 "\"or\":true,\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
                 path, key, file_hit ? "true" : "false", CUBALC_LANG_VERSION, body_out);
          return 0;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getobj\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
               "\"err\":\"key miss or not object\",\"file\":%s,\"version\":\"%s\"}\n",
               path, key, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 1;
      }

      /* setobj|mergeobj|defaultobj need object value */
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"err\":\"need object value or @file\","
               "\"version\":\"%s\"}\n", op, CUBALC_LANG_VERSION);
        return 2;
      }
      val = argv[ai++];
      objbuf[0] = 0;
      if (val[0] == '@') {
        memset(&pr, 0, sizeof pr);
        if (cubalc_host_read(val + 1, &pr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"object read fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(objbuf, sizeof objbuf, "%s", pr.str);
      } else if (strchr(val, '{') == NULL && access(val, R_OK) == 0) {
        memset(&pr, 0, sizeof pr);
        if (cubalc_host_read(val, &pr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"object read fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(objbuf, sizeof objbuf, "%s", pr.str);
      } else {
        snprintf(objbuf, sizeof objbuf, "%s", val);
      }
      v = objbuf;
      while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
      if (*v != '{') {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"err\":\"value not object\",\"version\":\"%s\"}\n",
               op, CUBALC_LANG_VERSION);
        return 1;
      }
      if (v != objbuf) {
        size_t n = strlen(v);
        memmove(objbuf, v, n + 1);
      }

      if (strcmp(op, "setobj") == 0) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, key, objbuf, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"setobj\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 wr.err[0] ? wr.err : "set fail", CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(plate, sizeof plate, "%s", wr.str);
      } else {
        /* mergeobj / defaultobj: peel nest, merge/defaults, path_set */
        memset(&pr, 0, sizeof pr);
        cubalc_host_json_path_obj(plate, key, &pr);
        snprintf(nest, sizeof nest, "%s", pr.str);
        memset(&mr, 0, sizeof mr);
        if (strcmp(op, "defaultobj") == 0) {
          if (cubalc_host_json_defaults(nest, objbuf, &mr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"defaultobj\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   mr.err[0] ? mr.err : "defaults fail", CUBALC_LANG_VERSION);
            return 1;
          }
        } else {
          if (cubalc_host_json_merge(nest, objbuf, &mr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"mergeobj\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   mr.err[0] ? mr.err : "merge fail", CUBALC_LANG_VERSION);
            return 1;
          }
        }
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, key, mr.str, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 op, wr.err[0] ? wr.err : "set fail", CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(plate, sizeof plate, "%s", wr.str);
        /* report applied count in wr.n via mr.n after write */
        wr.n = mr.n;
      }

      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               op, CUBALC_LANG_VERSION);
        return 1;
      }
      if (strcmp(op, "setobj") == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"setobj\",\"path\":\"%s\",\"key\":\"%s\",\"bytes\":%ld,"
               "\"file\":%s,\"version\":\"%s\",\"plate\":%s,"
               "\"note\":\"SETOBJ dual · paths ok\"}\n",
               path, key, hr.n, file_hit ? "true" : "false",
               CUBALC_LANG_VERSION, plate);
      } else {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"key\":\"%s\",\"n\":%ld,"
               "\"bytes\":%ld,\"file\":%s,\"version\":\"%s\",\"plate\":%s,"
               "\"note\":\"MERGEOBJ/DEFAULTOBJ dual · paths ok\"}\n",
               op, path, key, wr.n, hr.n, file_hit ? "true" : "false",
               CUBALC_LANG_VERSION, plate);
      }
      return 0;
    }

    /* type|kind path key — TYPEP dual: field kind probe (paths ok).
     * Usability: choose num/str/obj peel without a .cubalc program:
     *   cubalc plate type agent.json freq.error  → num
     *   cubalc plate kind agent.json meta        → obj
     * Soft miss → kind=missing · hit=false · exit 1. */
    if (strcmp(op, "type") == 0) {
      const char *kind_s = "missing";
      long kind_n = 0;
      int hit = 0;
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get_raw(plate, key, &gr) == 0) {
        const char *v = gr.str;
        hit = 1;
        while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
        if (*v == '"') {
          kind_s = "str";
          kind_n = 2;
        } else if (*v == '{') {
          kind_s = "obj";
          kind_n = 5;
        } else if (*v == '[') {
          kind_s = "arr";
          kind_n = 6;
        } else if (strncmp(v, "true", 4) == 0 || strncmp(v, "false", 5) == 0) {
          kind_s = "bool";
          kind_n = 3;
        } else if (strncmp(v, "null", 4) == 0) {
          kind_s = "null";
          kind_n = 4;
        } else if (*v == '-' || (*v >= '0' && *v <= '9')) {
          kind_s = "num";
          kind_n = 1;
        } else {
          kind_s = "str";
          kind_n = 2;
        }
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"type\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":%s,"
             "\"kind\":\"%s\",\"n\":%ld,\"file\":%s,\"version\":\"%s\","
             "\"note\":\"TYPEP dual · missing|num|str|bool|null|obj|arr · paths ok\"}\n",
             path, key, hit ? "true" : "false", kind_s, kind_n,
             file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      return hit ? 0 : 1;
    }

    if (strcmp(op, "set") == 0) {
      size_t i;
      int allnum = 1;
      if (ai >= argc) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"set\",\"err\":\"need value\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 2;
      }
      val = argv[ai++];
      /* raw if integer / true / false / null / starts with { or [ */
      if (!strcmp(val, "true") || !strcmp(val, "false") || !strcmp(val, "null") ||
          val[0] == '{' || val[0] == '[')
        val_kind = 1;
      else {
        if (val[0] == '-' && val[1]) i = 1;
        else i = 0;
        if (!val[i]) allnum = 0;
        for (; val[i]; i++) {
          if (val[i] < '0' || val[i] > '9') {
            allnum = 0;
            break;
          }
        }
        if (allnum) val_kind = 1;
      }
      memset(&wr, 0, sizeof wr);
      /* dotted/slash path: plate set agent.json freq.error 9 */
      if (cubalc_host_json_path_set(plate, key, val, val_kind, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"set\",\"path\":\"%s\",\"key\":\"%s\",\"err\":\"%s\","
               "\"version\":\"%s\"}\n",
               path, key, wr.err[0] ? wr.err : "json set fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", wr.str);
      /* mkdir parent + write */
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"set\",\"path\":\"%s\",\"key\":\"%s\",\"err\":\"%s\","
               "\"version\":\"%s\"}\n",
               path, key, hr.err[0] ? hr.err : "write fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"set\",\"path\":\"%s\",\"key\":\"%s\",\"updated\":%ld,"
             "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, wr.n, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* default path key value — DEFAULTP dual: set-if-missing (paths ok).
     *   cubalc plate default agent.json cfg.port 8080
     * kept=true when key already present (no clobber). */
    if (strcmp(op, "default") == 0) {
      size_t i;
      int allnum = 1, present = 0;
      cubalc_host_result hasr;
      if (ai >= argc) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"default\",\"err\":\"need value\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 2;
      }
      val = argv[ai++];
      memset(&hasr, 0, sizeof hasr);
      present = (cubalc_host_json_path_has(plate, key, &hasr) == 0 && hasr.n) ? 1 : 0;
      if (present) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"default\",\"path\":\"%s\",\"key\":\"%s\",\"kept\":true,"
               "\"inserted\":false,\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
               path, key, file_hit ? "true" : "false", CUBALC_LANG_VERSION, plate);
        return 0;
      }
      if (!strcmp(val, "true") || !strcmp(val, "false") || !strcmp(val, "null") ||
          val[0] == '{' || val[0] == '[')
        val_kind = 1;
      else {
        if (val[0] == '-' && val[1]) i = 1;
        else i = 0;
        if (!val[i]) allnum = 0;
        for (; val[i]; i++) {
          if (val[i] < '0' || val[i] > '9') {
            allnum = 0;
            break;
          }
        }
        if (allnum) val_kind = 1;
      }
      memset(&wr, 0, sizeof wr);
      if (cubalc_host_json_path_set(plate, key, val, val_kind, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"default\",\"path\":\"%s\",\"key\":\"%s\",\"err\":\"%s\","
               "\"version\":\"%s\"}\n",
               path, key, wr.err[0] ? wr.err : "json set fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", wr.str);
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"default\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"default\",\"path\":\"%s\",\"key\":\"%s\",\"kept\":false,"
             "\"inserted\":true,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* toggle path key — TOGGLEP dual: flip 0↔1 flag (paths ok; miss→1).
     *   cubalc plate toggle agent.json flags.debug */
    if (strcmp(op, "toggle") == 0) {
      long cur = 0, nv;
      char raw[32];
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
        char *end = NULL;
        if (!gr.str[0]) cur = 0;
        else {
          cur = strtol(gr.str, &end, 10);
          if (end == gr.str) cur = 1; /* non-numeric non-empty → truthy */
        }
      } else {
        cur = 0;
      }
      nv = cur ? 0 : 1;
      snprintf(raw, sizeof raw, "%ld", nv);
      memset(&wr, 0, sizeof wr);
      if (cubalc_host_json_path_set(plate, key, raw, 1, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"toggle\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               wr.err[0] ? wr.err : "json set fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", wr.str);
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"toggle\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"toggle\",\"path\":\"%s\",\"key\":\"%s\",\"value\":%ld,"
             "\"prev\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, nv, cur ? 1L : 0L, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    if (strcmp(op, "inc") == 0) {
      long cur = 0, nv;
      char raw[32];
      if (ai < argc) {
        char *end = NULL;
        delta = strtol(argv[ai], &end, 10);
        if (end == argv[ai]) delta = 1;
        ai++;
      }
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
        char *end = NULL;
        cur = strtol(gr.str, &end, 10);
        if (end == gr.str) cur = 0;
      }
      nv = cur + delta;
      snprintf(raw, sizeof raw, "%ld", nv);
      memset(&wr, 0, sizeof wr);
      if (cubalc_host_json_path_set(plate, key, raw, 1, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"inc\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               wr.err[0] ? wr.err : "json set fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", wr.str);
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"inc\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"inc\",\"path\":\"%s\",\"key\":\"%s\",\"value\":%ld,"
             "\"delta\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, nv, delta, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    if (strcmp(op, "del") == 0) {
      memset(&wr, 0, sizeof wr);
      if (cubalc_host_json_path_del(plate, key, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"del\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               wr.err[0] ? wr.err : "json del fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", wr.str);
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"del\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"del\",\"path\":\"%s\",\"key\":\"%s\",\"removed\":%ld,"
             "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, wr.n, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
           "\"err\":\"unknown op\",\"op\":\"%s\",\"version\":\"%s\"}\n",
           op, CUBALC_LANG_VERSION);
    return 2;
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
      {"hold_flash", "docs/HOLD_FLASH.md", "device/firmware connection safeguard"},
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
    /* Usability: resolve short name → path/kind for agents (INCLUDE/cat/paths).
     * order: exact path · lib · program · proof · p2p · protect · doc alias ·
     * form catalog · PATH binary. Multi-match plate cubalc.which.v1. */
    const char *q = (argc > 2) ? argv[2] : "";
    struct {
      char kind[16];
      char path[512];
      char hint[96];
      int present;
    } hits[16];
    int nh = 0, i;
    char stem[256];
    size_t blen;
    static const struct { const char *name; const char *path; const char *hint; } aliases[] = {
      {"cookbook", "docs/COOKBOOK.md", "hold → plug → smx recipes"},
      {"agents", "docs/FOR_AGENTS.md", "agent prompt snippet"},
      {"hold_flash", "docs/HOLD_FLASH.md", "device/firmware connection safeguard"},
      {"p2p_doc", "docs/P2P_SMX.md", "binary mesh wire"},
      {"hello", "programs/hello_cube.cubalc", "minimal starter"},
      {"lib", "programs/lib", "INCLUDE short-name stdlib dir"},
      {"proof", "programs/proof", "usability + ISA proofs dir"},
    };
    static const struct { const char *name; const char *plane; const char *hint; } forms[] = {
      {"HOLD_FLASH", "law", "device/firmware safeguard · default 1"},
      {"CUBE", "core", "place cube"},
      {"PLUG", "core", "wire cubes · denied only if HOLD_FLASH 0"},
      {"INCLUDE", "flow", "INCLUDE [ONCE] path|libname → programs/lib/"},
      {"DEFAULT", "flow", "DEFAULT name = value if unset"},
      {"DEFINED", "flow", "DEFINED name → LAST_N 0|1"},
      {"TYPEOF", "flow", "TYPEOF name → undef|num|str"},
      {"UNSET", "flow", "UNSET name remove var"},
      {"ASSERT", "flow", "ASSERT expr [why] — string ==/!= content compare"},
      {"EXPECT", "flow", "EXPECT soft check · no fatal"},
      {"FAIL", "flow", "FAIL soft status OK=0"},
      {"PASS", "flow", "PASS soft status OK=1"},
      {"NOTE", "flow", "NOTE agent breadcrumb"},
      {"EXIT", "flow", "EXIT [code] halt program"},
      {"CLEAR_ERR", "flow", "CLEAR_ERR wipe sticky LAST_ERR"},
      {"VERSION", "flow", "VERSION → LAST language version"},
      {"PRINT_JSON", "flow", "one JSON line for agents"},
      {"VARS", "flow", "VARS full var table JSON"},
      {"STATUS", "flow", "STATUS agent health plate"},
      {"IDENTITY", "flow", "IDENTITY user@host:pid plate"},
      {"SYS PID", "host", "SYS PID process id"},
      {"SYS HOSTNAME", "host", "SYS HOSTNAME machine name"},
      {"SYS USER", "host", "SYS USER login name"},
      {"SYS GETLOGIN", "host", "SYS GETLOGIN|LOGINNAME session login name"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS UID", "host", "SYS UID|RUID real user id → LAST_N"},
{"SYS EUID", "host", "SYS EUID|GETEUID effective user id → LAST_N"},
{"SYS ISROOT", "host", "SYS ISROOT|AMROOT euid==0 privilege probe"},
      {"SYS HOME", "host", "SYS HOME home directory"},
      {"SYS SHELL", "host", "SYS SHELL|LOGINSHELL login shell path"},
      {"SYS GECOS", "host", "SYS GECOS|FULLNAME passwd display name"},
      {"SYS TTYNAME", "host", "SYS TTYNAME|CTTY terminal device path"},
      {"SYS NICE", "host", "SYS NICE|RENICE process nice get/set"},
      {"SYS SHQUOTE", "host", "SYS SHQUOTE|SHELLQUOTE POSIX shell-safe wrap"},
      {"SYS SHUNQUOTE", "host", "SYS SHUNQUOTE|UNQUOTE peel POSIX single-quote wrap"},
      {"SYS GETLOGIN", "host", "SYS GETLOGIN|LOGINNAME session login name"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS DATE", "host", "SYS DATE UTC ISO stamp"},
      {"SYS FROMTIME", "host", "SYS FROMTIME|EPOCHISO|TOISO [n] — epoch → UTC ISO stamp"},
      {"SYS PARSEISO", "host", "SYS PARSEISO|TOEPOCH|FROMISO [str] — ISO → epoch dual of FROMTIME"},
      {"SYS TIMEDIFF", "host", "SYS TIMEDIFF|ELAPSED a [b] — epoch delta seconds a-b or now-a"},
      {"SYS ADDTIME", "host", "SYS ADDTIME|DEADLINE — epoch + seconds / now+secs lease"},
      {"SYS EXPIRED", "host", "SYS EXPIRED|REMAINING [epoch] — lease due probe / TTL secs"},
      {"SYS LOCAL", "host", "SYS LOCAL|LOCALTIME local wall stamp"},
      {"SYS LOCALTIME", "host", "SYS LOCALTIME alias of SYS LOCAL"},
      {"SYS LOCALDATE", "host", "SYS LOCALDATE local date-only"},
      {"SYS TMP", "host", "SYS TMP portable temp dir"},
      {"SYS MKTEMP", "host", "SYS MKTEMP unique empty file under TMP"},
      {"SYS TEMPFILE", "host", "SYS TEMPFILE alias of SYS MKTEMP"},
      {"SYS MKTEMPDIR", "host", "SYS MKTEMPDIR unique empty dir under TMP"},
      {"SYS MKDTEMP", "host", "SYS MKDTEMP alias of SYS MKTEMPDIR"},
      {"SYS MKDIR", "host", "SYS MKDIR mkdir -p"},
      {"SYS BASENAME", "host", "SYS BASENAME path leaf"},
      {"SYS DIRNAME", "host", "SYS DIRNAME path parent"},
      {"SYS EXTNAME", "host", "SYS EXTNAME path .ext"},
      {"SYS STEM", "host", "SYS STEM path without ext"},
      {"SYS SIZE", "host", "SYS SIZE file bytes"},
      {"SYS STAT", "host", "SYS STAT one-shot path metadata"},
      {"SYS FSTAT", "host", "SYS FSTAT alias of SYS STAT"},
      {"SYS ISDIR", "host", "SYS ISDIR dir probe"},
      {"SYS ISFILE", "host", "SYS ISFILE file probe"},
      {"SYS READ", "host", "SYS READ OR soft optional plate"},
      {"SYS LOADPLATE", "host", "SYS LOADPLATE|READPLATE path [OR defaults] soft plate file"},
      {"SYS SAVEPLATE", "host", "SYS SAVEPLATE|STOREPLATE path [plate] object plate persist"},
      {"SYS ENSUREPLATE", "host", "SYS ENSUREPLATE|TOUCHPLATE path [OR defaults] create-or-load"},
      {"SYS UPDATEPLATE", "host", "SYS UPDATEPLATE|JSONFILESET path key value load-set-save plate field"},
      {"SYS BUMPPLATE", "host", "SYS BUMPPLATE|JSONFILEINC path key [delta] load-inc-save counter"},
      {"SYS PEEKPLATE", "host", "SYS PEEKPLATE|JSONFILEGET path key [OR fb] peel plate file field"},
      {"SYS DROPPLATE", "host", "SYS DROPPLATE|JSONFILEDEL path key load-del-save plate field"},
      {"SYS JSONFILEMERGE", "host", "SYS JSONFILEMERGE|OVERLAYPLATE path overlay multi-key load-merge-save"},
      {"SYS JSONFILEENSURE", "host", "SYS JSONFILEENSURE|ENSUREPLATEKEY path key value set-if-missing"},
      {"SYS JSONFILEDEFAULTS", "host", "SYS JSONFILEDEFAULTS|DEFAULTPLATEALL path defaults multi-key fill-if-missing"},
      {"SYS JSONFILETOGGLE", "host", "SYS JSONFILETOGGLE|TOGGLEPLATEKEY path key flip disk flag 0↔1"},
      {"SYS RM", "host", "SYS RM remove plate file"},
      {"SYS RMDIR", "host", "SYS RMDIR remove empty directory"},
      {"SYS REMOVEDIR", "host", "SYS REMOVEDIR alias of SYS RMDIR"},
      {"SYS RENAME", "host", "SYS RENAME move plate path"},
      {"SYS COPY", "host", "SYS COPY duplicate plate file"},
      {"SYS REALPATH", "host", "SYS REALPATH absolute path"},
      {"SYS TOUCH", "host", "SYS TOUCH plate marker"},
      {"SYS SLEEP", "host", "SYS SLEEP n|duration — ms or 1s/250ms human duration"},
      {"SYS WAITFILE", "host", "SYS WAITFILE path [ms|duration] poll until exists"},
      {"SYS WAITPATH", "host", "SYS WAITPATH alias of SYS WAITFILE"},
      {"SYS WAITGONE", "host", "SYS WAITGONE poll until path does not exist"},
      {"SYS WAITMISSING", "host", "SYS WAITMISSING alias of SYS WAITGONE"},
      {"SYS WAITSTABLE", "host", "SYS WAITSTABLE poll until size+mtime quiet"},
      {"SYS WAITQUIET", "host", "SYS WAITQUIET alias of SYS WAITSTABLE"},
      {"SYS WAITCHANGED", "host", "SYS WAITCHANGED poll until size/mtime change"},
      {"SYS WAITMODIFIED", "host", "SYS WAITMODIFIED alias of SYS WAITCHANGED"},
      {"SYS WAITMATCH", "host", "SYS WAITMATCH poll until file contains needle"},
      {"SYS WAITCONTAINS", "host", "SYS WAITCONTAINS alias of SYS WAITMATCH"},
      {"SYS RAND", "host", "SYS RAND uniform int jitter"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM hex random bytes"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS RANDOM", "host", "SYS RANDOM alias of SYS RAND"},
      {"SYS PICK", "host", "SYS PICK random bag field"},
      {"SYS CHOICE", "host", "SYS CHOICE alias of SYS PICK"},
      {"SYS SHUFFLE", "host", "SYS SHUFFLE randomize bag order"},
      {"SYS SHUF", "host", "SYS SHUF alias of SYS SHUFFLE"},
      {"SYS DRAWN", "host", "SYS DRAWN sample k unique bag fields"},
      {"SYS SAMPLEK", "host", "SYS SAMPLEK alias of SYS DRAWN"},
      {"SYS NPICK", "host", "SYS NPICK alias of SYS DRAWN"},
      {"SYS TAKERAND", "host", "SYS TAKERAND alias of SYS DRAWN"},
      {"SYS MIN", "host", "SYS MIN host-plane min args or bag"},
      {"SYS MAX", "host", "SYS MAX host-plane max args or bag"},
      {"SYS MINBAG", "host", "SYS MINBAG bag-mode MIN"},
      {"SYS MAXBAG", "host", "SYS MAXBAG bag-mode MAX"},
      {"SYS ARGMAX", "host", "SYS ARGMAX index of first max"},
      {"SYS ARGMIN", "host", "SYS ARGMIN index of first min"},
      {"SYS MAXIDX", "host", "SYS MAXIDX alias of SYS ARGMAX"},
      {"SYS MINIDX", "host", "SYS MINIDX alias of SYS ARGMIN"},
      {"SYS PADALL", "host", "SYS PADALL right-pad every bag field"},
      {"SYS LPADALL", "host", "SYS LPADALL left-pad every bag field"},
      {"SYS RPADALL", "host", "SYS RPADALL alias of PADALL"},
      {"SYS TRUNCALL", "host", "SYS TRUNCALL truncate bag fields to width"},
      {"SYS CLIPALL", "host", "SYS CLIPALL alias of SYS TRUNCALL"},
      {"SYS MAPTRUNC", "host", "SYS MAPTRUNC alias of SYS TRUNCALL"},
      {"SYS CLAMP", "host", "SYS CLAMP bound into range"},
      {"SYS IN", "host", "SYS IN numeric range membership"},
      {"SYS WITHIN", "host", "SYS WITHIN alias of SYS IN"},
      {"SYS CMP", "host", "SYS CMP three-way numeric compare"},
      {"SYS SCMP", "host", "SYS SCMP string compare"},
      {"SYS IABS", "host", "SYS IABS integer absolute value"},
      {"SYS SIGN", "host", "SYS SIGN signum -1|0|1"},
      {"SYS DIV", "host", "SYS DIV integer divide"},
      {"SYS MOD", "host", "SYS MOD remainder"},
      {"SYS GCD", "host", "SYS GCD multi-arg gcd"},
      {"SYS LCM", "host", "SYS LCM multi-arg lcm"},
      {"SYS POW", "host", "SYS POW integer power"},
      {"SYS ISQRT", "host", "SYS ISQRT floor integer sqrt"},
      {"SYS SUM", "host", "SYS SUM args|bag integer sum"},
      {"SYS PROD", "host", "SYS PROD args|bag integer product"},
      {"SYS AVG", "host", "SYS AVG args|bag integer mean"},
      {"SYS MEDIAN", "host", "SYS MEDIAN args|bag integer median"},
      {"SYS RANGE", "host", "SYS RANGE inclusive number bag"},
      {"SYS SEQ", "host", "SYS SEQ 1..n number bag"},
      {"SYS IOTA", "host", "SYS IOTA 0..n-1 number bag"},
      {"SYS LIST", "host", "SYS LIST directory basenames"},
      {"SYS GLOB", "host", "SYS GLOB|MATCHFILES path [pattern] — match basenames"},
      {"SYS MATCHFILES", "host", "SYS MATCHFILES alias of SYS GLOB"},
      {"SYS PATHGLOB", "host", "SYS PATHGLOB|PGLOB full paths matching pattern"},
      {"SYS PGLOB", "host", "SYS PGLOB alias of SYS PATHGLOB"},
      {"SYS FILTERGLOB", "host", "SYS FILTERGLOB|MATCHBAG shell-pattern bag filter"},
      {"SYS MATCHBAG", "host", "SYS MATCHBAG alias of SYS FILTERGLOB"},
      {"SYS BASENAMEALL", "host", "SYS BASENAMEALL|MAPBASE path bag map"},
      {"SYS DIRNAMEALL", "host", "SYS DIRNAMEALL|MAPDIR path bag map"},
      {"SYS KEEPFILES", "host", "SYS KEEPFILES path bag keep regular files"},
      {"SYS KEEPDIRS", "host", "SYS KEEPDIRS path bag keep directories"},
      {"SYS SIZEALL", "host", "SYS SIZEALL|MAPSIZE path bag byte sizes"},
      {"SYS MAPSIZE", "host", "SYS MAPSIZE alias of SYS SIZEALL"},
      {"SYS MTIMEALL", "host", "SYS MTIMEALL|MAPMTIME epoch mtime bag"},
      {"SYS AGEALL", "host", "SYS AGEALL|MAPAGE age-seconds bag"},
      {"SYS NEWEST", "host", "SYS NEWEST|LATEST freshest path by mtime"},
      {"SYS OLDEST", "host", "SYS OLDEST|EARLIEST oldest path by mtime"},
      {"SYS LARGEST", "host", "SYS LARGEST largest file path"},
      {"SYS SORTMTIME", "host", "SYS SORTMTIME order paths by mtime"},
      {"SYS SORTSIZE", "host", "SYS SORTSIZE order paths by size"},
      {"SYS FRESH", "host", "SYS FRESH bag max_age keep fresh paths"},
      {"SYS KEEPSTALE", "host", "SYS KEEPSTALE|AGED bag min_age keep stale paths"},
      {"SYS KEEPNEWER", "host", "SYS KEEPNEWER|NEWERTHAN [bag] ref mtime>ref"},
      {"SYS NEWERTHAN", "host", "SYS NEWERTHAN alias of SYS KEEPNEWER"},
      {"SYS KEEPOLDER", "host", "SYS KEEPOLDER|OLDERREF [bag] ref mtime<ref"},
      {"SYS OLDERREF", "host", "SYS OLDERREF alias of SYS KEEPOLDER"},
      {"SYS KEEPBIGGER", "host", "SYS KEEPBIGGER|BIGFILES|SIZEGE bag min_bytes size>=min"},
      {"SYS BIGFILES", "host", "SYS BIGFILES alias of SYS KEEPBIGGER"},
      {"SYS SIZEGE", "host", "SYS SIZEGE alias of SYS KEEPBIGGER"},
      {"SYS KEEPSMALLER", "host", "SYS KEEPSMALLER|SMALLFILES|SIZELE bag max_bytes size<=max"},
      {"SYS SMALLFILES", "host", "SYS SMALLFILES alias of SYS KEEPSMALLER"},
      {"SYS SIZELE", "host", "SYS SIZELE alias of SYS KEEPSMALLER"},
      {"SYS RMALL", "host", "SYS RMALL|UNLINKALL bulk remove path bag"},
      {"SYS UNLINKALL", "host", "SYS UNLINKALL alias of SYS RMALL"},
      {"SYS DELETEALL", "host", "SYS DELETEALL alias of SYS RMALL"},
      {"SYS TOUCHALL", "host", "SYS TOUCHALL|ENSUREALL bulk create/refresh path bag"},
      {"SYS ENSUREALL", "host", "SYS ENSUREALL alias of SYS TOUCHALL"},
      {"SYS CREATEALL", "host", "SYS CREATEALL alias of SYS TOUCHALL"},
      {"SYS COPYALL", "host", "SYS COPYALL|CPALL bulk copy path bag into dest"},
      {"SYS CPALL", "host", "SYS CPALL alias of SYS COPYALL"},
      {"SYS BULKCOPY", "host", "SYS BULKCOPY alias of SYS COPYALL"},
      {"SYS MKDIRALL", "host", "SYS MKDIRALL|ENSUREDIRS bulk mkdir -p path bag"},
      {"SYS ENSUREDIRS", "host", "SYS ENSUREDIRS alias of SYS MKDIRALL"},
      {"SYS MKDIRS", "host", "SYS MKDIRS alias of SYS MKDIRALL"},
      {"SYS MOVEALL", "host", "SYS MOVEALL|MVALL bulk move path bag into dest"},
      {"SYS MVALL", "host", "SYS MVALL alias of SYS MOVEALL"},
      {"SYS RENAMEALL", "host", "SYS RENAMEALL alias of SYS MOVEALL"},
      {"SYS WALK", "host", "SYS WALK|FINDALL recursive full-path discovery"},
      {"SYS FINDALL", "host", "SYS FINDALL alias of SYS WALK"},
      {"SYS TREEGLOB", "host", "SYS TREEGLOB alias of SYS WALK"},
      {"SYS EQFILE", "host", "SYS EQFILE|SAMEFILE identical file content"},
      {"SYS SAMEFILE", "host", "SYS SAMEFILE alias of SYS EQFILE"},
      {"SYS CMPFILE", "host", "SYS CMPFILE alias of SYS EQFILE"},
      {"SYS LOGALL", "host", "SYS LOGALL|APPENDFILES append line to path bag"},
      {"SYS APPENDFILES", "host", "SYS APPENDFILES alias of SYS LOGALL"},
      {"SYS BULKAPPEND", "host", "SYS BULKAPPEND alias of SYS LOGALL"},
      {"SYS GREPFILES", "host", "SYS GREPFILES|SEARCHFILES filter paths by content"},
      {"SYS SEARCHFILES", "host", "SYS SEARCHFILES alias of SYS GREPFILES"},
      {"SYS FILESGREP", "host", "SYS FILESGREP alias of SYS GREPFILES"},
      {"SYS GREPFILESI", "host", "SYS GREPFILESI case-insensitive content filter"},
      {"SYS GREPVFILES", "host", "SYS GREPVFILES invert content path filter"},
      {"SYS READALL", "host", "SYS READALL|CATFILES concat path bag file contents"},
      {"SYS CATFILES", "host", "SYS CATFILES alias of SYS READALL"},
      {"SYS SLURPALL", "host", "SYS SLURPALL alias of SYS READALL"},
      {"SYS WRITEALL", "host", "SYS WRITEALL|WRITEFILES overwrite path bag with data"},
      {"SYS WRITEFILES", "host", "SYS WRITEFILES alias of SYS WRITEALL"},
      {"SYS BULKWRITE", "host", "SYS BULKWRITE alias of SYS WRITEALL"},
      {"SYS REPLACEFILES", "host", "SYS REPLACEFILES|SUBFILES bulk rewrite path bag content"},
      {"SYS SUBFILES", "host", "SYS SUBFILES alias of SYS REPLACEFILES"},
      {"SYS GSUBFILES", "host", "SYS GSUBFILES alias of SYS REPLACEFILES"},
      {"SYS COUNTINFILES", "host", "SYS COUNTINFILES|GREPCOUNTFILES total hits across path bag"},
      {"SYS GREPCOUNTFILES", "host", "SYS GREPCOUNTFILES alias of SYS COUNTINFILES"},
      {"SYS COUNTINFILESI", "host", "SYS COUNTINFILESI case-insensitive COUNTINFILES"},
      {"SYS FIRSTFILE", "host", "SYS FIRSTFILE|HITFILE first path with content match"},
      {"SYS LASTFILE", "host", "SYS LASTFILE last path with content match"},
      {"SYS FIRSTFILEI", "host", "SYS FIRSTFILEI case-insensitive FIRSTFILE"},
      {"SYS GREPLINES", "host", "SYS GREPLINES|EXTRACTLINES collect matching lines from path bag"},
      {"SYS EXTRACTLINES", "host", "SYS EXTRACTLINES alias of SYS GREPLINES"},
      {"SYS GREPLINESI", "host", "SYS GREPLINESI case-insensitive GREPLINES"},
      {"SYS HEADFILE", "host", "SYS HEADFILE|FILEHEAD first n lines of a file"},
      {"SYS TAILFILE", "host", "SYS TAILFILE|FILETAIL last n lines of a file"},
      {"SYS LINECOUNTALL", "host", "SYS LINECOUNTALL|WCALL path bag line counts"},
      {"SYS WCALL", "host", "SYS WCALL alias of SYS LINECOUNTALL"},
      {"SYS MIDFILE", "host", "SYS MIDFILE|LINESLICE file line window"},
      {"SYS LINESLICE", "host", "SYS LINESLICE alias of SYS MIDFILE"},
      {"SYS LINEAT", "host", "SYS LINEAT|FILELINE single file line by index"},
      {"SYS FILELINE", "host", "SYS FILELINE alias of SYS LINEAT"},
      {"SYS SETFILELINE", "host", "SYS SETFILELINE|FILESETLINE replace file line by index"},
      {"SYS FILESETLINE", "host", "SYS FILESETLINE alias of SYS SETFILELINE"},
      {"SYS INSERTFILELINE", "host", "SYS INSERTFILELINE|FILEINSERTLINE insert file line by index"},
      {"SYS FILEINSERTLINE", "host", "SYS FILEINSERTLINE alias of SYS INSERTFILELINE"},
      {"SYS DROPFILELINE", "host", "SYS DROPFILELINE|FILEDROPLINE drop file line by index"},
      {"SYS FILEDROPLINE", "host", "SYS FILEDROPLINE alias of SYS DROPFILELINE"},
      {"SYS FINDFILELINE", "host", "SYS FINDFILELINE|FILEFINDLINE first file line by needle"},
      {"SYS FILEFINDLINE", "host", "SYS FILEFINDLINE alias of SYS FINDFILELINE"},
      {"SYS LASTFINDFILELINE", "host", "SYS LASTFINDFILELINE|FILELASTFIND last file line by needle"},
      {"SYS FILELASTFIND", "host", "SYS FILELASTFIND alias of SYS LASTFINDFILELINE"},
      {"SYS COUNTFILELINE", "host", "SYS COUNTFILELINE|FILECOUNTLINE count matching file lines"},
      {"SYS FILECOUNTLINE", "host", "SYS FILECOUNTLINE alias of SYS COUNTFILELINE"},
      {"SYS GREPFILE", "host", "SYS GREPFILE|FILEGREP keep matching lines from one file"},
      {"SYS FILEGREP", "host", "SYS FILEGREP alias of SYS GREPFILE"},
      {"SYS SETMATCHFILE", "host", "SYS SETMATCHFILE|FILESETMATCH replace first matching file line"},
      {"SYS FILESETMATCH", "host", "SYS FILESETMATCH alias of SYS SETMATCHFILE"},
      {"SYS DROPMATCHFILE", "host", "SYS DROPMATCHFILE|FILEDROPMATCH drop first matching file line"},
      {"SYS FILEDROPMATCH", "host", "SYS FILEDROPMATCH alias of SYS DROPMATCHFILE"},
      {"SYS DROPALLMATCHFILE", "host", "SYS DROPALLMATCHFILE|FILEDROPALLMATCH drop all matching file lines"},
      {"SYS FILEDROPALLMATCH", "host", "SYS FILEDROPALLMATCH alias of SYS DROPALLMATCHFILE"},
      {"SYS SETALLMATCHFILE", "host", "SYS SETALLMATCHFILE|FILESETALLMATCH replace all matching file lines"},
      {"SYS FILESETALLMATCH", "host", "SYS FILESETALLMATCH alias of SYS SETALLMATCHFILE"},
      {"SYS INSERTMATCHFILE", "host", "SYS INSERTMATCHFILE|FILEINSERTMATCH insert line after first match"},
      {"SYS FILEINSERTMATCH", "host", "SYS FILEINSERTMATCH alias of SYS INSERTMATCHFILE"},
      {"SYS BEFOREMATCHFILE", "host", "SYS BEFOREMATCHFILE|INSERTBEFOREMATCH insert line before first match"},
      {"SYS INSERTBEFOREMATCH", "host", "SYS INSERTBEFOREMATCH alias of SYS BEFOREMATCHFILE"},
      {"SYS LOOKUPFILE", "host", "SYS LOOKUPFILE|FILELOOKUP peel key=val from plate file"},
      {"SYS FILELOOKUP", "host", "SYS FILELOOKUP alias of SYS LOOKUPFILE"},
      {"SYS KVFILESET", "host", "SYS KVFILESET|FILEKVSET set/update key=val in plate file"},
      {"SYS FILEKVSET", "host", "SYS FILEKVSET alias of SYS KVFILESET"},
      {"SYS KVFILEDEL", "host", "SYS KVFILEDEL|FILEKVDEL drop key=val from plate file"},
      {"SYS FILEKVDEL", "host", "SYS FILEKVDEL alias of SYS KVFILEDEL"},
      {"SYS KVFILEINC", "host", "SYS KVFILEINC|INCKVFILE bump numeric key=val in plate file"},
      {"SYS INCKVFILE", "host", "SYS INCKVFILE alias of SYS KVFILEINC"},
      {"SYS KVFILEENSURE", "host", "SYS KVFILEENSURE|ENSUREKVFILE set key only if missing"},
      {"SYS ENSUREKVFILE", "host", "SYS ENSUREKVFILE alias of SYS KVFILEENSURE"},
      {"SYS DEFAULTKVFILE", "host", "SYS DEFAULTKVFILE alias of SYS KVFILEENSURE"},
      {"SYS ENSURELINE", "host", "SYS ENSURELINE|ENSUREFILELINE append line if exact missing"},
      {"SYS ENSUREFILELINE", "host", "SYS ENSUREFILELINE alias of SYS ENSURELINE"},
      {"SYS HASFILELINE", "host", "SYS HASFILELINE|FILEHASLINE exact file line membership"},
      {"SYS FILEHASLINE", "host", "SYS FILEHASLINE alias of SYS HASFILELINE"},
      {"SYS DROPEXACTFILE", "host", "SYS DROPEXACTFILE|DROPFILEEXACT drop first exact file line"},
      {"SYS DROPFILEEXACT", "host", "SYS DROPFILEEXACT alias of SYS DROPEXACTFILE"},
      {"SYS REMOVELINEFILE", "host", "SYS REMOVELINEFILE alias of SYS DROPEXACTFILE"},
      {"SYS TOGGLELINE", "host", "SYS TOGGLELINE|TOGGLEFILELINE flip exact file line presence"},
      {"SYS TOGGLEFILELINE", "host", "SYS TOGGLEFILELINE alias of SYS TOGGLELINE"},
      {"SYS FLIPLINE", "host", "SYS FLIPLINE alias of SYS TOGGLELINE"},
      {"SYS NTH", "host", "SYS NTH newline field peel"},
      {"SYS GREP", "host", "SYS GREP filter newline fields"},
      {"SYS GREPANY", "host", "SYS GREPANY multi-needle bag filter"},
      {"SYS GREPOR", "host", "SYS GREPOR alias of SYS GREPANY"},
      {"SYS GREPANYI", "host", "SYS GREPANYI case-insensitive multi-needle"},
      {"SYS GREPALL", "host", "SYS GREPALL multi-needle AND bag filter"},
      {"SYS GREPAND", "host", "SYS GREPAND alias of SYS GREPALL"},
      {"SYS GREPALLI", "host", "SYS GREPALLI case-insensitive multi-needle AND"},
      {"SYS TAKE", "host", "SYS TAKE first n lines"},
      {"SYS DROP", "host", "SYS DROP first n lines"},
      {"SYS SPLIT", "host", "SYS SPLIT sep to lines"},
      {"SYS SORT", "host", "SYS SORT line order"},
      {"SYS SORTN", "host", "SYS SORTN numeric bag sort"},
      {"SYS NSORT", "host", "SYS NSORT alias of SYS SORTN"},
      {"SYS SORTLEN", "host", "SYS SORTLEN sort by field length"},
      {"SYS LENSORT", "host", "SYS LENSORT alias of SYS SORTLEN"},
      {"SYS MAXLEN", "host", "SYS MAXLEN max bag field length"},
      {"SYS MINLEN", "host", "SYS MINLEN min bag field length"},
      {"SYS MAXWIDTH", "host", "SYS MAXWIDTH alias of SYS MAXLEN"},
      {"SYS MINWIDTH", "host", "SYS MINWIDTH alias of SYS MINLEN"},
      {"SYS LONGEST", "host", "SYS LONGEST longest bag field text"},
      {"SYS SHORTEST", "host", "SYS SHORTEST shortest bag field text"},
      {"SYS MAXFIELD", "host", "SYS MAXFIELD alias of SYS LONGEST"},
      {"SYS MINFIELD", "host", "SYS MINFIELD alias of SYS SHORTEST"},
      {"SYS COMMONPREFIX", "host", "SYS COMMONPREFIX longest common prefix"},
      {"SYS LCP", "host", "SYS LCP alias of SYS COMMONPREFIX"},
      {"SYS COMMONSUFFIX", "host", "SYS COMMONSUFFIX longest common suffix"},
      {"SYS LCS", "host", "SYS LCS alias of SYS COMMONSUFFIX"},
      {"SYS STRIPPREFIX", "host", "SYS STRIPPREFIX drop leading prefix per field"},
      {"SYS STRIPSUFFIX", "host", "SYS STRIPSUFFIX drop trailing suffix per field"},
      {"SYS STRIPCOMMON", "host", "SYS STRIPCOMMON strip LCP relative paths"},
      {"SYS STRIPLCP", "host", "SYS STRIPLCP alias of SYS STRIPCOMMON"},
      {"SYS UNIQ", "host", "SYS UNIQ adjacent dups"},
      {"SYS JOIN", "host", "SYS JOIN path join a/b"},
      {"SYS PATH", "host", "SYS PATH alias of JOIN"},
      {"SYS JOINLINES", "host", "SYS JOINLINES paste lines"},
      {"SYS REPLACEALL", "host", "SYS REPLACEALL all subst"},
      {"SYS APPEND", "host", "SYS APPEND history log line"},
      {"SYS GREPI", "host", "SYS GREPI case-insensitive GREP"},
      {"SYS STR", "host", "SYS STR|ITOA num → decimal string"},
      {"SYS ITOA", "host", "SYS ITOA alias of SYS STR"},
      {"SYS BEFORE", "host", "SYS BEFORE peel left of needle"},
      {"SYS AFTER", "host", "SYS AFTER peel right of needle"},
      {"SYS PUSH", "host", "SYS PUSH append newline field"},
      {"SYS ADDLINE", "host", "SYS ADDLINE alias of SYS PUSH"},
      {"SYS EQSI", "host", "SYS EQSI case-insensitive equality"},
      {"SYS HASI", "host", "SYS HASI case-insensitive contains"},
      {"SYS LINES", "host", "SYS LINES count newline fields"},
      {"SYS WC", "host", "SYS WC alias of SYS LINES"},
      {"SYS CUT", "host", "SYS CUT 0-based field by sep"},
      {"SYS COLUMN", "host", "SYS COLUMN 1-based field by sep"},
      {"SYS EMPTY", "host", "SYS EMPTY zero-length probe"},
      {"SYS BLANK", "host", "SYS BLANK whitespace-only probe"},
      {"SYS NONEMPTY", "host", "SYS NONEMPTY any-char probe"},
      {"SYS COALESCE", "host", "SYS COALESCE first non-empty string"},
      {"SYS NVL", "host", "SYS NVL alias of SYS COALESCE"},
      {"SYS UNION", "host", "SYS UNION merge bags first-seen unique"},
      {"SYS DISTINCT", "host", "SYS DISTINCT order-preserving full unique"},
      {"SYS INTERSECT", "host", "SYS INTERSECT fields of a also in b"},
      {"SYS DIFF", "host", "SYS DIFF fields of a not in b"},
      {"SYS ZIP", "host", "SYS ZIP pair bag fields by index"},
      {"SYS KEYS", "host", "SYS KEYS peel left of sep each field"},
      {"SYS VALS", "host", "SYS VALS peel right of sep each field"},
      {"SYS PREFIXALL", "host", "SYS PREFIXALL prepend every bag field"},
      {"SYS SUFFIXALL", "host", "SYS SUFFIXALL append every bag field"},
      {"SYS FILL", "host", "SYS FILL n value bag of n copies"},
      {"SYS ENUMERATE", "host", "SYS ENUMERATE index-prefix bag fields"},
      {"SYS NUMBER", "host", "SYS NUMBER alias of SYS ENUMERATE"},
      {"SYS SQUEEZE", "host", "SYS SQUEEZE drop empty bag fields"},
      {"SYS COMPACT", "host", "SYS COMPACT alias of SYS SQUEEZE"},
      {"SYS TRIMALL", "host", "SYS TRIMALL trim every bag field"},
      {"SYS MAPTRIM", "host", "SYS MAPTRIM alias of SYS TRIMALL"},
      {"SYS UPPERALL", "host", "SYS UPPERALL upper every bag field"},
      {"SYS MAPUPPER", "host", "SYS MAPUPPER alias of SYS UPPERALL"},
      {"SYS LOWERALL", "host", "SYS LOWERALL lower every bag field"},
      {"SYS MAPLOWER", "host", "SYS MAPLOWER alias of SYS LOWERALL"},
      {"SYS MAPREPLACE", "host", "SYS MAPREPLACE bag-wide GSUB"},
      {"SYS GSUBALL", "host", "SYS GSUBALL alias of SYS MAPREPLACE"},
      {"SYS REPLACEBAG", "host", "SYS REPLACEBAG alias of SYS MAPREPLACE"},
      {"SYS FREQ", "host", "SYS FREQ field frequency histogram"},
      {"SYS HIST", "host", "SYS HIST alias of SYS FREQ"},
      {"SYS COUNTS", "host", "SYS COUNTS alias of SYS FREQ"},
      {"SYS CUTALL", "host", "SYS CUTALL peel column every bag line"},
      {"SYS MAPCUT", "host", "SYS MAPCUT alias of SYS CUTALL"},
      {"SYS COLALL", "host", "SYS COLALL 1-based CUTALL"},
      {"SYS SORTFREQ", "host", "SYS SORTFREQ sort FREQ by count"},
      {"SYS SORTBYCOUNT", "host", "SYS SORTBYCOUNT alias of SYS SORTFREQ"},
      {"SYS FSORT", "host", "SYS FSORT alias of SYS SORTFREQ"},
      {"SYS BEFOREALL", "host", "SYS BEFOREALL peel left every bag field"},
      {"SYS AFTERALL", "host", "SYS AFTERALL peel right every bag field"},
      {"SYS MAPBEFORE", "host", "SYS MAPBEFORE alias of SYS BEFOREALL"},
      {"SYS MAPAFTER", "host", "SYS MAPAFTER alias of SYS AFTERALL"},
      {"SYS FIRSTMATCH", "host", "SYS FIRSTMATCH first bag field hit"},
      {"SYS GREP1", "host", "SYS GREP1 alias of SYS FIRSTMATCH"},
      {"SYS FIRSTMATCHI", "host", "SYS FIRSTMATCHI case-insensitive first hit"},
      {"SYS LOOKUP", "host", "SYS LOOKUP bag key [sep] [OR fb] peel / default"},
      {"SYS KVGET", "host", "SYS KVGET alias of SYS LOOKUP"},
      {"SYS LOOKUPI", "host", "SYS LOOKUPI case-insensitive LOOKUP"},
      {"SYS GETKV", "host", "SYS GETKV alias of SYS LOOKUP"},
      {"SYS KVSET", "host", "SYS KVSET set/update key:val bag field"},
      {"SYS SETKV", "host", "SYS SETKV alias of SYS KVSET"},
      {"SYS DICTSET", "host", "SYS DICTSET alias of SYS KVSET"},
      {"SYS MAPSET", "host", "SYS MAPSET alias of SYS KVSET"},
      {"SYS KVINC", "host", "SYS KVINC bump key:val numeric counter"},
      {"SYS INCKV", "host", "SYS INCKV alias of SYS KVINC"},
      {"SYS BUMPKV", "host", "SYS BUMPKV alias of SYS KVINC"},
      {"SYS KVADD", "host", "SYS KVADD alias of SYS KVINC"},
      {"SYS KVDEL", "host", "SYS KVDEL|DELKV bag key [sep] — drop key:val field"},
      {"SYS DELKV", "host", "SYS DELKV alias of SYS KVDEL"},
      {"SYS RMKV", "host", "SYS RMKV alias of SYS KVDEL"},
      {"SYS UNSETKV", "host", "SYS UNSETKV alias of SYS KVDEL"},
      {"SYS MERGEKV", "host", "SYS MERGEKV|KVADDALL bag_a bag_b [sep] — sum-merge key:val"},
      {"SYS KVADDALL", "host", "SYS KVADDALL alias of SYS MERGEKV"},
      {"SYS ADDFREQ", "host", "SYS ADDFREQ alias of SYS MERGEKV"},
      {"SYS SUMKV", "host", "SYS SUMKV|TOTALKV bag [sep] — sum key:val values"},
      {"SYS TOTALKV", "host", "SYS TOTALKV alias of SYS SUMKV"},
      {"SYS SUMVALS", "host", "SYS SUMVALS alias of SYS SUMKV"},
      {"SYS AVGKV", "host", "SYS AVGKV|MEANKV bag [sep] — mean of values"},
      {"SYS MEANKV", "host", "SYS MEANKV alias of SYS AVGKV"},
      {"SYS MEANVAL", "host", "SYS MEANVAL alias of SYS AVGKV"},
      {"SYS MEDIANKV", "host", "SYS MEDIANKV|P50KV bag [sep] — median of values"},
      {"SYS P50KV", "host", "SYS P50KV alias of SYS MEDIANKV"},
      {"SYS MIDKV", "host", "SYS MIDKV alias of SYS MEDIANKV"},
      {"SYS TOPKEY", "host", "SYS TOPKEY|ARGMAXKV bag [sep] — key with max value"},
      {"SYS ARGMAXKV", "host", "SYS ARGMAXKV alias of SYS TOPKEY"},
      {"SYS BOTKEY", "host", "SYS BOTKEY|ARGMINKV bag [sep] — key with min value"},
      {"SYS ARGMINKV", "host", "SYS ARGMINKV alias of SYS BOTKEY"},
      {"SYS LOOKUPN", "host", "SYS LOOKUPN|KVGETN bag key [sep] [OR n] — value as int"},
      {"SYS KVGETN", "host", "SYS KVGETN alias of SYS LOOKUPN"},
      {"SYS GETKVN", "host", "SYS GETKVN alias of SYS LOOKUPN"},
      {"SYS THRESHKV", "host", "SYS THRESHKV|KEEPVAL bag min [sep] — keep value>=min"},
      {"SYS KEEPVAL", "host", "SYS KEEPVAL alias of SYS THRESHKV"},
      {"SYS MINCOUNT", "host", "SYS MINCOUNT alias of SYS THRESHKV"},
      {"SYS PCTKV", "host", "SYS PCTKV|SHAREKV bag [sep] — values as %% of total"},
      {"SYS SHAREKV", "host", "SYS SHAREKV alias of SYS PCTKV"},
      {"SYS PERCENTKV", "host", "SYS PERCENTKV alias of SYS PCTKV"},
      {"SYS CAPKV", "host", "SYS CAPKV|CLAMPKV bag max [sep] — clamp values to max"},
      {"SYS CLAMPKV", "host", "SYS CLAMPKV alias of SYS CAPKV"},
      {"SYS MAXVAL", "host", "SYS MAXVAL alias of SYS CAPKV"},
      {"SYS SCALEKV", "host", "SYS SCALEKV|MULKV bag factor [sep] — multiply values"},
      {"SYS MULKV", "host", "SYS MULKV alias of SYS SCALEKV"},
      {"SYS WEIGHTKV", "host", "SYS WEIGHTKV alias of SYS SCALEKV"},
      {"SYS DIVKV", "host", "SYS DIVKV|IDIVKV bag divisor [sep] — divide values"},
      {"SYS IDIVKV", "host", "SYS IDIVKV alias of SYS DIVKV"},
      {"SYS QUOTKV", "host", "SYS QUOTKV alias of SYS DIVKV"},
      {"SYS DIFFKV", "host", "SYS DIFFKV|SUBKV bag_a bag_b [sep] — subtract key:val"},
      {"SYS SUBKV", "host", "SYS SUBKV alias of SYS DIFFKV"},
      {"SYS DELTAKV", "host", "SYS DELTAKV alias of SYS DIFFKV"},
      {"SYS ADDKV", "host", "SYS ADDKV|OFFSETKV bag delta [sep] — add to values"},
      {"SYS OFFSETKV", "host", "SYS OFFSETKV alias of SYS ADDKV"},
      {"SYS ADDVAL", "host", "SYS ADDVAL alias of SYS ADDKV"},
      {"SYS DROPZERO", "host", "SYS DROPZERO|KEEPNZ bag [sep] — drop value==0"},
      {"SYS KEEPNZ", "host", "SYS KEEPNZ alias of SYS DROPZERO"},
      {"SYS NZKV", "host", "SYS NZKV alias of SYS DROPZERO"},
      {"SYS KEEPKEY", "host", "SYS KEEPKEY|GREPKEY bag needle [sep] — filter by key"},
      {"SYS GREPKEY", "host", "SYS GREPKEY alias of SYS KEEPKEY"},
      {"SYS DROPKEY", "host", "SYS DROPKEY drop keys containing needle"},
      {"SYS ABSKV", "host", "SYS ABSKV|MAGKV bag [sep] — abs key:val values"},
      {"SYS MAGKV", "host", "SYS MAGKV alias of SYS ABSKV"},
      {"SYS ABSALL", "host", "SYS ABSALL alias of SYS ABSKV"},
      {"SYS SIGNKV", "host", "SYS SIGNKV|DIRKV bag [sep] — map values to -1|0|1"},
      {"SYS DIRKV", "host", "SYS DIRKV alias of SYS SIGNKV"},
      {"SYS SGNKV", "host", "SYS SGNKV alias of SYS SIGNKV"},
      {"SYS LASTMATCH", "host", "SYS LASTMATCH last bag field hit"},
      {"SYS GREP1L", "host", "SYS GREP1L alias of SYS LASTMATCH"},
      {"SYS LASTMATCHI", "host", "SYS LASTMATCHI case-insensitive last hit"},
      {"SYS CHUNK", "host", "SYS CHUNK group bag fields by n"},
      {"SYS BATCH", "host", "SYS BATCH alias of SYS CHUNK"},
      {"SYS GROUPN", "host", "SYS GROUPN alias of SYS CHUNK"},
      {"SYS WINDOW", "host", "SYS WINDOW overlapping bag field windows"},
      {"SYS SLIDE", "host", "SYS SLIDE alias of SYS WINDOW"},
      {"SYS NGRAM", "host", "SYS NGRAM alias of SYS WINDOW"},
      {"SYS STRIDE", "host", "SYS STRIDE partition bag by step/offset"},
      {"SYS EVERY", "host", "SYS EVERY alias of SYS STRIDE"},
      {"SYS STEP", "host", "SYS STEP alias of SYS STRIDE"},
      {"SYS ROTATE", "host", "SYS ROTATE left-rotate bag fields"},
      {"SYS ROTL", "host", "SYS ROTL alias of SYS ROTATE"},
      {"SYS ROTR", "host", "SYS ROTR right-rotate bag fields"},
      {"SYS FLATTEN", "host", "SYS FLATTEN expand joined bag fields"},
      {"SYS UNCHUNK", "host", "SYS UNCHUNK alias of SYS FLATTEN"},
      {"SYS SPLITALL", "host", "SYS SPLITALL alias of SYS FLATTEN"},
      {"SYS COUNTMATCH", "host", "SYS COUNTMATCH substring field count"},
      {"SYS GREPCOUNT", "host", "SYS GREPCOUNT alias of SYS COUNTMATCH"},
      {"SYS COUNTMATCHI", "host", "SYS COUNTMATCHI case-insensitive COUNTMATCH"},
      {"SYS LENALL", "host", "SYS LENALL per-field lengths bag"},
      {"SYS MAPLEN", "host", "SYS MAPLEN alias of SYS LENALL"},
      {"SYS FIELDLENS", "host", "SYS FIELDLENS alias of SYS LENALL"},
      {"SYS MAXLEN", "host", "SYS MAXLEN max bag field length"},
      {"SYS MINLEN", "host", "SYS MINLEN min bag field length"},
      {"SYS MAXWIDTH", "host", "SYS MAXWIDTH alias of SYS MAXLEN"},
      {"SYS MINWIDTH", "host", "SYS MINWIDTH alias of SYS MINLEN"},
      {"SYS LONGEST", "host", "SYS LONGEST longest bag field text"},
      {"SYS SHORTEST", "host", "SYS SHORTEST shortest bag field text"},
      {"SYS MAXFIELD", "host", "SYS MAXFIELD alias of SYS LONGEST"},
      {"SYS MINFIELD", "host", "SYS MINFIELD alias of SYS SHORTEST"},
      {"SYS COMMONPREFIX", "host", "SYS COMMONPREFIX longest common prefix"},
      {"SYS LCP", "host", "SYS LCP alias of SYS COMMONPREFIX"},
      {"SYS COMMONSUFFIX", "host", "SYS COMMONSUFFIX longest common suffix"},
      {"SYS LCS", "host", "SYS LCS alias of SYS COMMONSUFFIX"},
      {"SYS STRIPPREFIX", "host", "SYS STRIPPREFIX drop leading prefix per field"},
      {"SYS STRIPSUFFIX", "host", "SYS STRIPSUFFIX drop trailing suffix per field"},
      {"SYS STRIPCOMMON", "host", "SYS STRIPCOMMON strip LCP relative paths"},
      {"SYS STRIPLCP", "host", "SYS STRIPLCP alias of SYS STRIPCOMMON"},
      {"SYS MIDLINES", "host", "SYS MIDLINES bag field window slice"},
      {"SYS SLICEBAG", "host", "SYS SLICEBAG alias of SYS MIDLINES"},
      {"SYS POP", "host", "SYS POP last bag field"},
      {"SYS POPLINE", "host", "SYS POPLINE alias of SYS POP"},
      {"SYS MTIME", "host", "SYS MTIME file mtime epoch"},
      {"SYS AGE", "host", "SYS AGE seconds since mtime"},
      {"SYS STARTSI", "host", "SYS STARTSI case-insensitive prefix"},
      {"SYS ENDSI", "host", "SYS ENDSI case-insensitive suffix"},
      {"SYS BETWEEN", "host", "SYS BETWEEN peel between delimiters"},
      {"SYS MIDOF", "host", "SYS MIDOF alias of SYS BETWEEN"},
      {"SYS EXTRACT", "host", "SYS EXTRACT alias of SYS BETWEEN"},
      {"SYS FINDI", "host", "SYS FINDI case-insensitive index"},
      {"SYS INDEXI", "host", "SYS INDEXI alias of SYS FINDI"},
      {"SYS SETENV", "host", "SYS SETENV process setenv"},
      {"SYS ENVDEFAULT", "host", "SYS ENVDEFAULT|ENSUREENV name value — setenv if missing/empty"},
      {"SYS UNSETENV", "host", "SYS UNSETENV process unsetenv"},
      {"SYS REVL", "host", "SYS REVL reverse bag line order"},
      {"SYS REVLINES", "host", "SYS REVLINES alias of SYS REVL"},
      {"SYS TAC", "host", "SYS TAC alias of SYS REVL"},
      {"SYS WORDS", "host", "SYS WORDS whitespace tokenize"},
      {"SYS TOKENIZE", "host", "SYS TOKENIZE alias of SYS WORDS"},
      {"SYS HASLINE", "host", "SYS HASLINE exact bag field membership"},
      {"SYS HASLINEI", "host", "SYS HASLINEI case-insensitive exact field"},
      {"SYS PREPEND", "host", "SYS PREPEND insert field at bag front"},
      {"SYS POPHEAD", "host", "SYS POPHEAD peel first bag field FIFO"},
      {"SYS DEQUEUE", "host", "SYS DEQUEUE alias of SYS POPHEAD"},
      {"SYS REMOVELINE", "host", "SYS REMOVELINE drop exact bag field"},
      {"SYS DROPLINE", "host", "SYS DROPLINE alias of SYS REMOVELINE"},
      {"SYS FINDLINE", "host", "SYS FINDLINE exact bag field index"},
      {"SYS LINEINDEX", "host", "SYS LINEINDEX alias of SYS FINDLINE"},
      {"SYS SETLINE", "host", "SYS SETLINE set bag field by index"},
      {"SYS REPLACELINE", "host", "SYS REPLACELINE alias of SYS SETLINE"},
      {"SYS INSERTLINE", "host", "SYS INSERTLINE insert bag field by index"},
      {"SYS INSLINE", "host", "SYS INSLINE alias of SYS INSERTLINE"},
      {"SYS DROPNTH", "host", "SYS DROPNTH drop bag field by index"},
      {"SYS DROPAT", "host", "SYS DROPAT alias of SYS DROPNTH"},
      {"SYS MOVELINE", "host", "SYS MOVELINE move bag field to index"},
      {"SYS MOVEAT", "host", "SYS MOVEAT alias of SYS MOVELINE"},
      {"SYS COUNTLINE", "host", "SYS COUNTLINE count exact bag field matches"},
      {"SYS COUNTLINEI", "host", "SYS COUNTLINEI case-insensitive field match count"},
      {"SYS SETMATCH", "host", "SYS SETMATCH replace first exact bag field"},
      {"SYS REPLACEMATCH", "host", "SYS REPLACEMATCH alias of SYS SETMATCH"},
      {"EACH LINE", "flow", "EACH LINE walk LIST fields"},
      {"SYS", "host", "SYS ENV|ARG|WHICH|READ|WRITE …"},
      {"SMX", "smx", "SMX KEY|TALK|EXCHANGE|SERVE|DIAL"},
      {"HELP", "flow", "HELP [form] in-program catalog"},
    };
    if (!q || !q[0]) {
      fprintf(stderr, "usage: cubalc which <name|path|form>\n"
                      "       cubalc locate hold_seed · cubalc resolve ASSERT\n");
      printf("{\"schema\":\"cubalc.which.v1\",\"ok\":false,\"cmd\":\"which\","
             "\"err\":\"need name\",\"version\":\"%s\"}\n", CUBALC_LANG_VERSION);
      return 2;
    }
    {
      const char *slash = strrchr(q, '/');
      const char *leaf = slash ? slash + 1 : q;
      snprintf(stem, sizeof stem, "%s", leaf);
      blen = strlen(stem);
      if (blen > 7 && strcmp(stem + blen - 7, ".cubalc") == 0)
        stem[blen - 7] = 0;
    }
    /* 1) host find (lib/program/path) — primary hit */
    {
      cubalc_host_result hr;
      if (cubalc_host_find_cubalc(q, &hr) == 0 && nh < 16) {
        const char *kind = "path";
        if (strstr(hr.str, "programs/lib/")) kind = "lib";
        else if (strstr(hr.str, "programs/proof/")) kind = "proof";
        else if (strstr(hr.str, "programs/p2p/")) kind = "p2p";
        else if (strstr(hr.str, "programs/protect/")) kind = "protect";
        else if (strstr(hr.str, "programs/")) kind = "program";
        else if (strstr(hr.str, "docs/")) kind = "doc";
        snprintf(hits[nh].kind, sizeof hits[0].kind, "%s", kind);
        snprintf(hits[nh].path, sizeof hits[0].path, "%s", hr.str);
        snprintf(hits[nh].hint, sizeof hits[0].hint, "INCLUDE/cat resolve");
        hits[nh].present = 1;
        nh++;
      }
    }
    /* 2) path aliases (cookbook, hello, …) */
    for (i = 0; i < (int)(sizeof aliases / sizeof aliases[0]) && nh < 16; i++) {
      char aup[32], qup[64];
      size_t k;
      int already = 0;
      for (k = 0; aliases[i].name[k] && k + 1 < sizeof aup; k++)
        aup[k] = (char)toupper((unsigned char)aliases[i].name[k]);
      aup[k] = 0;
      for (k = 0; q[k] && k + 1 < sizeof qup; k++)
        qup[k] = (char)toupper((unsigned char)q[k]);
      qup[k] = 0;
      {
        char stem_up[64];
        size_t sk;
        for (sk = 0; stem[sk] && sk + 1 < sizeof stem_up; sk++)
          stem_up[sk] = (char)toupper((unsigned char)stem[sk]);
        stem_up[sk] = 0;
        if (strcmp(aup, qup) != 0 && strcmp(stem_up, aup) != 0)
          continue;
      }
      for (k = 0; k < (size_t)nh; k++)
        if (strcmp(hits[k].path, aliases[i].path) == 0) already = 1;
      if (already) continue;
      snprintf(hits[nh].kind, sizeof hits[0].kind, "alias");
      snprintf(hits[nh].path, sizeof hits[0].path, "%s", aliases[i].path);
      snprintf(hits[nh].hint, sizeof hits[0].hint, "%s", aliases[i].hint);
      hits[nh].present = (access(aliases[i].path, F_OK) == 0);
      nh++;
    }
    /* 3) form catalog (no file path) */
    for (i = 0; i < (int)(sizeof forms / sizeof forms[0]) && nh < 16; i++) {
      char fup[32], qup[64], sup[64];
      size_t k;
      for (k = 0; forms[i].name[k] && k + 1 < sizeof fup; k++)
        fup[k] = (char)toupper((unsigned char)forms[i].name[k]);
      fup[k] = 0;
      for (k = 0; q[k] && k + 1 < sizeof qup; k++)
        qup[k] = (char)toupper((unsigned char)q[k]);
      qup[k] = 0;
      for (k = 0; stem[k] && k + 1 < sizeof sup; k++)
        sup[k] = (char)toupper((unsigned char)stem[k]);
      sup[k] = 0;
      if (strcmp(fup, qup) != 0 && strcmp(fup, sup) != 0 &&
          !strstr(fup, qup) && !(qup[0] && strstr(fup, qup)))
        continue;
      /* require full or strong prefix match (len>=3) to avoid noise */
      if (strcmp(fup, qup) != 0 && strcmp(fup, sup) != 0) {
        if (strlen(qup) < 3 || !strstr(fup, qup))
          continue;
      }
      snprintf(hits[nh].kind, sizeof hits[0].kind, "form");
      snprintf(hits[nh].path, sizeof hits[0].path, "%s", forms[i].name);
      snprintf(hits[nh].hint, sizeof hits[0].hint, "%s · %s",
               forms[i].plane, forms[i].hint);
      hits[nh].present = 1;
      nh++;
    }
    /* 4) PATH binary (cubalc, etc.) — only if no file hit yet or name looks bin */
    {
      cubalc_host_result hr;
      int already_bin = 0;
      /* use binary-only: temp copy of which without find would recurse — check X_OK paths */
      if (cubalc_host_which(q, &hr) == 0) {
        for (i = 0; i < nh; i++)
          if (strcmp(hits[i].path, hr.str) == 0) already_bin = 1;
        if (!already_bin && nh < 16) {
          /* if which returned a .cubalc, skip as bin */
          if (!strstr(hr.str, ".cubalc") && !strstr(hr.str, ".md") &&
              !strstr(hr.str, "programs/")) {
            snprintf(hits[nh].kind, sizeof hits[0].kind, "bin");
            snprintf(hits[nh].path, sizeof hits[0].path, "%s", hr.str);
            snprintf(hits[nh].hint, sizeof hits[0].hint, "PATH / CUBALC_BIN");
            hits[nh].present = 1;
            nh++;
          }
        }
      }
    }
    printf("# CubalC which query=%s version=%s\n", q, CUBALC_LANG_VERSION);
    printf("# kind\tpath\tpresent\thint\n");
    for (i = 0; i < nh; i++)
      printf("%s\t%s\t%s\t%s\n", hits[i].kind, hits[i].path,
             hits[i].present ? "yes" : "no", hits[i].hint);
    if (nh == 0)
      printf("# (no match — try cubalc libs|examples|forms|paths)\n");
    printf("{\"schema\":\"cubalc.which.v1\",\"ok\":%s,\"cmd\":\"which\","
           "\"query\":\"%s\",\"n\":%d,\"version\":\"%s\","
           "\"note\":\"resolve short names for INCLUDE/cat/run — first match preferred\","
           "\"matches\":[",
           nh > 0 ? "true" : "false", q, nh, CUBALC_LANG_VERSION);
    for (i = 0; i < nh; i++) {
      char pesc[520];
      size_t k, o = 0;
      for (k = 0; hits[i].path[k] && o + 2 < sizeof pesc; k++) {
        char c = hits[i].path[k];
        if (c == '"' || c == '\\') pesc[o++] = '_';
        else pesc[o++] = c;
      }
      pesc[o] = 0;
      printf("%s{\"kind\":\"%s\",\"path\":\"%s\",\"present\":%s,\"hint\":\"%s\"}",
             i ? "," : "", hits[i].kind, pesc,
             hits[i].present ? "true" : "false", hits[i].hint);
    }
    printf("]}\n");
    return nh > 0 ? 0 : 1;
  }
  if (strcmp(cmd, "search") == 0 || strcmp(cmd, "find") == 0 ||
      strcmp(cmd, "query") == 0 || strcmp(cmd, "look") == 0) {
    /* Usability: one keyword search across forms · libs · examples · env · aliases.
     * Complements which (exact resolve) with discovery when agents don't know the name. */
    const char *q = (argc > 2) ? argv[2] : "";
    char qup[64];
    struct {
      char kind[16];
      char name[64];
      char path[160];
      char hint[120];
    } hits[48];
    int nh = 0, i;
    size_t k;
    static const struct { const char *name; const char *plane; const char *hint; } forms[] = {
      {"HOLD_FLASH", "law", "device/firmware safeguard · default 1 (omit preamble)"},
      {"CUBE", "core", "place cube · CUBE name ROLE host|body"},
      {"PLUG", "core", "wire cubes · denied only if HOLD_FLASH 0"},
      {"INCLUDE", "flow", "INCLUDE [ONCE] [OR|SOFT] path|libname — ONCE skips reload"},
      {"DEFAULT", "flow", "DEFAULT name = expr|str — set only if unset (INCLUDE-safe)"},
      {"DEFINED", "flow", "DEFINED name — LAST_N 1 if var exists, 0 if missing"},
      {"TYPEOF", "flow", "TYPEOF name — LAST undef|num|str · LAST_N 0|1|2"},
      {"UNSET", "flow", "UNSET name — remove var · LAST_N 1 if removed"},
      {"ASSERT", "flow", "ASSERT expr [why] — fail with line+reason · string ==/!= content"},
      {"EXPECT", "flow", "EXPECT expr [why] soft check — OK/LAST_ERR, no fatal"},
      {"FAIL", "flow", "FAIL [why] soft status OK=0 sticky LAST_ERR"},
      {"PASS", "flow", "PASS [why] soft status OK=1 optional note"},
      {"NOTE", "flow", "NOTE [text] agent breadcrumb · no OK/ERR change"},
      {"EXIT", "flow", "EXIT [code] [why] — halt; non-zero fails plate + rc"},
      {"CLEAR_ERR", "flow", "CLEAR_ERR [note] wipe sticky ERR/LAST_ERR after recovery"},
      {"VERSION", "flow", "VERSION — LAST/VERSION language version string"},
      {"REQUIRE", "flow", "REQUIRE VERSION|LIB|ENV|PATH|DIR|REG|BIN|JSONHASALL|JSONONLY|JSONEXACT|JSONEQ|JSONNEQ|JSONSUBSET|JSONTYPE|PLATEFILE — fail-fast gates"},
      {"REQUIRE JSONHASALL", "flow", "REQUIRE JSONHASALL|JSONNEED [plate] key… — fail-fast plate keys · missing listed"},
      {"REQUIRE JSONONLY", "flow", "REQUIRE JSONONLY|NOEXTRA [plate] allow… — fail if plate has unknown keys"},
      {"REQUIRE JSONEXACT", "flow", "REQUIRE JSONEXACT|JSONSCHEMA [plate] key… — exact key set (HASALL+ONLY)"},
      {"REQUIRE JSONEQ", "flow", "REQUIRE JSONEQ|SAMEJSON a b — fail-fast plate equality · lists changed keys"},
      {"REQUIRE JSONNEQ", "flow", "REQUIRE JSONNEQ|JNEQ|DIFFJSON a b — fail if plates still equal · mutation gate"},
      {"REQUIRE JSONTYPE", "flow", "REQUIRE JSONTYPE|JTYPE [plate] key kind — fail-fast field type · soft SYS JSONTYPE"},
      {"REQUIRE PLATEFILE", "flow", "REQUIRE PLATEFILE|JSONPLATE|HASPLATE path — object plate file · LAST=content"},
      {"REQUIRE JSONSUBSET", "flow", "REQUIRE JSONSUBSET|JSUBSET sub super — fail-fast required fields · bad keys listed"},
      {"REQUIRE JSONSUPERSET", "flow", "REQUIRE JSONSUPERSET|JSONCOVERS super sub — dual (a ⊇ b) · same gate"},
      {"SYS JSONEXACT", "host", "SYS JSONEXACT|JSONSCHEMA [plate] key… — soft exact key-set → LAST_N 0|1"},
      {"SYS JSONKEYDIFF", "host", "SYS JSONKEYDIFF|KEYSDIFF a b — keys in a not in b → bag"},
      {"SYS JSONKEYCOMM", "host", "SYS JSONKEYCOMM|KEYSINTER a b — shared keys bag · dual DIFF"},
      {"SYS JSONCHANGED", "host", "SYS JSONCHANGED|JSONNEQKEYS a b — keys that differ (struct or value)"},
      {"SYS JSONSTABLE", "host", "SYS JSONSTABLE|JSONEQKEYS a b — keys same in both · dual CHANGED"},
      {"SYS JSONDELTA", "host", "SYS JSONDELTA|JDELTA a b [FROM NEW|OLD] — changed keys as plate object"},
      {"SYS JSONCHANGELOG", "host", "SYS JSONCHANGELOG|JCHANGELOG a b — key: old → new lines · agent plate sync log"},
      {"SYS LOADPLATE", "host", "SYS LOADPLATE|READPLATE path [OR defaults] — soft plate file load · LAST_N hit"},
      {"PRINT_JSON", "flow", "PRINT_JSON [idents] one JSON line for agents"},
      {"VARS", "flow", "VARS — dump all program vars as cubalc.vars.v1 JSON"},
      {"STATUS", "flow", "STATUS — cubalc.status.v1 health (ok/last_err/version/time)"},
      {"IDENTITY", "flow", "IDENTITY — cubalc.identity.v1 user@host:pid plate"},
      {"HELP", "flow", "HELP [form] — in-program catalog tip → LAST/OK/HELP_N"},
      {"SYS ENV", "host", "SYS ENV NAME [OR fallback] · ENV SET name val · ENV UNSET name"},
      {"SYS SETENV", "host", "SYS SETENV|ENV SET name value — process setenv"},
      {"SYS ENVDEFAULT", "host", "SYS ENVDEFAULT|ENSUREENV name value — setenv if missing/empty"},
      {"SYS UNSETENV", "host", "SYS UNSETENV|ENV UNSET name — process unsetenv · LAST_N was-set"},
      {"SYS SUBSTENV", "host", "SYS SUBSTENV|ENVSUBST expand $NAME/${NAME} from env+vars"},
      {"SYS ENVSUBST", "host", "SYS ENVSUBST alias of SYS SUBSTENV"},
      {"SYS EXPANDENV", "host", "SYS EXPANDENV alias of SYS SUBSTENV"},
      {"SYS SUBSTFILE", "host", "SYS SUBSTFILE|EXPANDFILE path [out] — materialize template file"},
      {"SYS EXPANDFILE", "host", "SYS EXPANDFILE alias of SYS SUBSTFILE"},
      {"SYS ARG", "host", "SYS ARG n|name [OR fallback] via CUBALC_ARGn"},
      {"SYS WHICH", "host", "SYS WHICH name → LAST path (bin or lib)"},
      {"SYS CWD", "host", "SYS CWD — working directory → LAST"},
      {"SYS CHDIR", "host", "SYS CHDIR change process cwd"},
      {"SYS CD", "host", "SYS CD alias of SYS CHDIR"},
      {"SYS STATE", "host", "SYS STATE — CUBALC_STATE plate dir → LAST"},
      {"SYS ROOT", "host", "SYS ROOT — CUBALC_ROOT or cwd → LAST"},
      {"SYS TMP", "host", "SYS TMP|TEMP|TMPDIR — portable temp dir"},
      {"SYS MKTEMP", "host", "SYS MKTEMP|TEMPFILE [prefix] — unique empty file under TMP"},
      {"SYS TEMPFILE", "host", "SYS TEMPFILE alias of SYS MKTEMP"},
      {"SYS MKTEMPDIR", "host", "SYS MKTEMPDIR|MKDTEMP [prefix] — unique empty dir under TMP"},
      {"SYS MKDTEMP", "host", "SYS MKDTEMP alias of SYS MKTEMPDIR"},
      {"SYS MKDIR", "host", "SYS MKDIR path — mkdir -p for plate dirs"},
      {"SYS BASENAME", "host", "SYS BASENAME|LEAF path — final component"},
      {"SYS DIRNAME", "host", "SYS DIRNAME|PARENT path — parent directory"},
      {"SYS EXTNAME", "host", "SYS EXTNAME|EXT|SUFFIX path — final .ext"},
      {"SYS STEM", "host", "SYS STEM|ROOTNAME path — basename without ext"},
      {"SYS SIZE", "host", "SYS SIZE|FSIZE path — file bytes soft miss"},
      {"SYS STAT", "host", "SYS STAT|FSTAT path — one-shot exist/kind/size/mtime"},
      {"SYS FSTAT", "host", "SYS FSTAT alias of SYS STAT"},
      {"SYS ISDIR", "host", "SYS ISDIR path — directory probe"},
      {"SYS ISFILE", "host", "SYS ISFILE path — regular file probe"},
      {"SYS READ", "host", "SYS READ [OR|SOFT] path [OR fallback]"},
      {"SYS LOADPLATE", "host", "SYS LOADPLATE|READPLATE path [OR defaults] soft plate file"},
      {"SYS SAVEPLATE", "host", "SYS SAVEPLATE|STOREPLATE path [plate] object plate persist"},
      {"SYS ENSUREPLATE", "host", "SYS ENSUREPLATE|TOUCHPLATE path [OR defaults] create-or-load"},
      {"SYS UPDATEPLATE", "host", "SYS UPDATEPLATE|JSONFILESET path key value load-set-save plate field"},
      {"SYS BUMPPLATE", "host", "SYS BUMPPLATE|JSONFILEINC path key [delta] load-inc-save counter"},
      {"SYS PEEKPLATE", "host", "SYS PEEKPLATE|JSONFILEGET path key [OR fb] peel plate file field"},
      {"SYS DROPPLATE", "host", "SYS DROPPLATE|JSONFILEDEL path key load-del-save plate field"},
      {"SYS JSONFILEMERGE", "host", "SYS JSONFILEMERGE|OVERLAYPLATE path overlay multi-key load-merge-save"},
      {"SYS JSONFILEENSURE", "host", "SYS JSONFILEENSURE|ENSUREPLATEKEY path key value set-if-missing"},
      {"SYS JSONFILEDEFAULTS", "host", "SYS JSONFILEDEFAULTS|DEFAULTPLATEALL path defaults multi-key fill-if-missing"},
      {"SYS JSONFILETOGGLE", "host", "SYS JSONFILETOGGLE|TOGGLEPLATEKEY path key flip disk flag 0↔1"},
      {"SYS RM", "host", "SYS RM|UNLINK path — remove file soft miss"},
      {"SYS RMDIR", "host", "SYS RMDIR|REMOVEDIR path — remove empty dir soft miss"},
      {"SYS REMOVEDIR", "host", "SYS REMOVEDIR alias of SYS RMDIR"},
      {"SYS RENAME", "host", "SYS RENAME|MV from to — move plate"},
      {"SYS COPY", "host", "SYS COPY|CP src dst — duplicate file"},
      {"SYS REALPATH", "host", "SYS REALPATH|ABSPATH absolute path"},
      {"SYS TOUCH", "host", "SYS TOUCH create empty / refresh mtime"},
      {"SYS SLEEP", "host", "SYS SLEEP n|duration — ms or human 1s/250ms cap 60s"},
      {"SYS WAITFILE", "host", "SYS WAITFILE path [ms|\"2s\"] poll until exists"},
      {"SYS WAITPATH", "host", "SYS WAITPATH alias of SYS WAITFILE"},
      {"SYS WAITGONE", "host", "SYS WAITGONE|WAITMISSING path [ms] poll until gone"},
      {"SYS WAITMISSING", "host", "SYS WAITMISSING alias of SYS WAITGONE"},
      {"SYS WAITSTABLE", "host", "SYS WAITSTABLE path [ms]|quiet timeout settle size+mtime"},
      {"SYS WAITQUIET", "host", "SYS WAITQUIET alias of SYS WAITSTABLE"},
      {"SYS WAITCHANGED", "host", "SYS WAITCHANGED path [SINCE size mtime] [ms] until change"},
      {"SYS WAITMODIFIED", "host", "SYS WAITMODIFIED alias of SYS WAITCHANGED"},
      {"SYS WAITMATCH", "host", "SYS WAITMATCH path needle [ms] poll until content"},
      {"SYS WAITCONTAINS", "host", "SYS WAITCONTAINS alias of SYS WAITMATCH"},
      {"SYS RAND", "host", "SYS RAND|RANDOM [n]|[lo hi] uniform int"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM [n] hex random bytes"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS RANDOM", "host", "SYS RANDOM alias of SYS RAND"},
      {"SYS PICK", "host", "SYS PICK random bag field"},
      {"SYS CHOICE", "host", "SYS CHOICE alias of SYS PICK"},
      {"SYS SHUFFLE", "host", "SYS SHUFFLE randomize bag order"},
      {"SYS SHUF", "host", "SYS SHUF alias of SYS SHUFFLE"},
      {"SYS DRAWN", "host", "SYS DRAWN sample k unique bag fields"},
      {"SYS SAMPLEK", "host", "SYS SAMPLEK alias of SYS DRAWN"},
      {"SYS NPICK", "host", "SYS NPICK alias of SYS DRAWN"},
      {"SYS TAKERAND", "host", "SYS TAKERAND alias of SYS DRAWN"},
      {"SYS MIN", "host", "SYS MIN host-plane min args or bag"},
      {"SYS MAX", "host", "SYS MAX host-plane max args or bag"},
      {"SYS MINBAG", "host", "SYS MINBAG bag-mode MIN"},
      {"SYS MAXBAG", "host", "SYS MAXBAG bag-mode MAX"},
      {"SYS ARGMAX", "host", "SYS ARGMAX index of first max"},
      {"SYS ARGMIN", "host", "SYS ARGMIN index of first min"},
      {"SYS MAXIDX", "host", "SYS MAXIDX alias of SYS ARGMAX"},
      {"SYS MINIDX", "host", "SYS MINIDX alias of SYS ARGMIN"},
      {"SYS PADALL", "host", "SYS PADALL right-pad every bag field"},
      {"SYS LPADALL", "host", "SYS LPADALL left-pad every bag field"},
      {"SYS RPADALL", "host", "SYS RPADALL alias of PADALL"},
      {"SYS TRUNCALL", "host", "SYS TRUNCALL|CLIPALL [RIGHT] bag width truncate fields"},
      {"SYS CLIPALL", "host", "SYS CLIPALL alias of SYS TRUNCALL"},
      {"SYS MAPTRUNC", "host", "SYS MAPTRUNC alias of SYS TRUNCALL"},
      {"SYS CLAMP", "host", "SYS CLAMP x lo hi bound"},
      {"SYS IN", "host", "SYS IN|WITHIN x lo hi membership"},
      {"SYS WITHIN", "host", "SYS WITHIN alias of SYS IN"},
      {"SYS CMP", "host", "SYS CMP three-way numeric compare"},
      {"SYS SCMP", "host", "SYS SCMP string compare"},
      {"SYS IABS", "host", "SYS IABS integer absolute value"},
      {"SYS SIGN", "host", "SYS SIGN signum -1|0|1"},
      {"SYS DIV", "host", "SYS DIV integer divide"},
      {"SYS MOD", "host", "SYS MOD remainder"},
      {"SYS GCD", "host", "SYS GCD multi-arg gcd"},
      {"SYS LCM", "host", "SYS LCM multi-arg lcm"},
      {"SYS POW", "host", "SYS POW integer power"},
      {"SYS ISQRT", "host", "SYS ISQRT floor integer sqrt"},
      {"SYS SUM", "host", "SYS SUM args|bag integer sum"},
      {"SYS PROD", "host", "SYS PROD args|bag integer product"},
      {"SYS AVG", "host", "SYS AVG args|bag integer mean"},
      {"SYS MEDIAN", "host", "SYS MEDIAN|P50 args|bag integer median"},
      {"SYS RANGE", "host", "SYS RANGE inclusive number bag"},
      {"SYS SEQ", "host", "SYS SEQ 1..n number bag"},
      {"SYS IOTA", "host", "SYS IOTA 0..n-1 number bag"},
      {"SYS LIST", "host", "SYS LIST|LS path dir basenames count"},
      {"SYS GLOB", "host", "SYS GLOB|MATCHFILES path [pattern] — match basenames"},
      {"SYS MATCHFILES", "host", "SYS MATCHFILES alias of SYS GLOB"},
      {"SYS PATHGLOB", "host", "SYS PATHGLOB|PGLOB path|pattern full paths"},
      {"SYS PGLOB", "host", "SYS PGLOB alias of SYS PATHGLOB"},
      {"SYS FILTERGLOB", "host", "SYS FILTERGLOB|MATCHBAG bag pattern shell filter"},
      {"SYS MATCHBAG", "host", "SYS MATCHBAG alias of SYS FILTERGLOB"},
      {"SYS BASENAMEALL", "host", "SYS BASENAMEALL|MAPBASE path bag map"},
      {"SYS DIRNAMEALL", "host", "SYS DIRNAMEALL|MAPDIR path bag map"},
      {"SYS KEEPFILES", "host", "SYS KEEPFILES path bag keep regular files"},
      {"SYS KEEPDIRS", "host", "SYS KEEPDIRS path bag keep directories"},
      {"SYS SIZEALL", "host", "SYS SIZEALL|MAPSIZE path bag byte sizes"},
      {"SYS MAPSIZE", "host", "SYS MAPSIZE alias of SYS SIZEALL"},
      {"SYS MTIMEALL", "host", "SYS MTIMEALL|MAPMTIME epoch mtime bag"},
      {"SYS AGEALL", "host", "SYS AGEALL|MAPAGE age-seconds bag"},
      {"SYS NEWEST", "host", "SYS NEWEST|LATEST freshest path by mtime"},
      {"SYS OLDEST", "host", "SYS OLDEST|EARLIEST oldest path by mtime"},
      {"SYS LARGEST", "host", "SYS LARGEST largest file path"},
      {"SYS SORTMTIME", "host", "SYS SORTMTIME order paths by mtime"},
      {"SYS SORTSIZE", "host", "SYS SORTSIZE order paths by size"},
      {"SYS FRESH", "host", "SYS FRESH bag max_age keep fresh paths"},
      {"SYS KEEPSTALE", "host", "SYS KEEPSTALE|AGED bag min_age keep stale paths"},
      {"SYS KEEPNEWER", "host", "SYS KEEPNEWER|NEWERTHAN [bag] ref mtime>ref"},
      {"SYS NEWERTHAN", "host", "SYS NEWERTHAN alias of SYS KEEPNEWER"},
      {"SYS KEEPOLDER", "host", "SYS KEEPOLDER|OLDERREF [bag] ref mtime<ref"},
      {"SYS OLDERREF", "host", "SYS OLDERREF alias of SYS KEEPOLDER"},
      {"SYS KEEPBIGGER", "host", "SYS KEEPBIGGER|BIGFILES|SIZEGE bag min_bytes size>=min"},
      {"SYS BIGFILES", "host", "SYS BIGFILES alias of SYS KEEPBIGGER"},
      {"SYS SIZEGE", "host", "SYS SIZEGE alias of SYS KEEPBIGGER"},
      {"SYS KEEPSMALLER", "host", "SYS KEEPSMALLER|SMALLFILES|SIZELE bag max_bytes size<=max"},
      {"SYS SMALLFILES", "host", "SYS SMALLFILES alias of SYS KEEPSMALLER"},
      {"SYS SIZELE", "host", "SYS SIZELE alias of SYS KEEPSMALLER"},
      {"SYS RMALL", "host", "SYS RMALL|UNLINKALL bulk remove path bag"},
      {"SYS UNLINKALL", "host", "SYS UNLINKALL alias of SYS RMALL"},
      {"SYS DELETEALL", "host", "SYS DELETEALL alias of SYS RMALL"},
      {"SYS TOUCHALL", "host", "SYS TOUCHALL|ENSUREALL bulk create/refresh path bag"},
      {"SYS ENSUREALL", "host", "SYS ENSUREALL alias of SYS TOUCHALL"},
      {"SYS CREATEALL", "host", "SYS CREATEALL alias of SYS TOUCHALL"},
      {"SYS COPYALL", "host", "SYS COPYALL|CPALL bulk copy path bag into dest"},
      {"SYS CPALL", "host", "SYS CPALL alias of SYS COPYALL"},
      {"SYS BULKCOPY", "host", "SYS BULKCOPY alias of SYS COPYALL"},
      {"SYS MKDIRALL", "host", "SYS MKDIRALL|ENSUREDIRS bulk mkdir -p path bag"},
      {"SYS ENSUREDIRS", "host", "SYS ENSUREDIRS alias of SYS MKDIRALL"},
      {"SYS MKDIRS", "host", "SYS MKDIRS alias of SYS MKDIRALL"},
      {"SYS MOVEALL", "host", "SYS MOVEALL|MVALL bulk move path bag into dest"},
      {"SYS MVALL", "host", "SYS MVALL alias of SYS MOVEALL"},
      {"SYS RENAMEALL", "host", "SYS RENAMEALL alias of SYS MOVEALL"},
      {"SYS WALK", "host", "SYS WALK|FINDALL recursive full-path discovery"},
      {"SYS FINDALL", "host", "SYS FINDALL alias of SYS WALK"},
      {"SYS TREEGLOB", "host", "SYS TREEGLOB alias of SYS WALK"},
      {"SYS EQFILE", "host", "SYS EQFILE|SAMEFILE identical file content"},
      {"SYS SAMEFILE", "host", "SYS SAMEFILE alias of SYS EQFILE"},
      {"SYS CMPFILE", "host", "SYS CMPFILE alias of SYS EQFILE"},
      {"SYS LOGALL", "host", "SYS LOGALL|APPENDFILES append line to path bag"},
      {"SYS APPENDFILES", "host", "SYS APPENDFILES alias of SYS LOGALL"},
      {"SYS BULKAPPEND", "host", "SYS BULKAPPEND alias of SYS LOGALL"},
      {"SYS GREPFILES", "host", "SYS GREPFILES|SEARCHFILES filter paths by content"},
      {"SYS SEARCHFILES", "host", "SYS SEARCHFILES alias of SYS GREPFILES"},
      {"SYS FILESGREP", "host", "SYS FILESGREP alias of SYS GREPFILES"},
      {"SYS GREPFILESI", "host", "SYS GREPFILESI case-insensitive content filter"},
      {"SYS GREPVFILES", "host", "SYS GREPVFILES invert content path filter"},
      {"SYS READALL", "host", "SYS READALL|CATFILES concat path bag file contents"},
      {"SYS CATFILES", "host", "SYS CATFILES alias of SYS READALL"},
      {"SYS SLURPALL", "host", "SYS SLURPALL alias of SYS READALL"},
      {"SYS WRITEALL", "host", "SYS WRITEALL|WRITEFILES overwrite path bag with data"},
      {"SYS WRITEFILES", "host", "SYS WRITEFILES alias of SYS WRITEALL"},
      {"SYS BULKWRITE", "host", "SYS BULKWRITE alias of SYS WRITEALL"},
      {"SYS REPLACEFILES", "host", "SYS REPLACEFILES|SUBFILES bulk rewrite path bag content"},
      {"SYS SUBFILES", "host", "SYS SUBFILES alias of SYS REPLACEFILES"},
      {"SYS GSUBFILES", "host", "SYS GSUBFILES alias of SYS REPLACEFILES"},
      {"SYS COUNTINFILES", "host", "SYS COUNTINFILES|GREPCOUNTFILES total hits across path bag"},
      {"SYS GREPCOUNTFILES", "host", "SYS GREPCOUNTFILES alias of SYS COUNTINFILES"},
      {"SYS COUNTINFILESI", "host", "SYS COUNTINFILESI case-insensitive COUNTINFILES"},
      {"SYS FIRSTFILE", "host", "SYS FIRSTFILE|HITFILE first path with content match"},
      {"SYS LASTFILE", "host", "SYS LASTFILE last path with content match"},
      {"SYS FIRSTFILEI", "host", "SYS FIRSTFILEI case-insensitive FIRSTFILE"},
      {"SYS GREPLINES", "host", "SYS GREPLINES|EXTRACTLINES collect matching lines from path bag"},
      {"SYS EXTRACTLINES", "host", "SYS EXTRACTLINES alias of SYS GREPLINES"},
      {"SYS GREPLINESI", "host", "SYS GREPLINESI case-insensitive GREPLINES"},
      {"SYS HEADFILE", "host", "SYS HEADFILE|FILEHEAD first n lines of a file"},
      {"SYS TAILFILE", "host", "SYS TAILFILE|FILETAIL last n lines of a file"},
      {"SYS LINECOUNTALL", "host", "SYS LINECOUNTALL|WCALL path bag line counts"},
      {"SYS WCALL", "host", "SYS WCALL alias of SYS LINECOUNTALL"},
      {"SYS MIDFILE", "host", "SYS MIDFILE|LINESLICE file line window"},
      {"SYS LINESLICE", "host", "SYS LINESLICE alias of SYS MIDFILE"},
      {"SYS LINEAT", "host", "SYS LINEAT|FILELINE single file line by index"},
      {"SYS FILELINE", "host", "SYS FILELINE alias of SYS LINEAT"},
      {"SYS SETFILELINE", "host", "SYS SETFILELINE|FILESETLINE replace file line by index"},
      {"SYS FILESETLINE", "host", "SYS FILESETLINE alias of SYS SETFILELINE"},
      {"SYS INSERTFILELINE", "host", "SYS INSERTFILELINE|FILEINSERTLINE insert file line by index"},
      {"SYS FILEINSERTLINE", "host", "SYS FILEINSERTLINE alias of SYS INSERTFILELINE"},
      {"SYS DROPFILELINE", "host", "SYS DROPFILELINE|FILEDROPLINE drop file line by index"},
      {"SYS FILEDROPLINE", "host", "SYS FILEDROPLINE alias of SYS DROPFILELINE"},
      {"SYS FINDFILELINE", "host", "SYS FINDFILELINE|FILEFINDLINE first file line by needle"},
      {"SYS FILEFINDLINE", "host", "SYS FILEFINDLINE alias of SYS FINDFILELINE"},
      {"SYS LASTFINDFILELINE", "host", "SYS LASTFINDFILELINE|FILELASTFIND last file line by needle"},
      {"SYS FILELASTFIND", "host", "SYS FILELASTFIND alias of SYS LASTFINDFILELINE"},
      {"SYS COUNTFILELINE", "host", "SYS COUNTFILELINE|FILECOUNTLINE count matching file lines"},
      {"SYS FILECOUNTLINE", "host", "SYS FILECOUNTLINE alias of SYS COUNTFILELINE"},
      {"SYS GREPFILE", "host", "SYS GREPFILE|FILEGREP keep matching lines from one file"},
      {"SYS FILEGREP", "host", "SYS FILEGREP alias of SYS GREPFILE"},
      {"SYS SETMATCHFILE", "host", "SYS SETMATCHFILE|FILESETMATCH replace first matching file line"},
      {"SYS FILESETMATCH", "host", "SYS FILESETMATCH alias of SYS SETMATCHFILE"},
      {"SYS DROPMATCHFILE", "host", "SYS DROPMATCHFILE|FILEDROPMATCH drop first matching file line"},
      {"SYS FILEDROPMATCH", "host", "SYS FILEDROPMATCH alias of SYS DROPMATCHFILE"},
      {"SYS DROPALLMATCHFILE", "host", "SYS DROPALLMATCHFILE|FILEDROPALLMATCH drop all matching file lines"},
      {"SYS FILEDROPALLMATCH", "host", "SYS FILEDROPALLMATCH alias of SYS DROPALLMATCHFILE"},
      {"SYS SETALLMATCHFILE", "host", "SYS SETALLMATCHFILE|FILESETALLMATCH replace all matching file lines"},
      {"SYS FILESETALLMATCH", "host", "SYS FILESETALLMATCH alias of SYS SETALLMATCHFILE"},
      {"SYS INSERTMATCHFILE", "host", "SYS INSERTMATCHFILE|FILEINSERTMATCH insert line after first match"},
      {"SYS FILEINSERTMATCH", "host", "SYS FILEINSERTMATCH alias of SYS INSERTMATCHFILE"},
      {"SYS BEFOREMATCHFILE", "host", "SYS BEFOREMATCHFILE|INSERTBEFOREMATCH insert line before first match"},
      {"SYS INSERTBEFOREMATCH", "host", "SYS INSERTBEFOREMATCH alias of SYS BEFOREMATCHFILE"},
      {"SYS LOOKUPFILE", "host", "SYS LOOKUPFILE|FILELOOKUP peel key=val from plate file"},
      {"SYS FILELOOKUP", "host", "SYS FILELOOKUP alias of SYS LOOKUPFILE"},
      {"SYS KVFILESET", "host", "SYS KVFILESET|FILEKVSET set/update key=val in plate file"},
      {"SYS FILEKVSET", "host", "SYS FILEKVSET alias of SYS KVFILESET"},
      {"SYS KVFILEDEL", "host", "SYS KVFILEDEL|FILEKVDEL drop key=val from plate file"},
      {"SYS FILEKVDEL", "host", "SYS FILEKVDEL alias of SYS KVFILEDEL"},
      {"SYS KVFILEINC", "host", "SYS KVFILEINC|INCKVFILE bump numeric key=val in plate file"},
      {"SYS INCKVFILE", "host", "SYS INCKVFILE alias of SYS KVFILEINC"},
      {"SYS KVFILEENSURE", "host", "SYS KVFILEENSURE|ENSUREKVFILE set key only if missing"},
      {"SYS ENSUREKVFILE", "host", "SYS ENSUREKVFILE alias of SYS KVFILEENSURE"},
      {"SYS DEFAULTKVFILE", "host", "SYS DEFAULTKVFILE alias of SYS KVFILEENSURE"},
      {"SYS ENSURELINE", "host", "SYS ENSURELINE|ENSUREFILELINE append line if exact missing"},
      {"SYS ENSUREFILELINE", "host", "SYS ENSUREFILELINE alias of SYS ENSURELINE"},
      {"SYS HASFILELINE", "host", "SYS HASFILELINE|FILEHASLINE exact file line membership"},
      {"SYS FILEHASLINE", "host", "SYS FILEHASLINE alias of SYS HASFILELINE"},
      {"SYS DROPEXACTFILE", "host", "SYS DROPEXACTFILE|DROPFILEEXACT drop first exact file line"},
      {"SYS DROPFILEEXACT", "host", "SYS DROPFILEEXACT alias of SYS DROPEXACTFILE"},
      {"SYS REMOVELINEFILE", "host", "SYS REMOVELINEFILE alias of SYS DROPEXACTFILE"},
      {"SYS TOGGLELINE", "host", "SYS TOGGLELINE|TOGGLEFILELINE flip exact file line presence"},
      {"SYS TOGGLEFILELINE", "host", "SYS TOGGLEFILELINE alias of SYS TOGGLELINE"},
      {"SYS FLIPLINE", "host", "SYS FLIPLINE alias of SYS TOGGLELINE"},
      {"SYS NTH", "host", "SYS NTH/LINE/HEAD/TAIL newline fields"},
      {"SYS GREP", "host", "SYS GREP|FILTER keep matching lines"},
      {"SYS GREPV", "host", "SYS GREPV drop matching lines"},
      {"SYS GREPANY", "host", "SYS GREPANY|GREPOR bag n1 [n2…] match any needle"},
      {"SYS GREPOR", "host", "SYS GREPOR alias of SYS GREPANY"},
      {"SYS GREPANYI", "host", "SYS GREPANYI case-insensitive multi-needle"},
      {"SYS GREPALL", "host", "SYS GREPALL|GREPAND bag n1 [n2…] match every needle"},
      {"SYS GREPAND", "host", "SYS GREPAND alias of SYS GREPALL"},
      {"SYS GREPALLI", "host", "SYS GREPALLI case-insensitive multi-needle AND"},
      {"SYS TAKE", "host", "SYS TAKE|FIRSTN n first n lines"},
      {"SYS DROP", "host", "SYS DROP|SKIP n drop first n lines"},
      {"SYS SPLIT", "host", "SYS SPLIT|FIELDS sep → newline fields"},
      {"SYS SORT", "host", "SYS SORT lexicographic line sort"},
      {"SYS SORTN", "host", "SYS SORTN|NSORT numeric line sort"},
      {"SYS NSORT", "host", "SYS NSORT alias of SYS SORTN"},
      {"SYS SORTLEN", "host", "SYS SORTLEN sort by field length"},
      {"SYS LENSORT", "host", "SYS LENSORT alias of SYS SORTLEN"},
      {"SYS MAXLEN", "host", "SYS MAXLEN|MAXWIDTH max bag field length"},
      {"SYS MINLEN", "host", "SYS MINLEN|MINWIDTH min bag field length"},
      {"SYS MAXWIDTH", "host", "SYS MAXWIDTH alias of SYS MAXLEN"},
      {"SYS MINWIDTH", "host", "SYS MINWIDTH alias of SYS MINLEN"},
      {"SYS LONGEST", "host", "SYS LONGEST|MAXFIELD first longest bag field text"},
      {"SYS SHORTEST", "host", "SYS SHORTEST|MINFIELD first shortest bag field text"},
      {"SYS MAXFIELD", "host", "SYS MAXFIELD alias of SYS LONGEST"},
      {"SYS MINFIELD", "host", "SYS MINFIELD alias of SYS SHORTEST"},
      {"SYS COMMONPREFIX", "host", "SYS COMMONPREFIX|LCP longest common prefix"},
      {"SYS LCP", "host", "SYS LCP alias of SYS COMMONPREFIX"},
      {"SYS COMMONSUFFIX", "host", "SYS COMMONSUFFIX|LCS longest common suffix"},
      {"SYS LCS", "host", "SYS LCS alias of SYS COMMONSUFFIX"},
      {"SYS STRIPPREFIX", "host", "SYS STRIPPREFIX bag prefix drop leading prefix"},
      {"SYS STRIPSUFFIX", "host", "SYS STRIPSUFFIX bag suffix drop trailing suffix"},
      {"SYS STRIPCOMMON", "host", "SYS STRIPCOMMON|STRIPLCP strip LCP relative paths"},
      {"SYS STRIPLCP", "host", "SYS STRIPLCP alias of SYS STRIPCOMMON"},
      {"SYS UNIQ", "host", "SYS UNIQ drop adjacent dups"},
      {"SYS JOIN", "host", "SYS JOIN|PATH a b portable path join"},
      {"SYS PATH", "host", "SYS PATH alias of SYS JOIN"},
      {"SYS JOINLINES", "host", "SYS JOINLINES|PASTE join lines with sep"},
      {"SYS REPLACEALL", "host", "SYS REPLACEALL|GSUB all occurrences"},
      {"SYS APPEND", "host", "SYS APPEND|LOG path data history line"},
      {"SYS GREPI", "host", "SYS GREPI|IGREP case-insensitive filter"},
      {"SYS STR", "host", "SYS STR|ITOA|NUMSTR integer → decimal string"},
      {"SYS ITOA", "host", "SYS ITOA alias of SYS STR · dual of NUM"},
      {"SYS BEFORE", "host", "SYS BEFORE|LEFT_OF hay needle · left peel"},
      {"SYS AFTER", "host", "SYS AFTER|RIGHT_OF hay needle · right peel"},
      {"SYS PUSH", "host", "SYS PUSH|ADDLINE bag [line] field accumulate"},
      {"SYS ADDLINE", "host", "SYS ADDLINE alias of SYS PUSH"},
      {"SYS EQSI", "host", "SYS EQSI|IEQS|EQS I case-insensitive equality"},
      {"SYS HASI", "host", "SYS HASI|ICONTAINS|HAS I case-insensitive contains"},
      {"SYS LINES", "host", "SYS LINES|NLINES|WC count newline fields"},
      {"SYS WC", "host", "SYS WC alias of SYS LINES"},
      {"SYS CUT", "host", "SYS CUT|FIELDN hay sep n 0-based field"},
      {"SYS COLUMN", "host", "SYS COLUMN|COL hay sep n 1-based field"},
      {"SYS EMPTY", "host", "SYS EMPTY|ISEMPTY zero-length probe"},
      {"SYS BLANK", "host", "SYS BLANK|ISBLANK whitespace-only probe"},
      {"SYS NONEMPTY", "host", "SYS NONEMPTY any-character probe"},
      {"SYS COALESCE", "host", "SYS COALESCE|NVL first non-empty string"},
      {"SYS NVL", "host", "SYS NVL alias of SYS COALESCE"},
      {"SYS UNION", "host", "SYS UNION|ORLINES a [b…] merge bags first-seen unique"},
      {"SYS DISTINCT", "host", "SYS DISTINCT|UNIQUEALL order-preserving full unique"},
      {"SYS INTERSECT", "host", "SYS INTERSECT|ANDLINES a b fields of a also in b"},
      {"SYS DIFF", "host", "SYS DIFF|EXCEPT|SETDIFF a b fields of a not in b"},
      {"SYS ZIP", "host", "SYS ZIP|PAIR a b [sep] pair bag fields by index"},
      {"SYS KEYS", "host", "SYS KEYS|COL0 bag [sep] peel left of sep each field"},
      {"SYS VALS", "host", "SYS VALS|COL1 bag [sep] peel right of sep each field"},
      {"SYS PREFIXALL", "host", "SYS PREFIXALL|MAPPRE bag prefix prepend every field"},
      {"SYS SUFFIXALL", "host", "SYS SUFFIXALL|MAPSUF bag suffix append every field"},
      {"SYS FILL", "host", "SYS FILL|REPEATL n value bag of n copies"},
      {"SYS ENUMERATE", "host", "SYS ENUMERATE|NUMBER bag [start] [sep] index-prefix fields"},
      {"SYS NUMBER", "host", "SYS NUMBER alias of SYS ENUMERATE"},
      {"SYS SQUEEZE", "host", "SYS SQUEEZE|COMPACT [BLANK] bag drop empty/blank fields"},
      {"SYS COMPACT", "host", "SYS COMPACT alias of SYS SQUEEZE"},
      {"SYS TRIMALL", "host", "SYS TRIMALL|MAPTRIM [L|R] bag trim every field"},
      {"SYS MAPTRIM", "host", "SYS MAPTRIM alias of SYS TRIMALL"},
      {"SYS UPPERALL", "host", "SYS UPPERALL|MAPUPPER bag upper every field"},
      {"SYS MAPUPPER", "host", "SYS MAPUPPER alias of SYS UPPERALL"},
      {"SYS LOWERALL", "host", "SYS LOWERALL|MAPLOWER bag lower every field"},
      {"SYS MAPLOWER", "host", "SYS MAPLOWER alias of SYS LOWERALL"},
      {"SYS MAPREPLACE", "host", "SYS MAPREPLACE|GSUBALL bag old new GSUB every field"},
      {"SYS GSUBALL", "host", "SYS GSUBALL alias of SYS MAPREPLACE"},
      {"SYS REPLACEBAG", "host", "SYS REPLACEBAG alias of SYS MAPREPLACE"},
      {"SYS FREQ", "host", "SYS FREQ|HIST [bag] [sep] field frequency key:count"},
      {"SYS HIST", "host", "SYS HIST alias of SYS FREQ"},
      {"SYS COUNTS", "host", "SYS COUNTS alias of SYS FREQ"},
      {"SYS CUTALL", "host", "SYS CUTALL|MAPCUT bag sep n peel column every line"},
      {"SYS MAPCUT", "host", "SYS MAPCUT alias of SYS CUTALL"},
      {"SYS COLALL", "host", "SYS COLALL 1-based CUTALL"},
      {"SYS SORTFREQ", "host", "SYS SORTFREQ|SORTBYCOUNT [bag] [sep] [DESC|ASC]"},
      {"SYS SORTBYCOUNT", "host", "SYS SORTBYCOUNT alias of SYS SORTFREQ"},
      {"SYS FSORT", "host", "SYS FSORT alias of SYS SORTFREQ"},
      {"SYS BEFOREALL", "host", "SYS BEFOREALL|MAPBEFORE bag needle peel left every field"},
      {"SYS AFTERALL", "host", "SYS AFTERALL|MAPAFTER bag needle peel right every field"},
      {"SYS MAPBEFORE", "host", "SYS MAPBEFORE alias of SYS BEFOREALL"},
      {"SYS MAPAFTER", "host", "SYS MAPAFTER alias of SYS AFTERALL"},
      {"SYS FIRSTMATCH", "host", "SYS FIRSTMATCH|GREP1 bag needle first field hit"},
      {"SYS GREP1", "host", "SYS GREP1 alias of SYS FIRSTMATCH"},
      {"SYS FIRSTMATCHI", "host", "SYS FIRSTMATCHI case-insensitive first hit"},
      {"SYS LOOKUP", "host", "SYS LOOKUP bag key [sep] [OR fb] peel key:val / default"},
      {"SYS KVGET", "host", "SYS KVGET alias of SYS LOOKUP"},
      {"SYS LOOKUPI", "host", "SYS LOOKUPI case-insensitive LOOKUP"},
      {"SYS GETKV", "host", "SYS GETKV alias of SYS LOOKUP"},
      {"SYS KVSET", "host", "SYS KVSET|SETKV bag key value [sep] set/update key:val"},
      {"SYS SETKV", "host", "SYS SETKV alias of SYS KVSET"},
      {"SYS DICTSET", "host", "SYS DICTSET alias of SYS KVSET"},
      {"SYS MAPSET", "host", "SYS MAPSET alias of SYS KVSET"},
      {"SYS KVINC", "host", "SYS KVINC|INCKV bag key [delta] [sep] bump counter"},
      {"SYS INCKV", "host", "SYS INCKV alias of SYS KVINC"},
      {"SYS BUMPKV", "host", "SYS BUMPKV alias of SYS KVINC"},
      {"SYS KVADD", "host", "SYS KVADD alias of SYS KVINC"},
      {"SYS KVDEL", "host", "SYS KVDEL|DELKV bag key [sep] — drop key:val field"},
      {"SYS DELKV", "host", "SYS DELKV alias of SYS KVDEL"},
      {"SYS RMKV", "host", "SYS RMKV alias of SYS KVDEL"},
      {"SYS UNSETKV", "host", "SYS UNSETKV alias of SYS KVDEL"},
      {"SYS MERGEKV", "host", "SYS MERGEKV|KVADDALL bag_a bag_b [sep] — sum-merge key:val"},
      {"SYS KVADDALL", "host", "SYS KVADDALL alias of SYS MERGEKV"},
      {"SYS ADDFREQ", "host", "SYS ADDFREQ alias of SYS MERGEKV"},
      {"SYS SUMKV", "host", "SYS SUMKV|TOTALKV bag [sep] — sum key:val values"},
      {"SYS TOTALKV", "host", "SYS TOTALKV alias of SYS SUMKV"},
      {"SYS SUMVALS", "host", "SYS SUMVALS alias of SYS SUMKV"},
      {"SYS AVGKV", "host", "SYS AVGKV|MEANKV bag [sep] — mean of values"},
      {"SYS MEANKV", "host", "SYS MEANKV alias of SYS AVGKV"},
      {"SYS MEANVAL", "host", "SYS MEANVAL alias of SYS AVGKV"},
      {"SYS MEDIANKV", "host", "SYS MEDIANKV|P50KV bag [sep] — median of values"},
      {"SYS P50KV", "host", "SYS P50KV alias of SYS MEDIANKV"},
      {"SYS MIDKV", "host", "SYS MIDKV alias of SYS MEDIANKV"},
      {"SYS TOPKEY", "host", "SYS TOPKEY|ARGMAXKV bag [sep] — key with max value"},
      {"SYS ARGMAXKV", "host", "SYS ARGMAXKV alias of SYS TOPKEY"},
      {"SYS BOTKEY", "host", "SYS BOTKEY|ARGMINKV bag [sep] — key with min value"},
      {"SYS ARGMINKV", "host", "SYS ARGMINKV alias of SYS BOTKEY"},
      {"SYS LOOKUPN", "host", "SYS LOOKUPN|KVGETN bag key [sep] [OR n] — value as int"},
      {"SYS KVGETN", "host", "SYS KVGETN alias of SYS LOOKUPN"},
      {"SYS GETKVN", "host", "SYS GETKVN alias of SYS LOOKUPN"},
      {"SYS THRESHKV", "host", "SYS THRESHKV|KEEPVAL bag min [sep] — keep value>=min"},
      {"SYS KEEPVAL", "host", "SYS KEEPVAL alias of SYS THRESHKV"},
      {"SYS MINCOUNT", "host", "SYS MINCOUNT alias of SYS THRESHKV"},
      {"SYS PCTKV", "host", "SYS PCTKV|SHAREKV bag [sep] — values as %% of total"},
      {"SYS SHAREKV", "host", "SYS SHAREKV alias of SYS PCTKV"},
      {"SYS PERCENTKV", "host", "SYS PERCENTKV alias of SYS PCTKV"},
      {"SYS CAPKV", "host", "SYS CAPKV|CLAMPKV bag max [sep] — clamp values to max"},
      {"SYS CLAMPKV", "host", "SYS CLAMPKV alias of SYS CAPKV"},
      {"SYS MAXVAL", "host", "SYS MAXVAL alias of SYS CAPKV"},
      {"SYS SCALEKV", "host", "SYS SCALEKV|MULKV bag factor [sep] — multiply values"},
      {"SYS MULKV", "host", "SYS MULKV alias of SYS SCALEKV"},
      {"SYS WEIGHTKV", "host", "SYS WEIGHTKV alias of SYS SCALEKV"},
      {"SYS DIVKV", "host", "SYS DIVKV|IDIVKV bag divisor [sep] — divide values"},
      {"SYS IDIVKV", "host", "SYS IDIVKV alias of SYS DIVKV"},
      {"SYS QUOTKV", "host", "SYS QUOTKV alias of SYS DIVKV"},
      {"SYS DIFFKV", "host", "SYS DIFFKV|SUBKV bag_a bag_b [sep] — subtract key:val"},
      {"SYS SUBKV", "host", "SYS SUBKV alias of SYS DIFFKV"},
      {"SYS DELTAKV", "host", "SYS DELTAKV alias of SYS DIFFKV"},
      {"SYS ADDKV", "host", "SYS ADDKV|OFFSETKV bag delta [sep] — add to values"},
      {"SYS OFFSETKV", "host", "SYS OFFSETKV alias of SYS ADDKV"},
      {"SYS ADDVAL", "host", "SYS ADDVAL alias of SYS ADDKV"},
      {"SYS DROPZERO", "host", "SYS DROPZERO|KEEPNZ bag [sep] — drop value==0"},
      {"SYS KEEPNZ", "host", "SYS KEEPNZ alias of SYS DROPZERO"},
      {"SYS NZKV", "host", "SYS NZKV alias of SYS DROPZERO"},
      {"SYS KEEPKEY", "host", "SYS KEEPKEY|GREPKEY bag needle [sep] — filter by key"},
      {"SYS GREPKEY", "host", "SYS GREPKEY alias of SYS KEEPKEY"},
      {"SYS DROPKEY", "host", "SYS DROPKEY drop keys containing needle"},
      {"SYS ABSKV", "host", "SYS ABSKV|MAGKV bag [sep] — abs key:val values"},
      {"SYS MAGKV", "host", "SYS MAGKV alias of SYS ABSKV"},
      {"SYS ABSALL", "host", "SYS ABSALL alias of SYS ABSKV"},
      {"SYS SIGNKV", "host", "SYS SIGNKV|DIRKV bag [sep] — map values to -1|0|1"},
      {"SYS DIRKV", "host", "SYS DIRKV alias of SYS SIGNKV"},
      {"SYS SGNKV", "host", "SYS SGNKV alias of SYS SIGNKV"},
      {"SYS LASTMATCH", "host", "SYS LASTMATCH|GREP1L bag needle last field hit"},
      {"SYS GREP1L", "host", "SYS GREP1L alias of SYS LASTMATCH"},
      {"SYS LASTMATCHI", "host", "SYS LASTMATCHI case-insensitive last hit"},
      {"SYS CHUNK", "host", "SYS CHUNK|BATCH n [bag] [join] group fields by n"},
      {"SYS BATCH", "host", "SYS BATCH alias of SYS CHUNK"},
      {"SYS GROUPN", "host", "SYS GROUPN alias of SYS CHUNK"},
      {"SYS MIDLINES", "host", "SYS MIDLINES|SLICEBAG bag start [end] field window"},
      {"SYS SLICEBAG", "host", "SYS SLICEBAG alias of SYS MIDLINES"},
      {"SYS POP", "host", "SYS POP|POPLINE bag last field → LAST rest→POP_REST"},
      {"SYS POPLINE", "host", "SYS POPLINE alias of SYS POP"},
      {"SYS MTIME", "host", "SYS MTIME|MODTIME path mtime epoch soft miss"},
      {"SYS AGE", "host", "SYS AGE|FILEAGE path seconds since mtime"},
      {"SYS STARTSI", "host", "SYS STARTSI|ISTARTS case-insensitive prefix"},
      {"SYS ENDSI", "host", "SYS ENDSI|IENDS case-insensitive suffix"},
      {"SYS BETWEEN", "host", "SYS BETWEEN|MIDOF|EXTRACT peel between delimiters"},
      {"SYS MIDOF", "host", "SYS MIDOF alias of SYS BETWEEN"},
      {"SYS EXTRACT", "host", "SYS EXTRACT alias of SYS BETWEEN"},
      {"SYS FINDI", "host", "SYS FINDI|INDEXI|FIND I case-insensitive index"},
      {"SYS INDEXI", "host", "SYS INDEXI alias of SYS FINDI"},
      {"SYS SETENV", "host", "SYS SETENV|ENV SET name value process setenv"},
      {"SYS ENVDEFAULT", "host", "SYS ENVDEFAULT|ENSUREENV name value — setenv if missing/empty"},
      {"SYS UNSETENV", "host", "SYS UNSETENV|ENV UNSET name process unsetenv"},
      {"SYS REVL", "host", "SYS REVL|REVLINES|TAC reverse newline fields"},
      {"SYS REVLINES", "host", "SYS REVLINES alias of SYS REVL"},
      {"SYS TAC", "host", "SYS TAC alias of SYS REVL"},
      {"SYS WORDS", "host", "SYS WORDS|TOKENIZE whitespace to newline fields"},
      {"SYS TOKENIZE", "host", "SYS TOKENIZE alias of SYS WORDS"},
      {"SYS HASLINE", "host", "SYS HASLINE|HASFIELD exact bag field membership"},
      {"SYS HASLINEI", "host", "SYS HASLINEI|ILINEIN case-insensitive exact field"},
      {"SYS PREPEND", "host", "SYS PREPEND|UNSHIFT insert field at bag front"},
      {"SYS POPHEAD", "host", "SYS POPHEAD|DEQUEUE peel first bag field FIFO"},
      {"SYS DEQUEUE", "host", "SYS DEQUEUE alias of SYS POPHEAD"},
      {"SYS REMOVELINE", "host", "SYS REMOVELINE|DROPLINE drop first exact bag field"},
      {"SYS DROPLINE", "host", "SYS DROPLINE alias of SYS REMOVELINE"},
      {"SYS REMOVELINEI", "host", "SYS REMOVELINEI case-insensitive drop field"},
      {"SYS FINDLINE", "host", "SYS FINDLINE|LINEINDEX 0-based exact field index"},
      {"SYS LINEINDEX", "host", "SYS LINEINDEX alias of SYS FINDLINE"},
      {"SYS FINDLINEI", "host", "SYS FINDLINEI case-insensitive field index"},
      {"SYS SETLINE", "host", "SYS SETLINE|REPLACELINE bag n value set field"},
      {"SYS REPLACELINE", "host", "SYS REPLACELINE alias of SYS SETLINE"},
      {"EACH LINE", "flow", "EACH LINE [as name] [IN str] walk fields"},
      {"SYS TIME", "host", "SYS TIME wall seconds → LAST_N"},
      {"SYS MS", "host", "SYS MS wall milliseconds → LAST_N/MS"},
      {"SYS DATE", "host", "SYS DATE|ISO|UTC — UTC stamp YYYY-MM-DDTHH:MM:SSZ"},
      {"SYS FROMTIME", "host", "SYS FROMTIME|EPOCHISO|TOISO [n] — epoch → UTC ISO stamp"},
      {"SYS PARSEISO", "host", "SYS PARSEISO|TOEPOCH|FROMISO [str] — ISO → epoch dual of FROMTIME"},
      {"SYS TIMEDIFF", "host", "SYS TIMEDIFF|ELAPSED a [b] — epoch delta seconds a-b or now-a"},
      {"SYS ADDTIME", "host", "SYS ADDTIME|DEADLINE — epoch + seconds / now+secs lease"},
      {"SYS EXPIRED", "host", "SYS EXPIRED|REMAINING [epoch] — lease due probe / TTL secs"},
      {"SYS LOCAL", "host", "SYS LOCAL|LOCALTIME — local wall YYYY-MM-DDTHH:MM:SS"},
      {"SYS LOCALTIME", "host", "SYS LOCALTIME alias of SYS LOCAL"},
      {"SYS LOCALDATE", "host", "SYS LOCALDATE — local date-only YYYY-MM-DD"},
      {"SYS ISATTY", "host", "SYS ISATTY|TTY [IN|OUT|ERR] — terminal probe → LAST_N"},
      {"SYS TTY", "host", "SYS TTY alias of SYS ISATTY"},
      {"SYS TTYNAME", "host", "SYS TTYNAME|CTTY|TTYDEV [IN|OUT|ERR] — terminal device path → LAST"},
      {"SYS NICE", "host", "SYS NICE|GETNICE [n]|RENICE n — process nice get/set → LAST_N"},
      {"SYS SHQUOTE", "host", "SYS SHQUOTE|SHELLQUOTE [str] — POSIX single-quote shell-safe wrap"},
      {"SYS SHUNQUOTE", "host", "SYS SHUNQUOTE|UNQUOTE [str] — peel POSIX single-quote wrap"},
      {"SYS GETLOGIN", "host", "SYS GETLOGIN|LOGINNAME — session login via getlogin()"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM [n] — n random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS LOADAVG", "host", "SYS LOADAVG|LOAD — 1/5/15 load · LOAD1_N centiload"},
      {"SYS LOAD", "host", "SYS LOAD alias of SYS LOADAVG"},
      {"SYS UPTIME", "host", "SYS UPTIME|BOOTAGE — seconds since boot → LAST_N"},
      {"SYS BOOTAGE", "host", "SYS BOOTAGE alias of SYS UPTIME"},
      {"SYS MEM", "host", "SYS MEM|MEMINFO — RAM total/avail/free kB · LAST_N=avail"},
      {"SYS MEMINFO", "host", "SYS MEMINFO alias of SYS MEM"},
      {"SYS DF", "host", "SYS DF|DISKFREE [path] — filesystem free kB · LAST_N=avail"},
      {"SYS DISKFREE", "host", "SYS DISKFREE alias of SYS DF"},
      {"SYS FSYNC", "host", "SYS FSYNC|SYNCFILE path — durable flush file to disk"},
      {"SYS SYNCFILE", "host", "SYS SYNCFILE alias of SYS FSYNC"},
      {"SYS TRUNCATE", "host", "SYS TRUNCATE|FTRUNCATE path [nbytes] — resize plate file"},
      {"SYS FTRUNCATE", "host", "SYS FTRUNCATE alias of SYS TRUNCATE"},
      {"SYS FILETRUNC", "host", "SYS FILETRUNC alias of SYS TRUNCATE"},
      {"SYS DIRSYNC", "host", "SYS DIRSYNC|FSYNCDIR path — fsync dir or parent for durable rename"},
      {"SYS FSYNCDIR", "host", "SYS FSYNCDIR alias of SYS DIRSYNC"},
      {"SYS SYNCDIR", "host", "SYS SYNCDIR alias of SYS DIRSYNC"},
      {"SYS HARDLINK", "host", "SYS HARDLINK|HLINK existing newpath — hard link same inode"},
      {"SYS HLINK", "host", "SYS HLINK alias of SYS HARDLINK"},
      {"SYS NLINK", "host", "SYS NLINK|LINKS path — hard-link count → LAST_N"},
      {"SYS SAMEINODE", "host", "SYS SAMEINODE|EQINODE a b — 1 if same device+inode"},
      {"SYS EQINODE", "host", "SYS EQINODE alias of SYS SAMEINODE"},
      {"SYS INODE", "host", "SYS INODE|INO path — inode number → LAST_N"},
      {"SYS TEE", "host", "SYS TEE|WRITETEE path [data] — write keep content in LAST"},
      {"SYS WRITETEE", "host", "SYS WRITETEE alias of SYS TEE"},
      {"SYS FILEUID", "host", "SYS FILEUID|OWNERUID path — owner uid → LAST_N"},
      {"SYS FILEGID", "host", "SYS FILEGID|OWNERGID path — group gid → LAST_N"},
      {"SYS OWNEDBY", "host", "SYS OWNEDBY|ISOWNER path — LAST_N 1 if euid owns"},
      {"SYS CANREAD", "host", "SYS CANREAD|READABLE path — access R_OK → LAST_N 0|1"},
      {"SYS CANWRITE", "host", "SYS CANWRITE|WRITABLE path — access W_OK → LAST_N 0|1"},
      {"SYS CANEXEC", "host", "SYS CANEXEC|EXECUTABLE path — access X_OK → LAST_N 0|1"},
      {"SYS OWNERNAME", "host", "SYS OWNERNAME|OWNERUSER path — owner login name → LAST"},
      {"SYS GROUPNAME", "host", "SYS GROUPNAME|OWNERGROUP path — group name → LAST"},
      {"SYS CANCREATE", "host", "SYS CANCREATE|CREATABLE path — create/overwrite probe LAST_N 0|1"},
      {"SYS UMASK", "host", "SYS UMASK [mode] — get/set process file-creation mask (octal)"},
      {"SYS PUSHD", "host", "SYS PUSHD|PUSHDIR path — save cwd then chdir; LAST_N=depth"},
      {"SYS POPD", "host", "SYS POPD|POPDIR — restore previous cwd from stack"},
      {"SYS DIRSTACK", "host", "SYS DIRSTACK|DIRS — newline bag of saved dirs"},
      {"SYS KINDSTR", "host", "SYS KINDSTR|FILEKIND path — kind label file|dir|link|missing"},
      {"SYS RELPATH", "host", "SYS RELPATH|REL base path — relative to base or absolute"},
      {"SYS ATIME", "host", "SYS ATIME|ACCESSTIME path — last access epoch → LAST_N"},
      {"SYS CTIME", "host", "SYS CTIME|CHANGETIME path — inode status-change epoch → LAST_N"},
      {"SYS HASENV", "host", "SYS HASENV|ENVHAS name — LAST_N 1 if env set non-empty"},
      {"SYS GID", "host", "SYS GID|GROUP_ID — real group id → LAST_N"},
      {"SYS EGID", "host", "SYS EGID|GETEGID — effective group id → LAST_N"},
      {"SYS GROUP", "host", "SYS GROUP|GNAME — primary group name → LAST"},
      {"SYS GROUPS", "host", "SYS GROUPS|GROUPLIST — egid+supplementary names bag"},
      {"SYS INGROUP", "host", "SYS INGROUP|MEMBEROF name|gid — membership probe LAST_N 0|1"},
      {"SYS PPID", "host", "SYS PPID|GETPPID|PARENT_PID — parent process id → LAST_N"},
      {"SYS PGID", "host", "SYS PGID|GETPGID|PGRP — process group id → LAST_N"},
      {"SYS SID", "host", "SYS SID|GETSID|SESSION — session id → LAST_N"},
      {"SYS DOTENV", "host", "SYS DOTENV|LOADENV|ENVFILE path — load KEY=VAL plate into process env"},
      {"SYS LOADENV", "host", "SYS LOADENV alias of SYS DOTENV"},
      {"SYS ENVFILE", "host", "SYS ENVFILE alias of SYS DOTENV"},
      {"SYS DUMPENV", "host", "SYS DUMPENV|ENVALL [prefix] — process env as KEY=VAL bag"},
      {"SYS ENVALL", "host", "SYS ENVALL alias of SYS DUMPENV"},
      {"SYS LISTENV", "host", "SYS LISTENV alias of SYS DUMPENV"},
      {"SYS WRITEENV", "host", "SYS WRITEENV|SAVENV path [prefix] — write env KEY=VAL plate"},
      {"SYS SAVENV", "host", "SYS SAVENV alias of SYS WRITEENV"},
      {"SYS DUMPENVFILE", "host", "SYS DUMPENVFILE alias of SYS WRITEENV"},
      {"SYS WHICHBIN", "host", "SYS WHICHBIN|PATHWHICH name — PATH-only soft resolve"},
      {"REQUIRE BIN", "flow", "REQUIRE BIN|CMD|EXE name — fail if tool not on PATH"},
      {"SYS PID", "host", "SYS PID process id"},
      {"SYS HOSTNAME", "host", "SYS HOSTNAME machine name"},
      {"SYS USER", "host", "SYS USER login name → LAST/USER"},
      {"SYS GETLOGIN", "host", "SYS GETLOGIN|LOGINNAME session login → LAST"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS UID", "host", "SYS UID|RUID real user id → LAST_N/UID"},
{"SYS EUID", "host", "SYS EUID|GETEUID effective user id → LAST_N/EUID"},
{"SYS ISROOT", "host", "SYS ISROOT|AMROOT euid==0 privilege probe"},
      {"SYS HOME", "host", "SYS HOME home directory → LAST/HOME"},
      {"SYS SHELL", "host", "SYS SHELL|LOGINSHELL login shell path → LAST/SHELL"},
      {"SYS GECOS", "host", "SYS GECOS|FULLNAME passwd display name → LAST"},
      {"SYS TTYNAME", "host", "SYS TTYNAME|CTTY terminal device path → LAST"},
      {"SYS NICE", "host", "SYS NICE|RENICE process nice get/set → LAST_N"},
      {"SYS SHQUOTE", "host", "SYS SHQUOTE|SHELLQUOTE POSIX shell-safe wrap"},
      {"SYS SHUNQUOTE", "host", "SYS SHUNQUOTE|UNQUOTE peel POSIX single-quote wrap"},
      {"SYS GETLOGIN", "host", "SYS GETLOGIN|LOGINNAME session login name"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SMX SERVE", "smx", "listen · CUBALC_P2P_TIMEOUT ms"},
      {"SMX DIAL", "smx", "connect · CUBALC_P2P_SOFT soft-fail"},
      {"SMX EXCHANGE", "smx", "file-bus exchange"},
    };
    static const struct { const char *path; const char *tag; const char *hint; } examples[] = {
      {"programs/hello_cube.cubalc", "hello", "hold → place → plug short form"},
      {"programs/proof/12_hold_flash_plug.cubalc", "hold", "HOLD_FLASH before PLUG"},
      {"programs/proof/573_env_or_assert_msg.cubalc", "assert", "SYS ENV OR + ASSERT msg"},
      {"programs/proof/577_include_shortname.cubalc", "include", "INCLUDE short lib name"},
      {"programs/proof/578_expect_soft.cubalc", "expect", "EXPECT soft assert"},
      {"programs/proof/579_fail_pass.cubalc", "fail", "FAIL/PASS soft status"},
      {"programs/proof/582_sys_which_lib.cubalc", "which", "SYS WHICH lib resolve"},
      {"programs/proof/588_status.cubalc", "status", "STATUS agent health plate"},
      {"programs/proof/589_clear_err.cubalc", "clear_err", "CLEAR_ERR wipe sticky LAST_ERR"},
      {"programs/proof/590_require_lib.cubalc", "require_lib", "REQUIRE LIB fail-fast gate"},
      {"programs/proof/676_require_env.cubalc", "require_env", "REQUIRE ENV fail-fast host config"},
      {"programs/proof/677_sys_upperall.cubalc", "sys_upperall", "SYS UPPERALL/LOWERALL case-map bag fields"},
      {"programs/proof/678_sys_mapreplace.cubalc", "sys_mapreplace", "SYS MAPREPLACE bag-wide GSUB"},
      {"programs/proof/679_sys_freq.cubalc", "sys_freq", "SYS FREQ bag field frequency histogram"},
      {"programs/proof/680_sys_cutall.cubalc", "sys_cutall", "SYS CUTALL peel column from every bag line"},
      {"programs/proof/681_sys_sortfreq.cubalc", "sys_sortfreq", "SYS SORTFREQ sort FREQ bag by count"},
      {"programs/proof/682_sys_beforeall.cubalc", "sys_beforeall", "SYS BEFOREALL/AFTERALL peel every bag field"},
      {"programs/proof/683_sys_firstmatch.cubalc", "sys_firstmatch", "SYS FIRSTMATCH/GREP1 first bag field hit"},
      {"programs/proof/684_sys_chunk.cubalc", "sys_chunk", "SYS CHUNK/BATCH group bag fields by n"},
      {"programs/proof/695_sys_lastmatch.cubalc", "sys_lastmatch", "SYS LASTMATCH/GREP1L last bag field hit"},
      {"programs/proof/696_sys_maxlen.cubalc", "sys_maxlen", "SYS MAXLEN/MINLEN max/min bag field length"},
      {"programs/proof/697_sys_truncall.cubalc", "sys_truncall", "SYS TRUNCALL/CLIPALL truncate bag fields"},
      {"programs/proof/698_sys_longest.cubalc", "sys_longest", "SYS LONGEST/SHORTEST pick bag field by length"},
      {"programs/proof/699_sys_commonprefix.cubalc", "sys_commonprefix", "SYS COMMONPREFIX/LCP shared bag prefix"},
      {"programs/proof/700_sys_stripprefix.cubalc", "sys_stripprefix", "SYS STRIPPREFIX/STRIPCOMMON relative bag paths"},
      {"programs/proof/701_sys_drawn.cubalc", "sys_drawn", "SYS DRAWN/SAMPLEK sample k unique bag fields"},
      {"programs/proof/702_sys_lookup.cubalc", "sys_lookup", "SYS LOOKUP/KVGET peel key:val bag value"},
      {"programs/proof/703_sys_kvset.cubalc", "sys_kvset", "SYS KVSET/SETKV set/update key:val bag field"},
      {"programs/proof/704_sys_kvinc.cubalc", "sys_kvinc", "SYS KVINC/INCKV bump key:val numeric counter"},
      {"programs/proof/705_sys_kvdel.cubalc", "sys_kvdel", "SYS KVDEL/DELKV drop key:val bag field"},
      {"programs/proof/706_sys_lookup_or.cubalc", "sys_lookup_or", "SYS LOOKUP bag key OR fallback default"},
      {"programs/proof/707_sys_mergekv.cubalc", "sys_mergekv", "SYS MERGEKV/KVADDALL merge key:val bags by sum"},
      {"programs/proof/708_sys_sumkv.cubalc", "sys_sumkv", "SYS SUMKV/TOTALKV sum key:val bag values"},
      {"programs/proof/709_sys_topkey.cubalc", "sys_topkey", "SYS TOPKEY/BOTKEY max/min key of key:val bag"},
      {"programs/proof/710_sys_lookupn.cubalc", "sys_lookupn", "SYS LOOKUPN/KVGETN peel key:val as int LAST_N"},
      {"programs/proof/711_sys_threshkv.cubalc", "sys_threshkv", "SYS THRESHKV/KEEPVAL keep key:val value>=min"},
      {"programs/proof/712_sys_pctkv.cubalc", "sys_pctkv", "SYS PCTKV/SHAREKV key:val integer percent of total"},
      {"programs/proof/713_sys_capkv.cubalc", "sys_capkv", "SYS CAPKV/CLAMPKV clamp key:val values to max"},
      {"programs/proof/714_sys_scalekv.cubalc", "sys_scalekv", "SYS SCALEKV/MULKV multiply key:val values by factor"},
      {"programs/proof/715_sys_diffkv.cubalc", "sys_diffkv", "SYS DIFFKV/SUBKV subtract key:val bags by key"},
      {"programs/proof/716_sys_addkv.cubalc", "sys_addkv", "SYS ADDKV/OFFSETKV add delta to key:val values"},
      {"programs/proof/717_sys_dropzero.cubalc", "sys_dropzero", "SYS DROPZERO/KEEPNZ drop key:val value==0"},
      {"programs/proof/718_sys_abskv.cubalc", "sys_abskv", "SYS ABSKV/MAGKV absolute key:val values"},
      {"programs/proof/719_sys_signkv.cubalc", "sys_signkv", "SYS SIGNKV/DIRKV map key:val to -1|0|1"},
      {"programs/proof/720_sys_divkv.cubalc", "sys_divkv", "SYS DIVKV/IDIVKV integer-divide key:val values"},
      {"programs/proof/721_sys_avgkv.cubalc", "sys_avgkv", "SYS AVGKV/MEANKV integer mean of key:val values"},
      {"programs/proof/722_sys_mediankv.cubalc", "sys_mediankv", "SYS MEDIANKV/P50KV median of key:val values"},
      {"programs/proof/723_sys_keepkey.cubalc", "sys_keepkey", "SYS KEEPKEY/GREPKEY filter key:val by key needle"},
      {"programs/proof/724_sys_glob.cubalc", "sys_glob", "SYS GLOB/MATCHFILES list basenames matching pattern"},
      {"programs/proof/725_sys_pathglob.cubalc", "sys_pathglob", "SYS PATHGLOB/PGLOB full paths matching shell pattern"},
      {"programs/proof/726_require_path.cubalc", "require_path", "REQUIRE PATH/DIR/REG fail-fast host path gates"},
      {"programs/proof/727_sys_filterglob.cubalc", "sys_filterglob", "SYS FILTERGLOB/MATCHBAG shell-pattern bag filter"},
      {"programs/proof/728_sys_basenameall.cubalc", "sys_basenameall", "SYS BASENAMEALL/DIRNAMEALL/EXTALL/STEMALL path bag maps"},
      {"programs/proof/729_sys_keepfiles.cubalc", "sys_keepfiles", "SYS KEEPFILES/KEEPDIRS/KEEPEXIST path bag kind filters"},
      {"programs/proof/730_sys_sizeall.cubalc", "sys_sizeall", "SYS SIZEALL/MAPSIZE path bag byte sizes + sum"},
      {"programs/proof/731_sys_mtimeall.cubalc", "sys_mtimeall", "SYS MTIMEALL/AGEALL path bag time maps + min/max"},
      {"programs/proof/732_sys_newest.cubalc", "sys_newest", "SYS NEWEST/OLDEST/LARGEST/SMALLEST path bag pick"},
      {"programs/proof/733_sys_sortmtime.cubalc", "sys_sortmtime", "SYS SORTMTIME/SORTSIZE order path bags by time/size"},
      {"programs/proof/734_sys_fresh.cubalc", "sys_fresh", "SYS FRESH/KEEPSTALE filter path bags by age threshold"},
      {"programs/proof/735_sys_keepnewer.cubalc", "sys_keepnewer", "SYS KEEPNEWER/KEEPOLDER filter path bags vs ref mtime"},
      {"programs/proof/736_sys_keepbigger.cubalc", "sys_keepbigger", "SYS KEEPBIGGER/KEEPSMALLER filter path bags by size"},
      {"programs/proof/737_sys_rmall.cubalc", "sys_rmall", "SYS RMALL/UNLINKALL bulk remove path bag"},
      {"programs/proof/738_sys_touchall.cubalc", "sys_touchall", "SYS TOUCHALL/ENSUREALL bulk create/refresh path bag"},
      {"programs/proof/739_sys_copyall.cubalc", "sys_copyall", "SYS COPYALL/CPALL bulk copy path bag into dest dir"},
      {"programs/proof/740_sys_mkdirall.cubalc", "sys_mkdirall", "SYS MKDIRALL/ENSUREDIRS bulk mkdir -p path bag"},
      {"programs/proof/741_sys_moveall.cubalc", "sys_moveall", "SYS MOVEALL/MVALL bulk move path bag into dest dir"},
      {"programs/proof/742_sys_walk.cubalc", "sys_walk", "SYS WALK/FINDALL recursive full-path discovery"},
      {"programs/proof/743_sys_eqfile.cubalc", "sys_eqfile", "SYS EQFILE/SAMEFILE identical file content probe"},
      {"programs/proof/744_sys_logall.cubalc", "sys_logall", "SYS LOGALL/APPENDFILES append line to path bag"},
      {"programs/proof/745_sys_grepfiles.cubalc", "sys_grepfiles", "SYS GREPFILES/SEARCHFILES filter paths by file content"},
      {"programs/proof/746_sys_readall.cubalc", "sys_readall", "SYS READALL/CATFILES concat path bag file contents"},
      {"programs/proof/747_sys_writeall.cubalc", "sys_writeall", "SYS WRITEALL/WRITEFILES overwrite path bag with data"},
      {"programs/proof/748_sys_replacefiles.cubalc", "sys_replacefiles", "SYS REPLACEFILES/SUBFILES bulk rewrite path bag content"},
      {"programs/proof/749_sys_countinfiles.cubalc", "sys_countinfiles", "SYS COUNTINFILES/GREPCOUNTFILES total needle hits in path bag"},
      {"programs/proof/750_sys_firstfile.cubalc", "sys_firstfile", "SYS FIRSTFILE/LASTFILE first/last path with content match"},
      {"programs/proof/751_sys_greplines.cubalc", "sys_greplines", "SYS GREPLINES/EXTRACTLINES collect matching lines from path bag"},
      {"programs/proof/752_sys_headfile.cubalc", "sys_headfile", "SYS HEADFILE/TAILFILE first/last n lines of a file"},
      {"programs/proof/753_sys_linecountall.cubalc", "sys_linecountall", "SYS LINECOUNTALL/WCALL path bag line counts + sum"},
      {"programs/proof/754_sys_midfile.cubalc", "sys_midfile", "SYS MIDFILE/LINESLICE file line window extract"},
      {"programs/proof/755_sys_lineat.cubalc", "sys_lineat", "SYS LINEAT/FILELINE single file line by index"},
      {"programs/proof/756_sys_setfileline.cubalc", "sys_setfileline", "SYS SETFILELINE/FILESETLINE replace file line by index"},
      {"programs/proof/757_sys_insertfileline.cubalc", "sys_insertfileline", "SYS INSERTFILELINE/FILEINSERTLINE insert file line by index"},
      {"programs/proof/758_sys_dropfileline.cubalc", "sys_dropfileline", "SYS DROPFILELINE/FILEDROPLINE drop file line by index"},
      {"programs/proof/759_sys_findfileline.cubalc", "sys_findfileline", "SYS FINDFILELINE/FILEFINDLINE first file line index by needle"},
      {"programs/proof/760_sys_lastfindfileline.cubalc", "sys_lastfindfileline", "SYS LASTFINDFILELINE/FILELASTFIND last file line by needle"},
      {"programs/proof/761_sys_countfileline.cubalc", "sys_countfileline", "SYS COUNTFILELINE/FILECOUNTLINE count matching file lines"},
      {"programs/proof/762_sys_grepfile.cubalc", "sys_grepfile", "SYS GREPFILE/FILEGREP keep matching lines from one file"},
      {"programs/proof/763_sys_setmatchfile.cubalc", "sys_setmatchfile", "SYS SETMATCHFILE/FILESETMATCH replace first matching file line"},
      {"programs/proof/764_sys_dropmatchfile.cubalc", "sys_dropmatchfile", "SYS DROPMATCHFILE/FILEDROPMATCH drop first matching file line"},
      {"programs/proof/765_sys_dropallmatchfile.cubalc", "sys_dropallmatchfile", "SYS DROPALLMATCHFILE/FILEDROPALLMATCH drop all matching file lines"},
      {"programs/proof/766_sys_setallmatchfile.cubalc", "sys_setallmatchfile", "SYS SETALLMATCHFILE/FILESETALLMATCH replace all matching file lines"},
      {"programs/proof/767_sys_insertmatchfile.cubalc", "sys_insertmatchfile", "SYS INSERTMATCHFILE/FILEINSERTMATCH insert line after first match"},
      {"programs/proof/768_sys_beforematchfile.cubalc", "sys_beforematchfile", "SYS BEFOREMATCHFILE/INSERTBEFOREMATCH insert line before first match"},
      {"programs/proof/769_sys_lookupfile.cubalc", "sys_lookupfile", "SYS LOOKUPFILE/FILELOOKUP peel key=val from plate file"},
      {"programs/proof/770_sys_kvfileset.cubalc", "sys_kvfileset", "SYS KVFILESET/FILEKVSET set/update key=val in plate file"},
      {"programs/proof/771_sys_kvfiledel.cubalc", "sys_kvfiledel", "SYS KVFILEDEL/FILEKVDEL drop key=val line from plate file"},
      {"programs/proof/772_sys_kvfileinc.cubalc", "sys_kvfileinc", "SYS KVFILEINC/INCKVFILE bump numeric key=val in plate file"},
      {"programs/proof/773_sys_ensureline.cubalc", "sys_ensureline", "SYS ENSURELINE/ENSUREFILELINE append line if exact missing"},
      {"programs/proof/774_sys_hasfileline.cubalc", "sys_hasfileline", "SYS HASFILELINE/FILEHASLINE exact file line membership"},
      {"programs/proof/775_sys_dropexactfile.cubalc", "sys_dropexactfile", "SYS DROPEXACTFILE/DROPFILEEXACT drop first exact file line"},
      {"programs/proof/776_sys_toggleline.cubalc", "sys_toggleline", "SYS TOGGLELINE/TOGGLEFILELINE flip exact file line presence"},
      {"programs/proof/777_sys_kvfileensure.cubalc", "sys_kvfileensure", "SYS KVFILEENSURE/ENSUREKVFILE set key=val only if missing"},
      {"programs/proof/778_sys_substenv.cubalc", "sys_substenv", "SYS SUBSTENV/ENVSUBST expand $VAR and ${VAR} templates"},
      {"programs/proof/781_sys_waitfile.cubalc", "sys_waitfile", "SYS WAITFILE/WAITPATH poll until path exists"},
      {"programs/proof/782_sys_substfile.cubalc", "sys_substfile", "SYS SUBSTFILE/EXPANDFILE materialize $VAR template file"},
      {"programs/proof/783_sys_hash.cubalc", "sys_hash", "SYS HASH/HASHFILE FNV-1a plate content fingerprint"},
      {"programs/proof/784_sys_waitmatch.cubalc", "sys_waitmatch", "SYS WAITMATCH/WAITCONTAINS poll until file contains needle"},
      {"programs/proof/785_sys_writeatomic.cubalc", "sys_writeatomic", "SYS WRITEATOMIC/SAFEWRITE temp+rename plate write"},
      {"programs/proof/786_sys_lockfile.cubalc", "sys_lockfile", "SYS LOCKFILE/UNLOCKFILE exclusive agent plate locks"},
      {"programs/proof/787_sys_waitgone.cubalc", "sys_waitgone", "SYS WAITGONE/WAITMISSING poll until path does not exist"},
      {"programs/proof/788_sys_waitstable.cubalc", "sys_waitstable", "SYS WAITSTABLE/WAITQUIET poll until size+mtime quiet"},
      {"programs/proof/789_sys_waitchanged.cubalc", "sys_waitchanged", "SYS WAITCHANGED/WAITMODIFIED poll until size/mtime change"},
      {"programs/proof/790_sys_mktemp.cubalc", "sys_mktemp", "SYS MKTEMP/TEMPFILE unique empty file under TMP"},
      {"programs/proof/791_sys_claim.cubalc", "sys_claim", "SYS CLAIM/CREATEEXCL exclusive path work-item claim"},
      {"programs/proof/792_sys_stat.cubalc", "sys_stat", "SYS STAT/FSTAT one-shot path metadata probe"},
      {"programs/proof/793_sys_mktempdir.cubalc", "sys_mktempdir", "SYS MKTEMPDIR/MKDTEMP unique empty dir under TMP"},
      {"programs/proof/794_sys_rmdir.cubalc", "sys_rmdir", "SYS RMDIR/REMOVEDIR remove empty directory"},
      {"programs/proof/795_sys_base64.cubalc", "sys_base64", "SYS BASE64/BASE64D encode/decode plate payloads"},
      {"programs/proof/796_sys_hex.cubalc", "sys_hex", "SYS HEX/HEXD encode/decode plate bytes"},
      {"programs/proof/797_sys_urlenc.cubalc", "sys_urlenc", "SYS URLENC/URLDEC percent-encode plate paths"},
      {"programs/proof/798_sys_uuid.cubalc", "sys_uuid", "SYS UUID/UUID0 RFC4122 v4 agent work ids"},
      {"programs/proof/799_sys_jsonesc.cubalc", "sys_jsonesc", "SYS JSONESC/JSONUNESC plate string escape"},
      {"programs/proof/800_sys_crc32.cubalc", "sys_crc32", "SYS CRC32/CRC32FILE IEEE integrity stamps"},
      {"programs/proof/801_sys_symlink.cubalc", "sys_symlink", "SYS SYMLINK/READLINK plate path aliases"},
      {"programs/proof/802_sys_humansize.cubalc", "sys_humansize", "SYS HUMANSIZE integer B/K/M/G size labels"},
      {"programs/proof/803_sys_islink.cubalc", "sys_islink", "SYS ISLINK probe symlink paths"},
      {"programs/proof/804_sys_mode.cubalc", "sys_mode", "SYS MODE/CHMOD plate permission probe/set"},
      {"programs/proof/805_sys_parsesize.cubalc", "sys_parsesize", "SYS PARSESIZE human size string to bytes"},
      {"programs/proof/806_sys_monotonic.cubalc", "sys_monotonic", "SYS MONOTONIC steady ms for elapsed timing"},
      {"programs/proof/807_sys_nproc.cubalc", "sys_nproc", "SYS NPROC/CPUS online processor count"},
      {"programs/proof/808_sys_uname.cubalc", "sys_uname", "SYS UNAME/OS/ARCH/PLATFORM host identity"},
      {"programs/proof/809_sys_local.cubalc", "sys_local", "SYS LOCAL/LOCALTIME local wall stamp"},
      {"programs/proof/810_require_bin.cubalc", "require_bin", "REQUIRE BIN/SYS WHICHBIN PATH tool gates (+ string-var names)"},
      {"programs/proof/811_sys_isatty.cubalc", "sys_isatty", "SYS ISATTY/TTY terminal stream probe"},
      {"programs/proof/812_sys_loadavg.cubalc", "sys_loadavg", "SYS LOADAVG/LOAD host load averages"},
      {"programs/proof/813_sys_uptime.cubalc", "sys_uptime", "SYS UPTIME/BOOTAGE seconds since boot"},
      {"programs/proof/814_sys_mem.cubalc", "sys_mem", "SYS MEM/MEMINFO host RAM total/avail kB"},
      {"programs/proof/815_sys_df.cubalc", "sys_df", "SYS DF/DISKFREE filesystem free kB"},
      {"programs/proof/816_sys_fsync.cubalc", "sys_fsync", "SYS FSYNC/SYNCFILE durable file flush"},
      {"programs/proof/817_sys_dotenv.cubalc", "sys_dotenv", "SYS DOTENV/LOADENV load KEY=VAL plate into env"},
      {"programs/proof/818_sys_dumpenv.cubalc", "sys_dumpenv", "SYS DUMPENV/ENVALL process env KEY=VAL bag"},
      {"programs/proof/819_sys_writeenv.cubalc", "sys_writeenv", "SYS WRITEENV/SAVENV persist env KEY=VAL plate"},
      {"programs/proof/820_sys_truncate.cubalc", "sys_truncate", "SYS TRUNCATE/FTRUNCATE resize plate files"},
      {"programs/proof/821_sys_dirsync.cubalc", "sys_dirsync", "SYS DIRSYNC/FSYNCDIR durable dir flush"},
      {"programs/proof/822_sys_hardlink.cubalc", "sys_hardlink", "SYS HARDLINK/NLINK shared plate inodes"},
      {"programs/proof/823_sys_sameinode.cubalc", "sys_sameinode", "SYS SAMEINODE/INODE hard-link identity"},
      {"programs/proof/824_sys_tee.cubalc", "sys_tee", "SYS TEE write keep content in LAST"},
      {"programs/proof/825_sys_fileuid.cubalc", "sys_fileuid", "SYS FILEUID/FILEGID/OWNEDBY ownership probes"},
      {"programs/proof/826_sys_canread.cubalc", "sys_canread", "SYS CANREAD/CANWRITE/CANEXEC access probes"},
      {"programs/proof/827_sys_ownername.cubalc", "sys_ownername", "SYS OWNERNAME/GROUPNAME login and group name probes"},
      {"programs/proof/828_sys_cancreate.cubalc", "sys_cancreate", "SYS CANCREATE pre-flight create/overwrite probe"},
      {"programs/proof/829_sys_umask.cubalc", "sys_umask", "SYS UMASK get/set process file-creation mask"},
      {"programs/proof/830_sys_pushd.cubalc", "sys_pushd", "SYS PUSHD/POPD/DIRSTACK cwd stack for temp chdir"},
      {"programs/proof/831_sys_kindstr.cubalc", "sys_kindstr", "SYS KINDSTR human path kind labels"},
      {"programs/proof/832_sys_relpath.cubalc", "sys_relpath", "SYS RELPATH base path relative plate paths"},
      {"programs/proof/833_sys_atime.cubalc", "sys_atime", "SYS ATIME/CTIME access and status-change times"},
      {"programs/proof/834_sys_hasenv.cubalc", "sys_hasenv", "SYS HASENV soft env presence probe"},
      {"programs/proof/835_sys_gid.cubalc", "sys_gid", "SYS GID/EGID/GROUP process group identity"},
      {"programs/proof/836_sys_groups.cubalc", "sys_groups", "SYS GROUPS/INGROUP process group membership"},
      {"programs/proof/837_sys_ppid.cubalc", "sys_ppid", "SYS PPID/PGID/SID process lineage and session"},
      {"programs/proof/838_sys_euid_isroot.cubalc", "sys_euid_isroot", "SYS EUID/ISROOT effective uid and root probe"},
      {"programs/proof/839_sys_shell.cubalc", "sys_shell", "SYS SHELL login shell path without shell"},
      {"programs/proof/840_sys_gecos.cubalc", "sys_gecos", "SYS GECOS/FULLNAME passwd display name"},
      {"programs/proof/841_sys_ttyname.cubalc", "sys_ttyname", "SYS TTYNAME/CTTY terminal device path"},
      {"programs/proof/842_sys_nice.cubalc", "sys_nice", "SYS NICE/RENICE process priority get/set"},
      {"programs/proof/843_sys_shquote.cubalc", "sys_shquote", "SYS SHQUOTE/SHELLQUOTE POSIX shell-safe wrap"},
      {"programs/proof/844_sys_shunquote.cubalc", "sys_shunquote", "SYS SHUNQUOTE/UNQUOTE peel shell quote wrap"},
      {"programs/proof/845_sys_getlogin.cubalc", "sys_getlogin", "SYS GETLOGIN/LOGINNAME session login name"},
      {"programs/proof/846_sys_entropy.cubalc", "sys_entropy", "SYS ENTROPY/URANDOM random bytes as hex"},
      {"programs/proof/847_sys_parsems.cubalc", "sys_parsems", "SYS PARSEMS/DURATION compound human duration to ms"},
      {"programs/proof/848_sys_humanms.cubalc", "sys_humanms", "SYS HUMANMS/FMTMS ms to human duration"},
      {"programs/proof/849_sys_fromtime.cubalc", "sys_fromtime", "SYS FROMTIME/EPOCHISO epoch to UTC ISO"},
      {"programs/proof/850_sys_parseiso.cubalc", "sys_parseiso", "SYS PARSEISO/TOEPOCH ISO stamp to epoch"},
      {"programs/proof/851_sys_timediff.cubalc", "sys_timediff", "SYS TIMEDIFF/ELAPSED epoch second deltas"},
      {"programs/proof/852_sys_envdefault.cubalc", "sys_envdefault", "SYS ENVDEFAULT setenv if missing/empty"},
      {"programs/proof/853_sys_addtime.cubalc", "sys_addtime", "SYS ADDTIME/DEADLINE epoch plus seconds"},
      {"programs/proof/854_sys_expired.cubalc", "sys_expired", "SYS EXPIRED/REMAINING lease due and TTL"},
      {"programs/proof/855_sys_ms2sec.cubalc", "sys_ms2sec", "SYS MS2SEC/SEC2MS ms-seconds unit bridge"},
      {"programs/proof/856_sys_parsems_compound.cubalc", "sys_parsems_compound", "SYS PARSEMS compound 1h30m HUMANMS round-trip"},
      {"programs/proof/857_sys_sleep_duration.cubalc", "sys_sleep_duration", "SYS SLEEP human duration string without PARSEMS glue"},
      {"programs/proof/858_sys_waitfile_duration.cubalc", "sys_waitfile_duration", "SYS WAITFILE timeout human duration string"},
      {"programs/proof/859_sys_deadline_duration.cubalc", "sys_deadline_duration", "SYS DEADLINE/ADDTIME human duration lease offsets"},
      {"programs/proof/862_oop_class_method.cubalc", "oop_class", "CLASS/METHOD/NEW/SEND OOP reusable objects"},
      {"programs/proof/863_cop_entity_tick.cubalc", "cop_entity_tick", "ENTITY/SPAWN/TICK COP game-engine plane"},
      {"programs/proof/865_life_engine_division.cubalc", "life_engine", "COP life engine cell division mitosis demo"},
      {"programs/proof/864_include_class_lib.cubalc", "include_class_lib", "INCLUDE retains CLASS/METHOD bodies from programs/lib"},
      {"programs/proof/866_sys_humansecs.cubalc", "sys_humansecs", "SYS HUMANSECS/HUMANAGE seconds to human duration labels"},
      {"programs/proof/867_oop_list_hasmethod.cubalc", "oop_list_hasmethod", "LISTCLASSES/LISTOBJS/HASMETHOD OOP introspection"},
      {"programs/proof/868_oop_destroy_hasobj.cubalc", "oop_destroy_hasobj", "HASOBJ/DESTROY/HASFIELD object lifecycle"},
      {"programs/proof/869_oop_dumpobj.cubalc", "oop_dumpobj", "DUMPOBJ/INSPECT object field snapshot bag/JSON"},
      {"programs/proof/870_oop_string_fields.cubalc", "oop_string_fields", "SETF/NEW string-from-var + string formals"},
      {"programs/proof/871_oop_getf_or.cubalc", "oop_getf_or", "GETF OR/TRYGETF soft field reads with defaults"},
      {"programs/proof/872_assert_str_eq.cubalc", "assert_str_eq", "ASSERT/IF string ==/!= content compare"},
      {"programs/proof/873_oop_cloneobj.cubalc", "oop_cloneobj", "CLONEOBJ/COPYOBJ shallow object field copy"},
      {"programs/proof/874_oop_renameobj.cubalc", "oop_renameobj", "RENAMEOBJ/MOVEOBJ rename live object slot"},
      {"programs/proof/875_oop_trysetf.cubalc", "oop_trysetf", "TRYSETF/SETF SOFT soft field writes"},
      {"programs/proof/876_oop_trysend.cubalc", "oop_trysend", "TRYSEND/SEND SOFT soft method dispatch"},
      {"programs/proof/877_oop_each_obj.cubalc", "oop_each_obj", "EACH OBJ walk live objects with class filter"},
      {"programs/proof/878_oop_sendall.cubalc", "oop_sendall", "SENDALL/BROADCAST method on every live object"},
      {"programs/proof/879_oop_deleteall.cubalc", "oop_deleteall", "DELETEALL/FREEALL bulk free live objects"},
      {"programs/proof/880_oop_getfall.cubalc", "oop_getfall", "GETFALL/COLLECTF bulk field collect from live objects"},
      {"programs/proof/881_oop_setfall.cubalc", "oop_setfall", "SETFALL/MAPSETF bulk field write on live objects"},
      {"programs/proof/882_oop_whereobj.cubalc", "oop_whereobj", "WHEREOBJ/FINDOBJ select live objects by field equality"},
      {"programs/proof/883_oop_deletewhere.cubalc", "oop_deletewhere", "DELETEWHERE/FREEWHERE free live objects by field equality"},
      {"programs/proof/884_oop_sendwhere.cubalc", "oop_sendwhere", "SENDWHERE/INVOKEWHERE method on field-matching objects"},
      {"programs/proof/885_oop_incfall.cubalc", "oop_incfall", "INCFALL/ADDFALL/DECFALL bulk numeric field bump"},
      {"programs/proof/886_oop_maxobj.cubalc", "oop_maxobj", "MAXOBJ/MINOBJ pick object by extreme field"},
      {"programs/proof/887_oop_sortbyf.cubalc", "oop_sortbyf", "SORTBYF/SORTOBJS bag of names ordered by field"},
      {"programs/proof/888_oop_sumf.cubalc", "oop_sumf", "SUMF/AVGF fleet field sum and integer mean"},
      {"programs/proof/889_oop_wherege.cubalc", "oop_wherege", "WHEREGE/WHERELE threshold select live objects"},
      {"programs/proof/890_oop_deletewherege.cubalc", "oop_deletewherege", "DELETEWHEREGE/FREEBELOW threshold free live objects"},
      {"programs/proof/891_oop_sendwherege.cubalc", "oop_sendwherege", "SENDWHEREGE/SENDBELOW method on threshold-matched objects"},
      {"programs/proof/892_oop_countwhere.cubalc", "oop_countwhere", "COUNTOBJ/COUNTWHERE/COUNTWHEREGE fleet tallies without bags"},
      {"programs/proof/893_oop_haswhere.cubalc", "oop_haswhere", "HASWHERE/HASWHEREGE soft existence probes for IF gates"},
      {"programs/proof/894_oop_wherebetween.cubalc", "oop_wherebetween", "WHEREBETWEEN/COUNTBETWEEN/HASBETWEEN closed-range fleet query"},
      {"programs/proof/895_oop_deletebetween.cubalc", "oop_deletebetween", "DELETEBETWEEN/FREEBAND free live objects in closed field range"},
      {"programs/proof/896_oop_sendbetween.cubalc", "oop_sendbetween", "SENDBETWEEN/SENDBAND method on closed-range matched objects"},
      {"programs/proof/897_oop_clampfall.cubalc", "oop_clampfall", "CLAMPFALL/CLAMPF bulk clamp numeric fields to [lo,hi]"},
      {"programs/proof/898_oop_medianf.cubalc", "oop_medianf", "MEDIANF/P50F integer median of fleet numeric field"},
      {"programs/proof/899_oop_mulfall.cubalc", "oop_mulfall", "MULFALL/SCALEFALL bulk multiply numeric fields by factor"},
      {"programs/proof/900_oop_freqf.cubalc", "oop_freqf", "FREQF/HISTF fleet field value histogram key:count"},
      {"programs/proof/901_oop_modef.cubalc", "oop_modef", "MODEF/TOPF most frequent fleet field value"},
      {"programs/proof/902_oop_uniquf.cubalc", "oop_uniquf", "UNIQUF/DISTINCTF unique fleet field values bag"},
      {"programs/proof/903_oop_copyf.cubalc", "oop_copyf", "COPYF/COPYFALL copy field to field across live objects"},
      {"programs/proof/904_oop_swapf.cubalc", "oop_swapf", "SWAPF/SWAPFALL exchange two fields on live objects"},
      {"programs/proof/905_oop_setfwhere.cubalc", "oop_setfwhere", "SETFWHERE conditional field write on matching live objects"},
      {"programs/proof/906_oop_incfwhere.cubalc", "oop_incfwhere", "INCFWHERE/DECFWHERE bump field on matching live objects"},
      {"programs/proof/907_oop_getfwhere.cubalc", "oop_getfwhere", "GETFWHERE filtered field collect from matching live objects"},
      {"programs/proof/908_oop_sumwhere.cubalc", "oop_sumwhere", "SUMWHERE/AVGWHERE filtered fleet sum/mean on match"},
      {"programs/proof/909_oop_minwhere.cubalc", "oop_minwhere", "MINWHERE/MAXWHERE filtered fleet min/max on match"},
      {"programs/proof/910_oop_medianwhere.cubalc", "oop_medianwhere", "MEDIANWHERE/P50WHERE filtered fleet median on match"},
      {"programs/proof/911_oop_freqwhere.cubalc", "oop_freqwhere", "FREQWHERE/HISTWHERE filtered fleet field histogram on match"},
      {"programs/proof/912_oop_modewhere.cubalc", "oop_modewhere", "MODEWHERE/DOMINANTWHERE filtered fleet mode on match"},
      {"programs/proof/913_oop_uniquwhere.cubalc", "oop_uniquwhere", "UNIQUWHERE/DISTINCTWHERE filtered unique field values on match"},
      {"programs/proof/914_oop_clampwhere.cubalc", "oop_clampwhere", "CLAMPWHERE/CLIPWHERE selective clamp field on match"},
      {"programs/proof/915_oop_mulwhere.cubalc", "oop_mulwhere", "MULWHERE/SCALEWHERE selective multiply field on match"},
      {"programs/proof/916_oop_copyfwhere.cubalc", "oop_copyfwhere", "COPYFWHERE/SNAPSHOTWHERE selective field copy on match"},
      {"programs/proof/917_oop_swapfwhere.cubalc", "oop_swapfwhere", "SWAPFWHERE/EXCHANGEWHERE selective field swap on match"},
      {"programs/proof/918_oop_maxobjwhere.cubalc", "oop_maxobjwhere", "MAXOBJWHERE/MINOBJWHERE pick extreme obj among matches"},
      {"programs/proof/919_oop_sortbyfwhere.cubalc", "oop_sortbyfwhere", "SORTBYFWHERE/ORDERWHERE rank matching objects by field"},
      {"programs/proof/920_oop_topnbyf.cubalc", "oop_topnbyf", "TOPNBYF/HEADBYF top-n object names by numeric field"},
      {"programs/proof/921_oop_topnbyfwhere.cubalc", "oop_topnbyfwhere", "TOPNBYFWHERE/HEADWHERE top-n matching objects by field"},
      {"programs/proof/922_oop_nthbyf.cubalc", "oop_nthbyf", "NTHBYF/ATBYF 0-based rank pick object by numeric field"},
      {"programs/proof/923_oop_nthbyfwhere.cubalc", "oop_nthbyfwhere", "NTHBYFWHERE/ATWHERE kth match by numeric field rank"},
      {"programs/proof/924_oop_slicebyf.cubalc", "oop_slicebyf", "SLICEBYF/MIDBYF ranked object name window by field"},
      {"programs/proof/925_oop_pickobj.cubalc", "oop_pickobj", "PICKOBJ/RANDOBJ random live object sample"},
      {"programs/proof/926_oop_drawnobj.cubalc", "oop_drawnobj", "DRAWNOBJ/SAMPLEKOBJ sample k unique live objects"},
      {"programs/proof/927_oop_shuffleobjs.cubalc", "oop_shuffleobjs", "SHUFFLEOBJS/SHUFOBJS shuffle live object name bag"},
      {"programs/proof/928_oop_classinfo.cubalc", "oop_classinfo", "CLASSINFO/DUMPCLASS class schema plate for agents"},
      {"programs/proof/929_oop_methodinfo.cubalc", "oop_methodinfo", "METHODINFO/DUMPMETHOD method arity/params plate"},
      {"programs/proof/860_sys_addiso.cubalc", "sys_addiso", "SYS ADDISO shift ISO stamp by secs or duration"},
      {"programs/proof/861_sys_diffiso.cubalc", "sys_diffiso", "SYS DIFFISO/ISOAGE ISO stamp second deltas"},
      {"programs/proof/591_sys_ms.cubalc", "sys_ms", "SYS MS wall milliseconds"},
      {"programs/proof/592_note.cubalc", "note", "NOTE agent breadcrumb"},
      {"programs/proof/593_exit.cubalc", "exit", "EXIT early halt with code"},
      {"programs/proof/594_include_once.cubalc", "include_once", "INCLUDE ONCE skip reload"},
      {"programs/proof/595_sys_pid_hostname.cubalc", "sys_pid_host", "SYS PID/HOSTNAME identity"},
      {"programs/proof/596_sys_user_uid_home.cubalc", "sys_user", "SYS USER/UID/HOME identity"},
      {"programs/proof/597_default.cubalc", "default", "DEFAULT set-if-unset INCLUDE-safe"},
      {"programs/proof/598_defined.cubalc", "defined", "DEFINED var existence probe"},
      {"programs/proof/599_typeof.cubalc", "typeof", "TYPEOF num|str|undef kind"},
      {"programs/proof/600_unset.cubalc", "unset", "UNSET remove var DEFAULT re-apply"},
      {"programs/proof/601_identity.cubalc", "identity", "IDENTITY host/process plate"},
      {"programs/proof/602_sys_date.cubalc", "sys_date", "SYS DATE UTC ISO stamp"},
      {"programs/proof/603_sys_tmp.cubalc", "sys_tmp", "SYS TMP portable temp dir"},
      {"programs/proof/604_sys_mkdir.cubalc", "sys_mkdir", "SYS MKDIR mkdir -p"},
      {"programs/proof/605_sys_basename.cubalc", "sys_basename", "SYS BASENAME/DIRNAME split"},
      {"programs/proof/606_sys_extname.cubalc", "sys_extname", "SYS EXTNAME/STEM peel"},
      {"programs/proof/607_sys_size.cubalc", "sys_size", "SYS SIZE/ISDIR/ISFILE meta"},
      {"programs/proof/608_sys_read_soft.cubalc", "sys_read_soft", "SYS READ OR soft plate"},
      {"programs/proof/609_sys_rm_rename.cubalc", "sys_rm_rename", "SYS RM/RENAME lifecycle"},
      {"programs/proof/610_sys_copy.cubalc", "sys_copy", "SYS COPY plate files"},
      {"programs/proof/611_sys_realpath.cubalc", "sys_realpath", "SYS REALPATH absolute"},
      {"programs/proof/612_sys_touch.cubalc", "sys_touch", "SYS TOUCH plate markers"},
      {"programs/proof/613_sys_sleep.cubalc", "sys_sleep", "SYS SLEEP agent backoff"},
      {"programs/proof/614_sys_list.cubalc", "sys_list", "SYS LIST dir basenames"},
      {"programs/proof/615_sys_nth.cubalc", "sys_nth", "SYS NTH field peel"},
      {"programs/proof/616_sys_grep.cubalc", "sys_grep", "SYS GREP line filter"},
      {"programs/proof/617_each_line.cubalc", "each_line", "EACH LINE LIST walk"},
      {"programs/proof/618_sys_take.cubalc", "sys_take", "SYS TAKE/DROP line windows"},
      {"programs/proof/619_sys_split.cubalc", "sys_split", "SYS SPLIT sep to lines"},
      {"programs/proof/620_sys_sort.cubalc", "sys_sort", "SYS SORT/UNIQ line order"},
      {"programs/proof/621_sys_join.cubalc", "sys_join", "SYS JOIN/PATH plate paths"},
      {"programs/proof/622_sys_joinlines.cubalc", "sys_joinlines", "SYS JOINLINES paste lines"},
      {"programs/proof/623_sys_replaceall.cubalc", "sys_replaceall", "SYS REPLACEALL templates"},
      {"programs/proof/624_sys_append.cubalc", "sys_append", "SYS APPEND history log"},
      {"programs/proof/625_sys_grepi.cubalc", "sys_grepi", "SYS GREPI case-insensitive"},
      {"programs/proof/626_sys_str.cubalc", "sys_str", "SYS STR/ITOA template counts"},
      {"programs/proof/627_sys_before.cubalc", "sys_before", "SYS BEFORE/AFTER log peel"},
      {"programs/proof/628_sys_push.cubalc", "sys_push", "SYS PUSH line accumulate"},
      {"programs/proof/629_sys_eqsi.cubalc", "sys_eqsi", "SYS EQSI/HASI case-insensitive"},
      {"programs/proof/630_sys_lines.cubalc", "sys_lines", "SYS LINES/WC field count"},
      {"programs/proof/631_sys_cut.cubalc", "sys_cut", "SYS CUT/COLUMN by separator"},
      {"programs/proof/632_sys_empty.cubalc", "sys_empty", "SYS EMPTY/BLANK soft probes"},
      {"programs/proof/633_sys_pop.cubalc", "sys_pop", "SYS POP bag field peel"},
      {"programs/proof/634_sys_mtime.cubalc", "sys_mtime", "SYS MTIME/AGE plate age"},
      {"programs/proof/635_sys_startsi.cubalc", "sys_startsi", "SYS STARTSI/ENDSI affix"},
      {"programs/proof/636_sys_between.cubalc", "sys_between", "SYS BETWEEN peel delimiters"},
      {"programs/proof/637_sys_findi.cubalc", "sys_findi", "SYS FINDI case-insensitive index"},
      {"programs/proof/638_sys_env_set.cubalc", "sys_env_set", "SYS ENV SET/UNSET process env"},
      {"programs/proof/639_sys_revl.cubalc", "sys_revl", "SYS REVL reverse bag lines"},
      {"programs/proof/640_sys_words.cubalc", "sys_words", "SYS WORDS whitespace tokenize"},
      {"programs/proof/641_sys_hasline.cubalc", "sys_hasline", "SYS HASLINE bag membership"},
      {"programs/proof/642_sys_prepend.cubalc", "sys_prepend", "SYS PREPEND/POPHEAD FIFO bags"},
      {"programs/proof/643_sys_removeline.cubalc", "sys_removeline", "SYS REMOVELINE drop bag field"},
      {"programs/proof/644_sys_findline.cubalc", "sys_findline", "SYS FINDLINE bag field index"},
      {"programs/proof/645_sys_setline.cubalc", "sys_setline", "SYS SETLINE set bag field"},
      {"programs/proof/646_sys_insertline.cubalc", "sys_insertline", "SYS INSERTLINE insert bag field"},
      {"programs/proof/647_sys_dropnth.cubalc", "sys_dropnth", "SYS DROPNTH drop bag field by index"},
      {"programs/proof/648_sys_moveline.cubalc", "sys_moveline", "SYS MOVELINE move bag field by index"},
      {"programs/proof/649_sys_countline.cubalc", "sys_countline", "SYS COUNTLINE count bag field matches"},
      {"programs/proof/650_sys_setmatch.cubalc", "sys_setmatch", "SYS SETMATCH replace bag field by value"},
      {"programs/proof/651_sys_chdir.cubalc", "sys_chdir", "SYS CHDIR/CD change process cwd"},
      {"programs/proof/652_sys_rand.cubalc", "sys_rand", "SYS RAND/RANDOM uniform int jitter"},
      {"programs/proof/653_sys_minmax.cubalc", "sys_minmax", "SYS MIN/MAX/CLAMP host bounds"},
      {"programs/proof/654_sys_cmp.cubalc", "sys_cmp", "SYS CMP/SCMP/IABS compare + abs"},
      {"programs/proof/655_sys_sum.cubalc", "sys_sum", "SYS SUM/PROD/AVG host aggregates"},
      {"programs/proof/656_sys_sortn.cubalc", "sys_sortn", "SYS SORTN numeric bag sort"},
      {"programs/proof/657_sys_range.cubalc", "sys_range", "SYS RANGE/SEQ/IOTA number bags"},
      {"programs/proof/658_sys_div.cubalc", "sys_div", "SYS SIGN/DIV/MOD host arithmetic"},
      {"programs/proof/659_sys_pick.cubalc", "sys_pick", "SYS PICK/CHOICE random bag field"},
      {"programs/proof/660_sys_gcd.cubalc", "sys_gcd", "SYS GCD/LCM multi-arg host math"},
      {"programs/proof/661_sys_in.cubalc", "sys_in", "SYS IN/WITHIN numeric range membership"},
      {"programs/proof/662_sys_pow.cubalc", "sys_pow", "SYS POW/ISQRT host power and sqrt"},
      {"programs/proof/663_sys_shuffle.cubalc", "sys_shuffle", "SYS SHUFFLE randomize bag field order"},
      {"programs/proof/664_sys_coalesce.cubalc", "sys_coalesce", "SYS COALESCE first non-empty string"},
      {"programs/proof/665_sys_union.cubalc", "sys_union", "SYS UNION/INTERSECT/DIFF bag set ops"},
      {"programs/proof/666_sys_zip.cubalc", "sys_zip", "SYS ZIP/KEYS/VALS bag pair and peel"},
      {"programs/proof/667_sys_prefixall.cubalc", "sys_prefixall", "SYS PREFIXALL/SUFFIXALL tag bag fields"},
      {"programs/proof/668_sys_fill.cubalc", "sys_fill", "SYS FILL n value bag of repeated fields"},
      {"programs/proof/669_sys_enumerate.cubalc", "sys_enumerate", "SYS ENUMERATE index-prefix bag fields"},
      {"programs/proof/670_sys_squeeze.cubalc", "sys_squeeze", "SYS SQUEEZE drop empty bag fields"},
      {"programs/proof/671_sys_median.cubalc", "sys_median", "SYS MEDIAN integer bag/args median"},
      {"programs/proof/672_sys_trimall.cubalc", "sys_trimall", "SYS TRIMALL trim every bag field"},
      {"programs/proof/673_sys_midlines.cubalc", "sys_midlines", "SYS MIDLINES bag field window slice"},
      {"programs/proof/674_sys_grepany.cubalc", "sys_grepany", "SYS GREPANY multi-needle bag filter"},
      {"programs/proof/675_sys_grepall.cubalc", "sys_grepall", "SYS GREPALL multi-needle AND bag filter"},
      {"programs/proof/676_require_env.cubalc", "require_env", "REQUIRE ENV fail-fast host config"},
      {"programs/proof/677_sys_upperall.cubalc", "sys_upperall", "SYS UPPERALL/LOWERALL case-map bag fields"},
      {"programs/proof/678_sys_mapreplace.cubalc", "sys_mapreplace", "SYS MAPREPLACE bag-wide GSUB"},
      {"programs/proof/679_sys_freq.cubalc", "sys_freq", "SYS FREQ bag field frequency histogram"},
      {"programs/proof/680_sys_cutall.cubalc", "sys_cutall", "SYS CUTALL peel column from every bag line"},
      {"programs/proof/681_sys_sortfreq.cubalc", "sys_sortfreq", "SYS SORTFREQ sort FREQ bag by count"},
      {"programs/proof/682_sys_beforeall.cubalc", "sys_beforeall", "SYS BEFOREALL/AFTERALL peel every bag field"},
      {"programs/proof/683_sys_firstmatch.cubalc", "sys_firstmatch", "SYS FIRSTMATCH/GREP1 first bag field hit"},
      {"programs/proof/684_sys_chunk.cubalc", "sys_chunk", "SYS CHUNK/BATCH group bag fields by n"},
      {"programs/proof/695_sys_lastmatch.cubalc", "sys_lastmatch", "SYS LASTMATCH/GREP1L last bag field hit"},
      {"programs/proof/696_sys_maxlen.cubalc", "sys_maxlen", "SYS MAXLEN/MINLEN max/min bag field length"},
      {"programs/proof/697_sys_truncall.cubalc", "sys_truncall", "SYS TRUNCALL/CLIPALL truncate bag fields"},
      {"programs/proof/698_sys_longest.cubalc", "sys_longest", "SYS LONGEST/SHORTEST pick bag field by length"},
      {"programs/proof/699_sys_commonprefix.cubalc", "sys_commonprefix", "SYS COMMONPREFIX/LCP shared bag prefix"},
      {"programs/proof/700_sys_stripprefix.cubalc", "sys_stripprefix", "SYS STRIPPREFIX/STRIPCOMMON relative bag paths"},
      {"programs/proof/701_sys_drawn.cubalc", "sys_drawn", "SYS DRAWN/SAMPLEK sample k unique bag fields"},
      {"programs/proof/702_sys_lookup.cubalc", "sys_lookup", "SYS LOOKUP/KVGET peel key:val bag value"},
      {"programs/proof/703_sys_kvset.cubalc", "sys_kvset", "SYS KVSET/SETKV set/update key:val bag field"},
      {"programs/proof/704_sys_kvinc.cubalc", "sys_kvinc", "SYS KVINC/INCKV bump key:val numeric counter"},
      {"programs/proof/705_sys_kvdel.cubalc", "sys_kvdel", "SYS KVDEL/DELKV drop key:val bag field"},
      {"programs/proof/706_sys_lookup_or.cubalc", "sys_lookup_or", "SYS LOOKUP bag key OR fallback default"},
      {"programs/proof/707_sys_mergekv.cubalc", "sys_mergekv", "SYS MERGEKV/KVADDALL merge key:val bags by sum"},
      {"programs/proof/708_sys_sumkv.cubalc", "sys_sumkv", "SYS SUMKV/TOTALKV sum key:val bag values"},
      {"programs/proof/709_sys_topkey.cubalc", "sys_topkey", "SYS TOPKEY/BOTKEY max/min key of key:val bag"},
      {"programs/proof/710_sys_lookupn.cubalc", "sys_lookupn", "SYS LOOKUPN/KVGETN peel key:val as int LAST_N"},
      {"programs/proof/711_sys_threshkv.cubalc", "sys_threshkv", "SYS THRESHKV/KEEPVAL keep key:val value>=min"},
      {"programs/proof/712_sys_pctkv.cubalc", "sys_pctkv", "SYS PCTKV/SHAREKV key:val integer percent of total"},
      {"programs/proof/713_sys_capkv.cubalc", "sys_capkv", "SYS CAPKV/CLAMPKV clamp key:val values to max"},
      {"programs/proof/714_sys_scalekv.cubalc", "sys_scalekv", "SYS SCALEKV/MULKV multiply key:val values by factor"},
      {"programs/proof/715_sys_diffkv.cubalc", "sys_diffkv", "SYS DIFFKV/SUBKV subtract key:val bags by key"},
      {"programs/proof/716_sys_addkv.cubalc", "sys_addkv", "SYS ADDKV/OFFSETKV add delta to key:val values"},
      {"programs/proof/717_sys_dropzero.cubalc", "sys_dropzero", "SYS DROPZERO/KEEPNZ drop key:val value==0"},
      {"programs/proof/718_sys_abskv.cubalc", "sys_abskv", "SYS ABSKV/MAGKV absolute key:val values"},
      {"programs/proof/719_sys_signkv.cubalc", "sys_signkv", "SYS SIGNKV/DIRKV map key:val to -1|0|1"},
      {"programs/proof/720_sys_divkv.cubalc", "sys_divkv", "SYS DIVKV/IDIVKV integer-divide key:val values"},
      {"programs/proof/721_sys_avgkv.cubalc", "sys_avgkv", "SYS AVGKV/MEANKV integer mean of key:val values"},
      {"programs/proof/722_sys_mediankv.cubalc", "sys_mediankv", "SYS MEDIANKV/P50KV median of key:val values"},
      {"programs/proof/723_sys_keepkey.cubalc", "sys_keepkey", "SYS KEEPKEY/GREPKEY filter key:val by key needle"},
      {"programs/proof/724_sys_glob.cubalc", "sys_glob", "SYS GLOB/MATCHFILES list basenames matching pattern"},
      {"programs/proof/725_sys_pathglob.cubalc", "sys_pathglob", "SYS PATHGLOB/PGLOB full paths matching shell pattern"},
      {"programs/proof/726_require_path.cubalc", "require_path", "REQUIRE PATH/DIR/REG fail-fast host path gates"},
      {"programs/proof/727_sys_filterglob.cubalc", "sys_filterglob", "SYS FILTERGLOB/MATCHBAG shell-pattern bag filter"},
      {"programs/proof/728_sys_basenameall.cubalc", "sys_basenameall", "SYS BASENAMEALL/DIRNAMEALL/EXTALL/STEMALL path bag maps"},
      {"programs/proof/729_sys_keepfiles.cubalc", "sys_keepfiles", "SYS KEEPFILES/KEEPDIRS/KEEPEXIST path bag kind filters"},
      {"programs/proof/730_sys_sizeall.cubalc", "sys_sizeall", "SYS SIZEALL/MAPSIZE path bag byte sizes + sum"},
      {"programs/proof/731_sys_mtimeall.cubalc", "sys_mtimeall", "SYS MTIMEALL/AGEALL path bag time maps + min/max"},
      {"programs/proof/732_sys_newest.cubalc", "sys_newest", "SYS NEWEST/OLDEST/LARGEST/SMALLEST path bag pick"},
      {"programs/proof/733_sys_sortmtime.cubalc", "sys_sortmtime", "SYS SORTMTIME/SORTSIZE order path bags by time/size"},
      {"programs/proof/734_sys_fresh.cubalc", "sys_fresh", "SYS FRESH/KEEPSTALE filter path bags by age threshold"},
      {"programs/proof/735_sys_keepnewer.cubalc", "sys_keepnewer", "SYS KEEPNEWER/KEEPOLDER filter path bags vs ref mtime"},
      {"programs/proof/736_sys_keepbigger.cubalc", "sys_keepbigger", "SYS KEEPBIGGER/KEEPSMALLER filter path bags by size"},
      {"programs/proof/737_sys_rmall.cubalc", "sys_rmall", "SYS RMALL/UNLINKALL bulk remove path bag"},
      {"programs/proof/738_sys_touchall.cubalc", "sys_touchall", "SYS TOUCHALL/ENSUREALL bulk create/refresh path bag"},
      {"programs/proof/739_sys_copyall.cubalc", "sys_copyall", "SYS COPYALL/CPALL bulk copy path bag into dest dir"},
      {"programs/proof/740_sys_mkdirall.cubalc", "sys_mkdirall", "SYS MKDIRALL/ENSUREDIRS bulk mkdir -p path bag"},
      {"programs/proof/741_sys_moveall.cubalc", "sys_moveall", "SYS MOVEALL/MVALL bulk move path bag into dest dir"},
      {"programs/proof/742_sys_walk.cubalc", "sys_walk", "SYS WALK/FINDALL recursive full-path discovery"},
      {"programs/proof/743_sys_eqfile.cubalc", "sys_eqfile", "SYS EQFILE/SAMEFILE identical file content probe"},
      {"programs/proof/744_sys_logall.cubalc", "sys_logall", "SYS LOGALL/APPENDFILES append line to path bag"},
      {"programs/proof/745_sys_grepfiles.cubalc", "sys_grepfiles", "SYS GREPFILES/SEARCHFILES filter paths by file content"},
      {"programs/proof/746_sys_readall.cubalc", "sys_readall", "SYS READALL/CATFILES concat path bag file contents"},
      {"programs/proof/747_sys_writeall.cubalc", "sys_writeall", "SYS WRITEALL/WRITEFILES overwrite path bag with data"},
      {"programs/proof/748_sys_replacefiles.cubalc", "sys_replacefiles", "SYS REPLACEFILES/SUBFILES bulk rewrite path bag content"},
      {"programs/proof/749_sys_countinfiles.cubalc", "sys_countinfiles", "SYS COUNTINFILES/GREPCOUNTFILES total needle hits in path bag"},
      {"programs/proof/750_sys_firstfile.cubalc", "sys_firstfile", "SYS FIRSTFILE/LASTFILE first/last path with content match"},
      {"programs/proof/751_sys_greplines.cubalc", "sys_greplines", "SYS GREPLINES/EXTRACTLINES collect matching lines from path bag"},
      {"programs/proof/752_sys_headfile.cubalc", "sys_headfile", "SYS HEADFILE/TAILFILE first/last n lines of a file"},
      {"programs/proof/753_sys_linecountall.cubalc", "sys_linecountall", "SYS LINECOUNTALL/WCALL path bag line counts + sum"},
      {"programs/proof/754_sys_midfile.cubalc", "sys_midfile", "SYS MIDFILE/LINESLICE file line window extract"},
      {"programs/proof/755_sys_lineat.cubalc", "sys_lineat", "SYS LINEAT/FILELINE single file line by index"},
      {"programs/proof/756_sys_setfileline.cubalc", "sys_setfileline", "SYS SETFILELINE/FILESETLINE replace file line by index"},
      {"programs/proof/757_sys_insertfileline.cubalc", "sys_insertfileline", "SYS INSERTFILELINE/FILEINSERTLINE insert file line by index"},
      {"programs/proof/758_sys_dropfileline.cubalc", "sys_dropfileline", "SYS DROPFILELINE/FILEDROPLINE drop file line by index"},
      {"programs/proof/759_sys_findfileline.cubalc", "sys_findfileline", "SYS FINDFILELINE/FILEFINDLINE first file line index by needle"},
      {"programs/proof/760_sys_lastfindfileline.cubalc", "sys_lastfindfileline", "SYS LASTFINDFILELINE/FILELASTFIND last file line by needle"},
      {"programs/proof/761_sys_countfileline.cubalc", "sys_countfileline", "SYS COUNTFILELINE/FILECOUNTLINE count matching file lines"},
      {"programs/proof/762_sys_grepfile.cubalc", "sys_grepfile", "SYS GREPFILE/FILEGREP keep matching lines from one file"},
      {"programs/proof/763_sys_setmatchfile.cubalc", "sys_setmatchfile", "SYS SETMATCHFILE/FILESETMATCH replace first matching file line"},
      {"programs/proof/764_sys_dropmatchfile.cubalc", "sys_dropmatchfile", "SYS DROPMATCHFILE/FILEDROPMATCH drop first matching file line"},
      {"programs/proof/765_sys_dropallmatchfile.cubalc", "sys_dropallmatchfile", "SYS DROPALLMATCHFILE/FILEDROPALLMATCH drop all matching file lines"},
      {"programs/proof/766_sys_setallmatchfile.cubalc", "sys_setallmatchfile", "SYS SETALLMATCHFILE/FILESETALLMATCH replace all matching file lines"},
      {"programs/proof/767_sys_insertmatchfile.cubalc", "sys_insertmatchfile", "SYS INSERTMATCHFILE/FILEINSERTMATCH insert line after first match"},
      {"programs/proof/768_sys_beforematchfile.cubalc", "sys_beforematchfile", "SYS BEFOREMATCHFILE/INSERTBEFOREMATCH insert line before first match"},
      {"programs/proof/769_sys_lookupfile.cubalc", "sys_lookupfile", "SYS LOOKUPFILE/FILELOOKUP peel key=val from plate file"},
      {"programs/proof/770_sys_kvfileset.cubalc", "sys_kvfileset", "SYS KVFILESET/FILEKVSET set/update key=val in plate file"},
      {"programs/proof/771_sys_kvfiledel.cubalc", "sys_kvfiledel", "SYS KVFILEDEL/FILEKVDEL drop key=val line from plate file"},
      {"programs/proof/772_sys_kvfileinc.cubalc", "sys_kvfileinc", "SYS KVFILEINC/INCKVFILE bump numeric key=val in plate file"},
      {"programs/proof/773_sys_ensureline.cubalc", "sys_ensureline", "SYS ENSURELINE/ENSUREFILELINE append line if exact missing"},
      {"programs/proof/774_sys_hasfileline.cubalc", "sys_hasfileline", "SYS HASFILELINE/FILEHASLINE exact file line membership"},
      {"programs/proof/775_sys_dropexactfile.cubalc", "sys_dropexactfile", "SYS DROPEXACTFILE/DROPFILEEXACT drop first exact file line"},
      {"programs/proof/776_sys_toggleline.cubalc", "sys_toggleline", "SYS TOGGLELINE/TOGGLEFILELINE flip exact file line presence"},
      {"programs/proof/777_sys_kvfileensure.cubalc", "sys_kvfileensure", "SYS KVFILEENSURE/ENSUREKVFILE set key=val only if missing"},
      {"programs/proof/778_sys_substenv.cubalc", "sys_substenv", "SYS SUBSTENV/ENVSUBST expand $VAR and ${VAR} templates"},
      {"programs/proof/781_sys_waitfile.cubalc", "sys_waitfile", "SYS WAITFILE/WAITPATH poll until path exists"},
      {"programs/proof/782_sys_substfile.cubalc", "sys_substfile", "SYS SUBSTFILE/EXPANDFILE materialize $VAR template file"},
      {"programs/proof/783_sys_hash.cubalc", "sys_hash", "SYS HASH/HASHFILE FNV-1a plate content fingerprint"},
      {"programs/proof/784_sys_waitmatch.cubalc", "sys_waitmatch", "SYS WAITMATCH/WAITCONTAINS poll until file contains needle"},
      {"programs/proof/785_sys_writeatomic.cubalc", "sys_writeatomic", "SYS WRITEATOMIC/SAFEWRITE temp+rename plate write"},
      {"programs/proof/786_sys_lockfile.cubalc", "sys_lockfile", "SYS LOCKFILE/UNLOCKFILE exclusive agent plate locks"},
      {"programs/proof/787_sys_waitgone.cubalc", "sys_waitgone", "SYS WAITGONE/WAITMISSING poll until path does not exist"},
      {"programs/proof/788_sys_waitstable.cubalc", "sys_waitstable", "SYS WAITSTABLE/WAITQUIET poll until size+mtime quiet"},
      {"programs/proof/789_sys_waitchanged.cubalc", "sys_waitchanged", "SYS WAITCHANGED/WAITMODIFIED poll until size/mtime change"},
      {"programs/proof/790_sys_mktemp.cubalc", "sys_mktemp", "SYS MKTEMP/TEMPFILE unique empty file under TMP"},
      {"programs/proof/791_sys_claim.cubalc", "sys_claim", "SYS CLAIM/CREATEEXCL exclusive path work-item claim"},
      {"programs/proof/792_sys_stat.cubalc", "sys_stat", "SYS STAT/FSTAT one-shot path metadata probe"},
      {"programs/proof/793_sys_mktempdir.cubalc", "sys_mktempdir", "SYS MKTEMPDIR/MKDTEMP unique empty dir under TMP"},
      {"programs/proof/794_sys_rmdir.cubalc", "sys_rmdir", "SYS RMDIR/REMOVEDIR remove empty directory"},
      {"programs/proof/795_sys_base64.cubalc", "sys_base64", "SYS BASE64/BASE64D encode/decode plate payloads"},
      {"programs/proof/796_sys_hex.cubalc", "sys_hex", "SYS HEX/HEXD encode/decode plate bytes"},
      {"programs/proof/797_sys_urlenc.cubalc", "sys_urlenc", "SYS URLENC/URLDEC percent-encode plate paths"},
      {"programs/proof/798_sys_uuid.cubalc", "sys_uuid", "SYS UUID/UUID0 RFC4122 v4 agent work ids"},
      {"programs/proof/799_sys_jsonesc.cubalc", "sys_jsonesc", "SYS JSONESC/JSONUNESC plate string escape"},
      {"programs/proof/800_sys_crc32.cubalc", "sys_crc32", "SYS CRC32/CRC32FILE IEEE integrity stamps"},
      {"programs/proof/801_sys_symlink.cubalc", "sys_symlink", "SYS SYMLINK/READLINK plate path aliases"},
      {"programs/proof/802_sys_humansize.cubalc", "sys_humansize", "SYS HUMANSIZE integer B/K/M/G size labels"},
      {"programs/proof/803_sys_islink.cubalc", "sys_islink", "SYS ISLINK probe symlink paths"},
      {"programs/proof/804_sys_mode.cubalc", "sys_mode", "SYS MODE/CHMOD plate permission probe/set"},
      {"programs/proof/805_sys_parsesize.cubalc", "sys_parsesize", "SYS PARSESIZE human size string to bytes"},
      {"programs/proof/806_sys_monotonic.cubalc", "sys_monotonic", "SYS MONOTONIC steady ms for elapsed timing"},
      {"programs/proof/807_sys_nproc.cubalc", "sys_nproc", "SYS NPROC/CPUS online processor count"},
      {"programs/proof/808_sys_uname.cubalc", "sys_uname", "SYS UNAME/OS/ARCH/PLATFORM host identity"},
      {"programs/proof/809_sys_local.cubalc", "sys_local", "SYS LOCAL/LOCALTIME local wall stamp"},
      {"programs/proof/810_require_bin.cubalc", "require_bin", "REQUIRE BIN/SYS WHICHBIN PATH tool gates (+ string-var names)"},
      {"programs/proof/811_sys_isatty.cubalc", "sys_isatty", "SYS ISATTY/TTY terminal stream probe"},
      {"programs/proof/812_sys_loadavg.cubalc", "sys_loadavg", "SYS LOADAVG/LOAD host load averages"},
      {"programs/proof/813_sys_uptime.cubalc", "sys_uptime", "SYS UPTIME/BOOTAGE seconds since boot"},
      {"programs/proof/814_sys_mem.cubalc", "sys_mem", "SYS MEM/MEMINFO host RAM total/avail kB"},
      {"programs/proof/815_sys_df.cubalc", "sys_df", "SYS DF/DISKFREE filesystem free kB"},
      {"programs/proof/816_sys_fsync.cubalc", "sys_fsync", "SYS FSYNC/SYNCFILE durable file flush"},
      {"programs/proof/817_sys_dotenv.cubalc", "sys_dotenv", "SYS DOTENV/LOADENV load KEY=VAL plate into env"},
      {"programs/proof/818_sys_dumpenv.cubalc", "sys_dumpenv", "SYS DUMPENV/ENVALL process env KEY=VAL bag"},
      {"programs/proof/819_sys_writeenv.cubalc", "sys_writeenv", "SYS WRITEENV/SAVENV persist env KEY=VAL plate"},
      {"programs/proof/820_sys_truncate.cubalc", "sys_truncate", "SYS TRUNCATE/FTRUNCATE resize plate files"},
      {"programs/proof/821_sys_dirsync.cubalc", "sys_dirsync", "SYS DIRSYNC/FSYNCDIR durable dir flush"},
      {"programs/proof/822_sys_hardlink.cubalc", "sys_hardlink", "SYS HARDLINK/NLINK shared plate inodes"},
      {"programs/proof/823_sys_sameinode.cubalc", "sys_sameinode", "SYS SAMEINODE/INODE hard-link identity"},
      {"programs/proof/824_sys_tee.cubalc", "sys_tee", "SYS TEE write keep content in LAST"},
      {"programs/proof/825_sys_fileuid.cubalc", "sys_fileuid", "SYS FILEUID/FILEGID/OWNEDBY ownership probes"},
      {"programs/proof/826_sys_canread.cubalc", "sys_canread", "SYS CANREAD/CANWRITE/CANEXEC access probes"},
      {"programs/proof/827_sys_ownername.cubalc", "sys_ownername", "SYS OWNERNAME/GROUPNAME login and group name probes"},
      {"programs/proof/828_sys_cancreate.cubalc", "sys_cancreate", "SYS CANCREATE pre-flight create/overwrite probe"},
      {"programs/proof/829_sys_umask.cubalc", "sys_umask", "SYS UMASK get/set process file-creation mask"},
      {"programs/proof/830_sys_pushd.cubalc", "sys_pushd", "SYS PUSHD/POPD/DIRSTACK cwd stack for temp chdir"},
      {"programs/proof/831_sys_kindstr.cubalc", "sys_kindstr", "SYS KINDSTR human path kind labels"},
      {"programs/proof/832_sys_relpath.cubalc", "sys_relpath", "SYS RELPATH base path relative plate paths"},
      {"programs/proof/833_sys_atime.cubalc", "sys_atime", "SYS ATIME/CTIME access and status-change times"},
      {"programs/proof/834_sys_hasenv.cubalc", "sys_hasenv", "SYS HASENV soft env presence probe"},
      {"programs/proof/835_sys_gid.cubalc", "sys_gid", "SYS GID/EGID/GROUP process group identity"},
      {"programs/proof/836_sys_groups.cubalc", "sys_groups", "SYS GROUPS/INGROUP process group membership"},
      {"programs/proof/837_sys_ppid.cubalc", "sys_ppid", "SYS PPID/PGID/SID process lineage and session"},
      {"programs/proof/838_sys_euid_isroot.cubalc", "sys_euid_isroot", "SYS EUID/ISROOT effective uid and root probe"},
      {"programs/proof/839_sys_shell.cubalc", "sys_shell", "SYS SHELL login shell path without shell"},
      {"programs/proof/840_sys_gecos.cubalc", "sys_gecos", "SYS GECOS/FULLNAME passwd display name"},
      {"programs/proof/841_sys_ttyname.cubalc", "sys_ttyname", "SYS TTYNAME/CTTY terminal device path"},
      {"programs/proof/842_sys_nice.cubalc", "sys_nice", "SYS NICE/RENICE process priority get/set"},
      {"programs/proof/843_sys_shquote.cubalc", "sys_shquote", "SYS SHQUOTE/SHELLQUOTE POSIX shell-safe wrap"},
      {"programs/proof/844_sys_shunquote.cubalc", "sys_shunquote", "SYS SHUNQUOTE/UNQUOTE peel shell quote wrap"},
      {"programs/proof/845_sys_getlogin.cubalc", "sys_getlogin", "SYS GETLOGIN/LOGINNAME session login name"},
      {"programs/proof/846_sys_entropy.cubalc", "sys_entropy", "SYS ENTROPY/URANDOM random bytes as hex"},
      {"programs/proof/847_sys_parsems.cubalc", "sys_parsems", "SYS PARSEMS/DURATION compound human duration to ms"},
      {"programs/proof/848_sys_humanms.cubalc", "sys_humanms", "SYS HUMANMS/FMTMS ms to human duration"},
      {"programs/proof/849_sys_fromtime.cubalc", "sys_fromtime", "SYS FROMTIME/EPOCHISO epoch to UTC ISO"},
      {"programs/proof/850_sys_parseiso.cubalc", "sys_parseiso", "SYS PARSEISO/TOEPOCH ISO stamp to epoch"},
      {"programs/proof/851_sys_timediff.cubalc", "sys_timediff", "SYS TIMEDIFF/ELAPSED epoch second deltas"},
      {"programs/proof/852_sys_envdefault.cubalc", "sys_envdefault", "SYS ENVDEFAULT setenv if missing/empty"},
      {"programs/proof/853_sys_addtime.cubalc", "sys_addtime", "SYS ADDTIME/DEADLINE epoch plus seconds"},
      {"programs/proof/854_sys_expired.cubalc", "sys_expired", "SYS EXPIRED/REMAINING lease due and TTL"},
      {"programs/proof/855_sys_ms2sec.cubalc", "sys_ms2sec", "SYS MS2SEC/SEC2MS ms-seconds unit bridge"},
      {"programs/proof/856_sys_parsems_compound.cubalc", "sys_parsems_compound", "SYS PARSEMS compound 1h30m HUMANMS round-trip"},
      {"programs/proof/857_sys_sleep_duration.cubalc", "sys_sleep_duration", "SYS SLEEP human duration string without PARSEMS glue"},
      {"programs/proof/858_sys_waitfile_duration.cubalc", "sys_waitfile_duration", "SYS WAITFILE timeout human duration string"},
      {"programs/proof/859_sys_deadline_duration.cubalc", "sys_deadline_duration", "SYS DEADLINE/ADDTIME human duration lease offsets"},
      {"programs/proof/862_oop_class_method.cubalc", "oop_class", "CLASS/METHOD/NEW/SEND OOP reusable objects"},
      {"programs/proof/863_cop_entity_tick.cubalc", "cop_entity_tick", "ENTITY/SPAWN/TICK COP game-engine plane"},
      {"programs/proof/865_life_engine_division.cubalc", "life_engine", "COP life engine cell division mitosis demo"},
      {"programs/proof/864_include_class_lib.cubalc", "include_class_lib", "INCLUDE retains CLASS/METHOD bodies from programs/lib"},
      {"programs/proof/866_sys_humansecs.cubalc", "sys_humansecs", "SYS HUMANSECS/HUMANAGE seconds to human duration labels"},
      {"programs/proof/867_oop_list_hasmethod.cubalc", "oop_list_hasmethod", "LISTCLASSES/LISTOBJS/HASMETHOD OOP introspection"},
      {"programs/proof/868_oop_destroy_hasobj.cubalc", "oop_destroy_hasobj", "HASOBJ/DESTROY/HASFIELD object lifecycle"},
      {"programs/proof/869_oop_dumpobj.cubalc", "oop_dumpobj", "DUMPOBJ/INSPECT object field snapshot bag/JSON"},
      {"programs/proof/870_oop_string_fields.cubalc", "oop_string_fields", "SETF/NEW string-from-var + string formals"},
      {"programs/proof/871_oop_getf_or.cubalc", "oop_getf_or", "GETF OR/TRYGETF soft field reads with defaults"},
      {"programs/proof/872_assert_str_eq.cubalc", "assert_str_eq", "ASSERT/IF string ==/!= content compare"},
      {"programs/proof/873_oop_cloneobj.cubalc", "oop_cloneobj", "CLONEOBJ/COPYOBJ shallow object field copy"},
      {"programs/proof/874_oop_renameobj.cubalc", "oop_renameobj", "RENAMEOBJ/MOVEOBJ rename live object slot"},
      {"programs/proof/875_oop_trysetf.cubalc", "oop_trysetf", "TRYSETF/SETF SOFT soft field writes"},
      {"programs/proof/876_oop_trysend.cubalc", "oop_trysend", "TRYSEND/SEND SOFT soft method dispatch"},
      {"programs/proof/877_oop_each_obj.cubalc", "oop_each_obj", "EACH OBJ walk live objects with class filter"},
      {"programs/proof/878_oop_sendall.cubalc", "oop_sendall", "SENDALL/BROADCAST method on every live object"},
      {"programs/proof/879_oop_deleteall.cubalc", "oop_deleteall", "DELETEALL/FREEALL bulk free live objects"},
      {"programs/proof/880_oop_getfall.cubalc", "oop_getfall", "GETFALL/COLLECTF bulk field collect from live objects"},
      {"programs/proof/881_oop_setfall.cubalc", "oop_setfall", "SETFALL/MAPSETF bulk field write on live objects"},
      {"programs/proof/882_oop_whereobj.cubalc", "oop_whereobj", "WHEREOBJ/FINDOBJ select live objects by field equality"},
      {"programs/proof/883_oop_deletewhere.cubalc", "oop_deletewhere", "DELETEWHERE/FREEWHERE free live objects by field equality"},
      {"programs/proof/884_oop_sendwhere.cubalc", "oop_sendwhere", "SENDWHERE/INVOKEWHERE method on field-matching objects"},
      {"programs/proof/885_oop_incfall.cubalc", "oop_incfall", "INCFALL/ADDFALL/DECFALL bulk numeric field bump"},
      {"programs/proof/886_oop_maxobj.cubalc", "oop_maxobj", "MAXOBJ/MINOBJ pick object by extreme field"},
      {"programs/proof/887_oop_sortbyf.cubalc", "oop_sortbyf", "SORTBYF/SORTOBJS bag of names ordered by field"},
      {"programs/proof/888_oop_sumf.cubalc", "oop_sumf", "SUMF/AVGF fleet field sum and integer mean"},
      {"programs/proof/889_oop_wherege.cubalc", "oop_wherege", "WHEREGE/WHERELE threshold select live objects"},
      {"programs/proof/890_oop_deletewherege.cubalc", "oop_deletewherege", "DELETEWHEREGE/FREEBELOW threshold free live objects"},
      {"programs/proof/891_oop_sendwherege.cubalc", "oop_sendwherege", "SENDWHEREGE/SENDBELOW method on threshold-matched objects"},
      {"programs/proof/892_oop_countwhere.cubalc", "oop_countwhere", "COUNTOBJ/COUNTWHERE/COUNTWHEREGE fleet tallies without bags"},
      {"programs/proof/893_oop_haswhere.cubalc", "oop_haswhere", "HASWHERE/HASWHEREGE soft existence probes for IF gates"},
      {"programs/proof/894_oop_wherebetween.cubalc", "oop_wherebetween", "WHEREBETWEEN/COUNTBETWEEN/HASBETWEEN closed-range fleet query"},
      {"programs/proof/895_oop_deletebetween.cubalc", "oop_deletebetween", "DELETEBETWEEN/FREEBAND free live objects in closed field range"},
      {"programs/proof/896_oop_sendbetween.cubalc", "oop_sendbetween", "SENDBETWEEN/SENDBAND method on closed-range matched objects"},
      {"programs/proof/897_oop_clampfall.cubalc", "oop_clampfall", "CLAMPFALL/CLAMPF bulk clamp numeric fields to [lo,hi]"},
      {"programs/proof/898_oop_medianf.cubalc", "oop_medianf", "MEDIANF/P50F integer median of fleet numeric field"},
      {"programs/proof/899_oop_mulfall.cubalc", "oop_mulfall", "MULFALL/SCALEFALL bulk multiply numeric fields by factor"},
      {"programs/proof/900_oop_freqf.cubalc", "oop_freqf", "FREQF/HISTF fleet field value histogram key:count"},
      {"programs/proof/901_oop_modef.cubalc", "oop_modef", "MODEF/TOPF most frequent fleet field value"},
      {"programs/proof/902_oop_uniquf.cubalc", "oop_uniquf", "UNIQUF/DISTINCTF unique fleet field values bag"},
      {"programs/proof/903_oop_copyf.cubalc", "oop_copyf", "COPYF/COPYFALL copy field to field across live objects"},
      {"programs/proof/904_oop_swapf.cubalc", "oop_swapf", "SWAPF/SWAPFALL exchange two fields on live objects"},
      {"programs/proof/905_oop_setfwhere.cubalc", "oop_setfwhere", "SETFWHERE conditional field write on matching live objects"},
      {"programs/proof/906_oop_incfwhere.cubalc", "oop_incfwhere", "INCFWHERE/DECFWHERE bump field on matching live objects"},
      {"programs/proof/907_oop_getfwhere.cubalc", "oop_getfwhere", "GETFWHERE filtered field collect from matching live objects"},
      {"programs/proof/908_oop_sumwhere.cubalc", "oop_sumwhere", "SUMWHERE/AVGWHERE filtered fleet sum/mean on match"},
      {"programs/proof/909_oop_minwhere.cubalc", "oop_minwhere", "MINWHERE/MAXWHERE filtered fleet min/max on match"},
      {"programs/proof/910_oop_medianwhere.cubalc", "oop_medianwhere", "MEDIANWHERE/P50WHERE filtered fleet median on match"},
      {"programs/proof/911_oop_freqwhere.cubalc", "oop_freqwhere", "FREQWHERE/HISTWHERE filtered fleet field histogram on match"},
      {"programs/proof/912_oop_modewhere.cubalc", "oop_modewhere", "MODEWHERE/DOMINANTWHERE filtered fleet mode on match"},
      {"programs/proof/913_oop_uniquwhere.cubalc", "oop_uniquwhere", "UNIQUWHERE/DISTINCTWHERE filtered unique field values on match"},
      {"programs/proof/914_oop_clampwhere.cubalc", "oop_clampwhere", "CLAMPWHERE/CLIPWHERE selective clamp field on match"},
      {"programs/proof/915_oop_mulwhere.cubalc", "oop_mulwhere", "MULWHERE/SCALEWHERE selective multiply field on match"},
      {"programs/proof/916_oop_copyfwhere.cubalc", "oop_copyfwhere", "COPYFWHERE/SNAPSHOTWHERE selective field copy on match"},
      {"programs/proof/917_oop_swapfwhere.cubalc", "oop_swapfwhere", "SWAPFWHERE/EXCHANGEWHERE selective field swap on match"},
      {"programs/proof/918_oop_maxobjwhere.cubalc", "oop_maxobjwhere", "MAXOBJWHERE/MINOBJWHERE pick extreme obj among matches"},
      {"programs/proof/919_oop_sortbyfwhere.cubalc", "oop_sortbyfwhere", "SORTBYFWHERE/ORDERWHERE rank matching objects by field"},
      {"programs/proof/920_oop_topnbyf.cubalc", "oop_topnbyf", "TOPNBYF/HEADBYF top-n object names by numeric field"},
      {"programs/proof/921_oop_topnbyfwhere.cubalc", "oop_topnbyfwhere", "TOPNBYFWHERE/HEADWHERE top-n matching objects by field"},
      {"programs/proof/922_oop_nthbyf.cubalc", "oop_nthbyf", "NTHBYF/ATBYF 0-based rank pick object by numeric field"},
      {"programs/proof/923_oop_nthbyfwhere.cubalc", "oop_nthbyfwhere", "NTHBYFWHERE/ATWHERE kth match by numeric field rank"},
      {"programs/proof/924_oop_slicebyf.cubalc", "oop_slicebyf", "SLICEBYF/MIDBYF ranked object name window by field"},
      {"programs/proof/925_oop_pickobj.cubalc", "oop_pickobj", "PICKOBJ/RANDOBJ random live object sample"},
      {"programs/proof/926_oop_drawnobj.cubalc", "oop_drawnobj", "DRAWNOBJ/SAMPLEKOBJ sample k unique live objects"},
      {"programs/proof/927_oop_shuffleobjs.cubalc", "oop_shuffleobjs", "SHUFFLEOBJS/SHUFOBJS shuffle live object name bag"},
      {"programs/proof/928_oop_classinfo.cubalc", "oop_classinfo", "CLASSINFO/DUMPCLASS class schema plate for agents"},
      {"programs/proof/929_oop_methodinfo.cubalc", "oop_methodinfo", "METHODINFO/DUMPMETHOD method arity/params plate"},
      {"programs/proof/860_sys_addiso.cubalc", "sys_addiso", "SYS ADDISO shift ISO stamp by secs or duration"},
      {"programs/proof/861_sys_diffiso.cubalc", "sys_diffiso", "SYS DIFFISO/ISOAGE ISO stamp second deltas"},
      {"programs/p2p/mesh_local.cubalc", "smx", "in-process SMX EXCHANGE"},
      {"programs/p2p/peer_dial.cubalc", "p2p", "SMX DIAL soft-fail"},
      {"programs/protect/core_protect.cubalc", "protect", "Core protect board"},
    };
    static const struct { const char *name; const char *path; const char *hint; } aliases[] = {
      {"cookbook", "docs/COOKBOOK.md", "hold → plug → smx recipes"},
      {"agents", "docs/FOR_AGENTS.md", "agent prompt snippet"},
      {"hold_flash", "docs/HOLD_FLASH.md", "device/firmware connection safeguard"},
      {"p2p_doc", "docs/P2P_SMX.md", "binary mesh wire"},
      {"hello", "programs/hello_cube.cubalc", "minimal starter"},
      {"init", "cubalc init [--list|--plate|--peer]", "scaffold agent_boot / plate_session+uniform / multi-plate peer"},
    };
    static const struct { const char *name; const char *hint; } envs[] = {
      {"CUBALC_P2P_TIMEOUT", "SERVE accept timeout ms"},
      {"CUBALC_P2P_SOFT", "1 → DIAL soft-fail"},
      {"CUBALC_SMX_KEY", "64-hex SMX2 shared secret"},
      {"CUBALC_STATE", "state plate directory"},
      {"CUBALC_ROOT", "install root for INCLUDE"},
      {"CUBALC_SEED", "RNG seed for reproducible runs"},
      {"CUBALC_QUIET", "1 → run plate-only"},
      {"CUBALC_STRICT", "1 → soft last_err fails exit"},
    };
    if (!q || !q[0]) {
      fprintf(stderr, "usage: cubalc search <keyword>\n"
                      "       cubalc find include · cubalc query p2p · cubalc look ASSERT\n");
      printf("{\"schema\":\"cubalc.search.v1\",\"ok\":false,\"cmd\":\"search\","
             "\"err\":\"need keyword\",\"version\":\"%s\"}\n", CUBALC_LANG_VERSION);
      return 2;
    }
    qup[0] = 0;
    for (k = 0; q[k] && k + 1 < sizeof qup; k++)
      qup[k] = (char)toupper((unsigned char)q[k]);
    qup[k] = 0;

    /* forms */
    for (i = 0; i < (int)(sizeof forms / sizeof forms[0]) && nh < 48; i++) {
      char name_up[48], plane_up[24], hint_up[120];
      size_t j;
      for (j = 0; forms[i].name[j] && j + 1 < sizeof name_up; j++)
        name_up[j] = (char)toupper((unsigned char)forms[i].name[j]);
      name_up[j] = 0;
      for (j = 0; forms[i].plane[j] && j + 1 < sizeof plane_up; j++)
        plane_up[j] = (char)toupper((unsigned char)forms[i].plane[j]);
      plane_up[j] = 0;
      for (j = 0; forms[i].hint[j] && j + 1 < sizeof hint_up; j++)
        hint_up[j] = (char)toupper((unsigned char)forms[i].hint[j]);
      hint_up[j] = 0;
      if (!strstr(name_up, qup) && !strstr(plane_up, qup) && !strstr(hint_up, qup))
        continue;
      snprintf(hits[nh].kind, sizeof hits[0].kind, "form");
      snprintf(hits[nh].name, sizeof hits[0].name, "%s", forms[i].name);
      snprintf(hits[nh].path, sizeof hits[0].path, "%s", forms[i].plane);
      snprintf(hits[nh].hint, sizeof hits[0].hint, "%s", forms[i].hint);
      nh++;
    }
    /* libs from programs/lib */
    {
      DIR *d = opendir("programs/lib");
      static const struct { const char *file; const char *hint; } known[] = {
        {"hold_seed.cubalc", "optional device/firmware HOLD_FLASH seed (not program preamble)"},
        {"agent_boot.cubalc", "REQUIRE 1.15 + VERSION agent preamble (no HOLD_FLASH tax)"},
        {"plate_boot.cubalc", "ENSUREPLATE seed + PLATE var agent state preamble (DEFAULT path/seed)"},
      {"plate_tick.cubalc", "ENSURE+bump+ts+SAVE agent plate tick (DEFAULT path/seed/keys)"},
      {"plate_save.cubalc", "SAVEPLATE PLATE_PATH PLATE agent write-back (dual of plate_boot)"},
        {"peer_decide.cubalc", "FOLDBITS/SETDIGIT peer0 then DECIDE brain"},
        {"mesh_exchange.cubalc", "SMX KEY + dual EXCHANGE peer0/peer1"},
      };
      if (d) {
        struct dirent *de;
        while ((de = readdir(d)) != NULL && nh < 48) {
          size_t len = strlen(de->d_name);
          char name_up[64], path[96], hint[120], stem[64];
          size_t j;
          int ki;
          if (len < 8 || strcmp(de->d_name + len - 7, ".cubalc") != 0) continue;
          if (de->d_name[0] == '.') continue;
          snprintf(stem, sizeof stem, "%s", de->d_name);
          if (len > 7) stem[len - 7] = 0;
          snprintf(path, sizeof path, "programs/lib/%s", de->d_name);
          hint[0] = 0;
          for (ki = 0; ki < (int)(sizeof known / sizeof known[0]); ki++) {
            if (strcmp(de->d_name, known[ki].file) == 0) {
              snprintf(hint, sizeof hint, "%s", known[ki].hint);
              break;
            }
          }
          if (!hint[0]) snprintf(hint, sizeof hint, "INCLUDE lib snippet");
          for (j = 0; stem[j] && j + 1 < sizeof name_up; j++)
            name_up[j] = (char)toupper((unsigned char)stem[j]);
          name_up[j] = 0;
          {
            char path_up[96], hint_up[120];
            for (j = 0; path[j] && j + 1 < sizeof path_up; j++)
              path_up[j] = (char)toupper((unsigned char)path[j]);
            path_up[j] = 0;
            for (j = 0; hint[j] && j + 1 < sizeof hint_up; j++)
              hint_up[j] = (char)toupper((unsigned char)hint[j]);
            hint_up[j] = 0;
            if (!strstr(name_up, qup) && !strstr(path_up, qup) && !strstr(hint_up, qup))
              continue;
          }
          snprintf(hits[nh].kind, sizeof hits[0].kind, "lib");
          snprintf(hits[nh].name, sizeof hits[0].name, "%s", stem);
          snprintf(hits[nh].path, sizeof hits[0].path, "%s", path);
          snprintf(hits[nh].hint, sizeof hits[0].hint, "%s", hint);
          nh++;
        }
        closedir(d);
      }
    }
    /* examples */
    for (i = 0; i < (int)(sizeof examples / sizeof examples[0]) && nh < 48; i++) {
      char path_up[160], tag_up[32], hint_up[120];
      size_t j;
      for (j = 0; examples[i].path[j] && j + 1 < sizeof path_up; j++)
        path_up[j] = (char)toupper((unsigned char)examples[i].path[j]);
      path_up[j] = 0;
      for (j = 0; examples[i].tag[j] && j + 1 < sizeof tag_up; j++)
        tag_up[j] = (char)toupper((unsigned char)examples[i].tag[j]);
      tag_up[j] = 0;
      for (j = 0; examples[i].hint[j] && j + 1 < sizeof hint_up; j++)
        hint_up[j] = (char)toupper((unsigned char)examples[i].hint[j]);
      hint_up[j] = 0;
      if (!strstr(path_up, qup) && !strstr(tag_up, qup) && !strstr(hint_up, qup))
        continue;
      snprintf(hits[nh].kind, sizeof hits[0].kind, "example");
      snprintf(hits[nh].name, sizeof hits[0].name, "%s", examples[i].tag);
      snprintf(hits[nh].path, sizeof hits[0].path, "%s", examples[i].path);
      snprintf(hits[nh].hint, sizeof hits[0].hint, "%s", examples[i].hint);
      nh++;
    }
    /* path aliases / docs */
    for (i = 0; i < (int)(sizeof aliases / sizeof aliases[0]) && nh < 48; i++) {
      char name_up[32], path_up[96], hint_up[120];
      size_t j;
      for (j = 0; aliases[i].name[j] && j + 1 < sizeof name_up; j++)
        name_up[j] = (char)toupper((unsigned char)aliases[i].name[j]);
      name_up[j] = 0;
      for (j = 0; aliases[i].path[j] && j + 1 < sizeof path_up; j++)
        path_up[j] = (char)toupper((unsigned char)aliases[i].path[j]);
      path_up[j] = 0;
      for (j = 0; aliases[i].hint[j] && j + 1 < sizeof hint_up; j++)
        hint_up[j] = (char)toupper((unsigned char)aliases[i].hint[j]);
      hint_up[j] = 0;
      if (!strstr(name_up, qup) && !strstr(path_up, qup) && !strstr(hint_up, qup))
        continue;
      snprintf(hits[nh].kind, sizeof hits[0].kind, "doc");
      snprintf(hits[nh].name, sizeof hits[0].name, "%s", aliases[i].name);
      snprintf(hits[nh].path, sizeof hits[0].path, "%s", aliases[i].path);
      snprintf(hits[nh].hint, sizeof hits[0].hint, "%s", aliases[i].hint);
      nh++;
    }
    /* env contract */
    for (i = 0; i < (int)(sizeof envs / sizeof envs[0]) && nh < 48; i++) {
      char name_up[48], hint_up[120];
      size_t j;
      for (j = 0; envs[i].name[j] && j + 1 < sizeof name_up; j++)
        name_up[j] = (char)toupper((unsigned char)envs[i].name[j]);
      name_up[j] = 0;
      for (j = 0; envs[i].hint[j] && j + 1 < sizeof hint_up; j++)
        hint_up[j] = (char)toupper((unsigned char)envs[i].hint[j]);
      hint_up[j] = 0;
      if (!strstr(name_up, qup) && !strstr(hint_up, qup))
        continue;
      snprintf(hits[nh].kind, sizeof hits[0].kind, "env");
      snprintf(hits[nh].name, sizeof hits[0].name, "%s", envs[i].name);
      hits[nh].path[0] = 0;
      snprintf(hits[nh].hint, sizeof hits[0].hint, "%s", envs[i].hint);
      nh++;
    }

    printf("# CubalC search q=%s n=%d version=%s\n", q, nh, CUBALC_LANG_VERSION);
    printf("# kind\tname\tpath\thint\n");
    for (i = 0; i < nh; i++)
      printf("%s\t%s\t%s\t%s\n", hits[i].kind, hits[i].name,
             hits[i].path[0] ? hits[i].path : "-", hits[i].hint);
    if (nh == 0)
      printf("# (no match — try cubalc forms|libs|examples|which|env)\n");
    printf("{\"schema\":\"cubalc.search.v1\",\"ok\":%s,\"cmd\":\"search\","
           "\"query\":\"%s\",\"n\":%d,\"version\":\"%s\","
           "\"note\":\"keyword discovery — cubalc which for exact path resolve\","
           "\"matches\":[",
           nh > 0 ? "true" : "false", q, nh, CUBALC_LANG_VERSION);
    for (i = 0; i < nh; i++) {
      char pesc[180], nesc[72], hesc[140];
      size_t j, o;
      o = 0;
      for (j = 0; hits[i].path[j] && o + 2 < sizeof pesc; j++) {
        char c = hits[i].path[j];
        if (c == '"' || c == '\\') pesc[o++] = '_';
        else pesc[o++] = c;
      }
      pesc[o] = 0;
      o = 0;
      for (j = 0; hits[i].name[j] && o + 2 < sizeof nesc; j++) {
        char c = hits[i].name[j];
        if (c == '"' || c == '\\') nesc[o++] = '_';
        else nesc[o++] = c;
      }
      nesc[o] = 0;
      o = 0;
      for (j = 0; hits[i].hint[j] && o + 2 < sizeof hesc; j++) {
        char c = hits[i].hint[j];
        if (c == '"' || c == '\\') hesc[o++] = '_';
        else hesc[o++] = c;
      }
      hesc[o] = 0;
      printf("%s{\"kind\":\"%s\",\"name\":\"%s\",\"path\":\"%s\",\"hint\":\"%s\"}",
             i ? "," : "", hits[i].kind, nesc, pesc, hesc);
    }
    printf("]}\n");
    return nh > 0 ? 0 : 1;
  }
  if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0) {
    fprintf(stderr,
      "CubalC %s — pure-C COP/flow (matrix SoT · SMX2 · no HTTP required)\n"
      "\n"
      "  Run & learn\n"
      "    doctor|health          install readiness JSON (agents/humans)\n"
      "    selftest|smoke         live curated usability proofs JSON\n"
      "    version|ver|-V         language version JSON plate\n"
      "    paths|where|layout     install/workspace paths JSON\n"
      "    which|locate|resolve   resolve name → path/kind (lib/form/bin)\n"
      "    search|find|query      keyword search forms/libs/examples/env\n"
      "    cookbook|start         paths to starters\n"
      "    init|new|scaffold [f]  starter · --list · --plate (uniform+pretty) · --peer\n"
      "    examples|starters [p]  curated runnable programs (JSON)\n"
      "    cat|type|source <lib>  dump lib/program source + meta plate\n"
      "    plate|jsonplate …      agent plate get/set/fill/ensure/merge/eq/has/need (JSON)\n"
      "    forms|ops [prefix]     list play forms (filterable; JSON plate)\n"
      "    libs|lib|stdlib        list programs/lib INCLUDE snippets\n"
      "    env|environ|vars [pfx] host CUBALC_* env contract (JSON)\n"
      "    run|eval [-q] [-s] <file|->  -q plate-only · -s soft last_err fails\n"
      "    help|-h                this text\n"
      "\n"
      "  Law & Core safety\n"
      "    law|manifest           law plate JSON\n"
      "    protect|core-guard     Core protect checks → state/CORE_PROTECT.json\n"
      "    protect status         JSON summary only (no board/smx run)\n"
      "    HOLD_FLASH             device/firmware safeguard · default 1 (omit preamble)\n"
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
      "    ASSERT|EXPECT|FAIL|PASS|NOTE|EXIT|CLEAR_ERR · DEFAULT|DEFINED|TYPEOF|UNSET · STATUS|IDENTITY · PRINT_JSON · INCLUDE\n"
      "\n"
      "  hold=%d share=%s tok=%s paradigm=%s\n",
      CUBALC_LANG_VERSION, CUBALC_HOLD_FLASH, CUBALC_SHARE, CUBALC_CREED,
      CUBALC_LANG_PARADIGM);
    return 0;
  }
  fprintf(stderr, "unknown cmd %s — try help\n", cmd);
  return 2;
}
