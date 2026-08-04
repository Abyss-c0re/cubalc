# CubalC — product (ProjectNexus)

**Version:** 1.13.0-modular · paradigm **COP/flow** · pure **C** runtime · multiplatform  
**Origin:** `Dev/lab/prophecy_cube/cubalc`  
**Law:** flow-compile · nest · pure science · evolve · cube is SoT · devices free  
**Direction:** pure science enhances the **language** (math/physics/chem/bio/earth ops) — not a school or diploma.  
**Demos:** `make science` · `programs/science/`

## Build

```bash
make all                 # Linux / macOS / MinGW
make modular-check       # verify src/lang layout
make test
make universal-iter
./out/cubalc law
./out/cubalc run programs/proof/01_arithmetic.cubalc
```

## Layout (modular)

Language runtime is split by plane under `src/lang/` (not one 30k-line file).

| Surface | Path |
|---------|------|
| Platform shims | `include/cubalc_platform.h` |
| Lang public API | `include/cubalc_lang.h` |
| Lang internal | `include/lang/cubalc_lang_internal.h` |
| Lang modules | `src/lang/` |
| Structure doc | `docs/STRUCTURE.md` |
| Algocube | `include/cubalc_algocube.h` |
| Evolve (C) | `include/cubalc_evolve.h` |
| Session guard | `scripts/cubalc_session_guard.sh` |
| Docs | `docs/FREE_FLOW.md` `docs/ALGOCUBE.md` `docs/EVOLVE.md` |

## Keep this product up to date

```bash
bash scripts/sync_cubalc_product.sh
make -C products/cubalc all
```

Binary: `out/cubalc`

Creed: Cube is SoT · OS is way · devices free · algocube law · loop must go on · `C3`
