/* CubalC CLI smx-exchange — F_main peel from main · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_smx_exchange.h"
#include "cubalc.h"
#include "cubalc_smx.h"
#include "cubalc_algocube.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

static void state_dir(char *buf, size_t n) {
  const char *e = getenv("CUBALC_STATE");
  if (e && e[0]) { snprintf(buf, n, "%s", e); return; }
  snprintf(buf, n, "state");
}

static void ensure_dir(const char *d) { mkdir(d, 0755); }

int cubalc_cmd_smx_exchange(int argc, char **argv) {
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
