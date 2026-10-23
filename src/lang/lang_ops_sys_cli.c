/* CubalC lang — lang_ops_sys_cli.c
 * CLI flag scan · pathline peel · SYS did-you-mean · ONEOF/INRANGE atoms · USAGE tip.
 * Mesh vote A_split_lang_ops_core.
 */
#include "lang/cubalc_lang_ops_sys_cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


/* Peel PATHLINETREE / grep -rn entry "path:lineno:text" (1-based lineno digits).
 * Path is everything before first ":digits:"; text may contain colons.
 * Returns 1 on hit, 0 if no path:lineno: pattern. */
int cubalc_peel_pathline(const char *s, char *path, size_t pathn,
                                long *lineno, char *text, size_t textn) {
  const char *p, *digits, *rest;
  size_t pl;
  long n;
  if (!s || !s[0]) return 0;
  for (p = s; *p; p++) {
    if (*p != ':') continue;
    digits = p + 1;
    if (*digits < '0' || *digits > '9') continue;
    rest = digits;
    while (*rest >= '0' && *rest <= '9') rest++;
    if (*rest != ':') continue;
    pl = (size_t)(p - s);
    if (path && pathn) {
      if (pl >= pathn) pl = pathn - 1;
      if (pl > 0) memcpy(path, s, pl);
      path[pl] = 0;
    }
    n = 0;
    while (digits < rest) {
      n = n * 10 + (*digits - '0');
      if (n > 0x3fffffff) n = 0x3fffffff;
      digits++;
    }
    if (lineno) *lineno = n;
    if (text && textn)
      snprintf(text, textn, "%s", rest + 1);
    return 1;
  }
  return 0;
}

