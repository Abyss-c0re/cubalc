# Changelog

## 1.12.55-universal — 2026-08-02

### Direction
- Algocube digit **7** → COP matrix reorder + complete boolean algebra + Hamming distance.

### Language
- `REVBITS`/`REVERSEBITS`/`BITREVM`/`MIRRORBITS` cube — reverse bit order in-place
- `SWAPBIT`/`XCHGBIT`/`EXCHBIT`/`SWBIT` cube i j — exchange two bit positions
- `XNORBITS`/`EQVBITS`/`NXORBITS` · `NORBITS` · `ANDNBITS`/`BICBITS` · `ORNBITS` dst src
- `DIFFBITS`/`HAMBITS`/`HAMMINGBITS`/`BITDIFF`/`XORDIST` a b → `LAST_N` = Hamming distance
- Proof `78_revbits_diff.cubalc`

### Prior
See 1.12.54-universal.


## 1.12.14-universal — 2026-08-02

### Direction
- Algocube digit **4** → stack select / within / clamp + zero-tests (control predicates pure-C).

### Language
- Zero-tests (unary TOS): `SZ`/`S0EQ` · `SNZ`/`S0NE` · `S0LT` · `S0GT` · `SSIGN`/`SGN` (−1/0/1)
- `SSEL`/`STACKSEL` — f t cond → (cond ? t : f)
- `SWITHIN`/`WITHIN` — n lo hi → 1 if lo ≤ n < hi
- `SCLAMP`/`STACKCLAMP` — n lo hi → clamp into [lo,hi]
- Proof `37_stack_select_clamp.cubalc`

### Prior
See 1.12.13-universal.

## 1.12.13-universal — 2026-08-02

### Direction
- Algocube digit **1** → stack compare + min/max (control predicates on the stack).

### Language
- `SEQ`/`SNE`/`SLT`/`SGT`/`SLE`/`SGE` (also `STACK*` / `CMP*`) — pop a b, push 0/1
- `SMIN`/`SMAX` (`STACKMIN`/`STACKMAX`) — ordered select of top two
- Proof `36_stack_compare.cubalc`

### Prior
See 1.12.12-universal.

## 1.12.12-universal — 2026-08-02

### Direction
- Algocube digit **1** → broaden computational expressiveness (stack bitwise ALU pure-C).

### Language
- Stack bitwise: `SAND`/`STACKAND`/`BANDST` · `SOR`/`STACKOR`/`BORST` · `SXOR`/`STACKXOR`/`BXORST`
- Unary: `SNOT`/`STACKNOT`/`BNOTST`/`SINVERT`
- Shifts: `SSHL`/`STACKSHL`/`SLSHL` · `SSHR`/`STACKSHR`/`SLSHR` (shift amount clamped 0..63)
- Proof `35_stack_bitops.cubalc`

### Prior
See 1.12.11-universal.

## 1.12.11-universal — 2026-08-02

### Direction
- Algocube digit **7** → stack ALU: Forth-style binary/unary arithmetic on the stack.

### Language
- `ADD`/`SUB`/`MUL`/`DIV`/`MOD` — pop a b, push result (OK/LAST_N/SP)
- `SNEG`/`STACKNEG`/`NEGATE` — negate TOS
- `SABS`/`STACKABS` — absolute value of TOS
- Proof `34_stack_alu.cubalc`

### Prior
See 1.12.10-universal.

## 1.12.10-universal — 2026-08-02

### Direction
- Algocube digit **2** → math plane ext: square, floor-div, binomial/permutation.

### Language
- `SQR`/`SQUARE`(n) — n²
- `DIVFLOOR`/`FLOORDIV`(a,b) — floor division (toward −∞)
- `BINOM`/`CHOOSE`(n,k) — binomial coefficient C(n,k)
- `PERM`/`PNR`(n,k) — P(n,k) = n!/(n−k)!
- Proof `33_math_binom_perm.cubalc`

### Prior
See 1.12.9-universal.

## 1.12.9-universal — 2026-08-02

