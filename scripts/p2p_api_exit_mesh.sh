#!/usr/bin/env bash
# CubalC P2P API-share mesh — discovery is SMX2 only; plate for nanobot exit list.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
BASE="${CUBALC_P2P_LAB:-$ROOT/state/p2p_lab}"
PORT="${PORT_API_EXIT:-17744}"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
export CUBALC_SMX_KEY="${CUBALC_SMX_KEY:-$(openssl rand -hex 32)}"

mkdir -p "$BASE/api_exit" "$BASE/api_need" "$CUBALC_STATE"
[[ -x "$BIN" ]] || make -C "$ROOT" all

echo "# CubalC API exit mesh  key=$(echo "$CUBALC_SMX_KEY" | head -c 8)…"

echo "# 1) proof 10b_api_exit_node"
"$BIN" run "$ROOT/programs/proof/10b_api_exit_node.cubalc" | tee "$BASE/api_exit_proof.out" | tail -4

echo "# 2) in-process api_share_mesh"
"$BIN" run "$ROOT/programs/p2p/api_share_mesh.cubalc" | tee "$BASE/api_share_mesh.out" | tail -4

echo "# 3) serve exit + dial need (TCP SMX)"
export CUBALC_P2P_SERVE=1
export CUBALC_P2P_BIND="127.0.0.1:$PORT"
export CUBALC_API_OK=1
export CUBALC_EXIT_NODE=1
unset CUBALC_NEED_EXIT || true
export CUBALC_STATE="$BASE/api_exit"
mkdir -p "$CUBALC_STATE"
"$BIN" run "$ROOT/programs/p2p/api_exit_node.cubalc" >"$BASE/api_exit/serve.out" 2>&1 &
SPID=$!
echo $SPID >"$BASE/api_exit/cubalc.pid"
for i in $(seq 1 50); do
  if ss -ltn 2>/dev/null | grep -q ":$PORT "; then break; fi
  sleep 0.05
done

unset CUBALC_P2P_SERVE CUBALC_API_OK CUBALC_EXIT_NODE
export CUBALC_NEED_EXIT=1
export CUBALC_P2P_PEER="127.0.0.1:$PORT"
export CUBALC_STATE="$BASE/api_need"
mkdir -p "$CUBALC_STATE"
echo "#    dial NEED_EXIT → EXIT serve"
"$BIN" run "$ROOT/programs/p2p/api_exit_node.cubalc" | tee "$BASE/api_need/dial.out" | tail -8
wait $SPID || true

python3 - <<PY
import json, time
from pathlib import Path
base = Path("$BASE")
plate = {
  "schema": "cubalc.p2p.api_exit.v1",
  "ok": True,
  "ts": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
  "http_required": False,
  "proto": "SMX2",
  "wire": "u32le+CBLC_HMAC",
  "law": "any_api_ok_is_exit_node",
  "language": "CubalC",
  "share": "state_matrix_only",
  "hold_flash": 1,
  "bits": {
    "0": "ALIVE",
    "4": "API_OK",
    "5": "EXIT_NODE",
    "6": "SIGNED_IN",
    "8": "NEED_EXIT",
  },
  "programs": [
    "programs/proof/10b_api_exit_node.cubalc",
    "programs/p2p/api_share_mesh.cubalc",
    "programs/p2p/api_exit_node.cubalc",
  ],
  "nanobot_import": {
    "path_hint": "mesh/api_peers.json",
    "note": "nanobot reads exit peers for HTTP prompt relay; SMX only discovers capability",
  },
  "exits_example": [
    {"url": "http://127.0.0.1:18787", "exit_node": True, "api_ok": True, "role": "blackcube"}
  ],
}
(base / "API_EXIT_MESH.json").write_text(json.dumps(plate, indent=2) + "\n")
print("wrote", base / "API_EXIT_MESH.json")
# always root state (nanobot import path)
root_state = Path("$ROOT") / "state"
root_state.mkdir(parents=True, exist_ok=True)
(root_state / "API_EXIT_MESH.json").write_text(json.dumps(plate, indent=2) + "\n")
print("wrote", root_state / "API_EXIT_MESH.json")
PY

echo "# OK api_exit mesh"
