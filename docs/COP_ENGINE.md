# CubalC COP — beyond C++ for modern game engines

**Paradigm:** Cube-Oriented Programming / **flow**  
**Version:** see `CUBALC_LANG_VERSION`

CubalC is **not** a C++ dialect. OOP is the *first* reusable layer; COP is the
upgrade: **composition by plug**, **State Matrix as SoT**, **flow before
compile**, binary talk — designed so agents and engines share one language.

## Why more advanced than C++

| C++ / classical OOP | CubalC COP |
|---------------------|------------|
| Class inheritance trees | **Compose** cubes / objects; plug if matrices compatible |
| Virtual tables | **METHOD** bodies + **SEND** (no vptr layout law) |
| Objects as memory bags | **State Matrix** is truth; fields ride optional layers |
| Frame loop in host C++ | **TICK** + **FLOW** in-language world step |
| ECS bolted on later | **ENTITY / SPAWN / SCENE** first-class COP surface |
| Headers / ABI thrash | Pure C11 runtime; one `.cubalc` surface for agents |
| Threads + locks ad hoc | SMX2 binary mesh, LOCKFILE, plate handoffs |

## Layer stack

1. **Functions** — `FN name a b … END` · `CALL name …` · `RET` (named formals)
2. **OOP** — `CLASS` · `FIELD` · `METHOD` · `NEW` · `SEND` · `GETF`/`SETF` · `ISOF`
3. **COP / engine** — `CUBE OF` · `ENTITY` · `SPAWN` · `SCENE` · `TICK` · `PLUG` · `FLOW`

## OOP surface

```cubalc
CLASS Counter
  FIELD count 0
  METHOD init n
    SETF THIS count n
  END
  METHOD inc
    GETF THIS count
    LET c = LAST_N + 1
    SETF THIS count c
    RET c
  END
END

NEW Counter c 10
SEND c inc
CALL c inc          # sugar: CALL obj method
GETF c count
ISOF c Counter
```

## Game-engine surface (COP)

```cubalc
CLASS Entity
  FIELD hp 100
  FIELD x 0
  FIELD y 0
  METHOD tick
    # per-frame: AI, animation, physics step hooks
    RET 1
  END
  METHOD hurt n
    GETF THIS hp
    LET h = LAST_N - n
    SETF THIS hp h
    RET h
  END
END

SCENE level1
ENTITY player OF Entity 100
ENTITY slime OF Entity 20
PLUG player slime

TICK 1              # SEND tick/update/frame to every live object + FLOW 1
SPAWN Entity boss ROLE host 500
FLOW 4              # energy / board law still holds
```

### Conventions for engines

| Method | Role |
|--------|------|
| `init` / `construct` / `spawn` / `new` | Constructor (auto on NEW/SPAWN/ENTITY) |
| `tick` / `update` / `frame` | Called by `TICK` |
| `hurt` / custom | Gameplay messages via `SEND` |

## Flow law (unchanged)

- Energy **must flow** before compile (`FLOW` / plugs / impulse).
- Matrix compatibility gates **PLUG**.
- **HOLD_FLASH** is device/firmware safeguard only (default 1).

## Demo: life engine (cell division)

**Path:** `programs/apps/life_engine/life_engine.cubalc`  
**Cell type:** `programs/lib/life_cell.cubalc` (`INCLUDE life_cell`)

In-language game loop that demonstrates **biology as COP**:

| Step | Form | Biology |
|------|------|---------|
| Scene + medium | `SCENE petri` · `CUBE medium` · `IMPULSE` / `FLOW` | extracellular nutrient |
| Founder | `ENTITY c0 OF Cell 1 4 0` | seed cell |
| Frame | `TICK 1` → live objects' `METHOD tick` | age, uptake, metabolism, ready |
| Mitosis | `SEND daughter birth half gen` · parent `after_div` | cytokinesis / biomass split |
| Pool | pre-allocated free slots (`alive=0`) | dish capacity |

```bash
./out/cubalc run programs/apps/life_engine/life_engine.cubalc
CUBALC_LIFE_STEPS=8 ./out/cubalc run programs/apps/life_engine/life_engine.cubalc
./out/cubalc run programs/proof/865_life_engine_division.cubalc
```

Plate: `$CUBALC_STATE/LIFE_ENGINE.txt` — births · max_pop · max_gen · dish bitmask.

## Proofs

- `programs/proof/862_oop_class_method.cubalc`
- `programs/proof/863_cop_entity_tick.cubalc`
- `programs/proof/865_life_engine_division.cubalc` — mitosis + population growth
- `programs/proof/870_oop_string_fields.cubalc` — SETF/NEW string formals
- Existing: `programs/proof/21_fn_return_case.cubalc`

## Roadmap (still COP, not C++ clone)

- Component bags on cubes (multi-CLASS attach)
- Inheritance optional via **compose** (no deep ISA trees by default)
- Scene graphs as **NEST** + PLUG
- Deterministic sim: MONOTONIC + TICK budgets
- Dynamic SPAWN names for open-ended populations (life_engine uses fixed slots today)
- Render/audio as host SYS adapters — language stays pure C core
