#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR="${TMPDIR:-/tmp}"
BASE="$TMPDIR/cubalc_mi_mkdir_$$"
trap 'rm -rf "$BASE"' EXIT
rm -rf "$BASE"

NEST="$BASE/a/b/c"
test ! -d "$NEST"

# -M creates nested path
OUT=$("$CUBALC" run -q -M "$NEST" -e 'SYS ISDIR "'"$NEST"'"
ASSERT LAST_N == 1
SYS ENV "CUBALC_MKDIR_N"
SYS EQS LAST "1"
ASSERT LAST_N == 1
SYS ENV "CUBALC_MKDIR_DIRS"
SYS HAS LAST "cubalc_mi_mkdir"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
test -d "$NEST"

# idempotent second ensure
OUT=$("$CUBALC" run -q -M "$NEST" -e 'SYS ISDIR "'"$NEST"'"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# -M then --cwd into fresh workdir
WORK="$BASE/work"
OUT=$("$CUBALC" run -q -M "$WORK" --cwd "$WORK" -e 'SYS CWD
SYS HAS LAST "work"
ASSERT LAST_N == 1
SYS WRITE "plate.txt" "ok"
ASSERT OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
test -f "$WORK/plate.txt"
grep -q 'ok' "$WORK/plate.txt"

# --mkdir= form
OUT=$("$CUBALC" run -q --mkdir="$BASE/d1" --mkdir="$BASE/d2" -e 'SYS ISDIR "'"$BASE/d1"'"
ASSERT LAST_N == 1
SYS ISDIR "'"$BASE/d2"'"
ASSERT LAST_N == 1
SYS ENV "CUBALC_MKDIR_N"
SYS EQS LAST "2"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# conflict: path is a file
printf 'x' > "$BASE/notdir"
set +e
OUT=$("$CUBALC" run -q -M "$BASE/notdir" -e 'PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -q 'MKDIR failed'
printf '%s\n' "$OUT" | grep -q '"ok":false'

# usage surfaces mkdir
set +e
OUT=$("$CUBALC" run 2>&1)
set -e
printf '%s\n' "$OUT" | grep -qi 'mkdir'

# env catalog
OUT=$("$CUBALC" env 2>&1)
printf '%s\n' "$OUT" | grep -q 'CUBALC_MKDIR'

echo "1379_cli_run_mkdir: PASS"
