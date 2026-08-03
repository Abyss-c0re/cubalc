# Changelog

## 1.12.143-universal — 2026-08-03

### Direction
- Algocube digit **8** → dual-stack 32-bit word pack (data-path).

### Language
- `DLO32`/`2LO32` · `DHI32`/`2HI32` — low/high 32-bit word of each of pair
- `DPACK32`/`2PACK32` — pack hi,lo words: `a b c d → (a<<32|c) (b<<32|d)` (masked)
- Proof `166_dlo32_dpack32.cubalc`

### Prior
See 1.12.142-universal.

## 1.12.142-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack bit isolate/mask path.

### Language
- `DBLS`/`2BLS`/`DBLSI` — isolate lowest set bit of each of pair (`x & -x`)
- `DBLC`/`2BLC`/`DBLSR` — clear lowest set bit (`x & (x-1)`)
- `DMASK`/`2MASK` — low-n-bit mask from pair widths (`(1<<n)-1`, n clamped 0..64)
- Proof `165_dbls_dmask.cubalc`

### Prior
See 1.12.141-universal.

## 1.12.141-universal — 2026-08-03

### Direction
- Algocube digit **4** → dual-stack control-word halfpack (16-bit).

### Language
- `DLO16`/`2LO16` · `DHI16`/`2HI16` — low/high halfword of each of pair
- `DPACK16`/`2PACK16` — pack hi,lo halfwords: `a b c d → (a<<16|c) (b<<16|d)` (masked)
- Proof `164_dlo16_dpack16.cubalc`

### Prior
See 1.12.140-universal.

## 1.12.140-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack sat-mul + range RNG (energy/flow).

### Language
- `DSATMUL`/`2SATMUL` — pairwise saturating multiply
- `DRANDRANGE`/`2RANDRANGE`/`DRANDIN` — pair uniform inclusive ranges `a b c d → U[a,c] U[b,d]`
- Proof `163_dsatmul_drandrange.cubalc`

### Prior
See 1.12.139-universal.

## 1.12.139-universal — 2026-08-03

### Direction
- Algocube digit **4** → dual-stack control-word byte pack/extract.

### Language
- `DLO8`/`2LO8` · `DHI8`/`2HI8` — low/high byte of each of pair
- `DPACK8`/`2PACK8` — pack hi,lo bytes: `a b c d → (a<<8|c) (b<<8|d)` (masked)
- Proof `162_dlo8_dpack8.cubalc`

### Prior
See 1.12.138-universal.

## 1.12.138-universal — 2026-08-03

### Direction
- Algocube digit **9** → dual-stack data-path clip + sign-extend.

### Language
- `DCLIP8`/`2CLIP8` · `DCLIP16`/`2CLIP16` — clamp pair to u8/u16 ranges
- `DSEXT8`/`2SEXT8` · `DSEXT16`/`2SEXT16` — sign-extend low 8/16 bits on pair
- Proof `161_dclip_dsext.cubalc`

### Prior
See 1.12.137-universal.

## 1.12.137-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack RNG + saturating energy ALU.

### Language
- `DRAND`/`2RAND`/`PAIRRAND` — pair uniform rand in `[0,max)`; max≤0 → 10
- `DSATADD`/`2SATADD` · `DSATSUB`/`2SATSUB` — pairwise saturating ±
- Proof `160_drand_dsatadd.cubalc`

### Prior
See 1.12.136-universal.

## 1.12.136-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack unary predicates (parity + sign-vs-zero).

### Language
- `DODD`/`2ODD` · `DEVEN`/`2EVEN` — parity predicates on pair
- `DLTZ`/`2LTZ` · `DGTZ`/`2GTZ` · `DLEZ`/`2LEZ` · `DGEZ`/`2GEZ` — compare to zero
- Proof `159_dodd_dltz.cubalc`

### Prior
See 1.12.135-universal.

## 1.12.135-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack number theory unary metrics.

### Language
- `DLOG2`/`2LOG2`/`DILOG2` — floor(log2); ≤0 → -1 on pair
- `DPHI`/`2PHI`/`DTOTIENT` — Euler totient φ(n); ≤0 → 0 on pair
- `DISPRIME`/`2ISPRIME`/`DPRIMEP` — prime predicate 0/1 on pair
- Proof `158_dlog2_dphi.cubalc`

### Prior
See 1.12.134-universal.

## 1.12.134-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack ALU unary scale + endian.

### Language
- `DDBL`/`2DBL`/`PAIRDBL` — pair double (`*2`)
- `DHALF`/`2HALF`/`PAIRHALF` — pair half toward zero (`/2`)
- `DBSWAP`/`2BSWAP`/`PAIRBSWAP` — pair 32-bit byte-swap
- Proof `157_ddbl_dbswap.cubalc`

### Prior
See 1.12.133-universal.

## 1.12.133-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack energy/flow distance metrics.

### Language
- `DAVG`/`2AVG`/`DMEAN` — pairwise truncated mean `(a+c)/2`, `(b+d)/2`
- `DDIST`/`2DIST`/`DABSDIFF` — pairwise absolute difference `|a-c|`, `|b-d|`
- `DHAMM`/`2HAMM`/`DHAMMING` — pairwise Hamming distance (popcount of XOR)
- Proof `156_davg_dhamm.cubalc`

### Prior
See 1.12.132-universal.

## 1.12.132-universal — 2026-08-03

### Direction
- Algocube digit **0** → dual-stack ones-metrics + power-of-two predicates.

### Language
- `DCLO`/`2CLO`/`PAIRCLO` — count leading ones (64-bit) on pair
- `DCTO`/`2CTO`/`PAIRCTO` — count trailing ones on pair
- `DISPOW2`/`2ISPOW2`/`DPOW2P`/`2POW2P` — 1 if value is power of two (>0, single bit)
- Lexer: digit-prefix compound tails accept alnum (enables `2ISPOW2` / `2POW2P`)
- Proof `155_dclo_dispow2.cubalc`

### Prior
See 1.12.131-universal.

## 1.12.131-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack bit-position metrics (ALU bit-path).

### Language
- `DFFS`/`2FFS`/`PAIRFFS` — find first set (1-based lowest 1-bit; 0 if zero) on pair
- `DFLS`/`2FLS`/`PAIRFLS`/`DMSB` — find last set (1-based highest 1-bit; 0 if zero)
- `DBWIDTH`/`2BWIDTH`/`PAIRBWIDTH` — minimal bit width of unsigned word (0 if zero)
- Proof `154_dffs_dbwidth.cubalc`

