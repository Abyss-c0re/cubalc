#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1432_fn_guard_native.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
FN add a b
  RET a + b
END
LET NEED_FNALL = "add\nMissing"
LET FN_GUARD_SOFT = 0
INCLUDE fn_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDFNS|FN_GUARD|missing'

OUT=$("$CUBALC" run -q programs/proof/1400_fn_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" which fn_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'fn_guard'

echo "1432_cli_fn_guard_native: PASS"
