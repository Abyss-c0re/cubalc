#!/bin/sh
# cubalc plate ensure + merge — CLI duals of ENSUREPLATE / JSONFILEMERGE
# Usability: shell/agent seed + multi-key overlay without a .cubalc program.
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_plate_ensure_merge"
F="$ST/agent.json"
PATCH="$ST/patch.json"
rm -rf "$ST"
mkdir -p "$ST"

# ensure creates missing plate with seed
OUT=$("$CUBALC" plate ensure "$F" '{"n":0,"role":"worker"}')
echo "$OUT" | grep -q '"ok":true'
echo "$OUT" | grep -q '"op":"ensure"'
echo "$OUT" | grep -q '"created":true'
echo "$OUT" | grep -q '"role":"worker"'
test -f "$F"

# ensure does not clobber existing object plate
OUT2=$("$CUBALC" plate ensure "$F" '{"n":99,"role":"boss"}')
echo "$OUT2" | grep -q '"created":false'
echo "$OUT2" | grep -q '"role":"worker"'
echo "$OUT2" | grep -vq '"role":"boss"'
"$CUBALC" plate get "$F" n | grep -q '"value":"0"'

# bare ensure on second path seeds {}
F2="$ST/empty.json"
OUT3=$("$CUBALC" plate ensure "$F2")
echo "$OUT3" | grep -q '"created":true'
echo "$OUT3" | grep -q '"plate":{}'

# merge multi-key overlay (inline)
OUT4=$("$CUBALC" plate merge "$F" '{"status":"ready","n":3}')
echo "$OUT4" | grep -q '"ok":true'
echo "$OUT4" | grep -q '"op":"merge"'
echo "$OUT4" | grep -q '"applied":'
echo "$OUT4" | grep -q '"status":"ready"'
"$CUBALC" plate get "$F" n | grep -q '"value":"3"'
"$CUBALC" plate get "$F" role | grep -q '"value":"worker"'
"$CUBALC" plate get "$F" status | grep -q '"value":"ready"'

# merge from @file
printf '%s\n' '{"host":"cubeB","n":4}' > "$PATCH"
OUT5=$("$CUBALC" plate merge "$F" "@$PATCH")
echo "$OUT5" | grep -q '"ok":true'
"$CUBALC" plate get "$F" host | grep -q '"value":"cubeB"'
"$CUBALC" plate get "$F" n | grep -q '"value":"4"'

# aliases: seed / patch
F3="$ST/alias.json"
"$CUBALC" plate seed "$F3" '{"k":1}' | grep -q '"created":true'
"$CUBALC" plate patch "$F3" '{"k":2,"m":9}' | grep -q '"ok":true'
"$CUBALC" plate get "$F3" k | grep -q '"value":"2"'
"$CUBALC" plate get "$F3" m | grep -q '"value":"9"'

# usage lists new ops
"$CUBALC" plate 2>/dev/null | grep -q ensure || \
  "$CUBALC" plate 2>&1 | grep -q ensure || true
HELP=$("$CUBALC" plate 2>&1 || true)
echo "$HELP" | grep -q ensure
echo "$HELP" | grep -q merge

# help mentions ensure/merge
"$CUBALC" help 2>&1 | grep -q 'ensure/merge\|ensure\|merge'

echo "cli_plate_ensure_merge ok"
