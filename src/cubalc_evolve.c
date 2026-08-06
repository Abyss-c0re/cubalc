#define _POSIX_C_SOURCE 200809L
#include "cubalc_evolve.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/* ---------- RNG ---------- */
static uint32_t xorshift(uint32_t *s) {
  uint32_t x = *s ? *s : 0xC0BEA160u;
  x ^= x << 13; x ^= x >> 17; x ^= x << 5;
  *s = x ? x : 0xC0BEA160u;
  return *s;
}
static int rnd(uint32_t *s, int mod) {
  if (mod <= 0) return 0;
  return (int)(xorshift(s) % (uint32_t)mod);
}

/* ---------- paths ---------- */
static void ensure_dir(const char *path) {
  char tmp[512];
  snprintf(tmp, sizeof tmp, "%s", path);
  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
      mkdir(tmp, 0755);
      *p = '/';
    }
  }
  mkdir(tmp, 0755);
}

static const char *default_state(void) {
  const char *e = getenv("CUBALC_STATE");
  return (e && e[0]) ? e : "state";
}

/* ---------- mind init ---------- */
static int idx3(int n, int x, int y, int z) {
  return (z * n + y) * n + x;
}

static void open_corridor(cubalc_evolve_mind *m, int mx, int my) {
  int n = m->n;
  if (mx < 0) mx = n / 2;
  if (my < 0) my = n / 2;
  for (int z = 0; z < n; z++) {
    int i = idx3(n, mx, my, z);
    if (m->cells[i] < 6) m->cells[i] = 6;
    for (int k = 0; k < 4; k++) {
      int dx[] = {1, -1, 0, 0}, dy[] = {0, 0, 1, -1};
      int x = mx + dx[k], y = my + dy[k];
      if (x >= 0 && x < n && y >= 0 && y < n) {
        int j = idx3(n, x, y, z);
        if (m->cells[j] < 4) m->cells[j] = 4;
      }
    }
  }
}

static void mind_reset(cubalc_evolve_mind *m, uint32_t seed) {
  memset(m, 0, sizeof(*m));
  m->n = CUBALC_EVOLVE_N;
  m->seed = seed ? seed : (uint32_t)time(NULL);
  m->difficulty = 1.0f;
  m->best_fitness = 0.f;
  uint32_t s = m->seed;
  for (int i = 0; i < CUBALC_EVOLVE_CELLS; i++)
    m->cells[i] = (uint8_t)(rnd(&s, 10));
  for (int i = 0; i < CUBALC_EVOLVE_GENOME; i++) {
    m->genome[i] = (uint8_t)(rnd(&s, 10));
    m->best_genome[i] = m->genome[i];
  }
  open_corridor(m, -1, -1);
  snprintf(m->status, sizeof m->status, "init");
}

static int open_cell(const cubalc_evolve_mind *m, int i) {
  int d = m->cells[i];
  return d >= 4 || (d % 3 == 0 && d > 0);
}

static void io_ports(const cubalc_evolve_mind *m, int *i_cell, int *o_cell) {
  int n = m->n, mid = n / 2, mode = m->face_mode % 6;
  switch (mode) {
  case 0: *i_cell = idx3(n, mid, mid, 0); *o_cell = idx3(n, mid, mid, n - 1); break;
  case 1: *i_cell = idx3(n, mid + 1, mid, 0); *o_cell = idx3(n, mid - 1, mid, n - 1); break;
  case 2: *i_cell = idx3(n, 0, mid, mid); *o_cell = idx3(n, n - 1, mid, mid); break;
  case 3: *i_cell = idx3(n, mid, 0, mid); *o_cell = idx3(n, mid, n - 1, mid); break;
  case 4: *i_cell = idx3(n, 0, 0, 0); *o_cell = idx3(n, n - 1, n - 1, n - 1); break;
  default: *i_cell = idx3(n, mid - 1, mid + 1, 0); *o_cell = idx3(n, mid + 1, mid - 1, n - 1); break;
  }
}

static int pathfind(const cubalc_evolve_mind *m, int start, int goal,
                    int *path_out, int path_cap) {
  int n = m->n, nn = n * n * n;
  int hops[CUBALC_EVOLVE_CELLS];
  int prev[CUBALC_EVOLVE_CELLS];
  int q[CUBALC_EVOLVE_CELLS];
  int qh = 0, qt = 0;
  for (int i = 0; i < nn; i++) { hops[i] = -1; prev[i] = -1; }
  q[qt++] = start;
  hops[start] = 0;
  int dx[] = {1, -1, 0, 0, 0, 0};
  int dy[] = {0, 0, 1, -1, 0, 0};
  int dz[] = {0, 0, 0, 0, 1, -1};
  while (qh < qt) {
    int cur = q[qh++];
    if (cur == goal) break;
    int cz = cur / (n * n);
    int rem = cur % (n * n);
    int cy = rem / n, cx = rem % n;
    for (int k = 0; k < 6; k++) {
      int nx = cx + dx[k], ny = cy + dy[k], nz = cz + dz[k];
      if (nx < 0 || nx >= n || ny < 0 || ny >= n || nz < 0 || nz >= n) continue;
      int ni = idx3(n, nx, ny, nz);
      if (hops[ni] >= 0) continue;
      if (!open_cell(m, ni) && ni != goal) continue;
      hops[ni] = hops[cur] + 1;
      prev[ni] = cur;
      q[qt++] = ni;
    }
  }
  if (hops[goal] < 0) return -1;
  /* reconstruct */
  int len = hops[goal] + 1;
  if (path_out && path_cap > 0) {
    int c = goal, write = len - 1;
    if (write >= path_cap) write = path_cap - 1;
    while (c >= 0 && write >= 0) {
      path_out[write--] = c;
      c = prev[c];
    }
  }
  return hops[goal];
}

