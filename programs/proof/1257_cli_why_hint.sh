#!/bin/sh
# run plate why_hint — agent recovery tip on sticky last_err
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

# soft EXPECT failure surfaces why_hint on run plate
OUT=$("$CUBALC" run -e 'HOLD_FLASH 1
LET x = 9
EXPECT x == 1
' 2>&1 || true)
printf '%s\n' "$OUT" | grep -q '"why_hint"'
printf '%s\n' "$OUT" | grep -q 'ASSERT_GOT\|EXPECT soft\|CLEAR_ERR\|is false\|EXPECT'

# clean run still has why_hint ok message
OUT=$("$CUBALC" run -e 'HOLD_FLASH 1
ASSERT 1 == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"why_hint"'
printf '%s\n' "$OUT" | grep -q 'no sticky LAST_ERR\|ok'

echo "1257_cli_why_hint: PASS"
