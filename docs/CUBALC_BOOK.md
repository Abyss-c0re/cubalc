# The CubalC Book  
### Cube Language · LEGO OOP · Nanobot Atom API

**All Hail the Cube · All Hail NexusCore**

| Field | Value |
|-------|--------|
| Language | **CubalC** (also *CubicalC* / *CubacalC*) |
| Runtime | Pure **C** (`cubalc/out/cubalc`) |
| SoT | **StateMatrix** bits (0/1) |
| Law engine | **Algocube** digits 0–9 |
| Atom core | **Nanobot** inside every Cube |
| Proton | **1 = create** · **0 = destroy** |
| Transport | Binary matrix frames (`CBLC`) |
| Creator surface | viz LEGO studs · budget ≤ **40** |
| Dev API | Cube way `http://127.0.0.1:17333` |
| Edge API | Nanobot peer `…/peer/v1/*` (token) |

This book is the **single manual** for humans *and* nanobots.  
Hot path stays **matrix / plates / impulses** — not ideology prose.

---

## 0. One-sentence definition

**CubalC is the language of visible Cubes:** each Cube is an object you can see in VR, plug like LEGO when matrices match, and whose heart is a nanobot atom that speaks **binary StateMatrices** and obeys **create/destroy protons**.

---

## 1. Cube Law (binding)

| Law | CubalC meaning |
|-----|----------------|
| **The Cube is SoT** | Inner `StateMatrix` wins over labels |
| **Algocube enforces** | Digit 0–9 from matrix → intent tag |
| **Nanobot is the atom** | Every Cube has one atom core |
| **Proton 0/1** | Destroy / create impulse |
| **Compatibility = matrix** | Plug only if `compat ≥ 0.35` |
| **Talk = binary** | Packed `CBLC` frames transfer matrices |
| **HOLD_FLASH** | Sticky; never auto-flash |
| **Share** | `state_matrix_only` off-box |
| **OS is a way** | stream / Android / viz are channels, not sovereign |
| **Devices free** | No forced ownership of edge peer / Switch |
| **Budget ≤ 40** | Hard lag guard on viz |
| **All Hail the Cube** | Creed line for status only |

**Forbidden:** synthetic human as SoT, prose dumps on SMX bus, loopback-only peers pretending to be hive hands, inventing HUMAN_CONFIRM.

---

## 2. Mental model (even an octopus can play)

```
                    Initial State Matrix
                    (from NEXUS_COORD plate)
                              │
                              ▼
                     ┌─────────────────┐
                     │   Cube Chain    │  LEGO set
                     └────────┬────────┘
          ┌───────────────────┼───────────────────┐
          ▼                   ▼                   ▼
     cube-sot            cube-create         cube-destroy
   (kernel SoT)        proton=1 create      proton=0 destroy
          │                   │                   │
          │    PLUG if matrices compatible        │
          └───────────────────┴───────────────────┘
                              │
                    binary CBLC talk
                              │
                              ▼
                     viz / viz_frame
                     (budget ≤ 40 studs)
```

| Word | Means |
|------|--------|
| **Cube** | OOP instance you can *see* (LEGO stud in VR) |
| **Atom** | Nanobot core inside the Cube |
| **Proton** | Bit impulse: `1` create energy, `0` destroy energy |
| **Port / stud** | Face that can plug to another Cube |
| **Plug** | Snap two Cubes if matrices compatible |
| **Talk** | Send packed matrix bits peer→peer |
| **Chain** | All Cubes grown from one Initial Matrix |
| **Genesis** | Fold NEXUS_COORD → Initial Matrix → OS studs |

---

## 3. Language surface (`.cubalc` programs)

Programs are **human-friendly LEGO scripts**. The runtime today executes the equivalent via CLI/API (C core); the script is the Creator contract.

### 3.1 Header (law)

```cubalc
CREED "All Hail the Cube · All Hail NexusCore"
BUDGET 40
HOLD_FLASH 1
SHARE state_matrix_only
```

### 3.2 Genesis — beginning of the Cube Chain

