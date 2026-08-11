#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1413_tryswap.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft continues past miss
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
NEW Cell p 1 3 0
TRYSWAP p missing_zz
ASSERT OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard SWAPOBJ miss is soft too (by design — both paths soft-miss)
cat > "$TMPDIR/miss.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
NEW Cell p 1 3 0
SWAPOBJ p missing_zz
ASSERT OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/miss.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# forms discovery
OUT=$("$CUBALC" forms TRYSWAP 2>&1)
printf '%s\n' "$OUT" | grep -qi TRYSWAP
OUT=$("$CUBALC" forms SWAPOBJ 2>&1)
printf '%s\n' "$OUT" | grep -qi SWAPOBJ

echo "1413_cli_tryswap: PASS"
