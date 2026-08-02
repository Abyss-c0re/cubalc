# Changelog

## 1.12.8-universal — 2026-08-02

### Direction
- Algocube digit **9** → cell logic plane: bitwise range masks + equality predicate.

### Language
- `ANDCELL`/`CELLAND` lo hi mask — bitwise AND each cell in range with mask
- `ORCELL`/`CELLOR` lo hi mask — bitwise OR each cell in range with mask
- `XORCELL`/`CELLXOR` lo hi mask — bitwise XOR each cell in range with mask
- `NOTCELL`/`CELLNOT` lo hi — bitwise NOT (~) each cell in range
- `EQCELL`/`CELLEQ` lo hi val — set cell to 1 if == val else 0; LAST_N = hit count
- Proof `31_cell_logic.cubalc`

### Prior
See 1.12.7-universal.

## 1.12.7-universal — 2026-08-02

### Direction
- Algocube digit **8** → stack↔cell bridge: block transfer between stack and cells.

### Language
- `TOCELL`/`>CELL` dst [n] — pop n stack values into cells[dst..] (TOS → highest)
- `FROMCELL`/`CELL>`/`PUSHCELL` src [n] — push cells[src..] onto stack
- Proof `30_stack_cell_bridge.cubalc`

### Prior
See 1.12.6-universal.

## 1.12.6-universal — 2026-08-02

### Direction
- Algocube digit **9** → cell fold ext: argmin/argmax + range rotate/shift.

### Language
- `MINIDX`/`ARGMIN` [lo [hi]] — first index of minimum in range (stmt + expr)
- `MAXIDX`/`ARGMAX` [lo [hi]] — first index of maximum in range (stmt + expr)
- `ROTCELL`/`CELLROT` lo hi k — rotate range left by k (k<0 right)
- `SHIFTCELL`/`CELLSHIFT` lo hi k — shift with zero-fill (k>0 left, k<0 right)
- Proof `29_cell_argmin_rot.cubalc`

### Prior
See 1.12.5-universal.

## 1.12.5-universal — 2026-08-02

### Direction
- Algocube digit **9** → cell fold plane: range arithmetic + iota + sort.

### Language
- `ADDCELL`/`CELLADD` lo hi delta — add to each cell in range
- `MULCELL`/`CELLMUL` lo hi k — scale each cell in range
- `IOTA`/`SEQCELL` lo hi [start [step]] — arithmetic sequence fill
- `SORTCELL`/`CELLSORT` lo hi [ASC|DESC|dir] — insertion sort (DESC or 0 = descending)
- Proof `28_cell_arith_sort.cubalc`

### Prior
See 1.12.4-universal.

## 1.12.4-universal — 2026-08-02

### Direction
- Algocube digit **8** → stack depth plane: double-width combinators + ROLL/DEPTH.

### Language
- `NIP` — drop under top (a b → b)
- `TUCK` — b under a, keep b (a b → b a b)
- `2DUP`/`DDUP` · `2DROP`/`DDROP` · `2SWAP`/`DSWAP`
- `ROLL` n — rotate top (n+1) items (n=1≡SWAP, n=2≡ROT)
- `DEPTH` — push current stack depth
- Proof `27_stack_depth_universal.cubalc`

### Prior
See 1.12.3-universal.

## 1.12.3-universal — 2026-08-02

### Direction
- Algocube digit **2** → math plane: integer log2 / bit ranks / digit folds / modular inverse.

### Language
- `ILOG2`/`LOG2`(n) — floor log2; -1 if n≤0
- `CTZ`(n) / `CLZ`(n) — trailing / leading zeros (64-bit word; 0→64)
- `ISPOW2`(n) / `POW2`(k) — power-of-two test / 2^k (k 0..62)
- `NDIGITS`(n) / `DIGSUM`(n) — decimal digit count / digit sum
- `MODINV`/`INVMOD`(a,m) — modular inverse via extended Euclid (0 if none)
- Proof `26_math_ilog_modinv.cubalc`

