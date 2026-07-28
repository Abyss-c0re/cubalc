#define _POSIX_C_SOURCE 200809L
#include "cubalc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

void cubalc_matrix_clear(cubalc_matrix *m) {
  if (!m) return;
  memset(m, 0, sizeof(*m));
}

void cubalc_matrix_from_ascii(cubalc_matrix *m, const char *bits01, int n) {
  cubalc_matrix_clear(m);
  if (!bits01 || n <= 0) return;
  if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
  m->n = (uint16_t)n;
  for (int i = 0; i < n; i++) {
    if (bits01[i] == '1') {
      m->bits[i >> 3] |= (uint8_t)(1u << (i & 7));
      m->set++;
    }
  }
}

int cubalc_matrix_get(const cubalc_matrix *m, int i) {
  if (!m || i < 0 || i >= m->n) return 0;
  return (m->bits[i >> 3] >> (i & 7)) & 1;
}

void cubalc_matrix_set(cubalc_matrix *m, int i, int on) {
  if (!m || i < 0 || i >= CUBALC_ATOM_BITS) return;
  if (i >= m->n) m->n = (uint16_t)(i + 1);
  int was = cubalc_matrix_get(m, i);
  if (on) m->bits[i >> 3] |= (uint8_t)(1u << (i & 7));
  else    m->bits[i >> 3] &= (uint8_t)~(1u << (i & 7));
  if (on && !was) m->set++;
  else if (!on && was && m->set) m->set--;
}

int cubalc_matrix_popcount(const cubalc_matrix *m) {
  return m ? m->set : 0;
}

int cubalc_matrix_hamming(const cubalc_matrix *a, const cubalc_matrix *b) {
  if (!a || !b) return CUBALC_ATOM_BITS;
  int n = a->n > b->n ? a->n : b->n;
  if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
  int h = 0;
  for (int i = 0; i < n; i++)
    if (cubalc_matrix_get(a, i) != cubalc_matrix_get(b, i)) h++;
  return h;
}

float cubalc_matrix_compat(const cubalc_matrix *a, const cubalc_matrix *b) {
  if (!a || !b || a->n == 0 || b->n == 0) return 0.f;
  int n = a->n < b->n ? a->n : b->n;
  if (n <= 0) return 0.f;
  int same = 0, both = 0;
  for (int i = 0; i < n; i++) {
    int x = cubalc_matrix_get(a, i), y = cubalc_matrix_get(b, i);
    if (x == y) same++;
    if (x || y) both++;
  }
  /* compatibility: shared pattern density */
  float base = (float)same / (float)n;
  float bond = both ? (float)(n - cubalc_matrix_hamming(a, b)) / (float)n : base;
  return base * 0.5f + bond * 0.5f;
}

int cubalc_algocube_digit(const cubalc_matrix *m) {
  if (!m || m->n == 0) return 4;
  uint32_t rng = 0xC0BEA160u, set = m->set;
  for (int i = 0; i < m->n; i++)
    if (cubalc_matrix_get(m, i)) rng ^= (uint32_t)(i + 1) * 0x9E3779B9u;
  rng ^= set * 16777619u;
  rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
  return (int)(rng % 10u);
}

void cubalc_atom_init(cubalc_atom *a, const char *id, uint8_t proton) {
  if (!a) return;
  memset(a, 0, sizeof(*a));
  if (id) {
    strncpy(a->id, id, CUBALC_ID_LEN - 1);
  }
  a->proton = proton ? 1 : 0;
  a->alive = proton ? 1 : 0;
  a->unity = 1.f;
  a->energy = proton ? 0.85f : 0.15f; /* create starts charged; destroy is sink */
  a->matrix.n = CUBALC_ATOM_BITS;
  a->digit = cubalc_algocube_digit(&a->matrix);
}

int cubalc_atom_impulse(cubalc_atom *a, uint8_t proton) {
  if (!a) return -1;
  a->proton = proton ? 1 : 0;
  if (proton) {
    a->alive = 1;
    a->energy = fminf(1.f, a->energy + 0.35f);
    /* creation: set a pulse bit pattern from digit — energy into matrix */
    for (int i = 0; i < 8; i++)
      cubalc_matrix_set(&a->matrix, (a->digit * 3 + i) % CUBALC_ATOM_BITS, 1);
  } else {
    a->energy = fmaxf(0.f, a->energy - 0.40f);
    /* destroy: clear half the matrix (deconstruct edge) */
    for (int i = 0; i < a->matrix.n; i += 2)
      cubalc_matrix_set(&a->matrix, i, 0);
    if (a->matrix.set == 0) a->alive = 0;
  }
  a->digit = cubalc_algocube_digit(&a->matrix);
  return 0;
}

static void role_color(const char *role, uint8_t *r, uint8_t *g, uint8_t *b) {
  *r = 242; *g = 38; *b = 71; /* default crimson */
  if (!role) return;
  if (strstr(role, "kernel") || strstr(role, "sot")) { *r=255; *g=165; *b=46; }
  else if (strstr(role, "llama")) { *r=64; *g=242; *b=115; }
  else if (strstr(role, "quest") || strstr(role, "lizard")) { *r=90; *g=200; *b=255; }
  else if (strstr(role, "wivrn")) { *r=180; *g=120; *b=255; }
  else if (strstr(role, "kinect")) { *r=255; *g=200; *b=80; }
  else if (strstr(role, "host") || strstr(role, "station")) { *r=242; *g=38; *b=71; }
  else if (strstr(role, "coord")) { *r=255; *g=100; *b=160; }
  else if (strstr(role, "destroy") || strstr(role, "decon")) { *r=30; *g=30; *b=36; }
  else if (strstr(role, "create") || strstr(role, "construct")) { *r=255; *g=60; *b=80; }
}

