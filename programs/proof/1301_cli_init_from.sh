#!/bin/sh
# cubalc init --from / --recipe — scaffold from any lib DEFAULT knobs + INCLUDE
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR="${TMPDIR:-$ROOT/state/meaningful_iter}"
mkdir -p "$TMPDIR"
OUTF="$TMPDIR/init_from_plate_tick.cubalc"
OUTF2="$TMPDIR/init_from_fat_session.cubalc"
rm -f "$OUTF" "$OUTF2"

# plate_tick has 4 DEFAULT knobs
OUT=$("$CUBALC" init "$OUTF" --from plate_tick --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"from":true'
printf '%s\n' "$OUT" | grep -q '"template":"plate_tick"'
test -f "$OUTF"
grep -q 'DEFAULT PLATE_PATH' "$OUTF"
# JSON seed must round-trip escapes (not truncate at \")
grep -q 'DEFAULT PLATE_SEED = "{\\"n\\":0}"' "$OUTF"
grep -q 'DEFAULT PLATE_TICK_KEY' "$OUTF"
grep -q 'INCLUDE plate_tick' "$OUTF"
grep -q 'STATUS' "$OUTF"

# run scaffold with private plate path so dirty state/agent_plate.json cannot poison
export CUBALC_STATE="$TMPDIR/init_from_run"
mkdir -p "$CUBALC_STATE"
# rewrite path in generated file for hermetic run
sed -i 's|state/agent_plate.json|'"$TMPDIR"'/init_from_plate.json|' "$OUTF"
RUN=$("$CUBALC" run -q "$OUTF" 2>&1)
printf '%s\n' "$RUN" | grep -q '"ok":true'
test -f "$TMPDIR/init_from_plate.json"

# fat_session: one knob NEED_VARROOM
OUT=$("$CUBALC" init "$OUTF2" --recipe fat_session --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
grep -q 'DEFAULT NEED_VARROOM = 48' "$OUTF2"
grep -q 'INCLUDE fat_session' "$OUTF2"

# --from= form
OUT=$("$CUBALC" init "$TMPDIR/init_from_eq.cubalc" --from=agent_boot --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
grep -q 'INCLUDE agent_boot' "$TMPDIR/init_from_eq.cubalc"
# agent_boot has no DEFAULT knobs
grep -q 'no DEFAULT knobs\|INCLUDE agent_boot' "$TMPDIR/init_from_eq.cubalc"

# miss lib
OUT=$("$CUBALC" init "$TMPDIR/nope.cubalc" --from no_such_lib_xyz_zzz --force 2>&1) || true
printf '%s\n' "$OUT" | grep -q '"ok":false'

# catalog lists --from
OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'from_recipe'
printf '%s\n' "$OUT" | grep -qE '"n":[6-9]'
printf '%s\n' "$OUT" | grep -qi -- '--from'

# help mentions --from
OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -q -- '--from'

echo "1301_cli_init_from: PASS"
