/* CubalC — CLI plate command plane (peeled from cubalc_main). */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_plate.h"
#include "cubalc_cmd_plate_flat.h"
#include "cubalc_cmd_plate_nest.h"
#include "cubalc_cmd_plate_mut.h"
#include "cubalc_cmd_plate_sync.h"
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

    /* flat/leaf plane → cubalc_cmd_plate_flat.c (B_cmd_plate_peel) */
    {
      int fr = cubalc_cmd_plate_try_flat(op, path, plate, &file_hit, argc, argv, &ai);
      if (fr >= 0) return fr;
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

    /* nest plane → cubalc_cmd_plate_nest.c (B_cmd_plate_nest) */
    {
      int nr = cubalc_cmd_plate_try_nest(op, path, plate, &file_hit, argc, argv, &ai);
      if (nr >= 0) return nr;
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

    /* sync fill/merge/eq/has/pluck → cubalc_cmd_plate_sync.c (B_cmd_plate_sync) */
    {
      int sr = cubalc_cmd_plate_try_sync(op, path, plate, &file_hit, argc, argv, &ai);
      if (sr >= 0) return sr;
    }


    /* mut core get/set/… → cubalc_cmd_plate_mut.c (B_cmd_plate_core) */
    {
      int mr = cubalc_cmd_plate_try_mut(op, path, plate, &file_hit, argc, argv, &ai);
      if (mr >= 0) return mr;
    }

    printf("{\"schema\":\"cubalc.plate.v1\",\"ok\":false,\"cmd\":\"plate\","
           "\"err\":\"unknown op\",\"op\":\"%s\",\"version\":\"%s\"}\n",
           op, CUBALC_LANG_VERSION);
    return 2;
  }

  return 2; /* not plate family */
}