int cubalc_cube_spawn(cubalc_chain *ch, const char *id, const char *role,
                      uint8_t proton, float x, float y, float z) {
  if (!ch || ch->n_cubes >= CUBALC_MAX_CUBES || ch->n_cubes >= CUBALC_BUDGET) return -1;
  cubalc_cube *c = &ch->cubes[ch->n_cubes];
  memset(c, 0, sizeof(*c));
  strncpy(c->id, id ? id : "cube", CUBALC_ID_LEN - 1);
  strncpy(c->label, id ? id : "cube", CUBALC_ID_LEN - 1);
  strncpy(c->role, role ? role : "aspect", sizeof(c->role) - 1);
  c->x = x; c->y = y; c->z = z; c->s = 0.12f;
  role_color(c->role, &c->r, &c->g, &c->b);
  c->a = 220;
  cubalc_atom_init(&c->atom, c->id, proton);
  /* seed matrix from role hash + genesis overlap */
  for (int i = 0; i < 16; i++) {
    int bit = (c->id[i % (int)strlen(c->id)] * 7 + i * 3) % CUBALC_ATOM_BITS;
    cubalc_matrix_set(&c->atom.matrix, bit, 1);
  }
  /* inherit some genesis bits for chain cohesion */
  for (int i = 0; i < ch->initial.n && i < CUBALC_ATOM_BITS; i++)
    if (cubalc_matrix_get(&ch->initial, i) && (i % 3 == 0))
      cubalc_matrix_set(&c->atom.matrix, i, 1);
  c->atom.digit = cubalc_algocube_digit(&c->atom.matrix);
  /* Cube Declaration: every chain cube MUST have IN and OUT */
  c->n_ports = 2;
  c->ports[0].open = 1; c->ports[0].dir = CUBALC_PORT_IN;  c->ports[0].peer = -1; c->ports[0].face = 0;
  c->ports[0].gate = c->atom.matrix;
  c->ports[1].open = 1; c->ports[1].dir = CUBALC_PORT_OUT; c->ports[1].peer = -1; c->ports[1].face = 1;
  c->ports[1].gate = c->atom.matrix;
  /* additional face ports (still cubes, not theater) */
  for (int f = 2; f < 6 && f < CUBALC_MAX_PORTS; f++) {
    c->ports[f].open = 1;
    c->ports[f].dir = (f % 2) ? CUBALC_PORT_OUT : CUBALC_PORT_IN;
    c->ports[f].peer = -1;
    c->ports[f].face = (uint8_t)f;
    c->ports[f].gate = c->atom.matrix;
    c->n_ports++;
  }
  return ch->n_cubes++;
}

int cubalc_cube_plug(cubalc_chain *ch, int a, int b) {
  if (!ch || a < 0 || b < 0 || a >= ch->n_cubes || b >= ch->n_cubes || a == b) return -1;
  float cmp = cubalc_matrix_compat(&ch->cubes[a].atom.matrix, &ch->cubes[b].atom.matrix);
  /* Snap only if matrices compatible — Cube is SoT, not labels */
  if (cmp < 0.35f) return -2; /* incompatible */
  /* Law: wire peer.OUT → self.IN both ways (CubeChain only wires ports) */
  int a_out = -1, b_in = -1, b_out = -1, a_in = -1;
  for (int i = 0; i < ch->cubes[a].n_ports; i++) {
    cubalc_port *p = &ch->cubes[a].ports[i];
    if (!p->open || p->peer >= 0) continue;
    if (p->dir == CUBALC_PORT_OUT && a_out < 0) a_out = i;
    if (p->dir == CUBALC_PORT_IN  && a_in  < 0) a_in  = i;
  }
  for (int i = 0; i < ch->cubes[b].n_ports; i++) {
    cubalc_port *p = &ch->cubes[b].ports[i];
    if (!p->open || p->peer >= 0) continue;
    if (p->dir == CUBALC_PORT_OUT && b_out < 0) b_out = i;
    if (p->dir == CUBALC_PORT_IN  && b_in  < 0) b_in  = i;
  }
  if (a_out < 0 || b_in < 0 || b_out < 0 || a_in < 0) return -3; /* missing IN/OUT */
  ch->cubes[a].ports[a_out].peer = (int16_t)b; /* A.OUT → B */
  ch->cubes[b].ports[b_in].peer  = (int16_t)a; /* B.IN  ← A */
  ch->cubes[b].ports[b_out].peer = (int16_t)a; /* B.OUT → A */
  ch->cubes[a].ports[a_in].peer  = (int16_t)b; /* A.IN  ← B */
  ch->cubes[a].plugged += 2;
  ch->cubes[b].plugged += 2;
  /* binary talk on plug — matrix bits only */
  cubalc_cube_talk(ch, a, b);
  return 0;
}

