/* CubalC async runtime — energy must flow.
 * Thread pool · job queue · non-blocking hostops · parallel matrix lanes.
 * CPU workers always; optional GPU path when built with accel.
 * Law: HOLD_FLASH · matrix SoT · loopback hostops · no prose on the wire.
 */
#ifndef CUBALC_ASYNC_H
#define CUBALC_ASYNC_H
#include "cubalc.h"
#include "cubalc_hostops.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CUBALC_ASYNC_MAX_JOBS   64
#define CUBALC_ASYNC_MAX_WORKERS 32

typedef enum {
  CUBALC_JOB_NONE = 0,
  CUBALC_JOB_HTTP,
  CUBALC_JOB_FLOW_TICK,   /* one parallel energy flow on a chain snapshot */
  CUBALC_JOB_COMPAT,      /* NxN compat into buffer */
  CUBALC_JOB_POPBATCH     /* popcount batch */
} cubalc_job_kind;

typedef enum {
  CUBALC_JOB_IDLE = 0,
  CUBALC_JOB_PENDING,
  CUBALC_JOB_RUNNING,
  CUBALC_JOB_DONE,
  CUBALC_JOB_FAILED,
  CUBALC_JOB_CANCELLED
} cubalc_job_state;

typedef struct cubalc_async_job {
  int id;
  cubalc_job_kind kind;
  cubalc_job_state state;
  int ok;
  int code;                 /* HTTP or internal */
  long n;
  char err[CUBALC_HOST_ERR_MAX];
  char str[CUBALC_HOST_STR_MAX];
  /* HTTP payload (owned by job until done) */
  char method[8];
  char url[512];
  char *body;               /* malloc'd, freed on complete */
  int timeout_ms;
} cubalc_async_job;

/* init once; n_workers<=0 → auto (min(nproc, 8)) */
int  cubalc_async_init(int n_workers);
void cubalc_async_shutdown(void);
int  cubalc_async_workers(void);
int  cubalc_async_gpu_ok(void); /* 1 if GPU accel path live */

/* non-blocking HTTP (loopback). returns job id >=1, or -1 */
int  cubalc_async_http(const char *method, const char *url, const char *body,
                       int timeout_ms);

/* poll: 0=still pending/running, 1=terminal (done/failed). copies snapshot to *out if non-NULL */
int  cubalc_async_poll(int job_id, cubalc_async_job *out);
/* wait up to timeout_ms (-1 = forever). 0 ok terminal, -1 timeout/missing */
int  cubalc_async_wait(int job_id, int timeout_ms, cubalc_async_job *out);
int  cubalc_async_await_all(int timeout_ms);

/* Parallel energy: multi-worker talk on independent edge chunks, then merge.
 * Safer than free-for-all mutation — energy still flows, Cube Law holds. */
int  cubalc_async_chain_flow(cubalc_chain *ch, int ticks);

/* Parallel NxN matrix compat (CPU threads; GPU when large & available) */
int  cubalc_async_compat_batch(const cubalc_chain *ch, float *out_n_by_n, int n_cap);

/* Device string for status: "cpu:N" or "cpu:N+gpu" */
const char *cubalc_async_backend(void);

#ifdef __cplusplus
}
#endif
#endif
