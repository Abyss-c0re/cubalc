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
/* Dotted/slash path sugar for nested plate fields (GETP "freq.error").
 * Path seps: '.' or '/'. No sep → same as top-level get/set/del.
 * Max 8 segments. Intermediate miss on get/del → soft fail.
 * path_set creates missing intermediate objects as {}.
 * Usability: one form for shallow+deep without GETPOBJ+GETP/SETP+SETOBJ. */
int cubalc_host_json_path_get(const char *json, const char *path, cubalc_host_result *r);
/* Raw leaf peel along path (quotes/nested preserved) · soft miss -1. */
int cubalc_host_json_path_get_raw(const char *json, const char *path,
                                  cubalc_host_result *r);
int cubalc_host_json_path_set(const char *json, const char *path, const char *val,
                              int val_kind, cubalc_host_result *r);
int cubalc_host_json_path_del(const char *json, const char *path, cubalc_host_result *r);
/* Soft presence: r->n = 0|1 · r->ok=1 always when args valid. */
int cubalc_host_json_path_has(const char *json, const char *path, cubalc_host_result *r);
/* Peel object at path for KEYSP/LENP/EMPTYP/VALSP probes.
 * path empty/NULL → whole json (must be object or soft "{}").
 * Soft miss / non-object leaf → r->str="{}" · r->n=0 · always OK.
 * Usability: nest bag size/keys without GETOBJ+KEYSP glue. */
int cubalc_host_json_path_obj(const char *json, const char *path, cubalc_host_result *r);
/* Recursively collect dotted leaf paths of a JSON object → newline bag.
 * path/prefix: optional nest root (NULL/"" = whole plate). Paths are relative
 * to that root (cfg.meta.role). Leaf = non-object value or empty {}.
 * Intermediate non-empty objects are walked, not listed. r->n = count.
 * Soft always OK. Max depth 8 (same as path get/set).
 * Usability: agent discover nest structure without KEYSP+GETOBJ recursion. */
int cubalc_host_json_leaf_paths(const char *json, const char *path,
                                cubalc_host_result *r);
/* Recursive path:value bag (FREQ/LOOKUP `path:val` lines) for nested plates.
 * path/prefix optional nest root; paths relative to that root.
 * Same leaf rules as leaf_paths; empty {} → `path:` (empty value).
 * Strings decoded; nums/bools/null/arr raw. r->n = pairs. Soft always OK.
 * Usability: agent flat report without PATHKEYS+GETP glue or TOKVP shallow-only. */
int cubalc_host_json_leaf_kv(const char *json, const char *path,
                             cubalc_host_result *r);
/* Apply recursive path:value bag onto base plate (round-trip of leaf_kv).
 * bag lines `path:val` or `path=val`. under: optional nest prefix (cfg → cfg.path).
 * Empty value → raw {} (restores empty-object terminals from leaf_kv).
 * true/false/null/int/float raw; else string. Creates intermediate {}.
 * r->str = result plate · r->n = pairs applied. Soft empty bag = copy base.
 * Usability: agent edit flat inventory then re-nest without multi SETP. */
int cubalc_host_json_unflat_kv(const char *base, const char *bag, const char *under,
                               cubalc_host_result *r);
/* Deep leaf-path changelog between two plates (FLATKV dual of CHANGELOGP).
 * paths_only=0 → "path: old → new" lines · (missing) for add/drop.
 * paths_only=1 → bag of changed leaf paths only.
 * Compares recursive leaf_kv inventories (not top-level keys only).
 * r->n = change count · soft always OK. Empty if equal.
 * Usability: nest-aware mesh/agent plate sync without shallow DIFFP. */
int cubalc_host_json_leaf_diff(const char *a, const char *b, int paths_only,
                               cubalc_host_result *r);
/* Filter recursive path:value bag by path needle (GREPFLAT).
 * invert=1 drop matches · icase=1 case-insensitive path match.
 * Empty needle → all leaves (or none if invert). r->n = kept lines.
 * Soft always OK. Usability: nest triage without FLATKV+SYS GREP glue. */
