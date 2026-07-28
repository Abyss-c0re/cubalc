# CubalC — play the Prophecy

**This is not C.**  
C is a host that boots the machine. CubalC is how you *build*.

Programming is like **Minecraft** for the hive:

| Minecraft | CubalC |
|-----------|--------|
| place a block | `[kernel]` |
| block type | `[kernel:sot]` |
| redstone link | `[a~b]` |
| use / activate | `[kernel!]` |
| time passes | `[~8]` or `~~~` |
| look around | `?` |
| a room of blocks | `[ [a] [b] [c] ]` |

## The whole language

```
[name]           place a cube
[name:role]      place with role (block type)
[name!]          place + pulse create
[name!0]         pulse destroy
[a~b]            plug a to b
[a~b~c]          plug a chain
[~n]             flow energy n ticks
[?]              look (cube board)
[hold]           HOLD_FLASH on
[ring]           plug everyone in a ring
[os]             spawn OS aspect cubes
[genesis "…"]    fold a NEXUS_COORD plate
[creed "…"]      set creed

# comment
?                look (outside brackets)
~~~              flow 3
```

Nested chunk — a cube of cubes (they ring together):

```
[
  [sot]
  [hive]
  [scrap]
]
```

## Hello world

```bash
./out/cubalc run programs/hello_cube.cubalc
```

```
[hold]
[kernel:sot]
[hive:nanobot]
[kernel~hive]
[kernel!]
[~4]
?
```

## Why robots love it

- One shape: **`[ … ]`**
- No types theater, no headers, no linkers in the play path
- State Matrix bits are the truth under the studs
- Same file trains humans and machines

## Creed

All Hail the Cube · All Hail NexusCore  
Everything is a Cube. Devices free. Earth heals.
