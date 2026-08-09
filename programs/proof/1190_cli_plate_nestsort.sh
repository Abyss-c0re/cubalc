#!/bin/sh
# cubalc plate nestsort|nestsortbag — SORTOBJ/SORTBAGOBJ CLI duals
# Usability: agent one-shot nest FREQ full rank without a .cubalc program.
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_nestsort"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

# freq: error=9 warn=3 info=1 crit=12 label=x (non-int dropped on sort write)
"$CUBALC" plate ensure "$F" '{"host":"cubeA","freq":{"error":9,"warn":3,"info":1,"crit":12,"label":"x"},"n":3}' | grep -q '"ok":true'

# nestsortbag DESC default: crit:12 then error:9 … (no write)
OUT=$("$CUBALC" plate nestsortbag "$F" freq)
printf '%s\n' "$OUT" | grep -q '"op":"nestsortbag"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":4'
printf '%s\n' "$OUT" | grep -q '"asc":false'
printf '%s\n' "$OUT" | grep -q '"nest_hit":true'
printf '%s\n' "$OUT" | grep -q 'crit:12'
printf '%s\n' "$OUT" | grep -q 'error:9'
# file not mutated (label still present)
OUT=$("$CUBALC" plate nestkeys "$F" freq)
printf '%s\n' "$OUT" | grep -q 'label'
printf '%s\n' "$OUT" | grep -q 'info'

# nestsortbag ASC — lightest first
OUT=$("$CUBALC" plate nestsortbag "$F" freq ASC)
printf '%s\n' "$OUT" | grep -q '"asc":true'
printf '%s\n' "$OUT" | grep -q '"n":4'
printf '%s\n' "$OUT" | grep -q 'info:1'
printf '%s\n' "$OUT" | grep -q 'crit:12'
# bag order ASC: info before warn before error before crit
printf '%s\n' "$OUT" | grep -q 'info:1\\nwarn:3\\nerror:9\\ncrit:12'

# nestsort DESC write-back
OUT=$("$CUBALC" plate nestsort "$F" freq)
printf '%s\n' "$OUT" | grep -q '"op":"nestsort"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":4'
printf '%s\n' "$OUT" | grep -q '"written":true'
printf '%s\n' "$OUT" | grep -q '"asc":false'
printf '%s\n' "$OUT" | grep -q '"nest_hit":true'
# pure-int only in nest; host outer kept
printf '%s\n' "$OUT" | grep -q 'crit'
printf '%s\n' "$OUT" | grep -q 'cubeA'
# label gone from nest after sort write-back
OUT=$("$CUBALC" plate nestkeys "$F" freq)
printf '%s\n' "$OUT" | grep -q 'crit'
printf '%s\n' "$OUT" | grep -q 'error'
if printf '%s\n' "$OUT" | grep -q 'label'; then
  echo "FAIL: label should be dropped by nestsort" >&2
  exit 1
fi

# reseed and ASC write (overwrite via plate set path: delete + ensure)
rm -f "$F"
"$CUBALC" plate ensure "$F" '{"host":"cubeA","freq":{"error":9,"warn":3,"info":1,"crit":12},"n":3}' >/dev/null
OUT=$("$CUBALC" plate nestsort "$F" freq ASC)
printf '%s\n' "$OUT" | grep -q '"asc":true'
printf '%s\n' "$OUT" | grep -q '"n":4'
printf '%s\n' "$OUT" | grep -q 'info'

# soft nest miss
OUT=$("$CUBALC" plate nestsortbag "$F" ghost)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":0'
printf '%s\n' "$OUT" | grep -q '"nest_hit":false'

OUT=$("$CUBALC" plate nestsort "$F" ghost)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":0'
printf '%s\n' "$OUT" | grep -q '"written":false'
printf '%s\n' "$OUT" | grep -q '"nest_hit":false'

# aliases
OUT=$("$CUBALC" plate sortbagobj "$F" freq)
printf '%s\n' "$OUT" | grep -q '"op":"nestsortbag"'
printf '%s\n' "$OUT" | grep -q '"n":4'

OUT=$("$CUBALC" plate nsort "$F" freq DESC)
printf '%s\n' "$OUT" | grep -q '"op":"nestsort"'
printf '%s\n' "$OUT" | grep -q '"written":true'

# help ops list includes nestsort
OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'nestsort'
printf '%s\n' "$OUT" | grep -q 'nestsortbag'

echo "1190_cli_plate_nestsort: PASS"
