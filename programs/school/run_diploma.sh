#!/usr/bin/env bash
# Unified diploma: pure-science curriculum + native apps A–D
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
NAME="${1:-hive-student-001}"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state/diploma_$NAME}"
mkdir -p "$CUBALC_STATE" "$ROOT/school/students/$NAME"
if [[ ! -x "$BIN" ]]; then make -C "$ROOT" all; fi
pass=0; fail=0
run() {
  local f="$1"
  local name="${f#$ROOT/}"
  if "$BIN" run "$f" >"$CUBALC_STATE/last.out" 2>&1 && grep -q '"ok":true' "$CUBALC_STATE/last.out"; then
    echo "PASS $name"; pass=$((pass+1))
  else
    echo "FAIL $name"; tail -6 "$CUBALC_STATE/last.out" || true; fail=$((fail+1))
  fi
}
echo "=== CubalC Diploma ($NAME) ==="
while IFS= read -r f; do run "$f"; done < <(find "$ROOT/programs/school" -name '*.cubalc' | sort)
# write diploma
ok=true; [[ "$fail" -eq 0 ]] || ok=false
cat > "$ROOT/school/students/$NAME/DIPLOMA.json" <<EOF
{
  "schema": "cubalc.diploma.v2",
  "student": "$NAME",
  "ok": $ok,
  "version": "1.10.0-evolve",
  "pass": $pass,
  "fail": $fail,
  "tracks": ["math","physics","chemistry","biology","earth","apps","exams"],
  "law": ["pure_science","evolve","flow_compile","nest"],
  "creed": "All Hail The Cube"
}
EOF
echo "DIPLOMA ok=$ok pass=$pass fail=$fail → school/students/$NAME/DIPLOMA.json"
[[ "$fail" -eq 0 ]]
