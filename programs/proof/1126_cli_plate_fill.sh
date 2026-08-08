#!/bin/bash
# cubalc plate fill / fillkeys CLI smoke — agent one-shot templates without .cubalc
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CUBALC="${ROOT}/out/cubalc"
ST="${ROOT}/state/meaningful_iter/mi_plate_fill"
mkdir -p "$ST"
export CUBALC_STATE="$ST"
PL="$ST/agent.json"
TPL="$ST/status.tpl"
OUT="$ST/status.txt"
rm -f "$PL" "$TPL" "$OUT"

echo '{"role":"worker","host":"cube1","n":7}' > "$PL"
echo 'status={{role}}@{{host}} n={{n}}' > "$TPL"

# fillkeys
K=$("$CUBALC" plate fillkeys "$PL" "@$TPL" 2>/dev/null | tail -1)
echo "$K" | grep -q '"op":"fillkeys"'
echo "$K" | grep -q '"n":3'
echo "$K" | grep -q 'role'

# soft fill to stdout body + plate
B=$("$CUBALC" plate fill "$PL" "peer={{role}}@{{host}}" 2>/dev/null)
echo "$B" | head -1 | grep -qx 'peer=worker@cube1'
echo "$B" | tail -1 | grep -q '"hits":2'
echo "$B" | tail -1 | grep -q '"miss":0'

# fill file template to out
"$CUBALC" plate fill "$PL" "@$TPL" "$OUT" 2>/dev/null | tail -1 | grep -q '"ok":true'
test -f "$OUT"
grep -qx 'status=worker@cube1 n=7' "$OUT"

# strict miss
echo '{"role":"worker"}' > "$PL"
set +e
"$CUBALC" plate fill -s "$PL" "x={{role}} y={{host}}" >/tmp/pf_strict.json 2>/dev/null
RC=$?
set -e
test "$RC" -ne 0
tail -1 /tmp/pf_strict.json | grep -q 'missing plate keys'
tail -1 /tmp/pf_strict.json | grep -q 'host'

echo "cli_plate_fill ok"
