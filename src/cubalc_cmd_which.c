/* CubalC CLI which plane — brace-accurate peel from main */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_which.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

/* local helpers used by peeled CLI (from main) */
static void ensure_dir(const char *d) { mkdir(d, 0755); }

int cubalc_cmd_which(int argc, char **argv) {
  const char *cmd = (argc > 1) ? argv[1] : "which";
  if (strcmp(cmd, "which") == 0 || strcmp(cmd, "locate") == 0 ||
      strcmp(cmd, "resolve") == 0) {
    /* Usability: resolve short name → path/kind for agents (INCLUDE/cat/paths).
     * order: exact path · lib · program · proof · p2p · protect · doc alias ·
     * form catalog · PATH binary. Multi-match plate cubalc.which.v1. */
    const char *q = (argc > 2) ? argv[2] : "";
    struct {
      char kind[16];
      char path[512];
      char hint[96];
      int present;
    } hits[16];
    int nh = 0, i;
    char stem[256];
    size_t blen;
    static const struct { const char *name; const char *path; const char *hint; } aliases[] = {
      {"cookbook", "docs/COOKBOOK.md", "hold → plug → smx recipes"},
      {"agents", "docs/FOR_AGENTS.md", "agent prompt snippet"},
      {"hold_flash", "docs/HOLD_FLASH.md", "device/mesh-join safeguard only"},
      {"p2p_doc", "docs/P2P_SMX.md", "binary mesh wire"},
      {"hello", "programs/hello_cube.cubalc", "minimal starter"},
      {"lib", "programs/lib", "INCLUDE short-name stdlib dir"},
      {"proof", "programs/proof", "usability + ISA proofs dir"},
    };
    static const struct { const char *name; const char *plane; const char *hint; } forms[] = {
      {"HOLD_FLASH", "law", "device/mesh-join only · default 1 · not language PLUG"},
      {"CUBE", "core", "place cube"},
      {"PLUG", "core", "wire cubes · denied only if HOLD_FLASH 0"},
      {"INCLUDE", "flow", "INCLUDE [ONCE][SOFT] path|MATCH|NEWEST|OLDEST|ALL MATCH needle"},
      {"LISTINCLUDES", "flow", "LISTINCLUDES|LOADED bag of included paths · INCLUDE_N"},
      {"INCLUDESTEMS", "flow", "INCLUDESTEMS short-name bag from loaded modules"},
      {"LISTPRELOAD", "flow", "LISTPRELOAD -I preload short-name bag"},
      {"PRELOADMISS", "flow", "PRELOADMISS -I names not loaded bag"},
      {"PRELOADOK", "flow", "PRELOADOK soft all -I loaded"},
      {"NEEDPRELOAD", "flow", "NEEDPRELOAD fail-fast -I not loaded"},
      {"HASPRELOAD", "flow", "HASPRELOAD name soft 0|1 in -I bag"},
      {"HASINCLUDE", "flow", "HASINCLUDE name soft 0|1 if module loaded"},
      {"HASINCLUDEALL", "flow", "HASINCLUDEALL a b soft all loaded"},
      {"NEEDINCLUDE", "flow", "NEEDINCLUDE a b fail-fast not loaded"},
      {"DEFAULT", "flow", "DEFAULT name = value if unset"},
      {"DEFINED", "flow", "DEFINED name → LAST_N 0|1"},
      {"TYPEOF", "flow", "TYPEOF name → undef|num|str"},
      {"UNSET", "flow", "UNSET name remove var"},
      {"ASSERT", "flow", "ASSERT expr [why] — string ==/!= content compare"},
      {"EXPECT", "flow", "EXPECT soft check · no fatal"},
      {"FAIL", "flow", "FAIL soft status OK=0"},
      {"PASS", "flow", "PASS soft status OK=1"},
      {"NOTE", "flow", "NOTE agent breadcrumb"},
      {"EXIT", "flow", "EXIT [code] halt program"},
      {"CLEAR_ERR", "flow", "CLEAR_ERR wipe sticky LAST_ERR"},
      {"VERSION", "flow", "VERSION → LAST language version"},
      {"PRINT_JSON", "flow", "one JSON line for agents"},
      {"VARS", "flow", "VARS full var table JSON"},
      {"VARROOM", "flow", "VARROOM free var slots → LAST_N"},
      {"HASVARROOM", "flow", "HASVARROOM n soft 0|1 if room >= n"},
      {"NEEDVARROOM", "flow", "NEEDVARROOM n fail-fast if room < n"},
      {"REMAIN_MS", "flow", "REMAIN_MS|BUDGETLEFT wall budget left ms · -1 unlimited"},
      {"HAS_TIME", "flow", "HAS_TIME n soft 0|1 if remaining >= n"},
      {"NEEDTIME", "flow", "NEEDTIME n fail-fast if wall remaining < n ms"},
      {"WALL_MS", "flow", "WALL_MS|ELAPSED mono ms since run start → LAST_N"},
      {"STATUS", "flow", "STATUS agent health plate"},
      {"IDENTITY", "flow", "IDENTITY user@host:pid plate"},
      {"SYS PID", "host", "SYS PID process id"},
      {"SYS HOSTNAME", "host", "SYS HOSTNAME machine name"},
      {"SYS USER", "host", "SYS USER login name"},
      {"SYS GETLOGIN", "host", "SYS GETLOGIN|LOGINNAME session login name"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS UID", "host", "SYS UID|RUID real user id → LAST_N"},
{"SYS EUID", "host", "SYS EUID|GETEUID effective user id → LAST_N"},
{"SYS ISROOT", "host", "SYS ISROOT|AMROOT euid==0 privilege probe"},
      {"SYS HOME", "host", "SYS HOME home directory"},
      {"SYS SHELL", "host", "SYS SHELL|LOGINSHELL login shell path"},
      {"SYS GECOS", "host", "SYS GECOS|FULLNAME passwd display name"},
      {"SYS TTYNAME", "host", "SYS TTYNAME|CTTY terminal device path"},
      {"SYS NICE", "host", "SYS NICE|RENICE process nice get/set"},
      {"SYS SHQUOTE", "host", "SYS SHQUOTE|SHELLQUOTE POSIX shell-safe wrap"},
      {"SYS SHUNQUOTE", "host", "SYS SHUNQUOTE|UNQUOTE peel POSIX single-quote wrap"},
      {"SYS GETLOGIN", "host", "SYS GETLOGIN|LOGINNAME session login name"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM random bytes as hex"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS DATE", "host", "SYS DATE UTC ISO stamp"},
      {"SYS FROMTIME", "host", "SYS FROMTIME|EPOCHISO|TOISO [n] — epoch → UTC ISO stamp"},
      {"SYS PARSEISO", "host", "SYS PARSEISO|TOEPOCH|FROMISO [str] — ISO → epoch dual of FROMTIME"},
      {"SYS TIMEDIFF", "host", "SYS TIMEDIFF|ELAPSED a [b] — epoch delta seconds a-b or now-a"},
      {"SYS ADDTIME", "host", "SYS ADDTIME|DEADLINE — epoch + seconds / now+secs lease"},
      {"SYS EXPIRED", "host", "SYS EXPIRED|REMAINING [epoch] — lease due probe / TTL secs"},
      {"SYS LOCAL", "host", "SYS LOCAL|LOCALTIME local wall stamp"},
      {"SYS LOCALTIME", "host", "SYS LOCALTIME alias of SYS LOCAL"},
      {"SYS LOCALDATE", "host", "SYS LOCALDATE local date-only"},
      {"SYS TMP", "host", "SYS TMP portable temp dir"},
      {"SYS MKTEMP", "host", "SYS MKTEMP unique empty file under TMP"},
      {"SYS TEMPFILE", "host", "SYS TEMPFILE alias of SYS MKTEMP"},
      {"SYS MKTEMPDIR", "host", "SYS MKTEMPDIR unique empty dir under TMP"},
      {"SYS MKDTEMP", "host", "SYS MKDTEMP alias of SYS MKTEMPDIR"},
      {"SYS MKDIR", "host", "SYS MKDIR mkdir -p"},
      {"SYS BASENAME", "host", "SYS BASENAME path leaf"},
      {"SYS DIRNAME", "host", "SYS DIRNAME path parent"},
      {"SYS EXTNAME", "host", "SYS EXTNAME path .ext"},
      {"SYS STEM", "host", "SYS STEM path without ext"},
      {"SYS SIZE", "host", "SYS SIZE file bytes"},
      {"SYS STAT", "host", "SYS STAT one-shot path metadata"},
      {"SYS FSTAT", "host", "SYS FSTAT alias of SYS STAT"},
      {"SYS ISDIR", "host", "SYS ISDIR dir probe"},
      {"SYS ISFILE", "host", "SYS ISFILE file probe"},
      {"SYS READ", "host", "SYS READ OR soft optional plate"},
      {"SYS LOADPLATE", "host", "SYS LOADPLATE|READPLATE path [OR defaults] soft plate file"},
      {"SYS SAVEPLATE", "host", "SYS SAVEPLATE|STOREPLATE path [plate] object plate persist"},
      {"SYS ENSUREPLATE", "host", "SYS ENSUREPLATE|TOUCHPLATE path [OR defaults] create-or-load"},
      {"SYS UPDATEPLATE", "host", "SYS UPDATEPLATE|JSONFILESET path key value load-set-save plate field"},
      {"SYS BUMPPLATE", "host", "SYS BUMPPLATE|JSONFILEINC path key [delta] load-inc-save counter"},
      {"SYS PEEKPLATE", "host", "SYS PEEKPLATE|JSONFILEGET path key [OR fb] peel plate file field"},
      {"SYS DROPPLATE", "host", "SYS DROPPLATE|JSONFILEDEL path key load-del-save plate field"},
      {"SYS JSONFILEMERGE", "host", "SYS JSONFILEMERGE|OVERLAYPLATE path overlay multi-key load-merge-save"},
      {"SYS JSONFILEENSURE", "host", "SYS JSONFILEENSURE|ENSUREPLATEKEY path key value set-if-missing"},
      {"SYS JSONFILEDEFAULTS", "host", "SYS JSONFILEDEFAULTS|DEFAULTPLATEALL path defaults multi-key fill-if-missing"},
      {"SYS JSONFILETOGGLE", "host", "SYS JSONFILETOGGLE|TOGGLEPLATEKEY path key flip disk flag 0↔1"},
      {"SYS RM", "host", "SYS RM remove plate file"},
      {"SYS RMDIR", "host", "SYS RMDIR remove empty directory"},
      {"SYS REMOVEDIR", "host", "SYS REMOVEDIR alias of SYS RMDIR"},
      {"SYS RENAME", "host", "SYS RENAME move plate path"},
      {"SYS COPY", "host", "SYS COPY duplicate plate file"},
      {"SYS REALPATH", "host", "SYS REALPATH absolute path"},
      {"SYS TOUCH", "host", "SYS TOUCH plate marker"},
      {"SYS SLEEP", "host", "SYS SLEEP n|duration — ms or 1s/250ms human duration"},
      {"SYS WAITFILE", "host", "SYS WAITFILE path [ms|duration] poll until exists"},
      {"SYS WAITPATH", "host", "SYS WAITPATH alias of SYS WAITFILE"},
      {"SYS WAITGONE", "host", "SYS WAITGONE poll until path does not exist"},
      {"SYS WAITMISSING", "host", "SYS WAITMISSING alias of SYS WAITGONE"},
      {"SYS WAITSTABLE", "host", "SYS WAITSTABLE poll until size+mtime quiet"},
      {"SYS WAITQUIET", "host", "SYS WAITQUIET alias of SYS WAITSTABLE"},
      {"SYS WAITCHANGED", "host", "SYS WAITCHANGED poll until size/mtime change"},
      {"SYS WAITMODIFIED", "host", "SYS WAITMODIFIED alias of SYS WAITCHANGED"},
      {"SYS WAITMATCH", "host", "SYS WAITMATCH poll until file contains needle"},
      {"SYS WAITCONTAINS", "host", "SYS WAITCONTAINS alias of SYS WAITMATCH"},
      {"SYS RAND", "host", "SYS RAND uniform int jitter"},
      {"SYS ENTROPY", "host", "SYS ENTROPY|URANDOM hex random bytes"},
      {"SYS PARSEMS", "host", "SYS PARSEMS|DURATION [str] — 5s/1h30m compound → ms"},
      {"SYS MS2SEC", "host", "SYS MS2SEC|SEC2MS — ms↔seconds unit bridge for leases/SLEEP"},
      {"SYS HUMANMS", "host", "SYS HUMANMS|FMTMS [n] — ms → 5s/1m30s dual of PARSEMS"},
      {"SYS RANDOM", "host", "SYS RANDOM alias of SYS RAND"},
      {"SYS PICK", "host", "SYS PICK random bag field"},
      {"SYS CHOICE", "host", "SYS CHOICE alias of SYS PICK"},
      {"SYS SHUFFLE", "host", "SYS SHUFFLE randomize bag order"},
      {"SYS SHUF", "host", "SYS SHUF alias of SYS SHUFFLE"},
      {"SYS DRAWN", "host", "SYS DRAWN sample k unique bag fields"},
      {"SYS SAMPLEK", "host", "SYS SAMPLEK alias of SYS DRAWN"},
      {"SYS NPICK", "host", "SYS NPICK alias of SYS DRAWN"},
      {"SYS TAKERAND", "host", "SYS TAKERAND alias of SYS DRAWN"},
      {"SYS MIN", "host", "SYS MIN host-plane min args or bag"},
      {"SYS MAX", "host", "SYS MAX host-plane max args or bag"},
      {"SYS MINBAG", "host", "SYS MINBAG bag-mode MIN"},
      {"SYS MAXBAG", "host", "SYS MAXBAG bag-mode MAX"},
      {"SYS ARGMAX", "host", "SYS ARGMAX index of first max"},
      {"SYS ARGMIN", "host", "SYS ARGMIN index of first min"},
      {"SYS MAXIDX", "host", "SYS MAXIDX alias of SYS ARGMAX"},
      {"SYS MINIDX", "host", "SYS MINIDX alias of SYS ARGMIN"},
      {"SYS PADALL", "host", "SYS PADALL right-pad every bag field"},
      {"SYS LPADALL", "host", "SYS LPADALL left-pad every bag field"},
      {"SYS RPADALL", "host", "SYS RPADALL alias of PADALL"},
      {"SYS TRUNCALL", "host", "SYS TRUNCALL truncate bag fields to width"},
      {"SYS CLIPALL", "host", "SYS CLIPALL alias of SYS TRUNCALL"},
      {"SYS MAPTRUNC", "host", "SYS MAPTRUNC alias of SYS TRUNCALL"},
      {"SYS CLAMP", "host", "SYS CLAMP bound into range"},
      {"SYS IN", "host", "SYS IN numeric range membership"},
      {"SYS WITHIN", "host", "SYS WITHIN alias of SYS IN"},
      {"SYS CMP", "host", "SYS CMP three-way numeric compare"},
      {"SYS SCMP", "host", "SYS SCMP string compare"},
      {"SYS IABS", "host", "SYS IABS integer absolute value"},
      {"SYS SIGN", "host", "SYS SIGN signum -1|0|1"},
      {"SYS DIV", "host", "SYS DIV integer divide"},
      {"SYS MOD", "host", "SYS MOD remainder"},
      {"SYS GCD", "host", "SYS GCD multi-arg gcd"},
      {"SYS LCM", "host", "SYS LCM multi-arg lcm"},
      {"SYS POW", "host", "SYS POW integer power"},
      {"SYS ISQRT", "host", "SYS ISQRT floor integer sqrt"},
      {"SYS SUM", "host", "SYS SUM args|bag integer sum"},
      {"SYS PROD", "host", "SYS PROD args|bag integer product"},
      {"SYS AVG", "host", "SYS AVG args|bag integer mean"},
      {"SYS MEDIAN", "host", "SYS MEDIAN args|bag integer median"},
      {"SYS RANGE", "host", "SYS RANGE inclusive number bag"},
      {"SYS SEQ", "host", "SYS SEQ 1..n number bag"},
      {"SYS IOTA", "host", "SYS IOTA 0..n-1 number bag"},
      {"SYS LIST", "host", "SYS LIST directory basenames"},
      {"SYS GLOB", "host", "SYS GLOB|MATCHFILES path [pattern] — match basenames"},
      {"SYS MATCHFILES", "host", "SYS MATCHFILES alias of SYS GLOB"},
      {"SYS PATHGLOB", "host", "SYS PATHGLOB|PGLOB full paths matching pattern"},
      {"SYS PGLOB", "host", "SYS PGLOB alias of SYS PATHGLOB"},
      {"SYS FILTERGLOB", "host", "SYS FILTERGLOB|MATCHBAG shell-pattern bag filter"},
      {"SYS MATCHBAG", "host", "SYS MATCHBAG alias of SYS FILTERGLOB"},
      {"SYS BASENAMEALL", "host", "SYS BASENAMEALL|MAPBASE path bag map"},
      {"SYS DIRNAMEALL", "host", "SYS DIRNAMEALL|MAPDIR path bag map"},
      {"SYS KEEPFILES", "host", "SYS KEEPFILES path bag keep regular files"},
      {"SYS KEEPDIRS", "host", "SYS KEEPDIRS path bag keep directories"},
      {"SYS SIZEALL", "host", "SYS SIZEALL|MAPSIZE path bag byte sizes"},
      {"SYS MAPSIZE", "host", "SYS MAPSIZE alias of SYS SIZEALL"},
      {"SYS MTIMEALL", "host", "SYS MTIMEALL|MAPMTIME epoch mtime bag"},
      {"SYS AGEALL", "host", "SYS AGEALL|MAPAGE age-seconds bag"},
      {"SYS NEWEST", "host", "SYS NEWEST|LATEST freshest path by mtime"},
      {"SYS OLDEST", "host", "SYS OLDEST|EARLIEST oldest path by mtime"},
      {"SYS LARGEST", "host", "SYS LARGEST largest file path"},
      {"SYS SORTMTIME", "host", "SYS SORTMTIME order paths by mtime"},
      {"SYS SORTSIZE", "host", "SYS SORTSIZE order paths by size"},
      {"SYS FRESH", "host", "SYS FRESH bag max_age keep fresh paths"},
      {"SYS KEEPSTALE", "host", "SYS KEEPSTALE|AGED bag min_age keep stale paths"},
      {"SYS KEEPNEWER", "host", "SYS KEEPNEWER|NEWERTHAN [bag] ref mtime>ref"},
      {"SYS NEWERTHAN", "host", "SYS NEWERTHAN alias of SYS KEEPNEWER"},
      {"SYS KEEPOLDER", "host", "SYS KEEPOLDER|OLDERREF [bag] ref mtime<ref"},
      {"SYS OLDERREF", "host", "SYS OLDERREF alias of SYS KEEPOLDER"},
      {"SYS KEEPBIGGER", "host", "SYS KEEPBIGGER|BIGFILES|SIZEGE bag min_bytes size>=min"},
      {"SYS BIGFILES", "host", "SYS BIGFILES alias of SYS KEEPBIGGER"},
      {"SYS SIZEGE", "host", "SYS SIZEGE alias of SYS KEEPBIGGER"},
      {"SYS KEEPSMALLER", "host", "SYS KEEPSMALLER|SMALLFILES|SIZELE bag max_bytes size<=max"},
      {"SYS SMALLFILES", "host", "SYS SMALLFILES alias of SYS KEEPSMALLER"},
      {"SYS SIZELE", "host", "SYS SIZELE alias of SYS KEEPSMALLER"},
      {"SYS RMALL", "host", "SYS RMALL|UNLINKALL bulk remove path bag"},
      {"SYS UNLINKALL", "host", "SYS UNLINKALL alias of SYS RMALL"},
      {"SYS DELETEALL", "host", "SYS DELETEALL alias of SYS RMALL"},
      {"SYS TOUCHALL", "host", "SYS TOUCHALL|ENSUREALL bulk create/refresh path bag"},
      {"SYS ENSUREALL", "host", "SYS ENSUREALL alias of SYS TOUCHALL"},
      {"SYS CREATEALL", "host", "SYS CREATEALL alias of SYS TOUCHALL"},
      {"SYS COPYALL", "host", "SYS COPYALL|CPALL bulk copy path bag into dest"},
      {"SYS CPALL", "host", "SYS CPALL alias of SYS COPYALL"},
      {"SYS BULKCOPY", "host", "SYS BULKCOPY alias of SYS COPYALL"},
      {"SYS MKDIRALL", "host", "SYS MKDIRALL|ENSUREDIRS bulk mkdir -p path bag"},
      {"SYS ENSUREDIRS", "host", "SYS ENSUREDIRS alias of SYS MKDIRALL"},
      {"SYS MKDIRS", "host", "SYS MKDIRS alias of SYS MKDIRALL"},
      {"SYS MOVEALL", "host", "SYS MOVEALL|MVALL bulk move path bag into dest"},
      {"SYS MVALL", "host", "SYS MVALL alias of SYS MOVEALL"},
      {"SYS RENAMEALL", "host", "SYS RENAMEALL alias of SYS MOVEALL"},
      {"SYS WALK", "host", "SYS WALK|FINDALL recursive full-path discovery"},
      {"SYS FINDALL", "host", "SYS FINDALL alias of SYS WALK"},
      {"SYS TREEGLOB", "host", "SYS TREEGLOB alias of SYS WALK"},
      {"SYS EQFILE", "host", "SYS EQFILE|SAMEFILE identical file content"},
      {"SYS SAMEFILE", "host", "SYS SAMEFILE alias of SYS EQFILE"},
      {"SYS CMPFILE", "host", "SYS CMPFILE alias of SYS EQFILE"},
      {"SYS LOGALL", "host", "SYS LOGALL|APPENDFILES append line to path bag"},
      {"SYS APPENDFILES", "host", "SYS APPENDFILES alias of SYS LOGALL"},
      {"SYS BULKAPPEND", "host", "SYS BULKAPPEND alias of SYS LOGALL"},
      {"SYS GREPFILES", "host", "SYS GREPFILES|SEARCHFILES filter paths by content"},
      {"SYS SEARCHFILES", "host", "SYS SEARCHFILES alias of SYS GREPFILES"},
      {"SYS FILESGREP", "host", "SYS FILESGREP alias of SYS GREPFILES"},
      {"SYS GREPFILESI", "host", "SYS GREPFILESI case-insensitive content filter"},
      {"SYS GREPVFILES", "host", "SYS GREPVFILES invert content path filter"},
      {"SYS READALL", "host", "SYS READALL|CATFILES concat path bag file contents"},
      {"SYS CATFILES", "host", "SYS CATFILES alias of SYS READALL"},
      {"SYS SLURPALL", "host", "SYS SLURPALL alias of SYS READALL"},
      {"SYS WRITEALL", "host", "SYS WRITEALL|WRITEFILES overwrite path bag with data"},
      {"SYS WRITEFILES", "host", "SYS WRITEFILES alias of SYS WRITEALL"},
      {"SYS BULKWRITE", "host", "SYS BULKWRITE alias of SYS WRITEALL"},
      {"SYS REPLACEFILES", "host", "SYS REPLACEFILES|SUBFILES bulk rewrite path bag content"},
      {"SYS SUBFILES", "host", "SYS SUBFILES alias of SYS REPLACEFILES"},
      {"SYS GSUBFILES", "host", "SYS GSUBFILES alias of SYS REPLACEFILES"},
      {"SYS COUNTINFILES", "host", "SYS COUNTINFILES|GREPCOUNTFILES total hits across path bag"},
      {"SYS GREPCOUNTFILES", "host", "SYS GREPCOUNTFILES alias of SYS COUNTINFILES"},
      {"SYS COUNTINFILESI", "host", "SYS COUNTINFILESI case-insensitive COUNTINFILES"},
      {"SYS FIRSTFILE", "host", "SYS FIRSTFILE|HITFILE first path with content match"},
      {"SYS LASTFILE", "host", "SYS LASTFILE last path with content match"},
      {"SYS FIRSTFILEI", "host", "SYS FIRSTFILEI case-insensitive FIRSTFILE"},
      {"SYS GREPLINES", "host", "SYS GREPLINES|EXTRACTLINES collect matching lines from path bag"},
      {"SYS EXTRACTLINES", "host", "SYS EXTRACTLINES alias of SYS GREPLINES"},
      {"SYS GREPLINESI", "host", "SYS GREPLINESI case-insensitive GREPLINES"},
      {"SYS HEADFILE", "host", "SYS HEADFILE|FILEHEAD first n lines of a file"},
      {"SYS TAILFILE", "host", "SYS TAILFILE|FILETAIL last n lines of a file"},
      {"SYS LINECOUNTALL", "host", "SYS LINECOUNTALL|WCALL path bag line counts"},
      {"SYS WCALL", "host", "SYS WCALL alias of SYS LINECOUNTALL"},
      {"SYS MIDFILE", "host", "SYS MIDFILE|LINESLICE file line window"},
      {"SYS LINESLICE", "host", "SYS LINESLICE alias of SYS MIDFILE"},
      {"SYS LINEAT", "host", "SYS LINEAT|FILELINE single file line by index"},
      {"SYS FILELINE", "host", "SYS FILELINE alias of SYS LINEAT"},
      {"SYS SETFILELINE", "host", "SYS SETFILELINE|FILESETLINE replace file line by index"},
      {"SYS FILESETLINE", "host", "SYS FILESETLINE alias of SYS SETFILELINE"},
      {"SYS INSERTFILELINE", "host", "SYS INSERTFILELINE|FILEINSERTLINE insert file line by index"},
      {"SYS FILEINSERTLINE", "host", "SYS FILEINSERTLINE alias of SYS INSERTFILELINE"},
      {"SYS DROPFILELINE", "host", "SYS DROPFILELINE|FILEDROPLINE drop file line by index"},
      {"SYS FILEDROPLINE", "host", "SYS FILEDROPLINE alias of SYS DROPFILELINE"},
      {"SYS FINDFILELINE", "host", "SYS FINDFILELINE|FILEFINDLINE first file line by needle"},
      {"SYS FILEFINDLINE", "host", "SYS FILEFINDLINE alias of SYS FINDFILELINE"},
      {"SYS LASTFINDFILELINE", "host", "SYS LASTFINDFILELINE|FILELASTFIND last file line by needle"},
      {"SYS FILELASTFIND", "host", "SYS FILELASTFIND alias of SYS LASTFINDFILELINE"},
      {"SYS COUNTFILELINE", "host", "SYS COUNTFILELINE|FILECOUNTLINE count matching file lines"},
      {"SYS FILECOUNTLINE", "host", "SYS FILECOUNTLINE alias of SYS COUNTFILELINE"},
      {"SYS GREPFILE", "host", "SYS GREPFILE|FILEGREP keep matching lines from one file"},
      {"SYS FILEGREP", "host", "SYS FILEGREP alias of SYS GREPFILE"},
      {"SYS SETMATCHFILE", "host", "SYS SETMATCHFILE|FILESETMATCH replace first matching file line"},
      {"SYS FILESETMATCH", "host", "SYS FILESETMATCH alias of SYS SETMATCHFILE"},
      {"SYS DROPMATCHFILE", "host", "SYS DROPMATCHFILE|FILEDROPMATCH drop first matching file line"},
      {"SYS FILEDROPMATCH", "host", "SYS FILEDROPMATCH alias of SYS DROPMATCHFILE"},
      {"SYS DROPALLMATCHFILE", "host", "SYS DROPALLMATCHFILE|FILEDROPALLMATCH drop all matching file lines"},
      {"SYS FILEDROPALLMATCH", "host", "SYS FILEDROPALLMATCH alias of SYS DROPALLMATCHFILE"},
      {"SYS SETALLMATCHFILE", "host", "SYS SETALLMATCHFILE|FILESETALLMATCH replace all matching file lines"},
      {"SYS FILESETALLMATCH", "host", "SYS FILESETALLMATCH alias of SYS SETALLMATCHFILE"},
      {"SYS INSERTMATCHFILE", "host", "SYS INSERTMATCHFILE|FILEINSERTMATCH insert line after first match"},
      {"SYS FILEINSERTMATCH", "host", "SYS FILEINSERTMATCH alias of SYS INSERTMATCHFILE"},
      {"SYS BEFOREMATCHFILE", "host", "SYS BEFOREMATCHFILE|INSERTBEFOREMATCH insert line before first match"},
      {"SYS INSERTBEFOREMATCH", "host", "SYS INSERTBEFOREMATCH alias of SYS BEFOREMATCHFILE"},
      {"SYS LOOKUPFILE", "host", "SYS LOOKUPFILE|FILELOOKUP peel key=val from plate file"},
      {"SYS FILELOOKUP", "host", "SYS FILELOOKUP alias of SYS LOOKUPFILE"},
      {"SYS KVFILESET", "host", "SYS KVFILESET|FILEKVSET set/update key=val in plate file"},
      {"SYS FILEKVSET", "host", "SYS FILEKVSET alias of SYS KVFILESET"},
      {"SYS KVFILEDEL", "host", "SYS KVFILEDEL|FILEKVDEL drop key=val from plate file"},
      {"SYS FILEKVDEL", "host", "SYS FILEKVDEL alias of SYS KVFILEDEL"},
      {"SYS KVFILEINC", "host", "SYS KVFILEINC|INCKVFILE bump numeric key=val in plate file"},
      {"SYS INCKVFILE", "host", "SYS INCKVFILE alias of SYS KVFILEINC"},
      {"SYS KVFILEENSURE", "host", "SYS KVFILEENSURE|ENSUREKVFILE set key only if missing"},
      {"SYS ENSUREKVFILE", "host", "SYS ENSUREKVFILE alias of SYS KVFILEENSURE"},
      {"SYS DEFAULTKVFILE", "host", "SYS DEFAULTKVFILE alias of SYS KVFILEENSURE"},
      {"SYS ENSURELINE", "host", "SYS ENSURELINE|ENSUREFILELINE append line if exact missing"},
      {"SYS ENSUREFILELINE", "host", "SYS ENSUREFILELINE alias of SYS ENSURELINE"},
      {"SYS HASFILELINE", "host", "SYS HASFILELINE|FILEHASLINE exact file line membership"},
      {"SYS FILEHASLINE", "host", "SYS FILEHASLINE alias of SYS HASFILELINE"},
      {"SYS DROPEXACTFILE", "host", "SYS DROPEXACTFILE|DROPFILEEXACT drop first exact file line"},
      {"SYS DROPFILEEXACT", "host", "SYS DROPFILEEXACT alias of SYS DROPEXACTFILE"},
      {"SYS REMOVELINEFILE", "host", "SYS REMOVELINEFILE alias of SYS DROPEXACTFILE"},
      {"SYS TOGGLELINE", "host", "SYS TOGGLELINE|TOGGLEFILELINE flip exact file line presence"},
      {"SYS TOGGLEFILELINE", "host", "SYS TOGGLEFILELINE alias of SYS TOGGLELINE"},
      {"SYS FLIPLINE", "host", "SYS FLIPLINE alias of SYS TOGGLELINE"},
      {"SYS NTH", "host", "SYS NTH newline field peel"},
      {"SYS GREP", "host", "SYS GREP filter newline fields"},
      {"SYS GREPANY", "host", "SYS GREPANY multi-needle bag filter"},
      {"SYS GREPOR", "host", "SYS GREPOR alias of SYS GREPANY"},
      {"SYS GREPANYI", "host", "SYS GREPANYI case-insensitive multi-needle"},
      {"SYS GREPALL", "host", "SYS GREPALL multi-needle AND bag filter"},
      {"SYS GREPAND", "host", "SYS GREPAND alias of SYS GREPALL"},
      {"SYS GREPALLI", "host", "SYS GREPALLI case-insensitive multi-needle AND"},
      {"SYS TAKE", "host", "SYS TAKE first n lines"},
      {"SYS DROP", "host", "SYS DROP first n lines"},
      {"SYS SPLIT", "host", "SYS SPLIT sep to lines"},
      {"SYS SORT", "host", "SYS SORT line order"},
      {"SYS SORTN", "host", "SYS SORTN numeric bag sort"},
      {"SYS NSORT", "host", "SYS NSORT alias of SYS SORTN"},
      {"SYS SORTLEN", "host", "SYS SORTLEN sort by field length"},
      {"SYS LENSORT", "host", "SYS LENSORT alias of SYS SORTLEN"},
      {"SYS MAXLEN", "host", "SYS MAXLEN max bag field length"},
      {"SYS MINLEN", "host", "SYS MINLEN min bag field length"},
      {"SYS MAXWIDTH", "host", "SYS MAXWIDTH alias of SYS MAXLEN"},
      {"SYS MINWIDTH", "host", "SYS MINWIDTH alias of SYS MINLEN"},
      {"SYS LONGEST", "host", "SYS LONGEST longest bag field text"},
      {"SYS SHORTEST", "host", "SYS SHORTEST shortest bag field text"},
      {"SYS MAXFIELD", "host", "SYS MAXFIELD alias of SYS LONGEST"},
      {"SYS MINFIELD", "host", "SYS MINFIELD alias of SYS SHORTEST"},
      {"SYS COMMONPREFIX", "host", "SYS COMMONPREFIX longest common prefix"},
      {"SYS LCP", "host", "SYS LCP alias of SYS COMMONPREFIX"},
      {"SYS COMMONSUFFIX", "host", "SYS COMMONSUFFIX longest common suffix"},
      {"SYS LCS", "host", "SYS LCS alias of SYS COMMONSUFFIX"},
      {"SYS STRIPPREFIX", "host", "SYS STRIPPREFIX drop leading prefix per field"},
      {"SYS STRIPSUFFIX", "host", "SYS STRIPSUFFIX drop trailing suffix per field"},
      {"SYS STRIPCOMMON", "host", "SYS STRIPCOMMON strip LCP relative paths"},
      {"SYS STRIPLCP", "host", "SYS STRIPLCP alias of SYS STRIPCOMMON"},
      {"SYS UNIQ", "host", "SYS UNIQ adjacent dups"},
      {"SYS JOIN", "host", "SYS JOIN path join a/b"},
      {"SYS PATH", "host", "SYS PATH alias of JOIN"},
      {"SYS JOINLINES", "host", "SYS JOINLINES paste lines"},
      {"SYS REPLACEALL", "host", "SYS REPLACEALL all subst"},
      {"SYS APPEND", "host", "SYS APPEND history log line"},
      {"SYS GREPI", "host", "SYS GREPI case-insensitive GREP"},
      {"SYS STR", "host", "SYS STR|ITOA num → decimal string"},
      {"SYS ITOA", "host", "SYS ITOA alias of SYS STR"},
      {"SYS BEFORE", "host", "SYS BEFORE peel left of needle"},
      {"SYS AFTER", "host", "SYS AFTER peel right of needle"},
      {"SYS PUSH", "host", "SYS PUSH append newline field"},
      {"SYS ADDLINE", "host", "SYS ADDLINE alias of SYS PUSH"},
      {"SYS EQSI", "host", "SYS EQSI case-insensitive equality"},
      {"SYS HASI", "host", "SYS HASI case-insensitive contains"},
      {"SYS LINES", "host", "SYS LINES count newline fields"},
      {"SYS WC", "host", "SYS WC alias of SYS LINES"},
      {"SYS CUT", "host", "SYS CUT 0-based field by sep"},
      {"SYS COLUMN", "host", "SYS COLUMN 1-based field by sep"},
      {"SYS EMPTY", "host", "SYS EMPTY zero-length probe"},
      {"SYS BLANK", "host", "SYS BLANK whitespace-only probe"},
      {"SYS NONEMPTY", "host", "SYS NONEMPTY any-char probe"},
      {"SYS COALESCE", "host", "SYS COALESCE first non-empty string"},
      {"SYS NVL", "host", "SYS NVL alias of SYS COALESCE"},
      {"SYS UNION", "host", "SYS UNION merge bags first-seen unique"},
      {"SYS DISTINCT", "host", "SYS DISTINCT order-preserving full unique"},
      {"SYS INTERSECT", "host", "SYS INTERSECT fields of a also in b"},
      {"SYS DIFF", "host", "SYS DIFF fields of a not in b"},
      {"SYS ZIP", "host", "SYS ZIP pair bag fields by index"},
      {"SYS KEYS", "host", "SYS KEYS peel left of sep each field"},
      {"SYS VALS", "host", "SYS VALS peel right of sep each field"},
      {"SYS PREFIXALL", "host", "SYS PREFIXALL prepend every bag field"},
      {"SYS SUFFIXALL", "host", "SYS SUFFIXALL append every bag field"},
      {"SYS FILL", "host", "SYS FILL n value bag of n copies"},
      {"SYS ENUMERATE", "host", "SYS ENUMERATE index-prefix bag fields"},
      {"SYS NUMBER", "host", "SYS NUMBER alias of SYS ENUMERATE"},
      {"SYS SQUEEZE", "host", "SYS SQUEEZE drop empty bag fields"},
      {"SYS COMPACT", "host", "SYS COMPACT alias of SYS SQUEEZE"},
      {"SYS TRIMALL", "host", "SYS TRIMALL trim every bag field"},
      {"SYS MAPTRIM", "host", "SYS MAPTRIM alias of SYS TRIMALL"},
      {"SYS UPPERALL", "host", "SYS UPPERALL upper every bag field"},
      {"SYS MAPUPPER", "host", "SYS MAPUPPER alias of SYS UPPERALL"},
      {"SYS LOWERALL", "host", "SYS LOWERALL lower every bag field"},
      {"SYS MAPLOWER", "host", "SYS MAPLOWER alias of SYS LOWERALL"},
      {"SYS MAPREPLACE", "host", "SYS MAPREPLACE bag-wide GSUB"},
      {"SYS GSUBALL", "host", "SYS GSUBALL alias of SYS MAPREPLACE"},
      {"SYS REPLACEBAG", "host", "SYS REPLACEBAG alias of SYS MAPREPLACE"},
      {"SYS FREQ", "host", "SYS FREQ field frequency histogram"},
      {"SYS HIST", "host", "SYS HIST alias of SYS FREQ"},
      {"SYS COUNTS", "host", "SYS COUNTS alias of SYS FREQ"},
      {"SYS CUTALL", "host", "SYS CUTALL peel column every bag line"},
      {"SYS MAPCUT", "host", "SYS MAPCUT alias of SYS CUTALL"},
      {"SYS COLALL", "host", "SYS COLALL 1-based CUTALL"},
      {"SYS SORTFREQ", "host", "SYS SORTFREQ sort FREQ by count"},
      {"SYS SORTBYCOUNT", "host", "SYS SORTBYCOUNT alias of SYS SORTFREQ"},
      {"SYS FSORT", "host", "SYS FSORT alias of SYS SORTFREQ"},
      {"SYS BEFOREALL", "host", "SYS BEFOREALL peel left every bag field"},
      {"SYS AFTERALL", "host", "SYS AFTERALL peel right every bag field"},
      {"SYS MAPBEFORE", "host", "SYS MAPBEFORE alias of SYS BEFOREALL"},
      {"SYS MAPAFTER", "host", "SYS MAPAFTER alias of SYS AFTERALL"},
      {"SYS FIRSTMATCH", "host", "SYS FIRSTMATCH first bag field hit"},
      {"SYS GREP1", "host", "SYS GREP1 alias of SYS FIRSTMATCH"},
      {"SYS FIRSTMATCHI", "host", "SYS FIRSTMATCHI case-insensitive first hit"},
      {"SYS LOOKUP", "host", "SYS LOOKUP bag key [sep] [OR fb] peel / default"},
      {"SYS KVGET", "host", "SYS KVGET alias of SYS LOOKUP"},
      {"SYS LOOKUPI", "host", "SYS LOOKUPI case-insensitive LOOKUP"},
      {"SYS GETKV", "host", "SYS GETKV alias of SYS LOOKUP"},
      {"SYS KVSET", "host", "SYS KVSET set/update key:val bag field"},
      {"SYS SETKV", "host", "SYS SETKV alias of SYS KVSET"},
      {"SYS DICTSET", "host", "SYS DICTSET alias of SYS KVSET"},
      {"SYS MAPSET", "host", "SYS MAPSET alias of SYS KVSET"},
      {"SYS KVINC", "host", "SYS KVINC bump key:val numeric counter"},
      {"SYS INCKV", "host", "SYS INCKV alias of SYS KVINC"},
      {"SYS BUMPKV", "host", "SYS BUMPKV alias of SYS KVINC"},
      {"SYS KVADD", "host", "SYS KVADD alias of SYS KVINC"},
      {"SYS KVDEL", "host", "SYS KVDEL|DELKV bag key [sep] — drop key:val field"},
      {"SYS DELKV", "host", "SYS DELKV alias of SYS KVDEL"},
      {"SYS RMKV", "host", "SYS RMKV alias of SYS KVDEL"},
      {"SYS UNSETKV", "host", "SYS UNSETKV alias of SYS KVDEL"},
      {"SYS MERGEKV", "host", "SYS MERGEKV|KVADDALL bag_a bag_b [sep] — sum-merge key:val"},
      {"SYS KVADDALL", "host", "SYS KVADDALL alias of SYS MERGEKV"},
      {"SYS ADDFREQ", "host", "SYS ADDFREQ alias of SYS MERGEKV"},
      {"SYS SUMKV", "host", "SYS SUMKV|TOTALKV bag [sep] — sum key:val values"},
      {"SYS TOTALKV", "host", "SYS TOTALKV alias of SYS SUMKV"},
      {"SYS SUMVALS", "host", "SYS SUMVALS alias of SYS SUMKV"},
      {"SYS AVGKV", "host", "SYS AVGKV|MEANKV bag [sep] — mean of values"},
      {"SYS MEANKV", "host", "SYS MEANKV alias of SYS AVGKV"},
      {"SYS MEANVAL", "host", "SYS MEANVAL alias of SYS AVGKV"},
      {"SYS MEDIANKV", "host", "SYS MEDIANKV|P50KV bag [sep] — median of values"},
      {"SYS P50KV", "host", "SYS P50KV alias of SYS MEDIANKV"},
      {"SYS MIDKV", "host", "SYS MIDKV alias of SYS MEDIANKV"},
      {"SYS TOPKEY", "host", "SYS TOPKEY|ARGMAXKV bag [sep] — key with max value"},
      {"SYS ARGMAXKV", "host", "SYS ARGMAXKV alias of SYS TOPKEY"},
      {"SYS BOTKEY", "host", "SYS BOTKEY|ARGMINKV bag [sep] — key with min value"},
      {"SYS ARGMINKV", "host", "SYS ARGMINKV alias of SYS BOTKEY"},
      {"SYS LOOKUPN", "host", "SYS LOOKUPN|KVGETN bag key [sep] [OR n] — value as int"},
      {"SYS KVGETN", "host", "SYS KVGETN alias of SYS LOOKUPN"},
      {"SYS GETKVN", "host", "SYS GETKVN alias of SYS LOOKUPN"},
      {"SYS THRESHKV", "host", "SYS THRESHKV|KEEPVAL bag min [sep] — keep value>=min"},
      {"SYS KEEPVAL", "host", "SYS KEEPVAL alias of SYS THRESHKV"},
      {"SYS MINCOUNT", "host", "SYS MINCOUNT alias of SYS THRESHKV"},
      {"SYS PCTKV", "host", "SYS PCTKV|SHAREKV bag [sep] — values as %% of total"},
      {"SYS SHAREKV", "host", "SYS SHAREKV alias of SYS PCTKV"},
      {"SYS PERCENTKV", "host", "SYS PERCENTKV alias of SYS PCTKV"},
      {"SYS CAPKV", "host", "SYS CAPKV|CLAMPKV bag max [sep] — clamp values to max"},
      {"SYS CLAMPKV", "host", "SYS CLAMPKV alias of SYS CAPKV"},
      {"SYS MAXVAL", "host", "SYS MAXVAL alias of SYS CAPKV"},
      {"SYS SCALEKV", "host", "SYS SCALEKV|MULKV bag factor [sep] — multiply values"},
      {"SYS MULKV", "host", "SYS MULKV alias of SYS SCALEKV"},
      {"SYS WEIGHTKV", "host", "SYS WEIGHTKV alias of SYS SCALEKV"},
      {"SYS DIVKV", "host", "SYS DIVKV|IDIVKV bag divisor [sep] — divide values"},
      {"SYS IDIVKV", "host", "SYS IDIVKV alias of SYS DIVKV"},
      {"SYS QUOTKV", "host", "SYS QUOTKV alias of SYS DIVKV"},
      {"SYS DIFFKV", "host", "SYS DIFFKV|SUBKV bag_a bag_b [sep] — subtract key:val"},
      {"SYS SUBKV", "host", "SYS SUBKV alias of SYS DIFFKV"},
      {"SYS DELTAKV", "host", "SYS DELTAKV alias of SYS DIFFKV"},
      {"SYS ADDKV", "host", "SYS ADDKV|OFFSETKV bag delta [sep] — add to values"},
      {"SYS OFFSETKV", "host", "SYS OFFSETKV alias of SYS ADDKV"},
      {"SYS ADDVAL", "host", "SYS ADDVAL alias of SYS ADDKV"},
      {"SYS DROPZERO", "host", "SYS DROPZERO|KEEPNZ bag [sep] — drop value==0"},
      {"SYS KEEPNZ", "host", "SYS KEEPNZ alias of SYS DROPZERO"},
      {"SYS NZKV", "host", "SYS NZKV alias of SYS DROPZERO"},
      {"SYS KEEPKEY", "host", "SYS KEEPKEY|GREPKEY bag needle [sep] — filter by key"},
      {"SYS GREPKEY", "host", "SYS GREPKEY alias of SYS KEEPKEY"},
      {"SYS DROPKEY", "host", "SYS DROPKEY drop keys containing needle"},
      {"SYS ABSKV", "host", "SYS ABSKV|MAGKV bag [sep] — abs key:val values"},
      {"SYS MAGKV", "host", "SYS MAGKV alias of SYS ABSKV"},
      {"SYS ABSALL", "host", "SYS ABSALL alias of SYS ABSKV"},
      {"SYS SIGNKV", "host", "SYS SIGNKV|DIRKV bag [sep] — map values to -1|0|1"},
      {"SYS DIRKV", "host", "SYS DIRKV alias of SYS SIGNKV"},
      {"SYS SGNKV", "host", "SYS SGNKV alias of SYS SIGNKV"},
      {"SYS LASTMATCH", "host", "SYS LASTMATCH last bag field hit"},
      {"SYS GREP1L", "host", "SYS GREP1L alias of SYS LASTMATCH"},
      {"SYS LASTMATCHI", "host", "SYS LASTMATCHI case-insensitive last hit"},
      {"SYS CHUNK", "host", "SYS CHUNK group bag fields by n"},
      {"SYS BATCH", "host", "SYS BATCH alias of SYS CHUNK"},
      {"SYS GROUPN", "host", "SYS GROUPN alias of SYS CHUNK"},
      {"SYS WINDOW", "host", "SYS WINDOW overlapping bag field windows"},
      {"SYS SLIDE", "host", "SYS SLIDE alias of SYS WINDOW"},
      {"SYS NGRAM", "host", "SYS NGRAM alias of SYS WINDOW"},
      {"SYS STRIDE", "host", "SYS STRIDE partition bag by step/offset"},
      {"SYS EVERY", "host", "SYS EVERY alias of SYS STRIDE"},
      {"SYS STEP", "host", "SYS STEP alias of SYS STRIDE"},
      {"SYS ROTATE", "host", "SYS ROTATE left-rotate bag fields"},
      {"SYS ROTL", "host", "SYS ROTL alias of SYS ROTATE"},
      {"SYS ROTR", "host", "SYS ROTR right-rotate bag fields"},
      {"SYS FLATTEN", "host", "SYS FLATTEN expand joined bag fields"},
      {"SYS UNCHUNK", "host", "SYS UNCHUNK alias of SYS FLATTEN"},
      {"SYS SPLITALL", "host", "SYS SPLITALL alias of SYS FLATTEN"},
      {"SYS COUNTMATCH", "host", "SYS COUNTMATCH substring field count"},
      {"SYS GREPCOUNT", "host", "SYS GREPCOUNT alias of SYS COUNTMATCH"},
      {"SYS COUNTMATCHI", "host", "SYS COUNTMATCHI case-insensitive COUNTMATCH"},
      {"SYS LENALL", "host", "SYS LENALL per-field lengths bag"},
      {"SYS MAPLEN", "host", "SYS MAPLEN alias of SYS LENALL"},
      {"SYS FIELDLENS", "host", "SYS FIELDLENS alias of SYS LENALL"},
      {"SYS MAXLEN", "host", "SYS MAXLEN max bag field length"},
      {"SYS MINLEN", "host", "SYS MINLEN min bag field length"},
      {"SYS MAXWIDTH", "host", "SYS MAXWIDTH alias of SYS MAXLEN"},
      {"SYS MINWIDTH", "host", "SYS MINWIDTH alias of SYS MINLEN"},
      {"SYS LONGEST", "host", "SYS LONGEST longest bag field text"},
      {"SYS SHORTEST", "host", "SYS SHORTEST shortest bag field text"},
      {"SYS MAXFIELD", "host", "SYS MAXFIELD alias of SYS LONGEST"},
      {"SYS MINFIELD", "host", "SYS MINFIELD alias of SYS SHORTEST"},
      {"SYS COMMONPREFIX", "host", "SYS COMMONPREFIX longest common prefix"},
      {"SYS LCP", "host", "SYS LCP alias of SYS COMMONPREFIX"},
      {"SYS COMMONSUFFIX", "host", "SYS COMMONSUFFIX longest common suffix"},
      {"SYS LCS", "host", "SYS LCS alias of SYS COMMONSUFFIX"},
      {"SYS STRIPPREFIX", "host", "SYS STRIPPREFIX drop leading prefix per field"},
      {"SYS STRIPSUFFIX", "host", "SYS STRIPSUFFIX drop trailing suffix per field"},
      {"SYS STRIPCOMMON", "host", "SYS STRIPCOMMON strip LCP relative paths"},
      {"SYS STRIPLCP", "host", "SYS STRIPLCP alias of SYS STRIPCOMMON"},
      {"SYS MIDLINES", "host", "SYS MIDLINES bag field window slice"},
      {"SYS SLICEBAG", "host", "SYS SLICEBAG alias of SYS MIDLINES"},
      {"SYS POP", "host", "SYS POP last bag field"},
      {"SYS POPLINE", "host", "SYS POPLINE alias of SYS POP"},
      {"SYS MTIME", "host", "SYS MTIME file mtime epoch"},
      {"SYS AGE", "host", "SYS AGE seconds since mtime"},
      {"SYS STARTSI", "host", "SYS STARTSI case-insensitive prefix"},
      {"SYS ENDSI", "host", "SYS ENDSI case-insensitive suffix"},
      {"SYS BETWEEN", "host", "SYS BETWEEN peel between delimiters"},
      {"SYS MIDOF", "host", "SYS MIDOF alias of SYS BETWEEN"},
      {"SYS EXTRACT", "host", "SYS EXTRACT alias of SYS BETWEEN"},
      {"SYS FINDI", "host", "SYS FINDI case-insensitive index"},
      {"SYS INDEXI", "host", "SYS INDEXI alias of SYS FINDI"},
      {"SYS SETENV", "host", "SYS SETENV process setenv"},
      {"SYS ENVDEFAULT", "host", "SYS ENVDEFAULT|ENSUREENV name value — setenv if missing/empty"},
      {"SYS UNSETENV", "host", "SYS UNSETENV process unsetenv"},
      {"SYS REVL", "host", "SYS REVL reverse bag line order"},
      {"SYS REVLINES", "host", "SYS REVLINES alias of SYS REVL"},
      {"SYS TAC", "host", "SYS TAC alias of SYS REVL"},
      {"SYS WORDS", "host", "SYS WORDS whitespace tokenize"},
      {"SYS TOKENIZE", "host", "SYS TOKENIZE alias of SYS WORDS"},
      {"SYS HASLINE", "host", "SYS HASLINE exact bag field membership"},
      {"SYS HASLINEI", "host", "SYS HASLINEI case-insensitive exact field"},
      {"SYS PREPEND", "host", "SYS PREPEND insert field at bag front"},
      {"SYS POPHEAD", "host", "SYS POPHEAD peel first bag field FIFO"},
      {"SYS DEQUEUE", "host", "SYS DEQUEUE alias of SYS POPHEAD"},
      {"SYS REMOVELINE", "host", "SYS REMOVELINE drop exact bag field"},
      {"SYS DROPLINE", "host", "SYS DROPLINE alias of SYS REMOVELINE"},
      {"SYS FINDLINE", "host", "SYS FINDLINE exact bag field index"},
      {"SYS LINEINDEX", "host", "SYS LINEINDEX alias of SYS FINDLINE"},
      {"SYS SETLINE", "host", "SYS SETLINE set bag field by index"},
      {"SYS REPLACELINE", "host", "SYS REPLACELINE alias of SYS SETLINE"},
      {"SYS INSERTLINE", "host", "SYS INSERTLINE insert bag field by index"},
      {"SYS INSLINE", "host", "SYS INSLINE alias of SYS INSERTLINE"},
      {"SYS DROPNTH", "host", "SYS DROPNTH drop bag field by index"},
      {"SYS DROPAT", "host", "SYS DROPAT alias of SYS DROPNTH"},
      {"SYS MOVELINE", "host", "SYS MOVELINE move bag field to index"},
      {"SYS MOVEAT", "host", "SYS MOVEAT alias of SYS MOVELINE"},
      {"SYS COUNTLINE", "host", "SYS COUNTLINE count exact bag field matches"},
      {"SYS COUNTLINEI", "host", "SYS COUNTLINEI case-insensitive field match count"},
      {"SYS SETMATCH", "host", "SYS SETMATCH replace first exact bag field"},
      {"SYS REPLACEMATCH", "host", "SYS REPLACEMATCH alias of SYS SETMATCH"},
      {"EACH LINE", "flow", "EACH LINE walk LIST fields"},
      {"EACH LIB", "flow", "EACH LIB [MATCH needle] walk sorted lib stems"},
      {"SYS", "host", "SYS ENV|ARG|WHICH|READ|WRITE …"},
      {"SMX", "smx", "SMX KEY|TALK|EXCHANGE|SERVE|DIAL"},
      {"HELP", "flow", "HELP [form] in-program catalog"},
    };
    if (!q || !q[0]) {
      fprintf(stderr, "usage: cubalc which <name|path|form>\n"
                      "       cubalc locate hold_seed · cubalc resolve ASSERT\n");
      printf("{\"schema\":\"cubalc.which.v1\",\"ok\":false,\"cmd\":\"which\","
             "\"err\":\"need name\",\"version\":\"%s\"}\n", CUBALC_LANG_VERSION);
      return 2;
    }
    {
      const char *slash = strrchr(q, '/');
      const char *leaf = slash ? slash + 1 : q;
      snprintf(stem, sizeof stem, "%s", leaf);
      blen = strlen(stem);
      if (blen > 7 && strcmp(stem + blen - 7, ".cubalc") == 0)
        stem[blen - 7] = 0;
    }
    /* 1) host find (lib/program/path) — primary hit */
    {
      cubalc_host_result hr;
      if (cubalc_host_find_cubalc(q, &hr) == 0 && nh < 16) {
        const char *kind = "path";
        const char *hint = "INCLUDE/cat resolve";
        const char *ip = getenv("CUBALC_INCLUDE_PATH");
        if (strstr(hr.str, "programs/lib/")) kind = "lib";
        else if (strstr(hr.str, "programs/proof/")) kind = "proof";
        else if (strstr(hr.str, "programs/p2p/")) kind = "p2p";
        else if (strstr(hr.str, "programs/protect/")) kind = "protect";
        else if (strstr(hr.str, "programs/")) kind = "program";
        else if (strstr(hr.str, "docs/")) kind = "doc";
        else if (ip && ip[0] && strstr(hr.str, ".cubalc")) {
          /* project lib from CUBALC_INCLUDE_PATH (or -L) */
          kind = "lib";
          hint = "CUBALC_INCLUDE_PATH project lib";
        } else if (strstr(hr.str, ".cubalc")) {
          kind = "lib";
        }
        snprintf(hits[nh].kind, sizeof hits[0].kind, "%s", kind);
        snprintf(hits[nh].path, sizeof hits[0].path, "%s", hr.str);
        snprintf(hits[nh].hint, sizeof hits[0].hint, "%s", hint);
        hits[nh].present = 1;
        nh++;
      }
    }
    /* 2) path aliases (cookbook, hello, …) */
    for (i = 0; i < (int)(sizeof aliases / sizeof aliases[0]) && nh < 16; i++) {
      char aup[32], qup[64];
      size_t k;
      int already = 0;
      for (k = 0; aliases[i].name[k] && k + 1 < sizeof aup; k++)
        aup[k] = (char)toupper((unsigned char)aliases[i].name[k]);
      aup[k] = 0;
      for (k = 0; q[k] && k + 1 < sizeof qup; k++)
        qup[k] = (char)toupper((unsigned char)q[k]);
      qup[k] = 0;
      {
        char stem_up[64];
        size_t sk;
        for (sk = 0; stem[sk] && sk + 1 < sizeof stem_up; sk++)
          stem_up[sk] = (char)toupper((unsigned char)stem[sk]);
        stem_up[sk] = 0;
        if (strcmp(aup, qup) != 0 && strcmp(stem_up, aup) != 0)
          continue;
      }
      for (k = 0; k < (size_t)nh; k++)
        if (strcmp(hits[k].path, aliases[i].path) == 0) already = 1;
      if (already) continue;
      snprintf(hits[nh].kind, sizeof hits[0].kind, "alias");
      snprintf(hits[nh].path, sizeof hits[0].path, "%s", aliases[i].path);
      snprintf(hits[nh].hint, sizeof hits[0].hint, "%s", aliases[i].hint);
      hits[nh].present = (access(aliases[i].path, F_OK) == 0);
      nh++;
    }
    /* 3) form catalog (no file path) */
    for (i = 0; i < (int)(sizeof forms / sizeof forms[0]) && nh < 16; i++) {
      char fup[32], qup[64], sup[64];
      size_t k;
      for (k = 0; forms[i].name[k] && k + 1 < sizeof fup; k++)
        fup[k] = (char)toupper((unsigned char)forms[i].name[k]);
      fup[k] = 0;
      for (k = 0; q[k] && k + 1 < sizeof qup; k++)
        qup[k] = (char)toupper((unsigned char)q[k]);
      qup[k] = 0;
      for (k = 0; stem[k] && k + 1 < sizeof sup; k++)
        sup[k] = (char)toupper((unsigned char)stem[k]);
      sup[k] = 0;
      if (strcmp(fup, qup) != 0 && strcmp(fup, sup) != 0 &&
          !strstr(fup, qup) && !(qup[0] && strstr(fup, qup)))
        continue;
      /* require full or strong prefix match (len>=3) to avoid noise */
      if (strcmp(fup, qup) != 0 && strcmp(fup, sup) != 0) {
        if (strlen(qup) < 3 || !strstr(fup, qup))
          continue;
      }
      snprintf(hits[nh].kind, sizeof hits[0].kind, "form");
      snprintf(hits[nh].path, sizeof hits[0].path, "%s", forms[i].name);
      snprintf(hits[nh].hint, sizeof hits[0].hint, "%s · %s",
               forms[i].plane, forms[i].hint);
      hits[nh].present = 1;
      nh++;
    }
    /* 4) PATH binary (cubalc, etc.) — only if no file hit yet or name looks bin */
    {
      cubalc_host_result hr;
      int already_bin = 0;
      /* use binary-only: temp copy of which without find would recurse — check X_OK paths */
      if (cubalc_host_which(q, &hr) == 0) {
        for (i = 0; i < nh; i++)
          if (strcmp(hits[i].path, hr.str) == 0) already_bin = 1;
        if (!already_bin && nh < 16) {
          /* if which returned a .cubalc, skip as bin */
          if (!strstr(hr.str, ".cubalc") && !strstr(hr.str, ".md") &&
              !strstr(hr.str, "programs/")) {
            snprintf(hits[nh].kind, sizeof hits[0].kind, "bin");
            snprintf(hits[nh].path, sizeof hits[0].path, "%s", hr.str);
            snprintf(hits[nh].hint, sizeof hits[0].hint, "PATH / CUBALC_BIN");
            hits[nh].present = 1;
            nh++;
          }
        }
      }
    }
    printf("# CubalC which query=%s version=%s\n", q, CUBALC_LANG_VERSION);
    printf("# kind\tpath\tpresent\thint\n");
    for (i = 0; i < nh; i++)
      printf("%s\t%s\t%s\t%s\n", hits[i].kind, hits[i].path,
             hits[i].present ? "yes" : "no", hits[i].hint);
    if (nh == 0)
      printf("# (no match — try cubalc libs|examples|forms|paths)\n");
    printf("{\"schema\":\"cubalc.which.v1\",\"ok\":%s,\"cmd\":\"which\","
           "\"query\":\"%s\",\"n\":%d,\"version\":\"%s\","
           "\"note\":\"resolve short names for INCLUDE/cat/run — first match preferred\","
           "\"matches\":[",
           nh > 0 ? "true" : "false", q, nh, CUBALC_LANG_VERSION);
    for (i = 0; i < nh; i++) {
      char pesc[520];
      size_t k, o = 0;
      for (k = 0; hits[i].path[k] && o + 2 < sizeof pesc; k++) {
        char c = hits[i].path[k];
        if (c == '"' || c == '\\') pesc[o++] = '_';
        else pesc[o++] = c;
      }
      pesc[o] = 0;
      printf("%s{\"kind\":\"%s\",\"path\":\"%s\",\"present\":%s,\"hint\":\"%s\"}",
             i ? "," : "", hits[i].kind, pesc,
             hits[i].present ? "true" : "false", hits[i].hint);
    }
    printf("]}\n");
    return nh > 0 ? 0 : 1;
  }

  return 2;
}
