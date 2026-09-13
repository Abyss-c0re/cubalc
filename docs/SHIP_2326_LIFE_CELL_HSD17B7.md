# SHIP 2326 LIFE_CELL_HSD17B7

## Feature
HSD17B7 (ERG27) 3-keto-sterol reductase after NSDHL on life_cell continuum.

## Continuum
`...->fdps->sqs->sqle->lss->cyp51->tm7sf2->sc4mol->nsdhl->hsd17b7`

## Gift
Post-NSDHL HSD17B7 reduces the C3 ketone to 3β-OH under ASSERT so BA/odd-chain free energy builds membrane sterol mass.

## Proofs PASS
| proof | asserts_ok |
|-------|------------|
| life 2326_life_cell_hsd17b7 | 217 |
| atomic_printer_life_hsd17b7 | 89 |
| smx_mesh_vesicle_hsd17b7_life | 80 |
| flagship 1620_atomic_printer_life_force | 12 |
| **sum** | **398** |

asserts_fail: 0

## Method
`METHOD hsd17b7 n` in `programs/lib/life_cell.cubalc`

Cube is SoT. Free energy must flow.
