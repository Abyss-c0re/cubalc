/* CubalC lang — lang_ops_sys_hostmeta.c
 * SYS host-meta residual: NPROC/ISATTY/TTYNAME/NICE/LOADAVG/UPTIME/MEM/DF/UNAME/ARCH (A2 peel).
 * Mesh vote A2_theme_peel_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_hostmeta.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <sys/stat.h>
#  include <sys/statvfs.h>
#  include <sys/resource.h>
#  include <sys/utsname.h>
/* getloadavg is BSD/POSIX extension; declare if libc omits under strict feature macros. */
int getloadavg(double loadavg[], int nelem);
#endif

int cubalc_lang_ops_sys_try_hostmeta(VM *vm, Lex *L) {
  /* SYS host-meta residual. Caller consumed SYS|HOST.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    /* SYS NPROC|CPUS|PROCESSORS|CORES|NCPU — online processor count → LAST_N.
     * LAST/NPROC = decimal string; NPROC/CPUS/CORES nums mirror LAST_N.
     * Usability: agent STRIDE/CHUNK worker partition without shell nproc. */
    if (kw(&L->cur,"NPROC") || kw(&L->cur,"CPUS") || kw(&L->cur,"PROCESSORS") ||
        kw(&L->cur,"CORES") || kw(&L->cur,"NCPU") || kw(&L->cur,"NUM_CPUS") ||
        kw(&L->cur,"CPU_COUNT") || kw(&L->cur,"NPROCESSORS") ||
        kw(&L->cur,"NUMCPUS") || kw(&L->cur,"CPUCOUNT")){
      long n = 1;
      char buf[32];
      lex_next(L);
#if defined(_SC_NPROCESSORS_ONLN)
      {
        long c = sysconf(_SC_NPROCESSORS_ONLN);
        if (c > 0) n = c;
      }
#elif defined(CUBALC_OS_WINDOWS)
      {
        const char *e = getenv("NUMBER_OF_PROCESSORS");
        if (e && e[0]) {
          long c = strtol(e, NULL, 10);
          if (c > 0) n = c;
        }
      }
#endif
      if (n < 1) n = 1;
      if (n > 65536) n = 65536; /* sanity cap */
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "NPROC", n);
      var_set_num(vm, "CPUS", n);
      var_set_num(vm, "CORES", n);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", n);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS ISATTY|TTY [IN|OUT|ERR|STDIN|STDOUT|STDERR] — terminal probe → LAST_N 0|1.
     * Default stream: OUT/stdout. Sets ISATTY + stream-specific var.
     * Usability: agents skip prompts/color/interactive paths when non-TTY (pipes). */
    if (kw(&L->cur,"ISATTY") || kw(&L->cur,"TTY") || kw(&L->cur,"ISTTY") ||
        kw(&L->cur,"IS_TTY") || kw(&L->cur,"TERMINAL") || kw(&L->cur,"IS_TERMINAL")){
      int fd = 1; /* stdout default */
      const char *stream = "OUT";
      long n = 0;
      char buf[8];
      lex_next(L);
      if (kw(&L->cur,"IN") || kw(&L->cur,"STDIN") || kw(&L->cur,"0") ||
          kw(&L->cur,"INPUT")){
        fd = 0; stream = "IN";
        lex_next(L);
      } else if (kw(&L->cur,"OUT") || kw(&L->cur,"STDOUT") || kw(&L->cur,"1") ||
                 kw(&L->cur,"OUTPUT")){
        fd = 1; stream = "OUT";
        lex_next(L);
      } else if (kw(&L->cur,"ERR") || kw(&L->cur,"STDERR") || kw(&L->cur,"2") ||
                 kw(&L->cur,"ERROR")){
        fd = 2; stream = "ERR";
        lex_next(L);
      }
#if defined(CUBALC_OS_WINDOWS)
      /* no isatty guarantee — treat as non-tty unless _isatty */
#  if defined(_MSC_VER) || defined(__MINGW32__)
      n = (_isatty(fd) != 0) ? 1 : 0;
#  else
      n = 0;
#  endif
#else
      n = (isatty(fd) != 0) ? 1 : 0;
#endif
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "ISATTY", n);
      var_set_num(vm, "TTY", n);
      var_set_num(vm, "OK", 1);
      if (fd == 0) var_set_num(vm, "ISATTY_IN", n);
      else if (fd == 2) var_set_num(vm, "ISATTY_ERR", n);
      else var_set_num(vm, "ISATTY_OUT", n);
      var_set_str(vm, "ISATTY_STREAM", stream);
      snprintf(buf, sizeof buf, "%ld", n);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS TTYNAME|CTTY|TTYDEV [IN|OUT|ERR|CTRL] — terminal device path → LAST.
     * Default: controlling tty (ctermid) if available else stdin ttyname.
     * Soft empty LAST + LAST_N=0 when no tty (pipes); OK=1 always (probe).
     * Usability: agent session identity beyond ISATTY 0|1 without shell tty. */
    if (kw(&L->cur,"TTYNAME") || kw(&L->cur,"CTTY") || kw(&L->cur,"TTYDEV") ||
        kw(&L->cur,"TTYPATH") || kw(&L->cur,"TTY_NAME") || kw(&L->cur,"DEV_TTY") ||
        kw(&L->cur,"CONTROLLING_TTY") || kw(&L->cur,"CTERMID")){
      char op[24];
      int fd = -1; /* -1 = controlling */
      const char *stream = "CTRL";
      char name[256];
      const char *tn = NULL;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      lex_next(L);
      if (kw(&L->cur,"IN") || kw(&L->cur,"STDIN") || kw(&L->cur,"0") ||
          kw(&L->cur,"INPUT")){
        fd = 0; stream = "IN";
        lex_next(L);
      } else if (kw(&L->cur,"OUT") || kw(&L->cur,"STDOUT") || kw(&L->cur,"1") ||
                 kw(&L->cur,"OUTPUT")){
        fd = 1; stream = "OUT";
        lex_next(L);
      } else if (kw(&L->cur,"ERR") || kw(&L->cur,"STDERR") || kw(&L->cur,"2") ||
                 kw(&L->cur,"ERROR")){
        fd = 2; stream = "ERR";
        lex_next(L);
      } else if (kw(&L->cur,"CTRL") || kw(&L->cur,"CTL") || kw(&L->cur,"CONTROL") ||
                 kw(&L->cur,"CTTY") || kw(&L->cur,"CONTROLLING")){
        fd = -1; stream = "CTRL";
        lex_next(L);
      }
      name[0] = 0;
#if !defined(CUBALC_OS_WINDOWS)
      if (fd < 0) {
        /* controlling terminal path (may be /dev/tty even if not attached) */
        char ct[L_ctermid > 0 ? L_ctermid + 1 : 64];
        ct[0] = 0;
        if (ctermid(ct) && ct[0])
          snprintf(name, sizeof name, "%s", ct);
        /* prefer real ttyname of stdin if controlling not open */
        if (isatty(0)) {
          tn = ttyname(0);
          if (tn && tn[0]) snprintf(name, sizeof name, "%s", tn);
        }
      } else {
        if (isatty(fd)) {
          tn = ttyname(fd);
          if (tn && tn[0]) snprintf(name, sizeof name, "%s", tn);
        }
      }
#endif
      var_set_str(vm, "TTYNAME", name);
      var_set_str(vm, "TTYDEV", name);
      var_set_str(vm, "CTTY", name);
      var_set_str(vm, "TTYNAME_STREAM", stream);
      var_set_str(vm, "LAST", name);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", name);
      vm->last_n = (long)strlen(name);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "TTYNAME_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS NICE|GETNICE — process nice → LAST_N/NICE.
     * SYS NICE n | SETNICE n | RENICE n — set priority (higher n = lower prio).
     * Soft fail OK=0 + LAST_ERR if setpriority denied; get always soft-safe.
     * Usability: agent yield/backpressure without shell nice/renice. */
    if (kw(&L->cur,"NICE") || kw(&L->cur,"GETNICE") || kw(&L->cur,"GETPRIORITY") ||
        kw(&L->cur,"PRIORITY") || kw(&L->cur,"PRIO") ||
        kw(&L->cur,"SETNICE") || kw(&L->cur,"RENICE") || kw(&L->cur,"SETPRIORITY") ||
        kw(&L->cur,"SETPRIO")){
      char op[24];
      int force_set = 0, force_get = 0;
      long want = 0;
      int do_set = 0;
      long n = 0;
      char buf[32];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      force_set = (strcmp(op, "SETNICE") == 0 || strcmp(op, "RENICE") == 0 ||
                   strcmp(op, "SETPRIORITY") == 0 || strcmp(op, "SETPRIO") == 0);
      force_get = (strcmp(op, "GETNICE") == 0 || strcmp(op, "GETPRIORITY") == 0);
      lex_next(L);
      if (!force_get) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          want = parse_expr(vm, L);
          do_set = 1;
        } else if (L->cur.kind == TK_IDENT) {
          Var *gv = var_get(vm, L->cur.text, 0);
          if (gv && !gv->is_str) {
            want = gv->val;
            lex_next(L);
            do_set = 1;
          } else if (force_set) {
            /* SETNICE then statement keyword — missing value */
            var_set_str(vm, "LAST", "");
            vm->last_str[0] = 0;
            vm->last_n = 0;
            var_set_num(vm, "LAST_N", 0);
            var_set_num(vm, "OK", 0);
            var_set_str(vm, "LAST_ERR", "NICE: need priority value");
            var_set_str(vm, "ERR", "NICE: need priority value");
            bump(vm); return 1;
          }
        } else if (force_set) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "NICE: need priority value");
          var_set_str(vm, "ERR", "NICE: need priority value");
          bump(vm); return 1;
        }
      }
