#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1404_oop_session.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  FIELD id
  METHOD ping
    LET LAST = 1
  END
END
DEFAULT NEED_CLASSES = "nosuch_oop_soft"
DEFAULT NEED_METHODS = ""
DEFAULT NEED_FIELDS = ""
DEFAULT CLASS_GUARD_SOFT = 1
DEFAULT METHOD_GUARD_SOFT = 1
DEFAULT FIELD_GUARD_SOFT = 1
INCLUDE oop_session
ASSERT OOP_SESSION_OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
DEFAULT NEED_CLASSES = "nosuch_oop_hard"
DEFAULT CLASS_GUARD_SOFT = 0
INCLUDE oop_session
PASS
C
set +e
OUT=$("$CUBALC" run -s -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'CLASS_GUARD|hard miss|ok.:false|nosuch'

OUT=$("$CUBALC" which oop_session 2>&1)
printf '%s\n' "$OUT" | grep -qi 'oop_session'

OUT=$("$CUBALC" recipe oop_session 2>&1)
printf '%s\n' "$OUT" | grep -q 'oop_session'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_oop_session":true'

echo "1404_cli_oop_session: PASS"