### Prior
See 1.12.130-universal.

## 1.12.130-universal — 2026-08-03

### Direction
- Algocube digit **3** → dual-stack bit-path: DORN DBREV DPARITY.

### Language
- `DORN`/`2ORN` — a b c d → (a|~c) (b|~d) (complements DANDN)
- `DBREV`/`2BREV`/`DBITREV` — reverse low 32 bits of each of top pair
- `DPARITY`/`2PARITY` — pairwise popcount mod 2 (0/1)
- Lexer 2-prefix: ORN BREV BITREV PARITY PAR
- Completes dual-stack bit metrics + ANDN/ORN plane
- Proof `153_dorn_dparity.cubalc`

### Prior
See 1.12.129-universal.

## 1.12.129-universal — 2026-08-03

### Direction
- Algocube digit **0** → dual-stack bit metrics: DPOPCNT DCLZ DCTZ.

### Language
- `DPOPCNT`/`2POPCNT` — a b → popcount(a) popcount(b)
- `DCLZ`/`2CLZ` — pair count leading zeros (0 → 64)
- `DCTZ`/`2CTZ` — pair count trailing zeros (0 → 64)
- Lexer 2-prefix: POPCNT PCNT CLZ CTZ
- Complements dual-stack shift/bitwise with vector bit-metrics
- Proof `152_dpopcnt_dclz.cubalc`

### Prior
See 1.12.128-universal.

## 1.12.128-universal — 2026-08-03

### Direction
- Algocube digit **5** → dual-stack extended bitwise: DNAND DNOR DXNOR DANDN.

### Language
- `DNAND`/`2NAND` — a b c d → ~(a&c) ~(b&d)
- `DNOR`/`2NOR` — a b c d → ~(a|c) ~(b|d)
- `DXNOR`/`2XNOR`/`DEQV` — a b c d → ~(a^c) ~(b^d)
- `DANDN`/`2ANDN`/`DBIC` — a b c d → (a&~c) (b&~d)
- Lexer 2-prefix: NAND NOR XNOR ANDN
- Completes dual-stack bitwise plane after DAND/DOR/DXOR/DNOT
- Proof `151_dnand_dandn.cubalc`

### Prior
See 1.12.127-universal.

## 1.12.127-universal — 2026-08-03

### Direction
- Algocube digit **4** → dual-stack rotate + range predicates: DROL DROR DWITHIN DBETWEEN.

### Language
- `DROL`/`2ROL` · `DROR`/`2ROR` — pairwise rotate left/right (amounts mod 64)
- `DWITHIN`/`2WITHIN` — a b lo hi → Forth half-open range tests (0/1)
- `DBETWEEN`/`2BETWEEN` — inclusive [lo,hi] (swap if inverted)
- Lexer 2-prefix: ROL ROR WITHIN BETWEEN
- Completes dual-stack shift plane with rotates + control range checks
- Proof `150_drol_dwithin.cubalc`

### Prior
See 1.12.126-universal.

## 1.12.126-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack unary control: DINC DDEC DNOT DEQZ DNEZ.

### Language
- `DINC`/`2INC` · `DDEC`/`2DEC` — pair increment / decrement
- `DNOT`/`2NOT` — pairwise bitwise complement
- `DEQZ`/`2EQZ` · `DNEZ`/`2NEZ` — pair zero / nonzero predicates (0/1)
- Lexer 2-prefix: INC DEC NOT EQZ NEZ
- Complements dual-stack compare plane with loop/counter + boolean duals
- Proof `149_dinc_dnot.cubalc`

### Prior
See 1.12.125-universal.

## 1.12.125-universal — 2026-08-03

### Direction
- Algocube digit **6** → dual-stack bound/select: DSIGN DCLAMP DSEL/DMUX.

### Language
- `DSIGN`/`2SIGN` — a b → sgn(a) sgn(b) as −1/0/1
- `DCLAMP`/`2CLAMP` — a b lo hi → clamp both into [lo,hi] (shared bounds)
- `DSEL`/`DMUX`/`2SEL`/`2MUX` — fa fb ta tb c → (c?ta:fa) (c?tb:fb)
- Lexer 2-prefix: SIGN CLAMP SEL MUX
- Energy-style shared bounds + vector mux plane
- Proof `148_dclamp_dsel.cubalc`

### Prior
See 1.12.124-universal.

## 1.12.124-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack numthy ext: DSQR DISQRT DCOPRIME.

### Language
- `DSQR`/`2SQR` — a b → a² b²
- `DISQRT`/`2ISQRT` — a b → isqrt(a) isqrt(b) (neg → 0)
- `DCOPRIME`/`2COPRIME` — a b c d → (gcd(a,c)==1) (gcd(b,d)==1)
- Lexer 2-prefix: SQR ISQRT SQRT COPRIME
- Complements dual-stack gcd/lcm/pow plane with square/root/coprime
- Proof `147_dsqr_dcoprime.cubalc`

### Prior
See 1.12.123-universal.

## 1.12.123-universal — 2026-08-03

### Direction
- Algocube digit **8** → dual-stack pair shifts: DSHL DSHR DSAR.

### Language
- `DSHL`/`2SHL` — a b c d → (a≪c) (b≪d) amounts clamped 0..63
- `DSHR`/`2SHR` — pairwise logical right shift
- `DSAR`/`2SAR` — pairwise arithmetic right shift
- Lexer 2-prefix: SHL SHR SAR
- Complements dual-stack ALU/bitwise with vector data-path shifts
- Proof `146_dshl_dsar.cubalc`

### Prior
See 1.12.122-universal.

## 1.12.122-universal — 2026-08-03

### Direction
- Algocube digit **2** → dual-stack pair number theory: DGCD DLCM DPOW.

### Language
- `DGCD`/`2GCD` — a b c d → gcd(a,c) gcd(b,d)
- `DLCM`/`2LCM` — a b c d → lcm(a,c) lcm(b,d) (0-safe)
- `DPOW`/`2POW` — a b c d → a^c b^d (neg exp → 0)
- Lexer 2-prefix: GCD LCM POW
- Complements dual-stack ALU with pairwise numthy plane
- Proof `145_dgcd_dlcm.cubalc`

### Prior
See 1.12.121-universal.

## 1.12.121-universal — 2026-08-03

### Direction
- Algocube digit **1** → dual-stack pair compare predicates: DEQ DNE DLT DLE DGT DGE.

