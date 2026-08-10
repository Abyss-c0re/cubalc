#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1373_firstarg.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "--a"
SYS ENV SET CUBALC_ARG1 "z.txt"
SYS ENV SET CUBALC_ARGC "2"
FIRSTARG
ASSERT FIRSTARG_HIT == 1
SYS EQS LAST "--a"
ASSERT LAST_N == 1
LASTARG
SYS EQS LAST "z.txt"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# live run args
OUT=$("$CUBALC" run -q -e 'FIRSTARG
ASSERT LAST_N == 1
SYS EQS LAST "--verbose"
ASSERT LAST_N == 1
LASTARG
SYS EQS LAST "file.txt"
ASSERT LAST_N == 1
PASS' -- --verbose file.txt 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# CLI dual
OUT=$("$CUBALC" firstarg -- --verbose mid tail 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.endarg.v1'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"which":"first"'
printf '%s\n' "$OUT" | grep -qF -- '--verbose'

OUT=$("$CUBALC" lastarg -- a b c 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.endarg.v1'
printf '%s\n' "$OUT" | grep -q '"which":"last"'
printf '%s\n' "$OUT" | grep -q '"value":"c"'

OUT=$("$CUBALC" firstarg 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.endarg.v1'
printf '%s\n' "$OUT" | grep -q '"hit":false'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'firstarg'

OUT=$("$CUBALC" run -q -e 'HASFORM "LASTARG"
ASSERT LAST_N == 1
RELATED "LASTARG"
SYS HASLINE LAST "NTHARG"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1373_cli_firstarg: PASS"
