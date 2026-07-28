# CubalC

**CubalC (C³)** is an **AI-native programming language** for agents and cube-shaped computation.

| | |
|--|--|
| **For** | AI agents, tool runtimes, hive/control planes |
| **Paradigm** | COP — Cube-Oriented Programming |
| **Talk** | Binary State Matrix (CBLC) · not prose |
| **Runtime** | Pure C · portable · no Python |
| **Humans** | Optional real-cube visuals (`CUBALC_HUMAN=1`, browser viewer) |

> Agents write CubalC. Humans may *see* cubes. The machine never needs the picture.

## Why agents use CubalC

- **Place · plug · pulse · flow · look** — small grammar, stable ops
- **`SYS` host ops** — READ/WRITE/ENV/HTTP/SPAWN/CHAT/JOIN/JSON (allowlisted)
- **State Matrix is SoT** — bits agents can verify, not free text
- **HOLD_FLASH** — never auto-flash hardware
- **JSON + ASCII boards** — easy for tools to parse

## Quick start

```bash
make
./out/cubalc run programs/hello_cube.cubalc
./out/cubalc help
make test
```

Install:

```bash
make install          # → ~/.local/bin/cubalc
# GPU pool (optional):
make USE_OPENCL=1
```

## Language (agent surface)

```cubalc
[hold]
[kernel:sot]
[hive:nanobot]
[kernel~hive]
[kernel!]
[~4]
?

SYS WRITE "/tmp/mark.txt" "ok"
SYS READ "/tmp/mark.txt"
SYS CHAT "local"
PRINT LAST
```

Play dialect `[]` and verb dialect (`CUBE`/`PLUG`/`FLOW`) both work.

## Optional human visuals (real cubes)

Agents ignore these. Humans may enable:

```bash
# ASCII isometric cubes + metrics
make human
# or:
CUBALC_HUMAN=1 ./out/cubalc run programs/hello_cube.cubalc

# Spinning kernel cube
./out/cubalc run programs/hello_cube.cubalc   # ? already prints board
# Browser (open local file after a run that publishes viz):
#   tools/cube_view.html  — loads state/cubalc_viz_frame.json
```

Viz schema: `cube.viz_frame.v1` JSON (positions, colors, edges, matrix bits).

## Agent integration

See **[AGENTS.md](AGENTS.md)** and **[docs/FOR_AGENTS.md](docs/FOR_AGENTS.md)**.

Env:

| var | meaning |
|-----|---------|
| `CUBALC_STATE` | state dir (default `./state`) |
| `CUBALC_BIN` | path to binary for SYS WHICH/SPAWN |
| `CUBALC_HUMAN` | `1` = isometric cube art |
| `CUBALC_ASCII` | `1` = ASCII boards |
| `GROKIUM_MSG` | message for `SYS CHAT` |

## Status

Standalone OSS-ready runtime. Products (e.g. Grokium) depend on CubalC — CubalC does not depend on them.

License: see LICENSE.
