/* CubalC CLI discover|open|topics|matchtopics|topichint|guide|err* —
 * F_main_carve try-dispatch peel from cubalc_main.c.
 * Returns exit code if handled, -1 if not this family. Zero bloat. Cube is SoT.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_discover_dispatch.h"
#include "cubalc_cmd_formsfor.h"
#include "cubalc_cmd_open.h"
#include "cubalc_cmd_matchtopics.h"
#include "cubalc_cmd_tips.h"
#include "cubalc_cmd_topichint.h"
#include "cubalc_cmd_guide.h"
#include "cubalc_cmd_formhints.h"
#include "cubalc_cmd_errrun.h"
#include "cubalc_cmd_whyguide.h"
#include <string.h>

int cubalc_cmd_try_discover_dispatch(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "";
  if (!cmd[0]) return -1;

  if (strcmp(cmd, "discover") == 0 || strcmp(cmd, "explore") == 0 ||
      strcmp(cmd, "discovertopics") == 0 || strcmp(cmd, "scout") == 0 ||
      strcmp(cmd, "opendiscover") == 0)
    return cubalc_cmd_formsfor(argc, argv);

  if (strcmp(cmd, "open") == 0 || strcmp(cmd, "openplay") == 0 ||
      strcmp(cmd, "openguide") == 0 || strcmp(cmd, "playopen") == 0 ||
      strcmp(cmd, "discoverguide") == 0 ||
      strcmp(cmd, "topics") == 0 || strcmp(cmd, "listtopics") == 0 ||
      strcmp(cmd, "topiclist") == 0 || strcmp(cmd, "discovery-topics") == 0 ||
      strcmp(cmd, "hastopic") == 0 || strcmp(cmd, "needtopic") == 0)
    return cubalc_cmd_open(argc, argv);

  if (strcmp(cmd, "matchtopics") == 0 || strcmp(cmd, "filtertopics") == 0 ||
      strcmp(cmd, "topicmatch") == 0 || strcmp(cmd, "greptopics") == 0 ||
      strcmp(cmd, "picktopic") == 0 || strcmp(cmd, "firsttopic") == 0 ||
      strcmp(cmd, "topicpick") == 0 ||
      strcmp(cmd, "hasmatchtopics") == 0 || strcmp(cmd, "hastopicmatch") == 0 ||
      strcmp(cmd, "needmatchtopics") == 0 || strcmp(cmd, "needtopicmatch") == 0 ||
      strcmp(cmd, "countmatchtopics") == 0 || strcmp(cmd, "counttopics") == 0 ||
      strcmp(cmd, "ntopicmatch") == 0)
    return cubalc_cmd_matchtopics(argc, argv);

  if (strcmp(cmd, "nthtopic") == 0 || strcmp(cmd, "indextopic") == 0 ||
      strcmp(cmd, "topicnth") == 0 || strcmp(cmd, "lasttopic") == 0 ||
      strcmp(cmd, "endtopic") == 0 || strcmp(cmd, "topiclast") == 0)
    return cubalc_cmd_tips(argc, argv);

  if (strcmp(cmd, "topichint") == 0 || strcmp(cmd, "describetopic") == 0 ||
      strcmp(cmd, "topicdoc") == 0 || strcmp(cmd, "topichelp") == 0 ||
      strcmp(cmd, "hinttopic") == 0 || strcmp(cmd, "topic-hint") == 0 ||
      strcmp(cmd, "relatedtopic") == 0 || strcmp(cmd, "seetopics") == 0 ||
      strcmp(cmd, "relatedtopics") == 0 || strcmp(cmd, "topicrel") == 0 ||
      strcmp(cmd, "neartopics") == 0 || strcmp(cmd, "topic-neighbors") == 0 ||
      strcmp(cmd, "formtopics") == 0 || strcmp(cmd, "topicsof") == 0 ||
      strcmp(cmd, "topicsfor") == 0 || strcmp(cmd, "form-topics") == 0 ||
      strcmp(cmd, "covertopics") == 0 || strcmp(cmd, "topics-of") == 0)
    return cubalc_cmd_topichint(argc, argv);

  if (strcmp(cmd, "formguide") == 0 || strcmp(cmd, "guideform") == 0 ||
      strcmp(cmd, "form-guide") == 0 || strcmp(cmd, "formcard") == 0 ||
      strcmp(cmd, "playform") == 0 || strcmp(cmd, "formboard") == 0 ||
      strcmp(cmd, "guide") == 0 || strcmp(cmd, "playguide") == 0 ||
      strcmp(cmd, "topicguide") == 0 || strcmp(cmd, "agentguide") == 0 ||
      strcmp(cmd, "fulltopic") == 0 || strcmp(cmd, "topicboard") == 0 ||
      strcmp(cmd, "guidematch") == 0 || strcmp(cmd, "matchguide") == 0 ||
      strcmp(cmd, "guidenth") == 0 || strcmp(cmd, "nthguide") == 0 ||
      strcmp(cmd, "guidelastmatch") == 0 || strcmp(cmd, "lastmatchguide") == 0)
    return cubalc_cmd_guide(argc, argv);

  if (strcmp(cmd, "errtips") == 0 || strcmp(cmd, "fixtips") == 0 ||
      strcmp(cmd, "recovertips") == 0 || strcmp(cmd, "err-tips") == 0 ||
      strcmp(cmd, "fixhints") == 0 || strcmp(cmd, "whytips") == 0)
    return cubalc_cmd_formhints(argc, argv);

  if (strcmp(cmd, "errrun") == 0 || strcmp(cmd, "recoversnip") == 0 ||
      strcmp(cmd, "fixrun") == 0 || strcmp(cmd, "err-run") == 0 ||
      strcmp(cmd, "errsnip") == 0 || strcmp(cmd, "fixsnip") == 0)
    return cubalc_cmd_errrun(argc, argv);

  if (strcmp(cmd, "errguide") == 0 || strcmp(cmd, "recoverguide") == 0 ||
      strcmp(cmd, "fixguide") == 0 || strcmp(cmd, "err-guide") == 0 ||
      strcmp(cmd, "whyguide") == 0 || strcmp(cmd, "errplayguide") == 0)
    return cubalc_cmd_whyguide(argc, argv);

  return -1;
}
