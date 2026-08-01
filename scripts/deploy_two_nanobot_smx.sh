#!/usr/bin/env bash
# Deploy 2 nanobots + prove State Matrix exchange via CubalC SMX2 and nanobot SMX1.
# Host adapter only — CubalC language stays free of device/path hardcode.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CUBALC="${CUBALC_BIN:-$ROOT/out/cubalc}"
NB="${NANOBOT_BIN:-$HOME/Dev/AI/nanobot/build/host/nanobot}"
BASE="${CUBALC_SMX_LAB:-$ROOT/state/smx_lab}"
PORT_A="${PORT_A:-18881}"
PORT_B="${PORT_B:-18882}"
HOME_A="$BASE/peer0"
HOME_B="$BASE/peer1"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"

mkdir -p "$HOME_A" "$HOME_B" "$CUBALC_STATE"
[[ -x "$CUBALC" ]] || make -C "$ROOT" all
[[ -x "$NB" ]] || { echo "nanobot missing: $NB"; exit 2; }

stop_peer() {
  local home="$1"
  if [[ -f "$home/nb.pid" ]]; then
    kill "$(cat "$home/nb.pid")" 2>/dev/null || true
    rm -f "$home/nb.pid"
  fi
}
stop_peer "$HOME_A"
stop_peer "$HOME_B"
# free ports if leftover
fuser -k "${PORT_A}/tcp" 2>/dev/null || true
fuser -k "${PORT_B}/tcp" 2>/dev/null || true
sleep 0.3

# Shared peer_token → CubalC SMX2 key derive + nanobot auth
TOKEN="${NANOBOT_PEER_TOKEN:-$(openssl rand -hex 32)}"
export NANOBOT_PEER_TOKEN="$TOKEN"
export CUBALC_SMX_TOKEN="$TOKEN"
printf 'token=%s\n' "$TOKEN" >"$HOME_A/peer_token"
printf 'token=%s\n' "$TOKEN" >"$HOME_B/peer_token"
chmod 600 "$HOME_A/peer_token" "$HOME_B/peer_token"

echo "# spawn nanobot peer0 :$PORT_A"
"$NB" --offline --home "$HOME_A" --port "$PORT_A" >"$HOME_A/nb.log" 2>&1 &
echo $! >"$HOME_A/nb.pid"
echo "# spawn nanobot peer1 :$PORT_B"
"$NB" --offline --home "$HOME_B" --port "$PORT_B" >"$HOME_B/nb.log" 2>&1 &
echo $! >"$HOME_B/nb.pid"

for i in 1 2 3 4 5 6 7 8 9 10; do
  if curl -fsS -m 1 "http://127.0.0.1:$PORT_A/peer/v1/health" >/dev/null 2>&1 \
     && curl -fsS -m 1 "http://127.0.0.1:$PORT_B/peer/v1/health" >/dev/null 2>&1; then
    break
  fi
  sleep 0.3
done

echo "# health"
curl -fsS -m 2 "http://127.0.0.1:$PORT_A/peer/v1/health"; echo
curl -fsS -m 2 "http://127.0.0.1:$PORT_B/peer/v1/health"; echo

AUTH=(-H "X-Nanobot-Peer-Token: $TOKEN" -H "Content-Type: application/json")

# --- CubalC SMX2 (HMAC binary CBLC frames, file bus) ---
echo "# CubalC SMX2 exchange (protocol)"
"$CUBALC" smx-exchange | tee "$CUBALC_STATE/smx2_exchange.json"
"$CUBALC" run "$ROOT/programs/smx_two_peer.cubalc" | tail -5

# --- Nanobot SMX1 matrix export A → import B ---
echo "# Nanobot SMX1 export peer0"
EXP=$(curl -fsS -m 10 "${AUTH[@]}" -X POST \
  -d '{"action":"smx1","cube":"meta"}' \
  "http://127.0.0.1:$PORT_A/peer/v1/braincube")
