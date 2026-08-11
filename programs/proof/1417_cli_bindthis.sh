#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1417_bindthis.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss continues
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
BINDTHIS missing_zz
ASSERT OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# THIS works after bind
cat > "$TMPDIR/use.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
NEW Cell c 1 8 0
BINDTHIS c
SETF THIS energy 3
GETF c energy
ASSERT LAST_N == 3
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/use.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms BINDTHIS 2>&1)
printf '%s\n' "$OUT" | grep -qi BINDTHIS

echo "1417_cli_bindthis: PASS"
