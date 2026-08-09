#!/bin/sh
# cubalc plate medianflat — MEDIANFLAT dual (read-only)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_medianflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"scores":{"a":10,"b":20,"c":30},"label":"x","n":5}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate medianflat "$F" scores)
printf '%s\n' "$OUT" | grep -q '"op":"medianflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":20'
printf '%s\n' "$OUT" | grep -q '"count":3'
printf '%s\n' "$OUT" | grep -q '"median":20'
printf '%s\n' "$OUT" | grep -q '"needle":"scores"'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" n)
printf '%s\n' "$OUT" | grep -q 5

OUT=$("$CUBALC" plate medianflat "$F" n)
printf '%s\n' "$OUT" | grep -q '"n":5'
printf '%s\n' "$OUT" | grep -q '"count":1'

# all pure-int: 10,20,30,5 → sorted 5,10,20,30 → lower mid index 1 → 10
OUT=$("$CUBALC" plate medianflat "$F")
printf '%s\n' "$OUT" | grep -q '"n":10'
printf '%s\n' "$OUT" | grep -q '"count":4'

OUT=$("$CUBALC" plate p50flat "$F" scores)
printf '%s\n' "$OUT" | grep -q '"op":"medianflat"'
printf '%s\n' "$OUT" | grep -q '"n":20'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'medianflat'

echo "1231_cli_plate_medianflat: PASS"
