/* CubalC — CLI plate op alias normalize (B_cmd_plate_husk).
 * Known-token gate + alias → canonical op. Cube is SoT. Zero bloat.
 */
#include "cubalc_cmd_plate_alias.h"
#include <string.h>

int cubalc_cmd_plate_is_op(const char *s) {
  if (!s || !s[0]) return 0;
  if (!strcmp(s, "show") || !strcmp(s, "dump") || !strcmp(s, "cat") || !strcmp(s, "read") ||
      !strcmp(s, "get") || !strcmp(s, "peek") || !strcmp(s, "getn") || !strcmp(s, "num") ||
      !strcmp(s, "getpn") || !strcmp(s, "jgetn") || !strcmp(s, "int") || !strcmp(s, "getobj") ||
      !strcmp(s, "peekobj") || !strcmp(s, "objget") || !strcmp(s, "setobj") || !strcmp(s, "putobj") ||
      !strcmp(s, "nestp") || !strcmp(s, "putnest") || !strcmp(s, "mergeobj") || !strcmp(s, "patchnest") ||
      !strcmp(s, "updateobj") || !strcmp(s, "defaultobj") || !strcmp(s, "ensureobj") || !strcmp(s, "defaultnest") ||
      !strcmp(s, "type") || !strcmp(s, "kind") || !strcmp(s, "typeof") || !strcmp(s, "typep") ||
      !strcmp(s, "jtype") || !strcmp(s, "set") || !strcmp(s, "put") || !strcmp(s, "default") ||
      !strcmp(s, "ensurekey") || !strcmp(s, "defaultp") || !strcmp(s, "setdefault") || !strcmp(s, "toggle") ||
      !strcmp(s, "flip") || !strcmp(s, "togglep") || !strcmp(s, "flipp") || !strcmp(s, "rename") ||
      !strcmp(s, "move") || !strcmp(s, "mv") || !strcmp(s, "renamep") || !strcmp(s, "movekey") ||
      !strcmp(s, "rekey") || !strcmp(s, "copy") || !strcmp(s, "cp") || !strcmp(s, "dup") ||
      !strcmp(s, "copyp") || !strcmp(s, "dupkey") || !strcmp(s, "swap") || !strcmp(s, "xchg") ||
      !strcmp(s, "swapp") || !strcmp(s, "exchange") || !strcmp(s, "inc") || !strcmp(s, "bump") ||
      !strcmp(s, "del") || !strcmp(s, "rm") || !strcmp(s, "drop") || !strcmp(s, "keys") ||
      !strcmp(s, "list") || !strcmp(s, "ls") || !strcmp(s, "leaves") || !strcmp(s, "leafkeys") ||
      !strcmp(s, "pathkeys") || !strcmp(s, "dotpaths") || !strcmp(s, "flatpaths") || !strcmp(s, "dotkeys") ||
      !strcmp(s, "flat") || !strcmp(s, "flatkv") || !strcmp(s, "leafkv") || !strcmp(s, "flatten") ||
      !strcmp(s, "flattenp") || !strcmp(s, "tokvpath") || !strcmp(s, "unflat") || !strcmp(s, "unflatkv") ||
      !strcmp(s, "fromflat") || !strcmp(s, "unflatten") || !strcmp(s, "unflattenp") || !strcmp(s, "pathfromkv") ||
      !strcmp(s, "diffflat") || !strcmp(s, "clogflat") || !strcmp(s, "changelogflat") || !strcmp(s, "leafdiff") ||
      !strcmp(s, "pathdiff") || !strcmp(s, "diffpath") || !strcmp(s, "diffpaths") || !strcmp(s, "grepf") ||
      !strcmp(s, "grepflat") || !strcmp(s, "keepflat") || !strcmp(s, "filterflat") || !strcmp(s, "grepvf") ||
      !strcmp(s, "grepvflat") || !strcmp(s, "dropflat") || !strcmp(s, "grepfi") || !strcmp(s, "grepflati") ||
      !strcmp(s, "prune") || !strcmp(s, "pruneflat") || !strcmp(s, "delflat") || !strcmp(s, "scrubflat") ||
      !strcmp(s, "keeponly") || !strcmp(s, "keeponlyflat") || !strcmp(s, "retainflat") || !strcmp(s, "projectflat") ||
      !strcmp(s, "mergeflat") || !strcmp(s, "overlayflat") || !strcmp(s, "patchflat") || !strcmp(s, "deepmerge") ||
      !strcmp(s, "leafmerge") || !strcmp(s, "syncflat") || !strcmp(s, "renameflat") || !strcmp(s, "moveflat") ||
      !strcmp(s, "prefixflat") || !strcmp(s, "repathflat") || !strcmp(s, "rekeyflat") || !strcmp(s, "setflat") ||
      !strcmp(s, "mapflat") || !strcmp(s, "setleaf") || !strcmp(s, "putflat") || !strcmp(s, "bulkset") ||
      !strcmp(s, "incflat") || !strcmp(s, "bumpflat") || !strcmp(s, "addflat") || !strcmp(s, "tickflat") ||
      !strcmp(s, "bulkinc") || !strcmp(s, "decflat") || !strcmp(s, "sumflat") || !strcmp(s, "totalflat") ||
      !strcmp(s, "leafsum") || !strcmp(s, "flattotal") || !strcmp(s, "avgflat") || !strcmp(s, "meanflat") ||
      !strcmp(s, "avgleaf") || !strcmp(s, "flatmean") || !strcmp(s, "leafmean") || !strcmp(s, "medianflat") ||
      !strcmp(s, "p50flat") || !strcmp(s, "midflat") || !strcmp(s, "flatmedian") || !strcmp(s, "leafmedian") ||
      !strcmp(s, "toppath") || !strcmp(s, "toppathflat") || !strcmp(s, "maxpath") || !strcmp(s, "maxpathflat") ||
      !strcmp(s, "botpath") || !strcmp(s, "botpathflat") || !strcmp(s, "minpath") || !strcmp(s, "minpathflat") ||
      !strcmp(s, "threshflat") || !strcmp(s, "keepvflat") || !strcmp(s, "mincountflat") || !strcmp(s, "dropzeroflat") ||
      !strcmp(s, "nzflat") || !strcmp(s, "capflat") || !strcmp(s, "clampflat") || !strcmp(s, "maxvflat") ||
      !strcmp(s, "ceilflat") || !strcmp(s, "scaleflat") || !strcmp(s, "mulflat") || !strcmp(s, "timesflat") ||
      !strcmp(s, "multflat") || !strcmp(s, "hasflat") || !strcmp(s, "anyflat") || !strcmp(s, "countflat") ||
      !strcmp(s, "nflat") || !strcmp(s, "leafcount") || !strcmp(s, "hasflatn") || !strcmp(s, "anyflatn") ||
      !strcmp(s, "countflatn") || !strcmp(s, "nflatn") || !strcmp(s, "leafcountn") || !strcmp(s, "hitflatn") ||
      !strcmp(s, "pathsflat") || !strcmp(s, "matchpaths") || !strcmp(s, "valsflat") || !strcmp(s, "matchvals") ||
      !strcmp(s, "leafvals") || !strcmp(s, "pathsflatn") || !strcmp(s, "matchpathsn") || !strcmp(s, "valsflatn") ||
      !strcmp(s, "matchvalsn") || !strcmp(s, "leafvalsn") || !strcmp(s, "numvalsflat") || !strcmp(s, "getflat") ||
      !strcmp(s, "firstflat") || !strcmp(s, "peelflat") || !strcmp(s, "leafget") || !strcmp(s, "lastflat") ||
      !strcmp(s, "endflat") || !strcmp(s, "tailflat") || !strcmp(s, "lastleafp") || !strcmp(s, "nthflat") ||
      !strcmp(s, "indexflat") || !strcmp(s, "atflat") || !strcmp(s, "nthleaf") || !strcmp(s, "needflat") ||
      !strcmp(s, "requireflat") || !strcmp(s, "mustflat") || !strcmp(s, "getflatn") || !strcmp(s, "firstflatn") ||
      !strcmp(s, "numflat") || !strcmp(s, "getnflat") || !strcmp(s, "lastflatn") || !strcmp(s, "endflatn") ||
      !strcmp(s, "tailflatn") || !strcmp(s, "lastnumflat") || !strcmp(s, "nthflatn") || !strcmp(s, "indexflatn") ||
      !strcmp(s, "atflatn") || !strcmp(s, "nthnumflat") || !strcmp(s, "typeflat") || !strcmp(s, "kindflat") ||
      !strcmp(s, "leaftype") || !strcmp(s, "flattype") || !strcmp(s, "freqflat") || !strcmp(s, "histflat") ||
      !strcmp(s, "leaffreq") || !strcmp(s, "countvalflat") || !strcmp(s, "modeflat") || !strcmp(s, "topvalflat") ||
      !strcmp(s, "majorflat") || !strcmp(s, "leafmode") || !strcmp(s, "pathbyval") || !strcmp(s, "valpath") ||
      !strcmp(s, "findval") || !strcmp(s, "pathofval") || !strcmp(s, "uniqflat") || !strcmp(s, "distinctflat") ||
      !strcmp(s, "uniqueflat") || !strcmp(s, "leafuniq") || !strcmp(s, "pathsbyval") || !strcmp(s, "allpathsbyval") ||
      !strcmp(s, "findvals") || !strcmp(s, "pathsofval") || !strcmp(s, "countbyval") || !strcmp(s, "nbyval") ||
      !strcmp(s, "valcount") || !strcmp(s, "nval") || !strcmp(s, "hasval") || !strcmp(s, "hasvalflat") ||
      !strcmp(s, "anyval") || !strcmp(s, "valhas") || !strcmp(s, "setbyval") || !strcmp(s, "replaceval") ||
      !strcmp(s, "rewval") || !strcmp(s, "mapval") || !strcmp(s, "delbyval") || !strcmp(s, "dropval") ||
      !strcmp(s, "rmval") || !strcmp(s, "pruneval") || !strcmp(s, "alleqflat") || !strcmp(s, "samevalflat") ||
      !strcmp(s, "eqvalsflat") || !strcmp(s, "allsameflat") || !strcmp(s, "firstuneq") || !strcmp(s, "firstuneqflat") ||
      !strcmp(s, "pathuneq") || !strcmp(s, "diverge1") || !strcmp(s, "uneqpaths") || !strcmp(s, "divergepaths") ||
      !strcmp(s, "alluneq") || !strcmp(s, "pathsuneq") || !strcmp(s, "uniform") || !strcmp(s, "uniformflat") ||
      !strcmp(s, "checkflat") || !strcmp(s, "samecheck") || !strcmp(s, "pretty") || !strcmp(s, "prettyp") ||
      !strcmp(s, "jsonpretty") || !strcmp(s, "prettyjson") || !strcmp(s, "needflatn") || !strcmp(s, "requireflatn") ||
      !strcmp(s, "mustflatn") || !strcmp(s, "needintflat") || !strcmp(s, "len") || !strcmp(s, "length") ||
      !strcmp(s, "nkeys") || !strcmp(s, "size") || !strcmp(s, "countkeys") || !strcmp(s, "empty") ||
      !strcmp(s, "isempty") || !strcmp(s, "emptyp") || !strcmp(s, "vals") || !strcmp(s, "values") ||
      !strcmp(s, "valsp") || !strcmp(s, "fill") || !strcmp(s, "expand") || !strcmp(s, "template") ||
      !strcmp(s, "render") || !strcmp(s, "fillkeys") || !strcmp(s, "fill-keys") || !strcmp(s, "tmplkeys") ||
      !strcmp(s, "ensure") || !strcmp(s, "seed") || !strcmp(s, "touch") || !strcmp(s, "create") ||
      !strcmp(s, "merge") || !strcmp(s, "patch") || !strcmp(s, "overlay") || !strcmp(s, "apply") ||
      !strcmp(s, "eq") || !strcmp(s, "same") || !strcmp(s, "equal") || !strcmp(s, "cmp") ||
      !strcmp(s, "ne") || !strcmp(s, "neq") || !strcmp(s, "differ") || !strcmp(s, "unequal") ||
      !strcmp(s, "diff") || !strcmp(s, "changed") || !strcmp(s, "delta") || !strcmp(s, "changelog") ||
      !strcmp(s, "clog") || !strcmp(s, "changes") || !strcmp(s, "logdiff") || !strcmp(s, "has") ||
      !strcmp(s, "hasall") || !strcmp(s, "has-keys") || !strcmp(s, "check") || !strcmp(s, "need") ||
      !strcmp(s, "require") || !strcmp(s, "needkeys") || !strcmp(s, "require-keys") || !strcmp(s, "pluck") ||
      !strcmp(s, "getall") || !strcmp(s, "getpall") || !strcmp(s, "peel") || !strcmp(s, "values") ||
      !strcmp(s, "pluckp") || !strcmp(s, "pluckobj") || !strcmp(s, "nestpluck") || !strcmp(s, "objpluck") ||
      !strcmp(s, "plucknest") || !strcmp(s, "pick") || !strcmp(s, "keep") || !strcmp(s, "select") ||
      !strcmp(s, "project") || !strcmp(s, "omit") || !strcmp(s, "strip") || !strcmp(s, "dropkeys") ||
      !strcmp(s, "drop-keys") || !strcmp(s, "sum") || !strcmp(s, "total") || !strcmp(s, "avg") ||
      !strcmp(s, "mean") || !strcmp(s, "average") || !strcmp(s, "median") || !strcmp(s, "p50") ||
      !strcmp(s, "mid") || !strcmp(s, "top") || !strcmp(s, "topn") || !strcmp(s, "head") ||
      !strcmp(s, "bot") || !strcmp(s, "bottom") || !strcmp(s, "botn") || !strcmp(s, "tail") ||
      !strcmp(s, "nestget") || !strcmp(s, "nget") || !strcmp(s, "getnest") || !strcmp(s, "getpobj") ||
      !strcmp(s, "nestset") || !strcmp(s, "nset") || !strcmp(s, "setnest") || !strcmp(s, "setpobj") ||
      !strcmp(s, "nestinc") || !strcmp(s, "ninc") || !strcmp(s, "incnest") || !strcmp(s, "incobj") ||
      !strcmp(s, "nestdel") || !strcmp(s, "ndel") || !strcmp(s, "delnest") || !strcmp(s, "delpobj") ||
      !strcmp(s, "nestkeys") || !strcmp(s, "nkeys") || !strcmp(s, "keysobj") || !strcmp(s, "keysnest") ||
      !strcmp(s, "nesthas") || !strcmp(s, "nhas") || !strcmp(s, "haspobj") || !strcmp(s, "hasnest") ||
      !strcmp(s, "nestpick") || !strcmp(s, "npick") || !strcmp(s, "picknest") || !strcmp(s, "pickobj") ||
      !strcmp(s, "nestomit") || !strcmp(s, "nomit") || !strcmp(s, "omitnest") || !strcmp(s, "omitobj") ||
      !strcmp(s, "stripnest") || !strcmp(s, "nestrename") || !strcmp(s, "nrename") || !strcmp(s, "renamenest") ||
      !strcmp(s, "renamepobj") || !strcmp(s, "nestcopy") || !strcmp(s, "ncopy") || !strcmp(s, "copynest") ||
      !strcmp(s, "copypobj") || !strcmp(s, "nestswap") || !strcmp(s, "nswap") || !strcmp(s, "swapnest") ||
      !strcmp(s, "swappobj") || !strcmp(s, "nestsum") || !strcmp(s, "nsum") || !strcmp(s, "sumnest") ||
      !strcmp(s, "sumnobj") || !strcmp(s, "nestavg") || !strcmp(s, "navg") || !strcmp(s, "avgnest") ||
      !strcmp(s, "nestmean") || !strcmp(s, "nestmedian") || !strcmp(s, "nmedian") || !strcmp(s, "nestp50") ||
      !strcmp(s, "mediannest") || !strcmp(s, "nesttop") || !strcmp(s, "ntop") || !strcmp(s, "topnest") ||
      !strcmp(s, "topnobj") || !strcmp(s, "nestbot") || !strcmp(s, "nbot") || !strcmp(s, "botnest") ||
      !strcmp(s, "botnobj") || !strcmp(s, "nestbottom") || !strcmp(s, "nestsort") || !strcmp(s, "nsort") ||
      !strcmp(s, "sortnest") || !strcmp(s, "sortobj") || !strcmp(s, "nestsortbag") || !strcmp(s, "nsortbag") ||
      !strcmp(s, "sortbagnest") || !strcmp(s, "sortbagobj") || !strcmp(s, "nestsortfreq") || !strcmp(s, "sortfreqobj"))
    return 1;
  return 0;
}