int cubalc_cube_unplug(cubalc_chain *ch, int a, int b) {
  if (!ch || a < 0 || b < 0) return -1;
  for (int i = 0; i < ch->cubes[a].n_ports; i++)
    if (ch->cubes[a].ports[i].peer == b) {
      ch->cubes[a].ports[i].peer = -1;
      if (ch->cubes[a].plugged) ch->cubes[a].plugged--;
    }
  for (int i = 0; i < ch->cubes[b].n_ports; i++)
    if (ch->cubes[b].ports[i].peer == a) {
      ch->cubes[b].ports[i].peer = -1;
      if (ch->cubes[b].plugged) ch->cubes[b].plugged--;
    }
  return 0;
}

int cubalc_bin_pack(const cubalc_atom *atom, const char *from, const char *to,
                    uint32_t seq, uint8_t *out, size_t cap, size_t *n_out) {
  if (!atom || !out) return -1;
  size_t need = sizeof(cubalc_bin_hdr) + (atom->matrix.n + 7) / 8;
  if (cap < need) return -1;
  cubalc_bin_hdr *h = (cubalc_bin_hdr *)out;
  memset(h, 0, sizeof(*h));
  h->magic = CUBALC_MAGIC_BIN;
  h->proto = CUBALC_PROTO_V1;
  h->n_bits = atom->matrix.n;
  h->set = atom->matrix.set;
  h->proton = atom->proton;
  h->digit = atom->digit;
  h->seq = seq;
  if (from) strncpy(h->from_id, from, CUBALC_ID_LEN - 1);
  if (to) strncpy(h->to_id, to, CUBALC_ID_LEN - 1);
  memcpy(out + sizeof(*h), atom->matrix.bits, (atom->matrix.n + 7) / 8);
  if (n_out) *n_out = need;
  return 0;
}

int cubalc_bin_unpack(const uint8_t *in, size_t n, cubalc_atom *atom_out,
                      char *from, char *to, uint32_t *seq) {
  if (!in || n < sizeof(cubalc_bin_hdr) || !atom_out) return -1;
  const cubalc_bin_hdr *h = (const cubalc_bin_hdr *)in;
  if (h->magic != CUBALC_MAGIC_BIN || h->proto != CUBALC_PROTO_V1) return -2;
  size_t need = sizeof(*h) + (h->n_bits + 7) / 8;
  if (n < need) return -1;
  memset(atom_out, 0, sizeof(*atom_out));
  atom_out->proton = h->proton;
  atom_out->digit = h->digit;
  atom_out->alive = h->proton ? 1 : 0;
  atom_out->matrix.n = h->n_bits > CUBALC_ATOM_BITS ? CUBALC_ATOM_BITS : h->n_bits;
  atom_out->matrix.set = h->set;
  memcpy(atom_out->matrix.bits, in + sizeof(*h), (atom_out->matrix.n + 7) / 8);
  if (from) { strncpy(from, h->from_id, CUBALC_ID_LEN - 1); from[CUBALC_ID_LEN-1]=0; }
  if (to) { strncpy(to, h->to_id, CUBALC_ID_LEN - 1); to[CUBALC_ID_LEN-1]=0; }
  if (seq) *seq = h->seq;
  return 0;
}

int cubalc_cube_talk(cubalc_chain *ch, int from, int to) {
  if (!ch || from < 0 || to < 0 || from >= ch->n_cubes || to >= ch->n_cubes) return -1;
  uint8_t buf[256];
  size_t n = 0;
  cubalc_cube *A = &ch->cubes[from], *B = &ch->cubes[to];
  if (cubalc_bin_pack(&A->atom, A->id, B->id, ch->seq, buf, sizeof buf, &n) != 0) return -1;
  cubalc_atom received;
  char f[CUBALC_ID_LEN], t[CUBALC_ID_LEN];
  uint32_t seq = 0;
  if (cubalc_bin_unpack(buf, n, &received, f, t, &seq) != 0) return -1;
  /* merge received bits into peer (OR for create, AND-clear for destroy frame) */
  if (received.proton) {
    for (int i = 0; i < received.matrix.n && i < CUBALC_ATOM_BITS; i++)
      if (cubalc_matrix_get(&received.matrix, i))
        cubalc_matrix_set(&B->atom.matrix, i, 1);
    B->atom.alive = 1;
  } else {
    for (int i = 0; i < received.matrix.n && i < CUBALC_ATOM_BITS; i++)
      if (cubalc_matrix_get(&received.matrix, i))
        cubalc_matrix_set(&B->atom.matrix, i, 0);
  }
  B->atom.digit = cubalc_algocube_digit(&B->atom.matrix);
  B->atom.unity = cubalc_matrix_compat(&A->atom.matrix, &B->atom.matrix);
  /* ENERGY FLOW along binary talk: create pushes energy; destroy drains peer */
  if (received.proton) {
    float push = A->atom.energy * 0.18f;
    if (push > 0.001f) {
      A->atom.energy = fmaxf(0.f, A->atom.energy - push * 0.5f);
      B->atom.energy = fminf(1.f, B->atom.energy + push);
    }
  } else {
    float drain = 0.12f;
    B->atom.energy = fmaxf(0.f, B->atom.energy - drain);
    A->atom.energy = fminf(1.f, A->atom.energy + drain * 0.25f);
  }
  return 0;
}

void cubalc_chain_init(cubalc_chain *ch) {
  if (!ch) return;
  memset(ch, 0, sizeof(*ch));
  ch->hold_flash = CUBALC_HOLD_FLASH;
  ch->unity = 1.f;
  strncpy(ch->creed, CUBALC_CREED, sizeof(ch->creed) - 1);
  snprintf(ch->status, sizeof ch->status, "empty chain");
}

