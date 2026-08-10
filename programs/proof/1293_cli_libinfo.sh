#!/bin/sh
# LIBINFO plate + forms + soft miss
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'LIBINFO fat_boot
ASSERT OK == 1
ASSERT LIB_DEPS_N == 2
ASSERT LIB_BYTES > 20
SYS HAS LAST "cubalc.libinfo.v1"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'LIBINFO missing_lib_qqq
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms libinfo 2>&1) || OUT=$("$CUBALC" forms LIBINFO 2>&1)
printf '%s\n' "$OUT" | grep -qi 'LIBINFO'

OUT=$("$CUBALC" run -q programs/proof/1293_libinfo.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1293_cli_libinfo: PASS"
