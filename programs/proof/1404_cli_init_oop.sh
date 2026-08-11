#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'oop_boot'
printf '%s\n' "$OUT" | grep -qE -- '--oop|--oop-boot|--oop-session'

OUT=$("$CUBALC" init "$TMPDIR/oop_demo" --oop --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"oop_boot"'
test -f "$TMPDIR/oop_demo.cubalc"
grep -q 'INCLUDE oop_boot' "$TMPDIR/oop_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/oop_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_oop_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_oop_session":true'

OUT=$("$CUBALC" recipe oop_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'oop_boot'

OUT=$("$CUBALC" which oop_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'oop_boot'

OUT=$("$CUBALC" libs oop_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'oop_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- '--oop|oop_boot'

OUT=$("$CUBALC" run -q programs/proof/1404_oop_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" init --oop-session "$TMPDIR/os" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"oop_boot"'

OUT=$("$CUBALC" init --need-oop "$TMPDIR/no" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"oop_boot"'

echo "1404_cli_init_oop: PASS"
