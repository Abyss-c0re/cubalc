#ifndef CUBALC_H
#define CUBALC_H
#include "cubalc_law.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Packed bit matrix — SoT for one Cube */
typedef struct cubalc_matrix {
  uint16_t n;     /* bit count (≤ CUBALC_ATOM_BITS) */
  uint16_t set;
  uint8_t  bits[(CUBALC_ATOM_BITS + 7) / 8];
} cubalc_matrix;

/* Nanobot atom core inside every Cube — State Matrix is the key */
typedef struct cubalc_atom {
  char     id[CUBALC_ID_LEN];
  uint8_t  proton;      /* 1 create · 0 destroy */
  uint8_t  alive;
  uint8_t  digit;       /* algocube 0–9 */
  uint8_t  digit_lock;  /* 1 = peer SETDIGIT sticky; tick must not recompute */
  float    unity;
  float    energy;      /* 0..1 — flows through binary CBLC talk */
  cubalc_matrix matrix; /* State Matrix SoT — machine key, cube soul */
} cubalc_atom;

/* Plug port — Cube Declaration: IN receive · OUT emit; chain only wires */
typedef struct cubalc_port {
  uint8_t  open;
  uint8_t  dir;         /* CUBALC_PORT_IN | CUBALC_PORT_OUT */
  int16_t  peer;        /* index in chain or -1 */
  uint8_t  face;        /* 0+X 1-X 2+Y 3-Y 4+Z 5-Z */
  cubalc_matrix gate;   /* required mask for plug */
} cubalc_port;

/* Visible Cube object — only CUBE is defined (COP). Nestable; compiles to matrix. */
typedef struct cubalc_cube {
  char     id[CUBALC_ID_LEN];
  char     label[CUBALC_ID_LEN];
  char     role[24];    /* os aspect: kernel|infer|headset|stream|depth|coord|host|… */
  float    x, y, z, s;
  float    yaw, pitch, roll; /* degrees — spin of the cube (Kernel Matrix face) */
  uint8_t  r, g, b, a;
  cubalc_atom atom;
  cubalc_port ports[CUBALC_MAX_PORTS];
  int      n_ports;
  uint8_t  plugged;     /* count of live plugs */
  /* Nest: cubes may nest (parent holds children). -1 = root. */
  int16_t  parent;
  /* Law flow_compile: energy must flow before compile into matrix. */
  uint8_t  flowed;      /* 1 if energy talk/flow touched this cube */
  uint8_t  compiled;    /* 1 if successfully compiled to matrix after flow */
  cubalc_matrix compiled_matrix; /* materialised matrix (SoT snapshot after compile) */
} cubalc_cube;

/* Cube Chain — wired cubes from Initial State Matrix */
typedef struct cubalc_chain {
  cubalc_matrix initial; /* genesis matrix — beginning of chain */
  cubalc_cube cubes[CUBALC_MAX_CUBES];
  int n_cubes;
  uint32_t seq;
  float unity;
  uint8_t hold_flash;
  char creed[80];
  char status[160];
} cubalc_chain;

/* Binary transfer frame (cube talk) */
typedef struct cubalc_bin_hdr {
  uint32_t magic;
  uint16_t proto;
  uint16_t n_bits;
  uint16_t set;
  uint8_t  proton;
  uint8_t  digit;
  uint32_t seq;
  char     from_id[CUBALC_ID_LEN];
  char     to_id[CUBALC_ID_LEN];
} cubalc_bin_hdr;

/* matrix ops */
void cubalc_matrix_clear(cubalc_matrix *m);
void cubalc_matrix_from_ascii(cubalc_matrix *m, const char *bits01, int n);
int  cubalc_matrix_get(const cubalc_matrix *m, int i);
void cubalc_matrix_set(cubalc_matrix *m, int i, int on);
int  cubalc_matrix_popcount(const cubalc_matrix *m);
int  cubalc_matrix_hamming(const cubalc_matrix *a, const cubalc_matrix *b);
float cubalc_matrix_compat(const cubalc_matrix *a, const cubalc_matrix *b); /* 0..1 */

/* atom / proton */
void cubalc_atom_init(cubalc_atom *a, const char *id, uint8_t proton);
int  cubalc_atom_impulse(cubalc_atom *a, uint8_t proton); /* create=1 destroy=0 */
/* Algocube law engine — see cubalc_algocube.h */
int  cubalc_algocube_digit(const cubalc_matrix *m);

/* cube COP — only CUBE is defined; I/O is pluggable (and reversible); nestable */
int  cubalc_cube_spawn(cubalc_chain *ch, const char *id, const char *role,
                       uint8_t proton, float x, float y, float z);
