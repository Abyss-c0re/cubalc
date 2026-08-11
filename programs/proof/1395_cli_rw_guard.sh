#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1395_rw_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft READ miss does not fatal
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_READ_PATHS = "/no/such/cubalc_rw_miss"
DEFAULT NEED_WRITE_PATHS = ""
DEFAULT RW_GUARD_SOFT = 1
INCLUDE rw_guard
ASSERT RW_GUARD_OK == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard READ miss fatal
set +e
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_READ_PATHS = "/no/such/cubalc_rw_hard"
DEFAULT RW_GUARD_SOFT = 0
INCLUDE rw_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'RW_GUARD READ|unreadable|ok.:false|no/such'

# hard WRITE miss fatal
set +e
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_READ_PATHS = ""
DEFAULT NEED_WRITE_PATHS = "/no/such/cubalc_rw_w"
DEFAULT RW_GUARD_SOFT = 0
INCLUDE rw_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'RW_GUARD WRITE|unwritable|ok.:false|no/such'

OUT=$("$CUBALC" which rw_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'rw_guard'

OUT=$("$CUBALC" recipe rw_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'rw_guard'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_rw_guard":true'

echo "1395_cli_rw_guard: PASS"
