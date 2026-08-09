#!/bin/sh
# cubalc doctor — lib layout + nest-check readiness (agent install plate)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" doctor)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.doctor.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"libs_dir":"programs/lib"'
printf '%s\n' "$OUT" | grep -q '"libs_dir_ok":true'
printf '%s\n' "$OUT" | grep -q '"libs_n":'
# at least agent_boot + plate_uniform present
printf '%s\n' "$OUT" | grep -q '"lib_agent_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_plate_session":true'
printf '%s\n' "$OUT" | grep -q '"lib_plate_uniform":true'
printf '%s\n' "$OUT" | grep -q '"lib_hold_seed":true'
printf '%s\n' "$OUT" | grep -q '"docs_cookbook":true'
printf '%s\n' "$OUT" | grep -q '"docs_for_agents":true'
printf '%s\n' "$OUT" | grep -q 'plate_uniform'
printf '%s\n' "$OUT" | grep -q 'nest_check'
printf '%s\n' "$OUT" | grep -q 'plate uniform'
printf '%s\n' "$OUT" | grep -q '"version":"1.15.'
# libs_n >= 16
n=$(printf '%s\n' "$OUT" | sed -n 's/.*"libs_n":\([0-9][0-9]*\).*/\1/p' | head -1)
test -n "$n"
test "$n" -ge 16

# health alias
OUT2=$("$CUBALC" health)
printf '%s\n' "$OUT2" | grep -q '"schema":"cubalc.doctor.v1"'
printf '%s\n' "$OUT2" | grep -q '"lib_plate_uniform":true'

echo "1254_cli_doctor_libs: PASS"
