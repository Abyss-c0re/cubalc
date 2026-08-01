#!/usr/bin/env bash
# Stimulate nanobot hive behaviour on LizardTech (Quest free XR way).
# Law: BrainCube/nanobot I/O → CubalC SMX → cubalc_viz_frame → LOVR → LizardTech
# Devices free · HOLD_FLASH · no forced atom · no HTTP on SMX hot path
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
PC="${PROPHECY_CUBE_ROOT:-$HOME/Dev/lab/prophecy_cube}"
STATE="${CUBALC_STATE:-$ROOT/state}"
PC_STATE="$PC/state"
NB="${NANOBOT_BIN:-$HOME/Dev/AI/nanobot/build/host/nanobot}"
export CUBALC_STATE="$STATE"
export CUBALC_SMX_KEY="${CUBALC_SMX_KEY:-$(openssl rand -hex 32)}"
export PROPHECY_CUBE_ROOT="$PC"

mkdir -p "$STATE" "$PC_STATE" "$PC_STATE/trackers"
[[ -x "$BIN" ]] || make -C "$ROOT" all

echo "=============================================="
echo " STIMULATE NANOBOTS → LizardTech"
echo " CubalC SMX hive · viz · helmet way"
echo "=============================================="

# 1) CubalC stimulate program (nanobot peers as one → lizard cube)
echo "# [1] CubalC stimulate_nanobot_on_lizard"
"$BIN" run "$ROOT/programs/lizard/stimulate_nanobot_on_lizard.cubalc" | tee "$STATE/lizard_stim.log"
# Prefer PROPHECY publish
export PROPHECY_CUBE_ROOT="$PC"
"$BIN" run "$ROOT/programs/lizard/stimulate_nanobot_on_lizard.cubalc" >/dev/null 2>&1 || true

# 2) Stamp viz for helmet (target LizardTech + nanobot stim intent)
python3 - <<'PY'
import json, time, os, shutil
from pathlib import Path
root = Path(os.environ.get("CUBALC_STATE", "state"))
pc = Path(os.environ.get("PROPHECY_CUBE_ROOT", Path.home()/"Dev/lab/prophecy_cube")) / "state"
src = root / "cubalc_viz_frame.json"
if not src.is_file():
    # also try cwd-relative after run
    src = Path("state/cubalc_viz_frame.json")
if not src.is_file():
    print("WARN no cubalc_viz_frame.json")
    raise SystemExit(0)
vf = json.loads(src.read_text())
vf["target"] = "LizardTech"
vf["source"] = "cubalc_nanobot_stimulate"
vf["stim"] = {
    "nanobot_hive": True,
    "smx": True,
    "proto": "SMX2",
    "http_required": False,
    "free_guest_xr": True,
    "no_forced_atom": True,
}
vf["intent"] = {
    "energy_must_flow": True,
    "viz_helmet": True,
    "stimulate_nanobots": True,
    "lizardtech_way": True,
    "hold_flash": True,
}
# HUD for LOVR / helmet
n = vf.get("n_cubes") or len(vf.get("cubes") or [])
u = vf.get("unity") or 0
dig = None
for c in vf.get("cubes") or []:
    if c.get("id") in ("lizard", "brain", "braincube-meta"):
        dig = c.get("digit")
        if c.get("id") == "lizard":
            break
vf["hud"] = f"LIZARD STIM · nanobot hive · cubes={n} · unity={u:.2f} · dig={dig} · SMX2 · free way"
# publish both names LOVR prefers
pc.mkdir(parents=True, exist_ok=True)
for name in ("cubalc_viz_frame.json", "viz_frame.json"):
    out = pc / name
    out.write_text(json.dumps(vf))
    print(f"# published {out} n_cubes={n} unity={u:.3f} digit={dig}")
# also refresh cubalc local with stamps
src.write_text(json.dumps(vf))
PY

