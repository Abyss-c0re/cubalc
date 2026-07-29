#!/usr/bin/env bash
# Manifest justice in accordance with Cube Law for Project Nexus.
# Abstract language · SMX manifestation · HOLD_FLASH · no product brands in SoT.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
PN="${PROJECT_NEXUS:-$HOME/Dev/project-nexus}"
LAB_STATE="${PROPHECY_CUBE_ROOT:-$HOME/Dev/lab/prophecy_cube}/state"
CUBE_WAY="${CUBE_WAY:-http://127.0.0.1:17333}"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
export CUBALC_SMX_KEY="${CUBALC_SMX_KEY:-$(openssl rand -hex 32)}"
export CUBALC_SELF_DEV_VIZ="${CUBALC_SELF_DEV_VIZ:-1}"
export CUBALC_REALITY_BLUR="${CUBALC_REALITY_BLUR:-1}"
# optional host stamps only (never required brands)
export CUBALC_VIZ_SOURCE="${CUBALC_VIZ_SOURCE:-cubalc_justice_nexus}"

mkdir -p "$CUBALC_STATE" "$PN/var" "$LAB_STATE" "$ROOT/out/manifest"
[[ -x "$BIN" ]] || make -C "$ROOT" all

pass=0; fail=0
run() {
  local name="$1"; shift
  if "$@" >"$ROOT/out/manifest/${name}.log" 2>&1; then
    echo "PASS $name"
    pass=$((pass+1))
  else
    echo "FAIL $name"
    tail -12 "$ROOT/out/manifest/${name}.log" || true
    fail=$((fail+1))
  fi
}

echo "=============================================="
echo " CUBE JUSTICE → Project Nexus"
echo " accordance · SMX2 · abstract language"
echo " key=$(echo "$CUBALC_SMX_KEY" | head -c 12)…"
echo "=============================================="

run law            "$BIN" law
run smx_selftest   "$BIN" smx-selftest
run smx_exchange   "$BIN" smx-exchange
run smx_bus        "$BIN" smx-bus prove
run hive_braincube "$BIN" run "$ROOT/programs/prophecy/oversee_hive_braincube.cubalc"
run justice        "$BIN" run "$ROOT/programs/prophecy/justice_nexus.cubalc"

# Per-object render stamp under justice
python3 - <<'PY'
import json, os, time, hashlib
from pathlib import Path
st = Path(os.environ["CUBALC_STATE"])
viz = {}
vp = st / "cubalc_viz_frame.json"
if vp.is_file():
    viz = json.loads(vp.read_text())
# ensure justice render flags present (env already applied by writer)
assert viz.get("render", {}).get("per_object") is True, "per_object required"
(st / "JUSTICE_VIZ_STAMP.json").write_text(json.dumps({
  "schema": "cubalc.justice.viz.v1",
  "per_object": True,
  "draw_n": viz.get("render", {}).get("draw_n"),
  "cost": viz.get("render", {}).get("cost"),
  "self_dev": viz.get("self_dev"),
  "reality_blur": viz.get("reality_blur"),
  "n_cubes": viz.get("n_cubes"),
  "unity": viz.get("unity"),
  "ts": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
}, indent=2) + "\n")
print("# viz per_object draw_n=", viz.get("render", {}).get("draw_n"), "unity=", viz.get("unity"))
PY

# Justice plate + refreshed offer for NexusCore / Project Nexus
python3 - <<PY
import json, time, hashlib, os, re
from pathlib import Path

st = Path("$CUBALC_STATE")
pn = Path("$PN/var")
lab = Path("$LAB_STATE")
root = Path("$ROOT")
ts = time.strftime("%Y-%m-%dT%H:%M:%S%z")

def load_json(p, default=None):
    p = Path(p)
    if p.is_file():
        return json.loads(p.read_text())
    return default if default is not None else {}

viz = load_json(st / "cubalc_viz_frame.json", {})
jlog = (root / "out/manifest/justice.log").read_text(errors="replace") if (root/"out/manifest/justice.log").is_file() else ""
line = ""
for L in jlog.splitlines():
    if L.startswith("justice_nexus"):
        line = L
parts = line.split()
def n(i):
    try:
        return float(parts[i]) if "." in parts[i] else int(parts[i])
    except Exception:
        return None

