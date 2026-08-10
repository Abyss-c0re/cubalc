#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR="${TMPDIR:-/tmp}"
WDIR="$TMPDIR/cubalc_mi_cwd_$$"
mkdir -p "$WDIR"
trap 'rm -rf "$WDIR"' EXIT

# no trailing newline — SYS EQS exact match
printf 'plate-ok' > "$WDIR/probe.txt"

# --cwd: relative READ under workdir
OUT=$("$CUBALC" run -q --cwd "$WDIR" -e 'SYS CWD
SYS HAS LAST "cubalc_mi_cwd"
ASSERT LAST_N == 1
SYS READ "probe.txt"
SYS EQS LAST "plate-ok"
ASSERT LAST_N == 1
SYS ENV "CUBALC_CWD_ACTIVE"
SYS HAS LAST "cubalc_mi_cwd"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# --cd= form
OUT=$("$CUBALC" run -q --cd="$WDIR" -e 'SYS READ "probe.txt"
SYS EQS LAST "plate-ok"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# CUBALC_CWD env dual
OUT=$(CUBALC_CWD="$WDIR" "$CUBALC" run -q -e 'SYS READ "probe.txt"
SYS EQS LAST "plate-ok"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# write relative plate under cwd
OUT=$("$CUBALC" run -q --cwd "$WDIR" -e 'SYS WRITE "out_mi.txt" "from-cwd"
ASSERT OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
test -f "$WDIR/out_mi.txt"
grep -q 'from-cwd' "$WDIR/out_mi.txt"

# missing dir hard-fails
set +e
OUT=$("$CUBALC" run -q --cwd /no/such/cubalc_cwd_mi_$$ -e 'PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -q 'CWD failed'
printf '%s\n' "$OUT" | grep -q '"ok":false'

# usage surfaces cwd
set +e
OUT=$("$CUBALC" run 2>&1)
set -e
printf '%s\n' "$OUT" | grep -qi 'cwd'

# env catalog
OUT=$("$CUBALC" env 2>&1)
printf '%s\n' "$OUT" | grep -q 'CUBALC_CWD'

# relative program path still works with --cwd (src abs-ized first)
printf 'SYS READ "probe.txt"\nSYS EQS LAST "plate-ok"\nASSERT LAST_N == 1\nPASS\n' > "$ROOT/programs/proof/_tmp_cwd_prog_$$.cubalc"
OUT=$("$CUBALC" run -q --cwd "$WDIR" "programs/proof/_tmp_cwd_prog_$$.cubalc" 2>&1) || true
rm -f "$ROOT/programs/proof/_tmp_cwd_prog_$$.cubalc"
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1375_cli_run_cwd: PASS"
