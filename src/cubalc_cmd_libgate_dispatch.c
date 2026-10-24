/* CubalC CLI libs|recipe|checkdeps|picklib|freshlibs|libage|flaggate|envform —
 * F_main_carve try-dispatch peel from cubalc_main.c.
 * Returns exit code if handled, -1 if not this family. Zero bloat. Cube is SoT.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_libgate_dispatch.h"
#include "cubalc_cmd_stdlib.h"
#include "cubalc_cmd_contracts.h"
#include "cubalc_cmd_vars.h"
#include "cubalc_cmd_examples.h"
#include "cubalc_cmd_cat.h"
#include "cubalc_cmd_recipe.h"
#include "cubalc_cmd_checkdeps.h"
#include "cubalc_cmd_libmatch.h"
#include "cubalc_cmd_freshlibs.h"
#include "cubalc_cmd_libage.h"
#include "cubalc_cmd_flaggate.h"
#include "cubalc_cmd_envform.h"
#include <string.h>

int cubalc_cmd_try_libgate_dispatch(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "";
  if (!cmd[0]) return -1;

  if (strcmp(cmd, "libs") == 0 || strcmp(cmd, "lib") == 0 ||
      strcmp(cmd, "stdlib") == 0)
    return cubalc_cmd_stdlib(argc, argv);

  if (strcmp(cmd, "guards") == 0 || strcmp(cmd, "guard") == 0 ||
      strcmp(cmd, "contracts") == 0 || strcmp(cmd, "contract") == 0)
    return cubalc_cmd_contracts(argc, argv);

  if (strcmp(cmd, "env") == 0 || strcmp(cmd, "environ") == 0 ||
      strcmp(cmd, "vars") == 0 || strcmp(cmd, "env-list") == 0)
    return cubalc_cmd_vars(argc, argv);

  if (strcmp(cmd, "examples") == 0 || strcmp(cmd, "example") == 0 ||
      strcmp(cmd, "starters") == 0 || strcmp(cmd, "recipes") == 0)
    return cubalc_cmd_examples(argc, argv);

  if (strcmp(cmd, "cat") == 0 || strcmp(cmd, "type") == 0 ||
      strcmp(cmd, "read-lib") == 0 || strcmp(cmd, "source") == 0)
    return cubalc_cmd_cat(argc, argv);

  if (strcmp(cmd, "recipe") == 0 || strcmp(cmd, "card") == 0 ||
      strcmp(cmd, "libcard") == 0 || strcmp(cmd, "lib-card") == 0 ||
      strcmp(cmd, "describe-lib") == 0)
    return cubalc_cmd_recipe(argc, argv);

  if (strcmp(cmd, "checkdeps") == 0 || strcmp(cmd, "deps") == 0 ||
      strcmp(cmd, "hasdeps") == 0 || strcmp(cmd, "needdeps") == 0 ||
      strcmp(cmd, "verifydeps") == 0 || strcmp(cmd, "libdeps-check") == 0)
    return cubalc_cmd_checkdeps(argc, argv);

  if (strcmp(cmd, "picklib") == 0 || strcmp(cmd, "firstlib") == 0 ||
      strcmp(cmd, "countmatchlibs") == 0 || strcmp(cmd, "countlibs") == 0 ||
      strcmp(cmd, "hasmatchlibs") == 0 || strcmp(cmd, "needmatchlibs") == 0 ||
      strcmp(cmd, "lastlib") == 0 || strcmp(cmd, "nthlib") == 0 ||
      strcmp(cmd, "newestlib") == 0 || strcmp(cmd, "latestlib") == 0 ||
      strcmp(cmd, "oldestlib") == 0 || strcmp(cmd, "earliestlib") == 0 ||
      strcmp(cmd, "sortlibs") == 0 || strcmp(cmd, "libsort") == 0 ||
      strcmp(cmd, "sortlib") == 0 || strcmp(cmd, "libsbymtime") == 0 ||
      strcmp(cmd, "libpick") == 0 || strcmp(cmd, "libcount") == 0)
    return cubalc_cmd_libmatch(argc, argv);

  if (strcmp(cmd, "freshlibs") == 0 || strcmp(cmd, "recentlibs") == 0 ||
      strcmp(cmd, "stalelibs") == 0 || strcmp(cmd, "olderlibs") == 0 ||
      strcmp(cmd, "keepfreshlibs") == 0 || strcmp(cmd, "keepstalelibs") == 0)
    return cubalc_cmd_freshlibs(argc, argv);

  if (strcmp(cmd, "libage") == 0 || strcmp(cmd, "ageoflib") == 0 ||
      strcmp(cmd, "hasfresh") == 0 || strcmp(cmd, "isfresh") == 0 ||
      strcmp(cmd, "needfresh") == 0 || strcmp(cmd, "requirefresh") == 0)
    return cubalc_cmd_libage(argc, argv);

  if (strcmp(cmd, "hasflagall") == 0 || strcmp(cmd, "hasallflags") == 0 ||
      strcmp(cmd, "needflags") == 0 || strcmp(cmd, "requireflags") == 0 ||
      strcmp(cmd, "mustflags") == 0 ||
      strcmp(cmd, "hasflagany") == 0 || strcmp(cmd, "hasanyflags") == 0 ||
      strcmp(cmd, "needflagany") == 0 || strcmp(cmd, "requireflagany") == 0 ||
      strcmp(cmd, "mustflagany") == 0 || strcmp(cmd, "needanyflags") == 0 ||
      strcmp(cmd, "hasargall") == 0 || strcmp(cmd, "hasallargs") == 0 ||
      strcmp(cmd, "needargs") == 0 || strcmp(cmd, "requireargs") == 0 ||
      strcmp(cmd, "mustargs") == 0 ||
      strcmp(cmd, "hasargany") == 0 || strcmp(cmd, "hasanyargs") == 0 ||
      strcmp(cmd, "needargany") == 0 || strcmp(cmd, "requireargany") == 0 ||
      strcmp(cmd, "mustargany") == 0 || strcmp(cmd, "needanyargs") == 0)
    return cubalc_cmd_flaggate(argc, argv);

  if (strcmp(cmd, "hasenvall") == 0 || strcmp(cmd, "hasallenv") == 0 ||
      strcmp(cmd, "needenvs") == 0 || strcmp(cmd, "requireenvs") == 0 ||
      strcmp(cmd, "mustenvs") == 0 || strcmp(cmd, "needenvall") == 0 ||
      strcmp(cmd, "requireenvall") == 0 ||
      strcmp(cmd, "hasenvany") == 0 || strcmp(cmd, "hasanyenv") == 0 ||
      strcmp(cmd, "needenvany") == 0 || strcmp(cmd, "requireenvany") == 0 ||
      strcmp(cmd, "mustenvany") == 0 || strcmp(cmd, "needanyenv") == 0 ||
      strcmp(cmd, "hasform") == 0 || strcmp(cmd, "formexists") == 0 ||
      strcmp(cmd, "needform") == 0 || strcmp(cmd, "requireform") == 0 ||
      strcmp(cmd, "knownform") == 0 ||
      strcmp(cmd, "hasforms") == 0 || strcmp(cmd, "needforms") == 0 ||
      strcmp(cmd, "requireforms") == 0 || strcmp(cmd, "allforms") == 0 ||
      strcmp(cmd, "hasformany") == 0 || strcmp(cmd, "hasanyforms") == 0 ||
      strcmp(cmd, "needformany") == 0 || strcmp(cmd, "requireformany") == 0 ||
      strcmp(cmd, "mustformany") == 0 || strcmp(cmd, "needanyforms") == 0)
    return cubalc_cmd_envform(argc, argv);

  return -1;
}
