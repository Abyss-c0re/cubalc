#!/bin/sh
# CHECKDEPS / HASDEPS / NEEDDEPS forms + soft miss + hard fail
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'HASDEPS fat_session
ASSERT OK == 1
ASSERT DEPS_MISS_N == 0
ASSERT DEPS_N >= 3
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'CHECKDEPS fat_session
ASSERT OK == 1
ASSERT LAST_N == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'NEEDDEPS fat_session
ASSERT OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss
OUT=$("$CUBALC" run -q -e 'HASDEPS missing_lib_qqq
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDDEPS hard fail
OUT=$("$CUBALC" run -q -e 'NEEDDEPS no_such_lib_xyz_zzz
PASS' 2>&1) || true
printf '%s\n' "$OUT" | grep -qi 'NEEDDEPS missing\|ok.:false\|"ok":false'

# forms
OUT=$("$CUBALC" forms checkdeps 2>&1) || OUT=$("$CUBALC" forms CHECKDEPS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'CHECKDEPS'

OUT=$("$CUBALC" forms needdeps 2>&1) || OUT=$("$CUBALC" forms NEEDDEPS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'NEEDDEPS'

# help surface
OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -q 'CHECKDEPS\|NEEDDEPS'

OUT=$("$CUBALC" run -q programs/proof/1302_checkdeps.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1302_cli_checkdeps: PASS"
