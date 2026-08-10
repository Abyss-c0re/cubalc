#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -qE '"n":(8|9|10)'
printf '%s\n' "$OUT" | grep -q 'discover_boot'
printf '%s\n' "$OUT" | grep -q -- '--discover'

OUT=$("$CUBALC" init "$TMPDIR/disc_demo" --discover --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"discover_boot"'
test -f "$TMPDIR/disc_demo.cubalc"
grep -q 'INCLUDE discover_boot' "$TMPDIR/disc_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/disc_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_discover_boot":true'

OUT=$("$CUBALC" recipe discover_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'discover_boot'
printf '%s\n' "$OUT" | grep -qi 'agent_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '--discover'

OUT=$("$CUBALC" run -q programs/proof/1353_discover_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" which discover_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'discover_boot'

OUT=$("$CUBALC" init --explore "$TMPDIR/expl" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"discover_boot"'

echo "1353_cli_init_discover: PASS"