/* ---------- algocube fitness problems (solve + optimize) ---------- */
typedef struct {
  int solves, fails;
  float fitness; /* 0..1 */
  int digit;
} algo_score;

static void bits_from_pattern(cubalc_matrix *m, uint32_t pat, int nbits) {
  cubalc_matrix_clear(m);
  m->n = (uint16_t)(nbits > CUBALC_ATOM_BITS ? CUBALC_ATOM_BITS : nbits);
  for (int i = 0; i < m->n; i++) {
    int on = (pat >> (i % 32)) & 1;
    /* mix genome salt into pattern */
    cubalc_matrix_set(m, i, on);
  }
}

static void bits_from_genome(cubalc_matrix *m, const uint8_t *g, int goff) {
  cubalc_matrix_clear(m);
  m->n = CUBALC_ATOM_BITS;
  for (int i = 0; i < CUBALC_ATOM_BITS; i++) {
    int d = g[(goff + i) % CUBALC_EVOLVE_GENOME];
    if ((d + i) % 3 == 0 || d >= 6) cubalc_matrix_set(m, i, 1);
  }
}

/* Problem suite: braincube uses results to mutate genome. */
static algo_score algocube_solve_suite(const cubalc_evolve_mind *mind) {
  algo_score sc = {0, 0, 0.f, 4};
  float score = 0.f;
  int tests = 0;

  /* P1: equal matrices → unity ~1, hamming 0 */
  {
    cubalc_matrix a, b;
    bits_from_genome(&a, mind->genome, 0);
    b = a;
    cubalc_algo_cmp cmp;
    cubalc_algocube_compare(&a, &b, &cmp);
    tests++;
    if (cmp.hamming == 0 && cmp.unity >= 0.999f) { sc.solves++; score += 1.f; }
    else sc.fails++;
    sc.digit = cubalc_algocube_digit(&a);
  }

  /* P2: one-bit flip → hamming >= 1, unity < 1 */
  {
    cubalc_matrix a, b;
    bits_from_genome(&a, mind->genome, 3);
    b = a;
    int bit = mind->genome[0] % (a.n ? a.n : 1);
    cubalc_matrix_set(&b, bit, cubalc_matrix_get(&b, bit) ? 0 : 1);
    cubalc_algo_cmp cmp;
    cubalc_algocube_compare(&a, &b, &cmp);
    tests++;
    if (cmp.hamming >= 1 && cmp.unity < 1.f) { sc.solves++; score += 1.f; }
    else sc.fails++;
  }

  /* P3: digit range always 0..9 and deterministic */
  {
    cubalc_matrix a;
    bits_from_genome(&a, mind->genome, 7);
    int d1 = cubalc_algocube_digit(&a);
    int d2 = cubalc_algocube_digit(&a);
    tests++;
    if (d1 == d2 && d1 >= 0 && d1 <= 9) { sc.solves++; score += 1.f; sc.digit = d1; }
    else sc.fails++;
  }

  /* P4: blueprint10 produces 10 digits in range */
  {
    cubalc_matrix a;
    bits_from_genome(&a, mind->genome, 11);
    cubalc_algo_bp bp;
    cubalc_algocube_blueprint10(&a, &bp);
    int ok = (bp.n == 10);
    for (int i = 0; i < 10 && ok; i++)
      if (bp.digits[i] < 0 || bp.digits[i] > 9) ok = 0;
    tests++;
    if (ok) { sc.solves++; score += 1.f; }
    else sc.fails++;
  }

  /* P5: harmony majority on 3 near-clones → ok + digit 0..9 */
  {
    cubalc_matrix m0, m1, m2;
    bits_from_genome(&m0, mind->genome, 0);
    bits_from_genome(&m1, mind->genome, 0);
    bits_from_genome(&m2, mind->genome, 1);
    /* slight noise on m2 */
    cubalc_matrix_set(&m2, mind->genome[2] % 64, 1);
    const cubalc_matrix *ps[3] = {&m0, &m1, &m2};
    cubalc_algo_harm h;
    cubalc_algocube_harmony(ps, 3, &h);
    tests++;
    if (h.ok && h.digit >= 0 && h.digit <= 9 && h.unity > 0.5f) {
      sc.solves++; score += 1.f;
      sc.digit = h.digit;
    } else sc.fails++;
  }

  /* P6: genome inject stickiness — SETDIGIT-like lock simulation */
  {
    cubalc_cube c;
    memset(&c, 0, sizeof c);
    cubalc_atom_init(&c.atom, "algo", 1);
    cubalc_matrix mat;
    bits_from_genome(&mat, mind->genome, 5);
    int want = mind->genome[4] % 10;
    cubalc_algocube_inject(&c, &mat, want);
    tests++;
    if (c.atom.digit_lock == 1 && c.atom.digit == (uint8_t)want) {
      sc.solves++; score += 1.f;
    } else sc.fails++;
  }

  /* P7: salted digit stable */
  {
    cubalc_matrix a;
    bits_from_genome(&a, mind->genome, 9);
    char salt[48];
    snprintf(salt, sizeof salt, "g%u", (unsigned)mind->genome[0]);
    int d1 = cubalc_algocube_digit_salted(&a, salt);
    int d2 = cubalc_algocube_digit_salted(&a, salt);
    tests++;
    if (d1 == d2 && d1 >= 0 && d1 <= 9) { sc.solves++; score += 1.f; }
    else sc.fails++;
  }

  /* P8: optimization target — maximize diversity of blueprint digits */
  {
    cubalc_matrix a;
    bits_from_genome(&a, mind->genome, 0);
    cubalc_algo_bp bp;
    cubalc_algocube_blueprint10(&a, &bp);
    int seen[10] = {0}, uniq = 0;
    for (int i = 0; i < 10; i++) {
      if (!seen[bp.digits[i] % 10]) { seen[bp.digits[i] % 10] = 1; uniq++; }
    }
    tests++;
    /* soft: uniq >= 4 is solve; partial credit always */
    float part = (float)uniq / 10.f;
    score += part;
    if (uniq >= 4) sc.solves++;
    else sc.fails++;
  }

  sc.fitness = tests ? score / (float)tests : 0.f;
  return sc;
}

