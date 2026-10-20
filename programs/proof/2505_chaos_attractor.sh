#!/usr/bin/env bash
# proof: Lorenz attractor deepen — span/rms/poincaré/chaotic (D_chaos_attractor)
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BIN=${CUBALC_BIN:-"$ROOT/out/cubalc"}
out=$("$BIN" science attractor --seed 7 --steps 500 --out "$ROOT/state" 2>/dev/null)
echo "$out" | grep -q '"schema":"cubalc.chaos_attractor.v1"'
echo "$out" | grep -q '"ok":true'
echo "$out" | grep -q '"system":"lorenz1963"'
echo "$out" | grep -q '"bbox"'
echo "$out" | grep -q '"centroid"'
echo "$out" | grep -q '"lobe_pos"'
echo "$out" | grep -q '"lobe_neg"'
echo "$out" | grep -q '"z_cross"'
echo "$out" | grep -q '"lyapunov"'
echo "$out" | grep -q '"span"'
echo "$out" | grep -q '"rms_r"'
echo "$out" | grep -q '"lobe_balance"'
echo "$out" | grep -q '"x_cross"'
echo "$out" | grep -q '"chaotic":true'
test -f "$ROOT/state/science_chaos_attractor.json"
echo "$out" | grep -q '"path_n"'
w1=$(echo "$out" | sed -n 's/.*"matrix_word":"\([^"]*\)".*/\1/p')
w2=$("$BIN" science chaos --seed 7 --steps 500 --attractor 2>/dev/null | sed -n 's/.*"matrix_word":"\([^"]*\)".*/\1/p')
test -n "$w1" && test "$w1" = "$w2"
echo "$out" > /tmp/chaos_attr.json
python3 - <<'PY'
import json
d=json.load(open("/tmp/chaos_attr.json"))
assert d["ok"] and d["chaotic"] is True
assert d["lyapunov"] > 0
assert d["path_n"] >= 8
assert d["span"]["x"] > 0 and d["span"]["y"] > 0 and d["span"]["z"] > 0
assert d["rms_r"] > 0
assert 0.0 <= d["lobe_balance"] <= 1.0
assert d["lobe_pos"] + d["lobe_neg"] == d["path_n"]
print("chaos_numerics_ok", "lyap", round(d["lyapunov"],4), "x_cross", d["x_cross"])
PY
# D residual: base chaos.v1 surfaces chaotic/rms/lobe/poincaré
base=$("$BIN" science chaos --seed 7 --steps 500 2>/dev/null | head -1)
echo "$base" | grep -q '"schema":"cubalc.chaos.v1"'
echo "$base" | grep -q '"chaotic":true'
echo "$base" | grep -q '"rms_r"'
echo "$base" | grep -q '"lobe_balance"'
echo "$base" | grep -q '"x_cross"'
echo "$base" | grep -q '"z_cross"'
echo "$base" > /tmp/chaos_base.json
python3 - <<'PY2'
import json
d=json.load(open("/tmp/chaos_base.json"))
assert d["ok"] and d["chaotic"] is True
assert d["lyapunov"] > 0 and d["rms_r"] > 0
assert 0.0 <= d["lobe_balance"] <= 1.0
assert d["x_cross"] >= 0 and d["z_cross"] >= 0
print("chaos_base_plate_ok", "lyap", round(d["lyapunov"],4), "rms", round(d["rms_r"],3))
PY2
echo "chaos_attractor_ok"