### Language
- `DEQ`/`2EQ` · `DNE`/`2NE` — pairwise equality / inequality (0/1)
- `DLT`/`2LT` · `DLE`/`2LE` · `DGT`/`2GT` · `DGE`/`2GE` — pairwise ordered compares
- Lexer 2-prefix whitelist: EQ NE LT LE GT GE
- Complements dual-stack ALU/bitwise with vector relational plane
- Proof `144_deq_dlt.cubalc`

### Prior
See 1.12.120-universal.

## 1.12.120-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack pair bitwise + unary: DAND DOR DXOR DNEG DABS.

### Language
- `DAND`/`2AND` · `DOR`/`2OR` · `DXOR`/`2XOR` — pairwise bitwise
- `DNEG`/`2NEG` · `DABS`/`2ABS` — unary pair negate/abs
- Completes dual-stack arith + logic plane
- Proof `143_dand_dneg.cubalc`

### Prior
See 1.12.119-universal.

## 1.12.119-universal — 2026-08-03

### Direction
- Algocube digit **2** → prime metrics: π(n), gap to next prime, composite test.

### Language
- `PRIMECOUNT`/`PRIMEPI`(n) — primes ≤ n (cap 200000)
- `PRIMEGAP`/`PGAP`(n) — nextprime(n) − n
- `ISCOMPOSITE`/`COMPOSITEP`(n) — composite predicate
- Stack: `SPRIMECOUNT` · `SPRIMEGAP` · `SISCOMPOSITE`
- Proof `142_primecount_gap.cubalc`

### Prior
See 1.12.118-universal.

## 1.12.118-universal — 2026-08-03

### Direction
- Algocube digit **0** → complete depth-5 stack foundation plane.

### Language
- `5NIP`/`NIP5` — a b c d e → a e
- `5ROT`/`ROT5` · `5RROT`/`RROT5` — rotate top 5
- `5OVER`/`OVER5` — copy under quintet (needs 10)
- `5TUCK`/`TUCK5` — e a b c d e
- Lexer whitelist for 5NIP/5ROT/5RROT/5OVER/5TUCK
- Proof `141_5nip_5rot.cubalc`

### Prior
See 1.12.117-universal.

## 1.12.117-universal — 2026-08-03

### Direction
- Algocube digit **5** → cell search: last-find + first/last nonzero + stack duals.

### Language
- `FINDLASTCELL`/`RFINDCELL` val [lo [hi]] — last index of val (−1 if none)
- `FIRSTNZ`/`LASTNZ` [lo [hi]] — first/last nonzero index
- Stack: `SFINDCELL` · `SFINDLAST` · `SFIRSTNZ` · `SLASTNZ`
- Proof `140_findlast_nz.cubalc`

### Prior
See 1.12.116-universal.

## 1.12.116-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack pair ALU: DDIV DMOD DMIN DMAX.

### Language
- `DDIV`/`2DIV` — a b c d → (a/c) (b/d) (0-safe)
- `DMOD`/`2MOD`/`DREM` — a b c d → (a%c) (b%d)
- `DMIN`/`2MIN` · `DMAX`/`2MAX` — pairwise min/max
- Completes DADD/DSUB/DMUL pair plane
- Proof `139_ddiv_dmin.cubalc`

### Prior
See 1.12.115-universal.

## 1.12.115-universal — 2026-08-03

### Direction
- Algocube digit **5** → cell range predicates: any/all/none, nz-count, equal-range.

### Language
- `ANYCELL`/`ALLCELL`/`NONECELL` lo hi — nonzero range predicates
- `NZCOUNT`/`COUNTNZCELL` lo hi — count nonzero cells
- `EQRANGE` a_lo b_lo n — pairwise range equality
- Stack: `SANYCELL`/`SALLCELL`/`SNONECELL`/`SNZCOUNT`/`SEQRANGES`
- Proof `138_anycell_eqrange.cubalc`

### Prior
See 1.12.114-universal.

## 1.12.114-universal — 2026-08-03

### Direction
- Algocube digit **1** → stack conditionals: dup-if-zero, swap/keep/nip under flag.

### Language
- `DUPZ`/`QDUP0` — duplicate TOS only if zero (complement of `QDUP`)
- `SSWAPIF`/`SWAPIF`/`QSWAP` — a b f → swap a,b if f
- `SKEEPIF`/`KEEPIF` — v f → keep v if f else drop both
- `SNIPIF`/`NIPIF`/`NIPWHEN` — a b f → f?b:a
- Proof `137_dupz_swapif.cubalc`

### Prior
See 1.12.113-universal.

## 1.12.113-universal — 2026-08-03

### Direction
- Algocube digit **2** → abundance class: aliquot / perfect / abundant / deficient.

### Language
- `ALIQUOT`/`PROPERSIGMA`(n) — proper divisor sum σ(n)−n
- `ISPERFECT`/`PERFECTP`(n) · `ISABUNDANT`/`ABUNDANTP`(n) · `ISDEFICIENT`/`DEFICIENTP`(n)
- Stack: `SALIQUOT` · `SISPERFECT` · `SISABUNDANT` · `SISDEFICIENT`
- Proof `136_aliquot_perfect.cubalc`

### Prior
See 1.12.112-universal.

## 1.12.112-universal — 2026-08-03

### Direction
- Algocube digit **6** → energy fleet metrics + pair equalize/swap.

### Language
- `ENERGYSWAP` a b — exchange energy planes
- `ENERGYSHARE` a b — equalize mean energy
- `ENERGYTOTAL`/`ENERGYAVG`/`ENERGYMIN`/`ENERGYMAX` — fleet metrics (0..100)
- `SENERGYTOTAL`/`SENERGYAVG`/`SENERGYMIN`/`SENERGYMAX` — stack duals
- Proof `135_energy_fleet.cubalc`

### Prior
See 1.12.111-universal.

## 1.12.111-universal — 2026-08-03

### Direction
- Algocube digit **5** → cell compare/mux: relational predicates + cell-plane mux.

### Language
- `LTCELL`/`GTCELL`/`LECELL`/`GECELL` lo hi val — 0/1 relational masks
- `SLTCELL`/`SGTCELL`/`SLECELL`/`SGECELL` — stack duals (lo hi val)
- `MUXCELL` dst_lo a_lo b_lo mask_lo n — mask?a:b cell blend
- `SMUXCELL` — stack dlo alo blo mlo n
- Proof `134_ltcell_mux.cubalc`

### Prior
See 1.12.110-universal.

## 1.12.110-universal — 2026-08-03

