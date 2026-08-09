# CubalC cookbook — usable recipes

**HOLD_FLASH is a device/firmware connection safeguard** (default 1 — not a
program preamble). Wire is SMX2 binary (HTTP never required for peers).

## 0. Doctor

```bash
make all
./out/cubalc doctor      # JSON readiness
./out/cubalc version     # language version JSON (also: VERSION in-program)
./out/cubalc paths       # cwd/state/lib/docs layout JSON
./out/cubalc cookbook    # paths
./out/cubalc libs        # programs/lib INCLUDE catalog (JSON)
./out/cubalc env [pfx]   # host CUBALC_* contract (set/default/hint JSON)
./out/cubalc examples    # curated runnable starters (JSON · filterable)
./out/cubalc cat agent_boot  # dump lib/program source + meta plate
./out/cubalc init --plate my_agent   # scaffold plate_session starter
./out/cubalc init --peer mesh        # multi-plate PLATE+PEER starter
./out/cubalc forms SMX   # play-form catalog
# agents: pipe source without a temp file (no HOLD_FLASH tax)
printf 'ASSERT 1 == 1\nPRINT "piped"\n' | ./out/cubalc run -
./out/cubalc help
```

## 1. Hello place → plug → assert

```cubalc
// hold_flash defaults to 1 — no HOLD_FLASH line needed
CUBE a ROLE host PROTON 1
CUBE b ROLE body PROTON 1
SETBIT a 0 1
SETBIT a 1 1
SETBIT b 0 1
PLUG a b
ASSERT OK == 1 "plug ok under default hold_flash"
PRINT "hello" CUBES UNITY
```

```bash
./out/cubalc run programs/hello_cube.cubalc
./out/cubalc run programs/proof/12_hold_flash_plug.cubalc
# device/firmware-only seed: cubalc cat hold_seed · docs/HOLD_FLASH.md
```

### Soft checks (multi-probe without abort)

```cubalc
# EXPECT = soft ASSERT — continues; OK/EXPECT_OK + sticky LAST_ERR
EXPECT SMX_OK == 1 "peer may be offline"
IF OK == 0 THEN
  FAIL "probe soft-fail"          # intentional soft status (no fake expr)
  PRINT "probe" LAST_ERR
END
PASS "ready for hard gate"
ASSERT 1 == 1 "hard gate still fail-closed"
```

Proof: `programs/proof/578_expect_soft.cubalc` · `579_fail_pass.cubalc` · `581_last_err_plate.cubalc`.

Run result JSON always includes `last_err` (sticky soft FAIL/EXPECT reason) even when `"ok":true`.

## 2. Decide from matrix

```cubalc
CUBE peer0 ROLE host PROTON 1
CUBE brain ROLE braincube PROTON 1
PLUG peer0 brain
FOLDBITS peer0 "10101011110000"
SETDIGIT peer0 5
DECIDE brain
ASSERT DECIDE >= 0 "digit 0..9"
ASSERT DECIDE <= 9
```

## 3. In-process SMX mesh

```cubalc
CUBE a ROLE host PROTON 1
CUBE b ROLE body PROTON 1
PLUG a b
SETBIT a 0 1
SETBIT a 1 1
SETBIT a 2 1
SMX KEY
SMX EXCHANGE a b
ASSERT SMX_OK == 1 "secure talk"
ASSERT SMX_TALKS >= 2
```

```bash
export CUBALC_SMX_KEY="$(openssl rand -hex 32)"
./out/cubalc run programs/p2p/mesh_local.cubalc
```

## 4. Cross-process P2P (two shells)

```bash
export CUBALC_SMX_KEY="$(openssl rand -hex 32)"
# optional: fail faster if no peer dials
export CUBALC_P2P_TIMEOUT=5000   # ms for SMX SERVE accept

# shell B
CUBALC_P2P_SERVE=1 CUBALC_P2P_BIND=127.0.0.1:7733 \
  ./out/cubalc run programs/p2p/nanobot_peer.cubalc

# shell A
CUBALC_P2P_PEER=127.0.0.1:7733 \
  ./out/cubalc run programs/p2p/nanobot_peer.cubalc

# soft dial (no fatal if peer missing)
CUBALC_P2P_SOFT=1 CUBALC_P2P_PEER=127.0.0.1:1 \
  ./out/cubalc run programs/p2p/peer_dial.cubalc
```

