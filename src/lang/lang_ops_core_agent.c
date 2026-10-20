/* CubalC lang — lang_ops_core_agent.c
 * Agent board plates: STATUS DOCTOR READY WHY IDENTITY CLIINFO (A_core peel).
 * Mesh vote A_core_forms_agent_peel · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_core_agent.h"
#include "lang/cubalc_lang_ops_core_help.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include "lang/cubalc_lang_ops_sys_codec.h"
#include "cubalc_hostops.h"
#include "cubalc_law.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <pwd.h>
#  include <sys/utsname.h>
#else
/* Windows: dirent may still be needed via compatibility; keep POSIX headers off. */
#endif

int cubalc_lang_ops_core_try_agent(VM *vm, Lex *L) {
  /* Agent status/identity/CLI board plane.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
  /* STATUS / HEALTH / AGENT_STATUS — one compact runtime health plate for agents.
   * Complements VARS (full table) and PRINT_JSON (named/snapshot): surfaces OK,
   * sticky LAST_ERR, version, wall time, hold, cubes — without guessing names.
   * Does not rewrite OK (reports current soft status). */
  if (kw(&L->cur,"STATUS")||kw(&L->cur,"HEALTH")||kw(&L->cur,"AGENT_STATUS")||
      kw(&L->cur,"STATUS_JSON")||kw(&L->cur,"PROBE_STATUS")){
    long okv = 1, exp_ok = 1, smx = 0, talks = 0;
    long tsec;
    const char *err = "";
    char esc[240];
    size_t eo = 0;
    char line[CUBALC_HOST_STR_MAX];
    char note[64];
    Var *vo, *ve, *vs, *vt, *vle;
    lex_next(L);
    vo = var_get(vm, "OK", 0);
    if (vo && !vo->is_str) okv = vo->val;
    ve = var_get(vm, "EXPECT_OK", 0);
    if (ve && !ve->is_str) exp_ok = ve->val;
    vs = var_get(vm, "SMX_OK", 0);
    smx = vs ? vs->val : (long)vm->smx_ok;
    vt = var_get(vm, "SMX_TALKS", 0);
    talks = vt ? vt->val : (long)vm->smx_talks;
    vle = var_get(vm, "LAST_ERR", 0);
    if (vle && vle->is_str && vle->sval[0]) err = vle->sval;
    else if (vm->err[0]) err = vm->err;
    for (const char *p = err; *p && eo + 2 < sizeof esc; p++) {
      if (*p == '"' || *p == '\\') { esc[eo++] = '\\'; esc[eo++] = *p; }
      else if ((unsigned char)*p < 0x20) continue;
      else esc[eo++] = *p;
    }
    esc[eo] = 0;
    tsec = (long)time(NULL);
    {
      long rem = cubalc_lang_timeout_remain_ms(vm);
      long tms = vm->run_timeout_ms > 0 ? vm->run_timeout_ms : 0L;
      long wall = 0;
      if (vm->run_start_ms > 0) {
        long now = cubalc_lang_mono_ms();
        if (now >= vm->run_start_ms)
          wall = now - vm->run_start_ms;
      }
      snprintf(line, sizeof line,
        "{\"schema\":\"cubalc.status.v1\",\"ok\":%s,\"last_err\":\"%s\","
        "\"version\":\"%s\",\"time\":%ld,\"n\":%d,\"hold\":%d,\"unity\":%ld,"
        "\"last_n\":%ld,\"expect_ok\":%ld,\"smx_ok\":%ld,\"smx_talks\":%ld,"
        "\"sp\":%d,\"stmts\":%d,\"vars_n\":%d,\"vars_max\":%d,\"vars_full\":%s,"
        "\"timeout_ms\":%ld,\"remain_ms\":%ld,\"wall_ms\":%ld}",
        okv ? "true" : "false", esc, CUBALC_LANG_VERSION, tsec,
        vm->ch.n_cubes, vm->hold_flash,
        (long)lround(vm->ch.unity * 100.0), vm->last_n, exp_ok, smx, talks,
        vm->sp, vm->res ? vm->res->stmts : 0,
        vm->n_vars, CUBALC_MAX_VARS, vm->vars_full ? "true" : "false",
        tms, rem < 0 ? -1L : rem, wall);
      var_set_num(vm, "TIMEOUT_MS", tms);
      var_set_num(vm, "REMAIN_MS", rem < 0 ? -1L : rem);
      var_set_num(vm, "WALL_MS", wall);
      var_set_num(vm, "ELAPSED_MS", wall);
    }
    if (vm->trace) fprintf(vm->trace, "%s\n", line);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", line);
    var_set_num(vm, "STATUS_OK", okv);
    var_set_num(vm, "TIME", tsec);
    /* Var-table pressure dual of VARS max/full — agents without full VARS dump. */
    var_set_num(vm, "VARS_N", (long)vm->n_vars);
    var_set_num(vm, "VARS_MAX", (long)CUBALC_MAX_VARS);
    var_set_num(vm, "VARS_FULL", vm->vars_full ? 1L : 0L);
    /* short LAST summary — full plate is last_print / trace JSON */
    if (okv)
      snprintf(note, sizeof note, "status:ok");
    else if (esc[0])
      snprintf(note, sizeof note, "status:err");
    else
      snprintf(note, sizeof note, "status:soft");
    var_set_str(vm, "LAST", note);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", note);
    vm->last_n = okv;
    var_set_num(vm, "LAST_N", okv);
    /* report-only: leave OK/EXPECT_OK/LAST_ERR as they were */
    if (vm->trace) fprintf(vm->trace, "# status ok=%ld err=%s\n", okv, esc[0] ? esc : "-");
    bump(vm); return 1;
  }


  /* DOCTOR|INSTALL_HEALTH — install readiness plate (dual of cubalc doctor).
   * Usability: agents gate INCLUDE/init without shelling cubalc doctor.
   * LAST=cubalc.doctor.v1 · DOCTOR_OK · DOCTOR_LIBS_N · key lib_* probes.
   * Soft always returns 1; OK follows readiness (modular+hold+agent_boot).
   * NEEDDOCTOR|REQUIRE DOCTOR — same plate then fail() if not ready (hard gate). */
  if (kw(&L->cur,"DOCTOR")||kw(&L->cur,"INSTALL_HEALTH")||kw(&L->cur,"INSTALLHEALTH")||
      kw(&L->cur,"DOCTOR_JSON")||kw(&L->cur,"HEALTHCHECK")||kw(&L->cur,"CHECK_INSTALL")||
      kw(&L->cur,"NEEDDOCTOR")||kw(&L->cur,"NEED_DOCTOR")||kw(&L->cur,"REQUIREDOCTOR")||
      kw(&L->cur,"REQUIRE_DOCTOR")||kw(&L->cur,"REQUIRE DOCTOR")||kw(&L->cur,"MUSTDOCTOR")){
    int hard = kw(&L->cur,"NEEDDOCTOR")||kw(&L->cur,"NEED_DOCTOR")||
               kw(&L->cur,"REQUIREDOCTOR")||kw(&L->cur,"REQUIRE_DOCTOR")||
               kw(&L->cur,"REQUIRE DOCTOR")||kw(&L->cur,"MUSTDOCTOR");
    char line[CUBALC_HOST_STR_MAX];
    char protect_path[640];
    const char *libdir = "programs/lib";
    int modular = 0, libdir_ok = 0, libs_n = 0;
    int lib_agent_boot = 0, lib_plate_session = 0, lib_plate_uniform = 0;
    int lib_hold_seed = 0, lib_var_guard = 0, lib_time_guard = 0;
    int lib_fat_boot = 0, lib_fat_session = 0, lib_form_guard = 0, lib_env_guard = 0, lib_env_boot = 0, lib_arg_guard = 0, lib_arg_boot = 0, lib_time_boot = 0;
    int lib_tool_session = 0, lib_product_session = 0;
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
    int lib_cap_boot = 0, lib_onboard_boot = 0, lib_discover_boot = 0, lib_open_boot = 0;
    int cookbook_ok = 0, for_agents_ok = 0;
    int include_path_set = 0, preload_set = 0, smx_key = 0, protect_plate = 0;
    int ok = 0, hold = CUBALC_HOLD_FLASH;
    const char *hx = getenv("CUBALC_SMX_KEY");
    const char *kf = getenv("CUBALC_SMX_KEY_FILE");
    const char *ipath = getenv("CUBALC_INCLUDE_PATH");
    const char *preload = getenv("CUBALC_PRELOAD");
    const char *state = NULL;
#if !defined(CUBALC_OS_WINDOWS)
    DIR *d;
    struct dirent *de;
#endif
    lex_next(L);
    if (ipath && ipath[0]) include_path_set = 1;
    if (preload && preload[0]) preload_set = 1;
    if (hx && strlen(hx) >= 64) smx_key = 1;
    else if (kf && kf[0] && access(kf, R_OK) == 0) smx_key = 1;
    modular = (access("src/lang/lang_parse.c", R_OK) == 0 &&
               access("include/lang/cubalc_lang_internal.h", R_OK) == 0 &&
               access("src/lang/lang_ops_smx.c", R_OK) == 0);
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
    lib_cap_boot = (access("programs/lib/cap_boot.cubalc", R_OK) == 0);
    lib_onboard_boot = (access("programs/lib/onboard_boot.cubalc", R_OK) == 0);
    lib_discover_boot = (access("programs/lib/discover_boot.cubalc", R_OK) == 0);
    lib_open_boot = (access("programs/lib/open_boot.cubalc", R_OK) == 0);
    cookbook_ok = (access("docs/COOKBOOK.md", R_OK) == 0);
    for_agents_ok = (access("docs/FOR_AGENTS.md", R_OK) == 0);
    state = "state";
    snprintf(protect_path, sizeof protect_path, "%s/CORE_PROTECT.json", state);
    protect_plate = (access(protect_path, R_OK) == 0);
#if !defined(CUBALC_OS_WINDOWS)
    if (libdir_ok) {
      d = opendir(libdir);
      if (d) {
        while ((de = readdir(d)) != NULL) {
          size_t len = strlen(de->d_name);
          if (len < 8 || strcmp(de->d_name + len - 7, ".cubalc") != 0) continue;
          if (de->d_name[0] == '.') continue;
          libs_n++;
        }
        closedir(d);
      }
    }
#else
    (void)libs_n;
#endif
    ok = modular && (hold == 1) && lib_agent_boot;
    snprintf(line, sizeof line,
      "{\"schema\":\"cubalc.doctor.v1\",\"ok\":%s,\"version\":\"%s\","
      "\"hold_flash\":%d,\"modular_lang\":%s,\"smx_key_configured\":%s,"
      "\"libs_dir_ok\":%s,\"libs_n\":%d,"
      "\"lib_agent_boot\":%s,\"lib_plate_session\":%s,\"lib_plate_uniform\":%s,"
      "\"lib_hold_seed\":%s,\"lib_var_guard\":%s,\"lib_time_guard\":%s,"
      "\"lib_fat_boot\":%s,\"lib_fat_session\":%s,\"lib_form_guard\":%s,"
      "\"lib_env_guard\":%s,\"lib_env_boot\":%s,\"lib_arg_guard\":%s,\"lib_arg_boot\":%s,\"lib_time_boot\":%s,"
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
      "\"lib_method_guard\":%s,\"lib_method_boot\":%s,"
      "\"lib_field_guard\":%s,\"lib_field_boot\":%s,"
      "\"lib_oop_session\":%s,\"lib_oop_boot\":%s,"
      "\"lib_obj_guard\":%s,\"lib_obj_boot\":%s,"
      "\"lib_live_session\":%s,\"lib_live_boot\":%s,"
      "\"lib_service_session\":%s,\"lib_service_boot\":%s,"
      "\"lib_cap_boot\":%s,\"lib_onboard_boot\":%s,\"lib_discover_boot\":%s,"
      "\"lib_open_boot\":%s,\"docs_cookbook\":%s,\"docs_for_agents\":%s,"
      "\"include_path_set\":%s,\"preload_set\":%s,\"core_protect_plate\":%s,"
      "\"vars_max\":%d,"
      "\"note\":\"install readiness · dual of cubalc doctor · gate INCLUDE/init\","
      "\"next\":\"INCLUDE open_boot · cubalc init --open · cubalc selftest\"}",
      ok ? "true" : "false", CUBALC_LANG_VERSION, hold,
      modular ? "true" : "false", smx_key ? "true" : "false",
      libdir_ok ? "true" : "false", libs_n,
      lib_agent_boot ? "true" : "false",
      lib_plate_session ? "true" : "false",
      lib_plate_uniform ? "true" : "false",
      lib_hold_seed ? "true" : "false",
      lib_var_guard ? "true" : "false",
      lib_time_guard ? "true" : "false",
      lib_fat_boot ? "true" : "false",
      lib_fat_session ? "true" : "false",
      lib_form_guard ? "true" : "false",
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
      lib_cap_boot ? "true" : "false",
      lib_onboard_boot ? "true" : "false",
      lib_discover_boot ? "true" : "false",
      lib_open_boot ? "true" : "false",
      cookbook_ok ? "true" : "false",
      for_agents_ok ? "true" : "false",
      include_path_set ? "true" : "false",
      preload_set ? "true" : "false",
      protect_plate ? "true" : "false",
      CUBALC_MAX_VARS);
    var_set_str(vm, "LAST", line);
    var_set_str(vm, "DOCTOR", line);
    var_set_str(vm, "INSTALL_HEALTH", line);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", line);
    vm->last_n = ok ? 1 : 0;
    var_set_num(vm, "LAST_N", ok ? 1 : 0);
    var_set_num(vm, "OK", ok ? 1 : 0);
    var_set_num(vm, "DOCTOR_OK", ok ? 1 : 0);
    var_set_num(vm, "DOCTOR_LIBS_N", (long)libs_n);
    var_set_num(vm, "DOCTOR_MODULAR", modular ? 1L : 0L);
    var_set_num(vm, "DOCTOR_HOLD", (long)hold);
    var_set_num(vm, "DOCTOR_AGENT_BOOT", lib_agent_boot ? 1L : 0L);
    var_set_num(vm, "DOCTOR_OPEN_BOOT", lib_open_boot ? 1L : 0L);
    var_set_num(vm, "DOCTOR_DISCOVER_BOOT", lib_discover_boot ? 1L : 0L);
    var_set_num(vm, "DOCTOR_SMX_KEY", smx_key ? 1L : 0L);
    if (!ok) {
      var_set_str(vm, "LAST_ERR",
                  "DOCTOR: not ready — need modular lang + HOLD_FLASH=1 + programs/lib/agent_boot");
      var_set_str(vm, "ERR",
                  "DOCTOR: not ready — need modular lang + HOLD_FLASH=1 + programs/lib/agent_boot");
    }
    if (hard && !ok) {
      fail_at(vm, L,
              "NEEDDOCTOR: install not ready — modular lang + HOLD_FLASH=1 + programs/lib/agent_boot · cubalc doctor");
      return -1;
    }
    if (vm->res)
      snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", "cubalc.doctor.v1");
    if (vm->trace)
      fprintf(vm->trace, "# doctor ok=%d libs_n=%d modular=%d hard=%d\n", ok, libs_n, modular, hard);
    bump(vm);
    return 1;
  }


  /* READY|PROVEREADY [soft] · NEEDREADY|REQUIRE READY —
   * Install prove checklist beyond soft DOCTOR: key forms + key libs + doctor ok.
   * Usability: agents prove surface after doctor_boot without cubalc selftest (heavy).
   * LAST=cubalc.ready.v1 · READY_OK · READY_PASS_N · READY_FAIL_N · READY_MISS bag. */
  if (kw(&L->cur,"READY")||kw(&L->cur,"PROVEREADY")||kw(&L->cur,"INSTALL_READY")||
      kw(&L->cur,"READYCHECK")||kw(&L->cur,"PROVE_INSTALL")||kw(&L->cur,"CHECK_READY")||
      kw(&L->cur,"NEEDREADY")||kw(&L->cur,"NEED_READY")||kw(&L->cur,"REQUIREREADY")||
      kw(&L->cur,"REQUIRE_READY")||kw(&L->cur,"REQUIRE READY")||kw(&L->cur,"MUSTREADY")){
    static const char *need_forms[] = {
      "DOCTOR", "NEEDDOCTOR", "OPEN", "DISCOVER", "HASLIB", "INCLUDE", "VERSION", "STATUS"
    };
    static const char *need_libs[] = {
      "agent_boot", "doctor_boot", "ready_boot", "open_boot", "discover_boot", "onboard_boot"
    };
    int hard = kw(&L->cur,"NEEDREADY")||kw(&L->cur,"NEED_READY")||
               kw(&L->cur,"REQUIREREADY")||kw(&L->cur,"REQUIRE_READY")||
               kw(&L->cur,"REQUIRE READY")||kw(&L->cur,"MUSTREADY");
    char line[CUBALC_HOST_STR_MAX];
    char miss[1024], pass_bag[1024], esc_miss[1200], esc_pass[1200];
    char path[128];
    const char *p;
    size_t o = 0, po = 0, eo = 0;
    int i, n_forms = (int)(sizeof need_forms / sizeof need_forms[0]);
    int n_libs = (int)(sizeof need_libs / sizeof need_libs[0]);
    int pass_n = 0, fail_n = 0;
    int modular = 0, hold_ok = 0, doctor_ok = 0, ready = 0;
    int hold = CUBALC_HOLD_FLASH;
    lex_next(L);
    miss[0] = 0;
    pass_bag[0] = 0;
    modular = (access("src/lang/lang_parse.c", R_OK) == 0 &&
               access("include/lang/cubalc_lang_internal.h", R_OK) == 0 &&
               access("src/lang/lang_ops_smx.c", R_OK) == 0);
    hold_ok = (hold == 1);
    doctor_ok = modular && hold_ok &&
                (access("programs/lib/agent_boot.cubalc", R_OK) == 0);
    /* structural checks */
    #define CUBALC_READY_PUSH(bag, bag_o, bag_sz, label, hit) do { \
      size_t __ln = strlen(label); \
      if (hit) { \
        if ((bag_o) && (bag_o) + 1 < (bag_sz)) (bag)[(bag_o)++] = '\n'; \
        if ((bag_o) + __ln < (bag_sz)) { memcpy((bag) + (bag_o), (label), __ln); (bag_o) += __ln; } \
        (bag)[(bag_o)] = 0; pass_n++; \
      } else { \
        if (o && o + 1 < sizeof miss) miss[o++] = '\n'; \
        if (o + __ln < sizeof miss) { memcpy(miss + o, (label), __ln); o += __ln; } \
        miss[o] = 0; fail_n++; \
      } \
    } while (0)
    CUBALC_READY_PUSH(pass_bag, po, sizeof pass_bag, "modular_lang", modular);
    CUBALC_READY_PUSH(pass_bag, po, sizeof pass_bag, "hold_flash", hold_ok);
    CUBALC_READY_PUSH(pass_bag, po, sizeof pass_bag, "doctor_ok", doctor_ok);
    for (i = 0; i < n_forms; i++) {
      int hit = cubalc_form_known(need_forms[i], NULL) ? 1 : 0;
      char lab[64];
      snprintf(lab, sizeof lab, "form:%s", need_forms[i]);
      CUBALC_READY_PUSH(pass_bag, po, sizeof pass_bag, lab, hit);
    }
    for (i = 0; i < n_libs; i++) {
      snprintf(path, sizeof path, "programs/lib/%s.cubalc", need_libs[i]);
      {
        int hit = (access(path, R_OK) == 0) ? 1 : 0;
        char lab[64];
        snprintf(lab, sizeof lab, "lib:%s", need_libs[i]);
        CUBALC_READY_PUSH(pass_bag, po, sizeof pass_bag, lab, hit);
      }
    }
    #undef CUBALC_READY_PUSH
    ready = (fail_n == 0) ? 1 : 0;
#define CUBALC_READY_ESC(dst, srcv) do { \
      eo = 0; \
      for (p = (srcv); *p && eo + 2 < sizeof(dst); p++) { \
        if (*p == '"' || *p == '\\') { (dst)[eo++] = '\\'; (dst)[eo++] = *p; } \
        else if (*p == '\n') { (dst)[eo++] = '\\'; (dst)[eo++] = 'n'; } \
        else if ((unsigned char)*p < 0x20) continue; \
        else (dst)[eo++] = *p; \
      } \
      (dst)[eo] = 0; \
    } while (0)
    CUBALC_READY_ESC(esc_miss, miss);
    CUBALC_READY_ESC(esc_pass, pass_bag);
