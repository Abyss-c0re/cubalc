#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1396_bin_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_BINS = "nosuch_cubalc_bg_miss"
DEFAULT NEED_BIN_ANY = ""
DEFAULT BIN_GUARD_SOFT = 1
INCLUDE bin_guard
ASSERT BIN_GUARD_OK == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_BINS = "nosuch_cubalc_bg_hard"
DEFAULT BIN_GUARD_SOFT = 0
INCLUDE bin_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'REQUIRE BIN|BIN_GUARD|missing|ok.:false|nosuch'

set +e
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_BINS = ""
DEFAULT NEED_BIN_ANY = "nosuch_a\nnosuch_b"
DEFAULT BIN_GUARD_SOFT = 0
INCLUDE bin_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'BIN_GUARD ANY|need one of|ok.:false'

OUT=$("$CUBALC" which bin_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'bin_guard'

OUT=$("$CUBALC" recipe bin_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'bin_guard'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_bin_guard":true'

echo "1396_cli_bin_guard: PASS"
