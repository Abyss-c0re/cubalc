#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1397_lib_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_LIBS = "nosuch_cubalc_lg_miss"
DEFAULT NEED_LIB_ANY = ""
DEFAULT LIB_GUARD_SOFT = 1
INCLUDE lib_guard
ASSERT LIB_GUARD_OK == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_LIBS = "nosuch_cubalc_lg_hard"
DEFAULT LIB_GUARD_SOFT = 0
INCLUDE lib_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'REQUIRE LIB|LIB_GUARD|missing|ok.:false|nosuch'

set +e
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_LIBS = ""
DEFAULT NEED_LIB_ANY = "nosuch_a\nnosuch_b"
DEFAULT LIB_GUARD_SOFT = 0
INCLUDE lib_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'LIB_GUARD ANY|need one of|ok.:false'

OUT=$("$CUBALC" which lib_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'lib_guard'

OUT=$("$CUBALC" recipe lib_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'lib_guard'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_lib_guard":true'

echo "1397_cli_lib_guard: PASS"
