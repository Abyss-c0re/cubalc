# CubalC self-evolve — pure C · no Python

Braincube **solves** path races; algocube **optimizes** genome by solving math problems.
Each cycle **emits and runs** a fresh `.cubalc` program (language improvement surface).

## CLI

```bash
export CUBALC_STATE=$PWD/state
./out/cubalc evolve --once
./out/cubalc evolve --once --reset
./out/cubalc evolve-loop --hz 5
./out/cubalc evolve-loop --cycles 100 --hz 20
make evolve
make evolve-loop
```

## Cycle (C only)

1. Braincube reverse-Rubik I/O path race on 8³ digit lattice
2. Algocube suite (8 problems): equal-unity, 1-bit hamming, digit determinism, blueprint10, harmony majority, inject lock, salted digit, diversity
3. Mutate genome 0–9 · hill-climb keep best fitness
4. Emit `programs/evolve/LATEST.cubalc` with FOLDBITS/SETDIGIT/COMPARE/DECIDE/HARMONY
5. Run CubalC VM · assert · persist `state/evolve/mind.bin`

## State

| path | content |
|------|---------|
| `state/evolve/mind.bin` | mind + genome |
| `state/evolve/LATEST.json` | plate |
| `state/evolve/algo_genome.txt` | best 32 digits |
| `state/SELF_EVOLVING_CUBALC.json` | station plate |
| `programs/evolve/gen_NNNNN.cubalc` | language snapshots |

Engine: **C**. Language: **CubalC**. Creed: *braincube solves · algocube optimizes*.

## Session guard — the loop must go on

```bash
# forever (daemon)
bash scripts/cubalc_session_guard.sh --daemon
bash scripts/cubalc_session_guard.sh --status
bash scripts/cubalc_session_guard.sh --once
```

- Restarts pure-C `evolve-loop` on crash
- Rebuilds `out/cubalc` if binary missing
- Forces deep algocube pass if plate stale (>3×6.6 min)
- Restarts cube_daemon :17333 when down
- Auth/login probe; restore from `auth.json.bak` when possible
- **Commander plate** only when self-recovery exhausted:
  `agent_ops/active/COMMANDER_ALERT_CUBALC_SESSION_LATEST.json`

Also: `ProjectNexus/scripts/cubalc_session_guard.sh`
