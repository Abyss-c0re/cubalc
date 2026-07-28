# CubalC — notes for AI agents

You are operating a **CubalC** machine. Prefer CubalC programs over shell scripts when modeling state, tools, or hive ops.

## Identity

- **Language:** CubalC / C³ / COP (Cube-Oriented Programming)
- **Not:** Python, not prose-as-code, not “just C”
- **Binary talk:** CBLC frames · State Matrix SoT
- **Token:** `C3` (opaque status id, not worship text)

## How to run

```bash
make -C <cubalc-root> all
./out/cubalc run <file.cubalc>
./out/cubalc decide "goal text"     # translate → braincube path
./out/cubalc sync [plate]           # hive fold
./out/cubalc peers                  # env-driven peer fold (no device hardcode)
./out/cubalc smx-exchange           # SMX2 two-peer matrix talk (HMAC, anti-replay)
# host adapter (optional): decode peer plates → CUBALC_PEER* env
./scripts/peer_fold.sh
# two live nanobots + SMX1/SMX2 proof:
./scripts/deploy_two_nanobot_smx.sh
```

Exit: non-zero on assert fail / hard SYS fail. Soft optional HTTP may set `OK=0`.

**CubalC does not need HTTP.** Prefer `SMX TALK` / `SMX EXCHANGE` / `smx-bus` / file frames.

## Write programs agents can verify

1. Start with `[hold]` / `HOLD_FLASH 1` when mutating.
2. Prefer `SYS` for host effects; do not invent flash/device writes.
3. Assert outcomes: `ASSERT LAST_N > 0`, `ASSERT OK == 1`.
4. Keep strings short; dump machine facts with `PRINT` / `?`.
5. **Do not hardcode devices, product paths, or peer file formats** in `.cubalc` or the VM.
6. Peer digits: inject via env (`CUBALC_PEER0_DIGIT=…`) or literals; `SETDIGIT` after `FLOW`.

## Tools (SYS)

| form | effect |
|------|--------|
| `SYS READ "path"\|LAST` | LAST = content |
| `SYS WRITE path data` | write file (path/data may be LAST) |
| `SYS ENV name` | LAST = env |
| `SYS EXIST "path"\|LAST` | EXIST / LAST_N |
| `SYS WHICH name` | resolve bin |
| `SYS HTTP …` | **optional host edge only** — not for matrix peer talk |
| `SYS SPAWN bin args…` | allowlist: `nanobot`/`cubalc` + `CUBALC_SPAWN_ALLOW` |
| `SYS JOIN a b` | path join → LAST |
| `SYS JSON "key"` | extract string **or number** field from LAST |
| `SYS NUM` / `SYS INT` | parse LAST → LAST_N integer |
| `SYS CHAT "local"\|"grok"` | chat; msg from `GROKIUM_MSG` / string |
| `SYS ARG n\|MSG\|BACKEND\|MODEL` | CLI/env args |

## Peer board ops (abstract)

| form | effect |
|------|--------|
| `SETDIGIT cube n` | inject algocube digit 0–9 (sticky digit_lock) |
| `FOLDBITS cube bits` | fold 0/1 stream into State Matrix |
| `DECIDE brain` | State Matrix → algocube digit |
| `SMX TALK a b` | **Law of Manifestation** — secure matrix a→b |
| `SMX EXCHANGE a b` | bidirectional SMX2 talk |
| `SMX SEAL a b path` / `SMX OPEN b path` | file-bus frames |
| `SMX SERVE local remote bind` | TCP listen one exchange (P2P) |
| `SMX DIAL a b "host:port"` | TCP dial (LAN/WAN, no HTTP) |
| `SMX KEY` | reload SMX key from env |

P2P for nanobot is **written in CubalC** (`programs/p2p/`):

```bash
export CUBALC_SMX_KEY="$(openssl rand -hex 32)"
# peer B
CUBALC_P2P_SERVE=1 CUBALC_P2P_BIND=0.0.0.0:7733 \
  cubalc run programs/p2p/nanobot_peer.cubalc
# peer A
CUBALC_P2P_PEER=192.168.x.y:7733 \
  cubalc run programs/p2p/nanobot_peer.cubalc
# local mesh proof
./scripts/p2p_nanobot_mesh.sh
```

### Law of Manifestation

Peers **manifest** only by exchanging State Matrix over SMX2 (HMAC, anti-replay, HOLD_FLASH).  
Prose is not talk. Assert `SMX_OK == 1` and `SMX_TALKS >= 1` after exchange.

Peer env contract (language surface only):

| env | meaning |
|-----|---------|
| `CUBALC_PEER0_DIGIT` / `PEER1` | digit 0–9 as text |
| `CUBALC_PEER0_BITS` / `PEER1` | path to 01 stream **or** raw 01 string |
| `CUBALC_SPAWN_ALLOW` | extra SPAWN basenames `a:b:c` |

## Humans (optional)

Do **not** require visuals. If the human asks to *see* cubes:

```bash
CUBALC_HUMAN=1 CUBALC_ASCII=1 cubalc run programs/hello_cube.cubalc
# or open tools/cube_view.html after viz publish
```

## Never

- Auto-flash devices
- Depend on machine-local absolute paths (`/home/...`)
- Reintroduce Python as product path
- Put creed/worship prose in machine I/O

## Product consumers

Grokium and other hosts should vendor or submodule this repo under `deps/cubalc` and discover `out/cubalc` via `CUBALC_BIN` / `PATH`.
