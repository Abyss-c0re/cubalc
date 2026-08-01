#!/usr/bin/env bash
# App A — Recycle Scanner: scrap electronics → LEGO cube board
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/out/cubalc"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
mkdir -p "$CUBALC_STATE" "$(dirname "$0")/../students/${STUDENT:-anon}/apps"
OUT="$(dirname "$0")/../students/${STUDENT:-anon}/apps/A_recycle_scanner.json"
PLATE="${1:-NEXUS_COORD v1 | from=scrap-lab | type=recycle_scan | role=earth_heal | status=ONLINE | hold_flash=1 | unity=1.0 |}"
"$BIN" genesis "$PLATE" | tee "$OUT.genesis"
"$BIN" flow 6 10 | tee "$OUT.flow"
"$BIN" cubes | tee "$OUT.cubes.txt" >/dev/null
# pass if genesis ok
grep -q '"ok":true' "$OUT.genesis" && echo '{"app":"A_recycle_scanner","ok":true}' | tee "$OUT"
