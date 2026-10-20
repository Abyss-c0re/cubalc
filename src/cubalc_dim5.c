/* CubalC 5D event space implementation — modular, O(1) hot path. */
#include "cubalc_dim5.h"
#include <string.h>
#include <math.h>
#include <time.h>

uint64_t cubalc_dim5_splitmix64(uint64_t *state) {
  uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

double cubalc_dim5_unit(uint64_t *state) {
  return (cubalc_dim5_splitmix64(state) >> 11) * (1.0 / 9007199254740992.0);
}

static uint64_t hash_mix_double(uint64_t h, double v) {
  uint64_t bits;
  memcpy(&bits, &v, sizeof bits);
  h ^= bits + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
  return h;
}

uint64_t cubalc_dim5_fold_seed(const cubalc_event5 *e) {
  uint64_t h = 0xC0B35D5EEDULL ^ e->seed;
  h = hash_mix_double(h, e->x);
  h = hash_mix_double(h, e->y);
  h = hash_mix_double(h, e->z);
  h = hash_mix_double(h, e->t);
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdULL;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53ULL;
  h ^= h >> 33;
  return h;
}

void cubalc_dim5_from_seed(uint64_t seed, cubalc_event5 *out) {
  uint64_t s = seed ? seed : ((uint64_t)time(NULL) * 0x9E3779B97F4A7C15ULL);
  out->seed = s;
  out->x = (cubalc_dim5_unit(&s) - 0.5) * 20.0;
  out->y = (cubalc_dim5_unit(&s) - 0.5) * 20.0;
  out->z = (cubalc_dim5_unit(&s) - 0.5) * 20.0;
  out->t = cubalc_dim5_unit(&s) * 10.0;
}

uint64_t cubalc_dim5_to_matrix_word(const cubalc_event5 *e) {
  return cubalc_dim5_fold_seed(e);
}

int cubalc_dim5_write_json(const cubalc_event5 *e, FILE *out) {
  if (!e || !out) return -1;
  return fprintf(out,
                 "{\"schema\":\"cubalc.dim5.v1\",\"ok\":true,"
                 "\"axes\":[\"x\",\"y\",\"z\",\"t\",\"seed\"],"
                 "\"x\":%.17g,\"y\":%.17g,\"z\":%.17g,\"t\":%.17g,"
                 "\"seed\":\"%016llx\",\"matrix_word\":\"%016llx\","
                 "\"law\":\"5d_xyz_t_seed;cube_is_sot\"}\n",
                 e->x, e->y, e->z, e->t,
                 (unsigned long long)e->seed,
                 (unsigned long long)cubalc_dim5_to_matrix_word(e));
}

int cubalc_dim5_write_json_path(const cubalc_event5 *e, const char *path) {
  FILE *f = fopen(path, "w");
  if (!f) return -1;
  int rc = cubalc_dim5_write_json(e, f);
  fclose(f);
  return rc < 0 ? -1 : 0;
}

double cubalc_dim5_event_distance(const cubalc_event5 *a, const cubalc_event5 *b,
                                  double time_w) {
  double dx, dy, dz, dt;
  if (!a || !b) return -1.0;
  dx = a->x - b->x;
  dy = a->y - b->y;
  dz = a->z - b->z;
  dt = a->t - b->t;
  if (time_w < 0.0) time_w = 0.0;
  return sqrt(dx * dx + dy * dy + dz * dz + time_w * dt * dt);
}

int cubalc_dim5_mesh_nearest(const cubalc_dim5_mesh *m, const cubalc_event5 *q,
                             double time_w, double *out_dist) {
  int i, best = -1;
  double best_d = 0.0;
  if (!m || !q || m->n < 1) return -1;
  for (i = 0; i < m->n; i++) {
    double d = cubalc_dim5_event_distance(q, &m->node[i], time_w);
    if (best < 0 || d < best_d) {
      best = i;
      best_d = d;
    }
  }
  if (out_dist) *out_dist = best_d;
  return best;
}

int cubalc_dim5_mesh_build(uint64_t root, int n, cubalc_dim5_mesh *out) {
  uint64_t s;
  int i, j;
  if (!out) return -1;
  if (n < 1) n = 1;
  if (n > CUBALC_DIM5_MESH_MAX) n = CUBALC_DIM5_MESH_MAX;
  memset(out, 0, sizeof *out);
  out->n = n;
  out->root = root ? root : 1ULL;
  s = out->root;
  out->mesh_word = 0x5D5EED5EEDULL ^ out->root;
  for (i = 0; i < n; i++) {
    uint64_t node_seed = (i == 0) ? out->root : cubalc_dim5_splitmix64(&s);
    cubalc_dim5_from_seed(node_seed, &out->node[i]);
    out->word[i] = cubalc_dim5_to_matrix_word(&out->node[i]);
    out->mesh_word ^= out->word[i] + 0x9E3779B97F4A7C15ULL + (out->mesh_word << 6) +
                      (out->mesh_word >> 2);
  }
  /* stats: span_xyz diameter, mean |dt| vs node0, edge_n = ring+star+chord */
  out->span_xyz = 0.0;
  out->mean_dt = 0.0;
  out->cx = out->cy = out->cz = 0.0;
  out->rms_r = 0.0;
  for (i = 0; i < n; i++) {
    double dti = out->node[i].t - out->node[0].t;
    if (dti < 0) dti = -dti;
    out->mean_dt += dti;
    out->cx += out->node[i].x;
    out->cy += out->node[i].y;
    out->cz += out->node[i].z;
    for (j = i + 1; j < n; j++) {
      double d = cubalc_dim5_event_distance(&out->node[i], &out->node[j], 0.0);
      if (d > out->span_xyz) out->span_xyz = d;
    }
  }
  if (n > 0) {
    double inv = 1.0 / (double)n;
    out->mean_dt *= inv;
    out->cx *= inv;
    out->cy *= inv;
    out->cz *= inv;
    for (i = 0; i < n; i++) {
      double dx = out->node[i].x - out->cx;
      double dy = out->node[i].y - out->cy;
      double dz = out->node[i].z - out->cz;
      out->rms_r += dx * dx + dy * dy + dz * dz;
    }
    out->rms_r = sqrt(out->rms_r * inv);
  }
  /* ring n + star (n-1 if n>2 else 0) + chord n (i→i+2) if n>2 */
  out->edge_n = n;
  if (n > 2) out->edge_n += (n - 1) + n; /* star + chord */
  else if (n == 2) out->edge_n += 0; /* ring only (2 edges counted as n=2) */
  return 0;
}

int cubalc_dim5_mesh_write_json(const cubalc_dim5_mesh *m, FILE *out) {
  int i;
  if (!m || !out) return -1;
  fprintf(out,
          "{\"schema\":\"cubalc.dim5_mesh.v1\",\"ok\":true,"
          "\"n\":%d,\"root\":\"%016llx\",\"mesh_word\":\"%016llx\","
          "\"span_xyz\":%.17g,\"mean_dt\":%.17g,\"edge_n\":%d,"
          "\"centroid\":{\"x\":%.17g,\"y\":%.17g,\"z\":%.17g},"
          "\"rms_r\":%.17g,"
          "\"law\":\"5d_seed_mesh;splitmix_walk;ring_star_chord;centroid_rms;cube_is_sot\","
          "\"nodes\":[",
          m->n, (unsigned long long)m->root, (unsigned long long)m->mesh_word,
          m->span_xyz, m->mean_dt, m->edge_n, m->cx, m->cy, m->cz, m->rms_r);
  for (i = 0; i < m->n; i++) {
    const cubalc_event5 *e = &m->node[i];
    if (i) fputc(',', out);
    fprintf(out,
            "{\"i\":%d,\"x\":%.17g,\"y\":%.17g,\"z\":%.17g,\"t\":%.17g,"
            "\"seed\":\"%016llx\",\"word\":\"%016llx\"}",
            i, e->x, e->y, e->z, e->t, (unsigned long long)e->seed,
            (unsigned long long)m->word[i]);
  }
  fputs("],\"edges\":[", out);
  /* ring + star + chord (i→i+2) — E_5d_seed_mesh deepen */
  for (i = 0; i < m->n; i++) {
    int j = (i + 1) % m->n;
    uint64_t d = m->word[i] ^ m->word[j];
    if (i) fputc(',', out);
    fprintf(out, "{\"a\":%d,\"b\":%d,\"xor\":\"%016llx\",\"kind\":\"ring\"}",
            i, j, (unsigned long long)d);
  }
  if (m->n > 2) {
    for (i = 1; i < m->n; i++) {
      uint64_t d = m->word[0] ^ m->word[i];
      fprintf(out,
              ",{\"a\":0,\"b\":%d,\"xor\":\"%016llx\",\"star\":1,"
              "\"kind\":\"star\"}",
              i, (unsigned long long)d);
    }
    for (i = 0; i < m->n; i++) {
      int j = (i + 2) % m->n;
      uint64_t d = m->word[i] ^ m->word[j];
      double spa = cubalc_dim5_event_distance(&m->node[i], &m->node[j], 0.0);
      fprintf(out,
              ",{\"a\":%d,\"b\":%d,\"xor\":\"%016llx\",\"kind\":\"chord\","
              "\"span\":%.17g}",
              i, j, (unsigned long long)d, spa);
    }
  }
  fputs("]}\n", out);
  return 0;
}

int cubalc_dim5_mesh_write_json_path(const cubalc_dim5_mesh *m, const char *path) {
  FILE *f = fopen(path, "w");
  if (!f) return -1;
  int rc = cubalc_dim5_mesh_write_json(m, f);
  fclose(f);
  return rc < 0 ? -1 : 0;
}
