#!/bin/sh
# LISTINCLUDES after cubalc run -I preload
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"

OUT=$("$CUBALC" run -q -I agent_boot -e 'LISTINCLUDES
ASSERT LAST_N == 1
HASINCLUDE agent_boot
ASSERT LAST_N == 1
SYS HAS LISTINCLUDES "agent_boot"
ASSERT LAST_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'
printf '%s\n' "$OUT" | grep -q '"preload_n":1'

# multi preload
OUT=$("$CUBALC" run -q -I hold_seed -I agent_boot -e 'LISTINCLUDES
ASSERT LAST_N == 2
HASINCLUDE hold_seed
ASSERT LAST_N == 1
HASINCLUDE agent_boot
ASSERT LAST_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"preload_n":2'

# HELP discovers forms
OUT=$("$CUBALC" forms LISTINCLUDE 2>&1 || true)
printf '%s\n' "$OUT" | grep -qi 'LISTINCLUDE\|INCLUDE'

echo "1261_cli_listincludes: PASS"