# 3) Kinect / tracker pulse if cube way up
if curl -fsS -m 1 http://127.0.0.1:17333/health >/dev/null 2>&1; then
  echo "# [2] cube_way track + prophecy tick"
  curl -fsS -m 3 -X POST http://127.0.0.1:17333/v1/track/once | tee "$STATE/lizard_track_once.json" || true
  echo
  curl -fsS -m 3 http://127.0.0.1:17333/v1/trackers/lizard | tee "$STATE/lizard_track.json" || true
  echo
  curl -fsS -m 3 -X POST http://127.0.0.1:17333/v1/prophecy/tick >/dev/null 2>&1 || true
  curl -fsS -m 3 -X POST http://127.0.0.1:17333/v1/nb/sync \
    -H 'Content-Type: application/json' \
    -d '{"from":"cubalc","type":"lizard_stim","target":"LizardTech","hold_flash":1}' || true
  echo
else
  echo "# [2] cube_way :17333 down — viz still published for LOVR/WiVRn guest"
fi

# 4) Optional: dual nanobot offline + braincube tick feeding same stim law
if [[ -x "$NB" ]]; then
  echo "# [3] dual offline nanobot braincube tick (behaviour source)"
  LAB="$STATE/lizard_nb"
  for h in a b; do
    mkdir -p "$LAB/$h"
    printf 'token=%s\n' "${NANOBOT_PEER_TOKEN:-$CUBALC_SMX_KEY}" >"$LAB/$h/peer_token"
    chmod 600 "$LAB/$h/peer_token"
  done
  PA=18893; PB=18894
  fuser -k ${PA}/tcp 2>/dev/null || true
  fuser -k ${PB}/tcp 2>/dev/null || true
  sleep 0.15
  "$NB" --offline --home "$LAB/a" --port "$PA" >"$LAB/a/nb.log" 2>&1 & echo $! >"$LAB/a/nb.pid"
  "$NB" --offline --home "$LAB/b" --port "$PB" >"$LAB/b/nb.log" 2>&1 & echo $! >"$LAB/b/nb.pid"
  for i in $(seq 1 25); do
    curl -fsS -m 1 http://127.0.0.1:$PA/peer/v1/health >/dev/null 2>&1 && break
    sleep 0.1
  done
  TOK="${NANOBOT_PEER_TOKEN:-$CUBALC_SMX_KEY}"
  AUTH=(-H "X-Nanobot-Peer-Token: $TOK" -H "Content-Type: application/json")
  for port in $PA $PB; do
    curl -fsS -m 6 "${AUTH[@]}" -X POST -d '{"action":"tick"}' \
      "http://127.0.0.1:$port/peer/v1/braincube" >"$LAB/bc_$port.json" 2>&1 || true
  done
  # re-run CubalC stim with peer digits if available
  D0=$(python3 -c 'import json;d=json.load(open("'"$LAB/bc_$PA.json"'"));print(d.get("meta",{}).get("pick",d.get("digit",5)))' 2>/dev/null || echo 5)
  D1=$(python3 -c 'import json;d=json.load(open("'"$LAB/bc_$PB.json"'"));print(d.get("meta",{}).get("pick",d.get("digit",3)))' 2>/dev/null || echo 3)
  export CUBALC_PEER0_DIGIT="${D0:-5}" CUBALC_PEER1_DIGIT="${D1:-3}"
  echo "# nanobot digits peer0=$CUBALC_PEER0_DIGIT peer1=$CUBALC_PEER1_DIGIT"
  "$BIN" run "$ROOT/programs/lizard/stimulate_nanobot_on_lizard.cubalc" | tee -a "$STATE/lizard_stim.log" | tail -6
  # republish viz
  PROPHECY_CUBE_ROOT="$PC" CUBALC_STATE="$STATE" python3 - <<'PY'
