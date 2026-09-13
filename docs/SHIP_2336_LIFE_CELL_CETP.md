# LIFE_CELL_CETP (MI 2336)

**Gift.** CETP shuttles LCAT-formed cholesteryl esters from HDL cores onto apoB acceptors so reverse-cholesterol-transport free energy redistributes after CE-core packing — not a hollow CE endpoint, not a rename dual of LCAT.

## Continuum
`...->dhcr7->dhcr24->soat1->nceh1->abca1->abcg1->lcat->cetp`

## Run
```bash
export CUBALC_INCLUDE=programs/lib
./out/cubalc run programs/proof/2336_life_cell_cetp.cubalc
./out/cubalc run programs/science/integration/atomic_printer_life_cetp.cubalc
./out/cubalc run programs/science/integration/smx_mesh_vesicle_cetp_life.cubalc
./out/cubalc run programs/proof/1620_atomic_printer_life_force.cubalc
```

## Proofs
| board | asserts_ok |
|-------|------------|
| life 2336 | 364 |
| atomic | 110 |
| smx | 106 |
| flagship 1620 | 12 |
| **total** | **592** |
| fail | 0 |

Signature: heat+n atp-n energy-n; age/pcr/reserve/pmf/thr held.
