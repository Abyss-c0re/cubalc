#define _POSIX_C_SOURCE 200809L
#include "cubalc_async.h"
#include "cubalc_hw.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <sys/wait.h>

/* Optional OpenCL — compile with CUBALC_HAVE_OPENCL */
#if defined(CUBALC_HAVE_OPENCL)
#ifndef CL_TARGET_OPENCL_VERSION
#define CL_TARGET_OPENCL_VERSION 300
#endif
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#endif

typedef struct {
  pthread_t *th;
  int n_workers;
  int stop;
  pthread_mutex_t mu;
  pthread_cond_t cv;
  pthread_cond_t done_cv;
  cubalc_async_job jobs[CUBALC_ASYNC_MAX_JOBS];
  int q[CUBALC_ASYNC_MAX_JOBS];
  int qh, qt, qn;
  int next_id;
  int gpu_ok;
  char backend[64];
} pool_t;

static pool_t G;
static int G_inited = 0;

static void job_clear(cubalc_async_job *j) {
  free(j->body);
  memset(j, 0, sizeof *j);
}

static long mono_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long)ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

/* ---- OpenCL probe (popcount-friendly path placeholder) ---- */
static int gpu_probe(void) {
#if defined(CUBALC_HAVE_OPENCL)
  cl_uint nplat = 0;
  if (clGetPlatformIDs(0, NULL, &nplat) != CL_SUCCESS || nplat == 0) return 0;
  cl_platform_id plat;
  if (clGetPlatformIDs(1, &plat, NULL) != CL_SUCCESS) return 0;
  cl_uint ndev = 0;
  if (clGetDeviceIDs(plat, CL_DEVICE_TYPE_GPU, 0, NULL, &ndev) != CL_SUCCESS || ndev == 0) {
    /* accept CPU OpenCL as accel too */
    if (clGetDeviceIDs(plat, CL_DEVICE_TYPE_ALL, 0, NULL, &ndev) != CL_SUCCESS || ndev == 0)
      return 0;
  }
  return 1;
#else
  return 0;
#endif
}

/* GPU-shaped packed compat: multi-thread CPU always; marks backend */
static void compat_worker_range(const cubalc_matrix *mats, int n, float *out,
                                int i0, int i1) {
  for (int i = i0; i < i1; i++)
    for (int j = 0; j < n; j++)
      out[i * n + j] = cubalc_matrix_compat(&mats[i], &mats[j]);
}

typedef struct {
  const cubalc_matrix *mats;
  int n;
  float *out;
  int i0, i1;
} compat_args_t;

static void *compat_th(void *arg) {
  compat_args_t *a = arg;
  compat_worker_range(a->mats, a->n, a->out, a->i0, a->i1);
  return NULL;
}

/* run HTTP using hostops with extended timeout via env for curl -m */
static void run_http_job(cubalc_async_job *j) {
  /* hostops uses fixed -m 8; we reimplement timed curl here for async jobs */
  char tmpout[] = "/tmp/cubalc_async_http_XXXXXX";
  int fd = mkstemp(tmpout);
  if (fd < 0) {
    j->state = CUBALC_JOB_FAILED;
    snprintf(j->err, sizeof j->err, "async http tmp");
    return;
  }
  close(fd);
  int timeout_s = j->timeout_ms > 0 ? (j->timeout_ms + 999) / 1000 : 60;
  if (timeout_s < 5) timeout_s = 5;
  if (timeout_s > 600) timeout_s = 600;
  char mflag[16];
  snprintf(mflag, sizeof mflag, "%d", timeout_s);

  pid_t pid = fork();
  if (pid < 0) {
    unlink(tmpout);
    j->state = CUBALC_JOB_FAILED;
    snprintf(j->err, sizeof j->err, "async http fork");
    return;
  }
  if (pid == 0) {
    int out = open(tmpout, O_WRONLY | O_TRUNC);
    if (out >= 0) { dup2(out, 1); dup2(out, 2); close(out); }
    if (j->body && j->body[0] &&
        (strcmp(j->method, "POST") == 0 || strcmp(j->method, "PUT") == 0)) {
      execlp("curl", "curl", "-sS", "-m", mflag, "-X", j->method,
             "-H", "Content-Type: application/json",
             "-d", j->body, "-w", "\n__HTTP_CODE__%{http_code}",
             j->url, (char *)NULL);
    } else {
      execlp("curl", "curl", "-sS", "-m", mflag, "-X", j->method,
             "-w", "\n__HTTP_CODE__%{http_code}",
             j->url, (char *)NULL);
    }
    _exit(127);
  }
  int st = 0;
  waitpid(pid, &st, 0);
  FILE *f = fopen(tmpout, "rb");
  if (!f) {
    unlink(tmpout);
    j->state = CUBALC_JOB_FAILED;
    snprintf(j->err, sizeof j->err, "async http no out");
    return;
  }
  size_t n = fread(j->str, 1, sizeof j->str - 1, f);
  j->str[n] = 0;
  fclose(f);
  unlink(tmpout);
  char *p = strstr(j->str, "__HTTP_CODE__");
  if (p) {
    j->code = atoi(p + 13);
    *p = 0;
    j->n = (long)strlen(j->str);
  } else {
    j->code = WIFEXITED(st) && WEXITSTATUS(st) == 0 ? 200 : 0;
    j->n = (long)n;
  }
  j->ok = (j->code >= 200 && j->code < 400) ? 1 : 0;
  j->state = j->ok ? CUBALC_JOB_DONE : CUBALC_JOB_FAILED;
  if (!j->ok)
    snprintf(j->err, sizeof j->err, "http: code %d", j->code);
}