### Direction
- Algocube digit **2** → primes/powers: perfect power, prime power, nth prime.

### Language
- `ISPOWER`/`PERFPOW`(n) · `SISPOWER` — perfect power b^e (e≥2)
- `ISPRIMEPOWER`/`IPP`(n) · `SIPP` — form p^k
- `NTHPRIME`/`PRIMEN`(k) · `SNTHPRIME` — k-th prime (1→2)
- Proof `133_ispower_nthprime.cubalc`

### Prior
See 1.12.109-universal.

## 1.12.109-universal — 2026-08-03

### Direction
- Algocube digit **2** → modular ext: square-part, primitive root, CRT stack.

### Language
- `SQPART`/`LARGESQ`(n) · `SSQPART` — largest square dividing n
- `ISPRIMITIVE`/`ISPRROOT`(a,m) · `SIPRIMITIVE` — primitive root test
- `SCRT`/`SCHINREM` — stack Chinese remainder (a m b n → x)
- Proof `132_sqpart_scrt.cubalc`

### Prior
See 1.12.108-universal.

## 1.12.108-universal — 2026-08-03

### Direction
- Algocube digit **2** → modular order plane: SOPF + Carmichael λ + mult. order.

### Language
- `SOPF`/`SOPFR`(n) · `SSOPF` — sum of prime factors (distinct / with mult.)
- `CARMICHAEL`/`LAMBDA`(n) · `SCARMICHAEL`/`SLAMBDA` — Carmichael λ(n)
- `ORDER`/`MULTORDER`(a,m) · `SORDER` — multiplicative order
- Proof `131_sorder_carmichael.cubalc`

### Prior
See 1.12.107-universal.

## 1.12.107-universal — 2026-08-03

### Direction
- Algocube digit **9** → cell-logic stack duals + NECELL predicate.

### Language
- `NECELL`/`CELLNE` — range ≠ val → 0/1 mask
- `SANDCELL`/`SORCELL`/`SXORCELL` — stack lo hi mask bitwise
- `SNOTCELL` — stack lo hi bitwise invert
- `SCELLEQ`/`SNECELL` — stack lo hi val predicate masks
- Proof `130_sandcell_sne.cubalc`

### Prior
See 1.12.106-universal.

## 1.12.106-universal — 2026-08-03

### Direction
- Algocube digit **9** → cell-fold stack duals: scan/diff/shift/clamp + DIFFCELL.

### Language
- `DIFFCELL`/`CELLDIFF` — adjacent differences (inverse of SCANCELL)
- `SSCANCELL`/`SPREFIXSUM` — stack lo hi → prefix sum
- `SDIFFCELL`/`SCELLDIFF` — stack lo hi → adjacent diffs
- `SSHIFTCELL`/`SCELLSHIFT` — stack lo hi k → zero-fill shift
- `SCLAMPCELL`/`SCELLCLAMP` — stack lo hi mn mx → clamp range
- Proof `129_sscan_sdiff.cubalc`

### Prior
See 1.12.105-universal.

## 1.12.105-universal — 2026-08-03

### Direction
- Algocube digit **2** → factor metrics after SPF/Jacobi: valuation + Ω/ω.

### Language
- `VALUATION`/`PVAL`/`VP`(n,p) · `SVAL`/`SPVAL` — p-adic valuation v_p(n)
- `OMEGA`/`BIGOMEGA`(n) · `SOMEGA` — Ω(n) factors with multiplicity
- `OMEGA0`/`LITTLEOMEGA`(n) · `SOMEGA0` — ω(n) distinct primes
- Proof `128_sval_somega.cubalc`

### Prior
See 1.12.104-universal.

## 1.12.104-universal — 2026-08-03

### Direction
- Algocube digit **8** → stack depth duals: 4TUCK + quintuple 5DUP/5DROP/5SWAP.

### Language
- `4TUCK`/`QTUCK`/`TUCK4` — a b c d → d a b c d
- `5DUP`/`DUP5` — duplicate top 5
- `5DROP`/`DROP5` — drop top 5
- `5SWAP`/`SWAP5` — reverse top 5
- Proof `127_4tuck_5dup.cubalc`

### Prior
See 1.12.103-universal.

## 1.12.103-universal — 2026-08-03

### Direction
- Algocube digit **5** → cell-memory stack duals of COPY/MOVE/REV/ROT.

### Language
- `SCOPYCELL`/`SCELLCOPY` — stack src dst n → copy range
- `SMOVECELL`/`SCELLMOVE` — stack src dst n → move (clear source)
- `SREVCELL`/`SCELLREV` — stack lo hi → reverse range
- `SROTCELL`/`SCELLROT` — stack lo hi k → rotate range
- Proof `126_scopycell_srot.cubalc`

### Prior
See 1.12.102-universal.

## 1.12.102-universal — 2026-08-03

### Direction
- Algocube digit **2** → modular / number-theory duals after SPOWMOD/SMODINV.

### Language
- `JACOBI`/`LEGENDRE`(a,n) · `SJACOBI`/`SLEGENDRE` — Jacobi symbol −1/0/1
- `MODDIV`/`DIVMODM`(a,b,m) · `SMODDIV`/`SDIVMODM` — a·b⁻¹ mod m
- `SPF`/`SMALLPF`/`MINPF`(n) · `SSPF`/`SSMALLPF` — smallest prime factor
- Proof `125_smoddiv_jacobi.cubalc`

### Prior
See 1.12.101-universal.

## 1.12.101-universal — 2026-08-03

### Direction
- Algocube digit **0** → stack foundation duals: complete depth-4 plane after 4DUP/4DROP/4SWAP.

### Language
- `4NIP`/`QNIP`/`NIP4` — a b c d → a d (drop middle two)
- `4ROT`/`QROT`/`ROT4` — a b c d → b c d a
- `4RROT`/`QRROT`/`RROT4` — a b c d → d a b c
- `4OVER`/`QOVER`/`OVER4` — copy under quartet (needs 8)
- Proof `124_4nip_4rot.cubalc`

### Prior
See 1.12.100-universal.

## 1.12.100-universal — 2026-08-03

### Direction
- Algocube digit **8** → stack depth duals: 3NIP + quadruple 4DUP/4DROP/4SWAP (after 3ROT/3TUCK).

### Language
- `3NIP`/`TNIP`/`NIP3` — a b c → a c
- `4DUP`/`DUP4` — duplicate top 4
- `4DROP`/`DROP4` — drop top 4
- `4SWAP`/`SWAP4` — reverse top 4
- Proof `123_3nip_4dup.cubalc`