int cubalc_host_json_leaf_grep(const char *json, const char *needle, int invert,
                               int icase, cubalc_host_result *r);
/* Write-back leaf filter by path needle.
 * keep_only=0 (PRUNEFLAT): path_del every matching leaf · r->n = deleted.
 * keep_only=1 (KEEPONLYFLAT): rebuild plate from matching path:val only · r->n = kept.
 * Empty needle + keep_only=0 → no-op copy · keep_only=1 → full copy (all leaves).
 * Soft always OK. Usability: scrub/project nested plate without multi DELP/UNFLAT glue. */
int cubalc_host_json_leaf_filter_write(const char *json, const char *needle,
                                       int keep_only, int icase,
                                       cubalc_host_result *r);
/* Deep leaf overlay: FLATKV(overlay) then UNFLAT onto base (nest-aware MERGEP).
 * Overlay leaf paths win; base leaves not in overlay kept. r->n = overlay leaf count.
 * Soft always OK. Empty overlay → copy base.
 * Usability: peer/agent deep sync without FLATKV+UNFLATKV or shallow MERGEP. */
int cubalc_host_json_leaf_merge(const char *base, const char *overlay,
                                cubalc_host_result *r);
/* Rename leaf path prefix across plate (RENAMEFLAT).
 * Paths starting with old_pfx get old_pfx replaced by new_pfx; other leaves kept.
 * Empty old_pfx → no-op copy. Rebuilds plate via unflat. r->n = renamed count.
 * Soft always OK. Usability: restructure nests without multi RENAMEP/DELP+SETP. */
int cubalc_host_json_leaf_rename_pfx(const char *json, const char *old_pfx,
                                     const char *new_pfx, cubalc_host_result *r);
/* Set value on every leaf whose path contains needle (SETFLAT).
 * Empty needle → all leaves. value classified like unflat (num/bool/null/raw/string).
 * Rebuilds plate. r->n = leaves updated. Soft always OK.
 * Usability: bulk nest flag/port/default write without multi SETP/GREPFLAT glue. */
int cubalc_host_json_leaf_set(const char *json, const char *needle, const char *val,
                              cubalc_host_result *r);
/* Increment pure-int leaves whose path contains needle (INCFLAT).
 * delta default 1. Non-numeric matching leaves soft-skipped (not counted).
 * Empty needle → all pure-int leaves. r->n = bumped · r->code = total leaves.
 * Soft always OK. Usability: bulk nest counters without multi INCP glue. */
int cubalc_host_json_leaf_inc(const char *json, const char *needle, long delta,
                              cubalc_host_result *r);
/* Sum pure-int leaves whose path contains needle (SUMFLAT).
 * Empty needle → all pure-int leaves. Non-numeric matching leaves soft-skipped.
 * r->n = sum · r->code = count of pure-int leaves summed · r->str = decimal sum.
 * Soft always OK. Read-only (no plate write). Usability: nest counter rollups
 * without FLATKV+GREPFLAT+AFTER+SUM glue. */
int cubalc_host_json_leaf_sum(const char *json, const char *needle,
                              cubalc_host_result *r);
/* Integer mean of pure-int leaves by path needle (AVGFLAT / MEANFLAT).
 * Empty needle → all pure-int. Non-numeric matching leaves soft-skipped.
 * r->n = avg (trunc toward 0) · r->code = count · r->str = decimal avg ·
 * r->err = decimal sum (side-channel). Soft always OK. Read-only.
 * Usability: nest mean scores without FLATKV+GREP+AVG / SUMFLAT+DIV glue. */
int cubalc_host_json_leaf_avg(const char *json, const char *needle,
                              cubalc_host_result *r);
