#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -qE '"n":(18|19)'
printf '%s\n' "$OUT" | grep -q 'product_session'
printf '%s\n' "$OUT" | grep -qE -- '--product|--full-product|--service'

OUT=$("$CUBALC" init "$TMPDIR/prod_demo" --product --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"product_session"'
test -f "$TMPDIR/prod_demo.cubalc"
grep -q 'INCLUDE product_session' "$TMPDIR/prod_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/prod_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_product_session":true'
printf '%s\n' "$OUT" | grep -q '"lib_tool_session":true'
printf '%s\n' "$OUT" | grep -q '"lib_env_guard":true'
printf '%s\n' "$OUT" | grep -q '"lib_time_guard":true'

OUT=$("$CUBALC" recipe product_session 2>&1)
printf '%s\n' "$OUT" | grep -q 'product_session'
printf '%s\n' "$OUT" | grep -qiE 'tool_session|env_guard|time_guard'

OUT=$("$CUBALC" which product_session 2>&1)
printf '%s\n' "$OUT" | grep -qi 'product_session'

OUT=$("$CUBALC" libs product_session 2>&1)
printf '%s\n' "$OUT" | grep -qi 'product_session'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- 'product|product_session|--product'

OUT=$("$CUBALC" run -q programs/proof/1392_product_session.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# aliases
OUT=$("$CUBALC" init --app "$TMPDIR/app" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"product_session"'

OUT=$("$CUBALC" init --service "$TMPDIR/svc" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"product_session"'

OUT=$("$CUBALC" init --full-product "$TMPDIR/fp" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"product_session"'

OUT=$("$CUBALC" init --cli-env-time "$TMPDIR/cet" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"product_session"'

# full-cli still distinct
OUT=$("$CUBALC" init "$TMPDIR/ts" --full-cli --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"tool_session"'

echo "1392_cli_init_product_session: PASS"
