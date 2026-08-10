#!/bin/sh
# LIBTREE forms + soft miss
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'LIBTREE fat_session
ASSERT OK == 1
ASSERT LAST_N >= 4
SYS HASLINE LAST "var_guard"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'LIBTREE missing_lib_qqq
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms libtree 2>&1) || OUT=$("$CUBALC" forms LIBTREE 2>&1)
printf '%s\n' "$OUT" | grep -qi 'LIBTREE'

OUT=$("$CUBALC" run -q programs/proof/1294_libtree.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1294_cli_libtree: PASS"
