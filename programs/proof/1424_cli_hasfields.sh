#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1424_hasfields.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss continues
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
LET need = "nope_a,nope_b"
HASFIELDS Cell need
ASSERT LAST_N == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDFIELDS hard when miss
cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
LET need = "nope_a,alive"
NEEDFIELDS Cell need
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qi NEEDFIELDS

# NEEDFIELDS ok if all present
cat > "$TMPDIR/ok.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
LET need = "alive,energy"
NEEDFIELDS Cell need
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/ok.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms HASFIELDS 2>&1)
printf '%s\n' "$OUT" | grep -qi HASFIELDS

echo "1424_cli_hasfields: PASS"
