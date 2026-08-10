#!/bin/sh
# cubalc newestlib|oldestlib — CLI dual of NEWESTLIB/OLDESTLIB
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" newestlib fat 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.libmatch.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"stem":"fat_session"'
printf '%s\n' "$OUT" | grep -qE '"mtime":[1-9]'
printf '%s\n' "$OUT" | grep -q 'fat_session.cubalc'

OUT=$("$CUBALC" oldestlib fat 2>&1)
printf '%s\n' "$OUT" | grep -q '"stem":"fat_boot"'
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" latestlib guard 2>&1)
printf '%s\n' "$OUT" | grep -q '"stem":"time_guard"'

set +e
OUT=$("$CUBALC" newestlib no_such_lib_xyz_zzz 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'newestlib\|oldestlib\|picklib'

# in-lang dual still green
OUT=$("$CUBALC" run -q -e 'NEWESTLIB fat
ASSERT LAST == "fat_session"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1319_cli_newestlib_cmd: PASS"
