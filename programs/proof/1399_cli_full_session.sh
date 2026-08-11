#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1399_full_session.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_PATHS = "nosuch_cubalc_fs_miss"
DEFAULT PATH_GUARD_SOFT = 1
DEFAULT NEED_FLAGS = ""
DEFAULT NEED_REST_N = 0
DEFAULT NEED_ENVS = ""
DEFAULT NEED_TIME = 1
DEFAULT NEED_READ_PATHS = ""
DEFAULT NEED_WRITE_PATHS = ""
DEFAULT NEED_BINS = ""
DEFAULT NEED_LIBS = ""
INCLUDE full_session
ASSERT FULL_SESSION_OK == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_PATHS = "nosuch_cubalc_fs_hard"
DEFAULT PATH_GUARD_SOFT = 0
INCLUDE full_session
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'PATH_GUARD|missing|ok.:false|nosuch'

OUT=$("$CUBALC" which full_session 2>&1)
printf '%s\n' "$OUT" | grep -qi 'full_session'

OUT=$("$CUBALC" recipe full_session 2>&1)
printf '%s\n' "$OUT" | grep -q 'full_session'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_full_session":true'

echo "1399_cli_full_session: PASS"
