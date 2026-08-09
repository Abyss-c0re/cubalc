#!/bin/sh
# cubalc plate rename|copy|swap — RENAMEP/COPYP/SWAPP CLI duals (paths ok)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_ren_cp_sw"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","cfg":{"port":8080,"tls":0},"flags":{"debug":0,"trace":1},"n":1}' | grep -q '"ok":true'

# rename nest path
OUT=$("$CUBALC" plate rename "$F" cfg.port cfg.listen)
printf '%s\n' "$OUT" | grep -q '"op":"rename"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":1'
OUT=$("$CUBALC" plate get "$F" cfg.listen)
printf '%s\n' "$OUT" | grep -q '"value":"8080"'
OUT=$("$CUBALC" plate get "$F" cfg.port 2>/dev/null || true)
printf '%s\n' "$OUT" | grep -q 'hit.:false\|key miss' || printf '%s\n' "$OUT" | grep -q '"hit":false'

# cross-nest move
OUT=$("$CUBALC" plate move "$F" cfg.tls net.tls)
printf '%s\n' "$OUT" | grep -q '"op":"rename"'
printf '%s\n' "$OUT" | grep -q '"n":1'
OUT=$("$CUBALC" plate get "$F" net.tls)
printf '%s\n' "$OUT" | grep -q '"value":"0"'

# copy path
OUT=$("$CUBALC" plate copy "$F" flags.trace flags.mirror)
printf '%s\n' "$OUT" | grep -q '"op":"copy"'
printf '%s\n' "$OUT" | grep -q '"n":1'
OUT=$("$CUBALC" plate get "$F" flags.mirror)
printf '%s\n' "$OUT" | grep -q '"value":"1"'
OUT=$("$CUBALC" plate get "$F" flags.trace)
printf '%s\n' "$OUT" | grep -q '"value":"1"'

# soft miss copy
OUT=$("$CUBALC" plate copy "$F" flags.nope flags.x)
printf '%s\n' "$OUT" | grep -q '"n":0'

# swap
OUT=$("$CUBALC" plate swap "$F" flags.debug flags.trace)
printf '%s\n' "$OUT" | grep -q '"op":"swap"'
printf '%s\n' "$OUT" | grep -q '"n":1'
OUT=$("$CUBALC" plate get "$F" flags.debug)
printf '%s\n' "$OUT" | grep -q '"value":"1"'
OUT=$("$CUBALC" plate get "$F" flags.trace)
printf '%s\n' "$OUT" | grep -q '"value":"0"'

# shallow rename alias
OUT=$("$CUBALC" plate mv "$F" host name)
printf '%s\n' "$OUT" | grep -q '"n":1'
OUT=$("$CUBALC" plate get "$F" name)
printf '%s\n' "$OUT" | grep -q 'cubeA'

# help lists ops
OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'rename'
printf '%s\n' "$OUT" | grep -q 'copy'
printf '%s\n' "$OUT" | grep -q 'swap'

echo "1200_cli_plate_rename_copy_swap: PASS"
