#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" matchtopics cap 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.topicmatch.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"cap"'
printf '%s\n' "$OUT" | grep -q '"n":1'

OUT=$("$CUBALC" matchtopics p 2>&1)
printf '%s\n' "$OUT" | grep -q '"plate"'
printf '%s\n' "$OUT" | grep -q '"p2p"'

OUT=$("$CUBALC" picktopic mesh 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"p2p"'
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" picktopic zzz OR general 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"general"'
printf '%s\n' "$OUT" | grep -q '"fallback":true'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'matchtopics\|picktopic'

OUT=$("$CUBALC" forms MATCHTOPICS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'MATCHTOPICS'

OUT=$("$CUBALC" forms PICKTOPIC 2>&1)
printf '%s\n' "$OUT" | grep -qi 'PICKTOPIC'

OUT=$("$CUBALC" run -q -e 'PICKTOPIC lib
ASSERT OK == 1
ASSERT PICKTOPIC == "lib"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" matchtopics zzz 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$rc" -ne 0

echo "1347_cli_matchtopics: PASS"
