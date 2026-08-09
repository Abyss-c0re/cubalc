#!/bin/sh
# cubalc plate pick|omit — keep/drop listed keys on disk plates (PICKP/OMITP duals)
# Usability: shell/agent plate projection without a .cubalc program.
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_plate_pick"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"n":1,"host":"cubeA","role":"worker","tmp":"x","debug":1}' | grep -q '"created":true'

# pick subset
OUT=$("$CUBALC" plate pick "$F" host role n)
echo "$OUT" | grep -q '"op":"pick"'
echo "$OUT" | grep -q '"ok":true'
echo "$OUT" | grep -q '"n":3'
echo "$OUT" | grep -q 'cubeA'
echo "$OUT" | grep -q 'worker'
echo "$OUT" | grep -q '"n":1' || echo "$OUT" | grep -q ':"n":1\|"n":1,'
# noise keys not in subset plate body
echo "$OUT" | grep -v tmp | grep -q '"op":"pick"'
echo "$OUT" | grep -q '"tmp"' && exit 1 || true

# alias keep
OUT2=$("$CUBALC" plate keep "$F" host)
echo "$OUT2" | grep -q '"op":"pick"'
echo "$OUT2" | grep -q 'cubeA'
echo "$OUT2" | grep -q '"n":1'

# omit strip noise
OUT3=$("$CUBALC" plate omit "$F" tmp debug)
echo "$OUT3" | grep -q '"op":"omit"'
echo "$OUT3" | grep -q '"ok":true'
echo "$OUT3" | grep -q 'cubeA'
echo "$OUT3" | grep -q 'worker'
echo "$OUT3" | grep -q '"tmp"' && exit 1 || true
echo "$OUT3" | grep -q '"debug"' && exit 1 || true
# n should be 2 removed when both present
echo "$OUT3" | grep -q '"n":2'

# alias strip
OUT4=$("$CUBALC" plate strip "$F" tmp)
echo "$OUT4" | grep -q '"op":"omit"'
echo "$OUT4" | grep -q '"n":1'

# soft miss key still ok
OUT5=$("$CUBALC" plate pick "$F" host missing_key)
echo "$OUT5" | grep -q '"ok":true'
echo "$OUT5" | grep -q 'cubeA'
echo "$OUT5" | grep -q '"n":1'

# original file unchanged (projection only)
SHOW=$("$CUBALC" plate show "$F")
echo "$SHOW" | grep -q '"tmp"'
echo "$SHOW" | grep -q 'debug'

echo "cli plate pick/omit ok"
