#!/bin/sh
# cubalc plate avgflat — AVGFLAT dual (read-only)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_avgflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"scores":{"a":10,"b":20,"c":30},"label":"x","n":5}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate avgflat "$F" scores)
printf '%s\n' "$OUT" | grep -q '"op":"avgflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":20'
printf '%s\n' "$OUT" | grep -q '"count":3'
printf '%s\n' "$OUT" | grep -q '"avg":20'
printf '%s\n' "$OUT" | grep -q '"sum":60'
printf '%s\n' "$OUT" | grep -q '"needle":"scores"'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" n)
printf '%s\n' "$OUT" | grep -q 5

OUT=$("$CUBALC" plate avgflat "$F" n)
printf '%s\n' "$OUT" | grep -q '"n":5'
printf '%s\n' "$OUT" | grep -q '"count":1'

# all pure-int mean 16
OUT=$("$CUBALC" plate avgflat "$F")
printf '%s\n' "$OUT" | grep -q '"n":16'
printf '%s\n' "$OUT" | grep -q '"count":4'
printf '%s\n' "$OUT" | grep -q '"sum":65'

OUT=$("$CUBALC" plate meanflat "$F" scores)
printf '%s\n' "$OUT" | grep -q '"op":"avgflat"'
printf '%s\n' "$OUT" | grep -q '"n":20'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'avgflat'

echo "1230_cli_plate_avgflat: PASS"
