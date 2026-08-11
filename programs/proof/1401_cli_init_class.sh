#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'class_boot'
printf '%s\n' "$OUT" | grep -qE -- '--class|--class-boot|--class-guard'

OUT=$("$CUBALC" init "$TMPDIR/class_demo" --class --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"class_boot"'
test -f "$TMPDIR/class_demo.cubalc"
grep -q 'INCLUDE class_boot' "$TMPDIR/class_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/class_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_class_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_class_guard":true'

OUT=$("$CUBALC" recipe class_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'class_boot'

OUT=$("$CUBALC" which class_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'class_boot'

OUT=$("$CUBALC" libs class_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'class_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- '--class|class_boot'

OUT=$("$CUBALC" run -q programs/proof/1401_class_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" init --class-guard "$TMPDIR/cg" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"class_boot"'

OUT=$("$CUBALC" init --need-classes "$TMPDIR/nc" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"class_boot"'

echo "1401_cli_init_class: PASS"
