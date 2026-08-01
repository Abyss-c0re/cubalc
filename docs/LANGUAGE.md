# CubalC language card

## Play forms

| form | meaning |
|------|---------|
| `[name]` / `[name:role]` | place cube |
| `[a~b~c]` | plug chain |
| `[name!]` / `[name!0\|1]` | pulse proton |
| `[~n]` | flow n ticks |
| `?` | show board |
| `[hold]` | hold_flash |
| `[genesis "plate"]` | fold plate → matrix |
| `[sync]` | hive join cubes |
| `[fleet]` | fleet map cubes |

## Core law: only **CUBE** is defined

Everything else is **pluggable I/O** on cubes (ports IN/OUT).  
Plugs wire cubes; **REVERSE** flips I/O direction when needed.

## Statements

`LET` `LOOP`/`WHILE` `FOR`/`EACH` `IF`/`END` `ASSERT` `PRINT`  
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
cubalc peers              # programs/peer_fold.cubalc (env-driven)
cubalc decide "goal"      # translate → braincube path
cubalc law
```

## Pure science (1.9.0-school)

| form | meaning |
|------|---------|
| `SCIENCE LOAD` | inject school constants into vars |
| `PI100` `E100` `G_EARTH` `C_LIGHT` | public-domain scaled constants |
| `ABS` `MIN` `MAX` `POW` `GCD` `LCM` `SQRT` `FACT` | math |
| `FORCE` `WORK` `KE` `PE` | mechanics (integer) |
| `CELSIUS_K` `KELVIN_C` | temperature |

Curriculum: `programs/school/` · `make school` (math + physics + chemistry + biology)

## Machine token

Default status token: `C3`. Share: `smx`. Hold: `1`. Version: `1.9.0-school`.
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
