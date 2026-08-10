#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1362_hasflagall.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "--a"
SYS ENV SET CUBALC_ARG1 "--b"
SYS ENV SET CUBALC_ARGC "2"
HASFLAGALL a b
ASSERT LAST_N == 1
HASFLAGALL a b c
ASSERT LAST_N == 0
ASSERT FLAGMISS_N == 1
SYS HASLINE FLAGMISS "c"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDFLAGS fatal when missing
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARGC "0"
USAGE "tool --need-x"
NEEDFLAGS need-x
PASS' 2>&1 || true)
printf '%s\n' "$OUT" | grep -qiE 'NEEDFLAGS miss|need-x|ok.:false'

OUT=$("$CUBALC" run -q -e 'HASFORM HASFLAGALL
ASSERT LAST_N == 1
HASFORM NEEDFLAGS
ASSERT LAST_N == 1
RELATED HASFLAGALL
SYS HASLINE LAST "NEEDFLAGS"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" related HASFLAGALL 2>&1)
printf '%s\n' "$OUT" | grep -qiE 'NEEDFLAGS|HASFLAG|FLAGMISS'

# real CLI args via run --
OUT=$("$CUBALC" run -q -e 'HASFLAGALL verbose out
ASSERT LAST_N == 1
NEEDFLAGS verbose out
ASSERT LAST_N == 1
PASS' -- --verbose --out=x 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1362_cli_hasflagall: PASS"
