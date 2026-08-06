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
