#!/bin/sh
# cubalc init --peer — scaffold multi-plate PLATE+PEER starter
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_init_peer_proof"
rm -rf "$ST"
mkdir -p "$ST"
OUT=$("$CUBALC" init "$ST/mesh" --peer --force)
echo "$OUT" | grep -q '"template":"plate_peer_session"'
echo "$OUT" | grep -q '"ok":true'
grep -q 'INCLUDE plate_peer_session' "$ST/mesh.cubalc"
grep -q 'SETP FROM PEER' "$ST/mesh.cubalc"
grep -q 'plate_peer_save' "$ST/mesh.cubalc"
grep -q 'NEEDP FROM PEER' "$ST/mesh.cubalc"
# run under private state so PEER/PLATE paths resolve under ST
mkdir -p "$ST/state"
# use paths relative to CWD; set STATE for any SYS STATE users in libs
RUN=$(CUBALC_STATE="$ST" "$CUBALC" run -q "$ST/mesh.cubalc")
echo "$RUN" | grep -q '"ok":true'
# --plate still works
OUT2=$("$CUBALC" init "$ST/solo" --plate --force)
echo "$OUT2" | grep -q '"template":"plate_session"'
echo "init_peer ok"
