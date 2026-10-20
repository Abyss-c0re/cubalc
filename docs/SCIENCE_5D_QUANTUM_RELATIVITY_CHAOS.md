# CubalC pure science — 5D · Quantum · Relativity · Chaos

**Law:** Cube is SoT · pure science · modular C · free energy · zero bloat.

## Axes (5D)

| # | Name | Symbol | Role |
|---|------|--------|------|
| 1 | space | `x` | metres (lab) |
| 2 | space | `y` | metres (lab) |
| 3 | space | `z` | metres (lab) |
| 4 | time | `t` | seconds (coordinate) |
| 5 | dimension seed | `seed` | worldline / IC / measurement RNG key |

Seed is the **fifth dimension**: it parameterizes initial conditions, quantum measurement draws, and chaos ICs, then **folds back** from measure + attractor + boost into `seed_next`.

## Modules (split headers + sources — no monolith growth)

| Plane | Header | Source |
|-------|--------|--------|
| 5D | `include/cubalc_dim5.h` | `src/cubalc_dim5.c` |
| Quantum | `include/cubalc_quantum.h` | `src/cubalc_quantum.c` |
| Relativity | `include/cubalc_relativity.h` | `src/cubalc_relativity.c` |
| Chaos | `include/cubalc_chaos.h` | `src/cubalc_chaos.c` |
| Loop | `include/cubalc_science_loop.h` | `src/cubalc_science_loop.c` |

CLI: `cubalc science <sub>` (also `cubalc quantum|relativity|chaos|dim5`).

## Science basis

### Quantum (gate model)

- Statevector simulator ≤ 8 qubits (classical host).
- Unitary gates: H, X, Z, S, T, RX, RZ, CNOT.
- Measurement: Born rule; collapse; seed-bound RNG.
- **QPU path:** `cubalc science quantum --qasm` · `--gates h:0,cx:0:1,ry:1:0.5,cz:1:2,swap:0:2` emits **OpenQASM 2.0 (H/X/Y/Z/S/T/sdg/tdg/CX/CZ/SWAP/RX/RY/RZ/barrier)** (H/X/Z/S/T/sdg/tdg/CX + RZ/RX θ) for IBM/Rigetti/IonQ-class backends.
- gates_csv: `h:0,cnot:0:1,rz:0:1.57,rx:1:0.5` → portable circuit text.
- Norm preserved under gates (prove: `cubalc science prove`).

### Special relativity

- \(c = 299792458\,\mathrm{m/s}\) (`CUBALC_SCI_C_LIGHT`).
- Lorentz factor \(\gamma = 1/\sqrt{1-\beta^2}\), \(|\beta|<1\) enforced.
- Time dilation \(\Delta t = \gamma\Delta\tau\); length contraction \(L=L_0/\gamma\).
- Minkowski interval (mostly-plus): \(ds^2 = -c^2 dt^2 + dx^2 + dy^2 + dz^2\).
- Exact check: \(\beta=0.6 \Rightarrow \gamma=1.25\).

### Chaos (Lorenz 1963)

- \(\dot x=\sigma(y-x),\ \dot y=x(\rho-z)-y,\ \dot z=xy-\beta z\)
- \(\sigma=10,\ \rho=28,\ \beta=8/3\); RK4 integrator.
- Finite-time Lyapunov estimate (sensitive dependence).
- Path ring → JSON viz for attractor plot.

## Wire loops

```
seed → event5(x,y,z,t,seed)
     → quantum prep (H+CNOT+RZ(seed)) → measure → seed⊕
     → Lorentz boost on (t,x) → proper time tick
     → Lorenz IC(seed) → path → xyz update
     → fold(matrix_word) → viz frame → seed_next
```

As many loops as `cubalc science loop --loops N`.

## Commands

```bash
make -j$(nproc) all && make modular-check
./out/cubalc science prove
./out/cubalc science loop --seed 0x5C1E7CE --loops 4 --qubits 2 --beta 0.5
./out/cubalc science quantum --qubits 2 --qasm
./out/cubalc science relativity --beta 0.6
./out/cubalc science chaos --steps 500
./out/cubalc science dim5 --seed 42
./out/cubalc run programs/proof/2500_science_5d_quantum_relativity_chaos.cubalc
./out/cubalc run programs/science/physics/10_quantum_relativity_chaos_5d.cubalc
```

Plates: `state/science_*.json`, `state/science_viz_frame.json`.

## Anti-heresy

- New science lives in **these modules only** — do not grow `lang_ops_core.c` / `lang_ops_flow.c`.
- No enzyme / life-cell continuum ship as “science”.
- No root ship pins; prove with ASSERT + JSON plates.
