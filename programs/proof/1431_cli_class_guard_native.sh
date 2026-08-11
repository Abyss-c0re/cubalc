#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1431_class_guard_native.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
CLASS A
END
LET NEED_CLASSES = "A\nMissing"
LET CLASS_GUARD_SOFT = 0
INCLUDE class_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qi CLASS_GUARD

OUT=$("$CUBALC" run -q programs/proof/1401_class_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1431_cli_class_guard_native: PASS"
