#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1430_field_guard_native.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
CLASS A
  FIELD x
END
LET NEED_FIELDS = "A.x\nA.missing"
LET FIELD_GUARD_SOFT = 0
INCLUDE field_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qi FIELD_GUARD

OUT=$("$CUBALC" run -q programs/proof/1403_field_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1430_cli_field_guard_native: PASS"