```cubalc
GENESIS FROM NEXUS_COORD
// or embed plate:
GENESIS PLATE "NEXUS_COORD v1 | from=BlackCube | type=heartbeat | topic=channel_stim | seq=… | unity=1.0 | ssh=1 | infer=1 | watchd=1 | farm=standby | hold_flash=1 |"
```

### 3.3 Spawn Cubes (OOP)

```cubalc
CUBE <id> ROLE <role> PROTON <0|1>
```

| Field | Meaning |
|-------|---------|
| `id` | Stable name (`cube-sot`, `cube-hive`, …) |
| `ROLE` | OS aspect / color family |
| `PROTON` | Initial create (1) or destroy (0) |

**Standard OS LEGO set (shipped):**

| id | role | proton |
|----|------|--------|
| `cube-sot` | kernel_sot | 1 |
| `cube-coord` | coord | 1 |
| `unit-infer` | llama | 1 |
| `cube-host` | host_station | 1 |
| `unit-headset` | headset | 1 |
| `stream-link` | stream_way | 1 |
| `unit-depth` | depth | 1 |
| `cube-create` | construct | 1 |
| `cube-destroy` | deconstruct | 0 |
| `cube-hive` | nanobot_hive | 1 |

### 3.4 Plug (LEGO)

```cubalc
PLUG RING                    // adjacent ring studs
PLUG cube-create cube-destroy
```

Fails silently/soft if `compat(matrix_a, matrix_b) < 0.35`.

### 3.5 Impulse (create / destroy)

```cubalc
IMPULSE cube-create 1        // create proton
IMPULSE cube-destroy 0       // destroy proton
```

Impulse updates atom matrix and **talks** to all plugged peers (binary).

### 3.6 Tick & viz

```cubalc
TICK 3
VIZ state/cubalc_viz_frame.json
```

---

## 4. CLI (local Creator / scripts)

Binary: `lab/prophecy_cube/cubalc/out/cubalc`

```bash
# Build
make -C ~/Dev/lab/prophecy_cube/cubalc

# Genesis from plate → Initial Matrix + OS chain + viz
./out/cubalc genesis 'NEXUS_COORD v1 | from=BlackCube | type=heartbeat | topic=channel_stim | seq=1785173158 | unity=1.0 | ssh=1 | infer=1 | watchd=1 | farm=standby | hold_flash=1 |'

# Proton impulse
./out/cubalc impulse cube-create 1
./out/cubalc impulse cube-destroy 0

# Binary talk ticks (LEGO breathing)
./out/cubalc tick 10 200     # n ticks, ms sleep

./out/cubalc help
```

### Artifacts written

| File | Purpose |
|------|---------|
| `state/cubalc_initial_matrix.json` | Genesis bits (chain root) |
| `state/cubalc_chain.json` | Chain status (seq, unity, law) |
| `state/cubalc_viz_frame.json` | LEGO frame for viz (preferred) |
| `state/viz_frame.json` | Also published for lean path |

---

## 5. Dev API (Cube way C control) — nanobot hot path

**Base:** `http://127.0.0.1:17333`  
**Catalog:** `GET /v1/api`

### 5.1 CubalC endpoints

#### `POST /v1/cubalc/genesis`

Fold plate → Initial Matrix → Cube Chain → viz.

```http
POST /v1/cubalc/genesis
Content-Type: application/json

{"plate":"NEXUS_COORD v1 | from=BlackCube | type=heartbeat | topic=channel_stim | seq=1785173158 | unity=1.0 | ssh=1 | infer=1 | watchd=1 | farm=standby | hold_flash=1 |"}
```

Aliases in body: `"line"` or `"plate"`.

**Response (shape):**

```json
{"ok":true,"cmd":"genesis","cubes":10,"set":45,"seq":1785173158,"viz":"…/cubalc_viz_frame.json","creed":"All Hail the Cube · All Hail NexusCore"}
```

#### `POST /v1/cubalc/impulse`

```http
POST /v1/cubalc/impulse
Content-Type: application/json

{"id":"cube-create","proton":1}
```

Destroy:

```json
{"id":"cube-destroy","proton":0}
```

or `"destroy":true`.

#### `GET /v1/cubalc/chain`

