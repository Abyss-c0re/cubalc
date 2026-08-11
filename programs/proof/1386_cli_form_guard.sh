#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1386_form_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_FORMS = "HASFORM\nNEEDFORM"
DEFAULT NEED_FORM_ANY = ""
DEFAULT FORM_GUARD_SOFT = 0
INCLUDE form_guard
ASSERT FORM_GUARD_OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss does not fatal
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_FORMS = "NO_SUCH_FORM_MISS_ZZZ"
DEFAULT FORM_GUARD_SOFT = 1
INCLUDE form_guard
ASSERT FORM_GUARD_OK == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard miss fatal
set +e
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_FORMS = "NO_SUCH_FORM_MISS_ZZZ"
DEFAULT FORM_GUARD_SOFT = 0
INCLUDE form_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDFORMS|NO_SUCH_FORM|ok.:false'

# hard ANY miss fatal
set +e
OUT=$("$CUBALC" run -q -e 'DEFAULT NEED_FORMS = ""
DEFAULT NEED_FORM_ANY = "NO_SUCH_FORM_X_ZZZ\nNO_SUCH_FORM_Y_ZZZ"
DEFAULT FORM_GUARD_SOFT = 0
INCLUDE form_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDFORMANY|need one of|ok.:false'

OUT=$("$CUBALC" which form_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'form_guard'

OUT=$("$CUBALC" recipe form_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'form_guard'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_form_guard":true'

OUT=$("$CUBALC" libs form_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'form_guard'

# formgate CLI duals still present
OUT=$("$CUBALC" hasformany HASFORM NO_SUCH_FORM 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'formgate'

echo "1386_cli_form_guard: PASS"
