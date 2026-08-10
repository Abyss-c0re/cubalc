#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -qE '"n":(14|15)'
printf '%s\n' "$OUT" | grep -q 'env_boot'
printf '%s\n' "$OUT" | grep -q -- '--env'

OUT=$("$CUBALC" init "$TMPDIR/env_demo" --env --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"env_boot"'
test -f "$TMPDIR/env_demo.cubalc"
grep -q 'INCLUDE env_boot' "$TMPDIR/env_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/env_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_env_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_env_guard":true'

OUT=$("$CUBALC" recipe env_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'env_boot'
printf '%s\n' "$OUT" | grep -qi 'env_guard\|agent_boot'

OUT=$("$CUBALC" which env_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'env_boot'

OUT=$("$CUBALC" libs env_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'env_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '--env'

OUT=$("$CUBALC" run -q programs/proof/1384_env_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# alias --env-guard / --hostenv
OUT=$("$CUBALC" init --env-guard "$TMPDIR/eg" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"env_boot"'

OUT=$("$CUBALC" init --hostenv "$TMPDIR/he" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"env_boot"'

echo "1384_cli_init_env: PASS"