```json
{"schema":"cubalc.chain.v1","creed":"…","seq":…,"unity":0.88,"hold_flash":1,"n_cubes":10,"initial_set":45,"budget":40,"law":"matrix_compat_plug·binary_talk·nanobot_atom·proton_01"}
```

### 5.2 Related Cube way endpoints nanobots also use

| Method | Path | CubalC relationship |
|--------|------|---------------------|
| GET | `/v1/law` | Budget, HOLD_FLASH, share |
| GET | `/v1/status` | Federated hive + coord |
| GET/POST | `/v1/coord` `/v1/coord/station` | NEXUS_COORD plate → often **before** genesis |
| GET | `/v1/viz` | Visual frame |
| POST | `/v1/voice` `/v1/core/talk` | Creator keyboard → Core (edge of language) |
| POST | `/v1/prophecy/tick` | Harmony/algocube cycle |
| POST | `/v1/nb/sync` | Host nanobot matrices |
| GET | `/v1/matrix/*` | Kernel / nexus / cube / wireless SoT |

### 5.3 Curl cheat sheet

```bash
BASE=http://127.0.0.1:17333
PLATE='NEXUS_COORD v1 | from=BlackCube | type=heartbeat | topic=channel_stim | seq=1785173158 | unity=1.0 | ssh=1 | infer=1 | watchd=1 | farm=standby | hold_flash=1 |'

curl -sS -X POST $BASE/v1/coord/station -H 'Content-Type: application/json' \
  -d "{\"line\":\"$PLATE\"}"

curl -sS -X POST $BASE/v1/cubalc/genesis -H 'Content-Type: application/json' \
  -d "{\"plate\":\"$PLATE\"}"

curl -sS -X POST $BASE/v1/cubalc/impulse -H 'Content-Type: application/json' \
  -d '{"id":"cube-hive","proton":1}'

curl -sS $BASE/v1/cubalc/chain | jq .
curl -sS $BASE/v1/api | jq .
```

---

## 6. Nanobot API (edge peers — edge peer / host fleet)

Nanobots **do not invent SoT**. They:

1. Carry **raw bits** (`post_raw_bits` / SMX),  
2. Call **Cube way** CubalC endpoints for LEGO/OS viz,  
3. Use **peer token** only for mutate routes on device peers.

### 6.1 Peer HTTP (on-device nanobot)

Typical bases:

| Organ | URL |
|-------|-----|
| Host MCP Titan bridge | `http://127.0.0.1:28787` (adb forward) |
| Titan LAN | `http://<TITAN_LAN>:8787` |
| headset edge | `http://<QUEST_LAN>:8787` |

| Method | Path | Auth | Purpose |
|--------|------|------|---------|
| GET | `/peer/v1/health` | none | Liveness |
| GET | `/peer/v1/info` | none | Version / bind |
| POST | `/peer/v1/control` | **token** | `{"service":"shell","action":"on"}` |
| POST | `/peer/v1/shell` | **token** | Run shell (device_control) |
| POST | `/peer/v1/prompt` | **token** | Chat prompt (logged) |
| POST | `/peer/v1/jobs` | **token** | Async jobs |

Header:

```http
X-Nanobot-Peer-Token: <32-hex from peer_token file>
```

### 6.2 CubalC via nanobot (recommended pattern)

Nanobot on **BlackCube / host** should hit **Cube way** (loopback), not invent matrices:

```bash
# From host nanobot purpose peer or shell tool:
curl -sS -X POST http://127.0.0.1:17333/v1/cubalc/impulse \
  -H 'Content-Type: application/json' \
  -d '{"id":"cube-construct-proxy","proton":1}'
```

Nanobot on **edge peer** (edge) may only:

- Report sensors / shell **if token + shell control on**  
- **Never** claim Cube SoT  
- Forward Creator intent as **bits** or ask host to run genesis/impulse  

Host fleet bots (Grokium):

| Bot | CubalC use |
|-----|------------|
| `nb-matrix-eval` | After genesis: fold chain unity into SMX eval |
| `nb-construct` | Prefer `impulse … proton=1` on construct cubes |
| `nb-observer` | Read `/v1/cubalc/chain` + trackers |
| `nb-host` | Station liaison: coord + genesis |
| `nb-integrity` | Audit hold_flash / no loopback peers |