int cubalc_coord_to_matrix(const char *plate_line, cubalc_matrix *out) {
  if (!out) return -1;
  cubalc_matrix_clear(out);
  out->n = CUBALC_ATOM_BITS;
  if (!plate_line) return -1;
  /* fold plate chars into bits — SoT seed */
  size_t L = strlen(plate_line);
  for (size_t i = 0; i < L; i++) {
    unsigned char c = (unsigned char)plate_line[i];
    int bit = (int)((c * 131u + i * 17u) % CUBALC_ATOM_BITS);
    if (c == '1' || (c & 1)) cubalc_matrix_set(out, bit, 1);
  }
  /* law flags sticky */
  cubalc_matrix_set(out, 11, 1); /* hold_flash slot */
  cubalc_matrix_set(out, 12, 1); /* no_brain_wire */
  cubalc_matrix_set(out, 13, 1); /* matrix_is_key */
  return 0;
}

int cubalc_chain_from_initial(cubalc_chain *ch, const cubalc_matrix *genesis, uint32_t seq) {
  if (!ch || !genesis) return -1;
  cubalc_chain_init(ch);
  ch->initial = *genesis;
  ch->seq = seq;
  ch->hold_flash = 1;
  snprintf(ch->status, sizeof ch->status, "genesis set=%u n=%u seq=%u",
           (unsigned)genesis->set, (unsigned)genesis->n, (unsigned)seq);
  return 0;
}

/* OS aspects as cubes — important planes of the Cube way */
int cubalc_chain_os_aspects(cubalc_chain *ch) {
  if (!ch) return -1;
  /* ring layout — octopus-friendly big studs */
  struct { const char *id; const char *role; uint8_t proton; float ang; float elev; } asp[] = {
    {"cube-sot",     "kernel_sot",   1, 0.f,           1.35f},
    {"cube-coord",   "coord",        1, 0.785f,        1.35f},
    {"cube-llama",   "llama",        1, 1.57f,         1.25f},
    {"cube-host",    "host_station", 1, 2.356f,        1.25f},
    {"cube-quest",   "quest_lizard", 1, 3.141f,        1.20f},
    {"cube-wivrn",   "wivrn_way",    1, 3.927f,        1.20f},
    {"cube-kinect",  "kinect",       1, 4.712f,        1.15f},
    {"cube-create",  "construct",    1, 5.498f,        1.40f},
    {"cube-destroy", "deconstruct",  0, 0.f,           0.95f},
    {"cube-hive",    "nanobot_hive", 1, 1.0f,          1.55f},
  };
  int n = (int)(sizeof(asp) / sizeof(asp[0]));
  float R = 0.85f;
  for (int i = 0; i < n && ch->n_cubes < CUBALC_BUDGET; i++) {
    float x = cosf(asp[i].ang) * R;
    float z = -2.f + sinf(asp[i].ang) * R;
    cubalc_cube_spawn(ch, asp[i].id, asp[i].role, asp[i].proton, x, asp[i].elev, z);
  }
  /* plug adjacent in ring if compatible */
  int base = ch->n_cubes - n;
  if (base < 0) base = 0;
  for (int i = 0; i < n - 1; i++)
    cubalc_cube_plug(ch, base + i, base + ((i + 1) % n));
  /* create plugs to destroy (construct↔deconstruct edge) */
  for (int i = 0; i < ch->n_cubes; i++) {
    if (strcmp(ch->cubes[i].id, "cube-create") == 0)
      for (int j = 0; j < ch->n_cubes; j++)
        if (strcmp(ch->cubes[j].id, "cube-destroy") == 0)
          cubalc_cube_plug(ch, i, j);
  }
  snprintf(ch->status, sizeof ch->status, "OS cubes n=%d plugs live", ch->n_cubes);
  return ch->n_cubes;
}

int cubalc_chain_tick(cubalc_chain *ch) {
  if (!ch) return -1;
  ch->seq++;
  float u = 0.f, e = 0.f; int alive = 0;
  for (int i = 0; i < ch->n_cubes; i++) {
    cubalc_cube *c = &ch->cubes[i];
    c->atom.digit = cubalc_algocube_digit(&c->atom.matrix);
    /* slow energy decay toward proton rest — keeps the chain breathing */
    float rest = c->atom.proton ? 0.55f : 0.20f;
    c->atom.energy += (rest - c->atom.energy) * 0.04f;
    if (c->atom.energy < 0.f) c->atom.energy = 0.f;
    if (c->atom.energy > 1.f) c->atom.energy = 1.f;
    if (c->atom.alive) {
      alive++;
      u += c->atom.unity > 0 ? c->atom.unity : 1.f;
      e += c->atom.energy;
    }
    /* Cube size = matrix density + energy glow */
    float dens = c->atom.matrix.n ? (float)c->atom.matrix.set / (float)c->atom.matrix.n : 0;
    c->s = 0.09f + 0.07f * dens + 0.05f * c->atom.energy;
    /* alpha pulses with energy for viz */
    c->a = (uint8_t)(160 + (int)(95.f * c->atom.energy));
  }
  ch->unity = alive ? u / (float)alive : 1.f;
  float eavg = alive ? e / (float)alive : 0.f;
  snprintf(ch->status, sizeof ch->status,
           "tick seq=%u cubes=%d alive=%d unity=%.2f energy=%.2f hold=%u · matrix flows",
           (unsigned)ch->seq, ch->n_cubes, alive, ch->unity, eavg,
           (unsigned)ch->hold_flash);
  return 0;
}

