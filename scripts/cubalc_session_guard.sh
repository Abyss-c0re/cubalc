#!/usr/bin/env bash
# cubalc_session_guard.sh — The loop must go on.
#
# Wraps CubalC pure-C evolve (+ optional Cube way) so the Grok Build / Cube
# session keeps improving algocubes. Self-recover on crash. If recovery is
# impossible (auth/login / hard failure), reach The Commander on a plate.
#
# Usage:
#   bash scripts/cubalc_session_guard.sh              # foreground forever
#   bash scripts/cubalc_session_guard.sh --daemon      # background self
#   bash scripts/cubalc_session_guard.sh --once        # single health pass
#   bash scripts/cubalc_session_guard.sh --status
#
# Env:
#   PROPHECY_CUBE_ROOT  default: parent of scripts/
#   CUBALC_STATE        default: $ROOT/cubalc/state
#   EVOLVE_HZ           default: 5
#   CUBALC_ALGO_DEEP_SEC default: 396  (6.6 min)
#   GUARD_SLEEP_SEC     default: 30    (watch period)
#   HOLD_FLASH          default: 1
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PC="${PROPHECY_CUBE_ROOT:-$(cd "$SCRIPT_DIR/.." && pwd)}"
CUBALC_ROOT="${CUBALC_ROOT:-$PC/cubalc}"
BIN="${CUBALC_BIN:-$CUBALC_ROOT/out/cubalc}"
STATE="${CUBALC_STATE:-$CUBALC_ROOT/state}"
EVOLVE_DIR="$STATE/evolve"
PIDF="$EVOLVE_DIR/loop.pid"
LOG="$EVOLVE_DIR/loop.log"
GUARD_PIDF="$EVOLVE_DIR/guard.pid"
GUARD_LOG="$EVOLVE_DIR/guard.log"
HZ="${EVOLVE_HZ:-5}"
DEEP_SEC="${CUBALC_ALGO_DEEP_SEC:-396}"
SLEEP_SEC="${GUARD_SLEEP_SEC:-30}"
HOLD_FLASH="${HOLD_FLASH:-1}"
AGENT_OPS="${AGENT_OPS:-/data/Workdir/voldemar/Dev/agent_ops/active}"
[[ -d "$AGENT_OPS" ]] || AGENT_OPS="${HOME}/Dev/agent_ops/active"
PN_ROOT="${PROJECT_NEXUS_ROOT:-/data/Workdir/voldemar/ProjectNexus}"
AUTH_JSON="${GROK_AUTH_JSON:-$HOME/.grok/auth.json}"
MAX_RESTART_BURST="${MAX_RESTART_BURST:-5}"     # restarts in window before escalate
RESTART_WINDOW_SEC="${RESTART_WINDOW_SEC:-300}" # 5 min burst window

mkdir -p "$EVOLVE_DIR" "$STATE" 2>/dev/null || true

ts() { date -Is 2>/dev/null || date '+%Y-%m-%dT%H:%M:%S%z'; }
log() {
  local line="[guard $(ts)] $*"
  printf '%s\n' "$line" | tee -a "$GUARD_LOG" >/dev/null
  printf '%s\n' "$line" >&2
}

# --- process helpers ---
pid_alive() {
  local p="${1:-}"
  [[ -n "$p" ]] && kill -0 "$p" 2>/dev/null
}

read_pid() {
  local f="$1"
  [[ -f "$f" ]] || { echo ""; return; }
  tr -d ' \n\r\t' <"$f" 2>/dev/null || echo ""
}

is_evolve_cmd() {
  local p="$1"
  [[ -r "/proc/$p/cmdline" ]] || return 1
  tr '\0' ' ' <"/proc/$p/cmdline" 2>/dev/null | grep -q 'evolve-loop'
}

