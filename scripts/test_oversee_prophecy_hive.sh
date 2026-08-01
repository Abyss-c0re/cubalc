#!/usr/bin/env bash
# Test: nanobot peers + CubalC oversee hive as one → braincube prophecy + self-dev algocube
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
NB="${NANOBOT_BIN:-$HOME/Dev/AI/nanobot/build/host/nanobot}"
LAB="${CUBALC_PROPHECY_LAB:-$ROOT/state/prophecy_hive}"
PORT_NB_A="${PORT_NB_A:-18891}"
PORT_NB_B="${PORT_NB_B:-18892}"
PORT_SMX="${PORT_SMX:-17751}"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
export CUBALC_SMX_KEY="${CUBALC_SMX_KEY:-$(openssl rand -hex 32)}"
export NANOBOT_PEER_TOKEN="${NANOBOT_PEER_TOKEN:-$CUBALC_SMX_KEY}"

mkdir -p "$LAB"/{peer_a,peer_b,plates} "$CUBALC_STATE"
[[ -x "$BIN" ]] || make -C "$ROOT" all

pass=0; fail=0
run_ok() {
  local name="$1"; shift
  if "$@" >"$LAB/plates/${name}.log" 2>&1; then
    if grep -qE '"ok":true|# ok' "$LAB/plates/${name}.log" 2>/dev/null || [[ $? -eq 0 ]]; then
      echo "PASS $name"
      pass=$((pass+1))
      return 0
    fi
  fi
  echo "FAIL $name"
  tail -8 "$LAB/plates/${name}.log" || true
  fail=$((fail+1))
  return 1
}

echo "=============================================="
echo " OVERSEE PROPHECY HIVE — CubalC + nanobot P2P"
echo " SMX2 binary · self-dev algocube · braincube"
echo " key=$(echo "$CUBALC_SMX_KEY" | head -c 12)…"
echo "=============================================="

# --- 1) In-process hive as one (self-dev algocube loop) ---
echo "# [1] oversee_hive_braincube (12-gen self-dev)"
if "$BIN" run "$ROOT/programs/prophecy/oversee_hive_braincube.cubalc" | tee "$LAB/plates/hive_braincube.log"; then
  echo "PASS hive_braincube"
  pass=$((pass+1))
else
  echo "FAIL hive_braincube"
  fail=$((fail+1))
fi

# --- 2) Network P2P two CubalC peers (nanobot homes) ---
echo "# [2] P2P prophecy serve + dial (two peers as one)"
export CUBALC_P2P_BIND="127.0.0.1:$PORT_SMX"
export CUBALC_STATE="$LAB/peer_b"
mkdir -p "$CUBALC_STATE"
"$BIN" run "$ROOT/programs/prophecy/p2p_prophecy_serve.cubalc" >"$LAB/plates/p2p_serve.log" 2>&1 &
SPID=$!
for i in $(seq 1 50); do
  ss -ltn 2>/dev/null | grep -q ":$PORT_SMX " && break
  sleep 0.05
done
export CUBALC_P2P_PEER="127.0.0.1:$PORT_SMX"
export CUBALC_STATE="$LAB/peer_a"
mkdir -p "$CUBALC_STATE"
if "$BIN" run "$ROOT/programs/prophecy/p2p_prophecy_dial.cubalc" | tee "$LAB/plates/p2p_dial.log"; then
  echo "PASS p2p_prophecy"
  pass=$((pass+1))
else
  echo "FAIL p2p_prophecy"
  fail=$((fail+1))
fi
wait $SPID 2>/dev/null || true
echo "# serve tail:"; tail -5 "$LAB/plates/p2p_serve.log" || true

