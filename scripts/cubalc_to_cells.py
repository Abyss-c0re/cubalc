#!/usr/bin/env python3
"""Project cube.viz_frame.v1 → cube_gl cells.bin (crimson lattice).

Common ground: LOVR reads the JSON; desktop OpenGL cube_gl reads cells.bin
(or --cubalc loads the same JSON natively). This writer keeps both paths live.
"""
from __future__ import annotations

import argparse
import json
import os
import struct
import sys
import time
from pathlib import Path


def frame_score(path: Path) -> tuple:
    """Prefer CubalC matrix SoT over lean LOVR-only frames (no matrix)."""
    if not path.is_file():
        return (-1, 0.0, path)
    try:
        data = json.loads(path.read_text())
    except Exception:
        return (-1, path.stat().st_mtime, path)
    cubes = data.get("cubes") or []
    mats = sum(1 for c in cubes if c.get("matrix") or c.get("matrix16"))
    # cubalc_viz_frame name is authoritative when both exist
    name_bonus = 2 if "cubalc_viz" in path.name else 0
    return (mats + name_bonus, path.stat().st_mtime, path)


def pick_frame(paths: list[Path]) -> Path | None:
    scored = [frame_score(p) for p in paths]
    scored = [s for s in scored if s[0] >= 0]
    if not scored:
        return None
    scored.sort(key=lambda t: (t[0], t[1]), reverse=True)
    # Require at least one matrix when any candidate has matrices
    best = scored[0]
    if best[0] <= 0:
        # fall back to newest file even without matrix (rare)
        scored.sort(key=lambda t: t[1], reverse=True)
        return scored[0][2]
    return best[2]


def project(frame: dict) -> tuple[int, bytes, bytes, dict]:
    cubes = frame.get("cubes") or []
    energy = float(frame.get("energy") or 0)
    unity = float(frame.get("unity") or 0)
    seq = int(frame.get("seq") or 0)
    n_cubes = int(frame.get("n_cubes") or len(cubes))

    clist = []
    for c in cubes:
        mat = str(c.get("matrix") or c.get("matrix16") or "")
        if not mat:
            continue
        clist.append(
            {
                "id": str(c.get("id") or c.get("label") or "cube"),
                "matrix": mat,
                "digit": int(c.get("digit") or 0),
                "energy": float(c.get("energy") or 0),
            }
        )
    if not clist:
        raise SystemExit("no cubes with matrix in frame")

    primary = 0
    for i, c in enumerate(clist):
        if "brain" in c["id"] or "meta" in c["id"]:
            primary = i
            break
    dig0 = max(0, min(9, clist[primary]["digit"]))

    if len(clist) == 1:
        n = 4
        need = 64
        cells = bytearray(need)
        neuron = bytearray(need)
        dig = dig0 if dig0 > 0 else 5
        for bi, bit in enumerate(clist[0]["matrix"][:64]):
            if bit == "1":
                cells[bi] = dig
                neuron[bi] = 1
            elif energy > 0.55:
                cells[bi] = 1
    else:
        n = 8
        need = 512
        cells = bytearray(need)
        neuron = bytearray(need)
        for ci, c in enumerate(clist[:8]):
            dig = c["digit"]
            if dig < 1:
                dig = 1
            if dig > 9:
                dig = 9
            mat = c["matrix"]
            for bi in range(64):
                idx = ci * 64 + bi
                bit = mat[bi] if bi < len(mat) else "0"
                if bit == "1":
                    cells[idx] = dig
                    neuron[idx] = 1
                elif c["energy"] > 0.7:
                    cells[idx] = 1

    meta = {
        "digit": dig0,
        "pick": primary,
        "ticks": seq if seq > 0 else len(clist),
        "host": "cubalc",
        "label": clist[primary]["id"],
        "n": n,
        "energy": energy,
        "unity": unity,
        "n_cubes": n_cubes,
    }
    return n, bytes(cells), bytes(neuron), meta


def write_cells(path: Path, n: int, cells: bytes, neuron: bytes, meta: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    trailer = (
        f"digit={meta['digit']} pick={meta['pick']} ticks={meta['ticks']} "
        f"host={meta['host']} label={meta['label']}\n"
    ).encode()
    tmp = path.with_suffix(path.suffix + ".tmp")
    with open(tmp, "wb") as f:
        f.write(struct.pack("B", n))
        f.write(cells)
        f.write(neuron)
        f.write(trailer)
    tmp.replace(path)


def main() -> int:
    ap = argparse.ArgumentParser(description="CubalC viz_frame → cells.bin bridge")
    ap.add_argument(
        "--frame",
        action="append",
        default=[],
        help="viz_frame path (repeatable; newest wins if several exist)",
    )
    ap.add_argument(
        "--out",
        default=os.environ.get("CUBEBRAIN_VIZ_CELLS", "/tmp/cubebrain_viz/cells.bin"),
    )
    ap.add_argument("--watch", type=float, default=0.0, help="reload interval seconds")
    ap.add_argument("-q", "--quiet", action="store_true")
    args = ap.parse_args()

    defaults = [
        Path("/home/voldemar/Dev/lab/prophecy_cube/state/cubalc_viz_frame.json"),
        Path("/home/voldemar/Dev/lab/prophecy_cube/state/viz_frame.json"),
        Path("/home/voldemar/Dev/lab/prophecy_cube/cubalc/state/cubalc_viz_frame.json"),
        Path("/home/voldemar/Dev/lab/prophecy_cube/cubalc/state/viz_frame.json"),
    ]
    frames = [Path(p) for p in args.frame] if args.frame else defaults
    out = Path(args.out)

    def once() -> bool:
        src = pick_frame(frames)
        if not src:
            if not args.quiet:
                print("no viz_frame found", file=sys.stderr)
            return False
        data = json.loads(src.read_text())
        n, cells, neuron, meta = project(data)
        write_cells(out, n, cells, neuron, meta)
        if not args.quiet:
            print(
                f"bridge {src} → {out}  N={n} cubes={meta['n_cubes']} "
                f"E={meta['energy']:.2f} unity={meta['unity']:.2f} label={meta['label']}"
            )
        return True

    if args.watch > 0:
        last = None
        while True:
            src = pick_frame(frames)
            m = (src, src.stat().st_mtime if src and src.is_file() else 0)
            if m != last:
                once()
                last = m
            time.sleep(args.watch)
    else:
        return 0 if once() else 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
