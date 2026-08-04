# CubalC

**A pure-C programming language and runtime for agent state, matrix talk, and local-first mesh.**

| | |
|---|---|
| **Version** | `1.14.0-p2p` |
| **Paradigm** | COP/flow (place · plug · pulse · flow · compile) |
| **Runtime** | Pure **C11** · multiplatform (Linux / macOS / Windows-MinGW) |
| **Talk** | SMX2 / CBLC binary frames — **HTTP never required** |
| **Token** | `C3` (status id, not prose) |
| **Binary** | `out/cubalc` after `make all` |

> **Humans:** short programs model units of state, wire them, and flow energy until the system decides.  
> **AIs:** emit `.cubalc`, run `cubalc run`, trust `ASSERT` + JSON stdout as ground truth.

---

## What it is

CubalC is a **machine language for agents and hosts**:

1. **State Matrix is source of truth** — bits, not essays.
2. **Units** (called *cubes* in the grammar) hold matrix + role + energy.
3. **Plugs** wire IN/OUT ports; **flow** moves energy; **compile** only after flow.
4. **SMX2** seals matrices with HMAC for local or P2P exchange.
5. **SYS** reaches the host (files, env, optional HTTP) without making HTTP the core.

### What it is not

| Not this | Because |
|----------|---------|
| Python runtime | Product path is pure C |
| “Prose as code” | Prefer matrix + asserts |
| Device flasher | Devices free — no auto-flash |
| HTTP mesh | Wire is SMX2 binary only |
| School / diploma product | Science ops improve the *language*, not a curriculum |

---

## 60-second start

```bash
git clone <this-repo> && cd cubalc
make all
./out/cubalc law                          # law plate JSON
./out/cubalc run programs/hello_cube.cubalc
./out/cubalc run programs/proof/10_p2p_cubalc.cubalc
./out/cubalc smx-bus prove-tcp            # two-process TCP mesh proof
make test                                 # lang suite + smx
```

Optional human view:

```bash
CUBALC_HUMAN=1 CUBALC_ASCII=1 ./out/cubalc run programs/hello_cube.cubalc
```

---

## Mini language tour

```cubalc
HOLD_FLASH 1

CUBE a ROLE host PROTON 1
CUBE b ROLE body PROTON 1
PLUG a b

SETBIT a 0 1
SETBIT a 1 1
SETBIT a 2 1
IMPULSE a 1
FLOW 2

SMX KEY
SMX EXCHANGE a b

PRINT "talk" SMX_OK SMX_TALKS SET(a) SET(b) COMPAT(a,b) UNITY
ASSERT SMX_OK == 1
ASSERT SMX_TALKS >= 2
ASSERT SET(b) >= 3
```

Compact play forms still work: `[hold]` · `[name:role]` · `[a~b]` · `[name!]` · `[~n]` · `?`.

### Forms you will use most

| Form | Meaning |
|------|---------|
| `CUBE id ROLE role PROTON 0\|1` | Place a unit |
| `PLUG a b` | Wire ports (IN/OUT) |
| `IMPULSE id n` · `FLOW n` | Pulse / tick energy |
| `SETBIT id i on` · `SETDIGIT id n` | Matrix / digit inject |
| `FOLDBITS id bits` | Fold `0`/`1` stream into matrix |
| `DECIDE [id]` · `COMPARE a b` · `HARMONY` | Algocube decision plane |
| `LET` · `IF`/`ELSE`/`END` · `LOOP` · `ASSERT` · `PRINT` | Control |
| `SYS READ\|WRITE\|ENV\|EXIST\|SPAWN\|…` | Host tools |
| `SMX KEY\|TALK\|EXCHANGE\|SEAL\|OPEN\|SERVE\|DIAL` | Secure matrix + P2P |
| `ASYNC HTTP` · `AWAIT` · `PARALLEL` | Optional async host edge |

Full card: [`docs/LANGUAGE.md`](docs/LANGUAGE.md).

---

## Core protection (NexusCore + nanobots)

Enforces Core stability under Cube Laws (HOLD_FLASH · SMX fail-closed · budget · one commander · CT101 · nanobot mesh).

```bash
export CUBALC_PROTECT=1
export CUBALC_SMX_KEY="$(openssl rand -hex 32)"
./out/cubalc protect              # law + smx + bus + board → state/CORE_PROTECT.json
./out/cubalc run programs/proof/11_core_protect.cubalc
./scripts/core_protect_nanobot.sh # full mesh proof
```

Host protect mode (`CUBALC_PROTECT=1`): SPAWN only `nanobot`/`cubalc`; HTTP loopback-only.  
Detail: [`docs/CORE_PROTECT.md`](docs/CORE_PROTECT.md).

