#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1385_hasformany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'HASFORMANY HASFORM NO_SUCH_X
ASSERT LAST_N == 1
ASSERT HASFORMANY_N == 1
HASFORMANY NO_SUCH_A NO_SUCH_B
ASSERT LAST_N == 0
ASSERT FORMMISS_N == 2
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDFORMANY fatal when none known
OUT=$("$CUBALC" run -q -e 'NEEDFORMANY NO_SUCH_A NO_SUCH_B
PASS' 2>&1 || true)
printf '%s\n' "$OUT" | grep -qiE 'NEEDFORMANY miss|need one of|ok.:false'

OUT=$("$CUBALC" run -q -e 'NEEDFORMANY HASFORM NO_SUCH_X
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" hasformany HASFORM NO_SUCH_FORM_XYZ 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.formgate.v1'
printf '%s\n' "$OUT" | grep -q '"any":true'
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" hasformany NO_SUCH_A NO_SUCH_B 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":false'

set +e
OUT=$("$CUBALC" needformany NO_SUCH_A NO_SUCH_B 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -q '"mode":"need"'

OUT=$("$CUBALC" needformany HASFORM NO_SUCH_X 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" related HASFORMANY 2>&1)
printf '%s\n' "$OUT" | grep -qiE 'NEEDFORMANY|HASFORMS|FORMHAVE'

OUT=$("$CUBALC" forms HASFORMANY 2>&1)
printf '%s\n' "$OUT" | grep -qi 'HASFORMANY'

OUT=$("$CUBALC" run -q -e 'LET NEED_FORMS = ""
LET NEED_FORM_ANY = "HASFORM\nNO_SUCH_X"
LET FORM_GUARD_SOFT = 1
INCLUDE form_guard
ASSERT FORM_GUARD_ANY_OK == 1
ASSERT FORM_GUARD_OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_form_guard":true'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'hasformany\|needformany'

echo "1385_cli_hasformany: PASS"
