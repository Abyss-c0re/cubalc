#!/bin/sh
# cubalc init --plate — scaffold plate_session durable-state starter
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_init_plate_proof"
rm -rf "$ST"
mkdir -p "$ST"
"$CUBALC" init "$ST/agent" --plate --force | grep -q '"template":"plate_session"'
grep -q 'INCLUDE plate_session' "$ST/agent.cubalc"
grep -q 'SETP' "$ST/agent.cubalc"
CUBALC_STATE="$ST" "$CUBALC" run "$ST/agent.cubalc" | grep -q '"ok":true'
"$CUBALC" init "$ST/hello" --force | grep -q '"template":"agent_boot"'
echo "init_plate ok"