CLI bus:

```bash
./out/cubalc smx-bus prove-tcp
```

## 5. Core protect

```bash
export CUBALC_PROTECT=1
./out/cubalc protect
./out/cubalc run programs/protect/core_protect.cubalc
```

## 6. INCLUDE a lib snippet

From a program under `programs/…` (INCLUDE resolves relative to file dir, `programs/`, and **short names → `programs/lib/`**):

```cubalc
INCLUDE hold_seed                    # short form → programs/lib/hold_seed.cubalc
# or: INCLUDE "lib/hold_seed.cubalc"
CUBE peer0 ROLE host PROTON 1
CUBE peer1 ROLE body PROTON 1
PLUG peer0 peer1
SETBIT peer0 0 1
SETBIT peer1 1 1
INCLUDE mesh_exchange                # SMX KEY + EXCHANGE rounds
ASSERT SMX_OK == 1 "mesh talk"
ASSERT SMX_TALKS >= 4
PRINT_JSON INCLUDE_PATH SMX_OK       # resolved path after INCLUDE
```

Libs: `hold_seed` · `peer_decide` · `mesh_exchange` under `programs/lib/` (`cubalc libs`).  
Proof: `programs/proof/577_include_shortname.cubalc` · `include_mesh_exchange.cubalc`.

## 7. SYS ENV / ARG with default

```cubalc
SYS ENV "CUBALC_P2P_BIND" OR "127.0.0.1:7733"
PRINT "bind" LAST LAST_N
ASSERT LAST_N > 0 "bind string present"

# Script args: host sets CUBALC_ARG0… or program supplies fallback
SYS ARG 0 OR "peer0"
SYS ARG 1 OR "7733"
PRINT "args" LAST
```

## Env cheat sheet

Live catalog (set/default/hint JSON): `./out/cubalc env` · filter `cubalc env P2P`.

| env | use |
|-----|-----|
| `CUBALC_SMX_KEY` | 64-hex shared secret |
| `CUBALC_P2P_BIND` / `PEER` / `SERVE` | mesh |
| `CUBALC_P2P_TIMEOUT` | SERVE accept timeout ms (default 30000) |
| `CUBALC_P2P_SOFT` | DIAL soft-fail (SMX_OK=0) |
| `CUBALC_PROTECT` | tight host SPAWN/HTTP |
| `CUBALC_STATE` | state directory |

## 8. Agent durable state plates

One INCLUDE starts a version-gated agent program with a durable JSON plate:

```cubalc
DEFAULT PLATE_PATH = "state/my_agent.json"
DEFAULT PLATE_SEED = "{\"n\":0,\"ok\":true}"
INCLUDE plate_session          # agent_boot + plate_boot

SETP "status" "ready"          # mutate PLATE (no LET glue)
INCP "n"
NEEDP "n" "ok" "status"        # fail-fast contract
DUMPP                          # cubalc.plate_info.v1 snapshot

SYS JSONOBJ "peer" "a"
LET PLATE_PATCH = LAST
INCLUDE plate_patch            # multi-key overlay + save

# or after ad-hoc SETP/MERGEP:
INCLUDE plate_save
```

CLI one-shots (no `.cubalc` file):

```bash
./out/cubalc plate show state/my_agent.json
./out/cubalc plate get  state/my_agent.json n
./out/cubalc plate set  state/my_agent.json role worker
./out/cubalc plate inc  state/my_agent.json n
./out/cubalc plate ensure state/my_agent.json '{"n":0,"ok":true}'   # create-or-keep
./out/cubalc plate merge  state/my_agent.json '{"status":"ready"}'  # multi-key overlay
./out/cubalc plate merge  state/peer.json @patch.json
./out/cubalc plate eq     state/a.json state/b.json                # order-indep equal
./out/cubalc plate diff   state/a.json state/b.json                # changed keys
./out/cubalc plate changelog state/a.json state/b.json             # key: old → new lines
./out/cubalc plate has  state/my_agent.json n ok status            # multi-key soft contract
./out/cubalc plate need state/my_agent.json n ok                   # hard gate (ok:false on miss)
./out/cubalc libs | grep plate
```

