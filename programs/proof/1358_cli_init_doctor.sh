#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -qE '"n":(10|11|12)'
printf '%s\n' "$OUT" | grep -q 'doctor_boot'
printf '%s\n' "$OUT" | grep -q -- '--doctor'

OUT=$("$CUBALC" init "$TMPDIR/doctor_demo" --doctor --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"doctor_boot"'
test -f "$TMPDIR/doctor_demo.cubalc"
grep -q 'INCLUDE doctor_boot' "$TMPDIR/doctor_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/doctor_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_doctor_boot":true'

OUT=$("$CUBALC" recipe doctor_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'doctor_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '--doctor'

OUT=$("$CUBALC" run -q programs/proof/1358_doctor_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" which doctor_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'doctor_boot'

OUT=$("$CUBALC" init --need-doctor "$TMPDIR/nd" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"doctor_boot"'

OUT=$("$CUBALC" run -q -e 'INCLUDE doctor_boot
ASSERT DOCTOR_OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1358_cli_init_doctor: PASS"