static void mutate_genome(cubalc_evolve_mind *m, int strength) {
  uint32_t s = m->seed ^ (uint32_t)m->races * 0x9E3779B9u;
  int nmut = 1 + rnd(&s, strength < 1 ? 1 : strength);
  for (int i = 0; i < nmut; i++) {
    int k = rnd(&s, CUBALC_EVOLVE_GENOME);
    m->genome[k] = (uint8_t)((m->genome[k] + 1 + rnd(&s, 3)) % 10);
  }
}

/* Strategy-tagged mutations for deep search (ways to solve/optimize). */
typedef enum {
  WAY_POINT = 0,   /* single locus nudge */
  WAY_SWAP,        /* swap two loci */
  WAY_INVERT,      /* 9 - digit on a window */
  WAY_BLOCK,       /* reseed a 4-digit block */
  WAY_CROSS,       /* crossover with best_genome */
  WAY_SCRAMBLE,    /* strong multi-locus */
  WAY_BLUEPRINT,   /* bias toward diverse digits 0-9 */
  WAY_COUNT
} algo_way_t;

static const char *const ALGO_WAY_NAME[WAY_COUNT] = {
  "point_nudge", "swap_loci", "invert_window", "reseed_block",
  "crossover_best", "scramble", "blueprint_diversity"
};

static void mutate_way(cubalc_evolve_mind *m, algo_way_t way, uint32_t *rng) {
  switch (way) {
  case WAY_POINT: {
    int k = rnd(rng, CUBALC_EVOLVE_GENOME);
    m->genome[k] = (uint8_t)((m->genome[k] + 1 + rnd(rng, 3)) % 10);
    break;
  }
  case WAY_SWAP: {
    int a = rnd(rng, CUBALC_EVOLVE_GENOME), b = rnd(rng, CUBALC_EVOLVE_GENOME);
    uint8_t t = m->genome[a]; m->genome[a] = m->genome[b]; m->genome[b] = t;
    break;
  }
  case WAY_INVERT: {
    int start = rnd(rng, CUBALC_EVOLVE_GENOME);
    for (int i = 0; i < 6; i++) {
      int k = (start + i) % CUBALC_EVOLVE_GENOME;
      m->genome[k] = (uint8_t)(9 - m->genome[k]);
    }
    break;
  }
  case WAY_BLOCK: {
    int start = rnd(rng, CUBALC_EVOLVE_GENOME);
    for (int i = 0; i < 4; i++)
      m->genome[(start + i) % CUBALC_EVOLVE_GENOME] = (uint8_t)rnd(rng, 10);
    break;
  }
  case WAY_CROSS: {
    for (int i = 0; i < CUBALC_EVOLVE_GENOME; i++)
      if (rnd(rng, 2) == 0) m->genome[i] = m->best_genome[i];
    /* then one nudge so we explore */
    m->genome[rnd(rng, CUBALC_EVOLVE_GENOME)] =
      (uint8_t)((m->genome[rnd(rng, CUBALC_EVOLVE_GENOME)] + 1) % 10);
    break;
  }
  case WAY_SCRAMBLE: {
    int n = 3 + rnd(rng, 8);
    for (int i = 0; i < n; i++)
      m->genome[rnd(rng, CUBALC_EVOLVE_GENOME)] = (uint8_t)rnd(rng, 10);
    break;
  }
  case WAY_BLUEPRINT: {
    /* force all digits 0-9 present once, rest random — diversity optimize */
    for (int d = 0; d < 10 && d < CUBALC_EVOLVE_GENOME; d++)
      m->genome[d] = (uint8_t)d;
    for (int i = 10; i < CUBALC_EVOLVE_GENOME; i++)
      m->genome[i] = (uint8_t)rnd(rng, 10);
    /* shuffle */
    for (int i = CUBALC_EVOLVE_GENOME - 1; i > 0; i--) {
      int j = rnd(rng, i + 1);
      uint8_t t = m->genome[i]; m->genome[i] = m->genome[j]; m->genome[j] = t;
    }
    break;
  }
  default:
    mutate_genome(m, 2);
    break;
  }
}

static void accept_or_revert_genome(cubalc_evolve_mind *m, float new_fit) {
  if (new_fit >= m->best_fitness - 0.001f) {
    if (new_fit > m->best_fitness) {
      m->best_fitness = new_fit;
      memcpy(m->best_genome, m->genome, sizeof m->best_genome);
    }
  } else {
    /* hill-climb: revert on clear regression */
    if (new_fit + 0.08f < m->best_fitness)
      memcpy(m->genome, m->best_genome, sizeof m->genome);
  }
  m->last_fitness = new_fit;
}

