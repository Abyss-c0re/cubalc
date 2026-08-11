#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -qE '"n":(15|16)'
printf '%s\n' "$OUT" | grep -q 'arg_boot'
printf '%s\n' "$OUT" | grep -qE -- '--arg|--args'

OUT=$("$CUBALC" init "$TMPDIR/arg_demo" --arg --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"arg_boot"'
test -f "$TMPDIR/arg_demo.cubalc"
grep -q 'INCLUDE arg_boot' "$TMPDIR/arg_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/arg_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_arg_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_arg_guard":true'

OUT=$("$CUBALC" recipe arg_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'arg_boot'
printf '%s\n' "$OUT" | grep -qi 'arg_guard\|agent_boot'

OUT=$("$CUBALC" which arg_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'arg_boot'

OUT=$("$CUBALC" libs arg_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'arg_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- '--arg|--args|arg_boot'

OUT=$("$CUBALC" run -q programs/proof/1389_arg_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# aliases --arg-guard / --args / --need-arg
OUT=$("$CUBALC" init --arg-guard "$TMPDIR/ag" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"arg_boot"'

OUT=$("$CUBALC" init --args "$TMPDIR/as" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"arg_boot"'

OUT=$("$CUBALC" init --need-arg "$TMPDIR/na" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"arg_boot"'

echo "1389_cli_init_arg: PASS"
