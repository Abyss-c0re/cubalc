#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" guide match p 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.guide.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'
printf '%s\n' "$OUT" | grep -q '"cmd":"guidematch"'
printf '%s\n' "$OUT" | grep -q '"filter":"p"'
printf '%s\n' "$OUT" | grep -q 'HASFORM\|NEEDFORMS'

OUT=$("$CUBALC" guidematch p 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'

OUT=$("$CUBALC" guide nth 1 p 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"plate"'
printf '%s\n' "$OUT" | grep -q '"cmd":"guidenth"'

OUT=$("$CUBALC" guidenth 2 p 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"p2p"'

OUT=$("$CUBALC" guide lastmatch p 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"protect"'
printf '%s\n' "$OUT" | grep -q '"cmd":"guidelastmatch"'

OUT=$("$CUBALC" guidelastmatch p 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"protect"'

OUT=$("$CUBALC" guide match zzz OR general 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"general"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'guidematch\|guide.*match\|lastmatch'

OUT=$("$CUBALC" forms GUIDEMATCH 2>&1)
printf '%s\n' "$OUT" | grep -qi 'GUIDEMATCH'

OUT=$("$CUBALC" run -q -e 'GUIDE MATCH p
ASSERT OK == 1
ASSERT TOPIC_NAME == "cap"
ASSERT GUIDE_RELATED_N >= 3
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" guide match zzz 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$rc" -ne 0

# bare guide still works
OUT=$("$CUBALC" guide lib 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"lib"'

echo "1350_cli_guidematch: PASS"
