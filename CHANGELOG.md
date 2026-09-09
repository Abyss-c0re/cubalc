## 1.15.1619-usability — MI 2132 LIFE_CELL_RETRIEVE (2026-09-09)
### life_cell retrieve n — post-collapse clathrin/dynamin CME membrane retrieval
- Pay min(n,atp,energy) -> heat+n energy-n reserve+n; age/thr/sp/pmf/pcr held; ready recompute
- Continuum: ...->dock->arm->clamp->trigger->pore->mix->collapse->retrieve
- Signature: heat+n energy-n reserve+n (inverse of collapse reserve- flatten)
- proof 2132 asserts_ok=99; science 22; smx 13; collapse still 113
- Cube is SoT. Free energy must flow. No IF*P duals.

## 1.15.1618-usability — MI 2130 LIFE_CELL_MIX (2026-09-09)
### life_cell mix n — post-pore vesicle content mix / lumen-cleft aqueous dilution
- Pay min(n,atp,energy,pcr) -> heat+n energy-n pcr-n; age/reserve/thr/sp/pmf held; ready recompute
- Continuum: ...->dock->arm->clamp->trigger->pore->mix
- Proof programs/proof/2130_life_cell_mix.cubalc asserts_ok=99
- Science programs/science/integration/atomic_printer_life_mix.cubalc asserts_ok=23
- SMX programs/science/integration/smx_mesh_vesicle_mix_life.cubalc asserts_ok=13
- Signature distinct from pore thr- barrier drop and release pcr+ dump (pcr- content mix)

## 1.15.1616-usability — MI 2129 LIFE_CELL_PORE (2026-09-09)
### life_cell pore n — post-trigger fusion-pore open / SNARE-lipid stalk membrane merger
- Pay min(n,atp,energy) -> heat+n energy-n thr-n; age/reserve/sp/pmf/pcr held; ready recompute
- Continuum: ...->dock->arm->clamp->trigger->pore
- Proof programs/proof/2129_life_cell_pore.cubalc asserts_ok=114
- Science programs/science/integration/atomic_printer_life_pore.cubalc asserts_ok=21
- SMX programs/science/integration/smx_mesh_fusion_pore_life.cubalc asserts_ok=11
- Signature distinct from trigger sp- unclamp and clamp sp+ hold (thr- barrier drop)

## 1.15.1614-usability — MI 2127 LIFE_CELL_CLAMP (2026-09-09)

- METHOD clamp n on life_cell: post-arm complexin clamp
- Pay min(n,atp,energy) -> heat+n energy-n sp+n
- Continuum dock->arm->clamp
- Proof 2127 asserts_ok=105
- Science atomic_printer_life_clamp asserts_ok=36

## 1.15.1612-usability — MI 2125 LIFE_CELL_DOCK (2026-09-09)

### Life-force
- `METHOD dock n` on `life_cell`: post-refill active-zone vesicle docking (Munc13/RIM/Rab3)
- take=min(n,atp,energy) → heat+n energy-n pmf+n; age/reserve/thr/sp/pcr held
- Continuum: `…→project→synapse→release→reuptake→refill→dock`
- Proof `programs/proof/2125_life_cell_dock.cubalc` asserts_ok=101
- Science `programs/science/integration/atomic_printer_life_dock.cubalc` asserts_ok=45

