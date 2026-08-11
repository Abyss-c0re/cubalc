/* CubalC raw C API — State Matrix visualization.
 * Law: matrix is SoT · bits are the picture · no decorative vacuum.
 * Real-time: producers fill frames; humans observe via JSON/bin/RGBA.
 * HOLD_FLASH · Binary talk · stream-friendly fixed layout.
 */
#ifndef CUBALC_VIZ_MATRIX_H
#define CUBALC_VIZ_MATRIX_H
#include "cubalc.h"
#include <stdint.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CUBALC_VIZ_MAGIC     0x4D5A5643u  /* 'CVZM' little-endian string */
#define CUBALC_VIZ_VERSION   1
#define CUBALC_VIZ_BITS      CUBALC_ATOM_BITS
#define CUBALC_VIZ_RING      64
#define CUBALC_VIZ_SRC_MAX   32
#define CUBALC_VIZ_BACK_MAX  32

/* Per-bit cell for UI heatmaps (channel/feature map when EEG-packed) */
typedef struct cubalc_viz_cell {
  uint8_t on;    /* 0/1 */
  uint8_t ch;    /* channel index (0..15) when mapped */
  uint8_t feat;  /* feature bit within channel */
  uint8_t pad;
  float   heat;  /* 0..1 activity / persistence */
} cubalc_viz_cell;

/* One visualization frame — fixed layout, streamable */
typedef struct cubalc_viz_frame {
  uint32_t magic;     /* CUBALC_VIZ_MAGIC */
  uint16_t version;   /* CUBALC_VIZ_VERSION */
  uint16_t n_bits;    /* usually 64 */
  uint16_t n_ch;      /* 0 = unmapped; 8 or 16 for EEG */
  uint16_t digit;     /* algocube digit 0..9 */
  uint32_t set;       /* popcount */
  uint32_t seq;
  int64_t  t_ms;      /* host mono ms */
  float    unity;     /* 0..1 fleet/window unity if known */
  char     source[CUBALC_VIZ_SRC_MAX];   /* eeg|harmony|chain|manual */
  char     backend[CUBALC_VIZ_BACK_MAX]; /* cpu:N[+gpu] */
  uint8_t  bits[CUBALC_VIZ_BITS];        /* dense 0/1 */
  cubalc_viz_cell cells[CUBALC_VIZ_BITS];
  uint64_t word;      /* packed little-endian atom */
} cubalc_viz_frame;

/* ---- fill ---- */
void cubalc_viz_frame_clear(cubalc_viz_frame *f);

/* Map State Matrix → viz frame. n_ch<=0 → no channel map (linear bits).
 * For EEG: n_ch 1..8 → 8 feats/ch; 9..16 → 4 feats/ch. */
int cubalc_viz_from_matrix(const cubalc_matrix *m, cubalc_viz_frame *out,
                           uint32_t seq, float unity, int digit, int n_ch,
                           const char *source, const char *backend);

/* Apply heat trail: blend with previous frame (persist on bits that stay on). */
void cubalc_viz_heat_blend(cubalc_viz_frame *cur, const cubalc_viz_frame *prev,
                           float decay);

/* ---- wire (raw binary, fixed sizeof frame) ---- */
int cubalc_viz_write_bin(const cubalc_viz_frame *f, FILE *out);
int cubalc_viz_read_bin(cubalc_viz_frame *f, FILE *in);
int cubalc_viz_write_bin_path(const cubalc_viz_frame *f, const char *path);
int cubalc_viz_read_bin_path(cubalc_viz_frame *f, const char *path);

/* ---- JSON (human UIs / HTTP) ---- */
int cubalc_viz_write_json(const cubalc_viz_frame *f, FILE *out);
int cubalc_viz_write_json_buf(const cubalc_viz_frame *f, char *buf, size_t cap);
int cubalc_viz_write_json_path(const cubalc_viz_frame *f, const char *path);

/* ---- ring for real-time history / sparklines ---- */
typedef struct cubalc_viz_ring {
  cubalc_viz_frame frames[CUBALC_VIZ_RING];
  int head; /* next write */
  int n;
} cubalc_viz_ring;

void cubalc_viz_ring_init(cubalc_viz_ring *r);
void cubalc_viz_ring_push(cubalc_viz_ring *r, const cubalc_viz_frame *f);
/* newest = index 0; returns 0 ok */
int  cubalc_viz_ring_get(const cubalc_viz_ring *r, int age, cubalc_viz_frame *out);
int  cubalc_viz_ring_latest(const cubalc_viz_ring *r, cubalc_viz_frame *out);
/* JSON array of recent frames (oldest→newest), max `limit` (0=all) */
int  cubalc_viz_ring_write_json(const cubalc_viz_ring *r, int limit, FILE *out);
int  cubalc_viz_ring_write_json_path(const cubalc_viz_ring *r, int limit,
                                    const char *path);

/* ---- RGBA8 thumbnail for canvas/GPU texture ----
 * grid_w*grid_h should be >= n_bits (e.g. 8x8). RGBA interleaved.
 * pitch = row stride bytes (0 → grid_w*4). */
int cubalc_viz_rgba8(const cubalc_viz_frame *f, int grid_w, int grid_h,
                     uint8_t *rgba, int pitch);

#ifdef __cplusplus
}
#endif
#endif