/* Integer median of pure-int leaves by path needle (MEDIANFLAT / P50FLAT).
 * Empty needle → all pure-int. Sort asc; even n → lower mid vals[(n-1)/2].
 * Cap 256 samples. r->n = median · r->code = count · r->str = decimal.
 * Soft always OK. Read-only. Usability: robust nest mid without FLATKV+GREP+MEDIAN. */
int cubalc_host_json_leaf_median(const char *json, const char *needle,
                                 cubalc_host_result *r);
/* Extreme pure-int leaf by path needle (TOPPATHFLAT / BOTPATHFLAT).
 * want_min=0 → max (TOP) · want_min=1 → min (BOT). First path wins ties.
 * Empty needle → all pure-int leaves. r->str = winning path · r->n = value ·
 * r->code = count of pure-int leaves considered. Soft empty → str="" n=0.
 * Usability: locate dominant/min nest counter without GREPFLAT+EACH walk. */
int cubalc_host_json_leaf_toppath(const char *json, const char *needle, int want_min,
                                  cubalc_host_result *r);
/* Keep pure-int leaves with value >= min among path-needle matches (THRESHFLAT).
 * drop_zero_only!=0 → DROPZEROFLAT: drop matching pure-int leaves with value==0 only.
 * Non-int leaves always kept. Empty needle → all pure-int. Rebuilds plate.
 * r->str = new plate · r->n = kept matching pure-int · r->code = dropped count.
 * Soft always OK. Usability: nest FREQ denoise without GETOBJ+THRESHP+SETOBJ. */
int cubalc_host_json_leaf_thresh(const char *json, const char *needle, long minv,
                                 int drop_zero_only, cubalc_host_result *r);
/* Clamp pure-int leaves value > max down to max among path-needle matches (CAPFLAT).
 * Non-int leaves kept. Empty needle → all pure-int. Rebuilds plate.
 * r->str = new plate · r->n = leaves capped · r->code = pure-int matches considered.
 * Soft always OK. Usability: nest score ceiling without multi GETP+SETP/CAPP glue. */
int cubalc_host_json_leaf_cap(const char *json, const char *needle, long maxv,
                              cubalc_host_result *r);
/* Multiply pure-int leaves by factor among path-needle matches (SCALEFLAT).
 * Non-int leaves kept. Empty needle → all pure-int. Rebuilds plate.
 * r->str = new plate · r->n = leaves scaled · r->code = pure-int matches considered.
 * Soft always OK. Usability: nest score scale without multi GETP+arith+SETP glue. */
int cubalc_host_json_leaf_scale(const char *json, const char *needle, long factor,
                                cubalc_host_result *r);
/* Count leaf paths containing needle (COUNTFLAT / HASFLAT).
 * Empty needle → total leaf count. r->n = match count · r->str = decimal count.
 * Soft always OK. Read-only. Usability: structure probe without GREPFLAT+LINES glue. */
int cubalc_host_json_leaf_count(const char *json, const char *needle,
                                cubalc_host_result *r);
/* Count pure-int leaves whose path contains needle (COUNTFLATN / HASFLATN).
 * Non-int matching leaves skipped. Empty needle → all pure-int leaves.
 * r->n = pure-int match count · r->str = decimal. Soft always OK. Read-only.
 * Usability: typed soft probe without GETFLATN+IF or NEEDFLATN fail glue. */
int cubalc_host_json_leaf_countn(const char *json, const char *needle,
                                 cubalc_host_result *r);
/* Matching leaf paths or values as newline bag (PATHSFLAT / VALSFLAT).
 * want_vals=0 → path bag · want_vals=1 → value bag. Empty needle → all leaves.
 * r->str = bag · r->n = field count. Soft always OK. Read-only.
 * Usability: peel GREPFLAT without BEFORE/AFTER · EACH LINE walk. */
int cubalc_host_json_leaf_match_bag(const char *json, const char *needle, int want_vals,
                                    cubalc_host_result *r);
/* Pure-int matching leaf paths or values bag (PATHSFLATN / VALSFLATN).
 * want_vals=0 → path bag · want_vals=1 → decimal value bag. Non-int skipped.
 * Empty needle → all pure-int leaves. r->str = bag · r->n = field count.
 * Soft always OK. Read-only. Usability: numeric EACH LINE without string pollution. */
