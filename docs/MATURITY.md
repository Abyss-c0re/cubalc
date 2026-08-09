# CubalC maturity — corrected against tip (post deep-research)

**Research workflow:** `deep-research-10` (host report; status **Partial**)  
**Research corpus used:** largely `/root/cubalc-flow-1.5` + mixed GitHub main samples  
**This tree tip:** **`1.15.316-usability`** · paradigm **COP/flow** · pure C11  

The research is useful as a **gap checklist**, but several “missing” items are already
implemented here. This note reconciles research claims with **current lab tip**.

---

## Executive take

CubalC is a **real, operational domain language** over a cube machine (not a CLI
shim). Maturity is **early-mid**: strong interpreter + host SYS + assert/selftest
culture; weaker packaging/public adoption and uneven doc/version freeze.

| Dimension | Research said | Tip `1.15.316` reality |
|-----------|---------------|------------------------|
| Version | 1.5.0-flow / README 1.14 | **`CUBALC_LANG_VERSION` 1.15.316-usability** |
| User functions | Not implemented | **`FN` / `CALL` / `RET` + named formals** |
| OOP | Not mentioned | **`CLASS` / `METHOD` / `NEW` / `SEND` / `GETF`/`SETF`** |
| COP / engine | Thin | **`ENTITY` / `SPAWN` / `TICK` / `SCENE` + `FLOW`** · life engine demo |
| Vars | flat 64 | **128 vars** + objects/classes |
| Proof harness | ~9 cases, thin proofs | **`cubalc selftest` ~291 curated proofs** (live plate) |
| Modular lang | monolith `cubalc_lang.c` | **`src/lang/*` ops planes** |
| Bidirectional host data | n/a | **SWAPFILES / DUPLEX / TCPXFER** |
| HOLD_FLASH | n/a | **default 1; device/mesh-join only (not language PLUG)** |

Research **still correct** on: fixed ceilings (cubes/ports/ISA), LOOP bounds,
JIT x86_64 hybrid + trampolines, OpenCL placeholder, synonym-heavy surface,
doc version drift, packaging/adoption, no formal verification.

---

## What is already solid (tip)

1. **Language front end** — LET, IF/WHILE/FOR, ASSERT/EXPECT, INCLUDE, FN, CLASS/METHOD  
2. **Cube machine** — CUBE, PLUG, SETBIT/SETDIGIT, FOLDBITS, DECIDE, FLOW, SMX2  
3. **Host SYS** — large file/env/path/time/encode surface; fail-closed + soft probes  
4. **Verification culture** — assert proofs + `cubalc selftest` plate (not just `lang_suite.sh`)  
5. **Demos** — science programs, realworld boards, **life_engine** (cell division)  
6. **Agent usability** — FOR_AGENTS, COOKBOOK, doctor/version JSON plates  

---

## Research highest-impact items — re-ranked for *this* tip

### P0 — freeze the story (docs/product)
| Action | Why |
|--------|-----|
| **Single version story** | Align README / book / LANGUAGE / AGENTS to `CUBALC_LANG_VERSION` only |
| **Primary grammar card** | Document play forms vs statements as aliases of one grammar |
| **`make prove` or document `selftest`** | Research expected `make prove`; tip has live `cubalc selftest` — pick one name and wire Makefile |

### P1 — scale for real engines / life sims
| Action | Why |
|--------|-----|
| Raise or soft-grow **cube / object / var** ceilings | Life engine pool + larger scenes hit fixed caps |
| **Call frames / local scopes** for FN/METHOD | Today mostly shared vars + ARG* / THIS |
| Dynamic **SPAWN names** (not only preallocated slots) | Cell division had to use free-slot pools |

### P2 — runtime depth
| Action | Why |
|--------|-----|
| Broader **JIT** ports + tests beyond arithmetic | Still x86_64 hybrid |
| Real **OpenCL/GPU** bulk matrix path if product goal | Still probe-ish |
| Deeper **SMX/crypto** external review | Selftest ≠ third-party audit |

### P3 — adoptability
| Action | Why |
|--------|-----|
| Packaging (install tarball, versioned release notes) | GitHub discovery weak |
| CI green badge on selftest | Research found no CI signal |
| Products tree sync cadence | Session standing order; last synced at 1.15.315 |

---

## Explicit research errors (do not re-plan as if true)

| Claim | Correction |
|-------|------------|
| “User functions not implemented” | False on tip — `FN`/`CALL`/`RET`, proofs 21 / 862 |
| “Only ~9 tests” | Incomplete — `cubalc selftest` runs **hundreds** of curated proofs |
| “Version 1.5.0-flow is the product” | Stale snapshot path; tip is **1.15.316-usability** |
| “Local tree has almost no proofs” | Stale; this tree has large `programs/proof/` + science/realworld/apps |

---

## Suggested next three MEANINGFUL ships

1. **Version coherence pass** — single source of truth from `cubalc_law.h` into README/AGENTS/book  
2. **`make prove` alias** → `cubalc selftest` + write `state/LANGUAGE_PROOF.json`  
3. **FN/METHOD local slots** or `LET LOCAL` to reduce global-var footguns in engines  

---

## Source map

| Artifact | Path |
|----------|------|
| Raw research report | session workflow `deep-research-10` scratch `report.md` |
| Session request audit | `docs/SESSION_REQUEST_MANIFEST.md` |
| COP engine design | `docs/COP_ENGINE.md` |
| Life engine demo | `programs/apps/life_engine/` |

*Research status Partial is fair for the old snapshot; tip is substantially further along on language completeness and tests.*
