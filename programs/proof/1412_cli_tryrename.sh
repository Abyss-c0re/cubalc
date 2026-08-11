#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1412_tryrename.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft continues past redefine
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
NEW Cell p 1 3 0
NEW Cell q 1 1 0
TRYRENAME p q
ASSERT OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard RENAMEOBJ redefine fails process
cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
NEW Cell p 1 3 0
NEW Cell q 1 1 0
RENAMEOBJ p q
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'destination live|redefine|ok.:false'

OUT=$("$CUBALC" forms TRYRENAME 2>&1)
printf '%s\n' "$OUT" | grep -qi TRYRENAME

echo "1412_cli_tryrename: PASS"