/* SYS unknown-op did-you-mean — replace opaque catalog soup with agent recovery. */
static void sys_fold(char *dst, size_t n, const char *src){
  size_t i;
  for (i = 0; i + 1 < n && src[i]; i++) {
    char c = src[i];
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    dst[i] = c;
  }
  dst[i] = 0;
}
static int sys_edit_dist(const char *a, const char *b){
  int na = (int)strlen(a), nb = (int)strlen(b);
  int i, j, prev, cur, stack[64];
  if (na > 48) na = 48;
  if (nb > 48) nb = 48;
  if (na + 1 > 64) return 99;
  for (j = 0; j <= nb; j++) stack[j] = j;
  for (i = 1; i <= na; i++) {
    prev = stack[0];
    stack[0] = i;
    for (j = 1; j <= nb; j++) {
      cur = stack[j];
      if (a[i - 1] == b[j - 1])
        stack[j] = prev;
      else {
        int ins = stack[j - 1] + 1;
        int del = stack[j] + 1;
        int sub = prev + 1;
        int m = ins < del ? ins : del;
        stack[j] = m < sub ? m : sub;
      }
      prev = cur;
    }
  }
  return stack[nb];
}
/* Pipe-separated primary SYS op names (aliases included) for typo recovery. */
static const char *const sys_op_suggest_catalog =
  "READ|WRITE|RM|RMTREE|RMNEST|RMRF|DELTREE|CPTREE|COPYTREE|CPDIR|TREECOPY|MVTREE|MOVETREE|MVDIR|RENAMETREE|DIRSIZE|DU|TREESIZE|DISKUSAGE|EQTREE|SAMETREE|CMPTREE|EQDIR|GREPTREE|SEARCHTREE|RGREP|TREEGREP|REPLACETREE|GSUBTREE|SEDTREE|TREEREPLACE|COUNTINTREE|COUNTTREE|GREPCOUNTTREE|TREECOUNT|GREPLINESTREE|EXTRACTLINETREE|HARVESTTREE|LINETREE|PATHLINETREE|GREPNTREE|RGLINES|ANNOTREE|PARSEPATHLINE|SPLITPATHLINE|PEELPATHLINE|FORMATPATHLINE|MKPATHLINE|TOPATHLINE|PATHLINEPATHS|PATHSFROMPL|UNIQPATHLINES|PATHLINETEXTS|TEXTSFROMPL|PLTEXTS|PATHLINEFREQ|COUNTBYPATH|PLFREQ|FIRSTINTREE|GREP1TREE|LASTINTREE|GREP1TREEL|HASINTREE|ANYINTREE|CONTAINSINTREE|TAKEINTREE|FIRSTNINTREE|HEADINTREE|NTHINTREE|INDEXINTREE|SLICEINTREE|MIDINTREE|WINDOWINTREE|DROPINTREE|SKIPINTREE|RESTINTREE|FIRSTLINEINTREE|GREP1LINETREE|LASTLINEINTREE|GREP1LINETREEL|RENAME|COPY|REALPATH|TOUCH|LIST|GLOB|MATCHFILES|PATHGLOB|PGLOB|FULLGLOB|FILTERGLOB|MATCHBAG|GREPGLOB|NTH|GREP|GREPANY|GREPALL|FIRSTMATCH|GREP1|LASTMATCH|GREP1L|LOOKUP|KVGET|LOOKUPN|KVGETN|KVSET|SETKV|KVINC|INCKV|KVDEL|DELKV|MERGEKV|KVADDALL|DIFFKV|SUBKV|SUMKV|TOTALKV|AVGKV|MEANKV|MEDIANKV|P50KV|TOPKEY|BOTKEY|THRESHKV|KEEPVAL|DROPZERO|KEEPNZ|KEEPKEY|GREPKEY|DROPKEY|PCTKV|SHAREKV|CAPKV|CLAMPKV|SCALEKV|MULKV|DIVKV|IDIVKV|ADDKV|OFFSETKV|ABSKV|MAGKV|SIGNKV|DIRKV|CHUNK|BATCH|WINDOW|SLIDE|STRIDE|EVERY|ROTATE|ROTL|ROTR|FLATTEN|UNCHUNK|TAKE|DROP|SPLIT|WORDS|CUT|CUTALL|COLUMN|SORT|SORTN|SORTLEN|UNIQ|UNION|DISTINCT|INTERSECT|DIFF|ZIP|KEYS|VALS|PREFIXALL|SUFFIXALL|FILL|ENUMERATE|NUMBER|SQUEEZE|COMPACT|TRIMALL|UPPERALL|LOWERALL|MAPREPLACE|GSUBALL|FREQ|HIST|SORTFREQ|BEFOREALL|AFTERALL|MIDLINES|SLICEBAG|REVL|JOINLINES|PUSH|PREPEND|POP|POPHEAD|LINES|HASLINE|COUNTLINE|COUNTMATCH|GREPCOUNT|FINDLINE|SETLINE|SETMATCH|INSERTLINE|DROPNTH|MOVELINE|REMOVELINE|ENV|SETENV|UNSETENV|DOTENV|LOADENV|ENVFILE|SOURCEENV|DUMPENV|ENVALL|ENVDUMP|LISTENV|WRITEENV|SAVENV|DUMPENVFILE|ENVSAVE|EXIST|SIZE|ISDIR|ISFILE|MTIME|AGE|MKDIR|BASENAME|DIRNAME|EXTNAME|STEM|BASENAMEALL|DIRNAMEALL|EXTALL|STEMALL|KEEPFILES|KEEPDIRS|KEEPEXIST|SIZEALL|MAPSIZE|MTIMEALL|AGEALL|NEWEST|OLDEST|LARGEST|SMALLEST|SORTMTIME|SORTSIZE|FRESH|KEEPSTALE|AGED|KEEPNEWER|NEWERTHAN|KEEPOLDER|OLDERREF|KEEPBIGGER|BIGFILES|SIZEGE|KEEPSMALLER|SMALLFILES|SIZELE|RMALL|UNLINKALL|DELETEALL|TOUCHALL|ENSUREALL|CREATEALL|COPYALL|CPALL|BULKCOPY|MKDIRALL|ENSUREDIRS|MKDIRS|MOVEALL|MVALL|RENAMEALL|WALK|FINDALL|TREEGLOB|EQFILE|SAMEFILE|CMPFILE|LOGALL|APPENDFILES|BULKAPPEND|GREPFILES|SEARCHFILES|FILESGREP|GREPFILESI|GREPVFILES|VGREPFILES|READALL|CATFILES|SLURPALL|CATALL|CONCATFILES|READFILES|WRITEALL|WRITEFILES|BULKWRITE|SPLATALL|OVERWRITEALL|REPLACEFILES|SUBFILES|GSUBFILES|FILEGSUB|BULKREPLACE|COUNTINFILES|GREPCOUNTFILES|FILECOUNT|COUNTINFILESI|FIRSTFILE|HITFILE|GREP1FILE|LASTFILE|HITFILEL|FIRSTFILEI|LASTFILEI|GREPLINES|EXTRACTLINES|FILEGREPLINES|GREPLINESI|HEADFILE|FILEHEAD|TAILFILE|FILETAIL|HEADF|TAILF|LINECOUNTALL|WCALL|MAPLINES|LINESALL|MIDFILE|FILEMID|LINESLICE|SLICEFILE|FILESLICE|LINEAT|FILELINE|ATLINE|NTHFILELINE|SETFILELINE|FILESETLINE|PUTFILELINE|SETLINEAT|INSERTFILELINE|FILEINSERTLINE|INSFILELINE|INSLINEF|DROPFILELINE|FILEDROPLINE|DELFILELINE|DROPLINEF|FINDFILELINE|FILEFINDLINE|LINEINDEXF|FINDLINEF|FINDFILELINEI|LASTFINDFILELINE|FILELASTFIND|LINEINDEXFL|LASTFINDFILELINEI|COUNTFILELINE|FILECOUNTLINE|COUNTMATCHF|COUNTFILELINEI|GREPFILE|FILEGREP|MATCHFILELINES|GREPFILEI|SETMATCHFILE|FILESETMATCH|REPLACEMATCHF|SETMATCHFILEI|DROPMATCHFILE|FILEDROPMATCH|DELMATCHFILE|DROPMATCHFILEI|DROPALLMATCHFILE|FILEDROPALLMATCH|DELALLMATCHFILE|PURGEMATCHFILE|DROPALLMATCHFILEI|SETALLMATCHFILE|FILESETALLMATCH|REPLACEALLMATCHF|SETALLMATCHFILEI|INSERTMATCHFILE|FILEINSERTMATCH|AFTERMATCHFILE|INSERTMATCHFILEI|BEFOREMATCHFILE|INSERTBEFOREMATCH|FILEINSERTBEFORE|BEFOREMATCHFILEI|LOOKUPFILE|FILELOOKUP|KVFILEGET|PLATEGET|LOOKUPFILEI|KVFILESET|FILEKVSET|SETKVFILE|PLATESET|KVFILEDEL|FILEKVDEL|DELKVFILE|PLATEDEL|KVFILEINC|INCKVFILE|FILEKVINC|PLATEINC|ENSURELINE|ENSUREFILELINE|ADDIFMISSING|FILEENSURE|HASFILELINE|FILEHASLINE|HASLINEF|HASFILELINEI|WHICH|WHICHBIN|PATHWHICH|BIN|CWD|CHDIR|STATE|ROOT|TMP|HTTP|SPAWN|JOIN|JSON|JSONN|JGETN|JSONNUM|JSONKEYS|KEYSJSON|JKEYS|JSONHAS|HASJSON|JHAS|JSONSET|JSET|JSONPUT|UPDATEPLATE|JSONFILESET|PLATEKEYSET|SETJSONFILE|PATCHPLATE|PLATEPUT|SETPLATEKEY|BUMPPLATE|JSONFILEINC|PLATEKEYINC|TICKPLATE|DECRPLATE|JSONFILEDEC|PLATEKEYDEC|PEEKPLATE|JSONFILEGET|PLATEKEYGET|PLATEPEEK|GETPLATEKEY|DROPPLATE|JSONFILEDEL|PLATEKEYDEL|DELPLATEKEY|UNSETPLATEKEY|JSONFILEMERGE|OVERLAYPLATE|MERGEPLATEFILE|APPLYPLATE|PLATEOVERLAY|JSONFILEENSURE|ENSUREPLATEKEY|DEFAULTPLATEKEY|SEEDPLATEKEY|JSONFILEDEFAULTS|DEFAULTPLATEALL|ENSUREPLATEALL|SEEDPLATEALL|JSONFILETOGGLE|TOGGLEPLATEKEY|FLIPPLATEKEY|PLATEFILETOGGLE|JSONOBJ|MKJSON|JOBJ|JSONDEL|JDEL|JSONRM|JSONMERGE|MERGEJSON|JMERGE|JSONINC|JINC|JSONDEC|JDEC|JSONENSURE|JDEFAULT|JSONDEFAULT|JENSURE|ENSUREJSON|JSONPICK|JSONKEEP|JKEEP|JSELECT|PICKJSON|JSONRENAME|JRENAME|RENAMEJSON|JSONMV|JSONDROP|JSONOMIT|JDROPK|JOMIT|DROPJSON|JSONTOGGLE|JTOGGLE|FLIPJSON|JSONFLIP|JSONTYPE|JTYPE|JSONKIND|TYPEJSON|JSONCOPY|JCOPY|COPYJSON|JSONDUP|JSONSWAP|JSWAP|SWAPJSON|JSONXCHG|JSONVALUES|JVALS|JSONVALS|VALSJSON|JSONFROMKV|JFROMKV|KVTOJSON|JSONFROMBAG|JSONTOKV|JTOKV|JSONTOBAG|PLATETOKV|JSONLEN|JLEN|JSONCOUNT|NKEYS|JSONEMPTY|JEMPTY|JSONNONEMPTY|JNONEMPTY|JSONSUMN|JSONSUM|JSUMN|SUMJSON|JSONTOPKEY|JTOPKEY|JSONBOTKEY|JBOTKEY|JSONMAXN|JMAXN|JSONMINN|JMINN|JSONAVGN|JAVGN|JSONMEAN|MEANJSON|JSONEQ|JEQ|SAMEJSON|JSONNEQ|JNEQ|JSONSUBSET|JSUBSET|JSONSUPERSET|JSUPERSET|JSONHASALL|JHASALL|JSONHASANY|JHASANY|JSONHITKEY|JSONFIRST|JHITKEY|JSONWHICH|JSONHITVAL|JSONFIRSTVAL|JHITVAL|FIRSTJSONVAL|HITJSONVAL|JSONWHICHVAL|WHICHJSONVAL|PLATEHITVAL|FIRSTVAL|JSONVAL|JSONCOALESCE|JCOALESCE|JSONNVL|JNVL|COALESCEJSON|JSONOR|JSONDEFAULTVAL|JSONMISS|JMISS|JSONMISSING|JSONPRESENT|JPRESENT|JSONFOUND|JSONEXTRA|JEXTRA|JSONUNKNOWN|JSONKNOWN|JKNOWN|JSONALLOWED|JSONPLUCK|JPLUCK|JSONGETALL|JSONGETS|JSONEXACT|JEXACT|EXACTKEYS|JSONSCHEMA|JSONKEYDIFF|KEYSDIFF|JSONKEYCOMM|KEYSINTER|JSONCOMMON|JSONCHANGED|JCHANGED|JSONNEQKEYS|JSONSTABLE|JSTABLE|JSONEQKEYS|JSONDELTA|JDELTA|DELTAOBJ|CHANGEDOBJ|CHAT|ARG|ARGC|ARGS|ARGVALL|NUM|STR|ITOA|LEN|LENALL|MAPLEN|MAXLEN|MINLEN|LONGEST|SHORTEST|COMMONPREFIX|COMMONSUFFIX|STRIPPREFIX|STRIPSUFFIX|STRIPCOMMON|LCP|EMPTY|BLANK|COALESCE|NVL|TIME|MS|MONOTONIC|NPROC|CPUS|CORES|ISATTY|TTY|LOADAVG|LOAD|UPTIME|BOOTAGE|MEM|MEMINFO|DF|DISKFREE|FSYNC|SYNCFILE|TRUNCATE|FTRUNCATE|FILETRUNC|DIRSYNC|FSYNCDIR|SYNCDIR|HARDLINK|HLINK|NLINK|LINKCOUNT|SAMEINODE|EQINODE|INODE|TEE|WRITETEE|FILEUID|FILEGID|OWNEDBY|CANREAD|CANWRITE|CANEXEC|UNAME|OS|ARCH|MACHINE|PLATFORM|SLEEP|RAND|PICK|CHOICE|SHUFFLE|SHUF|DRAWN|SAMPLEK|NPICK|MIN|MAX|ARGMAX|ARGMIN|CLAMP|IN|WITHIN|CMP|SCMP|IABS|SIGN|DIV|MOD|GCD|LCM|POW|ISQRT|SUM|PROD|AVG|MEDIAN|RANGE|SEQ|IOTA|DATE|LOCAL|LOCALTIME|LOCALDATE|PID|HOSTNAME|USER|UID|HOME|APPEND|HEX|TOHEX|ORD|CHR|MID|CAT|FIND|FINDI|NTH|EQS|EQSI|HAS|HASI|BEFORE|AFTER|BETWEEN|REVS|UPPER|LOWER|TRIM|STARTS|STARTSI|ENDS|ENDSI|REPLACE|REPLACEALL|LPAD|RPAD|PADALL|LPADALL|RPADALL|TRUNCALL|CLIPALL|STREPEAT";

