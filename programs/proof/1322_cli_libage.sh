#!/bin/sh
# cubalc libage|hasfresh|needfresh — CLI dual of LIBAGE/HASFRESH/NEEDFRESH
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" libage fat_session 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.libmatch.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"mode":"age"'
printf '%s\n' "$OUT" | grep -q '"stem":"fat_session"'
printf '%s\n' "$OUT" | grep -qE '"age":[0-9]+'
printf '%s\n' "$OUT" | grep -q 'fat_session.cubalc'

OUT=$("$CUBALC" hasfresh fat_session 1000000 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"fresh":true'
printf '%s\n' "$OUT" | grep -q '"max_age":1000000'

OUT=$("$CUBALC" hasfresh fat_session 3 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -q '"fresh":false'

OUT=$("$CUBALC" needfresh fat_session 1000000 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" needfresh fat_session 3 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

set +e
OUT=$("$CUBALC" libage no_such_lib_xyz_zzz 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'libage\|hasfresh\|needfresh'

OUT=$("$CUBALC" forms LIBAGE 2>&1)
printf '%s\n' "$OUT" | grep -qi 'LIBAGE'
OUT=$("$CUBALC" forms HASFRESH 2>&1)
printf '%s\n' "$OUT" | grep -qi 'HASFRESH'

OUT=$("$CUBALC" run -q -e 'LIBAGE fat_session
ASSERT LIB_AGE > 0
HASFRESH fat_session 1000000
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDFRESH fail in-lang
set +e
OUT=$("$CUBALC" run -q -e 'NEEDFRESH fat_session 1
PASS' 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false\|NEEDFRESH\|stale'
test "$RC" -ne 0

echo "1322_cli_libage: PASS"
