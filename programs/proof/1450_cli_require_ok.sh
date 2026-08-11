#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1450_require_ok.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard fail: soft miss then REQUIRE OK without recovery
set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
TRYCALL no_such_fn_xyz_zzz
REQUIRE OK "plugin required"
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'REQUIRE OK|NEEDOK|OK=0|missing'

# NEEDOK hard path
set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
FAIL "boom"
NEEDOK
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDOK|REQUIRE OK|boom'

# forms
OUT=$("$CUBALC" forms NEEDOK 2>&1)
printf '%s\n' "$OUT" | grep -qi NEEDOK

# regression TRY/CATCH
OUT=$("$CUBALC" run -q programs/proof/1449_try_catch.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1450_cli_require_ok: PASS"
