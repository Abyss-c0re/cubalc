#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1369_argmap.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "--a"
SYS ENV SET CUBALC_ARG1 "f.txt"
SYS ENV SET CUBALC_ARGC "2"
ARGMAP
ASSERT LAST_N == 2
SYS HASLINE LAST "0=--a"
ASSERT LAST_N == 1
SYS HASLINE LAST "1=f.txt"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" argmap -- --verbose file1 file2 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.argmap.v1'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '0=--verbose'
printf '%s\n' "$OUT" | grep -q '1=file1'
printf '%s\n' "$OUT" | grep -q '"n":3'

OUT=$("$CUBALC" argkv -- x 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.argmap.v1'
printf '%s\n' "$OUT" | grep -q '0=x'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'argmap'

OUT=$("$CUBALC" run -q -e 'HASFORM "ARGMAP"
ASSERT LAST_N == 1
RELATED "ARGMAP"
SYS HASLINE LAST "CLIINFO"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1369_cli_argmap: PASS"
