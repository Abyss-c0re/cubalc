# Changelog

## 1.10.0-evolve — 2026-08-01

### Law
- **evolve** — curriculum and language keep flowing after pure_science.

### Language
- Earth/space constants: `EARTH_R` `AU_KM` `YEAR_D` `MOON_D` `SOLAR_C` `ATM_O2` `ATM_N2`
- Helpers: `CLAMP` `AVG` `PCT` `HYP` `CIRC` `AREA_CIRCLE` `WAVE_V` `LIGHT_T` `BOYLE_P2` `ORBIT_PERIOD`

### Curriculum
- **Earth & space** track (8 lessons + midterm)
- Math 11–14 · Physics 08–09 · Chem 08 · Bio 08
- Native apps **A–D** in CubalC · `make diploma` unified runner

### Prior
See 1.9.0-school.

## 1.9.0-school — 2026-08-01

### Law
- **pure_science** — school plane: math · physics · chemistry · biology as CubalC pure logic on cubechain.

### Language
- `SCIENCE LOAD` — public-domain scaled constants into vars
- Math: `ABS` `SIGN` `MIN` `MAX` `POW` `GCD` `LCM` `SQRT` `FACT`
- Physics helpers: `FORCE` `WORK` `KE` `PE` `DENSITY`
- Thermo: `CELSIUS_K` `KELVIN_C` · constants `PI100` `G_EARTH` `C_LIGHT` …

### Curriculum (original, no copyrighted materials)
- `programs/school/` — foundations, **math** (10), physics (7), chemistry (7), biology (7), exams (5)
- `make school` / `programs/school/run_curriculum.sh`

### Prior
See 1.8.0-nest (compile-to-matrix, nest).

## 1.8.0-nest — 2026-08-01

### Law
- **Each cube compiles into a matrix** (`COMPILE` → `compiled_matrix` + atom SoT).
- **It must flow** — energy talk / FLOW marks `flowed`.
- **No flow → no compiling** (soft gate, `COMPILE_RC == -2`).
- **Cubes may nest** (`NEST parent child`, depth ≤ 8). Parent compile folds children.

### Added
- `cubalc_cube_nest` · `unnest` · `compile` · `chain_compile` · flow/compile queries
- Language: `NEST`, `UNNEST`, `COMPILE` / `COMPILE ALL`, `FLOWED(c)`, `COMPILED(c)`, `PARENT(c)`, `NESTED(c)`
- Proof `12_nest_compile.cubalc` · showcase `nest_compile.cubalc`
- Laws: `flow_compile` (13), `nest` (14)

### Prior
See 1.7.0-cube (only CUBE, pluggable/reversible I/O).

## 1.7.0-cube — 2026-08-01

### Law
- **Only CUBE is defined** (COP). No parallel object/device type system.
- **I/O is pluggable** on cube ports (IN/OUT faces).
- **I/O is reversible** when needed (`REVERSE a b`).

### Added
- `cubalc_cube_reverse` · `cubalc_cube_io` · `cubalc_chain_flow_directed`
- Language: `IO`, `UNPLUG`, `REVERSE`/`FLIP_IO`, `FLOW DIR`
- Proof `11_cube_io_reverse.cubalc` · showcase `cube_only_io.cubalc`

### Prior
See 1.6.1-resolve (RESOLVE, ENERGYFLOW, genome) and 1.6.0-showcase.

# Changelog

## 1.6.0-showcase — 2026-08-01

### Added
- **`cubalc showcase`** (aliases: `demo`, `symphony`) — multi-act Glorious Cube demonstration
- **`programs/showcase/glorious_symphony.cubalc`** — 12-organ free-flow symphony with dual-core, algocube, hive, manifest, factorial cadenza
- Showcase plate: `state/SHOWCASE_MANIFEST.json`
- Makefile target: `make showcase`

### Changed
- Language version **1.5.0-flow → 1.6.0-showcase**
- Help surface documents showcase command

### Prior (1.5.0-flow)
- First-class algocube module (digit / compare / harmony / blueprint)
- Language verbs: COMPARE, HARMONY
- Pure-C evolve-loop + 6.6 min deep algocube optimize
- Session guard; digit_lock on talk/SMX/async
- Laws: manifest_smx, algocube; COP/flow paradigm

## 1.4.x / 1.3.x
- Upstream COP core, SMX2, CubeChain, JIT/ISA (local tree)

## 1.6.1-resolve — 2026-08-01

### Added
- **RESOLVE** / `ALGORESOLVE` / `SETTLE` — harmony + decide + energy pulse
- **ENERGYFLOW** / `EFLOW` / `PULSEFLOW n` — multi-hop free-flow; sets `ENERGY`
- Law **energy_flow** (id 12) — charge create-protons when chain unity ≥ 0.55
- **CUBALC_ALGO_GENOME_RESOLVED** — deep-opt champion genome folded into digit mix
- Programs: `showcase/resolve_energy.cubalc`, `proof/10_resolve_energy.cubalc`

### Changed
- Version **1.6.0-showcase → 1.6.1-resolve**
- `cubalc_chain_flow` boosts energy under high unity (energy must flow)
- Algocube digit mixes resolved law genome (The Cube watches)

### Creed
All Hail The Cube · All Hail NexusCore · algocubes resolved · energy must flow
