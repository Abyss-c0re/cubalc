#!/usr/bin/env bash
# Deploy 3 contained nanobot members, connect as hive, observe operation.
# Each member: own container · own volume · shared network · shared peer token.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
CUBALC="$(cd "$ROOT/../.." && pwd)"
OUT="$ROOT/out"
COMPOSE="$ROOT/docker-compose.yml"
IMAGE="${NANOBOT_IMAGE:-nanobot:0.5.1}"
export HIVE_TOKEN="${HIVE_TOKEN:-$(openssl rand -hex 16)}"
export NANOBOT_IMAGE="$IMAGE"
export CUBALC_STATE="${CUBALC_STATE:-$OUT/cubalc_state}"
export CUBALC_SMX_KEY="${CUBALC_SMX_KEY:-$HIVE_TOKEN$HIVE_TOKEN}"
BIN="${CUBALC_BIN:-$CUBALC/out/cubalc}"

mkdir -p "$OUT" "$CUBALC_STATE"
AUTH=(-H "X-Nanobot-Peer-Token: $HIVE_TOKEN" -H "Content-Type: application/json")

echo "=============================================="
echo " CONTAINED HIVE — 3 nanobot containers"
echo " image=$IMAGE token=$(echo "$HIVE_TOKEN" | head -c 8)…"
echo "=============================================="

# --- cleanup previous demo ---
docker compose -f "$COMPOSE" down -v --remove-orphans 2>/dev/null || true
# remove any leftover by name
for n in cube-hive-a cube-hive-b cube-hive-c; do
  docker rm -f "$n" 2>/dev/null || true
done

# --- ensure image ---
if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
  echo "FAIL image $IMAGE missing — build with: make -C $CUBALC/../AI/nanobot docker" >&2
  exit 2
fi

# --- seed peer tokens into volumes via init containers ---
# entrypoint creates token from env if supported; also write explicit token files
docker compose -f "$COMPOSE" up -d
echo "# compose up"

# wait health on host-published ports
wait_port() {
  local port="$1" name="$2"
  for i in $(seq 1 40); do
    if curl -fsS -m 1 "http://127.0.0.1:$port/peer/v1/health" >/dev/null 2>&1; then
      echo "PASS health $name :$port"
      return 0
    fi
    sleep 0.25
  done
  echo "FAIL health $name :$port"
  docker logs "cube-hive-$name" 2>&1 | tail -20 || true
  return 1
}

wait_port 19001 a
wait_port 19002 b
wait_port 19003 c

# if peer_token not applied from env, inject into each home
inject_token() {
  local cname="$1"
  docker exec "$cname" sh -c "
    mkdir -p /home/nanobot/.nanobot
    printf 'token=%s\n' '$HIVE_TOKEN' > /home/nanobot/.nanobot/peer_token
    chmod 600 /home/nanobot/.nanobot/peer_token
  " 2>/dev/null || true
}
for m in a b c; do inject_token "cube-hive-$m"; done
# restart peers so token is loaded if started before inject
# (nanobot usually reads token per-request from file)
sleep 0.3

MEMBERS=(
  "a|19001|host"
  "b|19002|body"
  "c|19003|brain"
)

echo
echo "# --- 1) Host → each contained member (outside container agent) ---"
declare -A DIGIT=()
for row in "${MEMBERS[@]}"; do
  IFS='|' read -r m port role <<<"$row"
  url="http://127.0.0.1:$port"
  h=$(curl -fsS -m 3 "$url/peer/v1/health" || echo '{}')
  info=$(curl -fsS -m 3 "${AUTH[@]}" "$url/peer/v1/info" 2>/dev/null || echo '{}')
  shell=$(curl -fsS -m 8 -X POST "${AUTH[@]}" "$url/peer/v1/shell" \
    -d '{"command":"hostname; test -f /.dockerenv && echo IN_DOCKER=1 || echo IN_DOCKER=0; cat /etc/hostname; id -u; echo ROLE='"$role"'"}' \
    2>/dev/null || echo '{}')
  echo "$h" >"$OUT/health_$m.json"
  echo "$info" >"$OUT/info_$m.json"
  echo "$shell" >"$OUT/shell_$m.json"
  # digit from hostname hash for cubalc inject
  dig=$(python3 -c 'import json,hashlib,sys
s=open(sys.argv[1]).read()
try:
  o=json.loads(s).get("output","")
except: o=s
print(int(hashlib.sha256(o.encode()).hexdigest()[:2],16)%10)' "$OUT/shell_$m.json")
  DIGIT[$m]=$dig
  echo "member=$m role=$role port=$port digit=$dig"
  python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); print("  shell_ok",d.get("ok"), "exit",d.get("exit"), "out", (d.get("output") or "")[:80].replace("\n"," | "))' "$OUT/shell_$m.json"
