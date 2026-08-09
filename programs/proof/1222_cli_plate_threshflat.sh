#!/bin/sh
# cubalc plate threshflat|dropzeroflat dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_threshflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"hits":{"a":10,"b":2,"c":5},"meta":{"hits":1,"role":"w"}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate threshflat "$F" hits 5)
printf '%s\n' "$OUT" | grep -q '"op":"threshflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":2'
printf '%s\n' "$OUT" | grep -q '"drop":2'
printf '%s\n' "$OUT" | grep -q '"min":5'
printf '%s\n' "$OUT" | grep -q '"needle":"hits"'

OUT=$("$CUBALC" plate get "$F" hits.a)
printf '%s\n' "$OUT" | grep -q 10

# reseed for dropzero (ensure keeps existing plate)
rm -f "$F"
"$CUBALC" plate ensure "$F" '{"delta":{"x":0,"y":4,"z":0},"keep":1}' | grep -q '"ok":true'
OUT=$("$CUBALC" plate dropzeroflat "$F" delta)
printf '%s\n' "$OUT" | grep -q '"op":"dropzeroflat"'
printf '%s\n' "$OUT" | grep -q '"n":1'
printf '%s\n' "$OUT" | grep -q '"drop":2'
OUT=$("$CUBALC" plate get "$F" delta.y)
printf '%s\n' "$OUT" | grep -q 4
OUT=$("$CUBALC" plate get "$F" keep)
printf '%s\n' "$OUT" | grep -q 1

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'threshflat'

echo "1222_cli_plate_threshflat: PASS"
