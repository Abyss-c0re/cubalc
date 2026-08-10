#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" needdoctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.doctor.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"cmd":"needdoctor"'

OUT=$("$CUBALC" run -q -e 'NEEDDOCTOR
ASSERT OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -D -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms NEEDDOCTOR 2>&1)
printf '%s\n' "$OUT" | grep -qi 'NEEDDOCTOR'

OUT=$("$CUBALC" run -q programs/proof/1357_needdoctor.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1357_cli_needdoctor: PASS"
