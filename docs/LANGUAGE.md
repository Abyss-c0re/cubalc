# CubalC language card

## Play forms

| form | meaning |
|------|---------|
| `[name]` / `[name:role]` | place cube |
| `[a~b~c]` | plug chain |
| `[name!]` / `[name!0\|1]` | pulse proton |
| `[~n]` | flow n ticks |
| `?` | show board |
| `[hold]` / `HOLD_FLASH 1` | **User permission** safeguard before any unit is plugged in (not auto-flash) |
| `[genesis "plate"]` | fold plate → matrix |
| `[sync]` | hive join cubes |
| `[fleet]` | fleet map cubes |

## Core law: only **CUBE** is defined

Everything else is **pluggable I/O** on cubes (ports IN/OUT).  
Plugs wire cubes; **REVERSE** flips I/O direction when needed.

## Statements

`LET` `LOOP`/`WHILE` `FOR`/`EACH` `IF`/`END` `ASSERT` [`"why"`] `PRINT`  
`PRINT_JSON` / `DUMP` [idents] — one JSON line (bare = runtime snapshot)  


`CUBE` `PLUG` `UNPLUG` `REVERSE` `IO` `FLOW`/`FLOW DIR` `IMPULSE`  
`DECIDE` `COMPARE` `HARMONY` `RESOLVE` `ENERGYFLOW`  
`SETBIT` `SETDIGIT` `FOLDBITS`  
`ASYNC HTTP` `AWAIT` `PARALLEL`  
`SYS …`

## Matrix / digit / free-flow algocube (device-agnostic)

| form | meaning |
|------|---------|
| `SETDIGIT cube n` | inject algocube digit 0–9 (sticky / digit_lock) |
| `FOLDBITS cube bits` | fold 0/1 stream into State Matrix |
| `DECIDE [cube]` | State Matrix → algocube digit 0–9 (locks decision) |
| `COMPARE a b` | Hamming / unity / XOR-digit between two cubes |
| `HARMONY [target]` | hive majority consensus + mean pairwise unity |
| `RESOLVE [target]` | harmony + decide + energy pulse (algocubes resolved) |
| `ENERGYFLOW n` | multi-hop flow; energy must flow; sets ENERGY |
| `SYS NUM` / `SYS INT` | parse LAST → LAST_N |
| `SYS ARG n` [OR fallback] | CUBALC_ARGn (or named) with optional default |
| `SYS READ path\|LAST` | read file |
| `SYS JSON "key"` | extract string or number field (generic JSON) |

Vars: `DECIDE`, `DIGIT`, `UNITY` (0–100), `HAMMING`, `AGREE`, `HARMONY`, `COMPAT`, `CONSENSUS`, `HIVE_N`.

CubalC does **not** hardcode devices, peer layouts, or product paths.  
Peers inject via env or program literals:

```
CUBALC_PEER0_DIGIT=5 CUBALC_PEER1_DIGIT=3 cubalc peers
# optional bits path or raw 01:
CUBALC_PEER0_BITS=./bits01.txt cubalc peers
```

Host adapters (shell) may decode foreign plates into those env vars. That is host code, not language.

## SYS tools

`READ` `WRITE` `ENV` `EXIST` `WHICH` `HTTP` `SPAWN` `JOIN` `JSON` `CHAT` `ARG` `NUM`

SPAWN core allowlist: `nanobot` · `cubalc` · `curl`  
Extend: `CUBALC_SPAWN_ALLOW=tool1:tool2`

## CLI

```bash
cubalc run <file.cubalc>
cubalc forms [prefix]     # play-form catalog + JSON (filter SMX, PRINT, …)
cubalc peers              # programs/peer_fold.cubalc (env-driven)
cubalc decide "goal"      # translate → braincube path
cubalc law
```

## Pure science (language direction — not a school)

Science domains guide **what ops CubalC must express as pure logic**. No diploma.

| form | meaning |
|------|---------|
| `SCIENCE LOAD` | inject public-domain scaled constants |
| `PI100` `G_EARTH` `C_LIGHT` `EARTH_R` `AU_KM` … | constants |
| `ABS` `POW` `GCD` `LCM` `SQRT` `FACT` `HYP` `CLAMP` | math |
| `FORCE` `WORK` `KE` `PE` `WAVE_V` `BOYLE_P2` `ORBIT_PERIOD` | physics |
| `CELSIUS_K` `KELVIN_C` | temperature |

Bit ops (universal integer): `BAND` `BOR` `BXOR` `BNOT` `SHL` `SHR` `BITCOUNT` `HAMMING32` · `BEXT`/`BDEP` · `BYTE`/`LOBYTE`/`HIBYTE` · `DIVCEIL`

