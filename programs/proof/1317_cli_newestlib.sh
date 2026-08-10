#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'NEWESTLIB fat
ASSERT LAST == "fat_session"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'OLDESTLIB fat
ASSERT LAST == "fat_boot"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'NEWESTLIB miss OR hold_seed
ASSERT LAST == "hold_seed"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms newestlib 2>&1) || OUT=$("$CUBALC" forms NEWESTLIB 2>&1)
printf '%s\n' "$OUT" | grep -qi 'NEWESTLIB'

OUT=$("$CUBALC" forms oldestlib 2>&1) || OUT=$("$CUBALC" forms OLDESTLIB 2>&1)
printf '%s\n' "$OUT" | grep -qi 'OLDESTLIB'

OUT=$("$CUBALC" run -q programs/proof/1317_newestlib.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1317_cli_newestlib: PASS"
