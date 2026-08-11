#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1435_class_guard_multi_native.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard ALL miss (NEEDCLASSES)
cat > "$TMPDIR/hard_all.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  FIELD id
END
LET NEED_CLASSES = "Ticket\nMissingClass"
LET NEED_CLASS_ANY = ""
LET CLASS_GUARD_SOFT = 0
INCLUDE class_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_all.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDCLASSES|CLASS_GUARD|missing|MissingClass'

# hard ANY miss (NEEDCLASSANY)
cat > "$TMPDIR/hard_any.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  FIELD id
END
LET NEED_CLASSES = ""
LET NEED_CLASS_ANY = "MissingA\nMissingB"
LET CLASS_GUARD_SOFT = 0
INCLUDE class_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_any.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDCLASSANY|need one of|CLASS_GUARD|Missing'

# regression
OUT=$("$CUBALC" run -q programs/proof/1401_class_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q programs/proof/1431_class_guard_native.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" which class_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'class_guard'

echo "1435_cli_class_guard_multi_native: PASS"
