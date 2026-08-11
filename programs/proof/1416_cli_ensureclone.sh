#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1416_ensureclone.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# idempotent
cat > "$TMPDIR/idem.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
NEW Cell p 1 2 0
ENSURECLONE p a
ENSURECLONE p a
ASSERT ENSURECLONE_N == 0
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/idem.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss continues
cat > "$TMPDIR/miss.cubalc" << 'C'
HOLD_FLASH 1
ENSURECLONE missing_zz x
ASSERT OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/miss.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms ENSURECLONE 2>&1)
printf '%s\n' "$OUT" | grep -qi ENSURECLONE

echo "1416_cli_ensureclone: PASS"
