#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR="${TMPDIR:-/tmp}"
ENVF="$TMPDIR/cubalc_mi_dotenv_$$.env"
trap 'rm -f "$ENVF"' EXIT

cat > "$ENVF" <<'EOF'
# meaningful_iter dotenv proof
CUBALC_MI_DOTENV_A=alpha
export CUBALC_MI_DOTENV_B="beta gamma"
CUBALC_MI_DOTENV_C=plain
EOF

# -F loads before body
OUT=$("$CUBALC" run -q -F "$ENVF" -e 'SYS ENV "CUBALC_MI_DOTENV_A"
SYS EQS LAST "alpha"
ASSERT LAST_N == 1
SYS ENV "CUBALC_MI_DOTENV_B"
SYS EQS LAST "beta gamma"
ASSERT LAST_N == 1
SYS ENV "CUBALC_MI_DOTENV_C"
SYS EQS LAST "plain"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# --dotenv= form
OUT=$("$CUBALC" run -q --dotenv="$ENVF" -e 'SYS ENV "CUBALC_MI_DOTENV_A"
SYS EQS LAST "alpha"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# CUBALC_DOTENV env dual + ACTIVE path published
OUT=$(CUBALC_DOTENV="$ENVF" "$CUBALC" run -q -e 'SYS ENV "CUBALC_MI_DOTENV_A"
SYS EQS LAST "alpha"
ASSERT LAST_N == 1
SYS ENV "CUBALC_DOTENV_ACTIVE"
SYS HAS LAST "cubalc_mi_dotenv"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# missing path hard-fails
set +e
OUT=$("$CUBALC" run -q -F /no/such/cubalc_dotenv_mi_$$.env -e 'PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -q 'DOTENV failed'
printf '%s\n' "$OUT" | grep -q '"ok":false'

# usage surfaces dotenv
set +e
OUT=$("$CUBALC" run 2>&1)
set -e
printf '%s\n' "$OUT" | grep -qi 'dotenv'

# env catalog
OUT=$("$CUBALC" env 2>&1)
printf '%s\n' "$OUT" | grep -q 'CUBALC_DOTENV'

# in-lang dual still works
OUT=$("$CUBALC" run -q -e "SYS DOTENV \"$ENVF\"
ASSERT DOTENV_N >= 3
SYS ENV \"CUBALC_MI_DOTENV_A\"
SYS EQS LAST \"alpha\"
ASSERT LAST_N == 1
PASS" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1374_cli_run_dotenv: PASS"
