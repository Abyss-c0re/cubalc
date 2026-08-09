#!/bin/sh
# cubalc plate sum|avg|median|top|bot — CLI duals of SUMNP/AVGNP/MEDIANP/TOPNP/BOTNP
# Usability: agent one-shot score/FREQ plates without a .cubalc program.
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_plate_agg"
F="$ST/scores.json"
rm -rf "$ST"
mkdir -p "$ST"

# a=10 b=3 c=8 d=1 e=100 → sum 122, avg 24, median 8; top crit-style keys
"$CUBALC" plate ensure "$F" '{"error":9,"warn":3,"info":1,"crit":12,"host":"A"}' | grep -q '"ok":true'

# sum: 9+3+1+12 = 25
OUT=$("$CUBALC" plate sum "$F")
echo "$OUT" | grep -q '"op":"sum"'
echo "$OUT" | grep -q '"ok":true'
echo "$OUT" | grep -q '"n":25'

# avg: 25/4 = 6
OUT=$("$CUBALC" plate avg "$F")
echo "$OUT" | grep -q '"op":"avg"'
echo "$OUT" | grep -q '"n":6'
echo "$OUT" | grep -q '"used":4'

# mean alias
OUT=$("$CUBALC" plate mean "$F")
echo "$OUT" | grep -q '"op":"avg"'

# median: 1,3,9,12 → lower mid 3
OUT=$("$CUBALC" plate median "$F")
echo "$OUT" | grep -q '"op":"median"'
echo "$OUT" | grep -q '"n":3'
echo "$OUT" | grep -q '"used":4'

# p50 alias
OUT=$("$CUBALC" plate p50 "$F")
echo "$OUT" | grep -q '"op":"median"'

# top 1 → crit:12
OUT=$("$CUBALC" plate top "$F")
echo "$OUT" | grep -q '"op":"top"'
echo "$OUT" | grep -q '"ok":true'
echo "$OUT" | grep -q '"n":1'
echo "$OUT" | grep -q '"key":"crit"'
echo "$OUT" | grep -q '"value":12'
echo "$OUT" | grep -q '"crit":12'

# top 3
OUT=$("$CUBALC" plate top "$F" 3)
echo "$OUT" | grep -q '"n":3'
echo "$OUT" | grep -q '"cand":4'
echo "$OUT" | grep -q 'error'
echo "$OUT" | grep -q 'warn'
echo "$OUT" | grep -q 'info' && exit 1 || true

# bot 1 → info:1
OUT=$("$CUBALC" plate bot "$F")
echo "$OUT" | grep -q '"op":"bot"'
echo "$OUT" | grep -q '"key":"info"'
echo "$OUT" | grep -q '"value":1'

# bottom alias
OUT=$("$CUBALC" plate bottom "$F" 2)
echo "$OUT" | grep -q '"op":"bot"'
echo "$OUT" | grep -q '"n":2'

# total alias of sum
OUT=$("$CUBALC" plate total "$F")
echo "$OUT" | grep -q '"op":"sum"'
echo "$OUT" | grep -q '"n":25'

# original file unchanged
SHOW=$("$CUBALC" plate show "$F")
echo "$SHOW" | grep -q 'host'
echo "$SHOW" | grep -q 'crit'

echo "cli plate agg ok"
