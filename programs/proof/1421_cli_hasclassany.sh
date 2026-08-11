#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1421_hasclassany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft any miss continues
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
HASCLASSANY Ghost Nope
ASSERT LAST_N == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDCLASSANY hard when none
cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
NEEDCLASSANY Ghost Spectre
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qi NEEDCLASSANY

# NEEDCLASSANY ok if one present
cat > "$TMPDIR/ok.cubalc" << 'C'
HOLD_FLASH 1
CLASS A
END
NEEDCLASSANY Ghost A
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/ok.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms HASCLASSANY 2>&1)
printf '%s\n' "$OUT" | grep -qi HASCLASSANY

echo "1421_cli_hasclassany: PASS"