int  cubalc_cube_plug(cubalc_chain *ch, int a, int b); /* plug if matrices compatible */
int  cubalc_cube_unplug(cubalc_chain *ch, int a, int b);
/* Reverse pluggable I/O between two cubes (swap port dirs on the wire). */
int  cubalc_cube_reverse(cubalc_chain *ch, int a, int b);
/* Set one free port direction on a cube: face 0..5, dir IN|OUT. */
int  cubalc_cube_io(cubalc_chain *ch, int cube, int face, uint8_t dir);
/* Nest child inside parent (cubes may nest). Returns 0 ok; -1 args; -2 cycle/depth. */
int  cubalc_cube_nest(cubalc_chain *ch, int parent, int child);
/* Detach child from its parent (child becomes root). */
int  cubalc_cube_unnest(cubalc_chain *ch, int child);
/* Mark cube as having received energy flow (also set automatically on talk). */
void cubalc_cube_mark_flow(cubalc_chain *ch, int cube);
/* Law: each cube compiles into a matrix. No flow → no compile.
 * Folds atom (+ nested children if any) into compiled_matrix and atom.matrix.
 * Returns 0 ok; -1 bad; -2 no flow; -3 child not compiled; -4 nest depth. */
int  cubalc_cube_compile(cubalc_chain *ch, int cube);
/* Compile all cubes leaves-first. *failed_ix = first failure index or -1. */
int  cubalc_chain_compile(cubalc_chain *ch, int *failed_ix);
/* Queries */
int  cubalc_cube_has_flow(const cubalc_chain *ch, int cube);
int  cubalc_cube_is_compiled(const cubalc_chain *ch, int cube);
int  cubalc_cube_parent(const cubalc_chain *ch, int cube); /* -1 root / missing */
/* Directed energy talk only from → to (one-way I/O). */
int  cubalc_cube_talk(cubalc_chain *ch, int from, int to); /* binary matrix transfer (legacy v1) */
/* secure talk is cubalc_cube_talk_secure() in cubalc_smx.h */
/* One-way flow hop along OUT→IN only (respects port direction). */
int  cubalc_chain_flow_directed(cubalc_chain *ch);

/* binary pack/unpack */
int  cubalc_bin_pack(const cubalc_atom *atom, const char *from, const char *to,
                     uint32_t seq, uint8_t *out, size_t cap, size_t *n_out);
int  cubalc_bin_unpack(const uint8_t *in, size_t n, cubalc_atom *atom_out,
                       char *from, char *to, uint32_t *seq);

/* chain / genesis */
void cubalc_chain_init(cubalc_chain *ch);
int  cubalc_chain_from_initial(cubalc_chain *ch, const cubalc_matrix *genesis, uint32_t seq);
int  cubalc_chain_os_aspects(cubalc_chain *ch); /* spawn OS viz cubes from live probes */
int  cubalc_chain_tick(cubalc_chain *ch);
/* Energy flow: one binary talk hop along plugs (create pushes energy, destroy drains). */
int  cubalc_chain_flow(cubalc_chain *ch);
int  cubalc_chain_write_viz(const cubalc_chain *ch, const char *path);
int  cubalc_chain_write_json(const cubalc_chain *ch, const char *path);
/* cells.bin lattice projection — same matrix SoT as viz JSON */
int  cubalc_chain_write_cells(const cubalc_chain *ch, const char *path);
/* One SoT → all visual faces (viz JSON + cells.bin + unity plate).
 * devices free · share state_matrix only · core decides I/O · no brain wires */
int  cubalc_chain_publish_united(const cubalc_chain *ch);
int  cubalc_chain_impulse(cubalc_chain *ch, const char *cube_id, uint8_t proton);
/* Human cube board (ASCII) — machine-parseable JSON via write_json */
int  cubalc_chain_print_cubes(const cubalc_chain *ch, FILE *out);
/* alias kept for old scripts */
int  cubalc_chain_print_lego(const cubalc_chain *ch, FILE *out);
/* Spin / pose — Hello Cube & Kernel Matrix viz */
int  cubalc_cube_spin(cubalc_chain *ch, const char *cube_id,
                      float dyaw, float dpitch, float droll);
int  cubalc_cube_pose(cubalc_chain *ch, const char *cube_id,
                      float x, float y, float z, float s /* s<=0 keep */);
/* ASCII frame: spinning cube whose faces show Kernel / State Matrix bits */
int  cubalc_cube_print_spin(const cubalc_chain *ch, const char *cube_id, FILE *out);

/* NEXUS_COORD fold into genesis bit pattern */
int  cubalc_coord_to_matrix(const char *plate_line, cubalc_matrix *out);
/* Law accordance: 0 = all laws hold for this chain / build */
int  cubalc_law_check(const cubalc_chain *ch, char *err, size_t errn);
int  cubalc_law_manifest_json(const cubalc_chain *ch, FILE *out);

#ifdef __cplusplus
}
#endif
#endif
