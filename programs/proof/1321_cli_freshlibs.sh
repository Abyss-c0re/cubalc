#!/bin/sh
# cubalc freshlibs|stalelibs — CLI dual of FRESHLIBS/STALELIBS
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

# fat_boot mtime can be months old — wide window captures both fat_*
OUT=$("$CUBALC" freshlibs fat 100000000 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.libmatch.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"mode":"fresh"'
printf '%s\n' "$OUT" | grep -q '"age_lim":100000000'
printf '%s\n' "$OUT" | grep -q '"stem":"fat_session"'
printf '%s\n' "$OUT" | grep -q 'fat_boot'
printf '%s\n' "$OUT" | grep -q '"ages":\['

OUT=$("$CUBALC" stalelibs fat 10 2>&1)
printf '%s\n' "$OUT" | grep -q '"mode":"stale"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -E -q '"n":[12]'

# tight window: empty is soft miss (ok:false, exit≠0) — do not trip set -e
set +e
OUT=$("$CUBALC" freshlibs fat 5 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.libmatch.v1"'
printf '%s\n' "$OUT" | grep -E -q '"n":[01]'
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

OUT=$("$CUBALC" recentlibs guard 100000000 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'time_guard'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'freshlibs\|stalelibs'

OUT=$("$CUBALC" forms FRESHLIBS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'FRESHLIBS'

OUT=$("$CUBALC" run -q -e 'FRESHLIBS fat 100000000
ASSERT FRESHLIBS_N == 2
ASSERT FRESHLIBS_HEAD == "fat_session"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1321_cli_freshlibs: PASS"
