#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1406_live_session.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  FIELD id
END
NEW Ticket t1
DEFAULT NEED_CLASSES = ""
DEFAULT NEED_OBJS = "nosuch_live_soft"
DEFAULT OBJ_GUARD_SOFT = 1
DEFAULT CLASS_GUARD_SOFT = 1
DEFAULT METHOD_GUARD_SOFT = 1
DEFAULT FIELD_GUARD_SOFT = 1
INCLUDE live_session
ASSERT LIVE_SESSION_OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
DEFAULT NEED_OBJS = "nosuch_live_hard"
DEFAULT OBJ_GUARD_SOFT = 0
INCLUDE live_session
PASS
C
set +e
OUT=$("$CUBALC" run -s -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'OBJ_GUARD|hard miss|ok.:false|nosuch'

OUT=$("$CUBALC" which live_session 2>&1)
printf '%s\n' "$OUT" | grep -qi 'live_session'

OUT=$("$CUBALC" recipe live_session 2>&1)
printf '%s\n' "$OUT" | grep -q 'live_session'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_live_session":true'

echo "1406_cli_live_session: PASS"
