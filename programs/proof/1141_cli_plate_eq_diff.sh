#!/bin/sh
# cubalc plate eq|ne|diff — CLI duals of JSONEQ / JSONCHANGED for mesh plates
# Usability: shell/agent verify peer sync without a .cubalc program.
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_plate_eq_diff"
A="$ST/a.json"
B="$ST/b.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$A" '{"n":1,"role":"worker","host":"cubeA"}' | grep -q '"created":true'
"$CUBALC" plate ensure "$B" '{"role":"worker","n":1,"host":"cubeA"}' | grep -q '"created":true'

# order-independent equality
OUT=$("$CUBALC" plate eq "$A" "$B")
echo "$OUT" | grep -q '"ok":true'
echo "$OUT" | grep -q '"op":"eq"'
echo "$OUT" | grep -q '"equal":true'
"$CUBALC" plate eq "$A" "$B" >/dev/null

# same alias
"$CUBALC" plate same "$A" "$B" | grep -q '"equal":true'

# ne fails (exit 1) when equal
set +e
"$CUBALC" plate ne "$A" "$B" >/dev/null
RC=$?
set -e
test "$RC" -eq 1

# mutate B → unequal
"$CUBALC" plate set "$B" host cubeB | grep -q '"ok":true'
OUT2=$("$CUBALC" plate eq "$A" "$B" || true)
echo "$OUT2" | grep -q '"equal":false'
set +e
"$CUBALC" plate eq "$A" "$B" >/dev/null
RC=$?
set -e
test "$RC" -eq 1

"$CUBALC" plate ne "$A" "$B" | grep -q '"equal":false'
"$CUBALC" plate ne "$A" "$B" >/dev/null

# diff lists changed keys
OUT3=$("$CUBALC" plate diff "$A" "$B" || true)
echo "$OUT3" | grep -q '"op":"diff"'
echo "$OUT3" | grep -q '"equal":false'
echo "$OUT3" | grep -q '"n":'
echo "$OUT3" | grep -q 'host'
set +e
"$CUBALC" plate diff "$A" "$B" >/dev/null
RC=$?
set -e
test "$RC" -eq 1

# restore equality via merge; diff exit 0
"$CUBALC" plate merge "$B" '{"host":"cubeA"}' | grep -q '"ok":true'
"$CUBALC" plate eq "$A" "$B" | grep -q '"equal":true'
OUT4=$("$CUBALC" plate diff "$A" "$B")
echo "$OUT4" | grep -q '"equal":true'
echo "$OUT4" | grep -q '"n":0'
"$CUBALC" plate diff "$A" "$B" >/dev/null

# missing path2 soft {} unequal to seeded A
C="$ST/missing.json"
set +e
OUT5=$("$CUBALC" plate eq "$A" "$C")
RC=$?
set -e
echo "$OUT5" | grep -q '"equal":false'
echo "$OUT5" | grep -q '"file2":false'
test "$RC" -eq 1

# usage lists ops
HELP=$("$CUBALC" plate 2>&1 || true)
echo "$HELP" | grep -q eq
echo "$HELP" | grep -q diff
"$CUBALC" help 2>&1 | grep -q 'eq/diff\|eq\|diff'

echo "cli_plate_eq_diff ok"
