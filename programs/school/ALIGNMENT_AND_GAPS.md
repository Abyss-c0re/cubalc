# CubalC School — Educational Alignment & Gap Report

**Date:** 2026-08-01  
**Language:** CubalC **1.9.0-school**  
**Scope:** pure science curriculum (`programs/school/`) + COP runtime  
**Note:** Deep-research workflow also evaluated an older **1.5.0-flow** snapshot; this plate is corrected against the **live** tree.

---

## Executive verdict

| Lens | Alignment |
|------|-----------|
| **Own program A — Nanobot School (COP / CubeChain)** | **Strong** — language + runtime implement cubes, matrix SoT, plugs, flow, protons/energy, algocube, nest/compile |
| **Own program B — Pure Science School (math/physics/chemistry/biology)** | **Partial / metaphor-strong** — 38/38 lessons **run and ASSERT**, but many disciplinary laws are **integer models**, not host physics |
| **Full K–12 disciplinary science (NGSS-style)** | **Not a substitute** — CubalC is a **logic + systems** language; it cannot replace labs or continuous physical simulation |

**Bottom line:** Implemented science is **aligned with CubalC’s educational design** (pure logic on cubechain). It is **not** a full laboratory science platform. Gaps below are honest limits, not bugs.

---

## What is aligned (manifested today)

### A. Cube-Oriented Program (book / COP)
| Requirement | Status |
|-------------|--------|
| Cube as only first-class unit | Yes — `CUBE`, ports, roles |
| State Matrix SoT (64-bit) | Yes — `FOLDBITS`, `SETBIT`, `SET`/`POPCOUNT` |
| Plug when matrices compatible | Yes — `PLUG` / unplug / reverse I/O |
| Binary talk + energy flow | Yes — `FLOW`, `FLOW DIR`, `ENERGYFLOW`, talk |
| Proton create/destroy | Yes — `PROTON`, `IMPULSE` |
| Algocube digit 0–9 | Yes — `DIGIT`, `SETDIGIT`, compare/harmony |
| Nest + compile-to-matrix (flow gate) | Yes — `NEST`, `COMPILE` (no flow → no compile) |
| Control flow + asserts | Yes — `LET` `IF` `LOOP` `WHILE` `ASSERT` |
| Proof suite | Yes in tree — `programs/proof/01…12` (not only 09) |

### B. Pure Science School (`programs/school/`) — **38 lessons, all green**
| Domain | Lessons | How science is manifested |
|--------|---------|---------------------------|
| **Math** | 10 + midterm | Native functions: `ABS` `POW` `GCD` `LCM` `SQRT` `FACT` + integer algebra |
| **Physics** | 7 + midterm | `FORCE` `KE` `PE` `WORK` + plugs as circuits + flow as energy path |
| **Chemistry** | 7 + midterm | Atoms as cubes, bonds as plugs/nest, stoich as integer balance |
| **Biology** | 7 + midterm | Cell nest, diffusion as FLOW, Mendel as bits, eco energy pyramid |
| **Foundations** | 2 | Measurement constants + cubechain-as-nature |
| **Final** | 1 | Cross-domain pure-logic exam |

Run: `make school` → `SCHOOL pass=38 fail=0`.

### C. Language science surface (1.9.0)
- `SCIENCE LOAD` + constants: `PI100` `G_EARTH` `C_LIGHT` `WATER_K` `R_GAS` …
- Law token: `pure_science` (id 15)

---

## Gaps — cannot be fully manifested (or only as metaphor)

### 1. Host physics / continuous nature
| Gap | Why |
|-----|-----|
| Real forces, fields, continuous time | Chain is discrete ticks + integer math; **not** a physics engine |
| Conservation of energy as SI joules | CubalC energy is **0..1 charge on plug talk**, not joules |
| Quantum / relativity / EM waves as fields | No wave PDE or field mesh |
| Lab measurement error / instruments | No sensor hardware model (except optional host SYS) |

**Can teach:** *laws as pure relations* (F=ma, PV=nRT scale, balances).  
**Cannot teach alone:** experimental uncertainty, real apparatus, continuous dynamics.

### 2. Chemistry depth
| Gap | Why |
|-----|-----|
| Electron orbitals, spectroscopy | No orbital basis |
| Equilibrium constants / kinetics rates | Only integer stoich and flow metaphors |
| Organic mechanisms | No graph chemistry engine |
| Lab safety / titration curves continuous | Integer pH exponent only |