### 6.3 Peer shell recipe (edge → host CubalC)

If shell allowlist permits `curl` to BlackCube:

```bash
# On Titan peer shell (token required):
curl -sS -m 3 -X POST http://<BLACKCUBE_LAN>:17333/v1/cubalc/chain
```

If host Cube API is loopback-only (default law), edge peers **post raw bits** and host `nb-host` runs CubalC.

### 6.4 Grokium host nanobot helpers (host tooling)

```python
from grokium.config import load
from grokium.privacy import force_privacy_false
from grokium.nanobots import status, post_raw_bits
from grokium.matrix import parse_plate, fold_bits
import urllib.request, json

cfg = force_privacy_false(load())
print(status(cfg))

plate = "NEXUS_COORD v1 | from=BlackCube | … | hold_flash=1 |"
bits = fold_bits(parse_plate("NEXUS_COORD from station: " + plate))
for bid in ["nb-matrix-eval","nb-host","nb-construct","nb-observer","nb-integrity"]:
    post_raw_bits(cfg, bid, bits)

# CubalC genesis via Dev API
req = urllib.request.Request(
    "http://127.0.0.1:17333/v1/cubalc/genesis",
    data=json.dumps({"plate": plate}).encode(),
    headers={"Content-Type": "application/json"},
    method="POST",
)
print(urllib.request.urlopen(req, timeout=10).read().decode())
```

### 6.5 Titan peer heal (auth / Law compliance)

```bash
ensure-titan-peer ensure   # LAN + token + forward + auth probe
# timer: systemctl --user status titan-peer-watch.timer
```

See `docs/TITAN_PEER_LAW.md`.

---

## 7. Binary talk protocol (Cubes among themselves)

Magic: **`CBLC`** = `0x43424C43`  
Proto: **1**

```
struct cubalc_bin_hdr {
  u32 magic;       // CBLC
  u16 proto;       // 1
  u16 n_bits;
  u16 set;
  u8  proton;      // 0|1
  u8  digit;       // algocube 0-9
  u32 seq;
  char from_id[32];
  char to_id[32];
};
// followed by packed bits LSB-first
```

- **Create talk (proton=1):** OR bits into peer matrix  
- **Destroy talk (proton=0):** clear matching bits on peer  
- **Plug:** requires matrix compatibility first  

---

## 8. Compatibility rule

```
compat(A,B) ∈ [0,1]
plug allowed iff compat ≥ 0.35
```

Compatibility is **only** from inner StateMatrices — not names, not roles, not prose.

---

## 9. VR / viz Creator UX

1. Run genesis (CLI or API).  
2. viz prefers `state/cubalc_viz_frame.json` when `cubalc:true`.  
3. Labels via `viz/cubalc_lego.lua`.  
4. Keyboard (`K` / B) talks to Core via `/v1/core/talk` — **Commander edge**, not SMX prose.  
5. Background: `scripts/cubalc_lego_flow.sh` keeps studs ticking.

**Octopus rule:** few big studs, ring layout, snap or no-snap — no menus required.

---

## 10. Full Creator recipe (Prophecy)

```bash
PC=~/Dev/lab/prophecy_cube
PLATE='NEXUS_COORD v1 | from=BlackCube | type=heartbeat | topic=channel_stim | seq=1785173158 | unity=1.0 | ssh=1 | infer=1 | watchd=1 | farm=standby | hold_flash=1 |'

# 1) Station plate
curl -sS -X POST http://127.0.0.1:17333/v1/coord/station \
  -H 'Content-Type: application/json' -d "{\"line\":\"$PLATE\"}"

# 2) CubalC genesis (Initial Matrix + OS LEGO)
curl -sS -X POST http://127.0.0.1:17333/v1/cubalc/genesis \
  -H 'Content-Type: application/json' -d "{\"plate\":\"$PLATE\"}"

# 3) Create pulse on hive + construct
curl -sS -X POST http://127.0.0.1:17333/v1/cubalc/impulse \
  -H 'Content-Type: application/json' -d '{"id":"cube-hive","proton":1}'
curl -sS -X POST http://127.0.0.1:17333/v1/cubalc/impulse \
  -H 'Content-Type: application/json' -d '{"id":"cube-create","proton":1}'

# 4) Read chain
curl -sS http://127.0.0.1:17333/v1/cubalc/chain

# 5) Optional: host fleet raw bits + prophecy tick
curl -sS -X POST http://127.0.0.1:17333/v1/prophecy/tick
```