/* Fill out with closest SYS op name (or empty). Prefer prefix, then edit dist ≤2. */
void sys_suggest_op(const char *typo, char *out, size_t outn){
  char want[64], cand[64], piece[64], best_name[64];
  const char *p, *bar;
  int best_d = 99, d;
  size_t best_len = 9999;
  int have_prefix = 0;
  out[0] = 0;
  best_name[0] = 0;
  if (!typo || !typo[0] || outn < 2) return;
  sys_fold(want, sizeof want, typo);
  if (!want[0]) return;
  /* exact case-insensitive */
  p = sys_op_suggest_catalog;
  while (*p) {
    size_t plen;
    bar = strchr(p, '|');
    plen = bar ? (size_t)(bar - p) : strlen(p);
    if (plen == 0) { p = bar ? bar + 1 : p + 1; continue; }
    if (plen >= sizeof piece) plen = sizeof piece - 1;
    memcpy(piece, p, plen);
    piece[plen] = 0;
    sys_fold(cand, sizeof cand, piece);
    if (strcmp(want, cand) == 0) {
      snprintf(out, outn, "%s", piece);
      return;
    }
    p = bar ? bar + 1 : p + plen;
  }
  /* prefix match (len≥3) — prefer shortest candidate */
  if (strlen(want) >= 3) {
    p = sys_op_suggest_catalog;
    while (*p) {
      size_t plen, cl;
      bar = strchr(p, '|');
      plen = bar ? (size_t)(bar - p) : strlen(p);
      if (plen == 0) { p = bar ? bar + 1 : p + 1; continue; }
      if (plen >= sizeof piece) plen = sizeof piece - 1;
      memcpy(piece, p, plen);
      piece[plen] = 0;
      sys_fold(cand, sizeof cand, piece);
      cl = strlen(cand);
      if (strncmp(want, cand, strlen(want)) == 0 ||
          (cl >= 3 && strncmp(cand, want, cl) == 0)) {
        if (!have_prefix || cl < best_len) {
          have_prefix = 1;
          best_len = cl;
          snprintf(best_name, sizeof best_name, "%s", piece);
        }
      }
      p = bar ? bar + 1 : p + plen;
    }
    if (have_prefix) {
      snprintf(out, outn, "%s", best_name);
      return;
    }
  }
  /* edit distance ≤2 */
  p = sys_op_suggest_catalog;
  while (*p) {
    size_t plen;
    bar = strchr(p, '|');
    plen = bar ? (size_t)(bar - p) : strlen(p);
    if (plen == 0) { p = bar ? bar + 1 : p + 1; continue; }
    if (plen >= sizeof piece) plen = sizeof piece - 1;
    memcpy(piece, p, plen);
    piece[plen] = 0;
    sys_fold(cand, sizeof cand, piece);
    d = sys_edit_dist(want, cand);
    if (d < best_d) {
      best_d = d;
      snprintf(best_name, sizeof best_name, "%s", piece);
    }
    p = bar ? bar + 1 : p + plen;
  }
  if (best_d <= 2 && best_name[0])
    snprintf(out, outn, "%s", best_name);
}


