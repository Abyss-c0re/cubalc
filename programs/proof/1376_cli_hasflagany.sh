#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1376_hasflagany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "--a"
SYS ENV SET CUBALC_ARG1 "--b"
SYS ENV SET CUBALC_ARGC "2"
HASFLAGANY a b
ASSERT LAST_N == 1
HASFLAGANY a b c
ASSERT LAST_N == 1
ASSERT HASFLAGANY_N == 2
HASFLAGANY x y z
ASSERT LAST_N == 0
ASSERT FLAGMISS_N == 3
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDFLAGANY fatal when none present
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARGC "0"
USAGE "tool --json|--yaml"
NEEDFLAGANY json yaml
PASS' 2>&1 || true)
printf '%s\n' "$OUT" | grep -qiE 'NEEDFLAGANY miss|need one of|ok.:false'

# NEEDFLAGANY passes with any one
OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_ARG0 "--yaml"
SYS ENV SET CUBALC_ARGC "1"
NEEDFLAGANY json yaml xml
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'HASFORM "HASFLAGANY"
ASSERT LAST_N == 1
HASFORM "NEEDFLAGANY"
ASSERT LAST_N == 1
RELATED "HASFLAGANY"
SYS HASLINE LAST "NEEDFLAGANY"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" related HASFLAGANY 2>&1)
printf '%s\n' "$OUT" | grep -qiE 'NEEDFLAGANY|HASFLAG|FLAGHAVE'

# CLI dual any-of
OUT=$("$CUBALC" hasflagany json yaml -- --yaml x 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.flaggate.v1'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"any":true'

OUT=$("$CUBALC" hasflagany json yaml -- plain.txt 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":false'

set +e
OUT=$("$CUBALC" needflagany json yaml -- plain.txt 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -q '"ok":false'

OUT=$("$CUBALC" needflagany json yaml -- --json a 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# live run --
OUT=$("$CUBALC" run -q -e 'HASFLAGANY verbose out
ASSERT LAST_N == 1
NEEDFLAGANY verbose out
ASSERT LAST_N == 1
PASS' -- --out=x 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'hasflagany'

echo "1376_cli_hasflagany: PASS"