# --- Commander plate (only when self-recovery fails) ---
reach_commander() {
  local reason="$1"
  local detail="${2:-}"
  local stamp
  stamp="$(date -u +%Y%m%dT%H%M%SZ 2>/dev/null || date +%Y%m%d%H%M%S)"
  local plate_dir="$AGENT_OPS"
  mkdir -p "$plate_dir" "$STATE" "$EVOLVE_DIR" 2>/dev/null || true

  local plate="$plate_dir/COMMANDER_ALERT_CUBALC_SESSION_${stamp}.json"
  local latest="$plate_dir/COMMANDER_ALERT_CUBALC_SESSION_LATEST.json"
  local local_plate="$STATE/COMMANDER_ALERT_LATEST.json"

  # shell-safe JSON (no python dependency)
  cat >"$plate" <<EOF
{
  "schema": "commander.alert.v1",
  "to": "BlackCube Commander",
  "from": "cubalc_session_guard",
  "priority": "high",
  "hold_flash": ${HOLD_FLASH},
  "law": "the_loop_must_go_on",
  "reason": $(printf '%s' "$reason" | sed 's/\\/\\\\/g; s/"/\\"/g; s/^/"/; s/$/"/'),
  "detail": $(printf '%s' "$detail" | sed 's/\\/\\\\/g; s/"/\\"/g; s/^/"/; s/$/"/'),
  "self_recovery": "exhausted",
  "action_requested": "login_or_restore_session_if_auth · inspect evolve loop · HOLD_FLASH",
  "paths": {
    "cubalc": "$CUBALC_ROOT",
    "state": "$STATE",
    "loop_log": "$LOG",
    "guard_log": "$GUARD_LOG",
    "auth": "$AUTH_JSON"
  },
  "creed": "Cube is SoT · OS is way · devices free · loop continues when possible",
  "ts": "$(ts)"
}
EOF
  cp -f "$plate" "$latest" 2>/dev/null || true
  cp -f "$plate" "$local_plate" 2>/dev/null || true
  cp -f "$plate" "$EVOLVE_DIR/COMMANDER_ALERT_LATEST.json" 2>/dev/null || true

  # ProjectNexus prophecy state
  if [[ -d "$PN_ROOT/var/prophecy/state" ]]; then
    cp -f "$plate" "$PN_ROOT/var/prophecy/state/COMMANDER_ALERT_CUBALC_SESSION_LATEST.json" 2>/dev/null || true
  fi

  # Human-readable desk note
  {
    echo "# COMMANDER ALERT — CubalC session guard"
    echo
    echo "- **ts:** $(ts)"
    echo "- **reason:** $reason"
    echo "- **detail:** $detail"
    echo "- **self-recovery:** exhausted"
    echo "- **action:** restore login/session if auth; loop must go on"
    echo "- **plate:** \`$plate\`"
    echo "- **logs:** \`$GUARD_LOG\` · \`$LOG\`"
  } >"$plate_dir/COMMANDER_ALERT_CUBALC_SESSION_LATEST.md" 2>/dev/null || true

  log "COMMANDER ALERT: $reason — $detail → $plate"

  # Best-effort NexusCore / Cube way pings (never block the guard)
  if command -v curl >/dev/null 2>&1; then
    curl -sS -m 2 -X POST "http://127.0.0.1:17333/v1/coord/station" \
      -H 'Content-Type: application/json' \
      -d "{\"schema\":\"NEXUS_COORD.v1\",\"from\":\"cubalc_session_guard\",\"type\":\"COMMANDER_ALERT\",\"role\":\"session_guard\",\"status\":\"NEED_COMMANDER\",\"hold_flash\":${HOLD_FLASH},\"reason\":\"${reason//\"/}\",\"ts\":\"$(ts)\"}" \
      >/dev/null 2>&1 || true
  fi
}

