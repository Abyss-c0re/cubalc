# Meaningful language iteration (10-minute loop)

**Goal:** extend **language usability** — features people and agents *use* — not combinatorial duals of the same field op.

The 6-minute universal loop may still fill ISA gaps. This loop ships **one human-meaningful commit** every ~10 minutes.

## Forbidden (do not ship)

- Width ladders of an already-landed family (`S0EQ32N` then `S0EQ16N` then `S0EQ8N` …) unless that family is **brand new** and incomplete mid-feature.
- Dual-only mirrors (`D*` copy of existing `S*`) as the sole change of a tick.
- Version plate-only commits.
- Thrash restore churn without a product delta.
- Demoting or deleting: HOLD_FLASH plug permission, core-protect, SMX P2P, modular `src/lang/`.

## Preferred backlog (pick the highest unfinished item)

### A. Ergonomics (agents + humans)

1. ~~**`INCLUDE "path"`** + `programs/lib/`~~ **done** (`lib/hold_seed`, `lib/peer_decide`, proof 574).
2. ~~**Better fail messages** on SMX~~ **done** (line + hint on SERVE/DIAL/TALK).
3. ~~**`ASSERT` with message string**~~ **done** (core + proof 573).
4. ~~**`PRINT` formatting** · `PRINT_JSON` / `DUMP`~~ **done** (named + bare snapshot).
5. ~~**`HELP form`** / CLI `cubalc forms [prefix]`~~ **done** (human plane catalog + JSON).

### B. Host / mesh usability

6. ~~**`SMX SERVE` timeout**~~ **done** (`CUBALC_P2P_TIMEOUT` ms, default 30s).
7. ~~**`cubalc protect status`**~~ **done** (JSON plate summary, no board run).
8. ~~**`SYS ENV` default**~~ **done** (`SYS ENV "NAME" OR "fallback"`).
9. ~~**Soft-fail DIAL**~~ **done** (`CUBALC_P2P_SOFT=1`).

### C. Standard library (CubalC source)

10. ~~`programs/lib/hold_seed.cubalc`~~ **done**.
11. ~~`programs/lib/mesh_exchange.cubalc`~~ **done** (INCLUDE EXCHANGE loop + proof).
12. ~~`programs/lib/peer_decide.cubalc`~~ **done** (recipe).

### D. CLI product

13. ~~**`cubalc doctor`**~~ **done**.
14. ~~**`cubalc cookbook`**~~ **done**.
15. ~~**Help rewrite**~~ **done** (grouped).

### E. Proof & docs that unlock use

