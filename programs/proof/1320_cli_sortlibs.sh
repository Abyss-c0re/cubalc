#!/bin/sh
# cubalc sortlibs — CLI dual of SORTLIBS
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" sortlibs fat 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.libmatch.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"cmd":"sortlibs"'
printf '%s\n' "$OUT" | grep -q '"order":"newest"'
printf '%s\n' "$OUT" | grep -q '"stem":"fat_session"'
# stems newest-first
printf '%s\n' "$OUT" | grep -q '"stems":\["fat_session","fat_boot"\]'
printf '%s\n' "$OUT" | grep -q '"mtimes":\['
printf '%s\n' "$OUT" | grep -qE '"mtime":[1-9]'

OUT=$("$CUBALC" sortlibs fat oldest 2>&1)
printf '%s\n' "$OUT" | grep -q '"order":"oldest"'
printf '%s\n' "$OUT" | grep -q '"stem":"fat_boot"'
printf '%s\n' "$OUT" | grep -q '"stems":\["fat_boot","fat_session"\]'

OUT=$("$CUBALC" libsort guard 2>&1)
printf '%s\n' "$OUT" | grep -q '"stem":"time_guard"'
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" sortlibs no_such_lib_xyz_zzz 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'sortlibs'

OUT=$("$CUBALC" forms SORTLIBS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'SORTLIBS'

# in-lang dual still green
OUT=$("$CUBALC" run -q -e 'SORTLIBS fat
ASSERT SORTLIBS_HEAD == "fat_session"
ASSERT SORTLIBS_N == 2
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1320_cli_sortlibs: PASS"