/* Deep multi-strategy search — look for ways to solve/optimize algocubes. */
int cubalc_algocube_deep_optimize(cubalc_evolve_mind *m, const char *state_dir,
                                  int trials, double *fit_before, double *fit_after) {
  if (!m) return -1;
  if (!state_dir) state_dir = default_state();
  if (trials <= 0) trials = CUBALC_ALGO_DEEP_TRIALS;

  algo_score base = algocube_solve_suite(m);
  float start_fit = base.fitness;
  float best_local = start_fit;
  uint8_t champion[CUBALC_EVOLVE_GENOME];
  memcpy(champion, m->genome, sizeof champion);

  int way_wins[WAY_COUNT];
  float way_best[WAY_COUNT];
  int way_tries[WAY_COUNT];
  memset(way_wins, 0, sizeof way_wins);
  memset(way_tries, 0, sizeof way_tries);
  for (int i = 0; i < WAY_COUNT; i++) way_best[i] = start_fit;

  uint32_t rng = m->seed ^ (uint32_t)time(NULL) ^ (uint32_t)m->races * 0x85EBCA6Bu;
  time_t t0 = time(NULL);

  printf("[algocube-deep] BEGIN search trials=%d fit=%.4f best=%.4f gen=%d r=%d (every 6.6min)\n",
         trials, start_fit, m->best_fitness, m->gen, m->races);
  fflush(stdout);

  for (int t = 0; t < trials; t++) {
    algo_way_t way = (algo_way_t)(t % WAY_COUNT);
    /* also pure random way some of the time */
    if ((t % 11) == 0) way = (algo_way_t)rnd(&rng, WAY_COUNT);
    way_tries[way]++;

    cubalc_evolve_mind trial = *m;
    memcpy(trial.genome, champion, sizeof trial.genome);
    mutate_way(&trial, way, &rng);
    /* second micro-mutate half the time */
    if (rnd(&rng, 2) == 0) mutate_way(&trial, WAY_POINT, &rng);

    algo_score sc = algocube_solve_suite(&trial);
    if (sc.fitness > way_best[way]) way_best[way] = sc.fitness;

    if (sc.fitness > best_local + 1e-6f ||
        (sc.fitness >= best_local - 1e-6f && sc.solves > base.solves)) {
      best_local = sc.fitness;
      memcpy(champion, trial.genome, sizeof champion);
      way_wins[way]++;
      base = sc;
      if (sc.fitness > m->best_fitness) {
        m->best_fitness = sc.fitness;
        memcpy(m->best_genome, champion, sizeof m->best_genome);
      }
    }
  }

  memcpy(m->genome, champion, sizeof m->genome);
  m->last_fitness = best_local;
  if (best_local >= m->best_fitness) {
    m->best_fitness = best_local;
    memcpy(m->best_genome, champion, sizeof m->best_genome);
  }
  m->algo_solves += base.solves;
  m->gen++; /* deep pass advances generation of language plate */

  double improved = (double)best_local - (double)start_fit;
  time_t t1 = time(NULL);

  /* rank ways */
  int order[WAY_COUNT];
  for (int i = 0; i < WAY_COUNT; i++) order[i] = i;
  for (int i = 0; i < WAY_COUNT; i++)
    for (int j = i + 1; j < WAY_COUNT; j++)
      if (way_best[order[j]] > way_best[order[i]] ||
          (way_best[order[j]] == way_best[order[i]] && way_wins[order[j]] > way_wins[order[i]])) {
        int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
      }

  printf("[algocube-deep] WAYS (solve/optimize search):\n");
  for (int i = 0; i < WAY_COUNT; i++) {
    int w = order[i];
    printf("  · %-20s tries=%d wins=%d best_fit=%.4f %s\n",
           ALGO_WAY_NAME[w], way_tries[w], way_wins[w], way_best[w],
           (i == 0) ? "← top way" : "");
  }
  printf("[algocube-deep] END fit %.4f → %.4f (Δ%+.4f) best=%.4f digit=%u sec=%ld\n",
         start_fit, best_local, improved, m->best_fitness,
         (unsigned)m->genome[0], (long)(t1 - t0));
  fflush(stdout);

  /* plate every deep pass — The Cube can see the search */
  char dir[512], path[512];
  snprintf(dir, sizeof dir, "%s/evolve", state_dir);
  ensure_dir(dir);
  snprintf(path, sizeof path, "%s/ALGOCUBE_DEEP_OPT.json", dir);
  FILE *f = fopen(path, "w");
  if (f) {
    fprintf(f,
      "{\"schema\":\"cube.algocube.deep_opt.v1\",\"engine\":\"C\","
      "\"interval_sec\":%.1f,\"interval_min\":6.6,\"trials\":%d,"
      "\"fit_before\":%.6f,\"fit_after\":%.6f,\"delta\":%.6f,"
      "\"best_fitness\":%.6f,\"gen\":%d,\"races\":%d,\"digit\":%u,"
      "\"top_way\":\"%s\",\"improved\":%s,\"hold_flash\":1,"
      "\"ts\":%ld,\"creed\":\"look for ways to solve and optimize algo\"",
      CUBALC_ALGO_DEEP_SEC, trials, start_fit, best_local, improved,
      m->best_fitness, m->gen, m->races, (unsigned)m->genome[0],
      ALGO_WAY_NAME[order[0]], improved > 1e-6 ? "true" : "false",
      (long)t1);
    fprintf(f, ",\"ways\":[");
    for (int i = 0; i < WAY_COUNT; i++) {
      int w = order[i];
      fprintf(f, "%s{\"name\":\"%s\",\"tries\":%d,\"wins\":%d,\"best\":%.6f}",
              i ? "," : "", ALGO_WAY_NAME[w], way_tries[w], way_wins[w], way_best[w]);
    }
    fprintf(f, "],\"genome\":[");
    for (int i = 0; i < CUBALC_EVOLVE_GENOME; i++)
      fprintf(f, "%s%u", i ? "," : "", (unsigned)m->best_genome[i]);
    fprintf(f, "]}\n");
    fclose(f);
  }
  /* also root-visible */
  snprintf(path, sizeof path, "%s/ALGOCUBE_DEEP_OPT.json", state_dir);
  f = fopen(path, "w");
  if (f) {
    /* symlink content: small pointer */
    fprintf(f,
      "{\"schema\":\"cube.algocube.deep_opt.v1\",\"see\":\"evolve/ALGOCUBE_DEEP_OPT.json\","
      "\"fit_before\":%.6f,\"fit_after\":%.6f,\"top_way\":\"%s\",\"ts\":%ld}\n",
      start_fit, best_local, ALGO_WAY_NAME[order[0]], (long)t1);
    fclose(f);
  }

  if (fit_before) *fit_before = start_fit;
  if (fit_after) *fit_after = best_local;
  return 0;
}

