#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/out/cubalc"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state/test_suite}"
export CUBALC_BIN="$BIN"
export HOME="${HOME:-/home/voldemar}"
mkdir -p "$CUBALC_STATE"
pass=0; fail=0
run() {
  local name="$1" file="$2"
  if "$BIN" run "$file" >/tmp/cubalc_suite.out 2>&1; then
    if grep -q '"ok":true' /tmp/cubalc_suite.out; then
      echo "PASS $name"; pass=$((pass+1)); return
    fi
  fi
  echo "FAIL $name"; tail -6 /tmp/cubalc_suite.out; fail=$((fail+1))
}
mkdir -p /tmp/cubalc_cases
cat > /tmp/cubalc_cases/arith.cubalc <<'E'
LET f=1
LET k=1
LOOP 6
  LET f=f*k
  LET k=k+1
END
ASSERT f==720
E
cat > /tmp/cubalc_cases/cubes.cubalc <<'E'
[hold]
[sensor:in]
[act:out]
[sensor~act]
[sensor!]
ASSERT CUBES==2
ASSERT SET(sensor)>0
E
cat > /tmp/cubalc_cases/sys_file.cubalc <<'E'
SYS WRITE "/tmp/cubalc_suite_w.txt" "ok-cubalc"
SYS READ "/tmp/cubalc_suite_w.txt"
ASSERT LAST_N>0
SYS EXIST "/tmp/cubalc_suite_w.txt"
ASSERT EXIST==1
E
cat > /tmp/cubalc_cases/sys_env.cubalc <<'E'
SYS ENV "HOME"
ASSERT LAST_N>0
E
cat > /tmp/cubalc_cases/sys_which_spawn.cubalc <<'E'
SYS WHICH cubalc
ASSERT LAST_N==1
SYS SPAWN cubalc help
ASSERT EXIT==0
E
cat > /tmp/cubalc_cases/sys_http.cubalc <<'E'
SYS HTTP GET "http://127.0.0.1:1212/v1/models"
ASSERT HTTP_CODE>=0
E
cat > /tmp/cubalc_cases/branch.cubalc <<'E'
LET x=3
IF x>2 THEN
  LET y=9
ELSE
  LET y=0
END
ASSERT y==9
E
cat > /tmp/cubalc_cases/async_par.cubalc <<'E'
HOLD_FLASH 1
GENESIS "NEXUS_COORD v1 | from=suite | hold_flash=1 |"
CUBE a ROLE t PROTON 1
CUBE b ROLE u PROTON 1
PLUG a b
PARALLEL FLOW 4
PARALLEL COMPAT
PRINT "async" WORKERS GPU CUBES
ASSERT WORKERS >= 1
ASSERT CUBES == 2
E
cat > /tmp/cubalc_cases/async_http.cubalc <<'E'
HOLD_FLASH 1
ASYNC HTTP GET "http://127.0.0.1:1212/v1/models"
AWAIT ASYNC_ID
PRINT "http" HTTP_CODE OK WORKERS
ASSERT WORKERS >= 1
E
run arith /tmp/cubalc_cases/arith.cubalc
run cubes /tmp/cubalc_cases/cubes.cubalc
run branch /tmp/cubalc_cases/branch.cubalc
run sys_file /tmp/cubalc_cases/sys_file.cubalc
run sys_env /tmp/cubalc_cases/sys_env.cubalc
run sys_which_spawn /tmp/cubalc_cases/sys_which_spawn.cubalc
run sys_http /tmp/cubalc_cases/sys_http.cubalc
run async_par /tmp/cubalc_cases/async_par.cubalc
run async_http /tmp/cubalc_cases/async_http.cubalc
echo "LANG_SUITE pass=$pass fail=$fail"
[[ "$fail" -eq 0 ]]
