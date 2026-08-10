#!/bin/sh
# HASMATCHLIBS soft + NEEDMATCHLIBS hard fail
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'HASMATCHLIBS plate
ASSERT OK == 1
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'NEEDMATCHLIBS fat
ASSERT OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'HASMATCHLIBS missing_qqq
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" run -q -e 'NEEDMATCHLIBS no_such_lib_xyz_zzz
PASS' 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -qi 'NEEDMATCHLIBS miss\|"ok":false'
# run plate may still exit 0 with asserts_fail; ensure error text present
printf '%s\n' "$OUT" | grep -q 'NEEDMATCHLIBS'

OUT=$("$CUBALC" forms hasmatchlibs 2>&1) || OUT=$("$CUBALC" forms HASMATCHLIBS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'HASMATCHLIBS'

OUT=$("$CUBALC" forms needmatchlibs 2>&1) || OUT=$("$CUBALC" forms NEEDMATCHLIBS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'NEEDMATCHLIBS'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -q 'HASMATCHLIBS\|NEEDMATCHLIBS'

OUT=$("$CUBALC" run -q programs/proof/1309_hasmatchlibs.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1309_cli_hasmatchlibs: PASS"
