#!/usr/bin/env bash
# proof: science pack/loop surfaces B/E/C/D deep fields (science_loop polish)
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BIN=${CUBALC_BIN:-"$ROOT/out/cubalc"}
# pack may print prove plate then pack plate — take last JSON object
raw=$("$BIN" science pack --seed 42 --mesh 6 --viz-n 6 --steps 500 --out "$ROOT/state" 2>/dev/null)
out=$(echo "$raw" | grep '"schema":"cubalc.science_pack.v1"' | tail -1)
echo "$out" | grep -q '"schema":"cubalc.science_pack.v1"'
echo "$out" | grep -q '"ok":true'
echo "$out" | grep -q '"prove_ok":true'
echo "$out" | grep -q '"mesh_span_xyz"'
echo "$out" | grep -q '"mesh_edge_n"'
echo "$out" | grep -q '"rel_rapidity"'
echo "$out" | grep -q '"doppler_rec"'
echo "$out" | grep -q '"rms_r"'
echo "$out" | grep -q '"x_cross"'
echo "$out" | grep -q '"lobe_balance"'
echo "$out" | grep -q '"chaotic"'
echo "$out" | grep -q '"q_entropy_bits"'
echo "$out" | grep -q '"q_participation"'
echo "$out" | grep -q '"mesh_rms_r"'
echo "$out" | grep -q '"mesh_centroid"'
echo "$out" | grep -q '"doppler_app"'
echo "$out" | grep -q '"rel_proper_time"'
echo "$out" | grep -q '"z_cross"'
echo "$out" | grep -q 'science loop'
test -f "$ROOT/state/science_pack.json"
loop_raw=$("$BIN" science loop --seed 42 --loops 2 --steps 400 --out "$ROOT/state" 2>/dev/null)
loop=$(echo "$loop_raw" | grep '"schema":"cubalc.science_loop.v1"' | tail -1)
echo "$loop" | grep -q '"schema":"cubalc.science_loop.v1"'
echo "$loop" | grep -q '"rapidity"'
echo "$loop" | grep -q '"attractor"'
echo "$loop" | grep -q '"chaotic"'
echo "$loop" | grep -q '"modules"'
echo "$loop" | grep -q '"q_entropy_bits"'
echo "$loop" | grep -q '"q_participation"'
echo "$loop" > /tmp/sloop.json
python3 - <<'PY'
import json
d=json.load(open("/tmp/sloop.json"))
assert d["ok"] and d["q_entropy_bits"] >= 0 and d["q_participation"] >= 1.0-1e-9
# Bell+RZ prep is not pure 1-bit; just require finite positive support
assert d["q_participation"] <= (1 << 8) + 1e-9
print("loop_entropy_polish_ok", "H", round(d["q_entropy_bits"],4), "PR", round(d["q_participation"],4))
PY
echo "$out" > /tmp/spack.json
python3 - <<'PY'
import json
d=json.load(open("/tmp/spack.json"))
assert d["ok"] and d["prove_ok"]
assert d["mesh_edge_n"]>=6 and d["mesh_span_xyz"]>0
assert "rel_rapidity" in d and "doppler_rec" in d
assert d["rms_r"]>0 and 0<=d["lobe_balance"]<=1
assert "chaotic" in d and "x_cross" in d
# pack_surface_sync B/E/C/D
assert abs(d["q_entropy_bits"]-1.0)<1e-6 and abs(d["q_participation"]-2.0)<1e-6
assert d["mesh_rms_r"]>0 and d["mesh_rms_r"] < d["mesh_span_xyz"]+1e-9
c=d["mesh_centroid"]; assert "x" in c and "y" in c and "z" in c
assert d["doppler_app"]>0 and d["rel_proper_time"]>=0 and d["z_cross"]>=0
print("pack_quartet_ok", "edge", d["mesh_edge_n"], "H", d["q_entropy_bits"], "mesh_rms", round(d["mesh_rms_r"],3))
PY
echo "science_pack_quartet_ok"
