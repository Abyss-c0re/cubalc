#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1383_env_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'SYS ENV SET CUBALC_EG_A "1"
SYS ENV SET CUBALC_EG_B "2"
DEFAULT NEED_ENVS = "CUBALC_EG_A\nCUBALC_EG_B"
DEFAULT NEED_ENV_ANY = ""
DEFAULT ENV_GUARD_SOFT = 0
INCLUDE env_guard
ASSERT ENV_GUARD_OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss does not fatal
OUT=$("$CUBALC" run -q -e 'SYS ENV UNSET CUBALC_EG_MISS
DEFAULT NEED_ENVS = "CUBALC_EG_MISS"
DEFAULT ENV_GUARD_SOFT = 1
INCLUDE env_guard
ASSERT ENV_GUARD_OK == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard miss fatal
set +e
OUT=$("$CUBALC" run -q -e 'SYS ENV UNSET CUBALC_EG_MISS
DEFAULT NEED_ENVS = "CUBALC_EG_MISS"
DEFAULT ENV_GUARD_SOFT = 0
INCLUDE env_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDENVS|CUBALC_EG_MISS|ok.:false'

# hard ANY miss fatal
set +e
OUT=$("$CUBALC" run -q -e 'SYS ENV UNSET CUBALC_EG_X
SYS ENV UNSET CUBALC_EG_Y
DEFAULT NEED_ENVS = ""
DEFAULT NEED_ENV_ANY = "CUBALC_EG_X\nCUBALC_EG_Y"
DEFAULT ENV_GUARD_SOFT = 0
INCLUDE env_guard
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDENVANY|need one of|ok.:false'

OUT=$("$CUBALC" which env_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'env_guard'

OUT=$("$CUBALC" recipe env_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'env_guard'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_env_guard":true'

OUT=$("$CUBALC" libs env_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'env_guard'

echo "1383_cli_env_guard: PASS"
