#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1382_hasenvany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_HENVANY_A "a"
HASENVANY CUBALC_HENVANY_A CUBALC_HENVANY_B
ASSERT LAST_N == 1
ASSERT HASENVANY_N == 1
HASENVANY CUBALC_HENVANY_MISS_X CUBALC_HENVANY_MISS_Y
ASSERT LAST_N == 0
ASSERT ENVMISS_N == 2
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDENVANY fatal when none present
OUT=$("$CUBALC" run -q -e 'USAGE "need DB or REDIS"
NEEDENVANY CUBALC_HENVANY_MISS_X CUBALC_HENVANY_MISS_Y
PASS' 2>&1 || true)
printf '%s\n' "$OUT" | grep -qiE 'NEEDENVANY miss|need one of|ok.:false'

# NEEDENVANY passes with any one
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_HENVANY_A "a"
NEEDENVANY CUBALC_HENVANY_A CUBALC_HENVANY_B
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'HASFORM HASENVANY
ASSERT LAST_N == 1
HASFORM NEEDENVANY
ASSERT LAST_N == 1
RELATED HASENVANY
SYS HASLINE LAST "NEEDENVANY"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" related HASENVANY 2>&1)
printf '%s\n' "$OUT" | grep -qiE 'NEEDENVANY|HASENV|ENVHAVE'

# CLI duals
export CUBALC_HENVANY_A=a
OUT=$("$CUBALC" hasenvany CUBALC_HENVANY_A CUBALC_HENVANY_MISS_Z 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.envgate.v1'
printf '%s\n' "$OUT" | grep -q '"any":true'
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" needenvany CUBALC_HENVANY_MISS_X CUBALC_HENVANY_MISS_Y 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -q '"mode":"need"'

OUT=$("$CUBALC" needenvany CUBALC_HENVANY_A CUBALC_HENVANY_MISS_Z 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'hasenvany'

echo "1382_cli_hasenvany: PASS"
