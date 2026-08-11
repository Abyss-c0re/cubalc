#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1394_path_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft ALL miss does not fatal
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_PATHS = "/no/such/cubalc_pg_miss"
DEFAULT NEED_PATH_ANY = ""
DEFAULT PATH_GUARD_SOFT = 1
INCLUDE path_guard
ASSERT PATH_GUARD_OK == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard ALL miss fatal
set +e
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_PATHS = "/no/such/cubalc_pg_hard"
DEFAULT PATH_GUARD_SOFT = 0
INCLUDE path_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'REQUIRE PATH|no/such|ok.:false|PATH'

# hard ANY miss fatal
set +e
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_PATHS = ""
DEFAULT NEED_PATH_ANY = "/no/such/cubalc_a\n/no/such/cubalc_b"
DEFAULT PATH_GUARD_SOFT = 0
INCLUDE path_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'PATH_GUARD ANY|need one of|ok.:false'

OUT=$("$CUBALC" which path_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'path_guard'

OUT=$("$CUBALC" recipe path_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'path_guard'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_path_guard":true'

echo "1394_cli_path_guard: PASS"
