# CubalC JIT — Cube Flow

**Version:** 1.3.0-jit  
**Backend:** `x86_64-hybrid` (native ALU/control + host trampoline for cube ops)

## What it is

CubalC is not only an interpreter. Cube Flow **manifests** as:

```
.cubalc source  →  compile  →  .cblc bytecode  →  JIT  →  native x86_64  →  run
```

C remains the bootstrap host. The program form of truth is **CBLC** (magic `CBLC`).

## Commands

```bash
# lower high-level CubalC → bytecode
./out/cubalc compile programs/proof/01_arithmetic.cubalc -o out/01.cblc

# disassemble
./out/cubalc disasm out/01.cblc

# execute via JIT (falls back to interp if needed)
./out/cubalc jit out/01.cblc
./out/cubalc jit programs/flow/cube_flow.cubalc

# self-manifest: compile + save + JIT in one breath
./out/cubalc manifest programs/flow/cube_flow.cubalc out/self.cblc

# .cblc also works with run
./out/cubalc run out/01.cblc
```

## ISA surface (assembly)

```
LOADI r0 10
ADD r0 r1
CMP r0 eq r2
JZ r0 +5
FLOW_P 8
DECIDE brain
HALT
```

Or high-level:

```cubalc
LET sum = 0
LOOP 10
  LET sum = sum + IT + 1
END
ASSERT sum == 55
```

## Hybrid JIT

| Op class | Execution |
|----------|-----------|
| LOADI ADD SUB MUL DIV MOD MOV AND OR XOR NEG CMP JMP JZ JNZ | **native x86_64** |
| GENESIS SPAWN PLUG FLOW IMPULSE SETBIT PRINT ASSERT DECIDE … | **C trampoline** (matrix machine) |

Energy still flows (`FLOW_P` → async parallel path). HOLD_FLASH sticky.

## Law

- Bytecode is machine SoT for hot path  
- Prose never on the wire  
- Manifest is reproducible: same source → same `.cblc` → same JIT semantics  

All Hail the Cube · All Hail NexusCore · tok=C3
