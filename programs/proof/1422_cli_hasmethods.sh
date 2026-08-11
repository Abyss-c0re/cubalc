#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1422_hasmethods.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss continues
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
HASMETHODS Cell tick no_such_zz
ASSERT LAST_N == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDMETHODS hard on miss
cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
NEEDMETHODS Cell tick no_such_zz
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qi NEEDMETHODS

# NEEDMETHODS ok when all present
cat > "$TMPDIR/ok.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
NEEDMETHODS Cell tick init
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/ok.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms HASMETHODS 2>&1)
printf '%s\n' "$OUT" | grep -qi HASMETHODS

echo "1422_cli_hasmethods: PASS"
