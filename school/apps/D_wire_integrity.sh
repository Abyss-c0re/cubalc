#!/usr/bin/env bash
# App D — Wire integrity: SMX + CubeChain fail-closed
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/out/cubalc"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
mkdir -p "$CUBALC_STATE" "$(dirname "$0")/../students/${STUDENT:-anon}/apps"
OUT="$(dirname "$0")/../students/${STUDENT:-anon}/apps/D_wire_integrity.json"
SMX=$("$BIN" smx-selftest)
VER=$("$BIN" cubechain verify)
echo "$SMX" > "$OUT.smx"
echo "$VER" > "$OUT.verify"
python3 - <<PY
import json
smx=json.loads('''$SMX''')
ver=json.loads('''$VER''')
ok=bool(smx.get("ok") and ver.get("ok"))
doc={"app":"D_wire_integrity","ok":ok,"smx":smx,"cubechain":ver,"talk":"binary","hold_flash":1}
open("$OUT","w").write(json.dumps(doc,indent=2)+"\n")
print(json.dumps(doc))
raise SystemExit(0 if ok else 1)
PY