### Direction
- Algocube digit **0** → foundation bitfield + ceil-div plane.

### Language
- `BEXT`/`BITEXT`(val, pos, width) — extract bit field
- `BDEP`/`BITDEP`(base, field, pos) — deposit low 8 bits of field at pos
- `BYTE`(val, i) · `LOBYTE`/`HIBYTE` — little-endian byte access
- `DIVCEIL`/`CEILDIV`(a, b) — integer ceiling division
- Proof `32_bitfield_divceil.cubalc`

### Prior
See 1.12.8-universal.

## 1.12.8-universal — 2026-08-02

### Direction
- Algocube digit **9** → cell logic plane: bitwise range masks + equality predicate.

### Language
- `ANDCELL`/`CELLAND` lo hi mask — bitwise AND each cell in range with mask
- `ORCELL`/`CELLOR` lo hi mask — bitwise OR each cell in range with mask
- `XORCELL`/`CELLXOR` lo hi mask — bitwise XOR each cell in range with mask
- `NOTCELL`/`CELLNOT` lo hi — bitwise NOT (~) each cell in range
- `EQCELL`/`CELLEQ` lo hi val — set cell to 1 if == val else 0; LAST_N = hit count
- Proof `31_cell_logic.cubalc`

### Prior
See 1.12.7-universal.

## 1.12.7-universal — 2026-08-02

### Direction
- Algocube digit **8** → stack↔cell bridge: block transfer between stack and cells.

### Language
- `TOCELL`/`>CELL` dst [n] — pop n stack values into cells[dst..] (TOS → highest)
- `FROMCELL`/`CELL>`/`PUSHCELL` src [n] — push cells[src..] onto stack
- Proof `30_stack_cell_bridge.cubalc`

### Prior
See 1.12.6-universal.


## 1.12.6-universal — 2026-08-02

### Direction
- Algocube digit **9** → cell fold ext: argmin/argmax + range rotate/shift.

### Language
- `MINIDX`/`ARGMIN` [lo [hi]] — first index of minimum in range (stmt + expr)
- `MAXIDX`/`ARGMAX` [lo [hi]] — first index of maximum in range (stmt + expr)
- `ROTCELL`/`CELLROT` lo hi k — rotate range left by k (k<0 right)
- `SHIFTCELL`/`CELLSHIFT` lo hi k — shift with zero-fill (k>0 left, k<0 right)
- Proof `29_cell_argmin_rot.cubalc`

### Prior
See 1.12.5-universal.

## 1.12.5-universal — 2026-08-02

### Direction
- Algocube digit **9** → cell fold plane: range arithmetic + iota + sort.

### Language
- `ADDCELL`/`CELLADD` lo hi delta — add to each cell in range
- `MULCELL`/`CELLMUL` lo hi k — scale each cell in range
- `IOTA`/`SEQCELL` lo hi [start [step]] — arithmetic sequence fill
- `SORTCELL`/`CELLSORT` lo hi [ASC|DESC|dir] — insertion sort (DESC or 0 = descending)
- Proof `28_cell_arith_sort.cubalc`

### Prior
See 1.12.4-universal.

## 1.12.4-universal — 2026-08-02

### Direction
- Algocube digit **8** → stack depth plane: double-width combinators + ROLL/DEPTH.

### Language
- `NIP` — drop under top (a b → b)
- `TUCK` — b under a, keep b (a b → b a b)
- `2DUP`/`DDUP` · `2DROP`/`DDROP` · `2SWAP`/`DSWAP`
- `ROLL` n — rotate top (n+1) items (n=1≡SWAP, n=2≡ROT)
- `DEPTH` — push current stack depth
- Proof `27_stack_depth_universal.cubalc`

### Prior
See 1.12.3-universal.

## 1.12.3-universal — 2026-08-02

### Direction
- Algocube digit **2** → math plane: integer log2 / bit ranks / digit folds / modular inverse.

