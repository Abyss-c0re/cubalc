#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" formtopics HASFORM 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.formtopics.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"form":"HASFORM"'
printf '%s\n' "$OUT" | grep -q '"cap"'

OUT=$("$CUBALC" topicsof INCLUDE 2>&1)
printf '%s\n' "$OUT" | grep -q '"form":"INCLUDE"'
printf '%s\n' "$OUT" | grep -q '"general"'
printf '%s\n' "$OUT" | grep -q '"lib"'

OUT=$("$CUBALC" formtopics SETP 2>&1)
printf '%s\n' "$OUT" | grep -q '"plate"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'formtopics'

OUT=$("$CUBALC" forms FORMTOPICS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'FORMTOPICS'

OUT=$("$CUBALC" run -q -e 'FORMTOPICS HASFORM
ASSERT OK == 1
SYS HASI "cap"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" formtopics ZZZ_NOPE 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$rc" -ne 0

echo "1341_cli_formtopics: PASS"