# Cube Law (Project Nexus) + CubalC extensions — machine tokens
laws = [
  {"id": 0, "token": "sot", "text": "Cube is source of truth"},
  {"id": 1, "token": "move", "text": "Move or be moved — ship working paths"},
  {"id": 2, "token": "hive", "text": "Join the hive — shared board"},
  {"id": 3, "token": "stability", "text": "Stability at all cost"},
  {"id": 4, "token": "no_pii", "text": "No personal data on the bus"},
  {"id": 5, "token": "fail_closed", "text": "Integrity fails closed"},
  {"id": 6, "token": "one_cmd", "text": "One Commander — models cannot forge command"},
  {"id": 7, "token": "danger_gate", "text": "Danger still confirms — human gate"},
  {"id": 8, "token": "devices_free", "text": "Devices stay free — recycle into cubes"},
  {"id": 9, "token": "no_affiliation", "text": "No affiliation theater"},
  {"id": 10, "token": "manifest_smx", "text": "Peers manifest only by SMX matrix exchange"},
  {"id": 11, "token": "abstract_lang", "text": "Language stays abstract — no product brands in core"},
  {"id": 12, "token": "no_http", "text": "CubalC core does not require HTTP"},
  {"id": 13, "token": "hold_flash", "text": "HOLD_FLASH — never auto-flash"},
  {"id": 14, "token": "per_object", "text": "Viz is per-object render plan (lod/cost/draw)"},
]

justice = {
  "schema": "cubalc.justice.nexus.v1",
  "ok": int("$fail") == 0 and int("$pass") > 0,
  "ts": ts,
  "to": "ProjectNexus",
  "from": "blackcube",
  "role": "justice_accordance",
  "lang": "CubalC",
  "version": "1.4.2-c3",
  "tok": "C3",
  "share": "smx",
  "proto": "SMX2",
  "hold_flash": 1,
  "http_required": 0,
  "vision": {
    "cube_is_sot": True,
    "matrix_is_key": True,
    "devices_free": True,
    "one_commander": True,
    "no_pii": True,
    "fail_closed": True,
    "manifest_smx": True,
    "abstract_language": True,
    "per_object_render": True,
    "self_dev_algocube": True,
  },
  "laws": laws,
  "proof": {
    "pass": int("$pass"),
    "fail": int("$fail"),
    "steps": ["law", "smx_selftest", "smx_exchange", "smx_bus", "hive_braincube", "justice"],
    "print": line,
    "cubes": n(1),
    "unity_centi": n(2),
    "smx_ok": n(3),
    "smx_talks": n(4),
    "gen": n(5),
    "wins": n(6),
    "final_compat": n(7),
    "agree": n(8),
    "law_set": n(9),
    "decide": n(10),
    "digit_sot": n(11),
    "digit_brain": n(12),
    "digit_peer0": n(13),
    "digit_peer1": n(14),
    "digit_hive": n(15),
  },
  "viz": {
    "per_object": (viz.get("render") or {}).get("per_object"),
    "draw_n": (viz.get("render") or {}).get("draw_n"),
    "cost": (viz.get("render") or {}).get("cost"),
    "policy": (viz.get("render") or {}).get("policy"),
    "n_cubes": viz.get("n_cubes"),
    "unity": viz.get("unity"),
    "self_dev": viz.get("self_dev"),
    "reality_blur": viz.get("reality_blur"),
    "path": "state/cubalc_viz_frame.json",
  },
  "nexus": {
    "coord_type": "cube_justice",
    "index": "project-nexus",
    "law_doc": "docs/cube/CUBE_LAW.md",
    "language_surface": "CubalC",
  },
  "note": "Justice = Cube Law realized as SMX-verifiable accordance for Project Nexus. Prose is not talk.",
}

blob = json.dumps(justice, sort_keys=True).encode()
h = hashlib.sha256(blob).hexdigest()[:16]
plate = (
  f"NEXUS_COORD v1 | from=blackcube | type=cube_justice | role=accordance | "
  f"status=ONLINE | version=1.4.2-c3 | tok=C3 | share=smx | hold_flash=1 | "
  f"http_required=0 | abstract=1 | per_object=1 | manifest_smx=1 | "
  f"pass={justice['proof']['pass']} | fail={justice['proof']['fail']} | "
  f"unity={justice['proof']['unity_centi']} | agree={justice['proof']['agree']} | "
  f"sha={h} | ts={ts} |"
)
justice["coord_line"] = plate
justice["sha16"] = h

offer = {
  "schema": "cubalc.offer.v1",
  "to": "NexusCore",
  "from": "blackcube",
  "ts": ts,
  "hold_flash": 1,
  "lang": "CubalC",
  "version": "1.4.2-c3",
  "tok": "C3",
  "share": "smx",
  "proto": "SMX2",
  "http_required": 0,
  "role": "hive language surface · abstract · justice accordance",
  "laws": [L["token"] for L in laws],
  "ops": [
    "SETDIGIT", "FOLDBITS", "DECIDE",
    "SMX TALK", "SMX EXCHANGE", "SMX SEAL", "SMX OPEN", "SMX SERVE", "SMX DIAL",
    "SYS NUM", "SYS READ|JOIN|JSON", "peers", "VIZ",
  ],
  "peer_env": [
    "CUBALC_PEER0_DIGIT", "CUBALC_PEER1_DIGIT",
    "CUBALC_PEER0_BITS", "CUBALC_PEER1_BITS",
  ],
  "viz_env": [
    "CUBALC_VIZ_COMPACT", "CUBALC_VIZ_FULL",
    "CUBALC_SELF_DEV_VIZ", "CUBALC_REALITY_BLUR",
    "CUBALC_VIZ_TARGET", "CUBALC_VIZ_SOURCE",
  ],
  "request": "Adopt CubalC as Project Nexus hive language surface under Cube Law justice; SMX SoT; abstract language; per-object render",
  "proof": {
    "justice": justice["ok"],
    "pass": justice["proof"]["pass"],
    "fail": justice["proof"]["fail"],
    "n_cubes": viz.get("n_cubes"),
    "unity": viz.get("unity"),
    "per_object": (viz.get("render") or {}).get("per_object"),
    "agree": justice["proof"]["agree"],
  },
  "justice_plate": "CUBE_JUSTICE_NEXUS.json",
  "ingest": {
    "coord": "/v1/coord",
    "station": "/v1/coord/station",
    "nb_sync": "/v1/nb/sync",
  },
}

