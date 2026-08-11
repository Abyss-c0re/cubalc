#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'full_boot'
printf '%s\n' "$OUT" | grep -qE -- '--fleet|--full-session|--full-boot|--service-full'

OUT=$("$CUBALC" init "$TMPDIR/full_demo" --fleet --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"full_boot"'
test -f "$TMPDIR/full_demo.cubalc"
grep -q 'INCLUDE full_boot' "$TMPDIR/full_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/full_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_full_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_full_session":true'

OUT=$("$CUBALC" recipe full_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'full_boot'

OUT=$("$CUBALC" which full_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'full_boot'

OUT=$("$CUBALC" libs full_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'full_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- '--fleet|--full-session|full_session|full_boot'

OUT=$("$CUBALC" run -q programs/proof/1399_full_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" init --full-session "$TMPDIR/fs" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"full_boot"'

OUT=$("$CUBALC" init --full-boot "$TMPDIR/fb" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"full_boot"'

OUT=$("$CUBALC" init --service-full "$TMPDIR/sf" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"full_boot"'

echo "1399_cli_init_full: PASS"