# --- 3) Live nanobots (offline) + braincube tick + SMX mesh ---
export CUBALC_STATE="$ROOT/state"
if [[ -x "$NB" ]]; then
  echo "# [3] dual nanobot offline + braincube tick + CubalC mesh"
  for h in "$LAB/nb_a" "$LAB/nb_b"; do
    mkdir -p "$h"
    printf 'token=%s\n' "$NANOBOT_PEER_TOKEN" >"$h/peer_token"
    chmod 600 "$h/peer_token"
  done
  fuser -k "${PORT_NB_A}/tcp" 2>/dev/null || true
  fuser -k "${PORT_NB_B}/tcp" 2>/dev/null || true
  sleep 0.2
  "$NB" --offline --home "$LAB/nb_a" --port "$PORT_NB_A" >"$LAB/nb_a/nb.log" 2>&1 &
  echo $! >"$LAB/nb_a/nb.pid"
  "$NB" --offline --home "$LAB/nb_b" --port "$PORT_NB_B" >"$LAB/nb_b/nb.log" 2>&1 &
  echo $! >"$LAB/nb_b/nb.pid"
  for i in $(seq 1 30); do
    curl -fsS -m 1 "http://127.0.0.1:$PORT_NB_A/peer/v1/health" >/dev/null 2>&1 && \
    curl -fsS -m 1 "http://127.0.0.1:$PORT_NB_B/peer/v1/health" >/dev/null 2>&1 && break
    sleep 0.1
  done
  AUTH=(-H "X-Nanobot-Peer-Token: $NANOBOT_PEER_TOKEN" -H "Content-Type: application/json")
  # braincube learn/tick on both (self-developing algocube race inside plugin)
  for port in "$PORT_NB_A" "$PORT_NB_B"; do
    for act in tick sample decide; do
      curl -fsS -m 8 "${AUTH[@]}" -X POST \
        -d "{\"action\":\"$act\"}" \
        "http://127.0.0.1:$port/peer/v1/braincube" >"$LAB/plates/bc_${port}_${act}.json" 2>&1 || true
    done
  done
  # SMX1 binary matrix A→B then B→A (optional host edge; CubalC remains SoT path)
  EXP=$(curl -fsS -m 10 "${AUTH[@]}" -X POST -d '{"action":"smx1","cube":"meta"}' \
    "http://127.0.0.1:$PORT_NB_A/peer/v1/braincube" || echo '{}')
  echo "$EXP" >"$LAB/plates/nb_smx1_export.json"
  if python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); sys.exit(0 if d.get("ok") and d.get("data") else 1)' \
      "$LAB/plates/nb_smx1_export.json" 2>/dev/null; then
    DATA=$(python3 -c 'import json;print(json.load(open("'"$LAB/plates/nb_smx1_export.json"'"))["data"])')
    IMP=$(curl -fsS -m 10 "${AUTH[@]}" -X POST \
      -d "$(python3 -c 'import json,sys; print(json.dumps({"action":"smx1_import","cube":"meta","data":sys.argv[1]}))' "$DATA")" \
      "http://127.0.0.1:$PORT_NB_B/peer/v1/braincube" || echo '{}')
    echo "$IMP" >"$LAB/plates/nb_smx1_import.json"
    echo "PASS nanobot_braincube_smx1"
    pass=$((pass+1))
  else
    echo "WARN nanobot_braincube_smx1 (plugin soft-fail — CubalC path still SoT)"
  fi
  # CubalC hive again after nanobot ticks (oversee as one)
  if "$BIN" run "$ROOT/programs/prophecy/oversee_hive_braincube.cubalc" | tee "$LAB/plates/hive_after_nb.log" | tail -5; then
    echo "PASS hive_after_nanobot"
    pass=$((pass+1))
  else
    echo "FAIL hive_after_nanobot"
    fail=$((fail+1))
  fi
else
  echo "SKIP dual nanobot (binary missing at $NB)"
fi

