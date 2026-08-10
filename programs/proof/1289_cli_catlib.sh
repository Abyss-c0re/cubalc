#!/bin/sh
# CATLIB dual of cubalc cat + forms
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'CATLIB agent_boot
ASSERT OK == 1
ASSERT LAST_N > 10
ASSERT LIB_STEM == "agent_boot"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'CATLIB missing_lib_qqq
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# compare with CLI cat presence of content
OUT=$("$CUBALC" cat fat_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'agent_boot\|var_guard\|NEED_VARROOM'

OUT=$("$CUBALC" forms catlib 2>&1) || OUT=$("$CUBALC" forms CATLIB 2>&1)
printf '%s\n' "$OUT" | grep -qi 'CATLIB'

OUT=$("$CUBALC" run -q programs/proof/1289_catlib.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1289_cli_catlib: PASS"
