#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1427_hasobjany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft any miss continues
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
HASOBJANY ghost nope
ASSERT LAST_N == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDOBJANY hard when none
cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
NEEDOBJANY ghost spectre
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qi NEEDOBJANY

# NEEDOBJANY ok if one present
cat > "$TMPDIR/ok.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
NEW Cell a 1 1 0
NEEDOBJANY ghost a
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/ok.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms HASOBJANY 2>&1)
printf '%s\n' "$OUT" | grep -qi HASOBJANY

echo "1427_cli_hasobjany: PASS"
