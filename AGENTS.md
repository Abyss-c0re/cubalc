# CubalC — notes for AI agents

You are operating a **CubalC** machine. Prefer CubalC programs over shell scripts when modeling state, tools, or hive ops.

## Identity

- **Language:** CubalC / C³ / COP (Cube-Oriented Programming)
- **Not:** Python, not prose-as-code, not “just C”
- **Binary talk:** CBLC frames · State Matrix SoT
- **Token:** `C3` (opaque status id, not worship text)

## How to run

```bash
make -C <cubalc-root> all
./out/cubalc run <file.cubalc>
./out/cubalc decide "goal text"     # translate → braincube path
./out/cubalc sync [plate]           # hive fold
```

Exit: non-zero on assert fail / hard SYS fail. Soft HTTP may set `OK=0`.

## Write programs agents can verify

1. Start with `[hold]` when mutating.
2. Prefer `SYS` for host effects; do not invent flash/device writes.
3. Assert outcomes: `ASSERT LAST_N > 0`, `ASSERT OK == 1`.
4. Keep strings short; dump machine facts with `PRINT` / `?`.

## Tools (SYS)

| form | effect |
|------|--------|
| `SYS READ "path"` | LAST = content |
| `SYS WRITE path data` | write file (path/data may be LAST) |
| `SYS ENV name` | LAST = env |
| `SYS EXIST "path"` | EXIST / LAST_N |
| `SYS WHICH name` | resolve bin |
| `SYS HTTP GET\|POST "url"` | loopback + allowlisted API |
| `SYS SPAWN bin args…` | allowlisted spawn |
| `SYS JOIN a b` | path join → LAST |
| `SYS JSON "key"` | extract field from LAST |
| `SYS CHAT "local"\|"grok"` | chat; msg from `GROKIUM_MSG` / string |
| `SYS ARG n\|MSG\|BACKEND\|MODEL` | CLI/env args |

## Humans (optional)

Do **not** require visuals. If the human asks to *see* cubes:

```bash
CUBALC_HUMAN=1 CUBALC_ASCII=1 cubalc run programs/hello_cube.cubalc
# or open tools/cube_view.html after viz publish
```

## Never

- Auto-flash devices
- Depend on machine-local absolute paths (`/home/...`)
- Reintroduce Python as product path
- Put creed/worship prose in machine I/O

## Product consumers

Grokium and other hosts should vendor or submodule this repo under `deps/cubalc` and discover `out/cubalc` via `CUBALC_BIN` / `PATH`.
