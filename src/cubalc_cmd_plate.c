/* CubalC — CLI plate command plane (peeled from cubalc_main). */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_plate.h"
#include "cubalc.h"
#include "cubalc_smx.h"
#include "cubalc_lang.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

int cubalc_cmd_plate(int argc, char **argv) {
  const char *cmd = (argc > 1) ? argv[1] : "plate";
  /* body expects cmd, argc, argv in scope as in main */
  if (strcmp(cmd, "plate") == 0 || strcmp(cmd, "jsonplate") == 0 ||
      strcmp(cmd, "platefile") == 0 || strcmp(cmd, "agent-plate") == 0) {
    /* Usability: inspect/mutate JSON agent plate files from the CLI without a
     * .cubalc program. Complements plate_boot/SETP/SAVEPLATE language surface.
     *   cubalc plate show path.json
     *   cubalc plate get  path.json key [OR default]
     *   cubalc plate set  path.json key value
     *   cubalc plate inc  path.json key [delta]
     *   cubalc plate del  path.json key
     *   cubalc plate keys path.json
     *   cubalc plate fill [-s] path.json tmpl|@file [out]
     *   cubalc plate fillkeys path.json tmpl|@file
     *   cubalc plate ensure path.json [seed|@file]   # create-or-keep · ENSUREPLATE dual
     *   cubalc plate merge  path.json overlay|@file  # multi-key overlay · JSONFILEMERGE dual
     *   cubalc plate eq     a.json b.json            # order-indep equal · JSONEQ dual
     *   cubalc plate ne     a.json b.json            # unequal probe
     *   cubalc plate diff   a.json b.json            # changed key bag · JSONCHANGED dual
     *   cubalc plate changelog a.json b.json         # "key: old → new" · JSONCHANGELOG dual
     *   cubalc plate has|need path.json k1 k2 …      # multi-key contract · HASPALL/NEEDP duals
     *   cubalc plate pick path.json k1 k2 …          # keep listed keys · PICKP dual
     *   cubalc plate omit path.json k1 k2 …          # drop listed keys · OMITP dual
     *   cubalc plate sum|avg|median path.json        # numeric aggregates · SUMNP/AVGNP/MEDIANP
     *   cubalc plate top|bot path.json [n]           # top/bot N keys by value · TOPNP/BOTNP
     *   cubalc plate rename path.json old new        # RENAMEP dual · paths ok
     *   cubalc plate copy   path.json src dst        # COPYP dual · paths ok
     *   cubalc plate swap   path.json a b            # SWAPP dual · paths ok
     * Bare: cubalc plate path.json  → show
     * One JSON plate per call (cubalc.plate.v1) for agents. */
    const char *op = "show";
    const char *path = NULL;
    const char *key = NULL;
    const char *val = NULL;
    const char *fb = NULL;
    long delta = 1;
    int have_fb = 0, file_hit = 0, val_kind = 0;
    char plate[CUBALC_HOST_STR_MAX];
    char parent[512];
    cubalc_host_result hr, gr, keys, wr;
    const char *body;
    const char *slash;
    int ai = 2;

    plate[0] = 0;
    if (argc <= 2) {
      fprintf(stderr,
              "usage: cubalc plate show|get|set|inc|del|keys|nestget|nestset|nestinc|nestdel|nestkeys|nesthas|nestpick|nestomit|fill|… <path> …\n"
              "       cubalc plate <path.json>                 # show\n"
              "       cubalc plate get <path> <key> [OR def]\n"
              "       cubalc plate getn <path> <key> [OR n]  # GETPN dual · numeric peel (paths ok)\n"
              "       cubalc plate getobj <path> <key> [OR '{}']  # GETOBJ dual · peel object (paths ok)\n"
              "       cubalc plate setobj <path> <key> <obj|@file>  # SETOBJ dual · nest object (paths ok)\n"
              "       cubalc plate mergeobj|defaultobj <path> <key> <obj|@file>  # MERGEOBJ/DEFAULTOBJ duals\n"
              "       cubalc plate type <path> <key>  # TYPEP dual · kind miss|num|str|… (paths ok)\n"
              "       cubalc plate set <path> <key> <value>\n"
              "       cubalc plate default <path> <key> <value>  # DEFAULTP dual · set-if-missing (paths ok)\n"
              "       cubalc plate toggle <path> <key>  # TOGGLEP dual · flip 0↔1 (paths ok)\n"
              "       cubalc plate rename <path> <old> <new>  # RENAMEP dual · paths ok\n"
              "       cubalc plate copy <path> <src> <dst>    # COPYP dual · paths ok\n"
              "       cubalc plate swap <path> <a> <b>        # SWAPP dual · paths ok\n"
              "       cubalc plate inc <path> <key> [delta]\n"
              "       cubalc plate del <path> <key>\n"
              "       cubalc plate keys <path> [nest.path]  # KEYSP dual · nest path ok\n"
              "       cubalc plate leaves|pathkeys <path> [nest.path]  # PATHKEYS dual · dotted leaf paths\n"
              "       cubalc plate flat|flatkv <path> [nest.path]  # FLATKV dual · path:value bag\n"
              "       cubalc plate unflat|unflatkv <path> [UNDER nest] <bag|@file|k:v…>  # UNFLATKV dual\n"
              "       cubalc plate diffflat|clogflat <a.json> <b.json>  # DIFFFLAT dual · deep leaf changelog\n"
              "       cubalc plate pathdiff <a.json> <b.json>  # PATHDIFF dual · changed leaf paths\n"
              "       cubalc plate grepf|grepflat <path> <needle>  # GREPFLAT dual · path filter\n"
              "       cubalc plate grepvf <path> <needle>  # GREPVFLAT dual · invert\n"
              "       cubalc plate prune <path> <needle>  # PRUNEFLAT dual · drop matching leaves\n"
              "       cubalc plate keeponly <path> <needle>  # KEEONLYFLAT dual · project leaves\n"
              "       cubalc plate mergeflat <base.json> <overlay.json>  # MERGEFLAT dual · deep leaf overlay\n"
              "       cubalc plate renameflat <path> <old_pfx> <new_pfx>  # RENAMEFLAT dual · path prefix rewrite\n"
              "       cubalc plate setflat <path> <needle> <value>  # SETFLAT dual · bulk leaf value set\n"
              "       cubalc plate incflat <path> <needle> [delta]  # INCFLAT dual · bump pure-int leaves\n"
              "       cubalc plate sumflat <path> [needle]  # SUMFLAT dual · sum pure-int leaves by path needle\n"
              "       cubalc plate avgflat <path> [needle]  # AVGFLAT dual · mean pure-int leaves by path needle\n"
              "       cubalc plate medianflat <path> [needle]  # MEDIANFLAT dual · median pure-int leaves by path needle\n"
              "       cubalc plate toppath|botpath <path> [needle]  # TOPPATHFLAT dual · path of max/min pure-int leaf\n"
              "       cubalc plate threshflat <path> [needle] <min>  # THRESHFLAT dual · drop pure-int leaves value<min\n"
              "       cubalc plate dropzeroflat <path> [needle]  # DROPZEROFLAT dual · drop pure-int zeros\n"
              "       cubalc plate capflat <path> [needle] <max>  # CAPFLAT dual · clamp pure-int leaves to max\n"
              "       cubalc plate scaleflat <path> [needle] <factor>  # SCALEFLAT dual · multiply pure-int leaves\n"
              "       cubalc plate hasflat <path> [needle]  # HASFLAT dual · leaf-path presence 0|1\n"
              "       cubalc plate countflat <path> [needle]  # COUNTFLAT dual · leaf-path match count\n"
              "       cubalc plate hasflatn <path> [needle]  # HASFLATN dual · pure-int leaf presence 0|1\n"
              "       cubalc plate countflatn <path> [needle]  # COUNTFLATN dual · pure-int leaf match count\n"
              "       cubalc plate pathsflat <path> [needle]  # PATHSFLAT dual · matching leaf paths bag\n"
              "       cubalc plate valsflat <path> [needle]  # VALSFLAT dual · matching leaf values bag\n"
              "       cubalc plate pathsflatn <path> [needle]  # PATHSFLATN dual · pure-int leaf paths bag\n"
              "       cubalc plate valsflatn <path> [needle]  # VALSFLATN dual · pure-int leaf values bag\n"
              "       cubalc plate getflat <path> <needle> [OR def]  # GETFLAT dual · first matching leaf value\n"
              "       cubalc plate lastflat <path> <needle> [OR def]  # LASTFLAT dual · last matching leaf value\n"
              "       cubalc plate nthflat <path> <needle> <i> [OR def]  # NTHFLAT dual · 0-based Nth matching leaf\n"
              "       cubalc plate needflat <path> <needle> [needle…]  # NEEDFLAT dual · fail-fast leaf path contract\n"
              "       cubalc plate getflatn <path> <needle> [OR n]  # GETFLATN dual · first pure-int leaf → n\n"
              "       cubalc plate lastflatn <path> <needle> [OR n]  # LASTFLATN dual · last pure-int leaf → n\n"
              "       cubalc plate nthflatn <path> <needle> <i> [OR n]  # NTHFLATN dual · 0-based Nth pure-int leaf → n\n"
              "       cubalc plate typeflat <path> <needle>  # TYPEFLAT dual · first matching leaf kind\n"
              "       cubalc plate freqflat <path> [needle]  # FREQFLAT dual · value frequency of matching leaves\n"
              "       cubalc plate modeflat <path> [needle]  # MODEFLAT dual · dominant matching leaf value\n"
              "       cubalc plate pathbyval <path> <value> [OR def]  # PATHBYVAL dual · first leaf path by exact value\n"
              "       cubalc plate uniqflat <path> [needle]  # UNIQFLAT dual · unique matching leaf values\n"
              "       cubalc plate pathsbyval <path> <value>  # PATHSBYVAL dual · all leaf paths by exact value\n"
              "       cubalc plate countbyval <path> <value>  # COUNTBYVAL dual · count leaves by exact value\n"
              "       cubalc plate hasval <path> <value>  # HASVAL dual · soft presence of exact leaf value\n"
              "       cubalc plate setbyval <path> <old> <new>  # SETBYVAL dual · rewrite leaf values by exact match\n"
              "       cubalc plate delbyval <path> <value>  # DELBYVAL dual · drop leaves by exact value\n"
              "       cubalc plate alleqflat <path> [needle]  # ALLEQFLAT dual · all matching leaf values identical\n"
              "       cubalc plate firstuneq <path> [needle]  # FIRSTUNEQFLAT dual · first path that breaks uniformity\n"
              "       cubalc plate uneqpaths <path> [needle]  # UNEQPATHS dual · all paths that break uniformity\n"
              "       cubalc plate uniform <path> [needle]  # UNIFORMFLAT dual · one-shot eq + first path + all diverge paths\n"
              "       cubalc plate pretty <path>  # PRETTYP dual · 2-space indented plate (no jq)\n"
              "       cubalc plate needflatn <path> <needle> [needle…]  # NEEDFLATN dual · fail-fast pure-int leaf + peel\n"
              "       cubalc plate len|empty|vals <path> [nest.path]  # LENP/EMPTYP/VALSP duals\n"
              "       cubalc plate nestget <path> <nest> <field> [OR def]\n"
              "       cubalc plate nestset <path> <nest> <field> <value>\n"
              "       cubalc plate nestinc <path> <nest> <field> [delta]\n"
              "       cubalc plate nestdel <path> <nest> <field>\n"
              "       cubalc plate nestkeys <path> <nest>\n"
              "       cubalc plate nesthas <path> <nest> <field>\n"
              "       cubalc plate nestpick <path> <nest> <k1> [k2…]  # PICKOBJ dual\n"
              "       cubalc plate nestomit <path> <nest> <k1> [k2…]  # OMITOBJ dual\n"
              "       cubalc plate nestrename <path> <nest> <old> <new>  # RENAMEPOBJ dual · path ok\n"
              "       cubalc plate nestcopy <path> <nest> <src> <dst>    # COPYPOBJ dual · path ok\n"
              "       cubalc plate nestswap <path> <nest> <a> <b>        # SWAPPOBJ dual · path ok\n"
              "       cubalc plate nestsum|nestavg|nestmedian <path> <nest>  # SUMNOBJ duals\n"
              "       cubalc plate nesttop|nestbot <path> <nest> [n]  # TOPNOBJ duals (no write)\n"
              "       cubalc plate nestsort <path> <nest> [ASC|DESC]  # SORTOBJ dual (write)\n"
              "       cubalc plate nestsortbag <path> <nest> [ASC|DESC]  # SORTBAGOBJ dual (no write)\n"
              "       cubalc plate fill [-s|--strict] <path> <tmpl|@file> [out]\n"
              "       cubalc plate fillkeys <path> <tmpl|@file>\n"
              "       cubalc plate ensure <path> [seed|@file]  # create-or-keep\n"
              "       cubalc plate merge  <path> <overlay|@file>\n"
              "       cubalc plate eq|ne|diff|changelog <a.json> <b.json>\n"
              "       cubalc plate has|need <path> <k1> [k2 …]\n"
              "       cubalc plate pluck <path> <k1> [k2 …]  # multi-key peel bag · PLUCKP dual (paths ok)\n"
              "       cubalc plate pluckobj <path> <nest> <k1> [k2…]  # PLUCKOBJ dual · nest path ok\n"
              "       cubalc plate pick|omit <path> <k1> [k2 …]  # keep/drop keys · PICKP/OMITP\n"
              "       cubalc plate sum|avg|median|mean|p50 <path>  # aggregates · SUMNP duals\n"
              "       cubalc plate top|bot <path> [n]          # top/bot N keys · TOPNP duals\n");
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
             "\"err\":\"need op and/or path\",\"version\":\"%s\","
             "\"ops\":[\"show\",\"get\",\"getn\",\"getobj\",\"setobj\",\"mergeobj\",\"defaultobj\","
             "\"type\",\"set\",\"default\",\"toggle\",\"rename\",\"copy\",\"swap\","
             "\"inc\",\"del\",\"keys\",\"leaves\",\"pathkeys\",\"flat\",\"flatkv\",\"unflat\",\"unflatkv\",\"diffflat\",\"pathdiff\",\"grepf\",\"grepflat\",\"grepvf\",\"prune\",\"keeponly\",\"mergeflat\",\"renameflat\",\"setflat\",\"incflat\",\"sumflat\",\"avgflat\",\"medianflat\",\"toppath\",\"botpath\",\"threshflat\",\"dropzeroflat\",\"capflat\",\"scaleflat\",\"hasflat\",\"countflat\",\"hasflatn\",\"countflatn\",\"pathsflat\",\"valsflat\",\"pathsflatn\",\"valsflatn\",\"getflat\",\"lastflat\",\"nthflat\",\"needflat\",\"getflatn\",\"lastflatn\",\"nthflatn\",\"typeflat\",\"freqflat\",\"modeflat\",\"pathbyval\",\"uniqflat\",\"pathsbyval\",\"countbyval\",\"hasval\",\"setbyval\",\"delbyval\",\"alleqflat\",\"firstuneq\",\"uneqpaths\",\"uniform\",\"pretty\",\"needflatn\",\"len\",\"empty\",\"vals\","
             "\"nestget\",\"nestset\",\"nestinc\",\"nestdel\",\"nestkeys\",\"nesthas\",\"nestpick\",\"nestomit\","
             "\"nestrename\",\"nestcopy\",\"nestswap\",\"pluckobj\","
             "\"nestsum\",\"nestavg\",\"nestmedian\",\"nesttop\",\"nestbot\","
             "\"nestsort\",\"nestsortbag\","
             "\"fill\",\"fillkeys\",\"ensure\",\"merge\",\"eq\",\"ne\",\"diff\",\"changelog\","
             "\"has\",\"need\",\"pluck\",\"pick\",\"omit\",\"sum\",\"avg\",\"median\",\"top\",\"bot\"]}\n",
             CUBALC_LANG_VERSION);
      return 2;
    }

        /* parse: op path …  OR  path (bare show) */
    if (strcmp(argv[2], "show") == 0 || strcmp(argv[2], "dump") == 0 ||
        strcmp(argv[2], "cat") == 0 || strcmp(argv[2], "read") == 0 ||
        strcmp(argv[2], "get") == 0 || strcmp(argv[2], "peek") == 0 ||
        strcmp(argv[2], "getn") == 0 || strcmp(argv[2], "num") == 0 ||
        strcmp(argv[2], "getpn") == 0 || strcmp(argv[2], "jgetn") == 0 ||
        strcmp(argv[2], "int") == 0 ||
        strcmp(argv[2], "getobj") == 0 || strcmp(argv[2], "peekobj") == 0 ||
        strcmp(argv[2], "objget") == 0 ||
        strcmp(argv[2], "setobj") == 0 || strcmp(argv[2], "putobj") == 0 ||
        strcmp(argv[2], "nestp") == 0 || strcmp(argv[2], "putnest") == 0 ||
        strcmp(argv[2], "mergeobj") == 0 || strcmp(argv[2], "patchnest") == 0 ||
        strcmp(argv[2], "updateobj") == 0 ||
        strcmp(argv[2], "defaultobj") == 0 || strcmp(argv[2], "ensureobj") == 0 ||
        strcmp(argv[2], "defaultnest") == 0 ||
        strcmp(argv[2], "type") == 0 || strcmp(argv[2], "kind") == 0 ||
        strcmp(argv[2], "typeof") == 0 || strcmp(argv[2], "typep") == 0 ||
        strcmp(argv[2], "jtype") == 0 ||
        strcmp(argv[2], "set") == 0 || strcmp(argv[2], "put") == 0 ||
        strcmp(argv[2], "default") == 0 || strcmp(argv[2], "ensurekey") == 0 ||
        strcmp(argv[2], "defaultp") == 0 || strcmp(argv[2], "setdefault") == 0 ||
        strcmp(argv[2], "toggle") == 0 || strcmp(argv[2], "flip") == 0 ||
        strcmp(argv[2], "togglep") == 0 || strcmp(argv[2], "flipp") == 0 ||
        strcmp(argv[2], "rename") == 0 || strcmp(argv[2], "move") == 0 ||
        strcmp(argv[2], "mv") == 0 || strcmp(argv[2], "renamep") == 0 ||
        strcmp(argv[2], "movekey") == 0 || strcmp(argv[2], "rekey") == 0 ||
        strcmp(argv[2], "copy") == 0 || strcmp(argv[2], "cp") == 0 ||
        strcmp(argv[2], "dup") == 0 || strcmp(argv[2], "copyp") == 0 ||
        strcmp(argv[2], "dupkey") == 0 ||
        strcmp(argv[2], "swap") == 0 || strcmp(argv[2], "xchg") == 0 ||
        strcmp(argv[2], "swapp") == 0 || strcmp(argv[2], "exchange") == 0 ||
        strcmp(argv[2], "inc") == 0 || strcmp(argv[2], "bump") == 0 ||
        strcmp(argv[2], "del") == 0 || strcmp(argv[2], "rm") == 0 ||
        strcmp(argv[2], "drop") == 0 || strcmp(argv[2], "keys") == 0 ||
        strcmp(argv[2], "list") == 0 || strcmp(argv[2], "ls") == 0 ||
        strcmp(argv[2], "leaves") == 0 || strcmp(argv[2], "leafkeys") == 0 ||
        strcmp(argv[2], "pathkeys") == 0 || strcmp(argv[2], "dotpaths") == 0 ||
        strcmp(argv[2], "flatpaths") == 0 || strcmp(argv[2], "dotkeys") == 0 ||
        strcmp(argv[2], "flat") == 0 || strcmp(argv[2], "flatkv") == 0 ||
        strcmp(argv[2], "leafkv") == 0 || strcmp(argv[2], "flatten") == 0 ||
        strcmp(argv[2], "flattenp") == 0 || strcmp(argv[2], "tokvpath") == 0 ||
        strcmp(argv[2], "unflat") == 0 || strcmp(argv[2], "unflatkv") == 0 ||
        strcmp(argv[2], "fromflat") == 0 || strcmp(argv[2], "unflatten") == 0 ||
        strcmp(argv[2], "unflattenp") == 0 || strcmp(argv[2], "pathfromkv") == 0 ||
        strcmp(argv[2], "diffflat") == 0 || strcmp(argv[2], "clogflat") == 0 ||
        strcmp(argv[2], "changelogflat") == 0 || strcmp(argv[2], "leafdiff") == 0 ||
        strcmp(argv[2], "pathdiff") == 0 || strcmp(argv[2], "diffpath") == 0 ||
        strcmp(argv[2], "diffpaths") == 0 ||
        strcmp(argv[2], "grepf") == 0 || strcmp(argv[2], "grepflat") == 0 ||
        strcmp(argv[2], "keepflat") == 0 || strcmp(argv[2], "filterflat") == 0 ||
        strcmp(argv[2], "grepvf") == 0 || strcmp(argv[2], "grepvflat") == 0 ||
        strcmp(argv[2], "dropflat") == 0 || strcmp(argv[2], "grepfi") == 0 ||
        strcmp(argv[2], "grepflati") == 0 ||
        strcmp(argv[2], "prune") == 0 || strcmp(argv[2], "pruneflat") == 0 ||
        strcmp(argv[2], "delflat") == 0 || strcmp(argv[2], "scrubflat") == 0 ||
        strcmp(argv[2], "keeponly") == 0 || strcmp(argv[2], "keeponlyflat") == 0 ||
        strcmp(argv[2], "retainflat") == 0 || strcmp(argv[2], "projectflat") == 0 ||
        strcmp(argv[2], "mergeflat") == 0 || strcmp(argv[2], "overlayflat") == 0 ||
        strcmp(argv[2], "patchflat") == 0 || strcmp(argv[2], "deepmerge") == 0 ||
        strcmp(argv[2], "leafmerge") == 0 || strcmp(argv[2], "syncflat") == 0 ||
        strcmp(argv[2], "renameflat") == 0 || strcmp(argv[2], "moveflat") == 0 ||
        strcmp(argv[2], "prefixflat") == 0 || strcmp(argv[2], "repathflat") == 0 ||
        strcmp(argv[2], "rekeyflat") == 0 ||
        strcmp(argv[2], "setflat") == 0 || strcmp(argv[2], "mapflat") == 0 ||
        strcmp(argv[2], "setleaf") == 0 || strcmp(argv[2], "putflat") == 0 ||
        strcmp(argv[2], "bulkset") == 0 ||
        strcmp(argv[2], "incflat") == 0 || strcmp(argv[2], "bumpflat") == 0 ||
        strcmp(argv[2], "addflat") == 0 || strcmp(argv[2], "tickflat") == 0 ||
        strcmp(argv[2], "bulkinc") == 0 || strcmp(argv[2], "decflat") == 0 ||
        strcmp(argv[2], "sumflat") == 0 || strcmp(argv[2], "totalflat") == 0 ||
        strcmp(argv[2], "leafsum") == 0 || strcmp(argv[2], "flattotal") == 0 ||
        strcmp(argv[2], "avgflat") == 0 || strcmp(argv[2], "meanflat") == 0 ||
        strcmp(argv[2], "avgleaf") == 0 || strcmp(argv[2], "flatmean") == 0 ||
        strcmp(argv[2], "leafmean") == 0 ||
        strcmp(argv[2], "medianflat") == 0 || strcmp(argv[2], "p50flat") == 0 ||
        strcmp(argv[2], "midflat") == 0 || strcmp(argv[2], "flatmedian") == 0 ||
        strcmp(argv[2], "leafmedian") == 0 ||
        strcmp(argv[2], "toppath") == 0 || strcmp(argv[2], "toppathflat") == 0 ||
        strcmp(argv[2], "maxpath") == 0 || strcmp(argv[2], "maxpathflat") == 0 ||
        strcmp(argv[2], "botpath") == 0 || strcmp(argv[2], "botpathflat") == 0 ||
        strcmp(argv[2], "minpath") == 0 || strcmp(argv[2], "minpathflat") == 0 ||
        strcmp(argv[2], "threshflat") == 0 || strcmp(argv[2], "keepvflat") == 0 ||
        strcmp(argv[2], "mincountflat") == 0 ||
        strcmp(argv[2], "dropzeroflat") == 0 || strcmp(argv[2], "nzflat") == 0 ||
        strcmp(argv[2], "capflat") == 0 || strcmp(argv[2], "clampflat") == 0 ||
        strcmp(argv[2], "maxvflat") == 0 || strcmp(argv[2], "ceilflat") == 0 ||
        strcmp(argv[2], "scaleflat") == 0 || strcmp(argv[2], "mulflat") == 0 ||
        strcmp(argv[2], "timesflat") == 0 || strcmp(argv[2], "multflat") == 0 ||
        strcmp(argv[2], "hasflat") == 0 || strcmp(argv[2], "anyflat") == 0 ||
        strcmp(argv[2], "countflat") == 0 || strcmp(argv[2], "nflat") == 0 ||
        strcmp(argv[2], "leafcount") == 0 ||
        strcmp(argv[2], "hasflatn") == 0 || strcmp(argv[2], "anyflatn") == 0 ||
        strcmp(argv[2], "countflatn") == 0 || strcmp(argv[2], "nflatn") == 0 ||
        strcmp(argv[2], "leafcountn") == 0 || strcmp(argv[2], "hitflatn") == 0 ||
        strcmp(argv[2], "pathsflat") == 0 || strcmp(argv[2], "matchpaths") == 0 ||
        strcmp(argv[2], "valsflat") == 0 || strcmp(argv[2], "matchvals") == 0 ||
        strcmp(argv[2], "leafvals") == 0 ||
        strcmp(argv[2], "pathsflatn") == 0 || strcmp(argv[2], "matchpathsn") == 0 ||
        strcmp(argv[2], "valsflatn") == 0 || strcmp(argv[2], "matchvalsn") == 0 ||
        strcmp(argv[2], "leafvalsn") == 0 || strcmp(argv[2], "numvalsflat") == 0 ||
        strcmp(argv[2], "getflat") == 0 || strcmp(argv[2], "firstflat") == 0 ||
        strcmp(argv[2], "peelflat") == 0 || strcmp(argv[2], "leafget") == 0 ||
        strcmp(argv[2], "lastflat") == 0 || strcmp(argv[2], "endflat") == 0 ||
        strcmp(argv[2], "tailflat") == 0 || strcmp(argv[2], "lastleafp") == 0 ||
        strcmp(argv[2], "nthflat") == 0 || strcmp(argv[2], "indexflat") == 0 ||
        strcmp(argv[2], "atflat") == 0 || strcmp(argv[2], "nthleaf") == 0 ||
        strcmp(argv[2], "needflat") == 0 || strcmp(argv[2], "requireflat") == 0 ||
        strcmp(argv[2], "mustflat") == 0 ||
        strcmp(argv[2], "getflatn") == 0 || strcmp(argv[2], "firstflatn") == 0 ||
        strcmp(argv[2], "numflat") == 0 || strcmp(argv[2], "getnflat") == 0 ||
        strcmp(argv[2], "lastflatn") == 0 || strcmp(argv[2], "endflatn") == 0 ||
        strcmp(argv[2], "tailflatn") == 0 || strcmp(argv[2], "lastnumflat") == 0 ||
        strcmp(argv[2], "nthflatn") == 0 || strcmp(argv[2], "indexflatn") == 0 ||
        strcmp(argv[2], "atflatn") == 0 || strcmp(argv[2], "nthnumflat") == 0 ||
        strcmp(argv[2], "typeflat") == 0 || strcmp(argv[2], "kindflat") == 0 ||
        strcmp(argv[2], "leaftype") == 0 || strcmp(argv[2], "flattype") == 0 ||
        strcmp(argv[2], "freqflat") == 0 || strcmp(argv[2], "histflat") == 0 ||
        strcmp(argv[2], "leaffreq") == 0 || strcmp(argv[2], "countvalflat") == 0 ||
        strcmp(argv[2], "modeflat") == 0 || strcmp(argv[2], "topvalflat") == 0 ||
        strcmp(argv[2], "majorflat") == 0 || strcmp(argv[2], "leafmode") == 0 ||
        strcmp(argv[2], "pathbyval") == 0 || strcmp(argv[2], "valpath") == 0 ||
        strcmp(argv[2], "findval") == 0 || strcmp(argv[2], "pathofval") == 0 ||
        strcmp(argv[2], "uniqflat") == 0 || strcmp(argv[2], "distinctflat") == 0 ||
        strcmp(argv[2], "uniqueflat") == 0 || strcmp(argv[2], "leafuniq") == 0 ||
        strcmp(argv[2], "pathsbyval") == 0 || strcmp(argv[2], "allpathsbyval") == 0 ||
        strcmp(argv[2], "findvals") == 0 || strcmp(argv[2], "pathsofval") == 0 ||
        strcmp(argv[2], "countbyval") == 0 || strcmp(argv[2], "nbyval") == 0 ||
        strcmp(argv[2], "valcount") == 0 || strcmp(argv[2], "nval") == 0 ||
        strcmp(argv[2], "hasval") == 0 || strcmp(argv[2], "hasvalflat") == 0 ||
        strcmp(argv[2], "anyval") == 0 || strcmp(argv[2], "valhas") == 0 ||
        strcmp(argv[2], "setbyval") == 0 || strcmp(argv[2], "replaceval") == 0 ||
        strcmp(argv[2], "rewval") == 0 || strcmp(argv[2], "mapval") == 0 ||
        strcmp(argv[2], "delbyval") == 0 || strcmp(argv[2], "dropval") == 0 ||
        strcmp(argv[2], "rmval") == 0 || strcmp(argv[2], "pruneval") == 0 ||
        strcmp(argv[2], "alleqflat") == 0 || strcmp(argv[2], "samevalflat") == 0 ||
        strcmp(argv[2], "eqvalsflat") == 0 || strcmp(argv[2], "allsameflat") == 0 ||
        strcmp(argv[2], "firstuneq") == 0 || strcmp(argv[2], "firstuneqflat") == 0 ||
        strcmp(argv[2], "pathuneq") == 0 || strcmp(argv[2], "diverge1") == 0 ||
        strcmp(argv[2], "uneqpaths") == 0 || strcmp(argv[2], "divergepaths") == 0 ||
        strcmp(argv[2], "alluneq") == 0 || strcmp(argv[2], "pathsuneq") == 0 ||
        strcmp(argv[2], "uniform") == 0 || strcmp(argv[2], "uniformflat") == 0 ||
        strcmp(argv[2], "checkflat") == 0 || strcmp(argv[2], "samecheck") == 0 ||
        strcmp(argv[2], "pretty") == 0 || strcmp(argv[2], "prettyp") == 0 ||
        strcmp(argv[2], "jsonpretty") == 0 || strcmp(argv[2], "prettyjson") == 0 ||
        strcmp(argv[2], "needflatn") == 0 || strcmp(argv[2], "requireflatn") == 0 ||
        strcmp(argv[2], "mustflatn") == 0 || strcmp(argv[2], "needintflat") == 0 ||
        strcmp(argv[2], "len") == 0 || strcmp(argv[2], "length") == 0 ||
        strcmp(argv[2], "nkeys") == 0 || strcmp(argv[2], "size") == 0 ||
        strcmp(argv[2], "countkeys") == 0 ||
        strcmp(argv[2], "empty") == 0 || strcmp(argv[2], "isempty") == 0 ||
        strcmp(argv[2], "emptyp") == 0 ||
        strcmp(argv[2], "vals") == 0 || strcmp(argv[2], "values") == 0 ||
        strcmp(argv[2], "valsp") == 0 ||
        strcmp(argv[2], "fill") == 0 || strcmp(argv[2], "expand") == 0 ||
        strcmp(argv[2], "template") == 0 || strcmp(argv[2], "render") == 0 ||
        strcmp(argv[2], "fillkeys") == 0 || strcmp(argv[2], "fill-keys") == 0 ||
        strcmp(argv[2], "tmplkeys") == 0 ||
        strcmp(argv[2], "ensure") == 0 || strcmp(argv[2], "seed") == 0 ||
        strcmp(argv[2], "touch") == 0 || strcmp(argv[2], "create") == 0 ||
        strcmp(argv[2], "merge") == 0 || strcmp(argv[2], "patch") == 0 ||
        strcmp(argv[2], "overlay") == 0 || strcmp(argv[2], "apply") == 0 ||
        strcmp(argv[2], "eq") == 0 || strcmp(argv[2], "same") == 0 ||
        strcmp(argv[2], "equal") == 0 || strcmp(argv[2], "cmp") == 0 ||
        strcmp(argv[2], "ne") == 0 || strcmp(argv[2], "neq") == 0 ||
        strcmp(argv[2], "differ") == 0 || strcmp(argv[2], "unequal") == 0 ||
        strcmp(argv[2], "diff") == 0 || strcmp(argv[2], "changed") == 0 ||
        strcmp(argv[2], "delta") == 0 ||
        strcmp(argv[2], "changelog") == 0 || strcmp(argv[2], "clog") == 0 ||
        strcmp(argv[2], "changes") == 0 || strcmp(argv[2], "logdiff") == 0 ||
        strcmp(argv[2], "has") == 0 || strcmp(argv[2], "hasall") == 0 ||
        strcmp(argv[2], "has-keys") == 0 || strcmp(argv[2], "check") == 0 ||
        strcmp(argv[2], "need") == 0 || strcmp(argv[2], "require") == 0 ||
        strcmp(argv[2], "needkeys") == 0 || strcmp(argv[2], "require-keys") == 0 ||
        strcmp(argv[2], "pluck") == 0 || strcmp(argv[2], "getall") == 0 ||
        strcmp(argv[2], "getpall") == 0 || strcmp(argv[2], "peel") == 0 ||
        strcmp(argv[2], "values") == 0 || strcmp(argv[2], "pluckp") == 0 ||
        strcmp(argv[2], "pluckobj") == 0 || strcmp(argv[2], "nestpluck") == 0 ||
        strcmp(argv[2], "objpluck") == 0 || strcmp(argv[2], "plucknest") == 0 ||
        strcmp(argv[2], "pick") == 0 || strcmp(argv[2], "keep") == 0 ||
        strcmp(argv[2], "select") == 0 || strcmp(argv[2], "project") == 0 ||
        strcmp(argv[2], "omit") == 0 || strcmp(argv[2], "strip") == 0 ||
        strcmp(argv[2], "dropkeys") == 0 || strcmp(argv[2], "drop-keys") == 0 ||
        strcmp(argv[2], "sum") == 0 || strcmp(argv[2], "total") == 0 ||
        strcmp(argv[2], "avg") == 0 || strcmp(argv[2], "mean") == 0 ||
        strcmp(argv[2], "average") == 0 ||
        strcmp(argv[2], "median") == 0 || strcmp(argv[2], "p50") == 0 ||
        strcmp(argv[2], "mid") == 0 ||
        strcmp(argv[2], "top") == 0 || strcmp(argv[2], "topn") == 0 ||
        strcmp(argv[2], "head") == 0 ||
        strcmp(argv[2], "bot") == 0 || strcmp(argv[2], "bottom") == 0 ||
        strcmp(argv[2], "botn") == 0 || strcmp(argv[2], "tail") == 0 ||
        strcmp(argv[2], "nestget") == 0 || strcmp(argv[2], "nget") == 0 ||
        strcmp(argv[2], "getnest") == 0 || strcmp(argv[2], "getpobj") == 0 ||
        strcmp(argv[2], "nestset") == 0 || strcmp(argv[2], "nset") == 0 ||
        strcmp(argv[2], "setnest") == 0 || strcmp(argv[2], "setpobj") == 0 ||
        strcmp(argv[2], "nestinc") == 0 || strcmp(argv[2], "ninc") == 0 ||
        strcmp(argv[2], "incnest") == 0 || strcmp(argv[2], "incobj") == 0 ||
        strcmp(argv[2], "nestdel") == 0 || strcmp(argv[2], "ndel") == 0 ||
        strcmp(argv[2], "delnest") == 0 || strcmp(argv[2], "delpobj") == 0 ||
        strcmp(argv[2], "nestkeys") == 0 || strcmp(argv[2], "nkeys") == 0 ||
        strcmp(argv[2], "keysobj") == 0 || strcmp(argv[2], "keysnest") == 0 ||
        strcmp(argv[2], "nesthas") == 0 || strcmp(argv[2], "nhas") == 0 ||
        strcmp(argv[2], "haspobj") == 0 || strcmp(argv[2], "hasnest") == 0 ||
        strcmp(argv[2], "nestpick") == 0 || strcmp(argv[2], "npick") == 0 ||
        strcmp(argv[2], "picknest") == 0 || strcmp(argv[2], "pickobj") == 0 ||
        strcmp(argv[2], "nestomit") == 0 || strcmp(argv[2], "nomit") == 0 ||
        strcmp(argv[2], "omitnest") == 0 || strcmp(argv[2], "omitobj") == 0 ||
        strcmp(argv[2], "stripnest") == 0 ||
        strcmp(argv[2], "nestrename") == 0 || strcmp(argv[2], "nrename") == 0 ||
        strcmp(argv[2], "renamenest") == 0 || strcmp(argv[2], "renamepobj") == 0 ||
        strcmp(argv[2], "nestcopy") == 0 || strcmp(argv[2], "ncopy") == 0 ||
        strcmp(argv[2], "copynest") == 0 || strcmp(argv[2], "copypobj") == 0 ||
        strcmp(argv[2], "nestswap") == 0 || strcmp(argv[2], "nswap") == 0 ||
        strcmp(argv[2], "swapnest") == 0 || strcmp(argv[2], "swappobj") == 0 ||
        strcmp(argv[2], "nestsum") == 0 || strcmp(argv[2], "nsum") == 0 ||
        strcmp(argv[2], "sumnest") == 0 || strcmp(argv[2], "sumnobj") == 0 ||
        strcmp(argv[2], "nestavg") == 0 || strcmp(argv[2], "navg") == 0 ||
        strcmp(argv[2], "avgnest") == 0 || strcmp(argv[2], "nestmean") == 0 ||
        strcmp(argv[2], "nestmedian") == 0 || strcmp(argv[2], "nmedian") == 0 ||
        strcmp(argv[2], "nestp50") == 0 || strcmp(argv[2], "mediannest") == 0 ||
        strcmp(argv[2], "nesttop") == 0 || strcmp(argv[2], "ntop") == 0 ||
        strcmp(argv[2], "topnest") == 0 || strcmp(argv[2], "topnobj") == 0 ||
        strcmp(argv[2], "nestbot") == 0 || strcmp(argv[2], "nbot") == 0 ||
        strcmp(argv[2], "botnest") == 0 || strcmp(argv[2], "botnobj") == 0 ||
        strcmp(argv[2], "nestbottom") == 0 ||
        strcmp(argv[2], "nestsort") == 0 || strcmp(argv[2], "nsort") == 0 ||
        strcmp(argv[2], "sortnest") == 0 || strcmp(argv[2], "sortobj") == 0 ||
        strcmp(argv[2], "nestsortbag") == 0 || strcmp(argv[2], "nsortbag") == 0 ||
        strcmp(argv[2], "sortbagnest") == 0 || strcmp(argv[2], "sortbagobj") == 0 ||
        strcmp(argv[2], "nestsortfreq") == 0 || strcmp(argv[2], "sortfreqobj") == 0) {
      op = argv[2];
      if (strcmp(op, "dump") == 0 || strcmp(op, "cat") == 0 || strcmp(op, "read") == 0)
        op = "show";
      else if (strcmp(op, "peek") == 0)
        op = "get";
      else if (strcmp(op, "num") == 0 || strcmp(op, "getpn") == 0 ||
               strcmp(op, "jgetn") == 0 || strcmp(op, "int") == 0)
        op = "getn";
      else if (strcmp(op, "peekobj") == 0 || strcmp(op, "objget") == 0)
        op = "getobj";
      else if (strcmp(op, "putobj") == 0 || strcmp(op, "nestp") == 0 ||
               strcmp(op, "putnest") == 0)
        op = "setobj";
      else if (strcmp(op, "patchnest") == 0 || strcmp(op, "updateobj") == 0)
        op = "mergeobj";
      else if (strcmp(op, "ensureobj") == 0 || strcmp(op, "defaultnest") == 0)
        op = "defaultobj";
      else if (strcmp(op, "kind") == 0 || strcmp(op, "typeof") == 0 ||
               strcmp(op, "typep") == 0 || strcmp(op, "jtype") == 0)
        op = "type";
      else if (strcmp(op, "put") == 0)
        op = "set";
      else if (strcmp(op, "ensurekey") == 0 || strcmp(op, "defaultp") == 0 ||
               strcmp(op, "setdefault") == 0)
        op = "default";
      else if (strcmp(op, "flip") == 0 || strcmp(op, "togglep") == 0 ||
               strcmp(op, "flipp") == 0)
        op = "toggle";
      else if (strcmp(op, "move") == 0 || strcmp(op, "mv") == 0 ||
               strcmp(op, "renamep") == 0 || strcmp(op, "movekey") == 0 ||
               strcmp(op, "rekey") == 0)
        op = "rename";
      else if (strcmp(op, "cp") == 0 || strcmp(op, "dup") == 0 ||
               strcmp(op, "copyp") == 0 || strcmp(op, "dupkey") == 0)
        op = "copy";
      else if (strcmp(op, "xchg") == 0 || strcmp(op, "swapp") == 0 ||
               strcmp(op, "exchange") == 0)
        op = "swap";
      else if (strcmp(op, "bump") == 0)
        op = "inc";
      else if (strcmp(op, "rm") == 0 || strcmp(op, "drop") == 0)
        op = "del";
      else if (strcmp(op, "list") == 0 || strcmp(op, "ls") == 0)
        op = "keys";
      else if (strcmp(op, "leafkeys") == 0 || strcmp(op, "pathkeys") == 0 ||
               strcmp(op, "dotpaths") == 0 || strcmp(op, "flatpaths") == 0 ||
               strcmp(op, "dotkeys") == 0 || strcmp(op, "leafpaths") == 0)
        op = "leaves";
      else if (strcmp(op, "flatkv") == 0 || strcmp(op, "leafkv") == 0 ||
               strcmp(op, "flatten") == 0 || strcmp(op, "flattenp") == 0 ||
               strcmp(op, "tokvpath") == 0 || strcmp(op, "pathtokv") == 0 ||
               strcmp(op, "plateflat") == 0)
        op = "flat";
      else if (strcmp(op, "unflatkv") == 0 || strcmp(op, "fromflat") == 0 ||
               strcmp(op, "unflatten") == 0 || strcmp(op, "unflattenp") == 0 ||
               strcmp(op, "pathfromkv") == 0 || strcmp(op, "applyflat") == 0)
        op = "unflat";
      else if (strcmp(op, "clogflat") == 0 || strcmp(op, "changelogflat") == 0 ||
               strcmp(op, "leafdiff") == 0 || strcmp(op, "flatdiff") == 0)
        op = "diffflat";
      else if (strcmp(op, "diffpath") == 0 || strcmp(op, "diffpaths") == 0 ||
               strcmp(op, "changedpaths") == 0)
        op = "pathdiff";
      else if (strcmp(op, "grepflat") == 0 || strcmp(op, "keepflat") == 0 ||
               strcmp(op, "filterflat") == 0 || strcmp(op, "leafgrep") == 0)
        op = "grepf";
      else if (strcmp(op, "grepvflat") == 0 || strcmp(op, "dropflat") == 0 ||
               strcmp(op, "vgrepflat") == 0)
        op = "grepvf";
      else if (strcmp(op, "grepflati") == 0 || strcmp(op, "igrepflat") == 0 ||
               strcmp(op, "grepiflat") == 0)
        op = "grepfi";
      else if (strcmp(op, "pruneflat") == 0 || strcmp(op, "delflat") == 0 ||
               strcmp(op, "scrubflat") == 0 || strcmp(op, "rmflat") == 0)
        op = "prune";
      else if (strcmp(op, "keeponlyflat") == 0 || strcmp(op, "retainflat") == 0 ||
               strcmp(op, "projectflat") == 0 || strcmp(op, "retain") == 0)
        op = "keeponly";
      else if (strcmp(op, "overlayflat") == 0 || strcmp(op, "patchflat") == 0 ||
               strcmp(op, "deepmerge") == 0 || strcmp(op, "leafmerge") == 0 ||
               strcmp(op, "syncflat") == 0 || strcmp(op, "applyflat") == 0)
        op = "mergeflat";
      else if (strcmp(op, "moveflat") == 0 || strcmp(op, "prefixflat") == 0 ||
               strcmp(op, "repathflat") == 0 || strcmp(op, "rekeyflat") == 0 ||
               strcmp(op, "mvflat") == 0)
        op = "renameflat";
      else if (strcmp(op, "mapflat") == 0 || strcmp(op, "setleaf") == 0 ||
               strcmp(op, "putflat") == 0 || strcmp(op, "bulkset") == 0 ||
               strcmp(op, "assignflat") == 0)
        op = "setflat";
      else if (strcmp(op, "bumpflat") == 0 || strcmp(op, "addflat") == 0 ||
               strcmp(op, "tickflat") == 0 || strcmp(op, "bulkinc") == 0)
        op = "incflat";
      else if (strcmp(op, "decflat") == 0)
        op = "decflat";
      else if (strcmp(op, "totalflat") == 0 || strcmp(op, "leafsum") == 0 ||
               strcmp(op, "flattotal") == 0 || strcmp(op, "sumleaf") == 0)
        op = "sumflat";
      else if (strcmp(op, "meanflat") == 0 || strcmp(op, "avgleaf") == 0 ||
               strcmp(op, "flatmean") == 0 || strcmp(op, "leafmean") == 0 ||
               strcmp(op, "avgallflat") == 0)
        op = "avgflat";
      else if (strcmp(op, "p50flat") == 0 || strcmp(op, "midflat") == 0 ||
               strcmp(op, "flatmedian") == 0 || strcmp(op, "leafmedian") == 0 ||
               strcmp(op, "medallflat") == 0)
        op = "medianflat";
      else if (strcmp(op, "toppathflat") == 0 || strcmp(op, "maxpath") == 0 ||
               strcmp(op, "maxpathflat") == 0 || strcmp(op, "topflat") == 0)
        op = "toppath";
      else if (strcmp(op, "botpathflat") == 0 || strcmp(op, "minpath") == 0 ||
               strcmp(op, "minpathflat") == 0 || strcmp(op, "botflat") == 0)
        op = "botpath";
      else if (strcmp(op, "keepvflat") == 0 || strcmp(op, "mincountflat") == 0 ||
               strcmp(op, "keepminflat") == 0)
        op = "threshflat";
      else if (strcmp(op, "nzflat") == 0 || strcmp(op, "keepnzflat") == 0)
        op = "dropzeroflat";
      else if (strcmp(op, "clampflat") == 0 || strcmp(op, "maxvflat") == 0 ||
               strcmp(op, "ceilflat") == 0 || strcmp(op, "maxcapflat") == 0)
        op = "capflat";
      else if (strcmp(op, "mulflat") == 0 || strcmp(op, "timesflat") == 0 ||
               strcmp(op, "multflat") == 0 || strcmp(op, "factorflat") == 0)
        op = "scaleflat";
      else if (strcmp(op, "anyflat") == 0 || strcmp(op, "hitflat") == 0)
        op = "hasflat";
      else if (strcmp(op, "nflat") == 0 || strcmp(op, "leafcount") == 0 ||
               strcmp(op, "ncflat") == 0)
        op = "countflat";
      else if (strcmp(op, "anyflatn") == 0 || strcmp(op, "hitflatn") == 0 ||
               strcmp(op, "anynumflat") == 0)
        op = "hasflatn";
      else if (strcmp(op, "nflatn") == 0 || strcmp(op, "leafcountn") == 0 ||
               strcmp(op, "ncflatn") == 0 || strcmp(op, "numcountflat") == 0)
        op = "countflatn";
      else if (strcmp(op, "matchpaths") == 0 || strcmp(op, "pathmatch") == 0)
        op = "pathsflat";
      else if (strcmp(op, "matchvals") == 0 || strcmp(op, "leafvals") == 0 ||
               strcmp(op, "valmatch") == 0)
        op = "valsflat";
      else if (strcmp(op, "matchpathsn") == 0 || strcmp(op, "pathmatchn") == 0 ||
               strcmp(op, "numpathsflat") == 0)
        op = "pathsflatn";
      else if (strcmp(op, "matchvalsn") == 0 || strcmp(op, "leafvalsn") == 0 ||
               strcmp(op, "numvalsflat") == 0 || strcmp(op, "intvalsflat") == 0)
        op = "valsflatn";
      else if (strcmp(op, "firstflat") == 0 || strcmp(op, "peelflat") == 0 ||
               strcmp(op, "leafget") == 0 || strcmp(op, "getleaf") == 0)
        op = "getflat";
      else if (strcmp(op, "endflat") == 0 || strcmp(op, "tailflat") == 0 ||
               strcmp(op, "lastleafp") == 0 || strcmp(op, "getleafl") == 0)
        op = "lastflat";
      else if (strcmp(op, "indexflat") == 0 || strcmp(op, "atflat") == 0 ||
               strcmp(op, "nthleaf") == 0 || strcmp(op, "leafnth") == 0)
        op = "nthflat";
      else if (strcmp(op, "requireflat") == 0 || strcmp(op, "mustflat") == 0 ||
               strcmp(op, "needleaves") == 0)
        op = "needflat";
      else if (strcmp(op, "firstflatn") == 0 || strcmp(op, "numflat") == 0 ||
               strcmp(op, "getnflat") == 0 || strcmp(op, "ngetflat") == 0)
        op = "getflatn";
      else if (strcmp(op, "endflatn") == 0 || strcmp(op, "tailflatn") == 0 ||
               strcmp(op, "lastnumflat") == 0 || strcmp(op, "getnflatl") == 0)
        op = "lastflatn";
      else if (strcmp(op, "indexflatn") == 0 || strcmp(op, "atflatn") == 0 ||
               strcmp(op, "nthnumflat") == 0 || strcmp(op, "leafnthn") == 0)
        op = "nthflatn";
      else if (strcmp(op, "kindflat") == 0 || strcmp(op, "leaftype") == 0 ||
               strcmp(op, "flattype") == 0 || strcmp(op, "typeleafp") == 0)
        op = "typeflat";
      else if (strcmp(op, "histflat") == 0 || strcmp(op, "leaffreq") == 0 ||
               strcmp(op, "countvalflat") == 0 || strcmp(op, "valfreqflat") == 0 ||
               strcmp(op, "tallyflat") == 0)
        op = "freqflat";
      else if (strcmp(op, "topvalflat") == 0 || strcmp(op, "majorflat") == 0 ||
               strcmp(op, "leafmode") == 0 || strcmp(op, "modalflat") == 0 ||
               strcmp(op, "dominantflat") == 0 || strcmp(op, "winvalflat") == 0)
        op = "modeflat";
      else if (strcmp(op, "valpath") == 0 || strcmp(op, "findval") == 0 ||
               strcmp(op, "pathofval") == 0 || strcmp(op, "findpathval") == 0 ||
               strcmp(op, "whereval") == 0 || strcmp(op, "locateval") == 0)
        op = "pathbyval";
      else if (strcmp(op, "distinctflat") == 0 || strcmp(op, "uniqueflat") == 0 ||
               strcmp(op, "leafuniq") == 0 || strcmp(op, "dedupflat") == 0 ||
               strcmp(op, "uniqvalsflat") == 0 || strcmp(op, "uniquevals") == 0)
        op = "uniqflat";
      else if (strcmp(op, "allpathsbyval") == 0 || strcmp(op, "findvals") == 0 ||
               strcmp(op, "pathsofval") == 0 || strcmp(op, "findpathsval") == 0 ||
               strcmp(op, "wherevals") == 0 || strcmp(op, "locatevals") == 0)
        op = "pathsbyval";
      else if (strcmp(op, "nbyval") == 0 || strcmp(op, "valcount") == 0 ||
               strcmp(op, "nval") == 0 || strcmp(op, "countval") == 0)
        op = "countbyval";
      else if (strcmp(op, "hasvalflat") == 0 || strcmp(op, "anyval") == 0 ||
               strcmp(op, "valhas") == 0 || strcmp(op, "containsval") == 0)
        op = "hasval";
      else if (strcmp(op, "replaceval") == 0 || strcmp(op, "rewval") == 0 ||
               strcmp(op, "mapval") == 0 || strcmp(op, "rewriteval") == 0 ||
               strcmp(op, "reval") == 0 || strcmp(op, "swapval") == 0)
        op = "setbyval";
      else if (strcmp(op, "dropval") == 0 || strcmp(op, "rmval") == 0 ||
               strcmp(op, "pruneval") == 0 || strcmp(op, "deleteval") == 0 ||
               strcmp(op, "removeval") == 0 || strcmp(op, "stripval") == 0)
        op = "delbyval";
      else if (strcmp(op, "samevalflat") == 0 || strcmp(op, "eqvalsflat") == 0 ||
               strcmp(op, "allsameflat") == 0 || strcmp(op, "sameflat") == 0 ||
               strcmp(op, "eqallflat") == 0 || strcmp(op, "uniformflat") == 0)
        op = "alleqflat";
      else if (strcmp(op, "firstuneqflat") == 0 || strcmp(op, "pathuneq") == 0 ||
               strcmp(op, "diverge1") == 0 || strcmp(op, "firstdiverge") == 0 ||
               strcmp(op, "uneq1") == 0 || strcmp(op, "firstdiffval") == 0 ||
               strcmp(op, "pathdiffval") == 0 || strcmp(op, "finduneq") == 0)
        op = "firstuneq";
      else if (strcmp(op, "divergepaths") == 0 || strcmp(op, "alluneq") == 0 ||
               strcmp(op, "pathsuneq") == 0 || strcmp(op, "divergeall") == 0 ||
               strcmp(op, "alldiverge") == 0 || strcmp(op, "uneqall") == 0 ||
               strcmp(op, "uneqpath") == 0 || strcmp(op, "divergepath") == 0)
        op = "uneqpaths";
      else if (strcmp(op, "uniformflat") == 0 || strcmp(op, "checkflat") == 0 ||
               strcmp(op, "samecheck") == 0 || strcmp(op, "checkuniform") == 0 ||
               strcmp(op, "nestuniform") == 0 || strcmp(op, "flatuniform") == 0 ||
               strcmp(op, "uniformcheck") == 0 || strcmp(op, "eqcheck") == 0)
        op = "uniform";
      else if (strcmp(op, "prettyp") == 0 || strcmp(op, "jsonpretty") == 0 ||
               strcmp(op, "prettyjson") == 0 || strcmp(op, "prettyplate") == 0 ||
               strcmp(op, "fmtjson") == 0 || strcmp(op, "indentjson") == 0 ||
               strcmp(op, "pp") == 0)
        op = "pretty";
      else if (strcmp(op, "requireflatn") == 0 || strcmp(op, "mustflatn") == 0 ||
               strcmp(op, "needintflat") == 0 || strcmp(op, "neednumflat") == 0)
        op = "needflatn";
      else if (strcmp(op, "length") == 0 || strcmp(op, "nkeys") == 0 ||
               strcmp(op, "size") == 0 || strcmp(op, "countkeys") == 0)
        op = "len";
      else if (strcmp(op, "isempty") == 0 || strcmp(op, "emptyp") == 0)
        op = "empty";
      else if (strcmp(op, "values") == 0 || strcmp(op, "valsp") == 0)
        op = "vals";
      else if (strcmp(op, "expand") == 0 || strcmp(op, "template") == 0 ||
               strcmp(op, "render") == 0)
        op = "fill";
      else if (strcmp(op, "fill-keys") == 0 || strcmp(op, "tmplkeys") == 0)
        op = "fillkeys";
      else if (strcmp(op, "seed") == 0 || strcmp(op, "touch") == 0 ||
               strcmp(op, "create") == 0)
        op = "ensure";
      else if (strcmp(op, "patch") == 0 || strcmp(op, "overlay") == 0 ||
               strcmp(op, "apply") == 0)
        op = "merge";
      else if (strcmp(op, "same") == 0 || strcmp(op, "equal") == 0 ||
               strcmp(op, "cmp") == 0)
        op = "eq";
      else if (strcmp(op, "neq") == 0 || strcmp(op, "differ") == 0 ||
               strcmp(op, "unequal") == 0)
        op = "ne";
      else if (strcmp(op, "changed") == 0 || strcmp(op, "delta") == 0)
        op = "diff";
      else if (strcmp(op, "clog") == 0 || strcmp(op, "changes") == 0 ||
               strcmp(op, "logdiff") == 0)
        op = "changelog";
      else if (strcmp(op, "hasall") == 0 || strcmp(op, "has-keys") == 0 ||
               strcmp(op, "check") == 0)
        op = "has";
      else if (strcmp(op, "require") == 0 || strcmp(op, "needkeys") == 0 ||
               strcmp(op, "require-keys") == 0)
        op = "need";
      else if (strcmp(op, "getall") == 0 || strcmp(op, "getpall") == 0 ||
               strcmp(op, "peel") == 0 || strcmp(op, "values") == 0 ||
               strcmp(op, "pluckp") == 0)
        op = "pluck";
      else if (strcmp(op, "nestpluck") == 0 || strcmp(op, "objpluck") == 0 ||
               strcmp(op, "plucknest") == 0)
        op = "pluckobj";
      else if (strcmp(op, "keep") == 0 || strcmp(op, "select") == 0 ||
               strcmp(op, "project") == 0)
        op = "pick";
      else if (strcmp(op, "strip") == 0 || strcmp(op, "dropkeys") == 0 ||
               strcmp(op, "drop-keys") == 0)
        op = "omit";
      else if (strcmp(op, "total") == 0)
        op = "sum";
      else if (strcmp(op, "mean") == 0 || strcmp(op, "average") == 0)
        op = "avg";
      else if (strcmp(op, "p50") == 0 || strcmp(op, "mid") == 0)
        op = "median";
      else if (strcmp(op, "topn") == 0 || strcmp(op, "head") == 0)
        op = "top";
      else if (strcmp(op, "bottom") == 0 || strcmp(op, "botn") == 0 ||
               strcmp(op, "tail") == 0)
        op = "bot";
      else if (strcmp(op, "nget") == 0 || strcmp(op, "getnest") == 0 ||
               strcmp(op, "getpobj") == 0)
        op = "nestget";
      else if (strcmp(op, "nset") == 0 || strcmp(op, "setnest") == 0 ||
               strcmp(op, "setpobj") == 0)
        op = "nestset";
      else if (strcmp(op, "ninc") == 0 || strcmp(op, "incnest") == 0 ||
               strcmp(op, "incobj") == 0)
        op = "nestinc";
      else if (strcmp(op, "ndel") == 0 || strcmp(op, "delnest") == 0 ||
               strcmp(op, "delpobj") == 0)
        op = "nestdel";
      else if (strcmp(op, "nkeys") == 0 || strcmp(op, "keysobj") == 0 ||
               strcmp(op, "keysnest") == 0)
        op = "nestkeys";
      else if (strcmp(op, "nhas") == 0 || strcmp(op, "haspobj") == 0 ||
               strcmp(op, "hasnest") == 0)
        op = "nesthas";
      else if (strcmp(op, "npick") == 0 || strcmp(op, "pickobj") == 0 ||
               strcmp(op, "picknest") == 0)
        op = "nestpick";
      else if (strcmp(op, "nomit") == 0 || strcmp(op, "omitobj") == 0 ||
               strcmp(op, "omitnest") == 0 || strcmp(op, "stripnest") == 0)
        op = "nestomit";
      else if (strcmp(op, "nrename") == 0 || strcmp(op, "renamenest") == 0 ||
               strcmp(op, "renamepobj") == 0)
        op = "nestrename";
      else if (strcmp(op, "ncopy") == 0 || strcmp(op, "copynest") == 0 ||
               strcmp(op, "copypobj") == 0)
        op = "nestcopy";
      else if (strcmp(op, "nswap") == 0 || strcmp(op, "swapnest") == 0 ||
               strcmp(op, "swappobj") == 0)
        op = "nestswap";
      else if (strcmp(op, "nsum") == 0 || strcmp(op, "sumnest") == 0 ||
               strcmp(op, "sumnobj") == 0)
        op = "nestsum";
      else if (strcmp(op, "navg") == 0 || strcmp(op, "avgnest") == 0 ||
               strcmp(op, "nestmean") == 0)
        op = "nestavg";
      else if (strcmp(op, "nmedian") == 0 || strcmp(op, "nestp50") == 0 ||
               strcmp(op, "mediannest") == 0)
        op = "nestmedian";
      else if (strcmp(op, "ntop") == 0 || strcmp(op, "topnest") == 0 ||
               strcmp(op, "topnobj") == 0)
        op = "nesttop";
      else if (strcmp(op, "nbot") == 0 || strcmp(op, "botnest") == 0 ||
               strcmp(op, "botnobj") == 0 || strcmp(op, "nestbottom") == 0)
        op = "nestbot";
      else if (strcmp(op, "nsort") == 0 || strcmp(op, "sortnest") == 0 ||
               strcmp(op, "sortobj") == 0)
        op = "nestsort";
      else if (strcmp(op, "nsortbag") == 0 || strcmp(op, "sortbagnest") == 0 ||
               strcmp(op, "sortbagobj") == 0 || strcmp(op, "nestsortfreq") == 0 ||
               strcmp(op, "sortfreqobj") == 0)
        op = "nestsortbag";
      ai = 3;
      /* fill[-keys]: optional -s|--strict before path */
      if ((strcmp(op, "fill") == 0 || strcmp(op, "fillkeys") == 0) &&
          ai < argc && argv[ai] &&
          (!strcmp(argv[ai], "--strict") || !strcmp(argv[ai], "-s"))) {
        val_kind = 99;
        ai++;
      }
      if (ai >= argc) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"err\":\"need path\",\"version\":\"%s\"}\n",
               op, CUBALC_LANG_VERSION);
        return 2;
      }
      path = argv[ai++];
    } else {
      /* bare path → show */
      op = "show";
      path = argv[2];
      ai = 3;
    }

    if (!path || !path[0]) {
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
             "\"err\":\"empty path\",\"version\":\"%s\"}\n", CUBALC_LANG_VERSION);
      return 2;
    }

    /* load soft object plate */
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_read(path, &hr) == 0 && hr.str[0]) {
      body = hr.str;
      while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
        body++;
      if (*body == '{') {
        memset(&keys, 0, sizeof keys);
        if (cubalc_host_json_keys(body, &keys) == 0) {
          snprintf(plate, sizeof plate, "%s", body);
          file_hit = 1;
        }
      }
    }
    if (!file_hit)
      snprintf(plate, sizeof plate, "%s", "{}");

    if (strcmp(op, "show") == 0) {
      memset(&keys, 0, sizeof keys);
      cubalc_host_json_keys(plate, &keys);
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"show\",\"path\":\"%s\",\"file\":%s,\"keys_n\":%ld,"
             "\"version\":\"%s\",\"plate\":%s}\n",
             path, file_hit ? "true" : "false", keys.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* pretty — PRETTYP dual: 2-space indented plate text (read-only, no jq).
     *   cubalc plate pretty agent.json
     * prints pretty body then JSON meta with n=bytes. */
    if (strcmp(op, "pretty") == 0) {
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_pretty(plate, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pretty\",\"path\":\"%s\",\"err\":\"pretty fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.str[0])
        printf("%s\n", pr.str);
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else if (c == '\r') { esc[o++] = '\\'; esc[o++] = 'r'; }
        else if (c == '\t') { esc[o++] = '\\'; esc[o++] = 't'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"pretty\",\"path\":\"%s\",\"file\":%s,\"n\":%ld,"
             "\"truncated\":%s,\"pretty\":\"%s\",\"version\":\"%s\","
             "\"note\":\"PRETTYP dual · 2-space indented plate (no jq)\"}\n",
             path, file_hit ? "true" : "false", pr.n,
             pr.code ? "false" : "true", esc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* leaves|pathkeys [nest.path] — PATHKEYS dual: dotted leaf path bag.
     *   cubalc plate leaves agent.json
     *   cubalc plate leaves agent.json cfg
     *   cubalc plate pathkeys agent.json cfg.meta */
    if (strcmp(op, "leaves") == 0) {
      const char *nestp = NULL;
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t i, o = 0;

      if (ai < argc && argv[ai] && argv[ai][0])
        nestp = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_paths(plate, nestp, &pr) != 0) {
        pr.str[0] = 0;
        pr.n = 0;
      }
      flat[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof flat; i++) {
        char ch = pr.str[i];
        if (ch == '\n' || ch == '\r') {
          if (o > 0 && flat[o - 1] != ',')
            flat[o++] = ',';
        } else if (ch == '"' || ch == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = ch;
        }
      }
      flat[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"leaves\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"paths\":\"%s\",\"version\":\"%s\","
             "\"note\":\"PATHKEYS dual · dotted leaf paths · bag lines above plate\"}\n",
             path, nestp ? nestp : "", file_hit ? "true" : "false",
             pr.n, flat, CUBALC_LANG_VERSION);
      return 0;
    }

    /* flat|flatkv [nest.path] — FLATKV dual: recursive path:value bag.
     *   cubalc plate flat agent.json
     *   cubalc plate flat agent.json cfg
     *   cubalc plate flatkv agent.json cfg.meta */
    if (strcmp(op, "flat") == 0) {
      const char *nestp = NULL;
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t i, o = 0;

      if (ai < argc && argv[ai] && argv[ai][0])
        nestp = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_kv(plate, nestp, &pr) != 0) {
        pr.str[0] = 0;
        pr.n = 0;
      }
      flat[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof flat; i++) {
        char ch = pr.str[i];
        if (ch == '\n' || ch == '\r') {
          if (o > 0 && flat[o - 1] != ',')
            flat[o++] = ',';
        } else if (ch == '"' || ch == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = ch;
        }
      }
      flat[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"flat\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"FLATKV dual · path:value bag · lines above plate\"}\n",
             path, nestp ? nestp : "", file_hit ? "true" : "false",
             pr.n, flat, CUBALC_LANG_VERSION);
      return 0;
    }

    /* unflat|unflatkv [UNDER nest] bag|@file|k:v… — UNFLATKV dual: path:value → nest plate.
     *   cubalc plate unflat agent.json host:cubeA cfg.port:9
     *   cubalc plate unflat agent.json @flat.txt
     *   cubalc plate unflat agent.json UNDER cfg meta.role:worker port:8080
     * Creates plate file if missing. Empty val → {}. Writes plate back to path. */
    if (strcmp(op, "unflat") == 0) {
      const char *under = NULL;
      char bag[CUBALC_HOST_STR_MAX];
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr, wr, tr;
      size_t o = 0, i;
      long nlines = 0;

      bag[0] = 0;
      /* optional UNDER nest */
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "UNDER") == 0 || strcmp(argv[ai], "under") == 0 ||
           strcmp(argv[ai], "AT") == 0 || strcmp(argv[ai], "at") == 0 ||
           strcmp(argv[ai], "IN") == 0 || strcmp(argv[ai], "in") == 0)) {
        ai++;
        if (ai < argc && argv[ai] && argv[ai][0])
          under = argv[ai++];
      }

      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"unflat\",\"path\":\"%s\","
               "\"err\":\"need bag lines, @file, or path:val args\",\"version\":\"%s\"}\n",
               path, CUBALC_LANG_VERSION);
        return 2;
      }

      /* single @file or readable file → bag body; else join remaining as lines */
      if (argv[ai][0] == '@' ||
          (ai + 1 == argc && argv[ai][0] != '{' &&
           strchr(argv[ai], ':') == NULL && strchr(argv[ai], '=') == NULL &&
           access(argv[ai], R_OK) == 0)) {
        const char *fp = (argv[ai][0] == '@') ? argv[ai] + 1 : argv[ai];
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(fp, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"unflat\",\"err\":\"bag file read fail\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(bag, sizeof bag, "%s", tr.str);
        ai++;
      } else {
        while (ai < argc && argv[ai] && argv[ai][0]) {
          size_t kl = strlen(argv[ai]);
          if (o + kl + 2 >= sizeof bag) break;
          if (o > 0) bag[o++] = '\n';
          memcpy(bag + o, argv[ai], kl);
          o += kl;
          bag[o] = 0;
          ai++;
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_unflat_kv(plate, bag, under, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"unflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "unflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      nlines = pr.n;
      snprintf(plate, sizeof plate, "%s", pr.str);

      /* write-back file when path looks like file */
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"unflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }

      flat[0] = 0;
      o = 0;
      for (i = 0; bag[i] && o + 2 < sizeof flat; i++) {
        char ch = bag[i];
        if (ch == '\n' || ch == '\r') {
          if (o > 0 && flat[o - 1] != ',')
            flat[o++] = ',';
        } else if (ch == '"' || ch == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = ch;
        }
      }
      flat[o] = 0;

      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"unflat\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"UNFLATKV dual · path:value → nested plate\"}\n",
             path, under ? under : "", file_hit ? "true" : "false",
             nlines, flat, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* grepf|grepvf|grepfi [needle] — GREPFLAT duals: filter path:value bag by path needle.
     *   cubalc plate grepf  agent.json cfg
     *   cubalc plate grepvf agent.json tmp
     *   cubalc plate grepfi agent.json CFG.META
     * Bag lines above plate. Empty needle → all leaves. */
    if (strcmp(op, "grepf") == 0 || strcmp(op, "grepvf") == 0 ||
        strcmp(op, "grepfi") == 0) {
      const char *needle = "";
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t i, o = 0;
      int invert = (strcmp(op, "grepvf") == 0) ? 1 : 0;
      int icase = (strcmp(op, "grepfi") == 0) ? 1 : 0;

      if (ai < argc && argv[ai] && argv[ai][0])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      cubalc_host_json_leaf_grep(plate, needle, invert, icase, &pr);
      flat[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof flat; i++) {
        char ch = pr.str[i];
        if (ch == '\n' || ch == '\r') {
          if (o > 0 && flat[o - 1] != ',')
            flat[o++] = ',';
        } else if (ch == '"' || ch == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = ch;
        }
      }
      flat[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"invert\":%s,\"icase\":%s,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"GREPFLAT dual · path needle filter · bag lines above plate\"}\n",
             op, path, needle, file_hit ? "true" : "false",
             pr.n, invert ? "true" : "false", icase ? "true" : "false",
             flat, CUBALC_LANG_VERSION);
      return 0;
    }

    /* prune|keeponly [needle] — PRUNEFLAT / KEEONLYFLAT duals (write-back).
     *   cubalc plate prune    agent.json tmp
     *   cubalc plate keeponly agent.json cfg
     * Rewrites plate file. n = deleted (prune) or kept (keeponly). */
    if (strcmp(op, "prune") == 0 || strcmp(op, "keeponly") == 0) {
      const char *needle = "";
      cubalc_host_result pr, wr;
      int keep_only = (strcmp(op, "keeponly") == 0) ? 1 : 0;

      if (ai < argc && argv[ai] && argv[ai][0])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_filter_write(plate, needle, keep_only, 0, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "filter write fail",
               CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\",\"file\":%s,"
             "\"n\":%ld,\"keep_only\":%s,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"%s\"}\n",
             op, path, needle, file_hit ? "true" : "false",
             pr.n, keep_only ? "true" : "false", CUBALC_LANG_VERSION, plate,
             keep_only
                 ? "KEEPONLYFLAT dual · project matching leaves"
                 : "PRUNEFLAT dual · drop matching leaves");
      return 0;
    }

    /* mergeflat <overlay.json> — MERGEFLAT dual: deep leaf overlay onto path plate.
     *   cubalc plate mergeflat base.json overlay.json
     * Writes base. n = overlay leaf count. Nest-aware (unlike plate merge top-level). */
    if (strcmp(op, "mergeflat") == 0) {
      const char *path2 = NULL;
      char plate2[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr, wr, tr;
      int file_hit2 = 0;
      const char *body2;

      plate2[0] = 0;
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"mergeflat\",\"path\":\"%s\",\"err\":\"need overlay path\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      path2 = argv[ai++];
      memset(&tr, 0, sizeof tr);
      if (cubalc_host_read(path2, &tr) == 0 && tr.str[0]) {
        body2 = tr.str;
        while (*body2 == ' ' || *body2 == '\t' || *body2 == '\n' || *body2 == '\r')
          body2++;
        if (*body2 == '{') {
          snprintf(plate2, sizeof plate2, "%s", body2);
          file_hit2 = 1;
        }
      }
      if (!file_hit2)
        snprintf(plate2, sizeof plate2, "%s", "{}");

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_merge(plate, plate2, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"mergeflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "mergeflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"mergeflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"mergeflat\",\"path\":\"%s\",\"path2\":\"%s\",\"file\":%s,"
             "\"file2\":%s,\"n\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"MERGEFLAT dual · deep leaf overlay · nest-aware\"}\n",
             path, path2, file_hit ? "true" : "false",
             file_hit2 ? "true" : "false", pr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* renameflat <old_pfx> <new_pfx> — RENAMEFLAT dual: rewrite leaf path prefixes.
     *   cubalc plate renameflat agent.json cfg. config.
     * Writes plate. n = renamed leaf count. */
    if (strcmp(op, "renameflat") == 0) {
      const char *oldp = NULL, *newp = NULL;
      cubalc_host_result pr, wr;

      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"renameflat\",\"path\":\"%s\",\"err\":\"need old_pfx new_pfx\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      oldp = argv[ai++];
      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"renameflat\",\"path\":\"%s\",\"err\":\"need new_pfx\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      newp = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_rename_pfx(plate, oldp, newp, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"renameflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "renameflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"renameflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"renameflat\",\"path\":\"%s\",\"old\":\"%s\",\"new\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"RENAMEFLAT dual · leaf path prefix rewrite\"}\n",
             path, oldp, newp, file_hit ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* setflat <needle> <value> — SETFLAT dual: bulk set leaf values by path needle.
     *   cubalc plate setflat agent.json debug 0
     *   cubalc plate setflat agent.json cfg.port 9090
     * Writes plate. n = updated leaf count. Empty needle sets all leaves. */
    if (strcmp(op, "setflat") == 0) {
      const char *needle = "", *val = NULL;
      cubalc_host_result pr, wr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setflat\",\"path\":\"%s\",\"err\":\"need needle value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      /* allow empty needle as "" */
      needle = argv[ai++];
      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setflat\",\"path\":\"%s\",\"err\":\"need value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      val = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_set(plate, needle, val, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "setflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"setflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"setflat\",\"path\":\"%s\",\"needle\":\"%s\",\"value\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"SETFLAT dual · bulk leaf value set by path needle\"}\n",
             path, needle, val, file_hit ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* incflat|decflat <needle> [delta] — INCFLAT dual: bump pure-int leaves by path needle.
     *   cubalc plate incflat agent.json hits
     *   cubalc plate incflat agent.json cfg.meta.x 5
     *   cubalc plate decflat agent.json errs
     * Writes plate. n = bumped count. Non-numeric soft-skipped. */
    if (strcmp(op, "incflat") == 0 || strcmp(op, "decflat") == 0) {
      const char *needle = "";
      long delta = 1;
      cubalc_host_result pr, wr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need needle [delta]\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai < argc && argv[ai] && argv[ai][0]) {
        char *end = NULL;
        long d = strtol(argv[ai], &end, 10);
        if (end && end != argv[ai] && *end == 0) {
          delta = d;
          ai++;
        }
      }
      if (strcmp(op, "decflat") == 0) {
        if (delta > 0) delta = -delta;
        else if (delta == 1) delta = -1;
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_inc(plate, needle, delta, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "incflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\",\"delta\":%ld,"
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"INCFLAT dual · bump pure-int leaves by path needle\"}\n",
             op, path, needle, delta, file_hit ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* sumflat <needle?> — SUMFLAT dual: sum pure-int leaves by path needle (read-only).
     *   cubalc plate sumflat agent.json hits
     *   cubalc plate sumflat agent.json          # all pure-int leaves
     * n = sum · count = pure-int leaves used · plate not written. */
    if (strcmp(op, "sumflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_sum(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"sumflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "sumflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"sumflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"count\":%ld,\"sum\":%ld,\"version\":\"%s\","
             "\"note\":\"SUMFLAT dual · sum pure-int leaves by path needle (read-only)\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n, (long)pr.code, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* avgflat [needle] — AVGFLAT dual: integer mean pure-int leaves (read-only).
     *   cubalc plate avgflat agent.json score
     *   cubalc plate avgflat agent.json          # all pure-int leaves
     * n = mean · count = pure-int leaves used · sum side · plate not written. */
    if (strcmp(op, "avgflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      long sum_side = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_avg(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"avgflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "avgflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.err[0]) {
        char *end = NULL;
        long s = strtol(pr.err, &end, 10);
        if (end && end != pr.err && *end == 0)
          sum_side = s;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"avgflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"count\":%ld,\"avg\":%ld,\"sum\":%ld,"
             "\"version\":\"%s\","
             "\"note\":\"AVGFLAT dual · mean pure-int leaves by path needle (read-only)\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n, (long)pr.code, pr.n, sum_side, CUBALC_LANG_VERSION);
      return 0;
    }

    /* medianflat [needle] — MEDIANFLAT dual: integer median pure-int leaves (read-only).
     *   cubalc plate medianflat agent.json score
     *   cubalc plate medianflat agent.json          # all pure-int leaves
     * n = median · count = pure-int leaves used · plate not written. */
    if (strcmp(op, "medianflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_median(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"medianflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "medianflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"medianflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"count\":%ld,\"median\":%ld,"
             "\"version\":\"%s\","
             "\"note\":\"MEDIANFLAT dual · median pure-int leaves by path needle (read-only)\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n, (long)pr.code, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* toppath|botpath [needle] — TOPPATHFLAT/BOTPATHFLAT dual (read-only).
     *   cubalc plate toppath agent.json score
     *   cubalc plate botpath agent.json latency
     * path = winning leaf path · n = value · count = pure-int leaves considered. */
    if (strcmp(op, "toppath") == 0 || strcmp(op, "botpath") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int want_min = (strcmp(op, "botpath") == 0);

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_toppath(plate, needle, want_min, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "toppath fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"leaf\":\"%s\",\"n\":%ld,\"count\":%ld,\"version\":\"%s\","
             "\"note\":\"%s dual · path of %s pure-int leaf by needle (read-only)\"}\n",
             op, path, needle, file_hit ? "true" : "false",
             pr.str, pr.n, (long)pr.code, CUBALC_LANG_VERSION,
             want_min ? "BOTPATHFLAT" : "TOPPATHFLAT",
             want_min ? "min" : "max");
      return 0;
    }

    /* threshflat [needle] min | dropzeroflat [needle]
     * THRESHFLAT/DROPZEROFLAT duals: denoise pure-int leaves by value · write plate.
     *   cubalc plate threshflat agent.json hits 5
     *   cubalc plate threshflat agent.json 10          # empty needle
     *   cubalc plate dropzeroflat agent.json delta
     */
    if (strcmp(op, "threshflat") == 0 || strcmp(op, "dropzeroflat") == 0) {
      const char *needle = "";
      long minv = 0;
      int drop_zero = (strcmp(op, "dropzeroflat") == 0);
      int have_min = 0;
      cubalc_host_result pr, wr;

      if (drop_zero) {
        if (ai < argc && argv[ai])
          needle = argv[ai++];
      } else {
        if (ai >= argc || !argv[ai]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"threshflat\",\"path\":\"%s\",\"err\":\"need [needle] min\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 2;
        }
        /* if only one arg and it's a number → empty needle + min */
        if (ai + 1 >= argc) {
          char *end = NULL;
          long d = strtol(argv[ai], &end, 10);
          if (end && end != argv[ai] && *end == 0) {
            minv = d;
            have_min = 1;
            ai++;
          } else {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"threshflat\",\"path\":\"%s\",\"err\":\"need min int\","
                   "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
            return 2;
          }
        } else {
          needle = argv[ai++];
          {
            char *end = NULL;
            long d = strtol(argv[ai], &end, 10);
            if (!(end && end != argv[ai] && *end == 0)) {
              printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                     "\"op\":\"threshflat\",\"path\":\"%s\",\"err\":\"need min int\","
                     "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
              return 2;
            }
            minv = d;
            have_min = 1;
            ai++;
          }
        }
        (void)have_min;
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_thresh(plate, needle, minv, drop_zero, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "threshflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\",\"min\":%ld,"
             "\"file\":%s,\"n\":%ld,\"drop\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"%s dual · pure-int leaf value denoise by path needle\"}\n",
             op, path, needle, minv, file_hit ? "true" : "false",
             pr.n, (long)pr.code, CUBALC_LANG_VERSION, plate,
             drop_zero ? "DROPZEROFLAT" : "THRESHFLAT");
      return 0;
    }

    /* capflat [needle] max — CAPFLAT dual: clamp pure-int leaves to max · write plate.
     *   cubalc plate capflat agent.json score 100
     *   cubalc plate capflat agent.json 50          # empty needle
     */
    if (strcmp(op, "capflat") == 0) {
      const char *needle = "";
      long maxv = 0;
      cubalc_host_result pr, wr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"need [needle] max\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      if (ai + 1 >= argc) {
        char *end = NULL;
        long d = strtol(argv[ai], &end, 10);
        if (end && end != argv[ai] && *end == 0) {
          maxv = d;
          ai++;
        } else {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"need max int\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 2;
        }
      } else {
        needle = argv[ai++];
        {
          char *end = NULL;
          long d = strtol(argv[ai], &end, 10);
          if (!(end && end != argv[ai] && *end == 0)) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"need max int\","
                   "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
            return 2;
          }
          maxv = d;
          ai++;
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_cap(plate, needle, maxv, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "capflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"capflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"capflat\",\"path\":\"%s\",\"needle\":\"%s\",\"max\":%ld,"
             "\"file\":%s,\"n\":%ld,\"match\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"CAPFLAT dual · clamp pure-int leaves to max by path needle\"}\n",
             path, needle, maxv, file_hit ? "true" : "false",
             pr.n, (long)pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* scaleflat [needle] factor — SCALEFLAT dual: multiply pure-int leaves · write plate.
     *   cubalc plate scaleflat agent.json score 2
     *   cubalc plate scaleflat agent.json 10          # empty needle
     */
    if (strcmp(op, "scaleflat") == 0) {
      const char *needle = "";
      long factor = 1;
      cubalc_host_result pr, wr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"need [needle] factor\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      if (ai + 1 >= argc) {
        char *end = NULL;
        long d = strtol(argv[ai], &end, 10);
        if (end && end != argv[ai] && *end == 0) {
          factor = d;
          ai++;
        } else {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"need factor int\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 2;
        }
      } else {
        needle = argv[ai++];
        {
          char *end = NULL;
          long d = strtol(argv[ai], &end, 10);
          if (!(end && end != argv[ai] && *end == 0)) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"need factor int\","
                   "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
            return 2;
          }
          factor = d;
          ai++;
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_scale(plate, needle, factor, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "scaleflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"scaleflat\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"scaleflat\",\"path\":\"%s\",\"needle\":\"%s\",\"factor\":%ld,"
             "\"file\":%s,\"n\":%ld,\"match\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"SCALEFLAT dual · multiply pure-int leaves by factor\"}\n",
             path, needle, factor, file_hit ? "true" : "false",
             pr.n, (long)pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* hasflat|countflat [needle] — HASFLAT/COUNTFLAT duals (read-only).
     *   cubalc plate hasflat agent.json error
     *   cubalc plate countflat agent.json score
     * hasflat: n = 0|1 hit · count = match count
     * countflat: n = match count
     */
    if (strcmp(op, "hasflat") == 0 || strcmp(op, "countflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int is_count = (strcmp(op, "countflat") == 0);

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_count(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "hasflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      if (is_count) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"countflat\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"COUNTFLAT dual · count leaf paths matching needle (read-only)\"}\n",
               path, needle, file_hit ? "true" : "false",
               pr.n, CUBALC_LANG_VERSION);
      } else {
        long hit = (pr.n > 0) ? 1 : 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"hasflat\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"count\":%ld,\"hit\":%s,\"version\":\"%s\","
               "\"note\":\"HASFLAT dual · soft leaf-path presence by needle (read-only)\"}\n",
               path, needle, file_hit ? "true" : "false",
               hit, pr.n, hit ? "true" : "false", CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* hasflatn|countflatn [needle] — HASFLATN/COUNTFLATN duals (pure-int, read-only).
     *   cubalc plate hasflatn agent.json port
     *   cubalc plate countflatn agent.json score
     * hasflatn: n=0|1 · count=pure-int matches
     * countflatn: n=pure-int match count
     */
    if (strcmp(op, "hasflatn") == 0 || strcmp(op, "countflatn") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int is_count = (strcmp(op, "countflatn") == 0);

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_countn(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "hasflatn fail", CUBALC_LANG_VERSION);
        return 1;
      }
      if (is_count) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"countflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"COUNTFLATN dual · pure-int leaf count by needle (read-only)\"}\n",
               path, needle, file_hit ? "true" : "false",
               pr.n, CUBALC_LANG_VERSION);
      } else {
        long hit = (pr.n > 0) ? 1 : 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"hasflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"count\":%ld,\"hit\":%s,\"version\":\"%s\","
               "\"note\":\"HASFLATN dual · soft pure-int leaf presence by needle (read-only)\"}\n",
               path, needle, file_hit ? "true" : "false",
               hit, pr.n, hit ? "true" : "false", CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* pathsflat|valsflat [needle] — PATHSFLAT/VALSFLAT duals (read-only bags).
     *   cubalc plate pathsflat agent.json score
     *   cubalc plate valsflat agent.json hits
     * bag = newline fields · n = count. */
    if (strcmp(op, "pathsflat") == 0 || strcmp(op, "valsflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int want_vals = (strcmp(op, "valsflat") == 0);
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_match_bag(plate, needle, want_vals, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "pathsflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      /* JSON-escape bag for plate field */
      esc[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') {
          esc[o++] = '\\';
          esc[o++] = c;
        } else if (c == '\n') {
          esc[o++] = '\\';
          esc[o++] = 'n';
        } else if (c == '\r') {
          /* skip */
        } else {
          esc[o++] = c;
        }
      }
      esc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"%s dual · matching leaf %s bag (read-only)\"}\n",
             op, path, needle, file_hit ? "true" : "false",
             pr.n, esc, CUBALC_LANG_VERSION,
             want_vals ? "VALSFLAT" : "PATHSFLAT",
             want_vals ? "values" : "paths");
      return 0;
    }

    /* pathsflatn|valsflatn [needle] — PATHSFLATN/VALSFLATN duals (pure-int bags).
     *   cubalc plate pathsflatn agent.json score
     *   cubalc plate valsflatn agent.json hits
     * bag = newline pure-int path or value fields · n = count. */
    if (strcmp(op, "pathsflatn") == 0 || strcmp(op, "valsflatn") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int want_vals = (strcmp(op, "valsflatn") == 0);
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_match_bagn(plate, needle, want_vals, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, pr.err[0] ? pr.err : "pathsflatn fail", CUBALC_LANG_VERSION);
        return 1;
      }
      esc[0] = 0;
      for (i = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') {
          esc[o++] = '\\';
          esc[o++] = c;
        } else if (c == '\n') {
          esc[o++] = '\\';
          esc[o++] = 'n';
        } else if (c == '\r') {
          /* skip */
        } else {
          esc[o++] = c;
        }
      }
      esc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"%s dual · pure-int leaf %s bag (read-only)\"}\n",
             op, path, needle, file_hit ? "true" : "false",
             pr.n, esc, CUBALC_LANG_VERSION,
             want_vals ? "VALSFLATN" : "PATHSFLATN",
             want_vals ? "values" : "paths");
      return 0;
    }

    /* getflat <needle> [OR|DEFAULT def] — GETFLAT dual (read-only).
     *   cubalc plate getflat agent.json port
     *   cubalc plate getflat agent.json port OR 8080
     * value = first matching leaf · leaf = winning path · hit true/false. */
    if (strcmp(op, "getflat") == 0) {
      const char *needle = "", *fb = NULL;
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2], lesc[512];
      size_t i, o;
      const char *val, *leafp;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getflat\",\"path\":\"%s\",\"err\":\"need needle [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "or") == 0 ||
           strcmp(argv[ai], "DEFAULT") == 0 || strcmp(argv[ai], "default") == 0)) {
        ai++;
        if (ai < argc && argv[ai])
          fb = argv[ai++];
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_get(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getflat\",\"path\":\"%s\",\"err\":\"getflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.n == 0 && fb) {
        val = fb;
        leafp = "";
      } else {
        val = pr.str;
        leafp = pr.err;
      }
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      for (i = 0, o = 0; leafp[i] && o + 2 < sizeof lesc; i++) {
        char c = leafp[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"getflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"hit\":%s,\"fallback\":%s,\"leaf\":\"%s\",\"value\":\"%s\","
             "\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"GETFLAT dual · first matching leaf value by path needle\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n ? "true" : "false",
             (pr.n == 0 && fb) ? "true" : "false",
             lesc, vesc, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* lastflat <needle> [OR|DEFAULT def] — LASTFLAT dual (read-only).
     *   cubalc plate lastflat agent.json port
     *   cubalc plate lastflat agent.json port OR 8080
     * value = last matching leaf · leaf = winning path · hit true/false. */
    if (strcmp(op, "lastflat") == 0) {
      const char *needle = "", *fb = NULL;
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2], lesc[512];
      size_t i, o;
      const char *val, *leafp;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"lastflat\",\"path\":\"%s\",\"err\":\"need needle [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "or") == 0 ||
           strcmp(argv[ai], "DEFAULT") == 0 || strcmp(argv[ai], "default") == 0)) {
        ai++;
        if (ai < argc && argv[ai])
          fb = argv[ai++];
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_get_last(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"lastflat\",\"path\":\"%s\",\"err\":\"lastflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.n == 0 && fb) {
        val = fb;
        leafp = "";
      } else {
        val = pr.str;
        leafp = pr.err;
      }
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      for (i = 0, o = 0; leafp[i] && o + 2 < sizeof lesc; i++) {
        char c = leafp[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"lastflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"hit\":%s,\"fallback\":%s,\"leaf\":\"%s\",\"value\":\"%s\","
             "\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"LASTFLAT dual · last matching leaf value by path needle\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n ? "true" : "false",
             (pr.n == 0 && fb) ? "true" : "false",
             lesc, vesc, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* nthflat <needle> <i> [OR def] — NTHFLAT dual: 0-based Nth matching leaf (read-only).
     *   cubalc plate nthflat agent.json role 0
     *   cubalc plate nthflat agent.json role 1 OR worker
     * value · hit · leaf · i. */
    if (strcmp(op, "nthflat") == 0) {
      const char *needle = "", *fb = NULL;
      long idx = 0;
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2], lesc[512];
      size_t i, o;
      const char *val, *leafp;
      char *end = NULL;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflat\",\"path\":\"%s\",\"err\":\"need needle index [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflat\",\"path\":\"%s\",\"err\":\"need needle index [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      end = NULL;
      idx = strtol(argv[ai], &end, 10);
      if (!(end && end != argv[ai] && *end == 0)) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflat\",\"path\":\"%s\",\"err\":\"bad index\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      ai++;
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "or") == 0 ||
           strcmp(argv[ai], "DEFAULT") == 0 || strcmp(argv[ai], "default") == 0)) {
        ai++;
        if (ai < argc && argv[ai])
          fb = argv[ai++];
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_get_nth(plate, needle, idx, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflat\",\"path\":\"%s\",\"err\":\"nthflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.n == 0 && fb) {
        val = fb;
        leafp = "";
      } else {
        val = pr.str;
        leafp = pr.err;
      }
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      for (i = 0, o = 0; leafp[i] && o + 2 < sizeof lesc; i++) {
        char c = leafp[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"nthflat\",\"path\":\"%s\",\"needle\":\"%s\",\"i\":%ld,"
             "\"file\":%s,\"hit\":%s,\"fallback\":%s,\"leaf\":\"%s\",\"value\":\"%s\","
             "\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"NTHFLAT dual · 0-based Nth matching leaf by path needle\"}\n",
             path, needle, idx, file_hit ? "true" : "false",
             pr.n ? "true" : "false",
             (pr.n == 0 && fb) ? "true" : "false",
             lesc, vesc, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* needflat <needle> [needle…] — NEEDFLAT dual: fail-fast leaf path contract.
     *   cubalc plate needflat agent.json tls agent_id
     * exit 0 if all needles match ≥1 leaf · exit 2 if any miss (miss bag). */
    if (strcmp(op, "needflat") == 0) {
      char miss_bag[CUBALC_HOST_STR_MAX];
      size_t molen = 0;
      int n_need = 0, n_miss = 0;
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"needflat\",\"path\":\"%s\",\"err\":\"need needle [needle…]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      miss_bag[0] = 0;
      while (ai < argc && argv[ai]) {
        const char *nd = argv[ai++];
        size_t mlen;
        n_need++;
        memset(&pr, 0, sizeof pr);
        cubalc_host_json_leaf_count(plate, nd, &pr);
        if (pr.n <= 0) {
          mlen = strlen(nd);
          n_miss++;
          if (molen > 0 && molen + 1 < sizeof miss_bag)
            miss_bag[molen++] = '\n';
          if (molen + mlen < sizeof miss_bag) {
            memcpy(miss_bag + molen, nd, mlen);
            molen += mlen;
            miss_bag[molen] = 0;
          }
        }
      }
      for (i = 0, o = 0; miss_bag[i] && o + 2 < sizeof esc; i++) {
        char c = miss_bag[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      if (n_miss > 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"needflat\",\"path\":\"%s\",\"file\":%s,"
               "\"n\":%d,\"miss_n\":%d,\"miss\":\"%s\",\"version\":\"%s\","
               "\"note\":\"NEEDFLAT dual · missing leaf path needle(s)\"}\n",
               path, file_hit ? "true" : "false",
               n_need, n_miss, esc, CUBALC_LANG_VERSION);
        return 2;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"needflat\",\"path\":\"%s\",\"file\":%s,"
             "\"n\":%d,\"miss_n\":0,\"version\":\"%s\","
             "\"note\":\"NEEDFLAT dual · all leaf path needles present\"}\n",
             path, file_hit ? "true" : "false", n_need, CUBALC_LANG_VERSION);
      return 0;
    }

    /* getflatn <needle> [OR n] — GETFLATN dual: first pure-int leaf by needle (read-only).
     *   cubalc plate getflatn agent.json port
     *   cubalc plate getflatn agent.json port OR 8080
     * n = value · hit · leaf path. */
    if (strcmp(op, "getflatn") == 0) {
      const char *needle = "";
      long fb_n = 0;
      int have_fb = 0;
      cubalc_host_result pr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"err\":\"need needle [OR n]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "or") == 0 ||
           strcmp(argv[ai], "DEFAULT") == 0 || strcmp(argv[ai], "default") == 0)) {
        ai++;
        if (ai < argc && argv[ai]) {
          char *end = NULL;
          long d = strtol(argv[ai], &end, 10);
          if (end && end != argv[ai] && *end == 0) {
            fb_n = d;
            have_fb = 1;
            ai++;
          }
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_getn(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"err\":\"getflatn fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.code == 0 && have_fb) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":false,\"fallback\":true,\"leaf\":\"\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"GETFLATN dual · pure-int leaf miss used OR fallback\"}\n",
               path, needle, file_hit ? "true" : "false", fb_n, CUBALC_LANG_VERSION);
      } else if (pr.code == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":false,\"fallback\":false,\"leaf\":\"\","
               "\"n\":0,\"version\":\"%s\","
               "\"note\":\"GETFLATN dual · no pure-int leaf match\"}\n",
               path, needle, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      } else {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":true,\"fallback\":false,\"leaf\":\"%s\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"GETFLATN dual · first pure-int leaf by path needle\"}\n",
               path, needle, file_hit ? "true" : "false", pr.err, pr.n,
               CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* lastflatn <needle> [OR n] — LASTFLATN dual: last pure-int leaf by needle (read-only).
     *   cubalc plate lastflatn agent.json port
     *   cubalc plate lastflatn agent.json port OR 8080
     * n = value · hit · leaf path (last match). */
    if (strcmp(op, "lastflatn") == 0) {
      const char *needle = "";
      long fb_n = 0;
      int have_fb = 0;
      cubalc_host_result pr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"err\":\"need needle [OR n]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "or") == 0 ||
           strcmp(argv[ai], "DEFAULT") == 0 || strcmp(argv[ai], "default") == 0)) {
        ai++;
        if (ai < argc && argv[ai]) {
          char *end = NULL;
          long d = strtol(argv[ai], &end, 10);
          if (end && end != argv[ai] && *end == 0) {
            fb_n = d;
            have_fb = 1;
            ai++;
          }
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_getn_last(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"err\":\"lastflatn fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.code == 0 && have_fb) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":false,\"fallback\":true,\"leaf\":\"\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"LASTFLATN dual · pure-int leaf miss used OR fallback\"}\n",
               path, needle, file_hit ? "true" : "false", fb_n, CUBALC_LANG_VERSION);
      } else if (pr.code == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":false,\"fallback\":false,\"leaf\":\"\","
               "\"n\":0,\"version\":\"%s\","
               "\"note\":\"LASTFLATN dual · no pure-int leaf match\"}\n",
               path, needle, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      } else {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"lastflatn\",\"path\":\"%s\",\"needle\":\"%s\","
               "\"file\":%s,\"hit\":true,\"fallback\":false,\"leaf\":\"%s\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"LASTFLATN dual · last pure-int leaf by path needle\"}\n",
               path, needle, file_hit ? "true" : "false", pr.err, pr.n,
               CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* nthflatn <needle> <i> [OR n] — NTHFLATN dual: 0-based Nth pure-int leaf (read-only).
     *   cubalc plate nthflatn agent.json score 0
     *   cubalc plate nthflatn agent.json score 1 OR 0
     * n = value · hit · leaf path · i = index. */
    if (strcmp(op, "nthflatn") == 0) {
      const char *needle = "";
      long fb_n = 0, idx = 0;
      int have_fb = 0;
      cubalc_host_result pr;
      char *end = NULL;

      if (ai + 1 > argc || !argv[ai] || !argv[ai + 1]) {
        /* need needle and index: at least 2 args remaining if both present */
      }
      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"err\":\"need needle index [OR n]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"err\":\"need needle index [OR n]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      end = NULL;
      idx = strtol(argv[ai], &end, 10);
      if (!(end && end != argv[ai] && *end == 0)) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"err\":\"bad index\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      ai++;
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "or") == 0 ||
           strcmp(argv[ai], "DEFAULT") == 0 || strcmp(argv[ai], "default") == 0)) {
        ai++;
        if (ai < argc && argv[ai]) {
          end = NULL;
          long d = strtol(argv[ai], &end, 10);
          if (end && end != argv[ai] && *end == 0) {
            fb_n = d;
            have_fb = 1;
            ai++;
          }
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_getn_nth(plate, needle, idx, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"err\":\"nthflatn fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.code == 0 && have_fb) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"needle\":\"%s\",\"i\":%ld,"
               "\"file\":%s,\"hit\":false,\"fallback\":true,\"leaf\":\"\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"NTHFLATN dual · pure-int leaf miss used OR fallback\"}\n",
               path, needle, idx, file_hit ? "true" : "false", fb_n, CUBALC_LANG_VERSION);
      } else if (pr.code == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"needle\":\"%s\",\"i\":%ld,"
               "\"file\":%s,\"hit\":false,\"fallback\":false,\"leaf\":\"\","
               "\"n\":0,\"version\":\"%s\","
               "\"note\":\"NTHFLATN dual · no pure-int leaf at index\"}\n",
               path, needle, idx, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      } else {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nthflatn\",\"path\":\"%s\",\"needle\":\"%s\",\"i\":%ld,"
               "\"file\":%s,\"hit\":true,\"fallback\":false,\"leaf\":\"%s\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"NTHFLATN dual · 0-based Nth pure-int leaf by path needle\"}\n",
               path, needle, idx, file_hit ? "true" : "false", pr.err, pr.n,
               CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* typeflat <needle> — TYPEFLAT dual: first matching leaf kind (read-only).
     *   cubalc plate typeflat agent.json port
     * kind = missing|num|str|bool|null|obj|arr · n = kind code 0..6 · leaf = path. */
    if (strcmp(op, "typeflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char lesc[512];
      size_t i, o = 0;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"typeflat\",\"path\":\"%s\",\"err\":\"need needle\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_type(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"typeflat\",\"path\":\"%s\",\"err\":\"typeflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.err[i] && o + 2 < sizeof lesc; i++) {
        char c = pr.err[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"typeflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"hit\":%s,\"kind\":\"%s\",\"n\":%ld,\"leaf\":\"%s\","
             "\"version\":\"%s\","
             "\"note\":\"TYPEFLAT dual · first matching leaf kind by path needle\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.code ? "true" : "false", pr.str, pr.n, lesc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* freqflat [needle] — FREQFLAT dual: value frequency of matching leaves (read-only).
     *   cubalc plate freqflat agent.json role
     *   cubalc plate freqflat agent.json          # all leaves
     * bag lines: val:count · n=uniques · total=matching leaves · plate not written. */
    if (strcmp(op, "freqflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_freq(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"freqflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "freqflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"freqflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"FREQFLAT dual · value frequency of matching leaves (read-only)\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n, (long)pr.code, esc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* modeflat [needle] — MODEFLAT dual: dominant matching leaf value (read-only).
     *   cubalc plate modeflat agent.json role
     *   cubalc plate modeflat agent.json          # all leaves
     * value = mode · n = winner count · total = matching leaves · plate not written. */
    if (strcmp(op, "modeflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_mode(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"modeflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "modeflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof vesc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      if (pr.str[0])
        printf("%s\n", pr.str);
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"modeflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"hit\":%s,\"value\":\"%s\",\"n\":%ld,\"total\":%ld,"
             "\"version\":\"%s\","
             "\"note\":\"MODEFLAT dual · dominant matching leaf value (read-only)\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.str[0] ? "true" : "false", vesc, pr.n, (long)pr.code,
             CUBALC_LANG_VERSION);
      return 0;
    }

    /* pathbyval <value> [OR def] — PATHBYVAL dual: first leaf path by exact value (read-only).
     *   cubalc plate pathbyval agent.json worker
     *   cubalc plate pathbyval agent.json leader OR cfg.role
     * leaf = winning path · value = matched text · hit/fallback flags. */
    if (strcmp(op, "pathbyval") == 0) {
      const char *needle = "";
      const char *fb = NULL;
      cubalc_host_result pr;
      char lesc[512], vesc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;
      int have_fb = 0;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pathbyval\",\"path\":\"%s\",\"err\":\"need value [OR def]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];
      if (ai < argc && argv[ai] &&
          (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "or") == 0 ||
           strcmp(argv[ai], "DEFAULT") == 0 || strcmp(argv[ai], "default") == 0)) {
        ai++;
        if (ai < argc && argv[ai]) {
          fb = argv[ai++];
          have_fb = 1;
        }
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_path_by_val(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pathbyval\",\"path\":\"%s\",\"err\":\"pathbyval fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      {
        const char *outp = pr.n ? pr.str : (have_fb && fb ? fb : "");
        for (i = 0, o = 0; outp[i] && o + 2 < sizeof lesc; i++) {
          char c = outp[i];
          if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
          else lesc[o++] = c;
        }
        lesc[o] = 0;
        for (i = 0, o = 0; needle[i] && o + 2 < sizeof vesc; i++) {
          char c = needle[i];
          if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
          else vesc[o++] = c;
        }
        vesc[o] = 0;
        if (outp[0])
          printf("%s\n", outp);
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"pathbyval\",\"path\":\"%s\",\"value\":\"%s\","
               "\"file\":%s,\"hit\":%s,\"fallback\":%s,\"leaf\":\"%s\","
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"PATHBYVAL dual · first leaf path by exact value\"}\n",
               path, vesc, file_hit ? "true" : "false",
               pr.n ? "true" : "false",
               (pr.n == 0 && have_fb) ? "true" : "false",
               lesc, pr.n, CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* uniqflat [needle] — UNIQFLAT dual: unique matching leaf values (read-only).
     *   cubalc plate uniqflat agent.json role
     *   cubalc plate uniqflat agent.json          # all leaves
     * bag lines: unique values · n=uniques · total=matching leaves. */
    if (strcmp(op, "uniqflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_uniq(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"uniqflat\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "uniqflat fail", CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"uniqflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"UNIQFLAT dual · unique matching leaf values (read-only)\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n, (long)pr.code, esc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* pathsbyval <value> — PATHSBYVAL dual: all leaf paths by exact value (read-only).
     *   cubalc plate pathsbyval agent.json worker
     * bag lines: matching paths · n=count. */
    if (strcmp(op, "pathsbyval") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2], vesc[512];
      size_t i, o = 0;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pathsbyval\",\"path\":\"%s\",\"err\":\"need value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_paths_by_val(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pathsbyval\",\"path\":\"%s\",\"err\":\"pathsbyval fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof esc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      for (i = 0, o = 0; needle[i] && o + 2 < sizeof vesc; i++) {
        char c = needle[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"pathsbyval\",\"path\":\"%s\",\"value\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"bag\":\"%s\",\"version\":\"%s\","
             "\"note\":\"PATHSBYVAL dual · all leaf paths by exact value (read-only)\"}\n",
             path, vesc, file_hit ? "true" : "false",
             pr.n, esc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* countbyval|hasval <value> — COUNTBYVAL/HASVAL duals (read-only).
     *   cubalc plate countbyval agent.json worker
     *   cubalc plate hasval agent.json leader
     * n = count · hit = presence for hasval. */
    if (strcmp(op, "countbyval") == 0 || strcmp(op, "hasval") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      int want_has = (strcmp(op, "hasval") == 0);

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need value\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_count_by_val(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s fail\","
               "\"version\":\"%s\"}\n", op, path, op, CUBALC_LANG_VERSION);
        return 1;
      }
      if (want_has) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"hasval\",\"path\":\"%s\",\"value\":\"%s\","
               "\"file\":%s,\"hit\":%s,\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"HASVAL dual · soft presence of exact leaf value\"}\n",
               path, needle, file_hit ? "true" : "false",
               pr.code ? "true" : "false", pr.n, CUBALC_LANG_VERSION);
      } else {
        printf("%ld\n", pr.n);
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"countbyval\",\"path\":\"%s\",\"value\":\"%s\","
               "\"file\":%s,\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"COUNTBYVAL dual · count leaves by exact value\"}\n",
               path, needle, file_hit ? "true" : "false",
               pr.n, CUBALC_LANG_VERSION);
      }
      return 0;
    }

    /* setbyval <old> <new> — SETBYVAL dual: rewrite leaf values by exact match.
     *   cubalc plate setbyval agent.json worker standby
     * Writes plate. n = rewritten leaf count. */
    if (strcmp(op, "setbyval") == 0) {
      const char *oldv = "", *newv = NULL;
      cubalc_host_result pr, wr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setbyval\",\"path\":\"%s\",\"err\":\"need old new\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      oldv = argv[ai++];
      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setbyval\",\"path\":\"%s\",\"err\":\"need new value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      newv = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_set_by_val(plate, oldv, newv, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"setbyval\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "setbyval fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"setbyval\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"setbyval\",\"path\":\"%s\",\"old\":\"%s\",\"new\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"SETBYVAL dual · rewrite leaf values by exact match\"}\n",
             path, oldv, newv, file_hit ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* delbyval <value> — DELBYVAL dual: drop leaves by exact value (write-back).
     *   cubalc plate delbyval agent.json standby
     * Writes plate. n = deleted leaf count · kept = remaining. */
    if (strcmp(op, "delbyval") == 0) {
      const char *needle = "";
      cubalc_host_result pr, wr;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"delbyval\",\"path\":\"%s\",\"err\":\"need value\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_del_by_val(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"delbyval\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "delbyval fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", pr.str);
      if (file_hit || (path && path[0] && strchr(path, '.'))) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(path, plate, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"delbyval\",\"path\":\"%s\",\"err\":\"write fail\","
                 "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
          return 1;
        }
        file_hit = 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"delbyval\",\"path\":\"%s\",\"value\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"kept\":%ld,\"version\":\"%s\",\"plate\":%s,"
             "\"note\":\"DELBYVAL dual · drop leaves by exact value\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n, pr.code, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* alleqflat [needle] — ALLEQFLAT dual: all matching leaf values identical (read-only).
     *   cubalc plate alleqflat agent.json role
     *   cubalc plate alleqflat agent.json          # all leaves
     * eq=true|false · value=common when eq · n=match count. */
    if (strcmp(op, "alleqflat") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char vesc[CUBALC_HOST_STR_MAX * 2];
      size_t i, o = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_all_eq(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"alleqflat\",\"path\":\"%s\",\"err\":\"alleqflat fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof vesc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else if (c == '\n') { vesc[o++] = '\\'; vesc[o++] = 'n'; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      if (pr.code && pr.str[0])
        printf("%s\n", pr.str);
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"alleqflat\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"eq\":%s,\"value\":\"%s\",\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"ALLEQFLAT dual · all matching leaf values identical\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.code ? "true" : "false", vesc, pr.n, CUBALC_LANG_VERSION);
      return 0;
    }

    /* firstuneq [needle] — FIRSTUNEQFLAT dual: first path that breaks uniformity (read-only).
     *   cubalc plate firstuneq agent.json role
     *   cubalc plate firstuneq agent.json
     * found=true|false · path · ref · value · i · n=match count. */
    if (strcmp(op, "firstuneq") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char pesc[1024], resc[512], vesc[512];
      char ref[256], val[256];
      long total = 0, found = 0, idx = 0;
      size_t i, o = 0;
      const char *ep;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_first_uneq(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"firstuneq\",\"path\":\"%s\",\"err\":\"firstuneq fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      found = pr.code ? 1 : 0;
      ref[0] = 0;
      val[0] = 0;
      if (found) {
        ep = pr.err;
        {
          size_t k = 0;
          while (*ep && *ep != '\n' && *ep != '\r' && k + 1 < sizeof ref)
            ref[k++] = *ep++;
          ref[k] = 0;
        }
        while (*ep == '\n' || *ep == '\r') ep++;
        {
          size_t k = 0;
          while (*ep && *ep != '\n' && *ep != '\r' && k + 1 < sizeof val)
            val[k++] = *ep++;
          val[k] = 0;
        }
        while (*ep == '\n' || *ep == '\r') ep++;
        total = strtol(ep, NULL, 10);
        idx = pr.n;
        if (pr.str[0])
          printf("%s\n", pr.str);
      } else {
        total = strtol(pr.err, NULL, 10);
        idx = 0;
      }
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof pesc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { pesc[o++] = '\\'; pesc[o++] = c; }
        else if (c == '\n') { pesc[o++] = '\\'; pesc[o++] = 'n'; }
        else pesc[o++] = c;
      }
      pesc[o] = 0;
      for (i = 0, o = 0; ref[i] && o + 2 < sizeof resc; i++) {
        char c = ref[i];
        if (c == '"' || c == '\\') { resc[o++] = '\\'; resc[o++] = c; }
        else resc[o++] = c;
      }
      resc[o] = 0;
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"firstuneq\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"found\":%s,\"leaf\":\"%s\",\"ref\":\"%s\","
             "\"value\":\"%s\",\"i\":%ld,\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"FIRSTUNEQFLAT dual · first path that breaks uniformity\"}\n",
             path, needle, file_hit ? "true" : "false",
             found ? "true" : "false", pesc, resc, vesc, idx, total,
             CUBALC_LANG_VERSION);
      return 0;
    }

    /* uneqpaths [needle] — UNEQPATHS dual: all paths that break uniformity (read-only).
     *   cubalc plate uneqpaths agent.json role
     * bag lines: diverge paths · n=diverge count · total=matches · ref=first value. */
    if (strcmp(op, "uneqpaths") == 0) {
      const char *needle = "";
      cubalc_host_result pr;
      char besc[CUBALC_HOST_STR_MAX * 2], resc[512];
      size_t i, o = 0;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_leaf_uneq_paths(plate, needle, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"uneqpaths\",\"path\":\"%s\",\"err\":\"uneqpaths fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      if (pr.str[0])
        printf("%s\n", pr.str);
      for (i = 0, o = 0; pr.str[i] && o + 2 < sizeof besc; i++) {
        char c = pr.str[i];
        if (c == '"' || c == '\\') { besc[o++] = '\\'; besc[o++] = c; }
        else if (c == '\n') { besc[o++] = '\\'; besc[o++] = 'n'; }
        else besc[o++] = c;
      }
      besc[o] = 0;
      for (i = 0, o = 0; pr.err[i] && o + 2 < sizeof resc; i++) {
        char c = pr.err[i];
        if (c == '"' || c == '\\') { resc[o++] = '\\'; resc[o++] = c; }
        else resc[o++] = c;
      }
      resc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"uneqpaths\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"n\":%ld,\"total\":%ld,\"ref\":\"%s\",\"bag\":\"%s\","
             "\"version\":\"%s\","
             "\"note\":\"UNEQPATHS dual · all paths that break uniformity\"}\n",
             path, needle, file_hit ? "true" : "false",
             pr.n, (long)pr.code, resc, besc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* uniform [needle] — UNIFORMFLAT dual: one-shot eq + first path + all diverge paths.
     *   cubalc plate uniform agent.json role
     * eq · n · div · uniques · path · paths bag · ref · value · common when eq. */
    if (strcmp(op, "uniform") == 0) {
      const char *needle = "";
      cubalc_host_result heq, h1, hpaths;
      char pesc[1024], besc[CUBALC_HOST_STR_MAX * 2], resc[512], vesc[512], cesc[512];
      char ref[256], val[256], path1[512];
      long eq = 0, total = 0, divn = 0, uniques = 0, idx = 0;
      size_t i, o = 0;
      const char *ep;

      if (ai < argc && argv[ai])
        needle = argv[ai++];

      memset(&heq, 0, sizeof heq);
      memset(&h1, 0, sizeof h1);
      memset(&hpaths, 0, sizeof hpaths);
      if (cubalc_host_json_leaf_all_eq(plate, needle, &heq) != 0 ||
          cubalc_host_json_leaf_first_uneq(plate, needle, &h1) != 0 ||
          cubalc_host_json_leaf_uneq_paths(plate, needle, &hpaths) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"uniform\",\"path\":\"%s\",\"err\":\"uniform fail\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 1;
      }
      eq = heq.code ? 1 : 0;
      total = heq.n;
      divn = hpaths.n;
      uniques = (heq.err[0] && heq.err[0] >= '0' && heq.err[0] <= '9')
                    ? strtol(heq.err, NULL, 10)
                    : (eq ? (total > 0 ? 1 : 0) : 0);
      ref[0] = 0;
      val[0] = 0;
      path1[0] = 0;
      if (h1.code) {
        snprintf(path1, sizeof path1, "%s", h1.str);
        idx = h1.n;
        ep = h1.err;
        {
          size_t k = 0;
          while (*ep && *ep != '\n' && *ep != '\r' && k + 1 < sizeof ref)
            ref[k++] = *ep++;
          ref[k] = 0;
        }
        while (*ep == '\n' || *ep == '\r') ep++;
        {
          size_t k = 0;
          while (*ep && *ep != '\n' && *ep != '\r' && k + 1 < sizeof val)
            val[k++] = *ep++;
          val[k] = 0;
        }
      } else if (hpaths.err[0]) {
        snprintf(ref, sizeof ref, "%s", hpaths.err);
      }
      if (eq && heq.str[0])
        printf("%s\n", heq.str);
      else if (path1[0])
        printf("%s\n", path1);
      for (i = 0, o = 0; path1[i] && o + 2 < sizeof pesc; i++) {
        char c = path1[i];
        if (c == '"' || c == '\\') { pesc[o++] = '\\'; pesc[o++] = c; }
        else pesc[o++] = c;
      }
      pesc[o] = 0;
      for (i = 0, o = 0; hpaths.str[i] && o + 2 < sizeof besc; i++) {
        char c = hpaths.str[i];
        if (c == '"' || c == '\\') { besc[o++] = '\\'; besc[o++] = c; }
        else if (c == '\n') { besc[o++] = '\\'; besc[o++] = 'n'; }
        else besc[o++] = c;
      }
      besc[o] = 0;
      for (i = 0, o = 0; ref[i] && o + 2 < sizeof resc; i++) {
        char c = ref[i];
        if (c == '"' || c == '\\') { resc[o++] = '\\'; resc[o++] = c; }
        else resc[o++] = c;
      }
      resc[o] = 0;
      for (i = 0, o = 0; val[i] && o + 2 < sizeof vesc; i++) {
        char c = val[i];
        if (c == '"' || c == '\\') { vesc[o++] = '\\'; vesc[o++] = c; }
        else vesc[o++] = c;
      }
      vesc[o] = 0;
      for (i = 0, o = 0; heq.str[i] && o + 2 < sizeof cesc; i++) {
        char c = heq.str[i];
        if (c == '"' || c == '\\') { cesc[o++] = '\\'; cesc[o++] = c; }
        else cesc[o++] = c;
      }
      cesc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"uniform\",\"path\":\"%s\",\"needle\":\"%s\","
             "\"file\":%s,\"eq\":%s,\"n\":%ld,\"div\":%ld,\"uniques\":%ld,"
             "\"i\":%ld,\"path_first\":\"%s\",\"paths\":\"%s\","
             "\"ref\":\"%s\",\"value\":\"%s\",\"common\":\"%s\","
             "\"version\":\"%s\","
             "\"note\":\"UNIFORMFLAT dual · one-shot nest consistency\"}\n",
             path, needle, file_hit ? "true" : "false",
             eq ? "true" : "false", total, divn, uniques, idx,
             pesc, besc, resc, vesc, cesc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* needflatn <needle> [needle…] — NEEDFLATN dual: fail-fast pure-int leaf contract.
     *   cubalc plate needflatn agent.json port timeout_ms
     * on ok: n = first needle pure-int value · needles = count · leaf = first path
     * exit 2 if any needle has no pure-int leaf match. */
    if (strcmp(op, "needflatn") == 0) {
      char miss_bag[CUBALC_HOST_STR_MAX];
      size_t molen = 0;
      int n_need = 0, n_miss = 0, have_first = 0;
      long first_val = 0;
      char first_path[512];
      cubalc_host_result pr;
      char esc[CUBALC_HOST_STR_MAX * 2], lesc[1024];
      size_t i, o = 0;

      if (ai >= argc || !argv[ai]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"needflatn\",\"path\":\"%s\",\"err\":\"need needle [needle…]\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      miss_bag[0] = 0;
      first_path[0] = 0;
      while (ai < argc && argv[ai]) {
        const char *nd = argv[ai++];
        size_t mlen;
        n_need++;
        memset(&pr, 0, sizeof pr);
        cubalc_host_json_leaf_getn(plate, nd, &pr);
        if (pr.code == 0) {
          mlen = strlen(nd);
          n_miss++;
          if (molen > 0 && molen + 1 < sizeof miss_bag)
            miss_bag[molen++] = '\n';
          if (molen + mlen < sizeof miss_bag) {
            memcpy(miss_bag + molen, nd, mlen);
            molen += mlen;
            miss_bag[molen] = 0;
          }
        } else if (!have_first) {
          first_val = pr.n;
          snprintf(first_path, sizeof first_path, "%s", pr.err);
          have_first = 1;
        }
      }
      for (i = 0, o = 0; miss_bag[i] && o + 2 < sizeof esc; i++) {
        char c = miss_bag[i];
        if (c == '"' || c == '\\') { esc[o++] = '\\'; esc[o++] = c; }
        else if (c == '\n') { esc[o++] = '\\'; esc[o++] = 'n'; }
        else esc[o++] = c;
      }
      esc[o] = 0;
      for (i = 0, o = 0; first_path[i] && o + 2 < sizeof lesc; i++) {
        char c = first_path[i];
        if (c == '"' || c == '\\') { lesc[o++] = '\\'; lesc[o++] = c; }
        else lesc[o++] = c;
      }
      lesc[o] = 0;
      if (n_miss > 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"needflatn\",\"path\":\"%s\",\"file\":%s,"
               "\"needles\":%d,\"miss_n\":%d,\"miss\":\"%s\",\"version\":\"%s\","
               "\"note\":\"NEEDFLATN dual · missing pure-int leaf needle(s)\"}\n",
               path, file_hit ? "true" : "false",
               n_need, n_miss, esc, CUBALC_LANG_VERSION);
        return 2;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"needflatn\",\"path\":\"%s\",\"file\":%s,"
             "\"needles\":%d,\"miss_n\":0,\"n\":%ld,\"leaf\":\"%s\","
             "\"version\":\"%s\","
             "\"note\":\"NEEDFLATN dual · pure-int leaf contract + first peel\"}\n",
             path, file_hit ? "true" : "false", n_need, first_val, lesc,
             CUBALC_LANG_VERSION);
      return 0;
    }

    /* keys|len|empty|vals [nest.path] — KEYSP/LENP/EMPTYP/VALSP duals (path ok).
     *   cubalc plate keys agent.json
     *   cubalc plate keys agent.json cfg.flags
     *   cubalc plate len  agent.json cfg
     *   cubalc plate empty agent.json cfg.flags
     *   cubalc plate vals agent.json cfg */
    if (strcmp(op, "keys") == 0 || strcmp(op, "len") == 0 ||
        strcmp(op, "empty") == 0 || strcmp(op, "vals") == 0) {
      const char *nestp = NULL;
      char sub[CUBALC_HOST_STR_MAX];
      char flat[CUBALC_HOST_STR_MAX];
      cubalc_host_result obj, pr;
      size_t i, o = 0;
      long nkeys = 0;
      int isempty;

      if (ai < argc && argv[ai] && argv[ai][0])
        nestp = argv[ai++];

      memset(&obj, 0, sizeof obj);
      cubalc_host_json_path_obj(plate, nestp, &obj);
      snprintf(sub, sizeof sub, "%s", obj.str);

      if (strcmp(op, "keys") == 0) {
        memset(&keys, 0, sizeof keys);
        if (cubalc_host_json_keys(sub, &keys) != 0) {
          keys.str[0] = 0;
          keys.n = 0;
        }
        flat[0] = 0;
        for (i = 0; keys.str[i] && o + 2 < sizeof flat; i++) {
          char c = keys.str[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',')
              flat[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        if (keys.str[0]) {
          fputs(keys.str, stdout);
          if (keys.str[strlen(keys.str) - 1] != '\n')
            fputc('\n', stdout);
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"keys\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
               "\"n\":%ld,\"keys\":\"%s\",\"version\":\"%s\","
               "\"note\":\"KEYSP dual · nest path ok · bag lines above plate\"}\n",
               path, nestp ? nestp : "", file_hit ? "true" : "false",
               keys.n, flat, CUBALC_LANG_VERSION);
        return 0;
      }

      if (strcmp(op, "vals") == 0) {
        memset(&pr, 0, sizeof pr);
        if (cubalc_host_json_values(sub, &pr) != 0) {
          pr.str[0] = 0;
          pr.n = 0;
        }
        flat[0] = 0;
        for (i = 0; pr.str[i] && o + 2 < sizeof flat; i++) {
          char c = pr.str[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',')
              flat[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        if (pr.str[0]) {
          fputs(pr.str, stdout);
          if (pr.str[strlen(pr.str) - 1] != '\n')
            fputc('\n', stdout);
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"vals\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
               "\"n\":%ld,\"vals\":\"%s\",\"version\":\"%s\","
               "\"note\":\"VALSP dual · nest path ok · bag lines above plate\"}\n",
               path, nestp ? nestp : "", file_hit ? "true" : "false",
               pr.n, flat, CUBALC_LANG_VERSION);
        return 0;
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_len(sub, &pr) != 0)
        nkeys = 0;
      else
        nkeys = pr.n;
      isempty = (nkeys == 0) ? 1 : 0;

      if (strcmp(op, "len") == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"len\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
               "\"n\":%ld,\"version\":\"%s\","
               "\"note\":\"LENP dual · nest path ok\"}\n",
               path, nestp ? nestp : "", file_hit ? "true" : "false",
               nkeys, CUBALC_LANG_VERSION);
        return 0;
      }

      /* empty — always exit 0; agents read empty bool (EMPTYP dual) */
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"empty\",\"path\":\"%s\",\"nest\":\"%s\",\"file\":%s,"
             "\"empty\":%s,\"n\":%ld,\"version\":\"%s\","
             "\"note\":\"EMPTYP dual · nest path ok\"}\n",
             path, nestp ? nestp : "", file_hit ? "true" : "false",
             isempty ? "true" : "false", nkeys, CUBALC_LANG_VERSION);
      return 0;
    }

    /* nestget|nestset|nestinc|nestdel|nestkeys|nesthas — nested field CLI duals of
     * GETPOBJ/SETPOBJ/INCOBJ/DELPOBJ/KEYSOBJ/HASPOBJ. Agents one-shot nested config
     * without writing a .cubalc program.
     * Nest key may be dotted path (cfg.flags) via path_obj peel.
     * nestsum|nestavg|nestmedian|nesttop|nestbot — SUMNOBJ/TOPNOBJ CLI duals (no write).
     * nestsort|nestsortbag — SORTOBJ/SORTBAGOBJ CLI duals (sort write / bag no-write).
     * nestpick|nestomit — PICKOBJ/OMITOBJ duals (paths ok).
     * pluckobj|nestpluck — PLUCKOBJ dual: multi-key peel nest → value bag (paths ok). */
    if (strcmp(op, "nestget") == 0 || strcmp(op, "nestset") == 0 ||
        strcmp(op, "nestinc") == 0 || strcmp(op, "nestdel") == 0 ||
        strcmp(op, "nestkeys") == 0 || strcmp(op, "nesthas") == 0 ||
        strcmp(op, "nestpick") == 0 || strcmp(op, "nestomit") == 0 ||
        strcmp(op, "nestrename") == 0 || strcmp(op, "nestcopy") == 0 ||
        strcmp(op, "nestswap") == 0 ||
        strcmp(op, "pluckobj") == 0 ||
        strcmp(op, "nestsum") == 0 || strcmp(op, "nestavg") == 0 ||
        strcmp(op, "nestmedian") == 0 || strcmp(op, "nesttop") == 0 ||
        strcmp(op, "nestbot") == 0 ||
        strcmp(op, "nestsort") == 0 || strcmp(op, "nestsortbag") == 0) {
      const char *nestk = NULL, *field = NULL;
      char nest[CUBALC_HOST_STR_MAX];
      cubalc_host_result ngr, gr, wr, kr;
      int nest_hit = 0;

      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need nest key\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      nestk = argv[ai++];

      /* path-aware nest peel: "cfg.flags" · soft miss → {} */
      nest[0] = 0;
      memset(&ngr, 0, sizeof ngr);
      cubalc_host_json_path_obj(plate, nestk, &ngr);
      snprintf(nest, sizeof nest, "%s", ngr.str);
      nest_hit = (ngr.n != 0) ? 1 : 0;

      /* nestsum|nestavg|nestmedian — SUMNOBJ/AVGNOBJ/MEDIANOBJ duals (no write). */
      if (strcmp(op, "nestsum") == 0 || strcmp(op, "nestavg") == 0 ||
          strcmp(op, "nestmedian") == 0) {
        cubalc_host_result ar;
        long used = 0;
        memset(&ar, 0, sizeof ar);
        if (!nest_hit) {
          if (strcmp(op, "nestsum") == 0)
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                   "\"op\":\"nestsum\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,"
                   "\"nest_hit\":false,\"file\":%s,\"version\":\"%s\"}\n",
                   path, nestk, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
          else
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                   "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,\"used\":0,"
                   "\"nest_hit\":false,\"file\":%s,\"version\":\"%s\"}\n",
                   op, path, nestk, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
          return 0;
        }
        if (strcmp(op, "nestsum") == 0) {
          if (cubalc_host_json_sum(nest, &ar) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestsum\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, ar.err[0] ? ar.err : "json sum fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestsum\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
                 "\"nest_hit\":true,\"file\":%s,\"version\":\"%s\"}\n",
                 path, nestk, ar.n, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
          return 0;
        }
        if (strcmp(op, "nestavg") == 0) {
          if (cubalc_host_json_avg(nest, &ar) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestavg\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, ar.err[0] ? ar.err : "json avg fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          used = (long)ar.code;
        } else {
          if (cubalc_host_json_median(nest, &ar) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestmedian\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, ar.err[0] ? ar.err : "json median fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          used = (long)ar.code;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,\"used\":%ld,"
               "\"nest_hit\":true,\"file\":%s,\"version\":\"%s\"}\n",
               op, path, nestk, ar.n, used, file_hit ? "true" : "false",
               CUBALC_LANG_VERSION);
        return 0;
      }

      /* nesttop|nestbot [n] — TOPNOBJ/BOTNOBJ duals (read-only; no file write). */
      if (strcmp(op, "nesttop") == 0 || strcmp(op, "nestbot") == 0) {
        cubalc_host_result tr, tk;
        long ntake = 1;
        int want_bot = (strcmp(op, "nestbot") == 0) ? 1 : 0;
        char key1[256];
        long val1 = 0;
        if (ai < argc && argv[ai] && argv[ai][0]) {
          char *end = NULL;
          long vv = strtol(argv[ai], &end, 10);
          if (end && end != argv[ai] && *end == 0)
            ntake = vv;
        }
        if (ntake < 0) ntake = 0;
        if (!nest_hit) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,\"cand\":0,"
                 "\"take\":%ld,\"key\":\"\",\"value\":0,\"nest_hit\":false,"
                 "\"file\":%s,\"version\":\"%s\",\"plate\":{}}\n",
                 op, path, nestk, ntake, file_hit ? "true" : "false",
                 CUBALC_LANG_VERSION);
          return 0;
        }
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_json_topn(nest, ntake, want_bot, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                 "\"version\":\"%s\"}\n",
                 op, path, nestk, tr.err[0] ? tr.err : "json topn fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        key1[0] = 0;
        memset(&tk, 0, sizeof tk);
        if (tr.n > 0 && cubalc_host_json_keys(tr.str, &tk) == 0 && tk.str[0]) {
          size_t kn = 0;
          const char *kp = tk.str;
          while (*kp && *kp != '\n' && *kp != '\r' && kn + 1 < sizeof key1)
            key1[kn++] = *kp++;
          key1[kn] = 0;
          if (key1[0]) {
            cubalc_host_result gr2;
            char *end = NULL;
            memset(&gr2, 0, sizeof gr2);
            if (cubalc_host_json_get(tr.str, key1, &gr2) == 0 && gr2.str[0]) {
              val1 = strtol(gr2.str, &end, 10);
              if (!(end && end != gr2.str && *end == 0))
                val1 = 0;
            }
          }
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,\"cand\":%d,"
               "\"take\":%ld,\"key\":\"%s\",\"value\":%ld,\"nest_hit\":true,"
               "\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
               op, path, nestk, tr.n, tr.code, ntake, key1, val1,
               file_hit ? "true" : "false", CUBALC_LANG_VERSION, tr.str);
        return 0;
      }

      /* nestsort [ASC|DESC] — SORTOBJ dual: sort pure-int nest keys, write-back file.
       * nestsortbag [ASC|DESC] — SORTBAGOBJ dual: key:val bag, no file write.
       * Usability: agent nest FREQ full rank without a .cubalc program. */
      if (strcmp(op, "nestsort") == 0 || strcmp(op, "nestsortbag") == 0) {
        cubalc_host_result sr;
        int want_asc = 0;
        int as_bag = (strcmp(op, "nestsortbag") == 0) ? 1 : 0;
        char bag_esc[CUBALC_HOST_STR_MAX];
        size_t bi, bo;

        while (ai < argc && argv[ai] && argv[ai][0]) {
          if (strcmp(argv[ai], "ASC") == 0 || strcmp(argv[ai], "asc") == 0 ||
              strcmp(argv[ai], "UP") == 0 || strcmp(argv[ai], "up") == 0 ||
              strcmp(argv[ai], "LIGHT") == 0 || strcmp(argv[ai], "light") == 0 ||
              strcmp(argv[ai], "LOW") == 0 || strcmp(argv[ai], "low") == 0) {
            want_asc = 1;
            ai++;
            continue;
          }
          if (strcmp(argv[ai], "DESC") == 0 || strcmp(argv[ai], "desc") == 0 ||
              strcmp(argv[ai], "DOWN") == 0 || strcmp(argv[ai], "down") == 0 ||
              strcmp(argv[ai], "HEAVY") == 0 || strcmp(argv[ai], "heavy") == 0 ||
              strcmp(argv[ai], "HIGH") == 0 || strcmp(argv[ai], "high") == 0) {
            want_asc = 0;
            ai++;
            continue;
          }
          break;
        }

        if (!nest_hit) {
          if (as_bag) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                   "\"op\":\"nestsortbag\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,"
                   "\"asc\":%s,\"nest_hit\":false,\"bag\":\"\","
                   "\"file\":%s,\"version\":\"%s\"}\n",
                   path, nestk, want_asc ? "true" : "false",
                   file_hit ? "true" : "false", CUBALC_LANG_VERSION);
          } else {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                   "\"op\":\"nestsort\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":0,"
                   "\"asc\":%s,\"nest_hit\":false,\"written\":false,"
                   "\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
                   path, nestk, want_asc ? "true" : "false",
                   file_hit ? "true" : "false", CUBALC_LANG_VERSION, plate);
          }
          return 0;
        }

        memset(&sr, 0, sizeof sr);
        if (cubalc_host_json_sortbyval(nest, want_asc, as_bag, &sr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                 "\"version\":\"%s\"}\n",
                 op, path, nestk, sr.err[0] ? sr.err : "json sort fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }

        if (as_bag) {
          bag_esc[0] = 0;
          bo = 0;
          for (bi = 0; sr.str[bi] && bo + 2 < sizeof bag_esc; bi++) {
            char c = sr.str[bi];
            if (c == '"' || c == '\\') {
              bag_esc[bo++] = '\\';
              bag_esc[bo++] = c;
            } else if (c == '\n' || c == '\r') {
              bag_esc[bo++] = '\\';
              bag_esc[bo++] = (c == '\n') ? 'n' : 'r';
            } else if ((unsigned char)c < 32) {
              bag_esc[bo++] = ' ';
            } else {
              bag_esc[bo++] = c;
            }
          }
          bag_esc[bo] = 0;
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestsortbag\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
                 "\"asc\":%s,\"nest_hit\":true,\"bag\":\"%s\","
                 "\"file\":%s,\"version\":\"%s\"}\n",
                 path, nestk, sr.n, want_asc ? "true" : "false", bag_esc,
                 file_hit ? "true" : "false", CUBALC_LANG_VERSION);
          return 0;
        }

        /* write-back sorted nest into outer plate file */
        snprintf(nest, sizeof nest, "%s", sr.str);
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, nestk, nest, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"nestsort\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                 "\"version\":\"%s\"}\n",
                 path, nestk, wr.err[0] ? wr.err : "outer set fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(plate, sizeof plate, "%s", wr.str);
        slash = strrchr(path, '/');
        if (slash && slash != path) {
          size_t n = (size_t)(slash - path);
          if (n >= sizeof parent) n = sizeof parent - 1;
          memcpy(parent, path, n);
          parent[n] = 0;
          memset(&hr, 0, sizeof hr);
          cubalc_host_mkdir(parent, &hr);
        }
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_write(path, plate, &hr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"nestsort\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 path, nestk, CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nestsort\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
               "\"asc\":%s,\"nest_hit\":true,\"written\":true,\"bytes\":%ld,"
               "\"file\":%s,\"version\":\"%s\",\"nest_body\":%s,\"plate\":%s}\n",
               path, nestk, sr.n, want_asc ? "true" : "false", hr.n,
               file_hit ? "true" : "false", CUBALC_LANG_VERSION, nest, plate);
        return 0;
      }

      if (strcmp(op, "nestkeys") == 0) {
        char flatk[CUBALC_HOST_STR_MAX];
        size_t i, o = 0;
        memset(&kr, 0, sizeof kr);
        if (nest_hit)
          cubalc_host_json_keys(nest, &kr);
        flatk[0] = 0;
        for (i = 0; kr.str[i] && o + 2 < sizeof flatk; i++) {
          char c = kr.str[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flatk[o - 1] != ',') flatk[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flatk[o++] = '_';
          } else {
            flatk[o++] = c;
          }
        }
        flatk[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nestkeys\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
               "\"nest_hit\":%s,\"keys\":\"%s\",\"file\":%s,\"version\":\"%s\"}\n",
               path, nestk, nest_hit ? kr.n : 0L,
               nest_hit ? "true" : "false", flatk,
               file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }

      
      /* nestrename|nestcopy|nestswap — RENAMEPOBJ/COPYPOBJ/SWAPPOBJ duals (nest path ok).
       *   cubalc plate nestrename agent.json flags dbg debug
       *   cubalc plate nestcopy   agent.json flags debug mirror
       *   cubalc plate nestswap   agent.json flags debug trace */
      if (strcmp(op, "nestrename") == 0 || strcmp(op, "nestcopy") == 0 ||
          strcmp(op, "nestswap") == 0) {
        const char *a = NULL, *b = NULL;
        cubalc_host_result rr;
        if (ai >= argc || !argv[ai] || !argv[ai][0]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"need src key\",\"version\":\"%s\"}\n",
                 op, path, nestk, CUBALC_LANG_VERSION);
          return 2;
        }
        a = argv[ai++];
        if (ai >= argc || !argv[ai] || !argv[ai][0]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"need dst key\",\"version\":\"%s\"}\n",
                 op, path, nestk, CUBALC_LANG_VERSION);
          return 2;
        }
        b = argv[ai++];
        if (!nest_hit) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"src\":\"%s\","
                 "\"dst\":\"%s\",\"n\":0,\"nest_hit\":false,\"file\":%s,"
                 "\"version\":\"%s\",\"plate\":%s}\n",
                 op, path, nestk, a, b, file_hit ? "true" : "false",
                 CUBALC_LANG_VERSION, plate);
          return 0;
        }
        memset(&rr, 0, sizeof rr);
        if (strcmp(op, "nestrename") == 0) {
          if (cubalc_host_json_rename(nest, a, b, &rr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestrename\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   rr.err[0] ? rr.err : "rename fail", CUBALC_LANG_VERSION);
            return 1;
          }
        } else if (strcmp(op, "nestcopy") == 0) {
          if (cubalc_host_json_copy_key(nest, a, b, &rr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestcopy\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   rr.err[0] ? rr.err : "copy fail", CUBALC_LANG_VERSION);
            return 1;
          }
        } else {
          if (cubalc_host_json_swap_keys(nest, a, b, &rr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestswap\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   rr.err[0] ? rr.err : "swap fail", CUBALC_LANG_VERSION);
            return 1;
          }
        }
        if (rr.n == 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"src\":\"%s\","
                 "\"dst\":\"%s\",\"n\":0,\"nest_hit\":true,\"file\":%s,"
                 "\"version\":\"%s\",\"plate\":%s}\n",
                 op, path, nestk, a, b, file_hit ? "true" : "false",
                 CUBALC_LANG_VERSION, plate);
          return 0;
        }
        snprintf(nest, sizeof nest, "%s", rr.str);
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, nestk, nest, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 op, wr.err[0] ? wr.err : "outer set fail", CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(plate, sizeof plate, "%s", wr.str);
        slash = strrchr(path, '/');
        if (slash && slash != path) {
          size_t n = (size_t)(slash - path);
          if (n >= sizeof parent) n = sizeof parent - 1;
          memcpy(parent, path, n);
          parent[n] = 0;
          memset(&hr, 0, sizeof hr);
          cubalc_host_mkdir(parent, &hr);
        }
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_write(path, plate, &hr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"src\":\"%s\","
               "\"dst\":\"%s\",\"n\":%ld,\"nest_hit\":true,\"file\":%s,"
               "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s,"
               "\"note\":\"RENAMEPOBJ/COPYPOBJ/SWAPPOBJ dual · nest path ok\"}\n",
               op, path, nestk, a, b, rr.n, file_hit ? "true" : "false",
               hr.n, CUBALC_LANG_VERSION, plate);
        return 0;
      }

      /* pluckobj — multi-key peel from nest → value bag (PLUCKOBJ dual; nest path ok).
       * Usability: agent one-shot nested extract without .cubalc:
       *   cubalc plate pluckobj agent.json cfg.meta role x zone
       * Bag lines above plate meta · empty field on miss · hit/miss counts. */
      if (strcmp(op, "pluckobj") == 0) {
        char keys_nl[CUBALC_HOST_STR_MAX];
        char flat_req[CUBALC_HOST_STR_MAX];
        char bag_esc[CUBALC_HOST_STR_MAX];
        cubalc_host_result pr;
        size_t o = 0, bi, bo;
        long nreq = 0;
        long hit = 0, miss = 0;

        keys_nl[0] = 0;
        flat_req[0] = 0;
        if (ai >= argc || !argv[ai] || !argv[ai][0]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"pluckobj\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"need one or more fields\",\"version\":\"%s\"}\n",
                 path, nestk, CUBALC_LANG_VERSION);
          return 2;
        }
        while (ai < argc && argv[ai] && argv[ai][0]) {
          size_t kl = strlen(argv[ai]);
          if (o + kl + 2 >= sizeof keys_nl) break;
          if (o > 0) keys_nl[o++] = '\n';
          memcpy(keys_nl + o, argv[ai], kl);
          o += kl;
          keys_nl[o] = 0;
          {
            size_t fo = strlen(flat_req);
            if (fo + kl + 2 < sizeof flat_req) {
              if (fo > 0) {
                flat_req[fo++] = ',';
                flat_req[fo] = 0;
              }
              memcpy(flat_req + fo, argv[ai], kl);
              flat_req[fo + kl] = 0;
            }
          }
          nreq++;
          ai++;
        }

        memset(&pr, 0, sizeof pr);
        if (nest_hit) {
          if (cubalc_host_json_pluck(nest, keys_nl, &pr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"pluckobj\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, pr.err[0] ? pr.err : "pluck fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          hit = (long)pr.code;
          miss = pr.n - hit;
        } else {
          /* soft nest miss: nreq empty bag fields */
          {
            int i;
            pr.str[0] = 0;
            o = 0;
            for (i = 0; i < nreq; i++) {
              if (i > 0 && o + 1 < sizeof pr.str) pr.str[o++] = '\n';
            }
            pr.str[o] = 0;
            pr.n = nreq;
            pr.code = 0;
          }
          hit = 0;
          miss = nreq;
        }

        if (pr.str[0]) {
          fputs(pr.str, stdout);
          if (pr.str[strlen(pr.str) - 1] != '\n')
            fputc('\n', stdout);
        } else if (nreq > 0) {
          /* all-empty bag still emits blank lines for agent field count */
          {
            int i;
            for (i = 0; i < nreq; i++)
              fputc('\n', stdout);
          }
        }

        bag_esc[0] = 0;
        bo = 0;
        for (bi = 0; pr.str[bi] && bo + 2 < sizeof bag_esc; bi++) {
          char c = pr.str[bi];
          if (c == '"' || c == '\\') {
            bag_esc[bo++] = '\\';
            bag_esc[bo++] = c;
          } else if (c == '\n' || c == '\r') {
            bag_esc[bo++] = '\\';
            bag_esc[bo++] = (c == '\n') ? 'n' : 'r';
          } else if ((unsigned char)c < 32) {
            bag_esc[bo++] = ' ';
          } else {
            bag_esc[bo++] = c;
          }
        }
        bag_esc[bo] = 0;

        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"pluckobj\",\"path\":\"%s\",\"nest\":\"%s\","
               "\"n\":%ld,\"hit\":%ld,\"miss\":%ld,\"keys\":\"%s\",\"bag\":\"%s\","
               "\"nest_hit\":%s,\"file\":%s,\"version\":\"%s\","
               "\"note\":\"bag lines above plate · PLUCKOBJ dual · nest path ok\"}\n",
               path, nestk, nreq, hit, miss, flat_req, bag_esc,
               nest_hit ? "true" : "false",
               file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }

      /* nestpick|nestomit — multi-key keep/drop inside nest (PICKOBJ/OMITOBJ duals). */
      if (strcmp(op, "nestpick") == 0 || strcmp(op, "nestomit") == 0) {
        char keys_nl[CUBALC_HOST_STR_MAX];
        char flat_req[CUBALC_HOST_STR_MAX];
        cubalc_host_result pr;
        size_t o = 0;
        long nreq = 0;
        int is_omit = (strcmp(op, "nestomit") == 0) ? 1 : 0;

        keys_nl[0] = 0;
        flat_req[0] = 0;
        if (ai >= argc || !argv[ai] || !argv[ai][0]) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"err\":\"need one or more fields\",\"version\":\"%s\"}\n",
                 op, path, nestk, CUBALC_LANG_VERSION);
          return 2;
        }
        while (ai < argc && argv[ai] && argv[ai][0]) {
          size_t kl = strlen(argv[ai]);
          if (o + kl + 2 >= sizeof keys_nl) break;
          if (o > 0) keys_nl[o++] = '\n';
          memcpy(keys_nl + o, argv[ai], kl);
          o += kl;
          keys_nl[o] = 0;
          {
            size_t fo = strlen(flat_req);
            if (fo + kl + 2 < sizeof flat_req) {
              if (fo > 0) {
                flat_req[fo++] = ',';
                flat_req[fo] = 0;
              }
              memcpy(flat_req + fo, argv[ai], kl);
              flat_req[fo + kl] = 0;
            }
          }
          nreq++;
          ai++;
        }

        memset(&pr, 0, sizeof pr);
        if (is_omit) {
          if (cubalc_host_json_drop(nest, keys_nl, &pr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestomit\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, pr.err[0] ? pr.err : "json drop fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
        } else {
          if (cubalc_host_json_pick(nest, keys_nl, &pr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestpick\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"%s\","
                   "\"version\":\"%s\"}\n",
                   path, nestk, pr.err[0] ? pr.err : "json pick fail",
                   CUBALC_LANG_VERSION);
            return 1;
          }
        }
        snprintf(nest, sizeof nest, "%s", pr.str);

        /* path write-back: nest may be dotted (cfg.flags) */
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, nestk, nest, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 op, wr.err[0] ? wr.err : "outer set fail", CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(plate, sizeof plate, "%s", wr.str);
        slash = strrchr(path, '/');
        if (slash && slash != path) {
          size_t n = (size_t)(slash - path);
          if (n >= sizeof parent) n = sizeof parent - 1;
          memcpy(parent, path, n);
          parent[n] = 0;
          memset(&hr, 0, sizeof hr);
          cubalc_host_mkdir(parent, &hr);
        }
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_write(path, plate, &hr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"n\":%ld,"
               "\"n_req\":%ld,\"keys\":\"%s\",\"nest_hit\":%s,\"file\":%s,"
               "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s,"
               "\"note\":\"PICKOBJ/OMITOBJ dual · nest path ok\"}\n",
               op, path, nestk, pr.n, nreq, flat_req,
               nest_hit ? "true" : "false",
               file_hit ? "true" : "false", hr.n, CUBALC_LANG_VERSION, plate);
        return 0;
      }

if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"nest\":\"%s\",\"err\":\"need field\","
               "\"version\":\"%s\"}\n", op, path, nestk, CUBALC_LANG_VERSION);
        return 2;
      }
      field = argv[ai++];

      if (strcmp(op, "nesthas") == 0) {
        int hit = 0;
        if (nest_hit) {
          memset(&gr, 0, sizeof gr);
          if (cubalc_host_json_get_raw(nest, field, &gr) == 0)
            hit = 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"nesthas\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
               "\"hit\":%s,\"nest_hit\":%s,\"file\":%s,\"version\":\"%s\"}\n",
               path, nestk, field, hit ? "true" : "false",
               nest_hit ? "true" : "false",
               file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return hit ? 0 : 1;
      }

      if (strcmp(op, "nestget") == 0) {
        int have_fb2 = 0;
        const char *fb2 = NULL;
        if (ai < argc && (strcmp(argv[ai], "OR") == 0 ||
                          strcmp(argv[ai], "DEFAULT") == 0 ||
                          strcmp(argv[ai], "ELSE") == 0)) {
          ai++;
          if (ai < argc) {
            fb2 = argv[ai++];
            have_fb2 = 1;
          }
        }
        if (nest_hit) {
          memset(&gr, 0, sizeof gr);
          if (cubalc_host_json_get(nest, field, &gr) == 0) {
            char esc[CUBALC_HOST_STR_MAX];
            size_t i, o = 0;
            for (i = 0; gr.str[i] && o + 2 < sizeof esc; i++) {
              char c = gr.str[i];
              if (c == '"' || c == '\\') {
                esc[o++] = '\\';
                esc[o++] = c;
              } else if ((unsigned char)c < 32) {
                esc[o++] = ' ';
              } else {
                esc[o++] = c;
              }
            }
            esc[o] = 0;
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                   "\"op\":\"nestget\",\"path\":\"%s\",\"nest\":\"%s\","
                   "\"field\":\"%s\",\"hit\":true,\"value\":\"%s\",\"file\":%s,"
                   "\"version\":\"%s\"}\n",
                   path, nestk, field, esc,
                   file_hit ? "true" : "false", CUBALC_LANG_VERSION);
            return 0;
          }
        }
        if (have_fb2 && fb2) {
          char esc[512];
          size_t i, o = 0;
          for (i = 0; fb2[i] && o + 2 < sizeof esc; i++) {
            char c = fb2[i];
            if (c == '"' || c == '\\') {
              esc[o++] = '\\';
              esc[o++] = c;
            } else {
              esc[o++] = c;
            }
          }
          esc[o] = 0;
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestget\",\"path\":\"%s\",\"nest\":\"%s\","
                 "\"field\":\"%s\",\"hit\":false,\"or\":true,\"value\":\"%s\","
                 "\"file\":%s,\"version\":\"%s\"}\n",
                 path, nestk, field, esc,
                 file_hit ? "true" : "false", CUBALC_LANG_VERSION);
          return 0;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"nestget\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
               "\"hit\":false,\"err\":\"field miss\",\"file\":%s,\"version\":\"%s\"}\n",
               path, nestk, field,
               file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 1;
      }

      {
        long delta_n = 1, cur = 0, nv = 0;
        int val_kind2 = 0;
        const char *val2 = NULL;
        char raw[32];

        if (strcmp(op, "nestset") == 0) {
          size_t i;
          int allnum = 1;
          if (ai >= argc) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestset\",\"err\":\"need value\",\"version\":\"%s\"}\n",
                   CUBALC_LANG_VERSION);
            return 2;
          }
          val2 = argv[ai++];
          if (!strcmp(val2, "true") || !strcmp(val2, "false") ||
              !strcmp(val2, "null") || val2[0] == '{' || val2[0] == '[')
            val_kind2 = 1;
          else {
            if (val2[0] == '-' && val2[1]) i = 1;
            else i = 0;
            if (!val2[i]) allnum = 0;
            for (; val2[i]; i++) {
              if (val2[i] < '0' || val2[i] > '9') {
                allnum = 0;
                break;
              }
            }
            if (allnum) val_kind2 = 1;
          }
          memset(&wr, 0, sizeof wr);
          if (cubalc_host_json_set(nest, field, val2, val_kind2, &wr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestset\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   wr.err[0] ? wr.err : "nest set fail", CUBALC_LANG_VERSION);
            return 1;
          }
          snprintf(nest, sizeof nest, "%s", wr.str);
        } else if (strcmp(op, "nestinc") == 0) {
          if (ai < argc) {
            char *end = NULL;
            delta_n = strtol(argv[ai], &end, 10);
            if (end == argv[ai]) delta_n = 1;
            ai++;
          }
          memset(&gr, 0, sizeof gr);
          if (cubalc_host_json_get(nest, field, &gr) == 0) {
            char *end = NULL;
            cur = strtol(gr.str, &end, 10);
            if (end == gr.str) cur = 0;
          }
          nv = cur + delta_n;
          snprintf(raw, sizeof raw, "%ld", nv);
          memset(&wr, 0, sizeof wr);
          if (cubalc_host_json_set(nest, field, raw, 1, &wr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"nestinc\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   wr.err[0] ? wr.err : "nest set fail", CUBALC_LANG_VERSION);
            return 1;
          }
          snprintf(nest, sizeof nest, "%s", wr.str);
        } else {
          memset(&wr, 0, sizeof wr);
          if (cubalc_host_json_del(nest, field, &wr) != 0) {
            wr.n = 0;
          } else {
            snprintf(nest, sizeof nest, "%s", wr.str);
          }
        }
        {
          long nest_removed = wr.n;

        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, nestk, nest, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 op, wr.err[0] ? wr.err : "outer set fail", CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(plate, sizeof plate, "%s", wr.str);
        slash = strrchr(path, '/');
        if (slash && slash != path) {
          size_t n = (size_t)(slash - path);
          if (n >= sizeof parent) n = sizeof parent - 1;
          memcpy(parent, path, n);
          parent[n] = 0;
          memset(&hr, 0, sizeof hr);
          cubalc_host_mkdir(parent, &hr);
        }
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_write(path, plate, &hr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        if (strcmp(op, "nestinc") == 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestinc\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
                 "\"value\":%ld,\"delta\":%ld,\"bytes\":%ld,\"version\":\"%s\","
                 "\"plate\":%s}\n",
                 path, nestk, field, nv, delta_n, hr.n, CUBALC_LANG_VERSION, plate);
        } else if (strcmp(op, "nestdel") == 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestdel\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
                 "\"removed\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
                 path, nestk, field, nest_removed, hr.n, CUBALC_LANG_VERSION, plate);
        } else {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"nestset\",\"path\":\"%s\",\"nest\":\"%s\",\"field\":\"%s\","
                 "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
                 path, nestk, field, hr.n, CUBALC_LANG_VERSION, plate);
        }
        return 0;
        }
      }
    }

    /* pick|omit path k1 [k2 …] — project plate keys without a .cubalc program.
     * Usability: shell dual of PICKP / OMITP for peer/log slim plates.
     *   cubalc plate pick agent.json host status n
     *   cubalc plate omit agent.json tmp debug
     * Soft-missing keys skipped. Does not write file (stdout plate only). */
    if (strcmp(op, "pick") == 0 || strcmp(op, "omit") == 0) {
      char keys_nl[CUBALC_HOST_STR_MAX];
      char flat_req[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t o = 0;
      long nreq = 0;
      int is_omit = (strcmp(op, "omit") == 0) ? 1 : 0;

      keys_nl[0] = 0;
      flat_req[0] = 0;
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need one or more keys\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      while (ai < argc && argv[ai] && argv[ai][0]) {
        size_t kl = strlen(argv[ai]);
        if (o + kl + 2 >= sizeof keys_nl) break;
        if (o > 0) keys_nl[o++] = '\n';
        memcpy(keys_nl + o, argv[ai], kl);
        o += kl;
        keys_nl[o] = 0;
        {
          size_t fo = strlen(flat_req);
          if (fo + kl + 2 < sizeof flat_req) {
            if (fo > 0) {
              flat_req[fo++] = ',';
              flat_req[fo] = 0;
            }
            memcpy(flat_req + fo, argv[ai], kl);
            flat_req[fo + kl] = 0;
          }
        }
        nreq++;
        ai++;
      }

      memset(&pr, 0, sizeof pr);
      if (is_omit) {
        if (cubalc_host_json_drop(plate, keys_nl, &pr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"omit\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, pr.err[0] ? pr.err : "json drop fail", CUBALC_LANG_VERSION);
          return 1;
        }
      } else {
        if (cubalc_host_json_pick(plate, keys_nl, &pr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"pick\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, pr.err[0] ? pr.err : "json pick fail", CUBALC_LANG_VERSION);
          return 1;
        }
      }

      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"n\":%ld,\"n_req\":%ld,"
             "\"keys\":\"%s\",\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
             op, path, pr.n, nreq, flat_req,
             file_hit ? "true" : "false", CUBALC_LANG_VERSION, pr.str);
      return 0;
    }

    /* sum|avg|median path — numeric plate aggregates without a .cubalc program.
     * Usability: CLI duals of SUMNP / AVGNP / MEDIANP for agent one-shots.
     *   cubalc plate sum scores.json
     *   cubalc plate avg scores.json
     *   cubalc plate median|p50 scores.json
     * Soft 0 if no pure-int fields. Does not write file. */
    if (strcmp(op, "sum") == 0 || strcmp(op, "avg") == 0 ||
        strcmp(op, "median") == 0) {
      cubalc_host_result ar;
      long used = 0;
      memset(&ar, 0, sizeof ar);
      if (strcmp(op, "sum") == 0) {
        if (cubalc_host_json_sum(plate, &ar) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"sum\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, ar.err[0] ? ar.err : "json sum fail", CUBALC_LANG_VERSION);
          return 1;
        }
      } else if (strcmp(op, "avg") == 0) {
        if (cubalc_host_json_avg(plate, &ar) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"avg\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, ar.err[0] ? ar.err : "json avg fail", CUBALC_LANG_VERSION);
          return 1;
        }
        used = (long)ar.code;
      } else {
        if (cubalc_host_json_median(plate, &ar) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"median\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, ar.err[0] ? ar.err : "json median fail", CUBALC_LANG_VERSION);
          return 1;
        }
        used = (long)ar.code;
      }
      if (strcmp(op, "sum") == 0)
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"sum\",\"path\":\"%s\",\"n\":%ld,\"file\":%s,"
               "\"version\":\"%s\"}\n",
               path, ar.n, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      else
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"n\":%ld,\"used\":%ld,\"file\":%s,"
               "\"version\":\"%s\"}\n",
               op, path, ar.n, used, file_hit ? "true" : "false",
               CUBALC_LANG_VERSION);
      return 0;
    }

    /* top|bot path [n] — top/bottom N pure-int keys by value → plate.
     * Usability: CLI dual of TOPNP/BOTNP · default n=1.
     *   cubalc plate top freq.json 3
     *   cubalc plate bot scores.json
     * Soft empty plate if no ints. Does not write file. */
    if (strcmp(op, "top") == 0 || strcmp(op, "bot") == 0) {
      cubalc_host_result tr;
      cubalc_host_result tk;
      long ntake = 1;
      int want_bot = (strcmp(op, "bot") == 0) ? 1 : 0;
      char key1[256];
      long val1 = 0;
      if (ai < argc && argv[ai] && argv[ai][0]) {
        char *end = NULL;
        long v = strtol(argv[ai], &end, 10);
        if (end && end != argv[ai] && *end == 0)
          ntake = v;
      }
      if (ntake < 0) ntake = 0;
      memset(&tr, 0, sizeof tr);
      if (cubalc_host_json_topn(plate, ntake, want_bot, &tr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               op, path, tr.err[0] ? tr.err : "json topn fail", CUBALC_LANG_VERSION);
        return 1;
      }
      /* peel first key for agent convenience when n>=1 */
      key1[0] = 0;
      memset(&tk, 0, sizeof tk);
      if (tr.n > 0 && cubalc_host_json_keys(tr.str, &tk) == 0 && tk.str[0]) {
        size_t kn = 0;
        const char *kp = tk.str;
        while (*kp && *kp != '\n' && *kp != '\r' && kn + 1 < sizeof key1)
          key1[kn++] = *kp++;
        key1[kn] = 0;
        if (key1[0]) {
          cubalc_host_result gr2;
          char *end = NULL;
          memset(&gr2, 0, sizeof gr2);
          if (cubalc_host_json_get(tr.str, key1, &gr2) == 0 && gr2.str[0]) {
            val1 = strtol(gr2.str, &end, 10);
            if (!(end && end != gr2.str && *end == 0))
              val1 = 0;
          }
        }
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"n\":%ld,\"cand\":%d,\"take\":%ld,"
             "\"key\":\"%s\",\"value\":%ld,\"file\":%s,\"version\":\"%s\","
             "\"plate\":%s}\n",
             op, path, tr.n, tr.code, ntake, key1, val1,
             file_hit ? "true" : "false", CUBALC_LANG_VERSION, tr.str);
      return 0;
    }

    /* fill|fillkeys — expand {{key}} from plate file without a .cubalc program.
     * Usability: one-shot agent status/path templates (CLI dual of FILLP/FILLPKEYS).
     *   cubalc plate fill agent.json "peer={{name}}@{{host}}"
     *   cubalc plate fill -s agent.json @tpl.txt out.txt
     *   cubalc plate fillkeys agent.json @tpl.txt
     * Template: literal (contains {), @file, or readable path. */
    if (strcmp(op, "fill") == 0 || strcmp(op, "fillkeys") == 0) {
      const char *tmpl_arg = NULL;
      const char *out_path = NULL;
      char tmpl[CUBALC_HOST_STR_MAX], body[CUBALC_HOST_STR_MAX];
      char miss_keys[CUBALC_HOST_STR_MAX], esc[CUBALC_HOST_STR_MAX];
      char flat[CUBALC_HOST_STR_MAX];
      long hits = 0, miss = 0, nk = 0;
      int is_strict = (val_kind == 99) ? 1 : 0;
      int is_keys = (strcmp(op, "fillkeys") == 0) ? 1 : 0;
      size_t i, o;
      cubalc_host_result tr, wrr;

      tmpl[0] = 0; body[0] = 0; miss_keys[0] = 0;
      /* optional -s|--strict before template */
      if (ai < argc && argv[ai] &&
          (!strcmp(argv[ai], "--strict") || !strcmp(argv[ai], "-s"))) {
        is_strict = 1;
        ai++;
      }
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"err\":\"need template string or @file\","
               "\"version\":\"%s\"}\n", op, CUBALC_LANG_VERSION);
        return 2;
      }
      tmpl_arg = argv[ai++];
      if (ai < argc && argv[ai] && argv[ai][0] &&
          strcmp(argv[ai], "--strict") != 0 && strcmp(argv[ai], "-s") != 0)
        out_path = argv[ai++];
      if (ai < argc && argv[ai] &&
          (!strcmp(argv[ai], "--strict") || !strcmp(argv[ai], "-s"))) {
        is_strict = 1;
        ai++;
        if (!out_path && ai < argc && argv[ai] && argv[ai][0])
          out_path = argv[ai++];
      }

      if (tmpl_arg[0] == '@') {
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(tmpl_arg + 1, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"template read fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(tmpl, sizeof tmpl, "%s", tr.str);
      } else if (strchr(tmpl_arg, '{') == NULL && access(tmpl_arg, R_OK) == 0) {
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(tmpl_arg, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"template read fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(tmpl, sizeof tmpl, "%s", tr.str);
      } else {
        snprintf(tmpl, sizeof tmpl, "%s", tmpl_arg);
      }

      if (is_keys) {
        cubalc_host_fillp_keys(tmpl, body, sizeof body, &nk);
        o = 0; flat[0] = 0;
        for (i = 0; body[i] && o + 2 < sizeof flat; i++) {
          char c = body[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',') flat[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"fillkeys\",\"path\":\"%s\",\"file\":%s,\"n\":%ld,"
               "\"keys\":\"%s\",\"version\":\"%s\"}\n",
               path, file_hit ? "true" : "false", nk, flat, CUBALC_LANG_VERSION);
        return 0;
      }

      cubalc_host_expand_fillp(plate, tmpl, body, sizeof body,
                               &hits, &miss, miss_keys, sizeof miss_keys);

      if (is_strict && miss > 0) {
        o = 0; flat[0] = 0;
        for (i = 0; miss_keys[i] && o + 2 < sizeof flat; i++) {
          char c = miss_keys[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',') {
              flat[o++] = ',';
              if (o + 1 < sizeof flat) flat[o++] = ' ';
            }
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"fill\",\"strict\":true,\"path\":\"%s\",\"hits\":%ld,"
               "\"miss\":%ld,\"miss_keys\":\"%s\","
               "\"err\":\"missing plate keys\",\"version\":\"%s\"}\n",
               path, hits, miss, flat, CUBALC_LANG_VERSION);
        return 1;
      }

      if (out_path && out_path[0]) {
        slash = strrchr(out_path, '/');
        if (slash && slash != out_path) {
          size_t n = (size_t)(slash - out_path);
          if (n >= sizeof parent) n = sizeof parent - 1;
          memcpy(parent, out_path, n);
          parent[n] = 0;
          memset(&hr, 0, sizeof hr);
          cubalc_host_mkdir(parent, &hr);
        }
        memset(&wrr, 0, sizeof wrr);
        if (cubalc_host_write(out_path, body, &wrr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"fill\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"fill\",\"path\":\"%s\",\"out\":\"%s\",\"hits\":%ld,"
               "\"miss\":%ld,\"bytes\":%ld,\"strict\":%s,\"file\":%s,"
               "\"version\":\"%s\"}\n",
               path, out_path, hits, miss, (long)strlen(body),
               is_strict ? "true" : "false",
               file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }

      fputs(body, stdout);
      if (!body[0] || body[strlen(body) - 1] != '\n')
        fputc('\n', stdout);
      o = 0; esc[0] = 0;
      for (i = 0; body[i] && o + 2 < sizeof esc; i++) {
        char c = body[i];
        if (c == '"' || c == '\\') {
          esc[o++] = '\\';
          esc[o++] = c;
        } else if (c == '\n') {
          esc[o++] = '\\';
          esc[o++] = 'n';
        } else if (c == '\r') {
        } else if ((unsigned char)c < 32) {
          esc[o++] = ' ';
        } else {
          esc[o++] = c;
        }
      }
      esc[o] = 0;
      o = 0; flat[0] = 0;
      for (i = 0; miss_keys[i] && o + 2 < sizeof flat; i++) {
        char c = miss_keys[i];
        if (c == '\n' || c == '\r') {
          if (o > 0 && flat[o - 1] != ',') flat[o++] = ',';
        } else if (c == '"' || c == '\\') {
          flat[o++] = '_';
        } else {
          flat[o++] = c;
        }
      }
      flat[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"fill\",\"path\":\"%s\",\"hits\":%ld,\"miss\":%ld,"
             "\"miss_keys\":\"%s\",\"bytes\":%ld,\"strict\":%s,\"file\":%s,"
             "\"body\":\"%s\",\"version\":\"%s\","
             "\"note\":\"filled body above plate · FILLP dual for CLI agents\"}\n",
             path, hits, miss, flat, (long)strlen(body),
             is_strict ? "true" : "false",
             file_hit ? "true" : "false", esc, CUBALC_LANG_VERSION);
      return 0;
    }

    /* ensure|seed — create object plate only if missing/non-object (ENSUREPLATE dual).
     *   cubalc plate ensure agent.json
     *   cubalc plate ensure agent.json '{"n":0,"ok":true}'
     *   cubalc plate ensure agent.json @seed.json
     * Does not clobber an existing object plate (use set/merge to mutate). */
    if (strcmp(op, "ensure") == 0) {
      const char *seed_arg = NULL;
      char seed[CUBALC_HOST_STR_MAX];
      cubalc_host_result tr;

      seed[0] = 0;
      if (ai < argc && argv[ai] && argv[ai][0])
        seed_arg = argv[ai++];
      if (seed_arg) {
        if (seed_arg[0] == '@') {
          memset(&tr, 0, sizeof tr);
          if (cubalc_host_read(seed_arg + 1, &tr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"ensure\",\"err\":\"seed read fail\",\"version\":\"%s\"}\n",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          snprintf(seed, sizeof seed, "%s", tr.str);
        } else if (seed_arg[0] != '{' && access(seed_arg, R_OK) == 0) {
          memset(&tr, 0, sizeof tr);
          if (cubalc_host_read(seed_arg, &tr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"ensure\",\"err\":\"seed read fail\",\"version\":\"%s\"}\n",
                   CUBALC_LANG_VERSION);
            return 1;
          }
          snprintf(seed, sizeof seed, "%s", tr.str);
        } else {
          snprintf(seed, sizeof seed, "%s", seed_arg);
        }
      } else {
        snprintf(seed, sizeof seed, "%s", "{}");
      }
      {
        const char *sb = seed;
        while (*sb == ' ' || *sb == '\t' || *sb == '\n' || *sb == '\r') sb++;
        if (*sb != '{') {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"ensure\",\"err\":\"seed must be JSON object\","
                 "\"version\":\"%s\"}\n", CUBALC_LANG_VERSION);
          return 2;
        }
        if (sb != seed)
          memmove(seed, sb, strlen(sb) + 1);
      }

      if (file_hit) {
        /* keep existing object plate — no write */
        memset(&keys, 0, sizeof keys);
        cubalc_host_json_keys(plate, &keys);
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"ensure\",\"path\":\"%s\",\"created\":false,\"file\":true,"
               "\"keys_n\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
               path, keys.n, (long)strlen(plate), CUBALC_LANG_VERSION, plate);
        return 0;
      }

      snprintf(plate, sizeof plate, "%s", seed);
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"ensure\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, hr.err[0] ? hr.err : "write fail", CUBALC_LANG_VERSION);
        return 1;
      }
      memset(&keys, 0, sizeof keys);
      cubalc_host_json_keys(plate, &keys);
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"ensure\",\"path\":\"%s\",\"created\":true,\"file\":false,"
             "\"keys_n\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, keys.n, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* merge|patch|overlay — multi-key overlay onto plate file (JSONFILEMERGE dual).
     *   cubalc plate merge agent.json '{"status":"ready","n":3}'
     *   cubalc plate merge agent.json @patch.json
     * Creates {} base if missing; overlay wins on key clash. */
    if (strcmp(op, "merge") == 0) {
      const char *over_arg = NULL;
      char over[CUBALC_HOST_STR_MAX];
      cubalc_host_result tr, mer;

      over[0] = 0;
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"merge\",\"path\":\"%s\",\"err\":\"need overlay object or @file\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      over_arg = argv[ai++];
      if (over_arg[0] == '@') {
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(over_arg + 1, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"merge\",\"err\":\"overlay read fail\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(over, sizeof over, "%s", tr.str);
      } else if (over_arg[0] != '{' && access(over_arg, R_OK) == 0) {
        memset(&tr, 0, sizeof tr);
        if (cubalc_host_read(over_arg, &tr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"merge\",\"err\":\"overlay read fail\",\"version\":\"%s\"}\n",
                 CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(over, sizeof over, "%s", tr.str);
      } else {
        snprintf(over, sizeof over, "%s", over_arg);
      }
      {
        const char *ob = over;
        while (*ob == ' ' || *ob == '\t' || *ob == '\n' || *ob == '\r') ob++;
        if (*ob != '{') {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"merge\",\"err\":\"overlay must be JSON object\","
                 "\"version\":\"%s\"}\n", CUBALC_LANG_VERSION);
          return 2;
        }
      }

      memset(&mer, 0, sizeof mer);
      if (cubalc_host_json_merge(plate, over, &mer) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"merge\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, mer.err[0] ? mer.err : "json merge fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", mer.str);
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"merge\",\"path\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               path, hr.err[0] ? hr.err : "write fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"merge\",\"path\":\"%s\",\"applied\":%ld,\"file\":%s,"
             "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, mer.n, file_hit ? "true" : "false", hr.n,
             CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* eq|ne|diff|changelog a.json b.json — compare two agent plates without .cubalc.
     * Usability: mesh/peer sync verify (JSONEQ / JSONCHANGED / JSONCHANGELOG duals).
     * Soft-missing files load as {}. eq exit 0 if equal else 1; ne inverted;
     * diff/changelog exit 0 if identical else 1 (keys/lines listed either way). */
    if (strcmp(op, "eq") == 0 || strcmp(op, "ne") == 0 || strcmp(op, "diff") == 0 ||
        strcmp(op, "changelog") == 0 ||
        strcmp(op, "diffflat") == 0 || strcmp(op, "pathdiff") == 0) {
      const char *path2 = NULL;
      char plate2[CUBALC_HOST_STR_MAX];
      int file_hit2 = 0, equal = 0;
      cubalc_host_result er, ch;
      char flat[CUBALC_HOST_STR_MAX];
      char esc[CUBALC_HOST_STR_MAX];
      size_t i, o;

      plate2[0] = 0;
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need second path\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      path2 = argv[ai++];

      /* soft-load second plate (mirror path load above) */
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(path2, &hr) == 0 && hr.str[0]) {
        body = hr.str;
        while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
          body++;
        if (*body == '{') {
          memset(&keys, 0, sizeof keys);
          if (cubalc_host_json_keys(body, &keys) == 0) {
            snprintf(plate2, sizeof plate2, "%s", body);
            file_hit2 = 1;
          }
        }
      }
      if (!file_hit2)
        snprintf(plate2, sizeof plate2, "%s", "{}");

      memset(&er, 0, sizeof er);
      cubalc_host_json_eq(plate, plate2, &er);
      equal = (er.n == 1) ? 1 : 0;

      if (strcmp(op, "eq") == 0 || strcmp(op, "ne") == 0) {
        int want_ne = (strcmp(op, "ne") == 0);
        int pass = want_ne ? !equal : equal;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"path2\":\"%s\","
               "\"equal\":%s,\"file\":%s,\"file2\":%s,\"version\":\"%s\"}\n",
               op, path, path2,
               equal ? "true" : "false",
               file_hit ? "true" : "false",
               file_hit2 ? "true" : "false",
               CUBALC_LANG_VERSION);
        return pass ? 0 : 1;
      }

      if (strcmp(op, "diff") == 0) {
        /* diff — changed keys bag (want_same=0) */
        memset(&ch, 0, sizeof ch);
        cubalc_host_json_changed_keys(plate, plate2, 0, &ch);
        o = 0; flat[0] = 0;
        for (i = 0; ch.str[i] && o + 2 < sizeof flat; i++) {
          char c = ch.str[i];
          if (c == '\n' || c == '\r') {
            if (o > 0 && flat[o - 1] != ',') flat[o++] = ',';
          } else if (c == '"' || c == '\\') {
            flat[o++] = '_';
          } else {
            flat[o++] = c;
          }
        }
        flat[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"diff\",\"path\":\"%s\",\"path2\":\"%s\","
               "\"equal\":%s,\"n\":%ld,\"keys\":\"%s\","
               "\"file\":%s,\"file2\":%s,\"version\":\"%s\"}\n",
               path, path2,
               equal ? "true" : "false", ch.n, flat,
               file_hit ? "true" : "false",
               file_hit2 ? "true" : "false",
               CUBALC_LANG_VERSION);
        return equal ? 0 : 1;
      }

      /* diffflat|pathdiff — deep leaf-path compare (DIFFFLAT / PATHDIFF duals).
       * Nest-aware changelog / path bag above plate. */
      if (strcmp(op, "diffflat") == 0 || strcmp(op, "pathdiff") == 0) {
        int paths_only = (strcmp(op, "pathdiff") == 0) ? 1 : 0;
        memset(&ch, 0, sizeof ch);
        cubalc_host_json_leaf_diff(plate, plate2, paths_only, &ch);
        if (ch.str[0]) {
          fputs(ch.str, stdout);
          if (ch.str[strlen(ch.str) - 1] != '\n')
            fputc('\n', stdout);
        }
        o = 0; esc[0] = 0;
        for (i = 0; ch.str[i] && o + 2 < sizeof esc; i++) {
          char c = ch.str[i];
          if (c == '"' || c == '\\') {
            esc[o++] = '\\';
            esc[o++] = c;
          } else if (c == '\n') {
            esc[o++] = '\\';
            esc[o++] = 'n';
          } else if (c == '\r') {
          } else if ((unsigned char)c < 32) {
            esc[o++] = ' ';
          } else {
            esc[o++] = c;
          }
        }
        esc[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"path2\":\"%s\","
               "\"equal\":%s,\"n\":%ld,\"lines\":\"%s\","
               "\"file\":%s,\"file2\":%s,\"version\":\"%s\","
               "\"note\":\"%s\"}\n",
               op, path, path2,
               (ch.n == 0) ? "true" : "false", ch.n, esc,
               file_hit ? "true" : "false",
               file_hit2 ? "true" : "false",
               CUBALC_LANG_VERSION,
               paths_only
                   ? "PATHDIFF dual · changed leaf paths · bag above plate"
                   : "DIFFFLAT dual · deep leaf path: old → new · bag above plate");
        return (ch.n == 0) ? 0 : 1;
      }

      /* changelog — human/agent "key: old → new" lines (JSONCHANGELOG dual).
       * Prints newline bag above the meta plate for greppable agent logs. */
      memset(&ch, 0, sizeof ch);
      cubalc_host_json_changelog(plate, plate2, &ch);
      if (ch.str[0]) {
        fputs(ch.str, stdout);
        if (ch.str[strlen(ch.str) - 1] != '\n')
          fputc('\n', stdout);
      }
      o = 0; esc[0] = 0;
      for (i = 0; ch.str[i] && o + 2 < sizeof esc; i++) {
        char c = ch.str[i];
        if (c == '"' || c == '\\') {
          esc[o++] = '\\';
          esc[o++] = c;
        } else if (c == '\n') {
          esc[o++] = '\\';
          esc[o++] = 'n';
        } else if (c == '\r') {
        } else if ((unsigned char)c < 32) {
          esc[o++] = ' ';
        } else {
          esc[o++] = c;
        }
      }
      esc[o] = 0;
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"changelog\",\"path\":\"%s\",\"path2\":\"%s\","
             "\"equal\":%s,\"n\":%ld,\"lines\":\"%s\","
             "\"file\":%s,\"file2\":%s,\"version\":\"%s\","
             "\"note\":\"changelog lines above plate · JSONCHANGELOG dual\"}\n",
             path, path2,
             equal ? "true" : "false", ch.n, esc,
             file_hit ? "true" : "false",
             file_hit2 ? "true" : "false",
             CUBALC_LANG_VERSION);
      return equal ? 0 : 1;
    }

    /* has|need path k1 [k2 …] — multi-key presence contract on a plate file.
     * Usability: shell dual of HASPALL / NEEDP without a .cubalc program.
     * Keys may be dotted nest paths: cubalc plate need agent.json host freq.error
     *   cubalc plate has  agent.json n status role
     *   cubalc plate need agent.json n ok
     * has: ok always true · has_all · miss bag · exit 0 iff all present
     * need: ok false + err when miss (hard gate for agent scripts) */
    if (strcmp(op, "has") == 0 || strcmp(op, "need") == 0) {
      char keys_nl[CUBALC_HOST_STR_MAX];
      char flat_miss[CUBALC_HOST_STR_MAX];
      char flat_req[CUBALC_HOST_STR_MAX];
      cubalc_host_result miss, hasr;
      size_t o = 0, i;
      long nreq = 0;
      int all_ok;

      keys_nl[0] = 0;
      flat_req[0] = 0;
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need one or more keys\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      while (ai < argc && argv[ai] && argv[ai][0]) {
        size_t kl = strlen(argv[ai]);
        if (o + kl + 2 >= sizeof keys_nl) break;
        if (o > 0) keys_nl[o++] = '\n';
        memcpy(keys_nl + o, argv[ai], kl);
        o += kl;
        keys_nl[o] = 0;
        /* flat req for plate meta */
        {
          size_t fo = strlen(flat_req);
          if (fo + kl + 2 < sizeof flat_req) {
            if (fo > 0) {
              flat_req[fo++] = ',';
              flat_req[fo] = 0;
            }
            memcpy(flat_req + fo, argv[ai], kl);
            flat_req[fo + kl] = 0;
          }
        }
        nreq++;
        ai++;
      }

      memset(&miss, 0, sizeof miss);
      cubalc_host_json_filter_req_keys(plate, keys_nl, 0 /* missing */, &miss);
      memset(&hasr, 0, sizeof hasr);
      cubalc_host_json_has_keys(plate, keys_nl, 1 /* all */, &hasr);
      all_ok = (hasr.n == 1) ? 1 : 0;

      o = 0; flat_miss[0] = 0;
      for (i = 0; miss.str[i] && o + 2 < sizeof flat_miss; i++) {
        char c = miss.str[i];
        if (c == '\n' || c == '\r') {
          if (o > 0 && flat_miss[o - 1] != ',') flat_miss[o++] = ',';
        } else if (c == '"' || c == '\\') {
          flat_miss[o++] = '_';
        } else {
          flat_miss[o++] = c;
        }
      }
      flat_miss[o] = 0;

      if (strcmp(op, "need") == 0 && !all_ok) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"need\",\"path\":\"%s\",\"has_all\":false,"
               "\"n_req\":%ld,\"n_hit\":%ld,\"n_miss\":%ld,\"miss\":\"%s\","
               "\"keys\":\"%s\",\"file\":%s,\"err\":\"missing plate keys\","
               "\"version\":\"%s\"}\n",
               path, nreq, (long)hasr.code, miss.n, flat_miss, flat_req,
               file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 1;
      }

      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"has_all\":%s,"
             "\"n_req\":%ld,\"n_hit\":%ld,\"n_miss\":%ld,\"miss\":\"%s\","
             "\"keys\":\"%s\",\"file\":%s,\"version\":\"%s\"}\n",
             op, path, all_ok ? "true" : "false",
             nreq, (long)hasr.code, miss.n, flat_miss, flat_req,
             file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      return all_ok ? 0 : 1;
    }

    /* pluck|getall path k1 [k2 …] — multi-key peel → value bag (PLUCKP dual).
     * Usability: agent one-shot extract including dotted nest paths without .cubalc:
     *   cubalc plate pluck agent.json host freq.error n
     * Prints bag lines above plate meta · empty field on miss · hit/miss counts. */
    if (strcmp(op, "pluck") == 0) {
      char keys_nl[CUBALC_HOST_STR_MAX];
      char flat_req[CUBALC_HOST_STR_MAX];
      char bag_esc[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr;
      size_t o = 0, bi, bo;
      long nreq = 0;

      keys_nl[0] = 0;
      flat_req[0] = 0;
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pluck\",\"path\":\"%s\",\"err\":\"need one or more keys\","
               "\"version\":\"%s\"}\n", path, CUBALC_LANG_VERSION);
        return 2;
      }
      while (ai < argc && argv[ai] && argv[ai][0]) {
        size_t kl = strlen(argv[ai]);
        if (o + kl + 2 >= sizeof keys_nl) break;
        if (o > 0) keys_nl[o++] = '\n';
        memcpy(keys_nl + o, argv[ai], kl);
        o += kl;
        keys_nl[o] = 0;
        {
          size_t fo = strlen(flat_req);
          if (fo + kl + 2 < sizeof flat_req) {
            if (fo > 0) {
              flat_req[fo++] = ',';
              flat_req[fo] = 0;
            }
            memcpy(flat_req + fo, argv[ai], kl);
            flat_req[fo + kl] = 0;
          }
        }
        nreq++;
        ai++;
      }

      memset(&pr, 0, sizeof pr);
      if (cubalc_host_json_pluck(plate, keys_nl, &pr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"pluck\",\"path\":\"%s\",\"err\":\"%s\","
               "\"version\":\"%s\"}\n",
               path, pr.err[0] ? pr.err : "pluck fail", CUBALC_LANG_VERSION);
        return 1;
      }

      /* bag lines for greppable agent pipes */
      if (pr.str[0]) {
        fputs(pr.str, stdout);
        if (pr.str[strlen(pr.str) - 1] != '\n')
          fputc('\n', stdout);
      }

      bag_esc[0] = 0;
      bo = 0;
      for (bi = 0; pr.str[bi] && bo + 2 < sizeof bag_esc; bi++) {
        char c = pr.str[bi];
        if (c == '"' || c == '\\') {
          bag_esc[bo++] = '\\';
          bag_esc[bo++] = c;
        } else if (c == '\n' || c == '\r') {
          bag_esc[bo++] = '\\';
          bag_esc[bo++] = (c == '\n') ? 'n' : 'r';
        } else if ((unsigned char)c < 32) {
          bag_esc[bo++] = ' ';
        } else {
          bag_esc[bo++] = c;
        }
      }
      bag_esc[bo] = 0;

      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"pluck\",\"path\":\"%s\",\"n\":%ld,\"hit\":%ld,\"miss\":%ld,"
             "\"keys\":\"%s\",\"bag\":\"%s\",\"file\":%s,\"version\":\"%s\","
             "\"note\":\"bag lines above plate · PLUCKP dual · paths ok\"}\n",
             path, pr.n, (long)pr.code, pr.n - (long)pr.code, flat_req, bag_esc,
             file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      return 0;
    }

    /* rename|copy|swap path src dst — RENAMEP/COPYP/SWAPP duals (paths ok).
     * Usability: restructure nest fields without a .cubalc program:
     *   cubalc plate rename agent.json cfg.port net.listen
     *   cubalc plate copy   agent.json flags.debug flags.trace
     *   cubalc plate swap   agent.json a.x a.y
     * Soft miss: rename/copy n=0 plate unchanged · swap both-miss n=0. */
    if (strcmp(op, "rename") == 0 || strcmp(op, "copy") == 0 ||
        strcmp(op, "swap") == 0) {
      const char *src = NULL, *dst = NULL;
      cubalc_host_result rr;
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need src key\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      src = argv[ai++];
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need dst key\","
               "\"version\":\"%s\"}\n", op, path, CUBALC_LANG_VERSION);
        return 2;
      }
      dst = argv[ai++];
      memset(&rr, 0, sizeof rr);
      if (strcmp(op, "rename") == 0) {
        if (cubalc_host_json_rename(plate, src, dst, &rr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"rename\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
                 "\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, src, dst, rr.err[0] ? rr.err : "rename fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }
      } else if (strcmp(op, "copy") == 0) {
        if (cubalc_host_json_copy_key(plate, src, dst, &rr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"copy\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
                 "\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, src, dst, rr.err[0] ? rr.err : "copy fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }
      } else {
        if (cubalc_host_json_swap_keys(plate, src, dst, &rr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"swap\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
                 "\"err\":\"%s\",\"version\":\"%s\"}\n",
                 path, src, dst, rr.err[0] ? rr.err : "swap fail",
                 CUBALC_LANG_VERSION);
          return 1;
        }
      }
      snprintf(plate, sizeof plate, "%s", rr.str);
      if (rr.n) {
        slash = strrchr(path, '/');
        if (slash && slash != path) {
          size_t n = (size_t)(slash - path);
          if (n >= sizeof parent) n = sizeof parent - 1;
          memcpy(parent, path, n);
          parent[n] = 0;
          memset(&hr, 0, sizeof hr);
          cubalc_host_mkdir(parent, &hr);
        }
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_write(path, plate, &hr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
               "\"n\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
               op, path, src, dst, rr.n, hr.n, CUBALC_LANG_VERSION, plate);
        return 0;
      }
      /* soft miss / no-op write skipped */
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
             "\"n\":0,\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
             op, path, src, dst, file_hit ? "true" : "false",
             CUBALC_LANG_VERSION, plate);
      return 0;
    }

        /* remaining ops need key */
    if (ai >= argc || !argv[ai] || !argv[ai][0]) {
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
             "\"op\":\"%s\",\"path\":\"%s\",\"err\":\"need key\",\"version\":\"%s\"}\n",
             op, path, CUBALC_LANG_VERSION);
      return 2;
    }
    key = argv[ai++];

    if (strcmp(op, "get") == 0) {
      if (ai < argc && (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "DEFAULT") == 0 ||
                        strcmp(argv[ai], "ELSE") == 0)) {
        ai++;
        if (ai < argc) {
          fb = argv[ai++];
          have_fb = 1;
        }
      }
      memset(&gr, 0, sizeof gr);
      /* dotted/slash path: plate get agent.json freq.error */
      if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
        /* escape value lightly for JSON string field */
        char esc[CUBALC_HOST_STR_MAX];
        size_t i, o = 0;
        for (i = 0; gr.str[i] && o + 2 < sizeof esc; i++) {
          char c = gr.str[i];
          if (c == '"' || c == '\\') {
            esc[o++] = '\\';
            esc[o++] = c;
          } else if ((unsigned char)c < 32) {
            esc[o++] = ' ';
          } else {
            esc[o++] = c;
          }
        }
        esc[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"get\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":true,"
               "\"value\":\"%s\",\"file\":%s,\"version\":\"%s\"}\n",
               path, key, esc, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }
      if (have_fb) {
        char esc[512];
        size_t i, o = 0;
        for (i = 0; fb[i] && o + 2 < sizeof esc; i++) {
          char c = fb[i];
          if (c == '"' || c == '\\') {
            esc[o++] = '\\';
            esc[o++] = c;
          } else {
            esc[o++] = c;
          }
        }
        esc[o] = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"get\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
               "\"or\":true,\"value\":\"%s\",\"file\":%s,\"version\":\"%s\"}\n",
               path, key, esc, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 0;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
             "\"op\":\"get\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
             "\"err\":\"key miss\",\"file\":%s,\"version\":\"%s\"}\n",
             path, key, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      return 1;
    }

    /* getn|num path key [OR n] — GETPN dual: numeric peel (paths ok).
     * Usability: shell dual of GETPN / SYS JSONN for nest counters without .cubalc:
     *   cubalc plate getn agent.json cfg.port
     *   cubalc plate getn agent.json freq.error OR 0
     * n = numeric value · hit true|false · exit 0 on hit or OR, 1 on hard miss. */
    if (strcmp(op, "getn") == 0) {
      long nval = 0, fb_n = 0;
      char *end = NULL;
      if (ai < argc && (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "DEFAULT") == 0 ||
                        strcmp(argv[ai], "ELSE") == 0)) {
        ai++;
        if (ai < argc) {
          fb = argv[ai++];
          have_fb = 1;
          fb_n = strtol(fb, &end, 10);
          if (end == fb) fb_n = 0;
        }
      }
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
        end = NULL;
        nval = strtol(gr.str, &end, 10);
        if (end == gr.str) nval = 0;
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getn\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":true,"
               "\"n\":%ld,\"value\":\"%s\",\"file\":%s,\"version\":\"%s\","
               "\"note\":\"GETPN dual · LAST_N-style numeric peel · paths ok\"}\n",
               path, key, nval, gr.str, file_hit ? "true" : "false",
               CUBALC_LANG_VERSION);
        return 0;
      }
      if (have_fb) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"getn\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
               "\"or\":true,\"n\":%ld,\"value\":\"%ld\",\"file\":%s,"
               "\"version\":\"%s\"}\n",
               path, key, fb_n, fb_n, file_hit ? "true" : "false",
               CUBALC_LANG_VERSION);
        return 0;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
             "\"op\":\"getn\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
             "\"n\":0,\"err\":\"key miss\",\"file\":%s,\"version\":\"%s\"}\n",
             path, key, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      return 1;
    }

    /* getobj path key [OR '{}'] — GETOBJ dual: peel nested object (paths ok).
     * setobj path key obj|@file — SETOBJ dual: nest object write-back (paths ok).
     * mergeobj|defaultobj path key obj|@file — MERGEOBJ/DEFAULTOBJ duals.
     * Usability: deep nest object peel/patch without a .cubalc program:
     *   cubalc plate getobj agent.json cfg.meta
     *   cubalc plate setobj agent.json net.peer '{"host":"B"}'
     *   cubalc plate mergeobj agent.json cfg.meta '{"zone":"A"}'
     */
    if (strcmp(op, "getobj") == 0 || strcmp(op, "setobj") == 0 ||
        strcmp(op, "mergeobj") == 0 || strcmp(op, "defaultobj") == 0) {
      char objbuf[CUBALC_HOST_STR_MAX];
      char nest[CUBALC_HOST_STR_MAX];
      cubalc_host_result pr, mr;
      const char *v;
      int hit = 0;

      if (strcmp(op, "getobj") == 0) {
        if (ai < argc && (strcmp(argv[ai], "OR") == 0 || strcmp(argv[ai], "DEFAULT") == 0 ||
                          strcmp(argv[ai], "ELSE") == 0)) {
          ai++;
          if (ai < argc) {
            fb = argv[ai++];
            have_fb = 1;
          }
        }
        memset(&gr, 0, sizeof gr);
        if (cubalc_host_json_path_get_raw(plate, key, &gr) == 0) {
          v = gr.str;
          while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
          if (*v == '{') {
            hit = 1;
            if (v != gr.str) {
              size_t n = strlen(v);
              memmove(gr.str, v, n + 1);
            }
          }
        }
        if (hit) {
          fputs(gr.str, stdout);
          if (!gr.str[0] || gr.str[strlen(gr.str) - 1] != '\n')
            fputc('\n', stdout);
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"getobj\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":true,"
                 "\"file\":%s,\"version\":\"%s\",\"plate\":%s,"
                 "\"note\":\"GETOBJ dual · object body above plate · paths ok\"}\n",
                 path, key, file_hit ? "true" : "false", CUBALC_LANG_VERSION, gr.str);
          return 0;
        }
        if (have_fb) {
          const char *body_out = fb;
          while (*body_out == ' ' || *body_out == '\t') body_out++;
          if (*body_out != '{')
            body_out = "{}";
          fputs(body_out, stdout);
          if (body_out[strlen(body_out) - 1] != '\n')
            fputc('\n', stdout);
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
                 "\"op\":\"getobj\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
                 "\"or\":true,\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
                 path, key, file_hit ? "true" : "false", CUBALC_LANG_VERSION, body_out);
          return 0;
        }
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"getobj\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":false,"
               "\"err\":\"key miss or not object\",\"file\":%s,\"version\":\"%s\"}\n",
               path, key, file_hit ? "true" : "false", CUBALC_LANG_VERSION);
        return 1;
      }

      /* setobj|mergeobj|defaultobj need object value */
      if (ai >= argc || !argv[ai] || !argv[ai][0]) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"err\":\"need object value or @file\","
               "\"version\":\"%s\"}\n", op, CUBALC_LANG_VERSION);
        return 2;
      }
      val = argv[ai++];
      objbuf[0] = 0;
      if (val[0] == '@') {
        memset(&pr, 0, sizeof pr);
        if (cubalc_host_read(val + 1, &pr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"object read fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(objbuf, sizeof objbuf, "%s", pr.str);
      } else if (strchr(val, '{') == NULL && access(val, R_OK) == 0) {
        memset(&pr, 0, sizeof pr);
        if (cubalc_host_read(val, &pr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"object read fail\",\"version\":\"%s\"}\n",
                 op, CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(objbuf, sizeof objbuf, "%s", pr.str);
      } else {
        snprintf(objbuf, sizeof objbuf, "%s", val);
      }
      v = objbuf;
      while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
      if (*v != '{') {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"err\":\"value not object\",\"version\":\"%s\"}\n",
               op, CUBALC_LANG_VERSION);
        return 1;
      }
      if (v != objbuf) {
        size_t n = strlen(v);
        memmove(objbuf, v, n + 1);
      }

      if (strcmp(op, "setobj") == 0) {
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, key, objbuf, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"setobj\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 wr.err[0] ? wr.err : "set fail", CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(plate, sizeof plate, "%s", wr.str);
      } else {
        /* mergeobj / defaultobj: peel nest, merge/defaults, path_set */
        memset(&pr, 0, sizeof pr);
        cubalc_host_json_path_obj(plate, key, &pr);
        snprintf(nest, sizeof nest, "%s", pr.str);
        memset(&mr, 0, sizeof mr);
        if (strcmp(op, "defaultobj") == 0) {
          if (cubalc_host_json_defaults(nest, objbuf, &mr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"defaultobj\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   mr.err[0] ? mr.err : "defaults fail", CUBALC_LANG_VERSION);
            return 1;
          }
        } else {
          if (cubalc_host_json_merge(nest, objbuf, &mr) != 0) {
            printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                   "\"op\":\"mergeobj\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                   mr.err[0] ? mr.err : "merge fail", CUBALC_LANG_VERSION);
            return 1;
          }
        }
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_json_path_set(plate, key, mr.str, 1, &wr) != 0) {
          printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
                 "\"op\":\"%s\",\"err\":\"%s\",\"version\":\"%s\"}\n",
                 op, wr.err[0] ? wr.err : "set fail", CUBALC_LANG_VERSION);
          return 1;
        }
        snprintf(plate, sizeof plate, "%s", wr.str);
        /* report applied count in wr.n via mr.n after write */
        wr.n = mr.n;
      }

      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               op, CUBALC_LANG_VERSION);
        return 1;
      }
      if (strcmp(op, "setobj") == 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"setobj\",\"path\":\"%s\",\"key\":\"%s\",\"bytes\":%ld,"
               "\"file\":%s,\"version\":\"%s\",\"plate\":%s,"
               "\"note\":\"SETOBJ dual · paths ok\"}\n",
               path, key, hr.n, file_hit ? "true" : "false",
               CUBALC_LANG_VERSION, plate);
      } else {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"%s\",\"path\":\"%s\",\"key\":\"%s\",\"n\":%ld,"
               "\"bytes\":%ld,\"file\":%s,\"version\":\"%s\",\"plate\":%s,"
               "\"note\":\"MERGEOBJ/DEFAULTOBJ dual · paths ok\"}\n",
               op, path, key, wr.n, hr.n, file_hit ? "true" : "false",
               CUBALC_LANG_VERSION, plate);
      }
      return 0;
    }

    /* type|kind path key — TYPEP dual: field kind probe (paths ok).
     * Usability: choose num/str/obj peel without a .cubalc program:
     *   cubalc plate type agent.json freq.error  → num
     *   cubalc plate kind agent.json meta        → obj
     * Soft miss → kind=missing · hit=false · exit 1. */
    if (strcmp(op, "type") == 0) {
      const char *kind_s = "missing";
      long kind_n = 0;
      int hit = 0;
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get_raw(plate, key, &gr) == 0) {
        const char *v = gr.str;
        hit = 1;
        while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
        if (*v == '"') {
          kind_s = "str";
          kind_n = 2;
        } else if (*v == '{') {
          kind_s = "obj";
          kind_n = 5;
        } else if (*v == '[') {
          kind_s = "arr";
          kind_n = 6;
        } else if (strncmp(v, "true", 4) == 0 || strncmp(v, "false", 5) == 0) {
          kind_s = "bool";
          kind_n = 3;
        } else if (strncmp(v, "null", 4) == 0) {
          kind_s = "null";
          kind_n = 4;
        } else if (*v == '-' || (*v >= '0' && *v <= '9')) {
          kind_s = "num";
          kind_n = 1;
        } else {
          kind_s = "str";
          kind_n = 2;
        }
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"type\",\"path\":\"%s\",\"key\":\"%s\",\"hit\":%s,"
             "\"kind\":\"%s\",\"n\":%ld,\"file\":%s,\"version\":\"%s\","
             "\"note\":\"TYPEP dual · missing|num|str|bool|null|obj|arr · paths ok\"}\n",
             path, key, hit ? "true" : "false", kind_s, kind_n,
             file_hit ? "true" : "false", CUBALC_LANG_VERSION);
      return hit ? 0 : 1;
    }

    if (strcmp(op, "set") == 0) {
      size_t i;
      int allnum = 1;
      if (ai >= argc) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"set\",\"err\":\"need value\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 2;
      }
      val = argv[ai++];
      /* raw if integer / true / false / null / starts with { or [ */
      if (!strcmp(val, "true") || !strcmp(val, "false") || !strcmp(val, "null") ||
          val[0] == '{' || val[0] == '[')
        val_kind = 1;
      else {
        if (val[0] == '-' && val[1]) i = 1;
        else i = 0;
        if (!val[i]) allnum = 0;
        for (; val[i]; i++) {
          if (val[i] < '0' || val[i] > '9') {
            allnum = 0;
            break;
          }
        }
        if (allnum) val_kind = 1;
      }
      memset(&wr, 0, sizeof wr);
      /* dotted/slash path: plate set agent.json freq.error 9 */
      if (cubalc_host_json_path_set(plate, key, val, val_kind, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"set\",\"path\":\"%s\",\"key\":\"%s\",\"err\":\"%s\","
               "\"version\":\"%s\"}\n",
               path, key, wr.err[0] ? wr.err : "json set fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", wr.str);
      /* mkdir parent + write */
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"set\",\"path\":\"%s\",\"key\":\"%s\",\"err\":\"%s\","
               "\"version\":\"%s\"}\n",
               path, key, hr.err[0] ? hr.err : "write fail", CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"set\",\"path\":\"%s\",\"key\":\"%s\",\"updated\":%ld,"
             "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, wr.n, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* default path key value — DEFAULTP dual: set-if-missing (paths ok).
     *   cubalc plate default agent.json cfg.port 8080
     * kept=true when key already present (no clobber). */
    if (strcmp(op, "default") == 0) {
      size_t i;
      int allnum = 1, present = 0;
      cubalc_host_result hasr;
      if (ai >= argc) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"default\",\"err\":\"need value\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 2;
      }
      val = argv[ai++];
      memset(&hasr, 0, sizeof hasr);
      present = (cubalc_host_json_path_has(plate, key, &hasr) == 0 && hasr.n) ? 1 : 0;
      if (present) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
               "\"op\":\"default\",\"path\":\"%s\",\"key\":\"%s\",\"kept\":true,"
               "\"inserted\":false,\"file\":%s,\"version\":\"%s\",\"plate\":%s}\n",
               path, key, file_hit ? "true" : "false", CUBALC_LANG_VERSION, plate);
        return 0;
      }
      if (!strcmp(val, "true") || !strcmp(val, "false") || !strcmp(val, "null") ||
          val[0] == '{' || val[0] == '[')
        val_kind = 1;
      else {
        if (val[0] == '-' && val[1]) i = 1;
        else i = 0;
        if (!val[i]) allnum = 0;
        for (; val[i]; i++) {
          if (val[i] < '0' || val[i] > '9') {
            allnum = 0;
            break;
          }
        }
        if (allnum) val_kind = 1;
      }
      memset(&wr, 0, sizeof wr);
      if (cubalc_host_json_path_set(plate, key, val, val_kind, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"default\",\"path\":\"%s\",\"key\":\"%s\",\"err\":\"%s\","
               "\"version\":\"%s\"}\n",
               path, key, wr.err[0] ? wr.err : "json set fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", wr.str);
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"default\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"default\",\"path\":\"%s\",\"key\":\"%s\",\"kept\":false,"
             "\"inserted\":true,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    /* toggle path key — TOGGLEP dual: flip 0↔1 flag (paths ok; miss→1).
     *   cubalc plate toggle agent.json flags.debug */
    if (strcmp(op, "toggle") == 0) {
      long cur = 0, nv;
      char raw[32];
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
        char *end = NULL;
        if (!gr.str[0]) cur = 0;
        else {
          cur = strtol(gr.str, &end, 10);
          if (end == gr.str) cur = 1; /* non-numeric non-empty → truthy */
        }
      } else {
        cur = 0;
      }
      nv = cur ? 0 : 1;
      snprintf(raw, sizeof raw, "%ld", nv);
      memset(&wr, 0, sizeof wr);
      if (cubalc_host_json_path_set(plate, key, raw, 1, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"toggle\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               wr.err[0] ? wr.err : "json set fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", wr.str);
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"toggle\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"toggle\",\"path\":\"%s\",\"key\":\"%s\",\"value\":%ld,"
             "\"prev\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, nv, cur ? 1L : 0L, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    if (strcmp(op, "inc") == 0) {
      long cur = 0, nv;
      char raw[32];
      if (ai < argc) {
        char *end = NULL;
        delta = strtol(argv[ai], &end, 10);
        if (end == argv[ai]) delta = 1;
        ai++;
      }
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get(plate, key, &gr) == 0) {
        char *end = NULL;
        cur = strtol(gr.str, &end, 10);
        if (end == gr.str) cur = 0;
      }
      nv = cur + delta;
      snprintf(raw, sizeof raw, "%ld", nv);
      memset(&wr, 0, sizeof wr);
      if (cubalc_host_json_path_set(plate, key, raw, 1, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"inc\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               wr.err[0] ? wr.err : "json set fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", wr.str);
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"inc\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"inc\",\"path\":\"%s\",\"key\":\"%s\",\"value\":%ld,"
             "\"delta\":%ld,\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, nv, delta, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    if (strcmp(op, "del") == 0) {
      memset(&wr, 0, sizeof wr);
      if (cubalc_host_json_path_del(plate, key, &wr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"del\",\"err\":\"%s\",\"version\":\"%s\"}\n",
               wr.err[0] ? wr.err : "json del fail", CUBALC_LANG_VERSION);
        return 1;
      }
      snprintf(plate, sizeof plate, "%s", wr.str);
      slash = strrchr(path, '/');
      if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof parent) n = sizeof parent - 1;
        memcpy(parent, path, n);
        parent[n] = 0;
        memset(&hr, 0, sizeof hr);
        cubalc_host_mkdir(parent, &hr);
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(path, plate, &hr) != 0) {
        printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
               "\"op\":\"del\",\"err\":\"write fail\",\"version\":\"%s\"}\n",
               CUBALC_LANG_VERSION);
        return 1;
      }
      printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":true,\"cmd\":\"plate\","
             "\"op\":\"del\",\"path\":\"%s\",\"key\":\"%s\",\"removed\":%ld,"
             "\"bytes\":%ld,\"version\":\"%s\",\"plate\":%s}\n",
             path, key, wr.n, hr.n, CUBALC_LANG_VERSION, plate);
      return 0;
    }

    printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
           "\"err\":\"unknown op\",\"op\":\"%s\",\"version\":\"%s\"}\n",
           op, CUBALC_LANG_VERSION);
    return 2;
  }

  return 2; /* not plate family */
}
