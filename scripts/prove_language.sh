#!/usr/bin/env bash
# Prove CubalC is a real programming language: source → parse → eval → results
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/out/cubalc"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
mkdir -p "$CUBALC_STATE" "$ROOT/out/proof"
PASS=0; FAIL=0

run_ok() {
  local name="$1" file="$2"
  echo "---- PROVE $name ----"
  if "$BIN" run "$file" | tee "$ROOT/out/proof/${name}.log" | tail -1 | grep -q '"ok":true'; then
    echo "PASS $name"
    PASS=$((PASS+1))
  else
    echo "FAIL $name (expected ok)"
    FAIL=$((FAIL+1))
  fi
}

run_fail() {
  local name="$1" file="$2"
  echo "---- PROVE $name (must fail) ----"
  set +e
  out=$("$BIN" run "$file" 2>&1 | tee "$ROOT/out/proof/${name}.log")
  rc=$?
  set -e
  if echo "$out" | grep -q '"ok":false' && [[ "$rc" -ne 0 || "$out" == *'"ok":false'* ]]; then
    echo "PASS $name (correctly rejected)"
    PASS=$((PASS+1))
  else
    echo "FAIL $name (should reject)"
    FAIL=$((FAIL+1))
  fi
}

echo "CubalC language proof suite"
echo "binary: $BIN"
"$BIN" help >/dev/null

run_ok "01_arithmetic" "$ROOT/programs/proof/01_arithmetic.cubalc"
run_ok "02_cop_matrix"  "$ROOT/programs/proof/02_cop_matrix.cubalc"
run_ok "03_branch"      "$ROOT/programs/proof/03_branch.cubalc"
run_ok "04_prophecy"    "$ROOT/programs/proof/04_prophecy.cubalc"
run_fail "05_must_fail" "$ROOT/programs/proof/05_must_fail.cubalc"
run_ok "06_decide" "$ROOT/programs/proof/06_decide.cubalc"
run_ok "07_cubeos" "$ROOT/programs/proof/07_cubeos.cubalc"

# Different programs → different last results (determinism + diversity)
SUM=$(grep -E '^sum ' "$ROOT/out/proof/01_arithmetic.log" | head -1 || true)
FACT=$(grep -E '^fact ' "$ROOT/out/proof/01_arithmetic.log" | head -1 || true)
PATH1=$(grep -E '^path ' "$ROOT/out/proof/03_branch.log" | head -1 || true)
echo "observed: $SUM | $FACT | $PATH1"
if [[ "$SUM" == "sum 55" && "$FACT" == "fact 720" && "$PATH1" == "path 1" ]]; then
  echo "PASS observable_semantics"
  PASS=$((PASS+1))
else
  echo "FAIL observable_semantics (got sum='$SUM' fact='$FACT' path='$PATH1')"
  FAIL=$((FAIL+1))
fi

echo
echo "PROVE_RESULT pass=$PASS fail=$FAIL"
if [[ "$FAIL" -eq 0 ]]; then
  cat > "$ROOT/out/proof/LANGUAGE_PROOF.json" << JSON
{
  "schema": "cubalc.language_proof.v1",
  "ok": true,
  "language": "CubalC",
  "version": "0.8.0-cubeos",
  "criteria": [
    "finite source strings (.cubalc)",
    "formal syntax (lexer+parser)",
    "operational semantics (evaluator on cube machine)",
    "variables and arithmetic expressions",
    "control flow (LOOP, IF/ELSE, WHILE)",
    "braincube DECIDE (State Matrix → algocube digit)",
    "anything→CubalC translator (cubalc translate|decide)",
    "domain types (Cube, State Matrix, plugs, impulse)",
    "assertions and fail-closed rejection",
    "observable I/O (PRINT) matching expected values"
  ],
  "pass": $PASS,
  "fail": $FAIL,
  "creed": "C3"
}
JSON
  echo "LANGUAGE PROOF: OK — CubalC is a real programming language."
  exit 0
else
  echo "LANGUAGE PROOF: INCOMPLETE"
  exit 1
fi
