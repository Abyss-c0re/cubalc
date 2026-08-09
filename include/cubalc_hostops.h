/* CubalC host ops — C runtime under CubalC surface (Grokium = CubalC + C).
 * Allowlisted, fail-closed. HOLD_FLASH never flashes devices.
 */
#ifndef CUBALC_HOSTOPS_H
#define CUBALC_HOSTOPS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CUBALC_HOST_STR_MAX 8192
#define CUBALC_HOST_ERR_MAX 160

typedef struct cubalc_host_result {
  int ok;
  int code;          /* HTTP status or process exit */
  long n;            /* length / numeric */
  char str[CUBALC_HOST_STR_MAX];
  char err[CUBALC_HOST_ERR_MAX];
} cubalc_host_result;

/* file */
int cubalc_host_read(const char *path, cubalc_host_result *r);
int cubalc_host_write(const char *path, const char *data, cubalc_host_result *r);
int cubalc_host_exists(const char *path); /* 1/0 */
/* path kind: returns 0 missing, 1 regular file, 2 directory, 3 other; sets r->n size for files */
int cubalc_host_path_kind(const char *path, cubalc_host_result *r);
/* mtime: r->n = st_mtime epoch seconds; soft miss ok=0 code=0; exists ok=1 code=1|2|3 */
int cubalc_host_mtime(const char *path, cubalc_host_result *r);
/* atime: r->n = st_atime epoch seconds; soft miss ok=0 */
int cubalc_host_atime(const char *path, cubalc_host_result *r);
/* ctime: r->n = st_ctime epoch seconds (inode status change); soft miss ok=0 */
int cubalc_host_ctime(const char *path, cubalc_host_result *r);
/* mkdir -p style: create path and parents; OK if already a directory */
int cubalc_host_mkdir(const char *path, cubalc_host_result *r);
/* unlink regular file only; missing → ok with n=0; dir → error */
int cubalc_host_rm(const char *path, cubalc_host_result *r);
/* rmdir empty directory only; missing → ok with n=0; file/nonempty → error */
int cubalc_host_rmdir(const char *path, cubalc_host_result *r);
/* recursive tree remove (file or dir). missing → ok n=0; r->n = entries removed.
 * Does not follow directory symlinks (unlinks the link). Soft fail on errors. */
int cubalc_host_rmtree(const char *path, cubalc_host_result *r);
/* recursive tree copy src → dest. file → copy; dir → mkdir + children.
 * r->n = entries created/copied; r->code = bytes of file data. Soft miss src. */
int cubalc_host_cptree(const char *src, const char *dst, cubalc_host_result *r);
/* recursive tree move: rename if possible else cptree+rmtree.
 * r->n = entries (1 if rename); r->code = 1 renamed / 0 copy+delete. Soft miss src. */
int cubalc_host_mvtree(const char *src, const char *dst, cubalc_host_result *r);
/* recursive path size: r->n = total file bytes; r->code packs files|(dirs<<16).
 * File → own size; dir → sum children. Soft miss → error. Does not follow dir symlinks. */
int cubalc_host_dirsize(const char *path, cubalc_host_result *r);
/* recursive tree equality: structure + file content. r->n = 1 equal / 0 differ.
 * r->str = first relative path that differs ("" if equal). r->err = reason when differ.
 * Soft miss either root → return -1. Does not follow dir symlinks. */
int cubalc_host_eqtree(const char *a, const char *b, cubalc_host_result *r);
/* recursive content search under root. r->str = newline bag of matching full paths.
 * r->n = hit count; r->code = files scanned. icase/invert like GREPFILES.
 * Soft miss root → -1. Empty needle → all readable files. */
int cubalc_host_greptree(const char *root, const char *needle, int icase, int invert,
                         cubalc_host_result *r);
/* recursive REPLACEALL old→new under root. r->str = rewritten paths bag.
 * r->n = files rewritten; r->code = total substitutions. Soft miss root → -1.
 * Empty old → no-op (n=0). Only rewrites when content changes. */
int cubalc_host_replacetree(const char *root, const char *olds, const char *news,
                            cubalc_host_result *r);
/* recursive needle occurrence count under root. r->n = total hits;
 * r->code = files with ≥1 hit; r->str = abs root. Soft miss → -1.
 * Empty needle → 0. Non-overlapping matches (strstr advance). */
