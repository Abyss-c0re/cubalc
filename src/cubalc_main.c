#define _POSIX_C_SOURCE 200809L
#include "cubalc.h"
#include "cubalc_smx.h"
#include "cubalc_cubechain.h"
#include "cubalc_lang.h"
#include "cubalc_evolve.h"
#include "cubalc_science_loop.h"
#include "cubalc_cmd_smx_exchange.h"
#include "cubalc_cmd_smx_bus.h"
#include "cubalc_cmd_smx_selftest.h"
#include "cubalc_cmd_protect.h"
#include "cubalc_cmd_showcase.h"
#include "cubalc_cmd_genesis.h"
#include "cubalc_cmd_sync.h"
#include "cubalc_cmd_bootpeers.h"
#include "cubalc_cmd_forms.h"
#include "cubalc_cmd_which.h"
#include "cubalc_cmd_search.h"
#include "cubalc_cmd_init.h"
#include "cubalc_cmd_plate.h"
#include "cubalc_cmd_law.h"
#include "cubalc_cmd_doctor.h"
#include "cubalc_cmd_selftest.h"
#include "cubalc_cmd_run.h"
#include "cubalc_cmd_stdlib.h"
#include "cubalc_cmd_vars.h"
#include "cubalc_cmd_formrel.h"
#include "cubalc_cmd_contracts.h"
#include "cubalc_cmd_cliinfo.h"
#include "cubalc_cmd_argmap.h"
#include "cubalc_cmd_endarg.h"
#include "cubalc_cmd_findarg.h"
#include "cubalc_cmd_envform.h"
#include "cubalc_cmd_pathver.h"
#include "cubalc_cmd_help.h"
#include "cubalc_cmd_flaggate.h"
#include "cubalc_cmd_translate.h"
#include "cubalc_cmd_onboard.h"
#include "cubalc_cmd_cookbook.h"
#include "cubalc_cmd_compile.h"
#include "cubalc_cmd_libmatch.h"
#include "cubalc_cmd_recipe.h"
#include "cubalc_cmd_checkdeps.h"
#include "cubalc_cmd_freshlibs.h"
#include "cubalc_cmd_libage.h"
#include "cubalc_cmd_examples.h"
#include "cubalc_cmd_topic.h"
#include "cubalc_cmd_runsnip.h"
#include "cubalc_cmd_formsfor.h"
#include "cubalc_cmd_discover_dispatch.h"
#include "cubalc_cmd_snip.h"
#include "cubalc_cmd_tips.h"
#include "cubalc_cmd_cat.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/* CubalC machine — CBLC · smx · cubechain · hold=1 */

/* state_dir/paths/publish/store_chain/load_genesis → cubalc_cmd_genesis (already peeled)
 * lib_count_recipe_meta → cubalc_cmd_contracts (already peeled) */