### Prior
See 1.12.99-universal.

## 1.12.99-universal — 2026-08-03

### Direction
- Algocube digit **4** → stack combinator duals with n from TOS (complete PICK/ROLL/NDROP imm plane).

### Language
- `SPICK`/`PICKS` — pop n, copy n-th under remaining top
- `SROLL`/`ROLLS` — pop n, rotate top (n+1) items
- `SNDROP`/`DROPS` — pop n, drop n remaining top items
- Proof `122_spick_sroll.cubalc`

### Prior
See 1.12.98-universal.

## 1.12.98-universal — 2026-08-03

### Direction
- Algocube digit **3** → string plane duals: LEFT/RIGHT slices + occurrence COUNT (after MID/FIND/LPAD).

### Language
- `SYS LEFT`/`STRLEFT`/`TAKELEFT` str n — first n chars → LAST
- `SYS RIGHT`/`STRRIGHT`/`TAKERIGHT` str n — last n chars → LAST
- `SYS COUNT`/`STRCOUNT`/`OCCURS` hay needle — non-overlapping count → LAST_N
- Proof `121_str_left_count.cubalc`

### Prior
See 1.12.97-universal.

## 1.12.97-universal — 2026-08-03

### Direction
- Algocube digit **5** → stack-imm bitfield extract/deposit/mask (dual of SBEXT/SBDEP + mask generator).

### Language
- `SBEXTN`/`EXTN`/`BEXTN` pos width — extract width bits at pos from TOS
- `SBDEPN`/`DEPN`/`BDEPN` field pos — deposit low 8 bits of field into TOS at pos
- `SMASKN`/`MASKN`/`ONESN` n — TOS = low-n-bit mask ((1≪n)−1; n=64 → all ones)
- Proof `120_sbextn_smaskn.cubalc`

### Prior
See 1.12.96-universal.

## 1.12.96-universal — 2026-08-03

### Direction
- Algocube digit **8** → triple stack-depth duals (complete after 3DUP/3DROP/3SWAP/3OVER).

### Language
- `3ROT`/`TROT`/`ROT3` — a b c → b c a
- `3RROT`/`TRROT`/`RROT3` — a b c → c a b
- `3TUCK`/`TTUCK`/`TUCK3` — a b c → c a b c
- Proof `119_3rot_3tuck.cubalc`

### Prior
See 1.12.95-universal.

## 1.12.95-universal — 2026-08-03

### Direction
- Algocube digit **9** → stack↔cell range dual + cell transfer (complete after SGETCELL/SSETCELL single forms).

### Language
- `SLOADCELLS`/`SLOADN`/`SPUSHRANGE` lo n — push cells[lo..lo+n-1]
- `SPOPCELLS`/`SSTORECELLS`/`SSTORERANGE` lo n — pop n into cells[lo..]
- `CELLXFER`/`XFERCELL` i j amt — move amt from cell i → j
- Proof `118_sloadcells_cellxfer.cubalc`

### Prior
See 1.12.94-universal.

## 1.12.94-universal — 2026-08-03

### Direction
- Algocube digit **6** → energy transfer/clamp + stack-imm range RNG (extend after ENERGYSUB/SRANDN).

### Language
- `ENERGYXFER`/`XFERENERGY`/`SENRX` src dst n — move n energy units src→dst (clamp 0..100)
- `ENERGYCLAMP`/`CLAMPENERGY`/`SFLWX` id lo hi — clamp energy to [lo,hi]
- `SRANDRANGEN`/`RANDRANGEN`/`SRNGX` lo hi — push uniform [lo,hi] (stack-imm dual of SRANDRANGE)
- Proof `117_energyxfer_srandrangen.cubalc`

### Prior
See 1.12.93-universal.

## 1.12.93-universal — 2026-08-03

### Direction
- Algocube digit **7** → dual-stack pair ALU (vector pair +−× after binary stack ALU + 2DUP family).

### Language
- `DADD`/`2ADD`/`PAIRADD` — a b c d → (a+c) (b+d)
- `DSUB`/`2SUB`/`PAIRSUB` — a b c d → (a-c) (b-d)
- `DMUL`/`2MUL`/`PAIRMUL` — a b c d → (a*c) (b*d)
- Proof `116_dadd_dmul.cubalc`

### Prior
See 1.12.92-universal.

## 1.12.92-universal — 2026-08-03

### Direction
- Algocube digit **6** → energy / flow / RNG dual forms (stack-imm RAND + energy drain + stack ENERGYFLOW).

### Language
- `SRANDN`/`RANDN`/`SRNGN` n — push rand in [0,n) (stack-imm dual of SRAND)
- `ENERGYSUB`/`SUBENERGY`/`DRAIN`/`SENRN` id n — drain energy plane (complete set/add/sub)
- `SEFLOW`/`SENERGYFLOW` — hops from TOS · `SEFLOWN`/`SFLWN` n — immediate hops
- Proof `115_srandn_energysub.cubalc`

### Prior
See 1.12.91-universal.

## 1.12.91-universal — 2026-08-03

### Direction
- Algocube digit **0** → stack immediate inverted bitwise (complete NAND/NOR/XNOR-with-constant after SANDI/SORI/SXORI).

### Language
- `SNANDI`/`NANDI`/`NANDIMM` n — TOS = ~(TOS & n)
- `SNORI`/`NORI`/`NORIMM` n — TOS = ~(TOS | n)
- `SXNORI`/`XNORI`/`XNORIMM` n — TOS = ~(TOS ^ n) (equiv)
- Proof `114_snandi_sxnori.cubalc`

### Prior
See 1.12.90-universal.

## 1.12.90-universal — 2026-08-03

### Direction
- Algocube digit **9** → stack immediate bitwise mask (complete AND/OR/XOR-with-constant after bitfield + shift).

### Language
- `SANDI`/`ANDIMM`/`ANDI` n — TOS &= n
- `SORI`/`ORIMM`/`ORI` n — TOS |= n
- `SXORI`/`XORIMM`/`XORI` n — TOS ^= n
- Proof `113_sandi_sxori.cubalc`

### Prior
See 1.12.89-universal.



## 1.12.89-universal — 2026-08-03

### Direction
- Algocube digit **3** → stack immediate rotate + arithmetic shift (complete shift/rotate-by-constant after SSHLN/SSHRN).

