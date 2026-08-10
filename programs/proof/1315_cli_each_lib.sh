#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'LET n = 0
EACH LIB MATCH fat
  LET n = n + 1
END
ASSERT n == 2
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'LET n = 0
EACH LIB MATCH plate
  LET n = n + 1
END
ASSERT n >= 10
ASSERT EACHLIBS_N >= 10
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms "each lib" 2>&1) || OUT=$("$CUBALC" forms EACH 2>&1)
printf '%s\n' "$OUT" | grep -qi 'EACH LIB\|LIB'

OUT=$("$CUBALC" run -q programs/proof/1315_each_lib.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1315_cli_each_lib: PASS"
