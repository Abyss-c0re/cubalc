#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1428_method_guard_native.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard fail still works
cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
CLASS A
  METHOD m
  END
END
LET NEED_METHODS = "A.m\nA.missing"
LET METHOD_GUARD_SOFT = 0
INCLUDE method_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qi METHOD_GUARD

# regression: prior method_guard proof
OUT=$("$CUBALC" run -q programs/proof/1402_method_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1428_cli_method_guard_native: PASS"
