#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1370_ntharg.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "--a"
SYS ENV SET CUBALC_ARG1 "f.txt"
SYS ENV SET CUBALC_ARGC "2"
NTHARG 0
ASSERT LAST_N == 1
SYS EQS LAST "--a"
ASSERT LAST_N == 1
NTHARG 1
SYS EQS LAST "f.txt"
ASSERT LAST_N == 1
NTHARG 5 OR "miss"
ASSERT LAST_N == 0
SYS EQS LAST "miss"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# live run args
OUT=$("$CUBALC" run -q -e 'NTHARG 0
ASSERT LAST_N == 1
SYS EQS LAST "--verbose"
ASSERT LAST_N == 1
NTHARG 1
SYS EQS LAST "file.txt"
ASSERT LAST_N == 1
PASS' -- --verbose file.txt 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'HASFORM "NTHARG"
ASSERT LAST_N == 1
RELATED "NTHARG"
SYS HASLINE LAST "NTHPOS"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1370_cli_ntharg: PASS"
