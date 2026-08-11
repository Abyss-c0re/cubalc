#define _POSIX_C_SOURCE 200809L
#include "cubalc_hw.h"
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

/* Optional GPU — compile with CUBALC_HAVE_OPENCL (make USE_OPENCL=1) */
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

#define HW_MAX_WORKERS 32
#define HW_GPU_MIN_N   8   /* OpenCL only worth it for larger fleets */

/* ---- state ---- */
static int  g_inited;
static int  g_workers = 1;
static int  g_gpu_ok;
static char g_backend[64] = "cpu:1";

#if defined(CUBALC_HAVE_OPENCL)
static cl_context       g_cl_ctx;
static cl_command_queue g_cl_q;
static cl_program       g_cl_prog;
static cl_kernel        g_cl_kern;
static int              g_cl_ready;
#endif

static int pop64(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
  return (int)__builtin_popcountll(x);
#else
  int c = 0;
  while (x) { x &= x - 1; c++; }
  return c;
#endif
}

/* Pack matrix bits into first u64 (ATOM_BITS ≤ 64 → one word). */
static uint64_t pack_word(const cubalc_matrix *m) {
  if (!m) return 0;
  uint64_t w = 0;
  int nbytes = (CUBALC_ATOM_BITS + 7) / 8;
  if (nbytes > (int)sizeof(w)) nbytes = (int)sizeof(w);
  memcpy(&w, m->bits, (size_t)nbytes);
  return w;
}

static float unity_from_words(uint64_t a, uint64_t b) {
  int h = pop64(a ^ b);
  if (h > CUBALC_ATOM_BITS) h = CUBALC_ATOM_BITS;
  return 1.f - (float)h / (float)CUBALC_ATOM_BITS;
}

#if defined(CUBALC_HAVE_OPENCL)
static const char *HARMONY_CL_SRC =
"__kernel void harmony_pairs(__global const ulong *bank, const int n,\n"
"                            __global float *pair_u) {\n"
"  int k = get_global_id(0);\n"
"  int pairs = n * (n - 1) / 2;\n"
"  if (k >= pairs) return;\n"
"  int i = 0, rem = k;\n"
"  for (; i < n; i++) {\n"
"    int row = n - i - 1;\n"
"    if (rem < row) break;\n"
"    rem -= row;\n"
"  }\n"
"  int j = i + 1 + rem;\n"
"  ulong x = bank[i] ^ bank[j];\n"
"  int h = popcount(x);\n"
"  pair_u[k] = 1.0f - (float)h / 64.0f;\n"
"}\n";

static int cl_setup(void) {
  if (g_cl_ready) return 0;
  cl_int err;
  cl_uint nplat = 0;
  if (clGetPlatformIDs(0, NULL, &nplat) != CL_SUCCESS || nplat == 0) return -1;
  cl_platform_id plat;
  if (clGetPlatformIDs(1, &plat, NULL) != CL_SUCCESS) return -1;
  cl_uint ndev = 0;
  cl_device_id dev;
  if (clGetDeviceIDs(plat, CL_DEVICE_TYPE_GPU, 1, &dev, &ndev) != CL_SUCCESS || ndev == 0) {
    if (clGetDeviceIDs(plat, CL_DEVICE_TYPE_ALL, 1, &dev, &ndev) != CL_SUCCESS || ndev == 0)
      return -1;
  }
  g_cl_ctx = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
  if (err != CL_SUCCESS || !g_cl_ctx) return -1;
#if defined(CL_VERSION_2_0)
  g_cl_q = clCreateCommandQueueWithProperties(g_cl_ctx, dev, NULL, &err);
#else
  g_cl_q = clCreateCommandQueue(g_cl_ctx, dev, 0, &err);
#endif
  if (err != CL_SUCCESS || !g_cl_q) return -1;
  size_t src_len = strlen(HARMONY_CL_SRC);
  g_cl_prog = clCreateProgramWithSource(g_cl_ctx, 1, &HARMONY_CL_SRC, &src_len, &err);
  if (err != CL_SUCCESS || !g_cl_prog) return -1;
  err = clBuildProgram(g_cl_prog, 1, &dev, NULL, NULL, NULL);
  if (err != CL_SUCCESS) return -1;
  g_cl_kern = clCreateKernel(g_cl_prog, "harmony_pairs", &err);
  if (err != CL_SUCCESS || !g_cl_kern) return -1;
  g_cl_ready = 1;
  return 0;
}