### Prior
See 1.12.2-universal.

## 1.12.2-universal — 2026-08-02

### Direction
- Algocube digit **5** → cell memory plane: block copy/move + find/count + reverse.

### Language
- `COPYCELL`/`CELLCOPY`/`CMOVE` src dst n — overlap-safe cell block copy
- `MOVECELL`/`CELLMOVE` src dst n — copy then clear non-overlapping source
- `FINDCELL`/`CELLFIND` val [lo [hi]] — first index or -1 (`OK` = found); expr form too
- `COUNTCELL`/`CELLCOUNT` val [lo [hi]] — count matches; expr form too
- `REVCELL`/`CELLREV` lo hi — reverse cell range in place
- Proof `25_cell_mem_universal.cubalc`

### Prior
See 1.12.1-universal.

## 1.12.1-universal — 2026-08-02

### Direction
- Algocube digit **4** → data-plane stack combinators + cell fill.

### Language
- `DUP`/`DROP`/`SWAP`/`OVER`/`ROT` — Forth-style stack ops (OK/LAST_N/SP)
- `PICK` n — copy n-th under top (0=TOS) onto stack
- `FILLCELL`/`CELLFILL` lo hi val — fill cell range
- Proof `24_stack_ops_universal.cubalc`

### Prior
See 1.12.0-universal.


## 1.12.0-universal — 2026-08-02

### Direction
- Algocube digit **6** → energy / matrix flow + seeded RNG.

### Language
- `RAND`/`RND` [max] · `RAND(n)` (seed: `CUBALC_SEED` env)
- `ENERGYSET`/`ENERGYADD` cube n (0..100 scale)
- `ROTBITS`/`SHIFTBITS` cube k (State Matrix bit rotate; negative = right)
- Proof `23_rand_energy_rotbits.cubalc`

### Prior
See 1.11.9-universal.

## 1.11.9-universal — 2026-08-02

### Direction
- Algocube digit **9** → cell fold + INC/DEC (loop-friendly data).

### Language
- `INC`/`DEC` name [step] · `INC CELL`/`DEC CELL` i [step]
- `SUMCELL`/`MINCELL`/`MAXCELL` [lo [hi]] (stmt + expr forms)
- Proof `22_cell_fold_inc.cubalc`

### Prior
See 1.11.8-universal.

## 1.11.8-universal — 2026-08-02

### Direction
- Algocube digit **4** → control flow: FN return + CASE.

### Language
- `RET`/`RETURN` [expr] early exit from `FN` body (`RETVAL`/`LAST_N`)
- `CASE`/`SWITCH` … `WHEN` … `DEFAULT` … `END`
- Proof `21_fn_return_case.cubalc`

### Prior
See 1.11.7-universal.

## 1.11.7-universal — 2026-08-02

### Direction
- Algocube digit **3** → string plane (control + data interchange).

### Language
- `SYS CAT`/`STRCAT` · `SYS FIND`/`INDEX` · `SYS EQS`/`STREQ`
- `SYS HAS`/`CONTAINS` · `SYS REVS`/`STRREV` · `SYS UPPER` · `SYS LOWER`
- Proof `20_string_ops_universal.cubalc`
- Tree restore from HEAD if mass-delete corruption

### Prior
See 1.11.6-universal.

## 1.11.6-universal — 2026-08-02

### Direction
- Algocube digit **2** → math plane (modular + number theory).

### Language
- `ADDMOD` `SUBMOD` `MULMOD` `POWMOD` · `FIB`/`FIBONACCI` · `ISPRIME`/`PRIMEP`
- `IDIV` `IMOD` named integer div/mod
- Proof `19_math_modular_universal.cubalc`

### Prior
See 1.11.5-universal.

## 1.11.5-universal — 2026-08-02

### Direction
- Algocube digit **1** → data plane (integer cells + stack).

