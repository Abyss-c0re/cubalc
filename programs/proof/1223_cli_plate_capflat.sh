#!/bin/sh
# cubalc plate capflat — CAPFLAT dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_capflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"score":{"a":50,"b":150,"c":100},"meta":{"score":200}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate capflat "$F" score 100)
printf '%s\n' "$OUT" | grep -q '"op":"capflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":2'
printf '%s\n' "$OUT" | grep -q '"match":4'
printf '%s\n' "$OUT" | grep -q '"max":100'
printf '%s\n' "$OUT" | grep -q '"needle":"score"'

OUT=$("$CUBALC" plate get "$F" score.b)
printf '%s\n' "$OUT" | grep -q 100
OUT=$("$CUBALC" plate get "$F" score.a)
printf '%s\n' "$OUT" | grep -q 50
OUT=$("$CUBALC" plate get "$F" meta.score)
printf '%s\n' "$OUT" | grep -q 100

rm -f "$F"
"$CUBALC" plate ensure "$F" '{"a":5,"b":20,"c":10}' | grep -q '"ok":true'
OUT=$("$CUBALC" plate capflat "$F" 10)
printf '%s\n' "$OUT" | grep -q '"n":1'
OUT=$("$CUBALC" plate get "$F" b)
printf '%s\n' "$OUT" | grep -q 10

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'capflat'

echo "1223_cli_plate_capflat: PASS"
