#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

# "p" → cap, plate, p2p, protect
OUT=$("$CUBALC" nthtopic 0 p 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.topicmatch.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'
printf '%s\n' "$OUT" | grep -q '"idx":0'

OUT=$("$CUBALC" nthtopic 1 p 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"plate"'

OUT=$("$CUBALC" lasttopic p 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"protect"'
printf '%s\n' "$OUT" | grep -q '"cmd":"lasttopic"'

OUT=$("$CUBALC" nthtopic 0 lib 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"lib"'

OUT=$("$CUBALC" nthtopic 9 p OR general 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"general"'
printf '%s\n' "$OUT" | grep -q '"fallback":true'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'nthtopic\|lasttopic'

OUT=$("$CUBALC" forms NTHTOPIC 2>&1)
printf '%s\n' "$OUT" | grep -qi 'NTHTOPIC'

OUT=$("$CUBALC" run -q -e 'LASTTOPIC p
ASSERT OK == 1
ASSERT LASTTOPIC == "protect"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" nthtopic 5 p 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$rc" -ne 0

echo "1349_cli_nthtopic: PASS"