static float gpu_harmony_unity(const uint64_t *bank, int n) {
  if (!g_cl_ready || n < 2) return -1.f;
  int pairs = n * (n - 1) / 2;
  if (pairs < 1) return 1.f;
  cl_int err;
  cl_mem d_bank = clCreateBuffer(g_cl_ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                 (size_t)n * sizeof(uint64_t), (void *)bank, &err);
  if (err != CL_SUCCESS) return -1.f;
  cl_mem d_out = clCreateBuffer(g_cl_ctx, CL_MEM_WRITE_ONLY,
                                (size_t)pairs * sizeof(float), NULL, &err);
  if (err != CL_SUCCESS) {
    clReleaseMemObject(d_bank);
    return -1.f;
  }
  err  = clSetKernelArg(g_cl_kern, 0, sizeof(cl_mem), &d_bank);
  err |= clSetKernelArg(g_cl_kern, 1, sizeof(int), &n);
  err |= clSetKernelArg(g_cl_kern, 2, sizeof(cl_mem), &d_out);
  if (err != CL_SUCCESS) {
    clReleaseMemObject(d_bank);
    clReleaseMemObject(d_out);
    return -1.f;
  }
  size_t gsz = (size_t)pairs;
  err = clEnqueueNDRangeKernel(g_cl_q, g_cl_kern, 1, NULL, &gsz, NULL, 0, NULL, NULL);
  if (err != CL_SUCCESS) {
    clReleaseMemObject(d_bank);
    clReleaseMemObject(d_out);
    return -1.f;
  }
  float *host = (float *)malloc((size_t)pairs * sizeof(float));
  if (!host) {
    clReleaseMemObject(d_bank);
    clReleaseMemObject(d_out);
    return -1.f;
  }
  err = clEnqueueReadBuffer(g_cl_q, d_out, CL_TRUE, 0,
                            (size_t)pairs * sizeof(float), host, 0, NULL, NULL);
  clReleaseMemObject(d_bank);
  clReleaseMemObject(d_out);
  if (err != CL_SUCCESS) {
    free(host);
    return -1.f;
  }
  double sum = 0.0;
  for (int i = 0; i < pairs; i++) sum += (double)host[i];
  free(host);
  return (float)(sum / (double)pairs);
}
#endif /* CUBALC_HAVE_OPENCL */

static int gpu_probe(void) {
#if defined(CUBALC_HAVE_OPENCL)
  cl_uint nplat = 0;
  if (clGetPlatformIDs(0, NULL, &nplat) != CL_SUCCESS || nplat == 0) return 0;
  if (cl_setup() != 0) return 0;
  return 1;
#else
  return 0;
#endif
}

void cubalc_hw_init(int n_workers) {
  if (g_inited && n_workers <= 0) return;
  long nc = sysconf(_SC_NPROCESSORS_ONLN);
  if (nc < 1) nc = 2;
  if (n_workers <= 0) n_workers = (int)(nc > HW_MAX_WORKERS ? HW_MAX_WORKERS : nc);
  if (n_workers < 1) n_workers = 1;
  if (n_workers > HW_MAX_WORKERS) n_workers = HW_MAX_WORKERS;
  g_workers = n_workers;
  g_gpu_ok = gpu_probe();
  if (g_gpu_ok)
    snprintf(g_backend, sizeof g_backend, "cpu:%d+gpu", g_workers);
  else
    snprintf(g_backend, sizeof g_backend, "cpu:%d", g_workers);
  g_inited = 1;
}

