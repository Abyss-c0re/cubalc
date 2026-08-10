#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -qE '"n":(13|14)'
printf '%s\n' "$OUT" | grep -q 'cli_session'
printf '%s\n' "$OUT" | grep -q -- '--cli-session'

OUT=$("$CUBALC" init "$TMPDIR/cli_sess" --cli-session --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"cli_session"'
test -f "$TMPDIR/cli_sess.cubalc"
grep -q 'INCLUDE cli_session' "$TMPDIR/cli_sess.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/cli_sess.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_cli_session":true'

OUT=$("$CUBALC" recipe cli_session 2>&1)
printf '%s\n' "$OUT" | grep -q 'cli_session'

OUT=$("$CUBALC" which cli_session 2>&1)
printf '%s\n' "$OUT" | grep -qi 'cli_session'

OUT=$("$CUBALC" run -q programs/proof/1368_cli_session.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" init --cli-guard "$TMPDIR/cg" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"cli_session"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'cli-session\|cli_session\|--cli'

echo "1368_cli_init_cli_session: PASS"
