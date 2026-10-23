/* CubalC — CLI plate command plane (peeled from cubalc_main). */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_plate.h"
#include "cubalc_cmd_plate_flat.h"
#include "cubalc_cmd_plate_nest.h"
#include "cubalc_cmd_plate_mut.h"
#include "cubalc_cmd_plate_sync.h"
#include "cubalc_cmd_plate_view.h"
#include "cubalc_cmd_plate_alias.h"
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
    if (cubalc_cmd_plate_is_op(argv[2])) {
      op = cubalc_cmd_plate_canon_op(argv[2]);
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

    /* view show/keys/pick/sum → cubalc_cmd_plate_view.c (B_cmd_plate_view) */
    {
      int vr = cubalc_cmd_plate_try_view(op, path, plate, &file_hit, argc, argv, &ai);
      if (vr >= 0) return vr;
    }

    /* flat/leaf plane → cubalc_cmd_plate_flat.c (B_cmd_plate_peel) */
    {
      int fr = cubalc_cmd_plate_try_flat(op, path, plate, &file_hit, argc, argv, &ai);
      if (fr >= 0) return fr;
    }



    /* nest plane → cubalc_cmd_plate_nest.c (B_cmd_plate_nest) */
    {
      int nr = cubalc_cmd_plate_try_nest(op, path, plate, &file_hit, argc, argv, &ai);
      if (nr >= 0) return nr;
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