int cubalc_host_json_leaf_match_bagn(const char *json, const char *needle, int want_vals,
                                     cubalc_host_result *r);
/* First leaf whose path contains needle (GETFLAT).
 * Empty needle → first leaf. Soft miss → r->n=0 · r->str="" · r->err="".
 * Hit → r->n=1 · r->str=value · r->err=path (path side-channel on success).
 * Soft always OK. Usability: nest peel by needle without PATHSFLAT+GETP glue. */
int cubalc_host_json_leaf_get(const char *json, const char *needle,
                              cubalc_host_result *r);
/* Last leaf whose path contains needle (LASTFLAT).
 * Empty needle → last leaf. Soft miss → r->n=0 · r->str="" · r->err="".
 * Hit → r->n=1 · r->str=value · r->err=path. Soft always OK.
 * Usability: newest-wins nest peel without REVL+GETFLAT glue. */
int cubalc_host_json_leaf_get_last(const char *json, const char *needle,
                                   cubalc_host_result *r);
/* 0-based Nth leaf by path needle (NTHFLAT).
 * index < 0 or past end → miss. Hit → r->n=1 · r->str=value · r->err=path.
 * Soft always OK. Usability: index nest peels without PATHSFLAT+NTH+GETP glue. */
int cubalc_host_json_leaf_get_nth(const char *json, const char *needle, long index,
                                  cubalc_host_result *r);
/* First pure-int leaf whose path contains needle (GETFLATN).
 * Non-int matching leaves skipped. Empty needle → first pure-int leaf.
 * Hit → r->n=value · r->code=1 · r->str=decimal · r->err=path.
 * Miss → r->n=0 · r->code=0. Soft always OK.
 * Usability: nest numeric peel without GETFLAT+NUM glue. */
int cubalc_host_json_leaf_getn(const char *json, const char *needle,
                               cubalc_host_result *r);
/* Last pure-int leaf whose path contains needle (LASTFLATN).
 * Non-int matching leaves skipped. Empty needle → last pure-int leaf.
 * Hit → r->n=value · r->code=1 · r->str=decimal · r->err=path.
 * Miss → r->n=0 · r->code=0. Soft always OK.
 * Usability: newest-wins nest numeric peel without REVL+GETFLATN glue. */
int cubalc_host_json_leaf_getn_last(const char *json, const char *needle,
                                    cubalc_host_result *r);
/* 0-based Nth pure-int leaf by path needle (NTHFLATN).
 * index < 0 or past end → miss (code=0). Non-int matching leaves skipped.
 * Hit → r->n=value · r->code=1 · r->str=decimal · r->err=path.
 * Usability: index pure-int nest peels after COUNTFLATN without VALSFLATN+NTH. */
int cubalc_host_json_leaf_getn_nth(const char *json, const char *needle, long index,
                                   cubalc_host_result *r);
/* First matching leaf kind by path needle (TYPEFLAT).
 * r->str = missing|num|str|bool|null|obj|arr · r->n = 0..6 · r->err = path on hit.
 * r->code = hit 0|1. Soft always OK. Empty needle → first leaf.
 * Usability: choose GETFLAT vs GETFLATN vs nest peel without TYPEP+PATHSFLAT glue. */
int cubalc_host_json_leaf_type(const char *json, const char *needle,
                               cubalc_host_result *r);
/* Frequency of matching leaf values by path needle (FREQFLAT / HISTFLAT).
 * Empty needle → all leaves. r->str = "val:count" bag (first-seen order, sep ':').
 * r->n = unique keys · r->code = total matching leaves. Cap 64 uniques.
 * Soft always OK. Read-only. Usability: nest status rollups without VALSFLAT+FREQ. */
int cubalc_host_json_leaf_freq(const char *json, const char *needle,
                               cubalc_host_result *r);
