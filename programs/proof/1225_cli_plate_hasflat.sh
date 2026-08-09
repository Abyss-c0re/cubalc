#!/bin/sh
# cubalc plate hasflat|countflat dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_hasflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"score":{"a":1,"b":2},"meta":{"score":3},"error":{"code":9}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate hasflat "$F" score)
printf '%s\n' "$OUT" | grep -q '"op":"hasflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":1'
printf '%s\n' "$OUT" | grep -q '"count":3'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"needle":"score"'

OUT=$("$CUBALC" plate countflat "$F" score)
printf '%s\n' "$OUT" | grep -q '"op":"countflat"'
printf '%s\n' "$OUT" | grep -q '"n":3'

OUT=$("$CUBALC" plate hasflat "$F" nope)
printf '%s\n' "$OUT" | grep -q '"n":0'
printf '%s\n' "$OUT" | grep -q '"hit":false'

OUT=$("$CUBALC" plate countflat "$F")
printf '%s\n' "$OUT" | grep -q '"n":4'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" score.a)
printf '%s\n' "$OUT" | grep -q 1

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'hasflat'

echo "1225_cli_plate_hasflat: PASS"
