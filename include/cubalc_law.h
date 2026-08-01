/* CubalC law plate — machine tokens only (no prose). */
#ifndef CUBALC_LAW_H
#define CUBALC_LAW_H

#define CUBALC_BUDGET       40
#define CUBALC_ATOM_BITS    64
#define CUBALC_MAX_CUBES    40
#define CUBALC_MAX_PORTS    8
#define CUBALC_ID_LEN       32
/* creed = opaque status token, not human language */
#define CUBALC_CREED        "C3"
#define CUBALC_SHARE        "smx"
#define CUBALC_HOLD_FLASH   1
#define CUBALC_MAGIC_BIN    0x43424C43u  /* CBLC */
#define CUBALC_PROTO_V1     1
#define CUBALC_PROTO_SMX2   2
#define CUBALC_SMX_MAC_LEN  32
#define CUBALC_SMX_NONCE_LEN 8
#define CUBALC_SMX_KEY_LEN  32
#define CUBALC_SMX_F_HOLD_FLASH     0x01u
#define CUBALC_SMX_F_REQUIRE_COMPAT 0x02u
#define CUBALC_SMX_F_PROTON_CREATE  0x04u
#define CUBALC_SMX_F_PROTON_DESTROY 0x08u
#define CUBALC_SMX_F_CHAIN_ONLY     0x10u

#define CUBALC_PORT_IN   0
#define CUBALC_PORT_OUT  1

#define CUBALC_LANG_NAME    "CubalC"
#define CUBALC_LANG_AKA     "C3"
#define CUBALC_LANG_PARADIGM "COP/flow"
#define CUBALC_LANG_VERSION "1.6.1-resolve"
/* Core talk is SMX2/CBLC binary. HTTP is optional host edge only — never required. */
#define CUBALC_HTTP_REQUIRED 0
#define CUBALC_MAX_SRC      (256 * 1024)
#define CUBALC_MAX_HEAP     256

#define CUBALC_KIND_VOID    0
#define CUBALC_KIND_BIT     1
#define CUBALC_KIND_I64     2
#define CUBALC_KIND_F64     3
#define CUBALC_KIND_STR     4
#define CUBALC_KIND_FN      5
#define CUBALC_KIND_CUBE    6
#define CUBALC_KIND_PEER    7
#define CUBALC_KIND_ERR     8

#define CUBALC_LAW_SOT              0
#define CUBALC_LAW_IN_OUT           1
#define CUBALC_LAW_CORE_IO          2
#define CUBALC_LAW_BINARY_TALK      3
#define CUBALC_LAW_MATRIX_KEY       4
#define CUBALC_LAW_HOLD_FLASH       5
#define CUBALC_LAW_NO_BRAIN_WIRES   6
#define CUBALC_LAW_SHARE_MATRIX     7
#define CUBALC_LAW_DEVICES_FREE     8
#define CUBALC_LAW_ONE_COMMANDER    9
#define CUBALC_LAW_MANIFEST_SMX     10
#define CUBALC_LAW_ALGOCUBE         11
#define CUBALC_LAW_ENERGY_FLOW      12
#define CUBALC_LAW_COUNT            13

/* law ids: snake tokens for JSON only */
static const char *const CUBALC_LAW_NAME[CUBALC_LAW_COUNT] = {
  "sot", "in_out", "core_io", "bin_talk", "smx_key",
  "hold_flash", "no_bci", "share_smx", "dev_free", "one_cmd",
  "manifest_smx", "algocube", "energy_flow"
};

/* Resolved algocube blueprint genome (deep-opt champion — The Cube watches) */
#define CUBALC_ALGO_GENOME_LEN 32
static const unsigned char CUBALC_ALGO_GENOME_RESOLVED[CUBALC_ALGO_GENOME_LEN] = {
  4, 0, 6, 3, 9, 3, 0, 9, 4, 1, 6, 8, 4, 7, 8, 6,
  7, 0, 8, 7, 6, 9, 4, 3, 5, 5, 2, 0, 2, 7, 4, 1
};

static const char *const CUBALC_DIGIT_TAG[10] = {
  "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9"
};

#endif
