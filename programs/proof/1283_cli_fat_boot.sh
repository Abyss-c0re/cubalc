#!/bin/sh
# INCLUDE fat_boot + doctor lib_fat_boot + which/cat/libs
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'INCLUDE fat_boot
ASSERT OK == 1
ASSERT VARROOM >= 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'fat_boot\|agent_boot\|var_guard'

# preload dual
OUT=$("$CUBALC" run -q -I fat_boot -e 'ASSERT OK == 1
ASSERT VARROOM >= 1
STATUS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'fat_boot\|"preload'

# which/cat/libs
OUT=$("$CUBALC" which fat_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'fat_boot'
OUT=$("$CUBALC" cat fat_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'agent_boot\|var_guard\|NEED_VARROOM'
OUT=$("$CUBALC" libs 2>&1)
printf '%s\n' "$OUT" | grep -q 'fat_boot'

# doctor
OUT=$("$CUBALC" doctor)
printf '%s\n' "$OUT" | grep -q '"lib_fat_boot":true'
n=$(printf '%s\n' "$OUT" | sed -n 's/.*"libs_n":\([0-9][0-9]*\).*/\1/p' | head -1)
test -n "$n"
test "$n" -ge 19

# file proof
OUT=$("$CUBALC" run -q programs/proof/1283_fat_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1283_cli_fat_boot: PASS"