### Language
- `SROLN`/`ROLN`/`SROTLN` n — rotate left TOS by n mod 64
- `SRORN`/`RORN`/`SROTRN` n — rotate right TOS by n mod 64
- `SSARN`/`SARN`/`SASHRN`/`ASHRN` n — arithmetic TOS ≫= n (sign-preserving)
- Proof `112_sroln_ssarn.cubalc`

### Prior
See 1.12.88-universal.

## 1.12.88-universal — 2026-08-03

### Direction
- Algocube digit **1** → stack immediate compare + min/max-with-constant (complete SEQ/SLT/SMIN family after stack-stack).

### Language
- `SEQN`/`EQN`/`CMPEQN` n — TOS = (TOS == n) ? 1 : 0
- `SNEN`/`NEN`/`CMPNEN` n — TOS = (TOS != n) ? 1 : 0
- `SLTN`/`LTN` · `SGTN`/`GTN` n — ordered predicates vs constant
- `SLENN`/`SLEQN` · `SGENN`/`SGEQN` n — ≤ / ≥ vs constant
- `SMINN`/`MINN` · `SMAXN`/`MAXN` n — min/max(TOS, n)
- Proof `111_seqn_sminn.cubalc`

### Prior
See 1.12.87-universal.

## 1.12.87-universal — 2026-08-03

### Direction
- Algocube digit **5** → stack immediate bitfield + shift-by-constant (complete bit path after stack-stack SBTEST/SSETB).

### Language
- `SSETBN`/`SETBN`/`SSETBITN` n — TOS \|= (1≪n)
- `SCLRBN`/`CLRBN`/`SCLRBITN` n — TOS &= ~(1≪n)
- `SFLIPBN`/`FLIPBN`/`STGLBN` n — TOS ^= (1≪n)
- `SBTESTN`/`TESTBITN`/`SBITN` n — replace TOS with bit n (0/1)
- `SSHLN`/`SHLN` n — TOS ≪= n · `SSHRN`/`SHRN` n — logical TOS ≫= n
- Proof `110_ssetbn_stestn.cubalc`

### Prior
See 1.12.86-universal.

## 1.12.86-universal — 2026-08-02

### Direction
- Algocube digit **7** → stack immediate ALU: div/mod by constant (complete +−×÷% family).

### Language
- `SDIVN`/`DIVN`/`QUOTN` n — TOS /= n (n==0 → 0, soft)
- `SMODN`/`MODN`/`REMN` n — TOS %= n (n==0 → 0, soft)
- Proof `109_sdivn_smodn.cubalc`

### Prior
See 1.12.85-universal.

## 1.12.85-universal — 2026-08-02

### Direction
- Algocube digit **1** → data plane: stack↔cell accumulate (sub/mul/div).

### Language
- `SSUBTOC`/`SCELLSUB` — stack `i v → cells[i]-=v` leave result
- `SMULTOC`/`SCELLMUL` — stack `i v → cells[i]*=v` leave product
- `SDIVTOC`/`SCELLDIV` — stack `i v → cells[i]/=v` leave quotient
- Proof `108_ssubtoc_smultoc.cubalc`

### Prior
See 1.12.84-universal.

## 1.12.84-universal — 2026-08-02

### Direction
- Algocube digit **4** → stack immediate ALU: add/sub/mul by constant.

### Language
- `SADDN`/`PLUSN`/`ADDN` n — TOS += n
- `SSUBN`/`MINUSN`/`SUBN` n — TOS -= n
- `SMULN`/`TIMESN`/`MULN` n — TOS *= n
- Proof `107_saddn_smuln.cubalc`

### Prior
See 1.12.83-universal.

## 1.12.83-universal — 2026-08-02

### Direction
- Algocube digit **7** → stack ALU unary: inc/dec, double, half.

### Language
- `SINC`/`INCSTK`/`STACKINC` — TOS += 1
- `SDEC`/`DECSTK`/`STACKDEC` — TOS -= 1
- `SDBL`/`SDOUBLE`/`STACKDBL` — TOS *= 2
- `SHALF`/`SHALVE`/`STACKHALF` — TOS /= 2
- Proof `106_sinc_sdbl.cubalc`

### Prior
See 1.12.82-universal.

## 1.12.82-universal — 2026-08-02

### Direction
- Algocube digit **4** → stack combinators: fill top-N, conditional drop.

### Language
- `SFILL`/`FILLTOP` n v — write v into top n stack slots
- `DROPZ`/`SDROPZ`/`DROPIF0` — drop TOS if zero
- `DROPNZ`/`SDROPNZ`/`DROPIF` — drop TOS if nonzero
- Proof `105_sfill_dropz.cubalc`

### Prior
See 1.12.81-universal.

## 1.12.81-universal — 2026-08-02

### Direction
- Algocube digit **3** → stack structure: 3OVER + UNDER (complete triple plane after remote 1.12.80 2TUCK).

### Language
- `3OVER`/`TOVER`/`OVER3` — copy under-triple onto stack
- `UNDER`/`SUNDER`/`DUPUNDER` — a b → a a b
- Lexer: `3OVER` as ident
- Proof `104_3over_under.cubalc`

### Prior
See 1.12.80-universal.

## 1.12.80-universal — 2026-08-02

### Direction
- Algocube digit **8** → stack depth plane: triple ops + pair tuck.

### Language
- `3DUP`/`TDUP` — duplicate top 3
- `3DROP`/`TDROP` — drop top 3
- `2TUCK`/`DTUCK` — tuck top pair under second pair
- `3SWAP`/`TSWAP` — reverse top 3 (`a b c → c b a`)
- Lexer: `3DUP`/`3DROP`/`3SWAP`/`2TUCK` as idents
- Proof `103_3dup_2tuck.cubalc`

### Prior
See 1.12.79-universal.

## 1.12.79-universal — 2026-08-02

### Direction
- Algocube digit **1** → stack structure: keep-top-N (finish SINSERT/UNROLL plane).

### Language
- `SKEEP`/`KEEPN`/`KEEP` n — keep only top n items; drop under
- Proof `102_skeep.cubalc`

### Prior
See 1.12.78-universal.

## 1.12.78-universal — 2026-08-02

### Direction
- Algocube digit **9** → COP metrics: longest 1-run, zero-runs, first/last zero.

### Language
- `MAXRUN`/`LONGRUN` cube — longest contiguous ones run
- `ZRUNS`/`ZERORUNS` cube — count of maximal zero-runs
- `FINDZERO`/`FIRSTZERO` · `LASTZERO` cube — first/last zero index (−1 if none)
- Proof `101_maxrun_zruns.cubalc`

