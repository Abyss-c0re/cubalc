#!/bin/sh
# GREPLIB / SEARCHLIBS forms + soft miss
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'GREPLIB fat_boot "var_guard"
ASSERT OK == 1
ASSERT LAST_N >= 1
ASSERT LIB_STEM == "fat_boot"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SEARCHLIBS "plate_boot"
ASSERT OK == 1
ASSERT LAST_N >= 1
SYS HASLINE LAST "fat_session"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'GREPLIB missing_lib_qqq "x"
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms greplib 2>&1) || OUT=$("$CUBALC" forms GREPLIB 2>&1)
printf '%s\n' "$OUT" | grep -qi 'GREPLIB'

OUT=$("$CUBALC" forms searchlibs 2>&1) || OUT=$("$CUBALC" forms SEARCHLIBS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'SEARCHLIBS'

OUT=$("$CUBALC" run -q programs/proof/1290_greplib.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1290_cli_greplib: PASS"
