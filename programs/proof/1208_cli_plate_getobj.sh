#!/bin/sh
# cubalc plate getobj|setobj|mergeobj|defaultobj — GETOBJ/SETOBJ duals (paths ok)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_getobj"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","cfg":{"meta":{"x":1,"role":"worker"},"port":8080},"n":1}' | grep -q '"ok":true'

# getobj deep path — body above plate
OUT=$("$CUBALC" plate getobj "$F" cfg.meta)
printf '%s\n' "$OUT" | grep -q '"x"'
printf '%s\n' "$OUT" | grep -q worker
printf '%s\n' "$OUT" | grep -q '"op":"getobj"'
printf '%s\n' "$OUT" | grep -q '"hit":true'

# setobj create deep path
OUT=$("$CUBALC" plate setobj "$F" net.peer '{"host":"cubeB","n":2}')
printf '%s\n' "$OUT" | grep -q '"op":"setobj"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
OUT=$("$CUBALC" plate get "$F" net.peer.host)
printf '%s\n' "$OUT" | grep -q cubeB
OUT=$("$CUBALC" plate getn "$F" net.peer.n)
printf '%s\n' "$OUT" | grep -q '"n":2'

# mergeobj path
OUT=$("$CUBALC" plate mergeobj "$F" cfg.meta '{"zone":"A","x":9}')
printf '%s\n' "$OUT" | grep -q '"op":"mergeobj"'
printf '%s\n' "$OUT" | grep -q '"n":'
OUT=$("$CUBALC" plate get "$F" cfg.meta.zone)
printf '%s\n' "$OUT" | grep -q '"value":"A"'
OUT=$("$CUBALC" plate getn "$F" cfg.meta.x)
printf '%s\n' "$OUT" | grep -q '"n":9'
# role kept
OUT=$("$CUBALC" plate get "$F" cfg.meta.role)
printf '%s\n' "$OUT" | grep -q worker

# defaultobj path (fill miss only)
OUT=$("$CUBALC" plate defaultobj "$F" cfg.meta '{"role":"x","ttl":30}')
printf '%s\n' "$OUT" | grep -q '"op":"defaultobj"'
OUT=$("$CUBALC" plate get "$F" cfg.meta.role)
printf '%s\n' "$OUT" | grep -q worker
OUT=$("$CUBALC" plate getn "$F" cfg.meta.ttl)
printf '%s\n' "$OUT" | grep -q '"n":30'

# soft miss getobj
set +e
OUT=$("$CUBALC" plate getobj "$F" missing.nest)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"hit":false'
[ "$RC" -ne 0 ]

# OR fallback
OUT=$("$CUBALC" plate getobj "$F" missing.nest OR '{}')
printf '%s\n' "$OUT" | grep -q '"or":true'
printf '%s\n' "$OUT" | grep -q '"hit":false'

# aliases
OUT=$("$CUBALC" plate putobj "$F" tags.list '{"a":1}')
printf '%s\n' "$OUT" | grep -q '"op":"setobj"'

# help
OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'getobj'
printf '%s\n' "$OUT" | grep -q 'setobj'
printf '%s\n' "$OUT" | grep -q 'mergeobj'

echo "1208_cli_plate_getobj: PASS"