static void ensure_init(void) {
  if (!g_inited) cubalc_hw_init(0);
}

int cubalc_hw_workers(void) {
  ensure_init();
  return g_workers;
}

int cubalc_hw_gpu_ok(void) {
  ensure_init();
  return g_gpu_ok;
}

const char *cubalc_hw_backend(void) {
  ensure_init();
  return g_backend;
}

int cubalc_hw_matrix_popcount(const cubalc_matrix *m) {
  if (!m) return 0;
  return pop64(pack_word(m));
}

int cubalc_hw_matrix_hamming(const cubalc_matrix *a, const cubalc_matrix *b) {
  if (!a || !b) return CUBALC_ATOM_BITS;
  return pop64(pack_word(a) ^ pack_word(b));
}

float cubalc_hw_matrix_unity(const cubalc_matrix *a, const cubalc_matrix *b) {
  if (!a || !b) return 0.f;
  return unity_from_words(pack_word(a), pack_word(b));
}

int cubalc_hw_matrix_pack_u64(const cubalc_matrix *m, uint64_t *out, int max_words) {
  if (!m || !out || max_words < 1) return -1;
  memset(out, 0, (size_t)max_words * sizeof(uint64_t));
  out[0] = pack_word(m);
  return 1;
}

int cubalc_hw_pack_bank(const cubalc_matrix *mats, int n, uint64_t *out_words) {
  if (!mats || !out_words || n <= 0) return -1;
  for (int i = 0; i < n; i++)
    out_words[i] = pack_word(&mats[i]);
  return n;
}

typedef struct {
  const cubalc_matrix *mats;
  int n;
  float *out;
  int i0, i1;
} compat_args_t;

static void *compat_th(void *arg) {
  compat_args_t *a = arg;
  for (int i = a->i0; i < a->i1; i++)
    for (int j = 0; j < a->n; j++)
      a->out[i * a->n + j] = cubalc_matrix_compat(&a->mats[i], &a->mats[j]);
  return NULL;
}

int cubalc_hw_compat_batch(const cubalc_matrix *mats, int n, float *out) {
  if (!mats || !out || n <= 0) return -1;
  ensure_init();
  if (n == 1) {
    out[0] = cubalc_matrix_compat(&mats[0], &mats[0]);
    return 0;
  }
  int nw = g_workers;
  if (nw > n) nw = n;
  if (nw < 1) nw = 1;
  if (nw == 1 || n < 4) {
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        out[i * n + j] = cubalc_matrix_compat(&mats[i], &mats[j]);
    return 0;
  }
  pthread_t th[HW_MAX_WORKERS];
  compat_args_t args[HW_MAX_WORKERS];
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
  return 0;
}

typedef struct {
  const uint64_t *bank;
  int n;
  int i0, i1;
  double sum;
  int pairs;
} harm_args_t;

static void *harm_th(void *arg) {
  harm_args_t *a = arg;
  double sum = 0.0;
  int pairs = 0;
  for (int i = a->i0; i < a->i1; i++) {
    for (int j = i + 1; j < a->n; j++) {
      sum += (double)unity_from_words(a->bank[i], a->bank[j]);
      pairs++;
    }
  }
  a->sum = sum;
  a->pairs = pairs;
  return NULL;
}

