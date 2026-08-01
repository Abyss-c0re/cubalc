#!/usr/bin/env bash
# CubalC School — run entire pure-science curriculum (original content)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state/school}"
mkdir -p "$CUBALC_STATE"
if [[ ! -x "$BIN" ]]; then
  make -C "$ROOT" all
fi
pass=0; fail=0
run() {
  local f="$1"
  local name="${f#$ROOT/}"
  if "$BIN" run "$f" >"$CUBALC_STATE/last.out" 2>&1; then
    if grep -q '"ok":true' "$CUBALC_STATE/last.out"; then
      echo "PASS $name"; pass=$((pass+1)); return
    fi
  fi
  echo "FAIL $name"; tail -8 "$CUBALC_STATE/last.out" || true; fail=$((fail+1))
}
echo "=== CubalC School Pure Science ==="
echo "bin=$BIN version=$($BIN law 2>/dev/null | head -c 80 || true)"
mapfile -t LESSONS < <(find "$ROOT/programs/school" -name '*.cubalc' | sort)
for f in "${LESSONS[@]}"; do
  run "$f"
done
echo "SCHOOL pass=$pass fail=$fail total=$((pass+fail))"
[[ "$fail" -eq 0 ]]