### Prior
See 1.12.77-universal.

## 1.12.77-universal — 2026-08-02

### Direction
- Algocube digit **2** → math plane: coprime test, ceil power-of-two, extended GCD.

### Language
- `COPRIME`/`ISCOPRIME`(a,b) · stack `SCOPRIME`
- `CEILPOW2`/`NEXTPOW2`(n) · stack `SCEILPOW2`/`SNEXTPOW2`
- `SEGCD`/`SXGCD` — stack `a b → g x y` (Bézout)
- Proof `100_coprime_ceilpow2.cubalc`

### Prior
See 1.12.76-universal.

## 1.12.76-universal — 2026-08-02

### Direction
- Algocube digit **1** → data plane: depth insert/unroll, bulk push, cell accumulate.

### Language
- `SINSERT`/`UNROLL`/`RROLL` n — move TOS to depth n (n=1≡SWAP)
- `RROT`/`NROT`/`-ROT` — fixed reverse ROT (a b c → c a b)
- `NPUSH`/`PUSHN` v n — push value v, n times
- `SADDTOC`/`SCELLADD` — stack `i v → cells[i]+=v` leave sum
- Proof `99_sinsert_npush.cubalc`

### Prior
See 1.12.75-universal.

## 1.12.75-universal — 2026-08-02

### Direction
- Algocube digit **1** → data plane: stack↔cell fetch + depth-indexed stack mutators.

### Language
- `SGETCELL`/`SLOAD`/`SFETCH` — stack `i → cells[i]`
- `SREPLACE`/`SPUT`/`SSTOREN` n — write TOS into depth-n slot, drop TOS
- `SDROPAT`/`NIPN`/`DROPAT` n — remove item at depth n (0≡DROP, 1≡NIP)
- Proof `98_sgetcell_sreplace.cubalc`

### Prior
See 1.12.74-universal.

## 1.12.74-universal — 2026-08-02

### Direction
- Algocube digit **0** → stack foundation: multi-drop, empty/full probes, depth swap.

### Language
- `NDROP`/`DROPN` n — drop top n items
- `SEMPTY`/`ISEMPTY` · `SFULL`/`ISFULL` → LAST_N predicate
- `SSWAPN`/`SWAPN` n — exchange TOS with n-th under top
- Proof `97_ndrop_sswapn.cubalc`

### Prior
See 1.12.73-universal.

## 1.12.73-universal — 2026-08-02

### Direction
- Algocube digit **9** → COP metrics: transitions, one-runs, masked popcount.

### Language
- `TRANSBITS`/`BITTRANS` cube → adjacent flip count
- `RUNSBITS`/`ONERUNS` cube → number of 1-runs
- `MASKPOP`/`POPMASK` cube mask → popcount under mask ones
- Proof `96_trans_runs_mask.cubalc`

### Prior
See 1.12.72-universal.

## 1.12.72-universal — 2026-08-02

### Direction
- Algocube digit **2** → number theory: Möbius, radical, square-free test.

### Language
- `MOBIUS`/`MU`(n) · `RADICAL`/`RAD`(n) · `ISSQUAREFREE`/`SQFREE`(n)
- Stack: `SMOBIUS`/`SMU` · `SRAD`/`SRADICAL` · `SISSQFREE`/`SSQFREE`
- Proof `95_mobius_radical.cubalc`

### Prior
See 1.12.71-universal.

## 1.12.71-universal — 2026-08-02

### Direction
- Algocube digit **9** → COP metrics: leading/trailing zeros, union pop, Jaccard.

### Language
- `CLZBITS`/`NLZBITS` cube · `CTZBITS`/`NTZBITS` cube
- `ORPOP`/`UNIONPOP` a b → popcount(a OR b)
- `JACCARD`/`SIMBITS` a b → 100·|A∩B|/|A∪B| integer percent
- Proof `94_clz_jaccard.cubalc`

### Prior
See 1.12.70-universal.

## 1.12.70-universal — 2026-08-02

### Direction
- Algocube digit **3** → COP range algebra: bitwise range ops + range reduce.

### Language
- `ANDRANGE`/`ORRANGE`/`XORRANGE` a b lo hi — in-place a[i] OP= b[i]
- `NANDRANGE`/`NORRANGE`/`XNORRANGE` · `ANDNRANGE`/`ORNRANGE`
- `ANDREDUCE`/`ALLRANGE` cube lo hi · `ORREDUCE`/`ANYRANGE` cube lo hi
- Proof `93_andrange_reduce.cubalc`

### Prior
See 1.12.69-universal.

## 1.12.69-universal — 2026-08-02

### Direction
- Algocube digit **4** → control flow: iterate set-bit indices + no-op.

### Language
- `FORBIT`/`EACHBIT` cube [AS name] ... END — loop over set bits (`IT`/`IDX`/`BIT`)
- `PASS`/`NOP`/`NOOP` — no-op statement
- Proof `92_forbit_pass.cubalc`

### Prior
See 1.12.68-universal.

## 1.12.68-universal — 2026-08-02

### Direction
- Algocube digit **9** → COP metrics: bit-dot, majority threshold, Gray code.

### Language
- `DOTBITS`/`ANDPOP` a b → popcount(a AND b)
- `MAJBITS`/`THRESHBITS` cube [k] → 1 if ones ≥ k (default strict majority)
- `GRAYBITS`/`TOGRAY` cube · `UNGRAYBITS`/`GRAY2BIN` cube
- Proof `91_dot_maj_gray.cubalc`

### Prior
See 1.12.67-universal.

## 1.12.67-universal — 2026-08-02

### Direction
- Algocube digit **8** → matrix data-path: PEXT/PDEP under mask + bit interleave.

### Language
- `PEXTBITS`/`GATHERBITS` cube mask → LAST_N (parallel extract)
- `PDEPBITS`/`SCATTERBITS` cube mask val — deposit low bits into mask positions
- `ZIPBITS` dst a b — interleave a/b into dst (Morton)
- `UNZIPBITS` even odd src — deinterleave even/odd lanes
- Proof `90_pext_zip_matrix.cubalc`

### Prior
See 1.12.66-universal.

## 1.12.66-universal — 2026-08-02

### Direction
- Algocube digit **4** → control flow: cell-range iterators + single-token break/continue-if.

