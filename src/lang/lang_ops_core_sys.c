/* CubalC lang — lang_ops_core_sys.c
 * SYS/HOST megablock dispatch (mesh vote A_split_lang_ops_core).
 * Peel of ~33k-line SYS plane from lang_ops_core.
 */
#include "lang/cubalc_lang_ops_core_sys.h"
#include "lang/cubalc_lang_ops_sys_plate.h"
#include "lang/cubalc_lang_ops_sys_path_fs.h"
#include "lang/cubalc_lang_ops_sys_env.h"
#include "lang/cubalc_lang_ops_sys_string.h"
#include "lang/cubalc_lang_ops_sys_wait.h"
#include "lang/cubalc_lang_ops_sys_stat.h"
#include "lang/cubalc_lang_ops_sys_bulk.h"
#include "lang/cubalc_lang_ops_sys_bulk_mut.h"
#include "lang/cubalc_lang_ops_sys_net.h"
#include "lang/cubalc_lang_ops_sys_files.h"
#include "lang/cubalc_lang_ops_sys_json.h"
#include "lang/cubalc_lang_ops_sys_streq.h"
#include "lang/cubalc_lang_ops_sys_bagmut.h"
#include "lang/cubalc_lang_ops_sys_rand.h"
#include "lang/cubalc_lang_ops_sys_bagshape.h"
#include "lang/cubalc_lang_ops_sys_bagmap.h"
#include "lang/cubalc_lang_ops_sys_bagwin.h"
#include "lang/cubalc_lang_ops_sys_baglen.h"
#include "lang/cubalc_lang_ops_sys_runtime.h"
#include "lang/cubalc_lang_ops_sys_misc.h"
#include "lang/cubalc_lang_ops_sys_argjoin.h"
#include "lang/cubalc_lang_ops_sys_numagg.h"
#include "lang/cubalc_lang_ops_sys_hostmeta.h"
#include "lang/cubalc_lang_ops_sys_kv.h"
#include "lang/cubalc_lang_ops_sys_kvmath.h"
#include "lang/cubalc_lang_ops_sys_kvfilter.h"
#include "lang/cubalc_lang_ops_sys_grep.h"
#include "lang/cubalc_lang_ops_sys_identity.h"
#include "lang/cubalc_lang_ops_sys_codec.h"
#include "lang/cubalc_lang_ops_sys_textio.h"
#include "lang/cubalc_lang_ops_sys_time_walk.h"
#include "lang/cubalc_lang_ops_sys_human.h"
#include "lang/cubalc_lang_ops_sys_time_date.h"
#include "lang/cubalc_lang_ops_sys_plate_expand.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include "lang/cubalc_lang_ops_core_help.h"
#include "cubalc_hostops.h"
#include "cubalc_async.h"
#include "cubalc_smx.h"
#include "cubalc_law.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <dirent.h>
#  include <sys/stat.h>
#  include <sys/statvfs.h>
#  include <fcntl.h>
#  include <poll.h>
#  include <pwd.h>
#  include <grp.h>
#  include <fnmatch.h>
#  include <sys/utsname.h>
#  include <sys/resource.h>
#  include <sys/socket.h>
#  include <sys/wait.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
/* getloadavg is BSD/POSIX extension; declare if libc omits under strict feature macros. */
int getloadavg(double loadavg[], int nelem);
#endif

