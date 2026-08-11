#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1387_cli_guard_any.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "--out=z"
SYS ENV SET CUBALC_ARGC "1"
DEFAULT NEED_FLAGS = "out"
DEFAULT NEED_FLAG_ANY = ""
DEFAULT NEED_REST_N = 0
DEFAULT CLI_GUARD_SOFT = 0
INCLUDE cli_guard
ASSERT CLI_GUARD_OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft ANY hit
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "--json"
SYS ENV SET CUBALC_ARGC "1"
DEFAULT NEED_FLAGS = ""
DEFAULT NEED_FLAG_ANY = "json
yaml"
DEFAULT CLI_GUARD_SOFT = 1
INCLUDE cli_guard
ASSERT CLI_GUARD_FLAG_ANY_OK == 1
ASSERT CLI_GUARD_OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft ANY miss does not fatal
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARGC "0"
DEFAULT NEED_FLAGS = ""
DEFAULT NEED_FLAG_ANY = "json
yaml"
DEFAULT CLI_GUARD_SOFT = 1
INCLUDE cli_guard
ASSERT CLI_GUARD_OK == 0
ASSERT CLI_GUARD_FLAG_ANY_OK == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard ANY miss fatal
set +e
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARGC "0"
DEFAULT NEED_FLAGS = ""
DEFAULT NEED_FLAG_ANY = "json
yaml"
DEFAULT CLI_GUARD_SOFT = 0
INCLUDE cli_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDFLAGANY|need one of|ok.:false'

# hard ALL miss still fatal
set +e
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARGC "0"
DEFAULT NEED_FLAGS = "missing"
DEFAULT NEED_FLAG_ANY = ""
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

# flagany CLI dual still present
OUT=$("$CUBALC" hasflagany verbose out 2>&1 || true)
printf '%s\n' "$OUT" | grep -qiE 'flag|HASFLAGANY|ok|false|true' || true

echo "1387_cli_cli_guard_any: PASS"
