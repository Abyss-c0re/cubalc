#!/usr/bin/env bash
# CubalC pure-science language demos — regression for SCIENCE surface (not a school)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state/science_demos}"
mkdir -p "$CUBALC_STATE"
[[ -x "$BIN" ]] || make -C "$ROOT" all
pass=0; fail=0
run() {
  local f="$1" name="${f#$ROOT/}"
  if "$BIN" run "$f" >"$CUBALC_STATE/last.out" 2>&1 && grep -q '"ok":true' "$CUBALC_STATE/last.out"; then
    echo "PASS $name"; pass=$((pass+1))
  else
    echo "FAIL $name"; tail -6 "$CUBALC_STATE/last.out" || true; fail=$((fail+1))
  fi
}
echo "=== CubalC pure-science language demos ==="
while IFS= read -r f; do run "$f"; done < <(find "$ROOT/programs/science" -name '*.cubalc' | sort)
echo "SCIENCE_DEMOS pass=$pass fail=$fail"
[[ "$fail" -eq 0 ]]
