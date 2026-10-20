#!/usr/bin/env bash
# proof: 5D seed mesh — ring/star/chord + span stats (E_5d_seed_mesh deepen)
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BIN=${CUBALC_BIN:-"$ROOT/out/cubalc"}
out=$("$BIN" science mesh --seed 42 --mesh 6 --out "$ROOT/state" 2>/dev/null)
echo "$out" | grep -q '"schema":"cubalc.dim5_mesh.v1"'
echo "$out" | grep -q '"ok":true'
echo "$out" | grep -q '"n":6'
echo "$out" | grep -q '"mesh_word"'
echo "$out" | grep -q '"nodes"'
echo "$out" | grep -q '"edges"'
echo "$out" | grep -q '"star":1'
echo "$out" | grep -q '"kind":"ring"'
echo "$out" | grep -q '"kind":"chord"'
echo "$out" | grep -q '"span_xyz"'
echo "$out" | grep -q '"mean_dt"'
echo "$out" | grep -q '"edge_n"'
echo "$out" | grep -q '"centroid"'
echo "$out" | grep -q '"rms_r"'
test -f "$ROOT/state/science_dim5_mesh.json"
# deterministic: same seed → same mesh_word
w1=$(echo "$out" | sed -n 's/.*"mesh_word":"\([^"]*\)".*/\1/p')
w2=$("$BIN" science dim5 --seed 42 --mesh 6 2>/dev/null | sed -n 's/.*"mesh_word":"\([^"]*\)".*/\1/p')
test -n "$w1" && test "$w1" = "$w2"
# edge_n = ring6 + star5 + chord6 = 17 for n=6
echo "$out" | grep -q '"edge_n":17'
echo "$out" > /tmp/d5mesh.json
python3 - <<'PY'
import json
d = json.load(open("/tmp/d5mesh.json"))
assert d["ok"] and d["n"] == 6 and d["edge_n"] == 17
assert d["span_xyz"] > 0 and d["rms_r"] > 0
c = d["centroid"]
assert "x" in c and "y" in c and "z" in c
# rms radius must be < diameter (span)
assert d["rms_r"] < d["span_xyz"] + 1e-9
print("dim5_centroid_rms_ok", "rms", d["rms_r"], "span", d["span_xyz"])
PY
echo "dim5_seed_mesh_ok"