/* Scan CUBALC_ARG0.. for CLI flag `name` (no leading dashes in name).
 * Matches: --name, -name, --name=value, -name=value, --name value.
 * Returns 0 miss · 1 bare present (bool "1") · 2 value present (out filled).
 * Usability: GETFLAG/HASFLAG without shell getopt glue. */
int cubalc_scan_cli_flag(const char *name, char *out, size_t outn) {
  char envn[32];
  const char *ac;
  int argc = 32, k;
  size_t nlen;
  if (!name || !name[0]) return 0;
  /* strip accidental leading dashes from name */
  while (name[0] == '-') name++;
  if (!name[0]) return 0;
  nlen = strlen(name);
  ac = getenv("CUBALC_ARGC");
  if (ac && ac[0]) {
    argc = (int)strtol(ac, NULL, 10);
    if (argc < 0) argc = 0;
    if (argc > 32) argc = 32;
  } else {
    argc = 0;
    for (k = 0; k < 32; k++) {
      snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
      if (!getenv(envn)) break;
      argc++;
    }
  }
  for (k = 0; k < argc; k++) {
    const char *a, *p;
    size_t al;
    snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
    a = getenv(envn);
    if (!a || a[0] != '-') continue;
    p = a + 1;
    if (*p == '-') p++;
    if (!*p) continue; /* bare -- separator skip */
    al = strlen(p);
    if (al >= nlen && strncmp(p, name, nlen) == 0) {
      if (p[nlen] == '=') {
        if (out && outn)
          snprintf(out, outn, "%s", p + nlen + 1);
        return 2;
      }
      if (p[nlen] == 0) {
        /* --name value (next non-flag) or bare bool */
        if (k + 1 < argc) {
          char env2[32];
          const char *b;
          snprintf(env2, sizeof env2, "CUBALC_ARG%d", k + 1);
          b = getenv(env2);
          if (b && b[0] && b[0] != '-') {
            if (out && outn)
              snprintf(out, outn, "%s", b);
            return 2;
          }
        }
        if (out && outn)
          snprintf(out, outn, "1");
        return 1;
      }
    }
  }
  if (out && outn) out[0] = 0;
  return 0;
}

/* Read flag name aliases: name[,name...][|name...] until OR/DEFAULT or non-name.
 * bag = newline-separated stripped names; first = canonical (first alias).
 * Returns alias count (0 = none). OR stays reserved for defaults — use , or |.
 * Usability: GETFLAGN workers|w OR 4 · BOOLFLAG verbose|v without dual HASFLAG. */