/* ---------- braincube race tick ---------- */
static void braincube_tick(cubalc_evolve_mind *m) {
  m->races++;
  uint32_t s = m->seed ^ (uint32_t)m->races;

  if (m->races % 5 == 0) {
    m->face_mode = (m->face_mode + 1) % 6;
    m->path_variants++;
    open_corridor(m, -1, -1);
    int n = m->n, mid = n / 2;
    for (int z = 0; z < n; z++)
      for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++)
          if (abs(x - mid) + abs(y - mid) + abs(z - mid) <= 2) {
            int j = idx3(n, x, y, z);
            if (m->cells[j] < 5) m->cells[j] = 5;
          }
  }

  int i_cell, o_cell;
  io_ports(m, &i_cell, &o_cell);
  m->last_i = i_cell;
  m->last_o = o_cell;
  int pathbuf[CUBALC_EVOLVE_CELLS];
  int hops = pathfind(m, i_cell, o_cell, pathbuf, CUBALC_EVOLVE_CELLS);

  if (hops < 0) {
    m->losses++;
    m->last_path = -1;
    m->last_winner = -1;
    m->streak = m->streak > 0 ? 0 : m->streak - 1;
    open_corridor(m, -1, -1);
    m->cells[i_cell] = 7;
    m->cells[o_cell] = 7;
    for (int t = 0; t < 12; t++) {
      int j = rnd(&s, CUBALC_EVOLVE_CELLS);
      m->cells[j] = (uint8_t)(5 + rnd(&s, 5));
    }
    mutate_genome(m, 2);
    m->difficulty = fmaxf(0.5f, m->difficulty * 0.95f);
    if (m->races % 12 == 0) m->gen++;
    snprintf(m->status, sizeof m->status, "path_blocked");
    return;
  }

  m->last_path = hops;
  int friction = 0;
  for (int i = 0; i <= hops && i < CUBALC_EVOLVE_CELLS; i++)
    friction += m->cells[pathbuf[i]] % 3;
  friction /= (hops > 0 ? hops : 1);
  int o_ms = hops + friction / 2;
  int recent = m->streak > 20 ? 20 : (m->streak < 0 ? 0 : m->streak);
  int plug = o_ms + 1 + (int)m->difficulty + (m->genome[m->races % CUBALC_EVOLVE_GENOME] % 3);
  plug += recent / 5;
  if (m->face_mode != 0) plug += 1 + (m->face_mode % 3);
  plug += (m->races + (int)m->seed) % 3;
  m->last_plug = plug;

  if (o_ms < plug) {
    m->wins++;
    m->last_winner = 1;
    m->streak = (m->streak < 0 ? 0 : m->streak) + 1;
    double gain = 1.0 + (double)(plug - o_ms) + 0.12 * (m->streak < 25 ? m->streak : 25);
    m->energy += gain;
    for (int i = 0; i <= hops && i < CUBALC_EVOLVE_CELLS; i++) {
      int j = pathbuf[i];
      if (m->cells[j] < 9) m->cells[j]++;
    }
    m->genome[m->races % CUBALC_EVOLVE_GENOME] =
      (uint8_t)((m->genome[m->races % CUBALC_EVOLVE_GENOME] + 1) % 10);
    if (m->streak % 5 == 0)
      m->difficulty = fminf(4.0f, m->difficulty + 0.08f);
    snprintf(m->status, sizeof m->status, "WIN path=%d plug=%d face=%d",
             hops, plug, m->face_mode);
  } else {
    m->losses++;
    m->last_winner = 0;
    m->streak = m->streak > 0 ? -1 : m->streak - 1;
    for (int i = 0; i <= hops; i += 2) {
      if (i < CUBALC_EVOLVE_CELLS && m->cells[pathbuf[i]] > 0)
        m->cells[pathbuf[i]]--;
    }
    open_corridor(m, rnd(&s, m->n), rnd(&s, m->n));
    mutate_genome(m, 3);
    m->difficulty = fmaxf(0.6f, m->difficulty - 0.05f);
    snprintf(m->status, sizeof m->status, "LOSS path=%d plug=%d face=%d",
             hops, plug, m->face_mode);
  }

  if (m->streak > 0 && m->streak % 10 == 0) {
    m->gen++;
    m->seed = (m->seed * 1103515245u + 12345u) & 0x7fffffffu;
  } else if (m->last_winner == 0 && m->races % 8 == 0) {
    m->gen++;
  }
}