/* Dominant (mode) matching leaf value by path needle (MODEFLAT / TOPVALFLAT).
 * First-seen wins ties. Empty needle → all leaves.
 * Hit → r->str=value · r->n=winner count · r->code=total leaves · r->err=decimal count.
 * Soft miss → empty str · n=0 · code=0. Soft always OK. Read-only.
 * Usability: majority role/status without FREQFLAT+SORTFREQ+BEFORE glue. */
int cubalc_host_json_leaf_mode(const char *json, const char *needle,
                               cubalc_host_result *r);
/* First leaf path whose value equals needle (PATHBYVAL / VALPATH / FINDVAL).
 * Exact match on decoded leaf text. Empty needle → first empty-value leaf (rare).
 * Hit → r->str=path · r->err=value · r->n=1 · r->code=1.
 * Soft miss → str="" · n=0 · code=0. Soft always OK. Read-only.
 * Usability: reverse nest lookup without EACH FLAT+EQS glue (dual of GETFLAT). */
int cubalc_host_json_leaf_path_by_val(const char *json, const char *val,
                                      cubalc_host_result *r);
/* All leaf paths whose value equals needle (PATHSBYVAL / ALLPATHSBYVAL).
 * Exact match. Soft empty bag. r->str = path bag · r->n = count · r->err = value.
 * Soft always OK. Read-only. Usability: multi reverse lookup without EACH FLAT+IF+PUSH. */
int cubalc_host_json_leaf_paths_by_val(const char *json, const char *val,
                                       cubalc_host_result *r);
/* Count leaves whose value equals needle (COUNTBYVAL / HASVAL).
 * Exact match. r->n = count · r->str = decimal · r->code = hit 0|1 · r->err = value.
 * Soft always OK. Read-only. Usability: presence/count without PATHSBYVAL+LINES glue. */
int cubalc_host_json_leaf_count_by_val(const char *json, const char *val,
                                       cubalc_host_result *r);
/* Rewrite every leaf value equal to oldv → newv (SETBYVAL / REPLACEVAL).
 * Exact match. r->str = new plate · r->n = leaves rewritten · r->code = total leaves.
 * Soft always OK. Usability: bulk status rewrite without PATHSBYVAL+EACH+SETP glue. */
int cubalc_host_json_leaf_set_by_val(const char *json, const char *oldv, const char *newv,
                                     cubalc_host_result *r);
/* Drop every leaf whose value equals needle (DELBYVAL / DROPVAL).
 * Exact match. r->str = new plate · r->n = leaves deleted · r->code = leaves kept.
 * Soft always OK. Usability: scrub by value without PATHSBYVAL+EACH+DELP glue. */
int cubalc_host_json_leaf_del_by_val(const char *json, const char *val,
                                     cubalc_host_result *r);
/* Unique matching leaf values by path needle (UNIQFLAT / DISTINCTFLAT).
 * Empty needle → all leaves. First-seen order. Cap 64 uniques.
 * r->str = value bag · r->n = unique count · r->code = total matching leaves.
 * Soft always OK. Read-only. Usability: "what values exist" without VALSFLAT+UNIQ. */
int cubalc_host_json_leaf_uniq(const char *json, const char *needle,
                               cubalc_host_result *r);
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
/* rename key old→new preserving raw value. r->n=1 renamed|0 missing (copy).
 * If new exists, value is overwritten. Same name no-op with n=1 if present.
 * Paths: "freq.error" / "a/b/c" via path_get/del/set · create intermediate {} on dest.
 * Usability: promote/move nested fields without GETP+DELP+SETP glue. */
int cubalc_host_json_rename(const char *json, const char *oldk, const char *newk,
                            cubalc_host_result *r);
/* copy key src→dst preserving raw value (paths ok). r->n=1 copied|0 soft miss.
 * Dest overwrites if present. Same path no-op n=1 if present. Dual of rename keep-src. */
int cubalc_host_json_copy_key(const char *json, const char *src, const char *dst,
                              cubalc_host_result *r);
