#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" related HASFORM 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.related.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"form":"HASFORM"'
printf '%s\n' "$OUT" | grep -q 'NEEDFORM'
printf '%s\n' "$OUT" | grep -q 'FORMHINT\|LISTFORMS'

OUT=$("$CUBALC" seealso SETP 2>&1)
printf '%s\n' "$OUT" | grep -q '"form":"SETP"'
printf '%s\n' "$OUT" | grep -q 'GETP\|NEEDP\|SAVEPLATE'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'related'

OUT=$("$CUBALC" forms RELATED 2>&1)
printf '%s\n' "$OUT" | grep -qi 'RELATED'

OUT=$("$CUBALC" run -q -e 'RELATED HASFORM
ASSERT RELATED_N >= 5
SYS HASI "NEEDFORMS"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# miss
set +e
OUT=$("$CUBALC" related ZZZ_NOPE 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$rc" -ne 0

echo "1331_cli_related: PASS"