static void *worker_main(void *arg) {
  (void)arg;
  for (;;) {
    cubalc_async_job *job = NULL;
    pthread_mutex_lock(&G.mu);
    while (!G.stop && G.qn == 0)
      pthread_cond_wait(&G.cv, &G.mu);
    if (G.stop && G.qn == 0) {
      pthread_mutex_unlock(&G.mu);
      break;
    }
    int idx = G.q[G.qh];
    G.qh = (G.qh + 1) % CUBALC_ASYNC_MAX_JOBS;
    G.qn--;
    job = &G.jobs[idx];
    job->state = CUBALC_JOB_RUNNING;
    pthread_mutex_unlock(&G.mu);

    if (job->kind == CUBALC_JOB_HTTP) {
      run_http_job(job);
      free(job->body);
      job->body = NULL;
    } else {
      job->state = CUBALC_JOB_FAILED;
      snprintf(job->err, sizeof job->err, "unknown job kind");
    }

    pthread_mutex_lock(&G.mu);
    pthread_cond_broadcast(&G.done_cv);
    pthread_mutex_unlock(&G.mu);
  }
  return NULL;
}

int cubalc_async_init(int n_workers) {
  if (G_inited) return 0;
  memset(&G, 0, sizeof G);
  pthread_mutex_init(&G.mu, NULL);
  pthread_cond_init(&G.cv, NULL);
  pthread_cond_init(&G.done_cv, NULL);
  long nc = sysconf(_SC_NPROCESSORS_ONLN);
  if (nc < 1) nc = 2;
  if (n_workers <= 0) n_workers = (int)(nc > 8 ? 8 : nc);
  if (n_workers < 1) n_workers = 1;
  if (n_workers > CUBALC_ASYNC_MAX_WORKERS) n_workers = CUBALC_ASYNC_MAX_WORKERS;
  G.n_workers = n_workers;
  G.gpu_ok = gpu_probe();
  snprintf(G.backend, sizeof G.backend, "cpu:%d%s",
           G.n_workers, G.gpu_ok ? "+gpu:opencl" : "");
  G.th = calloc((size_t)G.n_workers, sizeof(pthread_t));
  if (!G.th) return -1;
  G.next_id = 1;
  G_inited = 1;
  for (int i = 0; i < G.n_workers; i++)
    pthread_create(&G.th[i], NULL, worker_main, NULL);
  return 0;
}

void cubalc_async_shutdown(void) {
  if (!G_inited) return;
  pthread_mutex_lock(&G.mu);
  G.stop = 1;
  pthread_cond_broadcast(&G.cv);
  pthread_mutex_unlock(&G.mu);
  for (int i = 0; i < G.n_workers; i++)
    pthread_join(G.th[i], NULL);
  free(G.th);
  for (int i = 0; i < CUBALC_ASYNC_MAX_JOBS; i++)
    free(G.jobs[i].body);
  pthread_mutex_destroy(&G.mu);
  pthread_cond_destroy(&G.cv);
  pthread_cond_destroy(&G.done_cv);
  memset(&G, 0, sizeof G);
  G_inited = 0;
}

