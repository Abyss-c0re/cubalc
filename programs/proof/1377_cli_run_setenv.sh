#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR="${TMPDIR:-/tmp}"
ENVF="$TMPDIR/cubalc_mi_setenv_$$.env"
trap 'rm -f "$ENVF"' EXIT

# -E injects before body
OUT=$("$CUBALC" run -q -E CUBALC_MI_SETENV_A=alpha -e 'SYS ENV "CUBALC_MI_SETENV_A"
SYS EQS LAST "alpha"
ASSERT LAST_N == 1
SYS ENV "CUBALC_SETENV_N"
SYS EQS LAST "1"
ASSERT LAST_N == 1
SYS ENV "CUBALC_SETENV_KEYS"
SYS HAS LAST "CUBALC_MI_SETENV_A"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# multiple -E
OUT=$("$CUBALC" run -q -E CUBALC_MI_SETENV_A=one -E CUBALC_MI_SETENV_B=two -e 'SYS ENV "CUBALC_MI_SETENV_A"
SYS EQS LAST "one"
ASSERT LAST_N == 1
SYS ENV "CUBALC_MI_SETENV_B"
SYS EQS LAST "two"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# --setenv= form
OUT=$("$CUBALC" run -q --setenv=CUBALC_MI_SETENV_C=three -e 'SYS ENV "CUBALC_MI_SETENV_C"
SYS EQS LAST "three"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# CLI -E wins over dotenv plate
printf 'CUBALC_MI_SETENV_A=from-file\n' > "$ENVF"
OUT=$("$CUBALC" run -q -F "$ENVF" -E CUBALC_MI_SETENV_A=from-cli -e 'SYS ENV "CUBALC_MI_SETENV_A"
SYS EQS LAST "from-cli"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# bad KEY=VAL hard-fails
set +e
OUT=$("$CUBALC" run -q -E 'NOEQUALS' -e 'PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -q 'SETENV failed'
printf '%s\n' "$OUT" | grep -q '"ok":false'

# usage surfaces setenv
set +e
OUT=$("$CUBALC" run 2>&1)
set -e
printf '%s\n' "$OUT" | grep -qi 'setenv'

# env catalog
OUT=$("$CUBALC" env 2>&1)
printf '%s\n' "$OUT" | grep -q 'CUBALC_SETENV'

echo "1377_cli_run_setenv: PASS"
