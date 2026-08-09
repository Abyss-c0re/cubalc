#!/bin/sh
# cubalc plate nestrename|nestcopy|nestswap — RENAMEPOBJ/COPYPOBJ/SWAPPOBJ duals
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_nestren"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","flags":{"dbg":0,"trace":1,"noise":9},"n":1}' | grep -q '"ok":true'

# nestrename
OUT=$("$CUBALC" plate nestrename "$F" flags dbg debug)
printf '%s\n' "$OUT" | grep -q '"op":"nestrename"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":1'
OUT=$("$CUBALC" plate get "$F" flags.debug)
printf '%s\n' "$OUT" | grep -q '"value":"0"'
OUT=$("$CUBALC" plate has "$F" flags.dbg || true)
printf '%s\n' "$OUT" | grep -q '"has_all":false'

# nestcopy
OUT=$("$CUBALC" plate nestcopy "$F" flags debug mirror)
printf '%s\n' "$OUT" | grep -q '"op":"nestcopy"'
printf '%s\n' "$OUT" | grep -q '"n":1'
OUT=$("$CUBALC" plate get "$F" flags.mirror)
printf '%s\n' "$OUT" | grep -q '"value":"0"'
OUT=$("$CUBALC" plate get "$F" flags.debug)
printf '%s\n' "$OUT" | grep -q '"value":"0"'

# nestswap
OUT=$("$CUBALC" plate nestswap "$F" flags debug trace)
printf '%s\n' "$OUT" | grep -q '"op":"nestswap"'
printf '%s\n' "$OUT" | grep -q '"n":1'
OUT=$("$CUBALC" plate get "$F" flags.debug)
printf '%s\n' "$OUT" | grep -q '"value":"1"'
OUT=$("$CUBALC" plate get "$F" flags.trace)
printf '%s\n' "$OUT" | grep -q '"value":"0"'

# soft miss nest
OUT=$("$CUBALC" plate nestrename "$F" missing.nest a b)
printf '%s\n' "$OUT" | grep -q '"n":0'
printf '%s\n' "$OUT" | grep -q '"nest_hit":false'

# soft key miss
OUT=$("$CUBALC" plate nestcopy "$F" flags nope x)
printf '%s\n' "$OUT" | grep -q '"n":0'

# aliases
OUT=$("$CUBALC" plate nrename "$F" flags mirror snap)
printf '%s\n' "$OUT" | grep -q '"op":"nestrename"'
printf '%s\n' "$OUT" | grep -q '"n":1'

# help
OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'nestrename'
printf '%s\n' "$OUT" | grep -q 'nestcopy'
printf '%s\n' "$OUT" | grep -q 'nestswap'

echo "1205_cli_plate_nestrename: PASS"
