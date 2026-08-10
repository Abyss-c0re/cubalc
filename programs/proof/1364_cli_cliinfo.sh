#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1364_cliinfo.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "--verbose"
SYS ENV SET CUBALC_ARG1 "--out=x"
SYS ENV SET CUBALC_ARG2 "f.txt"
SYS ENV SET CUBALC_ARGC "3"
CLIINFO
ASSERT ARGC == 3
ASSERT LISTFLAGS_N == 2
ASSERT RESTARGS_N == 1
SYS HASI "cubalc.cli.v1"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# CLI dual with live -- args
OUT=$("$CUBALC" cliinfo -- --verbose --mode=fast file1 file2 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.cli.v1'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'verbose'
printf '%s\n' "$OUT" | grep -q 'file1'

OUT=$("$CUBALC" dumpcli -- -n 3 x.y 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.cli.v1'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'cliinfo'

OUT=$("$CUBALC" run -q -e 'HASFORM "CLIINFO"
ASSERT LAST_N == 1
RELATED "CLIINFO"
SYS HASLINE LAST "FLAGMAP"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1364_cli_cliinfo: PASS"
