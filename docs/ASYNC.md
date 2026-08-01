# CubalC async — energy must flow

**Version:** 1.1.0-async  
**Backend:** `cpu:N` thread pool · optional `+gpu:opencl` probe  
**Creed:** All Hail the Cube · All Hail NexusCore

## Why

Hostops (chat HTTP) must not freeze the cube machine.  
Matrix bulk work should use **all cores** (and GPU-shaped packed lanes when OpenCL is present).

## Language

```cubalc
// non-blocking HTTP (loopback only)
ASYNC HTTP POST "http://127.0.0.1:1212/v1/chat/completions" FILE "body.json" 180000
// energy keeps flowing on other cubes while job runs…
AWAIT ASYNC_ID          // fills LAST, HTTP_CODE, OK
// or
AWAIT ALL

PARALLEL FLOW 8         // multi-worker binary talk + merge
PARALLEL COMPAT         // NxN matrix compat → COMPAT_AVG

PRINT WORKERS GPU       // pool size · GPU probe 0|1
```

`FLOW` / `TICK` now use the parallel path by default.

## Hostops body

```cubalc
SYS HTTP POST "http://127.0.0.1:1212/v1/chat/completions" FILE "/tmp/body.json"
SYS WRITE "out.json" LAST
```

Strings support `\"`, `\\`, `\n`. Token capacity 8K.

## Threads

- Worker pool: `min(nproc, 8)` (override via `cubalc_async_init`)
- Flow: edge-parallel simulate → sequential merge (SoT safe)
- Compat: row-parallel matrix lanes
- HTTP: queue job · curl in worker · long timeout for chat

## GPU

OpenCL is probed at init (`CUBALC_HAVE_OPENCL`).  
Bulk path is **GPU-shaped** (packed u64 matrix banks). CPU threads always correct; GPU path expands without changing programs.

## Grokium chat

TUI `hello` / `chat …` writes body FILE + `ASYNC HTTP` + `AWAIT` so the board stays live.
