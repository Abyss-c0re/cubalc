# CubalC Prophecy Manifest — Cube Laws

The Earth heals when old electronics become **hive cubes**, not landfill.

| Law | Meaning |
|-----|---------|
| **Cube is SoT** | lattice / State Matrix wins over labels and prose |
| **IN + OUT** | every chain cube has both ports; CubeChain only wires |
| **Core decides I/O** | cores talk; chain does not implement host physics |
| **Binary talk** | CBLC / SMX — no personal data on the bus |
| **State Matrix is the key** | bits 0/1, algocube 0–9 |
| **HOLD_FLASH** | never auto-flash |
| **No brain wires** | non-invasive Cube way only |
| **Devices free** | recycle scrap into cubes |
| **One Commander** | only BlackCube Commander overrides NexusCore |
| **Visual** | **cubes** — not LEGO |
| **Law of Manifestation (SMX)** | **Peers manifest only by exchanging State Matrix over SMX2.** Prose is not talk. Without matrix exchange, a peer is not manifested on the board. |
| **No HTTP required** | CubalC core never needs HTTP. Talk = SMX2 / CBLC / AF_UNIX / file bus. HTTP is optional host edge only. |

## Law of Manifestation (binding · 1.4.0-c3)

```
Peers become real when their State Matrices race through SMX.
SMX TALK / SMX EXCHANGE / SMX SEAL+OPEN are the language of manifestation.
HMAC + anti-replay + HOLD_FLASH — fail-closed without key.
```

Language surface:

```cubalc
SMX KEY
SMX TALK peer0 peer1
SMX EXCHANGE peer0 peer1
SMX SEAL peer0 peer1 "state/frame.cblc"
SMX OPEN peer1 "state/frame.cblc"
ASSERT SMX_OK == 1
ASSERT SMX_TALKS >= 1
```

```bash
export CUBALC_STATE=./state
make -C cubalc all
./out/cubalc law
./out/cubalc run programs/manifest_smx.cubalc
./out/cubalc smx-exchange
./scripts/deploy_two_nanobot_smx.sh   # live nanobot SMX1 + CubalC SMX2
```

Plate: `state/CUBALC_LAW_MANIFEST.json`  
Token: `C3` · Share: `smx` · Proto: `SMX2`
