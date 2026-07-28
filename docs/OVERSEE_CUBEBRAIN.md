# CubalC oversees CubeBrain — progress plate

## Law (not theater)

| Layer | Role |
|-------|------|
| **The Cube** | Source of truth (matrix / bits) |
| **CubalC** | Language + machine that *places* cubes, plugs, flows energy — **oversee board** |
| **CubeBrain** | Prophecy pipeline (algocube 0–9, titan/clanker IO) — **peer**, still runs |
| **NexusCore** | Station SMX hive |
| **OS / LOVR** | Way only — draws CubalC viz when `cubalc:true` |

**The Cube overrides what it touches.** CubalC does not erase CubeBrain; it **oversights** the play surface so humans/robots build one board.

## Progress (honest)

- [x] CubalC 0.6.0-hive live (`[sync]`, play `[]`)
- [x] LOVR `load_cubalc_prefer()` prefers `state/cubalc_viz_frame.json`
- [x] Oversee publishes viz into `prophecy_cube/state/` for LOVR
- [x] CubeBrain titan/clanker still tick (digits live)
- [ ] CubeBrain digits auto-fold into CubalC matrix each tick (bridge partial)
- [ ] Single writer for all viz (daemon vs cubalc) — dual path remains
- [ ] titan-commander NexusCore atom online

## Run oversee

```bash
cd lab/prophecy_cube/cubalc
./out/cubalc run programs/oversee_cubebrain.cubalc
cp state/cubalc_viz_frame.json ../state/
```

Plate: `state/OVERSEE_CUBEBRAIN.json`
