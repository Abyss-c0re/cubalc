#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -qE '"n":(9|10)'
printf '%s\n' "$OUT" | grep -q 'open_boot'
printf '%s\n' "$OUT" | grep -q -- '--open'

OUT=$("$CUBALC" init "$TMPDIR/open_demo" --open --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"open_boot"'
test -f "$TMPDIR/open_demo.cubalc"
grep -q 'INCLUDE open_boot' "$TMPDIR/open_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/open_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_open_boot":true'

OUT=$("$CUBALC" recipe open_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'open_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '--open'

OUT=$("$CUBALC" run -q programs/proof/1355_open_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" which open_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'open_boot'

OUT=$("$CUBALC" init --playbook "$TMPDIR/pb" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"open_boot"'

echo "1355_cli_init_open: PASS"
