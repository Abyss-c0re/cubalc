#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1410_trynew.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft continues past miss
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
TRYNEW Missing m1
ASSERT OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard NEW still fails process
cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
NEW Missing m1
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'CLASS|unknown|ok.:false|Missing'

OUT=$("$CUBALC" forms TRYNEW 2>&1)
printf '%s\n' "$OUT" | grep -qi TRYNEW

echo "1410_cli_trynew: PASS"