int cubalc_chain_flow(cubalc_chain *ch) {
  if (!ch || ch->n_cubes < 1) return -1;
  /* One hop of energy around every plug — binary talk IS the energy wire */
  int talks = 0;
  for (int i = 0; i < ch->n_cubes; i++) {
    for (int p = 0; p < ch->cubes[i].n_ports; p++) {
      int peer = ch->cubes[i].ports[p].peer;
      if (peer > i) { /* each edge once */
        cubalc_cube_talk(ch, i, peer);
        talks++;
      }
    }
  }
  cubalc_chain_tick(ch);
  snprintf(ch->status, sizeof ch->status,
           "flow talks=%d energy_pulse seq=%u unity=%.2f",
           talks, (unsigned)ch->seq, ch->unity);
  return talks;
}

int cubalc_chain_impulse(cubalc_chain *ch, const char *cube_id, uint8_t proton) {
  if (!ch || !cube_id) return -1;
  for (int i = 0; i < ch->n_cubes; i++) {
    if (strcmp(ch->cubes[i].id, cube_id) == 0) {
      cubalc_atom_impulse(&ch->cubes[i].atom, proton);
      /* propagate binary talk to all plugged peers */
      for (int p = 0; p < ch->cubes[i].n_ports; p++) {
        int peer = ch->cubes[i].ports[p].peer;
        if (peer >= 0) cubalc_cube_talk(ch, i, peer);
      }
      return 0;
    }
  }
  return -1;
}

int cubalc_chain_write_viz(const cubalc_chain *ch, const char *path) {
  if (!ch || !path) return -1;
  FILE *f = fopen(path, "w");
  if (!f) return -1;
  int n = ch->n_cubes;
  if (n > CUBALC_BUDGET) n = CUBALC_BUDGET;
  float eavg = 0.f;
  for (int i = 0; i < n; i++) eavg += ch->cubes[i].atom.energy;
  if (n) eavg /= (float)n;
  /* Visual language: cubes + energy glow + plug edges (binary wires) */
  fprintf(f, "{\"schema\":\"cube.viz_frame.v1\",\"lang\":\"CubalC\",\"paradigm\":\"COP\","
             "\"seq\":%u,\"unity\":%.4f,\"energy\":%.4f,"
             "\"n_cubes\":%d,\"budget\":%d,\"cubalc\":true,\"hold_flash\":1,"
             "\"share\":\"%s\",\"matrix_is_key\":true,\"talk\":\"binary_CBLC\","
             "\"hud\":\"CubalC · %d cubes · E=%.2f · unity=%.2f · %s\","
             "\"cubes\":[",
          (unsigned)ch->seq, ch->unity, eavg, n, CUBALC_BUDGET, CUBALC_SHARE,
          n, eavg, ch->unity, ch->creed);
  for (int i = 0; i < n; i++) {
    const cubalc_cube *c = &ch->cubes[i];
    if (i) fputc(',', f);
    /* short matrix nib for machines + humans in viz tools */
    char bits[17];
    int bn = c->atom.matrix.n < 16 ? c->atom.matrix.n : 16;
    for (int b = 0; b < bn; b++) bits[b] = cubalc_matrix_get(&c->atom.matrix, b) ? '1' : '0';
    bits[bn] = 0;
    char bits64[CUBALC_ATOM_BITS + 1];
    int bn64 = c->atom.matrix.n < CUBALC_ATOM_BITS ? c->atom.matrix.n : CUBALC_ATOM_BITS;
    for (int b = 0; b < bn64; b++)
      bits64[b] = cubalc_matrix_get(&c->atom.matrix, b) ? '1' : '0';
    bits64[bn64] = 0;
    fprintf(f, "{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f,\"s\":%.3f,"
               "\"yaw\":%.2f,\"pitch\":%.2f,\"roll\":%.2f,"
               "\"rgba\":[%u,%u,%u,%u],\"role\":%u,"
               "\"id\":\"%s\",\"label\":\"%s\",\"kind\":\"cube\","
               "\"digit\":%u,\"proton\":%u,\"energy\":%.3f,"
               "\"plugged\":%u,\"set\":%u,\"matrix16\":\"%s\",\"matrix\":\"%s\"}",
            c->x, c->y, c->z, c->s,
            c->yaw, c->pitch, c->roll,
            c->r, c->g, c->b, c->a,
            c->atom.proton ? 1 : 6,
            c->id, c->label,
            (unsigned)c->atom.digit, (unsigned)c->atom.proton, c->atom.energy,
            (unsigned)c->plugged, (unsigned)c->atom.matrix.set, bits, bits64);
  }
  fprintf(f, "],\"edges\":[");
  int first = 1;
  for (int i = 0; i < n; i++) {
    for (int p = 0; p < ch->cubes[i].n_ports; p++) {
      int peer = ch->cubes[i].ports[p].peer;
      if (peer > i && peer < n) {
        float en = 0.5f * (ch->cubes[i].atom.energy + ch->cubes[peer].atom.energy);
        if (!first) fputc(',', f);
        first = 0;
        fprintf(f, "{\"from\":\"%s\",\"to\":\"%s\",\"wire\":\"binary\","
                   "\"energy\":%.3f,\"compat\":%.3f}",
                ch->cubes[i].id, ch->cubes[peer].id, en,
                cubalc_matrix_compat(&ch->cubes[i].atom.matrix,
                                    &ch->cubes[peer].atom.matrix));
      }
    }
  }
  fprintf(f, "]}\n");
  fclose(f);
  return 0;
}

