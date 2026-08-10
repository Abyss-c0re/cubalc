#!/bin/sh
# cubalc picklib|countmatchlibs|hasmatchlibs|needmatchlibs|nthlib|lastlib
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" picklib plate 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.libmatch.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"stem":"plate_boot"'
printf '%s\n' "$OUT" | grep -qE '"n":1[0-9]'

OUT=$("$CUBALC" countmatchlibs plate 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -qE '"n":1[0-9]'

OUT=$("$CUBALC" hasmatchlibs fat 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":2'

OUT=$("$CUBALC" lastlib plate 2>&1)
printf '%s\n' "$OUT" | grep -q '"stem":"plate_uniform"'

OUT=$("$CUBALC" nthlib 1 fat 2>&1)
printf '%s\n' "$OUT" | grep -q '"stem":"fat_session"'

set +e
OUT=$("$CUBALC" needmatchlibs no_such_lib_xyz_zzz 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

set +e
OUT=$("$CUBALC" hasmatchlibs no_such_lib_xyz_zzz 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -eq 0

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'picklib\|countmatchlibs\|hasmatchlibs'

OUT=$("$CUBALC" forms countmatchlibs 2>&1) || OUT=$("$CUBALC" forms COUNTMATCHLIBS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'COUNTMATCHLIBS\|COUNTLIBS'

OUT=$("$CUBALC" run -q programs/proof/1313_countmatchlibs.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1313_cli_libmatch: PASS"