#undef CUBALC_READY_ESC
    snprintf(line, sizeof line,
      "{\"schema\":\"cubalc.ready.v1\",\"ok\":%s,\"pass_n\":%d,\"fail_n\":%d,"
      "\"version\":\"%s\",\"doctor_ok\":%s,\"modular\":%s,\"hold_flash\":%d,"
      "\"pass\":\"%s\",\"miss\":\"%s\","
      "\"note\":\"install prove checklist · forms+libs+DOCTOR · dual of cubalc ready\"}",
      ready ? "true" : "false", pass_n, fail_n, CUBALC_LANG_VERSION,
      doctor_ok ? "true" : "false", modular ? "true" : "false", hold,
      esc_pass, esc_miss);
    var_set_str(vm, "LAST", line);
    var_set_str(vm, "READY", line);
    var_set_str(vm, "PROVEREADY", line);
    var_set_str(vm, "READY_MISS", miss);
    var_set_str(vm, "READY_PASS", pass_bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", line);
    vm->last_n = ready ? 1 : 0;
    var_set_num(vm, "LAST_N", ready ? 1 : 0);
    var_set_num(vm, "OK", ready ? 1 : 0);
    var_set_num(vm, "READY_OK", ready ? 1 : 0);
    var_set_num(vm, "READY_PASS_N", (long)pass_n);
    var_set_num(vm, "READY_FAIL_N", (long)fail_n);
    if (!ready) {
      char em[240];
      snprintf(em, sizeof em,
               "%s: %d check(s) failed — READY_MISS · DOCTOR · cubalc ready",
               hard ? "NEEDREADY" : "READY", fail_n);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      if (hard) {
        if (vm->res)
          snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", line);
        fail(vm, em);
        return -1;
      }
    }
    if (vm->res)
      snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", line);
    if (vm->trace)
      fprintf(vm->trace, "# ready ok=%d pass=%d fail=%d hard=%d\n", ready, pass_n, fail_n, hard);
    bump(vm);
    return 1;
  }

  /* WHY|EXPLAIN|WHYERR — agent recovery plate from sticky failure state.
   * Surfaces LAST_ERR + ASSERT_GOT/EXPECTED/OP + actionable WHY_HINT without
   * re-parsing free text. Does not rewrite OK. Complements STATUS/CLEAR_ERR.
   *   EXPECT x == 1
   *   WHY
   *   # LAST = cubalc.why.v1 · WHY_HINT = recovery tip
   */
  if (kw(&L->cur,"WHY") || kw(&L->cur,"EXPLAIN") || kw(&L->cur,"WHYERR") ||
      kw(&L->cur,"EXPLAIN_ERR") || kw(&L->cur,"WHY_ERR") || kw(&L->cur,"DIAGNOSE") ||
      kw(&L->cur,"RECOVERY") || kw(&L->cur,"WHAT_FAILED")) {
    long okv = 1;
    const char *err = "", *got = "", *exp = "", *op = "", *hint;
    char esc_err[240], esc_got[120], esc_exp[120], esc_op[48], esc_hint[280];
    char line[CUBALC_HOST_STR_MAX];
    char hintbuf[256];
    size_t eo;
    Var *vo, *vle, *vg, *ve, *vop;
    const char *p;

    lex_next(L);
    vo = var_get(vm, "OK", 0);
    if (vo && !vo->is_str) okv = vo->val;
    vle = var_get(vm, "LAST_ERR", 0);
    if (vle && vle->is_str && vle->sval[0]) err = vle->sval;
    else if (vm->err[0]) err = vm->err;
    else {
      Var *ve2 = var_get(vm, "ERR", 0);
      if (ve2 && ve2->is_str && ve2->sval[0]) err = ve2->sval;
    }
    vg = var_get(vm, "ASSERT_GOT", 0);
    if (vg && vg->is_str) got = vg->sval;
    ve = var_get(vm, "ASSERT_EXPECTED", 0);
    if (ve && ve->is_str) exp = ve->sval;
    vop = var_get(vm, "ASSERT_OP", 0);
    if (vop && vop->is_str) op = vop->sval;

    /* Actionable recovery tips from sticky error text */
    if (!err[0] && okv)
      snprintf(hintbuf, sizeof hintbuf, "ok — no sticky LAST_ERR");
    else if (strstr(err, "NEEDMATCHLIBS") || strstr(err, "HASMATCHLIBS") ||
             strstr(err, "NEEDCOUNTMATCHLIBS") || strstr(err, "HASCOUNTMATCHLIBS") ||
             strstr(err, "COUNTMATCHLIBS") ||
             strstr(err, "NTHLIB") || strstr(err, "LASTLIB") ||
             strstr(err, "PICKLIB") || strstr(err, "NEWESTLIB") || strstr(err, "OLDESTLIB") ||
             strstr(err, "SORTLIBS") || strstr(err, "LIBSORT"))
      snprintf(hintbuf, sizeof hintbuf,
               "MATCHLIBS · SORTLIBS · NEWESTLIB/OLDESTLIB · cubalc libs [filter] · picklib");
    else if (strstr(err, "NEEDDEPS") || strstr(err, "HASDEPS") ||
             strstr(err, "CHECKDEPS") || strstr(err, "DEPS_MISS"))
      snprintf(hintbuf, sizeof hintbuf,
               "CHECKDEPS/HASDEPS · cubalc libs · cubalc recipe · install missing deps");
    else if (strstr(err, "INCLUDE") || strstr(err, "include") ||
             strstr(err, "REQUIRE LIB") || strstr(err, "lib missing"))
      snprintf(hintbuf, sizeof hintbuf,
               "cubalc libs · REQUIRE LIB name · INCLUDE SOFT · cubalc which name");
    else if (strstr(err, "ASSERT") || strstr(err, "EXPECT") ||
             strstr(err, "is false") || strstr(err, "falsey"))
      snprintf(hintbuf, sizeof hintbuf,
               "ASSERT_GOT/ASSERT_EXPECTED · EXPECT soft · CLEAR_ERR after fix");
    else if (strstr(err, "NEEDP") || strstr(err, "NEEDFLAT") ||
             strstr(err, "missing") || strstr(err, "NEED "))
      snprintf(hintbuf, sizeof hintbuf,
               "NEEDP/NEEDFLAT · plate has/need · DEFAULTP seed · cubalc plate need");
    else if (strstr(err, "VERSION") || strstr(err, "version") ||
             strstr(err, "too old"))
      snprintf(hintbuf, sizeof hintbuf,
               "REQUIRE VERSION floor · cubalc version · upgrade runtime");
    else if (strstr(err, "DIAL") || strstr(err, "SERVE") || strstr(err, "SMX") ||
             strstr(err, "TALK") || strstr(err, "P2P") || strstr(err, "timeout"))
      snprintf(hintbuf, sizeof hintbuf,
               "CUBALC_P2P_SOFT=1 · cubalc env · CUBALC_P2P_TIMEOUT ms");
    else if (strstr(err, "HOLD_FLASH") || strstr(err, "PLUG") ||
             strstr(err, "hold_flash"))
      snprintf(hintbuf, sizeof hintbuf,
               "HOLD_FLASH device/mesh-join only · docs/HOLD_FLASH.md");
    else if (strstr(err, "unknown form") || strstr(err, "did you mean") ||
             strstr(err, "unknown"))
      snprintf(hintbuf, sizeof hintbuf,
               "HELP form · cubalc forms prefix · cubalc search keyword");
    else if (strstr(err, "cannot open") || strstr(err, "ENOENT") ||
             strstr(err, "No such") || strstr(err, "path"))
      snprintf(hintbuf, sizeof hintbuf,
               "cubalc paths · SYS EXIST · REQUIRE PATH · check CUBALC_STATE");
    else if (strstr(err, "UNIFORM") || strstr(err, "roles must"))
      snprintf(hintbuf, sizeof hintbuf,
               "UNIFORMFLAT · UNEQPATHS · SETBYVAL · cubalc plate uniform");
    else if (!err[0])
      snprintf(hintbuf, sizeof hintbuf,
               "OK soft=0 without LAST_ERR — STATUS · VARS · FAIL/EXPECT probe");
    else
      snprintf(hintbuf, sizeof hintbuf,
               "STATUS · CLEAR_ERR after fix · cubalc doctor · docs/COOKBOOK.md");
    hint = hintbuf;

    eo = 0;
    for (p = err; *p && eo + 2 < sizeof esc_err; p++) {
      if (*p == '"' || *p == '\\') { esc_err[eo++] = '\\'; esc_err[eo++] = *p; }
      else if ((unsigned char)*p < 0x20) continue;
      else esc_err[eo++] = *p;
    }
    esc_err[eo] = 0;
    eo = 0;
    for (p = got; *p && eo + 2 < sizeof esc_got; p++) {
      if (*p == '"' || *p == '\\') { esc_got[eo++] = '\\'; esc_got[eo++] = *p; }
      else if ((unsigned char)*p < 0x20) continue;
      else esc_got[eo++] = *p;
    }
    esc_got[eo] = 0;
    eo = 0;
    for (p = exp; *p && eo + 2 < sizeof esc_exp; p++) {
      if (*p == '"' || *p == '\\') { esc_exp[eo++] = '\\'; esc_exp[eo++] = *p; }
      else if ((unsigned char)*p < 0x20) continue;
      else esc_exp[eo++] = *p;
    }
    esc_exp[eo] = 0;
    eo = 0;
    for (p = op; *p && eo + 2 < sizeof esc_op; p++) {
      if (*p == '"' || *p == '\\') { esc_op[eo++] = '\\'; esc_op[eo++] = *p; }
      else if ((unsigned char)*p < 0x20) continue;
      else esc_op[eo++] = *p;
    }
    esc_op[eo] = 0;
    eo = 0;
    for (p = hint; *p && eo + 2 < sizeof esc_hint; p++) {
      if (*p == '"' || *p == '\\') { esc_hint[eo++] = '\\'; esc_hint[eo++] = *p; }
      else if ((unsigned char)*p < 0x20) continue;
      else esc_hint[eo++] = *p;
    }
    esc_hint[eo] = 0;

    snprintf(line, sizeof line,
      "{\"schema\":\"cubalc.why.v1\",\"ok\":%s,\"has_err\":%s,"
      "\"last_err\":\"%s\",\"assert_got\":\"%s\",\"assert_expected\":\"%s\","
      "\"assert_op\":\"%s\",\"hint\":\"%s\",\"version\":\"%s\"}",
      okv ? "true" : "false", err[0] ? "true" : "false",
      esc_err, esc_got, esc_exp, esc_op, esc_hint, CUBALC_LANG_VERSION);
    if (vm->trace) fprintf(vm->trace, "%s\n", line);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", line);

    var_set_str(vm, "WHY", line);
    var_set_str(vm, "EXPLAIN", line);
    var_set_str(vm, "WHY_HINT", hint);
    var_set_str(vm, "EXPLAIN_HINT", hint);
    var_set_num(vm, "WHY_HAS_ERR", err[0] ? 1 : 0);
    var_set_num(vm, "LAST_N", err[0] ? 1 : 0);
    vm->last_n = err[0] ? 1 : 0;
    /* LAST = short hint for agents that only read LAST string */
    var_set_str(vm, "LAST", hint);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hint);
    /* report-only: leave OK/LAST_ERR intact */
    if (vm->trace)
      fprintf(vm->trace, "# why has_err=%d hint=%s\n", err[0] ? 1 : 0, hint);
    bump(vm); return 1;
  }

  /* IDENTITY / WHOAMI_PLATE — one host/process identity plate for agents.
   * Fills PID HOSTNAME USER UID HOME CWD VERSION without chaining SYS forms.
   * Full JSON → last_print; short LAST "user@host:pid". */
  if (kw(&L->cur,"IDENTITY")||kw(&L->cur,"WHO")||kw(&L->cur,"HOST_IDENTITY")||
      kw(&L->cur,"ID_PLATE")||kw(&L->cur,"PEER_ID")||kw(&L->cur,"WHO_AM_I")){
    long pid = 0, uid = 0;
    char host[256], user[128], home[512], cwd[512];
    char eh[280], eu[160], ehome[560], ecwd[560];
    char line[CUBALC_HOST_STR_MAX];
    char note[200];
    const char *e;
    size_t eo;
    const char *p;
    lex_next(L);
    host[0] = user[0] = home[0] = cwd[0] = 0;
#if defined(CUBALC_OS_WINDOWS)
    pid = (long)_getpid();
    {
      const char *ce = getenv("COMPUTERNAME");
      if (ce && ce[0]) snprintf(host, sizeof host, "%s", ce);
      else snprintf(host, sizeof host, "localhost");
    }
    e = getenv("UID");
    if (e && e[0]) uid = strtol(e, NULL, 10);
#else
    pid = (long)getpid();
    if (pid < 0) pid = 0;
    uid = (long)getuid();
    if (uid < 0) uid = 0;
    if (gethostname(host, sizeof host) != 0 || !host[0])
      snprintf(host, sizeof host, "localhost");
    host[sizeof host - 1] = 0;
#endif
    e = getenv("USER");
    if (!e || !e[0]) e = getenv("LOGNAME");
    if (!e || !e[0]) e = getenv("USERNAME");
    if (e && e[0]) snprintf(user, sizeof user, "%s", e);
#if !defined(CUBALC_OS_WINDOWS)
    if (!user[0]) {
      struct passwd *pw = getpwuid(getuid());
      if (pw && pw->pw_name && pw->pw_name[0])
        snprintf(user, sizeof user, "%s", pw->pw_name);
    }
#endif
    if (!user[0]) snprintf(user, sizeof user, "user");
    e = getenv("HOME");
#if defined(CUBALC_OS_WINDOWS)
    if (!e || !e[0]) e = getenv("USERPROFILE");
#endif
    if (e && e[0]) snprintf(home, sizeof home, "%s", e);
#if !defined(CUBALC_OS_WINDOWS)
    if (!home[0]) {
      struct passwd *pw = getpwuid(getuid());
      if (pw && pw->pw_dir && pw->pw_dir[0])
        snprintf(home, sizeof home, "%s", pw->pw_dir);
    }
#endif
    if (!home[0]) {
      if (getcwd(home, sizeof home) == NULL)
        snprintf(home, sizeof home, ".");
    }
    if (getcwd(cwd, sizeof cwd) == NULL)
      snprintf(cwd, sizeof cwd, ".");
    /* JSON-escape helper fields */
#define CUBALC_ID_ESC(dst, src) do { \
      eo = 0; \
      for (p = (src); *p && eo + 2 < sizeof(dst); p++) { \
        if (*p == '"' || *p == '\\') { (dst)[eo++] = '\\'; (dst)[eo++] = *p; } \
        else if ((unsigned char)*p < 0x20) continue; \
        else (dst)[eo++] = *p; \
      } \
      (dst)[eo] = 0; \
    } while (0)
    CUBALC_ID_ESC(eh, host);
    CUBALC_ID_ESC(eu, user);
    CUBALC_ID_ESC(ehome, home);
    CUBALC_ID_ESC(ecwd, cwd);