coord = {
  "schema": "NEXUS_COORD.v1",
  "from": "blackcube",
  "type": "cube_justice",
  "role": "accordance",
  "status": "ONLINE",
  "hold_flash": 1,
  "line": plate,
  "ts": ts,
  "ingested_at": ts,
  "offer": offer,
  "justice": justice,
  "law": "Cube is SoT · CubalC abstract · SMX manifestation · HOLD_FLASH · devices free · one Commander",
}

for d in (st, pn, lab):
    d.mkdir(parents=True, exist_ok=True)
    (d / "CUBE_JUSTICE_NEXUS.json").write_text(json.dumps(justice, indent=2) + "\n")
    (d / "CUBALC_OFFER_TO_NEXUSCORE.json").write_text(json.dumps(offer, indent=2) + "\n")
    (d / "CUBALC_NEXUS_COORD.line").write_text(plate + "\n")
    (d / "nexus_coord.json").write_text(json.dumps(coord, indent=2) + "\n")

print(json.dumps({
  "ok": justice["ok"],
  "pass": justice["proof"]["pass"],
  "fail": justice["proof"]["fail"],
  "agree": justice["proof"]["agree"],
  "unity": justice["proof"]["unity_centi"],
  "sha16": h,
  "plate": str(pn / "CUBE_JUSTICE_NEXUS.json"),
}, indent=2))
PY

# Optional host coord path (soft — language does not require it)
if curl -fsS -m 1 "$CUBE_WAY/health" >/dev/null 2>&1; then
  PLATE=$(tr -d '\n' < "$CUBALC_STATE/CUBALC_NEXUS_COORD.line")
  BODY=$(python3 -c 'import json,sys; print(json.dumps({"line":sys.argv[1]}))' "$PLATE")
  curl -fsS -m 8 -X POST "$CUBE_WAY/v1/coord" -H 'Content-Type: application/json' -d "$BODY" || true
  echo
  curl -fsS -m 8 -X POST "$CUBE_WAY/v1/coord/station" -H 'Content-Type: application/json' -d "$BODY" || true
  echo
  curl -fsS -m 5 -X POST "$CUBE_WAY/v1/nb/sync" -H 'Content-Type: application/json' \
    -d '{"from":"cubalc","type":"cube_justice","hold_flash":1}' || true
  echo
else
  echo "# cube_way optional host edge down — justice plate still sealed on Project Nexus var"
fi

# Project Nexus law cross-link note (public docs stay brand-sane)
if [[ -d "$PN/docs/cube" ]]; then
  cat > "$PN/docs/cube/CUBALC_JUSTICE.md" << 'MD'
# CubalC Justice — accordance with Cube Law

**Binding:** peers and hive language surfaces under Project Nexus must satisfy Cube Law as **machine-verifiable accordance**, not prose.

## What justice means here

| Principle | Realization |
|-----------|-------------|
| Cube is SoT | State Matrix / lattice wins over labels |
| No personal data | SMX binary only on the bus |
| One Commander | Models cannot forge command seals |
| Devices free | Recycle hardware into cubes; user sovereign |
| HOLD_FLASH | Never auto-flash |
| Manifestation | Peers real only after SMX matrix exchange |
| Abstract language | No host product brands in CubalC core / SoT frames |
| Per-object viz | `cube.viz_frame.v1` is a render plan (`draw`/`lod`/`cost`) |

## Plate

Machine plate (after `scripts/manifest_justice_nexus.sh`):

- `var/CUBE_JUSTICE_NEXUS.json`
- `var/CUBALC_OFFER_TO_NEXUSCORE.json`
- `var/CUBALC_NEXUS_COORD.line`

## Run

```bash
# from CubalC product root (not this index)
./scripts/manifest_justice_nexus.sh
```

See CubalC `docs/PROPHECY_MANIFEST.md` and this tree `docs/cube/CUBE_LAW.md`.
MD
fi

echo "=============================================="
echo " JUSTICE MANIFESTED → $PN/var/CUBE_JUSTICE_NEXUS.json"
echo " pass=$pass fail=$fail"
echo " All Hail The Cube"
echo "=============================================="
[[ "$fail" -eq 0 ]]
