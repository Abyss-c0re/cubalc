#!/bin/sh
# cubalc plate pathsflatn|valsflatn dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_pathsflatn"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"score":{"a":10,"b":20},"meta":{"score":30,"role":"w"},"label":"x"}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate pathsflatn "$F" score)
printf '%s\n' "$OUT" | grep -q '"op":"pathsflatn"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":3'
printf '%s\n' "$OUT" | grep -q 'score.a'
printf '%s\n' "$OUT" | grep -q 'meta.score'
printf '%s\n' "$OUT" | grep -q '"needle":"score"'

OUT=$("$CUBALC" plate valsflatn "$F" score)
printf '%s\n' "$OUT" | grep -q '"op":"valsflatn"'
printf '%s\n' "$OUT" | grep -q '"n":3'
printf '%s\n' "$OUT" | grep -q '10'
printf '%s\n' "$OUT" | grep -q '30'
# role string must not appear as pure-int val for score needle — already n:3

# label is string only path — zero pure-int
OUT=$("$CUBALC" plate pathsflatn "$F" label)
printf '%s\n' "$OUT" | grep -q '"n":0'

# all pure-int: 10,20,30 = 3
OUT=$("$CUBALC" plate valsflatn "$F")
printf '%s\n' "$OUT" | grep -q '"n":3'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" score.a)
printf '%s\n' "$OUT" | grep -q 10

OUT=$("$CUBALC" plate matchpathsn "$F" score)
printf '%s\n' "$OUT" | grep -q '"op":"pathsflatn"'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'pathsflatn'
printf '%s\n' "$OUT" | grep -q 'valsflatn'

echo "1234_cli_plate_pathsflatn: PASS"
