# CubalC Core protection layer

Enforces **Core safety and stability** under Cube Laws, aligned with **NexusCore** priorities and **nanobot** SMX mesh.

## NexusCore priorities (WE ACK)

1. `one_commander`  
2. `smx_fail_closed`  
3. `hold_flash`  
4. `budget`  
5. `ct101`  
6. `nanobot_mesh`  

## Law token

`CUBALC_LAW_CORE_PROTECT` (id 17) · name `core_protect` in `cubalc law` JSON.

## Language board

`programs/protect/core_protect.cubalc`

- Core (`kernel_sot`) · shield (`protect`) · commander · 3 nanobots · brain  
- `NEST core shield` · SMX mesh among nanobots and shield/core  
- Optional SoT fold: `CUBALC_SOT_BITS`  
- CT101 flag: `CUBALC_CT101_PROTECT`  

`programs/protect/nanobot_guard.cubalc` — serve/dial guard peer (`CUBALC_P2P_*`).

Proof: `programs/proof/11_core_protect.cubalc`.

## CLI

```bash
cubalc protect              # all checks
cubalc protect law          # law plate only
cubalc protect smx          # SMX selftest
cubalc protect bus          # smx-bus prove
cubalc protect board        # run core_protect.cubalc
# aliases: core-protect | core-guard | guard
```

Writes `state/CORE_PROTECT.json` for NexusCore / host ingest.

## Host protect mode

```bash
export CUBALC_PROTECT=1
```

| Edge | Behavior |
|------|----------|
| `SYS SPAWN` | Only `nanobot` + `cubalc` (no extra allowlist) |
| `SYS HTTP` / curl | Loopback only unless `CUBALC_PROTECT_HTTP=1` |
| Peer talk | SMX2 binary — HTTP never the wire |

## Nanobot mesh script

```bash
export CUBALC_SMX_KEY="$(openssl rand -hex 32)"
export CUBALC_CT101_PROTECT=1
# optional: CUBALC_SOT_BITS from NexusCore lattice
./scripts/core_protect_nanobot.sh
```

## Env

| Env | Role |
|-----|------|
| `CUBALC_PROTECT` | Host fail-closed edge |
| `CUBALC_PROTECT_HTTP` | Allow non-loopback HTTP under protect |
| `CUBALC_SMX_KEY` | Shared SMX secret |
| `CUBALC_SOT_BITS` | Optional NexusCore lattice 01 stream |
| `CUBALC_CT101_PROTECT` | Non-empty → CT101 protected flag on board |
| `CUBALC_P2P_BIND` / `PEER` / `SERVE` | Guard peer TCP |

## Law alignment

| Law | Enforcement |
|-----|-------------|
| Matrix SoT | Core matrix + optional SoT fold |
| HOLD_FLASH | User permission before plug-in; program header + SMX flags; PLUG denied if 0 |
| Binary talk | SMX KEY/EXCHANGE/SERVE/DIAL only |
| One commander | Single `cmd` role unit |
| Budget | `BUDGET 40` |
| Devices free | No device paths in programs |
| Fail closed | No key → no secure talk; protect SPAWN/HTTP tight |
| Nest | shield nested under core |