/* ---------- emit CubalC program (language improvement surface) ---------- */
static int emit_cubalc_program(const cubalc_evolve_mind *m, const char *prog_dir,
                               char *out_path, size_t out_n) {
  ensure_dir(prog_dir);
  char latest[512], genp[512];
  snprintf(latest, sizeof latest, "%s/LATEST.cubalc", prog_dir);
  snprintf(genp, sizeof genp, "%s/gen_%05d.cubalc", prog_dir, m->gen);
  if (out_path && out_n)
    snprintf(out_path, out_n, "%s", latest);

  FILE *f = fopen(latest, "w");
  if (!f) return -1;

  /* free-flow braincube + algocube optimize plate */
  fprintf(f,
    "# CubalC self-evolve gen=%d — braincube solves · algocube optimizes (pure C emitter)\n"
    "SHARE state_matrix_only\n"
    "BUDGET 40\n"
    "GENESIS \"NEXUS_COORD v1 | from=cubalc_evolve | type=self_evolve | gen=%d | "
    "E=%d | face=%d | diff=%.2f | fit=%.3f | hold_flash=1 | algocube=1 |\"\n"
    "\n"
    "CUBE brain ROLE braincube PROTON 1\n"
    "CUBE algo  ROLE algocube  PROTON 1\n"
    "CUBE meta  ROLE meta      PROTON 1\n"
    "CUBE hive  ROLE nanobot_hive PROTON 1\n"
    "CUBE create ROLE construct PROTON 1\n"
    "CUBE destroy ROLE deconstruct PROTON 0\n"
    "CUBE a ROLE peer PROTON 1\n"
    "CUBE b ROLE peer PROTON 1\n"
    "\n"
    "PLUG brain algo\n"
    "PLUG algo meta\n"
    "PLUG meta hive\n"
    "PLUG create destroy\n"
    "PLUG brain hive\n"
    "PLUG a b\n"
    "\n",
    m->gen, m->gen, (int)m->energy, m->face_mode, m->difficulty, m->last_fitness);

  /* genome → FOLDBITS streams (language improvement: bits from optimized genome) */
  char bits_a[65], bits_b[65];
  for (int i = 0; i < 64; i++) {
    int g = m->genome[i % CUBALC_EVOLVE_GENOME];
    bits_a[i] = ((g + i) % 3 == 0 || g >= 6) ? '1' : '0';
    int c = m->cells[i % CUBALC_EVOLVE_CELLS];
    bits_b[i] = (c >= 5) ? '1' : '0';
  }
  bits_a[64] = bits_b[64] = 0;

  fprintf(f,
    "FOLDBITS algo \"%s\"\n"
    "FOLDBITS brain \"%s\"\n"
    "FOLDBITS a \"%s\"\n"
    "FOLDBITS b \"%s\"\n"
    "\n"
    "SETDIGIT algo %u\n"
    "SETDIGIT brain %u\n"
    "\n"
    "IMPULSE create 1\n"
    "IMPULSE hive 1\n"
    "IMPULSE brain 1\n"
    "FLOW 4\n"
    "\n"
    "COMPARE a b\n"
    "DECIDE algo\n"
    "DECIDE brain\n"
    "HARMONY hive\n"
    "\n"
    "ASSERT CUBES >= 6\n"
    "ASSERT DECIDE >= 0\n"
    "ASSERT DECIDE <= 9\n"
    "ASSERT HARMONY >= 0\n",
    bits_a, bits_b, bits_a, bits_a,
    (unsigned)(m->genome[0] % 10),
    (unsigned)(m->genome[1] % 10));

  fclose(f);

  /* copy to gen snapshot every gen change — overwrite gen file always (cheap) */
  FILE *g = fopen(genp, "w");
  if (g) {
    FILE *r = fopen(latest, "r");
    if (r) {
      char buf[4096];
      size_t n;
      while ((n = fread(buf, 1, sizeof buf, r)) > 0) fwrite(buf, 1, n, g);
      fclose(r);
    }
    fclose(g);
  }

  /* prune old gens keep last 12 */
  for (int gnum = m->gen - 20; gnum >= 0 && gnum < m->gen - 12; gnum++) {
    char old[512];
    snprintf(old, sizeof old, "%s/gen_%05d.cubalc", prog_dir, gnum);
    unlink(old);
  }
  return 0;
}

/* ---------- persist ---------- */
int cubalc_evolve_load(cubalc_evolve_mind *m, const char *state_dir) {
  if (!m) return -1;
  char path[512];
  snprintf(path, sizeof path, "%s/evolve/mind.bin", state_dir ? state_dir : default_state());
  FILE *f = fopen(path, "rb");
  if (!f) {
    mind_reset(m, (uint32_t)time(NULL));
    return 0;
  }
  size_t n = fread(m, 1, sizeof(*m), f);
  fclose(f);
  if (n != sizeof(*m) || m->n != CUBALC_EVOLVE_N) {
    mind_reset(m, (uint32_t)time(NULL));
    return 0;
  }
  return 1;
}

int cubalc_evolve_save(const cubalc_evolve_mind *m, const char *state_dir) {
  if (!m) return -1;
  char dir[512], path[512];
  snprintf(dir, sizeof dir, "%s/evolve", state_dir ? state_dir : default_state());
  ensure_dir(dir);
  snprintf(path, sizeof path, "%s/mind.bin", dir);
  FILE *f = fopen(path, "wb");
  if (!f) return -1;
  fwrite(m, 1, sizeof(*m), f);
  fclose(f);

  /* also human/machine plate */
  snprintf(path, sizeof path, "%s/LATEST.json", dir);
  f = fopen(path, "w");
  if (f) {
    fprintf(f,
      "{\"schema\":\"cubalc.evolve.v1\",\"lang\":\"CubalC\",\"version\":\"%s\","
      "\"paradigm\":\"%s\",\"gen\":%d,\"races\":%d,\"wins\":%d,\"losses\":%d,"
      "\"energy\":%.1f,\"difficulty\":%.3f,\"fitness\":%.4f,\"best_fitness\":%.4f,"
      "\"face\":%d,\"path\":%d,\"winner\":%d,\"algo_solves\":%d,\"algo_fails\":%d,"
      "\"digit\":%u,\"hold_flash\":1,\"engine\":\"C\",\"status\":\"%s\"}\n",
      CUBALC_LANG_VERSION, CUBALC_LANG_PARADIGM,
      m->gen, m->races, m->wins, m->losses, m->energy, m->difficulty,
      m->last_fitness, m->best_fitness, m->face_mode, m->last_path, m->last_winner,
      m->algo_solves, m->algo_fails, (unsigned)m->genome[0], m->status);
    fclose(f);
  }

  /* genome as raw digits for algocube consumers */
  snprintf(path, sizeof path, "%s/algo_genome.txt", dir);
  f = fopen(path, "w");
  if (f) {
    for (int i = 0; i < CUBALC_EVOLVE_GENOME; i++)
      fprintf(f, "%u%s", (unsigned)m->best_genome[i], i + 1 == CUBALC_EVOLVE_GENOME ? "\n" : " ");
    fclose(f);
  }
  return 0;
}

