/* CubalC CLI protect — F_main peel from main · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_protect.h"
#include "cubalc_cmd_law.h"
#include "cubalc_cmd_smx_selftest.h"
#include "cubalc_cmd_smx_bus.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

static void state_dir(char *buf, size_t n) {
  const char *e = getenv("CUBALC_STATE");
  if (e && e[0]) { snprintf(buf, n, "%s", e); return; }
  snprintf(buf, n, "state");
}

static void ensure_dir(const char *d) { mkdir(d, 0755); }

/* Core protection layer — NexusCore + nanobot mesh enforcement under Cube Laws.
 * Priorities (WE ACK): one_cmd · SMX fail-closed · budget · CT101 · mesh · device hold
 */
int cubalc_cmd_protect(int argc, char **argv) {
  const char *mode = (argc > 2) ? argv[2] : "all";
  char dir[512], plate_path[640];
  int fail = 0, checks = 0;
  cubalc_run_result rr;
  const char *prog = "programs/protect/core_protect.cubalc";
  (void)argv;

  state_dir(dir, sizeof dir);
  ensure_dir(dir);
  snprintf(plate_path, sizeof plate_path, "%s/CORE_PROTECT.json", dir);

  /* protect status|plate — JSON summary without board/smx run (agents).
   * Surfaces docs/lib readiness + next actions so agents don't re-run board. */
  if (strcmp(mode, "status") == 0 || strcmp(mode, "plate") == 0 ||
      strcmp(mode, "show") == 0 || strcmp(mode, "summary") == 0 ||
      strcmp(mode, "info") == 0) {
    int plate = (access(plate_path, R_OK) == 0);
    int last_ok = -1; /* -1 unknown, 0 false, 1 true */
    const char *pe = getenv("CUBALC_PROTECT");
    const char *hx = getenv("CUBALC_SMX_KEY");
    const char *kf = getenv("CUBALC_SMX_KEY_FILE");
    int protect_env = (pe && pe[0] && pe[0] != '0');
    int prog_ok = (access(prog, R_OK) == 0);
    int docs_cp = (access("docs/CORE_PROTECT.md", R_OK) == 0);
    int docs_hf = (access("docs/HOLD_FLASH.md", R_OK) == 0);
    int modular = (access("src/lang/lang_parse.c", R_OK) == 0 &&
                   access("src/lang/lang_ops_smx.c", R_OK) == 0);
    int smx_key = 0;
    const char *why;
    if (hx && strlen(hx) >= 64) smx_key = 1;
    else if (kf && kf[0] && access(kf, R_OK) == 0) smx_key = 1;
    if (plate) {
      FILE *f = fopen(plate_path, "r");
      if (f) {
        char buf[4096];
        size_t n = fread(buf, 1, sizeof buf - 1, f);
        buf[n] = 0;
        fclose(f);
        if (strstr(buf, "\"ok\": true") || strstr(buf, "\"ok\":true"))
          last_ok = 1;
        else if (strstr(buf, "\"ok\": false") || strstr(buf, "\"ok\":false"))
          last_ok = 0;
      }
    }
    if (!prog_ok)
      why = "missing programs/protect/core_protect.cubalc — check checkout";
    else if (last_ok == 0)
      why = "last CORE_PROTECT plate ok:false — cubalc protect all to recheck";
    else if (!plate)
      why = "no plate yet — cubalc protect all writes CORE_PROTECT.json";
    else if (!smx_key)
      why = "status ok · set CUBALC_SMX_KEY for P2P · cubalc protect all to recheck";
    else
      why = "status only — plate present · cubalc protect all for live checks";
    printf("{\"schema\":\"cubalc.protect_status.v1\",\"ok\":true,"
           "\"cmd\":\"protect\",\"mode\":\"status\","
           "\"hold_flash\":%d,"
           "\"hold_flash_means\":\"device_firmware_connection_safeguard\","
           "\"budget\":%d,\"share\":\"%s\","
           "\"http_required\":false,\"law\":\"core_protect\","
           "\"protect_env\":%s,\"program_present\":%s,"
           "\"plate_present\":%s,\"plate\":\"%s\","
           "\"last_plate_ok\":%s,\"version\":\"%s\","
           "\"program\":\"%s\","
           "\"docs_core_protect\":%s,\"docs_hold_flash\":%s,"
           "\"modular_lang\":%s,\"smx_key_configured\":%s,"
           "\"why_hint\":\"%s\","
           "\"note\":\"status only — no board/smx; run cubalc protect all for checks\","
           "\"next\":[\"cubalc protect all\",\"cubalc doctor\",\"cubalc env\"],"
           "\"hints\":["
           "\"HOLD_FLASH device/mesh-join only · default 1 · not language PLUG\","
           "\"export CUBALC_SMX_KEY=$(openssl rand -hex 32) for P2P\","
           "\"cubalc protect all · cubalc doctor · docs/CORE_PROTECT.md\""
           "]}\n",
           CUBALC_HOLD_FLASH, CUBALC_BUDGET, CUBALC_SHARE,
           protect_env ? "true" : "false",
           prog_ok ? "true" : "false",
           plate ? "true" : "false", plate_path,
           last_ok < 0 ? "null" : (last_ok ? "true" : "false"),
           CUBALC_LANG_VERSION, prog,
           docs_cp ? "true" : "false",
           docs_hf ? "true" : "false",
           modular ? "true" : "false",
           smx_key ? "true" : "false",
           why);
    return 0;
  }

  /* always set protect host mode for this process subtree semantics */
  setenv("CUBALC_PROTECT", "1", 0);

  printf("# CubalC core-protect mode=%s version=%s\n", mode, CUBALC_LANG_VERSION);

  if (strcmp(mode, "all") == 0 || strcmp(mode, "law") == 0) {
    checks++;
    if (cubalc_cmd_law_manifest() != 0) {
      printf("{\"ok\":false,\"check\":\"law\"}\n");
      fail++;
    } else {
      printf("{\"ok\":true,\"check\":\"law\",\"law\":\"core_protect\"}\n");
    }
    if (strcmp(mode, "law") == 0)
      return fail ? 1 : 0;
  }

  if (strcmp(mode, "all") == 0 || strcmp(mode, "smx") == 0) {
    checks++;
    if (cubalc_cmd_smx_selftest() != 0) {
      printf("{\"ok\":false,\"check\":\"smx-selftest\"}\n");
      fail++;
    } else {
      printf("{\"ok\":true,\"check\":\"smx-selftest\"}\n");
    }
    if (strcmp(mode, "smx") == 0)
      return fail ? 1 : 0;
  }

  if (strcmp(mode, "all") == 0 || strcmp(mode, "bus") == 0 ||
      strcmp(mode, "mesh") == 0) {
    checks++;
    {
      char *av[] = { "cubalc", "smx-bus", "prove", NULL };
      if (cubalc_cmd_smx_bus(3, av) != 0) {
        printf("{\"ok\":false,\"check\":\"smx-bus-prove\"}\n");
        fail++;
      } else {
        printf("{\"ok\":true,\"check\":\"smx-bus-prove\",\"http\":false}\n");
      }
    }
    if (strcmp(mode, "bus") == 0 || strcmp(mode, "mesh") == 0)
      return fail ? 1 : 0;
  }

  if (strcmp(mode, "all") == 0 || strcmp(mode, "board") == 0 ||
      strcmp(mode, "run") == 0) {
    checks++;
    memset(&rr, 0, sizeof rr);
    if (cubalc_run_file(prog, &rr, stdout) != 0 || !rr.ok) {
      printf("{\"ok\":false,\"check\":\"core_protect_board\",\"err\":\"%s\","
             "\"asserts_fail\":%d}\n",
             rr.err[0] ? rr.err : "fail", rr.asserts_fail);
      fail++;
    } else {
      printf("{\"ok\":true,\"check\":\"core_protect_board\",\"asserts_ok\":%d,"
             "\"n\":%d,\"unity\":%.3f}\n",
             rr.asserts_ok, rr.n_cubes, rr.unity);
    }
  }

  /* write protect plate for NexusCore / nanobot hosts */
  {
    FILE *f = fopen(plate_path, "w");
    if (f) {
      fprintf(f,
        "{\n"
        "  \"schema\": \"cubalc.core_protect.v1\",\n"
        "  \"ok\": %s,\n"
        "  \"to\": \"NexusCore\",\n"
        "  \"from\": \"cubalc\",\n"
        "  \"version\": \"%s\",\n"
        "  \"language\": \"CubalC\",\n"
        "  \"tok\": \"C3\",\n"
        "  \"hold_flash\": %d,\n"
        "  \"budget\": %d,\n"
        "  \"share\": \"smx\",\n"
        "  \"http_required\": false,\n"
        "  \"protect_mode\": true,\n"
        "  \"law\": \"core_protect\",\n"
        "  \"priorities\": [\n"
        "    \"one_commander\",\n"
        "    \"smx_fail_closed\",\n"
        "    \"hold_flash\",\n"
        "    \"budget\",\n"
        "    \"ct101\",\n"
        "    \"nanobot_mesh\"\n"
        "  ],\n"
        "  \"checks\": %d,\n"
        "  \"fail\": %d,\n"
        "  \"program\": \"%s\",\n"
        "  \"note\": \"Core stability under Cube Laws · nanobot SMX mesh\"\n"
        "}\n",
        fail == 0 ? "true" : "false",
        CUBALC_LANG_VERSION,
        CUBALC_HOLD_FLASH,
        CUBALC_BUDGET,
        checks,
        fail,
        prog);
      fclose(f);
    }
  }

  printf("{\"ok\":%s,\"cmd\":\"protect\",\"mode\":\"%s\",\"checks\":%d,"
         "\"fail\":%d,\"plate\":\"%s\",\"version\":\"%s\","
         "\"law\":\"core_protect\",\"http\":false}\n",
         fail == 0 ? "true" : "false", mode, checks, fail, plate_path,
         CUBALC_LANG_VERSION);
  return fail == 0 ? 0 : 1;
}
