#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1405_obj_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  FIELD id
  METHOD ping
    LET LAST = 1
  END
END
NEW Ticket t1
DEFAULT NEED_OBJS = "nosuch_cubalc_og_miss"
DEFAULT NEED_OBJ_ANY = ""
DEFAULT OBJ_GUARD_SOFT = 1
INCLUDE obj_guard
ASSERT OBJ_GUARD_OK == 0
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
NEW Ticket t1
DEFAULT NEED_OBJS = "nosuch_cubalc_og_hard"
DEFAULT OBJ_GUARD_SOFT = 0
INCLUDE obj_guard
PASS
C
set +e
OUT=$("$CUBALC" run -s -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'OBJ_GUARD|missing|ok.:false|nosuch'

cat > "$TMPDIR/any.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  FIELD id
  METHOD ping
    LET LAST = 1
  END
END
NEW Ticket t1
DEFAULT NEED_OBJS = ""
DEFAULT NEED_OBJ_ANY = "nosuch_a\nnosuch_b"
DEFAULT OBJ_GUARD_SOFT = 0
INCLUDE obj_guard
PASS
C
set +e
OUT=$("$CUBALC" run -s -q "$TMPDIR/any.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'OBJ_GUARD ANY|need one of|ok.:false'

OUT=$("$CUBALC" which obj_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'obj_guard'

OUT=$("$CUBALC" recipe obj_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'obj_guard'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_obj_guard":true'

echo "1405_cli_obj_guard: PASS"
