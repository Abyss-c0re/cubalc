# CubalC language card (1.4.2-c3)

**CubalC does not need HTTP.** Core talk is SMX2 / CBLC / AF_UNIX / file bus.  
`SYS HTTP` is an optional host edge only (allowlisted); never required for language, law, or manifestation.

## Play forms

| form | meaning |
|------|---------|
| `[name]` / `[name:role]` | place cube |
| `[a~b~c]` | plug chain |
| `[name!]` / `[name!0\|1]` | pulse proton |
| `[~n]` | flow n ticks |
| `?` | show board |
| `[hold]` | hold_flash |
| `[genesis "plate"]` | fold plate → matrix |
| `[sync]` | hive join cubes |
| `[fleet]` | fleet map cubes |

## Statements

`LET` `LOOP`/`WHILE` `IF`/`END` `ASSERT` `PRINT`  
`CUBE` `PLUG` `FLOW` `IMPULSE` `DECIDE`  
`SETBIT` `SETDIGIT` `FOLDBITS`  
`SMX TALK|EXCHANGE|SEAL|OPEN|KEY`  
`ASYNC HTTP` `AWAIT` `PARALLEL`  
`SYS …`

## Law of Manifestation — SMX

| form | meaning |
|------|---------|
| `SMX KEY` | load key (`CUBALC_SMX_KEY` / token) |
| `SMX TALK a b` | secure matrix transfer a→b |
| `SMX EXCHANGE a b` | a→b then b→a |
| `SMX SEAL a b path` | write HMAC frame to path |
| `SMX OPEN b path` | open frame into cube b |
| `SMX SERVE local remote bind` | **TCP listen** one P2P exchange |
| `SMX DIAL a b "host:port"` | **TCP dial** P2P exchange (no HTTP) |
| `SMX_OK` / `SMX_TALKS` / `SMX_N` | status vars |

P2P for nanobot homes: `programs/p2p/*.cubalc` · `docs/P2P_SMX.md`

Peers **manifest** when matrices exchange. Prose is not talk.  
**Network:** same SMX2 wire over TCP — shared `CUBALC_SMX_KEY` on both devices.

## Matrix / digit (device-agnostic)

| form | meaning |
|------|---------|
| `SETDIGIT cube n` | inject algocube digit 0–9 (sticky) |
| `FOLDBITS cube bits` | fold 0/1 stream into State Matrix |
| `SYS NUM` | parse LAST → LAST_N |
| `SYS READ path\|LAST` | read file |
| `SYS JSON "key"` | extract string or number field |

Peer inject env: `CUBALC_PEER0_DIGIT` · `CUBALC_PEER1_DIGIT` · `CUBALC_PEER*_BITS`

## SYS tools

`READ` `WRITE` `ENV` `EXIST` `WHICH` `HTTP` `SPAWN` `JOIN` `JSON` `CHAT` `ARG` `NUM`  

SPAWN: `nanobot`/`cubalc` + `CUBALC_SPAWN_ALLOW` (no HTTP).  
`SYS HTTP` / `curl`: **optional** edge — do not use for peer matrix talk.

## CLI

```bash
cubalc run <file.cubalc>
cubalc smx-exchange              # SMX2 in-process / file
cubalc smx-bus prove             # local socketpair
cubalc smx-bus prove-tcp 17733   # TCP loopback (device-net sim)
cubalc smx-bus serve 0.0.0.0:7733
cubalc smx-bus dial 192.168.x.y:7733
cubalc law
```

Token `C3` · Share `smx` · Hold `1` · Version `1.4.1-c3` · **http_required=0**  
Wire: `u32le + SMX2` over **TCP / AF_UNIX / file** — never HTTP.