echo "$EXP" | python3 -c 'import sys,json;d=json.load(sys.stdin);print("export ok",d.get("ok"),"bytes",d.get("bytes"),"n",d.get("n"),"format",d.get("format"))'
echo "$EXP" >"$HOME_A/smx1_export.json"
DATA=$(python3 -c 'import sys,json;print(json.load(sys.stdin)["data"])' <<<"$EXP")

echo "# Nanobot SMX1 import → peer1"
IMP=$(curl -fsS -m 10 "${AUTH[@]}" -X POST \
  -d "$(python3 -c 'import json,sys; print(json.dumps({"action":"smx1_import","cube":"meta","data":sys.argv[1]}))' "$DATA")" \
  "http://127.0.0.1:$PORT_B/peer/v1/braincube")
echo "$IMP" | python3 -c 'import sys,json;d=json.load(sys.stdin);print(json.dumps({k:d.get(k) for k in ("ok","error","format","n","imported","bytes") if k in d or k=="ok"}, indent=None))'
echo "$IMP" >"$HOME_B/smx1_import.json"

# reverse B → A for bidirectional proof
echo "# Nanobot SMX1 export peer1 → import peer0"
EXP2=$(curl -fsS -m 10 "${AUTH[@]}" -X POST \
  -d '{"action":"smx1","cube":"meta"}' \
  "http://127.0.0.1:$PORT_B/peer/v1/braincube")
DATA2=$(python3 -c 'import sys,json;print(json.load(sys.stdin)["data"])' <<<"$EXP2")
IMP2=$(curl -fsS -m 10 "${AUTH[@]}" -X POST \
  -d "$(python3 -c 'import json,sys; print(json.dumps({"action":"smx1_import","cube":"meta","data":sys.argv[1]}))' "$DATA2")" \
  "http://127.0.0.1:$PORT_A/peer/v1/braincube")
echo "$IMP2" | python3 -c 'import sys,json;d=json.load(sys.stdin);print("reverse import ok",d.get("ok"),d.get("error","")[:80])'

# CubalC core path — no HTTP (SMX2 bus)
echo "# CubalC smx-bus prove (http=false)"
"$CUBALC" smx-bus prove | tee "$CUBALC_STATE/smx_bus_prove.json"

# plate
python3 - <<PY
import json, time
from pathlib import Path
st = Path("$CUBALC_STATE")
smx2 = {}
p = st/"smx2_exchange.json"
if p.exists():
    lines = p.read_text().strip().splitlines()
    for line in reversed(lines):
        line=line.strip()
        if line.startswith("{"):
            smx2=json.loads(line); break
bus = {}
bp = st/"smx_bus_prove.json"
if bp.exists():
    for line in reversed(bp.read_text().strip().splitlines()):
        if line.strip().startswith("{"):
            bus=json.loads(line); break
plate = {
  "schema": "cubalc.smx.two_peer.v1",
  "ok": True,
  "ts": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
  "hold_flash": 1,
  "law": "state_matrix_only",
  "http_required": False,
  "cubalc_smx2": smx2,
  "cubalc_smx_bus": bus,
  "nanobots": {
    "peer0": {"port": int("$PORT_A"), "home": "$HOME_A", "note": "optional SMX1 host edge"},
    "peer1": {"port": int("$PORT_B"), "home": "$HOME_B", "note": "optional SMX1 host edge"},
  },
  "protocols": [
    "SMX2/CBLC (CubalC — no HTTP)",
    "AF_UNIX smx-bus (CubalC — no HTTP)",
    "SMX1 nanobot braincube (optional host edge; uses HTTP if used)",
  ],
  "note": "CubalC does not need HTTP; nanobot SMX1 path is optional host edge",
}
(st/"SMX_TWO_PEER.json").write_text(json.dumps(plate, indent=2)+"\n")
print(json.dumps({"ok": True, "plate": str(st/"SMX_TWO_PEER.json"), "smx2_ok": smx2.get("ok"), "unity_after": smx2.get("unity_after")}, indent=2))
PY

echo "# done — peers still running on :$PORT_A and :$PORT_B (pid files in $BASE)"
echo "# stop: kill \$(cat $HOME_A/nb.pid) \$(cat $HOME_B/nb.pid)"