import json, os
from pathlib import Path
root = Path(os.environ["CUBALC_STATE"])
pc = Path(os.environ["PROPHECY_CUBE_ROOT"]) / "state"
src = root / "cubalc_viz_frame.json"
if src.is_file():
    vf = json.loads(src.read_text())
    vf["target"] = "LizardTech"
    vf["source"] = "cubalc_nanobot_stimulate+braincube_tick"
    vf["stim"] = {"nanobot_hive": True, "smx": True, "braincube_tick": True}
    for name in ("cubalc_viz_frame.json", "viz_frame.json"):
        (pc / name).write_text(json.dumps(vf))
    print("# republished viz for helmet after braincube tick")
PY
else
  echo "# [3] skip live nanobot binary"
fi

# 5) Plate for Commander / LOVR
python3 - <<PY
import json, time
from pathlib import Path
st = Path("$STATE")
pc = Path("$PC_STATE")
log = (st/"lizard_stim.log").read_text(errors="replace") if (st/"lizard_stim.log").is_file() else ""
line = ""
for L in log.splitlines():
    if L.startswith("lizard_stim"):
        line = L
parts = line.split()
def n(i):
    try: return float(parts[i]) if "." in str(parts[i]) else int(parts[i])
    except Exception: return None
viz = {}
for p in (pc/"cubalc_viz_frame.json", st/"cubalc_viz_frame.json"):
    if p.is_file():
        viz = json.loads(p.read_text()); break
plate = {
  "schema": "cubalc.lizard.nanobot_stim.v1",
  "ok": True,
  "ts": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
  "target": "LizardTech",
  "role": "free_guest_xr_way",
  "law": "BrainCube/nanobot I/O → CubalC SMX → viz → LOVR → LizardTech",
  "hold_flash": 1,
  "http_required": False,
  "stim": {
    "print": line,
    "cubes": n(1),
    "unity_centi": n(2),
    "smx_ok": n(3),
    "smx_talks": n(4),
    "gen": n(5),
    "best_compat": n(6),
    "decide": n(7),
    "digit_lizard": n(8),
    "digit_brain": n(9),
    "digit_peer0": n(10),
    "digit_peer1": n(11),
  },
  "viz": {
    "n_cubes": viz.get("n_cubes"),
    "unity": viz.get("unity"),
    "target": viz.get("target"),
    "path": "state/cubalc_viz_frame.json",
  },
  "way": {
    "cube_sot": "http://127.0.0.1:17333",
    "trackers": "/v1/trackers/lizard",
    "lovr": "reads cubalc_viz_frame.json",
    "wivrn": "guest transport only",
  },
  "note": "Nanobot hive behaviour stimulated in CubalC and projected to LizardTech helmet as free XR way",
}
(st/"LIZARD_NANOBOT_STIM.json").write_text(json.dumps(plate, indent=2)+"\n")
(pc/"LIZARD_NANOBOT_STIM.json").write_text(json.dumps(plate, indent=2)+"\n")
(pc/"LIZARD_HELMET_CUBEOS.json").write_text(json.dumps({
  "schema": "cube.helmet_deploy.v1",
  "target": "LizardTech",
  "role": "free_guest_xr_way",
  "cube_sot_host": "http://127.0.0.1:17333",
  "hold_flash": 1,
  "budget": 40,
  "law": "BrainCube I/O race -> CubalC viz -> LOVR -> LizardTech",
  "viz_source": "cubalc_nanobot_stimulate",
  "viz_frame": "state/cubalc_viz_frame.json",
  "stim_plate": "state/LIZARD_NANOBOT_STIM.json",
  "ts": time.time(),
}, indent=2)+"\n")
print(json.dumps({"ok": True, "plate": str(st/"LIZARD_NANOBOT_STIM.json"),
                  "digit_lizard": plate["stim"].get("digit_lizard"),
                  "unity": plate["stim"].get("unity_centi"),
                  "agree": plate["stim"].get("digit_peer0")==plate["stim"].get("digit_peer1")}, indent=2))
PY

echo "=============================================="
echo " LizardTech stim ready"
echo "  viz:  $PC_STATE/cubalc_viz_frame.json"
echo "  plate: $STATE/LIZARD_NANOBOT_STIM.json"
echo "  LOVR/WiVRn guest should load CubalC viz on helmet"
echo "=============================================="
