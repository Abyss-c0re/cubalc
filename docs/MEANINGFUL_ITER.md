# Meaningful language iteration (10-minute loop)

**Goal:** extend **language usability** — features people and agents *use* — not combinatorial duals of the same field op.

The 6-minute universal loop may still fill ISA gaps. This loop ships **one human-meaningful commit** every ~10 minutes.

## Forbidden (do not ship)

- Width ladders of an already-landed family (`S0EQ32N` then `S0EQ16N` then `S0EQ8N` …) unless that family is **brand new** and incomplete mid-feature.
- Dual-only mirrors (`D*` copy of existing `S*`) as the sole change of a tick.
- Version plate-only commits.
- Thrash restore churn without a product delta.
- Demoting or deleting: HOLD_FLASH plug permission, core-protect, SMX P2P, modular `src/lang/`.

## Preferred backlog (pick the highest unfinished item)

### A. Ergonomics (agents + humans)

1. ~~**`INCLUDE "path"`** + `programs/lib/`~~ **done** (`lib/hold_seed`, `lib/peer_decide`, proof 574).
2. ~~**Better fail messages** on SMX~~ **done** (line + hint on SERVE/DIAL/TALK).
3. ~~**`ASSERT` with message string**~~ **done** (core + proof 573).
4. ~~**`PRINT` formatting** · `PRINT_JSON` / `DUMP`~~ **done** (named + bare snapshot).
5. ~~**`HELP form`** / CLI `cubalc forms [prefix]`~~ **done** (human plane catalog + JSON).

### B. Host / mesh usability

6. ~~**`SMX SERVE` timeout**~~ **done** (`CUBALC_P2P_TIMEOUT` ms, default 30s).
7. ~~**`cubalc protect status`**~~ **done** (JSON plate summary, no board run).
8. ~~**`SYS ENV` default**~~ **done** (`SYS ENV "NAME" OR "fallback"`).
9. ~~**Soft-fail DIAL**~~ **done** (`CUBALC_P2P_SOFT=1`).

### C. Standard library (CubalC source)

10. ~~`programs/lib/hold_seed.cubalc`~~ **done**.
11. ~~`programs/lib/mesh_exchange.cubalc`~~ **done** (INCLUDE EXCHANGE loop + proof).
12. ~~`programs/lib/peer_decide.cubalc`~~ **done** (recipe).

### D. CLI product

13. ~~**`cubalc doctor`**~~ **done**.
14. ~~**`cubalc cookbook`**~~ **done**.
15. ~~**Help rewrite**~~ **done** (grouped).

### E. Proof & docs that unlock use

