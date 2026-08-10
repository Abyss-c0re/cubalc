#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1367_cli_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "--out=z"
SYS ENV SET CUBALC_ARGC "1"
DEFAULT NEED_FLAGS = "out"
DEFAULT NEED_REST_N = 0
DEFAULT CLI_GUARD_SOFT = 0
INCLUDE cli_guard
ASSERT CLI_GUARD_OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss does not fatal
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARGC "0"
DEFAULT NEED_FLAGS = "missing"
DEFAULT CLI_GUARD_SOFT = 1
INCLUDE cli_guard
ASSERT CLI_GUARD_OK == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard miss fatal
set +e
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARGC "0"
DEFAULT NEED_FLAGS = "missing"
DEFAULT CLI_GUARD_SOFT = 0
INCLUDE cli_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDFLAGS|missing|ok.:false'

OUT=$("$CUBALC" which cli_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'cli_guard'

OUT=$("$CUBALC" recipe cli_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'cli_guard'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_cli_guard":true'

OUT=$("$CUBALC" libs cli_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'cli_guard'

echo "1367_cli_cli_guard: PASS"
