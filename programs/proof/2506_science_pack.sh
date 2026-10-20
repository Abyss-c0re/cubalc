#!/usr/bin/env bash
# proof: science pack surface plate — mesh vote F_main_cli_extract (no main thrash)
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BIN=${CUBALC_BIN:-"$ROOT/out/cubalc"}
out=$("$BIN" science pack --seed 42 --out "$ROOT/state" 2>/dev/null)
echo "$out" | grep -q '"schema":"cubalc.science_pack.v1"'
echo "$out" | grep -q '"ok":true'
echo "$out" | grep -q '"prove_ok":true'
echo "$out" | grep -q '"modules"'
echo "$out" | grep -q 'quantum'
echo "$out" | grep -q 'dim5_mesh'
echo "$out" | grep -q 'relativity_viz'
echo "$out" | grep -q 'chaos_attractor'
echo "$out" | grep -q '"cli"'
echo "$out" | grep -q '"matrix_word"'
test -f "$ROOT/state/science_pack.json"
# also via alias
out2=$("$BIN" science bundle --seed 42 2>/dev/null)
echo "$out2" | grep -q '"schema":"cubalc.science_pack.v1"'
echo "science_pack_ok"