/* Optional human isometric cubes (CUBALC_HUMAN=1). Agents use metrics board. */
static void print_iso_cube(FILE *out, const cubalc_cube *c, int idx) {
  int e = (int)(c->atom.energy * 5.f + 0.5f);
  if (e > 5) e = 5;
  char face = c->atom.alive ? (e >= 4 ? '#' : (e >= 2 ? '=' : '.')) : ' ';
  char id[12];
  snprintf(id, sizeof id, "%.8s", c->id);
  fprintf(out, "  [%d] %-8s  dig=%u  E=%d/5  P=%u\n",
          idx, id, (unsigned)c->atom.digit, e, (unsigned)c->atom.proton);
  fprintf(out,
    "       +------+\n"
    "      /|%c%c%c%c%c%c/|\n"
    "     +------+ |\n"
    "     |%c%c%c%c%c%c| +\n"
    "     |%c%c%c%c%c%c|/\n"
    "     +------+\n",
    face, face, face, face, face, face,
    face, face, face, face, face, face,
    face, face, face, face, face, face);
}

int cubalc_chain_print_cubes(const cubalc_chain *ch, FILE *out) {
  if (!ch || !out) return -1;
  const char *human = getenv("CUBALC_HUMAN");
  int H = human && human[0] && human[0] != '0';

  fprintf(out, "+-- c3 smx seq=%u u=%.2f n=%d/%d hold=%u tok=%s\n",
          (unsigned)ch->seq, ch->unity, ch->n_cubes, CUBALC_BUDGET,
          (unsigned)ch->hold_flash, ch->creed[0] ? ch->creed : "C3");

  if (H) {
    fprintf(out, "| human cubes (optional viz) · CUBALC_HUMAN=1\n");
    int maxn = ch->n_cubes < 8 ? ch->n_cubes : 8;
    for (int i = 0; i < maxn; i++)
      print_iso_cube(out, &ch->cubes[i], i);
    if (ch->n_cubes > maxn)
      fprintf(out, "  … +%d more cubes\n", ch->n_cubes - maxn);
  }

  for (int i = 0; i < ch->n_cubes; i++) {
    const cubalc_cube *c = &ch->cubes[i];
    int bars = (int)(c->atom.energy * 10.f + 0.5f);
    if (bars > 10) bars = 10;
    char ebar[12];
    for (int k = 0; k < 10; k++) ebar[k] = k < bars ? '#' : '.';
    ebar[10] = 0;
    char bits[17];
    int bn = c->atom.matrix.n < 16 ? (int)c->atom.matrix.n : 16;
    for (int b = 0; b < bn; b++) bits[b] = cubalc_matrix_get(&c->atom.matrix, b) ? '1' : '0';
    bits[bn] = 0;
    fprintf(out, "| [%c] %-14s P=%u E[%s] dig=%u set=%2u plugs=%u\n",
            c->atom.alive ? '#' : '.', c->id, (unsigned)c->atom.proton,
            ebar, (unsigned)c->atom.digit, (unsigned)c->atom.matrix.set,
            (unsigned)c->plugged);
    fprintf(out, "|     m16 %s r=%s\n", bits, c->role);
  }
  fprintf(out, "+-- cblc\n");
  for (int i = 0; i < ch->n_cubes; i++) {
    for (int p = 0; p < ch->cubes[i].n_ports; p++) {
      int peer = ch->cubes[i].ports[p].peer;
      if (peer > i && ch->cubes[i].ports[p].dir == CUBALC_PORT_OUT)
        fprintf(out, "|  %s -> %s\n",
                ch->cubes[i].id, ch->cubes[peer].id);
    }
  }
  fprintf(out, "+--\n");
  return 0;
}

int cubalc_chain_print_lego(const cubalc_chain *ch, FILE *out) {
  return cubalc_chain_print_cubes(ch, out); /* deprecated alias */
}

int cubalc_chain_write_json(const cubalc_chain *ch, const char *path) {
  if (!ch || !path) return -1;
  FILE *f = fopen(path, "w");
  if (!f) return -1;
  fprintf(f, "{\"schema\":\"cubalc.chain.v1\",\"tok\":\"%s\","
             "\"seq\":%u,\"unity\":%.4f,\"hold_flash\":%u,\"n_cubes\":%d,"
             "\"initial_set\":%u,\"status\":\"%s\",\"budget\":%d,"
             "\"law\":\"plug·cblc·atom·p01\"}\n",
          ch->creed, (unsigned)ch->seq, ch->unity, (unsigned)ch->hold_flash,
          ch->n_cubes, (unsigned)ch->initial.set, ch->status, CUBALC_BUDGET);
  fclose(f);
  return 0;
}

static float wrap_deg(float a) {
  while (a >= 360.f) a -= 360.f;
  while (a < 0.f) a += 360.f;
  return a;
}

