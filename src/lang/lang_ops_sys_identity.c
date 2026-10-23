/* CubalC lang — lang_ops_sys_identity.c
 * SYS identity residual: PID/PPID/HOSTNAME/USER/UID/GID/GROUPS/HOME/SHELL/GECOS (A2 peel).
 * Mesh vote A2_theme_peel_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_identity.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <pwd.h>
#  include <grp.h>
#endif

int cubalc_lang_ops_sys_try_identity(VM *vm, Lex *L) {
  /* SYS identity residual. Caller consumed SYS|HOST.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    /* SYS PID — process id → LAST_N/PID (peer identity without shell) */
    if (kw(&L->cur,"PID") || kw(&L->cur,"GETPID") || kw(&L->cur,"PROCESS_ID")){
      long n;
      char buf[32];
      lex_next(L);
#if defined(CUBALC_OS_WINDOWS)
      n = (long)_getpid();
#else
      n = (long)getpid();
#endif
      if (n < 0) n = 0;
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "PID", n);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", n);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS PPID|GETPPID|PARENT_PID — parent process id → LAST_N/PPID.
     * (PARENT alone is SYS DIRNAME path alias — do not steal it.)
     * SYS PGID|GETPGID|PGRP — process group id → LAST_N/PGID.
     * SYS SID|GETSID|SESSION — session id → LAST_N/SID.
     * Usability: process tree/session identity without shell ps/echo $PPID. */
    if (kw(&L->cur,"PPID") || kw(&L->cur,"GETPPID") ||
        kw(&L->cur,"PARENT_PID") || kw(&L->cur,"PARENTPID") ||
        kw(&L->cur,"PGID") || kw(&L->cur,"GETPGID") || kw(&L->cur,"PGRP") ||
        kw(&L->cur,"PROCESS_GROUP") || kw(&L->cur,"PROCGRP") ||
        kw(&L->cur,"SID") || kw(&L->cur,"GETSID") || kw(&L->cur,"SESSION") ||
        kw(&L->cur,"SESSION_ID") || kw(&L->cur,"SESSIONID")){
      char op[24];
      long n = 0;
      char buf[32];
      int which = 0; /* 0=ppid 1=pgid 2=sid */
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "PGID") == 0 || strcmp(op, "GETPGID") == 0 ||
          strcmp(op, "PGRP") == 0 || strcmp(op, "PROCESS_GROUP") == 0 ||
          strcmp(op, "PROCGRP") == 0)
        which = 1;
      else if (strcmp(op, "SID") == 0 || strcmp(op, "GETSID") == 0 ||
               strcmp(op, "SESSION") == 0 || strcmp(op, "SESSION_ID") == 0 ||
               strcmp(op, "SESSIONID") == 0)
        which = 2;
      lex_next(L);
#if defined(CUBALC_OS_WINDOWS)
      n = 0;
#else
      if (which == 0) {
        n = (long)getppid();
      } else if (which == 1) {
        pid_t g = getpgid(0);
        n = (g < 0) ? 0 : (long)g;
      } else {
        pid_t s = getsid(0);
        n = (s < 0) ? 0 : (long)s;
      }