#if defined(CUBALC_OS_WINDOWS)
      n = 0;
      if (do_set) {
        /* no setpriority — report soft fail */
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "NICE: unsupported");
        var_set_str(vm, "ERR", "NICE: unsupported");
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
        bump(vm); return 1;
      }
#else
      if (do_set) {
        /* clamp to typical range */
        if (want < -20) want = -20;
        if (want > 19) want = 19;
        if (setpriority(PRIO_PROCESS, 0, (int)want) != 0) {
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "NICE: setpriority denied");
          var_set_str(vm, "ERR", "NICE: setpriority denied");
          /* still report current nice if readable */
          errno = 0;
          n = (long)getpriority(PRIO_PROCESS, 0);
          if (n == -1 && errno != 0) n = 0;
          vm->last_n = n;
          var_set_num(vm, "LAST_N", n);
          var_set_num(vm, "NICE", n);
          snprintf(buf, sizeof buf, "%ld", n);
          var_set_str(vm, "LAST", buf);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
          bump(vm); return 1;
        }
      }
      errno = 0;
      n = (long)getpriority(PRIO_PROCESS, 0);
      if (n == -1 && errno != 0) n = 0;
#endif
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "NICE", n);
      var_set_num(vm, "NICE_N", n);
      var_set_num(vm, "PRIORITY", n);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", n);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS LOADAVG|LOAD|GETLOADAVG — host load averages (1/5/15 min).
     * LAST = "a.bb c.dd e.ff" (space bag); LOAD1/5/15 str; LOAD1_N/5_N/15_N = centiload
     * (load*100 as int for IF/CMP without float). LAST_N = LOAD1_N (1-min primary).
     * Usability: agent backpressure / skip heavy work when load high (no shell uptime). */
    if (kw(&L->cur,"LOADAVG") || kw(&L->cur,"LOAD") || kw(&L->cur,"GETLOADAVG") ||
        kw(&L->cur,"LOAD_AVG") || kw(&L->cur,"LA") || kw(&L->cur,"SYSLOAD") ||
        kw(&L->cur,"HOSTLOAD")){
      double la[3] = {0.0, 0.0, 0.0};
      long c1 = 0, c5 = 0, c15 = 0;
      char bag[96], s1[24], s5[24], s15[24];
      int ok = 0;
      lex_next(L);
#if !defined(CUBALC_OS_WINDOWS)
      {
        int nla = 0;
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
        nla = getloadavg(la, 3);
#endif
        if (nla < 1) {
          /* /proc/loadavg fallback (Linux) */
          FILE *f = fopen("/proc/loadavg", "r");
          if (f) {
            if (fscanf(f, "%lf %lf %lf", &la[0], &la[1], &la[2]) >= 1)
              nla = 3;
            fclose(f);
          }
        }
        if (nla >= 1) ok = 1;
        if (nla < 3) {
          if (nla < 2) la[1] = la[0];
          la[2] = la[1];
        }
      }
#endif
      if (!ok) {
        /* soft zero plate when unsupported */
        la[0] = la[1] = la[2] = 0.0;
      }
      /* centiload: 1.23 → 123 */
      c1 = (long)(la[0] * 100.0 + (la[0] >= 0 ? 0.5 : -0.5));
      c5 = (long)(la[1] * 100.0 + (la[1] >= 0 ? 0.5 : -0.5));
      c15 = (long)(la[2] * 100.0 + (la[2] >= 0 ? 0.5 : -0.5));
      if (c1 < 0) c1 = 0;
      if (c5 < 0) c5 = 0;
      if (c15 < 0) c15 = 0;
      snprintf(s1, sizeof s1, "%.2f", la[0]);
      snprintf(s5, sizeof s5, "%.2f", la[1]);
      snprintf(s15, sizeof s15, "%.2f", la[2]);
      snprintf(bag, sizeof bag, "%s %s %s", s1, s5, s15);
      var_set_str(vm, "LAST", bag);
      var_set_str(vm, "LOADAVG", bag);
      var_set_str(vm, "LOAD", bag);
      var_set_str(vm, "LOAD1", s1);
      var_set_str(vm, "LOAD5", s5);
      var_set_str(vm, "LOAD15", s15);
      var_set_num(vm, "LOAD1_N", c1);
      var_set_num(vm, "LOAD5_N", c5);
      var_set_num(vm, "LOAD15_N", c15);
      var_set_num(vm, "LOADAVG_N", c1);
      var_set_num(vm, "LAST_N", c1);
      var_set_num(vm, "OK", ok ? 1 : 0);
      vm->last_n = c1;
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      if (!ok) {
        var_set_str(vm, "LAST_ERR", "LOADAVG: unavailable");
        var_set_str(vm, "ERR", "LOADAVG: unavailable");
      }
      bump(vm); return 1;
    }
    /* SYS UPTIME|BOOTAGE|UPSECS — seconds since boot → LAST_N/UPTIME.
     * LAST = decimal string; also UPTIME_MIN (minutes), UPTIME_HOUR (hours) ints.
     * Usability: agent warmup / lease age without shell uptime. */
    if (kw(&L->cur,"UPTIME") || kw(&L->cur,"BOOTAGE") || kw(&L->cur,"UPSECS") ||
        kw(&L->cur,"UP_SECONDS") || kw(&L->cur,"SECONDS_UP") || kw(&L->cur,"BOOT_AGE") ||
        kw(&L->cur,"HOST_UPTIME")){
      long n = 0;
      char buf[40];
      int ok = 0;
      lex_next(L);
#if !defined(CUBALC_OS_WINDOWS)
      {
        FILE *f = fopen("/proc/uptime", "r");
        if (f) {
          double up = 0.0;
          if (fscanf(f, "%lf", &up) == 1 && up >= 0.0) {
            n = (long)up;
            ok = 1;
          }
          fclose(f);
        }
#if defined(CLOCK_BOOTTIME)
        if (!ok) {
          struct timespec ts;
          if (clock_gettime(CLOCK_BOOTTIME, &ts) == 0 && ts.tv_sec >= 0) {
            n = (long)ts.tv_sec;
            ok = 1;
          }
        }
#endif
#if defined(CLOCK_MONOTONIC)
        if (!ok) {
          /* last resort: monotonic (approx; not true boot on all systems) */
          struct timespec ts;
          if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0 && ts.tv_sec >= 0) {
            n = (long)ts.tv_sec;
            ok = 1;
          }
        }
#endif
      }
