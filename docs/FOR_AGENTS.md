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
Do NOT put HOLD_FLASH in ordinary programs — only new device init / mesh-join
(INCLUDE hold_seed when needed). Language PLUG never requires HOLD_FLASH.
Prefer COP over C++-style inheritance: CLASS/METHOD + CUBE OF/ENTITY, compose with PLUG, step with TICK+FLOW.
Reusable logic: FN name a b … END / CALL; objects: NEW / SEND / GETF / SETF.
```

## Evaluation loop

```
agent → .cubalc source → cubalc run → stdout JSON/board → agent
```

### Preload libs without editing source

```bash
cubalc run -I agent_boot -e 'STATUS'              # INCLUDE ONCE agent_boot first
# STATUS/VARS + every run plate: vars_n · vars_max (256) · vars_full — fat boards no silent special drops
# VARROOM → free slots LAST_N · HASVARROOM 20 soft 0|1 · NEEDVARROOM 20 fail-fast before nest specials
# cubalc doctor → vars_max + varroom_forms + lib_var_guard (install fat-board budget)
# INCLUDE var_guard · DEFAULT NEED_VARROOM = 48 · soft: DEFAULT VAR_GUARD_SOFT = 1
cubalc -I plate_session my.cubalc                 # top-level -I (no run subcmd)
export CUBALC_PRELOAD=agent_boot:hold_seed        # colon list env dual
cubalc run -L "$PWD/mylibs" -I my_extra prog.cubalc  # one-shot CUBALC_INCLUDE_PATH
CUBALC_INCLUDE_PATH=$PWD/mylibs cubalc which my_extra   # resolve project lib path
CUBALC_INCLUDE_PATH=$PWD/mylibs cubalc cat my_extra     # dump project lib source
CUBALC_INCLUDE_PATH=$PWD/mylibs cubalc libs · cubalc recipe fat_session  # one plate: path+deps+defaults+head
```

Run plate includes `preload_n` / `include_path_n`. See `cubalc env PRELOAD`.
Host version floor: `export CUBALC_REQUIRE_VERSION=1.15` or `cubalc run -R 1.15` (fail if runtime older).
Wall budget for runaway loops: `cubalc run -T 5000` or `export CUBALC_RUN_TIMEOUT=5000` (ms).
Plate: `timeout_ms` · `timed_out` · `remain_ms` (−1 unlimited) · `wall_ms` (elapsed) · in-lang `TIMEOUT_MS` / `TIMED_OUT` · SLEEP clamps to remaining.
STATUS also reports `timeout_ms` / `remain_ms`.
Mid-run probe: `WALL_MS`/`ELAPSED` (ms since start) · `REMAIN_MS` → ms left (−1 unlimited) · `HAS_TIME 200` soft · `NEEDTIME 200` fail-fast before heavy work.
Recipe: `DEFAULT NEED_TIME = 200` · `INCLUDE time_guard` (soft: `DEFAULT TIME_GUARD_SOFT = 1`) · `cubalc doctor` → `lib_time_guard`.
Fat nest boards: `DEFAULT NEED_VARROOM = 48` · `INCLUDE fat_boot` (agent_boot + var_guard) · `-I fat_boot` · doctor `lib_fat_boot`.
Durable fat nest: `INCLUDE fat_session` (fat_boot + plate_boot) · doctor `lib_fat_session`.
Scaffold: `cubalc init my.cubalc --fat-session` (or `--durable`) · `--fat` fat_boot · `cubalc init --list`.
After load: `LISTINCLUDES` → path bag · `INCLUDESTEMS` short names · `HASINCLUDE agent_boot` soft 0|1 · `INCLUDE_N`.
Run plate always reports `includes_n` + `includes` paths + `include_stems` / `include_stems_n` short names (INCLUDESTEMS dual).
Also `preload_n` + `preload` short-name array from `-I` / `CUBALC_PRELOAD` (request).
Plate dual of PRELOADOK/PRELOADMISS: `preload_ok` · `preload_miss_n` · `preload_miss[]` (request vs include_stems).
In-lang: `LISTPRELOAD` → same short names (via `CUBALC_PRELOAD_ACTIVE`) · `INCLUDESTEMS` loaded.
Audit -I vs loaded (no bag diff glue): `PRELOADMISS` miss bag · `PRELOADOK` soft 0|1 · `NEEDPRELOAD` fail-fast.
Request probe: `HASPRELOAD agent_boot` soft 0|1 (was `-I` asked?) · dual of `HASINCLUDE` (did it load?).
Gates: `NEEDINCLUDE agent_boot plate_session` fail-fast · `HASINCLUDEALL a b` soft + `INCLUDE_MISS` bag.

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