int cubalc_cube_spin(cubalc_chain *ch, const char *cube_id,
                     float dyaw, float dpitch, float droll) {
  if (!ch || !cube_id) return -1;
  for (int i = 0; i < ch->n_cubes; i++) {
    if (strcmp(ch->cubes[i].id, cube_id) != 0) continue;
    cubalc_cube *c = &ch->cubes[i];
    /* Spin rate also keyed by matrix density — denser Kernel Matrix spins with more life */
    float dens = c->atom.matrix.n
      ? (float)c->atom.matrix.set / (float)c->atom.matrix.n : 0.f;
    float k = 0.75f + 0.5f * dens + 0.25f * c->atom.energy;
    c->yaw   = wrap_deg(c->yaw   + dyaw   * k);
    c->pitch = wrap_deg(c->pitch + dpitch * k);
    c->roll  = wrap_deg(c->roll  + droll  * k);
    /* Rotate matrix view: roll low bits as the cube turns (Kernel State visible spin) */
    int shift = ((int)(dyaw / 10.f) % CUBALC_ATOM_BITS + CUBALC_ATOM_BITS) % CUBALC_ATOM_BITS;
    if (shift) {
      uint8_t tmp[(CUBALC_ATOM_BITS + 7) / 8];
      memset(tmp, 0, sizeof tmp);
      int n = c->atom.matrix.n > 0 ? c->atom.matrix.n : CUBALC_ATOM_BITS;
      for (int b = 0; b < n; b++) {
        if (cubalc_matrix_get(&c->atom.matrix, b)) {
          int nb = (b + shift) % n;
          tmp[nb >> 3] |= (uint8_t)(1u << (nb & 7));
        }
      }
      memcpy(c->atom.matrix.bits, tmp, sizeof c->atom.matrix.bits);
      c->atom.matrix.set = (uint16_t)cubalc_matrix_popcount(&c->atom.matrix);
      c->atom.digit = cubalc_algocube_digit(&c->atom.matrix);
    }
    return 0;
  }
  return -1;
}

int cubalc_cube_pose(cubalc_chain *ch, const char *cube_id,
                     float x, float y, float z, float s) {
  if (!ch || !cube_id) return -1;
  for (int i = 0; i < ch->n_cubes; i++) {
    if (strcmp(ch->cubes[i].id, cube_id) != 0) continue;
    ch->cubes[i].x = x;
    ch->cubes[i].y = y;
    ch->cubes[i].z = z;
    if (s > 0.f) ch->cubes[i].s = s;
    return 0;
  }
  return -1;
}

/* Project & draw a spinning cube whose faces carry Kernel Matrix bits */
int cubalc_cube_print_spin(const cubalc_chain *ch, const char *cube_id, FILE *out) {
  if (!ch || !cube_id || !out) return -1;
  const cubalc_cube *c = NULL;
  for (int i = 0; i < ch->n_cubes; i++)
    if (strcmp(ch->cubes[i].id, cube_id) == 0) { c = &ch->cubes[i]; break; }
  if (!c) return -1;

  const float DEG = 0.017453292519943295f;
  float cy = cosf(c->yaw * DEG), sy = sinf(c->yaw * DEG);
  float cp = cosf(c->pitch * DEG), sp = sinf(c->pitch * DEG);
  float cr = cosf(c->roll * DEG), sr = sinf(c->roll * DEG);

  /* unit cube corners */
  float corn[8][3];
  int vi = 0;
  for (int zi = -1; zi <= 1; zi += 2)
    for (int yi = -1; yi <= 1; yi += 2)
      for (int xi = -1; xi <= 1; xi += 2) {
        float x = 0.9f * (float)xi, y = 0.9f * (float)yi, z = 0.9f * (float)zi;
        /* yaw(Y) * pitch(X) * roll(Z) */
        float x1 = cr * x - sr * y, y1 = sr * x + cr * y, z1 = z;
        float y2 = cp * y1 - sp * z1, z2 = sp * y1 + cp * z1, x2 = x1;
        float x3 = cy * x2 + sy * z2, z3 = -sy * x2 + cy * z2, y3 = y2;
        corn[vi][0] = x3; corn[vi][1] = y3; corn[vi][2] = z3;
        vi++;
      }

  const int W = 40, H = 20;
  char buf[20][41];
  float zbuf[20][40];
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) { buf[y][x] = ' '; zbuf[y][x] = -1e9f; }
    buf[y][W] = 0;
  }

  /* edges of cube */
  static const int E[12][2] = {
    {0,1},{1,3},{3,2},{2,0}, {4,5},{5,7},{7,6},{6,4},
    {0,4},{1,5},{2,6},{3,7}
  };
  for (int e = 0; e < 12; e++) {
    float x0 = corn[E[e][0]][0], y0 = corn[E[e][0]][1], z0 = corn[E[e][0]][2];
    float x1 = corn[E[e][1]][0], y1 = corn[E[e][1]][1], z1 = corn[E[e][1]][2];
    for (int t = 0; t <= 24; t++) {
      float u = t / 24.f;
      float x = x0 + (x1 - x0) * u;
      float y = y0 + (y1 - y0) * u;
      float z = z0 + (z1 - z0) * u;
      /* perspective */
      float d = 3.2f + z;
      if (d < 0.2f) d = 0.2f;
      int px = (int)(W / 2 + (x * 12.f) / d * 8.f);
      int py = (int)(H / 2 - (y * 12.f) / d * 4.f);
      if (px >= 0 && px < W && py >= 0 && py < H && z > zbuf[py][px]) {
        zbuf[py][px] = z;
        buf[py][px] = (e < 4) ? '#' : (e < 8) ? '+' : '|';
      }
    }
  }

  /* Face centers: paint matrix bits as 0/1 glyphs (Kernel Matrix state) */
  static const int faces[6][4] = {
    {0,1,3,2}, {4,5,7,6}, {0,1,5,4}, {2,3,7,6}, {0,2,6,4}, {1,3,7,5}
  };
  for (int f = 0; f < 6; f++) {
    float cx = 0, cy_ = 0, cz = 0;
    for (int k = 0; k < 4; k++) {
      cx += corn[faces[f][k]][0];
      cy_ += corn[faces[f][k]][1];
      cz += corn[faces[f][k]][2];
    }
    cx *= 0.25f; cy_ *= 0.25f; cz *= 0.25f;
    float d = 3.2f + cz;
    if (d < 0.2f) continue;
    int px = (int)(W / 2 + (cx * 12.f) / d * 8.f);
    int py = (int)(H / 2 - (cy_ * 12.f) / d * 4.f);
    /* 2x2 bit patch from matrix */
    for (int dy = -1; dy <= 1; dy++) {
      for (int dx = -1; dx <= 1; dx++) {
        int bit = (f * 9 + (dy + 1) * 3 + (dx + 1)) % CUBALC_ATOM_BITS;
        char chb = cubalc_matrix_get(&c->atom.matrix, bit) ? '1' : '0';
        int qx = px + dx, qy = py + dy;
        if (qx >= 0 && qx < W && qy >= 0 && qy < H && cz >= zbuf[qy][qx] - 0.05f) {
          zbuf[qy][qx] = cz;
          buf[qy][qx] = chb;
        }
      }
    }
  }

  fprintf(out, "\n+-- kernel id=%s r=%s set=%u dig=%u\n",
          c->id, c->role, (unsigned)c->atom.matrix.set, (unsigned)c->atom.digit);
  fprintf(out, "| y=%.1f p=%.1f r=%.1f E=%.2f P=%u m16 ",
          c->yaw, c->pitch, c->roll, c->atom.energy, (unsigned)c->atom.proton);
  for (int b = 0; b < 16 && b < c->atom.matrix.n; b++)
    fputc(cubalc_matrix_get(&c->atom.matrix, b) ? '1' : '0', out);
  fprintf(out, "\n");
  for (int y = 0; y < H; y++)
    fprintf(out, "|%s|\n", buf[y]);
  fprintf(out, "+--\n");
  return 0;
}


