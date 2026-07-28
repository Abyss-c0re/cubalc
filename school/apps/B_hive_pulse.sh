#!/usr/bin/env bash
# App B — Hive Pulse: charge hive atom, flow energy, store chain
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/out/cubalc"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
mkdir -p "$CUBALC_STATE" "$(dirname "$0")/../students/${STUDENT:-anon}/apps"
OUT="$(dirname "$0")/../students/${STUDENT:-anon}/apps/B_hive_pulse.json"
"$BIN" impulse cube-hive 1 | tee "$OUT.impulse"
"$BIN" flow 10 12 | tee "$OUT.flow"
"$BIN" cubechain verify | tee "$OUT.verify"
grep -q '"ok":true' "$OUT.impulse" && grep -q '"ok":true' "$OUT.verify" \
  && echo '{"app":"B_hive_pulse","ok":true}' | tee "$OUT"
