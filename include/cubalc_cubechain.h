/* CubeChain — CBLC append-only matrix blocks. */
#ifndef CUBALC_CUBECHAIN_H
#define CUBALC_CUBECHAIN_H
#include "cubalc.h"
#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CUBALC_CCHN_MAGIC   0x4E484343u  /* 'CCHN' little: N H C C */
#define CUBALC_CCHN_PROTO   1
#define CUBALC_HASH_LEN     32

/* One CubeChain block — stores Matrix State (+ optional binary talk payload). */
typedef struct cubalc_cchn_block {
  uint32_t magic;
  uint16_t proto;
  uint16_t flags;       /* bit0 hold_flash, bit1 genesis, bit2 talk, bit3 impulse */
  uint32_t seq;
  uint8_t  prev_hash[CUBALC_HASH_LEN];
  uint8_t  matrix_hash[CUBALC_HASH_LEN]; /* hash of matrix bits */
  char     cube_id[CUBALC_ID_LEN];
  uint8_t  proton;
  uint8_t  digit;
  uint16_t set;
  uint16_t n_bits;
  uint8_t  energy_q8;   /* energy * 255 */
  uint8_t  reserved;
  uint32_t payload_len; /* optional CBLC frame after matrix bytes */
  /* followed by: matrix bytes ((n_bits+7)/8) then payload */
} cubalc_cchn_block;

typedef struct cubalc_cchn {
  char     path[512];
  uint32_t tip_seq;
  uint8_t  tip_hash[CUBALC_HASH_LEN];
  uint8_t  open;
  uint8_t  hold_flash;
} cubalc_cchn;

/* Hash helper (BLAKE-ish simple cube hash — C-speed, no deps). */
void cubalc_cube_hash(const void *data, size_t n, uint8_t out[CUBALC_HASH_LEN]);

/* Open or create store under dir (default CUBALC_STATE or ./state). */
int  cubalc_cchn_open(cubalc_cchn *cc, const char *dir_or_null);
void cubalc_cchn_close(cubalc_cchn *cc);

/* Append matrix state of one cube (language store write). */
int  cubalc_cchn_append_cube(cubalc_cchn *cc, const cubalc_cube *cube,
                             uint16_t flags, const uint8_t *payload, uint32_t plen);

/* Append full chain snapshot (genesis / tick). */
int  cubalc_cchn_append_chain(cubalc_cchn *cc, const cubalc_chain *ch, uint16_t flags);

/* Verify whole file: prev_hash links + recompute matrix hashes. */
int  cubalc_cchn_verify(const char *path, char *err, size_t err_n);

/* Tip JSON for machines / hive. */
int  cubalc_cchn_tip_json(const cubalc_cchn *cc, FILE *out);

/* Human one-liner: blocks count + tip seq. */
int  cubalc_cchn_stat(const char *path, uint32_t *n_blocks, uint32_t *tip_seq);

#ifdef __cplusplus
}
#endif
#endif
