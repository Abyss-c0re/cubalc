#!/usr/bin/env bash
# Every 6 minutes: mechanical universal iter (build/test/plate).
# Pair with Grok scheduler agent for language synthesis each tick.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
INTERVAL="${UNIVERSAL_ITER_SEC:-360}"
PIDF="${CUBALC_STATE:-$ROOT/state}/evolve/universal_loop.pid"
LOG="${CUBALC_STATE:-$ROOT/state}/evolve/universal_loop.log"
mkdir -p "$(dirname "$PIDF")"
echo $$ >"$PIDF"
echo "[$(date -Iseconds)] universal_loop start interval=${INTERVAL}s" | tee -a "$LOG"
while true; do
  bash "$ROOT/scripts/universal_iter.sh" >>"$LOG" 2>&1 || true
  sleep "$INTERVAL"
done
