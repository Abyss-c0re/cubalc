#!/bin/sh
# cubalc plate changelog — CLI dual of JSONCHANGELOG for mesh plate sync
# Usability: shell/agent see "key: old → new" without a .cubalc program.
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_plate_changelog"
A="$ST/a.json"
B="$ST/b.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$A" '{"n":1,"role":"worker","host":"cubeA"}' | grep -q '"created":true'
"$CUBALC" plate ensure "$B" '{"n":1,"role":"worker","host":"cubeA"}' | grep -q '"created":true'

# identical → empty changelog, exit 0
OUT=$("$CUBALC" plate changelog "$A" "$B")
echo "$OUT" | grep -q '"op":"changelog"'
echo "$OUT" | grep -q '"equal":true'
echo "$OUT" | grep -q '"n":0'
"$CUBALC" plate changelog "$A" "$B" >/dev/null

# mutate B: host + new key
"$CUBALC" plate set "$B" host cubeB | grep -q '"ok":true'
"$CUBALC" plate set "$B" status ready | grep -q '"ok":true'

set +e
OUT2=$("$CUBALC" plate changelog "$A" "$B")
RC=$?
set -e
echo "$OUT2" | grep -q '"equal":false'
echo "$OUT2" | grep -q '"n":'
# human lines include key and arrow (or missing)
echo "$OUT2" | grep -E 'host|status' | grep -q .
# meta plate present
echo "$OUT2" | grep -q '"schema":"cubalc.plate.v1"'
test "$RC" -eq 1

# aliases
set +e
"$CUBALC" plate changes "$A" "$B" | grep -q '"op":"changelog"'
"$CUBALC" plate clog "$A" "$B" | grep -q '"n":'
set -e

# usage lists changelog
HELP=$("$CUBALC" plate 2>&1 || true)
echo "$HELP" | grep -q changelog

# reg: eq/diff still work
"$CUBALC" plate eq "$A" "$A" | grep -q '"equal":true'
set +e
"$CUBALC" plate diff "$A" "$B" >/dev/null
RC=$?
set -e
test "$RC" -eq 1

echo "cli_plate_changelog ok"
