#!/bin/sh
# cubalc plate nthflatn — NTHFLATN dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_nthflatn"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"cfg":{"port":9090,"host":"h"},"meta":{"port":1},"score":{"a":10,"b":20}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate nthflatn "$F" port 0)
printf '%s\n' "$OUT" | grep -q '"op":"nthflatn"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"n":9090'
printf '%s\n' "$OUT" | grep -q '"i":0'
printf '%s\n' "$OUT" | grep -q '"leaf":"cfg.port"'

OUT=$("$CUBALC" plate nthflatn "$F" port 1)
printf '%s\n' "$OUT" | grep -q '"n":1'
printf '%s\n' "$OUT" | grep -q '"leaf":"meta.port"'

OUT=$("$CUBALC" plate nthflatn "$F" port 2 OR 8080)
printf '%s\n' "$OUT" | grep -q '"hit":false'
printf '%s\n' "$OUT" | grep -q '"fallback":true'
printf '%s\n' "$OUT" | grep -q '"n":8080'

OUT=$("$CUBALC" plate nthflatn "$F" score 1)
printf '%s\n' "$OUT" | grep -q '"n":20'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" cfg.port)
printf '%s\n' "$OUT" | grep -q 9090

OUT=$("$CUBALC" plate indexflatn "$F" port 0)
printf '%s\n' "$OUT" | grep -q '"op":"nthflatn"'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'nthflatn'

echo "1238_cli_plate_nthflatn: PASS"
