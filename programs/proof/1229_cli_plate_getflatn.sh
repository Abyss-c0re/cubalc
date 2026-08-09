#!/bin/sh
# cubalc plate getflatn — GETFLATN dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_getflatn"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"cfg":{"port":9090,"host":"h"},"meta":{"port":1}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate getflatn "$F" port)
printf '%s\n' "$OUT" | grep -q '"op":"getflatn"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"n":9090'
printf '%s\n' "$OUT" | grep -q '"leaf":"cfg.port"'

OUT=$("$CUBALC" plate getflatn "$F" host OR 0)
printf '%s\n' "$OUT" | grep -q '"hit":false'
printf '%s\n' "$OUT" | grep -q '"fallback":true'
printf '%s\n' "$OUT" | grep -q '"n":0'

OUT=$("$CUBALC" plate getflatn "$F" nope OR 8080)
printf '%s\n' "$OUT" | grep -q '"n":8080'
printf '%s\n' "$OUT" | grep -q '"fallback":true'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" cfg.port)
printf '%s\n' "$OUT" | grep -q 9090

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'getflatn'

echo "1229_cli_plate_getflatn: PASS"
