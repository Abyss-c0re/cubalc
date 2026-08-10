#!/bin/sh
# LIBDEFAULTS forms + knobs + soft miss
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'LIBDEFAULTS var_guard
ASSERT OK == 1
ASSERT LAST_N == 2
SYS HASLINE LAST "NEED_VARROOM=32"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'LIBDEFAULTS missing_lib_qqq
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms libdefaults 2>&1) || OUT=$("$CUBALC" forms LIBDEFAULTS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'LIBDEFAULTS'

OUT=$("$CUBALC" run -q programs/proof/1295_libdefaults.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1295_cli_libdefaults: PASS"
