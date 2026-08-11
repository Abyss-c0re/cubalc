#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1409_service_session.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  FIELD id
END
NEW Ticket t1
DEFAULT NEED_FLAGS = ""
DEFAULT NEED_REST_N = 0
DEFAULT NEED_TIME = 1
DEFAULT NEED_CLASSES = ""
DEFAULT NEED_OBJS = "nosuch_svc_soft"
DEFAULT OBJ_GUARD_SOFT = 1
DEFAULT CLASS_GUARD_SOFT = 1
DEFAULT METHOD_GUARD_SOFT = 1
DEFAULT FIELD_GUARD_SOFT = 1
DEFAULT CLI_GUARD_SOFT = 1
DEFAULT ARG_GUARD_SOFT = 1
DEFAULT ENV_GUARD_SOFT = 1
DEFAULT TIME_GUARD_SOFT = 1
DEFAULT PATH_GUARD_SOFT = 1
DEFAULT RW_GUARD_SOFT = 1
DEFAULT BIN_GUARD_SOFT = 1
DEFAULT LIB_GUARD_SOFT = 1
INCLUDE service_session
ASSERT SERVICE_SESSION_OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
DEFAULT NEED_OBJS = "nosuch_svc_hard"
DEFAULT OBJ_GUARD_SOFT = 0
INCLUDE service_session
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'OBJ_GUARD|hard miss|ok.:false|nosuch'

OUT=$("$CUBALC" which service_session 2>&1)
printf '%s\n' "$OUT" | grep -qi 'service_session'

OUT=$("$CUBALC" recipe service_session 2>&1)
printf '%s\n' "$OUT" | grep -q 'service_session'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_service_session":true'

echo "1409_cli_service_session: PASS"