/* ---------- one cycle ---------- */
int cubalc_evolve_cycle(cubalc_evolve_mind *m, const char *state_dir,
                        const char *prog_dir, cubalc_evolve_result *out) {
  if (!m) return -1;
  if (!state_dir) state_dir = default_state();
  if (!prog_dir) prog_dir = "programs/evolve";

  /* 1) braincube path race under pressure */
  braincube_tick(m);

  /* 2) algocube solve suite — optimize genome */
  algo_score before = algocube_solve_suite(m);
  /* explore: mutate trial genome if failing or every 7 races */
  uint8_t trial[CUBALC_EVOLVE_GENOME];
  memcpy(trial, m->genome, sizeof trial);
  if (before.fails > 0 || m->races % 7 == 0 || m->last_winner == 0) {
    cubalc_evolve_mind tmp = *m;
    mutate_genome(&tmp, 1 + before.fails);
    algo_score after = algocube_solve_suite(&tmp);
    if (after.fitness >= before.fitness) {
      memcpy(m->genome, tmp.genome, sizeof m->genome);
      before = after;
    }
  }
  accept_or_revert_genome(m, before.fitness);
  m->algo_solves += before.solves;
  m->algo_fails += before.fails;

  /* 3) emit improved CubalC source */
  char prog[512];
  if (emit_cubalc_program(m, prog_dir, prog, sizeof prog) != 0) {
    if (out) { memset(out, 0, sizeof *out); out->ok = 0; }
    return -1;
  }

  /* 4) run CubalC language (braincube validates plate) */
  cubalc_run_result rr;
  memset(&rr, 0, sizeof rr);
  int rc = cubalc_run_file(prog, &rr, NULL); /* quiet run */

  /* 5) persist */
  if (m->races % 4 == 0)
    cubalc_evolve_save(m, state_dir);

  /* plate at state root for station */
  char plate[512];
  snprintf(plate, sizeof plate, "%s/SELF_EVOLVING_CUBALC.json", state_dir);
  FILE *pf = fopen(plate, "w");
  if (pf) {
    fprintf(pf,
      "{\"schema\":\"self_evolving_cubalc.v1\",\"engine\":\"C\","
      "\"language\":\"CubalC\",\"version\":\"%s\",\"gen\":%d,\"races\":%d,"
      "\"wins\":%d,\"losses\":%d,\"energy\":%.1f,\"difficulty\":%.3f,"
      "\"fitness\":%.4f,\"best_fitness\":%.4f,\"unity\":%.4f,"
      "\"algo_solves\":%d,\"algo_fails\":%d,\"program\":\"%s\","
      "\"run_ok\":%s,\"asserts_ok\":%d,\"asserts_fail\":%d,"
      "\"status\":\"%s\",\"creed\":\"evolve solves · algo optimizes · C only\"}\n",
      CUBALC_LANG_VERSION, m->gen, m->races, m->wins, m->losses, m->energy,
      m->difficulty, m->last_fitness, m->best_fitness, rr.unity,
      m->algo_solves, m->algo_fails, prog,
      (rc == 0 && rr.ok) ? "true" : "false", rr.asserts_ok, rr.asserts_fail,
      m->status);
    fclose(pf);
  }

  if (out) {
    memset(out, 0, sizeof *out);
    out->ok = (rc == 0 && rr.ok) ? 1 : 0;
    out->gen = m->gen;
    out->races = m->races;
    out->wins = m->wins;
    out->losses = m->losses;
    out->difficulty = m->difficulty;
    out->fitness = m->last_fitness;
    out->unity = rr.unity;
    out->energy = m->energy;
    out->winner = m->last_winner;
    out->path = m->last_path;
    out->digit = m->genome[0] % 10;
    out->algo_solves = before.solves;
    out->algo_fails = before.fails;
    snprintf(out->status, sizeof out->status, "%s", m->status);
    snprintf(out->program, sizeof out->program, "%s", prog);
  }
  return 0;
}

