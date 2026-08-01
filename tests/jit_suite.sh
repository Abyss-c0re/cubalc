#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/out/cubalc"
cd "$ROOT"
pass=0; fail=0
run() {
  local name="$1"; shift
  if "$@" >/tmp/jit_suite.out 2>&1 && grep -q '"ok":true' /tmp/jit_suite.out; then
    echo "PASS $name"; pass=$((pass+1))
  else
    echo "FAIL $name"; tail -5 /tmp/jit_suite.out; fail=$((fail+1))
  fi
}
run compile_arith "$BIN" compile programs/proof/01_arithmetic.cubalc -o out/01_arithmetic.cblc
run jit_arith "$BIN" jit out/01_arithmetic.cblc
run manifest_flow "$BIN" manifest programs/flow/cube_flow.cubalc out/cube_flow_self.cblc
run jit_cblc "$BIN" jit out/cube_flow_self.cblc
echo "JIT_SUITE pass=$pass fail=$fail backend=$($BIN help 2>&1 | head -1)"
[[ "$fail" -eq 0 ]]
