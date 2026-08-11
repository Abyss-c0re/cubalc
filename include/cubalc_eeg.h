/* CubalC EEG → State Matrix plane.
 * Law: brain samples are not prose — they fold into the atom SoT (64 bits).
 * Multi-channel window → quantized feature bits → cubalc_matrix.
 * Real-time path: CSV / NDJSON / raw f32 lines from device bridge.
 * HOLD_FLASH · Binary talk · matrix is the soul.
 */
#ifndef CUBALC_EEG_H
#define CUBALC_EEG_H
#include "cubalc.h"
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CUBALC_EEG_MAX_CH   16
#define CUBALC_EEG_MAX_WIN  256
#define CUBALC_EEG_DEF_CH   8
#define CUBALC_EEG_DEF_SCALE 50.0f   /* µV scale for band thresholds */

typedef struct cubalc_eeg_frame {
  int n_ch;       /* channels in this frame (1..16) */
  int n_samp;     /* samples per channel (1..256) */
  float scale_uv; /* magnitude scale (default 50 µV) */
  float samples[CUBALC_EEG_MAX_CH * CUBALC_EEG_MAX_WIN]; /* row-major: ch major */
  long  seq;      /* frame sequence if known */
  long  t_ms;     /* host mono ms stamp if known */
} cubalc_eeg_frame;

/* Clear / configure frame */
void cubalc_eeg_frame_clear(cubalc_eeg_frame *f);
void cubalc_eeg_frame_init(cubalc_eeg_frame *f, int n_ch, float scale_uv);

/* Set sample at (ch, samp). Returns 0 ok. */
int  cubalc_eeg_frame_set(cubalc_eeg_frame *f, int ch, int samp, float uv);

/* Pack frame → 64-bit State Matrix.
 * Layout: 8 bits × min(8,n_ch) channels (or 4 bits × 16 if n_ch>8).
 * Per channel: sign, energy bands, zero-cross, rise, clip, flat.
 * Returns 0 ok; fills out->n = CUBALC_ATOM_BITS.
 */
int cubalc_eeg_pack_matrix(const cubalc_eeg_frame *f, cubalc_matrix *out);

/* Pack contiguous sample array (n_ch channels, n_samp each, interleaved by sample).
 * Layout samples[i*n_ch + c] = channel c at sample i.
 */
int cubalc_eeg_pack_samples(const float *samples, int n_ch, int n_samp,
                            float scale_uv, cubalc_matrix *out);

/* Pack one CSV line of channel values (comma or whitespace separated).
 * Single sample across channels → synthetic 1-sample frame.
 */
int cubalc_eeg_pack_csv_line(const char *line, int n_ch, float scale_uv,
                             cubalc_matrix *out);

/* Matrix → compact "01…" string for FOLDBITS / plates (out must hold ATOM_BITS+1). */
void cubalc_eeg_matrix_bits(const cubalc_matrix *m, char *out, int out_max);

/* Synthetic demo frame (alpha-ish multi-channel) for proof without hardware. */
void cubalc_eeg_demo_frame(cubalc_eeg_frame *f, int n_ch, long seed);

/* Stream: parse one frame from open FILE.
 * Formats:
 *   CSV: ch0,ch1,...  (one sample line; call repeatedly to fill window)
 *   NDJSON: {"ch":[...],"t":ms} or {"samples":[[ch0...],[ch1...]]}
 * Returns 0 ok, -1 EOF/error, 1 incomplete.
 */
int cubalc_eeg_read_csv_sample(FILE *fp, float *ch_out, int n_ch);

/* Accumulate samples into frame window; when full (or force), pack matrix.
 * Returns 1 if packed (matrix ready), 0 if still buffering, -1 error.
 */
int cubalc_eeg_window_push(cubalc_eeg_frame *f, const float *ch, int n_ch,
                           int win_target, int force, cubalc_matrix *out);

/* JSON status line for observers */
int cubalc_eeg_status_json(const cubalc_matrix *m, int n_ch, long seq,
                           const char *backend, FILE *out);

#ifdef __cplusplus
}
#endif
#endif
