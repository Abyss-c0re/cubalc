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
SETP "freq.error" 0            # dotted nest path (no GETPOBJ glue)
NEEDP "n" "status" "freq.error"
PLUCKP "n" "status" "freq.error"   # multi-key peel → bag
INCLUDE plate_save             # or plate_patch with PLATE_PATCH
DUMPP                          # cubalc.plate_info.v1
```

Shell: `cubalc plate get|type|set|default|toggle|inc|show|ensure|merge|eq|diff|changelog|has|need|pluck|uniform path.json` · `cubalc libs` · `cubalc cat plate_session`.  
Keys may be dotted: `cubalc plate get agent.json freq.error` · `plate type … freq.error` → `num` · `plate default … cfg.port 8080` · `plate toggle … flags.debug`.  
`ensure`/`merge` seed+patch · `default` set-if-missing · `toggle` 0↔1 · `has`/`need` · `pluck` · `type` · `uniform` nest-eq (no `.cubalc`).  
INCLUDE `plate_uniform` after `DEFAULT UNIFORM_NEEDLE = "role"` for in-language nest consistency (`UNIFORM_EQ` / `UNIFORM_PATHS`).  
See `docs/COOKBOOK.md` §8 (single plate) · §9 (multi-plate PEER).

### Multi-plate PEER

```cubalc
INCLUDE plate_peer_session    # PLATE + PEER durable
SETP FROM PEER "host" "cubeB"
SETP FROM PEER "freq.error" 1
NEEDP FROM PEER "host" "freq.error"
EQP PLATE PEER                # soft mesh equality · or DIFFP / CHANGELOGP
REQUIRE EQP PLATE PEER        # hard gate when sync must match
SUBSETP need PEER             # soft required fields · REQUIRE SUBSETP hard
PLUCKP FROM PEER "host" "freq.error" "n"   # multi-key peel (paths ok)
DELTAP PLATE PEER             # sync payload plate · MERGEP FROM PLATE LAST
INCLUDE plate_both_save       # persist PLATE + PEER one INCLUDE
# tick: INCLUDE plate_peer_tick
```

Shell: `cubalc init --peer` · `cubalc cat plate_peer_session` · `cubalc cat plate_both_save`.

Self-test: `make test` (lang suite + smx).

## Visual channel (optional, human only)

`cube.viz_frame.v1` JSON is for UIs. Agents should use `?` metrics or `cubalc law` JSON, not pixels.
