#!/bin/sh
# HEADLIB / TAILLIB forms + soft miss
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'HEADLIB fat_boot 4
ASSERT OK == 1
ASSERT LAST_N == 4
ASSERT LIB_STEM == "fat_boot"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'TAILLIB var_guard 2
ASSERT OK == 1
ASSERT LAST_N == 2
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'HEADLIB missing_lib_qqq
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms headlib 2>&1) || OUT=$("$CUBALC" forms HEADLIB 2>&1)
printf '%s\n' "$OUT" | grep -qi 'HEADLIB'

OUT=$("$CUBALC" forms taillib 2>&1) || OUT=$("$CUBALC" forms TAILLIB 2>&1)
printf '%s\n' "$OUT" | grep -qi 'TAILLIB'

OUT=$("$CUBALC" run -q programs/proof/1291_headlib.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1291_cli_headlib: PASS"
