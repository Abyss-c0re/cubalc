#!/usr/bin/env bash
# CubeOS way flow — CubalC braincube core · viz viz · no flash
# C3
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PC="${PROPHECY_CUBE_ROOT:-$(cd "$ROOT/.." && pwd)}"
export PROPHECY_CUBE_ROOT="$PC"
export CUBALC_STATE="${CUBALC_STATE:-$PC/state}"
export HOLD_FLASH=1
mkdir -p "$CUBALC_STATE"
BIN="$ROOT/out/cubalc"
cd "$ROOT"
[[ -x "$BIN" ]] || make -s
echo "[cubeos-flow] boot braincube core → $CUBALC_STATE"
"$BIN" boot
N="${1:-8}"
for i in $(seq 1 "$N"); do
  "$BIN" os tick >/dev/null
  echo "[cubeos-flow] tick $i ok"
  sleep "${FLOW_SLEEP:-0.4}"
done
"$BIN" law 2>/dev/null || true
echo "[cubeos-flow] done · cubalc_viz_frame + viz_frame published"
