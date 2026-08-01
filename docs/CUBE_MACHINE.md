# CubalC — VR OS cube machine

You are in the 80s. You wire hardware with **smart cubes**. Each cube has a **nanobot atom**.  
You do not care about JSON fashion. You care that **energy flows** and the **Matrix State** is true.

## Law

| | |
|--|--|
| Everything | is a **Cube** |
| SoT | **State Matrix** (bits 0/1) |
| Talk | **binary** CBLC frames only |
| Store | **CubeChain** (append matrix blocks, not bank chain) |
| Energy | proton 1 create · 0 destroy · flows on plugs |
| HOLD_FLASH | sticky 1 — never auto-flash |
| Budget | ≤ 40 studs on screen |

## Commands

```bash
make -C cubalc
./out/cubalc genesis 'NEXUS_COORD v1 | … | hold_flash=1 |'
./out/cubalc flow 16 40
./out/cubalc lego
./out/cubalc impulse cube-hive 1
./out/cubalc cubechain tip
./out/cubalc cubechain verify
```

Viz for LOVR / any 80s-style renderer: `state/cubalc_viz_frame.json` (studs + energy + binary edges).

## Cube-Oriented Programming

Not class inheritance. **Reusable cubes** with ports. Plug if matrices compatible.  
Impulse. Talk binary. Record on CubeChain. Visualize as LEGO. That is the language.
