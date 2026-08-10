#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -qE '"n":(11|12|13)'
printf '%s\n' "$OUT" | grep -q 'ready_boot'
printf '%s\n' "$OUT" | grep -q -- '--ready'

OUT=$("$CUBALC" init "$TMPDIR/ready_demo" --ready --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"ready_boot"'
test -f "$TMPDIR/ready_demo.cubalc"
grep -q 'INCLUDE ready_boot' "$TMPDIR/ready_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/ready_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_ready_boot":true'

OUT=$("$CUBALC" recipe ready_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'ready_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '--ready'

OUT=$("$CUBALC" run -q programs/proof/1361_ready_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" which ready_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'ready_boot'

OUT=$("$CUBALC" init --need-ready "$TMPDIR/nr" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"ready_boot"'

OUT=$("$CUBALC" run -q -e 'INCLUDE ready_boot
ASSERT READY_OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# prove checklist lists ready_boot lib
OUT=$("$CUBALC" ready 2>&1)
printf '%s\n' "$OUT" | grep -q 'ready_boot\|"ok":true'

echo "1361_cli_init_ready: PASS"
