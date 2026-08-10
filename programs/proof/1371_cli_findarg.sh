#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1371_findarg.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "--a"
SYS ENV SET CUBALC_ARG1 "f.txt"
SYS ENV SET CUBALC_ARGC "2"
FINDARG "--a"
ASSERT FINDARG_HIT == 1
ASSERT LAST_N == 0
FINDARG "f.txt"
ASSERT LAST_N == 1
FINDARG "nope"
ASSERT FINDARG_HIT == 0
ASSERT LAST_N == -1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# live run args
OUT=$("$CUBALC" run -q -e 'FINDARG "--verbose"
ASSERT FINDARG_HIT == 1
ASSERT LAST_N == 0
FINDARG "file.txt"
ASSERT LAST_N == 1
PASS' -- --verbose file.txt 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# CLI dual
OUT=$("$CUBALC" findarg --verbose -- --verbose file1 file2 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.findarg.v1'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"index":0'

OUT=$("$CUBALC" argindex file2 -- --verbose file1 file2 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.findarg.v1'
printf '%s\n' "$OUT" | grep -q '"index":2'
printf '%s\n' "$OUT" | grep -q '"hit":true'

OUT=$("$CUBALC" findarg missing -- a b 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.findarg.v1'
printf '%s\n' "$OUT" | grep -q '"hit":false'
printf '%s\n' "$OUT" | grep -q '"index":-1'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'findarg'

OUT=$("$CUBALC" run -q -e 'HASFORM "FINDARG"
ASSERT LAST_N == 1
RELATED "FINDARG"
SYS HASLINE LAST "ARGMAP"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1371_cli_findarg: PASS"