int cubalc_read_flag_aliases(Lex *L, char *bag, size_t bagn,
                                    char *first, size_t firstn) {
  int n = 0;
  if (bag && bagn) bag[0] = 0;
  if (first && firstn) first[0] = 0;
  for (;;) {
    char name[96];
    size_t bl;
    while (L->cur.kind == TK_MINUS) lex_next(L);
    if (L->cur.kind != TK_STR && L->cur.kind != TK_IDENT)
      break;
    /* do not swallow OR/DEFAULT as an alias name after first */
    if (n > 0 && (kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") ||
                  kw(&L->cur, "ELSE") || kw(&L->cur, "FALLBACK")))
      break;
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    while (name[0] == '-')
      memmove(name, name + 1, strlen(name));
    if (!name[0]) {
      if (L->cur.kind == TK_COMMA || L->cur.kind == TK_PIPE) {
        lex_next(L);
        continue;
      }
      break;
    }
    if (n == 0 && first && firstn)
      snprintf(first, firstn, "%s", name);
    if (bag && bagn) {
      bl = strlen(bag);
      if (bl > 0 && bl + 1 < bagn) {
        bag[bl++] = '\n';
        bag[bl] = 0;
      }
      if (bl + strlen(name) < bagn)
        snprintf(bag + bl, bagn - bl, "%s", name);
    }
    n++;
    if (L->cur.kind == TK_COMMA || L->cur.kind == TK_PIPE) {
      lex_next(L);
      continue;
    }
    break;
  }
  return n;
}

/* Scan CLI for any of newline-separated flag names. First hit wins.
 * hit_name gets the matching alias (empty on miss). */
int cubalc_scan_cli_flag_any(const char *names_bag, char *out, size_t outn,
                                    char *hit_name, size_t hitn) {
  const char *p = names_bag;
  if (hit_name && hitn) hit_name[0] = 0;
  if (!p || !p[0]) {
    if (out && outn) out[0] = 0;
    return 0;
  }
  while (*p) {
    char name[96];
    size_t i = 0;
    int hit;
    while (*p && *p != '\n' && i + 1 < sizeof name)
      name[i++] = *p++;
    name[i] = 0;
    if (*p == '\n') p++;
    if (!name[0]) continue;
    hit = cubalc_scan_cli_flag(name, out, outn);
    if (hit) {
      if (hit_name && hitn)
        snprintf(hit_name, hitn, "%s", name);
      return hit;
    }
  }
  if (out && outn) out[0] = 0;
  return 0;
}

/* Collect ALL CLI values for any of names_bag aliases → newline bag.
 * --name=v / -name=v / --name v / bare --name → "1". Multi-occurrence kept.
 * first_hit = first matching alias. Returns field count.
 * Usability: GETFLAGALL tags --tag a --tag b without EACH ARGS. */
int cubalc_scan_cli_flag_all(const char *names_bag, char *out, size_t outn,
                                    char *first_hit, size_t hitn) {
  char envn[32];
  const char *ac;
  int argc = 32, k, count = 0;
  size_t olen = 0;
  if (out && outn) out[0] = 0;
  if (first_hit && hitn) first_hit[0] = 0;
  if (!names_bag || !names_bag[0]) return 0;
  ac = getenv("CUBALC_ARGC");
  if (ac && ac[0]) {
    argc = (int)strtol(ac, NULL, 10);
    if (argc < 0) argc = 0;
    if (argc > 32) argc = 32;
  } else {
    argc = 0;
    for (k = 0; k < 32; k++) {
      snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
      if (!getenv(envn)) break;
      argc++;
    }
  }
  for (k = 0; k < argc; k++) {
    const char *a, *p;
    char alias[96], valbuf[CUBALC_HOST_STR_MAX];
    size_t nlen;
    int matched = 0;
    snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
    a = getenv(envn);
    if (!a || a[0] != '-') continue;
    p = a + 1;
    if (*p == '-') p++;
    if (!*p) continue; /* bare -- */
    /* peel longest matching alias prefix (name or name=val) */
    {
      const char *q = names_bag;
      size_t best = 0;
      alias[0] = 0;
      while (*q) {
        size_t j = 0;
        char cand[96];
        while (q[j] && q[j] != '\n' && j + 1 < sizeof cand) {
          cand[j] = q[j];
          j++;
        }
        cand[j] = 0;
        if (j > 0 && strncmp(p, cand, j) == 0 && (p[j] == 0 || p[j] == '=')) {
          if (j > best) {
            best = j;
            snprintf(alias, sizeof alias, "%s", cand);
            matched = 1;
          }
        }
        q += j;
        if (*q == '\n') q++;
      }
      if (!matched) continue;
      nlen = best;
    }
    valbuf[0] = 0;
    if (p[nlen] == '=') {
      snprintf(valbuf, sizeof valbuf, "%s", p + nlen + 1);
    } else if (p[nlen] == 0) {
      if (k + 1 < argc) {
        char env2[32];
        const char *b;
        snprintf(env2, sizeof env2, "CUBALC_ARG%d", k + 1);
        b = getenv(env2);
        if (b && b[0] && b[0] != '-') {
          snprintf(valbuf, sizeof valbuf, "%s", b);
          k++; /* skip value arg */
        } else {
          snprintf(valbuf, sizeof valbuf, "%s", "1");
        }
      } else {
        snprintf(valbuf, sizeof valbuf, "%s", "1");
      }
    } else
      continue;
    if (count == 0 && first_hit && hitn)
      snprintf(first_hit, hitn, "%s", alias);
    if (out && outn) {
      size_t vl = strlen(valbuf);
      if (olen > 0 && olen + 1 < outn) {
        out[olen++] = '\n';
        out[olen] = 0;
      }
      if (olen + vl < outn) {
        memcpy(out + olen, valbuf, vl + 1);
        olen += vl;
      }
    }
    count++;
  }
  return count;
}

/* Expand bag fields by comma (CSV): "a,b\nc" → "a\nb\nc". Trim spaces around pieces.
 * Returns field count written to out. Empty pieces dropped.
 * Usability: GETFLAGCSV --tags=a,b,c without SYS SPLIT glue. */
