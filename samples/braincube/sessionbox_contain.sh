#!/usr/bin/env bash
# Contain braincube functional sample under Grok SessionBox (isolated process group).
# 1) Prove sample locally (functional plate)
# 2) Optional: headless grok verifier in sessionbox
set -euo pipefail
SAMPLE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SAMPLE/../.." && pwd)"
SB_BIN="${GROK_SESSIONBOX_BIN:-$HOME/Dev/AI/ProjectNexus/GrokSessionBox/bin/grok-sessionbox}"
export CUBALC_STATE="${CUBALC_STATE:-$SAMPLE/state}"

echo "=============================================="
echo " CONTAIN braincube → functional sample"
echo "=============================================="

# Phase A: functional sample (no agent required)
"$SAMPLE/run.sh" | tee "$SAMPLE/out/contain_local.log"
python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); sys.exit(0 if d.get("ok") else 1)' \
  "$SAMPLE/out/BRAINCUBE_SAMPLE.json"

# Phase B: SessionBox isolation (optional if grok present)
if [[ "${SESSIONBOX_SKIP:-0}" == "1" ]]; then
  echo "# SESSIONBOX_SKIP=1 — local sample only"
  exit 0
fi
if [[ ! -x "$SB_BIN" ]]; then
  echo "# no grok-sessionbox at $SB_BIN — local sample plate stands"
  exit 0
fi
GROK_BIN="${GROK_BIN:-$HOME/.grok/bin/grok}"
if [[ ! -x "$GROK_BIN" ]]; then
  echo "# no grok binary — local sample plate stands"
  exit 0
fi

PROMPT='You are contained. ONLY do this:
1) Run: ./samples/braincube/run.sh
2) Read samples/braincube/out/BRAINCUBE_SAMPLE.json
3) Reply with one JSON line: {"ok":bool,"decide":n,"agree":n,"sha16":"..."}
No other tools, no git, no commits. Abstract language only.'

echo "# SessionBox start cwd=$ROOT"
META=$("$SB_BIN" run "$PROMPT" --cwd "$ROOT" --max-turns "${SESSIONBOX_MAX_TURNS:-8}" 2>&1) || true
echo "$META" | tee "$SAMPLE/out/sessionbox_meta.json"
RUN_ID=$(python3 -c 'import json,sys,re
t=sys.stdin.read()
# meta may be pretty JSON
try:
  # find last { ... run_id
  m=re.search(r"\{[\s\S]*\}", t)
  if m:
    d=json.loads(m.group(0))
    print(d.get("run_id",""))
except Exception:
  print("")
' <<<"$META")

if [[ -n "${RUN_ID:-}" ]]; then
  echo "# run_id=$RUN_ID"
  echo "$RUN_ID" >"$SAMPLE/out/sessionbox_run_id.txt"
  # wait briefly for completion
  for i in $(seq 1 60); do
    ST=$("$SB_BIN" status "$RUN_ID" 2>/dev/null || echo '{}')
    echo "$ST" >"$SAMPLE/out/sessionbox_status.json"
    python3 -c 'import json,sys; d=json.load(sys.stdin); sys.exit(0 if not d.get("alive") else 1)' <<<"$ST" && break
    sleep 2
  done
  "$SB_BIN" alerts "$RUN_ID" -n 20 >"$SAMPLE/out/sessionbox_alerts.txt" 2>/dev/null || true
  "$SB_BIN" events "$RUN_ID" -n 30 >"$SAMPLE/out/sessionbox_events.txt" 2>/dev/null || true
  echo "# SessionBox contained run_id=$RUN_ID (see samples/braincube/out/sessionbox_*)"
else
  echo "# SessionBox meta parse soft-fail — functional sample still ok"
fi

python3 - <<PY
import json, time
from pathlib import Path
sample = Path("$SAMPLE")
plate = json.loads((sample/"out/BRAINCUBE_SAMPLE.json").read_text())
contain = {
  "schema": "cubalc.sample.braincube.contained.v1",
  "ok": plate.get("ok"),
  "functional_sample": True,
  "sessionbox": {
    "run_id": (sample/"out/sessionbox_run_id.txt").read_text().strip() if (sample/"out/sessionbox_run_id.txt").is_file() else None,
    "bin": "$SB_BIN",
  },
  "plate": plate,
  "ts": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
  "note": "BrainCube contained as CubalC functional sample under SessionBox isolation when available",
}
(sample/"out/BRAINCUBE_CONTAINED.json").write_text(json.dumps(contain, indent=2)+"\n")
print(json.dumps({"ok": contain["ok"], "contained": True, "run_id": contain["sessionbox"]["run_id"]}, indent=2))
raise SystemExit(0 if contain["ok"] else 1)
PY