#endif
      if (n < 0) n = 0;
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "UPTIME", n);
      var_set_num(vm, "UPTIME_N", n);
      var_set_num(vm, "BOOTAGE", n);
      var_set_num(vm, "UPTIME_MIN", n / 60);
      var_set_num(vm, "UPTIME_HOUR", n / 3600);
      var_set_num(vm, "OK", ok ? 1 : 0);
      snprintf(buf, sizeof buf, "%ld", n);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      if (!ok) {
        var_set_str(vm, "LAST_ERR", "UPTIME: unavailable");
        var_set_str(vm, "ERR", "UPTIME: unavailable");
      }
      bump(vm); return 1;
    }
    /* SYS MEM|MEMINFO|MEMORY — host RAM probe (kB).
     * MEM_TOTAL_N / MEM_AVAIL_N / MEM_FREE_N in kilobytes; LAST_N = MEM_AVAIL_N.
     * LAST = "total_kb avail_kb free_kb" bag for plates.
     * Usability: agent low-memory backpressure without shell free(1). */
    if (kw(&L->cur,"MEM") || kw(&L->cur,"MEMINFO") || kw(&L->cur,"MEMORY") ||
        kw(&L->cur,"RAM") || kw(&L->cur,"FREEMEM") || kw(&L->cur,"HOSTMEM") ||
        kw(&L->cur,"SYS_MEM")){
      long total_kb = 0, avail_kb = 0, free_kb = 0;
      char bag[96], buf[40];
      int ok = 0;
      lex_next(L);
#if !defined(CUBALC_OS_WINDOWS)
      {
        FILE *f = fopen("/proc/meminfo", "r");
        if (f) {
          char line[128];
          while (fgets(line, sizeof line, f)) {
            long v = 0;
            if (sscanf(line, "MemTotal: %ld kB", &v) == 1) total_kb = v;
            else if (sscanf(line, "MemAvailable: %ld kB", &v) == 1) avail_kb = v;
            else if (sscanf(line, "MemFree: %ld kB", &v) == 1) free_kb = v;
          }
          fclose(f);
          if (total_kb > 0) {
            ok = 1;
            if (avail_kb <= 0) avail_kb = free_kb; /* older kernels */
          }
        }
#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
        if (!ok) {
          long pages = sysconf(_SC_PHYS_PAGES);
          long pgsz = sysconf(_SC_PAGESIZE);
          long avpages = -1;
#if defined(_SC_AVPHYS_PAGES)
          avpages = sysconf(_SC_AVPHYS_PAGES);
#endif
          if (pages > 0 && pgsz > 0) {
            total_kb = (pages * (pgsz / 1024));
            if (pgsz < 1024)
              total_kb = (pages * pgsz) / 1024;
            if (avpages > 0) {
              if (pgsz >= 1024) avail_kb = avpages * (pgsz / 1024);
              else avail_kb = (avpages * pgsz) / 1024;
              free_kb = avail_kb;
            } else {
              avail_kb = total_kb;
              free_kb = total_kb;
            }
            ok = 1;
          }
        }
#endif
      }
#endif
      if (total_kb < 0) total_kb = 0;
      if (avail_kb < 0) avail_kb = 0;
      if (free_kb < 0) free_kb = 0;
      snprintf(bag, sizeof bag, "%ld %ld %ld", total_kb, avail_kb, free_kb);
      var_set_str(vm, "LAST", bag);
      var_set_str(vm, "MEM", bag);
      var_set_str(vm, "MEMINFO", bag);
      var_set_num(vm, "MEM_TOTAL_N", total_kb);
      var_set_num(vm, "MEM_AVAIL_N", avail_kb);
      var_set_num(vm, "MEM_FREE_N", free_kb);
      var_set_num(vm, "MEM_TOTAL_KB", total_kb);
      var_set_num(vm, "MEM_AVAIL_KB", avail_kb);
      var_set_num(vm, "MEM_FREE_KB", free_kb);
      /* primary: available kB for IF gates */
      vm->last_n = avail_kb;
      var_set_num(vm, "LAST_N", avail_kb);
      var_set_num(vm, "OK", ok ? 1 : 0);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      if (!ok) {
        var_set_str(vm, "LAST_ERR", "MEM: unavailable");
        var_set_str(vm, "ERR", "MEM: unavailable");
      }
      (void)buf;
      bump(vm); return 1;
    }
    /* SYS DF|DISKFREE|DISK|SPACE path — filesystem free space for path (kB).
     * DF_TOTAL_N / DF_AVAIL_N / DF_USED_N / DF_FREE_N in kilobytes; LAST_N = avail.
     * LAST = "total_kb avail_kb used_kb" bag; DF_PATH = resolved path.
     * Default path = "." (cwd) when omitted. Soft miss if path unstatable.
     * Usability: agent pre-write disk gate without shell df(1). */
    if (kw(&L->cur,"DF") || kw(&L->cur,"DISKFREE") || kw(&L->cur,"DISK") ||
        kw(&L->cur,"SPACE") || kw(&L->cur,"DISKSPACE") || kw(&L->cur,"FREESPACE") ||
        kw(&L->cur,"FSFREE") || kw(&L->cur,"STATVFS")){
      char path[512], bag[96];
      long total_kb = 0, avail_kb = 0, free_kb = 0, used_kb = 0;
      int ok = 0;
      lex_next(L);
      path[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN) {
        if (resolve_str_arg(vm, L, path, sizeof path) != 0)
          path[0] = 0;
      }
      if (!path[0])
        snprintf(path, sizeof path, ".");
#if !defined(CUBALC_OS_WINDOWS)
      {
        struct statvfs st;
        if (statvfs(path, &st) == 0) {
          unsigned long long bsize = (unsigned long long)st.f_frsize
              ? (unsigned long long)st.f_frsize
              : (unsigned long long)st.f_bsize;
          unsigned long long totb = (unsigned long long)st.f_blocks * bsize;
          unsigned long long freeb = (unsigned long long)st.f_bfree * bsize;
          unsigned long long availb = (unsigned long long)st.f_bavail * bsize;
          total_kb = (long)(totb / 1024ULL);
          free_kb = (long)(freeb / 1024ULL);
          avail_kb = (long)(availb / 1024ULL);
          if (total_kb >= avail_kb)
            used_kb = total_kb - avail_kb;
          else
            used_kb = 0;
          ok = 1;
        }
      }
#endif
      if (total_kb < 0) total_kb = 0;
      if (avail_kb < 0) avail_kb = 0;
      if (free_kb < 0) free_kb = 0;
      if (used_kb < 0) used_kb = 0;
      snprintf(bag, sizeof bag, "%ld %ld %ld", total_kb, avail_kb, used_kb);
      var_set_str(vm, "LAST", bag);
      var_set_str(vm, "DF", bag);
      var_set_str(vm, "DISKFREE", bag);
      var_set_str(vm, "DF_PATH", path);
      var_set_num(vm, "DF_TOTAL_N", total_kb);
      var_set_num(vm, "DF_AVAIL_N", avail_kb);
      var_set_num(vm, "DF_FREE_N", free_kb);
      var_set_num(vm, "DF_USED_N", used_kb);
      var_set_num(vm, "DF_TOTAL_KB", total_kb);
      var_set_num(vm, "DF_AVAIL_KB", avail_kb);
      var_set_num(vm, "DF_USED_KB", used_kb);
      vm->last_n = avail_kb;
      var_set_num(vm, "LAST_N", avail_kb);
      var_set_num(vm, "OK", ok ? 1 : 0);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      if (!ok) {
        char err[160];
        snprintf(err, sizeof err, "DF: cannot stat '%s'", path);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
      }
      bump(vm); return 1;
    }
    /* SYS UNAME|OS|SYSNAME — kernel/OS name → LAST/UNAME/OS (e.g. Linux, Darwin).
     * SYS ARCH|MACHINE|MACH|CPUARCH — machine arch → LAST/ARCH (e.g. x86_64, aarch64).
     * Usability: portable agent IF/guards without shell uname -sm. */
    if (kw(&L->cur,"UNAME") || kw(&L->cur,"OS") || kw(&L->cur,"SYSNAME") ||
        kw(&L->cur,"OSNAME") || kw(&L->cur,"KERNEL") || kw(&L->cur,"PLATFORM") ||
        kw(&L->cur,"ARCH") || kw(&L->cur,"MACHINE") || kw(&L->cur,"MACH") ||
        kw(&L->cur,"CPUARCH") || kw(&L->cur,"HW_MACHINE") || kw(&L->cur,"ISA_HOST")){
      char sysn[128], arch[128];
      int want_arch;
      const char *op = L->cur.text;
      want_arch = (kw(&L->cur,"ARCH") || kw(&L->cur,"MACHINE") || kw(&L->cur,"MACH") ||
                   kw(&L->cur,"CPUARCH") || kw(&L->cur,"HW_MACHINE") ||
                   kw(&L->cur,"ISA_HOST"));
      /* PLATFORM returns "os/arch" combined for one-shot plate tags */
      {
        int want_plat = kw(&L->cur,"PLATFORM");
        lex_next(L);
        sysn[0] = 0;
        arch[0] = 0;
#if defined(CUBALC_OS_WINDOWS)
        {
          const char *e = getenv("OS");
          if (e && e[0]) snprintf(sysn, sizeof sysn, "%s", e);
          else snprintf(sysn, sizeof sysn, "Windows");
          e = getenv("PROCESSOR_ARCHITECTURE");
          if (e && e[0]) snprintf(arch, sizeof arch, "%s", e);
          else snprintf(arch, sizeof arch, "unknown");
        }
#else
        {
          struct utsname u;
          if (uname(&u) == 0) {
            if (u.sysname[0]) snprintf(sysn, sizeof sysn, "%s", u.sysname);
            if (u.machine[0]) snprintf(arch, sizeof arch, "%s", u.machine);
          }
        }
#endif
        if (!sysn[0]) snprintf(sysn, sizeof sysn, "unknown");
        if (!arch[0]) snprintf(arch, sizeof arch, "unknown");
        /* always mirror both vars so agents can call either form once */
        var_set_str(vm, "UNAME", sysn);
        var_set_str(vm, "OS", sysn);
        var_set_str(vm, "SYSNAME", sysn);
        var_set_str(vm, "KERNEL", sysn);
        var_set_str(vm, "ARCH", arch);
        var_set_str(vm, "MACHINE", arch);
        var_set_str(vm, "MACH", arch);
        if (want_plat) {
          char plat[260];
          snprintf(plat, sizeof plat, "%s/%s", sysn, arch);
          var_set_str(vm, "PLATFORM", plat);
          var_set_str(vm, "LAST", plat);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", plat);
          vm->last_n = (long)strlen(plat);
        } else if (want_arch) {
          var_set_str(vm, "LAST", arch);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", arch);
          vm->last_n = (long)strlen(arch);
        } else {
          (void)op;
          var_set_str(vm, "LAST", sysn);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", sysn);
          vm->last_n = (long)strlen(sysn);
        }
        var_set_num(vm, "LAST_N", vm->last_n);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
    }

  return 0;
}
