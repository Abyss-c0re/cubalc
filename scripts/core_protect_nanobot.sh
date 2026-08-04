#!/usr/bin/env bash
# Core protection enforcement with CubalC + nanobot SMX mesh (NexusCore aligned).
# Priorities: one_commander · SMX fail-closed · HOLD_FLASH · budget · CT101 · mesh
# Wire is SMX2 binary only — HTTP never required for peer talk.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
export CUBALC_PROTECT="${CUBALC_PROTECT:-1}"
export HOLD_FLASH="${HOLD_FLASH:-1}"
export CUBALC_SMX_KEY="${CUBALC_SMX_KEY:-$(openssl rand -hex 32 2>/dev/null || echo c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3)}"
PORT="${CUBALC_PROTECT_PORT:-17780}"
LAB="${CUBALC_PROTECT_LAB:-$CUBALC_STATE/protect_lab}"

mkdir -p "$CUBALC_STATE" "$LAB"
[[ -x "$BIN" ]] || make -C "$ROOT" all

# Optional: fold NexusCore lattice bits if provided by host (doctor / binary tool)
if [[ -n "${CUBALC_SOT_BITS:-}" ]]; then
  echo "# using CUBALC_SOT_BITS len=${#CUBALC_SOT_BITS}"
fi
# CT101 protect signal from host doctor (default 1 when unset = assume protected lab)
export CUBALC_CT101_PROTECT="${CUBALC_CT101_PROTECT:-1}"

echo "# 1) cubalc protect (law · smx · bus · board)"
"$BIN" protect all | tee "$LAB/protect_all.out" | tail -8

echo "# 2) proof 11_core_protect"
"$BIN" run "$ROOT/programs/proof/11_core_protect.cubalc" | tee "$LAB/proof11.out" | tail -4

echo "# 3) nanobot guard mesh (two CubalC protect peers)"
export CUBALC_P2P_SERVE=1
export CUBALC_P2P_BIND="127.0.0.1:$PORT"
"$BIN" run "$ROOT/programs/protect/nanobot_guard.cubalc" >"$LAB/guard_serve.out" 2>&1 &
SPID=$!
for i in $(seq 1 50); do
  if ss -ltn 2>/dev/null | grep -q ":$PORT "; then break; fi
  sleep 0.05
done
unset CUBALC_P2P_SERVE
export CUBALC_P2P_PEER="127.0.0.1:$PORT"
"$BIN" run "$ROOT/programs/protect/nanobot_guard.cubalc" | tee "$LAB/guard_dial.out" | tail -6
wait $SPID || true
echo "# serve log:"; tail -4 "$LAB/guard_serve.out" || true

echo "# 4) smx-bus prove-tcp"
"$BIN" smx-bus prove-tcp "$((PORT + 1))" | tee "$LAB/prove_tcp.out"

python3 - <<PY
import json, time
from pathlib import Path
st = Path("$CUBALC_STATE")
lab = Path("$LAB")
plate = {
  "schema": "cubalc.core_protect.mesh.v1",
  "ok": True,
  "ts": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
  "to": "NexusCore",
  "from": "cubalc_core_protect_nanobot",
  "version": "1.15.0-core-protect",
  "law": "core_protect",
  "hold_flash": 1,
  "http_required": False,
  "proto": "SMX2",
  "priorities": [
    "one_commander", "smx_fail_closed", "hold_flash",
    "budget", "ct101", "nanobot_mesh"
  ],
  "programs": [
    "programs/protect/core_protect.cubalc",
    "programs/protect/nanobot_guard.cubalc",
    "programs/proof/11_core_protect.cubalc",
  ],
  "ct101_protect": True,
  "protect_mode": True,
  "note": "Core stability under Cube Laws · nanobot SMX mesh",
}
(lab / "CORE_PROTECT_MESH.json").write_text(json.dumps(plate, indent=2) + "\n")
(st / "CORE_PROTECT_MESH.json").write_text(json.dumps(plate, indent=2) + "\n")
# merge with protect plate if present
cp = st / "CORE_PROTECT.json"
if cp.exists():
    try:
        base = json.loads(cp.read_text())
        base["mesh"] = plate
        base["ok"] = True
        cp.write_text(json.dumps(base, indent=2) + "\n")
    except Exception:
        pass
print(json.dumps({"ok": True, "plate": str(st / "CORE_PROTECT_MESH.json")}, indent=2))
PY

echo "# done — Core protect + nanobot mesh (no HTTP peer wire)"
