#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" formguide HASFORM 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.formguide.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"form":"HASFORM"'
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'
printf '%s\n' "$OUT" | grep -q 'HASFORM\|NEEDFORMS\|capability'
printf '%s\n' "$OUT" | grep -q '"related"'

OUT=$("$CUBALC" guideform INCLUDE 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"general"'
printf '%s\n' "$OUT" | grep -q '"lib"'

OUT=$("$CUBALC" formguide SETP 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"plate"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'formguide'

OUT=$("$CUBALC" forms FORMGUIDE 2>&1)
printf '%s\n' "$OUT" | grep -qi 'FORMGUIDE'

OUT=$("$CUBALC" run -q -e 'FORMGUIDE HASFORM
ASSERT OK == 1
ASSERT FORMGUIDE_TOPIC == "cap"
ASSERT GUIDE_RELATED_N >= 3
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" formguide ZZZ_NOPE 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$rc" -ne 0

echo "1344_cli_formguide: PASS"