#undef CUBALC_ID_ESC
    var_set_num(vm, "PID", pid);
    var_set_num(vm, "UID", uid);
    var_set_str(vm, "HOSTNAME", host);
    var_set_str(vm, "USER", user);
    var_set_str(vm, "USERNAME", user);
    var_set_str(vm, "HOME", home);
    var_set_str(vm, "CWD", cwd);
    var_set_str(vm, "VERSION", CUBALC_LANG_VERSION);
    snprintf(line, sizeof line,
      "{\"schema\":\"cubalc.identity.v1\",\"ok\":true,"
      "\"pid\":%ld,\"uid\":%ld,\"user\":\"%s\",\"hostname\":\"%s\","
      "\"home\":\"%s\",\"cwd\":\"%s\",\"version\":\"%s\"}",
      pid, uid, eu, eh, ehome, ecwd, CUBALC_LANG_VERSION);
    if (vm->trace) fprintf(vm->trace, "%s\n", line);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", line);
    snprintf(note, sizeof note, "%s@%s:%ld", user, host, pid);
    var_set_str(vm, "IDENTITY", note);
    var_set_str(vm, "LAST", note);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", note);
    vm->last_n = pid;
    var_set_num(vm, "LAST_N", pid);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# identity %s\n", note);
    bump(vm); return 1;
  }
  /* CLIINFO|DUMPCLI|CLISNAP — one-shot program CLI surface plate for agents.
   * Collects ARGC · LISTFLAGS · FLAGMAP · RESTARGS without multi-form glue.
   * LAST = cubalc.cli.v1 JSON · LAST_N = argc · bags stashed in named vars.
   * Usability: inspect argv after run --flags files without shell getopt. */
  if (kw(&L->cur,"CLIINFO") || kw(&L->cur,"DUMPCLI") || kw(&L->cur,"CLISNAP") ||
      kw(&L->cur,"CLI_INFO") || kw(&L->cur,"CLI_STATUS") || kw(&L->cur,"ARGVINFO") ||
      kw(&L->cur,"CLI_PLATE") || kw(&L->cur,"DUMPARGS")) {
    char flags[CUBALC_HOST_STR_MAX], fmap[CUBALC_HOST_STR_MAX], rest[CUBALC_HOST_STR_MAX];
    char args_bag[CUBALC_HOST_STR_MAX];
    char eflags[CUBALC_HOST_STR_MAX], efmap[CUBALC_HOST_STR_MAX], erest[CUBALC_HOST_STR_MAX];
    char eargs[CUBALC_HOST_STR_MAX];
    char line[CUBALC_HOST_STR_MAX];
    char note[160];
    const char *ac, *p;
    long argc = 0, nflags = 0, nmap = 0, nrest = 0;
    size_t eo, o;
    int k;
    char envn[32];
    lex_next(L);
    flags[0] = fmap[0] = rest[0] = args_bag[0] = 0;
    ac = getenv("CUBALC_ARGC");
    if (ac && ac[0]) {
      argc = strtol(ac, NULL, 10);
      if (argc < 0) argc = 0;
      if (argc > 32) argc = 32;
    } else {
      for (k = 0; k < 32; k++) {
        snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
        if (!getenv(envn)) break;
        argc++;
      }
    }
    /* raw argv bag */
    o = 0;
    for (k = 0; k < (int)argc; k++) {
      const char *a;
      size_t ln, room;
      snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
      a = getenv(envn);
      if (!a) a = "";
      if (o && o + 1 < sizeof args_bag) args_bag[o++] = '\n';
      ln = strlen(a);
      room = sizeof args_bag - o - 1;
      if (ln > room) ln = room;
      if (ln > 0) {
        memcpy(args_bag + o, a, ln);
        o += ln;
      }
      args_bag[o] = 0;
    }
    nflags = cubalc_collect_listflags(flags, sizeof flags);
    nmap = cubalc_collect_flagmap(fmap, sizeof fmap);
    nrest = cubalc_collect_restargs(rest, sizeof rest);
#define CUBALC_CLI_ESC(dst, src) do { \
      eo = 0; \
      for (p = (src); *p && eo + 2 < sizeof(dst); p++) { \
        if (*p == '"' || *p == '\\') { (dst)[eo++] = '\\'; (dst)[eo++] = *p; } \
        else if (*p == '\n') { (dst)[eo++] = '\\'; (dst)[eo++] = 'n'; } \
        else if ((unsigned char)*p < 0x20) continue; \
        else (dst)[eo++] = *p; \
      } \
      (dst)[eo] = 0; \
    } while (0)
    CUBALC_CLI_ESC(eflags, flags);
    CUBALC_CLI_ESC(efmap, fmap);
    CUBALC_CLI_ESC(erest, rest);
    CUBALC_CLI_ESC(eargs, args_bag);