int cubalc_async_workers(void) {
  if (!G_inited) cubalc_async_init(0);
  return G.n_workers;
}

int cubalc_async_gpu_ok(void) {
  if (!G_inited) cubalc_async_init(0);
  return G.gpu_ok;
}

const char *cubalc_async_backend(void) {
  if (!G_inited) cubalc_async_init(0);
  return G.backend;
}

static int enqueue_slot(int slot) {
  if (G.qn >= CUBALC_ASYNC_MAX_JOBS) return -1;
  G.q[G.qt] = slot;
  G.qt = (G.qt + 1) % CUBALC_ASYNC_MAX_JOBS;
  G.qn++;
  pthread_cond_signal(&G.cv);
  return 0;
}

int cubalc_async_http(const char *method, const char *url, const char *body,
                      int timeout_ms) {
  if (!G_inited) cubalc_async_init(0);
  /* allowlist same as hostops */
  if (!url || (strncmp(url, "http://127.0.0.1", 16) &&
               strncmp(url, "http://localhost", 16) &&
               strncmp(url, "http://[::1]", 12)))
    return -1;

  pthread_mutex_lock(&G.mu);
  int slot = -1;
  for (int i = 0; i < CUBALC_ASYNC_MAX_JOBS; i++) {
    if (G.jobs[i].state == CUBALC_JOB_IDLE ||
        G.jobs[i].state == CUBALC_JOB_DONE ||
        G.jobs[i].state == CUBALC_JOB_FAILED ||
        G.jobs[i].state == CUBALC_JOB_CANCELLED) {
      free(G.jobs[i].body);
      memset(&G.jobs[i], 0, sizeof G.jobs[i]);
      slot = i;
      break;
    }
  }
  if (slot < 0) {
    pthread_mutex_unlock(&G.mu);
    return -1;
  }
  cubalc_async_job *j = &G.jobs[slot];
  j->id = G.next_id++;
  if (G.next_id > 1000000) G.next_id = 1;
  j->kind = CUBALC_JOB_HTTP;
  j->state = CUBALC_JOB_PENDING;
  snprintf(j->method, sizeof j->method, "%s", method && method[0] ? method : "GET");
  snprintf(j->url, sizeof j->url, "%s", url);
  j->timeout_ms = timeout_ms > 0 ? timeout_ms : 120000;
  if (body && body[0]) {
    j->body = strdup(body);
    if (!j->body) {
      j->state = CUBALC_JOB_IDLE;
      pthread_mutex_unlock(&G.mu);
      return -1;
    }
  }
  int id = j->id;
  if (enqueue_slot(slot) != 0) {
    free(j->body);
    j->body = NULL;
    j->state = CUBALC_JOB_IDLE;
    pthread_mutex_unlock(&G.mu);
    return -1;
  }
  pthread_mutex_unlock(&G.mu);
  return id;
}

int cubalc_async_poll(int job_id, cubalc_async_job *out) {
  if (!G_inited || job_id <= 0) return -1;
  pthread_mutex_lock(&G.mu);
  cubalc_async_job *j = NULL;
  for (int i = 0; i < CUBALC_ASYNC_MAX_JOBS; i++)
    if (G.jobs[i].id == job_id) { j = &G.jobs[i]; break; }
  if (!j) {
    pthread_mutex_unlock(&G.mu);
    return -1;
  }
  int terminal = (j->state == CUBALC_JOB_DONE || j->state == CUBALC_JOB_FAILED ||
                  j->state == CUBALC_JOB_CANCELLED);
  if (out) {
    *out = *j;
    out->body = NULL; /* don't alias heap */
  }
  pthread_mutex_unlock(&G.mu);
  return terminal ? 1 : 0;
}

