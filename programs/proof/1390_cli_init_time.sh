#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -qE '"n":(16|17)'
printf '%s\n' "$OUT" | grep -q 'time_boot'
printf '%s\n' "$OUT" | grep -qE -- '--time|--timeout-boot'

OUT=$("$CUBALC" init "$TMPDIR/time_demo" --time --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"time_boot"'
test -f "$TMPDIR/time_demo.cubalc"
grep -q 'INCLUDE time_boot' "$TMPDIR/time_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/time_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_time_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_time_guard":true'

OUT=$("$CUBALC" recipe time_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'time_boot'
printf '%s\n' "$OUT" | grep -qi 'time_guard\|agent_boot'

OUT=$("$CUBALC" which time_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'time_boot'

OUT=$("$CUBALC" libs time_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'time_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- '--time|time_boot'

OUT=$("$CUBALC" run -q programs/proof/1390_time_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# aliases --time-boot / --timeout-boot / --need-time
OUT=$("$CUBALC" init --time-boot "$TMPDIR/tb" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"time_boot"'

OUT=$("$CUBALC" init --timeout-boot "$TMPDIR/tob" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"time_boot"'

OUT=$("$CUBALC" init --need-time "$TMPDIR/nt" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"time_boot"'

echo "1390_cli_init_time: PASS"
