#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1400_fn_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_FNALL = "nosuch_cubalc_fg_miss"
DEFAULT NEED_FNANY = ""
DEFAULT FN_GUARD_SOFT = 1
INCLUDE fn_guard
ASSERT FN_GUARD_OK == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_FNALL = "nosuch_cubalc_fg_hard"
DEFAULT FN_GUARD_SOFT = 0
INCLUDE fn_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'REQUIRE FN|FN_GUARD|missing|ok.:false|nosuch'

set +e
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_FNALL = ""
DEFAULT NEED_FNANY = "nosuch_a\nnosuch_b"
DEFAULT FN_GUARD_SOFT = 0
INCLUDE fn_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'FN_GUARD ANY|need one of|ok.:false'

OUT=$("$CUBALC" which fn_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'fn_guard'

OUT=$("$CUBALC" recipe fn_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'fn_guard'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_fn_guard":true'

echo "1400_cli_fn_guard: PASS"
