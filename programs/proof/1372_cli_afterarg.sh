#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1372_afterarg.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "--out"
SYS ENV SET CUBALC_ARG1 "x.txt"
SYS ENV SET CUBALC_ARGC "2"
AFTERARG "--out"
ASSERT AFTERARG_HIT == 1
SYS EQS LAST "x.txt"
ASSERT LAST_N == 1
AFTERARG "missing" OR "fb"
ASSERT AFTERARG_HIT == 0
SYS EQS LAST "fb"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# live run args
OUT=$("$CUBALC" run -q -e 'AFTERARG "--out"
ASSERT AFTERARG_HIT == 1
SYS EQS LAST "live.txt"
ASSERT LAST_N == 1
PASS' -- --verbose --out live.txt 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# CLI dual
OUT=$("$CUBALC" afterarg --out -- --verbose --out report.json 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.afterarg.v1'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q 'report.json'

OUT=$("$CUBALC" nextarg run -- --v run script.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.afterarg.v1'
printf '%s\n' "$OUT" | grep -q 'script.cubalc'

OUT=$("$CUBALC" afterarg missing -- a b 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.afterarg.v1'
printf '%s\n' "$OUT" | grep -q '"hit":false'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'afterarg'

OUT=$("$CUBALC" run -q -e 'HASFORM "AFTERARG"
ASSERT LAST_N == 1
RELATED "AFTERARG"
SYS HASLINE LAST "NTHARG"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1372_cli_afterarg: PASS"
