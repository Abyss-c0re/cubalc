#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"

# -I loads modules → PRELOADMISS empty + NEEDPRELOAD ok
OUT=$("$CUBALC" run -q -I agent_boot -I hold_seed -e 'PRELOADMISS
ASSERT LAST_N == 0
PRELOADOK
ASSERT LAST_N == 1
NEEDPRELOAD
ASSERT OK == 1
LISTPRELOAD
ASSERT LAST_N == 2
INCLUDESTEMS
ASSERT LAST_N == 2
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# CHECKPRELOAD alias + PRELOAD_OK_N
OUT=$("$CUBALC" run -q -I hold_seed -e 'CHECKPRELOAD
ASSERT LAST_N == 0
ASSERT PRELOAD_OK_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# forms catalog
OUT=$("$CUBALC" forms PRELOAD 2>&1)
printf '%s\n' "$OUT" | grep -q 'PRELOADMISS\|PRELOADOK\|NEEDPRELOAD'

# NEEDPRELOAD fails when PRELOAD_ACTIVE names not loaded (in-lang request)
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_PRELOAD_ACTIVE "no_such_mod_zzz"
NEEDPRELOAD' 2>&1 || true)
printf '%s\n' "$OUT" | grep -qi 'NEEDPRELOAD\|missing\|no_such'
printf '%s\n' "$OUT" | grep -q '"ok":false\|"ok": false' || printf '%s\n' "$OUT" | grep -qi 'error\|fail\|NEEDPRELOAD'

echo "1270_cli_preloadmiss: PASS"
