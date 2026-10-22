/* CubalC CLI doctor/health — F_main peel · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_cmd_doctor.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

static void state_dir(char *buf, size_t n) {
  const char *e = getenv("CUBALC_STATE");
  if (e && e[0]) { snprintf(buf, n, "%s", e); return; }
  snprintf(buf, n, "state");
}

int cubalc_cmd_doctor(int argc, char **argv) {
  const char *cmd = (argc > 1 && argv[1]) ? argv[1] : "doctor";
  /* needdoctor/needready hard gates (F_main peel into doctor) */
  if (strcmp(cmd, "needdoctor") == 0 || strcmp(cmd, "need-doctor") == 0 ||
      strcmp(cmd, "require-doctor") == 0 || strcmp(cmd, "requiredoctor") == 0) {
    /* Usability: hard install gate (dual of NEEDDOCTOR). exit 1 if not ready. */
    cubalc_run_result rr;
    int ok;
    static const char src_nd[] = "NEEDDOCTOR\nPASS\n";
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src_nd, sizeof src_nd - 1, "<cli-needdoctor>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    {
      const char *em = ok ? "" : (rr.err[0] ? rr.err : (rr.last_err[0] ? rr.last_err : "not ready"));
      char esc[240];
      size_t e = 0, k;
      for (k = 0; em[k] && e + 2 < sizeof esc; k++) {
        char c = em[k];
        if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
        else if ((unsigned char)c < 32) esc[e++] = ' ';
        else esc[e++] = c;
      }
      esc[e] = 0;
      printf("{\"schema\":\"cubalc.doctor.v1\",\"ok\":%s,\"cmd\":\"needdoctor\","
             "\"version\":\"%s\","
             "\"note\":\"hard install gate · dual of NEEDDOCTOR · exit 1 if not ready\","
             "\"err\":\"%s\"}\n",
             ok ? "true" : "false", CUBALC_LANG_VERSION, esc);
    }
    return ok ? 0 : 1;
  }
    if (strcmp(cmd, "needready") == 0 || strcmp(cmd, "need-ready") == 0 ||
      strcmp(cmd, "require-ready") == 0 || strcmp(cmd, "requireready") == 0 ||
      strcmp(cmd, "mustready") == 0) {
    /* Usability: hard install prove gate (dual of NEEDREADY). exit 1 if checklist fails.
     * Completes ready triad: cubalc ready (soft) · cubalc needready (hard) · run -Y. */
    cubalc_run_result rr;
    int ok;
    static const char src_nr[] = "NEEDREADY\nPASS\n";
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src_nr, sizeof src_nr - 1, "<cli-needready>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    {
      const char *em = ok ? "" : (rr.err[0] ? rr.err : (rr.last_err[0] ? rr.last_err : "not ready"));
      char esc[240];
      size_t e = 0, k;
      for (k = 0; em[k] && e + 2 < sizeof esc; k++) {
        char c = em[k];
        if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = c; }
        else if ((unsigned char)c < 32) esc[e++] = ' ';
        else esc[e++] = c;
      }
      esc[e] = 0;
      printf("{\"schema\":\"cubalc.ready.v1\",\"ok\":%s,\"cmd\":\"needready\","
             "\"version\":\"%s\","
             "\"note\":\"hard install prove gate · dual of NEEDREADY · exit 1 if checklist fails\","
             "\"err\":\"%s\"}\n",
             ok ? "true" : "false", CUBALC_LANG_VERSION, esc);
    }
    return ok ? 0 : 1;
  }

  if (strcmp(cmd, "ready") == 0 || strcmp(cmd, "proveready") == 0 ||
      strcmp(cmd, "install-ready") == 0 || strcmp(cmd, "check-ready") == 0) {
    /* Usability: install prove checklist (dual of READY). Soft by default.
     *   cubalc ready · cubalc proveready
     * Schema cubalc.ready.v1 · agents skip multi HASFORM+HASLIB glue. */
    cubalc_run_result rr;
    int ok;
    static const char src[] = "READY\nPASS\n";
    memset(&rr, 0, sizeof rr);
    (void)cubalc_run_source(src, sizeof src - 1, "<cli-ready>", &rr, NULL);
    ok = (rr.ok && rr.asserts_fail == 0 && !rr.err[0]) ? 1 : 0;
    /* Prefer nested LAST if it is ready plate; else synthesize. */
    if (rr.last_print[0] && strstr(rr.last_print, "cubalc.ready.v1")) {
      printf("%s\n", rr.last_print);
    } else {
      printf("{\"schema\":\"cubalc.ready.v1\",\"ok\":%s,\"cmd\":\"ready\","
             "\"version\":\"%s\","
             "\"note\":\"install prove checklist · dual of READY\","
             "\"err\":\"%s\"}\n",
             ok ? "true" : "false", CUBALC_LANG_VERSION,
             ok ? "" : (rr.err[0] ? rr.err : (rr.last_err[0] ? rr.last_err : "not ready")));
    }
    return ok ? 0 : 1;
  }

  (void)argc;
    /* Usability: one JSON plate for agents/humans — is this install ready?
     * Also surfaces INCLUDE lib layout + nest-check readiness (plate_uniform). */
    char dir[512], protect_path[640], key_preview[16];
    int modular = 0, smx_key = 0, protect_plate = 0, bin_ok = 1;
    int libs_n = 0, lib_agent_boot = 0, lib_plate_session = 0, lib_plate_uniform = 0;
    int lib_hold_seed = 0, lib_var_guard = 0, lib_time_guard = 0, lib_fat_boot = 0;
    int lib_fat_session = 0;
    int lib_form_guard = 0;
    int lib_cap_boot = 0;
    int lib_onboard_boot = 0;
    int lib_discover_boot = 0;
    int lib_open_boot = 0;
    int lib_doctor_boot = 0;
    int lib_ready_boot = 0;
    int lib_cli_boot = 0;
    int lib_cli_guard = 0;
    int lib_cli_session = 0;
    int lib_env_guard = 0;
    int lib_env_boot = 0;
    int lib_arg_guard = 0;
    int lib_arg_boot = 0;
    int lib_time_boot = 0;
    int lib_tool_session = 0;
    int lib_product_session = 0;
    int lib_path_guard = 0, lib_path_boot = 0;
    int lib_rw_guard = 0, lib_rw_boot = 0;
    int lib_bin_guard = 0, lib_bin_boot = 0;
    int lib_lib_guard = 0, lib_lib_boot = 0;
    int lib_host_session = 0, lib_host_boot = 0;
    int lib_full_session = 0, lib_full_boot = 0;
    int lib_plate_guard = 0, lib_key_boot = 0;
    int lib_fn_guard = 0, lib_fn_boot = 0;
    int lib_class_guard = 0, lib_class_boot = 0;
    int lib_method_guard = 0, lib_method_boot = 0;
    int lib_field_guard = 0, lib_field_boot = 0;
    int lib_oop_session = 0, lib_oop_boot = 0;
    int lib_obj_guard = 0, lib_obj_boot = 0;
    int lib_live_session = 0, lib_live_boot = 0;
    int lib_service_session = 0, lib_service_boot = 0;
    int cookbook_ok = 0, for_agents_ok = 0, libdir_ok = 0;
    int include_path_set = 0, preload_set = 0;
    const char *hx = getenv("CUBALC_SMX_KEY");
    const char *kf = getenv("CUBALC_SMX_KEY_FILE");
    const char *ipath = getenv("CUBALC_INCLUDE_PATH");
    const char *preload = getenv("CUBALC_PRELOAD");
    const char *libdir = "programs/lib";
    state_dir(dir, sizeof dir);
    if (ipath && ipath[0]) include_path_set = 1;
    if (preload && preload[0]) preload_set = 1;
    snprintf(protect_path, sizeof protect_path, "%s/CORE_PROTECT.json", dir);
    modular = (access("src/lang/lang_parse.c", R_OK) == 0 &&
               access("include/lang/cubalc_lang_internal.h", R_OK) == 0 &&
               access("src/lang/lang_ops_smx.c", R_OK) == 0);
    if (hx && strlen(hx) >= 64) smx_key = 1;
    else if (kf && kf[0] && access(kf, R_OK) == 0) smx_key = 1;
    protect_plate = (access(protect_path, R_OK) == 0);
    key_preview[0] = 0;
    if (hx && strlen(hx) >= 8) {
      snprintf(key_preview, sizeof key_preview, "%.8s…", hx);
    }
    libdir_ok = (access(libdir, R_OK) == 0);
    lib_agent_boot = (access("programs/lib/agent_boot.cubalc", R_OK) == 0);
    lib_plate_session = (access("programs/lib/plate_session.cubalc", R_OK) == 0);
    lib_plate_uniform = (access("programs/lib/plate_uniform.cubalc", R_OK) == 0);
    lib_hold_seed = (access("programs/lib/hold_seed.cubalc", R_OK) == 0);
    lib_var_guard = (access("programs/lib/var_guard.cubalc", R_OK) == 0);
    lib_time_guard = (access("programs/lib/time_guard.cubalc", R_OK) == 0);
    lib_fat_boot = (access("programs/lib/fat_boot.cubalc", R_OK) == 0);
    lib_fat_session = (access("programs/lib/fat_session.cubalc", R_OK) == 0);
    lib_form_guard = (access("programs/lib/form_guard.cubalc", R_OK) == 0);
    lib_cap_boot = (access("programs/lib/cap_boot.cubalc", R_OK) == 0);
    lib_onboard_boot = (access("programs/lib/onboard_boot.cubalc", R_OK) == 0);
    lib_discover_boot = (access("programs/lib/discover_boot.cubalc", R_OK) == 0);
    lib_open_boot = (access("programs/lib/open_boot.cubalc", R_OK) == 0);
    lib_doctor_boot = (access("programs/lib/doctor_boot.cubalc", R_OK) == 0);
    lib_ready_boot = (access("programs/lib/ready_boot.cubalc", R_OK) == 0);
    lib_cli_boot = (access("programs/lib/cli_boot.cubalc", R_OK) == 0);
    lib_cli_guard = (access("programs/lib/cli_guard.cubalc", R_OK) == 0);
    lib_cli_session = (access("programs/lib/cli_session.cubalc", R_OK) == 0);
    lib_env_guard = (access("programs/lib/env_guard.cubalc", R_OK) == 0);
    lib_env_boot = (access("programs/lib/env_boot.cubalc", R_OK) == 0);
    lib_arg_guard = (access("programs/lib/arg_guard.cubalc", R_OK) == 0);
    lib_arg_boot = (access("programs/lib/arg_boot.cubalc", R_OK) == 0);
    lib_time_boot = (access("programs/lib/time_boot.cubalc", R_OK) == 0);
    lib_tool_session = (access("programs/lib/tool_session.cubalc", R_OK) == 0);
    lib_product_session = (access("programs/lib/product_session.cubalc", R_OK) == 0);
    lib_path_guard = (access("programs/lib/path_guard.cubalc", R_OK) == 0);
    lib_path_boot = (access("programs/lib/path_boot.cubalc", R_OK) == 0);
    lib_rw_guard = (access("programs/lib/rw_guard.cubalc", R_OK) == 0);
    lib_rw_boot = (access("programs/lib/rw_boot.cubalc", R_OK) == 0);
    lib_bin_guard = (access("programs/lib/bin_guard.cubalc", R_OK) == 0);
    lib_bin_boot = (access("programs/lib/bin_boot.cubalc", R_OK) == 0);
    lib_lib_guard = (access("programs/lib/lib_guard.cubalc", R_OK) == 0);
    lib_lib_boot = (access("programs/lib/lib_boot.cubalc", R_OK) == 0);
    lib_host_session = (access("programs/lib/host_session.cubalc", R_OK) == 0);
    lib_host_boot = (access("programs/lib/host_boot.cubalc", R_OK) == 0);
    lib_full_session = (access("programs/lib/full_session.cubalc", R_OK) == 0);
    lib_full_boot = (access("programs/lib/full_boot.cubalc", R_OK) == 0);
    lib_plate_guard = (access("programs/lib/plate_guard.cubalc", R_OK) == 0);
    lib_key_boot = (access("programs/lib/key_boot.cubalc", R_OK) == 0);
    lib_fn_guard = (access("programs/lib/fn_guard.cubalc", R_OK) == 0);
    lib_fn_boot = (access("programs/lib/fn_boot.cubalc", R_OK) == 0);
    lib_class_guard = (access("programs/lib/class_guard.cubalc", R_OK) == 0);
    lib_class_boot = (access("programs/lib/class_boot.cubalc", R_OK) == 0);
    lib_method_guard = (access("programs/lib/method_guard.cubalc", R_OK) == 0);
    lib_method_boot = (access("programs/lib/method_boot.cubalc", R_OK) == 0);
    lib_field_guard = (access("programs/lib/field_guard.cubalc", R_OK) == 0);
    lib_field_boot = (access("programs/lib/field_boot.cubalc", R_OK) == 0);
    lib_oop_session = (access("programs/lib/oop_session.cubalc", R_OK) == 0);
    lib_oop_boot = (access("programs/lib/oop_boot.cubalc", R_OK) == 0);
    lib_obj_guard = (access("programs/lib/obj_guard.cubalc", R_OK) == 0);
    lib_obj_boot = (access("programs/lib/obj_boot.cubalc", R_OK) == 0);
    lib_live_session = (access("programs/lib/live_session.cubalc", R_OK) == 0);
    lib_live_boot = (access("programs/lib/live_boot.cubalc", R_OK) == 0);
    lib_service_session = (access("programs/lib/service_session.cubalc", R_OK) == 0);
    lib_service_boot = (access("programs/lib/service_boot.cubalc", R_OK) == 0);
    cookbook_ok = (access("docs/COOKBOOK.md", R_OK) == 0);
    for_agents_ok = (access("docs/FOR_AGENTS.md", R_OK) == 0);
    if (libdir_ok) {
      DIR *d = opendir(libdir);
      if (d) {
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
          size_t len = strlen(de->d_name);
          if (len < 8 || strcmp(de->d_name + len - 7, ".cubalc") != 0)
            continue;
          if (de->d_name[0] == '.') continue;
          libs_n++;
        }
        closedir(d);
      }
    }
    {
      int ok = modular && (CUBALC_HOLD_FLASH == 1) && lib_agent_boot;
      printf("{\"schema\":\"cubalc.doctor.v1\",\"ok\":%s,"
             "\"version\":\"%s\",\"paradigm\":\"%s\",\"creed\":\"%s\","
             "\"hold_flash\":%d,\"hold_flash_means\":\"device_firmware_connection_safeguard\","
             "\"share\":\"%s\",\"http_required\":false,"
             "\"modular_lang\":%s,\"smx_key_configured\":%s,"
             "\"smx_key_preview\":\"%s\","
             "\"state_dir\":\"%s\",\"core_protect_plate\":%s,"
             "\"core_protect_path\":\"%s\","
             "\"bin_ok\":%s,"
             "\"libs_dir\":\"%s\",\"libs_dir_ok\":%s,\"libs_n\":%d,"
             "\"lib_agent_boot\":%s,\"lib_plate_session\":%s,"
             "\"lib_plate_uniform\":%s,\"lib_hold_seed\":%s,"
             "\"lib_var_guard\":%s,\"lib_time_guard\":%s,\"lib_fat_boot\":%s,"
             "\"lib_fat_session\":%s,\"lib_form_guard\":%s,\"lib_cap_boot\":%s,"
             "\"lib_onboard_boot\":%s,\"lib_discover_boot\":%s,\"lib_open_boot\":%s,\"lib_doctor_boot\":%s,"
             "\"lib_ready_boot\":%s,\"lib_cli_boot\":%s,\"lib_cli_guard\":%s,"
             "\"lib_cli_session\":%s,\"lib_env_guard\":%s,\"lib_env_boot\":%s,"
             "\"lib_arg_guard\":%s,\"lib_arg_boot\":%s,\"lib_time_boot\":%s,"
             "\"lib_tool_session\":%s,\"lib_product_session\":%s,"
             "\"lib_path_guard\":%s,\"lib_path_boot\":%s,"
             "\"lib_rw_guard\":%s,\"lib_rw_boot\":%s,"
             "\"lib_bin_guard\":%s,\"lib_bin_boot\":%s,"
             "\"lib_lib_guard\":%s,\"lib_lib_boot\":%s,"
             "\"lib_host_session\":%s,\"lib_host_boot\":%s,"
             "\"lib_full_session\":%s,\"lib_full_boot\":%s,"
             "\"lib_plate_guard\":%s,\"lib_key_boot\":%s,"
             "\"lib_fn_guard\":%s,\"lib_fn_boot\":%s,"
             "\"lib_class_guard\":%s,\"lib_class_boot\":%s,"
             "\"lib_method_guard\":%s,\"lib_method_boot\":%s,\"lib_field_guard\":%s,\"lib_field_boot\":%s,\"lib_oop_session\":%s,\"lib_oop_boot\":%s,\"lib_obj_guard\":%s,\"lib_obj_boot\":%s,\"lib_live_session\":%s,\"lib_live_boot\":%s,\"lib_service_session\":%s,\"lib_service_boot\":%s,"
             "\"include_path_set\":%s,\"preload_set\":%s,"
             "\"docs_cookbook\":%s,\"docs_for_agents\":%s,"
             "\"vars_max\":%d,\"varroom_forms\":true,"
             "\"run_timeout\":true,\"remain_ms_forms\":true,"
             "\"var_budget\":\"STATUS/VARS/run plate vars_n|max|full · "
             "VARROOM · INCLUDE fat_boot|fat_session|var_guard\","
             "\"nest_check\":\"%s\","
             "\"hints\":["
             "\"HOLD_FLASH device/mesh-join only · default 1 · not language PLUG\","
             "\"export CUBALC_SMX_KEY=$(openssl rand -hex 32) for P2P\","
             "\"cubalc protect · cubalc smx-bus prove-tcp\","
             "\"cubalc selftest — live usability proofs\","
             "\"cubalc onboard · INCLUDE onboard_boot · cubalc init --onboard\","
             "\"cubalc discover · INCLUDE discover_boot · cubalc init --discover\","
             "\"cubalc libs · cubalc guards · cubalc checkdeps fat_session · recipe · INCLUDE plate_uniform\","
             "\"CUBALC_INCLUDE_PATH + cubalc which name · run -I / NEEDINCLUDE\","
             "\"cubalc plate uniform agent.json role — nest value consistency\","
             "\"INCLUDE fat_session · cubalc init --fat-session durable nest (vars_max=%d)\","
             "\"cubalc run -T MS · INCLUDE time_guard · REMAIN_MS/NEEDTIME\","
             "\"INCLUDE form_guard · NEED_FORMS / NEED_FORM_ANY form contract\","
             "\"INCLUDE env_guard · NEED_ENVS / NEED_ENV_ANY host env contract\","
             "\"INCLUDE env_boot · cubalc init --env host env starter\","
             "\"INCLUDE arg_guard · NEED_ARGS / NEED_ARG_ANY program-arg contract\","
             "\"INCLUDE arg_boot · cubalc init --arg program-arg starter\","
             "\"INCLUDE time_boot · cubalc init --time wall-budget starter\","
             "\"INCLUDE tool_session · cubalc init --full-cli flags+args CLI\","
             "\"INCLUDE product_session · cubalc init --product flags+args+env+time\","
             "\"INCLUDE path_guard · NEED_PATHS / NEED_PATH_ANY host path contract\","
             "\"INCLUDE path_boot · cubalc init --path host path starter\","
             "\"INCLUDE rw_guard · NEED_READ_PATHS / NEED_WRITE_PATHS access contract\","
             "\"INCLUDE rw_boot · cubalc init --rw host access starter\","
             "\"INCLUDE bin_guard · NEED_BINS / NEED_BIN_ANY PATH tools\","
             "\"INCLUDE bin_boot · cubalc init --bin PATH tool starter\","
             "\"INCLUDE lib_guard · NEED_LIBS / NEED_LIB_ANY module contract\","
             "\"INCLUDE lib_boot · cubalc init --lib-boot stdlib gate starter\","
             "\"INCLUDE host_session · path+rw+bin+lib host readiness\","
             "\"INCLUDE host_boot · cubalc init --host host readiness starter\","
             "\"INCLUDE full_session · product+host FULL_SESSION_OK one-shot\","
             "\"INCLUDE full_boot · cubalc init --fleet product+host starter\","
             "\"INCLUDE plate_guard · NEED_KEYS / NEED_KEY_ANY plate schema\","
             "\"INCLUDE key_boot · cubalc init --key plate schema starter\","
             "\"INCLUDE fn_guard · NEED_FNALL / NEED_FNANY FN catalog\","
             "\"INCLUDE fn_boot · cubalc init --fn FN catalog starter\","
             "\"INCLUDE class_guard · NEED_CLASSES / NEED_CLASS_ANY OOP catalog\","
             "\"INCLUDE class_boot · cubalc init --class CLASS catalog starter\","
             "\"INCLUDE method_guard · NEED_METHODS / NEED_METHOD_ANY Class.method\","
             "\"INCLUDE method_boot · cubalc init --method METHOD catalog starter\","
             "\"INCLUDE field_guard · NEED_FIELDS / NEED_FIELD_ANY Class.field\","
             "\"INCLUDE field_boot · cubalc init --field FIELD catalog starter\","
             "\"INCLUDE oop_session · class+method+field OOP catalog\","
             "\"INCLUDE oop_boot · cubalc init --oop OOP catalog starter\","
             "\"INCLUDE obj_guard · NEED_OBJS / NEED_OBJ_ANY live instances\","
             "\"INCLUDE obj_boot · cubalc init --obj OBJ catalog starter\","
             "\"INCLUDE live_session · oop_session+obj_guard live OOP plane\","
             "\"INCLUDE live_boot · cubalc init --live live OOP starter\","
             "\"INCLUDE service_session · full_session+live_session fleet service\","
             "\"INCLUDE service_boot · cubalc init --service product+host+OOP\","
             "\"cubalc env · docs/COOKBOOK.md · docs/FOR_AGENTS.md\""
             "],"
             "\"cookbook\":[\"docs/COOKBOOK.md\",\"docs/P2P_SMX.md\","
             "\"docs/HOLD_FLASH.md\",\"docs/CORE_PROTECT.md\",\"docs/FOR_AGENTS.md\","
             "\"programs/hello_cube.cubalc\",\"programs/p2p/mesh_local.cubalc\","
             "\"programs/lib/plate_uniform.cubalc\",\"programs/lib/var_guard.cubalc\","
             "\"programs/lib/time_guard.cubalc\",\"programs/lib/fat_boot.cubalc\","
             "\"programs/lib/fat_session.cubalc\",\"programs/lib/form_guard.cubalc\","
             "\"programs/lib/env_guard.cubalc\",\"programs/lib/env_boot.cubalc\","
             "\"programs/lib/time_boot.cubalc\",\"programs/lib/tool_session.cubalc\","
             "\"programs/lib/product_session.cubalc\",\"programs/lib/path_guard.cubalc\","
             "\"programs/lib/path_boot.cubalc\",\"programs/lib/rw_guard.cubalc\","
             "\"programs/lib/rw_boot.cubalc\",\"programs/lib/bin_guard.cubalc\","
             "\"programs/lib/bin_boot.cubalc\",\"programs/lib/lib_guard.cubalc\","
             "\"programs/lib/lib_boot.cubalc\",\"programs/lib/host_session.cubalc\","
             "\"programs/lib/host_boot.cubalc\","
             "\"programs/lib/full_session.cubalc\",\"programs/lib/full_boot.cubalc\",\"programs/lib/plate_guard.cubalc\","
             "\"programs/lib/key_boot.cubalc\","
             "\"programs/lib/onboard_boot.cubalc\",\"programs/lib/discover_boot.cubalc\","
             "\"cubalc init --onboard\",\"cubalc init --discover\",\"cubalc init --fat-session\",\"cubalc hasforms SORTLIBS LIBAGE\"]"
             "}\n",
             ok ? "true" : "false",
             CUBALC_LANG_VERSION, CUBALC_LANG_PARADIGM, CUBALC_CREED,
             CUBALC_HOLD_FLASH, CUBALC_SHARE,
             modular ? "true" : "false",
             smx_key ? "true" : "false",
             key_preview,
             dir,
             protect_plate ? "true" : "false",
             protect_path,
             bin_ok ? "true" : "false",
             libdir,
             libdir_ok ? "true" : "false",
             libs_n,
             lib_agent_boot ? "true" : "false",
             lib_plate_session ? "true" : "false",
             lib_plate_uniform ? "true" : "false",
             lib_hold_seed ? "true" : "false",
             lib_var_guard ? "true" : "false",
             lib_time_guard ? "true" : "false",
             lib_fat_boot ? "true" : "false",
             lib_fat_session ? "true" : "false",
             lib_form_guard ? "true" : "false",
             lib_cap_boot ? "true" : "false",
             lib_onboard_boot ? "true" : "false",
             lib_discover_boot ? "true" : "false",
             lib_open_boot ? "true" : "false",
             lib_doctor_boot ? "true" : "false",
             lib_ready_boot ? "true" : "false",
             lib_cli_boot ? "true" : "false",
             lib_cli_guard ? "true" : "false",
             lib_cli_session ? "true" : "false",
             lib_env_guard ? "true" : "false",
             lib_env_boot ? "true" : "false",
             lib_arg_guard ? "true" : "false",
             lib_arg_boot ? "true" : "false",
             lib_time_boot ? "true" : "false",
             lib_tool_session ? "true" : "false",
             lib_product_session ? "true" : "false",
             lib_path_guard ? "true" : "false",
             lib_path_boot ? "true" : "false",
             lib_rw_guard ? "true" : "false",
             lib_rw_boot ? "true" : "false",
             lib_bin_guard ? "true" : "false",
             lib_bin_boot ? "true" : "false",
             lib_lib_guard ? "true" : "false",
             lib_lib_boot ? "true" : "false",
             lib_host_session ? "true" : "false",
             lib_host_boot ? "true" : "false",
             lib_full_session ? "true" : "false",
             lib_full_boot ? "true" : "false",
             lib_plate_guard ? "true" : "false",
             lib_key_boot ? "true" : "false",
             lib_fn_guard ? "true" : "false",
             lib_fn_boot ? "true" : "false",
             lib_class_guard ? "true" : "false",
             lib_class_boot ? "true" : "false",
             lib_method_guard ? "true" : "false",
             lib_method_boot ? "true" : "false",
             lib_field_guard ? "true" : "false",
             lib_field_boot ? "true" : "false",
             lib_oop_session ? "true" : "false",
             lib_oop_boot ? "true" : "false",
             lib_obj_guard ? "true" : "false",
             lib_obj_boot ? "true" : "false",
             lib_live_session ? "true" : "false",
             lib_live_boot ? "true" : "false",
             lib_service_session ? "true" : "false",
             lib_service_boot ? "true" : "false",
             include_path_set ? "true" : "false",
             preload_set ? "true" : "false",
             cookbook_ok ? "true" : "false",
             for_agents_ok ? "true" : "false",
             CUBALC_MAX_VARS,
             lib_plate_uniform
                 ? "INCLUDE plate_uniform · cubalc plate uniform"
                 : "missing programs/lib/plate_uniform.cubalc",
             CUBALC_MAX_VARS);
      return ok ? 0 : 1;
    }
}
