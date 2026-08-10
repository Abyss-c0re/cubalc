#!/bin/sh
# LISTLIBS / HASLIB CLI smoke + forms
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'LISTLIBS
ASSERT LAST_N >= 15
HASLIB fat_boot
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'HASLIB missing_lib_zzz
ASSERT LAST_N == 0
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms listlib 2>&1) || OUT=$("$CUBALC" forms lib 2>&1)
printf '%s\n' "$OUT" | grep -qi 'LISTLIBS\|HASLIB'

OUT=$("$CUBALC" run -q programs/proof/1288_listlibs.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1288_cli_listlibs: PASS"
