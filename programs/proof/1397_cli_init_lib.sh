#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'lib_boot'
printf '%s\n' "$OUT" | grep -qE -- '--lib-boot|--lib-guard|--need-libs'

OUT=$("$CUBALC" init "$TMPDIR/lib_demo" --lib-boot --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"lib_boot"'
test -f "$TMPDIR/lib_demo.cubalc"
grep -q 'INCLUDE lib_boot' "$TMPDIR/lib_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/lib_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_lib_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_lib_guard":true'

OUT=$("$CUBALC" recipe lib_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'lib_boot'

OUT=$("$CUBALC" which lib_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'lib_boot'

OUT=$("$CUBALC" libs lib_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'lib_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- '--lib-boot|--lib-guard|--need-libs'

OUT=$("$CUBALC" run -q programs/proof/1397_lib_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" init --lib-guard "$TMPDIR/lg" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"lib_boot"'

OUT=$("$CUBALC" init --need-libs "$TMPDIR/nl" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"lib_boot"'

echo "1397_cli_init_lib: PASS"
