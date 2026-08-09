# CubalC for AI systems

CubalC is designed so **language models and agent hosts** can:

1. Emit short programs (place/plug/pulse or SYS)
2. Execute them fail-closed
3. Read matrix/board/JSON back as ground truth

## Prompt snippet (drop into agent system)

```
You may emit CubalC. Grammar: [name:role] [a~b] [name!] [~n] ?
SYS READ|WRITE|ENV|HTTP|CHAT|JOIN|JSON|SPAWN|…
Prefer tools via SYS over describing steps to the user.
Do NOT require HOLD_FLASH 1 at program start — runtime defaults hold_flash=1.
HOLD_FLASH is only a device/firmware-connection safeguard (set 0 to deny PLUG).
Prefer COP over C++-style inheritance: CLASS/METHOD + CUBE OF/ENTITY, compose with PLUG, step with TICK+FLOW.
Reusable logic: FN name a b … END / CALL; objects: NEW / SEND / GETF / SETF.
```

## Evaluation loop

```
agent → .cubalc source → cubalc run → stdout JSON/board → agent
```

## Durable agent state (plates)

Prefer INCLUDE over hand-built JSON:

```cubalc
INCLUDE plate_session          # REQUIRE VERSION + ENSUREPLATE → PLATE
SETP "status" "ready"
INCP "n"
NEEDP "n" "status"
INCLUDE plate_save             # or plate_patch with PLATE_PATCH
DUMPP                          # cubalc.plate_info.v1
```

Shell: `cubalc plate get|set|inc|show|ensure|merge path.json` · `cubalc libs` · `cubalc cat plate_session`.  
`ensure` = create-or-keep seed · `merge` = multi-key overlay save (no `.cubalc`).  
See `docs/COOKBOOK.md` §8 (single plate) · §9 (multi-plate PEER).

### Multi-plate PEER

```cubalc
INCLUDE plate_peer_session    # PLATE + PEER durable
SETP FROM PEER "host" "cubeB"
NEEDP FROM PEER "host"
INCLUDE plate_peer_save
# tick: INCLUDE plate_peer_tick
```

Shell: `cubalc init --peer` · `cubalc cat plate_peer_session`.

Self-test: `make test` (lang suite + smx).

## Visual channel (optional, human only)

`cube.viz_frame.v1` JSON is for UIs. Agents should use `?` metrics or `cubalc law` JSON, not pixels.