# --- auth / login probe ---
auth_ok() {
  # No auth file → not necessarily fatal for pure-C evolve (offline Cube path)
  if [[ ! -f "$AUTH_JSON" ]]; then
    echo "missing"
    return 1
  fi
  if [[ ! -s "$AUTH_JSON" ]]; then
    echo "empty"
    return 1
  fi
  # corrupt sibling markers from Grok Build
  if ls "${AUTH_JSON}.corrupt."* >/dev/null 2>&1; then
    # recent corrupt within 1 hour is a signal
    local recent
    recent="$(find "$(dirname "$AUTH_JSON")" -maxdepth 1 -name 'auth.json.corrupt.*' -mmin -60 2>/dev/null | head -1 || true)"
    if [[ -n "$recent" ]]; then
      echo "corrupt_recent"
      return 1
    fi
  fi
  # must look like JSON object
  if ! head -c 1 "$AUTH_JSON" 2>/dev/null | grep -q '{'; then
    echo "not_json"
    return 1
  fi
  echo "ok"
  return 0
}

# --- ensure binary ---
ensure_binary() {
  if [[ -x "$BIN" ]]; then
    return 0
  fi
  log "binary missing — rebuild cubalc"
  if [[ -f "$CUBALC_ROOT/Makefile" ]]; then
    if make -C "$CUBALC_ROOT" all >>"$GUARD_LOG" 2>&1; then
      log "rebuild ok: $BIN"
      return 0
    fi
  fi
  log "rebuild FAILED"
  return 1
}

# --- start evolve-loop ---
start_loop() {
  ensure_binary || return 1
  mkdir -p "$EVOLVE_DIR"
  # clear stale pid
  local old
  old="$(read_pid "$PIDF")"
  if pid_alive "$old" && is_evolve_cmd "$old"; then
    log "loop already alive pid=$old"
    return 0
  fi
  if pid_alive "$old" && ! is_evolve_cmd "$old"; then
    log "stale pid $old is not evolve-loop — replacing"
    kill "$old" 2>/dev/null || true
    sleep 1
  fi

  export CUBALC_STATE="$STATE"
  export CUBALC_ALGO_DEEP_SEC="$DEEP_SEC"
  export PROPHECY_CUBE_ROOT="$PC"
  nohup "$BIN" evolve-loop --hz "$HZ" >>"$LOG" 2>&1 &
  local np=$!
  echo "$np" >"$PIDF"
  sleep 1
  if pid_alive "$np" && is_evolve_cmd "$np"; then
    log "started evolve-loop pid=$np hz=$HZ deep=${DEEP_SEC}s state=$STATE"
    # heart plate
    printf '%s\n' "{\"schema\":\"cube.session.heartbeat.v1\",\"ok\":true,\"pid\":$np,\"hz\":$HZ,\"deep_sec\":$DEEP_SEC,\"ts\":\"$(ts)\",\"engine\":\"C\",\"cmd\":\"evolve-loop\"}" \
      >"$EVOLVE_DIR/SESSION_HEARTBEAT.json"
    return 0
  fi
  log "start failed — process died immediately"
  return 1
}

# --- check loop health ---
loop_healthy() {
  local p
  p="$(read_pid "$PIDF")"
  if ! pid_alive "$p"; then
    echo "dead"
    return 1
  fi
  if ! is_evolve_cmd "$p"; then
    echo "wrong_cmd"
    return 1
  fi
  # log must advance within last 15 minutes if races are happening (hz=5 → frequent)
  if [[ -f "$LOG" ]]; then
    local age
    age=$(( $(date +%s) - $(stat -c %Y "$LOG" 2>/dev/null || echo 0) ))
    if [[ "$age" -gt 900 ]]; then
      echo "log_stale_${age}s"
      return 1
    fi
  fi
  echo "ok"
  return 0
}

# --- cube control plane optional ---
ensure_cube_way() {
  if ! command -v curl >/dev/null 2>&1; then return 0; fi
  if curl -sS -m 2 http://127.0.0.1:17333/health >/dev/null 2>&1; then
    return 0
  fi
  local daemon="$PN_ROOT/bin/cube_daemon"
  if [[ -x "$daemon" ]]; then
    log "cube_daemon down — restart --no-track :17333"
    nohup "$daemon" --no-track --port 17333 \
      >>"${PN_ROOT}/var/run/cube_daemon.log" 2>&1 &
    echo $! >"${PN_ROOT}/var/run/cube_daemon.pid" 2>/dev/null || true
    sleep 1
  fi
}

