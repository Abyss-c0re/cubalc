# Life Engine (CubalC COP)

A **functional game-engine-style demo** in pure CubalC: petri-dish **cell division**
(mitosis) and a little **life** (growth, aging, starvation death, nutrient medium).

Not a host C++ engine — the loop is **in-language**:

| Engine idea | CubalC form |
|-------------|-------------|
| Entity class | `CLASS Cell` + `METHOD tick` / `birth` / `after_div` |
| Spawn unit | `ENTITY c0 OF Cell …` (pool slots) |
| Frame loop | `TICK 1` then mitosis pass + `FLOW 1` |
| Shared medium | `CUBE medium` + `PLUG` + `IMPULSE` |
| Scene | `SCENE petri` |

## Run

```bash
make all
./out/cubalc run programs/apps/life_engine/life_engine.cubalc

# shorter run
CUBALC_LIFE_STEPS=8 ./out/cubalc run programs/apps/life_engine/life_engine.cubalc

# assert-only smoke
./out/cubalc run programs/proof/865_life_engine_division.cubalc
```

## Biology map

| Mechanic | Biology |
|----------|---------|
| `energy` growth each tick | nutrient uptake |
| basal cost / age death | metabolism & senescence |
| `ready` when energy ≥ 10 | G2/M checkpoint (integer metaphor) |
| `birth` into free slot | cytokinesis / daughter cell |
| `after_div` halves energy | biomass split |
| medium + FLOW | extracellular environment |

## Layout

- `life_engine.cubalc` — full 8-slot dish, multi-step sim, plate write
- `programs/lib/life_cell.cubalc` — reusable `CLASS Cell` (INCLUDE-safe)

## Plate

Writes `$CUBALC_STATE/LIFE_ENGINE.txt` with births, max_pop, max_gen, dish bitmask.
