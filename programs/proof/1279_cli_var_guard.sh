#!/bin/sh
# INCLUDE var_guard + doctor lib_var_guard
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'INCLUDE var_guard
ASSERT OK == 1
ASSERT VARROOM >= 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'var_guard.cubalc\|"includes_n":1'

# which/cat discover lib
OUT=$("$CUBALC" which var_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'var_guard'
OUT=$("$CUBALC" cat var_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'NEEDVARROOM\|NEED_VARROOM'

# doctor
OUT=$("$CUBALC" doctor)
printf '%s\n' "$OUT" | grep -q '"lib_var_guard":true'
printf '%s\n' "$OUT" | grep -q 'var_guard'
n=$(printf '%s\n' "$OUT" | sed -n 's/.*"libs_n":\([0-9][0-9]*\).*/\1/p' | head -1)
test -n "$n"
test "$n" -ge 17

# libs catalog
OUT=$("$CUBALC" libs 2>&1)
printf '%s\n' "$OUT" | grep -q 'var_guard'

echo "1279_cli_var_guard: PASS"