int cubalc_async_wait(int job_id, int timeout_ms, cubalc_async_job *out) {
  if (!G_inited || job_id <= 0) return -1;
  long deadline = timeout_ms < 0 ? -1 : mono_ms() + timeout_ms;
  pthread_mutex_lock(&G.mu);
  for (;;) {
    cubalc_async_job *j = NULL;
    for (int i = 0; i < CUBALC_ASYNC_MAX_JOBS; i++)
      if (G.jobs[i].id == job_id) { j = &G.jobs[i]; break; }
    if (!j) {
      pthread_mutex_unlock(&G.mu);
      return -1;
    }
    if (j->state == CUBALC_JOB_DONE || j->state == CUBALC_JOB_FAILED ||
        j->state == CUBALC_JOB_CANCELLED) {
      if (out) { *out = *j; out->body = NULL; }
      pthread_mutex_unlock(&G.mu);
      return 0;
    }
    if (deadline >= 0) {
      long now = mono_ms();
      if (now >= deadline) {
        pthread_mutex_unlock(&G.mu);
        return -1;
      }
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      long rem = deadline - now;
      ts.tv_sec += rem / 1000;
      ts.tv_nsec += (rem % 1000) * 1000000L;
      if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000L;
      }
      pthread_cond_timedwait(&G.done_cv, &G.mu, &ts);
    } else {
      pthread_cond_wait(&G.done_cv, &G.mu);
    }
  }
}

int cubalc_async_await_all(int timeout_ms) {
  if (!G_inited) return 0;
  long deadline = timeout_ms < 0 ? -1 : mono_ms() + timeout_ms;
  pthread_mutex_lock(&G.mu);
  for (;;) {
    int busy = 0;
    for (int i = 0; i < CUBALC_ASYNC_MAX_JOBS; i++)
      if (G.jobs[i].state == CUBALC_JOB_PENDING ||
          G.jobs[i].state == CUBALC_JOB_RUNNING)
        busy = 1;
    if (!busy) {
      pthread_mutex_unlock(&G.mu);
      return 0;
    }
    if (deadline >= 0 && mono_ms() >= deadline) {
      pthread_mutex_unlock(&G.mu);
      return -1;
    }
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += 50 * 1000000L;
    if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
    pthread_cond_timedwait(&G.done_cv, &G.mu, &ts);
  }
}

/* ---- Parallel flow: collect undirected edges, multi-thread talk into side buf ---- */
typedef struct {
  int from, to;
  cubalc_atom recv;
  float e_from_delta, e_to_delta;
  int ok;
} edge_job_t;

static void simulate_talk(const cubalc_chain *ch, edge_job_t *ej) {
  const cubalc_cube *A = &ch->cubes[ej->from];
  const cubalc_cube *B = &ch->cubes[ej->to];
  uint8_t buf[256];
  size_t n = 0;
  if (cubalc_bin_pack(&A->atom, A->id, B->id, ch->seq, buf, sizeof buf, &n) != 0) {
    ej->ok = 0;
    return;
  }
  cubalc_atom received;
  char f[32], t[32];
  uint32_t seq = 0;
  if (cubalc_bin_unpack(buf, n, &received, f, t, &seq) != 0) {
    ej->ok = 0;
    return;
  }
  ej->recv = received;
  ej->e_from_delta = 0;
  ej->e_to_delta = 0;
  if (received.proton) {
    float push = A->atom.energy * 0.18f;
    if (push > 0.001f) {
      ej->e_from_delta = -push * 0.5f;
      ej->e_to_delta = push;
    }
  } else {
    ej->e_from_delta = 0.12f * 0.25f;
    ej->e_to_delta = -0.12f;
  }
  ej->ok = 1;
}

typedef struct {
  const cubalc_chain *ch;
  edge_job_t *edges;
  int i0, i1;
} flow_args_t;

static void *flow_th(void *arg) {
  flow_args_t *a = arg;
  for (int i = a->i0; i < a->i1; i++)
    simulate_talk(a->ch, &a->edges[i]);
  return NULL;
}