#undef CUBALC_CLI_ESC
    snprintf(line, sizeof line,
      "{\"schema\":\"cubalc.cli.v1\",\"ok\":true,"
      "\"argc\":%ld,\"flags_n\":%ld,\"flagmap_n\":%ld,\"restargs_n\":%ld,"
      "\"version\":\"%s\","
      "\"flags\":\"%s\",\"flagmap\":\"%s\",\"restargs\":\"%s\",\"args\":\"%s\","
      "\"note\":\"CLI surface one-shot · dual of cubalc cliinfo · LISTFLAGS+FLAGMAP+RESTARGS\"}",
      argc, nflags, nmap, nrest, CUBALC_LANG_VERSION,
      eflags, efmap, erest, eargs);
    snprintf(note, sizeof note, "argc=%ld flags=%ld rest=%ld", argc, nflags, nrest);
    /* LAST = full plate for agents; short note in CLIINFO (like IDENTITY) so
     * HASFORM CLIINFO is not polluted by plate-as-var form-name lookup. */
    var_set_str(vm, "LAST", line);
    var_set_str(vm, "CLIINFO_PLATE", line);
    var_set_str(vm, "DUMPCLI_PLATE", line);
    var_set_str(vm, "CLISNAP", line);
    /* do not bind CLIINFO/DUMPCLI str vars — HASFORM CLIINFO would use plate/short as form name */
    var_set_str(vm, "CLIINFO_SHORT", note);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", line);
    var_set_str(vm, "LISTFLAGS", flags);
    var_set_str(vm, "FLAGS", flags);
    var_set_str(vm, "FLAGMAP", fmap);
    var_set_str(vm, "FLAGKV", fmap);
    var_set_str(vm, "RESTARGS", rest);
    var_set_str(vm, "POSITIONALS", rest);
    var_set_str(vm, "ARGS", args_bag);
    var_set_num(vm, "ARGC", argc);
    var_set_num(vm, "LISTFLAGS_N", nflags);
    var_set_num(vm, "FLAGMAP_N", nmap);
    var_set_num(vm, "RESTARGS_N", nrest);
    var_set_num(vm, "CLIINFO_FLAGS_N", nflags);
    var_set_num(vm, "CLIINFO_REST_N", nrest);
    vm->last_n = argc;
    var_set_num(vm, "LAST_N", argc);
    var_set_num(vm, "OK", 1);
    if (vm->res)
      snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", line);
    if (vm->trace)
      fprintf(vm->trace, "# cliinfo %s\n", note);
    bump(vm);
    return 1;
  }
  return 0;
}
