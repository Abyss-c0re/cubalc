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
2. **Better fail messages** on SMX still thin (keep improving).
3. ~~**`ASSERT` with message string**~~ **done** (core + proof 573).
4. ~~**`PRINT` formatting** · `PRINT_JSON` / `DUMP`~~ **done** (named + bare snapshot).
5. **`HELP form`** or CLI `cubalc forms [prefix]` listing live ops from one plane.

### B. Host / mesh usability

6. ~~**`SMX SERVE` timeout**~~ **done** (`CUBALC_P2P_TIMEOUT` ms, default 30s).
7. ~~**`cubalc protect status`**~~ **done** (JSON plate summary, no board run).
8. ~~**`SYS ENV` default**~~ **done** (`SYS ENV "NAME" OR "fallback"`).
9. ~~**Soft-fail DIAL**~~ **done** (`CUBALC_P2P_SOFT=1`).

### C. Standard library (CubalC source)

10. ~~`programs/lib/hold_seed.cubalc`~~ **done**.
11. `programs/lib/mesh_exchange.cubalc` · documented EXCHANGE loop snippet.
12. ~~`programs/lib/peer_decide.cubalc`~~ **done** (recipe).

### D. CLI product

13. ~~**`cubalc doctor`**~~ **done**.
14. ~~**`cubalc cookbook`**~~ **done**.
15. ~~**Help rewrite**~~ **done** (grouped).

### E. Proof & docs that unlock use

16. ~~**Cookbook**~~ **done** (`docs/COOKBOOK.md`).
17. Wire a **missing doc form that is promised** only if you implement the form too (no doc-only lies).
18. ~~**`PRINT_JSON`** / agent dump of vars~~ **done**.
19. **`cubalc forms [prefix]`** live op listing.
20. SERVE/DIAL remaining error hints + retry helpers.

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