int cubalc_lang_ops_core_try_sys(VM *vm, Lex *L) {
  /* SYS host ops — C runtime (Grokium without Python) */
  if (kw(&L->cur,"SYS") || kw(&L->cur,"HOST")){
    lex_next(L);
    skip_nl(L);
    /* plate/JSON-file theme → lang_ops_sys_plate.c (mesh vote A2) */
    {
      int pr = cubalc_lang_ops_sys_try_plate(vm, L);
      if (pr != 0) return pr;
    }

    /* path/FS theme → lang_ops_sys_path_fs.c (mesh vote A2) */
    {
      int fr = cubalc_lang_ops_sys_try_path_fs(vm, L);
      if (fr != 0) return fr;
    }

    /* ENV/process theme → lang_ops_sys_env.c (mesh vote A2) */
    {
      int er = cubalc_lang_ops_sys_try_env(vm, L);
      if (er != 0) return er;
    }

    /* string/codec theme → lang_ops_sys_string.c (mesh vote A2) */
    {
      int sr = cubalc_lang_ops_sys_try_string(vm, L);
      if (sr != 0) return sr;
    }

    /* wait/poll theme → lang_ops_sys_wait.c (mesh vote A2) */
    {
      int wr = cubalc_lang_ops_sys_try_wait(vm, L);
      if (wr != 0) return wr;
    }

    /* stat/meta theme → lang_ops_sys_stat.c (mesh vote A2) */
    {
      int tr = cubalc_lang_ops_sys_try_stat(vm, L);
      if (tr != 0) return tr;
    }

    /* bulk bag map/filter → lang_ops_sys_bulk.c (mesh vote A2) */
    {
      int br = cubalc_lang_ops_sys_try_bulk(vm, L);
      if (br != 0) return br;
    }

    /* bulk mutate → lang_ops_sys_bulk_mut.c (mesh vote A2) */
    {
      int mr = cubalc_lang_ops_sys_try_bulk_mut(vm, L);
      if (mr != 0) return mr;
    }

    /* net/TCP peer xfer → lang_ops_sys_net.c (mesh vote A2) */
    {
      int nr = cubalc_lang_ops_sys_try_net(vm, L);
      if (nr != 0) return nr;
    }

    /* files bag/content → lang_ops_sys_files.c */
    {
      int fir = cubalc_lang_ops_sys_try_files(vm, L);
      if (fir != 0) return fir;
    }

    /* residual SPAWN/CHAT/NUM/STR/LASTMATCH → lang_ops_sys_misc.c */
    {
      int mx = cubalc_lang_ops_sys_try_misc(vm, L);
      if (mx != 0) return mx;
    }

    /* JOIN/ARGC/ARGS/ARG → lang_ops_sys_argjoin.c (B_core_sys) */
    {
      int aj = cubalc_lang_ops_sys_try_argjoin(vm, L);
      if (aj != 0) return aj;
    }

    /* JSON plate theme → lang_ops_sys_json.c (A2) */
    {
      int jr = cubalc_lang_ops_sys_try_json(vm, L);
      if (jr != 0) return jr;
    }

    /* human size/duration → lang_ops_sys_human.c (mesh vote A2) */
    {
      int hu = cubalc_lang_ops_sys_try_human(vm, L);
      if (hu != 0) return hu;
    }

    /* runtime TIME/SLEEP/APPEND → lang_ops_sys_runtime.c (B_core_sys) */
    {
      int rt = cubalc_lang_ops_sys_try_runtime(vm, L);
      if (rt != 0) return rt;
    }

    /* bag field metrics → lang_ops_sys_baglen.c (B_core_sys) */
    {
      int bl = cubalc_lang_ops_sys_try_baglen(vm, L);
      if (bl != 0) return bl;
    }

    /* host-meta theme → lang_ops_sys_hostmeta.c (mesh vote A2) */
    {
      int hm = cubalc_lang_ops_sys_try_hostmeta(vm, L);
      if (hm != 0) return hm;
    }

    /* RAND/PICK/SHUFFLE → lang_ops_sys_rand.c (B_core_sys) */
    {
      int rd = cubalc_lang_ops_sys_try_rand(vm, L);
      if (rd != 0) return rd;
    }

    /* host numeric MIN..RANGE → lang_ops_sys_numagg.c (mesh vote A2) */
    {
      int na = cubalc_lang_ops_sys_try_numagg(vm, L);
      if (na != 0) return na;
    }

    /* time/date theme → lang_ops_sys_time_date.c (mesh vote A2) */
    {
      int td = cubalc_lang_ops_sys_try_time_date(vm, L);
      if (td != 0) return td;
    }

    /* identity theme → lang_ops_sys_identity.c (mesh vote A2) */
    {
      int id = cubalc_lang_ops_sys_try_identity(vm, L);
      if (id != 0) return id;
    }

    /* text I/O HEX..NTH → lang_ops_sys_textio.c (mesh vote A2) */
    {
      int tio = cubalc_lang_ops_sys_try_textio(vm, L);
      if (tio != 0) return tio;
    }

    /* bag-GREP theme → lang_ops_sys_grep.c (mesh vote A2) */
    {
      int gr = cubalc_lang_ops_sys_try_grep(vm, L);
      if (gr != 0) return gr;
    }

    /* bag shape TAKE..ZIP → lang_ops_sys_bagshape.c (B_core_sys) */
    {
      int bs = cubalc_lang_ops_sys_try_bagshape(vm, L);
      if (bs != 0) return bs;
    }

    /* bag map PREFIX..FIRSTMATCH → lang_ops_sys_bagmap.c (mesh vote A2) */
    {
      int bmap = cubalc_lang_ops_sys_try_bagmap(vm, L);
      if (bmap != 0) return bmap;
    }

    /* bag-KV theme → lang_ops_sys_kv.c (mesh vote A2) */
    {
      int kr = cubalc_lang_ops_sys_try_kv(vm, L);
      if (kr != 0) return kr;
    }

    /* bag-KV math theme → lang_ops_sys_kvmath.c (mesh vote A2) */
    {
      int km = cubalc_lang_ops_sys_try_kvmath(vm, L);
      if (km != 0) return km;
    }

    /* bag-KV filter theme → lang_ops_sys_kvfilter.c (mesh vote A2) */
    {
      int kf = cubalc_lang_ops_sys_try_kvfilter(vm, L);
      if (kf != 0) return kf;
    }

    /* bag window/reorder → lang_ops_sys_bagwin.c (B_core_sys) */
    {
      int bw = cubalc_lang_ops_sys_try_bagwin(vm, L);
      if (bw != 0) return bw;
    }

    /* bag-line mutators → lang_ops_sys_bagmut.c (B_core_sys) */
    {
      int bm = cubalc_lang_ops_sys_try_bagmut(vm, L);
      if (bm != 0) return bm;
    }

    /* string eq/transform residual → lang_ops_sys_streq.c (A2) */
    {
      int xr = cubalc_lang_ops_sys_try_streq(vm, L);
      if (xr != 0) return xr;
    }

    {
      char sug[48], ebuf[192], typo[64];
      if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR)
        snprintf(typo, sizeof typo, "%s", L->cur.text);
      else
        typo[0] = 0;
      sys_suggest_op(typo, sug, sizeof sug);
      if (sug[0] && typo[0])
        snprintf(ebuf, sizeof ebuf,
                 "SYS unknown '%s' — did you mean %s? (cubalc forms SYS / HELP)",
                 typo, sug);
      else if (typo[0])
        snprintf(ebuf, sizeof ebuf,
                 "SYS unknown '%s' — try cubalc forms SYS or HELP SYS <name>",
                 typo);
      else
        snprintf(ebuf, sizeof ebuf,
                 "SYS needs op — try cubalc forms SYS or HELP SYS ENV");
      fail(vm, ebuf);
    }
    return -1;
  }

  return 0;
}
