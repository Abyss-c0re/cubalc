#!/bin/sh
# cubalc plate scaleflat — SCALEFLAT dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_scaleflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"score":{"a":2,"b":5},"meta":{"score":10}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate scaleflat "$F" score 3)
printf '%s\n' "$OUT" | grep -q '"op":"scaleflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":3'
printf '%s\n' "$OUT" | grep -q '"match":3'
printf '%s\n' "$OUT" | grep -q '"factor":3'
printf '%s\n' "$OUT" | grep -q '"needle":"score"'

OUT=$("$CUBALC" plate get "$F" score.a)
printf '%s\n' "$OUT" | grep -q 6
OUT=$("$CUBALC" plate get "$F" score.b)
printf '%s\n' "$OUT" | grep -q 15
OUT=$("$CUBALC" plate get "$F" meta.score)
printf '%s\n' "$OUT" | grep -q 30

rm -f "$F"
"$CUBALC" plate ensure "$F" '{"a":3,"b":4}' | grep -q '"ok":true'
OUT=$("$CUBALC" plate scaleflat "$F" 2)
printf '%s\n' "$OUT" | grep -q '"n":2'
OUT=$("$CUBALC" plate get "$F" a)
printf '%s\n' "$OUT" | grep -q 6

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'scaleflat'

echo "1224_cli_plate_scaleflat: PASS"