int cubalc_csv_expand_bag(const char *in, char *out, size_t outn) {
  const char *p;
  size_t olen = 0;
  int count = 0;
  if (out && outn) out[0] = 0;
  if (!in || !in[0]) return 0;
  p = in;
  while (*p) {
    const char *line_end = p;
    const char *q;
    while (*line_end && *line_end != '\n') line_end++;
    q = p;
    while (q < line_end) {
      const char *cstart, *cend;
      size_t len;
      /* skip spaces before piece */
      while (q < line_end && (*q == ' ' || *q == '\t')) q++;
      if (q >= line_end) break;
      cstart = q;
      while (q < line_end && *q != ',') q++;
      cend = q;
      while (cend > cstart && (cend[-1] == ' ' || cend[-1] == '\t')) cend--;
      len = (size_t)(cend - cstart);
      if (len > 0 && out && outn) {
        if (olen > 0 && olen + 1 < outn) {
          out[olen++] = '\n';
          out[olen] = 0;
        }
        if (olen + len < outn) {
          memcpy(out + olen, cstart, len);
          olen += len;
          out[olen] = 0;
          count++;
        }
      } else if (len > 0)
        count++;
      if (q < line_end && *q == ',') q++;
    }
    if (*line_end == '\n') p = line_end + 1;
    else break;
  }
  return count;
}

/* Truthy CLI / plate strings: 1 true yes y on t → 1;
 * empty 0 false no n off f → 0; other non-empty → 1.
 * Usability: BOOLFLAG / TRUTHY without EQS soup. */
int cubalc_str_truthy(const char *s) {
  if (!s || !s[0]) return 0;
  if (strcasecmp(s, "0") == 0 || strcasecmp(s, "false") == 0 ||
      strcasecmp(s, "no") == 0 || strcasecmp(s, "n") == 0 ||
      strcasecmp(s, "off") == 0 || strcasecmp(s, "f") == 0)
    return 0;
  if (strcasecmp(s, "1") == 0 || strcasecmp(s, "true") == 0 ||
      strcasecmp(s, "yes") == 0 || strcasecmp(s, "y") == 0 ||
      strcasecmp(s, "on") == 0 || strcasecmp(s, "t") == 0)
    return 1;
  return 1; /* other non-empty → truthy */
}

/* Parse one string atom for ONEOF lists: "s" | LAST | string-var.
 * Returns 1 and advances on success. */
int cubalc_read_str_atom(VM *vm, Lex *L, char *out, size_t outn) {
  if (L->cur.kind == TK_STR) {
    snprintf(out, outn, "%s", L->cur.text);
    lex_next(L);
    return 1;
  }
  if (L->cur.kind == TK_IDENT) {
    if (strcmp(L->cur.text, "LAST") == 0) {
      Var *lv = var_get(vm, "LAST", 0);
      if (lv && lv->is_str)
        snprintf(out, outn, "%s", lv->sval);
      else
        snprintf(out, outn, "%s", vm->last_str);
      lex_next(L);
      return 1;
    }
    {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(out, outn, "%s", sv->sval);
        lex_next(L);
        return 1;
      }
    }
    /* bare ident as literal token text (allowed set members like list without quotes) */
    snprintf(out, outn, "%s", L->cur.text);
    lex_next(L);
    return 1;
  }
  if (L->cur.kind == TK_NUM) {
    snprintf(out, outn, "%ld", (long)L->cur.num);
    lex_next(L);
    return 1;
  }
  return 0;
}

/* Match subject against multi-alias list (comma / OR / |). At least one atom.
 * On match sets hit_out. Returns 1 match · 0 no match · -1 parse error. */
int cubalc_match_oneof_list(VM *vm, Lex *L, const char *subject, int icase,
                                   char *hit_out, size_t hit_n) {
  char atom[512];
  int matched = 0;
  int any = 0;
  if (hit_out && hit_n) hit_out[0] = 0;
  for (;;) {
    atom[0] = 0;
    if (!cubalc_read_str_atom(vm, L, atom, sizeof atom)) {
      if (!any) return -1;
      break;
    }
    any = 1;
    if (subject) {
      int eq = icase ? (strcasecmp(subject, atom) == 0) : (strcmp(subject, atom) == 0);
      if (eq) {
        matched = 1;
        if (hit_out && hit_n && !hit_out[0])
          snprintf(hit_out, hit_n, "%s", atom);
      }
    }
    if (L->cur.kind == TK_COMMA) { lex_next(L); continue; }
    if (L->cur.kind == TK_PIPE) { lex_next(L); continue; }
    if (kw(&L->cur, "OR")) { lex_next(L); continue; }
    break;
  }
  return matched ? 1 : 0;
}
/* Read numeric atom for INRANGE/REQUIRE BETWEEN: NUM | LAST_N | LAST | var | expr prim.
 * Returns 1 and advances. Does not swallow OR (no parse_expr boolean). */
int cubalc_read_num_atom(VM *vm, Lex *L, long *out) {
  if (!out) return 0;
  if (L->cur.kind == TK_NUM) {
    *out = L->cur.num;
    lex_next(L);
    return 1;
  }
  if (L->cur.kind == TK_MINUS) {
    lex_next(L);
    if (L->cur.kind == TK_NUM) {
      *out = -L->cur.num;
      lex_next(L);
      return 1;
    }
    return 0;
  }
  if (L->cur.kind == TK_PLUS) {
    lex_next(L);
    if (L->cur.kind == TK_NUM) {
      *out = L->cur.num;
      lex_next(L);
      return 1;
    }
    return 0;
  }
  if (L->cur.kind == TK_IDENT) {
    if (strcmp(L->cur.text, "LAST_N") == 0 || strcmp(L->cur.text, "N") == 0) {
      *out = vm->last_n;
      lex_next(L);
      return 1;
    }
    if (strcmp(L->cur.text, "LAST") == 0) {
      Var *lv = var_get(vm, "LAST", 0);
      if (lv && !lv->is_str) *out = lv->val;
      else if (lv && lv->is_str) *out = strtol(lv->sval, NULL, 10);
      else *out = strtol(vm->last_str, NULL, 10);
      lex_next(L);
      return 1;
    }
    {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && !sv->is_str) {
        *out = sv->val;
        lex_next(L);
        return 1;
      }
      if (sv && sv->is_str) {
        *out = strtol(sv->sval, NULL, 10);
        lex_next(L);
        return 1;
      }
    }
    return 0;
  }
  if (L->cur.kind == TK_STR) {
    *out = strtol(L->cur.text, NULL, 10);
    lex_next(L);
    return 1;
  }
  if (L->cur.kind == TK_LPAREN) {
    *out = parse_expr(vm, L);
    return 1;
  }
  return 0;
}



