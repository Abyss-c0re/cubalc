#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" formhint SORTLIBS 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.formhint.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"form":"SORTLIBS"'
printf '%s\n' "$OUT" | grep -qi 'hint'

OUT=$("$CUBALC" describeform LIBAGE 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'LIBAGE'

set +e
OUT=$("$CUBALC" formhint NO_SUCH_FORM_XYZ_ZZZ 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'formhint\|FORMHINT'

OUT=$("$CUBALC" forms FORMHINT 2>&1)
printf '%s\n' "$OUT" | grep -qi 'FORMHINT'

OUT=$("$CUBALC" run -q -e 'FORMHINT SORTLIBS
ASSERT OK == 1
ASSERT FORM_HINT != ""
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1328_cli_formhint: PASS"
