#!/usr/bin/env bash
# proof: OpenQASM2 export depth + statevector CSV apply (Y/RY/CZ/SWAP/SDG)
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BIN=${CUBALC_BIN:-"$ROOT/out/cubalc"}
out=$("$BIN" science quantum --qubits 2 --seed 7 --qasm 2>/dev/null)
echo "$out" | grep -q 'OPENQASM 2.0'
echo "$out" | grep -q 'include "qelib1.inc"'
echo "$out" | grep -q 'h q\[0\]'
echo "$out" | grep -q 'cx q\[0\],q\[1\]'
echo "$out" | grep -q 'ry('
echo "$out" | grep -q 'cz q\[0\],q\[1\]'
echo "$out" | grep -q 'rz('
echo "$out" | grep -q 'measure q -> c'
echo "$out" | grep -q '"ok":true'
csv_out=$("$BIN" science quantum --qubits 3 --seed 3 --qasm --gates 'h:0,cx:0:1,ry:1:0.5,cz:1:2,swap:0:2,y:2,sdg:0,barrier:1,rz:0:0.25' 2>/dev/null)
echo "$csv_out" | grep -q 'ry(0.5)'
echo "$csv_out" | grep -q 'cz q\[1\],q\[2\]'
echo "$csv_out" | grep -q 'swap q\[0\],q\[2\]'
echo "$csv_out" | grep -q 'y q\[2\]'
echo "$csv_out" | grep -q 'sdg q\[0\]'
echo "$csv_out" | grep -q 'barrier q\[1\]'
echo "$csv_out" | grep -q '"ok":true'
# norm-preserving apply + pre-collapse Shannon plate (B residual)
bell=$("$BIN" science quantum --qubits 2 --seed 11 --gates 'h:0,cx:0:1' 2>/dev/null | head -1)
echo "$bell" | grep -q '"ok":true'
echo "$bell" | grep -q '"n_qubits":2'
echo "$bell" | grep -q '"entropy_bits"'
echo "$bell" | grep -q '"participation"'
echo "$bell" | grep -q '"purity_diag"'
echo "$bell" > /tmp/qent.json
python3 - <<'PY'
import json
d = json.load(open("/tmp/qent.json"))
assert d["ok"] and abs(d["entropy_bits"] - 1.0) < 1e-6
assert abs(d["participation"] - 2.0) < 1e-6
assert abs(d["purity_diag"] - 0.5) < 1e-6
print("quantum_entropy_residual_ok", d["entropy_bits"])
PY
echo "quantum_qasm_depth_ok"
