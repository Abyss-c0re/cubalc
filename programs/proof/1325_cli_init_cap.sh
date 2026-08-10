#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -qE '"n":(8|9|10)'
printf '%s\n' "$OUT" | grep -q 'cap_boot'
printf '%s\n' "$OUT" | grep -q -- '--cap'

OUT=$("$CUBALC" init "$TMPDIR/cap_demo" --cap --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"cap_boot"'
test -f "$TMPDIR/cap_demo.cubalc"
grep -q 'INCLUDE cap_boot' "$TMPDIR/cap_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/cap_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_cap_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_form_guard":true'

OUT=$("$CUBALC" recipe cap_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'cap_boot'
printf '%s\n' "$OUT" | grep -qi 'form_guard\|agent_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '--cap'

OUT=$("$CUBALC" run -q programs/proof/1325_cap_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1325_cli_init_cap: PASS"
