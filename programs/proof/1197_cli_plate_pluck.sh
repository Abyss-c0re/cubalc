#!/bin/sh
# cubalc plate pluck — multi-key peel bag (PLUCKP dual, dotted paths)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_pluck"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","freq":{"error":5,"warn":2},"n":3}' | grep -q '"ok":true'

# bag lines + meta plate
OUT=$("$CUBALC" plate pluck "$F" host freq.error n)
printf '%s\n' "$OUT" | grep -q '^cubeA$'
printf '%s\n' "$OUT" | grep -q '^5$'
printf '%s\n' "$OUT" | grep -q '^3$'
printf '%s\n' "$OUT" | grep -q '"op":"pluck"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":3'
printf '%s\n' "$OUT" | grep -q '"hit":3'
printf '%s\n' "$OUT" | grep -q '"miss":0'
printf '%s\n' "$OUT" | grep -q 'freq.error'

# miss field empty line still counts
OUT=$("$CUBALC" plate pluck "$F" host freq.crit n)
printf '%s\n' "$OUT" | grep -q '"hit":2'
printf '%s\n' "$OUT" | grep -q '"miss":1'
printf '%s\n' "$OUT" | grep -q '^cubeA$'
printf '%s\n' "$OUT" | grep -q '^3$'

# aliases
OUT=$("$CUBALC" plate getall "$F" host n)
printf '%s\n' "$OUT" | grep -q '"op":"pluck"'
printf '%s\n' "$OUT" | grep -q '"hit":2'

OUT=$("$CUBALC" plate pluckp "$F" freq/error)
printf '%s\n' "$OUT" | grep -q '^5$'
printf '%s\n' "$OUT" | grep -q '"hit":1'

# help lists pluck
OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'pluck'

echo "1197_cli_plate_pluck: PASS"
