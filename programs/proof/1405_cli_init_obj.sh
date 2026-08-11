#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'obj_boot'
printf '%s\n' "$OUT" | grep -qE -- '--obj|--obj-boot|--obj-guard'

OUT=$("$CUBALC" init "$TMPDIR/obj_demo" --obj --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"obj_boot"'
test -f "$TMPDIR/obj_demo.cubalc"
grep -q 'INCLUDE obj_boot' "$TMPDIR/obj_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/obj_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_obj_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_obj_guard":true'

OUT=$("$CUBALC" recipe obj_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'obj_boot'

OUT=$("$CUBALC" which obj_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'obj_boot'

OUT=$("$CUBALC" libs obj_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'obj_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- '--obj|obj_boot'

OUT=$("$CUBALC" run -q programs/proof/1405_obj_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" init --obj-guard "$TMPDIR/og" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"obj_boot"'

OUT=$("$CUBALC" init --need-objs "$TMPDIR/no" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"obj_boot"'

echo "1405_cli_init_obj: PASS"