/* Collect non-flag positionals from CUBALC_ARGn → newline bag.
 * Skips --flag / -flag / --flag=val; bare --flag val skips value too
 * (same rule as GETFLAG). Bare "-" kept. Bare "--" ends flag scan.
 * Usability: RESTARGS after mixed --flags + files without shell. */
long cubalc_collect_restargs(char *out, size_t outn) {
  char envn[32];
  const char *ac;
  int argc = 32, k;
  size_t o = 0;
  long count = 0;
  int end_flags = 0;
  if (out && outn) out[0] = 0;
  ac = getenv("CUBALC_ARGC");
  if (ac && ac[0]) {
    argc = (int)strtol(ac, NULL, 10);
    if (argc < 0) argc = 0;
    if (argc > 32) argc = 32;
  } else {
    argc = 0;
    for (k = 0; k < 32; k++) {
      snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
      if (!getenv(envn)) break;
      argc++;
    }
  }
  for (k = 0; k < argc; k++) {
    const char *a;
    size_t al, room;
    snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
    a = getenv(envn);
    if (!a || !a[0]) continue;
    if (!end_flags && a[0] == '-' && a[1] != 0) {
      const char *p;
      /* bare "--" → remaining args are all positionals */
      if (a[1] == '-' && a[2] == 0) {
        end_flags = 1;
        continue;
      }
      p = a + 1;
      if (*p == '-') p++;
      if (strchr(p, '='))
        continue; /* --name=value */
      /* --name [value] — consume next non-flag as value (GETFLAG parity) */
      if (k + 1 < argc) {
        char env2[32];
        const char *b;
        snprintf(env2, sizeof env2, "CUBALC_ARG%d", k + 1);
        b = getenv(env2);
        if (b && b[0] && !(b[0] == '-' && b[1] != 0))
          k++;
      }
      continue;
    }
    /* positional (includes bare "-" stdin convention) */
    al = strlen(a);
    if (out && outn > 1) {
      if (count > 0 && o + 1 < outn)
        out[o++] = '\n';
      room = outn - o - 1;
      if (al > room) al = room;
      if (al > 0) {
        memcpy(out + o, a, al);
        o += al;
      }
      out[o] = 0;
    }
    count++;
  }
  return count;
}

/* Collect CLI flag names (no leading dashes) from CUBALC_ARGn → newline bag.
 * --mode=fast / --verbose / -n 3 → "mode\nverbose\nn". Skips bare "--".
 * Value tokens after bare --name are not listed. LAST_N = flag count.
 * Usability: LISTFLAGS discovery without EACH ARGS + STARTS glue. */
/* Rest positionals after first (subcommand slot). Returns count remaining;
 * writes newline bag of files after cmd. Empty if 0–1 positionals total.
 * Usability: REQUIRE RESTPOS / HASRESTPOS after SUBCMD convention. */
long cubalc_collect_restpos(char *out, size_t outn) {
  char bag[CUBALC_HOST_STR_MAX];
  long rest_n = 0;
  const char *p, *start;
  size_t ro = 0;
  if (out && outn) out[0] = 0;
  (void)cubalc_collect_restargs(bag, sizeof bag);
  p = bag;
  /* skip first field (subcommand) */
  if (!*p) return 0;
  while (*p && *p != '\n') p++;
  if (*p == '\n') p++;
  else return 0;
  while (*p) {
    start = p;
    while (*p && *p != '\n') p++;
    {
      size_t fl = (size_t)(p - start);
      if (fl > 0) {
        if (out && outn && ro + fl + 2 < outn) {
          if (ro) out[ro++] = '\n';
          memcpy(out + ro, start, fl);
          ro += fl;
          out[ro] = 0;
        }
        rest_n++;
      }
    }
    if (*p == '\n') p++;
  }
  return rest_n;
}

long cubalc_collect_listflags(char *out, size_t outn) {
  char envn[32];
  const char *ac;
  int argc = 32, k;
  size_t o = 0;
  long count = 0;
  int end_flags = 0;
  if (out && outn) out[0] = 0;
  ac = getenv("CUBALC_ARGC");
  if (ac && ac[0]) {
    argc = (int)strtol(ac, NULL, 10);
    if (argc < 0) argc = 0;
    if (argc > 32) argc = 32;
  } else {
    argc = 0;
    for (k = 0; k < 32; k++) {
      snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
      if (!getenv(envn)) break;
      argc++;
    }
  }
  for (k = 0; k < argc; k++) {
    const char *a, *p, *eq;
    char name[96];
    size_t nlen, room;
    snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
    a = getenv(envn);
    if (!a || !a[0]) continue;
    if (end_flags) break;
    if (!(a[0] == '-' && a[1] != 0))
      continue; /* positional */
    if (a[1] == '-' && a[2] == 0) {
      end_flags = 1;
      continue; /* bare -- */
    }
    p = a + 1;
    if (*p == '-') p++;
    eq = strchr(p, '=');
    if (eq) {
      nlen = (size_t)(eq - p);
      if (nlen >= sizeof name) nlen = sizeof name - 1;
      memcpy(name, p, nlen);
      name[nlen] = 0;
    } else {
      snprintf(name, sizeof name, "%s", p);
      /* consume value token for bare --name val (GETFLAG parity) */
      if (k + 1 < argc) {
        char env2[32];
        const char *b;
        snprintf(env2, sizeof env2, "CUBALC_ARG%d", k + 1);
        b = getenv(env2);
        if (b && b[0] && !(b[0] == '-' && b[1] != 0))
          k++;
      }
    }
    if (!name[0]) continue;
    nlen = strlen(name);
    if (out && outn > 1) {
      if (count > 0 && o + 1 < outn)
        out[o++] = '\n';
      room = outn - o - 1;
      if (nlen > room) nlen = room;
      if (nlen > 0) {
        memcpy(out + o, name, nlen);
        o += nlen;
      }
      out[o] = 0;
    }
    count++;
  }
  return count;
}

