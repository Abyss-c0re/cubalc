# HOLD_FLASH — device / mesh-join safeguard only

## Meaning

**HOLD_FLASH** is relevant **only** when you **initiate a new device** or
**plug a unit into the mesh** in a way that could touch firmware / physical
connect. It is **not** a fundamental requirement for ordinary CubalC programs.

| Is | Is not |
|----|--------|
| Sticky ack before device/firmware connect | Required first line of every `.cubalc` |
| Flag carried on SMX mesh frames (default 1) | Gate on language `PLUG a b` (virtual cubes) |
| Explicit seed for firmware-capable hosts | Boilerplate for SYS / plates / bags / host work |
| Never auto-flash | Curriculum / diploma law |

**Devices free:** CubalC never auto-flashes hardware. Hosts that *can* write
firmware still respect HOLD_FLASH as “commander said yes, hold (do not flash).”

## Runtime default

| Surface | Behavior |
|---------|----------|
| **Program start** | `hold_flash` **defaults to 1** — no `HOLD_FLASH` line needed |
| **Language `PLUG a b`** | Always virtual wire — **independent of HOLD_FLASH** |
| **SMX mesh frames** | Carry sticky hold bit (default 1) for device-safe mesh |
| `HOLD_FLASH 1` / `[hold]` | Explicit mesh/device ack (same as default) |
| `HOLD_FLASH 0` | Clear sticky hold (tests / lock-down of device-safe mesh ack) |
| Ordinary `SYS` / plates | Unaffected — never require HOLD_FLASH |

## Language

```cubalc
// Normal program — no HOLD_FLASH line, no device/mesh ceremony
CUBE a ROLE host PROTON 1
CUBE b ROLE body PROTON 1
PLUG a b              // language wire — always OK when ports match
FLOW 1

// Only when initiating a *new device* or mesh plug-in:
INCLUDE hold_seed     // optional sticky HOLD_FLASH 1 + BUDGET
// or: HOLD_FLASH 1
```

## When to set it

| Situation | Action |
|-----------|--------|
| Host SYS, bags, plates, logs, pure language | **Omit HOLD_FLASH** |
| Virtual cube `PLUG` / local SMX between cubes | **Omit** (default 1) |
| **New device** connect / firmware-capable host | Explicit `HOLD_FLASH 1` or `INCLUDE hold_seed` |
| **Mesh join** that must prove hold-ack | Explicit seed or rely on default 1 on frames |
| Test clearing sticky hold | `HOLD_FLASH 0` (does **not** break language PLUG) |

## Related

- `programs/lib/hold_seed.cubalc` — optional explicit seed for device hosts  
- `programs/proof/12_hold_flash_plug.cubalc` — language PLUG independent of HOLD_FLASH  
- `docs/CORE_PROTECT.md` — mesh/device protection plane  
