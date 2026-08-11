#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'rw_boot'
printf '%s\n' "$OUT" | grep -q -- '--rw'

OUT=$("$CUBALC" init "$TMPDIR/rw_demo" --rw --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"rw_boot"'
test -f "$TMPDIR/rw_demo.cubalc"
grep -q 'INCLUDE rw_boot' "$TMPDIR/rw_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/rw_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_rw_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_rw_guard":true'

OUT=$("$CUBALC" recipe rw_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'rw_boot'

OUT=$("$CUBALC" which rw_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'rw_boot'

OUT=$("$CUBALC" libs rw_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'rw_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '--rw'

OUT=$("$CUBALC" run -q programs/proof/1395_rw_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# aliases
OUT=$("$CUBALC" init --rw-guard "$TMPDIR/rg" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"rw_boot"'

OUT=$("$CUBALC" init --access "$TMPDIR/ac" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"rw_boot"'

echo "1395_cli_init_rw: PASS"
