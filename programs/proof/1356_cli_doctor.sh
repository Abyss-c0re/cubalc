#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.doctor.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'DOCTOR
ASSERT OK == 1
ASSERT DOCTOR_OK == 1
ASSERT DOCTOR_LIBS_N >= 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms DOCTOR 2>&1)
printf '%s\n' "$OUT" | grep -qi 'DOCTOR'

OUT=$("$CUBALC" run -q programs/proof/1356_doctor.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'doctor'

echo "1356_cli_doctor: PASS"
