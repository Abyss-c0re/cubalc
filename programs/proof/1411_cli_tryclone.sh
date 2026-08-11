#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1411_tryclone.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft continues past redefine
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
NEW Cell p 1 3 0
CLONEOBJ p a
TRYCLONE p a
ASSERT OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard CLONEOBJ redefine fails process
cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
NEW Cell p 1 3 0
CLONEOBJ p a
CLONEOBJ p a
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'redefine|ok.:false'

OUT=$("$CUBALC" forms TRYCLONE 2>&1)
printf '%s\n' "$OUT" | grep -qi TRYCLONE

echo "1411_cli_tryclone: PASS"