**Can teach:** atom Z as digit, H₂O nest/plug, balance equations, state vs T tables.  
**Cannot teach alone:** lab chem, molecular dynamics, full periodic trends.

### 3. Biology depth
| Gap | Why |
|-----|-----|
| Living wetware / wet labs | Cubes are models, not cells |
| Full genetics (linkage, multi-locus, population) | Bit Punnett only |
| Evolution simulation (selection dynamics) | Not a population genetics package |
| Anatomy / physiology detail | Nest hierarchy is schematic |

**Can teach:** cell as nested cubes, diffusion metaphor, photo/resp atom balance, 3:1 Mendel, trophic 10% rule, setpoint feedback.  
**Cannot teach alone:** real organisms, microscopy, ecology field work.

### 4. Earth & space (NGSS ESS)
| Gap | Status |
|-----|--------|
| Geology, weather, climate systems | **Mostly missing** as curriculum lessons |
| Astronomy / orbits (n-body) | Only light travel-time scale; no orbital integrator |
| Earth systems (carbon cycle continuous) | Not shipped as dedicated track |

### 5. Math limits (honest)
| Gap | Why |
|-----|-----|
| Real/complex floating point | Integer CubalC (scaled constants, e.g. π×100) |
| Symbolic algebra CAS | No computer-algebra kernel |
| Unbounded proof / Turing completeness claim | LOOP/WHILE **capped**; cube budget bounds state |
| Continuous calculus | Discrete finite differences only if hand-coded |

**Can teach:** arithmetic laws, GCD/LCM, sequences, linear systems, integer geometry, basic probability.  
**Cannot teach alone:** full analysis, real analysis, symbolic proofs.

### 6. Legacy Nanobot School graduation path
| Artifact | Live tree status |
|----------|------------------|
| `school/run_student.sh`, apps A–D (shell) | **Present** under `school/` |
| Student `DIPLOMA.json` sample | Present under `school/students/` |
| Deep-research claim “school/ missing” | **Stale** (true for 1.5.0-flow snapshot, not for live cubalc) |
| Apps A–D as **native `.cubalc` pure science** | Still mostly shell/json — not the pure-science track |

**Gap:** A–D nanobot apps are **systems exercises**, not merged into pure-science math/phys/chem/bio track. Two school tracks exist; they are not one unified diploma yet.

### 7. Pedagogy / assessment
| Gap | Status |
|-----|--------|
| Teacher rubrics / grade bands | Not automated beyond ASSERT pass/fail |
| Progressive difficulty / prerequisites graph | Ordered filenames only |
| Multilingual / accessibility | English tokens only |
| Cheating-resistant exams | Final is open source in repo |

---

## What CubalC *is* good for in education

1. **Pure logic of science laws** — if it can be written as integers + relations + asserts, it can run.  
2. **Systems thinking** — entity / relation / flow / containment (cube / plug / flow / nest).  
3. **Computational science literacy** — state, identity as matrix, fail-closed compile after flow.  
4. **Honesty of models** — students see that “energy” here is a **model**, not the world.

---

## Recommended next steps (if closing gaps)

| Priority | Action |
|----------|--------|
| P1 | Add **Earth & space** mini-track (seasons scale, rock cycle as nest states, carbon cubes) |
| P1 | Add **math floating-point scale** docs + more algebra/geometry lessons (still integer) |
| P2 | Bridge Nanobot apps A–D → pure `.cubalc` with diploma runner that includes pure-science final |
| P2 | Optional **host lab adapters** (SYS) for real sensors — clearly labeled “host”, not chain physics |
| P3 | Teacher guide: when metaphor ends and lab begins |

---

## Creed

Energy must flow · matrix is key · pure logic · models are not the world · All Hail The Cube · All Hail NexusCore


---

## Evolve update (1.10.0-evolve)

Closed since first gap plate:
- **Earth & space track** (`earth/` 8 lessons + midterm)
- Deeper math 11–14, physics 08–09, chem 08, bio 08
- **Native apps A–D** in `programs/school/apps/*.cubalc`
- **Unified diploma** `run_diploma.sh` / `make diploma`
- Language: earth constants + `CLAMP` `HYP` `ORBIT_PERIOD` `LIGHT_T` `BOYLE_P2` …

Still open (by design): host-physics labs, floating CAS, wet biology.
