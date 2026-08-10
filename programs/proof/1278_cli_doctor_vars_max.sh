#!/bin/sh
# cubalc doctor — vars_max / var budget fat-board readiness
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" doctor)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.doctor.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"vars_max":256'
printf '%s\n' "$OUT" | grep -q '"varroom_forms":true'
printf '%s\n' "$OUT" | grep -q 'VARROOM'
printf '%s\n' "$OUT" | grep -q 'HASVARROOM\|NEEDVARROOM\|var_budget'
printf '%s\n' "$OUT" | grep -q '"version":"1.15.'

# health alias
OUT2=$("$CUBALC" health)
printf '%s\n' "$OUT2" | grep -q '"vars_max":256'
printf '%s\n' "$OUT2" | grep -q '"varroom_forms":true'

# regression: libs still present
printf '%s\n' "$OUT" | grep -q '"lib_agent_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_plate_uniform":true'

echo "1278_cli_doctor_vars_max: PASS"
