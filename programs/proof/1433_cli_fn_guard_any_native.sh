#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1433_fn_guard_any_native.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard ANY miss must fail-fast (NEEDFNANY)
cat > "$TMPDIR/hard_any.cubalc" << 'C'
HOLD_FLASH 1
FN add a b
  RET a + b
END
LET NEED_FNALL = ""
LET NEED_FNANY = "MissingA\nMissingB"
LET FN_GUARD_SOFT = 0
INCLUDE fn_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_any.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDFNANY|FN_GUARD|need one of|HASFNANY'

# regression: original fn_guard proof + native ALL proof
OUT=$("$CUBALC" run -q programs/proof/1400_fn_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q programs/proof/1432_fn_guard_native.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" which fn_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'fn_guard'

echo "1433_cli_fn_guard_any_native: PASS"
