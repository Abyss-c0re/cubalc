#!/bin/sh
# LIBDEPS forms + composition + soft miss
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'LIBDEPS fat_session
ASSERT OK == 1
ASSERT LAST_N == 2
SYS HASLINE LAST "fat_boot"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'LIBDEPS missing_lib_qqq
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms libdeps 2>&1) || OUT=$("$CUBALC" forms LIBDEPS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'LIBDEPS'

OUT=$("$CUBALC" run -q programs/proof/1292_libdeps.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1292_cli_libdeps: PASS"
