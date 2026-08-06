# HOLD_FLASH — device/firmware connection safeguard

## Meaning

**HOLD_FLASH** is a safeguard for hosts that can **connect a device requiring
firmware changes**. It is **not** a required program preamble.

| Is | Is not |
|----|--------|
| Permission bit before device/firmware plug | Required first line of every `.cubalc` |
| Sticky gate when `hold_flash == 0` | Automatic firmware flash |
| Explicit ack for firmware-capable paths | Silent host device write |
| Optional override (`0` to deny PLUG) | Boilerplate for agents / host SYS work |

**Devices free:** CubalC never auto-flashes hardware. Hosts that *can* write
firmware must still respect HOLD_FLASH as “human (or commander) said yes.”

## Runtime default

| Surface | Behavior |
|---------|----------|
| **Program start** | `hold_flash` **defaults to 1** — no `HOLD_FLASH 1` line needed |
| `HOLD_FLASH 1` / `[hold]` | Explicit allow (same as default) |
| `HOLD_FLASH 0` | Revoke permission — `PLUG` denied |
| `PLUG a b` | **Denied only if** `hold_flash == 0` |
| Ordinary `SYS` / plates | Unaffected — never require HOLD_FLASH |

## Language

```cubalc
// Normal program — no HOLD_FLASH line
CUBE a ROLE host PROTON 1
CUBE b ROLE body PROTON 1
PLUG a b              // ok (default hold_flash=1)

HOLD_FLASH 0          // only when you mean lock-down
// PLUG a c           // fails: HOLD_FLASH=0

// Device/firmware-capable host: optional explicit seed
INCLUDE hold_seed     // sets HOLD_FLASH 1 + BUDGET (optional)
```

## When to set it

| Situation | Action |
|-----------|--------|
| Host SYS, bags, plates, logs | Omit HOLD_FLASH |
| Cube PLUG / SMX mesh | Omit (default 1) |
| Test “PLUG must fail” | `HOLD_FLASH 0` then PLUG |
| Firmware / physical device connect | Explicit `HOLD_FLASH 1` or `INCLUDE hold_seed` |
