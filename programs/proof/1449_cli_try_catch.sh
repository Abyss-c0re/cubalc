#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1449_try_catch.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms CATCH 2>&1)
printf '%s\n' "$OUT" | grep -qi CATCH

# regression TRY/FINALLY
OUT=$("$CUBALC" run -q programs/proof/1448_try_finally.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1449_cli_try_catch: PASS"