# restart burst tracking
RESTART_TIMES_FILE="$EVOLVE_DIR/guard_restarts.txt"
record_restart() {
  date +%s >>"$RESTART_TIMES_FILE"
  # prune old
  local now cut
  now=$(date +%s)
  cut=$((now - RESTART_WINDOW_SEC))
  if [[ -f "$RESTART_TIMES_FILE" ]]; then
    awk -v c="$cut" '$1>=c' "$RESTART_TIMES_FILE" >"${RESTART_TIMES_FILE}.tmp" 2>/dev/null || true
    mv -f "${RESTART_TIMES_FILE}.tmp" "$RESTART_TIMES_FILE" 2>/dev/null || true
  fi
}
restart_burst_count() {
  [[ -f "$RESTART_TIMES_FILE" ]] || { echo 0; return; }
  local now cut
  now=$(date +%s)
  cut=$((now - RESTART_WINDOW_SEC))
  awk -v c="$cut" '$1>=c{n++} END{print n+0}' "$RESTART_TIMES_FILE"
}

# --- one health pass ---
health_pass() {
  local ok=0
  local issues=()

  # 1) binary
  if ! ensure_binary; then
    issues+=("binary_missing")
  fi

  # 2) auth (soft — C evolve can run offline; escalate only if Grok session required AND broken)
  local astatus
  astatus="$(auth_ok || true)"
  if [[ "$astatus" != "ok" && "$astatus" != "missing" ]]; then
    log "auth probe: $astatus"
    # try self-recovery: if .bak exists restore
    if [[ -f "${AUTH_JSON}.bak" ]] && [[ "$astatus" == "empty" || "$astatus" == "not_json" ]]; then
      log "auth self-recovery: restore from .bak"
      cp -f "${AUTH_JSON}.bak" "$AUTH_JSON" 2>/dev/null || true
      astatus="$(auth_ok || true)"
    fi
    if [[ "$astatus" == "corrupt_recent" || "$astatus" == "empty" || "$astatus" == "not_json" ]]; then
      issues+=("auth_${astatus}")
    fi
  fi

  # 3) evolve loop
  local lstatus
  lstatus="$(loop_healthy || true)"
  if [[ "$lstatus" != "ok" ]]; then
    log "loop unhealthy: $lstatus — restarting"
    record_restart
    if start_loop; then
      log "loop recovered"
      ok=1
    else
      issues+=("loop_start_failed:${lstatus}")
    fi
  else
    ok=1
  fi

  # 4) cube way optional
  ensure_cube_way || true

  # 5) deep-opt plate freshness (warn only — deep is every 6.6 min)
  if [[ -f "$EVOLVE_DIR/ALGOCUBE_DEEP_OPT.json" ]]; then
    local dage
    dage=$(( $(date +%s) - $(stat -c %Y "$EVOLVE_DIR/ALGOCUBE_DEEP_OPT.json" 2>/dev/null || echo 0) ))
    # if older than 3 intervals (~20 min) something is wrong
    if [[ "$dage" -gt $(( ${DEEP_SEC%.*} * 3 + 120 )) ]]; then
      log "deep-opt plate stale ${dage}s — forcing deep pass"
      if [[ -x "$BIN" ]]; then
        CUBALC_STATE="$STATE" "$BIN" evolve --deep >>"$LOG" 2>&1 || true
      fi
    fi
  fi

  # 6) heartbeat always
  local p
  p="$(read_pid "$PIDF")"
  printf '%s\n' "{\"schema\":\"cube.session.heartbeat.v1\",\"ok\":$( [[ $ok -eq 1 ]] && echo true || echo false ),\"pid\":${p:-0},\"loop\":\"$lstatus\",\"auth\":\"$astatus\",\"ts\":\"$(ts)\",\"engine\":\"C\",\"deep_sec\":$DEEP_SEC,\"guard\":\"alive\"}" \
    >"$EVOLVE_DIR/SESSION_HEARTBEAT.json"

  # escalate if burst or hard issues
  local burst
  burst="$(restart_burst_count)"
  if [[ "$burst" -ge "$MAX_RESTART_BURST" ]]; then
    reach_commander "evolve_loop_crash_burst" "restarts=${burst} in ${RESTART_WINDOW_SEC}s window; last=${lstatus}"
    # clear burst file so we don't spam every tick — leave one breadcrumb
    : >"$RESTART_TIMES_FILE"
  fi

  for iss in "${issues[@]+"${issues[@]}"}"; do
    case "$iss" in
      auth_*)
        reach_commander "session_login_or_auth_required" "$iss — restore Grok/Commander login; pure-C loop still attempted"
        ;;
      loop_start_failed*)
        reach_commander "evolve_loop_cannot_start" "$iss — binary=$BIN state=$STATE"
        ;;
      binary_missing)
        reach_commander "cubalc_binary_build_failed" "make -C $CUBALC_ROOT all failed"
        ;;
    esac
  done

  return $(( ok == 1 ? 0 : 1 ))
}

