#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1438_callany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard CALLANY miss
cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
CALLANY ghost_a ghost_b WITH 1
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'CALLANY|none of|ghost'

# TRYCALLANY soft still ok plate
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
TRYCALLANY ghost_a ghost_b
ASSERT OK == 0
ASSERT CALLED == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# forms discoverable
OUT=$("$CUBALC" forms CALLANY 2>&1)
printf '%s\n' "$OUT" | grep -qi 'CALLANY'

OUT=$("$CUBALC" forms TRYCALLANY 2>&1)
printf '%s\n' "$OUT" | grep -qi 'TRYCALLANY'

echo "1438_cli_callany: PASS"
