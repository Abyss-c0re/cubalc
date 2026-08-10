#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1359_hasflags.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "--a"
SYS ENV SET CUBALC_ARG1 "--b"
SYS ENV SET CUBALC_ARGC "2"
HASFLAGS 2
ASSERT LAST_N == 1
HASFLAGS 3
ASSERT LAST_N == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms HASFLAGS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'HASFLAGS'

OUT=$("$CUBALC" forms HASFLAGC 2>&1)
printf '%s\n' "$OUT" | grep -qi 'HASFLAG'

echo "1359_cli_hasflags: PASS"
