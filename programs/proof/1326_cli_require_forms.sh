#!/bin/sh
# cubalc run -C / CUBALC_REQUIRE_FORMS — host form capability floor
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -C SORTLIBS,LIBAGE -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"require_forms":"SORTLIBS,LIBAGE"'

OUT=$("$CUBALC" run -q --need-forms "HASFORM NEEDFORM" -e 'PRINT \"ok\"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" run -q -C SORTLIBS,NO_SUCH_FORM_XYZ_ZZZ -e 'PASS' 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -qi 'REQUIRE FORMS\|missing\|NEEDFORMS'
test "$RC" -ne 0

set +e
OUT=$(CUBALC_REQUIRE_FORMS=NO_SUCH_FORM_XYZ_ZZZ "$CUBALC" run -q -e 'PASS' 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

# env ok path
OUT=$(CUBALC_REQUIRE_FORMS=HASFORM,SORTLIBS "$CUBALC" run -q -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'HASFORM'

OUT=$("$CUBALC" env CUBALC_REQUIRE_FORMS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'REQUIRE_FORMS\|form'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '-C\|need-forms\|form floor'

# dual of in-lang NEEDFORMS
OUT=$("$CUBALC" run -q -e 'NEEDFORMS SORTLIBS LIBAGE
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1326_cli_require_forms: PASS"
