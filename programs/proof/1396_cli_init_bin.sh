#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'bin_boot'
printf '%s\n' "$OUT" | grep -q -- '--bin'

OUT=$("$CUBALC" init "$TMPDIR/bin_demo" --bin --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"bin_boot"'
test -f "$TMPDIR/bin_demo.cubalc"
grep -q 'INCLUDE bin_boot' "$TMPDIR/bin_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/bin_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_bin_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_bin_guard":true'

OUT=$("$CUBALC" recipe bin_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'bin_boot'

OUT=$("$CUBALC" which bin_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'bin_boot'

OUT=$("$CUBALC" libs bin_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'bin_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '--bin'

OUT=$("$CUBALC" run -q programs/proof/1396_bin_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" init --bin-guard "$TMPDIR/bg" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"bin_boot"'

OUT=$("$CUBALC" init --tools "$TMPDIR/tl" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"bin_boot"'

echo "1396_cli_init_bin: PASS"