16. ~~**Cookbook**~~ **done** (`docs/COOKBOOK.md`).
17. Wire a **missing doc form that is promised** only if you implement the form too (no doc-only lies).
18. ~~**`PRINT_JSON`** / agent dump of vars~~ **done**.
19. ~~**`cubalc forms [prefix]`** live op listing~~ **done**.
20. ~~SERVE/DIAL error hints + soft timeout/connect messages~~ **done**.
21. ~~**`SYS ARG` OR fallback**~~ **done** (script defaults without shell glue).
22. ~~**In-language `HELP [form]`**~~ **done** (LAST/OK/HELP_N + CLI forms).
23. ~~**Sticky `ERR` / `LAST_ERR`**~~ **done** (soft-fail + fatal; survives LAST overwrite).
24. ~~**`cubalc libs`**~~ **done** (programs/lib catalog JSON for INCLUDE).
25. ~~**`cubalc env`**~~ **done** (host CUBALC_* contract JSON · set/default/hint).
26. ~~**INCLUDE short lib name**~~ **done** (`INCLUDE hold_seed` → programs/lib/… + INCLUDE_PATH).
27. ~~**`cubalc examples`**~~ **done** (curated runnable starters JSON · tags/filter).
28. ~~**`EXPECT` soft assert**~~ **done** (OK/LAST_ERR · no fatal · multi-check probes).
29. ~~**`cubalc run -` stdin**~~ **done** (pipe programs without temp files).
30. ~~**`cubalc cat`**~~ **done** (dump lib/program source + cubalc.cat.v1 plate).
31. ~~**`FAIL` / `PASS` soft status**~~ **done** (sticky OK/LAST_ERR without fake expr).
32. ~~**`VERSION` + `cubalc version`**~~ **done** (in-lang LAST + CLI JSON plate).
33. ~~**run plate `last_err`**~~ **done** (sticky LAST_ERR on JSON even when ok).
34. ~~**`cubalc paths`**~~ **done** (install/workspace layout JSON · present flags).
35. ~~**`cubalc which` / SYS WHICH lib**~~ **done** (resolve name → path/kind · INCLUDE-style).
36. ~~**run plate `err_line` / `err_src`**~~ **done** (source snippet when err cites line N).
37. ~~**`cubalc search`**~~ **done** (keyword discovery across forms/libs/examples/env/docs).
38. ~~**`REQUIRE VERSION`**~~ **done** (fail-fast if runtime older than need).
39. ~~**`cubalc selftest`**~~ **done** (live curated usability proofs JSON plate).
40. ~~**`SYS CWD` / `STATE` / `ROOT`**~~ **done** (layout paths in-language for portable scripts).
41. ~~**`INCLUDE OR|SOFT`**~~ **done** (optional module · OK=0 sticky LAST_ERR, no fatal).
42. ~~**`VARS`**~~ **done** (dump all program vars as cubalc.vars.v1 JSON for agents).
43. ~~**`cubalc run -q` / `CUBALC_QUIET`**~~ **done** (plate-only run · no board noise).
44. ~~**`programs/lib/agent_boot`**~~ **done** (INCLUDE agent_boot = REQUIRE + hold_seed + VERSION).
45. ~~**`STATUS`**~~ **done** (cubalc.status.v1 health plate · ok/last_err/version/time).
46. ~~**`CLEAR_ERR`**~~ **done** (wipe sticky ERR/LAST_ERR after soft recovery).
47. ~~**`cubalc init`**~~ **done** (scaffold agent_boot starter · cubalc.init.v1).
48. ~~**`cubalc run -s` / `CUBALC_STRICT`**~~ **done** (soft last_err fails exit + plate ok).
49. ~~**`REQUIRE LIB`**~~ **done** (fail-fast if INCLUDE-style module missing).
50. ~~**`SYS MS`**~~ **done** (wall milliseconds · catalog SYS TIME/MS).
51. ~~**`NOTE`**~~ **done** (agent breadcrumb · LAST/NOTE · no OK/ERR change).
52. ~~**`EXIT`**~~ **done** (halt program · code/reason · plate exit_code/halted).
53. ~~**`INCLUDE ONCE`**~~ **done** (skip reload of already-loaded module path).
54. ~~**`SYS PID` / `HOSTNAME`**~~ **done** (process id + machine name for agents).
55. ~~**`SYS USER` / `UID` / `HOME`**~~ **done** (login + uid + home dir without shell).
56. ~~**`DEFAULT name = value`**~~ **done** (set-if-unset · INCLUDE-safe lib knobs).
57. ~~**`DEFINED name`**~~ **done** (var existence probe · LAST_N 0|1).
58. ~~**`TYPEOF name`**~~ **done** (kind probe · undef|num|str · LAST_N 0|1|2).
59. ~~**`UNSET name`**~~ **done** (remove var · DEFAULT re-apply · LAST_N 0|1).
60. ~~**`IDENTITY`**~~ **done** (cubalc.identity.v1 plate · user@host:pid + vars).
61. ~~**`SYS DATE` / `ISO`**~~ **done** (UTC `YYYY-MM-DDTHH:MM:SSZ` plate stamp).
62. ~~**`SYS TMP` / `TEMP` / `TMPDIR`**~~ **done** (portable temp dir for plate writes).
63. ~~**`SYS MKDIR`**~~ **done** (mkdir -p for plate dirs · soft OK if exists).
64. ~~**`SYS BASENAME` / `DIRNAME`**~~ **done** (path leaf / parent without shell).
65. ~~**`SYS EXTNAME` / `STEM`**~~ **done** (extension with dot / basename without ext).
66. ~~**`SYS SIZE` / `ISDIR` / `ISFILE`**~~ **done** (path metadata probes · soft miss on SIZE).
67. ~~**`SYS READ OR|SOFT`**~~ **done** (optional plate read · soft miss + content fallback).
68. ~~**`SYS RM` / `RENAME`**~~ **done** (delete/move plate files · idempotent RM miss).
69. ~~**`SYS COPY`**~~ **done** (duplicate plate files · LAST_N=bytes · soft miss src).
70. ~~**`SYS REALPATH` / `ABSPATH`**~~ **done** (absolute paths · realpath or cwd-join).
71. ~~**`SYS TOUCH`**~~ **done** (create empty plate marker / refresh mtime).
72. ~~**`SYS SLEEP`**~~ **done** (pause n ms for agent backoff · cap 60s).
73. ~~**`SYS LIST` / `LS`**~~ **done** (dir basenames → LAST · count in LAST_N).
74. ~~**`SYS NTH` / `LINE` / `HEAD` / `TAIL`**~~ **done** (newline field peel · LIST walk).
75. ~~**`SYS GREP` / `FILTER` / `GREPV`**~~ **done** (keep/drop newline fields · LIST filter).
76. ~~**`EACH LINE`**~~ **done** (walk newline fields after LIST/GREP · no NTH index glue).
77. ~~**`SYS TAKE` / `DROP`**~~ **done** (first-n / drop-n newline windows · LIST page).
78. ~~**`SYS SPLIT` / `FIELDS`**~~ **done** (sep → newline fields · PATH/CSV → EACH LINE).
79. ~~**`SYS SORT` / `UNIQ`**~~ **done** (lex sort + adjacent dedupe · stable LIST walks).
80. ~~**`SYS JOIN` / `PATH` catalog**~~ **done** (forms/search/HELP discover path join · resolve_str_arg).
81. ~~**`SYS JOINLINES` / `PASTE`**~~ **done** (join newline fields with sep · anti-SPLIT reports).
82. ~~**`SYS REPLACEALL` / `GSUB`**~~ **done** (all-occurrence subst · plate templates).
83. ~~**`SYS APPEND` / `LOG` catalog**~~ **done** (forms/search/HELP · history lines · resolve_str_arg).
84. ~~**`SYS GREPI` / `IGREP`**~~ **done** (case-insensitive newline filter · log triage).
85. ~~**`SYS STR` / `ITOA`**~~ **done** (integer → decimal string · REPLACEALL {{COUNT}}).
86. ~~**`SYS BEFORE` / `AFTER`**~~ **done** (peel at first needle · log/kv without FIND+MID).
87. ~~**`SYS PUSH` / `ADDLINE`**~~ **done** (append newline field · multi-file hit accumulate).
88. ~~**`SYS EQSI` / `HASI`**~~ **done** (case-insensitive equality/contains · IF after GREPI).
89. ~~**`SYS LINES` / `WC`**~~ **done** (count newline fields · bag size after LAST_N clobber).
90. ~~**`SYS CUT` / `COLUMN`**~~ **done** (Nth field by sep · CSV/path without SPLIT+NTH).
91. ~~**`SYS EMPTY` / `BLANK`**~~ **done** (zero-length / whitespace probes · soft plate IF).
92. ~~**`SYS POP` / `POPLINE`**~~ **done** (peel last bag field · dual of PUSH · POP_REST).
93. ~~**`SYS MTIME` / `AGE`**~~ **done** (file mtime epoch / age seconds · plate freshness).
94. ~~**`SYS STARTSI` / `ENDSI`**~~ **done** (case-insensitive prefix/suffix · path/ext/severity).
95. ~~**`SYS BETWEEN` / `MIDOF` / `EXTRACT`**~~ **done** (peel between delimiters · plate/kv/log without FIND+MID).
96. ~~**`SYS FINDI` / `INDEXI`**~~ **done** (case-insensitive string index · locate without UPPER+FIND).
97. ~~**`SYS ENV SET` / `UNSET` · `SETENV`**~~ **done** (process setenv/unsetenv · CUBALC_* without shell).
98. ~~**`SYS REVL` / `REVLINES` / `TAC`**~~ **done** (reverse newline bag order · LIFO/newest-first without shell).
99. ~~**`SYS WORDS` / `TOKENIZE`**~~ **done** (whitespace → newline fields · free text → bag pipeline).
100. ~~**`SYS HASLINE` / `HASLINEI`**~~ **done** (exact bag-field membership · work bags without EACH+EQS).
101. ~~**`SYS PREPEND` / `POPHEAD`**~~ **done** (FIFO bag front · dual of PUSH/POP without REVL/NTH).
102. ~~**`SYS REMOVELINE` / `DROPLINE`**~~ **done** (drop first exact bag field · ack work without EACH rebuild).
103. ~~**`SYS FINDLINE` / `LINEINDEX`**~~ **done** (0-based exact field index · locate-then-NTH without EACH).
104. ~~**`SYS SETLINE` / `REPLACELINE`**~~ **done** (set bag field by index · FINDLINE then update in place).
105. ~~**`SYS INSERTLINE` / `INSLINE`**~~ **done** (insert bag field at 0-based index · priority/ordered bags).
106. ~~**`SYS DROPNTH` / `DROPAT`**~~ **done** (drop bag field by 0-based index · FINDLINE then delete).
107. ~~**`SYS MOVELINE` / `MOVEAT`**~~ **done** (move bag field to final index · promote/demote priority).
108. ~~**`SYS COUNTLINE` / `COUNTFIELD`**~~ **done** (count exact bag field matches · HASLINE with count).
109. ~~**`SYS SETMATCH` / `REPLACEMATCH`**~~ **done** (replace first exact bag field by value · status by name).
110. ~~**`cubalc run -e` / `--expr`**~~ **done** (inline program · multi -e · no temp file).
111. ~~**`SYS CHDIR` / `CD`**~~ **done** (change process cwd · relative plate I/O without shell).
112. ~~**`SYS RAND` / `RANDOM`**~~ **done** (uniform int · agent jitter/sample without shell).
113. ~~**`SYS MIN` / `MAX` / `CLAMP`**~~ **done** (host-plane numeric bounds · cap retries/jitter).
114. ~~**`SYS CMP` / `SCMP` / `IABS`**~~ **done** (three-way compare + abs · sort keys without dual IF).
115. ~~**`SYS SUM` / `PROD` / `AVG`**~~ **done** (host aggregates · multi-arg or newline bag · no shell awk).
116. ~~**`SYS SORTN` / `NSORT`**~~ **done** (numeric bag sort · lex SORT footgun fix for scores/sizes).
117. ~~**`SYS RANGE` / `SEQ` / `IOTA`**~~ **done** (int sequence bags · EACH/SUM fixtures without shell seq).
118. ~~**`SYS SIGN` / `DIV` / `MOD`**~~ **done** (signum + integer div/mod · page/bucket without ISA soup).
119. ~~**`SYS PICK` / `CHOICE` / `SAMPLE`**~~ **done** (random bag field · sample LIST/RANGE without shell).
120. ~~**`SYS GCD` / `LCM`**~~ **done** (multi-arg gcd/lcm · reduce ratios / cycles without ISA soup).
121. ~~**`SYS IN` / `WITHIN`**~~ **done** (inclusive numeric range membership · IF/guards without dual CMP).
122. ~~**`SYS POW` / `ISQRT`**~~ **done** (integer power + floor sqrt · backoff/geometry without ISA soup).
123. ~~**`SYS SHUFFLE` / `SHUF`**~~ **done** (Fisher–Yates bag shuffle · randomize peers/work without shell shuf).
124. ~~**`SYS COALESCE` / `NVL`**~~ **done** (first non-empty string · default chains without nested IF EMPTY).
125. ~~**`SYS UNION` / `INTERSECT` / `DIFF`**~~ **done** (bag set ops · merge/dedup/subtract work lists without EACH+HASLINE).
126. ~~**`SYS ZIP` / `KEYS` / `VALS`**~~ **done** (pair bag fields by index · peel kv columns without EACH+CAT).
127. ~~**`SYS PREFIXALL` / `SUFFIXALL`**~~ **done** (tag every bag field · path/peer labels without EACH+CAT).
128. ~~**`SYS FILL` / `REPEATL`**~~ **done** (bag of n copies · default ZIP columns without EACH).
129. ~~**`SYS ENUMERATE` / `NUMBER`**~~ **done** (index-prefix bag fields · ranked plates without EACH+STR+CAT).
130. ~~**`SYS SQUEEZE` / `COMPACT`**~~ **done** (drop empty/blank bag fields · clean VALS/SPLIT/LIST without EACH).
131. ~~**`SYS MEDIAN` / `P50`**~~ **done** (integer median of args or bag · score plates without shell).
132. ~~**`SYS TRIMALL` / `MAPTRIM`**~~ **done** (trim whitespace on every bag field · clean READ/SPLIT before SQUEEZE).
133. ~~**`SYS MIDLINES` / `SLICEBAG`**~~ **done** (bag field window [start..end] · middle slice without DROP+TAKE).
134. ~~**`SYS GREPANY` / `GREPOR`**~~ **done** (keep fields matching any of several needles · multi-severity log triage).
135. ~~**`SYS GREPALL` / `GREPAND`**~~ **done** (keep fields matching every needle · multi-tag AND triage).
136. ~~**`REQUIRE ENV` / `VAR`**~~ **done** (fail-fast if host env missing/empty · agent host config gate).
137. ~~**`SYS UPPERALL` / `LOWERALL`**~~ **done** (ASCII case-map every bag field · normalize before HASLINE/set ops).
138. ~~**`SYS MAPREPLACE` / `GSUBALL`**~~ **done** (REPLACEALL on every bag field · path/tag rewrite without EACH).
139. ~~**`SYS FREQ` / `HIST`**~~ **done** (bag field frequency histogram key:count · status rollups without EACH).
140. ~~**`SYS CUTALL` / `MAPCUT`**~~ **done** (peel Nth sep-field from every bag line · log columns → FREQ).
141. ~~**`SYS SORTFREQ` / `SORTBYCOUNT`**~~ **done** (sort FREQ key:count bag by count · top severities without shell).
142. ~~**`SYS BEFOREALL` / `AFTERALL`**~~ **done** (BEFORE/AFTER on every bag field · FREQ keys/counts without EACH).
143. ~~**`SYS FIRSTMATCH` / `GREP1`**~~ **done** (first bag field containing needle · one-line pick without GREP+TAKE).
144. ~~**`SYS CHUNK` / `BATCH`**~~ **done** (group bag fields into batches of n · work-list paging without EACH).
145. ~~**`SYS WINDOW` / `SLIDE`**~~ **done** (overlapping windows of n bag fields · consecutive pairs/ngrams without EACH).
146. ~~**`SYS STRIDE` / `EVERY`**~~ **done** (keep index%step==offset fields · multi-worker partition without EACH).
147. ~~**`SYS ROTATE` / `ROTL` / `ROTR`**~~ **done** (cyclic bag field rotate · round-robin peers without EACH).
148. ~~**`SYS FLATTEN` / `UNCHUNK`**~~ **done** (split every bag field by sep → flat bag · reverse CHUNK join).
149. ~~**`SYS COUNTMATCH` / `GREPCOUNT`**~~ **done** (count fields containing needle · log tallies without GREP clobber).
150. ~~**`SYS LENALL` / `MAPLEN`**~~ **done** (length of every bag field → decimal bag · LENALL_SUM size rollups).
151. ~~**`SYS MIN` / `MAX` bag mode**~~ **done** (min/max over newline numeric bags like SUM · after LENALL width).
152. ~~**`SYS ARGMAX` / `ARGMIN`**~~ **done** (0-based index of first extreme · LENALL→ARGMAX→NTH longest field).
153. ~~**`SYS PADALL` / `LPADALL` / `RPADALL`**~~ **done** (pad every bag field to width · column-align after LENALL+MAX).
154. ~~**`SYS SORTLEN` / `LENSORT`**~~ **done** (sort bag fields by string length · longest-first without LENALL rebuild).
155. ~~**`SYS LASTMATCH` / `GREP1L`**~~ **done** (last bag field containing needle · latest hit without REVL+FIRSTMATCH).
156. ~~**`SYS MAXLEN` / `MINLEN`**~~ **done** (max/min bag field string length → LAST_N · PADALL width without LENALL+MAX).
157. ~~**`SYS TRUNCALL` / `CLIPALL`**~~ **done** (truncate every bag field to width · clip logs without EACH+LEFT · dual of PADALL).
158. ~~**`SYS LONGEST` / `SHORTEST`**~~ **done** (first longest/shortest bag field text · no MAXLEN_I+NTH glue).
159. ~~**`SYS COMMONPREFIX` / `LCP` · `COMMONSUFFIX`**~~ **done** (shared bag prefix/suffix · path roots without EACH).
160. ~~**`SYS STRIPPREFIX` / `STRIPSUFFIX` / `STRIPCOMMON`**~~ **done** (strip ends / LCP from bag fields · relative paths one-shot).
161. ~~**`SYS DRAWN` / `SAMPLEK` / `NPICK`**~~ **done** (sample k unique bag fields without replacement · multi-peer without SHUFFLE+TAKE).
162. ~~**`SYS LOOKUP` / `KVGET`**~~ **done** (peel value from key:val bag field · FREQ counts without FIRSTMATCH+AFTER).
163. ~~**`SYS KVSET` / `SETKV`**~~ **done** (set/update key:val bag field · dual of LOOKUP · plate kv write).
164. ~~**`SYS KVINC` / `INCKV`**~~ **done** (bump key:val numeric counter · FREQ severity without LOOKUP+arith+KVSET).
165. ~~**`SYS KVDEL` / `DELKV`**~~ **done** (drop key:val bag field · dual of KVSET · ack without FINDLINE+DROPNTH).
166. ~~**`SYS LOOKUP … OR`**~~ **done** (LOOKUP bag key [sep] OR fallback · FREQ defaults like ENV/ARG).
167. ~~**`SYS MERGEKV` / `KVADDALL`**~~ **done** (merge key:val bags by sum · combine FREQ without EACH+KVINC).
168. ~~**`SYS SUMKV` / `TOTALKV`**~~ **done** (sum key:val numeric values · FREQ total without VALS+SUM).
169. ~~**`SYS TOPKEY` / `BOTKEY`**~~ **done** (key with max/min value · dominant FREQ without SORTFREQ+TAKE+BEFORE).
170. ~~**`SYS LOOKUPN` / `KVGETN`**~~ **done** (peel key:val as int LAST_N · FREQ arith without LOOKUP+NUM).
171. ~~**`SYS THRESHKV` / `KEEPVAL`**~~ **done** (keep key:val value>=min · denoise FREQ without EACH rebuild).
172. ~~**`SYS PCTKV` / `SHAREKV`**~~ **done** (key:val values as integer %% of total · FREQ share without EACH+arith).
173. ~~**`SYS CAPKV` / `CLAMPKV`**~~ **done** (clamp key:val values to max · cap FREQ outliers before PCTKV).
174. ~~**`SYS SCALEKV` / `MULKV`**~~ **done** (multiply key:val values · weight FREQ before MERGEKV).
175. ~~**`SYS DIFFKV` / `SUBKV`**~~ **done** (subtract key:val bags by key · FREQ now−baseline without EACH).
176. ~~**`SYS ADDKV` / `OFFSETKV`**~~ **done** (add delta to key:val values · Laplace/score offset without EACH).
177. ~~**`SYS DROPZERO` / `KEEPNZ`**~~ **done** (drop key:val value==0 · clean DIFFKV zeros keep +/−).
178. ~~**`SYS ABSKV` / `MAGKV`**~~ **done** (absolute key:val values · rank DIFFKV deltas by magnitude).
179. ~~**`SYS SIGNKV` / `DIRKV`**~~ **done** (map key:val to −1|0|1 · DIFFKV direction without EACH).
180. ~~**`SYS DIVKV` / `IDIVKV`**~~ **done** (integer-divide key:val values · mean after N-way MERGEKV).
181. ~~**`SYS AVGKV` / `MEANKV`**~~ **done** (integer mean of key:val values · typical FREQ count ≠ DIVKV).
182. ~~**`SYS MEDIANKV` / `P50KV`**~~ **done** (integer median of key:val values · robust FREQ mid vs AVGKV).
183. ~~**`SYS KEEPKEY` / `GREPKEY`**~~ **done** (filter key:val by key needle · DROPKEY invert · KEEPKEYI).
184. ~~**`SYS GLOB` / `MATCHFILES`**~~ **done** (list basenames matching * ? [] · plate discovery without shell).
185. ~~**`SYS PATHGLOB` / `PGLOB`**~~ **done** (shell-style path glob → full paths · ready for READ/RM).
186. ~~**`REQUIRE PATH` / `DIR` / `REG`**~~ **done** (fail-fast host path/kind gates · plate layout without EXIST+IF).
187. ~~**`SYS FILTERGLOB` / `MATCHBAG`**~~ **done** (shell-pattern bag filter · full field or basename · PATHGLOB re-filter).
188. ~~**`SYS BASENAMEALL` / `DIRNAMEALL` / `EXTALL` / `STEMALL`**~~ **done** (map path peels over bags · PATHGLOB without EACH).
189. ~~**`SYS KEEPFILES` / `KEEPDIRS` / `KEEPEXIST`**~~ **done** (filter path bags by host kind · PATHGLOB before READ).
190. ~~**`SYS SIZEALL` / `MAPSIZE`**~~ **done** (path bag → byte sizes · SIZEALL_SUM inventory without EACH+SIZE).
191. ~~**`SYS MTIMEALL` / `AGEALL`**~~ **done** (path bag mtime/age maps · MAX/MIN freshness without EACH).
192. ~~**`SYS NEWEST` / `OLDEST` / `LARGEST` / `SMALLEST`**~~ **done** (pick path by mtime/size · no ARGMAX+NTH glue).
193. ~~**`SYS SORTMTIME` / `SORTSIZE`**~~ **done** (order path bags by mtime/size · ASC|DESC · no shell ls -t).
194. ~~**`SYS FRESH` / `KEEPSTALE`**~~ **done** (filter path bags by age threshold · cache/lease without EACH+AGE).
195. ~~**`SYS KEEPNEWER` / `KEEPOLDER`**~~ **done** (filter path bags vs ref mtime · make rebuild without EACH+MTIME).
196. ~~**`SYS KEEPBIGGER` / `KEEPSMALLER`**~~ **done** (filter path bags by size threshold · GC reclaim without EACH+SIZE).
197. ~~**`SYS RMALL` / `UNLINKALL`**~~ **done** (bulk remove path bag · GC reclaim close without EACH+RM).
198. ~~**`SYS TOUCHALL` / `ENSUREALL`**~~ **done** (bulk create/refresh path bag · lease stamps without EACH+TOUCH).
199. ~~**`SYS COPYALL` / `CPALL`**~~ **done** (bulk copy path bag into dest dir · snapshot without EACH+COPY).
200. ~~**`SYS MKDIRALL` / `ENSUREDIRS`**~~ **done** (bulk mkdir -p path bag · layout trees without EACH+MKDIR).
201. ~~**`SYS MOVEALL` / `MVALL`**~~ **done** (bulk move path bag into dest dir · archive without EACH+RENAME).
202. ~~**`SYS WALK` / `FINDALL` / `TREEGLOB`**~~ **done** (recursive full-path discovery · deep trees beyond PATHGLOB).
203. ~~**`SYS EQFILE` / `SAMEFILE`**~~ **done** (identical file content probe · verify COPY without shell cmp).
204. ~~**`SYS LOGALL` / `APPENDFILES`**~~ **done** (append line to every path in bag · multi-log without EACH+APPEND).
205. ~~**`SYS GREPFILES` / `SEARCHFILES`**~~ **done** (filter path bag by file content · PATHGLOB triage without EACH+READ+HAS).
206. ~~**`SYS READALL` / `CATFILES`**~~ **done** (concat path bag file contents → LAST · multi-file join without EACH+READ+CAT).
207. ~~**`SYS WRITEALL` / `WRITEFILES`**~~ **done** (overwrite every path in bag with data · multi-file stamp without EACH+WRITE).
208. ~~**`SYS REPLACEFILES` / `SUBFILES`**~~ **done** (REPLACEALL inside every path · bulk rewrite without EACH+READ+WRITE).
209. ~~**`SYS COUNTINFILES` / `GREPCOUNTFILES`**~~ **done** (total needle hits across path bag · multi-file tally without EACH+READ).
210. ~~**`SYS FIRSTFILE` / `LASTFILE`**~~ **done** (first/last path whose content matches needle · no GREPFILES+HEAD glue).
211. ~~**`SYS GREPLINES` / `EXTRACTLINES`**~~ **done** (collect matching lines from path bag · multi-file harvest without EACH+READ+GREP).
212. ~~**`SYS HEADFILE` / `TAILFILE`**~~ **done** (first/last n lines of a file → bag · peek without full READ/shell head).
213. ~~**`SYS LINECOUNTALL` / `WCALL`**~~ **done** (path bag → line counts + sum · inventory without EACH+READ+LINES).
214. ~~**`SYS MIDFILE` / `LINESLICE`**~~ **done** (file line window [start..start+count) · page without READ+DROP+TAKE).
215. ~~**`SYS LINEAT` / `FILELINE`**~~ **done** (single 0-based file line → LAST · no MIDFILE+HEAD glue).
216. ~~**`SYS SETFILELINE` / `FILESETLINE`**~~ **done** (replace 0-based file line in place · dual of LINEAT).
217. ~~**`SYS INSERTFILELINE` / `FILEINSERTLINE`**~~ **done** (insert line at 0-based file index · append if past end).
218. ~~**`SYS DROPFILELINE` / `FILEDROPLINE`**~~ **done** (drop 0-based file line in place · dual of INSERTFILELINE).
219. ~~**`SYS FINDFILELINE` / `FILEFINDLINE`**~~ **done** (0-based first file line containing needle · locate then SET/DROP).
220. ~~**`SYS LASTFINDFILELINE` / `FILELASTFIND`**~~ **done** (0-based last file line containing needle · dual of FINDFILELINE).
221. ~~**`SYS COUNTFILELINE` / `FILECOUNTLINE`**~~ **done** (count file lines containing needle → LAST_N · single-file tally).
222. ~~**`SYS GREPFILE` / `FILEGREP`**~~ **done** (keep matching lines from one file → bag · single-log harvest).
223. ~~**`SYS SETMATCHFILE` / `FILESETMATCH`**~~ **done** (replace first file line containing needle · FIND+SET one-shot).
224. ~~**`SYS DROPMATCHFILE` / `FILEDROPMATCH`**~~ **done** (drop first file line containing needle · FIND+DROP one-shot).
225. ~~**`SYS DROPALLMATCHFILE` / `FILEDROPALLMATCH`**~~ **done** (drop all file lines containing needle · bulk purge without FIND+DROP loop).
226. ~~**`SYS SETALLMATCHFILE` / `FILESETALLMATCH`**~~ **done** (replace all file lines containing needle · bulk status rewrite without FIND+SET loop).
227. ~~**`SYS INSERTMATCHFILE` / `FILEINSERTMATCH` / `AFTERMATCHFILE`**~~ **done** (insert line after first match · FIND+INSERT one-shot for config patches).
228. ~~**`SYS BEFOREMATCHFILE` / `INSERTBEFOREMATCH`**~~ **done** (insert line before first match · dual of AFTERMATCHFILE for headers/imports).
229. ~~**`SYS LOOKUPFILE` / `FILELOOKUP` / `KVFILEGET`**~~ **done** (peel key=val from plate file · OR fallback · no READ+LOOKUP glue).
230. ~~**`SYS KVFILESET` / `FILEKVSET` / `SETKVFILE`**~~ **done** (set/update key=val in plate file · create if missing · dual of LOOKUPFILE).
231. ~~**`SYS KVFILEDEL` / `FILEKVDEL` / `DELKVFILE`**~~ **done** (drop key=val line from plate file · dual of KVFILESET · ack without READ+WRITE glue).
232. ~~**`SYS KVFILEINC` / `INCKVFILE` / `FILEKVINC`**~~ **done** (bump numeric key=val in plate file · retry/severity counters without LOOKUP+arith+SET glue).
233. ~~**`SYS ENSURELINE` / `ENSUREFILELINE` / `ADDIFMISSING`**~~ **done** (append exact line if missing · idempotent config flags/markers).
234. ~~**`SYS HASFILELINE` / `FILEHASLINE`**~~ **done** (exact file line membership probe · dual of bag HASLINE · IF before ENSURELINE).
235. ~~**`SYS DROPEXACTFILE` / `DROPFILEEXACT` / `REMOVELINEFILE`**~~ **done** (drop first exact file line · dual of bag REMOVELINE · no substring false positives).
236. ~~**`SYS TOGGLELINE` / `TOGGLEFILELINE` / `FLIPLINE`**~~ **done** (flip exact file line presence · feature flags without HAS+ENSURE/DROP IF).
237. ~~**`SYS KVFILEENSURE` / `ENSUREKVFILE` / `DEFAULTKVFILE`**~~ **done** (set plate key=val only if missing · file DEFAULT · no LOOKUP+IF+SET glue).
238. ~~**`SYS SUBSTENV` / `ENVSUBST` / `EXPANDENV`**~~ **done** (expand `$NAME`/`${NAME}` from env+vars · path templates without shell envsubst).
239. ~~**`SYS SWAPFILES` / `DUPLEX` / `TCPXFER` / `TCPLISTEN`**~~ **done** (bidirectional any-payload host flow · plate swap + mailbox RTT + CBXF TCP).
240. ~~**`SYS WAITFILE` / `WAITPATH` / `POLLFILE`**~~ **done** (poll until path exists · agent plate/peer handoff without EXIST+SLEEP loop).
241. ~~**`SYS SUBSTFILE` / `EXPANDFILE` / `ENVSUBSTFILE`**~~ **done** (materialize `$VAR` template file · READ+SUBSTENV+WRITE one-shot).
242. ~~**`SYS HASH` / `HASHFILE` / `FINGERPRINT`**~~ **done** (FNV-1a 32-bit hex stamps for plate/cache integrity without shell md5).
243. ~~**`SYS WAITMATCH` / `WAITCONTAINS` / `POLLMATCH`**~~ **done** (poll until file content contains needle · peer plate ready without READ+HAS+SLEEP).
244. ~~**`SYS WRITEATOMIC` / `ATOMICWRITE` / `SAFEWRITE`**~~ **done** (temp+rename plate write · crash-safer multi-agent status without torn mid-read).
245. ~~**`SYS LOCKFILE` / `UNLOCKFILE` / `FLOCK`**~~ **done** (exclusive path.lock agent mutex · multi-agent critical sections without shell flock).
246. ~~**`SYS WAITGONE` / `WAITMISSING` / `WAITDELETE`**~~ **done** (poll until path does not exist · dual of WAITFILE · unlock/cleanup handoff).
247. ~~**`SYS WAITSTABLE` / `WAITQUIET` / `STABLEFILE`**~~ **done** (poll until size+mtime quiet · settle peer write before READ without torn plate).
248. ~~**`SYS WAITCHANGED` / `WAITMODIFIED` / `POLLCHANGE`**~~ **done** (poll until appear or size/mtime change · SINCE snapshot · peer status without known content).
249. ~~**`SYS MKTEMP` / `TEMPFILE` / `MAKETEMP`**~~ **done** (unique empty file under TMP · collision-free agent plate paths without shell mktemp).
250. ~~**`SYS CLAIM` / `CLAIMFILE` / `CREATEEXCL`**~~ **done** (exclusive path create for one-shot work-item claims · soft miss if held · release with RM).
251. ~~**`SYS STAT` / `FSTAT` / `FILESTAT`**~~ **done** (one-shot path metadata · exist/kind/size/mtime/isfile · no EXIST+SIZE+MTIME glue).
252. ~~**`SYS MKTEMPDIR` / `MKDTEMP` / `MAKETEMPDIR`**~~ **done** (unique empty dir under TMP · collision-free agent workspaces without shell mktemp -d).
253. ~~**`SYS RMDIR` / `REMOVEDIR` / `RD`**~~ **done** (remove empty directory · missing soft OK · clean MKTEMPDIR without shell rmdir).
254. ~~**`SYS BASE64` / `B64ENC` · `BASE64D` / `B64DEC`**~~ **done** (standard base64 encode/decode · plate payload transport without shell base64).
255. ~~**`SYS HEX` / `HEXENC` · `HEXD` / `HEXDEC`**~~ **done** (lowercase hex encode/decode · binary-safe plate fields without shell xxd).
256. ~~**`SYS URLENC` / `URLENCODE` · `URLDEC` / `URLDECODE`**~~ **done** (RFC 3986 percent-encode/decode · path/query-safe plates without shell).
257. ~~**`SYS UUID` / `GUID` · `UUID0` / `UUIDNODASH`**~~ **done** (RFC 4122 v4 ids · agent work/claim stamps without shell uuidgen).
258. ~~**`SYS JSONESC` / `JESC` · `JSONUNESC` / `JUNESC`**~~ **done** (JSON string-body escape/unescape · plate fields without hand-escaping).
259. ~~**`SYS CRC32` / `CRC` · `CRC32FILE` / `FILECRC32`**~~ **done** (IEEE CRC-32 hex stamps · interoperable integrity without shell cksum).
260. ~~**`SYS SYMLINK` / `LN` · `READLINK` / `LINKTARGET`**~~ **done** (create/peel plate path aliases without shell ln -s/readlink).
261. ~~**`SYS HUMANSIZE` / `BYTESFMT` / `HUMANSZ`**~~ **done** (integer B/K/M/G/T labels · SIZE inventory without shell numfmt).
262. ~~**`SYS ISLINK` / `ISLNK` / `ISSYMLINK`**~~ **done** (symlink probe · IF before READLINK without soft-fail glue).
263. ~~**`SYS MODE` / `PERM` · `CHMOD` / `SETMODE`**~~ **done** (octal permission probe/set · plate lock-down without shell).
264. ~~**`SYS PARSESIZE` / `FROMHUMAN` / `TOBYTES`**~~ **done** (parse 1K/2M/1G → bytes · reverse of HUMANSIZE for configs).
265. ~~**`SYS CHMOD` string-var octal**~~ **done** (CHMOD path m with m="0600" parses octal · not strlen coerce).
266. ~~**`SYS MONOTONIC` / `MONO` / `STEADY_MS`**~~ **done** (CLOCK_MONOTONIC ms · elapsed timing immune to wall jumps).
267. ~~**`SYS NPROC` / `CPUS` / `CORES`**~~ **done** (online processor count · STRIDE/CHUNK worker partition without shell).
268. ~~**`SYS UNAME` / `OS` / `ARCH` / `MACHINE` / `PLATFORM`**~~ **done** (kernel + arch probes · portable agent IF without shell uname).
`269. ~~**`SYS LOCAL` / `LOCALTIME` / `LOCALDATE`**~~ **done** (local wall stamp · human logs without shell date · complements UTC DATE).
270. ~~**`REQUIRE BIN` / `SYS WHICHBIN`**~~ **done** (PATH-only tool gate · soft probe + fail-fast · no lib fallback).
271. ~~**`SYS WHICHBIN` string-var / LAST name**~~ **done** (resolve tool bag vars · not IDENT text "tool").
272. ~~**`SYS ISATTY` / `TTY`**~~ **done** (terminal stream probe · pipe-aware agents).
273. ~~**`SYS LOADAVG` / `LOAD`**~~ **done** (1/5/15 load · centiload IF for agent backpressure).
274. ~~**`SYS UPTIME` / `BOOTAGE`**~~ **done** (seconds since boot · agent warmup/lease without shell uptime).
275. ~~**`SYS MEM` / `MEMINFO`**~~ **done** (RAM total/avail/free kB · agent low-mem backpressure without shell free).
276. ~~**`SYS DF` / `DISKFREE`**~~ **done** (filesystem free kB for path · pre-write disk gate without shell df).
277. ~~**`SYS FSYNC` / `SYNCFILE`**~~ **done** (durable file flush after WRITE · multi-agent plate durability).
278. ~~**`SYS DOTENV` / `LOADENV` / `ENVFILE`**~~ **done** (load KEY=VAL plate into process env · agent .env without shell).
279. ~~**`SYS DUMPENV` / `ENVALL` / `LISTENV`**~~ **done** (process env as KEY=VAL bag · optional prefix · dual of DOTENV).
280. ~~**`SYS WRITEENV` / `SAVENV` / `DUMPENVFILE`**~~ **done** (persist process env KEY=VAL plate · dual of DOTENV · optional prefix).
281. ~~**`SYS TRUNCATE` / `FTRUNCATE` / `FILETRUNC`**~~ **done** (resize/clear plate file keep inode · default 0 · not TRUNCALL bag clip).
282. ~~**`SYS DIRSYNC` / `FSYNCDIR` / `SYNCDIR`**~~ **done** (fsync directory or parent of file · durable WRITEATOMIC renames).
283. ~~**`SYS HARDLINK` / `HLINK` · `NLINK` / `LINKS`**~~ **done** (hard link same inode + link count · shared plates without COPY).
284. ~~**`SYS SAMEINODE` / `EQINODE` · `INODE`**~~ **done** (same device+inode probe + inode stamp · verify HARDLINK ≠ EQFILE content).
285. ~~**`SYS TEE` / `WRITETEE` / `SAVEKEEP`**~~ **done** (write file keep content in LAST · chain without READ back).
286. ~~**`SYS FILEUID` / `FILEGID` / `OWNEDBY`**~~ **done** (file owner uid/gid + euid owns probe · agent plate security).
287. ~~**`SYS CANREAD` / `CANWRITE` / `CANEXEC`**~~ **done** (access R/W/X probes · pre-flight plate I/O without soft-fail glue).
288. ~~**`SYS OWNERNAME` / `GROUPNAME`**~~ **done** (path → login/group name · human plate labels without shell stat %U/%G).
289. ~~**`SYS CANCREATE` / `CREATABLE`**~~ **done** (create/overwrite pre-flight · parent W_OK|X_OK or path W_OK).
290. ~~**`SYS UMASK` / `GETUMASK` / `SETUMASK`**~~ **done** (process file-creation mask get/set · private plate defaults without shell).
291. ~~**`SYS PUSHD` / `POPD` / `DIRSTACK`**~~ **done** (cwd stack · temp relative plate I/O without losing base).
292. ~~**`SYS KINDSTR` / `FILEKIND`**~~ **done** (path → file|dir|link|missing|… · triage without multi-probe glue).
293. ~~**`SYS RELPATH` / `REL` / `STRIPBASE`**~~ **done** (base+path → relative plate path · portable after WALK/PUSHD).
294. ~~**`SYS ATIME` / `CTIME`**~~ **done** (access + status-change epoch · plate freshness beyond MTIME).
295. ~~**`SYS HASENV` / `ENVHAS`**~~ **done** (soft env presence probe · IF before REQUIRE ENV without fatal).
296. ~~**`SYS GID` / `EGID` / `GROUP`**~~ **done** (process group id + name · agent identity without shell id).
297. ~~**`SYS GROUPS` / `INGROUP`**~~ **done** (supplementary group bag + membership probe · ACL without shell id -nG).
298. ~~**`SYS PPID` / `PGID` / `SID`**~~ **done** (parent / process-group / session ids · process tree without shell · not DIRNAME PARENT).
299. ~~**`SYS EUID` / `ISROOT`**~~ **done** (effective uid + root privilege probe · setuid-aware gates without shell id -u).
300. ~~**`SYS SHELL`**~~ **done** (login shell path · $SHELL/passwd · agent profile without shell echo).
301. ~~**`SYS GECOS` / `FULLNAME`**~~ **done** (passwd display name · human plate labels without getent/finger).
302. ~~**`SYS TTYNAME` / `CTTY`**~~ **done** (terminal device path · session identity beyond ISATTY without shell tty).
303. ~~**`SYS NICE` / `RENICE`**~~ **done** (process nice get/set · agent yield/backpressure without shell nice).
304. ~~**`SYS SHQUOTE` / `SHELLQUOTE`**~~ **done** (POSIX single-quote shell-safe wrap · SPAWN/log strings without hand escaping).
305. ~~**`SYS SHUNQUOTE` / `UNQUOTE`**~~ **done** (peel POSIX single-quote wrap · dual of SHQUOTE without shell eval).
306. ~~**`SYS GETLOGIN` / `LOGINNAME`**~~ **done** (session login via getlogin · utmp identity without shell who am i).
307. ~~**`SYS ENTROPY` / `URANDOM`**~~ **done** (n random bytes as hex · agent nonces/tokens beyond UUID/RAND).
308. ~~**`SYS PARSEMS` / `DURATION`**~~ **done** (human duration 5s/2m/1h → ms · SLEEP/WAITFILE configs).
309. ~~**`SYS HUMANMS` / `FMTMS`**~~ **done** (ms → compact 5s/1m30s plate labels · dual of PARSEMS).
310. ~~**`SYS FROMTIME` / `EPOCHISO`**~~ **done** (epoch seconds → UTC ISO stamp · MTIME plates without shell date).
311. ~~**`SYS PARSEISO` / `TOEPOCH`**~~ **done** (ISO stamp → epoch · dual of FROMTIME · plate age/compare).
312. ~~**`SYS TIMEDIFF` / `ELAPSED`**~~ **done** (epoch second delta a−b or now−a · plate age/deadline math).
313. ~~**`SYS ENVDEFAULT` / `ENSUREENV`**~~ **done** (setenv only if missing/empty · agent boot defaults).
314. ~~**`SYS ADDTIME` / `DEADLINE`**~~ **done** (epoch + seconds / now+secs lease expiry · dual of TIMEDIFF).
315. ~~**`SYS EXPIRED` / `REMAINING`**~~ **done** (lease due probe + TTL secs · IF without TIMEDIFF+CMP).
316. ~~**`SYS MS2SEC` / `SEC2MS`**~~ **done** (ms↔seconds bridge · PARSEMS→DEADLINE / TTL→SLEEP).
317. ~~**`SYS PARSEMS` compound**~~ **done** (1h30m/2m15s multi-unit · HUMANMS round-trip).
318. ~~**`SYS SLEEP` duration string**~~ **done** (`SLEEP "1s"`/`"250ms"`/`"1s500ms"` · no PARSEMS glue).
319. ~~**`SYS WAITFILE` duration timeout**~~ **done** (`WAITFILE path "2s"` · WAITGONE/WAITMATCH/WAITCHANGED · no PARSEMS glue).
320. ~~**`SYS DEADLINE` duration**~~ **done** (`DEADLINE "5m"`/`"30s"` · ADDTIME offset · no PARSEMS+MS2SEC glue).
321. ~~**`SYS ADDISO` / `SHIFTISO`**~~ **done** (ISO + secs|duration → new ISO · no PARSEISO+ADDTIME+FROMTIME glue).
322. ~~**`SYS DIFFISO` / `ISOAGE`**~~ **done** (ISO − ISO / now−ISO secs · dual ADDISO · no PARSEISO+TIMEDIFF glue).
323. ~~**OOP/COP plane**~~ **done** (`CLASS`/`METHOD`/`NEW`/`SEND` · `ENTITY`/`SPAWN`/`TICK` · reusable cubes beyond FN).
324. ~~**INCLUDE retains source**~~ **done** (FN/CLASS METHOD bodies from lib stay valid · life_cell + life_engine).
325. ~~**`SYS HUMANSECS` / `HUMANAGE`**~~ **done** (secs → 5s/1m30s labels · AGE/REMAINING/DIFFISO without SEC2MS+HUMANMS).
326. ~~**`LISTCLASSES` / `LISTOBJS` / `HASMETHOD`**~~ **done** (OOP agent discovery · LISTMETHODS · soft probes).
327. ~~**`HASOBJ` / `DELETEOBJ` / `HASFIELD`**~~ **done** (object lifecycle + field probes · NEW reuses free slots · not cube DESTROY).
328. ~~**`DUMPOBJ` / `INSPECT`**~~ **done** (object field:value bag / cubalc.obj.v1 JSON · agent snapshot without EACH+GETF).
329. ~~**`GETF … OR` / `TRYGETF`**~~ **done** (soft field read + default like ENV/LOOKUP · string field name · no HASFIELD glue).
330. ~~**SETF/NEW string-from-var**~~ **done** (string formals + dynamic NEW name · ticket_object W1–W3).
331. ~~**ASSERT/IF string `==` / `!=`**~~ **done** (content compare for LAST/str vars · no SYS EQS glue).
332. ~~**`CLONEOBJ` / `COPYOBJ`**~~ **done** (shallow object field copy · template/pool without SETF walk).
333. ~~**`RENAMEOBJ` / `MOVEOBJ`**~~ **done** (rename live object slot in place · promote temp names after CLONE).
334. ~~**`TRYSETF` / `SETF SOFT`**~~ **done** (soft field write · mirrors TRYGETF · string field names).
335. ~~**`TRYSEND` / `SEND SOFT`**~~ **done** (soft method dispatch · completes soft OOP triad · string method names).
336. ~~**`EACH OBJ`**~~ **done** (walk live objects with optional class filter · no LISTOBJS+EACH LINE glue).
337. ~~**`SENDALL` / `BROADCAST`**~~ **done** (method on every live object · optional class · fleet without EACH+SEND).
338. ~~**`DELETEALL` / `FREEALL`**~~ **done** (bulk free live objects · optional class · pool wipe without EACH+DELETEOBJ).
339. ~~**`GETFALL` / `COLLECTF`**~~ **done** (bulk field collect from live objs · optional class · AS KV · no EACH+GETF+PUSH).
340. ~~**`SETFALL` / `MAPSETF`**~~ **done** (bulk field write on live objs · optional class · dual of GETFALL · no EACH+SETF).
341. ~~**`WHEREOBJ` / `FINDOBJ`**~~ **done** (select live objs by field==value · bag or first · no EACH+GETF+IF).
342. ~~**`DELETEWHERE` / `FREEWHERE`**~~ **done** (free live objs by field==value · pool GC without EACH+GETF+IF+DELETEOBJ).
343. ~~**`SENDWHERE` / `INVOKEWHERE`**~~ **done** (method on field==value objs · WHERE+SENDALL one-shot · no EACH+GETF+IF+SEND).
344. ~~**`INCFALL` / `ADDFALL` / `DECFALL`**~~ **done** (bulk numeric field bump · default ±1 · no EACH+GETF+arith+SETF).
345. ~~**`MAXOBJ` / `MINOBJ`**~~ **done** (first live obj with max/min numeric field · no GETFALL+ARGMAX+NTH glue).
346. ~~**`SORTBYF` / `SORTOBJS`**~~ **done** (bag of names ordered by numeric field ASC|DESC · no GETFALL+ZIP+SORTN glue).
347. ~~**`SUMF` / `AVGF`**~~ **done** (sum/mean numeric field over live objs · no GETFALL+SYS SUM/AVG glue).
348. ~~**`WHEREGE` / `WHERELE` / `WHEREGT` / `WHERELT`**~~ **done** (threshold select live objs · bag · no EACH+GETF+IF).
349. ~~**`DELETEWHEREGE` / `DELETEWHERELE` / `DELETEWHEREGT` / `DELETEWHERELT`**~~ **done** (threshold free live objs · FREEBELOW/FREEABOVE · no EACH+GETF+IF+DELETEOBJ).
350. ~~**`SENDWHEREGE` / `SENDWHERELE` / `SENDWHEREGT` / `SENDWHERELT`**~~ **done** (method on threshold-matched objs · SENDBELOW/SENDABOVE · no EACH+GETF+IF+SEND).
351. ~~**`COUNTOBJ` / `COUNTWHERE` / `COUNTWHEREGE`…`**~~ **done** (fleet tallies · equality + threshold · no WHERE bag materialize).
352. ~~**`HASWHERE` / `HASWHEREGE`…`**~~ **done** (soft 0|1 existence · ANYWHERE/HASATLEAST · IF without COUNTWHERE compare).
353. ~~**`WHEREBETWEEN` / `COUNTBETWEEN` / `HASBETWEEN`**~~ **done** (closed lo..hi band select/count/probe · no dual WHEREGE+WHERELE glue).
354. ~~**`DELETEBETWEEN` / `FREEBETWEEN` / `FREEBAND`**~~ **done** (free live objs in closed field range · band GC without dual threshold free).
355. ~~**`SENDBETWEEN` / `SENDBAND` / `INVOKEBETWEEN`**~~ **done** (method on closed-range matched objs · completes BETWEEN triad).
356. ~~**`CLAMPFALL` / `CLAMPF` / `CLIPFALL`**~~ **done** (bulk clamp numeric field to [lo,hi] · bound energy/retries without EACH+SETF).
357. ~~**`MEDIANF` / `P50F` / `MIDF`**~~ **done** (integer median of fleet field · robust mid vs AVGF · no GETFALL+SYS MEDIAN).
358. ~~**`MULFALL` / `SCALEFALL` / `MULF`**~~ **done** (bulk multiply numeric field by factor · scale/zero/double without EACH+SETF).
359. ~~**`FREQF` / `HISTF` / `COUNTF`**~~ **done** (fleet field value histogram key:count · status rollups without GETFALL+FREQ).
360. ~~**`MODEF` / `TOPF` / `MODEFALL`**~~ **done** (most frequent fleet field value · dominant status without FREQF+TOPKEY).
361. ~~**`UNIQUF` / `DISTINCTF` / `UNIQUEFALL`**~~ **done** (unique fleet field values bag · what statuses exist without FREQF+KEYS).
362. ~~**`COPYF` / `COPYFALL` / `SNAPSHOTF`**~~ **done** (copy src→dst field on live objs · snapshot before tick without EACH+GETF+SETF).
363. ~~**`SWAPF` / `SWAPFALL` / `EXCHANGEF`**~~ **done** (exchange two fields on live objs · dual buffers without temp+EACH).
364. ~~**`SETFWHERE` / `WHERESETF` / `PUTWHERE`**~~ **done** (set field on objs matching field==value · no EACH+GETF+IF+SETF).
365. ~~**`INCFWHERE` / `ADDWHERE` / `DECFWHERE`**~~ **done** (bump numeric field on field==value objs · selective counters).
366. ~~**`GETFWHERE` / `COLLECTWHERE` / `PLUCKWHERE`**~~ **done** (bag of field values from field==value objs · filtered GETFALL).
367. ~~**`SUMWHERE` / `AVGWHERE` / `SUMIF`**~~ **done** (sum/mean numeric field on field==value objs · no GETFWHERE+SUM).
368. ~~**`MINWHERE` / `MAXWHERE` / `MINIF`**~~ **done** (min/max numeric field on field==value objs · no GETFWHERE+MIN/MAX).
369. ~~**`MEDIANWHERE` / `P50WHERE` / `MEDIANIF`**~~ **done** (median numeric field on field==value objs · no GETFWHERE+MEDIAN).
370. ~~**`FREQWHERE` / `HISTWHERE` / `FREQIF`**~~ **done** (field histogram on field==value objs · no GETFWHERE+FREQ).
371. ~~**`MODEWHERE` / `DOMINANTWHERE` / `MODEIF`**~~ **done** (most frequent field on field==value objs · no FREQWHERE+TOPKEY).
372. ~~**`UNIQUWHERE` / `DISTINCTWHERE` / `UNIQUIF`**~~ **done** (unique field values on field==value objs · no GETFWHERE+UNIQ).
373. ~~**`CLAMPWHERE` / `CLIPWHERE` / `CLAMPIF`**~~ **done** (clamp numeric field on field==value objs · no EACH+GETF+IF+SETF).
374. ~~**`MULWHERE` / `SCALEWHERE` / `MULIF`**~~ **done** (multiply numeric field on field==value objs · selective scale).
375. ~~**`COPYFWHERE` / `SNAPSHOTWHERE` / `COPYIF`**~~ **done** (copy src→dst field on field==value objs · selective snapshot).
376. ~~**`SWAPFWHERE` / `EXCHANGEWHERE` / `FLIPWHERE`**~~ **done** (swap two fields on field==value objs · selective dual-buffer; not stack SWAPIF).
377. ~~**`MAXOBJWHERE` / `MINOBJWHERE` / `BESTWHERE`**~~ **done** (extreme obj among field==value · no WHERE bag+MAXOBJ glue).
378. ~~**`SORTBYFWHERE` / `ORDERWHERE` / `RANKWHERE`**~~ **done** (rank matching objs by field ASC|DESC · no WHERE bag+SORTBYF).
379. ~~**`TOPNBYF` / `HEADBYF` / `TAKEBYF`**~~ **done** (top-n live objs by numeric field · no SORTBYF+TAKE glue).
380. ~~**`TOPNBYFWHERE` / `HEADWHERE` / `TAKEWHERE`**~~ **done** (top-n matching objs by field · no WHERE+TOPNBYF or SORTBYFWHERE+TAKE).
381. ~~**`NTHBYF` / `ATBYF` / `INDEXBYF`**~~ **done** (0-based rank pick live obj by field · no SORTBYF+NTH glue).
382. ~~**`NTHBYFWHERE` / `ATWHERE` / `INDEXWHERE`**~~ **done** (0-based rank pick among matches · no WHERE+NTHBYF or SORTBYFWHERE+NTH).
383. ~~**`SLICEBYF` / `MIDBYF` / `WINDOWBYF`**~~ **done** (ranked name window [start,count) · no SORTBYF+DROP+TAKE glue).
384. ~~**`PICKOBJ` / `RANDOBJ` / `SAMPLEOBJ`**~~ **done** (random live object sample · no LISTOBJS+SYS PICK glue).
385. ~~**`DRAWNOBJ` / `SAMPLEKOBJ` / `PICKNOBJ`**~~ **done** (sample k unique live objs · no LISTOBJS+SYS DRAWN glue).
386. ~~**`SHUFFLEOBJS` / `SHUFOBJS` / `PERMUTEOBJS`**~~ **done** (shuffle live object names · no LISTOBJS+SYS SHUFFLE glue).
387. ~~**`CLASSINFO` / `DUMPCLASS` / `DESCRIBECLASS`**~~ **done** (class schema plate fields/methods/live · no LISTFIELDS+LISTMETHODS+count glue).
388. ~~**`METHODINFO` / `DUMPMETHOD` / `DESCRIBEMETHOD`**~~ **done** (method arity/params plate · no HASMETHOD-only guess for SEND).
389. ~~**`FNINFO` / `DUMPFN` / `DESCRIBEFN`**~~ **done** (FN arity/params plate · no guess of formals for CALL · twin of METHODINFO).
390. ~~**`LISTFNS` / `HASFN`**~~ **done** (FN catalog bag + soft probe · twin of LISTMETHODS/HASMETHOD · no CALL guess).
391. ~~**`TRYCALL` / `CALL SOFT`**~~ **done** (soft FN dispatch · OK=0 sticky LAST_ERR · twin of TRYSEND).
392. ~~**`FIELDINFO` / `DUMPFIELD` / `DESCRIBEFIELD`**~~ **done** (field kind/default plate · no HASFIELD-only guess for SETF/NEW).
393. ~~**`REQUIRE FN` / `CLASS` / `METHOD`**~~ **done** (fail-fast language-plane gates after INCLUDE · twin of HASFN soft).
394. ~~**`OBJINFO` / `DESCRIBEOBJ`**~~ **done** (live object plate name/class/methods+fields · no DUMPOBJ+CLASSINFO+LISTMETHODS glue).
395. ~~**`INCF` / `DECF` / `ADDF` / `BUMPF`**~~ **done** (single-object numeric field bump · no GETF+arith+SETF · METHOD bodies).
396. ~~**`BOUNDF` / `LIMITF` / `CLIPOBJ`**~~ **done** (single-object numeric field clamp · no GETF+IF+SETF · METHOD/THIS; CLAMPF stays fleet).
397. ~~**`TIMESF` / `SCALEOBJ` / `MULOBJ`**~~ **done** (single-object numeric field multiply · no GETF+*+SETF · METHOD/THIS; MULF stays fleet).
398. ~~**`CATF` / `APPENDF` / `PREPENDF`**~~ **done** (single-object string field concat · no GETF+CAT+SETF · METHOD/THIS · dual of INCF).
399. ~~**`CATFALL` / `APPENDFALL` / `PREPENDFALL`**~~ **done** (fleet string field concat · no EACH+GETF+CAT+SETF · dual of INCFALL / fleet of CATF).
400. ~~**`REPLACEF` / `GSUBF` / `SUBSTF`**~~ **done** (single-object string field replace · no GETF+REPLACEALL+SETF · METHOD/THIS · complements CATF).
401. ~~**`TRIMF` / `UPPERF` / `LOWERF`**~~ **done** (single-object string field normalize · no GETF+TRIM/UPPER/LOWER+SETF · METHOD/THIS · pairs REPLACEF/CATF).
402. ~~**`LENF` / `EMPTYF` / `BLANKF` / `NONEMPTYF`**~~ **done** (field probes · no GETF+SYS LEN/EMPTY · IF guards · no mutate · METHOD/THIS).
403. ~~**`HASINF` / `STARTSF` / `ENDSF`**~~ **done** (field contains/prefix/suffix probes · no GETF+SYS HAS/STARTS/ENDS · IF guards · HASIFI/STARTSIF/ENDSIF).
404. ~~**`CLEARF` / `RESETF` / `DEFAULTF`**~~ **done** (wipe field · restore CLASS default · set-if-empty · no GETF+IF+SETF · METHOD/THIS).
405. ~~**`EQF` / `NEQF` / `EQFI` / `FINDF`**~~ **done** (exact field==value + first-index locate · no GETF+SYS EQS/FIND · IF guards · complements HASINF).
406. ~~**`LEFTF` / `RIGHTF` / `SLICEF` / `TRUNCF`**~~ **done** (in-place field string slice/clip · no GETF+LEFT/MID+SETF · MIDSTRF not fleet MIDF · METHOD/THIS).
407. ~~**`BEFOREF` / `AFTERF` / `BETWEENF`**~~ **done** (delimiter field peels · no GETF+SYS BEFORE/AFTER/BETWEEN+SETF · kv/log · METHOD/THIS).
408. ~~**`NUMF` / `STRF`**~~ **done** (field type coerce · strtol / itoa in place · INCF after peel · CATF after count · METHOD/THIS).
409. ~~**`DIVF` / `MODF`**~~ **done** (single-object integer divide / remainder · no GETF+`/`+SETF · share/wrap counters · METHOD/THIS · pairs INCF/TIMESF/NUMF).
410. ~~**`ABSF` / `NEGF` / `SIGNF`**~~ **done** (single-object abs/negate/signum · no GETF+SYS IABS/SIGN+SETF · polarity after delta · METHOD/THIS).
411. ~~**`LPADF` / `RPADF`**~~ **done** (left/right pad one string field to width · promotes num · no GETF+SYS LPAD/RPAD+SETF · plate IDs · METHOD/THIS).
412. ~~**`MINF` / `MAXF`**~~ **done** (one-sided field min/max bound · cap/floor without full BOUNDF lo..hi · no GETF+IF+SETF · METHOD/THIS).
413. ~~**`COUNTINF` / `COUNTINFI`**~~ **done** (count needle hits in one field · no GETF+loop · path/delimiter depth · complements HASINF/FINDF · METHOD/THIS).
414. ~~**`SPLITF` / `FIELDSF`**~~ **done** (split field on sep → newline bag · no GETF+SYS SPLIT · path/CSV EACH LINE · no mutate · METHOD/THIS).
415. ~~**`JOINF` / `JOINLINESF`**~~ **done** (join newline bag into field with sep · inverse SPLITF · no GETF+SYS JOINLINES+SETF · METHOD/THIS).
416. ~~**`STRIPPREFIXF` / `STRIPSUFFIXF`**~~ **done** (drop leading/trailing affix on one field · path roots / extensions · no GETF+STARTS+SLICEF · METHOD/THIS).
417. ~~**`WORDSF` / `TOKENIZEF`**~~ **done** (whitespace tokenize field → bag · no GETF+SYS WORDS · free-text EACH LINE · no mutate · METHOD/THIS).
418. ~~**`CUTF` / `COLUMNF`**~~ **done** (Nth sep column peel into field · 0-based CUTF / 1-based COLUMNF · no GETF+SYS CUT+SETF · METHOD/THIS).
419. ~~**`LINEF` / `LINENOF`**~~ **done** (Nth newline bag line peel into field · 0-based LINEF / 1-based LINENOF · no GETF+SYS NTH+SETF · METHOD/THIS · pairs SPLITF/WORDSF).
420. ~~**`PUSHF` / `POPF` / `POPHEADF` / `UNSHIFTF`**~~ **done** (bag queue on field · LIFO/FIFO worklist without GETF+SYS PUSH/POP+SETF · METHOD/THIS · not string PREPENDF).
421. ~~**`GREPF` / `GREPVF` / `GREPFI`**~~ **done** (filter bag lines in field by needle · invert/icase · no GETF+SYS GREP+SETF · METHOD/THIS · log triage).
422. ~~**`SORTBAGF` / `UNIQBAGF` / `UNIQUEBAGF`**~~ **done** (lex sort + adjacent/full bag dedupe on field · worklist hygiene · not fleet UNIQUF/DISTINCTF · METHOD/THIS).
423. ~~**`LINESF` / `HASBAGLINE` / `DROPBAGLINE`**~~ **done** (bag count + exact membership + ack-drop on field · work queue IF/ack · not file HASLINEF/REMOVELINEF · METHOD/THIS).
424. ~~**`EACH LINE OF obj field`**~~ **done** (walk bag-in-field without GETF first · METHOD/THIS · pairs PUSHF/LINESF/HASBAGLINE).
425. ~~**`ENSUREBAGLINE` / `BAGENSURE`**~~ **done** (idempotent append exact bag line if missing · flags/work sets · no HAS+PUSH IF · not file ENSURELINE · METHOD/THIS).
426. ~~**`EACH PROP` / `ATTR` / `OBJFIELD OF obj|Class`**~~ **done** (walk schema field names · no LISTFIELDS+EACH LINE glue · not bag EACH FIELD · METHOD/THIS).
427. ~~**`EACH METHOD` / `MSG` / `HANDLER OF obj|Class`**~~ **done** (walk schema method names · no LISTMETHODS+EACH LINE glue · SEND prep · METHOD/THIS · twin EACH PROP).
428. ~~**unknown form did-you-mean**~~ **done** (typo recovery via edit/prefix · HELP/cubalc forms tip · CLI forms catalog gains core OOP CLASS/NEW/SEND/GETF/SETF).
429. ~~**OOP/FN name did-you-mean**~~ **done** (SEND/GETF/SETF/NEW/CALL typo → closest class/obj/method/field/FN · soft LAST_ERR + fatal).
430. ~~**SYS/INCLUDE name did-you-mean**~~ **done** (unknown SYS op → closest host form · INCLUDE lib typo → programs/lib stem · no catalog soup).
431. ~~**ASSERT/EXPECT got/expected**~~ **done** (cmp fail shows `3 == 1 is false` · ASSERT_GOT/EXPECTED/OP knobs · soft EXPECT too).
432. ~~**CALL/SEND arity check**~~ **done** (under-arity → needs N got M · params list · ARITY_NEED/GOT · soft TRY* · clears unbound formals).
433. ~~**optional THEN / LET =**~~ **done** (IF/UNLESS/ELIF body without THEN · LET name value without = · clearer LET syntax errs with line).
434. ~~**PRINT multi-arg strings**~~ **done** (PRINT/SAY/ECHO space-join str+num · string vars by content not strlen · LAST/PRINT_N · empty PRINT re-echoes LAST).
435. ~~**SYS ARGC / ARGS + run args**~~ **done** (ARGC count · ARGS newline bag · cubalc run file| -e … [--] args → CUBALC_ARGn/ARGC).
436. ~~**REQUIRE ARG / ARGC**~~ **done** (fail-fast CLI contract · ARG n|name · ARGC [min] · line+tip · pairs SYS ARGS/run args).
437. ~~**incomplete form tips + line**~~ **done** (NEW/SEND/GETF/SETF/CALL/CLASS/FN/CUBE/PLUG/INCLUDE/DEFAULT fail_at with example + line · DEFAULT optional =).
438. ~~**HASARG / HASARGC**~~ **done** (soft 0|1 CLI arg probes · twin REQUIRE ARG/ARGC · HASARG_N · IF LAST_N branch).
439. ~~**`HASFLAG / GETFLAG`**~~ **done** (soft CLI `--name` / `--name=val` / `--name val` probes · no shell getopt · OR fallback).
440. ~~**`RESTARGS / POSITIONALS`**~~ **done** (non-flag CUBALC_ARGn bag · files after --flags · LAST_N=count · bare `--` ends flags).
441. ~~**`REQUIRE FLAG`**~~ **done** (fail-fast `--name` contract · LAST=value · twin HASFLAG / REQUIRE ARG · tip pass --name).
442. ~~**`USAGE`**~~ **done** (sticky CLI usage string · bare re-echo · REQUIRE ARG/ARGC/FLAG fails append `· usage:` tip).
443. ~~**`REQUIRE RESTARGS`**~~ **done** (fail-fast min non-flag positionals · LAST=bag · flags do not inflate count · USAGE tip).
444. ~~**`HASRESTARGS / HASPOS`**~~ **done** (soft 0|1 min non-flag count · twin REQUIRE RESTARGS / HASARGC · RESTARGS_N have).
445. ~~**`LISTFLAGS / FLAGS`**~~ **done** (bag of flag names no dashes · LAST_N=count · discover --flags without EACH ARGS).
446. ~~**`NTHPOS / POSN`**~~ **done** (0-based non-flag positional peel · OR fallback · no RESTARGS+NTH · index avoids parse_expr OR).
447. ~~**`FLAGMAP / FLAGKV`**~~ **done** (name=value bag for every --flag · bare → 1 · LOOKUP sep `=` · coexists with RESTARGS files).
448. ~~**`CASE string arms`**~~ **done** (CASE/MATCH/SWITCH string selector + WHEN "s" · CLI action dispatch · numeric TO range kept).
449. ~~**`WHEN multi-alias`**~~ **done** (WHEN a, b OR c | d · CLI synonyms · MATCH_ARM/WHEN_HIT · numeric multi kept · TO range unchanged).
450. ~~**`CASEI` / `MATCHI`**~~ **done** (case-insensitive string CASE · CASE ICASE · CLI mixed-case GETFLAG · multi-alias kept).
451. ~~**`BOOLFLAG` / `TRUTHY` / `FALSY`**~~ **done** (CLI flag truthy LAST_N · false/0/off · IF without GETFLAG+EQS · OR default).
452. ~~**`GETFLAGN` / `FLAGN`**~~ **done** (CLI flag as int LAST_N · OR default · ports/retries without GETFLAG+NUM).
453. ~~**`ONEOF` / `REQUIRE ONEOF`**~~ **done** (soft/fatal membership in multi-alias set · ONEOFI icase · GETFLAG action gate).
454. ~~**`SUBCMD` / `COMMAND`**~~ **done** (first positional → LAST · RESTPOS remaining files · CASEI/ONEOF CLI tools).
455. ~~**`REQUIRE BETWEEN` / `INRANGE`**~~ **done** (numeric inclusive range gate · GETFLAGN ports/retries · soft twin).
456. ~~**`GETFLAGMS` / `FLAGMS`**~~ **done** (CLI duration flag to ms via PARSEMS 5s/1m · timeout OR default).
457. ~~**`REQUIRE RESTPOS` / `HASRESTPOS`**~~ **done** (files after subcommand min gate · not counting cmd · SUBCMD twin).
458. ~~**`CLAMPN` / `BOUNDN`**~~ **done** (clamp x into [lo,hi] → LAST_N · GETFLAGN/GETFLAGMS caps without SYS CLAMP).
459. ~~**flag aliases**~~ **done** (`GETFLAGN workers|w` · `,`/`|` multi-name · `FLAG_HIT_NAME` · short flags without dual HASFLAG).
460. ~~**`HELPFLAG` / `AUTOHELP`**~~ **done** (if --help|-h print sticky USAGE + EXIT 0 · default help|h|usage · no HASFLAG+IF+EXIT glue).
461. ~~**`VERSIONFLAG` / `VERFLAG`**~~ **done** (if --version|-V print VERSION/PROG_VERSION + EXIT 0 · default version|V|ver · twin of HELPFLAG).
462. ~~**`GETFLAGALL` / `MULTIFLAG`**~~ **done** (all --name occurrences → newline bag · LAST_N=count · tags without EACH ARGS).
463. ~~**`GETFLAGN OR ENV`**~~ **done** (`OR ENV NAME` chains · CLI > env > literal · GETFLAGN_SRC/FLAG_ENV · no shell `${VAR:-n}`).
464. ~~**flag family OR ENV**~~ **done** (GETFLAG/GETFLAGMS/BOOLFLAG OR ENV · same CLI>env>default · *_SRC/FLAG_ENV).
465. ~~**`GETFLAGCSV` / `FLAGCSV`**~~ **done** (multi --name + comma-split → bag · OR ENV · tag lists without SPLIT glue).
466. ~~**`FLOORN` / `CAPN`**~~ **done** (one-sided floor/cap → LAST_N · GETFLAGN bounds without full CLAMPN; not stack MINN/MAXN).
467. ~~**`DIVCEILN` / `PAGESN`**~~ **done** (ceil(a/b) → LAST_N · page/chunk counts after GETFLAGN without shell).
468. ~~**`GETFLAGPATH` / `FLAGPATH`**~~ **done** (path flag peel + ABSPATH · EXIST · OR ENV · no GETFLAG+REALPATH glue).
469. ~~**`REQUIRE FLAGPATH` / `FLAGFILE` / `FLAGDIR`**~~ **done** (fail-fast path flag · ABSPATH + exist/kind · twin of GETFLAGPATH).
470. ~~**`GETFLAGDIR` / `MKFLAGDIR`**~~ **done** (path flag peel + ABSPATH + mkdir -p · --out dirs without MKDIR glue).
471. ~~**`RESTPATHS` / `POSPATHS`**~~ **done** (positional files → absolute path bag · EXIST count · no EACH+REALPATH).
472. ~~**`NTHPOSPATH` / `POSNPATH`**~~ **done** (Nth positional → absolute path · EXIST · OR fallback · no NTHPOS+REALPATH).
473. ~~**`ENSUREPARENT` / `MKPARENT`**~~ **done** (dirname + mkdir -p · file WRITE prep after GETFLAGPATH · no DIRNAME+MKDIR glue).
474. ~~**`GETFLAGOUT` / `FLAGOUT`**~~ **done** (path flag peel + ABSPATH + mkdir parent · LAST=file ready for WRITE).
475. ~~**`RESTPOSPATHS` / `TAILPATHS`**~~ **done** (files after subcommand → abs path bag · EXIST_N · no RESTPOS+EACH+REALPATH).
476. ~~**`WRITEOUT` / `PUTOUT`**~~ **done** (ENSUREPARENT + WRITE one-shot · LAST=abs file · plate dumps without DIRNAME glue).
477. ~~**`APPENDOUT` / `LOGOUT`**~~ **done** (ENSUREPARENT + APPEND line · nested agent logs without DIRNAME+APPEND glue).
478. ~~**`WRITEOUTATOMIC` / `SAFEWRITEOUT`**~~ **done** (ENSUREPARENT + temp+rename write · multi-agent nested plates).
479. ~~**`COPYOUT` / `CPOUT`**~~ **done** (ENSUREPARENT dest + COPY · nested plate snapshot without DIRNAME+COPY glue).
480. ~~**`MOVEOUT` / `MVOUT`**~~ **done** (ENSUREPARENT dest + RENAME · promote temp plates into nested paths without DIRNAME+MV glue).
481. ~~**`TOUCHOUT` / `MKFILEOUT`**~~ **done** (ENSUREPARENT + TOUCH · nested lease/marker stamps without DIRNAME+TOUCH glue).
482. ~~**`WRITEOUTIF` / `PUTIFCHANGED`**~~ **done** (write nested plate only if content differs · mtime-stable for watchers · atomic when writing).
483. ~~**`LOGSTAMP` / `STAMPLOG`**~~ **done** (ENSUREPARENT + append `ISO msg` line · timestamped agent logs without DATE+CAT+APPENDOUT glue).
484. ~~**`LINKOUT` / `LNOUT`**~~ **done** (ENSUREPARENT link + SYMLINK · nested plate path aliases without DIRNAME+LN glue).
485. ~~**`HARDLINKOUT` / `HLOUT`**~~ **done** (ENSUREPARENT new + HARDLINK · nested same-inode plate shares without DIRNAME+HLINK glue).
486. ~~**`RELINKOUT` / `RELINK`**~~ **done** (ENSUREPARENT; RM if exists; SYMLINK · update nested plate aliases without RM+LINKOUT IF glue).
487. ~~**`TEEOUT` / `TEEWOUT`**~~ **done** (ENSUREPARENT + write keep body in LAST · nested plate write then GREP/HAS without READ back).
488. ~~**`CLAIMOUT` / `CLAIMNEST`**~~ **done** (ENSUREPARENT + exclusive create · nested work-item claims without DIRNAME+CLAIM glue).
489. ~~**`RMTREE` / `RMNEST` / `RMRF`**~~ **done** (recursive file/dir tree remove · clean nested MKTEMPDIR/CLAIMOUT workspaces without shell rm -rf).
490. ~~**`CPTREE` / `COPYTREE` / `CPDIR`**~~ **done** (recursive file/dir tree copy · snapshot nested workspaces without shell cp -r).
491. ~~**`MVTREE` / `MOVETREE` / `MVDIR`**~~ **done** (recursive tree move · rename or CPTREE+RMTREE · promote nested workspaces without shell mv).
492. ~~**`DIRSIZE` / `DU` / `TREESIZE`**~~ **done** (recursive path byte inventory · DIRSIZE_FILES/DIRS · agent disk gate without shell du).
493. ~~**`EQTREE` / `SAMETREE` / `CMPTREE`**~~ **done** (recursive structure+content equality · verify CPTREE without shell diff -r).
494. ~~**`GREPTREE` / `SEARCHTREE` / `RGREP`**~~ **done** (recursive content search under root · path bag · no WALK+GREPFILES or shell grep -r).
495. ~~**`REPLACETREE` / `GSUBTREE` / `SEDTREE`**~~ **done** (recursive REPLACEALL under root · bulk nested rewrite without WALK+REPLACEFILES or shell find|sed).
496. ~~**`COUNTINTREE` / `COUNTTREE` / `GREPCOUNTTREE`**~~ **done** (recursive needle occurrence tally · COUNTINTREE_FILES · no WALK+COUNTINFILES or shell grep -rc).
497. ~~**`GREPLINESTREE` / `HARVESTTREE` / `EXTRACTLINETREE`**~~ **done** (recursive matching-line harvest · line bag · no WALK+GREPLINES or shell grep -rn).
498. ~~**`FIRSTINTREE` / `LASTINTREE` / `GREP1TREE`**~~ **done** (first/last content-matching file under tree · no GREPTREE+TAKE/REVL glue).
499. ~~**`FIRSTLINEINTREE` / `LASTLINEINTREE` / `GREP1LINETREE`**~~ **done** (first/last matching line under tree · PATH+HIT+I · no GREPLINESTREE+TAKE/REVL glue).
500. ~~**`PATHLINETREE` / `GREPNTREE` / `RGLINES`**~~ **done** (recursive path:lineno:line bag · agent grep -rn · no WALK+EACH+READ).
501. ~~**`PARSEPATHLINE` / `SPLITPATHLINE` / `PEELPATHLINE`**~~ **done** (peel path:lineno:text → PATH · LINE_N · LAST=text · PATHLINETREE companion).
502. ~~**`PATHLINEPATHS` / `PATHSFROMPL` / `UNIQPATHLINES`**~~ **done** (unique paths from path:lineno:text bag · no EACH+PARSEPATHLINE+HASLINE+PUSH).
503. ~~**`PATHLINETEXTS` / `TEXTSFROMPL` / `PLTEXTS`**~~ **done** (message texts from path:lineno:text bag · FREQ-ready · no EACH+PARSEPATHLINE+PUSH).
504. ~~**`PATHLINEFREQ` / `COUNTBYPATH` / `PLFREQ`**~~ **done** (path:count hist from pathline bag · TOPKEY/SORTFREQ · no EACH+PARSEPATHLINE+KVINC).
505. ~~**`FORMATPATHLINE` / `MKPATHLINE` / `TOPATHLINE`**~~ **done** (build path:lineno:text cite · inverse of PARSEPATHLINE · no STR+CAT glue).
506. ~~**`TAKEINTREE` / `FIRSTNINTREE` / `HEADINTREE`**~~ **done** (first n content-matching paths under tree · no GREPTREE+TAKE glue).
507. ~~**`NTHINTREE` / `INDEXINTREE`**~~ **done** (0-based Nth content-matching path under tree · early-stop · no GREPTREE+NTH glue).
508. ~~**`HASINTREE` / `ANYINTREE` / `CONTAINSINTREE`**~~ **done** (soft 0|1 content exists under tree · IF guard · LAST string kept · PATH on hit).
509. ~~**`SLICEINTREE` / `MIDINTREE` / `WINDOWINTREE`**~~ **done** (path window [start,count) of content matches under tree · page without GREPTREE+DROP+TAKE).
510. ~~**`DROPINTREE` / `SKIPINTREE` / `RESTINTREE`**~~ **done** (skip first n content matches under tree · residual bag · dual of TAKEINTREE · no GREPTREE+DROP).
511. ~~**`JSONKEYS` / `JSONHAS` / `JSON … OR`**~~ **done** (top-level key bag + soft key probe + peel default · agent plates without guess/IF glue).
512. ~~**`JSON key FROM plate`**~~ **done** (peel field from explicit plate · multi-field without JOIN/LAST reload · FROM|IN|OF + OR).
513. ~~**`JSONSET` / `JSET` / `JSONPUT`**~~ **done** (set/update top-level JSON plate key · agent write without hand rebuild · 2/3-arg).
514. ~~**`JSONOBJ` / `MKJSON` / `JOBJ`**~~ **done** (one-shot multi-pair object plate · optional FROM base · no chained JSONSET).
515. ~~**`JSONDEL` / `JDEL` / `JSONRM`**~~ **done** (drop top-level JSON plate key · soft miss · agent field ack without hand rebuild).
516. ~~**`JSONN` / `JGETN` / `JSONNUM`**~~ **done** (peel JSON key as int LAST_N · FROM+OR · plate IF/arith without JSON+NUM glue).
517. ~~**`JSONMERGE` / `MERGEJSON` / `JMERGE`**~~ **done** (overlay keys onto base plate · raw values · no EACH+JSONSET glue).
518. ~~**`JSONINC` / `JINC` / `JSONDEC`**~~ **done** (bump numeric plate key · default ±1 · no JSONN+arith+JSONSET glue).
519. ~~**`JSONENSURE` / `JDEFAULT` / `JSONDEFAULT`**~~ **done** (set plate key only if missing · agent boot defaults · no clobber).
520. ~~**`JSONPICK` / `JSONKEEP` / `JKEEP`**~~ **done** (keep listed plate keys · raw values · strip noise for peer/log).
521. ~~**`JSONRENAME` / `JRENAME` / `JSONMV`**~~ **done** (rename plate key · raw value kept · promote tmp→status).
522. ~~**`JSONDROP` / `JSONOMIT` / `JDROPK`**~~ **done** (drop listed plate keys · bulk JSONDEL · strip known noise).
523. ~~**`JSONTOGGLE` / `JTOGGLE` / `FLIPJSON`**~~ **done** (flip plate 0↔1 flag · miss→1 · no JSONN+IF+JSONSET).
524. ~~**`JSONTYPE` / `JTYPE` / `JSONKIND`**~~ **done** (plate value kind probe · num|str|bool|null|obj|arr|missing).
525. ~~**`JSONCOPY` / `JCOPY` / `JSONDUP`**~~ **done** (copy plate key raw value · snapshot before mutate).
526. ~~**`JSONSWAP` / `JSWAP` / `JSONXCHG`**~~ **done** (exchange two plate keys · dual-buffer · move if one miss).
527. ~~**`JSONVALUES` / `JVALS` / `JSONVALS`**~~ **done** (top-level values bag · twin of JSONKEYS · ZIP/SUM/FREQ).
528. ~~**`JSONFROMKV` / `JFROMKV` / `KVTOJSON`**~~ **done** (key=val bag → plate · FREQ/FLAGMAP without EACH+JSONSET).
529. ~~**`JSONTOKV` / `JTOKV` / `JSONTOBAG`**~~ **done** (plate → key:val bag · dual of JSONFROMKV · LOOKUP/FREQ default sep · no ZIP keys+vals).
530. ~~**`JSONLEN` / `JLEN` / `JSONCOUNT`**~~ **done** (top-level key count → LAST_N · soft 0 · empty/size IF without JSONKEYS bag).
531. ~~**`JSONEMPTY` / `JEMPTY` · `JSONNONEMPTY` / `HASKEYS`**~~ **done** (soft empty/nonempty plate probe · IF without JSONLEN+CMP).
532. ~~**`JSONSUMN` / `JSONSUM` / `JSUMN`**~~ **done** (sum integer top-level plate values → LAST_N · no JSONVALUES+SUM glue).
533. ~~**`JSONTOPKEY` / `JTOPKEY` · `JSONBOTKEY` / `JBOTKEY`**~~ **done** (max/min numeric plate key · LAST=key LAST_N=v · no TOKV+TOPKEY).
534. ~~**`JSONMAXN` / `JMAXN` · `JSONMINN` / `JMINN`**~~ **done** (max/min integer top-level value → LAST_N · no JSONVALUES+MIN/MAX).
535. ~~**`JSONAVGN` / `JAVGN` / `JSONMEAN`**~~ **done** (integer mean of top-level ints → LAST_N · USED count · no JSONVALUES+AVG).
536. ~~**`JSONEQ` / `JEQ` · `JSONNEQ` / `JNEQ`**~~ **done** (order-independent plate equality · verify WRITE/MERGE without fragile EQS).
537. ~~**`JSONSUBSET` / `JSUBSET` · `JSONSUPERSET` / `JSONCOVERS`**~~ **done** (required-field plate match · sub keys+values in super · no multi JSONHAS).
538. ~~**`JSONHASALL` / `JSONNEED` · `JSONHASANY` / `JHASANY`**~~ **done** (multi-key presence → LAST_N · HIT count · contract keys without multi JSONHAS).
539. ~~**`JSONMISS` / `JSONMISSING` · `JSONPRESENT` / `JSONFOUND`**~~ **done** (bag of missing/present required keys · after HASALL fail name which · no multi JSONHAS).
540. ~~**`JSONEXTRA` / `JSONUNKNOWN` · `JSONKNOWN` / `JSONALLOWED`**~~ **done** (plate keys outside/inside allow-list bag · schema unknown-field reject without JSONKEYS+EACH).
541. ~~**`JSONPLUCK` / `JSONGETALL` / `JPLUCK`**~~ **done** (multi-key peel → value bag · empty field on miss · HIT=found · no multi JSON+PUSH).
542. ~~**`REQUIRE JSONHASALL` / `JSONNEED` / `JSONKEYS`**~~ **done** (fail-fast plate key contract · missing listed in error · soft twin SYS JSONHASALL).
543. ~~**`REQUIRE JSONONLY` / `NOEXTRA` / `JSONSTRICT`**~~ **done** (fail-fast if plate has keys outside allow-list · extras listed · soft twin SYS JSONEXTRA).
544. ~~**`JSONEXACT` / `JSONSCHEMA` · `REQUIRE JSONEXACT`**~~ **done** (exact key-set HASALL+ONLY one-shot · soft LAST_N + fail-fast · miss/extra listed).
545. ~~**`JSONKEYDIFF` / `KEYSDIFF` · `JSONKEYCOMM` / `KEYSINTER`**~~ **done** (plate key-set diff/intersection bags · no JSONKEYS+DIFF/INTERSECT glue).
546. ~~**`JSONCHANGED` / `JSONNEQKEYS` · `JSONSTABLE` / `JSONEQKEYS`**~~ **done** (plate keys that differ or stay same by raw value · sync delta without multi peels).
547. ~~**`JSONDELTA` / `JDELTA` / `DELTAOBJ`**~~ **done** (changed keys as plate object · values from NEW/OLD · sync payload without CHANGED+EACH+JSONSET).
548. ~~**`REQUIRE JSONEQ` / `SAMEJSON`**~~ **done** (fail-fast plate equality · lists changed keys in error · soft twin SYS JSONEQ).
549. ~~**`REQUIRE JSONSUBSET` / `JSONSUPERSET` / `JSONCOVERS`**~~ **done** (fail-fast required-field plate match · bad keys listed · soft twin SYS JSONSUBSET).
550. ~~**`REQUIRE JSONNEQ` / `JNEQ` / `DIFFJSON`**~~ **done** (fail-fast if plates still equal · mutation/write verify · soft twin SYS JSONNEQ · surfaces JSONCHANGED).
551. ~~**`JSONDEFAULTS` / `JSONENSUREALL` / `JDEFAULTS`**~~ **done** (multi-key fill missing from defaults plate · no clobber · dual of JSONMERGE for agent boot).
552. ~~**`JSONCHANGELOG` / `JCHANGELOG` / `PLATECHANGELOG`**~~ **done** (readable `key: old → new` bag · agent/log plate sync · complements CHANGED names + DELTA values).
553. ~~**`REQUIRE JSONTYPE` / `JTYPE` / `JSONKIND`**~~ **done** (fail-fast plate field type contract · got/want in error · soft twin SYS JSONTYPE).
554. ~~**`LOADPLATE` / `READPLATE` / `PLATEREAD`**~~ **done** (soft plate-file load · miss/non-object → OR defaults · LAST_N hit · agent state boot).
555. ~~**`SAVEPLATE` / `STOREPLATE` / `PLATESAVE`**~~ **done** (object-checked plate persist · mkdir parent · dual of LOADPLATE for agent state).
556. ~~**`ENSUREPLATE` / `TOUCHPLATE` / `SEEDPLATE`**~~ **done** (create-or-load state plate · LAST=content · CREATED 0|1 · first-run seed without IF glue).
557. ~~**`REQUIRE PLATEFILE` / `JSONPLATE` / `HASPLATE`**~~ **done** (fail-fast object plate file · LAST=content · soft twin SYS LOADPLATE · note REQUIRE PLATE stays REG-file).
558. ~~**`programs/lib/plate_boot`**~~ **done** (INCLUDE plate_boot = ENSUREPLATE seed + PLATE var · DEFAULT path/seed · agent state preamble).
559. ~~**`UPDATEPLATE` / `JSONFILESET` / `PLATEKEYSET`**~~ **done** (one-shot load-set-save JSON plate field · no LOAD+JSONSET+SAVE glue).
560. ~~**`BUMPPLATE` / `JSONFILEINC` / `DECRPLATE`**~~ **done** (one-shot load-inc/dec-save plate counter · no LOAD+JSONINC+SAVE glue).
561. ~~**`PEEKPLATE` / `JSONFILEGET` / `PLATEKEYGET`**~~ **done** (one-shot peel field from plate file · OR fallback · no LOAD+JSON glue).
562. ~~**`DROPPLATE` / `JSONFILEDEL` / `PLATEKEYDEL`**~~ **done** (one-shot load-del-save plate field · soft miss · no LOAD+JSONDEL+SAVE glue).
563. ~~**`JSONFILEMERGE` / `OVERLAYPLATE` / `MERGEPLATEFILE`**~~ **done** (multi-key load-merge-save plate · overlay wins · not in-memory MERGEPLATE).
564. ~~**`programs/lib/plate_tick`**~~ **done** (INCLUDE plate_tick = ENSURE + bump + ts stamp + SAVE · agent tick preamble).
565. ~~**`JSONFILEENSURE` / `ENSUREPLATEKEY` / `DEFAULTPLATEKEY`**~~ **done** (set plate-file key only if missing · seed defaults without clobber).
566. ~~**`JSONFILEDEFAULTS` / `DEFAULTPLATEALL` / `ENSUREPLATEALL`**~~ **done** (multi-key disk fill-if-missing · agent boot defaults without clobber).
567. ~~**`JSONFILETOGGLE` / `TOGGLEPLATEKEY` / `FLIPPLATEKEY`**~~ **done** (flip disk plate flag 0↔1 · agent feature flags without LOAD+TOGGLE+SAVE).
568. ~~**`programs/lib/plate_save`**~~ **done** (INCLUDE plate_save = SAVEPLATE PLATE_PATH PLATE · dual of plate_boot write-back).
569. ~~**`SETP` / `INCP` / `DELP` / `GETP`**~~ **done** (mutate conventional PLATE var in place · no JSONSET+LET glue · plate_boot/save workflow).
570. ~~**`MERGEP` / `DEFAULTP` / `TOGGLEP`**~~ **done** (multi-key overlay + set-if-missing + flag flip on PLATE · complete mutate surface without LET glue).
571. ~~**`cubalc plate` CLI**~~ **done** (show/get/set/inc/del/keys JSON agent plate files · no .cubalc for one-shots · cubalc.plate.v1).
572. ~~**`NEEDP` / `HASP` / `HASPALL` / `KEYSP`**~~ **done** (PLATE contract + soft probes + key bag · no REQUIRE JSONHASALL PLATE glue).
573. ~~**`CUBALC_VAR_STR_MAX` 4096**~~ **done** (program string vars hold real agent plates · no 512-byte PLATE truncate on SETP/SAVE).
574. ~~**`programs/lib/plate_patch`**~~ **done** (INCLUDE plate_patch = MERGEP/JSONFILEMERGE PLATE_PATCH + SAVE · multi-key update).
575. ~~**`DUMPP` / `PLATEINFO`**~~ **done** (cubalc.plate_info.v1 snapshot of PLATE · keys/bytes/path · no KEYSP+LEN glue).
576. ~~**`programs/lib/plate_session` + cookbook §8**~~ **done** (agent_boot+plate_boot one INCLUDE · COOKBOOK/FOR_AGENTS plate recipe).
577. ~~**`cubalc init --plate` + selftest plate proofs**~~ **done** (scaffold plate_session starter · selftest runs session/save/setp/dumpp).
578. ~~**`STREPEAT`/`LPAD`/`RPAD` wide**~~ **done** (host-buffer pad/repeat · multi-KB plate templates without 512 truncate).
579. ~~**`FILLP`/`SUBSTPLATE`/`EXPANDP`**~~ **done** (expand `{{key}}` from PLATE · agent templates without GETP+REPLACEALL).
580. ~~**`FILLPFILE`/`SUBSTPLATEFILE`**~~ **done** (materialize template file with `{{key}}` from PLATE · dual of SUBSTFILE).

## Tick protocol

1. `cd` to cubalc root; `git status` — restore thrash only if needed (`git restore` tracked sources).
2. Read this file + `docs/STRUCTURE.md` + current `CUBALC_LANG_VERSION`.
3. Pick **one** backlog item (or invent equally high-leverage usability).
4. Implement the smallest complete slice.
5. Add/adjust a **proof** under `programs/proof/` or a CLI smoke.
6. `make all` + targeted run + keep suite green if cheap; private `CUBALC_STATE` for runs.
7. Commit with a **human sentence** subject (not only opcodes), e.g.  
   `feat: ASSERT message strings for agent-readable failures`  
   Optional version bump only when the surface is user-visible.
8. `git push origin main` when green.
9. Write plate `state/MEANINGFUL_ITER.json` with `{iter, item, commit, summary}`.

## Success metric

After N ticks, a new user/agent can do something **new** without reading opcode soup — not “three more width duals.”