int cubalc_host_countintree(const char *root, const char *needle, int icase,
                            cubalc_host_result *r);
/* recursive matching-line harvest under root. r->str = newline bag of lines;
 * r->n = matching lines; r->code = files with hits. Soft miss → -1.
 * Empty needle → all lines. Case-insensitive when icase!=0. */
int cubalc_host_greplines_tree(const char *root, const char *needle, int icase,
                               cubalc_host_result *r);
/* recursive path:lineno:line harvest (grep -rn style, 1-based lineno).
 * r->str = bag; r->n = lines; r->code = files with hits. Soft miss → -1.
 * Empty needle → all lines annotated. Case-insensitive when icase!=0. */
int cubalc_host_pathlines_tree(const char *root, const char *needle, int icase,
                               cubalc_host_result *r);
/* first or last regular file under root whose content matches needle.
 * want_last=0 first (DFS), =1 last. r->str=path; r->n=1|0. Soft miss root → -1.
 * Empty needle → first/last readable regular file. */
int cubalc_host_findintree(const char *root, const char *needle, int icase,
                           int want_last, cubalc_host_result *r);
/* first maxn content-matching files under root (sorted DFS) → path bag.
 * r->str=paths; r->n=count (≤maxn); Soft miss root → -1. maxn≤0 → empty bag. */
int cubalc_host_takeintree(const char *root, const char *needle, int icase,
                           long maxn, cubalc_host_result *r);
/* 0-based Nth content-matching file under root (sorted DFS, early stop).
 * r->str=path; r->n=1|0. Soft miss root → -1. index<0 → miss. */
int cubalc_host_nthintree(const char *root, const char *needle, int icase,
                          long index, cubalc_host_result *r);
/* content-matching path window under root [start, start+count) sorted DFS.
 * r->str=path bag; r->n=kept. Soft miss root → -1. start/count≤0 → empty.
 * Early-stop once count paths kept. Usability: page without GREPTREE+DROP+TAKE. */
int cubalc_host_sliceintree(const char *root, const char *needle, int icase,
                            long start, long count, cubalc_host_result *r);
/* skip first skipn content matches under root → remaining path bag (sorted DFS).
 * Dual of takeintree. r->str=paths; r->n=kept. Soft miss root → -1. skipn≤0 → all. */
int cubalc_host_dropintree(const char *root, const char *needle, int icase,
                           long skipn, cubalc_host_result *r);
/* first or last matching *line* under root (sorted DFS).
 * want_last=0 first, =1 last. On hit: r->str = "path\\nline", r->n=1, r->code=0-based line idx.
 * Soft miss root → -1. Empty needle → first/last line of first/last readable file. */
int cubalc_host_findlineintree(const char *root, const char *needle, int icase,
                               int want_last, cubalc_host_result *r);
/* rename/move path; soft miss if from missing */
int cubalc_host_rename(const char *from, const char *to, cubalc_host_result *r);
/* copy regular file src → dst; r->n = bytes written */
int cubalc_host_copy(const char *src, const char *dst, cubalc_host_result *r);
/* create symlink linkpath → target; r->str = linkpath; soft fail if linkpath exists */
int cubalc_host_symlink(const char *target, const char *linkpath, cubalc_host_result *r);
/* create hard link newpath → existing (same inode); soft fail if new exists or src missing */
int cubalc_host_hardlink(const char *existing, const char *newpath, cubalc_host_result *r);
/* link count: r->n = st_nlink; soft miss if path missing */
int cubalc_host_nlink(const char *path, cubalc_host_result *r);
/* same inode+device: r->n = 1 if a and b are same file; soft miss either missing */
int cubalc_host_sameinode(const char *a, const char *b, cubalc_host_result *r);
/* inode number: r->n = st_ino; soft miss missing path */
int cubalc_host_inode(const char *path, cubalc_host_result *r);
/* owner uid: r->n = st_uid; soft miss missing path */
int cubalc_host_fileuid(const char *path, cubalc_host_result *r);
/* group gid: r->n = st_gid; soft miss missing path */
int cubalc_host_filegid(const char *path, cubalc_host_result *r);
/* access probe: mode is R_OK/W_OK/X_OK/F_OK; r->n = 1 if allowed, 0 if not; always ok if path non-empty */
int cubalc_host_can_access(const char *path, int mode, cubalc_host_result *r);
/* create/overwrite probe: existing file W_OK, else parent dir W_OK|X_OK; r->n = 0|1 */
int cubalc_host_can_create(const char *path, cubalc_host_result *r);
/* process umask: get current (r->n decimal, r->str 4-digit octal) */
int cubalc_host_umask_get(cubalc_host_result *r);
/* set umask to mode&0777; r->n/str = previous mask (decimal / 4-digit octal) */
int cubalc_host_umask_set(long mode, cubalc_host_result *r);
/* push cwd then chdir path; r->str = new cwd; r->n = stack depth */
int cubalc_host_pushd(const char *path, cubalc_host_result *r);
/* pop saved cwd and chdir; r->str = restored cwd; r->n = remaining depth */
int cubalc_host_popd(cubalc_host_result *r);
/* list dirstack as newline bag (bottom→top); r->n = depth */
int cubalc_host_dirstack(cubalc_host_result *r);
/* path kind label via lstat: r->str = missing|file|dir|link|fifo|sock|chr|blk|other;
 * r->n = 0..8 code; missing is probe ok=1 */
