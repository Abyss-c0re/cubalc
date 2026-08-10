#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" relatedtopic cap 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.relatedtopic.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'
printf '%s\n' "$OUT" | grep -q '"general"'
printf '%s\n' "$OUT" | grep -q '"run"'

OUT=$("$CUBALC" seetopics plate 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"plate"'
printf '%s\n' "$OUT" | grep -q '"fat"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'relatedtopic'

OUT=$("$CUBALC" forms RELATEDTOPIC 2>&1)
printf '%s\n' "$OUT" | grep -qi 'RELATEDTOPIC'

OUT=$("$CUBALC" run -q -e 'RELATEDTOPIC lib
ASSERT OK == 1
SYS HASI "cap"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" relatedtopic zzz 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$rc" -ne 0

echo "1340_cli_relatedtopic: PASS"
