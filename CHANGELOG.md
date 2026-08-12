## 1.15.965-usability — 2026-08-12

### Feat
- `MISSP`/`PRESENTP` multi-plate dual of `SYS JSONMISS`/`JSONPRESENT`
- bag of missing/present required keys from `PLATE` or `FROM` named plate
- aliases: `MISSINGKEYSP`/`ABSENTP`/`PLATEMISSP` · `FOUNDKEYSP`/`FOUNDP`/`PLATEPRESENTP`
- LAST=bag · LAST_N=count · MISSP_N / PRESENTP_N · MISSP_FROM / PRESENTP_FROM
- null/false/0/empty still present (presence dual of COALESCEP substance)
- Proofs: `1505_missp.cubalc` + `1505_cli_missp.sh`

### Version
- Version: `1.15.965-usability`

## Meaningful — string content == + life engine harden — 2026-08-07

### Fix
- `==` / `!=` on string operands (literals, `LAST`, is_str vars) compare **content**, not strlen
- Numeric `==` unchanged for numbers
- Proof `870_oop_string_fields` green (SETF/NEW string formals + ASSERT string eq)
- life_engine: `SYS MKDIR` state dir before plate write

### Demo
- COP life engine (cell division / mitosis + growth) verified end-to-end
- Version: `1.15.322-usability`

## Meaningful — GETF OR/TRYGETF + OOP string formals — 2026-08-07

### Feat
- `GETF obj field OR fallback` — soft default like SYS ENV/LOOKUP (`GETF_OR`)
- `TRYGETF` / `GETF SOFT` / `GETFSOFT` — soft miss OK=0 sticky LAST_ERR
- Field name may be `"string"` or string-var (LISTFIELDS walk)
- SETF stores string-var / LAST by value (not strlen)
- METHOD formals bind string vars by value; NEW/SPAWN dynamic slot names
- Proofs: `871_oop_getf_or.cubalc`, `870_oop_string_fields.cubalc`
- Version: `1.15.321-usability`

## Meaningful — COP life engine cell division demo — 2026-08-07

### Demo
- `programs/apps/life_engine/` petri-dish game loop: Cell CLASS, TICK growth, mitosis into free slots, FLOW medium
- `programs/lib/life_cell.cubalc` INCLUDE-safe Cell type
- INCLUDE keeps source buffers so CLASS METHOD bodies remain valid
- Proof: `865_life_engine_division.cubalc`
- Version: `1.15.316-usability`

## Meaningful — OOP + COP engine plane (beyond C++) — 2026-08-07

### Language
- **OOP:** `CLASS` / `FIELD` / `METHOD` / `NEW` / `SEND` / `GETF` / `SETF` / `ISOF` / `CLASSNAME`
- **FN** named formals: `FN add a b` · `CALL add 1 2` (still ARG0…)
- **COP / game engine:** `CUBE OF Class` · `ENTITY` · `SPAWN` · `SCENE` · `TICK` (+ optional `FLOW n`)
- `TICK` reserved for world-step (METHOD tick/update/frame); board energy stays `FLOW`
- Docs: `docs/COP_ENGINE.md` — composition over inheritance, matrix SoT, flow law
- Proofs: `862_oop_class_method` · `863_cop_entity_tick`
- Version: `1.15.315-usability`

## Meaningful — HOLD_FLASH not a program preamble — 2026-08-06

### Usability
- HOLD_FLASH is a **device/firmware connection safeguard** only; runtime defaults `hold_flash=1`
- Programs / recipes / HELP / doctor / forms no longer teach “start with HOLD_FLASH 1”
- HOLD_FLASH is device/mesh-join only (not language PLUG); `INCLUDE hold_seed` optional for new-device hosts
- Emitters (evolve / translate) drop forced HOLD_FLASH preamble
- Docs: `docs/HOLD_FLASH.md` · `docs/FOR_AGENTS.md` · `AGENTS.md` · COOKBOOK / README
- Version: `1.15.232-usability`

## Meaningful — SYS PID / HOSTNAME host identity — 2026-08-05

### Usability
- `SYS PID` (aliases `GETPID`) — process id → LAST_N / PID
- `SYS HOSTNAME` (aliases `HOST` `NODENAME`) — machine name → LAST / HOSTNAME
- Peer naming and multi-process plates without shell `hostname`/`echo $$`
- Proof: `programs/proof/595_sys_pid_hostname.cubalc` · selftest id `sys_pid_host`
- Version: `1.15.46-usability`

## Meaningful — INCLUDE ONCE skip reload — 2026-08-05

### Usability
- `INCLUDE ONCE name` (aliases `UNIQUE` `SINGLE`) — resolve like INCLUDE; skip if path already loaded this run
- Sets `INCLUDE_SKIPPED` 1 on skip, 0 on first load; still sets INCLUDE_PATH / OK / INCLUDE_OK
- Combines with SOFT: `INCLUDE ONCE SOFT optional_lib`
- Proof: `programs/proof/594_include_once.cubalc` · selftest id `include_once`
- Version: `1.15.45-usability`

## Meaningful — EXIT early program halt — 2026-08-05

### Usability
- `EXIT [code] ["why"]` (aliases `HALT` `QUIT`) — stop further statements
- `EXIT` / `EXIT 0` → clean halt (plate ok if no assert fails); `EXIT n` → fail + process rc
- Plate fields: `exit_code`, `halted`; sticky LAST_ERR on non-zero
- Loops/blocks respect halt (with break/return flags)
- Proof: `programs/proof/593_exit.cubalc` · selftest id `exit`
- Version: `1.15.44-usability`

## Meaningful — NOTE agent breadcrumb — 2026-08-05

### Usability
- `NOTE ["text"]` (aliases `REMARK` `LOG_NOTE` `BREADCRUMB`) — step log for agents
- Sets `NOTE` + `LAST`; does **not** rewrite OK / EXPECT_OK / sticky LAST_ERR
- Trace line `# note: …`; complements PASS/FAIL soft status and PRINT
- Proof: `programs/proof/592_note.cubalc` · selftest id `note`
- Version: `1.15.43-usability`

## Meaningful — SYS MS wall milliseconds — 2026-08-05

### Usability
- `SYS MS` (aliases `MILLIS` `TIME_MS` `EPOCH_MS` `NOW_MS`) — wall epoch ms → LAST_N / MS / TIME_MS
- Complements `SYS TIME` (seconds); agents stamp plates / measure without shell `date`
- Forms/HELP catalog lists SYS TIME + SYS MS
- Proof: `programs/proof/591_sys_ms.cubalc` · selftest id `sys_ms`
- Version: `1.15.42-usability`

## Meaningful — REQUIRE LIB fail-fast module gate — 2026-08-05

### Usability
- `REQUIRE LIB name` (aliases `MODULE` `INCLUDE` `FILE` `STDLIB`) — resolve like INCLUDE/`cubalc which`
- Missing → fatal with line + hint (`programs/lib · cubalc libs`); hit → LAST=path, OK=1, REQUIRE_LIB
- Complements `INCLUDE OR|SOFT` (optional) and `REQUIRE VERSION` (runtime floor)
- Proof: `programs/proof/590_require_lib.cubalc` · selftest id `require_lib`
- Version: `1.15.41-usability`

## Meaningful — cubalc run --strict soft last_err fails — 2026-08-05

### Usability
- `cubalc run -s|--strict` or `CUBALC_STRICT=1` — sticky soft `last_err` (FAIL/EXPECT) fails process exit and plate `ok`
- Default remains soft-continue (plate may be ok:true with last_err for probes)
- Plate field `strict`: true|false; combines with `-q`
- Env catalog + search hint for `CUBALC_STRICT`
- Version: `1.15.40-usability`

## Meaningful — cubalc init scaffold starter — 2026-08-05

### Usability
- `cubalc init|new|scaffold [path] [--force]` — write runnable `.cubalc` starter
- Template: `INCLUDE agent_boot` + CUBE/PLUG/FLOW/DECIDE/STATUS
- JSON plate `cubalc.init.v1` (path, wrote, next run hint); refuse overwrite unless `--force`
- Bare names get `.cubalc` suffix; parent dirs created
- Version: `1.15.39-usability`

## Meaningful — CLEAR_ERR wipe sticky soft errors — 2026-08-05

### Usability
- `CLEAR_ERR` [note] (aliases `CLEARERR` `WIPE_ERR` `RESET_ERR` `ERR_CLEAR`)
- Wipes sticky `ERR` / `LAST_ERR` / `vm->err` so run plate `last_err` is clean after recovery
- Does not rewrite `OK` (pair with `PASS` then `CLEAR_ERR`)
- Optional note → LAST; bare → LAST=`cleared`
- Proof: `programs/proof/589_clear_err.cubalc` · selftest id `clear_err`
- Version: `1.15.38-usability`

## Meaningful — STATUS agent health plate — 2026-08-05

### Usability
- `STATUS` (aliases `HEALTH` `AGENT_STATUS`) — one JSON line `cubalc.status.v1`
- Surfaces ok / last_err / version / time / hold / n / unity / smx without guessing names
- Report-only: does not rewrite OK or sticky LAST_ERR
- Sets `STATUS_OK`, `TIME`, short `LAST` (`status:ok`|`status:err`); full plate on trace/`last_print`
- Proof: `programs/proof/588_status.cubalc` · selftest id `status`
- Version: `1.15.37-usability`

## 1.15.16-usability — 2026-08-05

### Usability (agents + humans)
- `cubalc doctor|health` — install readiness JSON (`cubalc.doctor.v1`)
- `cubalc cookbook|start` — starter paths; grouped `help`
- `SYS ENV "NAME" OR "fallback"` — unset/empty → default string (`DEFAULT`/`ELSE`/`FALLBACK` aliases)
- `SMX SERVE` respects `CUBALC_P2P_TIMEOUT` ms (default 30000); timeout → soft `SMX_OK=0` (no hang)
- `CUBALC_P2P_SOFT=1` — `SMX DIAL` soft-fail on connect/wire errors
- `INCLUDE` resolves `programs/<rel>` from any program dir
- `programs/lib/hold_seed.cubalc`, `programs/lib/peer_decide.cubalc`
- `docs/COOKBOOK.md` recipes
- Proofs: `573_env_or_assert_msg`, `574_include_lib`, `575_p2p_soft_dial`, `576_p2p_serve_timeout`

### Prior
ASSERT message strings (1c2693e); 1.15.15-universal ISA.

## Meaningful — cubalc libs catalog — 2026-08-05

### Usability
- `cubalc libs` (aliases: `lib`, `stdlib`) — list `programs/lib/*.cubalc` with hints
- JSON plate `cubalc.libs.v1` for agents; TSV human lines
- Complements cookbook / forms / doctor

## Meaningful — sticky ERR / LAST_ERR — 2026-08-05

### Usability
- Fatal `fail()` and SMX soft-fail set string vars `ERR` and `LAST_ERR`
- Sticky: later `SYS`/`PRINT` LAST overwrites do not clear ERR
- Agents: `PRINT_JSON ERR` or `SYS LEN ERR` after soft dial
- Proof: `programs/proof/err_sticky.cubalc` (needs `CUBALC_P2P_SOFT=1`)

## Meaningful — HELP form in-language — 2026-08-05

### Usability
- `HELP` / `HELP form` — discover play forms inside a program (agents)
- Sets LAST (hint text), LAST_N, OK (found?), HELP_N (hit count)
- Complements CLI `cubalc forms [prefix]`
- Proof: `programs/proof/help_form.cubalc`

## Meaningful — SYS ARG OR fallback — 2026-08-05

### Usability
- `SYS ARG n|name OR "fallback"` — same default pattern as `SYS ENV … OR`
- Sets LAST / LAST_N / OK; prefers CUBALC_ARGn when present
- Proof: `programs/proof/sys_arg_or.cubalc`

## Meaningful — SMX fail hints — 2026-08-05

### Usability
- SMX errors include source line + `hint:` (SERVE/DIAL/TALK/EXCHANGE/SEAL/OPEN)
- Soft-fail LAST/trace text also carries recovery hints (timeout, connect, keys)
- Proof: `programs/proof/smx_fail_hint.cubalc` (must fail with SERVE bind hint)

## Meaningful — lib mesh_exchange — 2026-08-05

### Usability
- `programs/lib/mesh_exchange.cubalc` — documented SMX KEY + dual EXCHANGE rounds for `peer0`/`peer1`
- Proof: `programs/proof/include_mesh_exchange.cubalc` (INCLUDE hold_seed + mesh_exchange)

## Meaningful — cubalc forms catalog — 2026-08-05

### Usability
- `cubalc forms [prefix]` (aliases: `ops`) — human-meaningful form catalog + JSON plate
- Filter by name/plane substring (case-insensitive); `--json` for plate-only
- Catalog covers law/core/matrix/flow/host/smx/async/play — not dual/width ISA soup

## Meaningful — protect status CLI — 2026-08-05

### Usability
- `cubalc protect status` (aliases: `plate`, `show`) — one JSON plate without board/smx run
- Reports hold_flash, budget, plate path, last_plate_ok, program presence for agents
- Full checks still: `cubalc protect all`

## Meaningful — PRINT_JSON / DUMP — 2026-08-05

### Usability
- `PRINT_JSON` / `DUMP` [idents] — one stable JSON line for agents
- Bare form: runtime snapshot (`schema`, CUBES, LAST_N, OK, SP, UNITY, DECIDE, SMX_*, hold_flash)
- Named form: `{"a":42,"tag":"…"}` with string-var escaping
- Proof: `programs/proof/print_json.cubalc`

## Meaningful — ASSERT message strings — 2026-08-05

### Usability
- `ASSERT expr ["why"]` — optional quoted reason on failure for agents/humans
- Fail text includes source line: `ASSERT failed line N: why` (or bare line when no message)
- Proofs: `programs/proof/assert_msg.cubalc`, `programs/proof/assert_msg_fail.cubalc`

## 1.15.15-universal — 2026-08-05

### Direction
- Algocube digit **1** → pure-imm cell parity TOC (imm cell-index dual of SODDTOC/SEVENTOC after complete zero-rel TOCN plane).

### Language
- `SODDTOCN` — `n → cells[n]=(cells[n]&1)?1:0; push`
- `SEVENTOCN` — `n → cells[n]=((cells[n]&1)==0)?1:0; push`
- Proof `572_soddtocn_seventocn.cubalc`

### Prior
See 1.15.14-universal.

## 1.15.14-universal — 2026-08-05

### Direction
- Algocube digit **3** → pure-imm cell zero-eq/ne TOC (imm cell-index dual of SEQZTOC/SNEZTOC; completes zero-rel TOCN ladder).

### Language
- `SEQZTOCN` / `S0EQTOCN` — `n → cells[n]=(cells[n]==0)?1:0; push`
- `SNEZTOCN` / `S0NETOCN` — `n → cells[n]=(cells[n]!=0)?1:0; push`
- Proof `571_seqztocn_sneztocn.cubalc`

### Prior
See 1.15.13-universal.

## 1.15.13-universal — 2026-08-05

### Direction
- Algocube digit **1** → pure-imm cell zero-rel LT0/GT0 TOC (imm cell-index dual of SLTZTOC/SGTZTOC; complete signed zero-rel TOCN after SLEZTOCN/SGEZTOCN).

### Language
- `SLTZTOCN` / `S0LTTOCN` — `n → cells[n]=(cells[n]<0)?1:0; push`
- `SGTZTOCN` / `S0GTTOCN` — `n → cells[n]=(cells[n]>0)?1:0; push`
- Proof `570_sltztocn_sgtztocn.cubalc`

### Prior
See 1.15.12-universal.

## 1.15.12-universal — 2026-08-05

### Direction
- Algocube digit **2** → pure-imm cell zero-rel LE0/GE0 TOC (imm cell-index dual of SLEZTOC/SGEZTOC after complete field S0LE/S0GE plane).

### Language
- `SLEZTOCN` / `S0LETOCN` — `n → cells[n]=(cells[n]<=0)?1:0; push`
- `SGEZTOCN` / `S0GEZTOCN` — `n → cells[n]=(cells[n]>=0)?1:0; push`
- Proof `569_sleztocn_sgeztocn.cubalc`

### Prior
See 1.15.11-universal.

## 1.15.11-universal — 2026-08-05

### Direction
- Algocube digit **3** → stack+dual imm 8/4-bit field signed GE0 (completes field signed GE0 plane all widths).

### Language
- `S0GE8N` — `n → byte n = (int8(b) >= 0) ? 1 : 0`
- `S0GE4N` — nibble form of `S0GE8N`
- `D0GE8N` / `D0GE4N` — dual-stack forms
- Proof `568_s0ge8n_d0ge4n.cubalc`

### Prior
See 1.15.10-universal.

## 1.15.10-universal — 2026-08-05

### Direction
- Algocube digit **4** → stack+dual imm 8/4-bit field signed LE0 (completes field signed LE0 plane all widths).

### Language
- `S0LE8N` — `n → byte n = (int8(b) <= 0) ? 1 : 0`
- `S0LE4N` — nibble form of `S0LE8N`
- `D0LE8N` / `D0LE4N` — dual-stack forms
- Proof `567_s0le8n_d0le4n.cubalc`

### Prior
See 1.15.9-universal.

## 1.15.9-universal — 2026-08-05

### Direction
- Algocube digit **1** → stack+dual imm 32/16-bit field signed GE0 (field dual of S0GE after S0LE plane open).

### Language
- `S0GE32N` — `n → word n = (int32(w) >= 0) ? 1 : 0`
- `S0GE16N` — halfword form of `S0GE32N`
- `D0GE32N` / `D0GE16N` — dual-stack forms
- Proof `566_s0ge32n_d0ge16n.cubalc`

### Prior
See 1.15.8-universal.

## 1.15.8-universal — 2026-08-05

### Direction
- Algocube digit **4** → stack+dual imm 32/16-bit field signed LE0 (field dual of S0LE after complete S0GT plane).

### Language
- `S0LE32N` — `n → word n = (int32(w) <= 0) ? 1 : 0`
- `S0LE16N` — halfword form of `S0LE32N`
- `D0LE32N` / `D0LE16N` — dual-stack forms
- Proof `565_s0le32n_d0le16n.cubalc`

### Prior
See 1.15.7-universal.

## 1.15.7-universal — 2026-08-05

### Direction
- Algocube digit **0** → stack+dual imm 8/4-bit field signed GT0 (completes field signed GT0 plane all widths).

### Language
- `S0GT8N` — `n → byte n = (int8(b) > 0) ? 1 : 0`
- `S0GT4N` — nibble form of `S0GT8N`
- `D0GT8N` / `D0GT4N` — dual-stack forms
- Proof `564_s0gt8n_d0gt4n.cubalc`

### Prior
See 1.15.6-universal.

## 1.15.6-universal — 2026-08-05

### Direction
- Algocube digit **9** → stack+dual imm 32/16-bit field signed GT0 (field dual of S0GT after complete S0LT plane).

### Language
- `S0GT32N` — `n → word n = (int32(w) > 0) ? 1 : 0`
- `S0GT16N` — halfword form of `S0GT32N`
- `D0GT32N` / `D0GT16N` — dual-stack forms
- Proof `563_s0gt32n_d0gt16n.cubalc`

### Prior
See 1.15.5-universal.

## 1.15.5-universal — 2026-08-05

### Direction
- Algocube digit **7** → stack+dual imm 8/4-bit field signed LT0 (completes field signed LT0 plane all widths).

### Language
- `S0LT8N` — `n → byte n = (int8(b) < 0) ? 1 : 0`
- `S0LT4N` — nibble form of `S0LT8N`
- `D0LT8N` / `D0LT4N` — dual-stack forms
- Proof `562_s0lt8n_d0lt4n.cubalc`

### Prior
See 1.15.4-universal.

## 1.15.4-universal — 2026-08-05

### Direction
- Algocube digit **8** → stack+dual imm 32/16-bit field signed LT0 (field dual of S0LT after S0EQ/S0NE planes).

### Language
- `S0LT32N` — `n → word n = (int32(w) < 0) ? 1 : 0`
- `S0LT16N` — halfword form of `S0LT32N`
- `D0LT32N` / `D0LT16N` — dual-stack forms
- Proof `561_s0lt32n_d0lt16n.cubalc`

### Prior
See 1.15.3-universal.

## 1.15.3-universal — 2026-08-05

### Direction
- Algocube digit **3** → stack+dual imm 8/4-bit field nonzero (completes field nonzero plane all widths).

### Language
- `S0NE8N` — `n → byte n = (b != 0) ? 1 : 0`
- `S0NE4N` — nibble form of `S0NE8N`
- `D0NE8N` / `D0NE4N` — dual-stack forms
- Proof `560_s0ne8n_d0ne4n.cubalc`

### Prior
See 1.15.2-universal.

## 1.15.2-universal — 2026-08-05

### Direction
- Algocube digit **1** → stack+dual imm 32/16-bit field nonzero (field dual of S0NE/SNZ after S0EQ plane).

### Language
- `S0NE32N` — `n → word n = (w != 0) ? 1 : 0`
- `S0NE16N` — halfword form of `S0NE32N`
- `D0NE32N` / `D0NE16N` — dual-stack forms
- Proof `559_s0ne32n_d0ne16n.cubalc`

### Prior
See 1.15.1-universal.

## 1.15.1-universal — 2026-08-05

### Direction
- Algocube digit **7** → stack+dual imm 8/4-bit field zero-eq (completes field zero-eq plane all widths).

### Language
- `S0EQ8N` — `n → byte n = (b == 0) ? 1 : 0`
- `S0EQ4N` — nibble form of `S0EQ8N`
- `D0EQ8N` / `D0EQ4N` — dual-stack forms
- Proof `558_s0eq8n_d0eq4n.cubalc`

### Prior
See 1.15.0-universal.

## 1.15.0-universal — 2026-08-04

### Direction
- Algocube digit **4** → stack+dual imm 32/16-bit field zero-eq (field dual of S0EQ/SZ after signum plane; opens field zero-pred plane).

### Language
- `S0EQ32N` — `n → word n = (w == 0) ? 1 : 0`
- `S0EQ16N` — halfword form of `S0EQ32N`
- `D0EQ32N` / `D0EQ16N` — dual-stack forms
- Proof `557_s0eq32n_d0eq16n.cubalc`

### Prior
See 1.14.99-universal.

## 1.14.99-universal — 2026-08-04

### Direction
- Algocube digit **7** → stack+dual imm 8/4-bit field signed signum (completes field signum plane all widths).

### Language
- `SSIGN8N` — `n → byte n = sgn(int8): +1 / 0 / 0xFF(-1)`
- `SSIGN4N` — nibble form of `SSIGN8N`
- `DSIGN8N` / `DSIGN4N` — dual-stack forms
- Proof `556_ssign8n_dsign4n.cubalc`

### Prior
See 1.14.98-universal.

## 1.14.98-universal — 2026-08-04

### Direction
- Algocube digit **0** → stack+dual imm 32/16-bit field signed signum (field dual of SSIGN/DSIGN after signed WITHIN plane).

### Language
- `SSIGN32N` — `n → word n = sgn(int32): +1 / 0 / 0xFFFFFFFF(-1)`
- `SSIGN16N` — halfword form of `SSIGN32N`
- `DSIGN32N` — dual-stack form of `SSIGN32N`
- `DSIGN16N` — dual-stack form of `SSIGN16N`
- Proof `555_ssign32n_dsign16n.cubalc`

### Prior
See 1.14.97-universal.

## 1.14.97-universal — 2026-08-04

### Direction
- Algocube digit **3** → stack+dual imm 8/4-bit field signed WITHIN (completes hi-exclusive interval plane all widths).

### Language
- `SWITHINS8N` — `lo hi n → byte n = (lo <= int8(b) < hi) ? 1 : 0`
- `SWITHINS4N` — nibble form
- `DWITHINS8N` / `DWITHINS4N` — dual-stack forms
- Proof `554_swithins8n_dwithins4n.cubalc`

### Prior
See 1.14.96-universal.

## 1.14.96-universal — 2026-08-04

### Direction
- Algocube digit **7** → stack+dual imm 32/16-bit field signed WITHIN (hi exclusive; Forth WITHIN after SBETWEENS plane).

### Language
- `SWITHINS32N` — `lo hi n → word n = (lo <= int32(w) < hi) ? 1 : 0` (no lo/hi swap)
- `SWITHINS16N` — halfword form of `SWITHINS32N`
- `DWITHINS32N` — dual-stack form of `SWITHINS32N`
- `DWITHINS16N` — dual-stack form of `SWITHINS16N`
- Proof `553_swithins32n_dwithins16n.cubalc`

### Prior
See 1.14.95-universal.

## 1.14.95-universal — 2026-08-04

### Direction
- Algocube digit **1** → stack+dual imm 4-bit field signed clamp+between (nibble ladder of SCLAMPS8N; **completes signed range plane all widths**).

### Language
- `SCLAMPS4N` — `lo hi n → nibble n = clamp_signed(int4(nib), [lo,hi])`
- `SBETWEENS4N` — `lo hi n → nibble n = (lo <= int4(nib) <= hi) ? 1 : 0`
- `DCLAMPS4N` — dual-stack form of `SCLAMPS4N`
- `DBETWEENS4N` — dual-stack form of `SBETWEENS4N`
- Proof `552_sclamps4n_dbetweens4n.cubalc`

### Prior
See 1.14.94-universal.

## 1.14.94-universal — 2026-08-04

### Direction
- Algocube digit **5** → stack+dual imm 8-bit field signed clamp+between (byte ladder of SCLAMPS16N/DCLAMPS16N after SMINS8N plane).

### Language
- `SCLAMPS8N` — `lo hi n → byte n = clamp_signed(int8(b), [lo,hi])`
- `SBETWEENS8N` — `lo hi n → byte n = (lo <= int8(b) <= hi) ? 1 : 0`
- `DCLAMPS8N` — dual-stack form of `SCLAMPS8N`
- `DBETWEENS8N` — dual-stack form of `SBETWEENS8N`
- Proof `551_sclamps8n_dbetweens8n.cubalc`

### Prior
See 1.14.93-universal.

## 1.14.93-universal — 2026-08-04

### Direction
- Algocube digit **7** → stack+dual imm 16-bit field signed clamp+between (halfword ladder of SCLAMPS32N/DCLAMPS32N after SMINS16N plane).

### Language
- `SCLAMPS16N` — `lo hi n → halfword n = clamp_signed(int16(hw), [lo,hi])`
- `SBETWEENS16N` — `lo hi n → halfword n = (lo <= int16(hw) <= hi) ? 1 : 0`
- `DCLAMPS16N` — dual-stack form of `SCLAMPS16N`
- `DBETWEENS16N` — dual-stack form of `SBETWEENS16N`
- Proof `550_sclamps16n_dbetweens16n.cubalc`

### Prior
See 1.14.92-universal.

## 1.14.92-universal — 2026-08-04

### Direction
- Algocube digit **9** → stack+dual imm 32-bit field signed clamp+between (signed range plane after SMINS32N/DMINS32N; field dual of SCLAMPN/SBETWEENN).

### Language
- `SCLAMPS32N` — `lo hi n → word n = clamp_signed(int32(w), [lo,hi])`
- `SBETWEENS32N` — `lo hi n → word n = (lo <= int32(w) <= hi) ? 1 : 0`
- `DCLAMPS32N` — dual-stack form of `SCLAMPS32N`
- `DBETWEENS32N` — dual-stack form of `SBETWEENS32N`
- Proof `549_sclamps32n_dbetweens32n.cubalc`

### Prior
See 1.14.91-universal.

## 1.14.91-universal — 2026-08-04

### Direction
- Algocube digit **4** → stack+dual imm 4-bit field signed min/max (nibble ladder of SMINS8N/DMINS8N after SLTS4N; **completes signed select plane all widths**).

### Language
- `SMINS4N` — `field n → nibble n = min_signed(int4(nib), int4(field))`
- `SMAXS4N` — `field n → nibble n = max_signed(int4(nib), int4(field))`
- `DMINS4N` — `field n → nibble n of each = min_signed(int4(nib), int4(field))`
- `DMAXS4N` — `field n → nibble n of each = max_signed(int4(nib), int4(field))`
- Proof `548_smins4n_dmaxs4n.cubalc`

### Prior
See 1.14.90-universal.

## 1.14.90-universal — 2026-08-04

### Direction
- Algocube digit **2** → stack+dual imm 8-bit field signed min/max (byte ladder of SMINS16N/DMINS16N after SLTS8N plane).

### Language
- `SMINS8N` — `field n → byte n = min_signed(int8(b), int8(field))`
- `SMAXS8N` — `field n → byte n = max_signed(int8(b), int8(field))`
- `DMINS8N` — `field n → byte n of each = min_signed(int8(b), int8(field))`
- `DMAXS8N` — `field n → byte n of each = max_signed(int8(b), int8(field))`
- Proof `547_smins8n_dmaxs8n.cubalc`

### Prior
See 1.14.89-universal.

## 1.14.89-universal — 2026-08-04

### Direction
- Algocube digit **8** → stack+dual imm 16-bit field signed min/max (halfword ladder of SMINS32N/DMINS32N after SLTS16N plane).

### Language
- `SMINS16N` — `field n → halfword n = min_signed(int16(hw), int16(field))`
- `SMAXS16N` — `field n → halfword n = max_signed(int16(hw), int16(field))`
- `DMINS16N` — `field n → halfword n of each = min_signed(int16(hw), int16(field))`
- `DMAXS16N` — `field n → halfword n of each = max_signed(int16(hw), int16(field))`
- Proof `546_smins16n_dmaxs16n.cubalc`

### Prior
See 1.14.88-universal.

## 1.14.88-universal — 2026-08-04

### Direction
- Algocube digit **5** → stack+dual imm 32-bit field signed min/max (signed select after SLTS32N/DLTS32N plane; dual of unsigned SMIN32N/SMAX32N).

### Language
- `SMINS32N` — `field n → word n = min_signed(int32(w), int32(field))`
- `SMAXS32N` — `field n → word n = max_signed(int32(w), int32(field))`
- `DMINS32N` — `field n → word n of each = min_signed(int32(w), int32(field))`
- `DMAXS32N` — `field n → word n of each = max_signed(int32(w), int32(field))`
- Proof `545_smins32n_dmaxs32n.cubalc`

### Prior
See 1.14.87-universal.

## 1.14.87-universal — 2026-08-04

### Direction
- Algocube digit **7** → dual-stack imm 32-bit field signed ordered-cmp (dual of SLTS32N plane after DGTE32N; complete signed ordered plane all widths).

### Language
- `DLTS32N` — `field n → word n of each = (int32(w) < int32(field)) ? 1 : 0`
- `DGTS32N` — `field n → word n of each = (int32(w) > int32(field)) ? 1 : 0`
- `DLTES32N` — `field n → word n of each = (int32(w) <= int32(field)) ? 1 : 0`
- `DGTES32N` — `field n → word n of each = (int32(w) >= int32(field)) ? 1 : 0`
- Proof `544_dlts32n_dgtes32n.cubalc`

### Prior
See 1.14.86-universal.

## 1.14.86-universal — 2026-08-04

### Direction
- Algocube digit **6** → stack imm 32-bit field signed ordered-cmp (word ladder of SLTS16N after unsigned SGTE32N).

### Language
- `SLTS32N` — `field n → word n = (int32(w) < int32(field)) ? 1 : 0`
- `SGTS32N` — `field n → word n = (int32(w) > int32(field)) ? 1 : 0`
- `SLTES32N` — `field n → word n = (int32(w) <= int32(field)) ? 1 : 0`
- `SGTES32N` — `field n → word n = (int32(w) >= int32(field)) ? 1 : 0`
- Proof `543_slts32n_sgtes32n.cubalc`

### Prior
See 1.14.85-universal.

## 1.14.85-universal — 2026-08-04

### Direction
- Algocube digit **8** → dual-stack imm 16-bit field signed ordered-cmp (dual of SLTS16N plane after DGTE16N).

### Language
- `DLTS16N` — `field n → halfword n of each = (int16(hw) < int16(field)) ? 1 : 0`
- `DGTS16N` — `field n → halfword n of each = (int16(hw) > int16(field)) ? 1 : 0`
- `DLTES16N` — `field n → halfword n of each = (int16(hw) <= int16(field)) ? 1 : 0`
- `DGTES16N` — `field n → halfword n of each = (int16(hw) >= int16(field)) ? 1 : 0`
- Proof `542_dlts16n_dgtes16n.cubalc`

### Prior
See 1.14.84-universal.

## 1.14.84-universal — 2026-08-04

### Direction
- Algocube digit **9** → stack imm 16-bit field signed ordered-cmp (halfword ladder of SLTS8N after unsigned SGTE16N).

### Language
- `SLTS16N` — `field n → halfword n = (int16(hw) < int16(field)) ? 1 : 0`
- `SGTS16N` — `field n → halfword n = (int16(hw) > int16(field)) ? 1 : 0`
- `SLTES16N` — `field n → halfword n = (int16(hw) <= int16(field)) ? 1 : 0`
- `SGTES16N` — `field n → halfword n = (int16(hw) >= int16(field)) ? 1 : 0`
- Proof `541_slts16n_sgtes16n.cubalc`

### Prior
See 1.14.83-universal.

## 1.14.83-universal — 2026-08-04

### Direction
- Algocube digit **3** → dual-stack imm 8-bit field signed ordered-cmp (dual of SLTS8N plane after DGTE8N).

### Language
- `DLTS8N` — `field n → byte n of each = (int8(b) < int8(field)) ? 1 : 0`
- `DGTS8N` — `field n → byte n of each = (int8(b) > int8(field)) ? 1 : 0`
- `DLTES8N` — `field n → byte n of each = (int8(b) <= int8(field)) ? 1 : 0`
- `DGTES8N` — `field n → byte n of each = (int8(b) >= int8(field)) ? 1 : 0`
- Proof `540_dlts8n_dgtes8n.cubalc`

### Prior
See 1.14.82-universal.

## 1.14.82-universal — 2026-08-04

### Direction
- Algocube digit **4** → stack imm 8-bit field signed ordered-cmp (byte ladder of SLTS4N after unsigned SGTE8N).

### Language
- `SLTS8N` — `field n → byte n = (int8(b) < int8(field)) ? 1 : 0`
- `SGTS8N` — `field n → byte n = (int8(b) > int8(field)) ? 1 : 0`
- `SLTES8N` — `field n → byte n = (int8(b) <= int8(field)) ? 1 : 0`
- `SGTES8N` — `field n → byte n = (int8(b) >= int8(field)) ? 1 : 0`
- Proof `539_slts8n_sgtes8n.cubalc`

### Prior
See 1.14.81-universal.

## 1.14.81-universal — 2026-08-04

### Direction
- Algocube digit **1** → dual-stack imm 4-bit field signed ordered-cmp (dual of SLTS4N plane after DGTE4N).

### Language
- `DLTS4N` — `field n → nibble n of each = (int4(nib) < int4(field)) ? 1 : 0`
- `DGTS4N` — `field n → nibble n of each = (int4(nib) > int4(field)) ? 1 : 0`
- `DLTES4N` — `field n → nibble n of each = (int4(nib) <= int4(field)) ? 1 : 0`
- `DGTES4N` — `field n → nibble n of each = (int4(nib) >= int4(field)) ? 1 : 0`
- Proof `538_dlts4n_dgtes4n.cubalc`

### Prior
See 1.14.80-universal.

## 1.14.80-universal — 2026-08-04

### Direction
- Algocube digit **5** → stack imm 4-bit field signed ordered-cmp (int4 plane after unsigned SLT/SGT/SLTE/SGTE).

### Language
- `SLTS4N` — `field n → nibble n = (int4(nib) < int4(field)) ? 1 : 0`
- `SGTS4N` — `field n → nibble n = (int4(nib) > int4(field)) ? 1 : 0`
- `SLTES4N` — `field n → nibble n = (int4(nib) <= int4(field)) ? 1 : 0`
- `SGTES4N` — `field n → nibble n = (int4(nib) >= int4(field)) ? 1 : 0`
- Proof `537_slts4n_sgtes4n.cubalc`

### Prior
See 1.14.79-universal.

## 1.14.79-universal — 2026-08-04

### Direction
- Algocube digit **1** → stack+dual imm 4-bit field GTE (nibble ladder after SLTE4N/DLTE4N; complete ordered plane for all field widths).

### Language
- `SGTE4N` — `field n → nibble n of TOS = (nib >= field) ? 1 : 0`
- `DGTE4N` — `field n → nibble n of each = (nib >= field) ? 1 : 0` on top two
- Proof `536_sgte4n_dgte4n.cubalc`

### Prior
See 1.14.78-universal.

## 1.14.78-universal — 2026-08-04

### Direction
- Algocube digit **0** → dual-stack imm field GTE (unsigned >= predicates DGTE32N/DGTE16N/DGTE8N after stack SGTE + dual DLTE ladder).

### Language
- `DGTE32N` — `field n → word n of each = (w >= field) ? 1 : 0` on top two
- `DGTE16N` — `field n → halfword n of each = (hw >= field) ? 1 : 0`
- `DGTE8N` — `field n → byte n of each = (b >= field) ? 1 : 0`
- Proof `535_dgte32n_dgte8n.cubalc`

### Prior
See 1.14.77-universal.

## 1.14.77-universal — 2026-08-04

### Direction
- Algocube digit **6** → stack imm field GTE (unsigned >= predicates SGTE32N/SGTE16N/SGTE8N after SLTE ladder; complete ordered plane).

### Language
- `SGTE32N` — `field n → word n of TOS = (w >= field) ? 1 : 0`
- `SGTE16N` — `field n → halfword n of TOS = (hw >= field) ? 1 : 0`
- `SGTE8N` — `field n → byte n of TOS = (b >= field) ? 1 : 0`
- Proof `534_sgte32n_sgte8n.cubalc`

### Prior
See 1.14.76-universal.

## 1.14.76-universal — 2026-08-04

### Direction
- Algocube digit **1** → dual-stack imm 4-bit field ordered-cmp (dual of SLT4N/SGT4N/SLTE4N after DEQ4N/DNE4N).

### Language
- `DLT4N` — `field n → nibble n of each = (nib < field) ? 1 : 0` on top two
- `DGT4N` — `field n → nibble n of each = (nib > field) ? 1 : 0`
- `DLTE4N` — `field n → nibble n of each = (nib <= field) ? 1 : 0`
- Proof `533_dlt4n_dlte4n.cubalc`

### Prior
See 1.14.75-universal.

## 1.14.75-universal — 2026-08-04

### Direction
- Algocube digit **8** → stack imm 4-bit field ordered-cmp (nibble ladder of SLT8N/SGT8N/SLTE8N after SEQ4N/SNE4N).

### Language
- `SLT4N` — `field n → nibble n of TOS = (nib < field) ? 1 : 0`
- `SGT4N` — `field n → nibble n of TOS = (nib > field) ? 1 : 0`
- `SLTE4N` — `field n → nibble n of TOS = (nib <= field) ? 1 : 0`
- Proof `532_slt4n_slte4n.cubalc`

### Prior
See 1.14.74-universal.

## 1.14.74-universal — 2026-08-04

### Direction
- Algocube digit **3** → dual-stack imm 8-bit field ordered-cmp (dual of SLT8N/SGT8N/SLTE8N after DEQ8N/DNE8N).

### Language
- `DLT8N` — `field n → byte n of each = (b < field) ? 1 : 0` on top two
- `DGT8N` — `field n → byte n of each = (b > field) ? 1 : 0`
- `DLTE8N` — `field n → byte n of each = (b <= field) ? 1 : 0`
- Proof `531_dlt8n_dlte8n.cubalc`

### Prior
See 1.14.73-universal.

## 1.14.73-universal — 2026-08-04

### Direction
- Algocube digit **4** → stack imm 8-bit field ordered-cmp (byte ladder of SLT16N/SGT16N/SLTE16N after SEQ8N/SNE8N).

### Language
- `SLT8N` — `field n → byte n of TOS = (b < field) ? 1 : 0`
- `SGT8N` — `field n → byte n of TOS = (b > field) ? 1 : 0`
- `SLTE8N` — `field n → byte n of TOS = (b <= field) ? 1 : 0`
- Proof `530_slt8n_slte8n.cubalc`

### Prior
See 1.14.72-universal.

## 1.14.72-universal — 2026-08-04

### Direction
- Algocube digit **3** → dual-stack imm 16-bit field ordered-cmp (dual of SLT16N/SGT16N/SLTE16N after DEQ16N/DNE16N).

### Language
- `DLT16N` — `field n → halfword n of each = (hw < field) ? 1 : 0` on top two
- `DGT16N` — `field n → halfword n of each = (hw > field) ? 1 : 0`
- `DLTE16N` — `field n → halfword n of each = (hw <= field) ? 1 : 0`
- Proof `529_dlt16n_dlte16n.cubalc`

### Prior
See 1.14.71-universal.

## 1.14.71-universal — 2026-08-04

### Direction
- Algocube digit **4** → stack imm 16-bit field ordered-cmp (halfword ladder of SLT32N/SGT32N/SLTE32N after SEQ16N/SNE16N).

### Language
- `SLT16N` — `field n → halfword n of TOS = (hw < field) ? 1 : 0`
- `SGT16N` — `field n → halfword n of TOS = (hw > field) ? 1 : 0`
- `SLTE16N` — `field n → halfword n of TOS = (hw <= field) ? 1 : 0`
- Proof `528_slt16n_slte16n.cubalc`

### Prior
See 1.14.70-universal.

## 1.14.70-universal — 2026-08-04

### Direction
- Algocube digit **6** → dual-stack imm 32-bit field ordered-cmp (unsigned lt/gt/lte after DEQ32N/DNE32N; dual of SLT32N/SGT32N/SLTE32N).

### Language
- `DLT32N` — `field n → word n of each = (w < field) ? 1 : 0` on top two
- `DGT32N` — `field n → word n of each = (w > field) ? 1 : 0`
- `DLTE32N` — `field n → word n of each = (w <= field) ? 1 : 0`
- Proof `527_dlt32n_dlte32n.cubalc`

### Prior
See 1.14.69-universal.

## 1.14.69-universal — 2026-08-04

### Direction
- Algocube digit **1** → stack imm 32-bit field ordered-cmp (unsigned lt/gt/lte after SEQ32N/SNE32N; complete stack 32n field compare plane).

### Language
- `SLT32N` — `field n → word n of TOS = (w < field) ? 1 : 0`
- `SGT32N` — `field n → word n of TOS = (w > field) ? 1 : 0`
- `SLTE32N` — `field n → word n of TOS = (w <= field) ? 1 : 0`
- Proof `526_slt32n_slte32n.cubalc`

### Prior
See 1.14.68-universal.

## 1.14.68-universal — 2026-08-04

### Direction
- Algocube digit **5** → dual-stack imm 32-bit field max+eq (word ladder of DMAX16N/DEQ16N/DNE16N; dual of SMAX32N after DMIN32N).

### Language
- `DMAX32N` — `field n → word n of each = max(w, field)` unsigned on top two
- `DEQ32N` — `field n → word n of each = (w == field) ? 1 : 0`
- `DNE32N` — `field n → word n of each = (w != field) ? 1 : 0`
- Proof `525_dmax32n_dne32n.cubalc`

### Prior
See 1.14.67-universal.

## 1.14.67-universal — 2026-08-04

### Direction
- Algocube digit **6** → stack imm 32-bit field max+eq (word ladder of SMAX16N/SEQ16N/SNE16N after SMIN32N).

### Language
- `SMAX32N` — `field n → word n of TOS = max(w, field)` unsigned
- `SEQ32N` — `field n → word n of TOS = (w == field) ? 1 : 0`
- `SNE32N` — `field n → word n of TOS = (w != field) ? 1 : 0`
- Proof `524_smax32n_sne32n.cubalc`

### Prior
See 1.14.66-universal.

## 1.14.66-universal — 2026-08-04

### Direction
- Algocube digit **1** → dual-stack imm 32-bit field div/mod/min (word ladder of DDIV16N/DMOD16N/DMIN16N; dual of SDIV32N after DMUL32N).

### Language
- `DDIV32N` — `field n → word n of each = w / field` (field 0 → 0) on top two
- `DMOD32N` — `field n → word n of each = w % field` (field 0 → 0)
- `DMIN32N` — `field n → word n of each = min(w, field)` unsigned
- Proof `523_ddiv32n_dmin32n.cubalc`

### Prior
See 1.14.65-universal.

## 1.14.65-universal — 2026-08-04

### Direction
- Algocube digit **7** → stack imm 32-bit field div/mod/min (word ladder of SDIV16N/SMOD16N/SMIN16N after SMUL32N).

### Language
- `SDIV32N` — `field n → word n of TOS = w / field` (field 0 → 0)
- `SMOD32N` — `field n → word n of TOS = w % field` (field 0 → 0)
- `SMIN32N` — `field n → word n of TOS = min(w, field)` unsigned
- Proof `522_sdiv32n_smin32n.cubalc`

### Prior
See 1.14.64-universal.

## 1.14.64-universal — 2026-08-04

### Direction
- Algocube digit **8** → dual-stack imm 32-bit field arith (word ladder of DADD16N/DSUB16N/DMUL16N; dual of SADD32N after DPARITY32N).

### Language
- `DADD32N` — `field n → word n of each = (w + field) & 0xFFFFFFFF` wrap on top two
- `DSUB32N` — `field n → word n of each = (w - field) & 0xFFFFFFFF` wrap
- `DMUL32N` — `field n → word n of each = (w * field) & 0xFFFFFFFF` wrap
- Proof `521_dadd32n_dmul32n.cubalc`

### Prior
See 1.14.63-universal.

## 1.14.63-universal — 2026-08-04

### Direction
- Algocube digit **5** → stack imm 32-bit field arith merge (word ladder of SADD16N/SSUB16N/SMUL16N after SPARITY32N metrics).

### Language
- `SADD32N` — `field n → word n of TOS = (w + field) & 0xFFFFFFFF` wrap
- `SSUB32N` — `field n → word n of TOS = (w - field) & 0xFFFFFFFF` wrap
- `SMUL32N` — `field n → word n of TOS = (w * field) & 0xFFFFFFFF` wrap
- Proof `520_sadd32n_smul32n.cubalc`

### Prior
See 1.14.62-universal.

## 1.14.62-universal — 2026-08-04

### Direction
- Algocube digit **8** → dual-stack imm 32-bit field metrics (word ladder of DBITREV16N/DPOPCNT16N/DPARITY16N; dual of SBITREV32N after DCLZ32N).

### Language
- `DBITREV32N` — `n → word n of each = bitrev32(w)` on top two
- `DPOPCNT32N` — `n → word n of each = popcount(w)` (0..32)
- `DPARITY32N` — `n → word n of each = xor-reduce(w)` low bit
- Proof `519_dbitrev32n_dparity32n.cubalc`

### Prior
See 1.14.61-universal.

## 1.14.61-universal — 2026-08-04

### Direction
- Algocube digit **5** → stack imm 32-bit field metrics (word ladder of SBITREV16N/SPOPCNT16N/SPARITY16N after SABS32N plane).

### Language
- `SBITREV32N` — `n → word n of TOS = bitrev32(w)`
- `SPOPCNT32N` — `n → word n of TOS = popcount(w)` (0..32)
- `SPARITY32N` — `n → word n of TOS = xor-reduce(w)` low bit
- Proof `518_sbitrev32n_sparity32n.cubalc`

### Prior
See 1.14.60-universal.

## 1.14.60-universal — 2026-08-04

### Direction
- Algocube digit **1** → dual-stack imm 32-bit field zeros+signed (word ladder of DCLZ16N/DCTZ16N/DNEG16N; dual of SCLZ32N after DABS32N).

### Language
- `DCLZ32N` — `n → word n of each = clz32(w)` (0 → 32) on top two
- `DCTZ32N` — `n → word n of each = ctz32(w)` (0 → 32)
- `DNEG32N` — `n → word n of each = -(int32)` as uint32 (min stays 0x80000000)
- Proof `517_dclz32n_dneg32n.cubalc`

### Prior
See 1.14.59-universal.

## 1.14.59-universal — 2026-08-04

### Direction
- Algocube digit **1** → dual-stack imm 32-bit field abs+extend (word ladder of DABS16N/DSEXT16N/DZEXT16N; dual of SABS32N after DSHL32N).

### Language
- `DABS32N` — `n → word n of each = abs(int32)` on top two (min stays 0x80000000)
- `DSEXT32N` — `n → each = sign-extend word n to full width`
- `DZEXT32N` — `n → each = zero-extend word n`
- Proof `516_dabs32n_dzext32n.cubalc`

### Prior
See 1.14.58-universal.

## 1.14.58-universal — 2026-08-04

### Direction
- Algocube digit **9** → stack imm 32-bit field abs+extend (word ladder of SABS16N/SSEXT16N/SZEXT16N after SNEG32N plane).

### Language
- `SABS32N` — `n → word n of TOS = abs(int32)` (min stays 0x80000000)
- `SSEXT32N` — `n → TOS = sign-extend word n of TOS to full width`
- `SZEXT32N` — `n → TOS = zero-extend word n of TOS`
- Proof `515_sabs32n_szext32n.cubalc`

### Prior
See 1.14.57-universal.

## 1.14.57-universal — 2026-08-04

### Direction
- Algocube digit **3** → stack imm 32-bit field zeros+signed (word ladder of SCLZ16N/SCTZ16N/SNEG16N after SSHL32N plane).

### Language
- `SCLZ32N` — `n → word n of TOS = clz32(w)` (0 → 32)
- `SCTZ32N` — `n → word n of TOS = ctz32(w)` (0 → 32)
- `SNEG32N` — `n → word n of TOS = -(int32)` as uint32 (min stays 0x80000000)
- Proof `514_sclz32n_sneg32n.cubalc`

### Prior
See 1.14.56-universal.

## 1.14.56-universal — 2026-08-04

### Direction
- Algocube digit **8** → dual-stack imm 32-bit field shift (word ladder of DSHL16N/DSHR16N/DSAR16N; dual of SSHL32N after DNOT32N).

### Language
- `DSHL32N` — `k n → word n of each = (uint32)<<k` (k≥32 → 0) on top two
- `DSHR32N` — `k n → word n of each = (uint32)>>k` logical (k≥32 → 0)
- `DSAR32N` — `k n → word n of each = (int32)>>k` arithmetic (k≥32 → all sign)
- Proof `513_dshl32n_dsar32n.cubalc`

### Prior
See 1.14.55-universal.

## 1.14.55-universal — 2026-08-04

### Direction
- Algocube digit **1** → dual-stack imm 32-bit field unary+rotate (word ladder of DNOT16N/DROL16N/DROR16N; dual of SNOT32N).

### Language
- `DNOT32N` — `n → word n of each = ~w & 0xFFFFFFFF` on top two
- `DROL32N` — `k n → rotl32 word n of each by k&31`
- `DROR32N` — `k n → rotr32 word n of each by k&31`
- Proof `512_dnot32n_dror32n.cubalc`

### Prior
See 1.14.54-universal.

## 1.14.54-universal — 2026-08-04

### Direction
- Algocube digit **0** → stack imm 32-bit field shift (word ladder of SSHL16N/SSHR16N/SSAR16N after SNOT32N plane).

### Language
- `SSHL32N` — `k n → word n of TOS = (uint32)<<k` (k≥32 → 0)
- `SSHR32N` — `k n → word n of TOS = (uint32)>>k` logical (k≥32 → 0)
- `SSAR32N` — `k n → word n of TOS = (int32)>>k` arithmetic (k≥32 → all sign)
- Proof `511_sshl32n_ssar32n.cubalc`

### Prior
See 1.14.53-universal.

## 1.14.53-universal — 2026-08-04

### Direction
- Algocube digit **1** → stack imm 32-bit field unary+rotate (word ladder of SNOT16N/SROL16N/SROR16N; start stack 32n unary after SNAND32N).

### Language
- `SNOT32N` — `n → word n of TOS = ~w & 0xFFFFFFFF`
- `SROL32N` — `k n → rotl32 word n of TOS by k&31`
- `SROR32N` — `k n → rotr32 word n of TOS by k&31`
- Proof `510_snot32n_sror32n.cubalc`

### Prior
See 1.14.52-universal.

## 1.14.52-universal — 2026-08-04

### Direction
- Algocube digit **1** → dual-stack imm 16-bit field zeros+signed (halfword ladder of DCLZ8N/DCTZ8N/DNEG8N; dual of SCLZ16N after DPARITY16N).

### Language
- `DCLZ16N` — `n → halfword n of each = clz16(hw)` (0 → 16) on top two
- `DCTZ16N` — `n → halfword n of each = ctz16(hw)` (0 → 16)
- `DNEG16N` — `n → halfword n of each = -(int16)` as uint16 (min stays 0x8000)
- Proof `509_dclz16n_dneg16n.cubalc`

### Prior
See 1.14.51-universal.

## 1.14.51-universal — 2026-08-04

### Direction
- Algocube digit **5** → dual-stack imm 16-bit field metrics (halfword ladder of DBITREV8N/DPOPCNT8N/DPARITY8N; dual of SBITREV16N after DSHL16N).

### Language
- `DBITREV16N` — `n → halfword n of each = bitrev16(hw)` on top two
- `DPOPCNT16N` — `n → halfword n of each = popcount(hw)` (0..16)
- `DPARITY16N` — `n → halfword n of each = xor-reduce(hw)` low bit
- Proof `508_dbitrev16n_dparity16n.cubalc`

### Prior
See 1.14.50-universal.

## 1.14.50-universal — 2026-08-04

### Direction
- Algocube digit **3** → stack imm 16-bit field zeros+signed (halfword ladder of SCLZ8N/SCTZ8N/SNEG8N after SPARITY16N).

### Language
- `SCLZ16N` — `n → halfword n of TOS = clz16(hw)` (0 → 16)
- `SCTZ16N` — `n → halfword n of TOS = ctz16(hw)` (0 → 16)
- `SNEG16N` — `n → halfword n of TOS = -(int16)` as uint16 (min stays 0x8000)
- Proof `507_sclz16n_sneg16n.cubalc`

### Prior
See 1.14.49-universal.

## 1.14.49-universal — 2026-08-04

### Direction
- Algocube digit **5** → stack imm 16-bit field metrics (halfword ladder of SBITREV8N/SPOPCNT8N/SPARITY8N after SSHL16N plane).

### Language
- `SBITREV16N` — `n → halfword n of TOS = bitrev16(hw)`
- `SPOPCNT16N` — `n → halfword n of TOS = popcount(hw)` (0..16)
- `SPARITY16N` — `n → halfword n of TOS = xor-reduce(hw)` low bit
- Proof `506_sbitrev16n_sparity16n.cubalc`

### Prior
See 1.14.48-universal.

## 1.14.48-universal — 2026-08-04

### Direction
- Algocube digit **8** → stack imm 16-bit field shift (halfword ladder of SSHL8N/SSHR8N/SSAR8N; stack dual of DSHL16N after SNOT16N).

### Language
- `SSHL16N` — `k n → halfword n of TOS = (uint16)<<k` (k≥16 → 0)
- `SSHR16N` — `k n → halfword n of TOS = (uint16)>>k` logical (k≥16 → 0)
- `SSAR16N` — `k n → halfword n of TOS = (int16)>>k` arithmetic (k≥16 → all sign)
- Proof `505_sshl16n_ssar16n.cubalc`

### Prior
See 1.14.47-universal.

## 1.14.47-universal — 2026-08-04

### Direction
- Algocube digit **8** → dual-stack imm 16-bit field shift (halfword ladder of DSHL8N/DSHR8N/DSAR8N after DNOT16N).

### Language
- `DSHL16N` — `k n → (uint16 half n)<<k` (k≥16 → 0) on each of top two
- `DSHR16N` — `k n → (uint16 half n)>>k` logical (k≥16 → 0) on each
- `DSAR16N` — `k n → (int16 half n)>>k` arithmetic (k≥16 → all sign) on each
- Proof `504_dshl16n_dsar16n.cubalc`

### Prior
See 1.14.46-universal.

## 1.14.46-universal — 2026-08-04

### Direction
- Algocube digit **1** → stack imm 16-bit field unary+rotate (halfword ladder of SNOT8N/SROL8N/SROR8N; stack dual of DNOT16N).

### Language
- `SNOT16N` — `n → halfword n of TOS = ~hw & 0xFFFF`
- `SROL16N` — `k n → rotl16 halfword n of TOS by k&15`
- `SROR16N` — `k n → rotr16 halfword n of TOS by k&15`
- Proof `503_snot16n_sror16n.cubalc`

### Prior
See 1.14.45-universal.

## 1.14.45-universal — 2026-08-04

### Direction
- Algocube digit **1** → dual-stack imm 16-bit field unary+rotate (halfword ladder of DNOT8N/DROL8N/DROR8N; start dual 16n unary plane).

### Language
- `DNOT16N` — `n → halfword n of each = ~hw & 0xFFFF`
- `DROL16N` — `k n → rotl16 halfword n of each by k&15`
- `DROR16N` — `k n → rotr16 halfword n of each by k&15`
- Proof `502_dnot16n_dror16n.cubalc`

### Prior
See 1.14.44-universal.

## 1.14.44-universal — 2026-08-04

### Direction
- Algocube digit **5** → dual-stack imm 16-bit field abs+extend (halfword ladder of DABS8N; dual of SABS16N plane).

### Language
- `DABS16N` — `n → halfword n of each = abs(int16)` (min -32768 → 0x8000)
- `DSEXT16N` — `n → each cell = sign-extend halfword n to full width`
- `DZEXT16N` — `n → each cell = zero-extend halfword n`
- Proof `501_dabs16n_dzext16n.cubalc`

### Prior
See 1.14.43-universal.

## 1.14.43-universal — 2026-08-04

### Direction
- Algocube digit **9** → stack imm 16-bit field abs+extend (halfword ladder of SABS8N/SSEXT8N/SZEXT8N after SNE16N).

### Language
- `SABS16N` — `n → halfword n of TOS = abs(int16)` (min -32768 → 0x8000)
- `SSEXT16N` — `n → TOS = sign-extend halfword n to full width`
- `SZEXT16N` — `n → TOS = zero-extend halfword n`
- Proof `500_sabs16n_szext16n.cubalc`

### Prior
See 1.14.42-universal.

## 1.14.42-universal — 2026-08-04

### Direction
- Algocube digit **0** → stack imm 8-bit field shift (byte ladder of SSHL4N/SSHR4N/SSAR4N; stack dual of DSHL8N; complete stack 8n unary plane).

### Language
- `SSHL8N` — `k n → (uint8 byte n)<<k` (k≥8 → 0)
- `SSHR8N` — `k n → (uint8 byte n)>>k` logical (k≥8 → 0)
- `SSAR8N` — `k n → (int8 byte n)>>k` arithmetic (k≥8 → all sign)
- Proof `499_sshl8n_ssar8n.cubalc`

### Prior
See 1.14.41-universal.

## 1.14.41-universal — 2026-08-04

### Direction
- Algocube digit **7** → stack imm 8-bit field unary+rotate (byte ladder of SNOT4N/SROL4N/SROR4N; stack dual of DNOT8N; close stack 8n unary hole).

### Language
- `SNOT8N` — `n → byte n of TOS = ~byte & 0xFF`
- `SROL8N` — `k n → rotl8 byte n of TOS by k&7`
- `SROR8N` — `k n → rotr8 byte n of TOS by k&7`
- Proof `498_snot8n_sror8n.cubalc`

### Prior
See 1.14.40-universal.

## 1.14.40-universal — 2026-08-04

### Direction
- Algocube digit **9** → stack imm 8-bit field zeros+signed (byte ladder of SCLZ4N/SCTZ4N/SNEG4N; stack dual of DCLZ8N plane after SPARITY8N).

### Language
- `SCLZ8N` — `n → byte n of TOS = clz8(byte)` (0 → 8)
- `SCTZ8N` — `n → byte n of TOS = ctz8(byte)` (0 → 8)
- `SNEG8N` — `n → byte n of TOS = -(int8)` as uint8 (min -128 stays 0x80)
- Proof `497_sclz8n_sneg8n.cubalc`

### Prior
See 1.14.39-universal.

## 1.14.39-universal — 2026-08-04

### Direction
- Algocube digit **4** → stack imm 8-bit field metrics (byte ladder of SBITREV4N/SPOPCNT4N/SPARITY4N; stack dual of DBITREV8N plane).

### Language
- `SBITREV8N` — `n → byte n of TOS = bitrev8(byte)`
- `SPOPCNT8N` — `n → byte n of TOS = popcount(byte)`
- `SPARITY8N` — `n → byte n of TOS = xor-reduce(byte) in low bit`
- Proof `496_sbitrev8n_sparity8n.cubalc`

### Prior
See 1.14.38-universal.

## 1.14.38-universal — 2026-08-04

### Direction
- Algocube digit **5** → stack imm 8-bit field abs+extend (byte ladder of SABS4N/SSEXT4N/SZEXT4N; stack dual of DABS8N plane).

### Language
- `SABS8N` — `n → byte n of TOS = abs(int8)` (min -128 → 0x80)
- `SSEXT8N` — `n → TOS = sign-extend byte n to full width`
- `SZEXT8N` — `n → TOS = zero-extend byte n`
- Proof `495_sabs8n_szext8n.cubalc`

### Prior
See 1.14.37-universal.

## 1.14.37-universal — 2026-08-04

### Direction
- Algocube digit **5** → dual-stack imm 8-bit field abs+extend (byte ladder of DABS4N/DSEXT4N/DZEXT4N; complete dual 8n unary plane).

### Language
- `DABS8N` — `n → byte n of each = abs(int8)` (min -128 → 0x80)
- `DSEXT8N` — `n → each cell = sign-extend byte n to full width`
- `DZEXT8N` — `n → each cell = zero-extend byte n`
- Proof `494_dabs8n_dzext8n.cubalc`

### Prior
See 1.14.36-universal.

## 1.14.36-universal — 2026-08-04

### Direction
- Algocube digit **3** → dual-stack imm 8-bit field zeros+signed (byte ladder of DCLZ4N/DCTZ4N/DNEG4N; dual 8n zeros+negate after DPARITY8N).

### Language
- `DCLZ8N` — `n → byte n of each = clz8(byte)` (0 → 8)
- `DCTZ8N` — `n → byte n of each = ctz8(byte)` (0 → 8)
- `DNEG8N` — `n → byte n of each = -(int8)` as uint8 (min -128 stays 0x80)
- Proof `493_dclz8n_dneg8n.cubalc`

### Prior
See 1.14.35-universal.

## 1.14.35-universal — 2026-08-04

### Direction
- Algocube digit **0** → dual-stack imm 8-bit field metrics (byte ladder of DBITREV4N/DPOPCNT4N/DPARITY4N; dual 8n metrics after DSHL8N).

### Language
- `DBITREV8N` — `n → byte n of each = bitrev8(byte)`
- `DPOPCNT8N` — `n → byte n of each = popcount(byte)`
- `DPARITY8N` — `n → byte n of each = xor-reduce(byte) in low bit`
- Proof `492_dbitrev8n_dparity8n.cubalc`

### Prior
See 1.14.34-universal.

## 1.14.34-universal — 2026-08-04

### Direction
- Algocube digit **0** → dual-stack imm 8-bit field shift (byte ladder of DSHL4N/DSHR4N/DSAR4N; dual 8n shift after DNOT8N/DROL8N).

### Language
- `DSHL8N` — `k n → (uint8 byte n)<<k` (k≥8 → 0) on each of top two
- `DSHR8N` — `k n → (uint8 byte n)>>k` logical (k≥8 → 0) on each
- `DSAR8N` — `k n → (int8 byte n)>>k` arithmetic (k≥8 → all sign) on each
- Proof `491_dshl8n_dsar8n.cubalc`

### Prior
See 1.14.33-universal.

## 1.14.33-universal — 2026-08-04

### Direction
- Algocube digit **1** → dual-stack imm 8-bit field unary+rotate (byte ladder of DNOT4N/DROL4N/DROR4N; start dual 8n unary plane).

### Language
- `DNOT8N` — `n → byte n of each = ~byte & 0xFF`
- `DROL8N` — `k n → rotl8 byte n of each by k&7`
- `DROR8N` — `k n → rotr8 byte n of each by k&7`
- Proof `490_dnot8n_dror8n.cubalc`

### Prior
See 1.14.32-universal.

## 1.14.32-universal — 2026-08-04

### Direction
- Algocube digit **2** → dual-stack imm 4-bit field abs+extend (dual of SABS4N/SSEXT4N/SZEXT4N; complete dual nibble unary plane).

### Language
- `DABS4N` — `n → nibble n of each = abs(int4)` (min -8 → 0x8)
- `DSEXT4N` — `n → each cell = sign-extend nibble n to full width`
- `DZEXT4N` — `n → each cell = zero-extend nibble n`
- Proof `489_dabs4n_dzext4n.cubalc`

### Prior
See 1.14.31-universal.

## 1.14.31-universal — 2026-08-04

### Direction
- Algocube digit **2** → dual-stack imm 4-bit field zeros+signed (dual of SCLZ4N/SCTZ4N/SNEG4N; nibble zeros+negate on pair after DPARITY4N).

### Language
- `DCLZ4N` — `n → nibble n of each = clz4(nibble)` (0 → 4)
- `DCTZ4N` — `n → nibble n of each = ctz4(nibble)` (0 → 4)
- `DNEG4N` — `n → nibble n of each = -(int4)` as uint4 (min -8 stays 0x8)
- Proof `488_dclz4n_dneg4n.cubalc`

### Prior
See 1.14.30-universal.

## 1.14.30-universal — 2026-08-04

### Direction
- Algocube digit **0** → dual-stack imm 4-bit field metrics (dual of SBITREV4N/SPOPCNT4N/SPARITY4N; nibble metrics on pair after DSHL4N).

### Language
- `DBITREV4N` — `n → nibble n of each = bitrev4(nibble)`
- `DPOPCNT4N` — `n → nibble n of each = popcount(nibble)`
- `DPARITY4N` — `n → nibble n of each = xor-reduce(nibble) in low bit`
- Proof `487_dbitrev4n_dparity4n.cubalc`

### Prior
See 1.14.29-universal.

## 1.14.29-universal — 2026-08-04

### Direction
- Algocube digit **2** → dual-stack imm 4-bit field shift (dual of SSHL4N/SSHR4N/SSAR4N; nibble shift plane on pair after DNOT4N/DROL4N).

### Language
- `DSHL4N` — `k n → (uint4 nibble n)<<k` (k≥4 → 0) on each of top two
- `DSHR4N` — `k n → (uint4 nibble n)>>k` logical (k≥4 → 0) on each
- `DSAR4N` — `k n → (int4 nibble n)>>k` arithmetic (k≥4 → all sign) on each
- Proof `486_dshl4n_dsar4n.cubalc`

### Prior
See 1.14.28-universal.

## 1.14.28-universal — 2026-08-04

### Direction
- Algocube digit **1** → dual-stack imm 4-bit field unary+rotate (dual of SNOT4N/SROL4N/SROR4N; nibble unary plane on pair after dual 4n bitwise).

### Language
- `DNOT4N` — `n → nibble n of each = ~nibble & 0xF`
- `DROL4N` — `k n → rotl4 nibble n of each by k&3`
- `DROR4N` — `k n → rotr4 nibble n of each by k&3`
- Proof `485_dnot4n_dror4n.cubalc`

### Prior
See 1.14.27-universal.

## 1.14.27-universal — 2026-08-04

### Direction
- Algocube digit **9** → dual-stack imm 4-bit field max+eq (dual of SMAX4N/SEQ4N/SNE4N; complete min/max + equality on pair nibbles).

### Language
- `DMAX4N` — `field n → nibble n of each = max(nib, field)`
- `DEQ4N` — `field n → nibble n of each = (nib == field) ? 1 : 0`
- `DNE4N` — `field n → nibble n of each = (nib != field) ? 1 : 0`
- Proof `484_dmax4n_dne4n.cubalc`

### Prior
See 1.14.26-universal.

## 1.14.26-universal — 2026-08-04

### Direction
- Algocube digit **4** → dual-stack imm 4-bit field div/mod/min (dual of SDIV4N/SMOD4N/SMIN4N; field 0 → div/mod 0).

### Language
- `DDIV4N` — `field n → nibble n of each = nib / field` (field 0 → 0)
- `DMOD4N` — `field n → nibble n of each = nib % field` (field 0 → 0)
- `DMIN4N` — `field n → nibble n of each = min(nib, field)`
- Proof `483_ddiv4n_dmin4n.cubalc`

### Prior
See 1.14.25-universal.

## 1.14.25-universal — 2026-08-04

### Direction
- Algocube digit **2** → dual-stack imm 4-bit field arith (dual of SADD4N/SSUB4N/SMUL4N; wrap uint4 nibble plane after dual 8n ALU).

### Language
- `DADD4N` — `field n → nibble n of each = (nib + field) & 0xF`
- `DSUB4N` — `field n → nibble n of each = (nib - field) & 0xF`
- `DMUL4N` — `field n → nibble n of each = (nib * field) & 0xF`
- Proof `482_dadd4n_dmul4n.cubalc`

### Prior
See 1.14.24-universal.

## 1.14.24-universal — 2026-08-04

### Direction
- Algocube digit **7** → dual-stack imm 8-bit field max+eq (dual of SMAX8N/SEQ8N/SNE8N; complete min/max + equality on pair bytes).

### Language
- `DMAX8N` — `field n → byte n of each = max(byte, field)`
- `DEQ8N` — `field n → byte n of each = (byte == field) ? 1 : 0`
- `DNE8N` — `field n → byte n of each = (byte != field) ? 1 : 0`
- Proof `481_dmax8n_dne8n.cubalc`

### Prior
See 1.14.23-universal.

## 1.14.23-universal — 2026-08-04

### Direction
- Algocube digit **1** → dual-stack imm 8-bit field div/mod/min (dual of SDIV8N/SMOD8N/SMIN8N; field 0 → div/mod 0).

### Language
- `DDIV8N` — `field n → byte n of each = byte / field` (field 0 → 0)
- `DMOD8N` — `field n → byte n of each = byte % field` (field 0 → 0)
- `DMIN8N` — `field n → byte n of each = min(byte, field)`
- Proof `480_ddiv8n_dmin8n.cubalc`

### Prior
See 1.14.22-universal.

## 1.14.22-universal — 2026-08-04

### Direction
- Algocube digit **7** → dual-stack imm 8-bit field arith (dual of SADD8N/SSUB8N/SMUL8N; wrap uint8 byte plane after dual halfword ALU).

### Language
- `DADD8N` — `field n → byte n of each = (byte + field) & 0xFF`
- `DSUB8N` — `field n → byte n of each = (byte - field) & 0xFF`
- `DMUL8N` — `field n → byte n of each = (byte * field) & 0xFF`
- Proof `479_dadd8n_dmul8n.cubalc`

### Prior
See 1.14.21-universal.

## 1.14.21-universal — 2026-08-04

### Direction
- Algocube digit **0** → dual-stack imm 16-bit field max+eq (dual of SMAX16N/SEQ16N/SNE16N; complete min/max + equality on pair halfwords).

### Language
- `DMAX16N` — `field n → halfword n of each = max(hw, field)`
- `DEQ16N` — `field n → halfword n of each = (hw == field) ? 1 : 0`
- `DNE16N` — `field n → halfword n of each = (hw != field) ? 1 : 0`
- Proof `478_dmax16n_dne16n.cubalc`

### Prior
See 1.14.20-universal.

## 1.14.20-universal — 2026-08-04

### Direction
- Algocube digit **0** → dual-stack imm 16-bit field div/mod/min (dual of SDIV16N/SMOD16N/SMIN16N; field 0 → div/mod 0).

### Language
- `DDIV16N` — `field n → halfword n of each = hw / field` (field 0 → 0)
- `DMOD16N` — `field n → halfword n of each = hw % field` (field 0 → 0)
- `DMIN16N` — `field n → halfword n of each = min(hw, field)`
- Proof `477_ddiv16n_dmin16n.cubalc`

### Prior
See 1.14.19-universal.

## 1.14.19-universal — 2026-08-04

### Direction
- Algocube digit **2** → dual-stack imm 16-bit field arith (dual of SADD16N/SSUB16N/SMUL16N; wrap uint16 halfword plane on top two cells).

### Language
- `DADD16N` — `field n → halfword n of each = (hw + field) & 0xFFFF`
- `DSUB16N` — `field n → halfword n of each = (hw - field) & 0xFFFF`
- `DMUL16N` — `field n → halfword n of each = (hw * field) & 0xFFFF`
- Proof `476_dadd16n_dmul16n.cubalc`

### Prior
See 1.14.18-universal.

## 1.14.18-universal — 2026-08-04

### Direction
- Algocube digit **7** → stack imm 16-bit field max+eq (complete min/max + equality compare plane after SMIN16N; bool writes 0/1 into halfword).

### Language
- `SMAX16N` — `field n → halfword n = max(hw, field)`
- `SEQ16N` — `field n → halfword n = (hw == field) ? 1 : 0`
- `SNE16N` — `field n → halfword n = (hw != field) ? 1 : 0`
- Proof `475_smax16n_sne16n.cubalc`

### Prior
See 1.14.17-universal.

## 1.14.17-universal — 2026-08-04

### Direction
- Algocube digit **7** → stack imm 16-bit field div/mod/min (complete uint16 field ALU after SADD16N/SSUB16N/SMUL16N; field 0 → div/mod 0).

### Language
- `SDIV16N` — `field n → halfword n = hw / field` (field 0 → 0)
- `SMOD16N` — `field n → halfword n = hw % field` (field 0 → 0)
- `SMIN16N` — `field n → halfword n = min(hw, field)`
- Proof `474_sdiv16n_smin16n.cubalc`

### Prior
See 1.14.16-universal.

## 1.14.16-universal — 2026-08-04

### Direction
- Algocube digit **3** → stack imm 16-bit field arith merge (halfword dual of SADD8N/SSUB8N/SMUL8N; wrap uint16 ALU foundation after SAND16N plane).

### Language
- `SADD16N` — `field n → halfword n = (hw + field) & 0xFFFF`
- `SSUB16N` — `field n → halfword n = (hw - field) & 0xFFFF`
- `SMUL16N` — `field n → halfword n = (hw * field) & 0xFFFF`
- Proof `473_sadd16n_smul16n.cubalc`

### Prior
See 1.14.15-universal.

## 1.14.15-universal — 2026-08-04

### Direction
- Algocube digit **3** → stack imm 8-bit field max+eq (complete min/max + equality compare plane after SMIN8N; bool writes 0/1 into byte).

### Language
- `SMAX8N` — `field n → byte n = max(byte, field)`
- `SEQ8N` — `field n → byte n = (byte == field) ? 1 : 0`
- `SNE8N` — `field n → byte n = (byte != field) ? 1 : 0`
- Proof `472_smax8n_sne8n.cubalc`

### Prior
See 1.14.14-universal.

## 1.14.14-universal — 2026-08-04

### Direction
- Algocube digit **2** → stack imm 8-bit field div/mod/min (complete uint8 field ALU after SADD8N/SSUB8N/SMUL8N; field 0 → div/mod 0).

### Language
- `SDIV8N` — `field n → byte n = byte / field` (field 0 → 0)
- `SMOD8N` — `field n → byte n = byte % field` (field 0 → 0)
- `SMIN8N` — `field n → byte n = min(byte, field)`
- Proof `471_sdiv8n_smin8n.cubalc`

### Prior
See 1.14.13-universal.

## 1.14.13-universal — 2026-08-04

### Direction
- Algocube digit **0** → stack imm 8-bit field arith merge (byte-field dual of SADD4N/SSUB4N/SMUL4N; wrap uint8 ALU foundation after SAND8N plane).

### Language
- `SADD8N` — `field n → byte n = (byte + field) & 0xFF`
- `SSUB8N` — `field n → byte n = (byte - field) & 0xFF`
- `SMUL8N` — `field n → byte n = (byte * field) & 0xFF`
- Proof `470_sadd8n_smul8n.cubalc`

### Prior
See 1.14.12-universal.

## 1.14.12-universal — 2026-08-04

### Direction
- Algocube digit **9** → stack imm 4-bit field max+eq (complete min/max + equality compare plane after SMIN4N; bool writes 0/1 into nibble).

### Language
- `SMAX4N` — `field n → nibble n = max(nibble, field)`
- `SEQ4N` — `field n → nibble n = (nibble == field) ? 1 : 0`
- `SNE4N` — `field n → nibble n = (nibble != field) ? 1 : 0`
- Proof `469_smax4n_sne4n.cubalc`

### Prior
See 1.14.11-universal.

## 1.14.11-universal — 2026-08-04

### Direction
- Algocube digit **1** → stack imm 4-bit field div/mod/min (complete uint4 field ALU after SADD4N/SSUB4N/SMUL4N; field 0 → div/mod 0).

### Language
- `SDIV4N` — `field n → nibble n = nibble / field` (field 0 → 0)
- `SMOD4N` — `field n → nibble n = nibble % field` (field 0 → 0)
- `SMIN4N` — `field n → nibble n = min(nibble, field)`
- Proof `468_sdiv4n_smin4n.cubalc`

### Prior
See 1.14.10-universal.

## 1.14.10-universal — 2026-08-04

### Direction
- Algocube digit **0** → stack imm 4-bit field arith merge (nibble-field dual of bitwise SAND4N plane; wrap uint4 ALU foundation after SZEXT4N).

### Language
- `SADD4N` — `field n → nibble n = (nibble + field) & 0xF`
- `SSUB4N` — `field n → nibble n = (nibble - field) & 0xF`
- `SMUL4N` — `field n → nibble n = (nibble * field) & 0xF`
- Proof `467_sadd4n_smul4n.cubalc`

### Prior
See 1.14.9-universal.

## 1.14.9-universal — 2026-08-04

### Direction
- Algocube digit **7** → stack imm 4-bit field abs+extend (nibble-field dual of ABS4/SEXT4/ZEXT4 after SNEG4N; complete stack nibble signed extract).

### Language
- `SABS4N` — `n → nibble n = abs(int4)`; min −8 → +8
- `SSEXT4N` — `n → TOS = sign-extend nibble n`
- `SZEXT4N` — `n → TOS = zero-extend nibble n`
- Proof `466_sabs4n_szext4n.cubalc`

### Prior
See 1.14.8-universal.

## 1.14.8-universal — 2026-08-04

### Direction
- Algocube digit **6** → stack imm 4-bit field zeros+signed (nibble-field dual of CLZ4/CTZ4/NEG4 after SPARITY4N; complete stack nibble zeros+negate).

### Language
- `SCLZ4N` — `n → nibble n = clz4(nibble)`; 0 → 4
- `SCTZ4N` — `n → nibble n = ctz4(nibble)`; 0 → 4
- `SNEG4N` — `n → nibble n = -(int4)nibble` as uint4; min −8 stays −8
- Proof `465_sclz4n_sneg4n.cubalc`

### Prior
See 1.14.7-universal.

## 1.14.7-universal — 2026-08-04

### Direction
- Algocube digit **4** → stack imm 4-bit field metrics (nibble-field dual of BITREV4/POPCNT4/PARITY4 after SSHL4N; complete stack nibble metrics).

### Language
- `SBITREV4N` — `n → nibble n = bitrev4(nibble)`
- `SPOPCNT4N` — `n → nibble n = popcount(nibble)`
- `SPARITY4N` — `n → nibble n = xor-reduce(nibble)`
- Proof `464_sbitrev4n_sparity4n.cubalc`

### Prior
See 1.14.6-universal.

## 1.14.6-universal — 2026-08-04

### Direction
- Algocube digit **7** → stack imm 4-bit field shift (nibble-field dual of SHL4/SHR4/SAR4 after SROL4N; complete stack nibble shift).

### Language
- `SSHL4N` — `k n → (uint4 nibble n)<<k` (k≥4 → 0)
- `SSHR4N` — `k n → (uint4)>>k` logical
- `SSAR4N` — `k n → (int4)>>k` arithmetic
- Proof `463_sshl4n_ssar4n.cubalc`

### Prior
See 1.14.5-universal.

## 1.14.5-universal — 2026-08-04

### Direction
- Algocube digit **7** → stack imm 4-bit field unary (nibble-field dual of NOT4/ROL4 after SNAND4N; complete stack nibble unary+rotate).

### Language
- `SNOT4N` — `n → nibble n of TOS = ~nibble & 0xF`
- `SROL4N`/`SROR4N` — `k n → rotl4/rotr4 nibble n by k&3`
- Proof `462_snot4n_sror4n.cubalc`

### Prior
See 1.14.4-universal.

## 1.14.4-universal — 2026-08-04

### Direction
- Algocube digit **2** → cell fixed-width 4 nibble unary/signed (nibble dual of NOT8 + CLIPS8 after metrics; complete 4/8/16/32 unary+sclip).

### Language
- `NOT4CELL`/`NOT4RANGE` — `lo hi → ~low4 & 0xF`
- `CLIPS4CELL`/`CLIPS4RANGE` — `lo hi → clamp to signed [-8,7]`
- Proof `461_not4cell_clips4cell.cubalc`

### Prior
See 1.14.3-universal.

## 1.14.3-universal — 2026-08-04

### Direction
- Algocube digit **3** → cell fixed-width 4 nibble metrics foundation (nibble dual of CLZ8/CTZ8/PARITY8 after NEG4 plane; complete 4/8/16/32 metrics).

### Language
- `CLZ4CELL`/`CLZ4RANGE` — `lo hi → clz4(low4)`; 0 → 4
- `CTZ4CELL`/`CTZ4RANGE` — `lo hi → ctz4(low4)`; 0 → 4
- `PARITY4CELL`/`PARITY4RANGE` — `lo hi → xor-reduce low4 bits`
- Proof `460_clz4cell_parity4cell.cubalc`

### Prior
See 1.14.2-universal.

## 1.14.2-universal — 2026-08-04

### Direction
- Algocube digit **7** → cell fixed-width 4 nibble math (nibble dual of NEG8/ZEXT8/CLIP8 after ROL4; complete 4/8/16/32 arithmetic foundation).

### Language
- `NEG4CELL`/`NEG4RANGE` — `lo hi → -(int4)low4`; min int4 −8 stays −8
- `ZEXT4CELL`/`ZEXT4RANGE` — `lo hi → low4` zero-extend (mask 0xF)
- `CLIP4CELL`/`CLIP4RANGE` — `lo hi → clamp to [0,15]`
- Proof `459_neg4cell_clip4cell.cubalc`

### Prior
See 1.14.0-p2p / 1.14.1-universal.

## 1.14.1-universal — 2026-08-04

### Direction
- Algocube digit **2** → cell fixed-width 4 nibble rotate/bitrev (nibble dual of ROL8/ROR8/BITREV8 after SEXT4 plane; complete 4/8/16/32 rotate ladder).

### Language
- `ROL4CELL`/`ROL4RANGE` — `lo hi k → rotl4(low4, k&3)`
- `ROR4CELL`/`ROR4RANGE` — `lo hi k → rotr4(low4, k&3)`
- `BITREV4CELL`/`BITREV4RANGE` — `lo hi → bitrev4(low4)`
- Proof `458_rol4cell_bitrev4cell.cubalc`

### Prior
See 1.13.3-universal (p2p WIP at 1.14.0-p2p local only).

## 1.13.3-universal — 2026-08-04

### Direction
- Algocube digit **2** → cell fixed-width 4 nibble signed/metrics (nibble dual of SEXT8/ABS8/POPCNT8 after SAR4; complete 4/8/16/32 signed+pop).

### Language
- `SEXT4CELL`/`SEXT4RANGE` — `lo hi → (int4)low4` sign-extend to long
- `ABS4CELL`/`ABS4RANGE` — `lo hi → abs(int4)`; min int4 −8 stays 8
- `POPCNT4CELL`/`POPCNT4RANGE` — `lo hi → popcount(low4)`
- Proof `457_sext4cell_popcnt4cell.cubalc`

### Prior
See 1.13.2-universal.

## 1.13.2-universal — 2026-08-04

### Direction
- Algocube digit **9** → cell fixed-width 4 nibble shift ALU (nibble dual of SHL8/SHR8/SAR8 after inverted nibble; complete 4/8/16/32 shift ladder).

### Language
- `SHL4CELL`/`SHL4RANGE` — `lo hi k → (uint4)<<k` (k≥4 → 0)
- `SHR4CELL`/`SHR4RANGE` — `lo hi k → (uint4)>>k` logical (k≥4 → 0)
- `SAR4CELL`/`SAR4RANGE` — `lo hi k → (int4)>>k` arithmetic (k≥4 → all sign)
- Proof `456_shl4cell_sar4cell.cubalc`

### Prior
See 1.13.1-universal.

## 1.13.1-universal — 2026-08-04

### Direction
- Algocube digit **5** → cell fixed-width 4 nibble inverted bitwise (inverted dual of AND4/OR4/XOR4 after base nibble plane; complete 4/8/16/32 inverted ladder).

### Language
- `NAND4CELL`/`NAND4RANGE` — `lo hi mask → ~(low4 & low4(mask)) & 0xF`
- `NOR4CELL`/`NOR4RANGE` — `lo hi mask → ~(low4 | low4(mask)) & 0xF`
- `XNOR4CELL`/`XNOR4RANGE` — `lo hi mask → ~(low4 ^ low4(mask)) & 0xF`
- Proof `455_nand4cell_xnor4cell.cubalc`

### Prior
See 1.13.0-modular.

## 1.13.0-modular — 2026-08-04

### Direction
- **Project structure modularization** (Cube laws: pure C · multiplatform · SoT · devices free).
- Split monolithic `cubalc_lang.c` (~31k lines) into plane modules under `src/lang/`.

### Structure
- `include/cubalc_platform.h` — POSIX / Darwin / Windows shims
- `include/lang/cubalc_lang_internal.h` — VM / lexer / plane API (internal)
- `src/lang/lang_core.c` · `lang_ops_{core,toc,stack,dual,math,bit,cell,flow}.c`
- `src/lang/lang_parse.c` — plane dispatcher · `lang_run.c` — public run API
- Multiplatform `Makefile` (`CUBALC_TARGET`, optional OpenCL framework on Darwin)
- Docs: `docs/STRUCTURE.md` · `src/lang/README.md`
- Tool: `tools/modularize_lang.py` (re-split if thrash restores a monolith)

### Law
- Public API unchanged (`cubalc_lang.h`: `cubalc_run_source` / `cubalc_run_file`)
- Behavior proof-gated: universal suite green after split

### Prior
See 1.12.431-universal.

## 1.12.431-universal — 2026-08-04

### Direction
- Algocube digit **4** → cell fixed-width 4 nibble bitwise (nibble dual of AND8/OR8/XOR8 after NOT width; complete 4/8/16/32 bitwise base).

### Language
- `AND4CELL`/`AND4RANGE` — `lo hi mask → low4(cells[i]) & low4(mask)`
- `OR4CELL`/`OR4RANGE` — `lo hi mask → low4(cells[i]) | low4(mask)`
- `XOR4CELL`/`XOR4RANGE` — `lo hi mask → low4(cells[i]) ^ low4(mask)`
- Proof `454_and4cell_xor4cell.cubalc`

### Prior
See 1.12.430-universal.

## 1.12.430-universal — 2026-08-04

### Direction
- Algocube digit **5** → cell fixed-width unary NOT (width dual of NOTCELL after 8/16/32 inverted ladder; complete bitwise unary).

### Language
- `NOT8CELL`/`NOT8RANGE` — `lo hi → ~low8 as u8`
- `NOT16CELL`/`NOT16RANGE` — `lo hi → ~low16 as u16`
- `NOT32CELL`/`NOT32RANGE` — `lo hi → ~low32 as u32`
- Proof `453_not8cell_not32cell.cubalc`

### Prior
See 1.12.429-universal.

## 1.12.429-universal — 2026-08-04

### Direction
- Algocube digit **9** → cell fixed-width 16 inverted bitwise (inverted dual of AND16/OR16/XOR16; complete 8/16/32 inverted ladder).

### Language
- `NAND16CELL`/`NAND16RANGE` — `lo hi mask → ~(low16 & mask) as u16`
- `NOR16CELL`/`NOR16RANGE` — `lo hi mask → ~(low16 | mask) as u16`
- `XNOR16CELL`/`XNOR16RANGE` — `lo hi mask → ~(low16 ^ mask) as u16`
- Proof `452_nand16cell_xnor16cell.cubalc`

### Prior
See 1.12.428-universal.

## 1.12.428-universal — 2026-08-04

### Direction
- Algocube digit **9** → cell fixed-width 16 bitwise (word dual of AND8/OR8/XOR8 after inverted8; complete 8/16/32 bitwise ladder).

### Language
- `AND16CELL`/`AND16RANGE` — `lo hi mask → low16(cells[i]) & low16(mask)`
- `OR16CELL`/`OR16RANGE` — `lo hi mask → low16(cells[i]) | low16(mask)`
- `XOR16CELL`/`XOR16RANGE` — `lo hi mask → low16(cells[i]) ^ low16(mask)`
- Proof `451_and16cell_xor16cell.cubalc`

### Prior
See 1.12.427-universal.

## 1.12.427-universal — 2026-08-04

### Direction
- Algocube digit **0** → cell fixed-width 8 inverted bitwise (inverted dual of AND8/OR8/XOR8; complete low8 logic foundation).

### Language
- `NAND8CELL`/`NAND8RANGE` — `lo hi mask → ~(low8 & mask) as u8`
- `NOR8CELL`/`NOR8RANGE` — `lo hi mask → ~(low8 | mask) as u8`
- `XNOR8CELL`/`XNOR8RANGE` — `lo hi mask → ~(low8 ^ mask) as u8`
- Proof `450_nand8cell_xnor8cell.cubalc`

### Prior
See 1.12.426-universal.

## 1.12.426-universal — 2026-08-04

### Direction
- Algocube digit **1** → cell fixed-width 8 bitwise (byte dual of AND32/OR32/XOR32 after inverted32 plane; start 8/32 bitwise ladder).

### Language
- `AND8CELL`/`AND8RANGE` — `lo hi mask → low8(cells[i]) & low8(mask)`
- `OR8CELL`/`OR8RANGE` — `lo hi mask → low8(cells[i]) | low8(mask)`
- `XOR8CELL`/`XOR8RANGE` — `lo hi mask → low8(cells[i]) ^ low8(mask)`
- Proof `449_and8cell_xor8cell.cubalc`

### Prior
See 1.12.425-universal.

## 1.12.425-universal — 2026-08-04

### Direction
- Algocube digit **4** → cell fixed-width 32 inverted bitwise (inverted dual of AND32/OR32/XOR32; complete low32 logic plane).

### Language
- `NAND32CELL`/`NAND32RANGE` — `lo hi mask → ~(low32 & mask) as u32`
- `NOR32CELL`/`NOR32RANGE` — `lo hi mask → ~(low32 | mask) as u32`
- `XNOR32CELL`/`XNOR32RANGE` — `lo hi mask → ~(low32 ^ mask) as u32`
- Proof `448_nand32cell_xnor32cell.cubalc`

### Prior
See 1.12.424-universal.

## 1.12.424-universal — 2026-08-04

### Direction
- Algocube digit **3** → cell fixed-width 32 bitwise (dword dual of ANDCELL/ORCELL/XORCELL after 32 metrics; complete low32 logic plane).

### Language
- `AND32CELL`/`AND32RANGE` — `lo hi mask → low32(cells[i]) & low32(mask)`
- `OR32CELL`/`OR32RANGE` — `lo hi mask → low32(cells[i]) | low32(mask)`
- `XOR32CELL`/`XOR32RANGE` — `lo hi mask → low32(cells[i]) ^ low32(mask)`
- Proof `447_and32cell_xor32cell.cubalc`

### Prior
See 1.12.423-universal.

## 1.12.423-universal — 2026-08-04

### Direction
- Algocube digit **5** → cell fixed-width 32 metrics foundation (dword dual of CLZ8/CTZ8/PARITY8 after POPCNT32; complete 8/16/32 metrics).

### Language
- `CLZ32CELL`/`CLZ32RANGE` — `lo hi → clz32(low32)` (0 → 32)
- `CTZ32CELL`/`CTZ32RANGE` — `lo hi → ctz32(low32)` (0 → 32)
- `PARITY32CELL`/`PARITY32RANGE` — `lo hi → xor-reduce low32 bits`
- Proof `446_clz32cell_parity32cell.cubalc`

### Prior
See 1.12.422-universal.

## 1.12.422-universal — 2026-08-04

### Direction
- Algocube digit **4** → cell fixed-width 32 signed/metrics (dword dual of SEXT8/ABS8/POPCNT8 after NEG32 plane; complete 8/16/32 signed+pop).

### Language
- `SEXT32CELL`/`SEXT32RANGE` — `lo hi → (long)(int32)low32`
- `ABS32CELL`/`ABS32RANGE` — `lo hi → abs((int32)low32)` (min int32 stays `0x80000000`)
- `POPCNT32CELL`/`POPCNT32RANGE` — `lo hi → popcount(low32)`
- Proof `445_sext32cell_popcnt32cell.cubalc`

### Prior
See 1.12.421-universal.

## 1.12.421-universal — 2026-08-04

### Direction
- Algocube digit **3** → cell fixed-width 32 math dual ladder (dword dual of NEG16/ZEXT16/CLIP16; complete 8/16/32 math).

### Language
- `NEG32CELL`/`NEG32RANGE` — `lo hi → -(int32)low32` (min int32 stays `0x80000000`)
- `ZEXT32CELL`/`ZEXT32RANGE` — `lo hi → cells[i] &= 0xFFFFFFFF`
- `CLIP32CELL`/`CLIP32RANGE` — `lo hi → clamp to [0,4294967295]`
- Proof `444_neg32cell_clip32cell.cubalc`

### Prior
See 1.12.420-universal.

## 1.12.420-universal — 2026-08-04

### Direction
- Algocube digit **3** → cell signed-clip dual ladder (signed dual of CLIP8/16 after SCLIPS*TOC; complete 8/16/32 signed clamp).

### Language
- `CLIPS8CELL`/`CLIPS8RANGE` — `lo hi → clamp to [-128,127]`
- `CLIPS16CELL`/`CLIPS16RANGE` — `lo hi → clamp to [-32768,32767]`
- `CLIPS32CELL`/`CLIPS32RANGE` — `lo hi → clamp to signed 32-bit`
- Proof `443_clips8cell_clips32cell.cubalc`

### Prior
See 1.12.419-universal.

## 1.12.419-universal — 2026-08-04

### Direction
- Algocube digit **7** → cell fixed-width 16 math (word dual of NEG8/ZEXT8/CLIP8 after signed16 plane).

### Language
- `NEG16CELL`/`NEG16RANGE` — `lo hi → -(int16)low16` (min int16 stays `0x8000`)
- `ZEXT16CELL`/`ZEXT16RANGE` — `lo hi → cells[i] &= 0xFFFF`
- `CLIP16CELL`/`CLIP16RANGE` — `lo hi → clamp cells[i] to [0,65535]`
- Proof `442_neg16cell_clip16cell.cubalc`

### Prior
See 1.12.418-universal.

## 1.12.418-universal — 2026-08-04

### Direction
- Algocube digit **7** → cell fixed-width 8 math (NEG/ZEXT/CLIP after CLZ8 metrics; complete 8-bit arithmetic foundation).

### Language
- `NEG8CELL`/`NEG8RANGE` — `lo hi → -(int8)low8` (min int8 stays `0x80`)
- `ZEXT8CELL`/`ZEXT8RANGE` — `lo hi → cells[i] &= 0xFF`
- `CLIP8CELL`/`CLIP8RANGE` — `lo hi → clamp cells[i] to [0,255]`
- Proof `441_neg8cell_clip8cell.cubalc`

### Prior
See 1.12.417-universal.

## 1.12.417-universal — 2026-08-04

### Direction
- Algocube digit **0** → cell fixed-width 8 metrics foundation (CLZ/CTZ + parity after POPCNT8; complete 8-bit metrics).

### Language
- `CLZ8CELL`/`CLZ8RANGE` — `lo hi → clz8(low8)` (0 → 8)
- `CTZ8CELL`/`CTZ8RANGE` — `lo hi → ctz8(low8)` (0 → 8)
- `PARITY8CELL`/`PARITY8RANGE` — `lo hi → xor-reduce low8 bits`
- Proof `440_clz8cell_parity8cell.cubalc`

### Prior
See 1.12.416-universal.

## 1.12.416-universal — 2026-08-04

### Direction
- Algocube digit **6** → cell fixed-width 8 signed/metrics (byte dual of SEXT16/ABS16/POPCNT16 after ROL8 plane).

### Language
- `SEXT8CELL`/`SEXT8RANGE` — `lo hi → cells[i] = (long)(int8)low8`
- `ABS8CELL`/`ABS8RANGE` — `lo hi → abs((int8)low8)` (min int8 stays `0x80`)
- `POPCNT8CELL`/`POPCNT8RANGE` — `lo hi → popcount(low8)`
- Proof `439_sext8cell_popcnt8cell.cubalc`

### Prior
See 1.12.415-universal.

## 1.12.415-universal — 2026-08-04

### Direction
- Algocube digit **8** → cell fixed-width 8 rotate/bitrev (byte dual of ROL16/ROR16/BITREV16 after SHL8 plane).

### Language
- `ROL8CELL`/`ROL8RANGE` — `lo hi k → cells[i] = rotl8(low8, k&7)`
- `ROR8CELL`/`ROR8RANGE` — `lo hi k → cells[i] = rotr8(low8, k&7)`
- `BITREV8CELL`/`BITREV8RANGE` — `lo hi → cells[i] = bitrev8(low8)`
- Proof `438_rol8cell_bitrev8cell.cubalc`

### Prior
See 1.12.414-universal.

## 1.12.414-universal — 2026-08-04

### Direction
- Algocube digit **1** → cell fixed-width 8 shift ALU (byte dual of SHL16/SHR16/SAR16; complete 8/16/32 shift ladder).

### Language
- `SHL8CELL`/`SHL8RANGE` — `lo hi k → cells[i] = (uint8)cells[i] ≪ k` (k≥8 → 0)
- `SHR8CELL`/`SHR8RANGE` — `lo hi k → logical ≫ low8`
- `SAR8CELL`/`SAR8RANGE` — `lo hi k → (int8)low8 ≫ k` arithmetic (k≥8 → all sign)
- Proof `437_shl8cell_sar8cell.cubalc`

### Prior
See 1.12.413-universal.

## 1.12.413-universal — 2026-08-04

### Direction
- Algocube digit **9** → cell fixed-width 16 bit-metrics (width-16 dual of POPCNT/CLZ/CTZ after SAR16/SEXT16/ABS16 plane).

### Language
- `POPCNT16CELL`/`POPCNT16RANGE` — `lo hi → cells[i] = popcount(low16)`
- `CLZ16CELL`/`CLZ16RANGE` — `lo hi → cells[i] = clz16(low16)` (0 → 16)
- `CTZ16CELL`/`CTZ16RANGE` — `lo hi → cells[i] = ctz16(low16)` (0 → 16)
- Proof `436_popcnt16cell_ctz16cell.cubalc`

### Prior
See 1.12.412-universal.

## 1.12.412-universal — 2026-08-04

### Direction
- Algocube digit **2** → cell fixed-width 16 signed/bitfield (complete arithmetic shift + sign-extend + abs after logical SHL16/SHR16).

### Language
- `SAR16CELL`/`SAR16RANGE` — `lo hi k → cells[i] = (int16)low16 ≫ k` arithmetic (k≥16 → all sign)
- `SEXT16CELL`/`SEXT16RANGE` — `lo hi → cells[i] = (long)(int16)low16`
- `ABS16CELL`/`ABS16RANGE` — `lo hi → cells[i] = abs((int16)low16)` (min int16 stays `0x8000`)
- Proof `435_sar16cell_abs16cell.cubalc`

### Prior
See 1.12.411-universal.

## 1.12.411-universal — 2026-08-04

### Direction
- Algocube digit **1** → cell fixed-width 16 shift/rotate (complete 16-bit rotate + logical shift plane after ROL16CELL).

### Language
- `ROR16CELL`/`ROR16RANGE` — `lo hi k → cells[i] = rotr16(low16 cells[i], k&15)`
- `SHL16CELL`/`SHL16RANGE` — `lo hi k → cells[i] = (uint16)cells[i] ≪ k` (k≥16 → 0)
- `SHR16CELL`/`SHR16RANGE` — `lo hi k → logical ≫ low16`
- Proof `434_ror16cell_shr16cell.cubalc`

### Prior
See 1.12.410-universal.

## 1.12.410-universal — 2026-08-04

### Direction
- Algocube digit **4** → cell bitfield width ladder (complete bswap 16/32/64 + bitrev 16/32 + rotate16 after BSWAP16CELL).

### Language
- `BSWAP64CELL`/`BSWAP64RANGE` — `lo hi → cells[i] = bswap64(cells[i])`
- `BITREV16CELL`/`BREV16CELL` — `lo hi → cells[i] = bitrev16(low16 cells[i])`
- `ROL16CELL`/`ROL16RANGE` — `lo hi k → cells[i] = rotl16(low16 cells[i], k&15)`
- Proof `433_bswap64cell_rol16cell.cubalc`

### Prior
See 1.12.409-universal.

## 1.12.409-universal — 2026-08-04

### Direction
- Algocube digit **4** → cell bit-metrics ext (complete popcnt/clz/ctz + bswap16 ladder after PARITYCELL plane).

### Language
- `CLZCELL`/`CLZRANGE` — `lo hi → cells[i] = clz64(cells[i])` (0 → 64)
- `CTZCELL`/`CTZRANGE` — `lo hi → cells[i] = ctz64(cells[i])` (0 → 64)
- `BSWAP16CELL`/`BSWAP16RANGE` — `lo hi → cells[i] = bswap16(low16 cells[i])`
- Proof `432_clzcell_bswap16cell.cubalc`

### Prior
See 1.12.408-universal.

## 1.12.408-universal — 2026-08-04

### Direction
- Algocube digit **9** → cell bit-metrics plane (range dual of SPARITY/SBSWAP/SPOPCNT after BITREV32CELL).

### Language
- `PARITYCELL`/`PARITYRANGE` — `lo hi → cells[i] = parity(cells[i])` (xor of bits)
- `BSWAPCELL`/`BSWAP32CELL` — `lo hi → cells[i] = bswap32(low32 cells[i])`
- `POPCNTCELL`/`PCNTCELL` — `lo hi → cells[i] = popcount(cells[i])`
- Proof `431_paritycell_popcntcell.cubalc`

### Prior
See 1.12.407-universal.

## 1.12.407-universal — 2026-08-04

### Direction
- Algocube digit **4** → cell fixed-width rotate32 + bitrev plane (range dual of SROTL32/SROTR32/SBITREV32 after SHL32CELL).

### Language
- `ROL32CELL`/`ROL32RANGE` — `lo hi k → cells[i] = rotl32(low32 cells[i], k&31)`
- `ROR32CELL`/`ROR32RANGE` — `lo hi k → cells[i] = rotr32(low32 cells[i], k&31)`
- `BITREV32CELL`/`BREV32CELL` — `lo hi → cells[i] = bitrev32(low32 cells[i])`
- Proof `430_rol32cell_bitrev32cell.cubalc`

### Prior
See 1.12.406-universal.

## 1.12.406-universal — 2026-08-04

### Direction
- Algocube digit **9** → cell fixed-width shift32 plane (range dual of SSHL32/SSHR32/SSAR32 after NANDCELL logic).

### Language
- `SHL32CELL`/`SHL32RANGE` — `lo hi k → cells[i] = (uint32)cells[i] ≪ k` (k≥32 → 0)
- `SHR32CELL`/`SHR32RANGE` — `lo hi k → logical ≫ low32`
- `SAR32CELL`/`ASHR32CELL` — `lo hi k → arithmetic ≫ low32` (sign-extend)
- Proof `429_shl32cell_sar32cell.cubalc`

### Prior
See 1.12.405-universal.

## 1.12.405-universal — 2026-08-04

### Direction
- Algocube digit **0** → foundation imm fixed-width shift32 TOC (imm dual of SSHL32TOC plane; peer of SROTL32TOCN).

### Language
- `SSHL32TOCN`/`SHL32TOCN` — stack `i + imm k → cells[i] = (uint32)cells[i] ≪ k` (k≥32 → 0)
- `SSHR32TOCN`/`SHR32TOCN` — stack `i + imm k → logical ≫ low32`
- `SSAR32TOCN`/`SAR32TOCN` — stack `i + imm k → arithmetic ≫ low32` (sign-extend)
- Proof `428_sshl32tocn_ssar32tocn.cubalc`

### Prior
See 1.12.404-universal.

## 1.12.404-universal — 2026-08-04

### Direction
- Algocube digit **0** → foundation fixed-width rotate32 TOC (complete shift/rotate 32 into cell after SSHL32TOC plane).

### Language
- `SROTL32TOC`/`ROL32TOC` — stack `i k → cells[i] = rotl32(low32 cells[i], k&31)`
- `SROTR32TOC`/`ROR32TOC` — stack `i k → cells[i] = rotr32(low32 cells[i], k&31)`
- `SROTL32TOCN`/`ROL32TOCN` — stack `i + imm k → rotl32 into cell`
- `SROTR32TOCN`/`ROR32TOCN` — stack `i + imm k → rotr32 into cell`
- Proof `427_srotl32toc_srotr32tocn.cubalc`

### Prior
See 1.12.403-universal.

## 1.12.403-universal — 2026-08-04

### Direction
- Algocube digit **5** → stack↔cell fixed-width shift32 TOC (cell dual of SSHL32/SSHR32/SSAR32 after dual-stack DSHL32 plane).

### Language
- `SSHL32TOC`/`SHL32TOC` — stack `i k → cells[i] = (uint32)cells[i] ≪ k` (k≥32 → 0)
- `SSHR32TOC`/`SHR32TOC` — stack `i k → cells[i] = (uint32)cells[i] ≫ k` logical
- `SSAR32TOC`/`SAR32TOC` — stack `i k → cells[i] = arithmetic ≫ of low32` (sign-extend)
- Proof `426_sshl32toc_ssar32toc.cubalc`

### Prior
See 1.12.402-universal.

## 1.12.402-universal — 2026-08-04

### Direction
- Algocube digit **8** → stack↔cell endian/bitrev width ladder TOC (complete bswap 16/32/64 + bitrev 16/32 into cell).

### Language
- `SBSWAP16TOC`/`BSWAP16TOC` — stack `i → cells[i] = bswap16(low16 cells[i])`
- `SBSWAP64TOC`/`BSWAP64TOC` — stack `i → cells[i] = bswap64(cells[i])`
- `SBITREV16TOC`/`SREV16TOC` — stack `i → cells[i] = bitrev16(low16 cells[i])`
- Proof `425_sbswap16toc_sbitrev16toc.cubalc`

### Prior
See 1.12.401-universal.

## 1.12.401-universal — 2026-08-04

### Direction
- Algocube digit **9** → inverted cell logic (complete AND/OR/XOR cell plane with NAND/NOR/XNOR duals).

### Language
- `NANDCELL`/`CELLNAND` — `lo hi mask → cells[i] = ~(cells[i] & mask)` over range
- `NORCELL`/`CELLNOR` — `lo hi mask → cells[i] = ~(cells[i] | mask)` over range
- `XNORCELL`/`CELLXNOR` — `lo hi mask → cells[i] = ~(cells[i] ^ mask)` over range
- Proof `424_nandcell_xnorcell.cubalc`

### Prior
See 1.12.400-universal.

## 1.12.400-universal — 2026-08-04

### Direction
- Algocube digit **8** → stack↔cell word bit metrics TOC (dual of SPARITY/SBSWAP/SBITREV into cell after SPOPCNT/CLZ/CTZ TOC).

### Language
- `SPARITYTOC`/`PARITYTOC` — stack `i → cells[i] = parity(cells[i])` (xor of bits)
- `SBSWAPTOC`/`BSWAPTOC` — stack `i → cells[i] = bswap32(low32 cells[i])`
- `SBITREV32TOC`/`SREV32TOC` — stack `i → cells[i] = bitrev32(low32 cells[i])`
- Proof `423_sparitytoc_sbitrev32toc.cubalc`

### Prior
See 1.12.399-universal.

## 1.12.399-universal — 2026-08-04

### Direction
- Algocube digit **1** → stack word path 32 (arith SHR32 dual of SSHR32; rotate32 dual of SROTL8/16).

### Language
- `SSAR32`/`SASHR32` — stack `a k → arithmetic right shift low 32 of a by k`
- `SROTL32`/`SROL32` — stack `a k → rotate left within low 32 bits`
- `SROTR32`/`SROR32` — stack `a k → rotate right within low 32 bits`
- Proof `422_ssar32_srotr32.cubalc`

### Prior
See 1.12.398-universal.

## 1.12.398-universal — 2026-08-04

### Direction
- Algocube digit **5** → dual-stack fixed-width shift32 (complete dual-stack fixed shift 4/8/16/32 after DROL32/DROR32).

### Language
- `DSHL32`/`2SHL32` — dual-stack logical left within low 32 (a b c d → a≪c b≪d, masked)
- `DSHR32`/`2SHR32` — dual-stack logical right within low 32
- `DSAR32`/`2SAR32` — dual-stack arithmetic right (sign-extend bit31)
- Proof `421_dshl32_dsar32.cubalc`

### Prior
See 1.12.397-universal.

## 1.12.397-universal — 2026-08-04

### Direction
- Algocube digit **3** → dual-stack signed clip32 + fixed rotate32 (complete signed clip 4/8/16/32; fixed rotate 4/8/16/32).

### Language
- `DCLIPS32`/`2CLIPS32` — dual-stack clamp pair to signed 32-bit [INT32_MIN,INT32_MAX]
- `DROL32`/`2ROL32` — dual-stack rotate-left within low 32 bits (a b c d → rot(a,c) rot(b,d))
- `DROR32`/`2ROR32` — dual-stack rotate-right within low 32 bits
- Proof `420_dclips32_drol32.cubalc`

### Prior
See 1.12.396-universal.

## 1.12.396-universal — 2026-08-04

### Direction
- Algocube digit **3** → dual-stack signed clip (signed dual of DCLIP4/8/16; pair of SCLIPS4/8/16).

### Language
- `DCLIPS4`/`2CLIPS4` — dual-stack clamp pair to signed 4-bit [-8,7]
- `DCLIPS8`/`2CLIPS8` — dual-stack clamp pair to signed 8-bit [-128,127]
- `DCLIPS16`/`2CLIPS16` — dual-stack clamp pair to signed 16-bit [-32768,32767]
- Proof `419_dclips4_dclips16.cubalc`

### Prior
See 1.12.395-universal.

## 1.12.395-universal — 2026-08-04

### Direction
- Algocube digit **3** → stack clip complete 4/32 u+s (unsigned stack clip 4/8/16/32; signed 32 dual of SCLIPS4/8/16).

### Language
- `SCLIP4`/`CLIP4`/`UCLIP4` — stack `TOS = clamp to unsigned nibble [0,15]`
- `SCLIP32`/`CLIP32`/`UCLIP32` — stack `TOS = clamp to unsigned 32-bit [0,0xFFFFFFFF]`
- `SCLIPS32`/`CLIPS32` — stack `TOS = clamp to signed 32-bit [INT32_MIN,INT32_MAX]`
- Proof `418_sclip4_sclips32.cubalc`

### Prior
See 1.12.394-universal.

## 1.12.394-universal — 2026-08-04

### Direction
- Algocube digit **8** → stack signed clip word path (signed dual of SCLIP8/SCLIP16; complete stack clamp 4/8/16 plane).

### Language
- `SCLIPS4`/`CLIPS4` — stack `TOS = clamp to signed 4-bit [-8,7]`
- `SCLIPS8`/`CLIPS8` — stack `TOS = clamp to signed 8-bit [-128,127]`
- `SCLIPS16`/`CLIPS16` — stack `TOS = clamp to signed 16-bit [-32768,32767]`
- Proof `417_sclips4_sclips16.cubalc`

### Prior
See 1.12.393-universal.

## 1.12.393-universal — 2026-08-04

### Direction
- Algocube digit **4** → imm dual signed clip TOC (imm dual of SCLIPS4/16/32TOC; complete signed clip imm plane with SCLIPS8TOCN).

### Language
- `SCLIPS4TOCN`/`CLIPS4TOCN` — stack `i → cells[i] = clamp to signed 4-bit [-8,7]` (i replaced)
- `SCLIPS16TOCN`/`CLIPS16TOCN` — stack `i → cells[i] = clamp to signed 16-bit [-32768,32767]` (i replaced)
- `SCLIPS32TOCN`/`CLIPS32TOCN` — stack `i → cells[i] = clamp to signed 32-bit [INT32_MIN,INT32_MAX]` (i replaced)
- Proof `416_sclips4tocn_sclips32tocn.cubalc`

### Prior
See 1.12.392-universal.

## 1.12.392-universal — 2026-08-04

### Direction
- Algocube digit **7** → signed clip 4/32 + imm signed8 TOC (complete signed clip 4/8/16/32 plane; imm dual of SCLIPS8TOC).

### Language
- `SCLIPS4TOC`/`CLIPS4TOC` — stack `i → cells[i] = clamp to signed 4-bit [-8,7]`
- `SCLIPS32TOC`/`CLIPS32TOC` — stack `i → cells[i] = clamp to signed 32-bit [INT32_MIN,INT32_MAX]`
- `SCLIPS8TOCN`/`CLIPS8TOCN` — stack `i → cells[i] = clamp to signed 8-bit [-128,127]` (i replaced)
- Proof `415_sclips4toc_sclips8tocn.cubalc`

### Prior
See 1.12.391-universal.

## 1.12.391-universal — 2026-08-04

### Direction
- Algocube digit **6** → imm clip32 + signed clip TOC (complete uclip imm 4/8/16/32; signed 8/16 clamp into cell).

### Language
- `SCLIP32TOCN`/`CLIP32TOCN` — stack `i → cells[i] = clamp to [0,0xFFFFFFFF]`
- `SCLIPS8TOC`/`CLIPS8TOC` — stack `i → cells[i] = clamp to signed 8-bit [-128,127]`
- `SCLIPS16TOC`/`CLIPS16TOC` — stack `i → cells[i] = clamp to signed 16-bit [-32768,32767]`
- Proof `414_sclip32tocn_sclips16toc.cubalc`

### Prior
See 1.12.390-universal.

## 1.12.390-universal — 2026-08-04

### Direction
- Algocube digit **4** → imm dual fixed clip TOC (imm dual of SCLIP4/8/16TOC; keep i on stack).

### Language
- `SCLIP4TOCN`/`CLIP4TOCN` — stack `i → cells[i] = clamp to [0,15]`
- `SCLIP8TOCN`/`CLIP8TOCN` — stack `i → cells[i] = clamp to [0,255]`
- `SCLIP16TOCN`/`CLIP16TOCN` — stack `i → cells[i] = clamp to [0,65535]`
- Proof `413_sclip4tocn_sclip16tocn.cubalc`

### Prior
See 1.12.389-universal.

## 1.12.389-universal — 2026-08-04

### Direction
- Algocube digit **3** → stack dual fixed clip TOC (complete unsigned clip 4/8/16/32 plane after SCLIP8TOC).

### Language
- `SCLIP4TOC`/`CLIP4TOC` — stack `i → cells[i] = clamp to unsigned nibble [0,15]`
- `SCLIP16TOC`/`CLIP16TOC` — stack `i → cells[i] = clamp to unsigned 16-bit [0,65535]`
- `SCLIP32TOC`/`CLIP32TOC` — stack `i → cells[i] = clamp to unsigned 32-bit [0,0xFFFFFFFF]`
- Proof `412_sclip4toc_sclip32toc.cubalc`

### Prior
See 1.12.388-universal.

## 1.12.388-universal — 2026-08-04

### Direction
- Algocube digit **5** → imm dual 32-bit extend + clip8 TOC (complete zext/sext imm 4/8/16/32; CLIP8 into cell).

### Language
- `SZEXT32TOCN`/`ZEXT32TOCN` — stack `i → cells[i] = zero-extend low 32 bits of cells[i]`
- `SSEXT32TOCN`/`SEXT32TOCN` — stack `i → cells[i] = sign-extend low 32 bits of cells[i]`
- `SCLIP8TOC`/`CLIP8TOC` — stack `i → cells[i] = clamp to unsigned 8-bit [0,255]`
- Proof `411_szext32tocn_sclip8toc.cubalc`

### Prior
See 1.12.387-universal.

## 1.12.387-universal — 2026-08-04

### Direction
- Algocube digit **6** → stack/imm zext dual ladder (stack dual of SZEXT4TOCN; imm dual of SZEXT8/16TOC).

### Language
- `SZEXT4TOC`/`ZEXT4TOC` — stack `i → cells[i] = zero-extend low 4 bits of cells[i]`
- `SZEXT8TOCN`/`ZEXT8TOCN` — stack `i → cells[i] = zero-extend low 8 bits of cells[i]`
- `SZEXT16TOCN`/`ZEXT16TOCN` — stack `i → cells[i] = zero-extend low 16 bits of cells[i]`
- Proof `410_szext4toc_szext16tocn.cubalc`

### Prior
See 1.12.386-universal.

## 1.12.386-universal — 2026-08-04

### Direction
- Algocube digit **1** → imm dual fixed nibble/byte extend TOC (imm dual of SSEXT8TOC/SSEXT4TOC; nibble zext).

### Language
- `SSEXT8TOCN`/`SEXT8TOCN` — stack `i → cells[i] = sign-extend low 8 bits of cells[i]`
- `SSEXT4TOCN`/`SEXT4TOCN` — stack `i → cells[i] = sign-extend low 4 bits of cells[i]`
- `SZEXT4TOCN`/`ZEXT4TOCN` — stack `i → cells[i] = zero-extend low 4 bits of cells[i]`
- Proof `409_ssext8tocn_szext4tocn.cubalc`

### Prior
See 1.12.385-universal.

## 1.12.385-universal — 2026-08-04

### Direction
- Algocube digit **4** → stack dual fixed zext + nibble sext TOC (complete zext 8/16/32; close 4-bit extend).

### Language
- `SZEXT16TOC`/`ZEXT16TOC` — stack `i → cells[i] = zero-extend low 16 bits of cells[i]`
- `SZEXT32TOC`/`ZEXT32TOC` — stack `i → cells[i] = zero-extend low 32 bits of cells[i]`
- `SSEXT4TOC`/`SEXT4TOC` — stack `i → cells[i] = sign-extend low 4 bits of cells[i]`
- Proof `408_szext16toc_ssext4toc.cubalc`

### Prior
See 1.12.384-universal.

## 1.12.384-universal — 2026-08-04

### Direction
- Algocube digit **9** → stack dual fixed-width extend TOC (complete 8/16/32 fixed extend plane after SSEXT8TOC).

### Language
- `SSEXT16TOC`/`SEXT16TOC` — stack `i → cells[i] = sign-extend low 16 bits of cells[i]`
- `SSEXT32TOC`/`SEXT32TOC` — stack `i → cells[i] = sign-extend low 32 bits of cells[i]`
- `SZEXT8TOC`/`ZEXT8TOC` — stack `i → cells[i] = zero-extend low 8 bits of cells[i]`
- Proof `407_ssext16toc_szext8toc.cubalc`

### Prior
See 1.12.383-universal.

## 1.12.383-universal — 2026-08-04

### Direction
- Algocube digit **5** → imm dual sign/zero extend TOC (imm dual of SSEXTTOC/SZEXTTOC; fixed 16-bit path).

### Language
- `SSEXTTOCN`/`SEXTTOCN` w — stack `i → cells[i] = sign-extend low w bits of cells[i]` (w 0..63)
- `SZEXTTOCN`/`ZEXTTOCN` w — stack `i → cells[i] = zero-extend low w bits of cells[i]`
- `SSEXT16TOCN`/`SEXT16TOCN` — stack `i → cells[i] = sign-extend low 16 bits of cells[i]`
- Proof `406_ssexttocn_szexttocn.cubalc`

### Prior
See 1.12.382-universal.

## 1.12.382-universal — 2026-08-04

### Direction
- Algocube digit **8** → stack dual sign/zero extend TOC (stack dual of SSEXT/SZEXT/SSEXT8 into cell after field ladder).

### Language
- `SSEXTTOC`/`SEXTTOC` — stack `i w → cells[i] = sign-extend low w bits of cells[i]` (w 0..63)
- `SZEXTTOC`/`ZEXTTOC` — stack `i w → cells[i] = zero-extend low w bits of cells[i]`
- `SSEXT8TOC`/`SEXT8TOC` — stack `i → cells[i] = sign-extend low 8 bits of cells[i]`
- Proof `405_ssexttoc_szexttoc.cubalc`

### Prior
See 1.12.381-universal.

## 1.12.381-universal — 2026-08-04

### Direction
- Algocube digit **0** → stack dual LE nibble field TOC (stack dual of SNIBTOCN; foundation completes 4/8/16/32 stack ladder).

### Language
- `SNIBTOC`/`NIBTOC` — stack `i n → cells[i] = LE nibble n of cells[i]` (n 0..15)
- `SSETNIBTOC`/`SETNIBTOC` — stack `i field n → deposit low 4 bits of field into LE nibble n`
- `SCLRNIBTOC`/`CLRNIBTOC` — stack `i n → clear LE nibble n of cells[i]`
- Proof `404_snibtoc_sclrnibtoc.cubalc`

### Prior
See 1.12.380-universal.

## 1.12.380-universal — 2026-08-04

### Direction
- Algocube digit **0** → stack dual LE 32-bit field TOC (stack dual of SGET32TOCN; foundation completes 4/8/16/32 ladder).

### Language
- `SGET32TOC`/`GET32TOC` — stack `i n → cells[i] = LE 32-bit word n of cells[i]` (n 0..1)
- `SSET32TOC`/`SET32TOC` — stack `i field n → deposit low 32 bits of field into LE word n`
- `SCLR32TOC`/`CLR32TOC` — stack `i n → clear LE 32-bit word n of cells[i]`
- Proof `403_sget32toc_sclr32toc.cubalc`

### Prior
See 1.12.379-universal.

## 1.12.379-universal — 2026-08-04

### Direction
- Algocube digit **7** → stack dual LE halfword field TOC (stack dual of SWORDTOCN plane after SBYTETOC).

### Language
- `SWORDTOC`/`WORDTOC` — stack `i n → cells[i] = LE halfword n of cells[i]` (n 0..3)
- `SSET16TOC`/`SET16TOC` — stack `i field n → deposit low 16 bits of field into LE halfword n`
- `SCLR16TOC`/`CLR16TOC` — stack `i n → clear LE halfword n of cells[i]`
- Proof `402_swordtoc_sclr16toc.cubalc`

### Prior
See 1.12.378-universal.

## 1.12.378-universal — 2026-08-04

### Direction
- Algocube digit **7** → stack dual LE byte-field TOC (stack dual of SBYTETOCN plane after bitfield TOC).

### Language
- `SBYTETOC`/`BYTETOC` — stack `i n → cells[i] = LE byte n of cells[i]` (n 0..7)
- `SSETBYTETOC`/`SETBYTETOC` — stack `i field n → deposit low 8 bits of field into LE byte n`
- `SCLRBYTETOC`/`CLRBYTETOC` — stack `i n → clear LE byte n of cells[i]`
- Proof `401_sbytetoc_sclrbytetoc.cubalc`

### Prior
See 1.12.377-universal.

## 1.12.377-universal — 2026-08-04

### Direction
- Algocube digit **3** → stack dual high-n reverse/rotate TOC (stack dual of SBREVHNTOCN plane; high dual of SBREVTOC).

### Language
- `SBREVHNTOC`/`SREVHIGHTOC` — stack `i n → reverse high n bits of cells[i]` (low kept; n 0..64)
- `SROLHNTOC`/`SHIGHROLTOC` — stack `i n → rotl-1 within high n bits of cells[i]`
- `SRORHNTOC`/`SHIGHRORTOC` — stack `i n → rotr-1 within high n bits of cells[i]`
- Proof `400_sbrevhntoc_srorhntoc.cubalc`

### Prior
See 1.12.376-universal.

## 1.12.376-universal — 2026-08-04

### Direction
- Algocube digit **8** → stack dual low-n reverse/rotate TOC (stack dual of SBREVTOCN plane after SCLRMNTOC).

### Language
- `SBREVTOC`/`BREVTOC` — stack `i n → reverse low n bits of cells[i]` (high kept; n 0..64)
- `SROLBTOC`/`ROLBTOC` — stack `i n → rotl-1 within low n bits of cells[i]`
- `SRORBTOC`/`RORBTOC` — stack `i n → rotr-1 within low n bits of cells[i]`
- Proof `399_sbrevtoc_srorbtoc.cubalc`

### Prior
See 1.12.375-universal.

## 1.12.375-universal — 2026-08-04

### Direction
- Algocube digit **1** → stack dual clear + high-mask TOC (stack dual of SCLRMNTOCN plane after SNANDHNTOC).

### Language
- `SCLRMNTOC`/`CLRMNTOC` — stack `i n → cells[i] &= ~low-n mask`; clear low n bits (n 0..64)
- `SCLRHNTOC`/`CLRHNTOC` — stack `i n → cells[i] &= ~high-n mask`; clear high n bits
- `SHMASKTOC`/`HMASKTOC` — stack `i n → cells[i] = high-n-bit mask`
- Proof `398_sclrmntoc_shmasktoc.cubalc`

### Prior
See 1.12.374-universal.

## 1.12.374-universal — 2026-08-04

### Direction
- Algocube digit **7** → stack dual inverted high-n mask TOC (stack dual of SNANDHNTOCN plane after SANDHNTOC).

### Language
- `SNANDHNTOC`/`NANDHNTOC` — stack `i n → cells[i] = ~(cells[i] & high-n mask)` (n 0..64)
- `SNORHNTOC`/`NORHNTOC` — stack `i n → cells[i] = ~(cells[i] | high-n mask)`
- `SXNORHNTOC`/`XNORHNTOC` — stack `i n → cells[i] = ~(cells[i] ^ high-n mask)`
- Proof `397_snandhntoc_sxnorhntoc.cubalc`

### Prior
See 1.12.373-universal.

## 1.12.373-universal — 2026-08-04

### Direction
- Algocube digit **6** → stack dual high-n mask TOC (stack dual of SANDHNTOCN plane; high dual of SANDMNTOC).

### Language
- `SANDHNTOC`/`ANDHNTOC` — stack `i n → cells[i] &= high-n mask`; keep high n bits (n 0..64)
- `SORHNTOC`/`ORHNTOC` — stack `i n → cells[i] |= high-n mask`; set high n bits
- `SXORHNTOC`/`XORHNTOC` — stack `i n → cells[i] ^= high-n mask`; toggle high n bits
- Proof `396_sandhntoc_sxorhntoc.cubalc`

### Prior
See 1.12.372-universal.

## 1.12.372-universal — 2026-08-04

### Direction
- Algocube digit **9** → stack dual inverted low-n mask TOC (stack dual of SNANDMNTOCN plane after SANDMNTOC).

### Language
- `SNANDMNTOC`/`NANDMNTOC` — stack `i n → cells[i] = ~(cells[i] & low-n mask)` (n 0..64)
- `SNORMNTOC`/`NORMNTOC` — stack `i n → cells[i] = ~(cells[i] | low-n mask)`
- `SXNORMNTOC`/`XNORMNTOC` — stack `i n → cells[i] = ~(cells[i] ^ low-n mask)`
- Proof `395_snandmntoc_sxnormntoc.cubalc`

### Prior
See 1.12.371-universal.

## 1.12.371-universal — 2026-08-04

### Direction
- Algocube digit **5** → stack dual low-n mask TOC (stack dual of SANDMNTOCN plane; n from stack).

### Language
- `SANDMNTOC`/`ANDMNTOC` — stack `i n → cells[i] &= low-n mask`; keep low n bits (n 0..64)
- `SORMNTOC`/`ORMNTOC` — stack `i n → cells[i] |= low-n mask`; set low n bits
- `SXORMNTOC`/`XORMNTOC` — stack `i n → cells[i] ^= low-n mask`; toggle low n bits
- Proof `394_sandmntoc_sxormntoc.cubalc`

### Prior
See 1.12.370-universal.

## 1.12.370-universal — 2026-08-04

### Direction
- Algocube digit **4** → stack dual high-n metric TOC (stack dual of SPOPHNTOCN plane; high dual of SPOPMNTOC).

### Language
- `SPOPHNTOC`/`POPHNTOC` — stack `i n → cells[i]=popcount(cells[i]&high-n mask)` (n 0..64)
- `SANYHNTOC`/`ANYHNTOC` — stack `i n → cells[i]=1 if any high-n bit set else 0`
- `SALLHNTOC`/`ALLHNTOC` — stack `i n → cells[i]=1 if all high-n bits set else 0` (n=0 vacuous 1)
- Proof `393_spophntoc_sallhntoc.cubalc`

### Prior
See 1.12.369-universal.

## 1.12.369-universal — 2026-08-04

### Direction
- Algocube digit **9** → stack dual low-n metric TOC (stack dual of SPOPMNTOCN plane; n from stack).

### Language
- `SPOPMNTOC`/`POPMNTOC` — stack `i n → cells[i]=popcount(cells[i]&low-n mask)` (n 0..64)
- `SANYMNTOC`/`ANYMNTOC` — stack `i n → cells[i]=1 if any low-n bit set else 0`
- `SALLMNTOC`/`ALLMNTOC` — stack `i n → cells[i]=1 if all low-n bits set else 0` (n=0 vacuous 1)
- Proof `392_spopmntoc_sallmntoc.cubalc`

### Prior
See 1.12.368-universal.

## 1.12.368-universal — 2026-08-04

### Direction
- Algocube digit **5** → imm high-n metric TOC (imm dual of SPOPHN/SANYHN/SALLHN into cell; high dual of SPOPMNTOCN).

### Language
- `SPOPHNTOCN`/`POPHNTOCN` n — stack `i → cells[i]=popcount(cells[i]&high-n mask)` (n 0..64)
- `SANYHNTOCN`/`ANYHNTOCN` n — stack `i → cells[i]=1 if any high-n bit set else 0`
- `SALLHNTOCN`/`ALLHNTOCN` n — stack `i → cells[i]=1 if all high-n bits set else 0` (n=0 vacuous 1)
- Proof `391_spophntocn_sallhntocn.cubalc`

### Prior
See 1.12.367-universal.

## 1.12.367-universal — 2026-08-04

### Direction
- Algocube digit **3** → imm high-n reverse/rotate TOC (imm dual of SBREVHN/SROLHN/SRORHN into cell after SBREVTOCN).

### Language
- `SBREVHNTOCN`/`BREVHNTOCN` n — stack `i → reverse high n bits of cells[i]`; low kept (n 0..64)
- `SROLHNTOCN`/`ROLHNTOCN` n — stack `i → rotl-1 within high n bits of cells[i]`; low kept
- `SRORHNTOCN`/`RORHNTOCN` n — stack `i → rotr-1 within high n bits of cells[i]`; low kept
- Proof `390_sbrevhntocn_srorhntocn.cubalc`

### Prior
See 1.12.366-universal.

## 1.12.366-universal — 2026-08-04

### Direction
- Algocube digit **3** → imm low-n reverse/rotate TOC (imm dual of SBREVN/SROLBN/SRORBN into cell after SCLRMNTOCN).

### Language
- `SBREVTOCN`/`BREVTOCN` n — stack `i → reverse low n bits of cells[i]`; high kept (n 0..64)
- `SROLBTOCN`/`ROLBTOCN` n — stack `i → rotl-1 within low n bits of cells[i]`; high kept
- `SRORBTOCN`/`RORBTOCN` n — stack `i → rotr-1 within low n bits of cells[i]`; high kept
- Proof `389_sbrevtocn_srorbtocn.cubalc`

### Prior
See 1.12.365-universal.

## 1.12.365-universal — 2026-08-04

### Direction
- Algocube digit **8** → imm clear + high-mask TOC (imm dual of SCLRMN/SCLRHN/SHMASKN into cell after SANDMNTOCN plane).

### Language
- `SCLRMNTOCN`/`CLRMNTOCN` n — stack `i → cells[i] &= ~low-n mask`; clear low n bits (n 0..64)
- `SCLRHNTOCN`/`CLRHNTOCN` n — stack `i → cells[i] &= ~high-n mask`; clear high n bits
- `SHMASKTOCN`/`HMASKTOCN` n — stack `i → cells[i] = high-n mask`; leave result
- Proof `388_sclrmntocn_shmasktocn.cubalc`

### Prior
See 1.12.364-universal.

## 1.12.364-universal — 2026-08-04

### Direction
- Algocube digit **3** → imm inverted high-n mask TOC (imm dual of SNANDHN/SNORHN/SXNORHN into cell after SANDHNTOCN).

### Language
- `SNANDHNTOCN`/`NANDHNTOCN` n — stack `i → cells[i]=~(cells[i]&high-n mask)`; leave result (n 0..64)
- `SNORHNTOCN`/`NORHNTOCN` n — stack `i → cells[i]=~(cells[i]|high-n mask)`; leave result
- `SXNORHNTOCN`/`XNORHNTOCN` n — stack `i → cells[i]=~(cells[i]^high-n mask)`; leave result
- Proof `387_snandhntocn_sxnorhntocn.cubalc`

### Prior
See 1.12.363-universal.

## 1.12.363-universal — 2026-08-04

### Direction
- Algocube digit **4** → imm high-n mask TOC (imm dual of SANDHN/SORHN/SXORHN into cell; high dual of SANDMNTOCN).

### Language
- `SANDHNTOCN`/`ANDHNTOCN` n — stack `i → cells[i] &= high-n mask`; keep high n bits (n 0..64)
- `SORHNTOCN`/`ORHNTOCN` n — stack `i → cells[i] |= high-n mask`; set high n bits
- `SXORHNTOCN`/`XORHNTOCN` n — stack `i → cells[i] ^= high-n mask`; toggle high n bits
- Proof `386_sandhntocn_sxorhntocn.cubalc`

### Prior
See 1.12.362-universal.

## 1.12.362-universal — 2026-08-04

### Direction
- Algocube digit **3** → imm inverted low-n mask TOC (imm dual of SNANDMN/SNORMN/SXNORMN into cell after SANDMNTOCN).

### Language
- `SNANDMNTOCN`/`NANDMNTOCN` n — stack `i → cells[i]=~(cells[i]&low-n mask)`; leave result (n 0..64)
- `SNORMNTOCN`/`NORMNTOCN` n — stack `i → cells[i]=~(cells[i]|low-n mask)`; leave result
- `SXNORMNTOCN`/`XNORMNTOCN` n — stack `i → cells[i]=~(cells[i]^low-n mask)`; leave result
- Proof `385_snandmntocn_sxnormntocn.cubalc`

### Prior
See 1.12.361-universal.

## 1.12.361-universal — 2026-08-04

### Direction
- Algocube digit **9** → imm low-n metric TOC (imm dual of SPOPMN/SANYMN/SALLMN into cell after SANDMNTOCN).

### Language
- `SPOPMNTOCN`/`POPMNTOCN` n — stack `i → cells[i]=popcount(cells[i]&low-n mask)` (n 0..64)
- `SANYMNTOCN`/`ANYMNTOCN` n — stack `i → cells[i]=1 if any low-n bit set else 0`
- `SALLMNTOCN`/`ALLMNTOCN` n — stack `i → cells[i]=1 if all low-n bits set else 0` (n=0 vacuous 1)
- Proof `384_spopmntocn_sallmntocn.cubalc`

### Prior
See 1.12.360-universal.

## 1.12.360-universal — 2026-08-04

### Direction
- Algocube digit **8** → imm low-n mask TOC (imm dual of SANDMN/SORMN/SXORMN into cell; data-path bit fill).

### Language
- `SANDMNTOCN`/`ANDMNTOCN` n — stack `i → cells[i] &= low-n mask`; keep low n bits (n 0..64)
- `SORMNTOCN`/`ORMNTOCN` n — stack `i → cells[i] |= low-n mask`; set low n bits
- `SXORMNTOCN`/`XORMNTOCN` n — stack `i → cells[i] ^= low-n mask`; toggle low n bits
- Proof `383_sandmntocn_sxormntocn.cubalc`

### Prior
See 1.12.359-universal.

## 1.12.359-universal — 2026-08-04

### Direction
- Algocube digit **7** → imm 32-bit field TOC (imm dual of SGET32N/SSET32N/SCLR32N into cell; complete 4/8/16/32 ladder).

### Language
- `SGET32TOCN`/`GET32TOCN` n — stack `i → cells[i]=LE 32-bit word n`; leave result (n 0..1)
- `SSET32TOCN`/`SET32TOCN` field n — deposit low 32 bits of field into LE word n of cells[i]
- `SCLR32TOCN`/`CLR32TOCN` n — clear LE 32-bit word n of cells[i]; leave result
- Proof `382_sget32tocn_sclr32tocn.cubalc`

### Prior
See 1.12.358-universal.

## 1.12.358-universal — 2026-08-04

### Direction
- Algocube digit **4** → imm nibble field TOC (imm dual of SNIBN/SSETNIBN/SCLRNIBN into cell; complete 4/8/16 field ladder).

### Language
- `SNIBTOCN`/`NIBTOCN` n — stack `i → cells[i]=LE nibble n`; leave result (n 0..15)
- `SSETNIBTOCN`/`SETNIBTOCN` field n — deposit low 4 bits of field into LE nibble n of cells[i]
- `SCLRNIBTOCN`/`CLRNIBTOCN` n — clear LE nibble n of cells[i]; leave result
- Proof `381_snibtocn_sclrnibtocn.cubalc`

### Prior
See 1.12.357-universal.

## 1.12.357-universal — 2026-08-04

### Direction
- Algocube digit **2** → imm ceil/floor div TOC (imm dual of SDIVCEILN/SDIVFLOORN into cell after SDIVTOCN).

### Language
- `SDIVCEILTOCN`/`CEILDIVTOCN` n — stack `i → cells[i]=ceil(cells[i]/n)`; n==0→0 soft; leave result
- `SDIVFLOORTOCN`/`FLOORDIVTOCN` n — stack `i → cells[i]=floor(cells[i]/n)`; n==0→0 soft; leave result
- Proof `380_sdivceiltocn_sdivfloortocn.cubalc`

### Prior
See 1.12.356-universal.

## 1.12.356-universal — 2026-08-04

### Direction
- Algocube digit **9** → imm halfword field TOC (imm dual of SWORDN/SSET16N/SCLR16N into cell after byte TOC).

### Language
- `SWORDTOCN`/`WORDTOCN` n — stack `i → cells[i]=LE halfword n`; leave result (n 0..3)
- `SSET16TOCN`/`SET16TOCN` field n — deposit low 16 bits of field into LE halfword n of cells[i]
- `SCLR16TOCN`/`CLR16TOCN` n — clear LE halfword n of cells[i]; leave result
- Proof `379_swordtocn_sclr16tocn.cubalc`

### Prior
See 1.12.355-universal.

## 1.12.355-universal — 2026-08-04

### Direction
- Algocube digit **1** → imm byte field TOC (imm dual of SBYTEN/SSETBYTEN/SCLRBYTEN into cell).

### Language
- `SBYTETOCN`/`BYTETOCN` n — stack `i → cells[i]=LE byte n`; leave result (n 0..7)
- `SSETBYTETOCN`/`SETBYTETOCN` field n — deposit low 8 bits of field into LE byte n of cells[i]
- `SCLRBYTETOCN`/`CLRBYTETOCN` n — clear LE byte n of cells[i]; leave result
- Proof `378_sbytetocn_sclrbytetocn.cubalc`

### Prior
See 1.12.354-universal.

## 1.12.354-universal — 2026-08-04

### Direction
- Algocube digit **7** → stack bitfield TOC (stack dual of SSETBTOCN plane; i+bit into cell).

### Language
- `SSETBTOC`/`SETBTOC` — stack `i bit → cells[i] |= (1<<bit)`; leave result (bit 0..63)
- `SCLRBTOC`/`CLRBTOC` — stack `i bit → cells[i] &= ~(1<<bit)`; leave result
- `SFLPBTOC`/`FLPBTOC` — stack `i bit → cells[i] ^= (1<<bit)`; leave result
- Proof `377_ssetbtoc_sflpbtoc.cubalc`

### Prior
See 1.12.353-universal.

## 1.12.353-universal — 2026-08-04

### Direction
- Algocube digit **2** → imm numthy TOC (imm dual of SGCDN/SLCMN into cell after SSQRTOC).

### Language
- `SGCDTOCN`/`GCDTOCN` n — stack `i → cells[i]=gcd(|cells[i]|,|n|)`; leave result
- `SLCMTOCN`/`LCMTOCN` n — stack `i → cells[i]=lcm(|cells[i]|,|n|)` (0 if either 0); leave result
- Proof `376_sgcdtocn_slcmtocn.cubalc`

### Prior
See 1.12.352-universal.

## 1.12.352-universal — 2026-08-04

### Direction
- Algocube digit **5** → bitfield imm TOC (imm dual of SSETBN/SCLRBN/SFLIPBN into cell after SANDTOCN).

### Language
- `SSETBTOCN`/`SETBTOCN` n — stack `i → cells[i] |= (1<<n)`; leave result (n 0..63)
- `SCLRBTOCN`/`CLRBTOCN` n — stack `i → cells[i] &= ~(1<<n)`; leave result
- `SFLPBTOCN`/`FLPBTOCN` n — stack `i → cells[i] ^= (1<<n)`; leave result
- Proof `375_ssetbtocn_sflpbtocn.cubalc`

### Prior
See 1.12.351-universal.

## 1.12.351-universal — 2026-08-04

### Direction
- Algocube digit **4** → multiword carry/borrow imm TOC (imm dual of SADDCN/SSUBBN into cell after SADDTOCN).

### Language
- `SADDCTOCN`/`ADDCTOCN` n — stack `i → cells[i]+=n+cin(CARRY)`; update CARRY/CY leave sum
- `SSUBBTOCN`/`SUBBTOCN` n — stack `i → cells[i]-=n+bin(BORROW|CARRY)`; update BORROW/CARRY leave diff
- Proof `374_saddctocn_ssubbtocn.cubalc`

### Prior
See 1.12.350-universal.

## 1.12.350-universal — 2026-08-04

### Direction
- Algocube digit **2** → reverse unsigned stack↔cell TOC (stack dual of SUDIVFROMTOCN; reverse of SUDIVTOC).

### Language
- `SUDIVFROMTOC`/`RUDIVTOC` — stack `i v → cells[i]=(u)v/(u)cells[i]` leave quotient
- `SUMODFROMTOC`/`RUMODTOC` — stack `i v → cells[i]=(u)v%(u)cells[i]` leave remainder
- Proof `373_sudivfromtoc_sumodfromtoc.cubalc`

### Prior
See 1.12.349-universal.

## 1.12.349-universal — 2026-08-04

### Direction
- Algocube digit **9** → reverse imm sat TOC (complete SSATADDTOCN with /; reverse dual of SSATSUBFROMTOC).

### Language
- `SSATDIVTOCN`/`SATDIVTOCN` n — stack `i → cells[i]=sat(cells[i]/n)` leave result
- `SSATSUBFROMTOCN`/`RSATSUBTOCN` n — stack `i → cells[i]=sat(n-cells[i])` leave result
- `SSATDIVFROMTOCN`/`RSATDIVTOCN` n — stack `i → cells[i]=sat(n/cells[i])` leave result
- Proof `372_ssatdivtocn_ssatdivfromtocn.cubalc`

### Prior
See 1.12.348-universal.

## 1.12.348-universal — 2026-08-04

### Direction
- Algocube digit **3** → imm sat TOC (imm dual of SSATADDTOC/SCLAMPTOC after SSATADDN plane).

### Language
- `SSATADDTOCN`/`SATADDTOCN` n — stack `i → cells[i]=sat(cells[i]+n)` leave result
- `SSATSUBTOCN`/`SATSUBTOCN` n — stack `i → cells[i]=sat(cells[i]-n)` leave result
- `SSATMULTOCN`/`SATMULTOCN` n — stack `i → cells[i]=sat(cells[i]*n)` leave result
- `SCLAMPTOCN`/`CLAMPTOCN` lo hi — stack `i → cells[i]=clamp(cells[i],lo,hi)` leave result
- Proof `371_ssataddtocn_sclamptocn.cubalc`

### Prior
See 1.12.347-universal.

## 1.12.347-universal — 2026-08-04

### Direction
- Algocube digit **4** → stack↔cell modular accumulate TOC (stack dual of SADDMODTOCN plane).

### Language
- `SADDMODTOC`/`ADDMODTOC` — stack `i k m → cells[i]=(cells[i]+k) mod m` leave result
- `SSUBMODTOC`/`SUBMODTOC` — stack `i k m → cells[i]=(cells[i]-k) mod m` leave result
- `SMULMODTOC`/`MULMODTOC` — stack `i k m → cells[i]=(cells[i]*k) mod m` leave result
- Proof `370_saddmodtoc_smulmodtoc.cubalc`

### Prior
See 1.12.346-universal.

## 1.12.346-universal — 2026-08-04

### Direction
- Algocube digit **9** → reverse modular stack↔cell TOC (stack dual of SSUBMODFROMTOCN plane).

### Language
- `SSUBMODFROMTOC`/`RSUBMODTOC` — stack `i k m → cells[i]=(k-cells[i]) mod m` leave result
- `SPOWMODFROMTOC`/`RPOWMODTOC` — stack `i base m → cells[i]=base^cells[i] mod m` leave result
- `SMODDIVFROMTOC`/`RMODDIVTOC` — stack `i a m → cells[i]=a·cells[i]⁻¹ mod m` leave result
- Proof `369_ssubmodfromtoc_smoddivfromtoc.cubalc`

### Prior
See 1.12.345-universal.

## 1.12.345-universal — 2026-08-04

### Direction
- Algocube digit **0** → foundation stack↔cell unsigned TOC (stack dual of SUDIVTOCN plane).

### Language
- `SUDIVTOC`/`UDIVTOC` — stack `i v → cells[i]=(u)cells[i]/(u)v` leave quotient
- `SUMODTOC`/`UMODTOC` — stack `i v → cells[i]=(u)cells[i]%(u)v` leave remainder
- `SUMINTOC`/`UMINTOC` — stack `i v → cells[i]=umin(cells[i],v)` leave result
- `SUMAXTOC`/`UMAXTOC` — stack `i v → cells[i]=umax(cells[i],v)` leave result
- Proof `368_sudivtoc_sumaxtoc.cubalc`

### Prior
See 1.12.344-universal.

## 1.12.344-universal — 2026-08-04

### Direction
- Algocube digit **2** → stack↔cell modular inv/pow/div TOC (stack dual of SPOWMODTOCN plane).

### Language
- `SPOWMODTOC`/`POWMODTOC` — stack `i exp m → cells[i]=cells[i]^exp mod m` leave result
- `SMODDIVTOC`/`MODDIVTOC` — stack `i b m → cells[i]=cells[i]·b⁻¹ mod m` leave result
- `SMODINVTOC`/`INVMODTOC` — stack `i m → cells[i]=cells[i]⁻¹ mod m` leave result
- Proof `367_spowmodtoc_smodinvtoc.cubalc`

### Prior
See 1.12.343-universal.

## 1.12.343-universal — 2026-08-04

### Direction
- Algocube digit **0** → foundation forward unsigned imm TOC (unsigned peer of SDIVTOCN plane after SUDIVN/SUMINN).

### Language
- `SUDIVTOCN`/`UDIVTOCN`/`SUDIVATN` n — stack `i → cells[i]=(u)cells[i]/(u)n` leave quotient
- `SUMODTOCN`/`UMODTOCN`/`SUMODATN` n — stack `i → cells[i]=(u)cells[i]%(u)n` leave remainder
- `SUMINTOCN`/`UMINTOCN`/`SUMINATN` n — stack `i → cells[i]=umin(cells[i],n)` leave result
- `SUMAXTOCN`/`UMAXTOCN`/`SUMAXATN` n — stack `i → cells[i]=umax(cells[i],n)` leave result
- Proof `366_sudivtocn_sumaxtocn.cubalc`

### Prior
See 1.12.342-universal.

## 1.12.342-universal — 2026-08-04

### Direction
- Algocube digit **2** → forward imm modular inv/pow/div TOC (imm dual of SPOWMODN after SADDMODTOCN plane).

### Language
- `SPOWMODTOCN`/`POWMODTOCN`/`SPOWMODATN` exp m — stack `i → cells[i]=cells[i]^exp mod m` leave result
- `SMODDIVTOCN`/`MODDIVTOCN`/`SMODDIVATN` b m — stack `i → cells[i]=cells[i]·b⁻¹ mod m` leave result
- `SMODINVTOCN`/`MODINVTOCN`/`SINVMODTOCN` m — stack `i → cells[i]=cells[i]⁻¹ mod m` leave result
- Proof `365_spowmodtocn_smodinvtocn.cubalc`

### Prior
See 1.12.341-universal.

## 1.12.341-universal — 2026-08-04

### Direction
- Algocube digit **3** → forward imm modular TOC (imm dual of SADDMODN after reverse modular TOC plane).

### Language
- `SADDMODTOCN`/`ADDMODTOCN`/`SADDMODATN` k m — stack `i → cells[i]=(cells[i]+k) mod m` leave result
- `SSUBMODTOCN`/`SUBMODTOCN`/`SSUBMODATN` k m — stack `i → cells[i]=(cells[i]-k) mod m` leave result
- `SMULMODTOCN`/`MULMODTOCN`/`SMULMODATN` k m — stack `i → cells[i]=(cells[i]*k) mod m` leave result
- Proof `364_saddmodtocn_smulmodtocn.cubalc`

### Prior
See 1.12.340-universal.

## 1.12.340-universal — 2026-08-04

### Direction
- Algocube digit **3** → reverse imm modular TOC (imm dual of SSUBMODFROMN after SMODFROMTOCN plane).

### Language
- `SSUBMODFROMTOCN`/`RSUBMODTOCN` k m — stack `i → cells[i]=(k-cells[i]) mod m` leave result
- `SPOWMODFROMTOCN`/`RPOWMODTOCN` base m — stack `i → cells[i]=base^cells[i] mod m` leave result
- `SMODDIVFROMTOCN`/`RMODDIVTOCN` a m — stack `i → cells[i]=a·cells[i]⁻¹ mod m` leave result
- Proof `363_ssubmodfromtocn_smoddivfromtocn.cubalc`

### Prior
See 1.12.339-universal.

## 1.12.339-universal — 2026-08-04

### Direction
- Algocube digit **6** → dual-stack reverse imm modular (dual of SSUBMODFROMN plane; energy-math reverse).

### Language
- `DSUBMODFROMN`/`DRSUBMODN` k m — `a b → (k-a)%m (k-b)%m`
- `DPOWMODFROMN`/`DRPOWMODN` base m — `a b → base^a mod m , base^b mod m`
- `DMODDIVFROMN`/`DRMODDIVN` c m — `a b → c·a⁻¹ mod m , c·b⁻¹ mod m`
- Proof `362_dsubmodfromn_dmoddivfromn.cubalc`

### Prior
See 1.12.338-universal.

## 1.12.338-universal — 2026-08-04

### Direction
- Algocube digit **2** → reverse imm modular plane (reverse dual of SSUBMODN/SPOWMODN/SMODDIVN).

### Language
- `SSUBMODFROMN`/`RSUBMODN` k m — `TOS = (k - TOS) mod m` (m≤0 → 0)
- `SPOWMODFROMN`/`RPOWMODN` base m — `TOS = base^TOS mod m` (m≤0 or exp<0 → 0)
- `SMODDIVFROMN`/`RMODDIVN` a m — `TOS = a * TOS^{-1} mod m` (0 if none)
- Proof `361_ssubmodfromn_smoddivfromn.cubalc`

### Prior
See 1.12.337-universal.

## 1.12.337-universal — 2026-08-04

### Direction
- Algocube digit **7** → reverse imm inverted ANDN TOC (imm dual of SNANDNFROMN after SANDNFROMTOCN).

### Language
- `SNANDNFROMTOCN`/`NANDNFROMTOCN`/`SNANDNFROMATN` n — stack `i → cells[i]=~(n&~cells[i])` leave result
- `SNORNFROMTOCN`/`NORNFROMTOCN`/`SNORNFROMATN` n — stack `i → cells[i]=~(n|~cells[i])` leave result
- `SXNORNFROMTOCN`/`XNORNFROMTOCN`/`SXNORNFROMATN` n — stack `i → cells[i]=~(n^~cells[i])` leave result
- Proof `360_snandnfromtocn_sxnornfromtocn.cubalc`

### Prior
See 1.12.336-universal.

## 1.12.336-universal — 2026-08-04

### Direction
- Algocube digit **8** → dual-stack reverse imm inverted ANDN (dual of SNANDNFROMN after DANDNFROMN).

### Language
- `DNANDNFROMN`/`PAIRNANDNFROMN`/`DRNANDNFROMN` n — `a b → ~(n&~a) ~(n&~b)`
- `DNORNFROMN`/`PAIRNORNFROMN`/`DRNORNFROMN` n — `a b → ~(n|~a) ~(n|~b)`
- `DXNORNFROMN`/`PAIRXNORNFROMN`/`DRXNORNFROMN` n — `a b → ~(n^~a) ~(n^~b)` (equiv `n^a` / `n^b`)
- Proof `359_dnandnfromn_dxnornfromn.cubalc`

### Prior
See 1.12.335-universal.

## 1.12.335-universal — 2026-08-04

### Direction
- Algocube digit **3** → reverse imm inverted ANDN stack (reverse dual of SNANDNI after SANDNFROMN).

### Language
- `SNANDNFROMN`/`NANDNFROMN`/`RNANDNFROMN` n — `TOS = ~(n & ~TOS)` (= `~n | TOS`)
- `SNORNFROMN`/`NORNFROMN`/`RNORNFROMN` n — `TOS = ~(n | ~TOS)` (= `~n & TOS`)
- `SXNORNFROMN`/`XNORNFROMN`/`RXNORNFROMN` n — `TOS = ~(n ^ ~TOS)` (equiv `n ^ TOS`)
- Proof `358_snandnfromn_sxnornfromn.cubalc`

### Prior
See 1.12.334-universal.

## 1.12.334-universal — 2026-08-04

### Direction
- Algocube digit **5** → dual-stack reverse imm ANDN (dual of SANDNFROMN after DANDNI plane).

### Language
- `DANDNFROMN`/`DBICFROMN`/`PAIRANDNFROMN` n — stack `a b → (n&~a) (n&~b)`
- `DORNFROMN`/`DRORNFROMN`/`PAIRORNFROMN` n — stack `a b → (n|~a) (n|~b)`
- `DXORNFROMN`/`DRXORNFROMN`/`PAIRXORNFROMN` n — stack `a b → (n^~a) (n^~b)`
- Proof `357_dandnfromn_dxornfromn.cubalc`

### Prior
See 1.12.333-universal.

## 1.12.333-universal — 2026-08-04

### Direction
- Algocube digit **7** → reverse imm ANDN stack plane (reverse dual of SANDNI after SANDNFROMTOCN / SUDIVFROMN).

### Language
- `SANDNFROMN`/`BICFROMN`/`RANDNFROMN` n — `TOS = n & ~TOS`
- `SORNFROMN`/`ORNFROMN`/`RORNFROMN` n — `TOS = n | ~TOS`
- `SXORNFROMN`/`XORNFROMN`/`RXORNFROMN` n — `TOS = n ^ ~TOS`
- Proof `356_sandnfromn_sxornfromn.cubalc`

### Prior
See 1.12.332-universal.

## 1.12.332-universal — 2026-08-04

### Direction
- Algocube digit **6** → reverse imm ANDN-plane TOC (energy dual reverse of SANDNTOCN after SUDIVFROMTOCN).

### Language
- `SANDNFROMTOCN`/`BICFROMTOCN`/`SANDNFROMATN` n — stack `i → cells[i]=n&~cells[i]` leave result
- `SORNFROMTOCN`/`ORNFROMTOCN`/`SORNFROMATN` n — stack `i → cells[i]=n|~cells[i]` leave result
- `SXORNFROMTOCN`/`XORNFROMTOCN`/`SXORNFROMATN` n — stack `i → cells[i]=n^~cells[i]` leave result
- Proof `355_sandnfromtocn_sxornfromtocn.cubalc`

### Prior
See 1.12.331-universal.

## 1.12.331-universal — 2026-08-04

### Direction
- Algocube digit **1** → imm ANDN-plane TOC (imm dual of SANDNTOC after SNANDTOCN/SANDTOCN plane).

### Language
- `SANDNTOCN`/`BICTOCN`/`SANDNATN` n — stack `i → cells[i]&=~n` leave result
- `SORNTOCN`/`ORNTOCN`/`SORNATN` n — stack `i → cells[i]|=~n` leave result
- `SXORNTOCN`/`XORNTOCN`/`SXORNATN` n — stack `i → cells[i]^=~n` leave result
- Proof `354_sandntocn_sxorntocn.cubalc`

### Prior
See 1.12.330-universal.

## 1.12.330-universal — 2026-08-04

### Direction
- Algocube digit **9** → reverse unsigned imm TOC (imm dual of SUDIVFROMN plane after SDIVFROMTOCN).

### Language
- `SUDIVFROMTOCN`/`RUDIVTOCN`/`SUDIVFROMATN` n — stack `i → cells[i]=(u)n/(u)cells[i]` (0→0) leave result
- `SUMODFROMTOCN`/`RUMODTOCN`/`SUMODFROMATN` n — stack `i → cells[i]=(u)n%(u)cells[i]` (0→0) leave result
- Proof `353_sudivfromtocn_sumodfromtocn.cubalc`

### Prior
See 1.12.329-universal.

## 1.12.329-universal — 2026-08-04

### Direction
- Algocube digit **0** → foundation imm inverted bitwise TOC (imm dual of SNANDTOC plane after SANDTOCN).

### Language
- `SNANDTOCN`/`NANDTOCN`/`SNANDATN` n — stack `i → cells[i]=~(cells[i]&n)` leave result
- `SNORTOCN`/`NORTOCN`/`SNORATN` n — stack `i → cells[i]=~(cells[i]|n)` leave result
- `SXNORTOCN`/`XNORTOCN`/`SXNORATN` n — stack `i → cells[i]=~(cells[i]^n)` leave result
- Proof `352_snandtocn_sxnortocn.cubalc`

### Prior
See 1.12.328-universal.

## 1.12.328-universal — 2026-08-04

### Direction
- Algocube digit **1** → imm rotate TOC (imm dual of SROLTOC/SRORTOC; complete shift/rotate TOCN after SSHLTOCN).

### Language
- `SROLTOCN`/`ROLTOCN`/`SROLATN` n — stack `i → cells[i]=rotl(cells[i],n mod 64)` leave result
- `SRORTOCN`/`RORTOCN`/`SRORATN` n — stack `i → cells[i]=rotr(cells[i],n mod 64)` leave result
- Proof `351_sroltocn_srortocn.cubalc`

### Prior
See 1.12.327-universal.

## 1.12.327-universal — 2026-08-04

### Direction
- Algocube digit **8** → stack foundation imm shift TOC (imm dual of SSHLTOC/SSHRTOC/SSARTOC after SSHLN plane).

### Language
- `SSHLTOCN`/`SHLTOCN`/`SSHLATN` n — stack `i → cells[i]<<=n` (n clamp 0..63) leave result
- `SSHRTOCN`/`SHRTOCN`/`SSHRATN` n — stack `i → cells[i] logical >>=n` leave result
- `SSARTOCN`/`SARTOCN`/`SSARATN` n — stack `i → cells[i] arithmetic >>=n` leave result
- Proof `350_sshltocn_ssartocn.cubalc`

### Prior
See 1.12.326-universal.

## 1.12.326-universal — 2026-08-04

### Direction
- Algocube digit **9** → reverse imm accumulate TOC (imm dual of SSUBFROMTOC plane after SSUBFROMN).

### Language
- `SSUBFROMTOCN`/`RSUBTOCN`/`SSUBFROMATN` n — stack `i → cells[i]=n-cells[i]` leave result
- `SDIVFROMTOCN`/`RDIVTOCN`/`SDIVFROMATN` n — stack `i → cells[i]=n/cells[i]` (cell0→0) leave result
- `SMODFROMTOCN`/`RMODTOCN`/`SMODFROMATN` n — stack `i → cells[i]=n%cells[i]` (cell0→0) leave result
- Proof `349_ssubfromtocn_smodfromtocn.cubalc`

### Prior
See 1.12.325-universal.

## 1.12.325-universal — 2026-08-04

### Direction
- Algocube digit **4** → imm accumulate bound TOC (complete SADDTOCN plane with / % min max).

### Language
- `SDIVTOCN`/`DIVTOCN`/`SDIVATN` n — stack `i → cells[i]/=n` (n==0→0) leave result
- `SMODTOCN`/`MODTOCN`/`SMODATN` n — stack `i → cells[i]%=n` (n==0→0) leave result
- `SMINTOCN`/`MINTOCN`/`SMINATN` n — stack `i → cells[i]=min(cells[i],n)` leave result
- `SMAXTOCN`/`MAXTOCN`/`SMAXATN` n — stack `i → cells[i]=max(cells[i],n)` leave result
- Proof `348_sdivtocn_smaxtocn.cubalc`

### Prior
See 1.12.324-universal.

## 1.12.324-universal — 2026-08-04

### Direction
- Algocube digit **5** → imm bitwise TOC (imm dual of SANDTOC/SORTOC/SXORTOC after bitfield/SADDTOCN plane).

### Language
- `SANDTOCN`/`ANDTOCN`/`SANDATN` n — stack `i → cells[i]&=n` leave result
- `SORTOCN`/`ORTOCN`/`SORATN` n — stack `i → cells[i]|=n` leave result
- `SXORTOCN`/`XORTOCN`/`SXORATN` n — stack `i → cells[i]^=n` leave result
- Proof `347_sandtocn_sxortocn.cubalc`

### Prior
See 1.12.323-universal.

## 1.12.323-universal — 2026-08-04

### Direction
- Algocube digit **4** → imm accumulate TOC (imm dual of SADDTOC/SSUBTOC/SMULTOC after SADDN plane).

### Language
- `SADDTOCN`/`ADDTOCN`/`SADDATN` n — stack `i → cells[i]+=n` leave result
- `SSUBTOCN`/`SUBTOCN`/`SSUBATN` n — stack `i → cells[i]-=n` leave result
- `SMULTOCN`/`MULTOCN`/`SMULATN` n — stack `i → cells[i]*=n` leave result
- Proof `346_saddtocn_smultocn.cubalc`

### Prior
See 1.12.322-universal.

## 1.12.322-universal — 2026-08-04

### Direction
- Algocube digit **8** → stack↔cell bit metrics TOC (dual of SPOPCNT/SCLZ/SCTZ after parity/unary TOC plane).

### Language
- `SPOPCNTTOC`/`SPCNTTOC`/`SPOPAT` — stack `i → cells[i]=popcount(cells[i])` leave result
- `SCLZTOC`/`CLZTOC`/`SCLZAT` — stack `i → cells[i]=clz64(cells[i])` (0→64) leave result
- `SCTZTOC`/`CTZTOC`/`SCTZAT` — stack `i → cells[i]=ctz64(cells[i])` (0→64) leave result
- Proof `345_spopcnttoc_sclztoc.cubalc`

### Prior
See 1.12.321-universal.

## 1.12.321-universal — 2026-08-04

### Direction
- Algocube digit **1** → stack↔cell 3-way imm compare TOC (imm dual of SCMPTOC/SUCMPTOC after 0/1 TOCN plane).

### Language
- `SCMPTOCN`/`CMP3TOCN`/`SCMPATN` n — stack `i → cells[i]=signed 3-way(cells[i],n)` as −1/0/+1 leave result
- `SUCMPTOCN`/`UCMP3TOCN`/`SUCMPATN` n — stack `i → cells[i]=unsigned 3-way` as −1/0/+1 leave result
- Proof `344_scmptocn_sucmptocn.cubalc`

### Prior
See 1.12.320-universal.

## 1.12.320-universal — 2026-08-04

### Direction
- Algocube digit **8** → stack foundation unsigned imm compare TOC (unsigned dual of signed TOCN plane).

### Language
- `SULTTOCN`/`ULTTOCN`/`SULTATN` n — stack `i → cells[i]=((u)cells[i]<(u)n)?1:0` leave result
- `SUGTTOCN`/`UGTTOCN`/`SUGTATN` n — stack `i → cells[i]=((u)cells[i]>(u)n)?1:0` leave result
- `SULETOCN`/`ULETOCN`/`SULEATN` n — stack `i → cells[i]=((u)cells[i]<=(u)n)?1:0` leave result
- `SUGETOCN`/`UGETOCN`/`SUGEATN` n — stack `i → cells[i]=((u)cells[i]>=(u)n)?1:0` leave result
- Proof `343_sulttocn_sugtocn.cubalc`

### Prior
See 1.12.319-universal.

## 1.12.319-universal — 2026-08-04

### Direction
- Algocube digit **9** → data-path imm compare bounds TOC (LE/GE complete SEQ/NE/LT/GT TOCN plane).

### Language
- `SLETOCN`/`LETOCN`/`SLEATN` n — stack `i → cells[i]=(cells[i]<=n)?1:0` leave result
- `SGETOCN`/`GETOCN`/`SGEATN` n — stack `i → cells[i]=(cells[i]>=n)?1:0` leave result
- Proof `342_sletocn_sgetocn.cubalc`

### Prior
See 1.12.318-universal.

## 1.12.318-universal — 2026-08-04

### Direction
- Algocube digit **3** → imm compare TOC (imm dual of SEQTOC/SNETOC/SLTTOC/SGTTOC; peer of SEQN plane).

### Language
- `SEQTOCN`/`EQTOCN`/`SEQATN` n — stack `i → cells[i]=(cells[i]==n)?1:0` leave result
- `SNETOCN`/`NETOCN`/`SNEATN` n — stack `i → cells[i]=(cells[i]!=n)?1:0` leave result
- `SLTTOCN`/`LTTOCN`/`SLTATN` n — stack `i → cells[i]=(cells[i]<n)?1:0` leave result
- `SGTTOCN`/`GTTOCN`/`SGTATN` n — stack `i → cells[i]=(cells[i]>n)?1:0` leave result
- Proof `341_seqtocn_sgttocn.cubalc`

### Prior
See 1.12.317-universal.

## 1.12.317-universal — 2026-08-04

### Direction
- Algocube digit **7** → unary mutator TOC (dual of SINC/SDEC after SDBL/SHALF scale TOC plane).

### Language
- `SINCTOC`/`INCTOC`/`SINCAT` — stack `i → cells[i]+=1` leave result
- `SDECTOC`/`DECTOC`/`SDECAT` — stack `i → cells[i]-=1` leave result
- Proof `340_sinctoc_sdectoc.cubalc`

### Prior
See 1.12.316-universal.

## 1.12.316-universal — 2026-08-04

### Direction
- Algocube digit **1** → unary parity TOC (dual of SODD/SEVEN into cell after zero-rel predicate TOC).

### Language
- `SODDTOC`/`ODDTOC`/`SODDAT` — stack `i → cells[i]=(cells[i]&1)?1:0` leave result
- `SEVENTOC`/`EVENTOC`/`SEVENAT` — stack `i → cells[i]=((cells[i]&1)==0)?1:0` leave result
- Proof `339_soddtoc_seventoc.cubalc`

### Prior
See 1.12.315-universal.

## 1.12.315-universal — 2026-08-04

### Direction
- Algocube digit **4** → zero-relative compare TOC (complete S0*/DLTZ plane into cell after SEQZ/SNEZ TOC).

### Language
- `SLTZTOC`/`S0LTTOC`/`SLTZAT` — stack `i → cells[i]=(cells[i]<0)?1:0` leave result
- `SGTZTOC`/`S0GTTOC`/`SGTZAT` — stack `i → cells[i]=(cells[i]>0)?1:0` leave result
- `SLEZTOC`/`S0LETOC`/`SLEZAT` — stack `i → cells[i]=(cells[i]<=0)?1:0` leave result
- `SGEZTOC`/`S0GEZTOC`/`SGEZAT` — stack `i → cells[i]=(cells[i]>=0)?1:0` leave result
- Proof `338_sltz_sgez_toc.cubalc`

### Prior
See 1.12.314-universal.

## 1.12.314-universal — 2026-08-03

### Direction
- Algocube digit **7** → unary control TOC (dual of SSIGN/SEQZ/SNEZ into cell after 3-way compare TOC).

### Language
- `SSIGNTOC`/`SGNTOC`/`SSIGNAT` — stack `i → cells[i]=sign(cells[i])` as −1/0/+1 leave result
- `SEQZTOC`/`EQZTOC`/`SEQZAT` — stack `i → cells[i]=(cells[i]==0)?1:0` leave result
- `SNEZTOC`/`NEZTOC`/`SNEZAT` — stack `i → cells[i]=(cells[i]!=0)?1:0` leave result
- Proof `337_ssigntoc_snez.cubalc`

### Prior
See 1.12.313-universal.

## 1.12.313-universal — 2026-08-03

### Direction
- Algocube digit **1** → stack↔cell 3-way compare TOC (dual of SCMP/SUCMP into cell after 0/1 predicate TOC plane).

### Language
- `SCMPTOC`/`SICMPTOC`/`SCMPAT` — stack `i v → cells[i]=signed 3-way(cells[i],v)` as −1/0/+1 leave result
- `SUCMPTOC`/`SUCMP3TOC`/`SUCMPAT` — stack `i v → cells[i]=unsigned 3-way` as −1/0/+1 leave result
- Proof `336_scmptoc_sucmptoc.cubalc`

### Prior
See 1.12.312-universal.

## 1.12.312-universal — 2026-08-03

### Direction
- Algocube digit **9** → data-path unsigned compare TOC (unsigned dual of signed SEQ/SLT plane into cell).

### Language
- `SULTTOC`/`CMPULTTOC`/`SULTAT` — stack `i v → cells[i]=((u)cells[i]<(u)v)?1:0` leave result
- `SUGTTOC`/`CMPUGTTOC`/`SUGTAT` — stack `i v → cells[i]=((u)cells[i]>(u)v)?1:0` leave result
- `SULETOC`/`CMPULETOC`/`SULEAT` — stack `i v → cells[i]=((u)cells[i]<=(u)v)?1:0` leave result
- `SUGETOC`/`CMPUGETOC`/`SUGEAT` — stack `i v → cells[i]=((u)cells[i]>=(u)v)?1:0` leave result
- Proof `335_sulttoc_sugtoc.cubalc`

### Prior
See 1.12.311-universal.

## 1.12.311-universal — 2026-08-03

### Direction
- Algocube digit **6** → energy compare bounds TOC (NE/LE/GE after SEQ/SLT/SGT plane).

### Language
- `SNETOC`/`CMPNETOC`/`SNEAT` — stack `i v → cells[i]=(cells[i]!=v)?1:0` leave result
- `SLETOC`/`CMPLETOC`/`SLEAT` — stack `i v → cells[i]=(cells[i]<=v)?1:0` leave result
- `SGETOC`/`CMPGETOC`/`SGEAT` — stack `i v → cells[i]=(cells[i]>=v)?1:0` leave result
- Proof `334_snetoc_sgetoc.cubalc`

### Prior
See 1.12.310-universal.

## 1.12.310-universal — 2026-08-03

### Direction
- Algocube digit **4** → stack↔cell compare plane (predicate dual of SEQ/SLT/SGT into cell).

### Language
- `SEQTOC`/`CMPEQTOC`/`SEQAT` — stack `i v → cells[i]=(cells[i]==v)?1:0` leave result
- `SLTTOC`/`CMPLTTOC`/`SLTAT` — stack `i v → cells[i]=(cells[i]<v)?1:0` leave result
- `SGTTOC`/`CMPGTTOC`/`SGTAT` — stack `i v → cells[i]=(cells[i]>v)?1:0` leave result
- Proof `333_seqtoc_sgttoc.cubalc`

### Prior
See 1.12.309-universal.

## 1.12.309-universal — 2026-08-03

### Direction
- Algocube digit **9** → depth-9 over/tuck (complete depth-9 plane after 9NIP/9ROT).
- Lexer: promote `9OVER`/`9TUCK` tails.

### Language
- `9OVER`/`OVER9`/`STACK9OVER` — 18-deep: copy under nonet onto stack
- `9TUCK`/`TUCK9`/`STACK9TUCK` — `a…i → i a…h i` (copy TOS under top 8)
- Proof `332_9over_9tuck.cubalc`

### Prior
See 1.12.308-universal.

## 1.12.308-universal — 2026-08-03

### Direction
- Algocube digit **9** → depth-9 combinator ext (NIP/ROT/RROT after 9DUP/DROP/SWAP foundation).
- Lexer: promote `9NIP`/`9ROT`/`9RROT` tails.

### Language
- `9NIP`/`NIP9`/`STACK9NIP` — keep ends of top 9
- `9ROT`/`ROT9`/`STACK9ROT` — rotate top 9 left
- `9RROT`/`RROT9`/`STACK9RROT` — rotate top 9 right
- Proof `331_9nip_9rot.cubalc`

### Prior
See 1.12.307-universal.

## 1.12.307-universal — 2026-08-03

### Direction
- Algocube digit **6** → energy ANDN stack↔cell (cells[i] op ~v after SNAND/NOR/XNOR TOC plane).

### Language
- `SANDNTOC`/`SBICTOC`/`BICTOC` — stack `i v → cells[i]&=~v` leave result
- `SORNTOC`/`SORNOTTOC`/`ORNOTTOC` — stack `i v → cells[i]|=~v` leave result
- `SXORNTOC`/`SXORNOTTOC`/`XORNOTTOC` — stack `i v → cells[i]^=~v` leave result
- Proof `330_sandntoc_sxorntoc.cubalc`

### Prior
See 1.12.306-universal.

## 1.12.306-universal — 2026-08-03

### Direction
- Algocube digit **2** → stack↔cell math dual (square / integer sqrt after scale TOC; dual of SSQR/SISQRT).

### Language
- `SSQRTOC`/`SCELLSQR`/`SQRTOC` — stack `i → cells[i]*=cells[i]` leave result
- `SISQRTTOC`/`SCELLISQRT`/`ISQRTTOC` — stack `i → cells[i]=isqrt(cells[i])` (neg→0) leave result
- Proof `329_ssqrtoc_sisqrtoc.cubalc`

### Prior
See 1.12.305-universal.

## 1.12.305-universal — 2026-08-03

### Direction
- Algocube digit **8** → depth-9 foundation (nonuple after complete depth-8 plane).
- Lexer: promote `9DUP`/`9DROP`/`9SWAP` tails.

### Language
- `9DUP`/`DUP9`/`STACK9DUP` — duplicate top 9
- `9DROP`/`DROP9`/`STACK9DROP` — drop top 9
- `9SWAP`/`SWAP9`/`STACK9SWAP` — reverse top 9
- Proof `328_9dup_9drop.cubalc`

### Prior
See 1.12.304-universal.

## 1.12.304-universal — 2026-08-03

### Direction
- Algocube digit **7** → stack↔cell unary scale (double/half after ABS/NEG/NOT TOC; dual of SDBL/SHALF).

### Language
- `SDBLTOC`/`SDBLTOCELL`/`SCELLDBL`/`DBLTOC` — stack `i → cells[i]*=2` leave result
- `SHALFTOC`/`SHALFTOCELL`/`SCELLHALF`/`HALFTOC` — stack `i → cells[i]/=2` (toward zero) leave result
- Proof `327_sdbltoc_shalftoc.cubalc`

### Prior
See 1.12.303-universal.

## 1.12.303-universal — 2026-08-03

### Direction
- Algocube digit **1** → stack↔cell rotate dual (complete shift/rotate TOC after SSHL/SHR/SAR TOC plane).

### Language
- `SROLTOC`/`SROLTOCELL`/`ROLTOC`/`SCELLROL` — stack `i n → cells[i]=rotl(cells[i],n mod 64)` leave result
- `SRORTOC`/`SRORTOCELL`/`RORTOC`/`SCELLROR` — stack `i n → cells[i]=rotr(cells[i],n mod 64)` leave result
- Proof `326_sroltoc_srortoc.cubalc`

### Prior
See 1.12.302-universal.

## 1.12.302-universal — 2026-08-03

### Direction
- Algocube digit **0** → stack foundation shift stack↔cell (SHL/SHR/SAR into cell after bitwise TOC plane).

### Language
- `SSHLTOC`/`SSHLTOCELL`/`SHLTOC`/`SCELLSHL` — stack `i n → cells[i]<<=n` (n clamp 0..63) leave result
- `SSHRTOC`/`SSHRTOCELL`/`SHRTOC`/`SCELLSHR` — stack `i n → cells[i]=logical>>n` leave result
- `SSARTOC`/`SASHRTOC`/`SARTOC`/`SCELLSAR` — stack `i n → cells[i]=arithmetic>>n` leave result
- Proof `325_sshltoc_ssartoc.cubalc`

### Prior
See 1.12.301-universal.

## 1.12.301-universal — 2026-08-03

### Direction
- Algocube digit **6** → energy inverted bitwise stack↔cell (NAND/NOR/XNOR after SANDTOC/SORTOC/SXORTOC plane).

### Language
- `SNANDTOC`/`SNANDTOCELL`/`BNANDTOC` — stack `i v → cells[i]=~(cells[i]&v)` leave result
- `SNORTOC`/`SNORTOCELL`/`BNORTOC` — stack `i v → cells[i]=~(cells[i]|v)` leave result
- `SXNORTOC`/`SEQUIVTOC`/`BXNORTOC` — stack `i v → cells[i]=~(cells[i]^v)` leave result
- Proof `324_snandtoc_sxnortoc.cubalc`

### Prior
See 1.12.300-universal.

## 1.12.300-universal — 2026-08-03

### Direction
- Algocube digit **4** → depth-8 over/tuck (complete depth-8 plane after 8NIP/8ROT; stack structure control).
- Lexer: promote `8OVER`/`8TUCK` tails.

### Language
- `8OVER`/`OVER8`/`STACK8OVER` — 16-deep: copy under octet onto stack
- `8TUCK`/`TUCK8`/`STACK8TUCK` — `a…h → h a…g h` (copy TOS under top 7)
- Proof `323_8over_8tuck.cubalc`

### Prior
See 1.12.299-universal.

## 1.12.299-universal — 2026-08-03

### Direction
- Algocube digit **6** → sat energy stack↔cell complete (mul + clamp after SSATADD/SUB/DIV TOC).

### Language
- `SSATMULTOC`/`SCELLSATMUL`/`SATMULTOC` — stack `i v → cells[i]=sat(cells[i]*v)` leave result
- `SCLAMPTOC`/`SCELLCLAMP`/`SBOUNDTOC` — stack `i lo hi → cells[i]=clamp(cells[i],lo,hi)` leave result
- Proof `322_ssatmultoc_sclamptoc.cubalc`

### Prior
See 1.12.298-universal.

## 1.12.298-universal — 2026-08-03

### Direction
- Algocube digit **9** → depth-8 combinator extension after 8DUP/8DROP/8SWAP foundation (parity with 7NIP/7ROT plane).
- Lexer: promote `8NIP`/`8ROT`/`8RROT` tails.

### Language
- `8NIP`/`NIP8`/`STACK8NIP` — keep ends of top 8 (`a…h → a h`)
- `8ROT`/`ROT8`/`STACK8ROT` — rotate top 8 left (`a…h → b…h a`)
- `8RROT`/`RROT8`/`STACK8RROT` — rotate top 8 right (`a…h → h a…g`)
- Proof `321_8nip_8rot.cubalc`

### Prior
See 1.12.297-universal.

## 1.12.297-universal — 2026-08-03

### Direction
- Algocube digit **7** → stack↔cell unary ALU (dual of SABS/SNEG/SNOT after bitwise TOC plane).

### Language
- `SABSTOC`/`SABSCELL`/`SCELLABS` — stack `i → cells[i]=|cells[i]|` leave abs
- `SNEGTOC`/`SNEGCELL`/`SCELLNEG` — stack `i → cells[i]=-cells[i]` leave result
- `SNOTTOC`/`SNOTTOCELL`/`BINVTOC` — stack `i → cells[i]=~cells[i]` leave result
- Proof `320_sabstoc_snottoc.cubalc`

### Prior
See 1.12.296-universal.

## 1.12.296-universal — 2026-08-03

### Direction
- Algocube digit **1** → stack↔cell bitwise accumulate (single-index dual of range SANDCELL plane after arith TOC).

### Language
- `SANDTOC`/`SANDTOCELL`/`BANDTOC` — stack `i v → cells[i]&=v` leave result
- `SORTOC`/`SORTOCELL`/`BORTOC` — stack `i v → cells[i]|=v` leave result
- `SXORTOC`/`SXORTOCELL`/`BXORTOC` — stack `i v → cells[i]^=v` leave result
- Proof `319_sandtoc_sxortoc.cubalc`

### Prior
See 1.12.295-universal.

## 1.12.295-universal — 2026-08-03

### Direction
- Algocube digit **6** → forward saturating stack↔cell energy (sat dual of SADDTOC/SSUBTOC/SDIVTOC; complement reverse sat TOC).

### Language
- `SSATADDTOC`/`SCELLSATADD`/`SATADDTOC` — stack `i v → cells[i]=sat(cells[i]+v)` leave result
- `SSATSUBTOC`/`SCELLSATSUB`/`SATSUBTOC` — stack `i v → cells[i]=sat(cells[i]−v)` leave result
- `SSATDIVTOC`/`SCELLSATDIV`/`SATDIVTOC` — stack `i v → cells[i]=sat(cells[i]/v)` (0 if v==0; LONG_MIN/−1 → LONG_MAX)
- Proof `318_ssataddtoc_ssatdivtoc.cubalc`

### Prior
See 1.12.294-universal.

## 1.12.294-universal — 2026-08-03

### Direction
- Algocube digit **9** → reverse saturating stack↔cell (sat dual of SSUBFROMTOC/SDIVFROMTOC).

### Language
- `SSATSUBFROMTOC`/`SCELLSATSUBFROM`/`SATSUBFROMTOC` — stack `i v → cells[i]=sat(v-cells[i])` leave result
- `SSATDIVFROMTOC`/`SCELLSATDIVFROM`/`SATDIVFROMTOC` — stack `i v → cells[i]=sat(v/cells[i])` (0 if cell 0; LONG_MIN/-1 → LONG_MAX)
- Proof `317_ssatsubfromtoc_ssatdivfromtoc.cubalc`

### Prior
See 1.12.293-universal.

## 1.12.293-universal — 2026-08-03

### Direction
- Algocube digit **9** → stack↔cell reverse accumulate (v op cells[i] after SSUBTOC/SDIVTOC/SMODTOC plane).

### Language
- `SSUBFROMTOC`/`SCELLSUBFROM` — stack `i v → cells[i]=v-cells[i]` leave result
- `SDIVFROMTOC`/`SCELLDIVFROM` — stack `i v → cells[i]=v/cells[i]` (0 if cell 0) leave quotient
- `SMODFROMTOC`/`SCELLMODFROM` — stack `i v → cells[i]=v%cells[i]` (0 if cell 0) leave remainder
- Proof `316_ssubfromtoc_smodfromtoc.cubalc`

### Prior
See 1.12.292-universal.

## 1.12.292-universal — 2026-08-03

### Direction
- Algocube digit **1** → stack↔cell accumulate bound (mod + min/max after SADDTOC/SSUBTOC/SMULTOC/SDIVTOC).

### Language
- `SMODTOC`/`SCELLMOD`/`SMODCELL` — stack `i v → cells[i]%=v` (0 if v==0) leave remainder
- `SMINTOC`/`SCELLMIN`/`SMINCELL` — stack `i v → cells[i]=min(cells[i],v)` leave result
- `SMAXTOC`/`SCELLMAX`/`SMAXCELL` — stack `i v → cells[i]=max(cells[i],v)` leave result
- Proof `315_smodtoc_smintoc.cubalc`

### Prior
See 1.12.291-universal.

## 1.12.291-universal — 2026-08-03

### Direction
- Algocube digit **8** → depth-8 foundation (extend after complete depth-7 plane).
- Lexer: promote `8…` depth idents (DUP/DROP/SWAP) like 2…7.

### Language
- `8DUP`/`DUP8`/`STACK8DUP` — duplicate top 8
- `8DROP`/`DROP8`/`STACK8DROP` — drop top 8
- `8SWAP`/`SWAP8`/`STACK8SWAP` — reverse top 8
- Proof `314_8dup_8drop.cubalc`

### Prior
See 1.12.290-universal.

## 1.12.290-universal — 2026-08-03

### Direction
- Algocube digit **7** → multiword negate-via-complement+cin (stack dual of DNEGC2 + shared-CARRY/imm duals).

### Language
- `SNEGC`/`SCOMADC` — TOS = (~TOS)+cin(CARRY); CARRY = cout
- `SNEGCN`/`NEGCN` n — TOS = (~TOS)+(n?1:0); CARRY = cout
- `DNEGCC`/`PAIRNEGCC` — pair shared-CARRY cin form
- `DNEGCN`/`PAIRNEGCN` n — pair shared imm cin
- Proof `313_snegc_dnegcn.cubalc`

### Prior
See 1.12.289-universal.

## 1.12.289-universal — 2026-08-03

### Direction
- Algocube digit **0** → imm unsigned min/max (imm dual of SMINN/SMAXN for unsigned + dual-stack after SULTN plane).

### Language
- `SUMINN`/`UMINN` n — TOS = unsigned min(TOS, n)
- `SUMAXN`/`UMAXN` n — TOS = unsigned max(TOS, n)
- `DUMINN`/`PAIRUMINN` n — a b → unsigned min(a,n) min(b,n)
- `DUMAXN`/`PAIRUMAXN` n — a b → unsigned max(a,n) max(b,n)
- Proof `312_suminn_dumaxn.cubalc`

### Prior
See 1.12.288-universal.

## 1.12.288-universal — 2026-08-03

### Direction
- Algocube digit **3** → imm unsigned compare (imm dual of SULT/SUGT/SULE/SUGE + dual-stack after signed SEQN plane).

### Language
- `SULTN`/`ULTN` n — TOS = ((unsigned)TOS < n) ? 1 : 0
- `SUGTN`/`UGTN` n — TOS = ((unsigned)TOS > n) ? 1 : 0
- `SULEN`/`ULEN` n — TOS = ((unsigned)TOS ≤ n) ? 1 : 0
- `SUGEN`/`UGEN` n — TOS = ((unsigned)TOS ≥ n) ? 1 : 0
- `DULTN`/`DUGTN`/`DULEN`/`DUGEN` n — pair unsigned cmp vs n
- Proof `311_sultn_dugen.cubalc`

### Prior
See 1.12.287-universal.

## 1.12.287-universal — 2026-08-03

### Direction
- Algocube digit **2** → imm coprime/pow (imm dual of SCOPRIME/SPOW after SGCDN plane + dual-stack).

### Language
- `SCOPRIMEN`/`COPRIMEN` n — TOS = 1 if gcd(|TOS|,|n|)==1 else 0
- `SPOWN`/`POWN` n — TOS = TOS^n; n<0 → 0
- `DCOPRIMEN`/`PAIRCOPRIMEN` n — a b → coprime(a,n) coprime(b,n) as 0/1
- `DPOWN`/`PAIRPOWN` n — a b → a^n b^n; n<0 → 0,0
- Proof `310_scoprimenn_dpown.cubalc`

### Prior
See 1.12.286-universal.

## 1.12.286-universal — 2026-08-03

### Direction
- Algocube digit **7** → imm ceil/floor div (imm dual of SDIVCEIL/SDIVFLOOR + dual-stack after SDIVN plane).

### Language
- `SDIVCEILN`/`CEILDIVN`/`CEILN` n — TOS = ceil(TOS/n); n==0 → 0
- `SDIVFLOORN`/`FLOORDIVN`/`FLOORN` n — TOS = floor(TOS/n); n==0 → 0
- `DDIVCEILN`/`PAIRDIVCEILN` n — a b → ceil(a/n) ceil(b/n)
- `DDIVFLOORN`/`PAIRDIVFLOORN` n — a b → floor(a/n) floor(b/n)
- Proof `309_sdivceiln_ddivfloorn.cubalc`

### Prior
See 1.12.285-universal.

## 1.12.285-universal — 2026-08-03

### Direction
- Algocube digit **2** → imm GCD/LCM (imm dual of SGCD/SLCM + shared-n dual-stack).

### Language
- `SGCDN`/`GCDN` n — TOS = gcd(|TOS|,|n|)
- `SLCMN`/`LCMN` n — TOS = lcm(|TOS|,|n|); 0 if either side 0
- `DGCDN`/`PAIRGCDN` n — a b → gcd(a,n) gcd(b,n)
- `DLCMN`/`PAIRLCMN` n — a b → lcm(a,n) lcm(b,n)
- Proof `308_sgcdn_dlcmn.cubalc`

### Prior
See 1.12.284-universal.

## 1.12.284-universal — 2026-08-03

### Direction
- Algocube digit **5** → multiword imm rotate-through-CARRY (imm dual of SSHLC/SSHRC + dual-stack).

### Language
- `SSHLCN`/`RCLN`/`SHLCYN` n — RCL TOS by n (0..64); CARRY in/out
- `SSHRCN`/`RCRN`/`SHRCYN` n — RCR TOS by n (0..64); CARRY in/out
- `DSHLCCN`/`PAIRSHLCCN`/`DRCLN` n — pair RCL; shared cin; CARRY = OR cout
- `DSHRCCN`/`PAIRSHRCCN`/`DRCRN` n — pair RCR; shared cin; CARRY = OR cout
- Proof `307_sshlcn_dshrccn.cubalc`

### Prior
See 1.12.283-universal.

## 1.12.283-universal — 2026-08-03

### Direction
- Algocube digit **0** → depth-7 foundation complete (7OVER/7TUCK after 7NIP/7ROT plane).

### Language
- `7OVER`/`OVER7`/`STACK7OVER` — 14-deep: copy under septet onto stack
- `7TUCK`/`TUCK7`/`STACK7TUCK` — a…g → g a…f g (copy TOS under top 6)
- Lexer: `7…` tails include OVER/TUCK
- Proof `306_7over_7tuck.cubalc`

### Prior
See 1.12.282-universal.

## 1.12.282-universal — 2026-08-03

### Direction
- Algocube digit **6** → reverse imm sat ALU (energy/sat dual of SSUBFROMN/SDIVFROMN + dual-stack).

### Language
- `SSATSUBFROMN`/`SATSUBFROMN`/`SSRSUBN` n — TOS = sat(n − TOS)
- `SSATDIVFROMN`/`SATDIVFROMN`/`SSRDIVN` n — TOS = sat(n / TOS); TOS==0 → 0; `LONG_MIN/-1→LONG_MAX`
- `DSATSUBFROMN`/`PAIRSATSUBFROMN` n — a b → sat(n−a) sat(n−b)
- `DSATDIVFROMN`/`PAIRSATDIVFROMN` n — a b → sat(n/a) sat(n/b); lane0 → 0
- Proof `305_ssatsubfromn_dsatdivfromn.cubalc`

### Prior
See 1.12.281-universal.

## 1.12.281-universal — 2026-08-03

### Direction
- Algocube digit **3** → stack saturating divide (complete SSATADD/SSATSUB/SSATMUL plane; dual of DSATDIV path).

### Language
- `SSATDIV`/`STACKSATDIV`/`SATDIVST` — a b → sat(a/b); `/0→0`; `LONG_MIN/-1→LONG_MAX`
- Proof `304_ssatdiv.cubalc`

### Prior
See 1.12.280-universal.

## 1.12.280-universal — 2026-08-03

### Direction
- Algocube digit **7** → depth-7 rotate/nip ext (parity with 6NIP/6ROT after 7DUP foundation).

### Language
- `7NIP`/`NIP7` — a…g → a g (keep ends of top 7)
- `7ROT`/`ROT7` — a b c d e f g → b c d e f g a
- `7RROT`/`RROT7` — a b c d e f g → g a b c d e f
- Lexer: `7…` tails include NIP/ROT/RROT
- Proof `303_7nip_7rot.cubalc`

### Prior
See 1.12.279-universal.

## 1.12.279-universal — 2026-08-03

### Direction
- Algocube digit **6** → imm energy wrap (imm dual of SWMOD + shared-modulus dual of DWRAP).

### Language
- `SWMODN`/`SWRAPN`/`WRAPN` m — TOS = wrap(TOS,m) in [0,m); m≤0 → 0
- `DWRAPN`/`DWMODN`/`PAIRWRAPN` m — a b → wrap(a,m) wrap(b,m); m≤0 → 0,0
- Proof `302_swmodn_dwrapn.cubalc`

### Prior
See 1.12.278-universal.

## 1.12.278-universal — 2026-08-03

### Direction
- Algocube digit **9** → reverse unsigned imm div/mod (n op_u TOS; complete SUDIVN plane after SSUBFROMN).

### Language
- `SUDIVFROMN`/`UDIVFROMN` n — TOS = (unsigned)n / (unsigned)TOS; TOS==0 → 0
- `SUMODFROMN`/`UMODFROMN` n — TOS = (unsigned)n % (unsigned)TOS; TOS==0 → 0
- `DUDIVFROMN`/`PAIRUDIVFROMN` n — a b → unsigned (n/a) (n/b); lane 0 → 0
- `DUMODFROMN`/`PAIRUMODFROMN` n — a b → unsigned (n%a) (n%b); lane 0 → 0
- Proof `301_sudivfromn_dumodfromn.cubalc`

### Prior
See 1.12.277-universal.

## 1.12.277-universal — 2026-08-03

### Direction
- Algocube digit **0** → stack/dual-stack imm unsigned div/mod (imm dual of SUDIV/SUMOD + DUDIV/DUMOD).

### Language
- `SUDIVN`/`UDIVN` n — TOS = (unsigned)TOS / n; n==0 → 0
- `SUMODN`/`UMODN` n — TOS = (unsigned)TOS % n; n==0 → 0
- `DUDIVN`/`PAIRUDIVN` n — a b → unsigned (a/n) (b/n)
- `DUMODN`/`PAIRUMODN` n — a b → unsigned (a%n) (b%n)
- Proof `300_sudivn_dumodn.cubalc`

### Prior
See 1.12.276-universal.

## 1.12.276-universal — 2026-08-03

### Direction
- Algocube digit **8** → stack depth-7 foundation (extend after complete depth-6 plane).
- Lexer: promote `7…` depth idents (same path as 2…6) so `7DUP`/`7DROP`/`7SWAP` tokenize.

### Language
- `7DUP`/`DUP7`/`STACK7DUP` — duplicate top 7
- `7DROP`/`DROP7`/`STACK7DROP` — drop top 7
- `7SWAP`/`SWAP7`/`STACK7SWAP` — reverse top 7
- Lex digit-prefix idents: `7` + DUP/DROP/SWAP/NIP/ROT/RROT/OVER/TUCK
- Proof `299_7dup_7drop.cubalc`

### Prior
See 1.12.275-universal.

## 1.12.275-universal — 2026-08-03

### Direction
- Algocube digit **6** → zero-bound predicates on energy/sign plane (complete ≤0/≥0 after D0LT/D0GT + stack duals).

### Language
- `D0LE`/`PAIR0LE`/`DNONPOS` — a b → (a≤0?1:0) (b≤0?1:0)
- `D0GE`/`PAIR0GE`/`DNONNEG` — a b → (a≥0?1:0) (b≥0?1:0)
- `S0LE`/`STACK0LE`/`SNONPOS` — TOS = (TOS≤0?1:0)
- `S0GE`/`STACK0GE`/`SNONNEG` — TOS = (TOS≥0?1:0)
- Proof `298_d0le_d0ge.cubalc`

### Prior
See 1.12.274-universal.

## 1.12.274-universal — 2026-08-03

### Direction
- Algocube digit **5** → multiword shift-through-CARRY flag (bitfield path; stack dual of DSHLC plane).

### Language
- `SSHLC`/`SHLCY`/`SSHLCF` — TOS = (TOS≪1)|cin(CARRY); CARRY = old MSB
- `SSHRC`/`SHRCY`/`SSHRCF` — TOS = (TOS≫1)|(cin≪MSB); CARRY = old LSB
- `DSHLCC`/`PAIRSHLCC`/`DSHLCF` — pair shift-left through shared CARRY
- `DSHRCC`/`PAIRSHRCC`/`DSHRCF` — pair shift-right through shared CARRY
- Proof `297_sshlc_sshrc.cubalc`

### Prior
See 1.12.273-universal.

## 1.12.273-universal — 2026-08-03

### Direction
- Algocube digit **4** → dual-stack zero/sign predicates (dual of SZ/SNZ/S0LT/S0GT; complete after DSIGN).

### Language
- `D0EQ`/`DZ`/`PAIR0EQ` — a b → (a==0?1:0) (b==0?1:0)
- `D0NE`/`DNZ`/`PAIR0NE` — a b → (a!=0?1:0) (b!=0?1:0)
- `D0LT`/`PAIR0LT` — a b → (a<0?1:0) (b<0?1:0)
- `D0GT`/`PAIR0GT`/`DPOS` — a b → (a>0?1:0) (b>0?1:0)
- Proof `296_d0eq_d0ne.cubalc`

### Prior
See 1.12.272-universal.

## 1.12.272-universal — 2026-08-03

### Direction
- Algocube digit **2** → multiword imm add-with-carry / sub-with-borrow (imm dual of SADDC/SSUBB + DADDC/DSUBB).

### Language
- `SADDCN`/`ADDCN`/`SADCIMM` n — TOS = TOS+n+cin(CARRY); update CARRY
- `SSUBBN`/`SUBBN`/`SSBBIMM` n — TOS = TOS−n−bin(BORROW|CARRY); update BORROW/CARRY
- `DADDCN`/`PAIRADDCN` n — a b → (a+n+cin) (b+n+cin); CARRY = any cout
- `DSUBBN`/`PAIRSUBBN` n — a b → (a−n−bin) (b−n−bin); BORROW=CARRY = any bout
- Proof `295_saddcn_ssubbn.cubalc`

### Prior
See 1.12.271-universal.

## 1.12.271-universal — 2026-08-03

### Direction
- Algocube digit **6** → imm saturating divide (completes sat energy ALU plane + dual-stack form).

### Language
- `SATDIV(a,b)` — trunc-toward-zero; `/0→0`; `LONG_MIN/-1→LONG_MAX`
- `SSATDIVN`/`SATDIVN`/`SDIVSATN` n — TOS = sat(TOS/n)
- `DSATDIVN`/`PAIRSATDIVN`/`DDIVSATN` n — a b → sat(a/n) sat(b/n)
- Proof `294_ssatdivn_dsatdivn.cubalc`

### Prior
See 1.12.270-universal.

## 1.12.270-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack imm saturating ALU (dual of SSATADDN/SSATSUBN/SSATMULN).

### Language
- `DSATADDN`/`PAIRSATADDN`/`DADDSATN` n — a b → sat(a+n) sat(b+n)
- `DSATSUBN`/`PAIRSATSUBN`/`DSUBSATN` n — a b → sat(a−n) sat(b−n)
- `DSATMULN`/`PAIRSATMULN`/`DMULSATN` n — a b → sat(a*n) sat(b*n)
- Proof `293_dsataddn_dsatsubn.cubalc`

### Prior
See 1.12.269-universal.

## 1.12.269-universal — 2026-08-03

### Direction
- Algocube digit **1** → stack imm saturating ALU (imm dual of SSATADD/SSATSUB/SSATMUL).

### Language
- `SSATADDN`/`SATADDN`/`SADDSATN` n — TOS = sat(TOS+n) to LONG_MIN..LONG_MAX
- `SSATSUBN`/`SATSUBN`/`SSUBSATN` n — TOS = sat(TOS−n)
- `SSATMULN`/`SATMULN`/`SMULSATN` n — TOS = sat(TOS*n)
- Proof `292_ssataddn_ssatsubn.cubalc`

### Prior
See 1.12.268-universal.

## 1.12.268-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack imm overflow predicates (dual of SADDOVFN/SSUBOVFN/SMULOVFN).

### Language
- `DADDOVFN`/`PAIRADDOVFN` n — a b → ovf(a+n) ovf(b+n) as 0/1
- `DSUBOVFN`/`PAIRSUBOVFN` n — a b → ovf(a−n) ovf(b−n) as 0/1
- `DMULOVFN`/`PAIRMULOVFN` n — a b → ovf(a*n) ovf(b*n) as 0/1
- Proof `291_daddovfn_dsubovfn.cubalc`

### Prior
See 1.12.267-universal.

## 1.12.267-universal — 2026-08-03

### Direction
- Algocube digit **0** → stack imm overflow predicates (imm dual of SADDOVF/SSUBOVF/SMULOVF).

### Language
- `SADDOVFN`/`ADDOVFN` n — TOS = 1 if TOS+n signed overflow else 0
- `SSUBOVFN`/`SUBOVFN` n — TOS = 1 if TOS−n signed overflow else 0
- `SMULOVFN`/`MULOVFN` n — TOS = 1 if TOS*n signed overflow else 0
- Proof `290_saddovfn_ssubovfn.cubalc`

### Prior
See 1.12.266-universal.

## 1.12.266-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack imm reverse ALU (dual of SSUBFROMN/SDIVFROMN/SMODFROMN).

### Language
- `DSUBFROMN`/`PAIRSUBFROMN`/`DRSUBN` n — a b → (n−a) (n−b)
- `DDIVFROMN`/`PAIRDIVFROMN`/`DRDIVN` n — a b → (n/a) (n/b) (lane 0 → 0)
- `DMODFROMN`/`PAIRMODFROMN`/`DREMFROMN` n — a b → (n%a) (n%b) (lane 0 → 0)
- Proof `289_dsubfromn_ddivfromn.cubalc`

### Prior
See 1.12.265-universal.

## 1.12.265-universal — 2026-08-03

### Direction
- Algocube digit **9** → stack imm reverse ALU plane (n op TOS after SSUBN/SDIVN/SMODN).

### Language
- `SSUBFROMN`/`SRSUBN`/`RSUBN` n — TOS = n − TOS
- `SDIVFROMN`/`SRDIVN`/`RDIVN` n — TOS = n / TOS (TOS==0 → 0)
- `SMODFROMN`/`SRMODN`/`REMFROMN` n — TOS = n % TOS (TOS==0 → 0)
- Proof `288_ssubfromn_sdivfromn.cubalc`

### Prior
See 1.12.264-universal.

## 1.12.264-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack imm range predicates (dual of SBETWEENN/SWITHINN; complete DCLAMPN plane).

### Language
- `DBETWEENN`/`PAIRBETWEENN`/`DINRANGEN` lo hi — a b → (a∈[lo,hi]?1:0) (b∈[lo,hi]?1:0); swap lo/hi
- `DWITHINN`/`PAIRWITHINN` lo hi — a b → (lo≤a<hi?1:0) (lo≤b<hi?1:0); hi exclusive
- Proof `287_dbetweenn_dwithinn.cubalc`

### Prior
See 1.12.263-universal.

## 1.12.263-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack imm inverted ANDN plane (dual of SNANDNI/SNORNI/SXNORNI; completes ANDN ladder).

### Language
- `DNANDNI`/`PAIRNANDNI` n — a b → ~(a&~n) ~(b&~n)
- `DNORNI`/`PAIRNORNI` n — a b → ~(a|~n) ~(b|~n)
- `DXNORNI`/`PAIRXNORNI`/`DEQUIVNI` n — a b → ~(a^~n) ~(b^~n) (equiv `DXORI`)
- Proof `286_dnandni_dnorni.cubalc`

### Prior
See 1.12.262-universal.

## 1.12.262-universal — 2026-08-03

### Direction
- Algocube digit **9** → stack imm inverted ANDN plane (after SANDNI + SNANDI; dual pending DNANDNI).

### Language
- `SNANDNI`/`NANDNI` n — TOS = ~(TOS & ~n) (= ~TOS | n)
- `SNORNI`/`NORNI` n — TOS = ~(TOS | ~n) (= ~TOS & n)
- `SXNORNI`/`XNORNI`/`SEQUIVNI` n — TOS = ~(TOS ^ ~n) (equiv `SXORI`)
- Proof `285_snandni_snorni.cubalc`

### Prior
See 1.12.261-universal.

## 1.12.261-universal — 2026-08-03

### Direction
- Algocube digit **9** → dual-stack immediate ANDN plane (dual of SANDNI/SORNI/SXORNI after DANDI/DNANDI).

### Language
- `DANDNI`/`DBICI`/`PAIRANDNI` n — a b → (a&~n) (b&~n)
- `DORNI`/`PAIRORNI` n — a b → (a|~n) (b|~n)
- `DXORNI`/`PAIRXORNI` n — a b → (a^~n) (b^~n) (equiv `DXNORI`)
- Proof `284_dandni_dorni.cubalc`

### Prior
See 1.12.260-universal.

## 1.12.260-universal — 2026-08-03

### Direction
- Algocube digit **7** → stack immediate ANDN plane (TOS op ~imm after SANDI/SORI/SXORI + SNANDI family).

### Language
- `SANDNI`/`SBICI`/`ANDNI` n — TOS &= ~n (clear bits set in n)
- `SORNI`/`ORNI` n — TOS |= ~n
- `SXORNI`/`XORNI` n — TOS ^= ~n (equiv `SXNORI`)
- Proof `283_sandni_sorni.cubalc`

### Prior
See 1.12.259-universal.

## 1.12.259-universal — 2026-08-03

### Direction
- Algocube digit **0** → dual-stack imm inverted 32-bit word field (dual of SNAND32N; complete dual inverted field ladder).

### Language
- `DNAND32N`/`PAIRNAND32N` field n — word n of each = ~(word & field) & 0xFFFFFFFF (n 0..1)
- `DNOR32N`/`PAIRNOR32N` field n — word n of each = ~(word | field) & 0xFFFFFFFF
- `DXNOR32N`/`PAIRXNOR32N`/`DEQUIV32N` field n — word n of each = ~(word ^ field) & 0xFFFFFFFF
- Proof `282_dnand32n_dnor32n.cubalc`

### Prior
See 1.12.258-universal.

## 1.12.258-universal — 2026-08-03

### Direction
- Algocube digit **1** → stack imm inverted 32-bit word field (NAND/NOR/XNOR after SAND32N; complete stack inverted field ladder).

### Language
- `SNAND32N`/`NAND32N` field n — word n = ~(word & field) & 0xFFFFFFFF (n 0..1)
- `SNOR32N`/`NOR32N` field n — word n = ~(word | field) & 0xFFFFFFFF
- `SXNOR32N`/`XNOR32N`/`SEQUIV32N` field n — word n = ~(word ^ field) & 0xFFFFFFFF
- Proof `281_snand32n_snor32n.cubalc`

### Prior
See 1.12.257-universal.

## 1.12.257-universal — 2026-08-03

### Direction
- Algocube digit **4** → dual-stack imm inverted 4-bit nibble field (dual of SNAND4N/SNOR4N/SXNOR4N).

### Language
- `DNAND4N`/`PAIRNAND4N` field n — nibble n of each = ~(nibble & field) & 0xF (n 0..15)
- `DNOR4N`/`PAIRNOR4N` field n — nibble n of each = ~(nibble | field) & 0xF
- `DXNOR4N`/`PAIRXNOR4N`/`DEQUIV4N` field n — nibble n of each = ~(nibble ^ field) & 0xF
- Proof `280_dnand4n_dnor4n.cubalc`

### Prior
See 1.12.256-universal.

## 1.12.256-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack imm inverted 16-bit halfword field (dual of SNAND16N/SNOR16N/SXNOR16N).

### Language
- `DNAND16N`/`PAIRNAND16N` field n — halfword n of each = ~(half & field) & 0xFFFF (n 0..3)
- `DNOR16N`/`PAIRNOR16N` field n — halfword n of each = ~(half | field) & 0xFFFF
- `DXNOR16N`/`PAIRXNOR16N`/`DEQUIV16N` field n — halfword n of each = ~(half ^ field) & 0xFFFF
- Proof `279_dnand16n_dnor16n.cubalc`

### Prior
See 1.12.255-universal.

## 1.12.255-universal — 2026-08-03

### Direction
- Algocube digit **0** → stack imm inverted 16-bit halfword field (NAND/NOR/XNOR after SAND16N; ladder after SNAND8N).

### Language
- `SNAND16N`/`NAND16N` field n — halfword n = ~(half & field) & 0xFFFF (n 0..3)
- `SNOR16N`/`NOR16N` field n — halfword n = ~(half | field) & 0xFFFF
- `SXNOR16N`/`XNOR16N`/`SEQUIV16N` field n — halfword n = ~(half ^ field) & 0xFFFF
- Proof `278_snand16n_snor16n.cubalc`

### Prior
See 1.12.254-universal.

## 1.12.254-universal — 2026-08-03

### Direction
- Algocube digit **5** → dual-stack imm inverted 8-bit byte field (dual of SNAND8N/SNOR8N/SXNOR8N).

### Language
- `DNAND8N`/`PAIRNAND8N` field n — byte n of each = ~(byte & field) & 0xFF (n 0..7)
- `DNOR8N`/`PAIRNOR8N` field n — byte n of each = ~(byte | field) & 0xFF
- `DXNOR8N`/`PAIRXNOR8N`/`DEQUIV8N` field n — byte n of each = ~(byte ^ field) & 0xFF
- Proof `277_dnand8n_dnor8n.cubalc`

### Prior
See 1.12.253-universal.

## 1.12.253-universal — 2026-08-03

### Direction
- Algocube digit **8** → stack imm inverted 8-bit byte field (NAND/NOR/XNOR after SAND8N; ladder after SNAND4N).

### Language
- `SNAND8N`/`NAND8N` field n — byte n = ~(byte & field) & 0xFF (n 0..7)
- `SNOR8N`/`NOR8N` field n — byte n = ~(byte | field) & 0xFF
- `SXNOR8N`/`XNOR8N`/`SEQUIV8N` field n — byte n = ~(byte ^ field) & 0xFF
- Proof `276_snand8n_snor8n.cubalc`

### Prior
See 1.12.252-universal.

## 1.12.252-universal — 2026-08-03

### Direction
- Algocube digit **9** → stack imm inverted 4-bit nibble field (NAND/NOR/XNOR after SAND4N plane).

### Language
- `SNAND4N`/`NAND4N` field n — nibble n = ~(nibble & field) & 0xF (n 0..15)
- `SNOR4N`/`NOR4N` field n — nibble n = ~(nibble | field) & 0xF
- `SXNOR4N`/`XNOR4N`/`SEQUIV4N` field n — nibble n = ~(nibble ^ field) & 0xF
- Proof `275_snand4n_snor4n.cubalc`

### Prior
See 1.12.251-universal.

## 1.12.251-universal — 2026-08-03

### Direction
- Algocube digit **4** → dual-stack imm 4-bit nibble field bitwise (dual of SAND4N/SOR4N/SXOR4N; complete dual field bitwise ladder).

### Language
- `DAND4N`/`PAIRAND4N` field n — nibble n of each &= field (n 0..15)
- `DOR4N`/`PAIROR4N` field n — nibble n of each |= field
- `DXOR4N`/`PAIRXOR4N`/`DFLIP4N` field n — nibble n of each ^= field
- Proof `274_dand4n_dor4n.cubalc`

### Prior
See 1.12.250-universal.

## 1.12.250-universal — 2026-08-03

### Direction
- Algocube digit **7** → stack imm 4-bit nibble field bitwise merge (complete nibble/byte/half/word bitwise ladder).

### Language
- `SAND4N`/`AND4N` field n — nibble n of TOS &= field (n 0..15)
- `SOR4N`/`OR4N` field n — nibble n of TOS |= field
- `SXOR4N`/`XOR4N`/`SFLIP4N` field n — nibble n of TOS ^= field
- Proof `273_sand4n_sor4n.cubalc`

### Prior
See 1.12.249-universal.

## 1.12.249-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack imm 8-bit byte field bitwise (dual of SAND8N/SOR8N/SXOR8N).

### Language
- `DAND8N`/`PAIRAND8N` field n — byte n of each &= field (n 0..7)
- `DOR8N`/`PAIROR8N` field n — byte n of each |= field
- `DXOR8N`/`PAIRXOR8N`/`DFLIP8N` field n — byte n of each ^= field
- Proof `272_dand8n_dor8n.cubalc`

### Prior
See 1.12.248-universal.

## 1.12.248-universal — 2026-08-03

### Direction
- Algocube digit **8** → stack imm 8-bit byte field bitwise merge (complete byte plane after SAND16N/SAND32N).

### Language
- `SAND8N`/`AND8N` field n — byte n of TOS &= field (n 0..7)
- `SOR8N`/`OR8N` field n — byte n of TOS |= field
- `SXOR8N`/`XOR8N`/`SFLIP8N` field n — byte n of TOS ^= field
- Proof `271_sand8n_sor8n.cubalc`

### Prior
See 1.12.247-universal.

## 1.12.247-universal — 2026-08-03

### Direction
- Algocube digit **8** → dual-stack imm 16-bit halfword field bitwise (dual of SAND16N/SOR16N/SXOR16N).

### Language
- `DAND16N`/`PAIRAND16N` field n — halfword n of each &= field (n 0..3)
- `DOR16N`/`PAIROR16N` field n — halfword n of each |= field
- `DXOR16N`/`PAIRXOR16N`/`DFLIP16N` field n — halfword n of each ^= field
- Proof `270_dand16n_dor16n.cubalc`

### Prior
See 1.12.246-universal.

## 1.12.246-universal — 2026-08-03

### Direction
- Algocube digit **0** → stack imm 16-bit halfword field bitwise merge (complete halfword plane after SSET16N/SAND32N).

### Language
- `SAND16N`/`AND16N` field n — halfword n of TOS &= field (n 0..3)
- `SOR16N`/`OR16N` field n — halfword n of TOS |= field
- `SXOR16N`/`XOR16N`/`SFLIP16N` field n — halfword n of TOS ^= field
- Proof `269_sand16n_sor16n.cubalc`

### Prior
See 1.12.245-universal.

## 1.12.245-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack imm 32-bit field bitwise merge (dual of SAND32N/SOR32N/SXOR32N).

### Language
- `DAND32N`/`PAIRAND32N` field n — word n of each &= field (n 0..1)
- `DOR32N`/`PAIROR32N` field n — word n of each |= field
- `DXOR32N`/`PAIRXOR32N`/`DFLIP32N` field n — word n of each ^= field
- Proof `268_dand32n_dor32n.cubalc`

### Prior
See 1.12.244-universal.

## 1.12.244-universal — 2026-08-03

### Direction
- Algocube digit **0** → stack imm 32-bit field bitwise merge (AND/OR/XOR into word n after SSET32N).

### Language
- `SAND32N`/`AND32N` field n — word n of TOS &= field (n 0..1)
- `SOR32N`/`OR32N` field n — word n of TOS |= field
- `SXOR32N`/`XOR32N`/`SFLIP32N` field n — word n of TOS ^= field
- Proof `267_sand32n_sor32n.cubalc`

### Prior
See 1.12.243-universal.

## 1.12.243-universal — 2026-08-03

### Direction
- Algocube digit **8** → dual-stack imm 32-bit field (complete dual of SGET32N ladder).

### Language
- `DGET32N`/`PAIRGET32N`/`DWORD32N` n — extract LE 32-bit word n of each of pair (n 0..1)
- `DSET32N`/`PAIRSET32N`/`DPUT32N` field n — deposit low 32 bits into word n of each
- `DCLR32N`/`PAIRCLR32N`/`DZAP32N` n — clear word n of each
- Proof `266_dget32n_dset32n.cubalc`

### Prior
See 1.12.242-universal.

## 1.12.242-universal — 2026-08-03

### Direction
- Algocube digit **5** → stack imm 32-bit field (complete nibble/byte/halfword/word ladder).

### Language
- `SGET32N`/`SWORD32N`/`GET32N` n — extract LE 32-bit word n of TOS (n 0..1)
- `SSET32N`/`SET32N`/`PUT32N` field n — deposit low 32 bits into word n of TOS
- `SCLR32N`/`CLR32N`/`ZAP32N` n — clear word n of TOS
- Proof `265_sget32n_sset32n.cubalc`

### Prior
See 1.12.241-universal.

## 1.12.241-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack imm 16-bit halfword field (complete dual of SWORDN ladder).

### Language
- `DWORDN`/`PAIRWORDN`/`DGET16N`/`DHALFN` · `DSET16N`/`PAIRSET16N`/`DSETWORDN`/`DPUT16N` · `DCLR16N`/`PAIRCLR16N`/`DZAP16N`/`DCLRWORDN`
- Dual of stack SWORDN/SSET16N/SCLR16N: extract/deposit/clear LE halfword n on top two stack cells; n clamped 0..3
- Proof `264_dwordn_dset16n.cubalc`

### Prior
See 1.12.240-universal.

## 1.12.240-universal — 2026-08-03

### Direction
- Algocube digit **9** → stack imm 16-bit halfword field (complete nibble/byte ladder).

### Language
- `SWORDN`/`SGET16N`/`SHALFN` n — extract LE 16-bit halfword n of TOS (n 0..3)
- `SSET16N`/`SET16N` field n — deposit low 16 bits into halfword n of TOS
- `SCLR16N`/`CLR16N` n — clear halfword n of TOS
- Proof `263_swordn_sset16n.cubalc`

### Prior
See 1.12.239-universal.

## 1.12.239-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack imm nibble field (dual of SNIBN/SSETNIBN/SCLRNIBN; pair after DBYTEN).

### Language
- `DNIBN`/`2NIBN`/`PAIRNIBN` n — extract LE nibble n of each of pair
- `DSETNIBN`/`2SETNIBN` field n — deposit field into nibble n of each
- `DCLRNIBN`/`2CLRNIBN` n — clear nibble n of each
- Proof `262_dnibn_dsetnibn.cubalc`

### Prior
See 1.12.238-universal.

## 1.12.238-universal — 2026-08-03

### Direction
- Algocube digit **5** → dual-stack imm byte field (dual of SBYTEN/SSETBYTEN/SCLRBYTEN).

### Language
- `DBYTEN`/`2BYTEN`/`PAIRBYTEN` n — extract LE byte n of each of pair
- `DSETBYTEN`/`2SETBYTEN` field n — deposit field into byte n of each
- `DCLRBYTEN`/`2CLRBYTEN` n — clear byte n of each
- Proof `261_dbyten_dsetbyten.cubalc`

### Prior
See 1.12.237-universal.

## 1.12.237-universal — 2026-08-03

### Direction
- Algocube digit **4** → stack imm nibble field (imm dual of SNIB/SSETNIB; control-word plane after SBYTEN).

### Language
- `SNIBN`/`GETNIBN` n — extract little-endian nibble n of TOS
- `SSETNIBN`/`SETNIBN` field n — deposit low 4 bits into nibble n of TOS
- `SCLRNIBN`/`CLRNIBN` n — clear nibble n of TOS
- Proof `260_snibn_ssetnibn.cubalc`

### Prior
See 1.12.236-universal.

## 1.12.236-universal — 2026-08-03

### Direction
- Algocube digit **1** → stack imm byte field (imm dual of SBYTE/SSETBYTE; word data-path).

### Language
- `SBYTEN`/`GETBYTEN` n — extract little-endian byte n of TOS
- `SSETBYTEN`/`SETBYTEN` field n — deposit low 8 bits into byte n of TOS
- `SCLRBYTEN`/`CLRBYTEN` n — clear byte n of TOS
- Proof `259_sbyten_ssetbyten.cubalc`

### Prior
See 1.12.235-universal.

## 1.12.235-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack immediate modular inv/pow/div (dual of SMODINVN/SPOWMODN/SMODDIVN).

### Language
- `DMODINVN`/`2MODINVN`/`PAIRMODINVN` m — pair inverses mod m
- `DPOWMODN`/`2POWMODN`/`PAIRPOWMODN` exp m — pair pow mod m
- `DMODDIVN`/`2MODDIVN`/`PAIRMODDIVN` c m — pair · c⁻¹ mod m
- Proof `258_dmodinvn_dpowmodn.cubalc`

### Prior
See 1.12.234-universal.

## 1.12.234-universal — 2026-08-03

### Direction
- Algocube digit **2** → stack immediate modular inv/pow/div (complete SADDMODN plane after inv/pow stack forms).

### Language
- `SMODINVN`/`SINVMODN`/`MODINVN` m — TOS = TOS⁻¹ mod m (0 if none)
- `SPOWMODN`/`POWMODN` exp m — TOS = TOS^exp mod m
- `SMODDIVN`/`MODDIVN` b m — TOS = TOS · b⁻¹ mod m
- Proof `257_smodinvn_spowmodn.cubalc`

### Prior
See 1.12.233-universal.

## 1.12.233-universal — 2026-08-03

### Direction
- Algocube digit **3** → stack immediate clamp/range (imm dual of SCLAMP/SBETWEEN/SWITHIN; pair of DCLAMPN).

### Language
- `SCLAMPN`/`STACKCLAMPN`/`CLAMPN` lo hi — clamp TOS into [lo,hi]
- `SBETWEENN`/`SINRANGEN`/`BETWEENN` lo hi — TOS in [lo,hi] inclusive → 0/1
- `SWITHINN`/`STACKWITHINN`/`WITHINN` lo hi — lo ≤ TOS < hi → 0/1
- Proof `256_sclampn_swithinn.cubalc`

### Prior
See 1.12.232-universal.

## 1.12.232-universal — 2026-08-03

### Direction
- Algocube digit **3** → stack inverted high-n mask (dual of dual-stack DNANDHN/DNORHN/DXNORHN; complete SNANDMN plane).

### Language
- `SNANDHN`/`STACKNANDHN`/`NANDHN` n — TOS = ~(TOS & high-n mask)
- `SNORHN`/`STACKNORHN`/`NORHN` n — TOS = ~(TOS | high-n mask)
- `SXNORHN`/`STACKXNORHN`/`XNORHN` n — TOS = ~(TOS ^ high-n mask)
- Proof `255_snandhn_sxnorhn.cubalc`

### Prior
See 1.12.231-universal.

## 1.12.231-universal — 2026-08-03

### Direction
- Algocube digit **3** → stack inverted low-n mask (dual of dual-stack DNANDMN/DNORMN/DXNORMN; invert SANDMN plane).

### Language
- `SNANDMN`/`STACKNANDMN`/`NANDMN` n — TOS = ~(TOS & low-n mask)
- `SNORMN`/`STACKNORMN`/`NORMN` n — TOS = ~(TOS | low-n mask)
- `SXNORMN`/`STACKXNORMN`/`XNORMN` n — TOS = ~(TOS ^ low-n mask)
- Proof `254_snandmn_sxnormn.cubalc`

### Prior
See 1.12.230-universal.

## 1.12.230-universal — 2026-08-03

### Direction
- Algocube digit **7** → stack clear + high-mask (dual of dual-stack DCLRLN/DCLRHN/DHMASKN).

### Language
- `SCLRMN`/`STACKCLRMN`/`CLRLN` n — clear low n bits of TOS
- `SCLRHN`/`STACKCLRHN`/`CLRHN` n — clear high n bits of TOS
- `SHMASKN`/`STACKHMASKN`/`HMASKN` n — TOS = high-n-bit mask
- Proof `253_sclrmn_shmaskn.cubalc`

### Prior
See 1.12.229-universal.


## 1.12.229-universal — 2026-08-03

### Direction
- Algocube digit **1** → stack high-n bitfield metrics (dual of dual-stack DPOPHN; complete SPOPMN low plane).

### Language
- `SPOPHN`/`STACKPOPHN`/`SONESHN` n — popcount of high n bits of TOS
- `SANYHN`/`STACKANYHN`/`ANYHN` n — any high-n bit set → 0/1
- `SALLHN`/`STACKALLHN`/`ALLHN` n — all high-n bits set → 0/1 (n=0 vacuous 1)
- Proof `252_spophn_sallhn.cubalc`

### Prior
See 1.12.228-universal.


## 1.12.228-universal — 2026-08-03

### Direction
- Algocube digit **2** → stack immediate modular ALU (dual of dual-stack DADDMODN/DSUBMODN/DMULMODN).

### Language
- `SADDMODN`/`STACKADDMODN`/`ADDMODN` k m — TOS = `(TOS+k) mod m`
- `SSUBMODN`/`STACKSUBMODN`/`SUBMODN` k m — TOS = `(TOS-k) mod m`
- `SMULMODN`/`STACKMULMODN`/`MULMODN` k m — TOS = `(TOS*k) mod m`
- Proof `251_saddmodn_smulmodn.cubalc`

### Prior
See 1.12.227-universal.


## 1.12.227-universal — 2026-08-03

### Direction
- Algocube digit **8** → dual-stack word unpack (complete DLO32/DHI32/DPACK32 plane).

### Language
- `DUNPACK32`/`2UNPACK32`/`PAIRUNPACK32` — pair words → hi32,hi32,lo32,lo32 (inverse of DPACK32)
- Aliases: `DUNPACKDW` `DWORDSPLIT` `2WORDSPLIT`
- Proof `250_dunpack32.cubalc`

### Prior
See 1.12.226-universal.


## 1.12.226-universal — 2026-08-03

### Direction
- Algocube digit **1** → stack low-n bitfield metrics (dual of dual-stack DPOPMN/DANYMN/DALLMN).

### Language
- `SPOPMN`/`STACKPOPMN`/`SONESMN` n — popcount of low n bits of TOS
- `SANYMN`/`STACKANYMN`/`ANYMN` n — any low-n bit set → 0/1
- `SALLMN`/`STACKALLMN`/`ALLMN` n — all low-n bits set → 0/1 (n=0 vacuous 1)
- Proof `249_spopmn_sallmn.cubalc`

### Prior
See 1.12.225-universal.


## 1.12.225-universal — 2026-08-03

### Direction
- Algocube digit **5** → stack high-n mask plane (dual of dual-stack DANDHN/DORHN/DXORHN; complete SANDMN low plane).

### Language
- `SANDHN`/`STACKANDHN`/`KEEPHN` n — TOS &= high-n mask (keep high n bits)
- `SORHN`/`STACKORHN`/`SETHN` n — TOS |= high-n mask (set high n bits)
- `SXORHN`/`STACKXORHN`/`FLIPHN` n — TOS ^= high-n mask (toggle high n bits)
- Proof `248_sandhn_sorhn.cubalc`

### Prior
See 1.12.224-universal.


## 1.12.224-universal — 2026-08-03

### Direction
- Algocube digit **6** → stack low-n mask plane (dual of dual-stack DANDMN/DORMN/DXORMN energy bit-fill).

### Language
- `SANDMN`/`STACKANDMN`/`KEEPLN` n — TOS &= low-n mask (keep low n bits)
- `SORMN`/`STACKORMN`/`SETLN` n — TOS |= low-n mask (set low n bits)
- `SXORMN`/`STACKXORMN`/`FLIPLN` n — TOS ^= low-n mask (toggle low n bits)
- Proof `247_sandmn_sormn.cubalc`

### Prior
See 1.12.223-universal.


## 1.12.223-universal — 2026-08-03

### Direction
- Algocube digit **0** → stack high-n field reverse/rotate (complete SBREVN low plane + dual of DBREVHN).

### Language
- `SBREVHN`/`STACKBREVHN`/`BREVHNS` n — reverse high n bits of TOS (low kept)
- `SROLHN`/`STACKROLHN`/`ROLHNS` n — rotate left by 1 within high n bits of TOS
- `SRORHN`/`STACKRORHN`/`RORHNS` n — rotate right by 1 within high n bits of TOS
- Proof `246_sbrevhn_srolhn.cubalc`

### Prior
See 1.12.222-universal.


## 1.12.222-universal — 2026-08-03

### Direction
- Algocube digit **4** → stack low-n field reverse/rotate (dual of dual-stack DBREVN plane).

### Language
- `SBREVN`/`STACKBREVN`/`BREVNS` n — reverse low n bits of TOS (high kept)
- `SROLBN`/`STACKROLBN`/`ROLBNS` n — rotate left by 1 within low n bits of TOS
- `SRORBN`/`STACKRORBN`/`RORBNS` n — rotate right by 1 within low n bits of TOS
- Proof `245_sbrevn_srolbn.cubalc`

### Prior
See 1.12.221-universal.


## 1.12.221-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack high-n field reverse/rotate (complete low-n DBREVN/DROLBN/DRORBN plane).

### Language
- `DBREVHN`/`2BREVHN`/`PAIRBREVHN` n — reverse high n bits (low kept)
- `DROLHN`/`2ROLHN` n — rotate left by 1 within high n bits
- `DRORHN`/`2RORHN` n — rotate right by 1 within high n bits
- Proof `244_dbrevhn_drolhn.cubalc`

### Prior
See 1.12.220-universal.


# Changelog

## 1.12.220-universal — 2026-08-03

### Direction
- Algocube digit **5** → dual-stack low-n field reverse/rotate (bitfield path).

### Language
- `DBREVN`/`2BREVN`/`PAIRBREVN` n — reverse low n bits (high kept)
- `DROLBN`/`2ROLBN` n — rotate left by 1 within low n bits
- `DRORBN`/`2RORBN` n — rotate right by 1 within low n bits
- Proof `243_dbrevn_drolbn.cubalc`

### Prior
See 1.12.219-universal.


## 1.12.219-universal — 2026-08-03

### Direction
- Algocube digit **9** → dual-stack high-n bitfield metrics (complete DPOPMN/DANYMN/DALLMN low plane).

### Language
- `DPOPHN`/`DONESHN`/`2POPHN` n — pairwise popcount of high n bits
- `DANYHN`/`2ANYHN`/`PAIRANYHN` n — pairwise any high-n bit set → 0/1
- `DALLHN`/`2ALLHN`/`PAIRALLHN` n — pairwise all high-n bits set → 0/1
- Proof `242_dpophn_dallhn.cubalc`

### Prior
See 1.12.218-universal.


## 1.12.218-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack immediate modular ALU (complete DADDMOD/DMULMOD stack forms).

### Language
- `DADDMODN`/`2ADDMODN` k m — pairwise `(x+k) mod m`
- `DSUBMODN`/`2SUBMODN` k m — pairwise `(x-k) mod m`
- `DMULMODN`/`2MULMODN` k m — pairwise `(x*k) mod m`
- Proof `241_daddmodn_dmulmodn.cubalc`

### Prior
See 1.12.217-universal.


## 1.12.217-universal — 2026-08-03

### Direction
- Algocube digit **5** → dual-stack low-n bitfield metrics (popcount / any / all).

### Language
- `DPOPMN`/`DONESMN`/`2POPMN` n — pairwise popcount of low n bits
- `DANYMN`/`2ANYMN`/`PAIRANYMN` n — pairwise `(x & mask) != 0` → 0/1
- `DALLMN`/`2ALLMN`/`PAIRALLMN` n — pairwise `(x & mask) == mask` → 0/1
- Proof `240_dpopmn_dallmn.cubalc`

### Prior
See 1.12.216-universal.


## 1.12.216-universal — 2026-08-03

### Direction
- Algocube digit **0** → dual-stack inverted high-n mask plane (complete DANDHN/DORHN/DXORHN + DNANDMN family).

### Language
- `DNANDHN`/`2NANDHN`/`PAIRNANDHN` n — pairwise `~(x & himask)`
- `DNORHN`/`2NORHN`/`PAIRNORHN` n — pairwise `~(x | himask)`
- `DXNORHN`/`DEQUIVHN`/`2XNORHN` n — pairwise `~(x ^ himask)`
- Proof `239_dnandhn_dxnorhn.cubalc`

### Prior
See 1.12.215-universal.


## 1.12.215-universal — 2026-08-03

### Direction
- Algocube digit **0** → dual-stack inverted low-n mask plane (complete DANDMN/DORMN/DXORMN).

### Language
- `DNANDMN`/`2NANDMN`/`PAIRNANDMN` n — pairwise `~(x & lowmask)`
- `DNORMN`/`2NORMN`/`PAIRNORMN` n — pairwise `~(x | lowmask)`
- `DXNORMN`/`DEQUIVMN`/`2XNORMN` n — pairwise `~(x ^ lowmask)`
- Proof `238_dnandmn_dxnormn.cubalc`

### Prior
See 1.12.214-universal.


## 1.12.214-universal — 2026-08-03

### Direction
- Algocube digit **5** → dual-stack high-plane or/xor/clear (complete DHMASKN/DANDHN bitfield).

### Language
- `DORHN`/`DSETHN`/`2ORHN` n — pairwise set high n bits: `x |= himask`
- `DXORHN`/`DFLIPHN`/`2XORHN` n — pairwise toggle high n bits: `x ^= himask`
- `DCLRHN`/`2CLRHN`/`PAIRCLRHN` n — pairwise clear high n bits: `x &= ~himask`
- Proof `237_dorhn_dclrh.cubalc`

### Prior
See 1.12.213-universal.


## 1.12.213-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack data-path high-mask + clear-low (complete DMASKN/DANDMN low plane).

### Language
- `DHMASKN`/`2HMASKN`/`PAIRHMASKN` n — both lanes ← high-n-bit mask
- `DANDHN`/`DKEEPHN`/`2ANDHN` n — pairwise keep high n bits: `x &= himask`
- `DCLRLN`/`2CLRLN`/`PAIRCLRLN` n — pairwise clear low n bits: `x &= ~lowmask`
- Proof `236_dhmaskn_dclrln.cubalc`

### Prior
See 1.12.212-universal.


## 1.12.212-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack boolean control plane (dual of SLAND/SLOR/SLXOR/SIMP).

### Language
- `DLAND`/`2LAND`/`PAIRLAND` — pairwise `(a&&c)?1:0` / `(b&&d)?1:0`
- `DLOR`/`2LOR`/`PAIRLOR` — pairwise logical OR → 0/1
- `DLXOR`/`2LXOR`/`PAIRLXOR` — pairwise logical XOR → 0/1
- `DIMP`/`DIMPLY`/`2IMP` — pairwise imply `(!a||c)` → 0/1
- Proof `235_dland_dimp.cubalc`

### Prior
See 1.12.211-universal.


## 1.12.211-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack data-path imm deposit + low-n or/xor (complete DMASKN/DANDMN/DBEXTN; energy bit-fill/toggle).

### Language
- `DBDEPN`/`2BDEPN`/`PAIRBDEPN` field pos — pairwise deposit low 8 bits of field at pos (dual of SBDEPN)
- `DORMN`/`DSETLN`/`2ORMN` n — pairwise set low n bits: `x |= mask`
- `DXORMN`/`DFLIPLN`/`2XORMN` n — pairwise toggle low n bits: `x ^= mask`
- Proof `234_dbdepn_dormn.cubalc`

### Prior
See 1.12.210-universal.


## 1.12.210-universal — 2026-08-03

### Direction
- Algocube digit **9** → dual-stack data-path imm mask/extract (dual of SMASKN/SBEXTN).

### Language
- `DMASKN`/`2MASKN`/`PAIRMASKN` n — both lanes ← low-n-bit mask `(1<<n)-1`
- `DANDMN`/`DKEEPLN`/`2ANDMN` n — pairwise keep low n bits: `x &= mask`
- `DBEXTN`/`2BEXTN` pos width — pairwise field extract at pos
- Proof `233_dmaskn_dbextn.cubalc`

### Prior
See 1.12.209-universal.



## 1.12.209-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack control-flag flip (complete DSETBN/DCLRBN/DBTESTN; dual of SFLIPBN).

### Language
- `DFLIPBN`/`2FLIPBN`/`PAIRFLIPBN` n — pairwise toggle bit n: `x ^= 1<<n` (n clamped 0..63)
- Proof `232_dflipbn.cubalc`

### Prior
See 1.12.208-universal.



## 1.12.208-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack immediate compare complete (dual of SNEN/SLENN/SGENN).

### Language
- `DNEN`/`2NEN`/`PAIRNEN` n — pairwise `(x!=n)?1:0`
- `DLENN`/`DLEQN`/`2LENN` n — pairwise `(x<=n)?1:0`
- `DGENN`/`DGEQN`/`2GENN` n — pairwise `(x>=n)?1:0`
- Proof `231_dnen_dgenn.cubalc`

### Prior
See 1.12.207-universal.



## 1.12.207-universal — 2026-08-03

### Direction
- Algocube digit **8** → depth-6 tuck (complete plane after 6ROT/6OVER).

### Language
- `6TUCK`/`TUCK6`/`HTUCK` — copy TOS under top 5: `a b c d e f → f a b c d e f`
- Proof `230_6tuck.cubalc`

### Prior
See 1.12.206-universal.



## 1.12.206-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack immediate compare (dual of SEQN/SLTN/SGTN).

### Language
- `DEQN`/`2EQN`/`PAIREQN` n — pairwise `(x==n)?1:0`
- `DLTN`/`2LTN`/`PAIRLTN` n — pairwise `(x<n)?1:0`
- `DGTN`/`2GTN`/`PAIRGTN` n — pairwise `(x>n)?1:0`
- Proof `229_deqn_dgtn.cubalc`

### Prior
See 1.12.205-universal.



## 1.12.205-universal — 2026-08-03

### Direction
- Algocube digit **9** → dual-stack immediate min/max/clamp (dual of SMINN/SMAXN).

### Language
- `DMINN`/`2MINN`/`PAIRMINN` n — pairwise `min(x, n)`
- `DMAXN`/`2MAXN`/`PAIRMAXN` n — pairwise `max(x, n)`
- `DCLAMPN`/`2CLAMPN`/`PAIRCLAMPN` lo hi — pairwise clamp into [lo,hi]
- Proof `228_dminn_dclampn.cubalc`

### Prior
See 1.12.204-universal.



## 1.12.204-universal — 2026-08-03

### Direction
- Algocube digit **8** → depth-6 rotate/over (parity with 5-plane after 6SWAP/6NIP).

### Language
- `6ROT`/`ROT6`/`HROT` — rotate top 6: `a b c d e f → b c d e f a`
- `6RROT`/`RROT6`/`HRROT` — reverse-rotate top 6: `a b c d e f → f a b c d e`
- `6OVER`/`OVER6`/`HOVER` — copy under-sextet onto stack (needs depth 12)
- Proof `227_6rot_6over.cubalc`

### Prior
See 1.12.203-universal.



## 1.12.203-universal — 2026-08-03

### Direction
- Algocube digit **0** → dual-stack immediate inverted bitwise (dual of SNANDI/SNORI/SXNORI).

### Language
- `DNANDI`/`2NANDI`/`PAIRNANDI` n — pairwise `x = ~(x & n)`
- `DNORI`/`2NORI`/`PAIRNORI` n — pairwise `x = ~(x | n)`
- `DXNORI`/`2XNORI`/`PAIRXNORI` n — pairwise `x = ~(x ^ n)` (equiv)
- Proof `226_dnandi_dxnori.cubalc`

### Prior
See 1.12.202-universal.


## 1.12.202-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack immediate div/mod (dual of SDIVN/SMODN; complete imm ALU).

### Language
- `DDIVN`/`2DIVN`/`PAIRDIVN` n — pairwise `x /= n` (n==0 → 0 soft)
- `DMODN`/`2MODN`/`PAIRMODN` n — pairwise `x %= n` (n==0 → 0 soft)
- Proof `225_ddivn_dmodn.cubalc`

### Prior
See 1.12.201-universal.


## 1.12.201-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack immediate ALU (dual of SADDN/SSUBN/SMULN).

### Language
- `DADDN`/`2ADDN`/`PAIRADDN` n — pairwise add constant: `a b → (a+n) (b+n)`
- `DSUBN`/`2SUBN`/`PAIRSUBN` n — pairwise sub constant
- `DMULN`/`2MULN`/`PAIRMULN` n — pairwise mul constant
- Proof `224_daddn_dmuln.cubalc`

### Prior
See 1.12.200-universal.


## 1.12.200-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack immediate shift (dual of SSHLN/SSHRN/SSARN).

### Language
- `DSHLN`/`2SHLN`/`PAIRSHLN` n — pairwise logical left shift by n (0..63)
- `DSHRN`/`2SHRN`/`PAIRSHRN` n — pairwise logical right shift by n
- `DSARN`/`2SARN`/`DASHRN` n — pairwise arithmetic right shift by n
- Proof `223_dshln_dsarn.cubalc`

### Prior
See 1.12.199-universal.


## 1.12.199-universal — 2026-08-03

### Direction
- Algocube digit **8** → stack depth-6 plane complete (after 6DUP/6DROP).

### Language
- `6SWAP`/`SWAP6`/`HSWAP` — reverse top 6 stack items
- `6NIP`/`NIP6`/`HNIP` — keep ends of top 6: `a b c d e f → a f`
- Proof `222_6swap_6nip.cubalc`

### Prior
See 1.12.198-universal.


## 1.12.198-universal — 2026-08-03

### Direction
- Algocube digit **0** → dual-stack immediate bitwise mask (dual of SANDI/SORI/SXORI).

### Language
- `DANDI`/`2ANDI`/`PAIRANDI` n — pairwise `x &= n`
- `DORI`/`2ORI`/`PAIRORI` n — pairwise `x |= n`
- `DXORI`/`2XORI`/`PAIRXORI` n — pairwise `x ^= n`
- Proof `221_dandi_dxori.cubalc`

### Prior
See 1.12.197-universal.


## 1.12.197-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack immediate rotate (dual of SROLN/SRORN).

### Language
- `DROLN`/`2ROLN`/`PAIRROLN` n — pairwise rotate-left by n (mod 64)
- `DRORN`/`2RORN`/`PAIRRORN` n — pairwise rotate-right by n (mod 64)
- Proof `220_droln_drorn.cubalc`

### Prior
See 1.12.196-universal.


## 1.12.196-universal — 2026-08-03

### Direction
- Algocube digit **5** → dual-stack immediate bitfield (dual of SSETBN/SBTESTN).

### Language
- `DSETBN`/`2SETBN`/`PAIRSETBN` n — pairwise set bit n: `x |= 1<<n`
- `DCLRBN`/`2CLRBN`/`PAIRCLRBN` n — pairwise clear bit n
- `DBTESTN`/`2BTESTN`/`DBITN` n — pairwise test bit n → 0/1
- Proof `219_dsetbn_dbtestn.cubalc`

### Prior
See 1.12.195-universal.


## 1.12.195-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack shared-flag control gates (complete after per-lane DGATE).

### Language
- `DSHGATE`/`2SHGATE`/`DGATES` — shared-flag gate: `a b f → (f?a:0) (f?b:0)`
- `DZEROIF`/`2ZEROIF`/`DZAPIF` — shared-flag zero-if: `a b f → (f?0:a) (f?0:b)`
- Proof `218_dshgate_dzeroif.cubalc`

### Prior
See 1.12.194-universal.


## 1.12.194-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack control rotate conditionals (complete after DOVERIF/DTUCKIF).

### Language
- `DROTIF`/`2ROTIF`/`DQROT` — conditional ROT: `a b c f → b c a` if f else `a b c`
- `DRROTIF`/`2RROTIF`/`DQRROT` — conditional RROT: `a b c f → c a b` if f else `a b c`
- Proof `217_drotif_drrotif.cubalc`

### Prior
See 1.12.193-universal.


## 1.12.193-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack control combinator conditionals (after DSWAPIF/DDUPIF).

### Language
- `DOVERIF`/`2OVERIF`/`DQOVER` — conditional OVER: `a b f → a b a` if f else `a b`
- `DTUCKIF`/`2TUCKIF`/`DQTUCK` — conditional TUCK: `a b f → b a b` if f else `a b`
- Proof `216_doverif_dtuckif.cubalc`

### Prior
See 1.12.192-universal.


## 1.12.192-universal — 2026-08-03

### Direction
- Algocube digit **4** → dual-stack control-word unpack (complete after DPACK8/16).

### Language
- `DUNPACK8`/`2UNPACK8`/`DBYTSPLIT` — inverse of `DPACK8`: `x y → hi8_x hi8_y lo8_x lo8_y`
- `DUNPACK16`/`2UNPACK16`/`DHALFSPLIT` — inverse of `DPACK16`: `x y → hi16_x hi16_y lo16_x lo16_y`
- Proof `215_dunpack8_dunpack16.cubalc`

### Prior
See 1.12.191-universal.


## 1.12.191-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack energy soft activations (after residual/step).

### Language
- `DLEAKY`/`2LEAKY`/`DLEAKYRELU` — pairwise leaky ReLU (`x≥0 → x`, else `x/4`)
- `DSOFTSIGN`/`2SOFTSIGN`/`DSOFTSGN` — pairwise soft-sign on energy plane `±100·|x|/(100+|x|)`
- Proof `214_dleaky_dsoftsign.cubalc`

### Prior
See 1.12.190-universal.


## 1.12.190-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack energy residual + step activation.

### Language
- `DDIFFSQ`/`2DIFFSQ`/`DSQDIFF` — pairwise squared difference `(a-c)²` / `(b-d)²`
- `DSTEP`/`2STEP`/`DHEAVI` — pairwise unit-step / Heaviside (`x>0 → 1`, else 0)
- Proof `213_ddiffsq_dstep.cubalc`

### Prior
See 1.12.189-universal.


## 1.12.189-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack arithmetic numthy (möbius / radical / square-free).

### Language
- `DMOBIUS`/`2MOBIUS`/`DMU` — pairwise Möbius μ(n) (`n<=0 → 0`; square factor → 0)
- `DRAD`/`2RAD`/`DRADICAL` — pairwise radical (product of distinct primes)
- `DSQFREE`/`2SQFREE`/`DISSQFREE` — pairwise square-free predicate (0/1)
- Proof `212_dmobius_drad_dsqfree.cubalc`

### Prior
See 1.12.188-universal.


## 1.12.188-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack energy unit/complement + power sum.

### Language
- `DCLIP01`/`2CLIP01`/`DUNIT` — pairwise clamp to unit interval `[0,1]`
- `DCOMP100`/`2COMP100`/`DENCOMP` — pairwise energy complement `100-x`
- `DSUMSQ`/`2SUMSQ`/`DSSQ` — pairwise sum-of-squares `a²+c²` / `b²+d²`
- Proof `211_dclip01_dcomp100_dsumsq.cubalc`

### Prior
See 1.12.187-universal.


## 1.12.187-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack nibble plane unpack + fixed-width nibble shifts.

### Language
- `DUNPACK4`/`2UNPACK4`/`DNIBSPLIT` — inverse of `DPACK4`: `x y → hi_x hi_y lo_x lo_y`
- `DSHL4`/`2SHL4` · `DSHR4`/`2SHR4` · `DSAR4`/`2SAR4` — pairwise logical/arithmetic shifts on low 4 bits
- Proof `210_dunpack4_dshl4.cubalc`

### Prior
See 1.12.186-universal.


## 1.12.186-universal — 2026-08-03

### Direction
- Algocube digit **8** → dual-stack nibble rotate/swap (complete fixed-width 4/8/16).

### Language
- `DROL4`/`2ROL4`/`PAIRROL4` — pairwise rotate-left low 4 bits
- `DROR4`/`2ROR4`/`PAIRROR4` — pairwise rotate-right low 4 bits
- `DNIBSWAP`/`2NIBSWAP`/`DSWAPNIB` — pairwise swap high/low nibble of low byte
- Proof `209_drol4_dnibswap.cubalc`

### Prior
See 1.12.185-universal.


## 1.12.185-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack control conditionals (complete after `DKEEPIF`).

### Language
- `DSWAPIF`/`2SWAPIF`/`PAIRSWAPIF` — `a b f →` swap pair if flag (dual of `SSWAPIF`)
- `DDROPIF`/`2DROPIF`/`PAIRDROPIF` — drop pair if flag (inverse of `DKEEPIF`)
- `DDUPIF`/`2DUPIF`/`PAIRDUPIF` — duplicate pair if flag
- Proof `208_dswapif_ddropif_ddupif.cubalc`

### Prior
See 1.12.184-universal.


## 1.12.184-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack numthy unary ext (fib/fact/decade).

### Language
- `DFIB`/`2FIB`/`DFIBONACCI` — pairwise Fibonacci (`n<=0 → 0`; clamp n≤92)
- `DFACT`/`2FACT`/`DFACTORIAL` — pairwise factorial (`n<0 → 0`; clamp n≤20)
- `DLOG10`/`2LOG10`/`DILOG10` — pairwise `floor(log10)`; `n<=0 → -1`
- `DPOW10`/`2POW10`/`DTENPOW` — pairwise `10^n` for n in 0..18 else 0
- Proof `207_dfib_dfact_dlog10.cubalc`

### Prior
See 1.12.183-universal.


## 1.12.183-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack energy means (complete after `DAVG`).

### Language
- `DGEOM`/`2GEOM`/`DGEOMEAN` — pairwise geometric mean `floor(sqrt(a*c))` (non-neg; else 0)
- `DHARM`/`2HARM`/`DHARMMEAN` — pairwise harmonic mean `2*a*c/(a+c)` (sum 0 → 0)
- `DRMS`/`2RMS`/`DROOTMS` — pairwise root-mean-square `floor(sqrt((a²+c²)/2))`
- Proof `206_dgeom_dharm_drms.cubalc`

### Prior
See 1.12.182-universal.


## 1.12.182-universal — 2026-08-03

### Direction
- Algocube digit **5** → dual-stack fixed-width shift bitfield plane (byte/halfword).

### Language
- `DSHL8`/`2SHL8`/`PAIRSHL8` — pairwise logical left shift low 8 bits
- `DSHR8`/`2SHR8`/`PAIRSHR8` — pairwise logical right shift low 8 bits
- `DSAR8`/`2SAR8`/`DASHR8` — pairwise arithmetic right shift low 8 bits (sign-extend)
- `DSHL16`/`2SHL16` · `DSHR16`/`2SHR16` · `DSAR16`/`2SAR16` — same for 16-bit halfwords
- Proof `205_dshl8_dsar16.cubalc`

### Prior
See 1.12.181-universal.


## 1.12.181-universal — 2026-08-03

### Direction
- Algocube digit **8** → dual-stack fixed-width rotate plane (byte/halfword).

### Language
- `DROL8`/`2ROL8`/`PAIRROL8` — pairwise rotate-left low 8 bits (`a b c d → rol8(a,c) rol8(b,d)`)
- `DROR8`/`2ROR8`/`PAIRROR8` — pairwise rotate-right low 8 bits
- `DROL16`/`2ROL16`/`PAIRROL16` — pairwise rotate-left low 16 bits
- `DROR16`/`2ROR16`/`PAIRROR16` — pairwise rotate-right low 16 bits
- Proof `204_drol8_dror16.cubalc`

### Prior
See 1.12.180-universal.


## 1.12.180-universal — 2026-08-03

### Direction
- Algocube digit **9** → dual-stack nibble data-path clip/extend.

### Language
- `DCLIP4`/`2CLIP4`/`DCLIPN` — pairwise clamp to unsigned nibble `[0,15]`
- `DSEXT4`/`2SEXT4`/`DSEXTN` — pairwise sign-extend low 4 bits
- `DZEXT4`/`2ZEXT4`/`DZEXTN` — pairwise zero-extend low 4 bits
- Proof `203_dclip4_dext4.cubalc`

### Prior
See 1.12.179-universal.


## 1.12.179-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack conditionals (pair nip/keep-if).

### Language
- `DNIPIF`/`2NIPIF`/`PAIRNIPIF` — shared-flag choose per lane: `(f?b:a)(f?d:c)`
- `DKEEPIF`/`2KEEPIF`/`PAIRKEEPIF` — keep pair `a b` if flag else drop both
- Proof `202_dnipif_dkeepif.cubalc`

### Prior
See 1.12.178-universal.


## 1.12.178-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack multiword complement+cin negate.

### Language
- `DNEGC2`/`2NEGC2`/`DNEGC` — pairwise `~x + cin` (two's-complement limb); `CARRY` any cout
- `DCOMADC`/`2COMADC`/`DNEGADC` — aliases
- Proof `201_dnegc2.cubalc`

### Prior
See 1.12.177-universal.


## 1.12.177-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack nibble plane (bit-path pack).

### Language
- `DLO4`/`2LO4`/`DNIBLO` — pairwise low nibble (`x & 0xF`)
- `DHI4`/`2HI4`/`DNIBHI` — pairwise high nibble (`(x>>4) & 0xF`)
- `DPACK4`/`2PACK4`/`DPACKN` — pairwise pack two nibbles into a byte
- Proof `200_dlo4_dpack4.cubalc`

### Prior
See 1.12.176-universal.


## 1.12.176-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack multiword shift-through-carry.

### Language
- `DSHLC`/`2SHLC`/`DSHLCY` — pairwise left-shift-1 with per-lane cin LSB; `CARRY` any MSB out
- `DSHRC`/`2SHRC`/`DSHRCY` — pairwise right-shift-1 with per-lane cin MSB; `CARRY` any LSB out
- Proof `199_dshlc_dshrc.cubalc`

### Prior
See 1.12.175-universal.


## 1.12.175-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack control: per-lane select/mux.

### Language
- `DSEL2`/`2SEL2`/`PAIRSEL2` — pairwise mux with independent cond per lane
- `DMUX2`/`2MUX2`/`PAIRMUX2` — alias of `DSEL2`
- Proof `198_dsel2_dmux2.cubalc`

### Prior
See 1.12.174-universal.


## 1.12.174-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack multiword add/sub with per-lane cin/bin.

### Language
- `DADDC2`/`2ADDC2`/`DADC2` — pairwise unsigned add with stack cin per lane; `CARRY` any cout
- `DSUBB2`/`2SUBB2`/`DSBB2` — pairwise unsigned sub with stack bin per lane; flags updated
- Proof `197_daddc2_dsubb2.cubalc`

### Prior
See 1.12.173-universal.


## 1.12.173-universal — 2026-08-03

### Direction
- Algocube digit **0** → dual-stack unsigned overflow predicates.

### Language
- `DUADDOVF`/`2UADDOVF`/`DUADDOVER` — pairwise unsigned add wrap → 0/1
- `DUSUBOVF`/`2USUBOVF`/`DUSUBOVER` — pairwise unsigned sub borrow → 0/1
- `DUMULOVF`/`2UMULOVF`/`DUMULOVER` — pairwise unsigned mul wrap → 0/1
- Proof `196_duaddovf_dumulovf.cubalc`

### Prior
See 1.12.172-universal.


## 1.12.172-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack control gates (threshold + mask pass).

### Language
- `DTHRESH`/`2THRESH`/`PAIRTHRESH` — pairwise `x ≥ t` → 0/1
- `DGATE`/`2GATE`/`DANDIF` — pairwise pass-if-mask (`mask≠0 ? x : 0`)
- Proof `195_dthresh_dgate.cubalc`

### Prior
See 1.12.171-universal.


## 1.12.171-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack energy activations (ReLU6 + deadzone).

### Language
- `DRELU6`/`2RELU6`/`DCLAMP6` — pairwise clamp to energy band `[0, 6]`
- `DDEADZ`/`2DEADZ`/`DDEADZONE` — pairwise deadzone noise gate (`|x|≤z → 0`)
- Proof `194_drelu6_ddeadz.cubalc`

### Prior
See 1.12.170-universal.


## 1.12.170-universal — 2026-08-03

### Direction
- Algocube digit **0** → dual-stack signed overflow predicates.

### Language
- `DADDOVF`/`2ADDOVF`/`DADDOVER` — pairwise signed add overflow → 0/1
- `DSUBOVF`/`2SUBOVF`/`DSUBOVER` — pairwise signed sub overflow → 0/1
- `DMULOVF`/`2MULOVF`/`DMULOVER` — pairwise signed mul overflow → 0/1
- Proof `193_daddovf_dmulovf.cubalc`

### Prior
See 1.12.169-universal.


## 1.12.169-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack modular inverse / modular divide.

### Language
- `DMODINV`/`2MODINV`/`DINVMOD` — pairwise modular inverse (`0` if none)
- `DMODDIV`/`2MODDIV` — pairwise `a·c⁻¹ mod ma` / `b·d⁻¹ mod mb`
- Proof `192_dmodinv_dmoddiv.cubalc`

### Prior
See 1.12.168-universal.


## 1.12.168-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack 3-way compare.

### Language
- `DCMP`/`2CMP`/`DICMP` — pairwise signed compare → −1/0/+1
- `DUCMP`/`2UCMP`/`DUCMP3` — pairwise unsigned compare → −1/0/+1
- Proof `191_dcmp_ducmp.cubalc`

### Prior
See 1.12.167-universal.


## 1.12.167-universal — 2026-08-03

### Direction
- Algocube digit **9** → dual-stack 32-bit data-path clip/extend.

### Language
- `DCLIP32`/`2CLIP32` — pairwise clamp to u32 `[0, 2^32-1]`
- `DSEXT32`/`2SEXT32`/`DSEXTD` — pairwise sign-extend low 32 bits
- `DZEXT32`/`2ZEXT32`/`DZEXTD` — pairwise zero-extend low 32 bits
- Proof `190_dclip32_dext32.cubalc`

### Prior
See 1.12.166-universal.


## 1.12.166-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack multiword add/sub with carry/borrow.

### Language
- `DADDC`/`2ADDC`/`DADC` — pairwise unsigned add with `CARRY` cin; `CARRY` = any cout
- `DSUBB`/`2SUBB`/`DSBB` — pairwise unsigned sub with `BORROW`/`CARRY` bin; flags updated
- Proof `189_daddc_dsubb.cubalc`

### Prior
See 1.12.165-universal.


## 1.12.165-universal — 2026-08-03

### Direction
- Algocube digit **9** → dual-stack data-path zero-extend.

### Language
- `DZEXT8`/`2ZEXT8`/`DZEXTB` — pairwise zero-extend low 8 bits
- `DZEXT16`/`2ZEXT16`/`DZEXTW` — pairwise zero-extend low 16 bits
- Proof `188_dzext8_dzext16.cubalc`

### Prior
See 1.12.164-universal.


## 1.12.164-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack unsigned compare predicates.

### Language
- `DULT`/`2ULT`/`PAIRULT` — pairwise unsigned `<` → 0/1
- `DULE`/`2ULE`/`PAIRULE` — pairwise unsigned `≤` → 0/1
- `DUGT`/`2UGT`/`PAIRUGT` — pairwise unsigned `>` → 0/1
- `DUGE`/`2UGE`/`PAIRUGE` — pairwise unsigned `≥` → 0/1
- Proof `187_dult_duge.cubalc`

### Prior
See 1.12.163-universal.


## 1.12.163-universal — 2026-08-03

### Direction
- Algocube digit **0** → dual-stack unsigned mulhi + endian width duals.

### Language
- `DUMULHI`/`2UMULHI`/`DUMULH` — pairwise unsigned high-64 of product
- `DBSWAP16`/`2BSWAP16` — pairwise 16-bit byte swap
- `DBSWAP64`/`2BSWAP64` — pairwise 64-bit byte swap
- Proof `186_dumulhi_dbswap.cubalc`

### Prior
See 1.12.162-universal.


## 1.12.162-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack energy invert + normalize.

### Language
- `DINV`/`2INV`/`DRECIP` — pairwise integer reciprocal (`/0→0`)
- `DNORM100`/`2NORM100`/`DENORM` — pair → energy plane peak ±100 via max-abs scale
- Proof `185_dinv_dnorm100.cubalc`

### Prior
See 1.12.161-universal.


## 1.12.161-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack unsigned min/max.

### Language
- `DUMIN`/`2UMIN`/`PAIRUMIN` — pairwise unsigned min
- `DUMAX`/`2UMAX`/`PAIRUMAX` — pairwise unsigned max
- Proof `184_dumin_dumax.cubalc`

### Prior
See 1.12.160-universal.


## 1.12.160-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack unsigned div/mod.

### Language
- `DUDIV`/`2UDIV`/`DUDIVIDE` — pairwise unsigned divide (`/0→0`)
- `DUMOD`/`2UMOD`/`DUREM` — pairwise unsigned remainder (`%0→0`)
- Proof `183_dudiv_dumod.cubalc`

### Prior
See 1.12.159-universal.

## 1.12.159-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack energy scale + percent clip.

### Language
- `DSCALE`/`2SCALE`/`DSCL` — pairwise percent scale `a*sa/100` / `b*sb/100`
- `DCLIP100`/`DENCLIP`/`DCLIPPCT` — clamp pair into energy plane `[0,100]`
- Proof `182_dscale_dclip100.cubalc`

### Prior
See 1.12.158-universal.

## 1.12.158-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack modular mul/pow.

### Language
- `DMULMOD`/`2MULMOD` — pairwise `(a*c) mod ma` / `(b*d) mod mb` (`m≤0→0`)
- `DPOWMOD`/`2POWMOD` — pairwise modular power; neg exp → 0
- Proof `181_dmulmod_dpowmod.cubalc`

### Prior
See 1.12.157-universal.

## 1.12.157-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack ALU fused multiply-add + mul-high.

### Language
- `DMADD`/`2MADD`/`DFMA` — pairwise `a*c+e` / `b*d+f`
- `DMULHI`/`2MULHI`/`DHMUL` — pairwise signed high-64 of product
- Proof `180_dmadd_dmulhi.cubalc`

### Prior
See 1.12.156-universal.

## 1.12.156-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack energy percent + lerp mix.

### Language
- `DPCT`/`2PCT`/`DPERCENT` — pairwise `(a*100)/c` / `(b*100)/d` (`/0→0`)
- `DLERP`/`2LERP`/`DMIX` — shared-t percent lerp: `a+(c-a)*t/100` (t clamp 0..100)
- Proof `179_dpct_dlerp.cubalc`

### Prior
See 1.12.155-universal.

## 1.12.155-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack select: median + abs extremum.

### Language
- `DMEDIAN`/`2MED`/`DMID3` — pairwise median of three pairs
- `DMAXABS`/`DMINABS` — pairwise max/min of absolute values
- Proof `178_dmedian_dmaxabs.cubalc`

### Prior
See 1.12.154-universal.

## 1.12.154-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack unary control: ReLU + copysign.

### Language
- `DRELU`/`2RELU`/`DCLAMP0` — pairwise `max(0,a)` / `max(0,b)`
- `DCOPYSIGN`/`2COPYSIGN`/`DCSIGN` — pairwise magnitude of first with sign of second pair
- Proof `177_drelu_dcopysign.cubalc`

### Prior
See 1.12.153-universal.

## 1.12.153-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack energy wrap + hypot.

### Language
- `DWRAP`/`2WRAP`/`DWMOD` — pairwise wrap-mod into `[0,m)` (`m≤0→0`)
- `DHYPOT`/`2HYP`/`PAIRHYP` — pairwise Euclidean `isqrt(a²+c²)` / `isqrt(b²+d²)`
- Proof `176_dwrap_dhypot.cubalc`

### Prior
See 1.12.152-universal.

## 1.12.152-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack modular add/sub.

### Language
- `DADDMOD`/`2ADDMOD`/`PAIRADDMOD` — pairwise `(a±c) mod ma`, `(b±d) mod mb` (add)
- `DSUBMOD`/`2SUBMOD`/`PAIRSUBMOD` — pairwise modular subtract (`m≤0→0`, result in `[0,m)`)
- Proof `175_daddmod_dsubmod.cubalc`

### Prior
See 1.12.151-universal.

## 1.12.151-universal — 2026-08-03

### Direction
- Algocube digit **8** → stack depth plane: pair right-rotate + depth-6.

### Language
- `2RROT`/`DRROT`/`RROT2` — rotate three pairs right (`a b c d e f → e f a b c d`)
- `6DUP`/`HDUP`/`DUP6` — duplicate top 6
- `6DROP`/`HDROP`/`DROP6` — drop top 6
- Proof `174_2rrot_6dup.cubalc`

### Prior
See 1.12.150-universal.

## 1.12.150-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack ceil/floor divide modes.

### Language
- `DDIVCEIL`/`2DIVCEIL`/`DCEILDIV` — pairwise ceil-divide (`/0→0`)
- `DDIVFLOOR`/`2DIVFLOOR`/`DFLOORDIV` — pairwise floor-divide (`/0→0`)
- Proof `173_ddivceil_ddivfloor.cubalc`

### Prior
See 1.12.149-universal.

## 1.12.149-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack Morton zip/unzip (bit-path).

### Language
- `DZIP`/`2ZIP`/`DMORTON` — pair interleave: `a b c d → zip(a,c) zip(b,d)` (low-32 lanes)
- `DUNZIP`/`2UNZIP` — pair deinterleave: `a b → even(a) even(b) odd(a) odd(b)`
- Proof `172_dzip_dunzip.cubalc`

### Prior
See 1.12.148-universal.

## 1.12.148-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack parallel extract/deposit (bit-path).

### Language
- `DPEXT`/`2PEXT` — pair BMI2-style parallel extract under masks: `a b ma mb → pext(a,ma) pext(b,mb)`
- `DPDEP`/`2PDEP` — pair parallel deposit of low bits into mask positions
- Proof `171_dpext_dpdep.cubalc`

### Prior
See 1.12.147-universal.

## 1.12.147-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack high-bit isolate + ceil power-of-2.

### Language
- `DBHSI`/`2BHSI`/`DHIBIT` — isolate highest set bit of each of pair (0 if zero)
- `DCEILPOW2`/`2CEILPOW2`/`DNEXTPOW2` — smallest power of 2 ≥ n (n≤0 → 0)
- Proof `170_dbhsi_dceilpow2.cubalc`

### Prior
See 1.12.146-universal.

## 1.12.146-universal — 2026-08-03

### Direction
- Algocube digit **5** → dual-stack field extract/deposit (bitfield path).

### Language
- `DBEXT`/`2BEXT` — extract width bits at pos from pair: `a b pos width → field field`
- `DBDEP`/`2BDEP` — deposit low 8 bits of pair fields at pos: `a b fa fb pos → base' base'`
- Proof `169_dbext_dbdep.cubalc`

### Prior
See 1.12.145-universal.

## 1.12.145-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack bitfield position path.

### Language
- `DBTEST`/`2BTEST` — test bit at positions: `a b na nb → bit(a,na) bit(b,nb)`
- `DSETB`/`2SETB` · `DCLRB`/`2CLRB` · `DFLIPB`/`2FLIPB` — set/clear/toggle bit at pair indices
- Proof `168_dbtest_dsetb.cubalc`

### Prior
See 1.12.144-universal.

## 1.12.144-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack sat-div + n-bit RNG (energy/flow).

### Language
- `DSATDIV`/`2SATDIV` — pairwise saturating divide (`/0→0`, `LONG_MIN/-1→LONG_MAX`)
- `DRANDBITS`/`2RANDBITS` — pair uniform in `[0, 2^n)` from width pair (n clamped 0..62)
- Proof `167_dsatdiv_drandbits.cubalc`

### Prior
See 1.12.143-universal.

## 1.12.143-universal — 2026-08-03

### Direction
- Algocube digit **8** → dual-stack 32-bit word pack (data-path).

### Language
- `DLO32`/`2LO32` · `DHI32`/`2HI32` — low/high 32-bit word of each of pair
- `DPACK32`/`2PACK32` — pack hi,lo words: `a b c d → (a<<32|c) (b<<32|d)` (masked)
- Proof `166_dlo32_dpack32.cubalc`

### Prior
See 1.12.142-universal.

## 1.12.142-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack bit isolate/mask path.

### Language
- `DBLS`/`2BLS`/`DBLSI` — isolate lowest set bit of each of pair (`x & -x`)
- `DBLC`/`2BLC`/`DBLSR` — clear lowest set bit (`x & (x-1)`)
- `DMASK`/`2MASK` — low-n-bit mask from pair widths (`(1<<n)-1`, n clamped 0..64)
- Proof `165_dbls_dmask.cubalc`

### Prior
See 1.12.141-universal.

## 1.12.141-universal — 2026-08-03

### Direction
- Algocube digit **4** → dual-stack control-word halfpack (16-bit).

### Language
- `DLO16`/`2LO16` · `DHI16`/`2HI16` — low/high halfword of each of pair
- `DPACK16`/`2PACK16` — pack hi,lo halfwords: `a b c d → (a<<16|c) (b<<16|d)` (masked)
- Proof `164_dlo16_dpack16.cubalc`

### Prior
See 1.12.140-universal.

## 1.12.140-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack sat-mul + range RNG (energy/flow).

### Language
- `DSATMUL`/`2SATMUL` — pairwise saturating multiply
- `DRANDRANGE`/`2RANDRANGE`/`DRANDIN` — pair uniform inclusive ranges `a b c d → U[a,c] U[b,d]`
- Proof `163_dsatmul_drandrange.cubalc`

### Prior
See 1.12.139-universal.

## 1.12.139-universal — 2026-08-03

### Direction
- Algocube digit **4** → dual-stack control-word byte pack/extract.

### Language
- `DLO8`/`2LO8` · `DHI8`/`2HI8` — low/high byte of each of pair
- `DPACK8`/`2PACK8` — pack hi,lo bytes: `a b c d → (a<<8|c) (b<<8|d)` (masked)
- Proof `162_dlo8_dpack8.cubalc`

### Prior
See 1.12.138-universal.

## 1.12.138-universal — 2026-08-03

### Direction
- Algocube digit **9** → dual-stack data-path clip + sign-extend.

### Language
- `DCLIP8`/`2CLIP8` · `DCLIP16`/`2CLIP16` — clamp pair to u8/u16 ranges
- `DSEXT8`/`2SEXT8` · `DSEXT16`/`2SEXT16` — sign-extend low 8/16 bits on pair
- Proof `161_dclip_dsext.cubalc`

### Prior
See 1.12.137-universal.

## 1.12.137-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack RNG + saturating energy ALU.

### Language
- `DRAND`/`2RAND`/`PAIRRAND` — pair uniform rand in `[0,max)`; max≤0 → 10
- `DSATADD`/`2SATADD` · `DSATSUB`/`2SATSUB` — pairwise saturating ±
- Proof `160_drand_dsatadd.cubalc`

### Prior
See 1.12.136-universal.

## 1.12.136-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack unary predicates (parity + sign-vs-zero).

### Language
- `DODD`/`2ODD` · `DEVEN`/`2EVEN` — parity predicates on pair
- `DLTZ`/`2LTZ` · `DGTZ`/`2GTZ` · `DLEZ`/`2LEZ` · `DGEZ`/`2GEZ` — compare to zero
- Proof `159_dodd_dltz.cubalc`

### Prior
See 1.12.135-universal.

## 1.12.135-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack number theory unary metrics.

### Language
- `DLOG2`/`2LOG2`/`DILOG2` — floor(log2); ≤0 → -1 on pair
- `DPHI`/`2PHI`/`DTOTIENT` — Euler totient φ(n); ≤0 → 0 on pair
- `DISPRIME`/`2ISPRIME`/`DPRIMEP` — prime predicate 0/1 on pair
- Proof `158_dlog2_dphi.cubalc`

### Prior
See 1.12.134-universal.

## 1.12.134-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack ALU unary scale + endian.

### Language
- `DDBL`/`2DBL`/`PAIRDBL` — pair double (`*2`)
- `DHALF`/`2HALF`/`PAIRHALF` — pair half toward zero (`/2`)
- `DBSWAP`/`2BSWAP`/`PAIRBSWAP` — pair 32-bit byte-swap
- Proof `157_ddbl_dbswap.cubalc`

### Prior
See 1.12.133-universal.

## 1.12.133-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack energy/flow distance metrics.

### Language
- `DAVG`/`2AVG`/`DMEAN` — pairwise truncated mean `(a+c)/2`, `(b+d)/2`
- `DDIST`/`2DIST`/`DABSDIFF` — pairwise absolute difference `|a-c|`, `|b-d|`
- `DHAMM`/`2HAMM`/`DHAMMING` — pairwise Hamming distance (popcount of XOR)
- Proof `156_davg_dhamm.cubalc`

### Prior
See 1.12.132-universal.

## 1.12.132-universal — 2026-08-03

### Direction
- Algocube digit **0** → dual-stack ones-metrics + power-of-two predicates.

### Language
- `DCLO`/`2CLO`/`PAIRCLO` — count leading ones (64-bit) on pair
- `DCTO`/`2CTO`/`PAIRCTO` — count trailing ones on pair
- `DISPOW2`/`2ISPOW2`/`DPOW2P`/`2POW2P` — 1 if value is power of two (>0, single bit)
- Lexer: digit-prefix compound tails accept alnum (enables `2ISPOW2` / `2POW2P`)
- Proof `155_dclo_dispow2.cubalc`

### Prior
See 1.12.131-universal.

## 1.12.131-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack bit-position metrics (ALU bit-path).

### Language
- `DFFS`/`2FFS`/`PAIRFFS` — find first set (1-based lowest 1-bit; 0 if zero) on pair
- `DFLS`/`2FLS`/`PAIRFLS`/`DMSB` — find last set (1-based highest 1-bit; 0 if zero)
- `DBWIDTH`/`2BWIDTH`/`PAIRBWIDTH` — minimal bit width of unsigned word (0 if zero)
- Proof `154_dffs_dbwidth.cubalc`

### Prior
See 1.12.130-universal.

## 1.12.130-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack bit-path: DORN DBREV DPARITY.

### Language
- `DORN`/`2ORN` — a b c d → (a|~c) (b|~d) (complements DANDN)
- `DBREV`/`2BREV`/`DBITREV` — reverse low 32 bits of each of top pair
- `DPARITY`/`2PARITY` — pairwise popcount mod 2 (0/1)
- Lexer 2-prefix: ORN BREV BITREV PARITY PAR
- Completes dual-stack bit metrics + ANDN/ORN plane
- Proof `153_dorn_dparity.cubalc`

### Prior
See 1.12.129-universal.

## 1.12.129-universal — 2026-08-03

### Direction
- Algocube digit **0** → dual-stack bit metrics: DPOPCNT DCLZ DCTZ.

### Language
- `DPOPCNT`/`2POPCNT` — a b → popcount(a) popcount(b)
- `DCLZ`/`2CLZ` — pair count leading zeros (0 → 64)
- `DCTZ`/`2CTZ` — pair count trailing zeros (0 → 64)
- Lexer 2-prefix: POPCNT PCNT CLZ CTZ
- Complements dual-stack shift/bitwise with vector bit-metrics
- Proof `152_dpopcnt_dclz.cubalc`

### Prior
See 1.12.128-universal.

## 1.12.128-universal — 2026-08-03

### Direction
- Algocube digit **5** → dual-stack extended bitwise: DNAND DNOR DXNOR DANDN.

### Language
- `DNAND`/`2NAND` — a b c d → ~(a&c) ~(b&d)
- `DNOR`/`2NOR` — a b c d → ~(a|c) ~(b|d)
- `DXNOR`/`2XNOR`/`DEQV` — a b c d → ~(a^c) ~(b^d)
- `DANDN`/`2ANDN`/`DBIC` — a b c d → (a&~c) (b&~d)
- Lexer 2-prefix: NAND NOR XNOR ANDN
- Completes dual-stack bitwise plane after DAND/DOR/DXOR/DNOT
- Proof `151_dnand_dandn.cubalc`

### Prior
See 1.12.127-universal.

## 1.12.127-universal — 2026-08-03

### Direction
- Algocube digit **4** → dual-stack rotate + range predicates: DROL DROR DWITHIN DBETWEEN.

### Language
- `DROL`/`2ROL` · `DROR`/`2ROR` — pairwise rotate left/right (amounts mod 64)
- `DWITHIN`/`2WITHIN` — a b lo hi → Forth half-open range tests (0/1)
- `DBETWEEN`/`2BETWEEN` — inclusive [lo,hi] (swap if inverted)
- Lexer 2-prefix: ROL ROR WITHIN BETWEEN
- Completes dual-stack shift plane with rotates + control range checks
- Proof `150_drol_dwithin.cubalc`

### Prior
See 1.12.126-universal.

## 1.12.126-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack unary control: DINC DDEC DNOT DEQZ DNEZ.

### Language
- `DINC`/`2INC` · `DDEC`/`2DEC` — pair increment / decrement
- `DNOT`/`2NOT` — pairwise bitwise complement
- `DEQZ`/`2EQZ` · `DNEZ`/`2NEZ` — pair zero / nonzero predicates (0/1)
- Lexer 2-prefix: INC DEC NOT EQZ NEZ
- Complements dual-stack compare plane with loop/counter + boolean duals
- Proof `149_dinc_dnot.cubalc`

### Prior
See 1.12.125-universal.

## 1.12.125-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack bound/select: DSIGN DCLAMP DSEL/DMUX.

### Language
- `DSIGN`/`2SIGN` — a b → sgn(a) sgn(b) as −1/0/1
- `DCLAMP`/`2CLAMP` — a b lo hi → clamp both into [lo,hi] (shared bounds)
- `DSEL`/`DMUX`/`2SEL`/`2MUX` — fa fb ta tb c → (c?ta:fa) (c?tb:fb)
- Lexer 2-prefix: SIGN CLAMP SEL MUX
- Energy-style shared bounds + vector mux plane
- Proof `148_dclamp_dsel.cubalc`

### Prior
See 1.12.124-universal.

## 1.12.124-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack numthy ext: DSQR DISQRT DCOPRIME.

### Language
- `DSQR`/`2SQR` — a b → a² b²
- `DISQRT`/`2ISQRT` — a b → isqrt(a) isqrt(b) (neg → 0)
- `DCOPRIME`/`2COPRIME` — a b c d → (gcd(a,c)==1) (gcd(b,d)==1)
- Lexer 2-prefix: SQR ISQRT SQRT COPRIME
- Complements dual-stack gcd/lcm/pow plane with square/root/coprime
- Proof `147_dsqr_dcoprime.cubalc`

### Prior
See 1.12.123-universal.

## 1.12.123-universal — 2026-08-03

### Direction
- Algocube digit **8** → dual-stack pair shifts: DSHL DSHR DSAR.

### Language
- `DSHL`/`2SHL` — a b c d → (a≪c) (b≪d) amounts clamped 0..63
- `DSHR`/`2SHR` — pairwise logical right shift
- `DSAR`/`2SAR` — pairwise arithmetic right shift
- Lexer 2-prefix: SHL SHR SAR
- Complements dual-stack ALU/bitwise with vector data-path shifts
- Proof `146_dshl_dsar.cubalc`

### Prior
See 1.12.122-universal.

## 1.12.122-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack pair number theory: DGCD DLCM DPOW.

### Language
- `DGCD`/`2GCD` — a b c d → gcd(a,c) gcd(b,d)
- `DLCM`/`2LCM` — a b c d → lcm(a,c) lcm(b,d) (0-safe)
- `DPOW`/`2POW` — a b c d → a^c b^d (neg exp → 0)
- Lexer 2-prefix: GCD LCM POW
- Complements dual-stack ALU with pairwise numthy plane
- Proof `145_dgcd_dlcm.cubalc`

### Prior
See 1.12.121-universal.

## 1.12.121-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack pair compare predicates: DEQ DNE DLT DLE DGT DGE.

### Language
- `DEQ`/`2EQ` · `DNE`/`2NE` — pairwise equality / inequality (0/1)
- `DLT`/`2LT` · `DLE`/`2LE` · `DGT`/`2GT` · `DGE`/`2GE` — pairwise ordered compares
- Lexer 2-prefix whitelist: EQ NE LT LE GT GE
- Complements dual-stack ALU/bitwise with vector relational plane
- Proof `144_deq_dlt.cubalc`

### Prior
See 1.12.120-universal.

## 1.12.120-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack pair bitwise + unary: DAND DOR DXOR DNEG DABS.

### Language
- `DAND`/`2AND` · `DOR`/`2OR` · `DXOR`/`2XOR` — pairwise bitwise
- `DNEG`/`2NEG` · `DABS`/`2ABS` — unary pair negate/abs
- Completes dual-stack arith + logic plane
- Proof `143_dand_dneg.cubalc`

### Prior
See 1.12.119-universal.

## 1.12.119-universal — 2026-08-03

### Direction
- Algocube digit **2** → prime metrics: π(n), gap to next prime, composite test.

### Language
- `PRIMECOUNT`/`PRIMEPI`(n) — primes ≤ n (cap 200000)
- `PRIMEGAP`/`PGAP`(n) — nextprime(n) − n
- `ISCOMPOSITE`/`COMPOSITEP`(n) — composite predicate
- Stack: `SPRIMECOUNT` · `SPRIMEGAP` · `SISCOMPOSITE`
- Proof `142_primecount_gap.cubalc`

### Prior
See 1.12.118-universal.

## 1.12.118-universal — 2026-08-03

### Direction
- Algocube digit **0** → complete depth-5 stack foundation plane.

### Language
- `5NIP`/`NIP5` — a b c d e → a e
- `5ROT`/`ROT5` · `5RROT`/`RROT5` — rotate top 5
- `5OVER`/`OVER5` — copy under quintet (needs 10)
- `5TUCK`/`TUCK5` — e a b c d e
- Lexer whitelist for 5NIP/5ROT/5RROT/5OVER/5TUCK
- Proof `141_5nip_5rot.cubalc`

### Prior
See 1.12.117-universal.

## 1.12.117-universal — 2026-08-03

### Direction
- Algocube digit **5** → cell search: last-find + first/last nonzero + stack duals.

### Language
- `FINDLASTCELL`/`RFINDCELL` val [lo [hi]] — last index of val (−1 if none)
- `FIRSTNZ`/`LASTNZ` [lo [hi]] — first/last nonzero index
- Stack: `SFINDCELL` · `SFINDLAST` · `SFIRSTNZ` · `SLASTNZ`
- Proof `140_findlast_nz.cubalc`

### Prior
See 1.12.116-universal.

## 1.12.116-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack pair ALU: DDIV DMOD DMIN DMAX.

### Language
- `DDIV`/`2DIV` — a b c d → (a/c) (b/d) (0-safe)
- `DMOD`/`2MOD`/`DREM` — a b c d → (a%c) (b%d)
- `DMIN`/`2MIN` · `DMAX`/`2MAX` — pairwise min/max
- Completes DADD/DSUB/DMUL pair plane
- Proof `139_ddiv_dmin.cubalc`

### Prior
See 1.12.115-universal.

## 1.12.115-universal — 2026-08-03

### Direction
- Algocube digit **5** → cell range predicates: any/all/none, nz-count, equal-range.

### Language
- `ANYCELL`/`ALLCELL`/`NONECELL` lo hi — nonzero range predicates
- `NZCOUNT`/`COUNTNZCELL` lo hi — count nonzero cells
- `EQRANGE` a_lo b_lo n — pairwise range equality
- Stack: `SANYCELL`/`SALLCELL`/`SNONECELL`/`SNZCOUNT`/`SEQRANGES`
- Proof `138_anycell_eqrange.cubalc`

### Prior
See 1.12.114-universal.

## 1.12.114-universal — 2026-08-03

### Direction
- Algocube digit **1** → stack conditionals: dup-if-zero, swap/keep/nip under flag.

### Language
- `DUPZ`/`QDUP0` — duplicate TOS only if zero (complement of `QDUP`)
- `SSWAPIF`/`SWAPIF`/`QSWAP` — a b f → swap a,b if f
- `SKEEPIF`/`KEEPIF` — v f → keep v if f else drop both
- `SNIPIF`/`NIPIF`/`NIPWHEN` — a b f → f?b:a
- Proof `137_dupz_swapif.cubalc`

### Prior
See 1.12.113-universal.

## 1.12.113-universal — 2026-08-03

### Direction
- Algocube digit **2** → abundance class: aliquot / perfect / abundant / deficient.

### Language
- `ALIQUOT`/`PROPERSIGMA`(n) — proper divisor sum σ(n)−n
- `ISPERFECT`/`PERFECTP`(n) · `ISABUNDANT`/`ABUNDANTP`(n) · `ISDEFICIENT`/`DEFICIENTP`(n)
- Stack: `SALIQUOT` · `SISPERFECT` · `SISABUNDANT` · `SISDEFICIENT`
- Proof `136_aliquot_perfect.cubalc`

### Prior
See 1.12.112-universal.

## 1.12.112-universal — 2026-08-03

### Direction
- Algocube digit **6** → energy fleet metrics + pair equalize/swap.

### Language
- `ENERGYSWAP` a b — exchange energy planes
- `ENERGYSHARE` a b — equalize mean energy
- `ENERGYTOTAL`/`ENERGYAVG`/`ENERGYMIN`/`ENERGYMAX` — fleet metrics (0..100)
- `SENERGYTOTAL`/`SENERGYAVG`/`SENERGYMIN`/`SENERGYMAX` — stack duals
- Proof `135_energy_fleet.cubalc`

### Prior
See 1.12.111-universal.

## 1.12.111-universal — 2026-08-03

### Direction
- Algocube digit **5** → cell compare/mux: relational predicates + cell-plane mux.

### Language
- `LTCELL`/`GTCELL`/`LECELL`/`GECELL` lo hi val — 0/1 relational masks
- `SLTCELL`/`SGTCELL`/`SLECELL`/`SGECELL` — stack duals (lo hi val)
- `MUXCELL` dst_lo a_lo b_lo mask_lo n — mask?a:b cell blend
- `SMUXCELL` — stack dlo alo blo mlo n
- Proof `134_ltcell_mux.cubalc`

### Prior
See 1.12.110-universal.

## 1.12.110-universal — 2026-08-03

### Direction
- Algocube digit **2** → primes/powers: perfect power, prime power, nth prime.

### Language
- `ISPOWER`/`PERFPOW`(n) · `SISPOWER` — perfect power b^e (e≥2)
- `ISPRIMEPOWER`/`IPP`(n) · `SIPP` — form p^k
- `NTHPRIME`/`PRIMEN`(k) · `SNTHPRIME` — k-th prime (1→2)
- Proof `133_ispower_nthprime.cubalc`

### Prior
See 1.12.109-universal.

## 1.12.109-universal — 2026-08-03

### Direction
- Algocube digit **2** → modular ext: square-part, primitive root, CRT stack.

### Language
- `SQPART`/`LARGESQ`(n) · `SSQPART` — largest square dividing n
- `ISPRIMITIVE`/`ISPRROOT`(a,m) · `SIPRIMITIVE` — primitive root test
- `SCRT`/`SCHINREM` — stack Chinese remainder (a m b n → x)
- Proof `132_sqpart_scrt.cubalc`

### Prior
See 1.12.108-universal.

## 1.12.108-universal — 2026-08-03

### Direction
- Algocube digit **2** → modular order plane: SOPF + Carmichael λ + mult. order.

### Language
- `SOPF`/`SOPFR`(n) · `SSOPF` — sum of prime factors (distinct / with mult.)
- `CARMICHAEL`/`LAMBDA`(n) · `SCARMICHAEL`/`SLAMBDA` — Carmichael λ(n)
- `ORDER`/`MULTORDER`(a,m) · `SORDER` — multiplicative order
- Proof `131_sorder_carmichael.cubalc`

### Prior
See 1.12.107-universal.

## 1.12.107-universal — 2026-08-03

### Direction
- Algocube digit **9** → cell-logic stack duals + NECELL predicate.

### Language
- `NECELL`/`CELLNE` — range ≠ val → 0/1 mask
- `SANDCELL`/`SORCELL`/`SXORCELL` — stack lo hi mask bitwise
- `SNOTCELL` — stack lo hi bitwise invert
- `SCELLEQ`/`SNECELL` — stack lo hi val predicate masks
- Proof `130_sandcell_sne.cubalc`

### Prior
See 1.12.106-universal.

## 1.12.106-universal — 2026-08-03

### Direction
- Algocube digit **9** → cell-fold stack duals: scan/diff/shift/clamp + DIFFCELL.

### Language
- `DIFFCELL`/`CELLDIFF` — adjacent differences (inverse of SCANCELL)
- `SSCANCELL`/`SPREFIXSUM` — stack lo hi → prefix sum
- `SDIFFCELL`/`SCELLDIFF` — stack lo hi → adjacent diffs
- `SSHIFTCELL`/`SCELLSHIFT` — stack lo hi k → zero-fill shift
- `SCLAMPCELL`/`SCELLCLAMP` — stack lo hi mn mx → clamp range
- Proof `129_sscan_sdiff.cubalc`

### Prior
See 1.12.105-universal.

## 1.12.105-universal — 2026-08-03

### Direction
- Algocube digit **2** → factor metrics after SPF/Jacobi: valuation + Ω/ω.

### Language
- `VALUATION`/`PVAL`/`VP`(n,p) · `SVAL`/`SPVAL` — p-adic valuation v_p(n)
- `OMEGA`/`BIGOMEGA`(n) · `SOMEGA` — Ω(n) factors with multiplicity
- `OMEGA0`/`LITTLEOMEGA`(n) · `SOMEGA0` — ω(n) distinct primes
- Proof `128_sval_somega.cubalc`

### Prior
See 1.12.104-universal.

## 1.12.104-universal — 2026-08-03

### Direction
- Algocube digit **8** → stack depth duals: 4TUCK + quintuple 5DUP/5DROP/5SWAP.

### Language
- `4TUCK`/`QTUCK`/`TUCK4` — a b c d → d a b c d
- `5DUP`/`DUP5` — duplicate top 5
- `5DROP`/`DROP5` — drop top 5
- `5SWAP`/`SWAP5` — reverse top 5
- Proof `127_4tuck_5dup.cubalc`

### Prior
See 1.12.103-universal.

## 1.12.103-universal — 2026-08-03

### Direction
- Algocube digit **5** → cell-memory stack duals of COPY/MOVE/REV/ROT.

### Language
- `SCOPYCELL`/`SCELLCOPY` — stack src dst n → copy range
- `SMOVECELL`/`SCELLMOVE` — stack src dst n → move (clear source)
- `SREVCELL`/`SCELLREV` — stack lo hi → reverse range
- `SROTCELL`/`SCELLROT` — stack lo hi k → rotate range
- Proof `126_scopycell_srot.cubalc`

### Prior
See 1.12.102-universal.

## 1.12.102-universal — 2026-08-03

### Direction
- Algocube digit **2** → modular / number-theory duals after SPOWMOD/SMODINV.

### Language
- `JACOBI`/`LEGENDRE`(a,n) · `SJACOBI`/`SLEGENDRE` — Jacobi symbol −1/0/1
- `MODDIV`/`DIVMODM`(a,b,m) · `SMODDIV`/`SDIVMODM` — a·b⁻¹ mod m
- `SPF`/`SMALLPF`/`MINPF`(n) · `SSPF`/`SSMALLPF` — smallest prime factor
- Proof `125_smoddiv_jacobi.cubalc`

### Prior
See 1.12.101-universal.

## 1.12.101-universal — 2026-08-03

### Direction
- Algocube digit **0** → stack foundation duals: complete depth-4 plane after 4DUP/4DROP/4SWAP.

### Language
- `4NIP`/`QNIP`/`NIP4` — a b c d → a d (drop middle two)
- `4ROT`/`QROT`/`ROT4` — a b c d → b c d a
- `4RROT`/`QRROT`/`RROT4` — a b c d → d a b c
- `4OVER`/`QOVER`/`OVER4` — copy under quartet (needs 8)
- Proof `124_4nip_4rot.cubalc`

### Prior
See 1.12.100-universal.

## 1.12.100-universal — 2026-08-03

### Direction
- Algocube digit **8** → stack depth duals: 3NIP + quadruple 4DUP/4DROP/4SWAP (after 3ROT/3TUCK).

### Language
- `3NIP`/`TNIP`/`NIP3` — a b c → a c
- `4DUP`/`DUP4` — duplicate top 4
- `4DROP`/`DROP4` — drop top 4
- `4SWAP`/`SWAP4` — reverse top 4
- Proof `123_3nip_4dup.cubalc`

### Prior
See 1.12.99-universal.

## 1.12.99-universal — 2026-08-03

### Direction
- Algocube digit **4** → stack combinator duals with n from TOS (complete PICK/ROLL/NDROP imm plane).

### Language
- `SPICK`/`PICKS` — pop n, copy n-th under remaining top
- `SROLL`/`ROLLS` — pop n, rotate top (n+1) items
- `SNDROP`/`DROPS` — pop n, drop n remaining top items
- Proof `122_spick_sroll.cubalc`

### Prior
See 1.12.98-universal.

## 1.12.98-universal — 2026-08-03

### Direction
- Algocube digit **3** → string plane duals: LEFT/RIGHT slices + occurrence COUNT (after MID/FIND/LPAD).

### Language
- `SYS LEFT`/`STRLEFT`/`TAKELEFT` str n — first n chars → LAST
- `SYS RIGHT`/`STRRIGHT`/`TAKERIGHT` str n — last n chars → LAST
- `SYS COUNT`/`STRCOUNT`/`OCCURS` hay needle — non-overlapping count → LAST_N
- Proof `121_str_left_count.cubalc`

### Prior
See 1.12.97-universal.

## 1.12.97-universal — 2026-08-03

### Direction
- Algocube digit **5** → stack-imm bitfield extract/deposit/mask (dual of SBEXT/SBDEP + mask generator).

### Language
- `SBEXTN`/`EXTN`/`BEXTN` pos width — extract width bits at pos from TOS
- `SBDEPN`/`DEPN`/`BDEPN` field pos — deposit low 8 bits of field into TOS at pos
- `SMASKN`/`MASKN`/`ONESN` n — TOS = low-n-bit mask ((1≪n)−1; n=64 → all ones)
- Proof `120_sbextn_smaskn.cubalc`

### Prior
See 1.12.96-universal.

## 1.12.96-universal — 2026-08-03

### Direction
- Algocube digit **8** → triple stack-depth duals (complete after 3DUP/3DROP/3SWAP/3OVER).

### Language
- `3ROT`/`TROT`/`ROT3` — a b c → b c a
- `3RROT`/`TRROT`/`RROT3` — a b c → c a b
- `3TUCK`/`TTUCK`/`TUCK3` — a b c → c a b c
- Proof `119_3rot_3tuck.cubalc`

### Prior
See 1.12.95-universal.

## 1.12.95-universal — 2026-08-03

### Direction
- Algocube digit **9** → stack↔cell range dual + cell transfer (complete after SGETCELL/SSETCELL single forms).

### Language
- `SLOADCELLS`/`SLOADN`/`SPUSHRANGE` lo n — push cells[lo..lo+n-1]
- `SPOPCELLS`/`SSTORECELLS`/`SSTORERANGE` lo n — pop n into cells[lo..]
- `CELLXFER`/`XFERCELL` i j amt — move amt from cell i → j
- Proof `118_sloadcells_cellxfer.cubalc`

### Prior
See 1.12.94-universal.

## 1.12.94-universal — 2026-08-03

### Direction
- Algocube digit **6** → energy transfer/clamp + stack-imm range RNG (extend after ENERGYSUB/SRANDN).

### Language
- `ENERGYXFER`/`XFERENERGY`/`SENRX` src dst n — move n energy units src→dst (clamp 0..100)
- `ENERGYCLAMP`/`CLAMPENERGY`/`SFLWX` id lo hi — clamp energy to [lo,hi]
- `SRANDRANGEN`/`RANDRANGEN`/`SRNGX` lo hi — push uniform [lo,hi] (stack-imm dual of SRANDRANGE)
- Proof `117_energyxfer_srandrangen.cubalc`

### Prior
See 1.12.93-universal.

## 1.12.93-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack pair ALU (vector pair +−× after binary stack ALU + 2DUP family).

### Language
- `DADD`/`2ADD`/`PAIRADD` — a b c d → (a+c) (b+d)
- `DSUB`/`2SUB`/`PAIRSUB` — a b c d → (a-c) (b-d)
- `DMUL`/`2MUL`/`PAIRMUL` — a b c d → (a*c) (b*d)
- Proof `116_dadd_dmul.cubalc`

### Prior
See 1.12.92-universal.

## 1.12.92-universal — 2026-08-03

### Direction
- Algocube digit **6** → energy / flow / RNG dual forms (stack-imm RAND + energy drain + stack ENERGYFLOW).

### Language
- `SRANDN`/`RANDN`/`SRNGN` n — push rand in [0,n) (stack-imm dual of SRAND)
- `ENERGYSUB`/`SUBENERGY`/`DRAIN`/`SENRN` id n — drain energy plane (complete set/add/sub)
- `SEFLOW`/`SENERGYFLOW` — hops from TOS · `SEFLOWN`/`SFLWN` n — immediate hops
- Proof `115_srandn_energysub.cubalc`

### Prior
See 1.12.91-universal.

## 1.12.91-universal — 2026-08-03

### Direction
- Algocube digit **0** → stack immediate inverted bitwise (complete NAND/NOR/XNOR-with-constant after SANDI/SORI/SXORI).

### Language
- `SNANDI`/`NANDI`/`NANDIMM` n — TOS = ~(TOS & n)
- `SNORI`/`NORI`/`NORIMM` n — TOS = ~(TOS | n)
- `SXNORI`/`XNORI`/`XNORIMM` n — TOS = ~(TOS ^ n) (equiv)
- Proof `114_snandi_sxnori.cubalc`

### Prior
See 1.12.90-universal.

## 1.12.90-universal — 2026-08-03

### Direction
- Algocube digit **9** → stack immediate bitwise mask (complete AND/OR/XOR-with-constant after bitfield + shift).

### Language
- `SANDI`/`ANDIMM`/`ANDI` n — TOS &= n
- `SORI`/`ORIMM`/`ORI` n — TOS |= n
- `SXORI`/`XORIMM`/`XORI` n — TOS ^= n
- Proof `113_sandi_sxori.cubalc`

### Prior
See 1.12.89-universal.



## 1.12.89-universal — 2026-08-03

### Direction
- Algocube digit **3** → stack immediate rotate + arithmetic shift (complete shift/rotate-by-constant after SSHLN/SSHRN).

### Language
- `SROLN`/`ROLN`/`SROTLN` n — rotate left TOS by n mod 64
- `SRORN`/`RORN`/`SROTRN` n — rotate right TOS by n mod 64
- `SSARN`/`SARN`/`SASHRN`/`ASHRN` n — arithmetic TOS ≫= n (sign-preserving)
- Proof `112_sroln_ssarn.cubalc`

### Prior
See 1.12.88-universal.

## 1.12.88-universal — 2026-08-03

### Direction
- Algocube digit **1** → stack immediate compare + min/max-with-constant (complete SEQ/SLT/SMIN family after stack-stack).

### Language
- `SEQN`/`EQN`/`CMPEQN` n — TOS = (TOS == n) ? 1 : 0
- `SNEN`/`NEN`/`CMPNEN` n — TOS = (TOS != n) ? 1 : 0
- `SLTN`/`LTN` · `SGTN`/`GTN` n — ordered predicates vs constant
- `SLENN`/`SLEQN` · `SGENN`/`SGEQN` n — ≤ / ≥ vs constant
- `SMINN`/`MINN` · `SMAXN`/`MAXN` n — min/max(TOS, n)
- Proof `111_seqn_sminn.cubalc`

### Prior
See 1.12.87-universal.

## 1.12.87-universal — 2026-08-03

### Direction
- Algocube digit **5** → stack immediate bitfield + shift-by-constant (complete bit path after stack-stack SBTEST/SSETB).

### Language
- `SSETBN`/`SETBN`/`SSETBITN` n — TOS \|= (1≪n)
- `SCLRBN`/`CLRBN`/`SCLRBITN` n — TOS &= ~(1≪n)
- `SFLIPBN`/`FLIPBN`/`STGLBN` n — TOS ^= (1≪n)
- `SBTESTN`/`TESTBITN`/`SBITN` n — replace TOS with bit n (0/1)
- `SSHLN`/`SHLN` n — TOS ≪= n · `SSHRN`/`SHRN` n — logical TOS ≫= n
- Proof `110_ssetbn_stestn.cubalc`

### Prior
See 1.12.86-universal.

## 1.12.86-universal — 2026-08-02

### Direction
- Algocube digit **7** → stack immediate ALU: div/mod by constant (complete +−×÷% family).

### Language
- `SDIVN`/`DIVN`/`QUOTN` n — TOS /= n (n==0 → 0, soft)
- `SMODN`/`MODN`/`REMN` n — TOS %= n (n==0 → 0, soft)
- Proof `109_sdivn_smodn.cubalc`

### Prior
See 1.12.85-universal.

## 1.12.85-universal — 2026-08-02

### Direction
- Algocube digit **1** → data plane: stack↔cell accumulate (sub/mul/div).

### Language
- `SSUBTOC`/`SCELLSUB` — stack `i v → cells[i]-=v` leave result
- `SMULTOC`/`SCELLMUL` — stack `i v → cells[i]*=v` leave product
- `SDIVTOC`/`SCELLDIV` — stack `i v → cells[i]/=v` leave quotient
- Proof `108_ssubtoc_smultoc.cubalc`

### Prior
See 1.12.84-universal.

## 1.12.84-universal — 2026-08-02

### Direction
- Algocube digit **4** → stack immediate ALU: add/sub/mul by constant.

### Language
- `SADDN`/`PLUSN`/`ADDN` n — TOS += n
- `SSUBN`/`MINUSN`/`SUBN` n — TOS -= n
- `SMULN`/`TIMESN`/`MULN` n — TOS *= n
- Proof `107_saddn_smuln.cubalc`

### Prior
See 1.12.83-universal.

## 1.12.83-universal — 2026-08-02

### Direction
- Algocube digit **7** → stack ALU unary: inc/dec, double, half.

### Language
- `SINC`/`INCSTK`/`STACKINC` — TOS += 1
- `SDEC`/`DECSTK`/`STACKDEC` — TOS -= 1
- `SDBL`/`SDOUBLE`/`STACKDBL` — TOS *= 2
- `SHALF`/`SHALVE`/`STACKHALF` — TOS /= 2
- Proof `106_sinc_sdbl.cubalc`

### Prior
See 1.12.82-universal.

## 1.12.82-universal — 2026-08-02

### Direction
- Algocube digit **4** → stack combinators: fill top-N, conditional drop.

### Language
- `SFILL`/`FILLTOP` n v — write v into top n stack slots
- `DROPZ`/`SDROPZ`/`DROPIF0` — drop TOS if zero
- `DROPNZ`/`SDROPNZ`/`DROPIF` — drop TOS if nonzero
- Proof `105_sfill_dropz.cubalc`

### Prior
See 1.12.81-universal.

## 1.12.81-universal — 2026-08-02

### Direction
- Algocube digit **3** → stack structure: 3OVER + UNDER (complete triple plane after remote 1.12.80 2TUCK).

### Language
- `3OVER`/`TOVER`/`OVER3` — copy under-triple onto stack
- `UNDER`/`SUNDER`/`DUPUNDER` — a b → a a b
- Lexer: `3OVER` as ident
- Proof `104_3over_under.cubalc`

### Prior
See 1.12.80-universal.

## 1.12.80-universal — 2026-08-02

### Direction
- Algocube digit **8** → stack depth plane: triple ops + pair tuck.

### Language
- `3DUP`/`TDUP` — duplicate top 3
- `3DROP`/`TDROP` — drop top 3
- `2TUCK`/`DTUCK` — tuck top pair under second pair
- `3SWAP`/`TSWAP` — reverse top 3 (`a b c → c b a`)
- Lexer: `3DUP`/`3DROP`/`3SWAP`/`2TUCK` as idents
- Proof `103_3dup_2tuck.cubalc`

### Prior
See 1.12.79-universal.

## 1.12.79-universal — 2026-08-02

### Direction
- Algocube digit **1** → stack structure: keep-top-N (finish SINSERT/UNROLL plane).

### Language
- `SKEEP`/`KEEPN`/`KEEP` n — keep only top n items; drop under
- Proof `102_skeep.cubalc`

### Prior
See 1.12.78-universal.

## 1.12.78-universal — 2026-08-02

### Direction
- Algocube digit **9** → COP metrics: longest 1-run, zero-runs, first/last zero.

### Language
- `MAXRUN`/`LONGRUN` cube — longest contiguous ones run
- `ZRUNS`/`ZERORUNS` cube — count of maximal zero-runs
- `FINDZERO`/`FIRSTZERO` · `LASTZERO` cube — first/last zero index (−1 if none)
- Proof `101_maxrun_zruns.cubalc`

### Prior
See 1.12.77-universal.

## 1.12.77-universal — 2026-08-02

### Direction
- Algocube digit **2** → math plane: coprime test, ceil power-of-two, extended GCD.

### Language
- `COPRIME`/`ISCOPRIME`(a,b) · stack `SCOPRIME`
- `CEILPOW2`/`NEXTPOW2`(n) · stack `SCEILPOW2`/`SNEXTPOW2`
- `SEGCD`/`SXGCD` — stack `a b → g x y` (Bézout)
- Proof `100_coprime_ceilpow2.cubalc`

### Prior
See 1.12.76-universal.

## 1.12.76-universal — 2026-08-02

### Direction
- Algocube digit **1** → data plane: depth insert/unroll, bulk push, cell accumulate.

### Language
- `SINSERT`/`UNROLL`/`RROLL` n — move TOS to depth n (n=1≡SWAP)
- `RROT`/`NROT`/`-ROT` — fixed reverse ROT (a b c → c a b)
- `NPUSH`/`PUSHN` v n — push value v, n times
- `SADDTOC`/`SCELLADD` — stack `i v → cells[i]+=v` leave sum
- Proof `99_sinsert_npush.cubalc`

### Prior
See 1.12.75-universal.

## 1.12.75-universal — 2026-08-02

### Direction
- Algocube digit **1** → data plane: stack↔cell fetch + depth-indexed stack mutators.

### Language
- `SGETCELL`/`SLOAD`/`SFETCH` — stack `i → cells[i]`
- `SREPLACE`/`SPUT`/`SSTOREN` n — write TOS into depth-n slot, drop TOS
- `SDROPAT`/`NIPN`/`DROPAT` n — remove item at depth n (0≡DROP, 1≡NIP)
- Proof `98_sgetcell_sreplace.cubalc`

### Prior
See 1.12.74-universal.

## 1.12.74-universal — 2026-08-02

### Direction
- Algocube digit **0** → stack foundation: multi-drop, empty/full probes, depth swap.

### Language
- `NDROP`/`DROPN` n — drop top n items
- `SEMPTY`/`ISEMPTY` · `SFULL`/`ISFULL` → LAST_N predicate
- `SSWAPN`/`SWAPN` n — exchange TOS with n-th under top
- Proof `97_ndrop_sswapn.cubalc`

### Prior
See 1.12.73-universal.

## 1.12.73-universal — 2026-08-02

### Direction
- Algocube digit **9** → COP metrics: transitions, one-runs, masked popcount.

### Language
- `TRANSBITS`/`BITTRANS` cube → adjacent flip count
- `RUNSBITS`/`ONERUNS` cube → number of 1-runs
- `MASKPOP`/`POPMASK` cube mask → popcount under mask ones
- Proof `96_trans_runs_mask.cubalc`

### Prior
See 1.12.72-universal.

## 1.12.72-universal — 2026-08-02

### Direction
- Algocube digit **2** → number theory: Möbius, radical, square-free test.

### Language
- `MOBIUS`/`MU`(n) · `RADICAL`/`RAD`(n) · `ISSQUAREFREE`/`SQFREE`(n)
- Stack: `SMOBIUS`/`SMU` · `SRAD`/`SRADICAL` · `SISSQFREE`/`SSQFREE`
- Proof `95_mobius_radical.cubalc`

### Prior
See 1.12.71-universal.

## 1.12.71-universal — 2026-08-02

### Direction
- Algocube digit **9** → COP metrics: leading/trailing zeros, union pop, Jaccard.

### Language
- `CLZBITS`/`NLZBITS` cube · `CTZBITS`/`NTZBITS` cube
- `ORPOP`/`UNIONPOP` a b → popcount(a OR b)
- `JACCARD`/`SIMBITS` a b → 100·|A∩B|/|A∪B| integer percent
- Proof `94_clz_jaccard.cubalc`

### Prior
See 1.12.70-universal.

## 1.12.70-universal — 2026-08-02

### Direction
- Algocube digit **3** → COP range algebra: bitwise range ops + range reduce.

### Language
- `ANDRANGE`/`ORRANGE`/`XORRANGE` a b lo hi — in-place a[i] OP= b[i]
- `NANDRANGE`/`NORRANGE`/`XNORRANGE` · `ANDNRANGE`/`ORNRANGE`
- `ANDREDUCE`/`ALLRANGE` cube lo hi · `ORREDUCE`/`ANYRANGE` cube lo hi
- Proof `93_andrange_reduce.cubalc`

### Prior
See 1.12.69-universal.

## 1.12.69-universal — 2026-08-02

### Direction
- Algocube digit **4** → control flow: iterate set-bit indices + no-op.

### Language
- `FORBIT`/`EACHBIT` cube [AS name] ... END — loop over set bits (`IT`/`IDX`/`BIT`)
- `PASS`/`NOP`/`NOOP` — no-op statement
- Proof `92_forbit_pass.cubalc`

### Prior
See 1.12.68-universal.

## 1.12.68-universal — 2026-08-02

### Direction
- Algocube digit **9** → COP metrics: bit-dot, majority threshold, Gray code.

### Language
- `DOTBITS`/`ANDPOP` a b → popcount(a AND b)
- `MAJBITS`/`THRESHBITS` cube [k] → 1 if ones ≥ k (default strict majority)
- `GRAYBITS`/`TOGRAY` cube · `UNGRAYBITS`/`GRAY2BIN` cube
- Proof `91_dot_maj_gray.cubalc`

### Prior
See 1.12.67-universal.

## 1.12.67-universal — 2026-08-02

### Direction
- Algocube digit **8** → matrix data-path: PEXT/PDEP under mask + bit interleave.

### Language
- `PEXTBITS`/`GATHERBITS` cube mask → LAST_N (parallel extract)
- `PDEPBITS`/`SCATTERBITS` cube mask val — deposit low bits into mask positions
- `ZIPBITS` dst a b — interleave a/b into dst (Morton)
- `UNZIPBITS` even odd src — deinterleave even/odd lanes
- Proof `90_pext_zip_matrix.cubalc`

### Prior
See 1.12.66-universal.

## 1.12.66-universal — 2026-08-02

### Direction
- Algocube digit **4** → control flow: cell-range iterators + single-token break/continue-if.

### Language
- `EACH CELL` [as name] [FROM lo TO hi] ... END — iterate cells, bind value/`VAL`, `IT`/`IDX`
- `FORCELL`/`EACHCELL` [name] lo hi ... END — compact cell-range loop
- `BREAKIF` expr · `CONTINUEIF`/`CONTIF`/`SKIPIF`/`NEXTIF` expr
- Proof `89_eachcell_breakif.cubalc`

### Prior
See 1.12.65-universal.


## 1.12.65-universal — 2026-08-02

### Direction
- Algocube digit **1** → data expressiveness: local range rotate/shift on matrix bits.

### Language
- `ROTRANGE`/`ROLRANGE` cube lo hi k — rotate-left bits in [lo..hi]
- `RORRANGE` cube lo hi k — rotate-right in range
- `SHLRANGE`/`SHIFTRANGE` cube lo hi k — logical left shift (zero-fill) in range
- `SHRRANGE` cube lo hi k — logical right shift in range
- Outside [lo..hi] untouched; negative k flips direction
- Proof `88_rotrange_shift.cubalc`

### Prior
See 1.12.64-universal.



## 1.12.64-universal — 2026-08-02

### Direction
- Algocube digit **7** → COP matrix parity, bit-range copy, local reverse.

### Language
- `PARITYBITS`/`XORREDUCE` cube → XOR-reduce of all bits
- `COPYRANGE` dst doff src soff n — copy n bits between cubes
- `SWAPRANGE`/`REVRANGE` cube lo hi — reverse bits in range
- Proof `87_copyrange_parity.cubalc`

### Prior
See 1.12.63-universal.


## 1.12.63-universal — 2026-08-02

### Direction
- Algocube digit **5** → COP matrix mux + masked equality (select/match pure-C).

### Language
- `MUXBITS`/`BLENDBITS`/`SELECTBITS` dst a b mask — dst = mask?a:b per bit
- `MATCHBITS`/`EQMASK` a b mask → 1 if a≡b under mask ones
- Proof `86_muxbits_match.cubalc`

### Prior
See 1.12.62-universal.


## 1.12.62-universal — 2026-08-02

### Direction
- Algocube digit **6** → RNG range, cell shuffle/pick, random matrix bits, energy read.

### Language
- `RANDRANGE`/`RANDIN` lo hi · `SRANDRANGE` stack lo hi → rand in [lo,hi]
- `SHUFFLECELL` lo hi — Fisher–Yates (seeded)
- `PICKCELL` lo hi — random cell value (`IT`=index)
- `RANDBITS` cube [pct] — randomize matrix (density 0..100, default 50)
- `ENERGYGET` cube → energy 0..100
- Proof `85_rand_shuffle.cubalc`

### Prior
See 1.12.61-universal.


## 1.12.61-universal — 2026-08-02

### Direction
- Algocube digit **5** → COP matrix set relations (equality/subset/disjoint pure-C).

### Language
- `EQBITS`/`SAMEBITS` · `NEBITS`/`NEQBITS` a b → 0/1
- `SUBSETBITS`/`ISSUBSET` · `SUPERSETBITS`/`ISSUPERSET` a b
- `DISJOINTBITS` · `OVERLAPBITS`/`INTERSECTBITS` a b
- Proof `84_eqbits_subset.cubalc`

### Prior
See 1.12.60-universal.


## 1.12.60-universal — 2026-08-02

### Direction
- Algocube digit **8** → stack pack32 + PEXT/PDEP + bit interleave (data-path pure-C).

### Language
- `SPACK32`/`SPACKW` hi lo → 64-bit word; `SHI32`/`SLO32` unpack halves
- `SPEXT`/`SPDEP` src mask — parallel bit extract/deposit (BMI2 duals)
- `SZIP`/`SINTERLEAVE` a b — Morton interleave low 32 bits
- `SUNZIP`/`SDEINTERLEAVE` z → even, odd halves on stack
- Proof `83_pack32_pext_zip.cubalc`

### Prior
See 1.12.59-universal.


## 1.12.59-universal — 2026-08-02

### Direction
- Algocube digit **9** → cell fold arithmetic + scan + reduce (data plane pure-C).

### Language
- `SUBCELL`/`DIVCELL`/`MODCELL` lo hi k — range element-wise arith
- `SCANCELL`/`PREFIXSUM`/`CUMSUM` lo hi — in-place prefix sum
- `CLAMPCELL` lo hi min max — clamp range into bounds
- `PRODCELL`/`MEANCELL`/`AVGCELL` [lo [hi]] — product / integer mean reduce
- Proof `82_cell_div_scan.cubalc`

### Prior
See 1.12.58-universal.


## 1.12.58-universal — 2026-08-02

### Direction
- Algocube digit **3** → COP matrix range fill/clear/flip/count (bit-block ops pure-C).

### Language
- `FILLRANGE`/`SETRANGE` cube lo hi [val] — set bits [lo..hi] (default val=1)
- `CLEARRANGE`/`CLRRANGE`/`ZERORANGE` cube lo hi — clear range
- `FLIPRANGE`/`NOTRANGE`/`INVERTRANGE` cube lo hi — invert range
- `COUNTRANGE`/`ONESRANGE`/`POPRANGE` cube lo hi → `LAST_N` ones in range
- Proof `81_range_bits.cubalc`

### Prior
See 1.12.57-universal.


## 1.12.57-universal — 2026-08-02

### Direction
- Algocube digit **1** → control flow: C-style ternary expressions + unbounded FOREVER loops.

### Language
- Expression ternary: `cond ? then : else` (right-associative; after AND/OR)
- `FOREVER` / `LOOPINF` / `INFINITE` / `LOOPFOREVER` ... `END` — loop until `BREAK` (guard 1e5)
- Proof `80_ternary_forever.cubalc`

### Prior
See 1.12.56-universal.


## 1.12.56-universal — 2026-08-02

### Direction
- Algocube digit **8** → COP matrix↔word data path (pack/extract bitfields pure-C).

### Language
- `WORDFROM`/`MAT2WORD`/`BITS2WORD`/`BITS2N`/`LOADWORD` cube → `LAST_N` from bits 0..63
- `WORDTO`/`WORD2MAT`/`N2BITS`/`STOREWORD`/`WORD2BITS` cube n — deposit word into low bits
- `EXTRACTBITS`/`GETBITS`/`SLICEBITS`/`BITFIELD`/`FIELDGET` cube lo hi → field value
- `DEPOSITBITS`/`PUTBITS`/`SETBITS`/`FIELDSET`/`INJECTBITS` cube lo hi val — write field
- Proof `79_word_bits_bridge.cubalc`

### Prior
See 1.12.55-universal.


## 1.12.55-universal — 2026-08-02

### Direction
- Algocube digit **7** → COP matrix reorder + complete boolean algebra + Hamming distance.

### Language
- `REVBITS`/`REVERSEBITS`/`BITREVM`/`MIRRORBITS` cube — reverse bit order in-place
- `SWAPBIT`/`XCHGBIT`/`EXCHBIT`/`SWBIT` cube i j — exchange two bit positions
- `XNORBITS`/`EQVBITS`/`NXORBITS` · `NORBITS` · `ANDNBITS`/`BICBITS` · `ORNBITS` dst src
- `DIFFBITS`/`HAMBITS`/`HAMMINGBITS`/`BITDIFF`/`XORDIST` a b → `LAST_N` = Hamming distance
- Proof `78_revbits_diff.cubalc`

### Prior
See 1.12.54-universal.


## 1.12.14-universal — 2026-08-02

### Direction
- Algocube digit **4** → stack select / within / clamp + zero-tests (control predicates pure-C).

### Language
- Zero-tests (unary TOS): `SZ`/`S0EQ` · `SNZ`/`S0NE` · `S0LT` · `S0GT` · `SSIGN`/`SGN` (−1/0/1)
- `SSEL`/`STACKSEL` — f t cond → (cond ? t : f)
- `SWITHIN`/`WITHIN` — n lo hi → 1 if lo ≤ n < hi
- `SCLAMP`/`STACKCLAMP` — n lo hi → clamp into [lo,hi]
- Proof `37_stack_select_clamp.cubalc`

### Prior
See 1.12.13-universal.

## 1.12.13-universal — 2026-08-02

### Direction
- Algocube digit **1** → stack compare + min/max (control predicates on the stack).

### Language
- `SEQ`/`SNE`/`SLT`/`SGT`/`SLE`/`SGE` (also `STACK*` / `CMP*`) — pop a b, push 0/1
- `SMIN`/`SMAX` (`STACKMIN`/`STACKMAX`) — ordered select of top two
- Proof `36_stack_compare.cubalc`

### Prior
See 1.12.12-universal.

## 1.12.12-universal — 2026-08-02

### Direction
- Algocube digit **1** → broaden computational expressiveness (stack bitwise ALU pure-C).

### Language
- Stack bitwise: `SAND`/`STACKAND`/`BANDST` · `SOR`/`STACKOR`/`BORST` · `SXOR`/`STACKXOR`/`BXORST`
- Unary: `SNOT`/`STACKNOT`/`BNOTST`/`SINVERT`
- Shifts: `SSHL`/`STACKSHL`/`SLSHL` · `SSHR`/`STACKSHR`/`SLSHR` (shift amount clamped 0..63)
- Proof `35_stack_bitops.cubalc`

### Prior
See 1.12.11-universal.

## 1.12.11-universal — 2026-08-02

### Direction
- Algocube digit **7** → stack ALU: Forth-style binary/unary arithmetic on the stack.

### Language
- `ADD`/`SUB`/`MUL`/`DIV`/`MOD` — pop a b, push result (OK/LAST_N/SP)
- `SNEG`/`STACKNEG`/`NEGATE` — negate TOS
- `SABS`/`STACKABS` — absolute value of TOS
- Proof `34_stack_alu.cubalc`

### Prior
See 1.12.10-universal.

## 1.12.10-universal — 2026-08-02

### Direction
- Algocube digit **2** → math plane ext: square, floor-div, binomial/permutation.

### Language
- `SQR`/`SQUARE`(n) — n²
- `DIVFLOOR`/`FLOORDIV`(a,b) — floor division (toward −∞)
- `BINOM`/`CHOOSE`(n,k) — binomial coefficient C(n,k)
- `PERM`/`PNR`(n,k) — P(n,k) = n!/(n−k)!
- Proof `33_math_binom_perm.cubalc`

### Prior
See 1.12.9-universal.

## 1.12.9-universal — 2026-08-02

### Direction
- Algocube digit **0** → foundation bitfield + ceil-div plane.

### Language
- `BEXT`/`BITEXT`(val, pos, width) — extract bit field
- `BDEP`/`BITDEP`(base, field, pos) — deposit low 8 bits of field at pos
- `BYTE`(val, i) · `LOBYTE`/`HIBYTE` — little-endian byte access
- `DIVCEIL`/`CEILDIV`(a, b) — integer ceiling division
- Proof `32_bitfield_divceil.cubalc`

### Prior
See 1.12.8-universal.

## 1.12.8-universal — 2026-08-02

### Direction
- Algocube digit **9** → cell logic plane: bitwise range masks + equality predicate.

### Language
- `ANDCELL`/`CELLAND` lo hi mask — bitwise AND each cell in range with mask
- `ORCELL`/`CELLOR` lo hi mask — bitwise OR each cell in range with mask
- `XORCELL`/`CELLXOR` lo hi mask — bitwise XOR each cell in range with mask
- `NOTCELL`/`CELLNOT` lo hi — bitwise NOT (~) each cell in range
- `EQCELL`/`CELLEQ` lo hi val — set cell to 1 if == val else 0; LAST_N = hit count
- Proof `31_cell_logic.cubalc`

### Prior
See 1.12.7-universal.

## 1.12.7-universal — 2026-08-02

### Direction
- Algocube digit **8** → stack↔cell bridge: block transfer between stack and cells.

### Language
- `TOCELL`/`>CELL` dst [n] — pop n stack values into cells[dst..] (TOS → highest)
- `FROMCELL`/`CELL>`/`PUSHCELL` src [n] — push cells[src..] onto stack
- Proof `30_stack_cell_bridge.cubalc`

### Prior
See 1.12.6-universal.


## 1.12.6-universal — 2026-08-02

### Direction
- Algocube digit **9** → cell fold ext: argmin/argmax + range rotate/shift.

### Language
- `MINIDX`/`ARGMIN` [lo [hi]] — first index of minimum in range (stmt + expr)
- `MAXIDX`/`ARGMAX` [lo [hi]] — first index of maximum in range (stmt + expr)
- `ROTCELL`/`CELLROT` lo hi k — rotate range left by k (k<0 right)
- `SHIFTCELL`/`CELLSHIFT` lo hi k — shift with zero-fill (k>0 left, k<0 right)
- Proof `29_cell_argmin_rot.cubalc`

### Prior
See 1.12.5-universal.

## 1.12.5-universal — 2026-08-02

### Direction
- Algocube digit **9** → cell fold plane: range arithmetic + iota + sort.

### Language
- `ADDCELL`/`CELLADD` lo hi delta — add to each cell in range
- `MULCELL`/`CELLMUL` lo hi k — scale each cell in range
- `IOTA`/`SEQCELL` lo hi [start [step]] — arithmetic sequence fill
- `SORTCELL`/`CELLSORT` lo hi [ASC|DESC|dir] — insertion sort (DESC or 0 = descending)
- Proof `28_cell_arith_sort.cubalc`

### Prior
See 1.12.4-universal.

## 1.12.4-universal — 2026-08-02

### Direction
- Algocube digit **8** → stack depth plane: double-width combinators + ROLL/DEPTH.

### Language
- `NIP` — drop under top (a b → b)
- `TUCK` — b under a, keep b (a b → b a b)
- `2DUP`/`DDUP` · `2DROP`/`DDROP` · `2SWAP`/`DSWAP`
- `ROLL` n — rotate top (n+1) items (n=1≡SWAP, n=2≡ROT)
- `DEPTH` — push current stack depth
- Proof `27_stack_depth_universal.cubalc`

### Prior
See 1.12.3-universal.

## 1.12.3-universal — 2026-08-02

### Direction
- Algocube digit **2** → math plane: integer log2 / bit ranks / digit folds / modular inverse.

### Language
- `ILOG2`/`LOG2`(n) — floor log2; -1 if n≤0
- `CTZ`(n) / `CLZ`(n) — trailing / leading zeros (64-bit word; 0→64)
- `ISPOW2`(n) / `POW2`(k) — power-of-two test / 2^k (k 0..62)
- `NDIGITS`(n) / `DIGSUM`(n) — decimal digit count / digit sum
- `MODINV`/`INVMOD`(a,m) — modular inverse via extended Euclid (0 if none)
- Proof `26_math_ilog_modinv.cubalc`

### Prior
See 1.12.2-universal.

## 1.12.2-universal — 2026-08-02

### Direction
- Algocube digit **5** → cell memory plane: block copy/move + find/count + reverse.

### Language
- `COPYCELL`/`CELLCOPY`/`CMOVE` src dst n — overlap-safe cell block copy
- `MOVECELL`/`CELLMOVE` src dst n — copy then clear non-overlapping source
- `FINDCELL`/`CELLFIND` val [lo [hi]] — first index or -1 (`OK` = found); expr form too
- `COUNTCELL`/`CELLCOUNT` val [lo [hi]] — count matches; expr form too
- `REVCELL`/`CELLREV` lo hi — reverse cell range in place
- Proof `25_cell_mem_universal.cubalc`

### Prior
See 1.12.1-universal.

## 1.12.1-universal — 2026-08-02

### Direction
- Algocube digit **4** → data-plane stack combinators + cell fill.

### Language
- `DUP`/`DROP`/`SWAP`/`OVER`/`ROT` — Forth-style stack ops (OK/LAST_N/SP)
- `PICK` n — copy n-th under top (0=TOS) onto stack
- `FILLCELL`/`CELLFILL` lo hi val — fill cell range
- Proof `24_stack_ops_universal.cubalc`

### Prior
See 1.12.0-universal.


## 1.12.0-universal — 2026-08-02

### Direction
- Algocube digit **6** → energy / matrix flow + seeded RNG.

### Language
- `RAND`/`RND` [max] · `RAND(n)` (seed: `CUBALC_SEED` env)
- `ENERGYSET`/`ENERGYADD` cube n (0..100 scale)
- `ROTBITS`/`SHIFTBITS` cube k (State Matrix bit rotate; negative = right)
- Proof `23_rand_energy_rotbits.cubalc`

### Prior
See 1.11.9-universal.

## 1.11.9-universal — 2026-08-02

### Direction
- Algocube digit **9** → cell fold + INC/DEC (loop-friendly data).

### Language
- `INC`/`DEC` name [step] · `INC CELL`/`DEC CELL` i [step]
- `SUMCELL`/`MINCELL`/`MAXCELL` [lo [hi]] (stmt + expr forms)
- Proof `22_cell_fold_inc.cubalc`

### Prior
See 1.11.8-universal.

## 1.11.8-universal — 2026-08-02

### Direction
- Algocube digit **4** → control flow: FN return + CASE.

### Language
- `RET`/`RETURN` [expr] early exit from `FN` body (`RETVAL`/`LAST_N`)
- `CASE`/`SWITCH` … `WHEN` … `DEFAULT` … `END`
- Proof `21_fn_return_case.cubalc`

### Prior
See 1.11.7-universal.

## 1.11.7-universal — 2026-08-02

### Direction
- Algocube digit **3** → string plane (control + data interchange).

### Language
- `SYS CAT`/`STRCAT` · `SYS FIND`/`INDEX` · `SYS EQS`/`STREQ`
- `SYS HAS`/`CONTAINS` · `SYS REVS`/`STRREV` · `SYS UPPER` · `SYS LOWER`
- Proof `20_string_ops_universal.cubalc`
- Tree restore from HEAD if mass-delete corruption

### Prior
See 1.11.6-universal.

## 1.11.6-universal — 2026-08-02

### Direction
- Algocube digit **2** → math plane (modular + number theory).

### Language
- `ADDMOD` `SUBMOD` `MULMOD` `POWMOD` · `FIB`/`FIBONACCI` · `ISPRIME`/`PRIMEP`
- `IDIV` `IMOD` named integer div/mod
- Proof `19_math_modular_universal.cubalc`
