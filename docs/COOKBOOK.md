# CubalC cookbook — usable recipes

**HOLD_FLASH is user permission before plug-in** (not auto-flash).  
**Wire is SMX2 binary** (HTTP never required for peers).

## 0. Doctor

```bash
make all
./out/cubalc doctor      # JSON readiness
./out/cubalc cookbook    # paths
./out/cubalc help
```

## 1. Hello hold → place → plug → assert

```cubalc
HOLD_FLASH 1
CUBE a ROLE host PROTON 1
CUBE b ROLE body PROTON 1
SETBIT a 0 1
SETBIT a 1 1
SETBIT b 0 1
PLUG a b
ASSERT OK == 1 "plug needs HOLD_FLASH 1"
PRINT "hello" CUBES UNITY
```

```bash
./out/cubalc run programs/hello_cube.cubalc
./out/cubalc run programs/proof/12_hold_flash_plug.cubalc
```

## 2. Decide from matrix

```cubalc
HOLD_FLASH 1
CUBE peer0 ROLE host PROTON 1
CUBE brain ROLE braincube PROTON 1
PLUG peer0 brain
FOLDBITS peer0 "10101011110000"
SETDIGIT peer0 5
DECIDE brain
ASSERT DECIDE >= 0 "digit 0..9"
ASSERT DECIDE <= 9
```

## 3. In-process SMX mesh

```cubalc
HOLD_FLASH 1
CUBE a ROLE host PROTON 1
CUBE b ROLE body PROTON 1
PLUG a b
SETBIT a 0 1
SETBIT a 1 1
SETBIT a 2 1
SMX KEY
SMX EXCHANGE a b
ASSERT SMX_OK == 1 "secure talk"
ASSERT SMX_TALKS >= 2
```

```bash
export CUBALC_SMX_KEY="$(openssl rand -hex 32)"
./out/cubalc run programs/p2p/mesh_local.cubalc
```

## 4. Cross-process P2P (two shells)

```bash
export CUBALC_SMX_KEY="$(openssl rand -hex 32)"
# optional: fail faster if no peer dials
export CUBALC_P2P_TIMEOUT=5000   # ms for SMX SERVE accept

# shell B
CUBALC_P2P_SERVE=1 CUBALC_P2P_BIND=127.0.0.1:7733 \
  ./out/cubalc run programs/p2p/nanobot_peer.cubalc

# shell A
CUBALC_P2P_PEER=127.0.0.1:7733 \
  ./out/cubalc run programs/p2p/nanobot_peer.cubalc

# soft dial (no fatal if peer missing)
CUBALC_P2P_SOFT=1 CUBALC_P2P_PEER=127.0.0.1:1 \
  ./out/cubalc run programs/p2p/peer_dial.cubalc
```

CLI bus:

```bash
./out/cubalc smx-bus prove-tcp
```

## 5. Core protect

```bash
export CUBALC_PROTECT=1
./out/cubalc protect
./out/cubalc run programs/protect/core_protect.cubalc
```

## 6. INCLUDE a lib snippet

From a program under `programs/…` (INCLUDE resolves relative to file dir and `programs/`):

```cubalc
INCLUDE "lib/hold_seed.cubalc"
CUBE self ROLE host PROTON 1
CUBE brain ROLE braincube PROTON 1
PLUG self brain
```

## 7. SYS ENV with default

```cubalc
SYS ENV "CUBALC_P2P_BIND" OR "127.0.0.1:7733"
PRINT "bind" LAST LAST_N
ASSERT LAST_N > 0 "bind string present"
```

## Env cheat sheet

| env | use |
|-----|-----|
| `CUBALC_SMX_KEY` | 64-hex shared secret |
| `CUBALC_P2P_BIND` / `PEER` / `SERVE` | mesh |
| `CUBALC_P2P_TIMEOUT` | SERVE accept timeout ms (default 30000) |
| `CUBALC_P2P_SOFT` | DIAL soft-fail (SMX_OK=0) |
| `CUBALC_PROTECT` | tight host SPAWN/HTTP |
| `CUBALC_STATE` | state directory |
