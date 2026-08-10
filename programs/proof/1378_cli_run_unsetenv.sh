#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR="${TMPDIR:-/tmp}"
ENVF="$TMPDIR/cubalc_mi_unsetenv_$$.env"
trap 'rm -f "$ENVF"' EXIT

# seed env in parent then clear via -U
export CUBALC_MI_UNSET_A=parent
OUT=$("$CUBALC" run -q -U CUBALC_MI_UNSET_A -e 'SYS ENV "CUBALC_MI_UNSET_A" OR "gone"
SYS EQS LAST "gone"
ASSERT LAST_N == 1
SYS ENV "CUBALC_UNSETENV_N"
SYS EQS LAST "1"
ASSERT LAST_N == 1
SYS ENV "CUBALC_UNSETENV_KEYS"
SYS HAS LAST "CUBALC_MI_UNSET_A"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# -U after dotenv clears file-loaded key; -E can re-set
printf 'CUBALC_MI_UNSET_B=from-file\nCUBALC_MI_UNSET_C=keep\n' > "$ENVF"
OUT=$("$CUBALC" run -q -F "$ENVF" -U CUBALC_MI_UNSET_B -E CUBALC_MI_UNSET_B=from-cli -e 'SYS ENV "CUBALC_MI_UNSET_B"
SYS EQS LAST "from-cli"
ASSERT LAST_N == 1
SYS ENV "CUBALC_MI_UNSET_C"
SYS EQS LAST "keep"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# multiple -U
export CUBALC_MI_UNSET_X=1
export CUBALC_MI_UNSET_Y=2
OUT=$("$CUBALC" run -q -U CUBALC_MI_UNSET_X -U CUBALC_MI_UNSET_Y -e 'SYS ENV "CUBALC_MI_UNSET_X" OR "xgone"
SYS EQS LAST "xgone"
ASSERT LAST_N == 1
SYS ENV "CUBALC_MI_UNSET_Y" OR "ygone"
SYS EQS LAST "ygone"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# --unsetenv= form
export CUBALC_MI_UNSET_Z=z
OUT=$("$CUBALC" run -q --unsetenv=CUBALC_MI_UNSET_Z -e 'SYS ENV "CUBALC_MI_UNSET_Z" OR "cleared"
SYS EQS LAST "cleared"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# missing KEY arg hard-fails usage path
set +e
OUT=$("$CUBALC" run -q -U 2>&1)
RC=$?
set -e
test "$RC" -ne 0

# usage surfaces unsetenv
set +e
OUT=$("$CUBALC" run 2>&1)
set -e
printf '%s\n' "$OUT" | grep -qi 'unsetenv'

# env catalog
OUT=$("$CUBALC" env 2>&1)
printf '%s\n' "$OUT" | grep -q 'CUBALC_UNSETENV'

echo "1378_cli_run_unsetenv: PASS"
