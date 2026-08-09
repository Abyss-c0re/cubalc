#!/bin/sh
# cubalc plate needflat — NEEDFLAT dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_needflat"
F="$ST/agent.json"
MISS="$ST/miss.cubalc"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"tls":{"on":1},"agent_id":"a1","cfg":{"port":8080}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate needflat "$F" tls agent_id port)
printf '%s\n' "$OUT" | grep -q '"op":"needflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":3'
printf '%s\n' "$OUT" | grep -q '"miss_n":0'

set +e
OUT=$("$CUBALC" plate needflat "$F" tls missing_key 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -q '"miss_n":1'
printf '%s\n' "$OUT" | grep -q 'missing_key'
test "$RC" -eq 2

# language miss fail-fast
cat > "$MISS" << 'CUB'
HOLD_FLASH 1
LET PLATE = "{\"a\":1}"
NEEDFLAT "nope"
PASS
CUB
set +e
OUT=$("$CUBALC" run -q "$MISS" 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q 'NEEDFLAT'
test "$RC" -ne 0

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'needflat'

echo "1228_cli_plate_needflat: PASS"