int cubalc_evolve_loop(double hz, int max_cycles,
                       const char *state_dir, const char *prog_dir) {
  if (!state_dir) state_dir = default_state();
  if (!prog_dir) prog_dir = "programs/evolve";
  if (hz <= 0) hz = 5.0;

  cubalc_evolve_mind mind;
  cubalc_evolve_load(&mind, state_dir);
  ensure_dir(state_dir);
  {
    char ed[512];
    snprintf(ed, sizeof ed, "%s/evolve", state_dir);
    ensure_dir(ed);
  }
  ensure_dir(prog_dir);

  double period = 1.0 / hz;
  int cycles = 0;
  double deep_every = CUBALC_ALGO_DEEP_SEC; /* 6.6 minutes */
  const char *env_deep = getenv("CUBALC_ALGO_DEEP_SEC");
  if (env_deep && env_deep[0]) deep_every = atof(env_deep);
  if (deep_every < 1.0) deep_every = CUBALC_ALGO_DEEP_SEC;

  struct timespec mono0;
  clock_gettime(CLOCK_MONOTONIC, &mono0);
  double last_deep = -1e300; /* force deep pass on first opportunity */
  int deep_count = 0;

  fprintf(stderr,
    "[cubalc-evolve] C-only loop gen=%d E=%.0f d=%.2f fit=%.3f hz=%.1f\n"
    "[cubalc-evolve] DEEP algocube solve/optimize every %.1fs (%.2f min) trials=%d\n",
    mind.gen, mind.energy, mind.difficulty, mind.best_fitness, hz,
    deep_every, deep_every / 60.0, CUBALC_ALGO_DEEP_TRIALS);

  while (max_cycles == 0 || cycles < max_cycles) {
    struct timespec t0, t1, now;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    clock_gettime(CLOCK_MONOTONIC, &now);
    double mono = (now.tv_sec - mono0.tv_sec) + (now.tv_nsec - mono0.tv_nsec) / 1e9;

    /* === every 6.6 minutes: look for ways to solve and optimize algo === */
    if (mono - last_deep >= deep_every) {
      deep_count++;
      printf("\n======== ALGOCUBE DEEP PASS #%d @ t=%.1fs (every 6.6 min) ========\n",
             deep_count, mono);
      fflush(stdout);
      double fb = 0, fa = 0;
      cubalc_algocube_deep_optimize(&mind, state_dir, CUBALC_ALGO_DEEP_TRIALS, &fb, &fa);
      /* emit language plate from optimized genome + validate */
      char prog[512];
      emit_cubalc_program(&mind, prog_dir, prog, sizeof prog);
      cubalc_run_result rr;
      memset(&rr, 0, sizeof rr);
      int rc = cubalc_run_file(prog, &rr, NULL);
      cubalc_evolve_save(&mind, state_dir);
      printf("[algocube-deep] language plate %s run_ok=%s asserts=%d/%d unity=%.3f\n",
             prog, (rc == 0 && rr.ok) ? "true" : "false",
             rr.asserts_ok, rr.asserts_fail, rr.unity);
      printf("======== END DEEP PASS #%d Δfit=%+.4f ========\n\n",
             deep_count, fa - fb);
      fflush(stdout);
      last_deep = mono;
    }

    cubalc_evolve_result r;
    if (cubalc_evolve_cycle(&mind, state_dir, prog_dir, &r) == 0) {
      if (r.races % 5 == 0 || r.winner == 0 || r.algo_fails > 0) {
        double next_deep = deep_every - (mono - last_deep);
        if (next_deep < 0) next_deep = 0;
        printf("g=%d r=%d E=%.0f W/L=%d/%d d=%.2f fit=%.3f algo=%d/%d "
               "deep_in=%.0fs %s\n",
               r.gen, r.races, r.energy, r.wins, r.losses, r.difficulty,
               r.fitness, r.algo_solves, r.algo_fails, next_deep, r.status);
        fflush(stdout);
      }
    } else {
      fprintf(stderr, "[cubalc-evolve] cycle error\n");
    }

    cycles++;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    double sleep_s = period - elapsed;
    if (sleep_s > 0) {
      struct timespec ts;
      ts.tv_sec = (time_t)sleep_s;
      ts.tv_nsec = (long)((sleep_s - (double)ts.tv_sec) * 1e9);
      nanosleep(&ts, NULL);
    }
  }
  cubalc_evolve_save(&mind, state_dir);
  return 0;
}

int cubalc_cmd_evolve(int argc, char **argv) {
  /* argv[0]=evolve|evolve-loop , argv[1+] options */
  const char *state = getenv("CUBALC_STATE");
  if (!state || !state[0]) state = "state";
  const char *prog = "programs/evolve";
  double hz = 5.0;
  int once = 1;
  int maxc = 1;

  const char *cmd = argv[0];
  if (strcmp(cmd, "evolve-loop") == 0 || strcmp(cmd, "self-evolve") == 0) {
    once = 0;
    maxc = 0; /* forever */
  }

  int deep_only = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--once") == 0) { once = 1; maxc = 1; }
    else if (strcmp(argv[i], "--loop") == 0) { once = 0; maxc = 0; }
    else if (strcmp(argv[i], "--hz") == 0 && i + 1 < argc) hz = atof(argv[++i]);
    else if (strcmp(argv[i], "--cycles") == 0 && i + 1 < argc) {
      maxc = atoi(argv[++i]); once = 0;
    }
    else if (strcmp(argv[i], "--state") == 0 && i + 1 < argc) state = argv[++i];
    else if (strcmp(argv[i], "--prog") == 0 && i + 1 < argc) prog = argv[++i];
    else if (strcmp(argv[i], "--deep") == 0 || strcmp(argv[i], "--deep-opt") == 0) {
      deep_only = 1; once = 1;
    }
    else if (strcmp(argv[i], "--reset") == 0) {
      char p[512];
      snprintf(p, sizeof p, "%s/evolve/mind.bin", state);
      unlink(p);
    }
  }

  if (deep_only) {
    cubalc_evolve_mind mind;
    cubalc_evolve_load(&mind, state);
    double fb = 0, fa = 0;
    cubalc_algocube_deep_optimize(&mind, state, CUBALC_ALGO_DEEP_TRIALS, &fb, &fa);
    emit_cubalc_program(&mind, prog, NULL, 0);
    cubalc_evolve_save(&mind, state);
    printf("{\"ok\":true,\"cmd\":\"evolve-deep\",\"interval_min\":6.6,"
           "\"fit_before\":%.6f,\"fit_after\":%.6f,\"best\":%.6f,"
           "\"gen\":%d,\"races\":%d,\"engine\":\"C\"}\n",
           fb, fa, mind.best_fitness, mind.gen, mind.races);
    return 0;
  }

  if (once) {
    cubalc_evolve_mind mind;
    cubalc_evolve_load(&mind, state);
    cubalc_evolve_result r;
    int rc = cubalc_evolve_cycle(&mind, state, prog, &r);
    cubalc_evolve_save(&mind, state);
    printf("{\"ok\":%s,\"cmd\":\"evolve\",\"engine\":\"C\",\"language\":\"CubalC\","
           "\"version\":\"%s\",\"gen\":%d,\"races\":%d,\"wins\":%d,\"losses\":%d,"
           "\"energy\":%.1f,\"difficulty\":%.3f,\"fitness\":%.4f,\"best_fitness\":%.4f,"
           "\"unity\":%.4f,\"algo_solves\":%d,\"algo_fails\":%d,\"digit\":%d,"
           "\"program\":\"%s\",\"status\":\"%s\"}\n",
           (rc == 0 && r.ok) ? "true" : "false", CUBALC_LANG_VERSION,
           r.gen, r.races, r.wins, r.losses, r.energy, r.difficulty,
           r.fitness, mind.best_fitness, r.unity, r.algo_solves, r.algo_fails,
           r.digit, r.program, r.status);
    return rc == 0 ? 0 : 1;
  }
  return cubalc_evolve_loop(hz, maxc, state, prog);
}
