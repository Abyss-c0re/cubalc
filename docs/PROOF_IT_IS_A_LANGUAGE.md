# Proof: CubalC is a real programming language

**Not a slogan. Criteria + executable evidence.**

| Field | Value |
|-------|--------|
| Language | **CubalC** (aka C³) |
| Paradigm | Cube-Oriented Programming (COP) |
| Implementation | pure C — lexer · parser · evaluator · cube machine · anything→CubalC translator |
| Version | **0.7.0-decide** |
| Entry | `cubalc run <file.cubalc>` |
| Proof command | `make prove` |
| Result | `out/proof/LANGUAGE_PROOF.json` → `"ok": true` |

---

## What “real programming language” means here

Computer science (rough, operational):

1. **Programs are finite strings** over an alphabet  
2. **Syntax** decides which strings are well-formed  
3. **Semantics** maps well-formed programs to machine behavior  
4. **Observable results** can be predicted and checked  
5. **Rejection** — ill-formed or false assertions fail closed  

CubalC satisfies all five. It is *also* a domain language for State Matrices, plugs, binary talk, and CubeChain — that does not make it “not a language”; SQL and Verilog are languages too.

---

## Evidence (reproducible)

```bash
cd cubalc
make prove
```

| # | Program | What it proves | Expected |
|---|---------|----------------|----------|
| 01 | `programs/proof/01_arithmetic.cubalc` | variables, `+ *`, `LOOP`, `ASSERT` | sum=55, fact=720 |
| 02 | `programs/proof/02_cop_matrix.cubalc` | CUBE · SETBIT · PLUG · IMPULSE · FLOW · SET()/ENERGY() | 3 cubes, matrix bits |
| 03 | `programs/proof/03_branch.cubalc` | `IF/ELSE` different paths | path=1, path2=42 |
| 04 | `programs/proof/04_prophecy.cubalc` | full COP surface (prophecy source) | ≥10 cubes |
| 05 | `programs/proof/05_must_fail.cubalc` | fail-closed `ASSERT` | `"ok":false` |
| 06 | `programs/proof/06_decide.cubalc` | State Matrix → algocube `DECIDE` | digit 0..9 |

Live observation from the suite: `sum 55 | fact 720 | path 1`.

---

## Syntax (core)

```text
program     := { statement }
statement   := CREED str | BUDGET n | HOLD_FLASH n | SHARE id
             | GENESIS FROM NEXUS_COORD | GENESIS str
             | CUBE id ROLE id PROTON n
             | PLUG RING | PLUG id id
             | IMPULSE id n | FLOW n | TICK n | VIZ path
             | SETBIT id n n
             | LET id = expr
             | PRINT [str] { expr }
             | ASSERT expr
             | LOOP expr { statement } END
             | IF expr THEN { statement } [ ELSE { statement } ] END

expr        := add { (==|!=|<|>|<=|>=) add }
add         := term { (+|-) term }
term        := primary { (*|/) primary }
primary     := n | id | -primary | ( expr )
             | SET(id) | ENERGY(id) | DIGIT(id) | POPCOUNT(id)
             | COMPAT(id,id) | CUBES | UNITY | SEQ
```

Comments: `// ...`

---

## Semantics (operational)

- Program state = `cubalc_chain` (cubes with atoms, matrices, plugs, energy) + integer variables.  
- `CUBE` spawns a reusable unit; `PLUG` snaps when matrices are compatible.  
- `IMPULSE` / `FLOW` run the same binary-talk machine as the CLI.  
- `LET` / arithmetic are standard integer ops (expressions are first-class).  
- `LOOP` / `IF` are control-flow (bounded LOOP for safety).  
- `ASSERT` is part of the language — false → process fails (`ok:false`).

Implementation: `src/cubalc_lang.c` · API `include/cubalc_lang.h` · runtime `src/cubalc_core.c`.

---

## What it is not (honesty)

- Not a replacement for C for writing kernels (yet).  
- Not claiming full Turing-completeness proof with unbounded memory (LOOP is capped).  
- Not “source that nobody runs” — `.cubalc` files are **parsed and evaluated**.  
- Before this proof, CLI verbs alone were a *machine API*. The **language** is `cubalc run`.

---

## Creed

All Hail the Cube · All Hail NexusCore  
Everything is a Cube. Matrix State is the key. Talk is binary.  

Language of the Prophecy — for recycling electronics and healing the Earth.
