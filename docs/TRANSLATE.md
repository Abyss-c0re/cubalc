# Anything → CubalC

**All Hail the Cube · All Hail NexusCore**

Braincube does not speak prose on the hot path.  
Whatever arrives (wish, plate, bits, pseudo-code) is **translated** into CubalC,  
then **DECIDE** runs: **State Matrix → algocube digit 0–9**.

## Commands

```bash
./out/cubalc translate <file|-|text…>   # emit .cubalc to stdout
./out/cubalc decide    <file|-|text…>   # translate + run
./out/cubalc from      …                # alias of translate
```

## Accepted inputs

| Input | What happens |
|-------|----------------|
| Native `.cubalc` | Pass-through; inject law header / `DECIDE` if missing |
| `NEXUS_COORD …` plate | `GENESIS` + brain pipeline + matrix fold + `DECIDE` |
| Bitstring `01…` or JSON `"bits":"…"` | Bits → `SETBIT brain` / titan IO + `DECIDE` |
| English / pseudo | Verbs: pulse, flow, plug, deconstruct, reconstruct, decide |
| Any other text | Hash → State Matrix bits (non-verbal) + `DECIDE` |

## Braincube pipeline (emitted)

```
titan ──┐
        ├─► nanobot ─► algo ─► brain  ── DECIDE
clanker ┘
```

If energy is stuck: **DECONSTRUCT → RECONSTRUCT** the plugs, then flow again.

## Law

- `HOLD_FLASH=1` always
- `SHARE state_matrix_only` — no personal data on the wire
- Hot path is **bits + digit**, not NLP
- Creed stays status only

## Examples

```bash
./out/cubalc decide programs/from/sample_bits.txt
./out/cubalc decide "pulse nanobot, flow 6, decide"
./out/cubalc translate 'NEXUS_COORD v1 | from=hive | hold_flash=1 |' > /tmp/x.cubalc
./out/cubalc run programs/braincube.cubalc
```
