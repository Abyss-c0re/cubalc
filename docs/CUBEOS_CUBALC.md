# CubeOS way — CubalC (braincube core)

**Version:** 0.8.0-cubeos  
**Law:** The Cube is sovereign · **OS is only the path** · HOLD_FLASH · devices free  
**Creed:** All Hail the Cube · All Hail NexusCore  

## Separation (no regression)

| Layer | Role |
|-------|------|
| **CubalC CubeOS** (`programs/cubeos/*`) | OS lattice as cubes; **braincube decides** |
| **c_cubeos** (`:17333` cube_daemon) | Existing C way API / Kinect / pick — **unchanged** |
| **LOVR** | Machine-native cube UI from `state/cubalc_viz_frame.json` |

CubalC does **not** replace the daemon hot path. It **is** the CubeOS program layer: boot, tick, decide, energy flow.

## Boot lattice

```
ways (linux/android/xr) ─┐
titan / clanker IO ──────┼─► nanobot ─► algo ─► brain* ─► hive ─► purposes
sot ─────────────────────┘              (*core DECIDE)
```

If energy stalls: `DECONSTRUCT` → `RECONSTRUCT` → re-plug → `FLOW`.

## Commands

```bash
export PROPHECY_CUBE_ROOT=~/Dev/lab/prophecy_cube
export CUBALC_STATE=$PROPHECY_CUBE_ROOT/state
export HOLD_FLASH=1

make -C cubalc
./out/cubalc boot          # full CubeOS lattice + DECIDE
./out/cubalc os tick       # one energy+decide cycle
bash scripts/cubeos_flow.sh 8
```

Publishes:

- `$CUBALC_STATE/cubalc_viz_frame.json`
- `$PROPHECY_CUBE_ROOT/state/viz_frame.json` (LOVR prefer path)

## LOVR

`lovr/cubalc_lego.lua` draws **energy bars, algocube digits, binary wires, brain pulse** — matrix-native, not prose theater.

## Prove

`make prove` includes `07_cubeos` — no language regression.