### Language
- `EACH CELL` [as name] [FROM lo TO hi] ... END — iterate cells, bind value/`VAL`, `IT`/`IDX`
- `FORCELL`/`EACHCELL` [name] lo hi ... END — compact cell-range loop
- `BREAKIF` expr · `CONTINUEIF`/`CONTIF`/`SKIPIF`/`NEXTIF` expr
- Proof `89_eachcell_breakif.cubalc`

### Prior
See 1.12.65-universal.


## 1.12.65-universal — 2026-08-02

### Direction
- Algocube digit **1** → data expressiveness: local range rotate/shift on matrix bits.

### Language
- `ROTRANGE`/`ROLRANGE` cube lo hi k — rotate-left bits in [lo..hi]
- `RORRANGE` cube lo hi k — rotate-right in range
- `SHLRANGE`/`SHIFTRANGE` cube lo hi k — logical left shift (zero-fill) in range
- `SHRRANGE` cube lo hi k — logical right shift in range
- Outside [lo..hi] untouched; negative k flips direction
- Proof `88_rotrange_shift.cubalc`

### Prior
See 1.12.64-universal.



## 1.12.64-universal — 2026-08-02

### Direction
- Algocube digit **7** → COP matrix parity, bit-range copy, local reverse.

### Language
- `PARITYBITS`/`XORREDUCE` cube → XOR-reduce of all bits
- `COPYRANGE` dst doff src soff n — copy n bits between cubes
- `SWAPRANGE`/`REVRANGE` cube lo hi — reverse bits in range
- Proof `87_copyrange_parity.cubalc`

### Prior
See 1.12.63-universal.


## 1.12.63-universal — 2026-08-02

### Direction
- Algocube digit **5** → COP matrix mux + masked equality (select/match pure-C).

### Language
- `MUXBITS`/`BLENDBITS`/`SELECTBITS` dst a b mask — dst = mask?a:b per bit
- `MATCHBITS`/`EQMASK` a b mask → 1 if a≡b under mask ones
- Proof `86_muxbits_match.cubalc`

### Prior
See 1.12.62-universal.


## 1.12.62-universal — 2026-08-02

### Direction
- Algocube digit **6** → RNG range, cell shuffle/pick, random matrix bits, energy read.

### Language
- `RANDRANGE`/`RANDIN` lo hi · `SRANDRANGE` stack lo hi → rand in [lo,hi]
- `SHUFFLECELL` lo hi — Fisher–Yates (seeded)
- `PICKCELL` lo hi — random cell value (`IT`=index)
- `RANDBITS` cube [pct] — randomize matrix (density 0..100, default 50)
- `ENERGYGET` cube → energy 0..100
- Proof `85_rand_shuffle.cubalc`

### Prior
See 1.12.61-universal.


## 1.12.61-universal — 2026-08-02

### Direction
- Algocube digit **5** → COP matrix set relations (equality/subset/disjoint pure-C).

### Language
- `EQBITS`/`SAMEBITS` · `NEBITS`/`NEQBITS` a b → 0/1
- `SUBSETBITS`/`ISSUBSET` · `SUPERSETBITS`/`ISSUPERSET` a b
- `DISJOINTBITS` · `OVERLAPBITS`/`INTERSECTBITS` a b
- Proof `84_eqbits_subset.cubalc`

### Prior
See 1.12.60-universal.


## 1.12.60-universal — 2026-08-02

### Direction
- Algocube digit **8** → stack pack32 + PEXT/PDEP + bit interleave (data-path pure-C).

### Language
- `SPACK32`/`SPACKW` hi lo → 64-bit word; `SHI32`/`SLO32` unpack halves
- `SPEXT`/`SPDEP` src mask — parallel bit extract/deposit (BMI2 duals)
- `SZIP`/`SINTERLEAVE` a b — Morton interleave low 32 bits
- `SUNZIP`/`SDEINTERLEAVE` z → even, odd halves on stack
- Proof `83_pack32_pext_zip.cubalc`

### Prior
See 1.12.59-universal.


## 1.12.59-universal — 2026-08-02

### Direction
- Algocube digit **9** → cell fold arithmetic + scan + reduce (data plane pure-C).

### Language
- `SUBCELL`/`DIVCELL`/`MODCELL` lo hi k — range element-wise arith
- `SCANCELL`/`PREFIXSUM`/`CUMSUM` lo hi — in-place prefix sum
- `CLAMPCELL` lo hi min max — clamp range into bounds
- `PRODCELL`/`MEANCELL`/`AVGCELL` [lo [hi]] — product / integer mean reduce
- Proof `82_cell_div_scan.cubalc`

### Prior
See 1.12.58-universal.


## 1.12.58-universal — 2026-08-02

### Direction
- Algocube digit **3** → COP matrix range fill/clear/flip/count (bit-block ops pure-C).

### Language
- `FILLRANGE`/`SETRANGE` cube lo hi [val] — set bits [lo..hi] (default val=1)
- `CLEARRANGE`/`CLRRANGE`/`ZERORANGE` cube lo hi — clear range
- `FLIPRANGE`/`NOTRANGE`/`INVERTRANGE` cube lo hi — invert range
- `COUNTRANGE`/`ONESRANGE`/`POPRANGE` cube lo hi → `LAST_N` ones in range
- Proof `81_range_bits.cubalc`

### Prior
See 1.12.57-universal.


## 1.12.57-universal — 2026-08-02

### Direction
- Algocube digit **1** → control flow: C-style ternary expressions + unbounded FOREVER loops.

### Language
- Expression ternary: `cond ? then : else` (right-associative; after AND/OR)
- `FOREVER` / `LOOPINF` / `INFINITE` / `LOOPFOREVER` ... `END` — loop until `BREAK` (guard 1e5)
- Proof `80_ternary_forever.cubalc`

### Prior
See 1.12.56-universal.


## 1.12.56-universal — 2026-08-02

### Direction
- Algocube digit **8** → COP matrix↔word data path (pack/extract bitfields pure-C).

### Language
- `WORDFROM`/`MAT2WORD`/`BITS2WORD`/`BITS2N`/`LOADWORD` cube → `LAST_N` from bits 0..63
- `WORDTO`/`WORD2MAT`/`N2BITS`/`STOREWORD`/`WORD2BITS` cube n — deposit word into low bits
- `EXTRACTBITS`/`GETBITS`/`SLICEBITS`/`BITFIELD`/`FIELDGET` cube lo hi → field value
- `DEPOSITBITS`/`PUTBITS`/`SETBITS`/`FIELDSET`/`INJECTBITS` cube lo hi val — write field
- Proof `79_word_bits_bridge.cubalc`

### Prior
See 1.12.55-universal.


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
