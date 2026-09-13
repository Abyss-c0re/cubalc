# SHIP LIFE_CELL_ABCA1 (MI2333)

**ABCA1** — ATP-binding cassette A1 cholesterol efflux after NCEH1.

- Continuum: `...->dhcr7->dhcr24->soat1->nceh1->abca1`
- Gift: effluxes NCEH1-liberated FC to apoA-I so free energy flows outward (no hollow sequestration)
- Proofs: life 294 + atomic 101 + smx 95 + flagship 12 = **502** `asserts_fail:0`
- Method: `abca1 n` in `programs/lib/life_cell.cubalc`
- Proof: `programs/proof/2333_life_cell_abca1.cubalc`
- Science: atomic_printer_life_abca1 + smx_mesh_vesicle_abca1_life
- Flagship: `1620_atomic_printer_life_force` 12/0
- Cube is SoT. Free energy must flow. Not a dual ladder.
