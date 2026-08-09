#!/bin/sh
# cubalc plate keys|len|empty|vals — nest path duals of KEYSP/LENP/EMPTYP/VALSP
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_keys_len"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","cfg":{"port":8080,"tls":0,"role":"edge"},"flags":{"debug":0},"empty":{},"n":1}' | grep -q '"ok":true'

# top-level keys
OUT=$("$CUBALC" plate keys "$F")
printf '%s\n' "$OUT" | grep -q '"op":"keys"'
printf '%s\n' "$OUT" | grep -q '"n":5'
printf '%s\n' "$OUT" | grep -q host

# nest path keys
OUT=$("$CUBALC" plate keys "$F" cfg)
printf '%s\n' "$OUT" | grep -q '"nest":"cfg"'
printf '%s\n' "$OUT" | grep -q '"n":3'
printf '%s\n' "$OUT" | grep -q port
printf '%s\n' "$OUT" | grep -q role

# deeper
OUT=$("$CUBALC" plate keys "$F" flags)
printf '%s\n' "$OUT" | grep -q '"n":1'
printf '%s\n' "$OUT" | grep -q debug

# len
OUT=$("$CUBALC" plate len "$F")
printf '%s\n' "$OUT" | grep -q '"op":"len"'
printf '%s\n' "$OUT" | grep -q '"n":5'
OUT=$("$CUBALC" plate len "$F" cfg)
printf '%s\n' "$OUT" | grep -q '"nest":"cfg"'
printf '%s\n' "$OUT" | grep -q '"n":3'
OUT=$("$CUBALC" plate nkeys "$F" flags)
printf '%s\n' "$OUT" | grep -q '"n":1'

# empty
OUT=$("$CUBALC" plate empty "$F" empty)
printf '%s\n' "$OUT" | grep -q '"empty":true'
printf '%s\n' "$OUT" | grep -q '"n":0'
OUT=$("$CUBALC" plate empty "$F" cfg)
printf '%s\n' "$OUT" | grep -q '"empty":false'
printf '%s\n' "$OUT" | grep -q '"n":3'

# soft miss path
OUT=$("$CUBALC" plate keys "$F" missing.nest)
printf '%s\n' "$OUT" | grep -q '"n":0'
OUT=$("$CUBALC" plate len "$F" missing.nest)
printf '%s\n' "$OUT" | grep -q '"n":0'
OUT=$("$CUBALC" plate empty "$F" missing.nest)
printf '%s\n' "$OUT" | grep -q '"empty":true'

# vals path
OUT=$("$CUBALC" plate vals "$F" cfg)
printf '%s\n' "$OUT" | grep -q '"op":"vals"'
printf '%s\n' "$OUT" | grep -q '"n":3'
printf '%s\n' "$OUT" | grep -q 8080
printf '%s\n' "$OUT" | grep -q edge

# help lists ops
OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'len'
printf '%s\n' "$OUT" | grep -q 'empty'
printf '%s\n' "$OUT" | grep -q 'vals'

echo "1201_cli_plate_keys_len: PASS"
