#!/bin/sh
# cubalc plate needflatn — NEEDFLATN dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_needflatn"
F="$ST/agent.json"
MISS="$ST/miss.cubalc"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"cfg":{"port":9090,"host":"h"},"timeout_ms":5000}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate needflatn "$F" port)
printf '%s\n' "$OUT" | grep -q '"op":"needflatn"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":9090'
printf '%s\n' "$OUT" | grep -q '"needles":1'
printf '%s\n' "$OUT" | grep -q '"miss_n":0'
printf '%s\n' "$OUT" | grep -q '"leaf":"cfg.port"'

OUT=$("$CUBALC" plate needflatn "$F" port timeout_ms)
printf '%s\n' "$OUT" | grep -q '"needles":2'
printf '%s\n' "$OUT" | grep -q '"n":9090'

# host is string-only leaf — pure-int miss
set +e
OUT=$("$CUBALC" plate needflatn "$F" host 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -q '"miss_n":1'
printf '%s\n' "$OUT" | grep -q 'host'
test "$RC" -eq 2

set +e
OUT=$("$CUBALC" plate needflatn "$F" port missing_int 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -q 'missing_int'
test "$RC" -eq 2

# language miss fail-fast
cat > "$MISS" << 'CUB'
HOLD_FLASH 1
LET PLATE = "{\"a\":\"str\"}"
NEEDFLATN "a"
PASS
CUB
set +e
OUT=$("$CUBALC" run -q "$MISS" 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q 'NEEDFLATN'
test "$RC" -ne 0

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'needflatn'

echo "1232_cli_plate_needflatn: PASS"
