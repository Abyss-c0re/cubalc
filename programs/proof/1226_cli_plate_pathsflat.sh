#!/bin/sh
# cubalc plate pathsflat|valsflat dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_pathsflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"score":{"a":10,"b":20},"meta":{"score":30},"label":"x"}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate pathsflat "$F" score)
printf '%s\n' "$OUT" | grep -q '"op":"pathsflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":3'
printf '%s\n' "$OUT" | grep -q 'score.a'
printf '%s\n' "$OUT" | grep -q 'meta.score'
printf '%s\n' "$OUT" | grep -q '"needle":"score"'

OUT=$("$CUBALC" plate valsflat "$F" score)
printf '%s\n' "$OUT" | grep -q '"op":"valsflat"'
printf '%s\n' "$OUT" | grep -q '"n":3'
printf '%s\n' "$OUT" | grep -q '10'
printf '%s\n' "$OUT" | grep -q '30'

OUT=$("$CUBALC" plate pathsflat "$F" nope)
printf '%s\n' "$OUT" | grep -q '"n":0'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" score.a)
printf '%s\n' "$OUT" | grep -q 10

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'pathsflat'

echo "1226_cli_plate_pathsflat: PASS"
