/* CubalC CLI onboard|welcome|bootstrap — F_main_carve peel.
 * Peel from cubalc_main.c. Cube is SoT. Zero bloat.
 */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_onboard.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>

int cubalc_cmd_onboard(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "onboard";
  (void)cmd;

    /* Usability: agent onboarding JSON (dual of START/ONBOARD).
     *   cubalc onboard · cubalc welcome
     * Schema cubalc.start.v1 · (cubalc start remains cookbook alias). */
    static const char *topics[] = {
      "general", "cap", "fat", "plate", "p2p", "run", "lib", "protect"
    };
    static const char *nexts[] = {
      "cubalc doctor",
      "cubalc guide general",
      "cubalc topics",
      "cubalc init --list",
      "cubalc search KEYWORD",
      "INCLUDE agent_boot",
      "cubalc formguide HASFORM",
      "cubalc errguide LAST_ERR",
      "HOLD_FLASH device/mesh-join only",
    };
    static const char *hint =
      "install/doctor/init surface · VERSION STATUS IDENTITY · GUIDE general";
    int i, nt = (int)(sizeof topics / sizeof topics[0]);
    int nn = (int)(sizeof nexts / sizeof nexts[0]);
    char hj[320];
    size_t o = 0, j;
    for (j = 0; hint[j] && o + 2 < sizeof hj; j++) {
      char c = hint[j];
      if (c == '"' || c == '\\') { hj[o++] = '\\'; hj[o++] = c; }
      else hj[o++] = c;
    }
    hj[o] = 0;
    printf("{\"schema\":\"cubalc.start.v1\",\"ok\":true,\"cmd\":\"onboard\","
           "\"version\":\"%s\",\"topics_n\":%d,\"next_n\":%d,"
           "\"hint\":\"%s\","
           "\"note\":\"agent onboarding · dual of START/ONBOARD · chain guide/doctor/init\","
           "\"topics\":[",
           CUBALC_LANG_VERSION, nt, nn, hj);
    for (i = 0; i < nt; i++)
      printf("%s\"%s\"", i ? "," : "", topics[i]);
    printf("],\"next\":[");
    for (i = 0; i < nn; i++) {
      char esc[160];
      size_t e = 0;
      for (j = 0; nexts[i][j] && e + 2 < sizeof esc; j++) {
        char c = nexts[i][j];
        if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
        else esc[e++] = c;
      }
      esc[e] = 0;
      printf("%s\"%s\"", i ? "," : "", esc);
    }
    printf("]}\n");
    return 0;
  
}
