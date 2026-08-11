#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1434_obj_guard_native.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard ALL miss must fail-fast (NEEDOBJS)
cat > "$TMPDIR/hard_all.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  FIELD id
  METHOD ping
    LET LAST = 1
  END
END
NEW Ticket t1
LET NEED_OBJS = "t1\nMissingObj"
LET NEED_OBJ_ANY = ""
LET OBJ_GUARD_SOFT = 0
INCLUDE obj_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_all.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDOBJS|OBJ_GUARD|missing|MissingObj'

# hard ANY miss must fail-fast (NEEDOBJANY)
cat > "$TMPDIR/hard_any.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  FIELD id
  METHOD ping
    LET LAST = 1
  END
END
NEW Ticket t1
LET NEED_OBJS = ""
LET NEED_OBJ_ANY = "MissingA\nMissingB"
LET OBJ_GUARD_SOFT = 0
INCLUDE obj_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_any.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDOBJANY|need one of|OBJ_GUARD|Missing'

# regression original + boot
OUT=$("$CUBALC" run -q programs/proof/1405_obj_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q programs/proof/1405_obj_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" which obj_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'obj_guard'

echo "1434_cli_obj_guard_native: PASS"
