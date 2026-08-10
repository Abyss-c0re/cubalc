#!/bin/sh
# cubalc checkdeps|hasdeps|needdeps — CLI dual of CHECKDEPS install gate
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" checkdeps fat_session 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.checkdeps.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"stem":"fat_session"'
printf '%s\n' "$OUT" | grep -q 'fat_boot'
printf '%s\n' "$OUT" | grep -q 'plate_boot'
printf '%s\n' "$OUT" | grep -q '"miss_n":0'
printf '%s\n' "$OUT" | grep -qE '"deps_n":[3-9]'

OUT=$("$CUBALC" hasdeps agent_boot 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"deps_n":1'

# missing lib → ok:false + exit 1
set +e
OUT=$("$CUBALC" checkdeps no_such_lib_xyz_zzz 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -q '"miss_n":'
test "$RC" -ne 0

set +e
OUT=$("$CUBALC" needdeps no_such_lib_xyz_zzz 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

# usage
set +e
OUT=$("$CUBALC" checkdeps 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -qi 'usage\|need libname'
test "$RC" -ne 0

# help surfaces checkdeps
OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -q 'checkdeps'

# doctor mentions checkdeps
OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -qi 'checkdeps'

# in-lang dual still green
OUT=$("$CUBALC" run -q -e 'HASDEPS fat_session
ASSERT OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1303_cli_checkdeps_cmd: PASS"
