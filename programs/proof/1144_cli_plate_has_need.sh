#!/bin/sh
# cubalc plate has|need — multi-key contract on disk plates (HASPALL/NEEDP duals)
# Usability: shell/agent key gates without a .cubalc program.
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_plate_has_need"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"n":1,"ok":true,"role":"worker"}' | grep -q '"created":true'

# has: all present → exit 0
OUT=$("$CUBALC" plate has "$F" n ok role)
echo "$OUT" | grep -q '"op":"has"'
echo "$OUT" | grep -q '"has_all":true'
echo "$OUT" | grep -q '"n_miss":0'
"$CUBALC" plate has "$F" n ok role >/dev/null

# has: missing key → exit 1, miss listed
set +e
OUT2=$("$CUBALC" plate has "$F" n status host)
RC=$?
set -e
echo "$OUT2" | grep -q '"has_all":false'
echo "$OUT2" | grep -q 'status\|host'
echo "$OUT2" | grep -q '"ok":true'
test "$RC" -eq 1

# need: all present ok
"$CUBALC" plate need "$F" n ok | grep -q '"has_all":true'
"$CUBALC" plate need "$F" n ok >/dev/null

# need: miss → ok:false + err (hard gate)
set +e
OUT3=$("$CUBALC" plate need "$F" n missing_key)
RC=$?
set -e
echo "$OUT3" | grep -q '"ok":false'
echo "$OUT3" | grep -q 'missing plate keys'
echo "$OUT3" | grep -q 'missing_key'
test "$RC" -eq 1

# aliases
"$CUBALC" plate hasall "$F" n | grep -q '"has_all":true'
"$CUBALC" plate check "$F" n ok | grep -q '"op":"has"'
set +e
"$CUBALC" plate require "$F" nope >/dev/null
RC=$?
set -e
test "$RC" -eq 1

# empty plate / missing file soft {} → need fails
G="$ST/empty.json"
"$CUBALC" plate ensure "$G" | grep -q '"created":true'
set +e
"$CUBALC" plate need "$G" n | grep -q '"ok":false'
set -e

# usage lists ops
HELP=$("$CUBALC" plate 2>&1 || true)
echo "$HELP" | grep -q has
echo "$HELP" | grep -q need

# reg ensure/merge still work
"$CUBALC" plate merge "$F" '{"status":"ready"}' | grep -q '"ok":true'
"$CUBALC" plate has "$F" n status | grep -q '"has_all":true'

echo "cli_plate_has_need ok"
