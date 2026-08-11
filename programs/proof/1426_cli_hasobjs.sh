#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1426_hasobjs.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss continues
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
NEW Cell a 1 1 0
HASOBJS a ghost
ASSERT LAST_N == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDOBJS hard on miss
cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
NEW Cell a 1 1 0
NEEDOBJS a ghost
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qi NEEDOBJS

# NEEDOBJS ok when all live
cat > "$TMPDIR/ok.cubalc" << 'C'
HOLD_FLASH 1
INCLUDE life_cell
NEW Cell a 1 1 0
NEW Cell b 1 1 0
NEEDOBJS a b
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/ok.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms HASOBJS 2>&1)
printf '%s\n' "$OUT" | grep -qi HASOBJS

echo "1426_cli_hasobjs: PASS"
