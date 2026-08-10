#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -qE '"n":(8|9)'
printf '%s\n' "$OUT" | grep -q 'onboard_boot'
printf '%s\n' "$OUT" | grep -q -- '--onboard'

OUT=$("$CUBALC" init "$TMPDIR/onboard_demo" --onboard --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"onboard_boot"'
test -f "$TMPDIR/onboard_demo.cubalc"
grep -q 'INCLUDE onboard_boot' "$TMPDIR/onboard_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/onboard_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_onboard_boot":true'

OUT=$("$CUBALC" recipe onboard_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'onboard_boot'
printf '%s\n' "$OUT" | grep -qi 'agent_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '--onboard'

OUT=$("$CUBALC" run -q programs/proof/1346_onboard_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" which onboard_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'onboard_boot'

echo "1346_cli_init_onboard: PASS"
