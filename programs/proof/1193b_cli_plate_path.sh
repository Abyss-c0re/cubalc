#!/bin/sh
# cubalc plate get|set|inc|del dotted path — dual of path sugar
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_path"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","freq":{"error":5,"warn":2}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate get "$F" freq.error)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"value":"5"'

OUT=$("$CUBALC" plate set "$F" freq.crit 12)
printf '%s\n' "$OUT" | grep -q '"ok":true'
OUT=$("$CUBALC" plate get "$F" freq.crit)
printf '%s\n' "$OUT" | grep -q '"value":"12"'
OUT=$("$CUBALC" plate get "$F" host)
printf '%s\n' "$OUT" | grep -q 'cubeA'

OUT=$("$CUBALC" plate inc "$F" freq.error)
printf '%s\n' "$OUT" | grep -q '"value":6'
OUT=$("$CUBALC" plate get "$F" freq.error)
printf '%s\n' "$OUT" | grep -q '"value":"6"'

OUT=$("$CUBALC" plate del "$F" freq.warn)
printf '%s\n' "$OUT" | grep -q '"ok":true'
OUT=$("$CUBALC" plate get "$F" freq.warn OR miss)
printf '%s\n' "$OUT" | grep -q '"or":true'
printf '%s\n' "$OUT" | grep -q 'miss'

# create deep path via set
OUT=$("$CUBALC" plate set "$F" cfg.net.port 8080)
printf '%s\n' "$OUT" | grep -q '"ok":true'
OUT=$("$CUBALC" plate get "$F" cfg.net.port)
printf '%s\n' "$OUT" | grep -q '8080'
OUT=$("$CUBALC" plate inc "$F" cfg.net.port 1)
printf '%s\n' "$OUT" | grep -q '"value":8081'

echo "1193b_cli_plate_path: PASS"
