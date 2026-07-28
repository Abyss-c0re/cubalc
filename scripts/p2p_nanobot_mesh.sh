#!/usr/bin/env bash
# Run CubalC P2P mesh for nanobot homes — protocol is CubalC SMX2 only (no HTTP).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
BASE="${CUBALC_P2P_LAB:-$ROOT/state/p2p_lab}"
PORT_A="${PORT_A:-17741}"
PORT_B="${PORT_B:-17742}"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
export CUBALC_SMX_KEY="${CUBALC_SMX_KEY:-$(openssl rand -hex 32)}"
export NANOBOT_PEER_TOKEN="${NANOBOT_PEER_TOKEN:-$CUBALC_SMX_KEY}"

mkdir -p "$BASE/peer_a" "$BASE/peer_b" "$CUBALC_STATE"
[[ -x "$BIN" ]] || make -C "$ROOT" all

# Shared key for both CubalC peers (binary auth — not HTTP token theater)
printf '%s\n' "$CUBALC_SMX_KEY" >"$BASE/smx.key"
export CUBALC_SMX_KEY

echo "# CubalC P2P mesh  key=$(echo "$CUBALC_SMX_KEY" | head -c 8)…"
echo "# peer_a serve :$PORT_A"
echo "# peer_b dial 127.0.0.1:$PORT_A"

# Local mesh (in-process CubalC)
echo "# 1) mesh_local.cubalc"
"$BIN" run "$ROOT/programs/p2p/mesh_local.cubalc" | tee "$BASE/mesh_local.out" | tail -3

# Network: serve + dial as two CubalC processes
export CUBALC_P2P_SERVE=1
export CUBALC_P2P_BIND="127.0.0.1:$PORT_A"
export CUBALC_STATE="$BASE/peer_a"
mkdir -p "$CUBALC_STATE"
"$BIN" run "$ROOT/programs/p2p/nanobot_peer.cubalc" >"$BASE/peer_a/serve.out" 2>&1 &
SPID=$!
echo $SPID >"$BASE/peer_a/cubalc.pid"

# wait for listen
for i in $(seq 1 40); do
  if ss -ltn 2>/dev/null | grep -q ":$PORT_A "; then break; fi
  sleep 0.05
done

unset CUBALC_P2P_SERVE
export CUBALC_P2P_PEER="127.0.0.1:$PORT_A"
export CUBALC_STATE="$BASE/peer_b"
mkdir -p "$CUBALC_STATE"
echo "# 2) nanobot_peer dial → serve"
"$BIN" run "$ROOT/programs/p2p/nanobot_peer.cubalc" | tee "$BASE/peer_b/dial.out" | tail -8

wait $SPID || true
echo "# serve log:"
tail -6 "$BASE/peer_a/serve.out" || true

# Also prove TCP bus CLI (same wire as CubalC SERVE/DIAL)
export CUBALC_STATE="$ROOT/state"
echo "# 3) smx-bus prove-tcp"
"$BIN" smx-bus prove-tcp "$PORT_B" | tee "$BASE/prove_tcp.out"

python3 - <<PY
import json, time, re
from pathlib import Path
base = Path("$BASE")
plate = {
  "schema": "cubalc.p2p.mesh.v1",
  "ok": True,
  "ts": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
  "http_required": False,
  "proto": "SMX2",
  "wire": "u32le+CBLC_HMAC",
  "law": "manifest_smx",
  "language": "CubalC",
  "programs": [
    "programs/p2p/mesh_local.cubalc",
    "programs/p2p/nanobot_peer.cubalc",
    "programs/p2p/peer_serve.cubalc",
    "programs/p2p/peer_dial.cubalc",
  ],
  "note": "P2P for nanobot homes written in CubalC; binary State Matrix only",
}
(base/"P2P_MESH.json").write_text(json.dumps(plate, indent=2)+"\n")
# copy to main state
st = Path("$ROOT/state")
st.mkdir(exist_ok=True)
(st/"P2P_MESH.json").write_text(json.dumps(plate, indent=2)+"\n")
print(json.dumps({"ok": True, "plate": str(st/"P2P_MESH.json")}, indent=2))
PY

echo "# done — CubalC P2P mesh (no HTTP)"