int cubalc_host_kindstr(const char *path, cubalc_host_result *r);
/* path relative to base (abspath both). Under base → relative; same → ".";
 * outside → absolute path. r->n = strlen; r->code = 1 if under base, 0 else */
int cubalc_host_relpath(const char *base, const char *path, cubalc_host_result *r);
/* process groups as newline names (egid+supplementary); r->n = count */
int cubalc_host_groups(cubalc_host_result *r);
/* membership: name or decimal gid; r->n = 0|1 */
int cubalc_host_ingroup(const char *name_or_gid, cubalc_host_result *r);
/* owner login name from st_uid via getpwuid; fallback decimal uid string; r->n = uid */
int cubalc_host_ownername(const char *path, cubalc_host_result *r);
/* group name from st_gid via getgrgid; fallback decimal gid string; r->n = gid */
int cubalc_host_groupname(const char *path, cubalc_host_result *r);
/* read symlink target into r->str; soft fail if not a symlink / missing */
int cubalc_host_readlink(const char *path, cubalc_host_result *r);
/* probe: r->n = 1 if path is a symlink, 0 if missing or not a link; always ok */
int cubalc_host_islink(const char *path, cubalc_host_result *r);
/* permission bits: r->n = st_mode & 07777; r->str = 4-digit octal "0644"; soft miss */
int cubalc_host_mode(const char *path, cubalc_host_result *r);
/* set permission bits (mode & 07777); soft miss if path missing */
int cubalc_host_chmod(const char *path, long mode, cubalc_host_result *r);
/* absolute path: realpath if exists, else cwd-join relative; always fills r->str */
int cubalc_host_abspath(const char *path, cubalc_host_result *r);
/* create empty file or update mtime; r->n=1 created, 0 updated */
int cubalc_host_touch(const char *path, cubalc_host_result *r);
/* list directory entries (skip . ..) → r->str newline-joined names, r->n=count */
int cubalc_host_listdir(const char *path, cubalc_host_result *r);

/* env */
int cubalc_host_env(const char *name, cubalc_host_result *r);
/* set process env (overwrite); val may be empty string */
int cubalc_host_env_set(const char *name, const char *val, cubalc_host_result *r);
/* unset process env; r->n = 1 if was set, 0 if already absent */
int cubalc_host_env_unset(const char *name, cubalc_host_result *r);

/* HTTP — loopback + allowlisted api hosts; auth via CUBALC_HTTP_AUTH / XAI_API_KEY */
int cubalc_host_http(const char *method, const char *url, const char *body,
                     cubalc_host_result *r);

/* process — allowlist only */
int cubalc_host_spawn(const char *bin, char *const argv[], cubalc_host_result *r);

/* which — resolve binary on PATH, then CubalC lib/program (readable) */
int cubalc_host_which(const char *name, cubalc_host_result *r);

/* which_bin — PATH / absolute / ./out / ~/.local/bin only (X_OK). No lib fallback.
 * Usability: REQUIRE BIN / SYS WHICHBIN host tool gates. */