16. ~~**Cookbook**~~ **done** (`docs/COOKBOOK.md`).
17. Wire a **missing doc form that is promised** only if you implement the form too (no doc-only lies).
18. ~~**`PRINT_JSON`** / agent dump of vars~~ **done**.
19. ~~**`cubalc forms [prefix]`** live op listing~~ **done**.
20. ~~SERVE/DIAL error hints + soft timeout/connect messages~~ **done**.
21. ~~**`SYS ARG` OR fallback**~~ **done** (script defaults without shell glue).
22. ~~**In-language `HELP [form]`**~~ **done** (LAST/OK/HELP_N + CLI forms).
23. ~~**Sticky `ERR` / `LAST_ERR`**~~ **done** (soft-fail + fatal; survives LAST overwrite).
24. ~~**`cubalc libs`**~~ **done** (programs/lib catalog JSON for INCLUDE).
25. ~~**`cubalc env`**~~ **done** (host CUBALC_* contract JSON · set/default/hint).
26. ~~**INCLUDE short lib name**~~ **done** (`INCLUDE hold_seed` → programs/lib/… + INCLUDE_PATH).
27. ~~**`cubalc examples`**~~ **done** (curated runnable starters JSON · tags/filter).
28. ~~**`EXPECT` soft assert**~~ **done** (OK/LAST_ERR · no fatal · multi-check probes).
29. ~~**`cubalc run -` stdin**~~ **done** (pipe programs without temp files).
30. ~~**`cubalc cat`**~~ **done** (dump lib/program source + cubalc.cat.v1 plate).
31. ~~**`FAIL` / `PASS` soft status**~~ **done** (sticky OK/LAST_ERR without fake expr).
32. ~~**`VERSION` + `cubalc version`**~~ **done** (in-lang LAST + CLI JSON plate).
33. ~~**run plate `last_err`**~~ **done** (sticky LAST_ERR on JSON even when ok).
34. ~~**`cubalc paths`**~~ **done** (install/workspace layout JSON · present flags).
35. ~~**`cubalc which` / SYS WHICH lib**~~ **done** (resolve name → path/kind · INCLUDE-style).
36. ~~**run plate `err_line` / `err_src`**~~ **done** (source snippet when err cites line N).
37. ~~**`cubalc search`**~~ **done** (keyword discovery across forms/libs/examples/env/docs).
38. ~~**`REQUIRE VERSION`**~~ **done** (fail-fast if runtime older than need).
39. ~~**`cubalc selftest`**~~ **done** (live curated usability proofs JSON plate).
40. ~~**`SYS CWD` / `STATE` / `ROOT`**~~ **done** (layout paths in-language for portable scripts).
41. ~~**`INCLUDE OR|SOFT`**~~ **done** (optional module · OK=0 sticky LAST_ERR, no fatal).
42. ~~**`VARS`**~~ **done** (dump all program vars as cubalc.vars.v1 JSON for agents).
43. ~~**`cubalc run -q` / `CUBALC_QUIET`**~~ **done** (plate-only run · no board noise).
44. ~~**`programs/lib/agent_boot`**~~ **done** (INCLUDE agent_boot = REQUIRE + hold_seed + VERSION).
45. ~~**`STATUS`**~~ **done** (cubalc.status.v1 health plate · ok/last_err/version/time).
46. ~~**`CLEAR_ERR`**~~ **done** (wipe sticky ERR/LAST_ERR after soft recovery).
47. ~~**`cubalc init`**~~ **done** (scaffold agent_boot starter · cubalc.init.v1).
48. ~~**`cubalc run -s` / `CUBALC_STRICT`**~~ **done** (soft last_err fails exit + plate ok).
49. ~~**`REQUIRE LIB`**~~ **done** (fail-fast if INCLUDE-style module missing).
50. ~~**`SYS MS`**~~ **done** (wall milliseconds · catalog SYS TIME/MS).
51. ~~**`NOTE`**~~ **done** (agent breadcrumb · LAST/NOTE · no OK/ERR change).
52. ~~**`EXIT`**~~ **done** (halt program · code/reason · plate exit_code/halted).
53. ~~**`INCLUDE ONCE`**~~ **done** (skip reload of already-loaded module path).
54. ~~**`SYS PID` / `HOSTNAME`**~~ **done** (process id + machine name for agents).
55. ~~**`SYS USER` / `UID` / `HOME`**~~ **done** (login + uid + home dir without shell).
56. ~~**`DEFAULT name = value`**~~ **done** (set-if-unset · INCLUDE-safe lib knobs).
57. ~~**`DEFINED name`**~~ **done** (var existence probe · LAST_N 0|1).
58. ~~**`TYPEOF name`**~~ **done** (kind probe · undef|num|str · LAST_N 0|1|2).
59. ~~**`UNSET name`**~~ **done** (remove var · DEFAULT re-apply · LAST_N 0|1).
60. ~~**`IDENTITY`**~~ **done** (cubalc.identity.v1 plate · user@host:pid + vars).

## Tick protocol

1. `cd` to cubalc root; `git status` — restore thrash only if needed (`git restore` tracked sources).
2. Read this file + `docs/STRUCTURE.md` + current `CUBALC_LANG_VERSION`.
3. Pick **one** backlog item (or invent equally high-leverage usability).
4. Implement the smallest complete slice.
5. Add/adjust a **proof** under `programs/proof/` or a CLI smoke.
6. `make all` + targeted run + keep suite green if cheap; private `CUBALC_STATE` for runs.
7. Commit with a **human sentence** subject (not only opcodes), e.g.  
   `feat: ASSERT message strings for agent-readable failures`  
   Optional version bump only when the surface is user-visible.
8. `git push origin main` when green.
9. Write plate `state/MEANINGFUL_ITER.json` with `{iter, item, commit, summary}`.

## Success metric

After N ticks, a new user/agent can do something **new** without reading opcode soup — not “three more width duals.”
