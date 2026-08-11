#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1398_host_session.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_PATHS = "nosuch_cubalc_hs_miss"
DEFAULT PATH_GUARD_SOFT = 1
DEFAULT NEED_READ_PATHS = ""
DEFAULT NEED_WRITE_PATHS = ""
DEFAULT RW_GUARD_SOFT = 0
DEFAULT NEED_BINS = ""
DEFAULT BIN_GUARD_SOFT = 0
DEFAULT NEED_LIBS = ""
DEFAULT LIB_GUARD_SOFT = 0
INCLUDE host_session
ASSERT HOST_SESSION_OK == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_PATHS = "nosuch_cubalc_hs_hard"
DEFAULT PATH_GUARD_SOFT = 0
INCLUDE host_session
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'PATH_GUARD|missing|ok.:false|nosuch'

OUT=$("$CUBALC" which host_session 2>&1)
printf '%s\n' "$OUT" | grep -qi 'host_session'

OUT=$("$CUBALC" recipe host_session 2>&1)
printf '%s\n' "$OUT" | grep -q 'host_session'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_host_session":true'

echo "1398_cli_host_session: PASS"
