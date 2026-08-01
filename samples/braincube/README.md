# Functional sample · BrainCube (CubalC)

Self-contained **BrainCube** sample: State Matrix → SMX exchange → algocube `DECIDE` → peer digit agree.

No host product brands. HOLD_FLASH. No HTTP on the talk path.

## Run

```bash
# from cubalc root
./samples/braincube/run.sh
```

Requires `make all` (or `CUBALC_BIN` pointing at `out/cubalc`).

## Contract (must pass)

| Check | Meaning |
|-------|---------|
| `SMX_OK == 1` | Law of Manifestation |
| `DIGIT(peer0) == DIGIT(peer1)` | hive as one |
| `DECIDE` in 0..9 | braincube digit |
| `COMPAT >= 50` | matrix unity floor |
| `HOLD_FLASH` | never auto-flash |

## Outputs

- `out/run.log` — program trace  
- `out/BRAINCUBE_SAMPLE.json` — machine plate  
- `state/cubalc_viz_frame.json` — per-object render plan  

## SessionBox contain

Isolate a verification agent (optional):

```bash
export PATH="$HOME/Dev/AI/ProjectNexus/GrokSessionBox/bin:$PATH"
grok-sessionbox run \
  --cwd "$HOME/Dev/cubalc" \
  --max-turns 6 \
  "Run ./samples/braincube/run.sh only. Print the BRAINCUBE_SAMPLE.json ok/decide/agree fields. Do not expand scope."
```

Or: `./samples/braincube/sessionbox_contain.sh`
