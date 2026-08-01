#!/usr/bin/env bash
# Functional sample: BrainCube contained in CubalC (no host product brands).
# Exit 0 only if SMX + digit agree + DECIDE in 0..9.
set -euo pipefail
SAMPLE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SAMPLE/../.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
export CUBALC_STATE="${CUBALC_STATE:-$SAMPLE/state}"
export CUBALC_SMX_KEY="${CUBALC_SMX_KEY:-$(openssl rand -hex 32 2>/dev/null || echo '0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef')}"
export CUBALC_SELF_DEV_VIZ="${CUBALC_SELF_DEV_VIZ:-1}"

mkdir -p "$CUBALC_STATE" "$SAMPLE/out"
[[ -x "$BIN" ]] || make -C "$ROOT" all

echo "# braincube functional sample"
echo "# state=$CUBALC_STATE bin=$BIN"

"$BIN" run "$SAMPLE/braincube_sample.cubalc" | tee "$SAMPLE/out/run.log"

python3 - <<PY
import json, time, hashlib, re
from pathlib import Path
sample = Path("$SAMPLE")
st = Path("$CUBALC_STATE")
log = (sample / "out/run.log").read_text(errors="replace")
line = ""
ok_line = None
for L in log.splitlines():
    if L.startswith("braincube_sample"):
        line = L
    if L.startswith("{") and '"ok"' in L:
        try:
            ok_line = json.loads(L)
        except Exception:
            pass
parts = line.split()
def n(i):
    try:
        return float(parts[i]) if "." in parts[i] else int(parts[i])
    except Exception:
        return None
viz = {}
vp = st / "cubalc_viz_frame.json"
if vp.is_file():
    viz = json.loads(vp.read_text())
plate = {
  "schema": "cubalc.sample.braincube.v1",
  "ok": bool(ok_line and ok_line.get("ok")),
  "sample": "braincube",
  "functional": True,
  "contained": True,
  "lang": "CubalC",
  "version": "1.4.2-c3",
  "tok": "C3",
  "share": "smx",
  "proto": "SMX2",
  "hold_flash": 1,
  "http_required": 0,
  "abstract": True,
  "ts": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
  "print": line,
  "metrics": {
    "cubes": n(1),
    "unity_centi": n(2),
    "smx_ok": n(3),
    "smx_talks": n(4),
    "gen": n(5),
    "wins": n(6),
    "final_compat": n(7),
    "agree": n(8),
    "decide": n(9),
    "digit_brain": n(10),
    "digit_algo": n(11),
    "digit_peer0": n(12),
    "digit_peer1": n(13),
  },
  "viz": {
    "per_object": (viz.get("render") or {}).get("per_object"),
    "draw_n": (viz.get("render") or {}).get("draw_n"),
    "cost": (viz.get("render") or {}).get("cost"),
    "n_cubes": viz.get("n_cubes"),
    "unity": viz.get("unity"),
  },
  "contract": {
    "smx_exchange": True,
    "decide_0_9": True,
    "peer_digit_agree": True,
    "compat_ge_50": True,
    "hold_flash": True,
  },
  "paths": {
    "program": "samples/braincube/braincube_sample.cubalc",
    "run": "samples/braincube/run.sh",
    "state": str(st),
  },
}
plate["ok"] = bool(
  plate["ok"]
  and plate["metrics"].get("smx_ok") == 1
  and plate["metrics"].get("agree") == 1
  and plate["metrics"].get("decide") is not None
  and 0 <= int(plate["metrics"]["decide"]) <= 9
)
blob = json.dumps(plate, sort_keys=True).encode()
plate["sha16"] = hashlib.sha256(blob).hexdigest()[:16]
out = sample / "out" / "BRAINCUBE_SAMPLE.json"
out.write_text(json.dumps(plate, indent=2) + "\n")
(st / "BRAINCUBE_SAMPLE.json").write_text(json.dumps(plate, indent=2) + "\n")
print(json.dumps({"ok": plate["ok"], "plate": str(out), "decide": plate["metrics"].get("decide"),
                  "agree": plate["metrics"].get("agree"), "unity": plate["metrics"].get("unity_centi"),
                  "sha16": plate["sha16"]}, indent=2))
raise SystemExit(0 if plate["ok"] else 1)
PY
