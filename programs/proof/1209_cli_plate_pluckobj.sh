#!/bin/sh
# cubalc plate pluckobj — PLUCKOBJ dual (nest path ok)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_pluckobj"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","cfg":{"meta":{"x":1,"role":"worker","zone":"A"},"port":8080},"n":3}' | grep -q '"ok":true'

# deep nest path peel — bag lines above plate
OUT=$("$CUBALC" plate pluckobj "$F" cfg.meta role x zone)
printf '%s\n' "$OUT" | grep -q worker
printf '%s\n' "$OUT" | grep -q '"op":"pluckobj"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"hit":3'
printf '%s\n' "$OUT" | grep -q '"miss":0'
printf '%s\n' "$OUT" | grep -q '"nest_hit":true'
printf '%s\n' "$OUT" | grep -q '"nest":"cfg.meta"'

# miss field counts
OUT=$("$CUBALC" plate pluckobj "$F" cfg.meta role missing x)
printf '%s\n' "$OUT" | grep -q '"hit":2'
printf '%s\n' "$OUT" | grep -q '"miss":1'
printf '%s\n' "$OUT" | grep -q '"n":3'

# soft nest path miss
OUT=$("$CUBALC" plate pluckobj "$F" cfg.nope a b)
printf '%s\n' "$OUT" | grep -q '"nest_hit":false'
printf '%s\n' "$OUT" | grep -q '"hit":0'
printf '%s\n' "$OUT" | grep -q '"n":2'

# shallow nest still works
OUT=$("$CUBALC" plate pluckobj "$F" cfg port)
printf '%s\n' "$OUT" | grep -q 8080
printf '%s\n' "$OUT" | grep -q '"hit":1'

# aliases
OUT=$("$CUBALC" plate nestpluck "$F" cfg.meta zone)
printf '%s\n' "$OUT" | grep -q '"op":"pluckobj"'
printf '%s\n' "$OUT" | grep -q '^A$'

# help lists pluckobj
OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'pluckobj'

echo "1209_cli_plate_pluckobj: PASS"
