#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" runsnip match p 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.runsnip.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'
printf '%s\n' "$OUT" | grep -q '"cmd":"runsnipmatch"'
printf '%s\n' "$OUT" | grep -q '"filter":"p"'

OUT=$("$CUBALC" runsnipmatch p 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'

OUT=$("$CUBALC" runsnip nth 1 p 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"plate"'
printf '%s\n' "$OUT" | grep -q '"cmd":"runsnipnth"'

OUT=$("$CUBALC" runsnipnth 2 p 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"p2p"'

OUT=$("$CUBALC" runsnip lastmatch p 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"protect"'
printf '%s\n' "$OUT" | grep -q '"cmd":"runsniplastmatch"'

OUT=$("$CUBALC" runsniplastmatch p 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"protect"'

OUT=$("$CUBALC" runsnip match zzz OR general 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"general"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'runsnip.*match\|match|nth|lastmatch'

OUT=$("$CUBALC" forms RUNSNIPMATCH 2>&1)
printf '%s\n' "$OUT" | grep -qi 'RUNSNIPMATCH'

OUT=$("$CUBALC" run -q -e 'RUNSNIP MATCH p
ASSERT OK == 1
ASSERT RUNSNIP_TOPIC == "cap"
ASSERT RUNSNIP_OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" runsnip match zzz 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$rc" -ne 0

# bare runsnip still works
OUT=$("$CUBALC" runsnip lib 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"lib"'
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1351_cli_runsnipmatch: PASS"
