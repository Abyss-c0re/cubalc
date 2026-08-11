#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1432_hasfnany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
HASFNANY ghost nope
ASSERT LAST_N == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
NEEDFNANY ghost spectre
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qi NEEDFNANY

cat > "$TMPDIR/ok.cubalc" << 'C'
HOLD_FLASH 1
FN add a b
  RET a + b
END
NEEDFNANY ghost add
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/ok.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms HASFNANY 2>&1)
printf '%s\n' "$OUT" | grep -qi HASFNANY

echo "1432_cli_hasfnany: PASS"