int cubalc_host_which_bin(const char *name, cubalc_host_result *r);

/* find CubalC resource only: lib short name · programs/ · proof/ · path.
 * Prefer over PATH when callers want INCLUDE-style resolution (not bins). */
int cubalc_host_find_cubalc(const char *name, cubalc_host_result *r);

/* path join: a + "/" + b (no double slash) → r->str */
int cubalc_host_join(const char *a, const char *b, cubalc_host_result *r);

/* extract last non-empty JSON string field "key" from json blob → r->str */
int cubalc_host_json_get(const char *json, const char *key, cubalc_host_result *r);
/* Expand {{key}} from plate JSON object into out. Returns 0 always.
 * *hits = slots, *miss = missing keys; optional miss_bag newline names. */
int cubalc_host_expand_fillp(const char *plate, const char *tmpl,
                             char *out, size_t outcap,
                             long *hits, long *miss,
                             char *miss_bag, size_t miss_cap);
/* Unique {{key}} names in template order → newline bag. *nkeys out. */
int cubalc_host_fillp_keys(const char *tmpl, char *out, size_t outcap, long *nkeys);
/* top-level object keys of a JSON object → newline bag in r->str; r->n = count.
 * Nested objects/arrays skipped (depth). Soft fail if no object. */
int cubalc_host_json_keys(const char *json, cubalc_host_result *r);
/* set/update top-level key in JSON object → r->str = new object text.
 * val_kind: 0=string (quote+escape), 1=raw (number/true/false/null/JSON fragment).
 * Empty/non-object json → new {"key":val}. r->n = 1 updated|0 inserted. */
int cubalc_host_json_set(const char *json, const char *key, const char *val,
                         int val_kind, cubalc_host_result *r);
/* delete top-level key from JSON object → r->str = new object.
 * r->n = 1 if key was removed, 0 if missing (still OK, object unchanged/copy).
 * Soft fail only on empty key / malformed. Usability: plate field ack/drop. */
int cubalc_host_json_del(const char *json, const char *key, cubalc_host_result *r);
/* merge overlay top-level keys into base (overlay wins). r->str = result object.
 * r->n = keys applied from overlay. Raw values preserved (nested objects ok).
 * Empty/non-object base → {}. Empty overlay → copy of base. */
int cubalc_host_json_merge(const char *base, const char *overlay, cubalc_host_result *r);
/* fill missing keys on plate from defaults (plate wins when present). r->str=result.
 * r->n = keys inserted · r->code = keys already present (skipped). Soft always OK.
 * Empty defaults → copy plate. Non-object plate → start {}. Dual of merge for boots.
 * Usability: multi-key agent defaults without multi JSONENSURE or clobber. */
int cubalc_host_json_defaults(const char *plate, const char *defaults,
                              cubalc_host_result *r);
/* raw top-level value text for key (includes quotes for strings, nested as-is).
 * Soft miss → -1. Usability: rename/pick/copy without re-encoding. */
int cubalc_host_json_get_raw(const char *json, const char *key, cubalc_host_result *r);
/* keep only listed top-level keys (newline bag order). r->str=new object · r->n=kept.
 * Missing keys skipped. Raw values preserved. Empty keys bag → {}. */
int cubalc_host_json_pick(const char *json, const char *keys_nl, cubalc_host_result *r);
/* rename top-level key old→new preserving raw value. r->n=1 renamed|0 missing (copy).
 * If new exists, value is overwritten. Same name no-op with n=1 if present. */
int cubalc_host_json_rename(const char *json, const char *oldk, const char *newk,
                            cubalc_host_result *r);
/* drop listed top-level keys (newline bag). r->str=new object · r->n=removed count.
 * Missing keys soft-skipped. Dual of JSONPICK when noise fields are known. */
int cubalc_host_json_drop(const char *json, const char *keys_nl, cubalc_host_result *r);
/* top-level values as newline bag (JSONKEYS order). Strings decoded; nums/bools raw;
 * nested obj/arr as raw text. r->n = count. Soft empty if not object. */
int cubalc_host_json_values(const char *json, cubalc_host_result *r);
/* build object from key=val / key:val newline bag. r->n = pairs applied.
 * Numeric/true/false/null values raw; others quoted strings. Empty bag → {}. */
