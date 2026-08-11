#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1388_arg_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "a"
SYS ENV SET CUBALC_ARG1 "b"
SYS ENV SET CUBALC_ARGC "2"
DEFAULT NEED_ARGS = "0\n1"
DEFAULT NEED_ARG_ANY = ""
DEFAULT ARG_GUARD_SOFT = 0
INCLUDE arg_guard
ASSERT ARG_GUARD_OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss does not fatal
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARGC "0"
DEFAULT NEED_ARGS = "0"
DEFAULT ARG_GUARD_SOFT = 1
INCLUDE arg_guard
ASSERT ARG_GUARD_OK == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard miss fatal
set +e
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARGC "0"
DEFAULT NEED_ARGS = "0"
DEFAULT ARG_GUARD_SOFT = 0
INCLUDE arg_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDARGS|miss|ok.:false'

# hard ANY miss fatal
set +e
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARGC "0"
DEFAULT NEED_ARGS = ""
DEFAULT NEED_ARG_ANY = "0\n1"
DEFAULT ARG_GUARD_SOFT = 0
INCLUDE arg_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDARGANY|need one of|ok.:false'

OUT=$("$CUBALC" which arg_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'arg_guard'

OUT=$("$CUBALC" recipe arg_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'arg_guard'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_arg_guard":true'

OUT=$("$CUBALC" libs arg_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'arg_guard'

# compose with live argv
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_ARGS = "0"
DEFAULT ARG_GUARD_SOFT = 0
INCLUDE arg_guard
ASSERT ARG_GUARD_OK == 1
PASS' -- file0 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1388_cli_arg_guard: PASS"
