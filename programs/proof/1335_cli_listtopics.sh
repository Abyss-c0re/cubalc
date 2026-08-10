#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" topics 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.topics.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":8'
printf '%s\n' "$OUT" | grep -q '"id":"cap"'
printf '%s\n' "$OUT" | grep -q '"id":"protect"'

OUT=$("$CUBALC" listtopics 2>&1)
printf '%s\n' "$OUT" | grep -q '"n":8'

OUT=$("$CUBALC" hastopic cap 2>&1)
printf '%s\n' "$OUT" | grep -q '"known":true'
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" hastopic zzz 2>&1)
printf '%s\n' "$OUT" | grep -q '"known":false'

set +e
OUT=$("$CUBALC" needtopic zzz 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"known":false'
test "$rc" -ne 0

OUT=$("$CUBALC" needtopic fat 2>&1)
printf '%s\n' "$OUT" | grep -q '"known":true'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'topics'

OUT=$("$CUBALC" forms LISTTOPICS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'LISTTOPICS'

OUT=$("$CUBALC" run -q -e 'LISTTOPICS
ASSERT LAST_N == 8
HASTOPIC cap
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1335_cli_listtopics: PASS"