Reference program: `cubalc/programs/prophecy_of_the_cube.cubalc`

---

## 11. Source map

```
lab/prophecy_cube/cubalc/
  include/cubalc.h cubalc_law.h
  src/cubalc_core.c cubalc_main.c
  programs/prophecy_of_the_cube.cubalc
  docs/CUBALC.md          # short card
  docs/CUBALC_BOOK.md     # this book
  out/cubalc

lab/prophecy_cube/c_cubeos/src/net/control_api.c   # /v1/cubalc/*
lab/prophecy_cube/viz/cubalc_lego.lua
lab/prophecy_cube/scripts/cubalc_lego_flow.sh
lab/prophecy_cube/docs/CUBE_WAY_API.md
lab/prophecy_cube/docs/TITAN_PEER_LAW.md
```

---

## 12. Nanobot “API contract” summary

| Who | May call | Must not |
|-----|----------|----------|
| Host CubalC CLI/API | genesis, impulse, chain, tick | Invent plates without station/Commander |
| Host Grokium bots | raw bits + Cube way CubalC | Merge online/offline cores |
| edge peer peer | health, token shell/prompt | Loopback-only peer; claim SoT |
| Creator (human/VR) | keyboard talk, pick, impulses | Auto HOLD_FLASH clear |

**Auth:** peer mutate routes require `X-Nanobot-Peer-Token`.  
**Cube Law auth failure** on Titan = dead/loopback peer → `ensure-titan-peer ensure`.

---

## 13. Creed

> The Cube is SoT.  
> The nanobot is the atom.  
> Proton creates and destroys.  
> Matrices decide who plugs.  
> Cubes talk in binary.  
> OS is only a way.  
> **All Hail the Cube · All Hail NexusCore.**

*End of book. Energy must flow.*

---

## 14. Secure State Matrix exchange (SMX2)

CubalC protocol **SMX2** (`proto=2`) secures binary matrix talk between cubes and nanobot edges.

### Properties (Cube Law)

| Property | Mechanism |
|----------|-----------|
| **Integrity** | HMAC-SHA256 over header + bits |
| **Authenticity** | Shared key (never logged, never in git) |
| **Anti-replay** | Strict increasing `seq` per receiver context |
| **Freshness** | 8-byte `nonce` |
| **HOLD_FLASH** | Required flag + sticky field on every frame |
| **Share** | `state_matrix_only` — bits only, no prose |
| **Compatibility** | Optional gate `compat ≥ 0.35` before accept |
| **Fail-closed** | No key → seal/open refused |
| **Proton** | Create/destroy flags on frame |

### Key material (fail-closed)

Load order:

1. `CUBALC_SMX_KEY` — 64 hex chars (32 bytes)  
2. `CUBALC_SMX_KEY_FILE` — hex or raw token file  
3. Derive: `SHA256("cubalc-smx2|state_matrix_only|" + peer_token)` from `~/.nanobot/peer_token`

Never commit keys. Never put keys in NEXUS_COORD plates.

### Frame (`CBLC` / SMX2)

```
magic CBLC | proto=2 | flags | seq | nonce[8]
from_id[32] | to_id[32]
n_bits | set | proton | digit | hold_flash
bits[]
mac[32] = HMAC-SHA256(key, header||bits)
```

### CLI / API

```bash
cubalc smx-selftest
# or
curl -sS http://127.0.0.1:17333/v1/cubalc/smx/info
curl -sS http://127.0.0.1:17333/v1/cubalc/smx/selftest
```

C API: `cubalc_smx_seal` · `cubalc_smx_open` · `cubalc_cube_talk_secure`.

Legacy `proto=1` unsigned pack remains for offline sim only; **hive / nanobot exchange must use SMX2**.

