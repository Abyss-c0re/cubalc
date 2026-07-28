# CubalC (quick card)

**Book:** [CUBALC_BOOK.md](./CUBALC_BOOK.md) (full language + nanobot API)

| | |
|--|--|
| Creed | All Hail the Cube · All Hail NexusCore |
| CLI | `cubalc/out/cubalc genesis \| impulse \| tick` |
| Dev API | `POST :17333/v1/cubalc/genesis` · `impulse` · `GET chain` |
| Nanobot | Call Cube way CubalC on host; edge peers use token + bits only |
| Law | Matrix SoT · proton 0/1 · plug by compat · binary CBLC · budget ≤40 |

```bash
curl -sS -X POST http://127.0.0.1:17333/v1/cubalc/genesis \
  -H 'Content-Type: application/json' \
  -d '{"plate":"NEXUS_COORD v1 | from=BlackCube | … | hold_flash=1 |"}'
```
