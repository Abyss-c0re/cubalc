# CubalC project structure

**Law alignment:** cube is SoT · pure C · COP/flow · devices free · HTTP never required · multiplatform.

## Layout

```
cubalc/
  include/
    cubalc.h                 # public chain / matrix / cube COP API
    cubalc_law.h             # law plate + version tokens
    cubalc_platform.h        # multiplatform shims (POSIX / Darwin / Windows)
    cubalc_lang.h            # public language run API
    lang/
      cubalc_lang_internal.h # VM / lexer / op-plane internals (not product edge)
    cubalc_*.h               # smx, evolve, jit, hostops, …
  src/
    cubalc_main.c            # CLI
    cubalc_core.c            # matrix / cube core
    cubalc_*.c               # smx, evolve, jit, …
    lang/
      lang_core.c            # lexer, VM, expr, place/plug/flow
      lang_ops_core.c        # hold, ASYNC, SYS, strings
      lang_ops_toc.c         # stack↔cell TOC plane
      lang_ops_stack.c       # depth combinators + stack ALU
      lang_ops_dual.c        # dual-stack D*
      lang_ops_math.c        # numthy / modular / pack
      lang_ops_bit.c         # bitfield / mask / sat
      lang_ops_cell.c        # *CELL range plane
      lang_ops_flow.c        # FN / LET / control / ASSERT
      lang_parse.c           # parse_form dispatcher (planes)
      lang_run.c             # cubalc_run_source / run_file
      README.md
  programs/proof/            # language proofs
  programs/science/          # pure-science demos (language direction)
  scripts/                   # universal iter, peers, …
  tests/
  tools/modularize_lang.py   # re-split helper if monolith reappears
  Makefile                   # multiplatform build
```

## Build (multiplatform)

```bash
make all                 # Linux / macOS / MinGW (detects uname)
make CUBALC_CROSS=1 ...  # skip -march=native when cross-compiling
make USE_OPENCL=1 all    # optional OpenCL (Darwin uses -framework OpenCL)
make modular-check
make test
make universal-iter
```

Binary: `out/cubalc` (or `out/cubalc.exe` on Windows).

## Language module law

1. **New ops go only in the matching plane** (`ops_toc`, `ops_dual`, …).
2. Public surface stays `cubalc_lang.h` — do not export VM internals.
3. Platform code uses `cubalc_platform.h` (no hard-coded device paths).
4. Behavior is proof-gated: suite must stay `fail=0` after structural edits.

## Regenerating the split

If a thrash reintroduces a monolith:

```bash
# ensure src/cubalc_lang.c is the full source (from git history if needed)
python3 tools/modularize_lang.py
make all && make universal-iter
```
