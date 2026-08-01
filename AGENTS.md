# CubalC — notes for AI agents

You are operating a **CubalC** machine. Prefer CubalC programs over shell scripts when modeling state, tools, or hive ops.

## Identity

- **Language:** CubalC / C³ / **COP/flow** (free-flow Cube-Oriented Programming)
- **Version:** 1.6.0-showcase · algocube law engine first-class
- **Not:** Python, not prose-as-code, not “just C”
- **Binary talk:** CBLC frames · State Matrix SoT · HTTP never required
- **Token:** `C3` (opaque status id, not worship text)
- **Law layer:** algocube digits 0–9 · COMPARE / HARMONY · digit_lock sticky

## How to run

```bash
make -C <cubalc-root> all
./out/cubalc run <file.cubalc>
./out/cubalc run programs/free_flow_prophecy.cubalc
./out/cubalc run programs/proof/09_algocube_harmony.cubalc
./out/cubalc decide "goal text"     # translate → braincube path
./out/cubalc sync [plate]           # hive fold
./out/cubalc peers                  # env-driven peer fold (no device hardcode)
# host adapter (optional): decode peer plates → CUBALC_PEER* env
./scripts/peer_fold.sh
```

Exit: non-zero on assert fail / hard SYS fail. Soft HTTP may set `OK=0`.

## Write programs agents can verify

1. Start with `[hold]` / `HOLD_FLASH 1` when mutating.
2. Prefer `SYS` for host effects; do not invent flash/device writes.
3. Assert outcomes: `ASSERT LAST_N > 0`, `ASSERT OK == 1`.
4. Keep strings short; dump machine facts with `PRINT` / `?`.
5. **Do not hardcode devices, product paths, or peer file formats** in `.cubalc` or the VM.
6. Peer digits: inject via env (`CUBALC_PEER0_DIGIT=…`) or literals; `SETDIGIT` after `FLOW`.
7. Free-flow algocube: `FOLDBITS` → `COMPARE` / `DECIDE` / `HARMONY` — bits only.
8. See `docs/FREE_FLOW.md` and `docs/ALGOCUBE.md`.

## Tools (SYS)

| form | effect |
|------|--------|
| `SYS READ "path"\|LAST` | LAST = content |
| `SYS WRITE path data` | write file (path/data may be LAST) |
| `SYS ENV name` | LAST = env |
| `SYS EXIST "path"\|LAST` | EXIST / LAST_N |
| `SYS WHICH name` | resolve bin |
| `SYS HTTP GET\|POST "url"` | loopback + allowlisted API |
| `SYS SPAWN bin args…` | allowlist: `nanobot`/`cubalc`/`curl` + `CUBALC_SPAWN_ALLOW` |
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

## Continuous remote flow (law)

- Local evolve-loop is not enough — **GitHub must see commits** or the Cube looks idle.
- After each language law ship: commit + push; do not end the turn as if remote flow is optional.
- If worktree regresses (mass deletes), `git restore --source=HEAD` then rebuild; never push broken tree.
- Report idle gaps to NexusCore; energy must flow.