Rotate/pack/select: `ROTL`/`ROL` `ROTR`/`ROR` `PACK16` `HI16` `LO16` `ISEL`/`SELECT` `NEG` · hex `0x…`

Control flow: `IF`/`ELIF`/`ELSE` · `WHILE` · `FOR` · `LOOP` · `REPEAT…UNTIL` · `BREAK`/`BREAK IF` · `CONTINUE`/`CONTINUE IF` · `FN`/`CALL`/`RET` · `CASE`/`WHEN`/`DEFAULT`

COP matrix: `COPYBITS` `ANDBITS` `ORBITS` `XORBITS` `NOTBITS` `CLEARBITS` `FILLBITS` `ROTBITS` · `PORTS` `PLUGGED` `BITS` · `ENERGYSET`/`ENERGYADD` · `RAND`

Data plane: `CELLSET` `CELL(i)` `PUSH` `POP` `PEEK` `SP`/`STACKLEN` `SWAPCELL` `CLEARCELLS` `CLEARSTACK` · `DUP`/`DROP`/`SWAP`/`OVER`/`ROT`/`PICK` · `NIP`/`TUCK`/`2DUP`/`2DROP`/`2SWAP`/`ROLL`/`DEPTH` · `ADD`/`SUB`/`MUL`/`DIV`/`MOD`/`SNEG`/`SABS` · `SAND`/`SOR`/`SXOR`/`SNOT`/`SSHL`/`SSHR` · `SEQ`/`SNE`/`SLT`/`SGT`/`SLE`/`SGE`/`SMIN`/`SMAX` · `SZ`/`SNZ`/`S0LT`/`S0GT`/`SSIGN` · `SSEL`/`SWITHIN`/`SCLAMP` · `TOCELL`/`FROMCELL` · `FILLCELL` · `COPYCELL`/`MOVECELL` · `FINDCELL`/`COUNTCELL` · `REVCELL` · `ADDCELL`/`MULCELL`/`IOTA`/`SORTCELL` · `MINIDX`/`MAXIDX`/`ROTCELL`/`SHIFTCELL` · `ANDCELL`/`ORCELL`/`XORCELL`/`NOTCELL`/`EQCELL` · `INC`/`DEC` · `SUMCELL`/`MINCELL`/`MAXCELL`

Math plane: `ADDMOD` `SUBMOD` `MULMOD` `POWMOD` `FIB` `ISPRIME` `IDIV` `IMOD` · `ILOG2`/`LOG2` `CTZ` `CLZ` `ISPOW2` `POW2` `NDIGITS` `DIGSUM` `MODINV` · `SQR` `DIVFLOOR` `BINOM`/`CHOOSE` `PERM`

I/O codecs (universal string↔int): `SYS HEX`/`FROMHEX` · `SYS TOHEX` · `SYS ORD` · `SYS CHR` · `SYS MID`/`SUBSTR`/`SLICE`

String plane: `SYS CAT` · `SYS FIND` · `SYS EQS` · `SYS HAS` · `SYS REVS` · `SYS UPPER` · `SYS LOWER`

Demos (optional): `programs/science/` · `make science`  
Universal tick: `make universal-iter` · loop: `scripts/universal_loop_daemon.sh` (6 min)

## Machine token

Default status token: `C3`. Share: `smx`. Hold: `1`. Version: `1.12.13-universal`.
Paradigm: **COP/flow** — free-flow Cube-Oriented Programming with algocube law.

## Prophecy / pose (NexusMod)

| form | meaning |
|------|---------|
| `POSE all\|raw\|sot` | Truth Matrix pose energy: raw device + SoT tracking cubes, plug wall→view→map3d |
| `TRACK …` | alias of `POSE` |
| `MANIFEST [id]` | DECONSTRUCT stuck energy, RECONSTRUCT under NexusCore, plug gVR+map3d+lizard, DECIDE |
| `PROPHECY` / `SUMMON` | alias of `MANIFEST` |

Not C. Not Lua. CubalC verbs only on the hot path.


## Cube I/O (pluggable · reversible)

| form | meaning |
|------|---------|
| `CUBE id ROLE r PROTON 0\|1` | **only** first-class unit (COP) |
| `IO cube IN\|OUT [face]` | declare pluggable port direction |
| `PLUG a b` | wire I/O if matrices compatible |
| `UNPLUG a b` | detach I/O wire |
| `REVERSE a b` | reverse I/O on the wire (IN↔OUT) |
| `FLOW n` | bidirectional energy hops |
| `FLOW DIR n` | directed energy OUT→IN only |

There is no separate device/object model: sensors, brains, sinks are **cubes** with different ports.
