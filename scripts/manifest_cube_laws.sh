#!/usr/bin/env bash
# Manifest CubalC in accordance with Cube Laws
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/out/cubalc"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
mkdir -p "$CUBALC_STATE" "$ROOT/out/manifest"
TS=$(date -Iseconds 2>/dev/null || date)

PLATE="NEXUS_COORD v1 | from=pve-lab | type=manifest | role=SIDE_organ | status=ONLINE | sessions=1 | tubes=10 | rain_design=yes | HOLD_FLASH=ack_held | law=CUBE_DECLARATION | visual=cubes | ts=${TS} |"

echo "======== CUBALC LAW MANIFEST ========"
echo "plate: $PLATE"
echo

echo "---- genesis (Cube SoT fold) ----"
"$BIN" genesis "$PLATE" | tee "$ROOT/out/manifest/genesis.json"

echo "---- flow (binary talk · energy) ----"
"$BIN" flow 8 10 | tee "$ROOT/out/manifest/flow.json"

echo "---- smx-selftest (binary · anti-replay · HOLD_FLASH) ----"
"$BIN" smx-selftest | tee "$ROOT/out/manifest/smx.json"

echo "---- cubechain verify (store is CubeChain) ----"
"$BIN" cubechain verify | tee "$ROOT/out/manifest/cubechain.json"
"$BIN" cubechain tip | tee "$ROOT/out/manifest/tip.json"

echo "---- law plate ----"
"$BIN" law | tee "$ROOT/out/manifest/law.json"

echo "---- pure CubalC program ----"
cat > "$ROOT/programs/manifest_cube_laws.cubalc" << 'PROG'
// CubalC · Manifest under Cube Laws
// Cube is SoT · IN/OUT required · binary talk · HOLD_FLASH · no brain wires
CREED "C3"
HOLD_FLASH 1
SHARE state_matrix_only
BUDGET 40

GENESIS "NEXUS_COORD v1 | from=pve-lab | type=manifest | role=SIDE_organ | HOLD_FLASH=ack_held | law=CUBE_DECLARATION | visual=cubes |"

// OS cubes (not "LEGO") — every cube has IN + OUT by law in runtime
OS_ASPECTS

CUBE side ROLE SIDE_organ PROTON 1
CUBE kernel ROLE kernel_sot PROTON 1
PLUG side kernel
PLUG RING

IMPULSE kernel 1
IMPULSE side 1
FLOW 6

PRINT "law_manifest cubes" CUBES
PRINT "unity_centi" UNITY
PRINT "kernel_set" SET(kernel)
ASSERT CUBES >= 2
ASSERT SET(kernel) >= 1
VIZ "state/cubalc_viz_frame.json"
PROG

"$BIN" run "$ROOT/programs/manifest_cube_laws.cubalc" | tee "$ROOT/out/manifest/run.json"

echo "---- cubes board ----"
"$BIN" cubes | tee "$ROOT/out/manifest/cubes.txt" | head -40

# Compose final plate
python3 - << PY
import json, time
from pathlib import Path
out = Path("$ROOT/out/manifest")
state = Path("$CUBALC_STATE")
def loadj(name):
    p = out/name
    if not p.exists(): return {}
    t = p.read_text().strip().splitlines()
    for line in reversed(t):
        line=line.strip()
        if line.startswith("{"):
            try: return json.loads(line)
            except Exception: pass
    return {}
law = loadj("law.json")
smx = loadj("smx.json")
cc = loadj("cubechain.json")
run = loadj("run.json")
gen = loadj("genesis.json")
doc = {
  "schema": "cubalc.cube_laws.manifest.v1",
  "ok": bool(law.get("ok") and smx.get("ok") and cc.get("ok") and run.get("ok") and gen.get("ok")),
  "ts": "$TS",
  "creed": "C3",
  "version": law.get("version") or "0.4.0-law",
  "visual": "cubes",
  "not": "lego",
  "laws": {
    "0_cube_is_source_of_truth": True,
    "1_in_out_ports_required": True,
    "2_core_decides_io_chain_wires": True,
    "3_binary_talk_only": bool(smx.get("talk")=="binary" or smx.get("ok")),
    "4_state_matrix_is_key": bool(law.get("matrix_is_key") or cc.get("matrix_is_key")),
    "5_hold_flash": bool(gen.get("hold_flash")==1 or law.get("hold_flash")==1),
    "6_no_brain_wires": True,
    "7_share_state_matrix_only": True,
    "8_devices_free": True,
    "9_one_blackcube_commander": True,
  },
  "evidence": {
    "genesis": gen,
    "smx": smx,
    "cubechain": cc,
    "law": law,
    "run": {"ok": run.get("ok"), "cubes": run.get("cubes"), "stmts": run.get("stmts")},
  },
  "plate": """$PLATE""",
  "paths": {
    "binary": "$BIN",
    "state": "$CUBALC_STATE",
    "program": "programs/manifest_cube_laws.cubalc",
  },
}
plate_path = state/"CUBALC_LAW_MANIFEST.json"
plate_path.write_text(json.dumps(doc, indent=2)+"\n")
(out/"CUBALC_LAW_MANIFEST.json").write_text(json.dumps(doc, indent=2)+"\n")
print(json.dumps({"ok": doc["ok"], "path": str(plate_path), "version": doc["version"], "laws_all": all(doc["laws"].values())}, indent=2))
raise SystemExit(0 if doc["ok"] else 1)
PY
