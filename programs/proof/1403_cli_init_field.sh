#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'field_boot'
printf '%s\n' "$OUT" | grep -qE -- '--field|--field-boot|--field-guard'

OUT=$("$CUBALC" init "$TMPDIR/field_demo" --field --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"field_boot"'
test -f "$TMPDIR/field_demo.cubalc"
grep -q 'INCLUDE field_boot' "$TMPDIR/field_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/field_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_field_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_field_guard":true'

OUT=$("$CUBALC" recipe field_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'field_boot'

OUT=$("$CUBALC" which field_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'field_boot'

OUT=$("$CUBALC" libs field_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'field_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- '--field|field_boot'

OUT=$("$CUBALC" run -q programs/proof/1403_field_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" init --field-guard "$TMPDIR/fg" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"field_boot"'

OUT=$("$CUBALC" init --need-fields "$TMPDIR/nf" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"field_boot"'

echo "1403_cli_init_field: PASS"