const char *cubalc_cmd_plate_canon_op(const char *s) {
  if (!s) return NULL;
  /* alias → canonical; unknown returns s unchanged if is_op, else caller gates */
      if (strcmp(s, "dump") == 0 || strcmp(s, "cat") == 0 || strcmp(s, "read") == 0)
        return "show";
      else if (strcmp(s, "peek") == 0)
        return "get";
      else if (strcmp(s, "num") == 0 || strcmp(s, "getpn") == 0 ||
               strcmp(s, "jgetn") == 0 || strcmp(s, "int") == 0)
        return "getn";
      else if (strcmp(s, "peekobj") == 0 || strcmp(s, "objget") == 0)
        return "getobj";
      else if (strcmp(s, "putobj") == 0 || strcmp(s, "nestp") == 0 ||
               strcmp(s, "putnest") == 0)
        return "setobj";
      else if (strcmp(s, "patchnest") == 0 || strcmp(s, "updateobj") == 0)
        return "mergeobj";
      else if (strcmp(s, "ensureobj") == 0 || strcmp(s, "defaultnest") == 0)
        return "defaultobj";
      else if (strcmp(s, "kind") == 0 || strcmp(s, "typeof") == 0 ||
               strcmp(s, "typep") == 0 || strcmp(s, "jtype") == 0)
        return "type";
      else if (strcmp(s, "put") == 0)
        return "set";
      else if (strcmp(s, "ensurekey") == 0 || strcmp(s, "defaultp") == 0 ||
               strcmp(s, "setdefault") == 0)
        return "default";
      else if (strcmp(s, "flip") == 0 || strcmp(s, "togglep") == 0 ||
               strcmp(s, "flipp") == 0)
        return "toggle";
      else if (strcmp(s, "move") == 0 || strcmp(s, "mv") == 0 ||
               strcmp(s, "renamep") == 0 || strcmp(s, "movekey") == 0 ||
               strcmp(s, "rekey") == 0)
        return "rename";
      else if (strcmp(s, "cp") == 0 || strcmp(s, "dup") == 0 ||
               strcmp(s, "copyp") == 0 || strcmp(s, "dupkey") == 0)
        return "copy";
      else if (strcmp(s, "xchg") == 0 || strcmp(s, "swapp") == 0 ||
               strcmp(s, "exchange") == 0)
        return "swap";
      else if (strcmp(s, "bump") == 0)
        return "inc";
      else if (strcmp(s, "rm") == 0 || strcmp(s, "drop") == 0)
        return "del";
      else if (strcmp(s, "list") == 0 || strcmp(s, "ls") == 0)
        return "keys";
      else if (strcmp(s, "leafkeys") == 0 || strcmp(s, "pathkeys") == 0 ||
               strcmp(s, "dotpaths") == 0 || strcmp(s, "flatpaths") == 0 ||
               strcmp(s, "dotkeys") == 0 || strcmp(s, "leafpaths") == 0)
        return "leaves";
      else if (strcmp(s, "flatkv") == 0 || strcmp(s, "leafkv") == 0 ||
               strcmp(s, "flatten") == 0 || strcmp(s, "flattenp") == 0 ||
               strcmp(s, "tokvpath") == 0 || strcmp(s, "pathtokv") == 0 ||
               strcmp(s, "plateflat") == 0)
        return "flat";
      else if (strcmp(s, "unflatkv") == 0 || strcmp(s, "fromflat") == 0 ||
               strcmp(s, "unflatten") == 0 || strcmp(s, "unflattenp") == 0 ||
               strcmp(s, "pathfromkv") == 0 || strcmp(s, "applyflat") == 0)
        return "unflat";
      else if (strcmp(s, "clogflat") == 0 || strcmp(s, "changelogflat") == 0 ||
               strcmp(s, "leafdiff") == 0 || strcmp(s, "flatdiff") == 0)
        return "diffflat";
      else if (strcmp(s, "diffpath") == 0 || strcmp(s, "diffpaths") == 0 ||
               strcmp(s, "changedpaths") == 0)
        return "pathdiff";
      else if (strcmp(s, "grepflat") == 0 || strcmp(s, "keepflat") == 0 ||
               strcmp(s, "filterflat") == 0 || strcmp(s, "leafgrep") == 0)
        return "grepf";
      else if (strcmp(s, "grepvflat") == 0 || strcmp(s, "dropflat") == 0 ||
               strcmp(s, "vgrepflat") == 0)
        return "grepvf";
      else if (strcmp(s, "grepflati") == 0 || strcmp(s, "igrepflat") == 0 ||
               strcmp(s, "grepiflat") == 0)
        return "grepfi";
      else if (strcmp(s, "pruneflat") == 0 || strcmp(s, "delflat") == 0 ||
               strcmp(s, "scrubflat") == 0 || strcmp(s, "rmflat") == 0)
        return "prune";
      else if (strcmp(s, "keeponlyflat") == 0 || strcmp(s, "retainflat") == 0 ||
               strcmp(s, "projectflat") == 0 || strcmp(s, "retain") == 0)
        return "keeponly";
      else if (strcmp(s, "overlayflat") == 0 || strcmp(s, "patchflat") == 0 ||
               strcmp(s, "deepmerge") == 0 || strcmp(s, "leafmerge") == 0 ||
               strcmp(s, "syncflat") == 0 || strcmp(s, "applyflat") == 0)
        return "mergeflat";
      else if (strcmp(s, "moveflat") == 0 || strcmp(s, "prefixflat") == 0 ||
               strcmp(s, "repathflat") == 0 || strcmp(s, "rekeyflat") == 0 ||
               strcmp(s, "mvflat") == 0)
        return "renameflat";
      else if (strcmp(s, "mapflat") == 0 || strcmp(s, "setleaf") == 0 ||
               strcmp(s, "putflat") == 0 || strcmp(s, "bulkset") == 0 ||
               strcmp(s, "assignflat") == 0)
        return "setflat";
      else if (strcmp(s, "bumpflat") == 0 || strcmp(s, "addflat") == 0 ||
               strcmp(s, "tickflat") == 0 || strcmp(s, "bulkinc") == 0)
        return "incflat";
      else if (strcmp(s, "decflat") == 0)
        return "decflat";
      else if (strcmp(s, "totalflat") == 0 || strcmp(s, "leafsum") == 0 ||
               strcmp(s, "flattotal") == 0 || strcmp(s, "sumleaf") == 0)
        return "sumflat";
      else if (strcmp(s, "meanflat") == 0 || strcmp(s, "avgleaf") == 0 ||
               strcmp(s, "flatmean") == 0 || strcmp(s, "leafmean") == 0 ||
               strcmp(s, "avgallflat") == 0)
        return "avgflat";
      else if (strcmp(s, "p50flat") == 0 || strcmp(s, "midflat") == 0 ||
               strcmp(s, "flatmedian") == 0 || strcmp(s, "leafmedian") == 0 ||
               strcmp(s, "medallflat") == 0)
        return "medianflat";
      else if (strcmp(s, "toppathflat") == 0 || strcmp(s, "maxpath") == 0 ||
               strcmp(s, "maxpathflat") == 0 || strcmp(s, "topflat") == 0)
        return "toppath";
      else if (strcmp(s, "botpathflat") == 0 || strcmp(s, "minpath") == 0 ||
               strcmp(s, "minpathflat") == 0 || strcmp(s, "botflat") == 0)
        return "botpath";
      else if (strcmp(s, "keepvflat") == 0 || strcmp(s, "mincountflat") == 0 ||
               strcmp(s, "keepminflat") == 0)
        return "threshflat";
      else if (strcmp(s, "nzflat") == 0 || strcmp(s, "keepnzflat") == 0)
        return "dropzeroflat";
      else if (strcmp(s, "clampflat") == 0 || strcmp(s, "maxvflat") == 0 ||
               strcmp(s, "ceilflat") == 0 || strcmp(s, "maxcapflat") == 0)
        return "capflat";
      else if (strcmp(s, "mulflat") == 0 || strcmp(s, "timesflat") == 0 ||
               strcmp(s, "multflat") == 0 || strcmp(s, "factorflat") == 0)
        return "scaleflat";
      else if (strcmp(s, "anyflat") == 0 || strcmp(s, "hitflat") == 0)
        return "hasflat";
      else if (strcmp(s, "nflat") == 0 || strcmp(s, "leafcount") == 0 ||
               strcmp(s, "ncflat") == 0)
        return "countflat";
      else if (strcmp(s, "anyflatn") == 0 || strcmp(s, "hitflatn") == 0 ||
               strcmp(s, "anynumflat") == 0)
        return "hasflatn";
      else if (strcmp(s, "nflatn") == 0 || strcmp(s, "leafcountn") == 0 ||
               strcmp(s, "ncflatn") == 0 || strcmp(s, "numcountflat") == 0)
        return "countflatn";
      else if (strcmp(s, "matchpaths") == 0 || strcmp(s, "pathmatch") == 0)
        return "pathsflat";
      else if (strcmp(s, "matchvals") == 0 || strcmp(s, "leafvals") == 0 ||
               strcmp(s, "valmatch") == 0)
        return "valsflat";
      else if (strcmp(s, "matchpathsn") == 0 || strcmp(s, "pathmatchn") == 0 ||
               strcmp(s, "numpathsflat") == 0)
        return "pathsflatn";
      else if (strcmp(s, "matchvalsn") == 0 || strcmp(s, "leafvalsn") == 0 ||
               strcmp(s, "numvalsflat") == 0 || strcmp(s, "intvalsflat") == 0)
        return "valsflatn";
      else if (strcmp(s, "firstflat") == 0 || strcmp(s, "peelflat") == 0 ||
               strcmp(s, "leafget") == 0 || strcmp(s, "getleaf") == 0)
        return "getflat";
      else if (strcmp(s, "endflat") == 0 || strcmp(s, "tailflat") == 0 ||
               strcmp(s, "lastleafp") == 0 || strcmp(s, "getleafl") == 0)
        return "lastflat";
      else if (strcmp(s, "indexflat") == 0 || strcmp(s, "atflat") == 0 ||
               strcmp(s, "nthleaf") == 0 || strcmp(s, "leafnth") == 0)
        return "nthflat";
      else if (strcmp(s, "requireflat") == 0 || strcmp(s, "mustflat") == 0 ||
               strcmp(s, "needleaves") == 0)
        return "needflat";
      else if (strcmp(s, "firstflatn") == 0 || strcmp(s, "numflat") == 0 ||
               strcmp(s, "getnflat") == 0 || strcmp(s, "ngetflat") == 0)
        return "getflatn";
      else if (strcmp(s, "endflatn") == 0 || strcmp(s, "tailflatn") == 0 ||
               strcmp(s, "lastnumflat") == 0 || strcmp(s, "getnflatl") == 0)
        return "lastflatn";
      else if (strcmp(s, "indexflatn") == 0 || strcmp(s, "atflatn") == 0 ||
               strcmp(s, "nthnumflat") == 0 || strcmp(s, "leafnthn") == 0)
        return "nthflatn";
      else if (strcmp(s, "kindflat") == 0 || strcmp(s, "leaftype") == 0 ||
               strcmp(s, "flattype") == 0 || strcmp(s, "typeleafp") == 0)
        return "typeflat";
      else if (strcmp(s, "histflat") == 0 || strcmp(s, "leaffreq") == 0 ||
               strcmp(s, "countvalflat") == 0 || strcmp(s, "valfreqflat") == 0 ||
               strcmp(s, "tallyflat") == 0)
        return "freqflat";
      else if (strcmp(s, "topvalflat") == 0 || strcmp(s, "majorflat") == 0 ||
               strcmp(s, "leafmode") == 0 || strcmp(s, "modalflat") == 0 ||
               strcmp(s, "dominantflat") == 0 || strcmp(s, "winvalflat") == 0)
        return "modeflat";
      else if (strcmp(s, "valpath") == 0 || strcmp(s, "findval") == 0 ||
               strcmp(s, "pathofval") == 0 || strcmp(s, "findpathval") == 0 ||
               strcmp(s, "whereval") == 0 || strcmp(s, "locateval") == 0)
        return "pathbyval";
      else if (strcmp(s, "distinctflat") == 0 || strcmp(s, "uniqueflat") == 0 ||
               strcmp(s, "leafuniq") == 0 || strcmp(s, "dedupflat") == 0 ||
               strcmp(s, "uniqvalsflat") == 0 || strcmp(s, "uniquevals") == 0)
        return "uniqflat";
      else if (strcmp(s, "allpathsbyval") == 0 || strcmp(s, "findvals") == 0 ||
               strcmp(s, "pathsofval") == 0 || strcmp(s, "findpathsval") == 0 ||
               strcmp(s, "wherevals") == 0 || strcmp(s, "locatevals") == 0)
        return "pathsbyval";
      else if (strcmp(s, "nbyval") == 0 || strcmp(s, "valcount") == 0 ||
               strcmp(s, "nval") == 0 || strcmp(s, "countval") == 0)
        return "countbyval";
      else if (strcmp(s, "hasvalflat") == 0 || strcmp(s, "anyval") == 0 ||
               strcmp(s, "valhas") == 0 || strcmp(s, "containsval") == 0)
        return "hasval";
      else if (strcmp(s, "replaceval") == 0 || strcmp(s, "rewval") == 0 ||
               strcmp(s, "mapval") == 0 || strcmp(s, "rewriteval") == 0 ||
               strcmp(s, "reval") == 0 || strcmp(s, "swapval") == 0)
        return "setbyval";
      else if (strcmp(s, "dropval") == 0 || strcmp(s, "rmval") == 0 ||
               strcmp(s, "pruneval") == 0 || strcmp(s, "deleteval") == 0 ||
               strcmp(s, "removeval") == 0 || strcmp(s, "stripval") == 0)
        return "delbyval";
      else if (strcmp(s, "samevalflat") == 0 || strcmp(s, "eqvalsflat") == 0 ||
               strcmp(s, "allsameflat") == 0 || strcmp(s, "sameflat") == 0 ||
               strcmp(s, "eqallflat") == 0 || strcmp(s, "uniformflat") == 0)
        return "alleqflat";
      else if (strcmp(s, "firstuneqflat") == 0 || strcmp(s, "pathuneq") == 0 ||
               strcmp(s, "diverge1") == 0 || strcmp(s, "firstdiverge") == 0 ||
               strcmp(s, "uneq1") == 0 || strcmp(s, "firstdiffval") == 0 ||
               strcmp(s, "pathdiffval") == 0 || strcmp(s, "finduneq") == 0)
        return "firstuneq";
      else if (strcmp(s, "divergepaths") == 0 || strcmp(s, "alluneq") == 0 ||
               strcmp(s, "pathsuneq") == 0 || strcmp(s, "divergeall") == 0 ||
               strcmp(s, "alldiverge") == 0 || strcmp(s, "uneqall") == 0 ||
               strcmp(s, "uneqpath") == 0 || strcmp(s, "divergepath") == 0)
        return "uneqpaths";
      else if (strcmp(s, "uniformflat") == 0 || strcmp(s, "checkflat") == 0 ||
               strcmp(s, "samecheck") == 0 || strcmp(s, "checkuniform") == 0 ||
               strcmp(s, "nestuniform") == 0 || strcmp(s, "flatuniform") == 0 ||
               strcmp(s, "uniformcheck") == 0 || strcmp(s, "eqcheck") == 0)
        return "uniform";
      else if (strcmp(s, "prettyp") == 0 || strcmp(s, "jsonpretty") == 0 ||
               strcmp(s, "prettyjson") == 0 || strcmp(s, "prettyplate") == 0 ||
               strcmp(s, "fmtjson") == 0 || strcmp(s, "indentjson") == 0 ||
               strcmp(s, "pp") == 0)
        return "pretty";
      else if (strcmp(s, "requireflatn") == 0 || strcmp(s, "mustflatn") == 0 ||
               strcmp(s, "needintflat") == 0 || strcmp(s, "neednumflat") == 0)
        return "needflatn";
      else if (strcmp(s, "length") == 0 || strcmp(s, "nkeys") == 0 ||
               strcmp(s, "size") == 0 || strcmp(s, "countkeys") == 0)
        return "len";
      else if (strcmp(s, "isempty") == 0 || strcmp(s, "emptyp") == 0)
        return "empty";
      else if (strcmp(s, "values") == 0 || strcmp(s, "valsp") == 0)
        return "vals";
      else if (strcmp(s, "expand") == 0 || strcmp(s, "template") == 0 ||
               strcmp(s, "render") == 0)
        return "fill";
      else if (strcmp(s, "fill-keys") == 0 || strcmp(s, "tmplkeys") == 0)
        return "fillkeys";
      else if (strcmp(s, "seed") == 0 || strcmp(s, "touch") == 0 ||
               strcmp(s, "create") == 0)
        return "ensure";
      else if (strcmp(s, "patch") == 0 || strcmp(s, "overlay") == 0 ||
               strcmp(s, "apply") == 0)
        return "merge";
      else if (strcmp(s, "same") == 0 || strcmp(s, "equal") == 0 ||
               strcmp(s, "cmp") == 0)
        return "eq";
      else if (strcmp(s, "neq") == 0 || strcmp(s, "differ") == 0 ||
               strcmp(s, "unequal") == 0)
        return "ne";
      else if (strcmp(s, "changed") == 0 || strcmp(s, "delta") == 0)
        return "diff";
      else if (strcmp(s, "clog") == 0 || strcmp(s, "changes") == 0 ||
               strcmp(s, "logdiff") == 0)
        return "changelog";
      else if (strcmp(s, "hasall") == 0 || strcmp(s, "has-keys") == 0 ||
               strcmp(s, "check") == 0)
        return "has";
      else if (strcmp(s, "require") == 0 || strcmp(s, "needkeys") == 0 ||
               strcmp(s, "require-keys") == 0)
        return "need";
      else if (strcmp(s, "getall") == 0 || strcmp(s, "getpall") == 0 ||
               strcmp(s, "peel") == 0 || strcmp(s, "values") == 0 ||
               strcmp(s, "pluckp") == 0)
        return "pluck";
      else if (strcmp(s, "nestpluck") == 0 || strcmp(s, "objpluck") == 0 ||
               strcmp(s, "plucknest") == 0)
        return "pluckobj";
      else if (strcmp(s, "keep") == 0 || strcmp(s, "select") == 0 ||
               strcmp(s, "project") == 0)
        return "pick";
      else if (strcmp(s, "strip") == 0 || strcmp(s, "dropkeys") == 0 ||
               strcmp(s, "drop-keys") == 0)
        return "omit";
      else if (strcmp(s, "total") == 0)
        return "sum";
      else if (strcmp(s, "mean") == 0 || strcmp(s, "average") == 0)
        return "avg";
      else if (strcmp(s, "p50") == 0 || strcmp(s, "mid") == 0)
        return "median";
      else if (strcmp(s, "topn") == 0 || strcmp(s, "head") == 0)
        return "top";
      else if (strcmp(s, "bottom") == 0 || strcmp(s, "botn") == 0 ||
               strcmp(s, "tail") == 0)
        return "bot";
      else if (strcmp(s, "nget") == 0 || strcmp(s, "getnest") == 0 ||
               strcmp(s, "getpobj") == 0)
        return "nestget";
      else if (strcmp(s, "nset") == 0 || strcmp(s, "setnest") == 0 ||
               strcmp(s, "setpobj") == 0)
        return "nestset";
      else if (strcmp(s, "ninc") == 0 || strcmp(s, "incnest") == 0 ||
               strcmp(s, "incobj") == 0)
        return "nestinc";
      else if (strcmp(s, "ndel") == 0 || strcmp(s, "delnest") == 0 ||
               strcmp(s, "delpobj") == 0)
        return "nestdel";
      else if (strcmp(s, "nkeys") == 0 || strcmp(s, "keysobj") == 0 ||
               strcmp(s, "keysnest") == 0)
        return "nestkeys";
      else if (strcmp(s, "nhas") == 0 || strcmp(s, "haspobj") == 0 ||
               strcmp(s, "hasnest") == 0)
        return "nesthas";
      else if (strcmp(s, "npick") == 0 || strcmp(s, "pickobj") == 0 ||
               strcmp(s, "picknest") == 0)
        return "nestpick";
      else if (strcmp(s, "nomit") == 0 || strcmp(s, "omitobj") == 0 ||
               strcmp(s, "omitnest") == 0 || strcmp(s, "stripnest") == 0)
        return "nestomit";
      else if (strcmp(s, "nrename") == 0 || strcmp(s, "renamenest") == 0 ||
               strcmp(s, "renamepobj") == 0)
        return "nestrename";
      else if (strcmp(s, "ncopy") == 0 || strcmp(s, "copynest") == 0 ||
               strcmp(s, "copypobj") == 0)
        return "nestcopy";
      else if (strcmp(s, "nswap") == 0 || strcmp(s, "swapnest") == 0 ||
               strcmp(s, "swappobj") == 0)
        return "nestswap";
      else if (strcmp(s, "nsum") == 0 || strcmp(s, "sumnest") == 0 ||
               strcmp(s, "sumnobj") == 0)
        return "nestsum";
      else if (strcmp(s, "navg") == 0 || strcmp(s, "avgnest") == 0 ||
               strcmp(s, "nestmean") == 0)
        return "nestavg";
      else if (strcmp(s, "nmedian") == 0 || strcmp(s, "nestp50") == 0 ||
               strcmp(s, "mediannest") == 0)
        return "nestmedian";
      else if (strcmp(s, "ntop") == 0 || strcmp(s, "topnest") == 0 ||
               strcmp(s, "topnobj") == 0)
        return "nesttop";
      else if (strcmp(s, "nbot") == 0 || strcmp(s, "botnest") == 0 ||
               strcmp(s, "botnobj") == 0 || strcmp(s, "nestbottom") == 0)
        return "nestbot";
      else if (strcmp(s, "nsort") == 0 || strcmp(s, "sortnest") == 0 ||
               strcmp(s, "sortobj") == 0)
        return "nestsort";
      else if (strcmp(s, "nsortbag") == 0 || strcmp(s, "sortbagnest") == 0 ||
               strcmp(s, "sortbagobj") == 0 || strcmp(s, "nestsortfreq") == 0 ||
               strcmp(s, "sortfreqobj") == 0)
        return "nestsortbag";
  return s;
}
