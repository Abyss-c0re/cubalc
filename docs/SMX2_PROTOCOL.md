# SMX2 — Secure State Matrix Exchange (CubalC)

**All Hail the Cube · state_matrix_only · HOLD_FLASH=1 · http_required=0**

Peers exchange **binary State Matrix frames** — not HTTP, not JSON chat.

| | |
|--|--|
| Proto | **SMX2** (`CUBALC_PROTO_SMX2`) |
| Magic | `CBLC` |
| MAC | HMAC-SHA256 |
| Replay | reject `seq <= last_rx` |
| Key | `CUBALC_SMX_KEY` (64 hex) · `CUBALC_SMX_KEY_FILE` · `CUBALC_SMX_TOKEN` / peer_token |
| Wire | **`u32le length` + sealed SMX2 frame** |

## Transports (all no HTTP)

| Transport | Use |
|-----------|-----|
| In-process | `SMX TALK` / `SMX EXCHANGE` |
| File bus | `SMX SEAL` / `SMX OPEN` · `.cblc` |
| AF_UNIX | `smx-bus listen\|connect path` |
| **TCP network** | **`smx-bus serve` / `smx-bus dial host:port`** · `SMX DIAL` |

## Cross-device (LAN / WAN)

Shared secret required on both devices:

```bash
export CUBALC_SMX_KEY="$(openssl rand -hex 32)"   # same on both hosts
# Device B (listener)
cubalc smx-bus serve 0.0.0.0:7733
# Device A (caller)
cubalc smx-bus dial 192.168.8.50:7733
```

Or in CubalC (caller):

```cubalc
SMX KEY
SMX DIAL peer0 peer1 "192.168.8.50:7733"
ASSERT SMX_OK == 1
```

Local network proof (two processes, TCP loopback):

```bash
cubalc smx-bus prove-tcp 17733
```

## Wire layout

```
[ 4 bytes little-endian frame length N ]
[ N bytes SMX2 sealed frame: hdr + matrix bits + HMAC-SHA256 ]
```

Frame is fail-closed without key. HOLD_FLASH sticky on every open.

## C API

`cubalc_smx_send_frame` · `cubalc_smx_recv_frame`  
`cubalc_smx_tcp_listen` · `cubalc_smx_tcp_connect`  
`cubalc_smx_unix_listen` · `cubalc_smx_unix_connect`

## Law

**Law of Manifestation:** peers manifest by SMX matrix exchange.  
**No HTTP required** for CubalC. HTTP is optional host edge only.
