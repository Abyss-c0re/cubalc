#!/bin/sh
# cubalc plate nestpick|nestomit — CLI duals of PICKOBJ/OMITOBJ
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_nestpick"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","meta":{"x":1,"role":"worker","zone":"A","tmp":"z"},"n":3}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate nestpick "$F" meta role zone)
echo "$OUT" | grep -q '"op":"nestpick"'
echo "$OUT" | grep -q '"ok":true'
echo "$OUT" | grep -q '"n":2'
OUT=$("$CUBALC" plate nestkeys "$F" meta)
echo "$OUT" | grep -q '"keys":"role,zone"'
echo "$OUT" | grep -q '"n":2'

"$CUBALC" plate nestset "$F" meta tmp z | grep -q '"ok":true'
"$CUBALC" plate nestset "$F" meta x 9 | grep -q '"ok":true'
OUT=$("$CUBALC" plate nestomit "$F" meta tmp)
echo "$OUT" | grep -q '"op":"nestomit"'
echo "$OUT" | grep -q '"ok":true'
OUT=$("$CUBALC" plate nestkeys "$F" meta)
echo "$OUT" | grep -q '"keys":"role,zone,x"'
echo "$OUT" | grep -q tmp && exit 1 || true

OUT=$("$CUBALC" plate get "$F" host)
echo "$OUT" | grep -q '"value":"cubeA"'

F2="$ST/cfg.json"
"$CUBALC" plate ensure "$F2" '{"cfg":{"a":1,"b":2,"c":3}}' | grep -q '"ok":true'
OUT=$("$CUBALC" plate pickobj "$F2" cfg a c)
echo "$OUT" | grep -q '"op":"nestpick"'
echo "$OUT" | grep -q '"n":2'
OUT=$("$CUBALC" plate nestkeys "$F2" cfg)
echo "$OUT" | grep -q '"keys":"a,c"'
echo "$OUT" | grep -q '"n":2'

OUT=$("$CUBALC" plate omitobj "$F2" cfg a)
echo "$OUT" | grep -q '"op":"nestomit"'
OUT=$("$CUBALC" plate nestkeys "$F2" cfg)
echo "$OUT" | grep -q '"keys":"c"'
echo "$OUT" | grep -q '"n":1'

echo "cli plate nestpick ok"
