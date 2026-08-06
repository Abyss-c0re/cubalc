# CubalC — notes for AI agents

You are operating a **CubalC** machine. Prefer CubalC programs over shell scripts when modeling state, peers, or hive ops.

**Start here for humans + agents:** [`README.md`](README.md) (version, P2P, CLI, layout).  
**Prompt stub:** [`docs/FOR_AGENTS.md`](docs/FOR_AGENTS.md).

## Identity

| Field | Value |
|-------|--------|
| Language | CubalC / C³ / **COP/flow** |
| Version | `1.15.232-usability` (see `CUBALC_LANG_VERSION` in `include/cubalc_law.h`) |
| Runtime | Pure C11 · multiplatform |
| Binary talk | SMX2 / CBLC · State Matrix SoT · **HTTP never required** |
| Token | `C3` (opaque status id) |
| Not | Python product path · prose-as-code · device flasher |

## How to run

```bash
make all
./out/cubalc run <file.cubalc>
./out/cubalc run programs/proof/10_p2p_cubalc.cubalc
./out/cubalc smx-bus prove-tcp
./out/cubalc decide "goal text"
./out/cubalc peers
./out/cubalc law
./scripts/p2p_nanobot_mesh.sh
./scripts/peer_fold.sh
```

Exit non-zero on assert fail / hard SYS fail. Soft HTTP may set `OK=0`.

## Write programs agents can verify

1. **Do not** start every program with `HOLD_FLASH 1`. Runtime already defaults
   `hold_flash=1`. `HOLD_FLASH` is a **device/firmware connection safeguard**
   (use `HOLD_FLASH 0` to deny PLUG). See `docs/HOLD_FLASH.md`.
2. Prefer `SYS` for host effects; do not invent flash/device writes.
3. Assert outcomes: `ASSERT SMX_OK == 1`, `ASSERT OK == 1`, `ASSERT LAST_N > 0`.  
   Optional reason: `ASSERT ready == 1 "peer not ready"` → err includes line + message.  
   Soft-fail / fatal set sticky `ERR` / `LAST_ERR` (survives later `LAST` overwrites).
4. Keep strings short; dump machine facts with `PRINT` / `?`.
5. **Do not hardcode devices, product paths, or peer file formats** in `.cubalc`.
6. Peer digits: env (`CUBALC_PEER0_DIGIT=…`) or literals; `SETDIGIT` after flow when needed.
7. Free-flow algocube: `FOLDBITS` → `COMPARE` / `DECIDE` / `HARMONY` — bits only.
8. P2P: `SMX KEY` + `SERVE`/`DIAL`/`EXCHANGE`; share `CUBALC_SMX_KEY` only via env.

## SYS tools (host)

| Form | Effect |
|------|--------|
| `SYS READ "path"\|LAST` | LAST = content |
| `SYS WRITE path data` | write file |
| `SYS ENV name` | LAST = env · LAST_N = length |
| `SYS EXIST "path"\|LAST` | EXIST / LAST_N |
| `SYS WHICH name` | resolve bin |
| `SYS HTTP GET\|POST "url"` | optional edge (allowlisted) |
| `SYS SPAWN bin args…` | allowlist + `CUBALC_SPAWN_ALLOW` |
| `SYS JOIN a b` | path join → LAST |
| `SYS JSON "key"` | field from LAST |
| `SYS NUM` / `SYS INT` | parse LAST → LAST_N |
| `SYS CHAT "local"\|"grok"` | chat; msg from env/string |
| `SYS ARG n\|name` [OR fallback] | CLI/env args (`CUBALC_ARGn`) with optional default |

## SMX / P2P (language)

| Form | Effect |
|------|--------|
| `SMX KEY` | load key (env / token / lab demo) |
| `SMX TALK a b` | secure a→b matrix transfer |
| `SMX EXCHANGE a b` | a→b then b→a |
| `SMX SEAL a b "path"` | write sealed frame · `SMX_N` |
| `SMX OPEN b "path"` | open frame into b |
| `SMX SERVE local remote "host:port"` | TCP listen one exchange |
| `SMX DIAL local remote "host:port"` | TCP dial one exchange |

Vars: `SMX_OK` · `SMX_TALKS` · `SMX_N` · `ERR` / `LAST_ERR` (sticky fail text).  
Wire: `[u32le N][SMX2 frame]`. Docs: [`docs/P2P_SMX.md`](docs/P2P_SMX.md).

## Peer / digit ops

| Form | Effect |
|------|--------|
| `SETDIGIT cube n` | inject digit 0–9 (sticky digit_lock) |
| `FOLDBITS cube bits` | fold 0/1 into State Matrix |
| `DECIDE brain` | matrix → algocube digit |

| Env | Meaning |
|-----|---------|
| `CUBALC_SMX_KEY` | 64-hex shared secret |
| `CUBALC_P2P_BIND` / `PEER` / `SERVE` | serve/dial control |
| `CUBALC_P2P_TIMEOUT` | SERVE accept timeout ms (default 30000) |
| `CUBALC_P2P_SOFT` | DIAL soft-fail (`SMX_OK=0`) |
| `CUBALC_PEER0_DIGIT` / `PEER1` | digit 0–9 text |
| `CUBALC_PEER0_BITS` / `PEER1` | path or raw 01 stream |
| `CUBALC_STATE` | state directory |
| `CUBALC_SPAWN_ALLOW` | extra SPAWN basenames `a:b:c` |

Quick readiness: `./out/cubalc doctor` · recipes: `docs/COOKBOOK.md` · `cubalc cookbook`

## Humans (optional)

Do **not** require visuals. If a human asks to *see* units:

```bash
CUBALC_HUMAN=1 CUBALC_ASCII=1 cubalc run programs/hello_cube.cubalc
```

## Core protection

```bash
export CUBALC_PROTECT=1
./out/cubalc protect
./scripts/core_protect_nanobot.sh
```

Law id `core_protect` · plate `state/CORE_PROTECT.json` · see `docs/CORE_PROTECT.md`.

## Never

- Auto-flash devices  
- Depend on machine-local absolute paths (`/home/...`)  
- Reintroduce Python as product path  
- Put SMX secrets in git  
- Use HTTP as the peer wire  
- End a language ship without commit + push when remote flow is expected  

## Continuous remote flow

- After each language law ship: **commit + push** (idle remote looks like a dead machine).
- If worktree regresses (mass deletes): `git restore --source=HEAD` then rebuild; never push broken tree.
- Product consumers: set `CUBALC_BIN` / `PATH` to `out/cubalc`.
