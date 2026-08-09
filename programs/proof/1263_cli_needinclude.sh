#!/bin/sh
# NEEDINCLUDE after cubalc run -I · soft HASINCLUDEALL plate
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"

# -I then NEEDINCLUDE ok
OUT=$("$CUBALC" run -q -I agent_boot -e 'NEEDINCLUDE agent_boot
PASS
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"includes_n":1'

# multi -I + NEEDINCLUDE both
OUT=$("$CUBALC" run -q -I hold_seed -I agent_boot -e 'NEEDINCLUDE hold_seed agent_boot
HASINCLUDEALL hold_seed agent_boot
ASSERT LAST_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"includes_n":2'

# NEEDINCLUDE miss → fail plate
OUT=$("$CUBALC" run -q -e 'NEEDINCLUDE agent_boot
PASS
' 2>&1 || true)
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -q 'NEEDINCLUDE'
printf '%s\n' "$OUT" | grep -qi 'why_hint'

# soft HASINCLUDEALL does not fail process
OUT=$("$CUBALC" run -q -e 'HASINCLUDEALL missing_lib_zzz
ASSERT LAST_N == 0
PASS
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1263_cli_needinclude: PASS"