#endif
      if (n < 0) n = 0;
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      if (which == 0) {
        var_set_num(vm, "PPID", n);
        var_set_num(vm, "PPID_N", n);
        var_set_num(vm, "PARENT", n);
      } else if (which == 1) {
        var_set_num(vm, "PGID", n);
        var_set_num(vm, "PGID_N", n);
        var_set_num(vm, "PGRP", n);
      } else {
        var_set_num(vm, "SID", n);
        var_set_num(vm, "SID_N", n);
        var_set_num(vm, "SESSION", n);
      }
      snprintf(buf, sizeof buf, "%ld", n);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS HOSTNAME|HOST — machine hostname → LAST/HOSTNAME */
    if (kw(&L->cur,"HOSTNAME") || kw(&L->cur,"HOST") || kw(&L->cur,"NODENAME")){
      char host[256];
      lex_next(L);
      host[0] = 0;
#if defined(CUBALC_OS_WINDOWS)
      {
        const char *e = getenv("COMPUTERNAME");
        if (e && e[0]) snprintf(host, sizeof host, "%s", e);
        else snprintf(host, sizeof host, "localhost");
      }
#else
      if (gethostname(host, sizeof host) != 0 || !host[0])
        snprintf(host, sizeof host, "localhost");
      host[sizeof host - 1] = 0;
#endif
      var_set_str(vm, "HOSTNAME", host);
      var_set_str(vm, "LAST", host);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", host);
      vm->last_n = (long)strlen(host);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS USER|USERNAME|LOGNAME — login name → LAST/USER (identity without shell)
     * Prefer USER/LOGNAME/USERNAME env, then getpwuid; fallback "user". */
    if (kw(&L->cur,"USER") || kw(&L->cur,"USERNAME") || kw(&L->cur,"LOGNAME") ||
        kw(&L->cur,"WHOAMI")){
      char user[128];
      const char *e;
      lex_next(L);
      user[0] = 0;
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
      var_set_str(vm, "USER", user);
      var_set_str(vm, "USERNAME", user);
      var_set_str(vm, "LAST", user);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", user);
      vm->last_n = (long)strlen(user);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS GETLOGIN|LOGINNAME|SESSION_USER — session login via getlogin() → LAST.
     * Prefer getlogin(); fallback USER/LOGNAME env then passwd name.
     * Usability: utmp/session identity distinct from $USER override without shell. */
    if (kw(&L->cur,"GETLOGIN") || kw(&L->cur,"LOGINNAME") || kw(&L->cur,"SESSION_USER") ||
        kw(&L->cur,"SESSIONUSER") || kw(&L->cur,"LOGIN_NAME") || kw(&L->cur,"UTMPUSER") ||
        kw(&L->cur,"TTYUSER") || kw(&L->cur,"WHO_AM_I")){
      char user[128];
      const char *e;
      lex_next(L);
      user[0] = 0;
#if !defined(CUBALC_OS_WINDOWS)
      {
        char *gl = getlogin();
        if (gl && gl[0])
          snprintf(user, sizeof user, "%s", gl);
      }
#endif
      if (!user[0]) {
        e = getenv("LOGNAME");
        if (!e || !e[0]) e = getenv("USER");
        if (!e || !e[0]) e = getenv("USERNAME");
        if (e && e[0]) snprintf(user, sizeof user, "%s", e);
      }
#if !defined(CUBALC_OS_WINDOWS)
      if (!user[0]) {
        struct passwd *pw = getpwuid(getuid());
        if (pw && pw->pw_name && pw->pw_name[0])
          snprintf(user, sizeof user, "%s", pw->pw_name);
      }
#endif
      if (!user[0]) snprintf(user, sizeof user, "user");
      var_set_str(vm, "GETLOGIN", user);
      var_set_str(vm, "LOGINNAME", user);
      var_set_str(vm, "SESSION_USER", user);
      var_set_str(vm, "LAST", user);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", user);
      vm->last_n = (long)strlen(user);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS UID|RUID|GETUID — real user id → LAST_N/UID.
     * SYS EUID|GETEUID|EFFECTIVE_UID — effective user id → LAST_N/EUID.
     * SYS ISROOT|AMROOT|ROOTPRIV — LAST_N 1 if euid==0 (privilege probe OK=1).
     * Usability: setuid-aware identity + agent root gates without shell id -u. */
    if (kw(&L->cur,"UID") || kw(&L->cur,"USER_ID") || kw(&L->cur,"USERID") ||
        kw(&L->cur,"RUID") || kw(&L->cur,"REAL_UID") || kw(&L->cur,"REALUID") ||
        kw(&L->cur,"GETUID") ||
        kw(&L->cur,"EUID") || kw(&L->cur,"GETEUID") || kw(&L->cur,"EFFECTIVE_UID") ||
        kw(&L->cur,"EFFECTIVEUID") ||
        kw(&L->cur,"ISROOT") || kw(&L->cur,"AMROOT") || kw(&L->cur,"ROOTPRIV") ||
        kw(&L->cur,"AS_ROOT")){
      char op[24];
      long n = 0;
      char buf[32];
      int want_euid = 0, want_root = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      want_root = (strcmp(op, "ISROOT") == 0 || strcmp(op, "AMROOT") == 0 ||
                   strcmp(op, "ROOTPRIV") == 0 || strcmp(op, "AS_ROOT") == 0);
      want_euid = (strcmp(op, "EUID") == 0 || strcmp(op, "GETEUID") == 0 ||
                   strcmp(op, "EFFECTIVE_UID") == 0 || strcmp(op, "EFFECTIVEUID") == 0);
      lex_next(L);
      if (want_root) {
#if defined(CUBALC_OS_WINDOWS)
        n = 0;
#else
        n = (geteuid() == 0) ? 1 : 0;
#endif
        vm->last_n = n;
        var_set_num(vm, "LAST_N", n);
        var_set_num(vm, "ISROOT", n);
        var_set_num(vm, "ISROOT_N", n);
        var_set_num(vm, "AMROOT", n);
        var_set_num(vm, "OK", 1);
        snprintf(buf, sizeof buf, "%ld", n);
        var_set_str(vm, "LAST", buf);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
        bump(vm); return 1;
      }
#if defined(CUBALC_OS_WINDOWS)
      {
        const char *e = getenv(want_euid ? "EUID" : "UID");
        if (e && e[0]) n = strtol(e, NULL, 10);
      }
#else
      n = want_euid ? (long)geteuid() : (long)getuid();
      if (n < 0) n = 0;
#endif
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      if (want_euid) {
        var_set_num(vm, "EUID", n);
        var_set_num(vm, "EUID_N", n);
      } else {
        var_set_num(vm, "UID", n);
        var_set_num(vm, "UID_N", n);
        var_set_num(vm, "RUID", n);
      }
      snprintf(buf, sizeof buf, "%ld", n);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS GID|GROUP_ID|GETGID — real group id → LAST_N/GID.
     * SYS EGID|EFFECTIVE_GID|GETEGID — effective group id → LAST_N/EGID.
     * SYS GROUP|GNAME|PRIMARYGROUP — primary group name → LAST/GROUP.
     * Usability: agent identity + plate ACL checks without shell id. */
    if (kw(&L->cur,"GID") || kw(&L->cur,"GROUP_ID") || kw(&L->cur,"GROUPID") ||
        kw(&L->cur,"GETGID") || kw(&L->cur,"RGID") ||
        kw(&L->cur,"EGID") || kw(&L->cur,"EFFECTIVE_GID") || kw(&L->cur,"GETEGID") ||
        kw(&L->cur,"EFFECTIVEGID") ||
        kw(&L->cur,"GROUP") || kw(&L->cur,"GNAME") || kw(&L->cur,"PRIMARYGROUP") ||
        kw(&L->cur,"PRIMARY_GROUP") || kw(&L->cur,"MYGROUP") || kw(&L->cur,"GROUPNAME_ME")){
      char op[24];
      int want_name, want_egid;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      want_name = (strcmp(op, "GROUP") == 0 || strcmp(op, "GNAME") == 0 ||
                   strcmp(op, "PRIMARYGROUP") == 0 || strcmp(op, "PRIMARY_GROUP") == 0 ||
                   strcmp(op, "MYGROUP") == 0 || strcmp(op, "GROUPNAME_ME") == 0);
      want_egid = (strcmp(op, "EGID") == 0 || strcmp(op, "EFFECTIVE_GID") == 0 ||
                   strcmp(op, "GETEGID") == 0 || strcmp(op, "EFFECTIVEGID") == 0);
      lex_next(L);
      if (want_name) {
        char gname[128];
        long gid = 0;
        gname[0] = 0;
#if defined(CUBALC_OS_WINDOWS)
        {
          const char *e = getenv("GROUP");
          if (e && e[0]) snprintf(gname, sizeof gname, "%s", e);
          else snprintf(gname, sizeof gname, "%s", "group");
        }
#else
        gid = (long)getegid();
        {
          struct group *gr = getgrgid((gid_t)gid);
          if (gr && gr->gr_name && gr->gr_name[0])
            snprintf(gname, sizeof gname, "%s", gr->gr_name);
          else
            snprintf(gname, sizeof gname, "%ld", gid);
        }
#endif
        if (!gname[0]) snprintf(gname, sizeof gname, "%s", "group");
        var_set_str(vm, "GROUP", gname);
        var_set_str(vm, "GNAME", gname);
        var_set_str(vm, "LAST", gname);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", gname);
        vm->last_n = (long)strlen(gname);
        var_set_num(vm, "LAST_N", vm->last_n);
        var_set_num(vm, "GID", gid);
        var_set_num(vm, "EGID", gid);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      {
        long n = 0;
        char buf[32];
#if defined(CUBALC_OS_WINDOWS)
        {
          const char *e = getenv(want_egid ? "EGID" : "GID");
          if (e && e[0]) n = strtol(e, NULL, 10);
        }
#else
        n = want_egid ? (long)getegid() : (long)getgid();
        if (n < 0) n = 0;
#endif
        vm->last_n = n;
        var_set_num(vm, "LAST_N", n);
        if (want_egid) {
          var_set_num(vm, "EGID", n);
          var_set_num(vm, "EGID_N", n);
        } else {
          var_set_num(vm, "GID", n);
          var_set_num(vm, "GID_N", n);
        }
        var_set_num(vm, "OK", 1);
        snprintf(buf, sizeof buf, "%ld", n);
        var_set_str(vm, "LAST", buf);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
        bump(vm); return 1;
      }
    }
    /* SYS GROUPS|GROUPLIST|MYGROUPS — egid+supplementary names → LAST bag; LAST_N=count.
     * SYS INGROUP|MEMBEROF|INGRP name|gid — LAST_N 1 if process in group (probe OK=1).
     * Usability: agent ACL membership without shell id -nG. */
    if (kw(&L->cur,"GROUPS") || kw(&L->cur,"GROUPLIST") || kw(&L->cur,"MYGROUPS") ||
        kw(&L->cur,"SUPGROUPS") || kw(&L->cur,"ALLGROUPS") ||
        kw(&L->cur,"INGROUP") || kw(&L->cur,"MEMBEROF") || kw(&L->cur,"INGRP") ||
        kw(&L->cur,"IN_GROUP") || kw(&L->cur,"HASGROUP") || kw(&L->cur,"GROUPMEMBER")){
      char op[24];
      cubalc_host_result hr;
      int is_in;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_in = (strcmp(op, "INGROUP") == 0 || strcmp(op, "MEMBEROF") == 0 ||
               strcmp(op, "INGRP") == 0 || strcmp(op, "IN_GROUP") == 0 ||
               strcmp(op, "HASGROUP") == 0 || strcmp(op, "GROUPMEMBER") == 0);
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      if (is_in) {
        char name[256];
        name[0] = 0;
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(name, sizeof name, "%ld", n);
        } else if (L->cur.kind == TK_IDENT) {
          Var *gv = var_get(vm, L->cur.text, 0);
          if (gv && !gv->is_str) {
            /* numeric var (incl. 0) → decimal gid text */
            snprintf(name, sizeof name, "%ld", gv->val);
            lex_next(L);
          } else if (gv && gv->is_str && gv->sval[0]) {
            snprintf(name, sizeof name, "%s", gv->sval);
            lex_next(L);
          } else {
            /* bare group name IDENT */
            snprintf(name, sizeof name, "%s", L->cur.text);
            lex_next(L);
          }
        } else if (resolve_str_arg(vm, L, name, sizeof name) != 0) {
          if (L->cur.kind == TK_STR) {
            snprintf(name, sizeof name, "%s", L->cur.text);
            lex_next(L);
          }
        }
        if (!name[0] || cubalc_host_ingroup(name, &hr) != 0) {
          var_set_str(vm, "LAST", "0");
          snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "INGROUP_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "INGROUP: empty name");
          var_set_str(vm, "ERR", "INGROUP: empty name");
          bump(vm); return 1;
        }
        var_set_str(vm, "LAST", hr.str);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
        vm->last_n = hr.n;
        var_set_num(vm, "LAST_N", hr.n);
        var_set_num(vm, "INGROUP", hr.n);
        var_set_num(vm, "INGROUP_N", hr.n);
        var_set_num(vm, "MEMBEROF", hr.n);
        var_set_str(vm, "INGROUP_NAME", name);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* GROUPS list */
      if (cubalc_host_groups(&hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "GROUPS_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "GROUPS_N", hr.n);
      var_set_str(vm, "GROUPS", hr.str);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS HOME|HOMEDIR|USER_HOME — home directory → LAST/HOME */
    if (kw(&L->cur,"HOME") || kw(&L->cur,"HOMEDIR") || kw(&L->cur,"USER_HOME") ||
        kw(&L->cur,"HOMEPATH")){
      char home[512];
      const char *e;
      lex_next(L);
      home[0] = 0;
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
      var_set_str(vm, "HOME", home);
      var_set_str(vm, "LAST", home);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", home);
      vm->last_n = (long)strlen(home);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SHELL|LOGINSHELL|USERSHELL — login shell path → LAST/SHELL.
     * Prefer $SHELL, then getpwuid()->pw_shell; fallback /bin/sh.
     * Usability: agent profile/SPAWN shell without shell echo $SHELL. */
    if (kw(&L->cur,"SHELL") || kw(&L->cur,"LOGINSHELL") || kw(&L->cur,"USERSHELL") ||
        kw(&L->cur,"USER_SHELL") || kw(&L->cur,"LOGIN_SHELL") || kw(&L->cur,"PSHELL")){
      char shell[512];
      const char *e;
      lex_next(L);
      shell[0] = 0;
      e = getenv("SHELL");
      if (e && e[0]) snprintf(shell, sizeof shell, "%s", e);
#if !defined(CUBALC_OS_WINDOWS)
      if (!shell[0]) {
        struct passwd *pw = getpwuid(getuid());
        if (pw && pw->pw_shell && pw->pw_shell[0])
          snprintf(shell, sizeof shell, "%s", pw->pw_shell);
      }
#endif
#if defined(CUBALC_OS_WINDOWS)
      if (!shell[0]) {
        e = getenv("COMSPEC");
        if (e && e[0]) snprintf(shell, sizeof shell, "%s", e);
        else snprintf(shell, sizeof shell, "%s", "cmd.exe");
      }
#else
      if (!shell[0]) snprintf(shell, sizeof shell, "%s", "/bin/sh");
#endif
      var_set_str(vm, "SHELL", shell);
      var_set_str(vm, "LOGINSHELL", shell);
      var_set_str(vm, "LAST", shell);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", shell);
      vm->last_n = (long)strlen(shell);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS GECOS|FULLNAME|REALNAME — passwd GECOS / display name → LAST.
     * Full gecos in LAST/GECOS; FULLNAME peels field before first comma.
     * Usability: human plate labels without shell getent/finger. */
    if (kw(&L->cur,"GECOS") || kw(&L->cur,"FULLNAME") || kw(&L->cur,"REALNAME") ||
        kw(&L->cur,"DISPLAYNAME") || kw(&L->cur,"CN") || kw(&L->cur,"GECOS_NAME") ||
        kw(&L->cur,"PW_GECOS") || kw(&L->cur,"COMMENT")){
      char op[24];
      char gecos[512];
      char full[256];
      int want_full;
      size_t i;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      want_full = (strcmp(op, "FULLNAME") == 0 || strcmp(op, "REALNAME") == 0 ||
                   strcmp(op, "DISPLAYNAME") == 0 || strcmp(op, "CN") == 0 ||
                   strcmp(op, "GECOS_NAME") == 0);
      lex_next(L);
      gecos[0] = 0;
      full[0] = 0;
#if !defined(CUBALC_OS_WINDOWS)
      {
        struct passwd *pw = getpwuid(getuid());
        if (pw && pw->pw_gecos && pw->pw_gecos[0])
          snprintf(gecos, sizeof gecos, "%s", pw->pw_gecos);
      }
#endif
      /* peel name before first comma (finger/GECOS convention) */
      for (i = 0; gecos[i] && gecos[i] != ',' && i + 1 < sizeof full; i++)
        full[i] = gecos[i];
      full[i] = 0;
      /* trim trailing spaces on full */
      while (i > 0 && (full[i - 1] == ' ' || full[i - 1] == '\t')) {
        i--;
        full[i] = 0;
      }
      if (!full[0] && gecos[0])
        snprintf(full, sizeof full, "%s", gecos);
      if (!gecos[0]) {
        /* soft fallback: USER env / login */
        const char *e = getenv("USER");
        if (!e || !e[0]) e = getenv("LOGNAME");
        if (!e || !e[0]) e = getenv("USERNAME");
        if (e && e[0]) {
          snprintf(gecos, sizeof gecos, "%s", e);
          snprintf(full, sizeof full, "%s", e);
        }
#if !defined(CUBALC_OS_WINDOWS)
        if (!gecos[0]) {
          struct passwd *pw = getpwuid(getuid());
          if (pw && pw->pw_name && pw->pw_name[0]) {
            snprintf(gecos, sizeof gecos, "%s", pw->pw_name);
            snprintf(full, sizeof full, "%s", pw->pw_name);
          }
        }
#endif
        if (!gecos[0]) {
          snprintf(gecos, sizeof gecos, "%s", "user");
          snprintf(full, sizeof full, "%s", "user");
        }
      }
      {
        const char *out = want_full ? full : gecos;
        var_set_str(vm, "GECOS", gecos);
        var_set_str(vm, "FULLNAME", full);
        var_set_str(vm, "REALNAME", full);
        var_set_str(vm, "LAST", out);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
        vm->last_n = (long)strlen(out);
        var_set_num(vm, "LAST_N", vm->last_n);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
    }

  return 0;
}
