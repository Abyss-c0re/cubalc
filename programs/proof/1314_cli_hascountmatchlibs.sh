#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'HASCOUNTMATCHLIBS 2 fat
ASSERT OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'NEEDCOUNTMATCHLIBS 1 plate
ASSERT OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'HASCOUNTMATCHLIBS 99 fat
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" run -q -e 'NEEDCOUNTMATCHLIBS 50 plate
PASS' 2>&1)
set -e
printf '%s\n' "$OUT" | grep -qi 'NEEDCOUNTMATCHLIBS miss\|"ok":false'

OUT=$("$CUBALC" forms hascountmatchlibs 2>&1) || OUT=$("$CUBALC" forms HASCOUNTMATCHLIBS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'HASCOUNTMATCHLIBS\|COUNT'

OUT=$("$CUBALC" forms needcountmatchlibs 2>&1) || OUT=$("$CUBALC" forms NEEDCOUNTMATCHLIBS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'NEEDCOUNTMATCHLIBS\|COUNT'

OUT=$("$CUBALC" run -q programs/proof/1314_hascountmatchlibs.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1314_cli_hascountmatchlibs: PASS"