static float cpu_harmony_unity_words(const uint64_t *bank, int n) {
  if (n < 2) return 1.f;
  ensure_init();
  int nw = g_workers;
  if (nw > n) nw = n;
  if (nw < 1) nw = 1;
  if (nw == 1 || n < 4) {
    double sum = 0.0;
    int pairs = 0;
    for (int i = 0; i < n; i++)
      for (int j = i + 1; j < n; j++) {
        sum += (double)unity_from_words(bank[i], bank[j]);
        pairs++;
      }
    return pairs ? (float)(sum / (double)pairs) : 1.f;
  }
  pthread_t th[HW_MAX_WORKERS];
  harm_args_t args[HW_MAX_WORKERS];
  int chunk = (n + nw - 1) / nw;
  int nt = 0;
  for (int w = 0; w < nw; w++) {
    int i0 = w * chunk;
    int i1 = i0 + chunk;
    if (i0 >= n) break;
    if (i1 > n) i1 = n;
    args[nt].bank = bank;
    args[nt].n = n;
    args[nt].i0 = i0;
    args[nt].i1 = i1;
    args[nt].sum = 0;
    args[nt].pairs = 0;
    if (pthread_create(&th[nt], NULL, harm_th, &args[nt]) != 0)
      harm_th(&args[nt]);
    else
      nt++;
  }
  for (int w = 0; w < nt; w++)
    pthread_join(th[w], NULL);
  double sum = 0.0;
  int pairs = 0;
  for (int w = 0; w < nt; w++) {
    sum += args[w].sum;
    pairs += args[w].pairs;
  }
  return pairs ? (float)(sum / (double)pairs) : 1.f;
}

float cubalc_hw_harmony_unity(const cubalc_matrix *mats, int n) {
  if (!mats || n <= 0) return 0.f;
  if (n == 1) return 1.f;
  ensure_init();
  uint64_t *bank = (uint64_t *)malloc((size_t)n * sizeof(uint64_t));
  if (!bank) return 0.f;
  for (int i = 0; i < n; i++)
    bank[i] = pack_word(&mats[i]);

  float u = -1.f;
#if defined(CUBALC_HAVE_OPENCL)
  if (g_gpu_ok && n >= HW_GPU_MIN_N) {
    u = gpu_harmony_unity(bank, n);
    if (u >= 0.f) {
      free(bank);
      return u;
    }
  }
#endif
  u = cpu_harmony_unity_words(bank, n);
  free(bank);
  return u;
}

static void consensus_majority(const uint64_t *bank, int n, cubalc_matrix *out) {
  cubalc_matrix_clear(out);
  out->n = CUBALC_ATOM_BITS;
  if (n <= 0) return;
  if (n == 1) {
    memcpy(out->bits, &bank[0], sizeof(uint64_t) < sizeof(out->bits)
                                    ? sizeof(uint64_t) : sizeof(out->bits));
    out->set = (uint16_t)pop64(bank[0]);
    return;
  }
  for (int b = 0; b < CUBALC_ATOM_BITS; b++) {
    int ones = 0;
    uint64_t mask = 1ULL << b;
    for (int i = 0; i < n; i++)
      if (bank[i] & mask) ones++;
    if (ones * 2 >= n)
      cubalc_matrix_set(out, b, 1);
  }
}

int cubalc_hw_harmony_solve(const cubalc_matrix *const *mats, int n,
                            float *out_unity, cubalc_matrix *out_consensus) {
  if (!mats || n <= 0) return -1;
  ensure_init();

  uint64_t *bank = (uint64_t *)malloc((size_t)n * sizeof(uint64_t));
  if (!bank) return -1;
  for (int i = 0; i < n; i++)
    bank[i] = mats[i] ? pack_word(mats[i]) : 0ULL;

  float u;
  if (n == 1) {
    u = 1.f;
  } else {
    u = -1.f;
#if defined(CUBALC_HAVE_OPENCL)
    if (g_gpu_ok && n >= HW_GPU_MIN_N)
      u = gpu_harmony_unity(bank, n);
#endif
    if (u < 0.f)
      u = cpu_harmony_unity_words(bank, n);
  }
  if (out_unity) *out_unity = u;
  if (out_consensus)
    consensus_majority(bank, n, out_consensus);
  free(bank);
  return 0;
}
