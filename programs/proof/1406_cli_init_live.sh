#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'live_boot'
printf '%s\n' "$OUT" | grep -qE -- '--live|--live-boot|--live-session'

OUT=$("$CUBALC" init "$TMPDIR/live_demo" --live --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"live_boot"'
test -f "$TMPDIR/live_demo.cubalc"
grep -q 'INCLUDE live_boot' "$TMPDIR/live_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/live_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_live_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_live_session":true'

OUT=$("$CUBALC" recipe live_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'live_boot'

OUT=$("$CUBALC" which live_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'live_boot'

OUT=$("$CUBALC" libs live_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'live_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- '--live|live_boot'

OUT=$("$CUBALC" run -q programs/proof/1406_live_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" init --live-session "$TMPDIR/ls" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"live_boot"'

OUT=$("$CUBALC" init --need-live "$TMPDIR/nl" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"live_boot"'

echo "1406_cli_init_live: PASS"
