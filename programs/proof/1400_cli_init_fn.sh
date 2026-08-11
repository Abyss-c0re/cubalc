#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'fn_boot'
printf '%s\n' "$OUT" | grep -q -- '--fn'

OUT=$("$CUBALC" init "$TMPDIR/fn_demo" --fn --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"fn_boot"'
test -f "$TMPDIR/fn_demo.cubalc"
grep -q 'INCLUDE fn_boot' "$TMPDIR/fn_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/fn_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_fn_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_fn_guard":true'

OUT=$("$CUBALC" recipe fn_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'fn_boot'

OUT=$("$CUBALC" which fn_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'fn_boot'

OUT=$("$CUBALC" libs fn_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'fn_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '--fn'

OUT=$("$CUBALC" run -q programs/proof/1400_fn_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" init --fn-guard "$TMPDIR/fg" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"fn_boot"'

OUT=$("$CUBALC" init --need-fns "$TMPDIR/nf" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"fn_boot"'

echo "1400_cli_init_fn: PASS"