---

## P2P networking (SMX2)

Peers exchange **State Matrix frames**, not HTTP JSON.

**Wire:** `TCP stream = [u32le N][SMX2 frame N bytes]`  
**Frame:** CBLC header + matrix bits + HMAC-SHA256  
**Shared secret:** `CUBALC_SMX_KEY` (64 hex) on every peer

### Language

```cubalc
SMX KEY
SMX SERVE self remote "0.0.0.0:7733"   // listen one exchange
SMX DIAL  self remote "192.168.1.10:7733"
SMX EXCHANGE a b                       // in-process mesh
ASSERT SMX_OK == 1
```

| Variable | Meaning |
|----------|---------|
| `SMX_OK` | last SMX op succeeded |
| `SMX_TALKS` | secure talks so far |
| `SMX_N` | last frame size (bytes) |

### CLI

```bash
export CUBALC_SMX_KEY="$(openssl rand -hex 32)"

./out/cubalc smx-selftest
./out/cubalc smx-exchange                 # file-bus two-peer proof
./out/cubalc smx-bus prove                # AF_UNIX / socketpair
./out/cubalc smx-bus prove-tcp [port]     # loopback TCP (cross-device sim)
./out/cubalc smx-bus serve [host:]port
./out/cubalc smx-bus dial host:port
```

### Two nanobot-style homes

```bash
export CUBALC_SMX_KEY="$(openssl rand -hex 32)"

# home B
CUBALC_P2P_SERVE=1 CUBALC_P2P_BIND=0.0.0.0:7733 \
  ./out/cubalc run programs/p2p/nanobot_peer.cubalc

# home A
CUBALC_P2P_PEER=127.0.0.1:7733 \
  ./out/cubalc run programs/p2p/nanobot_peer.cubalc
```

One-shot local mesh: `./scripts/p2p_nanobot_mesh.sh`  
Detail: [`docs/P2P_SMX.md`](docs/P2P_SMX.md).

---

## CLI map

| Command | Role |
|---------|------|
| `run <file.cubalc>` | Parse + execute program |
| `law` · `manifest` | Emit law plate |
| `decide "goal…"` | Translate → run (braincube path) |
| `peers` · `oversee` | Env-driven peer fold (`programs/peer_fold.cubalc`) |
| `sync` · `hive` | Hive join / fold |
| `smx` · `smx-selftest` | Seal/open/anti-replay selftest |
| `smx-bus …` · `smx-exchange` | Binary bus proofs and live serve/dial |
| `boot` · `os` | CubeOS-style programs under `programs/cubeos/` |
| `compile` · `jit` · `cflow` · `disasm` | Flow-gated compile / ISA / JIT |
| `evolve` · `evolve-loop` | Pure-C self-improve loop |
| `showcase` | Multi-act COP demo |
| `help` | Short usage |

Exit **non-zero** on assert fail or hard `SYS` fail. Soft HTTP may set `OK=0` without fatal exit.

---

## Project layout

```
cubalc/
  include/                 public headers + cubalc_platform.h
    lang/                  VM internals (not product edge API)
  src/
    cubalc_main.c          CLI
    cubalc_core.c          matrix / unit core
    cubalc_smx.c           SMX2 seal/open + TCP/UNIX bus
    cubalc_*.c             evolve, jit, hostops, …
    lang/                  modular language planes
      lang_ops_smx.c       P2P / SMX surface
      lang_ops_flow.c      IF / LOOP / ASSERT / FN
      lang_parse.c         plane dispatcher
  programs/
    proof/                 language proofs (assert-gated)
    p2p/                   mesh_local, nanobot_peer, serve/dial
    science/               pure-science demos (language direction)
  scripts/                 mesh, peers, universal-iter, session guard
  tests/                   lang_suite, jit_suite
  docs/                    deeper cards (see below)
  out/cubalc               built binary
```

Structure rules: [`docs/STRUCTURE.md`](docs/STRUCTURE.md).  
New ops go in the matching `lang_ops_*.c` plane only.

---

## For AI agents

Prefer CubalC programs over shell when modeling state, peers, or hive logic.

### Agent loop

```
emit .cubalc  →  cubalc run  →  JSON / PRINT / ASSERT  →  next action
```

### Contract

