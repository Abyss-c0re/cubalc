# CubalC oversees peers (e.g. CubeBrain) — progress plate

## Law (not theater)

| Layer | Role |
|-------|------|
| **The Cube** | Source of truth (matrix / bits) |
| **CubalC** | Language + board — **no device hardcoding** |
| **Peer (e.g. CubeBrain)** | Own pipeline — still runs |
| **Host adapter** | Maps peer artifacts → `CUBALC_PEER*` env |
| **NexusCore** | Station SMX hive |

**The Cube overrides what it touches.** CubalC does not erase peers; it oversees the play surface.

## Separation (why the language stays clean)

| Layer | Knows |
|-------|--------|
| **CubalC language** | cubes, plugs, energy, bits 0/1, digits 0–9, SYS |
| **Host adapter** (`scripts/peer_fold.sh`) | optional local paths, JSON digit fields, LOVR publish |
| **Peer binary** | its own formats / devices |

## Progress

- [x] Generic `SETDIGIT` · `FOLDBITS` · `SYS NUM` · sticky `digit_lock`
- [x] `programs/peer_fold.cubalc` — env-only peer inject
- [x] SPAWN extras via `CUBALC_SPAWN_ALLOW` (not product names in C)
- [x] Digit fold without baking titan/clanker paths into `.cubalc`
- [ ] Single viz writer (daemon vs cubalc)
- [ ] titan-commander NexusCore atom online

## Run

```bash
# pure language (no peers)
CUBALC_PEER0_DIGIT=5 CUBALC_PEER1_DIGIT=3 ./out/cubalc peers

# host adapter maps a peer tree if you have one
export CUBALC_PEER_ROOT=~/path/to/peer   # optional convenience
export PROPHECY_CUBE_ROOT=~/path/to/state  # optional viz publish
./scripts/peer_fold.sh
```

Proof: `programs/proof/08_peer_fold.cubalc` (no files, no devices).