### Language
- Cells: `CELLSET`/`SLOTSET` · `CELLGET` · `CELL(i)`/`SLOT(i)` · `SWAPCELL` · `CLEARCELLS` · `CELLS`
- Stack: `PUSH` · `POP` [var] · `PEEK` · `CLEARSTACK` · `SP`/`STACKLEN`
- Proof `18_data_cells_stack.cubalc`

### Prior
See 1.11.4-universal.

## 1.11.4-universal — 2026-08-02

### Direction
- Algocube digit **5** → COP matrix algebra (cube State Matrix ops).

### Language
- `CLEARBITS` · `FILLBITS` · `NOTBITS` · `COPYBITS` · `ANDBITS` · `ORBITS` · `XORBITS` · `NANDBITS`
- Cube queries: `PORTS`/`NPORTS` · `PLUGGED` · `BITS`/`WIDTH`
- Proof `17_cop_matrix_ops.cubalc`

### Prior
See 1.11.3-universal.

## 1.11.3-universal — 2026-08-02

### Direction
- Algocube digit **4** → control-flow expressiveness.

### Language
- `BREAK IF expr` · `CONTINUE IF expr` (aliases `NEXT`/`SKIP`)
- `REPEAT … UNTIL cond` post-test loop
- `WHILE` honors BREAK/CONTINUE; deeper nest depth for LOOP/WHILE
- Proof `16_control_flow_universal.cubalc`

### Prior
See 1.11.2-universal.

## 1.11.2-universal — 2026-08-02

### Direction
- Algocube digit **9** → universal integer data-path (rotate · pack · select).

### Language
- Hex integer literals: `0x…` / `0X…`
- `ROTL`/`ROL` · `ROTR`/`ROR` (32-bit rotate)
- `PACK16`/`PACK` · `HI16`/`HIWORD` · `LO16`/`LOWORD`
- `ISEL`/`SELECT` (expr ternary) · `NEG`
- Proof `15_rotate_pack_universal.cubalc`

### Prior
See 1.11.1-universal.

## 1.11.1-universal — 2026-08-02

### Direction
- Algocube digit **3** → I/O expressiveness (string↔int codecs).

### Language
- I/O codecs under `SYS`: `HEX`/`FROMHEX` · `TOHEX` · `ORD` · `CHR` · `MID`/`SUBSTR`/`SLICE`
- Proof `14_io_codec_universal.cubalc`

### Prior
See 1.11.0-universal.

## 1.11.0-universal — 2026-08-02

### Direction
- **Universal improve loop** every 6 min: NexusCore brief · random algocube · language delta · build/test.

### Language
- Bit algebra: `BAND` `BOR` `BXOR` `BNOT` `SHL` `SHR` `BITCOUNT` `HAMMING32`
- Proof `13_bitops_universal.cubalc`

### Ops
- `scripts/universal_iter.sh` · `scripts/universal_loop_daemon.sh` (360s)
- Plate: `state/evolve/UNIVERSAL_ITER.json`

### Prior
See 1.10.1-science.

## 1.10.1-science — 2026-08-01

### Clarify
- Pure science is a **language design direction**, not a school or diploma product.
- `programs/school` → `programs/science` demos; removed diploma runner / graduation framing.
- `make science` runs language regression demos only.

### Language (retained)
- Science constants + pure ops: `SCIENCE LOAD`, `FORCE`/`KE`/`PE`, `GCD`/`POW`/`HYP`, earth scales, …

### Prior
See 1.10.0-evolve (ops + demos added).

## 1.10.0-evolve — 2026-08-01

### Law
- **evolve** — language keeps flowing after pure_science.

### Language
- Earth/space constants: `EARTH_R` `AU_KM` `YEAR_D` `MOON_D` `SOLAR_C` `ATM_O2` `ATM_N2`
- Helpers: `CLAMP` `AVG` `PCT` `HYP` `CIRC` `AREA_CIRCLE` `WAVE_V` `LIGHT_T` `BOYLE_P2` `ORBIT_PERIOD`

### Demos
- Pure-science language demos (math/physics/chem/bio/earth) under programs/science

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