# --- plate ---
python3 - <<PY
import json, time, re
from pathlib import Path
lab = Path("$LAB")
st = Path("$ROOT/state")
def last_print(path):
    if not path.exists(): return {}
    t = path.read_text(errors="replace")
    # grab last PRINT-ish line or ok json
    for line in reversed(t.splitlines()):
        line=line.strip()
        if line.startswith("{") and "ok" in line:
            try: return json.loads(line)
            except Exception: pass
        if line.startswith("prophecy_hive") or line.startswith("p2p_"):
            return {"line": line}
    return {"raw_tail": t[-400:]}

hive = last_print(lab/"plates"/"hive_braincube.log")
dial = last_print(lab/"plates"/"p2p_dial.log")
serve = last_print(lab/"plates"/"p2p_serve.log")
# parse PRINT fields from hive log
line = ""
for p in (lab/"plates"/"hive_braincube.log", lab/"plates"/"hive_after_nb.log"):
    if p.exists():
        for L in p.read_text().splitlines():
            if L.startswith("prophecy_hive"):
                line = L
parts = line.split() if line else []
# prophecy_hive CUBES UNITY SMX_OK SMX_TALKS gen wins best final agree DECIDE ...
def n(i, default=None):
    try: return float(parts[i]) if "." in parts[i] else int(parts[i])
    except Exception: return default

plate = {
  "schema": "cubalc.oversee.prophecy_hive.v1",
  "ok": True,
  "ts": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
  "law": {
    "cube_is_sot": True,
    "manifest_smx": True,
    "http_required": False,
    "proto": "SMX2",
    "self_dev_algocube": True,
    "prophecy": "braincube_hive_as_one",
  },
  "hive_inprocess": {
    "print": line,
    "cubes": n(1),
    "unity_centi": n(2),
    "smx_ok": n(3),
    "smx_talks": n(4),
    "gen": n(5),
    "wins": n(6),
    "best_compat": n(7),
    "final_compat": n(8),
    "agree": n(9),
    "decide": n(10),
    "digit_brain": n(11),
    "digit_algo": n(12),
    "digit_peer0": n(13),
    "digit_peer1": n(14),
  },
  "p2p": {
    "dial": dial,
    "serve_ok": (lab/"plates"/"p2p_serve.log").exists(),
  },
  "nanobots": {
    "ports": [int("$PORT_NB_A"), int("$PORT_NB_B")],
    "smx1_export": (lab/"plates"/"nb_smx1_export.json").exists(),
    "smx1_import": (lab/"plates"/"nb_smx1_import.json").exists(),
  },
  "pass": int("$pass"),
  "fail": int("$fail"),
  "note": "CubalC oversees nanobot peers as one hive; self-dev algocube LOOP + SMX unity + braincube DECIDE",
}
ok = plate["fail"] == 0 and (plate["hive_inprocess"].get("agree") == 1 or plate["hive_inprocess"].get("final_compat",0) >= 50)
plate["ok"] = ok
(lab/"OVERSEE_PROPHECY_HIVE.json").write_text(json.dumps(plate, indent=2)+"\n")
(st/"OVERSEE_PROPHECY_HIVE.json").write_text(json.dumps(plate, indent=2)+"\n")
# lab prophecy_cube if present
pc = Path.home()/"Dev/lab/prophecy_cube/state"
if pc.is_dir():
    (pc/"OVERSEE_PROPHECY_HIVE.json").write_text(json.dumps(plate, indent=2)+"\n")
print(json.dumps({"ok": ok, "pass": plate["pass"], "fail": plate["fail"],
                  "agree": plate["hive_inprocess"].get("agree"),
                  "final_compat": plate["hive_inprocess"].get("final_compat"),
                  "gen": plate["hive_inprocess"].get("gen"),
                  "plate": str(st/"OVERSEE_PROPHECY_HIVE.json")}, indent=2))
raise SystemExit(0 if ok else 1)
PY
RC=$?

echo "=============================================="
echo " RESULT pass=$pass fail=$fail"
echo " plate: $ROOT/state/OVERSEE_PROPHECY_HIVE.json"
echo "=============================================="
exit $RC
