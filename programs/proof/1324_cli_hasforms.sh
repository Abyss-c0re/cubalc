#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" hasforms SORTLIBS LIBAGE 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.formgate.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":2'
printf '%s\n' "$OUT" | grep -q 'SORTLIBS'

OUT=$("$CUBALC" hasforms SORTLIBS NO_SUCH_FORM_XYZ_ZZZ 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":false'

OUT=$("$CUBALC" needforms HASFORM LIBAGE 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" needforms HASFORM NO_SUCH_FORM_XYZ_ZZZ 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_form_guard":true'
printf '%s\n' "$OUT" | grep -qi 'form_guard'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'hasforms\|needforms'

OUT=$("$CUBALC" run -q -e 'HASFORMS SORTLIBS LIBAGE
ASSERT LAST_N == 1
INCLUDE form_guard
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms HASFORMS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'HASFORMS'

echo "1324_cli_hasforms: PASS"