int cubalc_host_json_from_kv(const char *bag, cubalc_host_result *r);
/* dual: plate → key:val newline bag (JSONKEYS order). r->n = pairs.
 * ':' sep matches LOOKUP/FREQ default. Strings decoded; true/false/null tokens;
 * nums/nested raw. Soft empty if not object. Round-trips with json_from_kv. */
int cubalc_host_json_to_kv(const char *json, cubalc_host_result *r);
/* top-level key count → r->n. Soft 0 if not object (always OK).
 * Usability: empty/size IF without JSONKEYS bag materialize. r->str = decimal. */
int cubalc_host_json_len(const char *json, cubalc_host_result *r);
/* sum pure-integer top-level values → r->n. Soft 0 if not object.
 * Bools via values map (true=1/false=0); strings/null/nested skipped.
 * Usability: score/status totals without JSONVALUES+SUM glue. r->str = decimal. */
int cubalc_host_json_sum(const char *json, cubalc_host_result *r);
/* key with max (want_min=0) or min (want_min=1) pure-integer value.
 * r->str = key · r->n = value. Soft empty key + n=0 if none. First wins ties.
 * Usability: dominant FREQ key without JSONTOKV+TOPKEY glue. */
int cubalc_host_json_topkey(const char *json, int want_min, cubalc_host_result *r);
/* max (want_min=0) or min (want_min=1) pure-integer top-level value → r->n.
 * Soft 0 if none. Bools via values map. r->str = decimal.
 * Usability: score extremes without JSONVALUES+MIN/MAX glue. */
int cubalc_host_json_minmax(const char *json, int want_min, cubalc_host_result *r);
/* integer mean of pure-integer top-level values → r->n (trunc toward 0).
 * r->code = count of numeric fields used. Soft 0 if none. r->str = decimal.
 * Usability: typical score without JSONVALUES+AVG glue. */
int cubalc_host_json_avg(const char *json, cubalc_host_result *r);
/* plate value denoise / clamp (multi-plate duals of THRESHKV / DROPZERO / CAPKV).
 * mode 0 THRESH: keep pure-int keys with value >= limit; drop non-int + below.
 * mode 1 DROPZERO: drop pure-int keys with value == 0; keep non-int + nonzero.
 * mode 2 CAP: clamp pure-int values to <= limit; keep all keys (non-int unchanged).
 * r->str = result plate · r->n = kept|capped count · r->code = dropped|capped keys.
 * Soft {} if not object. Usability: FREQ plate denoise without TOKV+THRESHKV+FROMKVP. */
int cubalc_host_json_valfilter(const char *json, int mode, long limit,
                               cubalc_host_result *r);
/* plate value rewrite (multi-plate duals of PCTKV / SCALEKV / ADDKV / DIVKV).
 * mode 0 PCT: pure-int values → (v*100)/sum · zero sum → 0 · non-int kept.
 * mode 1 SCALE: pure-int values → v * arg · non-int kept.
 * mode 2 ADD: pure-int values → v + arg · non-int kept.
 * mode 3 DIV: pure-int values → v / arg (trunc toward 0) · arg 0 → 0 · non-int kept.
 * r->str = result plate · r->n = fields rewritten · r->code = total (PCT) or nkeys.
 * Soft {}. Usability: FREQ share/weight/offset/mean without TOKV+…+FROMKVP. */
int cubalc_host_json_valmap(const char *json, int mode, long arg,
                            cubalc_host_result *r);
/* two-plate numeric combine (multi-plate duals of MERGEKV / DIFFKV).
 * mode 0 SUMMERGE: start with a; for each b key pure-int: if a int → sum else set b.
 * mode 1 SUB: start with a; for each b key pure-int: if a int → a−b else set −b.
 * Non-int keys in a kept; non-int-only in b ignored for SUB if a missing (for SUM keep raw b).
 * r->str = plate · r->n = result key count · r->code = hit (shared int keys combined).
 * Soft {}. Usability: peer FREQ combine/delta without TOKV+MERGEKV/DIFFKV+FROMKVP. */
int cubalc_host_json_valmerge(const char *a, const char *b, int mode,
                              cubalc_host_result *r);
