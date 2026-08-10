#!/bin/sh
# cubalc init --list + --plate uniform/pretty scaffold
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_init_list"
rm -rf "$ST"
mkdir -p "$ST"
cd "$ROOT"

OUT=$("$CUBALC" init --list)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.init.v1"'
printf '%s\n' "$OUT" | grep -q '"op":"list"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -qE '"n":[3-9]'
printf '%s\n' "$OUT" | grep -q 'agent_boot'
printf '%s\n' "$OUT" | grep -q 'plate_session'
printf '%s\n' "$OUT" | grep -q 'plate_peer_session'
printf '%s\n' "$OUT" | grep -q 'fat_session\|fat_boot'
printf '%s\n' "$OUT" | grep -q 'plate_uniform\|PRETTYP'

# templates alias
OUT=$("$CUBALC" init --templates)
printf '%s\n' "$OUT" | grep -q '"op":"list"'

# plate scaffold includes uniform + pretty and runs green
"$CUBALC" init "$ST/agent" --plate --force | grep -q '"template":"plate_session"'
grep -q 'INCLUDE plate_session' "$ST/agent.cubalc"
grep -q 'INCLUDE plate_uniform' "$ST/agent.cubalc"
grep -q 'PRETTYP' "$ST/agent.cubalc"
grep -q 'UNIFORM_NEEDLE' "$ST/agent.cubalc"
grep -q 'ASSERT UNIFORM_EQ' "$ST/agent.cubalc"

# run from ROOT so DEFAULT PLATE_PATH state/agent_plate.json resolves
cd "$ROOT"
CUBALC_STATE="$ST" "$CUBALC" run "$ST/agent.cubalc" | grep -q '"ok":true'
# durable plate under workspace state/ (scaffold default path)
test -f "$ROOT/state/agent_plate.json"
grep -q 'worker' "$ROOT/state/agent_plate.json"
grep -q 'ready' "$ROOT/state/agent_plate.json"
grep -q '"n"' "$ROOT/state/agent_plate.json"

echo "1256_cli_init_list: PASS"