int main(int argc, char **argv) {
  const char *cmd = argc > 1 ? argv[1] : "genesis";
  /* Usability: cubalc -e CODE ≡ cubalc run -e CODE (no subcommand required).
   * Also -I/--include/-L top-level → run so agents skip the subcommand. */
  if (strcmp(cmd, "-e") == 0 || strcmp(cmd, "--expr") == 0 ||
      strcmp(cmd, "--code") == 0 || strcmp(cmd, "-c") == 0 ||
      strcmp(cmd, "-I") == 0 || strcmp(cmd, "--include") == 0 ||
      strcmp(cmd, "--preload") == 0 ||
      strncmp(cmd, "--include=", 10) == 0 ||
      strncmp(cmd, "--preload=", 10) == 0 ||
      strcmp(cmd, "-L") == 0 || strcmp(cmd, "--include-path") == 0 ||
      strcmp(cmd, "--lib-path") == 0 ||
      strncmp(cmd, "--include-path=", 15) == 0 ||
      strncmp(cmd, "--lib-path=", 11) == 0 ||
      strcmp(cmd, "-R") == 0 || strcmp(cmd, "--require-version") == 0 ||
      strncmp(cmd, "--require-version=", 18) == 0)
    cmd = "run";
  if (strcmp(cmd, "genesis") == 0)
    return cubalc_cmd_genesis(argc > 2 ? argv[2] :
      "NEXUS_COORD v1 | from=BlackCube | type=heartbeat | hold_flash=1 |");
  if (strcmp(cmd, "impulse") == 0)
    return cubalc_cmd_impulse(argc > 2 ? argv[2] : "cube-create",
                       argc > 3 ? atoi(argv[3]) : 1);
  if (strcmp(cmd, "tick") == 0 || strcmp(cmd, "flow") == 0)
    return cubalc_cmd_flow(argc > 2 ? atoi(argv[2]) : 8,
                    argc > 3 ? atoi(argv[3]) : 50);
  if (strcmp(cmd, "cubes") == 0 || strcmp(cmd, "show") == 0 ||
      strcmp(cmd, "lego") == 0) /* lego alias retired — cubes only */
    return cubalc_cmd_cubes();
  if (strcmp(cmd, "cubechain") == 0 || strcmp(cmd, "store") == 0)
    return cubalc_cmd_cubechain(argc > 2 ? argv[2] : "tip");
  if (strcmp(cmd, "smx-selftest") == 0 || strcmp(cmd, "smx") == 0)
    return cubalc_cmd_smx_selftest();
  if (strcmp(cmd, "smx-exchange") == 0)
    return cubalc_cmd_smx_exchange(argc, argv);
  if (strcmp(cmd, "smx-bus") == 0 || strcmp(cmd, "smx-nohttp") == 0 ||
      strcmp(cmd, "smx-net") == 0)
    return cubalc_cmd_smx_bus(argc, argv);
  if (strcmp(cmd, "law") == 0 || strcmp(cmd, "manifest") == 0)
    return cubalc_cmd_law_manifest();
  if (strcmp(cmd, "science") == 0 || strcmp(cmd, "sci") == 0 ||
      strcmp(cmd, "pure-science") == 0) {
    /* cubalc_cmd_science expects argv[0]=cubalc argv[1]=science argv[2]=sub */
    return cubalc_cmd_science(argc, argv);
  }
  if (strcmp(cmd, "protect") == 0 || strcmp(cmd, "core-protect") == 0 ||
      strcmp(cmd, "core-guard") == 0 || strcmp(cmd, "guard") == 0)
    return cubalc_cmd_protect(argc, argv);
  if (strcmp(cmd, "sync") == 0 || strcmp(cmd, "hive") == 0)
    return cubalc_cmd_sync(argc > 2 ? argv[2] : NULL);
  if (strcmp(cmd, "boot") == 0 || strcmp(cmd, "os") == 0) {
    /* boot|os → cubalc_cmd_bootpeers peel (F_main_carve) */
    return cubalc_cmd_boot(argc, argv);
  }
  if (strcmp(cmd, "translate") == 0 || strcmp(cmd, "from") == 0 ||
      strcmp(cmd, "decide") == 0) {
    /* translate|from|decide → cubalc_cmd_translate peel (F_main_carve) */
    return cubalc_cmd_translate(argc, argv);
  }
  if (strcmp(cmd, "compile") == 0 ||
      strcmp(cmd, "jit") == 0 || strcmp(cmd, "cflow") == 0 ||
      strcmp(cmd, "disasm") == 0) {
    /* compile|jit|cflow|disasm → cubalc_cmd_compile peel (F_main_carve) */
    return cubalc_cmd_compile(argc, argv);
  }
  if (strcmp(cmd, "run") == 0 || strcmp(cmd, "eval") == 0) {
    /* run/eval → cubalc_cmd_run peel (F_main_carve) */
    return cubalc_cmd_run(argc, argv);
  }
  if (strcmp(cmd, "peers") == 0 || strcmp(cmd, "oversee") == 0) {
    /* peers|oversee → cubalc_cmd_bootpeers peel (F_main_carve) */
    return cubalc_cmd_peers(argc, argv);
  }
  if (strcmp(cmd, "showcase") == 0 || strcmp(cmd, "demo") == 0 ||
      strcmp(cmd, "symphony") == 0) {
    return cubalc_cmd_showcase();
  }
  if (strcmp(cmd, "evolve") == 0 || strcmp(cmd, "evolve-loop") == 0 ||
      strcmp(cmd, "self-evolve") == 0) {
    /* Pure C: braincube solves · algocube optimizes · emits .cubalc */
    return cubalc_cmd_evolve(argc - 1, argv + 1);
  }
  if (strcmp(cmd, "cliinfo") == 0 || strcmp(cmd, "dumpcli") == 0 ||
      strcmp(cmd, "clisnap") == 0 || strcmp(cmd, "cli-info") == 0 ||
      strcmp(cmd, "argvinfo") == 0 || strcmp(cmd, "cli-status") == 0) {
    /* cliinfo → cubalc_cmd_cliinfo peel (F_main_carve) */
    return cubalc_cmd_cliinfo(argc, argv);
  }
  if (strcmp(cmd, "argmap") == 0 || strcmp(cmd, "argkv") == 0 ||
      strcmp(cmd, "argvmap") == 0 || strcmp(cmd, "args-kv") == 0 ||
      strcmp(cmd, "cli-argmap") == 0) {
    /* argmap → cubalc_cmd_argmap peel (F_main_carve) */
    return cubalc_cmd_argmap(argc, argv);
  }
  if (strcmp(cmd, "findarg") == 0 || strcmp(cmd, "argindex") == 0 ||
      strcmp(cmd, "indexarg") == 0 || strcmp(cmd, "whicharg") == 0 ||
      strcmp(cmd, "argvindex") == 0 ||
      strcmp(cmd, "afterarg") == 0 || strcmp(cmd, "nextarg") == 0 ||
      strcmp(cmd, "argafter") == 0 || strcmp(cmd, "followarg") == 0 ||
      strcmp(cmd, "argnext") == 0) {
    /* findarg/afterarg → cubalc_cmd_findarg peel (F_main_carve) */
    return cubalc_cmd_findarg(argc, argv);
  }
  if (strcmp(cmd, "firstarg") == 0 || strcmp(cmd, "headarg") == 0 ||
      strcmp(cmd, "argv0") == 0 || strcmp(cmd, "lastarg") == 0 ||
      strcmp(cmd, "tailarg") == 0 || strcmp(cmd, "argvlast") == 0) {
    /* firstarg/lastarg → cubalc_cmd_endarg peel (F_main_carve) */
    return cubalc_cmd_endarg(argc, argv);
  }
  if (strcmp(cmd, "needdoctor") == 0 || strcmp(cmd, "need-doctor") == 0 ||
      strcmp(cmd, "require-doctor") == 0 || strcmp(cmd, "requiredoctor") == 0 ||
      strcmp(cmd, "needready") == 0 || strcmp(cmd, "need-ready") == 0 ||
      strcmp(cmd, "require-ready") == 0 || strcmp(cmd, "requireready") == 0 ||
      strcmp(cmd, "mustready") == 0 ||
      strcmp(cmd, "ready") == 0 || strcmp(cmd, "proveready") == 0 ||
      strcmp(cmd, "install-ready") == 0 || strcmp(cmd, "check-ready") == 0 ||
      strcmp(cmd, "doctor") == 0 || strcmp(cmd, "health") == 0) {
    /* doctor/ready/needdoctor/needready → cubalc_cmd_doctor peel (F_main_carve) */
    return cubalc_cmd_doctor(argc, argv);
  }
  if (strcmp(cmd, "selftest") == 0 || strcmp(cmd, "smoke") == 0 ||
      strcmp(cmd, "usability-test") == 0 || strcmp(cmd, "prove-usability") == 0) {
    /* selftest/smoke → cubalc_cmd_selftest peel (F_main_carve) */
    return cubalc_cmd_selftest(argc, argv);
  }
  if (strcmp(cmd, "forms") == 0 || strcmp(cmd, "ops") == 0 ||
      strcmp(cmd, "forms-list") == 0 ||
      strcmp(cmd, "listforms") == 0 || strcmp(cmd, "formlist") == 0 ||
      strcmp(cmd, "countforms") == 0 || strcmp(cmd, "nforms") == 0 ||
      strcmp(cmd, "formhint") == 0 || strcmp(cmd, "describeform") == 0 ||
      strcmp(cmd, "formdoc") == 0 || strcmp(cmd, "formhelp") == 0) {
    /* forms/listforms/formhint → cubalc_cmd_forms peel (F_main_carve) */
    return cubalc_cmd_forms(argc, argv);
  }
  if (strcmp(cmd, "onboard") == 0 || strcmp(cmd, "welcome") == 0 ||
      strcmp(cmd, "begin") == 0 || strcmp(cmd, "agent-start") == 0 ||
      strcmp(cmd, "hello-agent") == 0 || strcmp(cmd, "bootstrap") == 0) {
    /* onboard|welcome|bootstrap → cubalc_cmd_onboard peel (F_main_carve) */
    return cubalc_cmd_onboard(argc, argv);
  }
  if (strcmp(cmd, "cookbook") == 0 || strcmp(cmd, "start") == 0) {
    /* cookbook|start → cubalc_cmd_cookbook peel (F_main_carve) */
    return cubalc_cmd_cookbook();
  }
  if (strcmp(cmd, "tips") == 0 || strcmp(cmd, "tip") == 0 ||
      strcmp(cmd, "howto") == 0 || strcmp(cmd, "next") == 0 ||
      strcmp(cmd, "playbook") == 0) {
    /* tips·howto → cubalc_cmd_tips peel (F_main_carve) */
    return cubalc_cmd_tips(argc, argv);
  }
  if (strcmp(cmd, "formsfor") == 0 || strcmp(cmd, "topicforms") == 0 ||
      strcmp(cmd, "formstopic") == 0 || strcmp(cmd, "surface") == 0 ||
      strcmp(cmd, "forms-for") == 0 || strcmp(cmd, "capforms") == 0) {
    /* formsfor·topicforms → cubalc_cmd_formsfor peel (F_main_carve) */
    return cubalc_cmd_formsfor(argc, argv);
  }
  if (strcmp(cmd, "related") == 0 || strcmp(cmd, "seealso") == 0 ||
      strcmp(cmd, "formslike") == 0 || strcmp(cmd, "nearforms") == 0 ||
      strcmp(cmd, "formrel") == 0 || strcmp(cmd, "neighbors") == 0) {
    /* related/formrel → cubalc_cmd_formrel peel (F_main_carve) */
    return cubalc_cmd_formrel(argc, argv);
  }
  if (strcmp(cmd, "snip") == 0 || strcmp(cmd, "snippet") == 0 ||
      strcmp(cmd, "mini") == 0 || strcmp(cmd, "miniprog") == 0 ||
      strcmp(cmd, "codesnip") == 0 || strcmp(cmd, "starter-src") == 0) {
    /* snip·snippet → cubalc_cmd_snip peel (F_main_carve) */
    return cubalc_cmd_snip(argc, argv);
  }
  if (strcmp(cmd, "topic") == 0 || strcmp(cmd, "topiccard") == 0 ||
      strcmp(cmd, "topicinfo") == 0 || strcmp(cmd, "playcard") == 0 ||
      strcmp(cmd, "discover-topic") == 0 || strcmp(cmd, "topicpack") == 0) {
    /* topic·topiccard → cubalc_cmd_topic peel (F_main_carve) */
    return cubalc_cmd_topic(argc, argv);
  }
  if (strcmp(cmd, "runsnip") == 0 || strcmp(cmd, "sniprun") == 0 ||
      strcmp(cmd, "snip-run") == 0 || strcmp(cmd, "run-snip") == 0 ||
      strcmp(cmd, "exec-snip") == 0 || strcmp(cmd, "play-snip") == 0 ||
      strcmp(cmd, "runsnipmatch") == 0 || strcmp(cmd, "matchrunsnip") == 0 ||
      strcmp(cmd, "runsnipnth") == 0 || strcmp(cmd, "nthrunsnip") == 0 ||
      strcmp(cmd, "runsniplastmatch") == 0 || strcmp(cmd, "lastmatchrunsnip") == 0) {
    /* runsnip·sniprun → cubalc_cmd_runsnip peel (F_main_carve) */
    return cubalc_cmd_runsnip(argc, argv);
  }
  /* discover·open·topics·matchtopics·topichint·guide·err* → cubalc_cmd_try_discover_dispatch (F_main_carve) */
  {
    int drc = cubalc_cmd_try_discover_dispatch(argc, argv);
    if (drc >= 0) return drc;
  }
  if (strcmp(cmd, "init") == 0 || strcmp(cmd, "new") == 0 ||
      strcmp(cmd, "scaffold") == 0 || strcmp(cmd, "create") == 0) {
    return cubalc_cmd_init(argc, argv);
  }
  if (strcmp(cmd, "libs") == 0 || strcmp(cmd, "lib") == 0 ||
      strcmp(cmd, "stdlib") == 0) {
    /* libs/lib/stdlib → cubalc_cmd_stdlib peel (F_main_carve) */
    return cubalc_cmd_stdlib(argc, argv);
  }
  
  if (strcmp(cmd, "guards") == 0 || strcmp(cmd, "guard") == 0 ||
      strcmp(cmd, "contracts") == 0 || strcmp(cmd, "contract") == 0) {
    /* contracts → cubalc_cmd_contracts peel (F_main_carve) */
    return cubalc_cmd_contracts(argc, argv);
  }
  if (strcmp(cmd, "env") == 0 || strcmp(cmd, "environ") == 0 ||
      strcmp(cmd, "vars") == 0 || strcmp(cmd, "env-list") == 0) {
    /* env/vars/env-list → cubalc_cmd_vars peel (F_main_carve) */
    return cubalc_cmd_vars(argc, argv);
  }
  if (strcmp(cmd, "examples") == 0 || strcmp(cmd, "example") == 0 ||
      strcmp(cmd, "starters") == 0 || strcmp(cmd, "recipes") == 0)
    return cubalc_cmd_examples(argc, argv);
  if (strcmp(cmd, "cat") == 0 || strcmp(cmd, "type") == 0 ||
      strcmp(cmd, "read-lib") == 0 || strcmp(cmd, "source") == 0) {
    /* cat/type/source → cubalc_cmd_cat peel (F_main_carve) */
    return cubalc_cmd_cat(argc, argv);
  }
  if (strcmp(cmd, "recipe") == 0 || strcmp(cmd, "card") == 0 ||
      strcmp(cmd, "libcard") == 0 || strcmp(cmd, "lib-card") == 0 ||
      strcmp(cmd, "describe-lib") == 0) {
    /* recipe/card → cubalc_cmd_recipe peel (F_main_carve) */
    return cubalc_cmd_recipe(argc, argv);
  }
  if (strcmp(cmd, "checkdeps") == 0 || strcmp(cmd, "deps") == 0 ||
      strcmp(cmd, "hasdeps") == 0 || strcmp(cmd, "needdeps") == 0 ||
      strcmp(cmd, "verifydeps") == 0 || strcmp(cmd, "libdeps-check") == 0) {
    /* checkdeps/hasdeps/needdeps → cubalc_cmd_checkdeps peel (F_main_carve) */
    return cubalc_cmd_checkdeps(argc, argv);
  }
  if (strcmp(cmd, "picklib") == 0 || strcmp(cmd, "firstlib") == 0 ||
      strcmp(cmd, "countmatchlibs") == 0 || strcmp(cmd, "countlibs") == 0 ||
      strcmp(cmd, "hasmatchlibs") == 0 || strcmp(cmd, "needmatchlibs") == 0 ||
      strcmp(cmd, "lastlib") == 0 || strcmp(cmd, "nthlib") == 0 ||
      strcmp(cmd, "newestlib") == 0 || strcmp(cmd, "latestlib") == 0 ||
      strcmp(cmd, "oldestlib") == 0 || strcmp(cmd, "earliestlib") == 0 ||
      strcmp(cmd, "sortlibs") == 0 || strcmp(cmd, "libsort") == 0 ||
      strcmp(cmd, "sortlib") == 0 || strcmp(cmd, "libsbymtime") == 0 ||
      strcmp(cmd, "libpick") == 0 || strcmp(cmd, "libcount") == 0) {
    /* picklib/sortlibs/match → cubalc_cmd_libmatch peel (F_main_carve) */
    return cubalc_cmd_libmatch(argc, argv);
  }
  if (strcmp(cmd, "freshlibs") == 0 ||
      strcmp(cmd, "recentlibs") == 0 ||
      strcmp(cmd, "stalelibs") == 0 ||
      strcmp(cmd, "olderlibs") == 0 ||
      strcmp(cmd, "keepfreshlibs") == 0 ||
      strcmp(cmd, "keepstalelibs") == 0) {
    /* freshlibs/stalelibs → cubalc_cmd_freshlibs peel (F_main_carve) */
    return cubalc_cmd_freshlibs(argc, argv);
  }
  if (strcmp(cmd, "libage") == 0 ||
      strcmp(cmd, "ageoflib") == 0 ||
      strcmp(cmd, "hasfresh") == 0 ||
      strcmp(cmd, "isfresh") == 0 ||
      strcmp(cmd, "needfresh") == 0 ||
      strcmp(cmd, "requirefresh") == 0) {
    /* libage/hasfresh/needfresh → cubalc_cmd_libage peel (F_main_carve) */
    return cubalc_cmd_libage(argc, argv);
  }
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
      strcmp(cmd, "mustargany") == 0 || strcmp(cmd, "needanyargs") == 0) {
    /* hasflagall/needflags/hasargall/needargs → cubalc_cmd_flaggate peel (F_main_carve) */
    return cubalc_cmd_flaggate(argc, argv);
  }
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
      strcmp(cmd, "mustformany") == 0 || strcmp(cmd, "needanyforms") == 0) {
    /* hasenvall/hasform gates → cubalc_cmd_envform peel (F_main_carve) */
    return cubalc_cmd_envform(argc, argv);
  }
  if (strcmp(cmd, "plate") == 0 || strcmp(cmd, "jsonplate") == 0 ||
      strcmp(cmd, "platefile") == 0 || strcmp(cmd, "agent-plate") == 0) {
    return cubalc_cmd_plate(argc, argv);
  }
  if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0 ||
      strcmp(cmd, "-V") == 0 || strcmp(cmd, "ver") == 0 ||
      strcmp(cmd, "paths") == 0 || strcmp(cmd, "path") == 0 ||
      strcmp(cmd, "where") == 0 || strcmp(cmd, "layout") == 0) {
    /* version/paths → cubalc_cmd_pathver peel (F_main_carve) */
    return cubalc_cmd_pathver(argc, argv);
  }
  if (strcmp(cmd, "which") == 0 || strcmp(cmd, "locate") == 0 ||
      strcmp(cmd, "resolve") == 0) {
    return cubalc_cmd_which(argc, argv);
  }
  if (strcmp(cmd, "search") == 0 || strcmp(cmd, "find") == 0 ||
      strcmp(cmd, "query") == 0 || strcmp(cmd, "look") == 0) {
    return cubalc_cmd_search(argc, argv);
  }
  if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0) {
    /* help → cubalc_cmd_help peel (F_main_carve) */
    return cubalc_cmd_help(argc, argv);
  }
  fprintf(stderr, "unknown cmd %s — try help\n", cmd);
  return 2;
}
