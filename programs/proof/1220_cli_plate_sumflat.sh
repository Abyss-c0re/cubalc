#!/bin/sh
# cubalc plate sumflat — SUMFLAT dual (read-only)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_sumflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"hits_total":1,"cfg":{"meta":{"hits":2,"role":"w"},"port":8080},"errs":3}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate sumflat "$F" hits)
printf '%s\n' "$OUT" | grep -q '"op":"sumflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":3'
printf '%s\n' "$OUT" | grep -q '"count":2'
printf '%s\n' "$OUT" | grep -q '"sum":3'
printf '%s\n' "$OUT" | grep -q '"needle":"hits"'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" hits_total)
printf '%s\n' "$OUT" | grep -q 1

OUT=$("$CUBALC" plate sumflat "$F" port)
printf '%s\n' "$OUT" | grep -q '"n":8080'
printf '%s\n' "$OUT" | grep -q '"count":1'

OUT=$("$CUBALC" plate sumflat "$F")
printf '%s\n' "$OUT" | grep -q '"n":8086'
printf '%s\n' "$OUT" | grep -q '"count":4'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'sumflat'

echo "1220_cli_plate_sumflat: PASS"
