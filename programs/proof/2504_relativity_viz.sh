#!/usr/bin/env bash
# proof: relativity light-cone viz — rapidity/Doppler/proper_time (C_relativity_viz)
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BIN=${CUBALC_BIN:-"$ROOT/out/cubalc"}
out=$("$BIN" science relativity --beta 0.6 --seed 7 --viz --viz-n 8 --out "$ROOT/state" 2>/dev/null)
echo "$out" | grep -q '"schema":"cubalc.relativity_viz.v1"'
echo "$out" | grep -q '"ok":true'
echo "$out" | grep -q '"n":8'
echo "$out" | grep -q '"gamma"'
echo "$out" | grep -q '"lab"'
echo "$out" | grep -q '"boosted"'
echo "$out" | grep -q '"null_cone"'
echo "$out" | grep -q '"rapidity"'
echo "$out" | grep -q '"doppler_rec"'
echo "$out" | grep -q '"doppler_app"'
echo "$out" | grep -q '"proper_time"'
echo "$out" | grep -q '"worldline_causal":true'
echo "$out" | grep -q '"inside_cone":1'
echo "$out" | grep -q '"tau"'
echo "$out" | grep -q '"gamma":1.25'
echo "$out" > /tmp/rel_viz_out.json
python3 - <<'PY'
import json, math
d=json.load(open("/tmp/rel_viz_out.json"))
assert abs(d["gamma"]-1.25)<1e-12
assert abs(d["doppler_rec"]-0.5)<1e-12
assert abs(d["doppler_app"]-2.0)<1e-12
assert abs(d["rapidity"]-math.atanh(0.6))<1e-12
assert d["worldline_causal"] is True
assert d["proper_time"]>0
assert all(p.get("inside_cone")==1 for p in d["lab"])
print("rel_numerics_ok")
PY
# C residual: base relativity plate surfaces rapidity/doppler/proper_time
base=$("$BIN" science relativity --beta 0.6 --seed 7 2>/dev/null | head -1)
echo "$base" | grep -q '"schema":"cubalc.relativity.v1"'
echo "$base" | grep -q '"rapidity"'
echo "$base" | grep -q '"doppler_rec"'
echo "$base" | grep -q '"doppler_app"'
echo "$base" | grep -q '"proper_time"'
echo "$base" | grep -q '"dilate_ratio"'
echo "$base" | grep -q '"contract_ratio"'
echo "$base" > /tmp/rel_base.json
python3 - <<'PY'
import json, math
d=json.load(open("/tmp/rel_base.json"))
assert d["ok"] and abs(d["gamma"]-1.25)<1e-12
assert abs(d["rapidity"]-math.atanh(0.6))<1e-12
assert abs(d["doppler_rec"]-0.5)<1e-12
assert abs(d["doppler_app"]-2.0)<1e-12
assert abs(d["dilate_ratio"]-d["gamma"])<1e-15
assert abs(d["contract_ratio"]-0.8)<1e-12
assert d["proper_time"] == d["event"]["t"] / d["gamma"] or abs(d["proper_time"] - d["event"]["t"]/d["gamma"])<1e-9
print("rel_base_plate_ok", "φ", d["rapidity"])
PY
echo "relativity_viz_ok"