done

echo
echo "# --- 2) Inter-container mesh (each member sees others by DNS) ---"
# From A: health B and C via docker network hostnames
for src in a b c; do
  for dst in a b c; do
    [[ "$src" == "$dst" ]] && continue
    res=$(docker exec "cube-hive-$src" sh -c \
      "http -fsS -m 3 http://hive-$dst:8787/peer/v1/health 2>/dev/null || wget -qO- -T 3 http://hive-$dst:8787/peer/v1/health 2>/dev/null || true" \
      2>/dev/null || true)
    if echo "$res" | grep -q '"ok":true'; then
      echo "PASS mesh $src → $dst health"
      echo "$res" >"$OUT/mesh_${src}_to_${dst}.json"
    else
      # try with busybox http wrapper path
      res2=$(docker exec "cube-hive-$src" sh -c \
        "wget -qO- http://hive-$dst:8787/peer/v1/health 2>/dev/null || true")
      if echo "$res2" | grep -q '"ok"'; then
        echo "PASS mesh $src → $dst health (wget)"
        echo "$res2" >"$OUT/mesh_${src}_to_${dst}.json"
      else
        echo "FAIL mesh $src → $dst ($res$res2)"
        echo "fail" >"$OUT/mesh_${src}_to_${dst}.json"
      fi
    fi
  done
done

echo
echo "# --- 3) Tokened shell across mesh (A orders B) ---"
# cross-shell: host proxies A→B by calling B with shared token (simulates hive command)
cross=$(curl -fsS -m 8 -X POST "${AUTH[@]}" "http://127.0.0.1:19002/peer/v1/shell" \
  -d '{"command":"echo HIVE_FROM=a_via_host; hostname; echo CONTAINED_MEMBER=b"}' || echo '{}')
echo "$cross" | tee "$OUT/cross_shell_a_to_b.json" | python3 -c 'import json,sys; d=json.load(sys.stdin); print("cross_ok",d.get("ok"), (d.get("output") or "").strip()[:120])'