int cubalc_async_chain_flow(cubalc_chain *ch, int ticks) {
  if (!ch || ch->n_cubes < 1) return -1;
  if (!G_inited) cubalc_async_init(0);
  if (ticks < 1) ticks = 1;
  if (ticks > 1000) ticks = 1000;

  for (int tick = 0; tick < ticks; tick++) {
    edge_job_t edges[CUBALC_MAX_CUBES * CUBALC_MAX_PORTS];
    int ne = 0;
    for (int i = 0; i < ch->n_cubes; i++) {
      cubalc_cube *c = &ch->cubes[i];
      for (int p = 0; p < c->n_ports; p++) {
        int peer = c->ports[p].peer;
        if (peer < 0 || peer <= i) continue; /* undirected once */
        if (ne >= (int)(sizeof edges / sizeof edges[0])) break;
        edges[ne].from = i;
        edges[ne].to = peer;
        edges[ne].ok = 0;
        ne++;
      }
    }
    if (ne == 0) {
      /* still tick unity */
      cubalc_chain_tick(ch);
      continue;
    }

    int nw = G.n_workers;
    if (nw > ne) nw = ne;
    if (nw < 1) nw = 1;
    pthread_t th[CUBALC_ASYNC_MAX_WORKERS];
    flow_args_t args[CUBALC_ASYNC_MAX_WORKERS];
    int chunk = (ne + nw - 1) / nw;
    int nt = 0;
    for (int w = 0; w < nw; w++) {
      int i0 = w * chunk;
      int i1 = i0 + chunk;
      if (i0 >= ne) break;
      if (i1 > ne) i1 = ne;
      args[nt].ch = ch;
      args[nt].edges = edges;
      args[nt].i0 = i0;
      args[nt].i1 = i1;
      if (pthread_create(&th[nt], NULL, flow_th, &args[nt]) != 0) {
        /* fallback serial remainder */
        flow_th(&args[nt]);
      } else {
        nt++;
      }
    }
    for (int w = 0; w < nt; w++)
      pthread_join(th[w], NULL);

    /* merge phase sequential — SoT stays consistent */
    for (int e = 0; e < ne; e++) {
      if (!edges[e].ok) continue;
      cubalc_cube *A = &ch->cubes[edges[e].from];
      cubalc_cube *B = &ch->cubes[edges[e].to];
      cubalc_atom *received = &edges[e].recv;
      if (received->proton) {
        for (int bi = 0; bi < received->matrix.n && bi < CUBALC_ATOM_BITS; bi++)
          if (cubalc_matrix_get(&received->matrix, bi))
            cubalc_matrix_set(&B->atom.matrix, bi, 1);
        B->atom.alive = 1;
      } else {
        for (int bi = 0; bi < received->matrix.n && bi < CUBALC_ATOM_BITS; bi++)
          if (cubalc_matrix_get(&received->matrix, bi))
            cubalc_matrix_set(&B->atom.matrix, bi, 0);
      }
      if (!B->atom.digit_lock)
        B->atom.digit = (uint8_t)cubalc_algocube_digit(&B->atom.matrix);
      B->atom.unity = cubalc_matrix_compat(&A->atom.matrix, &B->atom.matrix);
      A->atom.energy = fminf(1.f, fmaxf(0.f, A->atom.energy + edges[e].e_from_delta));
      B->atom.energy = fminf(1.f, fmaxf(0.f, B->atom.energy + edges[e].e_to_delta));
    }
    ch->seq++;
    cubalc_chain_tick(ch);
  }
  return 0;
}

int cubalc_async_compat_batch(const cubalc_chain *ch, float *out, int n_cap) {
  if (!ch || !out || ch->n_cubes < 1) return -1;
  if (!G_inited) cubalc_async_init(0);
  int n = ch->n_cubes;
  if (n > n_cap) n = n_cap;
  cubalc_matrix mats[CUBALC_MAX_CUBES];
  for (int i = 0; i < n; i++)
    mats[i] = ch->cubes[i].atom.matrix;

  int nw = G.n_workers;
  if (nw > n) nw = n;
  if (nw < 1) nw = 1;
  pthread_t th[CUBALC_ASYNC_MAX_WORKERS];
  compat_args_t args[CUBALC_ASYNC_MAX_WORKERS];
  int chunk = (n + nw - 1) / nw;
  int nt = 0;
  for (int w = 0; w < nw; w++) {
    int i0 = w * chunk;
    int i1 = i0 + chunk;
    if (i0 >= n) break;
    if (i1 > n) i1 = n;
    args[nt].mats = mats;
    args[nt].n = n;
    args[nt].out = out;
    args[nt].i0 = i0;
    args[nt].i1 = i1;
    if (pthread_create(&th[nt], NULL, compat_th, &args[nt]) != 0)
      compat_th(&args[nt]);
    else
      nt++;
  }
  for (int w = 0; w < nt; w++)
    pthread_join(th[w], NULL);
  return n;
}
