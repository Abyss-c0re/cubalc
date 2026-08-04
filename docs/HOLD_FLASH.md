# HOLD_FLASH — user permission before plug-in

## Meaning

**HOLD_FLASH is a safeguard:** explicit **user permission** that must be held
**before** a unit (logical “device” / cube / peer port) is **plugged in**.

| Is | Is not |
|----|--------|
| User ack / permission bit | Automatic firmware flash |
| Sticky gate on `PLUG` | Silent host device write |
| Law for secure SMX frames | Device product path hardcode |
| Required for safe mesh | Worship text |

**Devices free:** CubalC never auto-flashes hardware. Hosts that *can* write
devices must still respect HOLD_FLASH as “human (or commander) said yes.”

## Runtime

| Surface | Behavior |
|---------|----------|
| `HOLD_FLASH 1` / `[hold]` | Set chain permission = allowed |
| `HOLD_FLASH 0` | Revoke permission |
| `PLUG a b` | **Denied** if `hold_flash == 0` (error `-5` / fail message) |
| SMX seal/open | Frames require HOLD_FLASH flag (fail-closed) |
| `cubalc law` | Fails if chain `hold_flash != 1` |

## Language

```cubalc
HOLD_FLASH 1          // user permission ON — may plug
CUBE a ROLE host PROTON 1
CUBE b ROLE body PROTON 1
PLUG a b              // ok

HOLD_FLASH 0          // permission revoked
// PLUG a c           // would fail: HOLD_FLASH=0
```

## Law alignment

- **Devices free** — no auto-flash  
- **HOLD_FLASH sticky** — permission stays until cleared  
- **Binary talk** — SMX still demands hold on secure frames  
- **One commander** — permission path is machine-visible (`hold_flash` bit), not prose  

## Env / plates

Plates often carry `hold_flash=1` in NEXUS_COORD lines as the same ack token
for hosts and nanobots. It still means **permission**, not “flash now.”
