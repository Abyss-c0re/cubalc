#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" hasform SORTLIBS 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.formgate.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"known":true'
printf '%s\n' "$OUT" | grep -q '"form":"SORTLIBS"'

OUT=$("$CUBALC" hasform NO_SUCH_FORM_XYZ_ZZZ 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -q '"known":false'

OUT=$("$CUBALC" needform LIBAGE 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" needform NO_SUCH_FORM_XYZ_ZZZ 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'hasform\|needform'

OUT=$("$CUBALC" forms HASFORM 2>&1)
printf '%s\n' "$OUT" | grep -qi 'HASFORM'

OUT=$("$CUBALC" run -q -e 'HASFORM SORTLIBS
ASSERT LAST_N == 1
REQUIRE FORM LIBAGE
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" run -q -e 'NEEDFORM NO_SUCH_FORM_XYZ_ZZZ
PASS' 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -qE '"ok":false|NEEDFORM|miss'
test "$RC" -ne 0

# HELP still works after catalog hoist
OUT=$("$CUBALC" run -q -e 'HELP SORTLIBS
ASSERT OK == 1
ASSERT HELP_N >= 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1323_cli_hasform: PASS"