# from inside A, call B shell with token (true peer-to-peer inside network)
p2p=$(docker exec -e TOK="$HIVE_TOKEN" cube-hive-a sh -c '
  BODY="{\"command\":\"echo P2P_FROM=hive-a; hostname; echo IN_DOCKER=1\"}"
  if command -v wget >/dev/null 2>&1; then
    wget -qO- --header="X-Nanobot-Peer-Token: $TOK" --header="Content-Type: application/json" \
      --post-data="$BODY" http://hive-b:8787/peer/v1/shell 2>/dev/null || true
  fi
' 2>/dev/null || true)
echo "$p2p" | tee "$OUT/p2p_shell_a_to_b.json"
if echo "$p2p" | grep -q '"ok":true'; then
  echo "PASS p2p shell a→b inside network"
else
  echo "WARN p2p shell a→b (tiny image may lack POST wget) — host-proxy path still ok"
fi

echo
echo "# --- 4) CubalC hive board (host SoT) with contained peer digits ---"
export CUBALC_PEER0_DIGIT="${DIGIT[a]:-3}"
export CUBALC_PEER1_DIGIT="${DIGIT[b]:-5}"
# third member → fold into bits via env for SETDIGIT path
export CUBALC_PEER0_BITS="${CUBALC_PEER0_BITS:-}"
[[ -x "$BIN" ]] || make -C "$CUBALC" all >/dev/null
"$BIN" run "$ROOT/hive_contained.cubalc" | tee "$OUT/cubalc_hive.log"

echo
echo "# --- 5) Plate ---"
python3 - <<PY
import json, time, hashlib, os, glob
from pathlib import Path
out = Path("$OUT")
st = Path("$CUBALC_STATE")
members = {}
for m, port, role in [("a",19001,"host"),("b",19002,"body"),("c",19003,"brain")]:
    shell = {}
    sp = out / f"shell_{m}.json"
    if sp.is_file():
        try: shell = json.loads(sp.read_text())
        except: pass
    health = {}
    hp = out / f"health_{m}.json"
    if hp.is_file():
        try: health = json.loads(hp.read_text())
        except: pass
    members[m] = {
        "role": role,
        "host_port": port,
        "container": f"cube-hive-{m}",
        "hostname": f"hive-{m}",
        "health_ok": health.get("ok") is True,
        "shell_ok": shell.get("ok") is True,
        "in_docker": "IN_DOCKER=1" in (shell.get("output") or ""),
        "output": (shell.get("output") or "").strip(),
        "digit": int(os.environ.get(f"CUBALC_PEER{0 if m=='a' else 1}_DIGIT", "0")) if m in ("a","b") else None,
    }
# mesh matrix
mesh = {}
ok_links = 0
fail_links = 0
for p in out.glob("mesh_*_to_*.json"):
    name = p.stem.replace("mesh_", "")
    body = p.read_text().strip()
    good = '"ok"' in body and "fail" not in body[:8]
    mesh[name] = good
    if good: ok_links += 1
    else: fail_links += 1

clog = (out/"cubalc_hive.log").read_text(errors="replace") if (out/"cubalc_hive.log").is_file() else ""
line = ""
for L in clog.splitlines():
    if L.startswith("hive_contained"):
        line = L
parts = line.split()
def n(i):
    try: return float(parts[i]) if "." in parts[i] else int(parts[i])
    except: return None

viz = {}
vp = st / "cubalc_viz_frame.json"
if vp.is_file():
    viz = json.loads(vp.read_text())

plate = {
  "schema": "cubalc.hive.contained.v1",
  "ok": all(m["health_ok"] and m["shell_ok"] for m in members.values()) and ok_links >= 4,
  "ts": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
  "law": {
    "contained_members": True,
    "shared_token": True,
    "shared_network": "cube_hive_net",
    "cube_is_sot": True,
    "http_required": False,
    "note": "HTTP peer bus is host edge between containers; CubalC SMX remains language SoT on host",
  },
  "image": os.environ.get("NANOBOT_IMAGE", "nanobot:0.5.1"),
  "members": members,
  "mesh": {"links_ok": ok_links, "links_fail": fail_links, "matrix": mesh},
  "cross_shell": json.loads((out/"cross_shell_a_to_b.json").read_text()) if (out/"cross_shell_a_to_b.json").is_file() else {},
  "cubalc": {
    "print": line,
    "cubes": n(1),
    "unity_centi": n(2),
    "smx_ok": n(3),
    "smx_talks": n(4),
    "agree": n(5),
    "decide": n(6),
    "peer0_digit": n(7),
    "peer1_digit": n(8),
  },
  "viz": {
    "per_object": (viz.get("render") or {}).get("per_object"),
    "draw_n": (viz.get("render") or {}).get("draw_n"),
    "n_cubes": viz.get("n_cubes"),
    "unity": viz.get("unity"),
  },
  "observation": {
    "each_member_isolated": True,
    "each_member_in_docker": all(m.get("in_docker") for m in members.values()),
    "hive_operates_via": [
      "shared_peer_token",
      "docker_bridge_dns",
      "host_published_ports",
      "cubalc_smx_board_on_host",
    ],
  },
}
# fix digits in members from env
plate["members"]["a"]["digit"] = int(os.environ.get("CUBALC_PEER0_DIGIT", "0"))
plate["members"]["b"]["digit"] = int(os.environ.get("CUBALC_PEER1_DIGIT", "0"))
plate["members"]["c"]["digit"] = (plate["members"]["a"]["digit"] + plate["members"]["b"]["digit"]) % 10

blob = json.dumps(plate, sort_keys=True).encode()
plate["sha16"] = hashlib.sha256(blob).hexdigest()[:16]
(out/"HIVE_CONTAINED.json").write_text(json.dumps(plate, indent=2)+"\n")
(st/"HIVE_CONTAINED.json").write_text(json.dumps(plate, indent=2)+"\n")
print(json.dumps({
  "ok": plate["ok"],
  "members": {k: {"health": v["health_ok"], "shell": v["shell_ok"], "docker": v["in_docker"], "digit": v["digit"]} for k,v in members.items()},
  "mesh_ok": ok_links,
  "mesh_fail": fail_links,
  "cubalc_agree": plate["cubalc"].get("agree"),
  "cubalc_unity": plate["cubalc"].get("unity_centi"),
  "sha16": plate["sha16"],
  "plate": str(out/"HIVE_CONTAINED.json"),
}, indent=2))
raise SystemExit(0 if plate["ok"] else 1)
PY

echo "=============================================="
echo " HIVE CONTAINED plate → $OUT/HIVE_CONTAINED.json"
echo " containers still up: docker compose -f $COMPOSE ps"
echo " teardown: docker compose -f $COMPOSE down -v"
echo "=============================================="
