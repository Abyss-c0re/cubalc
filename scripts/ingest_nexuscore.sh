#!/usr/bin/env bash
# Ingest CubalC offer into NexusCore way (cube_way coord + station SoT).
# Language stays device-agnostic; this is the host station path only.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CUBE_WAY="${CUBE_WAY:-http://127.0.0.1:17333}"
PN="${PROJECT_NEXUS:-$HOME/Dev/project-nexus}"
LAB_STATE="${PROPHECY_CUBE_ROOT:-$HOME/Dev/lab/prophecy_cube}/state"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
mkdir -p "$CUBALC_STATE" "$PN/var" "$LAB_STATE"

[[ -x "$BIN" ]] || make -C "$ROOT" all

# optional peer env already set by caller
"$BIN" peers >/tmp/cubalc_peers_ingest.out 2>&1 || true

python3 - <<'PY'
import json, time, hashlib
from pathlib import Path
import os
dev = Path(os.environ.get("CUBALC_STATE", ".")).resolve().parent if False else Path(os.environ["CUBALC_STATE"]).parent
# CUBALC_STATE is state dir
st = Path(os.environ["CUBALC_STATE"])
root = st.parent
viz = {}
vp = st / "cubalc_viz_frame.json"
if vp.exists():
    viz = json.loads(vp.read_text())
ts = time.strftime("%Y-%m-%dT%H:%M:%S%z")
offer = {
  "schema": "cubalc.offer.v1",
  "to": "NexusCore",
  "from": "blackcube",
  "ts": ts,
  "hold_flash": 1,
  "lang": "CubalC",
  "version": "1.3.1-c3",
  "tok": "C3",
  "share": "smx",
  "role": "hive language surface · peer_fold env-only",
  "request": "Adopt CubalC as hive language surface; SMX remains SoT",
  "peer_env": ["CUBALC_PEER0_DIGIT","CUBALC_PEER1_DIGIT","CUBALC_PEER0_BITS","CUBALC_PEER1_BITS"],
  "proof": {"peer_fold": True, "n_cubes": viz.get("n_cubes"), "unity": viz.get("unity")},
}
blob = json.dumps(offer, sort_keys=True).encode()
h = hashlib.sha256(blob).hexdigest()[:16]
plate = (
  f"NEXUS_COORD v1 | from=blackcube | type=cubalc_offer | role=language | "
  f"status=ONLINE | version=1.3.1-c3 | tok=C3 | share=smx | hold_flash=1 | "
  f"peer_fold=1 | sha={h} | ts={ts} |"
)
(st / "CUBALC_OFFER_TO_NEXUSCORE.json").write_text(json.dumps(offer, indent=2) + "\n")
(st / "CUBALC_NEXUS_COORD.line").write_text(plate + "\n")
print(plate)
PY

PLATE=$(tr -d '\n' < "$CUBALC_STATE/CUBALC_NEXUS_COORD.line")
BODY=$(python3 -c 'import json,sys; print(json.dumps({"line":sys.argv[1]}))' "$PLATE")

curl -fsS -m 8 -X POST "$CUBE_WAY/v1/coord" -H 'Content-Type: application/json' -d "$BODY"
echo
curl -fsS -m 8 -X POST "$CUBE_WAY/v1/coord/station" -H 'Content-Type: application/json' -d "$BODY"
echo
curl -fsS -m 8 -X POST "$CUBE_WAY/v1/cubalc/genesis" -H 'Content-Type: application/json' -d "$BODY" || true
echo
curl -fsS -m 5 -X POST "$CUBE_WAY/v1/nb/sync" -H 'Content-Type: application/json' \
  -d '{"from":"cubalc","type":"peer_fold","hold_flash":1}' || true
echo

cp -f "$CUBALC_STATE/CUBALC_OFFER_TO_NEXUSCORE.json" "$PN/var/CUBALC_OFFER_TO_NEXUSCORE.json"
cp -f "$CUBALC_STATE/CUBALC_OFFER_TO_NEXUSCORE.json" "$LAB_STATE/CUBALC_OFFER_TO_NEXUSCORE.json"
cp -f "$CUBALC_STATE/CUBALC_NEXUS_COORD.line" "$PN/var/CUBALC_NEXUS_COORD.line"
python3 - <<PY
import json, time
from pathlib import Path
pn = Path("$PN/var")
offer = json.loads((pn/"CUBALC_OFFER_TO_NEXUSCORE.json").read_text())
line = (pn/"CUBALC_NEXUS_COORD.line").read_text().strip()
coord = {
  "schema": "NEXUS_COORD.v1",
  "from": "blackcube",
  "type": "cubalc_offer",
  "role": "language",
  "status": "ONLINE",
  "hold_flash": 1,
  "line": line,
  "ts": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
  "ingested_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
  "offer": offer,
  "law": "Cube is SoT · CubalC peer_fold · SMX remains SoT · HOLD_FLASH",
}
(pn/"nexus_coord.json").write_text(json.dumps(coord, indent=2)+"\n")
Path("$LAB_STATE/NEXUS_COORD.json").write_text(json.dumps(coord, indent=2)+"\n")
print("ingested", line[:80], "...")
PY
