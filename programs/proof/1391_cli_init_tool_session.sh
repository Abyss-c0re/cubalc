#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -qE '"n":(17|18|19|20|21|22)'
printf '%s\n' "$OUT" | grep -q 'tool_session'
printf '%s\n' "$OUT" | grep -qE -- '--full-cli|--tool-full|--cli-full'

OUT=$("$CUBALC" init "$TMPDIR/tool_demo" --full-cli --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"tool_session"'
test -f "$TMPDIR/tool_demo.cubalc"
grep -q 'INCLUDE tool_session' "$TMPDIR/tool_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/tool_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_tool_session":true'
printf '%s\n' "$OUT" | grep -q '"lib_cli_session":true'
printf '%s\n' "$OUT" | grep -q '"lib_arg_guard":true'

OUT=$("$CUBALC" recipe tool_session 2>&1)
printf '%s\n' "$OUT" | grep -q 'tool_session'
printf '%s\n' "$OUT" | grep -qiE 'cli_guard|arg_guard|cli_boot'

OUT=$("$CUBALC" which tool_session 2>&1)
printf '%s\n' "$OUT" | grep -qi 'tool_session'

OUT=$("$CUBALC" libs tool_session 2>&1)
printf '%s\n' "$OUT" | grep -qi 'tool_session'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- 'full-cli|tool_session|tool-full'

OUT=$("$CUBALC" run -q programs/proof/1391_tool_session.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# aliases
OUT=$("$CUBALC" init --cli-full "$TMPDIR/cf" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"tool_session"'

OUT=$("$CUBALC" init --tool-full "$TMPDIR/tf" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"tool_session"'

OUT=$("$CUBALC" init --flags-args "$TMPDIR/fa" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"tool_session"'

# cli-session still works and is distinct
OUT=$("$CUBALC" init "$TMPDIR/cs" --cli-session --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"cli_session"'

echo "1391_cli_init_tool_session: PASS"
