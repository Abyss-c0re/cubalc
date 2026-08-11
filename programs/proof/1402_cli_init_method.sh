#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'method_boot'
printf '%s\n' "$OUT" | grep -qE -- '--method|--method-boot|--method-guard'

OUT=$("$CUBALC" init "$TMPDIR/method_demo" --method --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"method_boot"'
test -f "$TMPDIR/method_demo.cubalc"
grep -q 'INCLUDE method_boot' "$TMPDIR/method_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/method_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_method_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_method_guard":true'

OUT=$("$CUBALC" recipe method_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'method_boot'

OUT=$("$CUBALC" which method_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'method_boot'

OUT=$("$CUBALC" libs method_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'method_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- '--method|method_boot'

OUT=$("$CUBALC" run -q programs/proof/1402_method_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" init --method-guard "$TMPDIR/mg" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"method_boot"'

OUT=$("$CUBALC" init --need-methods "$TMPDIR/nm" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"method_boot"'

echo "1402_cli_init_method: PASS"