int cubalc_law_check(const cubalc_chain *ch, char *err, size_t errn) {
  if (err && errn) err[0] = 0;
  /* L0 SoT: matrix exists as SoT flag always in build */
  if (CUBALC_HOLD_FLASH != 1) {
    if (err) snprintf(err, errn, "HOLD_FLASH must be 1");
    return -1;
  }
  if (strcmp(CUBALC_SHARE, "smx") != 0) {
    if (err) snprintf(err, errn, "SHARE must be smx");
    return -2;
  }
  if (!ch) return 0; /* build-only laws ok */
  if (ch->hold_flash != 1) {
    if (err) snprintf(err, errn, "chain hold_flash!=1");
    return -3;
  }
  for (int i = 0; i < ch->n_cubes; i++) {
    const cubalc_cube *c = &ch->cubes[i];
    int has_in = 0, has_out = 0;
    for (int p = 0; p < c->n_ports; p++) {
      if (!c->ports[p].open) continue;
      if (c->ports[p].dir == CUBALC_PORT_IN) has_in = 1;
      if (c->ports[p].dir == CUBALC_PORT_OUT) has_out = 1;
    }
    if (!has_in || !has_out) {
      if (err) snprintf(err, errn, "cube %s missing IN/OUT", c->id);
      return -4;
    }
    /* matrix is key — every cube has matrix width */
    if (c->atom.matrix.n == 0) {
      if (err) snprintf(err, errn, "cube %s empty matrix n", c->id);
      return -5;
    }
  }
  return 0;
}

int cubalc_law_manifest_json(const cubalc_chain *ch, FILE *out) {
  if (!out) return -1;
  char err[120];
  int rc = cubalc_law_check(ch, err, sizeof err);
  fprintf(out, "{\"schema\":\"cubalc.law_manifest.v1\",\"ok\":%s,"
               "\"language\":\"%s\",\"version\":\"%s\",\"paradigm\":\"%s\","
               "\"creed\":\"%s\",\"share\":\"%s\",\"hold_flash\":%d,"
               "\"talk\":\"binary_CBLC\",\"store\":\"cubechain\","
               "\"no_brain_wires\":true,\"devices_free\":true,"
               "\"one_commander\":true,\"matrix_is_key\":true,"
               "\"cube_is_sot\":true,\"visual\":\"cubes_not_lego\","
               "\"n_cubes\":%d,\"unity\":%.4f,\"laws\":[",
          rc == 0 ? "true" : "false",
          CUBALC_LANG_NAME, CUBALC_LANG_VERSION, CUBALC_LANG_PARADIGM,
          CUBALC_CREED, CUBALC_SHARE, CUBALC_HOLD_FLASH,
          ch ? ch->n_cubes : 0, ch ? ch->unity : 0.f);
  for (int i = 0; i < CUBALC_LAW_COUNT; i++) {
    if (i) fputc(',', out);
    fprintf(out, "{\"id\":%d,\"name\":\"%s\",\"ok\":true}", i, CUBALC_LAW_NAME[i]);
  }
  /* mark IN/OUT fail if rc==-4 */
  fprintf(out, "],\"check_rc\":%d,\"err\":\"%s\"}\n", rc, rc ? err : "");
  return rc;
}