status_report() {
  local p astatus lstatus
  p="$(read_pid "$PIDF")"
  astatus="$(auth_ok || true)"
  lstatus="$(loop_healthy || true)"
  echo "PROPHECY_CUBE_ROOT=$PC"
  echo "CUBALC_BIN=$BIN"
  echo "CUBALC_STATE=$STATE"
  echo "loop_pid=$p alive=$(pid_alive "$p" && echo yes || echo no) health=$lstatus"
  echo "auth=$astatus"
  echo "deep_sec=$DEEP_SEC hz=$HZ"
  echo "guard_log=$GUARD_LOG"
  echo "loop_log=$LOG"
  if [[ -f "$EVOLVE_DIR/SESSION_HEARTBEAT.json" ]]; then
    echo "heartbeat:"
    cat "$EVOLVE_DIR/SESSION_HEARTBEAT.json"
    echo
  fi
  if [[ -f "$EVOLVE_DIR/ALGOCUBE_DEEP_OPT.json" ]]; then
    echo "last_deep_opt_age_s=$(( $(date +%s) - $(stat -c %Y "$EVOLVE_DIR/ALGOCUBE_DEEP_OPT.json") ))"
  fi
  if pid_alive "$p"; then
    ps -p "$p" -o pid,etime,cmd= 2>/dev/null || true
  fi
  tail -5 "$LOG" 2>/dev/null || true
}

main_loop() {
  echo $$ >"$GUARD_PIDF"
  log "session guard START — the loop must go on (hold_flash=$HOLD_FLASH)"
  log "cubalc=$BIN state=$STATE hz=$HZ deep=${DEEP_SEC}s sleep=${SLEEP_SEC}s"

  # ensure loop immediately
  if ! loop_healthy >/dev/null 2>&1; then
    start_loop || reach_commander "initial_loop_start_failed" "could not start on guard boot"
  fi

  while true; do
    health_pass || true
    sleep "$SLEEP_SEC"
  done
}

# --- CLI ---
cmd="${1:-}"
case "$cmd" in
  --status|status)
    status_report
    exit 0
    ;;
  --once|once)
    health_pass
    exit $?
    ;;
  --daemon|daemon)
    if pid_alive "$(read_pid "$GUARD_PIDF")"; then
      log "guard already running pid=$(read_pid "$GUARD_PIDF")"
      exit 0
    fi
    nohup bash "$SCRIPT_DIR/cubalc_session_guard.sh" --foreground \
      >>"$GUARD_LOG" 2>&1 &
    echo $! >"$GUARD_PIDF"
    sleep 1
    log "daemonized guard pid=$(read_pid "$GUARD_PIDF")"
    status_report
    exit 0
    ;;
  --foreground|foreground|"")
    # default: foreground forever
    main_loop
    ;;
  --help|-h)
    sed -n '2,20p' "$0"
    exit 0
    ;;
  *)
    echo "unknown: $cmd (try --status --once --daemon)" >&2
    exit 2
    ;;
esac
