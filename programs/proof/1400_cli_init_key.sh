#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'key_boot'
printf '%s\n' "$OUT" | grep -qE -- '--key|--key-boot|--plate-guard|--need-keys'

OUT=$("$CUBALC" init "$TMPDIR/key_demo" --key --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"key_boot"'
test -f "$TMPDIR/key_demo.cubalc"
grep -q 'INCLUDE key_boot' "$TMPDIR/key_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/key_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_key_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_plate_guard":true'

OUT=$("$CUBALC" recipe key_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'key_boot'

OUT=$("$CUBALC" which key_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'key_boot'

OUT=$("$CUBALC" libs key_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'key_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- '--key|key_boot|plate_guard'

OUT=$("$CUBALC" run -q programs/proof/1400_key_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" init --key-boot "$TMPDIR/kb" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"key_boot"'

OUT=$("$CUBALC" init --plate-guard "$TMPDIR/pg" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"key_boot"'

OUT=$("$CUBALC" init --need-keys "$TMPDIR/nk" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"key_boot"'

echo "1400_cli_init_key: PASS"