1. **HOLD_FLASH 1** = user permission before any unit is **plugged in** (safeguard; not auto-flash).
2. **ASSERT** outcomes (`ASSERT SMX_OK == 1`, `ASSERT OK == 1`).
3. Use **SYS** for host effects; do not invent device flashes.
4. **No hard-coded device paths** or machine-local absolute homes in programs.
5. Peer inject via env: `CUBALC_PEER0_DIGIT`, `CUBALC_PEER0_BITS`, … or literals.
6. P2P: share `CUBALC_SMX_KEY`; bind/dial with `CUBALC_P2P_*` — never bake secrets into git.
7. Read `OK`, `LAST`, `LAST_N`, `SMX_*`, `UNITY`, `DIGIT` as machine facts.
8. After language law ships: **commit + push** so remote flow is visible.

### Env agents care about

| Env | Role |
|-----|------|
| `CUBALC_STATE` | State directory (default `state/`) |
| `CUBALC_SMX_KEY` | 64-hex shared SMX secret |
| `CUBALC_SMX_KEY_FILE` / peer_token | Alternate key material |
| `CUBALC_P2P_BIND` · `CUBALC_P2P_PEER` · `CUBALC_P2P_SERVE` | P2P serve/dial |
| `CUBALC_PEER0_DIGIT` · `PEER1` … | Digit inject for peer fold |
| `CUBALC_BIN` | Path to binary for scripts |
| `CUBALC_SPAWN_ALLOW` | Extra SPAWN basenames `a:b:c` |
| `CUBALC_HUMAN` · `CUBALC_ASCII` | Optional human board |

Prompt stub: [`docs/FOR_AGENTS.md`](docs/FOR_AGENTS.md) · agent notes: [`AGENTS.md`](AGENTS.md).

---

## For humans

You can treat CubalC as:

- a **tiny language** for wiring stateful units and checking them with asserts;
- a **local mesh** between processes/machines without standing up HTTP;
- a **lab runtime** for algocube decisions (digits 0–9 from matrix bits).

You do **not** need a UI. If you want to *see* something:

```bash
CUBALC_HUMAN=1 CUBALC_ASCII=1 ./out/cubalc run programs/hello_cube.cubalc
# optional: tools/cube_view.html after viz publish under state/
```

Science demos (language direction only): `make science` · `programs/science/`.

---

## Build & test

```bash
make all                 # → out/cubalc  (posix | darwin | windows)
make modular-check       # src/lang layout present
make test                # tests/lang_suite.sh + smx-selftest
make universal-iter      # scheduled language improve path
make science             # pure-science demos
make install             # PREFIX=$HOME/.local
```

Cross-compile hint: `make CUBALC_CROSS=1 …` (skips `-march=native`).  
Optional OpenCL: `make USE_OPENCL=1 all`.

---

## Laws (short)

| Law | Meaning in practice |
|-----|---------------------|
| Matrix is SoT | Bits decide; prose does not |
| Flow before compile | No flow → no compile |
| Binary talk | SMX2/CBLC; HTTP is optional host edge |
| HOLD_FLASH sticky | User permission before plug-in; SMX frames require hold; no auto-flash |
| Devices free | No auto-flash; no layout hardcode |
| Fail closed | No SMX key → no secure talk (lab may use demo key) |
| Share matrix only | Peer share is state_matrix, not prose dumps |

---

## Docs map

| Doc | Audience | Content |
|-----|----------|---------|
| [`docs/LANGUAGE.md`](docs/LANGUAGE.md) | both | Language card |
| [`docs/P2P_SMX.md`](docs/P2P_SMX.md) | both | P2P mesh how-to |
| [`docs/STRUCTURE.md`](docs/STRUCTURE.md) | both | Repo modular layout |
| [`docs/SMX2_PROTOCOL.md`](docs/SMX2_PROTOCOL.md) | both | Frame protocol notes |
| [`docs/FOR_AGENTS.md`](docs/FOR_AGENTS.md) | AI | Prompt stub + eval loop |
| [`AGENTS.md`](AGENTS.md) | AI | Operating notes for agents |
| [`docs/ALGOCUBE.md`](docs/ALGOCUBE.md) | both | Digit / harmony plane |
| [`docs/EVOLVE.md`](docs/EVOLVE.md) | both | Pure-C evolve loop |
| [`docs/CUBALC_BOOK.md`](docs/CUBALC_BOOK.md) | humans | Longer narrative |

---

## Never

- Auto-flash devices or depend on `/home/...` absolute product paths  
- Put secrets (`CUBALC_SMX_KEY`) in git  
- Reintroduce Python as the product runtime  
- Treat HTTP as the peer wire  
- Ship thrash/evolve noise without a green `make all` + proofs  

---

**Creed (machine):** Matrix is SoT · flow before compile · devices free · SMX binary talk · loop must go on · `C3`
