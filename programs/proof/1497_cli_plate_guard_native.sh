#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1497_plate_guard_native.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard ALL miss must fail-fast (NEEDP)
cat > "$TMPDIR/hard_all.cubalc" << 'C'
HOLD_FLASH 1
LET PLATE = "{\"n\":1}"
LET NEED_KEYS = "n\nMissingKey"
LET NEED_KEY_ANY = ""
LET PLATE_GUARD_SOFT = 0
INCLUDE plate_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_all.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDP|PLATE_GUARD|missing|MissingKey'

# hard ANY miss must fail-fast (NEEDPANY)
cat > "$TMPDIR/hard_any.cubalc" << 'C'
HOLD_FLASH 1
LET PLATE = "{\"n\":1}"
LET NEED_KEYS = ""
LET NEED_KEY_ANY = "MissingA\nMissingB"
LET PLATE_GUARD_SOFT = 0
INCLUDE plate_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_any.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDPANY|need one of|PLATE_GUARD|Missing'

# regression original plate_guard proof + CLI
OUT=$("$CUBALC" run -q programs/proof/1400_plate_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" which plate_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'plate_guard'

echo "1497_cli_plate_guard_native: PASS"