/* order-independent top-level plate equality → r->n 1|0. Soft always OK.
 * Same keys + same raw values (whitespace-trimmed). Nested compared as raw text.
 * Usability: agent verify after WRITE/MERGE without fragile string EQS. */
int cubalc_host_json_eq(const char *a, const char *b, cubalc_host_result *r);
/* every key in sub exists in super with same raw value → r->n 1|0.
 * Soft: non-object sub/super → 0 (empty {} is subset of any object).
 * Usability: required-field verify without multi JSONHAS+JSON peels. */
int cubalc_host_json_subset(const char *sub, const char *super, cubalc_host_result *r);
/* bag of keys in sub that fail subset match (missing in super or raw value ≠).
 * r->str newline bag · r->n count. Soft always OK. Non-object sub → empty.
 * Usability: REQUIRE JSONSUBSET fail lists which required fields broke. */
int cubalc_host_json_subset_bad_keys(const char *sub, const char *super,
                                     cubalc_host_result *r);
/* multi-key presence: keys_nl newline bag. want_all=1 all present, 0=any.
 * r->n 0|1 · r->code = number of listed keys found. Soft non-object → 0.
 * Usability: contract keys without multi JSONHAS glue. */
int cubalc_host_json_has_keys(const char *json, const char *keys_nl, int want_all,
                              cubalc_host_result *r);
/* filter required keys into bag: want_present=0 → missing names, 1 → present.
 * r->str = newline bag · r->n = count. Soft always OK. Null/false still present.
 * Usability: after JSONHASALL fail, report WHICH keys without multi JSONHAS. */
int cubalc_host_json_filter_req_keys(const char *json, const char *keys_nl,
                                     int want_present, cubalc_host_result *r);
/* filter plate keys vs allow-list: want_extra=1 → not in allowed, 0 → in allowed.
 * r->str = newline bag · r->n = count. Soft always OK. Non-object → empty.
 * Usability: schema reject unknown fields without JSONKEYS+EACH+HASLINE glue. */
int cubalc_host_json_filter_plate_keys(const char *json, const char *allowed_nl,
                                       int want_extra, cubalc_host_result *r);
/* multi-key peel → newline bag of values (empty field on miss). Order = keys_nl.
 * r->n = listed keys · r->code = found count. Soft always OK.
 * Usability: multi-field extract without multi JSON+PUSH glue. */
int cubalc_host_json_pluck(const char *json, const char *keys_nl,
                           cubalc_host_result *r);
/* plate key-set ops: want_inter=0 → keys in a not in b; 1 → intersection.
 * r->str = newline bag · r->n = count. Soft empty if non-object.
 * Usability: plate key diff/common without JSONKEYS+DIFF/INTERSECT glue. */
int cubalc_host_json_key_set_op(const char *a, const char *b, int want_inter,
                                cubalc_host_result *r);
/* plate value+structure delta: want_same=0 → changed keys (only-a/only-b/value≠);
 * want_same=1 → keys present in both with same raw value.
 * r->str bag · r->n count. Soft empty non-object.
 * Usability: plate sync delta without multi JSONKEYS+JSON peel glue. */
int cubalc_host_json_changed_keys(const char *a, const char *b, int want_same,
                                  cubalc_host_result *r);
/* object of changed keys with raw values from prefer_b ? b : a.
 * Keys only in the other plate still included (value from the side that has it).
 * r->str = plate · r->n = key count. Soft {}.
 * Usability: sync payload without CHANGED+EACH+JSONSET glue. */
int cubalc_host_json_delta_obj(const char *a, const char *b, int prefer_b,
                               cubalc_host_result *r);
/* human/agent changelog bag: one line per changed key "key: old → new".
 * Missing side shown as (missing). Internal newlines collapsed to space.
 * r->str newline bag · r->n count. Soft empty if identical/non-object.
 * Usability: log plate sync without CHANGED+EACH+JSON+CAT glue. */
int cubalc_host_json_changelog(const char *a, const char *b, cubalc_host_result *r);

/* chat: backend "local"|"grok", model may be ""/"local", msg required.
 * Reply text in r->str. Uses loopback :1212 or api.x.ai. */
int cubalc_host_chat(const char *backend, const char *model, const char *msg,
                     const char *state_dir, cubalc_host_result *r);

#ifdef __cplusplus
}
#endif
#endif