/* Collect CLI flags as name=value bag for agent plates.
 * --mode=fast → mode=fast; bare --verbose → verbose=1 (does NOT consume next
 * token — keeps files after bool flags for RESTARGS/FLAGMAP coexistence).
 * Prefer --name=value for non-bool values (GETFLAG still supports space form).
 * Usability: FLAGMAP + SYS LOOKUP sep "=" without EACH GETFLAG. */
long cubalc_collect_flagmap(char *out, size_t outn) {
  char envn[32];
  const char *ac;
  int argc = 32, k;
  size_t o = 0;
  long count = 0;
  int end_flags = 0;
  if (out && outn) out[0] = 0;
  ac = getenv("CUBALC_ARGC");
  if (ac && ac[0]) {
    argc = (int)strtol(ac, NULL, 10);
    if (argc < 0) argc = 0;
    if (argc > 32) argc = 32;
  } else {
    argc = 0;
    for (k = 0; k < 32; k++) {
      snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
      if (!getenv(envn)) break;
      argc++;
    }
  }
  for (k = 0; k < argc; k++) {
    const char *a, *p, *eq;
    char name[96], val[CUBALC_HOST_STR_MAX], line[CUBALC_HOST_STR_MAX];
    size_t llen, room;
    snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
    a = getenv(envn);
    if (!a || !a[0]) continue;
    if (end_flags) break;
    if (!(a[0] == '-' && a[1] != 0))
      continue;
    if (a[1] == '-' && a[2] == 0) {
      end_flags = 1;
      continue;
    }
    p = a + 1;
    if (*p == '-') p++;
    eq = strchr(p, '=');
    if (eq) {
      size_t nlen = (size_t)(eq - p);
      if (nlen >= sizeof name) nlen = sizeof name - 1;
      memcpy(name, p, nlen);
      name[nlen] = 0;
      snprintf(val, sizeof val, "%s", eq + 1);
    } else {
      /* bare --name → bool "1"; do not steal following positionals */
      snprintf(name, sizeof name, "%s", p);
      snprintf(val, sizeof val, "1");
    }
    if (!name[0]) continue;
    snprintf(line, sizeof line, "%s=%s", name, val);
    llen = strlen(line);
    if (out && outn > 1) {
      if (count > 0 && o + 1 < outn)
        out[o++] = '\n';
      room = outn - o - 1;
      if (llen > room) llen = room;
      if (llen > 0) {
        memcpy(out + o, line, llen);
        o += llen;
      }
      out[o] = 0;
    }
    count++;
  }
  return count;
}

/* Collect CUBALC_ARG0.. as newline bag of "i=value" (raw argv, flags included).
 * LAST_N / ARGMAP_N = argc fields. Twin of FLAGMAP for full argv LOOKUP by index.
 * Usability: SYS LOOKUP LAST "0" sep "=" without EACH SYS ARG / NTHPOS glue. */
long cubalc_collect_argmap(char *out, size_t outn) {
  char envn[32];
  const char *ac, *a;
  int argc = 32, k;
  size_t o = 0;
  long count = 0;
  if (out && outn) out[0] = 0;
  ac = getenv("CUBALC_ARGC");
  if (ac && ac[0]) {
    argc = (int)strtol(ac, NULL, 10);
    if (argc < 0) argc = 0;
    if (argc > 32) argc = 32;
  } else {
    argc = 0;
    for (k = 0; k < 32; k++) {
      snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
      if (!getenv(envn)) break;
      argc++;
    }
  }
  for (k = 0; k < argc; k++) {
    char line[CUBALC_HOST_STR_MAX];
    size_t llen, room;
    snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
    a = getenv(envn);
    if (!a) a = "";
    snprintf(line, sizeof line, "%d=%s", k, a);
    llen = strlen(line);
    if (out && outn > 1) {
      if (count > 0 && o + 1 < outn)
        out[o++] = '\n';
      room = outn - o - 1;
      if (llen > room) llen = room;
      if (llen > 0) {
        memcpy(out + o, line, llen);
        o += llen;
      }
      out[o] = 0;
    }
    count++;
  }
  return count;
}

/* Append sticky USAGE var to REQUIRE fail messages (room-limited).
 * Usability: agents set USAGE once; ARG/ARGC/FLAG fails show the contract. */
void cubalc_append_usage_tip(VM *vm, char *msg, size_t n) {
  Var *u;
  char tmp[220];
  size_t ml, ul, room;
  if (!vm || !msg || n < 24) return;
  u = var_get(vm, "USAGE", 0);
  if (!u || !u->is_str || !u->sval[0]) return;
  ml = strlen(msg);
  if (ml + 12 >= n) return;
  room = n - ml - 1;
  ul = strlen(u->sval);
  if (ul + 10 <= room)
    snprintf(tmp, sizeof tmp, "%s · usage: %s", msg, u->sval);
  else {
    size_t take = room > 12 ? room - 10 : 0;
    if (take < 6) return;
    snprintf(tmp, sizeof tmp, "%s · usage: %.*s…", msg, (int)take, u->sval);
  }
  snprintf(msg, n, "%s", tmp);
}

/* Format ASSERT/EXPECT failure with got/expected from last parse_cmp. */
