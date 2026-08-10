#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1380_hasargany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "a"
SYS ENV SET CUBALC_ARGC "1"
HASARGANY 0 1
ASSERT LAST_N == 1
HASARGANY 0 1 2
ASSERT LAST_N == 1
ASSERT HASARGANY_N == 1
HASARGANY 9 8
ASSERT LAST_N == 0
ASSERT ARGMISS_N == 2
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDARGANY fatal when none present
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARGC "0"
USAGE "tool file|HOST"
NEEDARGANY 0 HOST
PASS' 2>&1 || true)
printf '%s\n' "$OUT" | grep -qiE 'NEEDARGANY miss|need one of|ok.:false'

# NEEDARGANY passes with any one
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "a.txt"
SYS ENV SET CUBALC_ARGC "1"
NEEDARGANY 0 1 HOST
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'HASFORM HASARGANY
ASSERT LAST_N == 1
HASFORM NEEDARGANY
ASSERT LAST_N == 1
RELATED HASARGANY
SYS HASLINE LAST "NEEDARGANY"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" related HASARGANY 2>&1)
printf '%s\n' "$OUT" | grep -qiE 'NEEDARGANY|HASARG|ARGHAVE'

# live run -- any-of positionals
OUT=$("$CUBALC" run -q -e 'HASARGANY 0 1
ASSERT LAST_N == 1
NEEDARGANY 0 1
ASSERT LAST_N == 1
PASS' -- a.txt 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'hasargany'

echo "1380_cli_hasargany: PASS"
