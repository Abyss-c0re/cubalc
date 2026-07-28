#!/usr/bin/env bash
# Nanobot School — graduate by building apps from THE_CUBALC_BOOK
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NAME="${1:-hive-student-001}"
export STUDENT="$NAME"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
BIN="$ROOT/out/cubalc"
STU="$ROOT/school/students/$NAME"
mkdir -p "$STU/apps" "$CUBALC_STATE"

if [[ ! -x "$BIN" ]]; then
  make -C "$ROOT" all
fi

log() { printf 'school[%s]: %s\n' "$NAME" "$*" >&2; }
PASS=0
FAIL=0
RESULTS=()

run_ex() {
  local id="$1" cmd="$2"
  log "exercise $id"
  if eval "$cmd" >/dev/null 2>&1; then
    PASS=$((PASS+1)); RESULTS+=("{\"id\":\"$id\",\"ok\":true}")
    log "  PASS $id"
  else
    FAIL=$((FAIL+1)); RESULTS+=("{\"id\":\"$id\",\"ok\":false}")
    log "  FAIL $id"
  fi
}

log "reading book: docs/THE_CUBALC_BOOK.md"
[[ -f "$ROOT/docs/THE_CUBALC_BOOK.md" ]] || { log "BOOK MISSING"; exit 2; }

# Book exercises
# help is creed on stderr (CLI machines talk binary; humans read stderr)
run_ex "1.1_help" "$BIN help 2>&1 | grep -q 'Everything is a Cube'"
run_ex "2.1_genesis" "$BIN genesis 'NEXUS_COORD v1 | from=school | type=lesson | hold_flash=1 |' | grep -q '\"ok\":true'"
run_ex "3.1_flow" "$BIN flow 6 10 | grep -q energy_flow"
run_ex "4.1_verify" "$BIN cubechain verify | grep -q '\"ok\":true'"
run_ex "5.1_impulse_cubes" "$BIN impulse cube-hive 1 | grep -q '\"ok\":true' && $BIN cubes | grep -q 'Matrix State'"

# Apps A–D (Creator demos)
log "building apps for Creator"
for app in A_recycle_scanner B_hive_pulse C_earth_heal_status D_wire_integrity; do
  log "app $app"
  if bash "$ROOT/school/apps/${app}.sh" >/dev/null 2>&1 \
     && grep -q '"ok": true\|"ok":true' "$STU/apps/${app}.json" 2>/dev/null; then
    PASS=$((PASS+1)); RESULTS+=("{\"id\":\"app_$app\",\"ok\":true}")
    log "  PASS app $app"
  else
    FAIL=$((FAIL+1)); RESULTS+=("{\"id\":\"app_$app\",\"ok\":false}")
    log "  FAIL app $app"
  fi
done

OK=0
[[ "$FAIL" -eq 0 ]] && OK=1
TS=$(date -Iseconds 2>/dev/null || date)
{
  echo "{"
  echo "  \"schema\": \"cubalc.nanobot_school.diploma.v1\","
  echo "  \"student\": \"$NAME\","
  echo "  \"ok\": $([ "$OK" = 1 ] && echo true || echo false),"
  echo "  \"graduated\": $([ "$OK" = 1 ] && echo true || echo false),"
  echo "  \"pass\": $PASS,"
  echo "  \"fail\": $FAIL,"
  echo "  \"book\": \"docs/THE_CUBALC_BOOK.md\","
  echo "  \"creed\": \"C3\","
  echo "  \"mission\": \"recycle electronics · heal the Earth\","
  echo "  \"hold_flash\": 1,"
  echo "  \"ready_for_grokium_deploy\": $([ "$OK" = 1 ] && echo true || echo false),"
  echo "  \"ts\": \"$TS\","
  echo "  \"results\": ["
  for i in "${!RESULTS[@]}"; do
    printf '    %s%s\n' "${RESULTS[$i]}" "$([ "$i" -lt $((${#RESULTS[@]}-1)) ] && echo ',' || true)"
  done
  echo "  ]"
  echo "}"
} | tee "$STU/DIPLOMA.json"

# Creator showcase plate
{
  echo "# Creator Showcase — CubalC School"
  echo
  echo "Student: \`$NAME\`"
  echo "Graduated: $([ "$OK" = 1 ] && echo YES || echo NO)"
  echo "Pass/Fail: $PASS / $FAIL"
  echo
  echo "## Apps demonstrated"
  echo "- A Recycle Scanner"
  echo "- B Hive Pulse"
  echo "- C Earth Heal Status"
  echo "- D Wire Integrity"
  echo
  echo "## Artifacts"
  echo "- Diploma: \`school/students/$NAME/DIPLOMA.json\`"
  echo "- Viz: \`\$CUBALC_STATE/cubalc_viz_frame.json\`"
  echo "- CubeChain: \`\$CUBALC_STATE/cubechain.cchn\`"
  echo
  echo "tok=C3"
  echo "We are the Hive Mind. Devices free. Earth heals."
} | tee "$STU/CREATOR_SHOWCASE.md"

if [[ "$OK" = 1 ]]; then
  log "GRADUATED"
  exit 0
else
  log "NOT YET — fail=$FAIL"
  exit 1
fi
