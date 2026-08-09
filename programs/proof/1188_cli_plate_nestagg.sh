#!/bin/sh
# cubalc plate nestsum|nestavg|nestmedian|nesttop|nestbot — SUMNOBJ/TOPNOBJ CLI duals
# Usability: agent one-shot nested FREQ plates without a .cubalc program.
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_nestagg"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

# freq: error=9 warn=3 info=1 crit=12 → sum 25, avg 6, median 3|6 mid of 4
"$CUBALC" plate ensure "$F" '{"host":"cubeA","freq":{"error":9,"warn":3,"info":1,"crit":12,"label":"x"},"n":3}' | grep -q '"ok":true'

# nestsum: 9+3+1+12 = 25
OUT=$("$CUBALC" plate nestsum "$F" freq)
echo "$OUT" | grep -q '"op":"nestsum"'
echo "$OUT" | grep -q '"ok":true'
echo "$OUT" | grep -q '"n":25'
echo "$OUT" | grep -q '"nest":"freq"'
echo "$OUT" | grep -q '"nest_hit":true'

# nestavg: 25/4 = 6
OUT=$("$CUBALC" plate nestavg "$F" freq)
echo "$OUT" | grep -q '"op":"nestavg"'
echo "$OUT" | grep -q '"n":6'
echo "$OUT" | grep -q '"used":4'

# nestmean alias
OUT=$("$CUBALC" plate nestmean "$F" freq)
echo "$OUT" | grep -q '"op":"nestavg"'

# nestmedian: 1,3,9,12 → lower mid 3
OUT=$("$CUBALC" plate nestmedian "$F" freq)
echo "$OUT" | grep -q '"op":"nestmedian"'
echo "$OUT" | grep -q '"n":3'
echo "$OUT" | grep -q '"used":4'

# nestp50 alias
OUT=$("$CUBALC" plate nestp50 "$F" freq)
echo "$OUT" | grep -q '"op":"nestmedian"'

# nesttop 1 → crit:12
OUT=$("$CUBALC" plate nesttop "$F" freq)
echo "$OUT" | grep -q '"op":"nesttop"'
echo "$OUT" | grep -q '"ok":true'
echo "$OUT" | grep -q '"n":1'
echo "$OUT" | grep -q '"key":"crit"'
echo "$OUT" | grep -q '"value":12'
echo "$OUT" | grep -q '"crit":12'

# nesttop 3
OUT=$("$CUBALC" plate nesttop "$F" freq 3)
echo "$OUT" | grep -q '"n":3'
echo "$OUT" | grep -q '"cand":4'
echo "$OUT" | grep -q 'error'
echo "$OUT" | grep -q 'warn'
echo "$OUT" | grep -q 'info' && exit 1 || true

# nestbot 1 → info:1
OUT=$("$CUBALC" plate nestbot "$F" freq)
echo "$OUT" | grep -q '"op":"nestbot"'
echo "$OUT" | grep -q '"key":"info"'
echo "$OUT" | grep -q '"value":1'

# soft nest miss
OUT=$("$CUBALC" plate nestsum "$F" ghost)
echo "$OUT" | grep -q '"ok":true'
echo "$OUT" | grep -q '"n":0'
echo "$OUT" | grep -q '"nest_hit":false'

# outer file not mutated by nesttop (read-only CLI)
OUT=$("$CUBALC" plate nestkeys "$F" freq)
echo "$OUT" | grep -q 'info'
echo "$OUT" | grep -q 'label'

# sumnobj / topnobj aliases
OUT=$("$CUBALC" plate sumnobj "$F" freq)
echo "$OUT" | grep -q '"op":"nestsum"'
echo "$OUT" | grep -q '"n":25'
OUT=$("$CUBALC" plate topnobj "$F" freq 1)
echo "$OUT" | grep -q '"op":"nesttop"'
echo "$OUT" | grep -q '"key":"crit"'

# outer intact
OUT=$("$CUBALC" plate get "$F" host)
echo "$OUT" | grep -q '"value":"cubeA"'

echo "cli_plate_nestagg ok"
