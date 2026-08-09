#!/bin/sh
# cubalc plate nest* — CLI duals of GETPOBJ/SETPOBJ/INCOBJ/DELPOBJ/KEYSOBJ/HASPOBJ
# Usability: agent one-shot nested config without a .cubalc program.
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_plate_nest"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","meta":{"x":1,"role":"worker"},"n":3}' | grep -q '"ok":true'

# nestget
OUT=$("$CUBALC" plate nestget "$F" meta role)
echo "$OUT" | grep -q '"op":"nestget"'
echo "$OUT" | grep -q '"ok":true'
echo "$OUT" | grep -q '"hit":true'
echo "$OUT" | grep -q '"value":"worker"'

# nestget miss + OR
OUT=$("$CUBALC" plate nestget "$F" meta missing OR fallback)
echo "$OUT" | grep -q '"or":true'
echo "$OUT" | grep -q '"value":"fallback"'

# nestset
OUT=$("$CUBALC" plate nestset "$F" meta role leader)
echo "$OUT" | grep -q '"op":"nestset"'
echo "$OUT" | grep -q '"ok":true'
OUT=$("$CUBALC" plate nestget "$F" meta role)
echo "$OUT" | grep -q '"value":"leader"'
# sibling intact
OUT=$("$CUBALC" plate nestget "$F" meta x)
echo "$OUT" | grep -q '"value":"1"'

# nestinc creates nest
OUT=$("$CUBALC" plate nestinc "$F" stats hits)
echo "$OUT" | grep -q '"op":"nestinc"'
echo "$OUT" | grep -q '"value":1'
OUT=$("$CUBALC" plate nestinc "$F" stats hits 2)
echo "$OUT" | grep -q '"value":3'
OUT=$("$CUBALC" plate nestget "$F" stats hits)
echo "$OUT" | grep -q '"value":"3"'

# nestkeys
OUT=$("$CUBALC" plate nestkeys "$F" meta)
echo "$OUT" | grep -q '"op":"nestkeys"'
echo "$OUT" | grep -q '"n":2'
echo "$OUT" | grep -q 'role'
echo "$OUT" | grep -q 'x'

# nesthas
OUT=$("$CUBALC" plate nesthas "$F" meta role)
echo "$OUT" | grep -q '"hit":true'
set +e
OUT=$("$CUBALC" plate nesthas "$F" meta ghost)
RC=$?
set -e
echo "$OUT" | grep -q '"hit":false'
test "$RC" -ne 0

# nestdel
"$CUBALC" plate nestset "$F" meta tmp z | grep -q '"ok":true'
OUT=$("$CUBALC" plate nestdel "$F" meta tmp)
echo "$OUT" | grep -q '"op":"nestdel"'
set +e
OUT=$("$CUBALC" plate nestget "$F" meta tmp)
RC=$?
set -e
echo "$OUT" | grep -q '"hit":false'
test "$RC" -ne 0
# role still there
OUT=$("$CUBALC" plate nestget "$F" meta role)
echo "$OUT" | grep -q '"value":"leader"'

# aliases
OUT=$("$CUBALC" plate nget "$F" meta x)
echo "$OUT" | grep -q '"op":"nestget"'
OUT=$("$CUBALC" plate getpobj "$F" meta role)
echo "$OUT" | grep -q '"value":"leader"'

# outer keys intact
OUT=$("$CUBALC" plate get "$F" host)
echo "$OUT" | grep -q '"value":"cubeA"'

echo "cli plate nest ok"