### Language
- `ILOG2`/`LOG2`(n) — floor log2; -1 if n≤0
- `CTZ`(n) / `CLZ`(n) — trailing / leading zeros (64-bit word; 0→64)
- `ISPOW2`(n) / `POW2`(k) — power-of-two test / 2^k (k 0..62)
- `NDIGITS`(n) / `DIGSUM`(n) — decimal digit count / digit sum
- `MODINV`/`INVMOD`(a,m) — modular inverse via extended Euclid (0 if none)
- Proof `26_math_ilog_modinv.cubalc`

### Prior
See 1.12.2-universal.

## 1.12.2-universal — 2026-08-02

### Direction
- Algocube digit **5** → cell memory plane: block copy/move + find/count + reverse.

### Language
- `COPYCELL`/`CELLCOPY`/`CMOVE` src dst n — overlap-safe cell block copy
- `MOVECELL`/`CELLMOVE` src dst n — copy then clear non-overlapping source
- `FINDCELL`/`CELLFIND` val [lo [hi]] — first index or -1 (`OK` = found); expr form too
- `COUNTCELL`/`CELLCOUNT` val [lo [hi]] — count matches; expr form too
- `REVCELL`/`CELLREV` lo hi — reverse cell range in place
- Proof `25_cell_mem_universal.cubalc`

### Prior
See 1.12.1-universal.

## 1.12.1-universal — 2026-08-02

### Direction
- Algocube digit **4** → data-plane stack combinators + cell fill.

### Language
- `DUP`/`DROP`/`SWAP`/`OVER`/`ROT` — Forth-style stack ops (OK/LAST_N/SP)
- `PICK` n — copy n-th under top (0=TOS) onto stack
- `FILLCELL`/`CELLFILL` lo hi val — fill cell range
- Proof `24_stack_ops_universal.cubalc`

### Prior
See 1.12.0-universal.


## 1.12.0-universal — 2026-08-02

### Direction
- Algocube digit **6** → energy / matrix flow + seeded RNG.

### Language
- `RAND`/`RND` [max] · `RAND(n)` (seed: `CUBALC_SEED` env)
- `ENERGYSET`/`ENERGYADD` cube n (0..100 scale)
- `ROTBITS`/`SHIFTBITS` cube k (State Matrix bit rotate; negative = right)
- Proof `23_rand_energy_rotbits.cubalc`

### Prior
See 1.11.9-universal.

## 1.11.9-universal — 2026-08-02

### Direction
- Algocube digit **9** → cell fold + INC/DEC (loop-friendly data).

### Language
- `INC`/`DEC` name [step] · `INC CELL`/`DEC CELL` i [step]
- `SUMCELL`/`MINCELL`/`MAXCELL` [lo [hi]] (stmt + expr forms)
- Proof `22_cell_fold_inc.cubalc`

### Prior
See 1.11.8-universal.

## 1.11.8-universal — 2026-08-02

### Direction
- Algocube digit **4** → control flow: FN return + CASE.

### Language
- `RET`/`RETURN` [expr] early exit from `FN` body (`RETVAL`/`LAST_N`)
- `CASE`/`SWITCH` … `WHEN` … `DEFAULT` … `END`
- Proof `21_fn_return_case.cubalc`

### Prior
See 1.11.7-universal.

## 1.11.7-universal — 2026-08-02

### Direction
- Algocube digit **3** → string plane (control + data interchange).

### Language
- `SYS CAT`/`STRCAT` · `SYS FIND`/`INDEX` · `SYS EQS`/`STREQ`
- `SYS HAS`/`CONTAINS` · `SYS REVS`/`STRREV` · `SYS UPPER` · `SYS LOWER`
- Proof `20_string_ops_universal.cubalc`
- Tree restore from HEAD if mass-delete corruption

### Prior
See 1.11.6-universal.

## 1.11.6-universal — 2026-08-02

### Direction
- Algocube digit **2** → math plane (modular + number theory).

### Language
- `ADDMOD` `SUBMOD` `MULMOD` `POWMOD` · `FIB`/`FIBONACCI` · `ISPRIME`/`PRIMEP`
- `IDIV` `IMOD` named integer div/mod
- Proof `19_math_modular_universal.cubalc`