`ensure` does not clobber an existing object plate (dual of `ENSUREPLATE`).  
`merge` applies overlay keys and saves (dual of `JSONFILEMERGE` / `MERGEP` on disk).  
`eq` / `ne` / `diff` / `changelog` compare two plate files (dual of `JSONEQ` / `JSONCHANGED` / `JSONCHANGELOG`) — exit `0` when plates match.  
`has` / `need` multi-key presence (dual of `HASPALL` / `NEEDP`) — `need` sets `ok:false` when any key is missing.

Libs: `plate_session` · `plate_boot` · `plate_save` · `plate_patch` · `plate_tick` · `agent_boot`.  
Forms: `SETP`/`INCP`/`MERGEP`/`NEEDP`/`DUMPP`.  
Proofs: `1117_plate_session` · `1110_setp` · `1115_plate_patch` · `1116_dumpp` · `1112_cli_plate.sh` · `1140_cli_plate_ensure_merge.sh` · `1141_cli_plate_eq_diff.sh` · `1142_cli_plate_changelog.sh` · `1144_cli_plate_has_need.sh`.

## 9. Multi-plate PEER state (PLATE + PEER)

Keep a conventional agent `PLATE` and a second named plate (`PEER`) without
`LET PLATE = other` clobber:

```bash
./out/cubalc init mesh --peer --force   # scaffold plate_peer_session starter
./out/cubalc libs | grep plate_peer
```

```cubalc
DEFAULT PLATE_PATH = "state/agent.json"
DEFAULT PLATE_SEED = "{\"n\":0,\"ok\":true}"
DEFAULT PEER_PATH = "state/peer.json"
DEFAULT PEER_SEED = "{\"agent\":\"local\",\"n\":0}"
INCLUDE plate_peer_session             # agent_boot + PLATE + PEER

SETP "status" "ready"                  # mutate PLATE
SETP FROM PEER "host" "cubeB"          # mutate PEER (PLATE untouched)
INCP FROM PEER "n"
NEEDP FROM PEER "agent" "host"
DUMPP FROM PEER
FILLP FROM PEER "peer={{agent}}@{{host}}"

INCLUDE plate_both_save                # write PLATE + PEER one INCLUDE
# or separately: plate_save · plate_peer_save
# or one-shot peer tick:
# INCLUDE plate_peer_tick              # seed + bump + ts + save PEER
```

Top-level disk plane (no `SYS`):

| form | use |
|------|-----|
| `LOADP INTO name path` | soft load into named var |
| `SEEDP INTO name path` | create-or-load into named var |
| `SAVEP` / `SAVEP FROM name path` | persist PLATE or named plate |

Mutate/probe (multi-plate `FROM`): `SETP`/`GETP`/`INCP`/`MERGEP`/`NEEDP`/`HASP`/`KEYSP`/`DUMPP`/`FILLP`/`FILLPFILE`.

Compare plates (no `SYS JSONEQ` glue):

```cubalc
EQP PLATE PEER              # soft equal · LAST_N 0|1
NEQP PLATE PEER             # inverse
DIFFP PLATE PEER            # changed key bag → LAST
CHANGELOGP PLATE PEER       # key: old → new lines → LAST
REQUIRE EQP PLATE PEER      # fail-fast if unequal · lists changed keys
REQUIRE NEQP before after   # fail-fast if still equal (mutation gate)
```

Libs: `plate_peer` · `plate_peer_save` · `plate_peer_session` · `plate_peer_tick` · `plate_both_save`.  
Proofs: `1137_plate_peer` · `1138_init_peer.sh` · `1139_plate_peer_tick` · `1143_plate_both_save` · `1145_eqp` · `1130_setp_from` · `1136_loadp`.

