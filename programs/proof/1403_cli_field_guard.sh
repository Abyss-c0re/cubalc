#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1403_field_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  FIELD id
  METHOD ping
    LET LAST = 1
  END
END
DEFAULT NEED_FIELDS = "nosuch_cubalc_fg_miss"
DEFAULT NEED_FIELD_ANY = ""
DEFAULT FIELD_GUARD_SOFT = 1
INCLUDE field_guard
ASSERT FIELD_GUARD_OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  FIELD id
  METHOD ping
    LET LAST = 1
  END
END
DEFAULT NEED_FIELDS = "Ticket.nosuch_cubalc_fg_hard"
DEFAULT FIELD_GUARD_SOFT = 0
INCLUDE field_guard
PASS
C
set +e
OUT=$("$CUBALC" run -s -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'FIELD_GUARD|missing|ok.:false|nosuch'

cat > "$TMPDIR/any.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  FIELD id
  METHOD ping
    LET LAST = 1
  END
END
DEFAULT NEED_FIELDS = ""
DEFAULT NEED_FIELD_ANY = "nosuch_a\nnosuch_b"
DEFAULT FIELD_GUARD_SOFT = 0
INCLUDE field_guard
PASS
C
set +e
OUT=$("$CUBALC" run -s -q "$TMPDIR/any.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'FIELD_GUARD ANY|need one of|ok.:false'

OUT=$("$CUBALC" which field_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'field_guard'

OUT=$("$CUBALC" recipe field_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'field_guard'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_field_guard":true'

echo "1403_cli_field_guard: PASS"
