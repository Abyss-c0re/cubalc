#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1363_hasargall.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "a"
SYS ENV SET CUBALC_ARG1 "b"
SYS ENV SET CUBALC_ARGC "2"
HASARGALL 0 1
ASSERT LAST_N == 1
HASARGALL 0 1 2
ASSERT LAST_N == 0
ASSERT ARGMISS_N == 1
SYS HASLINE ARGMISS "2"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDARGS fatal when missing
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARGC "0"
USAGE "tool file1 file2"
NEEDARGS 0 1
PASS' 2>&1 || true)
printf '%s\n' "$OUT" | grep -qiE 'NEEDARGS miss|ok.:false'

# real CLI positionals via run --
OUT=$("$CUBALC" run -q -e 'HASARGALL 0 1
ASSERT LAST_N == 1
NEEDARGS 0 1
ASSERT LAST_N == 1
PASS' -- a.txt b.txt 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'HASFORM HASARGALL
ASSERT LAST_N == 1
HASFORM NEEDARGS
ASSERT LAST_N == 1
RELATED HASARGALL
SYS HASLINE LAST "NEEDARGS"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" related HASARGALL 2>&1)
printf '%s\n' "$OUT" | grep -qiE 'NEEDARGS|HASARG|ARGMISS'

echo "1363_cli_hasargall: PASS"
