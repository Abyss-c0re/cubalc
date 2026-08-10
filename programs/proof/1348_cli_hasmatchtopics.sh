#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" hasmatchtopics cap 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.topicmatch.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"mode":"has"'

OUT=$("$CUBALC" countmatchtopics p 2>&1)
printf '%s\n' "$OUT" | grep -q '"mode":"count"'
printf '%s\n' "$OUT" | grep -qE '"n":[2-9]'

OUT=$("$CUBALC" needmatchtopics plate 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"mode":"need"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'hasmatchtopics'

OUT=$("$CUBALC" forms HASMATCHTOPICS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'HASMATCHTOPICS'

OUT=$("$CUBALC" run -q -e 'COUNTMATCHTOPICS p
ASSERT OK == 1
ASSERT LAST_N >= 2
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" needmatchtopics zzz 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$rc" -ne 0

set +e
OUT=$("$CUBALC" hasmatchtopics zzz 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"hit":false'
# has always exit 0
test "$rc" -eq 0

# NEEDMATCHTOPICS hard in-lang
set +e
OUT=$("$CUBALC" run -q -e 'NEEDMATCHTOPICS zzz_nope
PASS' 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false\|NEEDMATCHTOPICS'
test "$rc" -ne 0

echo "1348_cli_hasmatchtopics: PASS"
