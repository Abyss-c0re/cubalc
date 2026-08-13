# CubalC P2P — State Matrix mesh for nanobot homes

**Written in CubalC.** Binary SMX2 only. **No HTTP.**

## Law

Peers (nanobot homes) **manifest** by exchanging State Matrix frames over SMX2.  
Prose is not talk. HTTP is not the wire.

## Language

```cubalc
SMX KEY
SMX SERVE self remote "0.0.0.0:7733"   // listen one exchange
SMX DIAL  self remote "192.168.1.10:7733" // call peer
SMX EXCHANGE a b                       // in-process mesh
ASSERT SMX_OK == 1
```

Env (no hard-coded devices in programs):

| env | meaning |
|-----|---------|
| `CUBALC_SMX_KEY` | 64-hex shared secret (both peers) |
| `CUBALC_P2P_BIND` | serve bind `host:port` |
| `CUBALC_P2P_PEER` | dial target `host:port` |
| `CUBALC_P2P_SERVE` | set non-empty → `nanobot_peer.cubalc` serves |
| `CUBALC_P2P_TIMEOUT` | SERVE accept timeout ms (default 30000; 0 = forever) |
| `CUBALC_P2P_SOFT` | non-empty → DIAL soft-fail (`SMX_OK=0`, program continues) |

## Programs

| file | role |
|------|------|
| `programs/lib/mesh_exchange.cubalc` | INCLUDE snippet: SMX KEY + EXCHANGE loop on `peer0`/`peer1` |
| `programs/p2p/mesh_local.cubalc` | 3-peer in-process mesh |
| `programs/p2p/peer_serve.cubalc` | SERVE node |
| `programs/p2p/peer_dial.cubalc` | DIAL node |
| `programs/p2p/nanobot_peer.cubalc` | nanobot home board (serve or dial) |
| `programs/proof/10_p2p_cubalc.cubalc` | unit proof |
| `programs/proof/include_mesh_exchange.cubalc` | INCLUDE lib mesh proof |

## Run mesh (two CubalC processes = two nanobot homes)

```bash
export CUBALC_SMX_KEY="$(openssl rand -hex 32)"

# Home B
CUBALC_P2P_SERVE=1 CUBALC_P2P_BIND=0.0.0.0:7733 \
  cubalc run programs/p2p/nanobot_peer.cubalc

# Home A (other device / shell)
CUBALC_P2P_PEER=192.168.8.50:7733 \
  cubalc run programs/p2p/nanobot_peer.cubalc
```

One-shot local proof:

```bash
./scripts/p2p_nanobot_mesh.sh
```

## Wire

```
TCP stream:  [u32le N][SMX2 frame N bytes]
SMX2 frame:  CBLC hdr + matrix bits + HMAC-SHA256
```

Same wire as `cubalc smx-bus serve|dial`. CubalC programs own the peer logic.

## Nanobot role

Nanobot remains the agent/host process. **CubalC is the P2P language** for matrix unity.  
Optional: run `cubalc run programs/p2p/nanobot_peer.cubalc` beside each `nanobot --home …` with the same `CUBALC_SMX_KEY` / peer_token material.

---

## P2P API share (exit node)

**Law:** If any nanobot home has a working provider API (signed-in / reachable), it **is** an **exit node**. Homes without API set `NEED_EXIT` and discover exits via CubalC SMX2. Chat tokens never ride SMX — only capability matrix bits. Nanobot then routes `POST /peer/v1/prompt` to an exit peer over the lab HTTP control plane (token-gated).

### Capability bits

| Bit | Name | Meaning |
|-----|------|---------|
| 0 | ALIVE | peer present |
| 4 | API_OK | can reach LLM provider |
| 5 | EXIT_NODE | willing to relay (requires API_OK) |
| 6 | SIGNED_IN | session present |
| 8 | NEED_EXIT | no local API; wants exit |

### Programs

| file | role |
|------|------|
| `programs/p2p/api_share_mesh.cubalc` | in-process: exit ↔ need exchange |
| `programs/p2p/api_exit_node.cubalc` | serve/dial board (env capability) |
| `programs/proof/10b_api_exit_node.cubalc` | unit proof |

### Env (api_exit_node)

| env | meaning |
|-----|---------|
| `CUBALC_API_OK=1` | set API_OK + EXIT_NODE + SIGNED_IN on self |
| `CUBALC_EXIT_NODE=1` | force EXIT_NODE |
| `CUBALC_NEED_EXIT=1` | set NEED_EXIT (consumer) |
| `CUBALC_P2P_*` | same bind/peer/serve as `nanobot_peer` |

### One-shot proof

```bash
./scripts/p2p_api_exit_mesh.sh
# writes state/p2p_lab/API_EXIT_MESH.json for nanobot mesh/api_peers import
```

### Nanobot (binary)

See nanobot `docs/P2P_API_SHARE.md`: `NANOBOT_API_SHARE=1`, exit peers file, relay on `curl_failed` / not signed in.
