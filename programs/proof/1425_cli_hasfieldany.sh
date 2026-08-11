#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1425_hasfieldany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft any miss continues
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
LET opts = "nope_a,nope_b"
HASFIELDANY Cell opts
ASSERT LAST_N == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDFIELDANY hard when none
cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
LET opts = "nope_a,nope_b"
NEEDFIELDANY Cell opts
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qi NEEDFIELDANY

# NEEDFIELDANY ok if one present
cat > "$TMPDIR/ok.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
LET opts = "nope_a,alive"
NEEDFIELDANY Cell opts
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/ok.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms HASFIELDANY 2>&1)
printf '%s\n' "$OUT" | grep -qi HASFIELDANY

echo "1425_cli_hasfieldany: PASS"
