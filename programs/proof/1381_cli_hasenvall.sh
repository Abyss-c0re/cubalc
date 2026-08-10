#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1381_hasenvall.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_HENVALL_A "a"
SYS ENV SET CUBALC_HENVALL_B "b"
HASENVALL CUBALC_HENVALL_A CUBALC_HENVALL_B
ASSERT LAST_N == 1
HASENVALL CUBALC_HENVALL_A CUBALC_HENVALL_B CUBALC_HENVALL_C
ASSERT LAST_N == 0
ASSERT ENVMISS_N == 1
SYS HASLINE ENVMISS "CUBALC_HENVALL_C"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDENVS fatal when missing
OUT=$("$CUBALC" run -q -e 'USAGE "need A B"
NEEDENVS CUBALC_HENVALL_MISS_X CUBALC_HENVALL_MISS_Y
PASS' 2>&1 || true)
printf '%s\n' "$OUT" | grep -qiE 'NEEDENVS miss|ok.:false'

# NEEDENVS passes when present
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_HENVALL_A "1"
SYS ENV SET CUBALC_HENVALL_B "2"
NEEDENVS CUBALC_HENVALL_A CUBALC_HENVALL_B
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'HASFORM HASENVALL
ASSERT LAST_N == 1
HASFORM NEEDENVS
ASSERT LAST_N == 1
RELATED HASENVALL
SYS HASLINE LAST "NEEDENVS"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" related HASENVALL 2>&1)
printf '%s\n' "$OUT" | grep -qiE 'NEEDENVS|HASENV|ENVMISS'

echo "1381_cli_hasenvall: PASS"
