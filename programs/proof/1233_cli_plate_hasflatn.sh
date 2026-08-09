#!/bin/sh
# cubalc plate hasflatn|countflatn dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_hasflatn"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"cfg":{"port":9090,"host":"h"},"meta":{"port":1},"score":{"a":10,"b":20}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate hasflatn "$F" port)
printf '%s\n' "$OUT" | grep -q '"op":"hasflatn"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":1'
printf '%s\n' "$OUT" | grep -q '"count":2'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"needle":"port"'

OUT=$("$CUBALC" plate countflatn "$F" port)
printf '%s\n' "$OUT" | grep -q '"op":"countflatn"'
printf '%s\n' "$OUT" | grep -q '"n":2'

# host string-only → no pure-int
OUT=$("$CUBALC" plate hasflatn "$F" host)
printf '%s\n' "$OUT" | grep -q '"n":0'
printf '%s\n' "$OUT" | grep -q '"hit":false'

OUT=$("$CUBALC" plate countflatn "$F" score)
printf '%s\n' "$OUT" | grep -q '"n":2'

# all pure-int leaves: 9090,1,10,20 = 4
OUT=$("$CUBALC" plate countflatn "$F")
printf '%s\n' "$OUT" | grep -q '"n":4'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" cfg.port)
printf '%s\n' "$OUT" | grep -q 9090

OUT=$("$CUBALC" plate anyflatn "$F" port)
printf '%s\n' "$OUT" | grep -q '"op":"hasflatn"'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'hasflatn'
printf '%s\n' "$OUT" | grep -q 'countflatn'

echo "1233_cli_plate_hasflatn: PASS"
