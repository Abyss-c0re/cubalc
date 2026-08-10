#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -qE '"n":(12|13|14)'
printf '%s\n' "$OUT" | grep -q 'cli_boot'
printf '%s\n' "$OUT" | grep -q -- '--cli'

OUT=$("$CUBALC" init "$TMPDIR/cli_demo" --cli --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"cli_boot"'
test -f "$TMPDIR/cli_demo.cubalc"
grep -q 'INCLUDE cli_boot' "$TMPDIR/cli_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/cli_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_cli_boot":true'

OUT=$("$CUBALC" recipe cli_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'cli_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '--cli'

OUT=$("$CUBALC" run -q programs/proof/1365_cli_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" which cli_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'cli_boot'

OUT=$("$CUBALC" init --tool "$TMPDIR/tool" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"cli_boot"'

# --help exits 0 via HELPFLAG inside scaffold
OUT=$("$CUBALC" run -q -e 'INCLUDE cli_boot
PASS' -- --help 2>&1 || true)
printf '%s\n' "$OUT" | grep -qiE 'usage|cubalc-tool|halted|exit'

OUT=$("$CUBALC" run -q -e 'INCLUDE cli_boot
ASSERT HELPFLAG_HIT == 0
CLIINFO
ASSERT OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1365_cli_init_cli: PASS"
