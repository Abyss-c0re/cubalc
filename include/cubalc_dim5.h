/* CubalC 5D event space — modular plane (not lang monolith).
 * Axes: x,y,z (space) · t (time) · s (dimension seed).
 * Law: cube is SoT · pure science · free energy · no bloat.
 */
#ifndef CUBALC_DIM5_H
#define CUBALC_DIM5_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 5D event: first 3 spatial, 4th time, 5th dimension seed */
typedef struct cubalc_event5 {
  double x, y, z; /* metres (lab frame) */
  double t;       /* seconds (coordinate time) */
  uint64_t seed;  /* dimension seed — worldline / IC / basis key */
} cubalc_event5;

/* Pack / unpack seed from spatial+time hash (deterministic fold) */
uint64_t cubalc_dim5_fold_seed(const cubalc_event5 *e);
void cubalc_dim5_from_seed(uint64_t seed, cubalc_event5 *out);

/* Splitmix64 PRNG driven by dimension seed */
uint64_t cubalc_dim5_splitmix64(uint64_t *state);

/* Map seed → unit interval [0,1) */
double cubalc_dim5_unit(uint64_t *state);

/* Write JSON plate for SoT / hive */
int cubalc_dim5_write_json(const cubalc_event5 *e, FILE *out);
int cubalc_dim5_write_json_path(const cubalc_event5 *e, const char *path);

/* Fold 5D → 64-bit State Matrix word (science → lattice) */
uint64_t cubalc_dim5_to_matrix_word(const cubalc_event5 *e);

/* Seed mesh: N events from root seed (splitmix walk) → lattice plate.
 * mesh[i] = from_seed(splitmix^i(root)); edges = fold XOR distance.
 * Usability: hive 5D neighborhood without thrash — E_5d_seed_mesh. */
#define CUBALC_DIM5_MESH_MAX 16
typedef struct cubalc_dim5_mesh {
  int n;
  uint64_t root;
  cubalc_event5 node[CUBALC_DIM5_MESH_MAX];
  uint64_t word[CUBALC_DIM5_MESH_MAX];
  uint64_t mesh_word; /* fold of all node words */
  /* E_5d_seed_mesh deepen: spatial stats (metres / seconds) */
  double span_xyz;   /* max pairwise Euclidean in xyz */
  double mean_dt;    /* mean |t_i - t_0| */
  int edge_n;        /* ring + star + chord count */
  /* E_5d residual: barycenter + RMS radius of xyz cloud */
  double cx, cy, cz; /* centroid metres */
  double rms_r;      /* sqrt(mean ||r - c||^2) */
} cubalc_dim5_mesh;

/* Spatial distance (xyz Euclidean). time_w: weight on |dt| (0 = pure space). */
double cubalc_dim5_event_distance(const cubalc_event5 *a, const cubalc_event5 *b,
                                  double time_w);

/* Nearest mesh node to query by event_distance; returns index or -1. */
int cubalc_dim5_mesh_nearest(const cubalc_dim5_mesh *m, const cubalc_event5 *q,
                             double time_w, double *out_dist);

int cubalc_dim5_mesh_build(uint64_t root, int n, cubalc_dim5_mesh *out);
int cubalc_dim5_mesh_write_json(const cubalc_dim5_mesh *m, FILE *out);
int cubalc_dim5_mesh_write_json_path(const cubalc_dim5_mesh *m, const char *path);

#ifdef __cplusplus
}
#endif
#endif /* CUBALC_DIM5_H */
