#define _POSIX_C_SOURCE 200809L
#include "cubalc.h"
#include "cubalc_smx.h"
#include "cubalc_cubechain.h"
#include "cubalc_lang.h"
#include "cubalc_cmd_corehive_dispatch.h"
#include "cubalc_cmd_forms.h"
#include "cubalc_cmd_which.h"
#include "cubalc_cmd_search.h"
#include "cubalc_cmd_init.h"
#include "cubalc_cmd_plate.h"
#include "cubalc_cmd_doctor.h"
#include "cubalc_cmd_selftest.h"
#include "cubalc_cmd_formrel.h"
#include "cubalc_cmd_cliinfo.h"
#include "cubalc_cmd_argmap.h"
#include "cubalc_cmd_endarg.h"
#include "cubalc_cmd_findarg.h"
#include "cubalc_cmd_pathver.h"
#include "cubalc_cmd_help.h"
#include "cubalc_cmd_onboard.h"
#include "cubalc_cmd_cookbook.h"
#include "cubalc_cmd_topic.h"
#include "cubalc_cmd_runsnip.h"
#include "cubalc_cmd_formsfor.h"
#include "cubalc_cmd_discover_dispatch.h"
#include "cubalc_cmd_libgate_dispatch.h"
#include "cubalc_cmd_snip.h"
#include "cubalc_cmd_tips.h"
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
  /* genesis…evolve core-hive → cubalc_cmd_try_corehive_dispatch (F_main_carve) */
  {
    int crc = cubalc_cmd_try_corehive_dispatch(cmd, argc, argv);
    if (crc >= 0) return crc;
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
  /* libs·recipe·flag·env → cubalc_cmd_try_libgate_dispatch (F_main_carve) */
  {
    int lrc = cubalc_cmd_try_libgate_dispatch(argc, argv);
    if (lrc >= 0) return lrc;
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