/* swap two keys (paths ok). r->n=1 swapped|0 both miss. One-sided miss moves value.
 * Same path no-op with n=1 if present. Dual of dual path_set. */
int cubalc_host_json_swap_keys(const char *json, const char *a, const char *b,
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
/* integer median of pure-integer top-level values → r->n.
 * Sort ascending; even n → lower mid (vals[(n-1)/2]) like MEDIANKV/SYS MEDIAN.
 * Cap 256 values. Soft 0 if none. r->code = count used. r->str = decimal.
 * Usability: robust mid score without JSONVALUES+MEDIAN / TOKV+MEDIANKV. */
int cubalc_host_json_median(const char *json, cubalc_host_result *r);
/* plate value denoise / clamp (multi-plate duals of THRESHKV / DROPZERO / CAPKV).
 * mode 0 THRESH: keep pure-int keys with value >= limit; drop non-int + below.
 * mode 1 DROPZERO: drop pure-int keys with value == 0; keep non-int + nonzero.
 * mode 2 CAP: clamp pure-int values to <= limit; keep all keys (non-int unchanged).
 * r->str = result plate · r->n = kept|capped count · r->code = dropped|capped keys.
 * Soft {} if not object. Usability: FREQ plate denoise without TOKV+THRESHKV+FROMKVP. */
int cubalc_host_json_valfilter(const char *json, int mode, long limit,
                               cubalc_host_result *r);
/* plate value rewrite (multi-plate duals of PCT/SCALE/ADD/DIV/ABS/SIGN KV).
 * mode 0 PCT: pure-int values → (v*100)/sum · zero sum → 0 · non-int kept.
 * mode 1 SCALE: pure-int values → v * arg · non-int kept.
 * mode 2 ADD: pure-int values → v + arg · non-int kept.
 * mode 3 DIV: pure-int values → v / arg (trunc toward 0) · arg 0 → 0 · non-int kept.
 * mode 4 ABS: pure-int values → |v| · r->n = negatives flipped · non-int kept.
 * mode 5 SIGN: pure-int values → −1|0|1 · r->n = rewritten · r->code packs pos/neg/zero
 *   as (pos<<16)|(neg<<8)|zero (each 0..255) when possible; else nkeys.
 * Soft {}. Usability: FREQ rewrite without TOKV+…+FROMKVP. */
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
/* filter plate keys by needle substring (multi-plate dual of KEEPKEY/DROPKEY).
 * invert=0 keep matching · invert=1 drop matching · icase=1 ASCII case-insensitive.
 * empty needle: keep all (or drop all if invert). Soft {}.
 * r->str = plate · r->n = kept · r->code = dropped.
 * Usability: FREQ key pattern filter without TOKV+KEEPKEY+FROMKVP or multi PICKP. */
int cubalc_host_json_keygrep(const char *json, const char *needle, int invert,
                             int icase, cubalc_host_result *r);
/* top/bottom N pure-int keys by value → result plate (want_bot=1 for min-first).
 * Cap 256 candidates. Ties keep first-seen order among equal values.
 * r->str = plate of up to n keys · r->n = kept · r->code = candidates scanned.
 * Soft {}. Usability: top severities without TOKV+SORTFREQ+TAKE+FROMKVP. */
int cubalc_host_json_topn(const char *json, long n, int want_bot,
                          cubalc_host_result *r);
/* sort all pure-int keys by value (want_asc=1 asc, 0 desc).
 * as_bag=0 → plate in sort order · as_bag=1 → key:val bag (sep ':') for EACH/FREQ.
 * Cap 256. r->n = count · r->code = same. Soft {}.
 * Usability: full FREQ rank without TOKV+SORTFREQ or TOPNP cand guess. */
int cubalc_host_json_sortbyval(const char *json, int want_asc, int as_bag,
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
/* Multi-key presence (HASPALL/NEEDP/JSONHASALL). Keys may be dotted paths. */
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
