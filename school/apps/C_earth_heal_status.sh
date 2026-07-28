#!/usr/bin/env bash
# App C — Earth Heal Status: plate → status artifact for Creator
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/out/cubalc"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
STU="$(dirname "$0")/../students/${STUDENT:-anon}"
mkdir -p "$CUBALC_STATE" "$STU/apps"
OUT="$STU/apps/C_earth_heal_status.json"
PLATE="${1:-NEXUS_COORD v1 | from=pve-lab | type=heartbeat | role=SIDE_organ | status=ONLINE | sessions=1 | tubes=10 | rain_design=yes | HOLD_FLASH=ack_held |}"
"$BIN" genesis "$PLATE" >/dev/null
"$BIN" flow 4 10 >/dev/null
TIP=$("$BIN" cubechain tip)
VIZ="$CUBALC_STATE/cubalc_viz_frame.json"
python3 - <<PY
import json, os, time
tip=json.loads('''$TIP''')
viz={}
try:
  viz=json.load(open("$VIZ"))
except Exception:
  pass
doc={
  "app":"C_earth_heal_status",
  "ok": True,
  "creed":"C3",
  "mission":"recycle electronics · heal the Earth",
  "hold_flash":1,
  "plate_ack":"HOLD_FLASH=ack_held",
  "cubechain_tip": tip.get("tip_seq"),
  "unity": viz.get("unity"),
  "energy": viz.get("energy"),
  "n_cubes": viz.get("n_cubes"),
  "talk": viz.get("talk"),
  "ts": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
}
open("$OUT","w").write(json.dumps(doc, indent=2)+"\n")
print(json.dumps(doc))
PY
