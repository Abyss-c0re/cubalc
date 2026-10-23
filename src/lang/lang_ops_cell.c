/* CubalC lang — lang_ops_cell.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_cell_nibble4.h"
#include "lang/cubalc_lang_ops_cell_width8.h"
#include "lang/cubalc_lang_ops_cell_bitwise_w.h"
#include "lang/cubalc_lang_ops_cell_w16rot.h"
#include "lang/cubalc_lang_ops_cell_shift32.h"
#include "lang/cubalc_lang_ops_cell_rot32.h"
#include "lang/cubalc_lang_ops_cell_metrics.h"
#include "lang/cubalc_lang_ops_cell_rng.h"
#include "lang/cubalc_lang_ops_cell_energy.h"
#include "lang/cubalc_lang_ops_cell_matbits.h"
#include "lang/cubalc_lang_ops_cell_matscan.h"
#include "lang/cubalc_lang_ops_cell_matmetric.h"
#include "lang/cubalc_lang_ops_cell_matruns.h"
#include "lang/cubalc_lang_ops_cell_matfind.h"
#include "lang/cubalc_lang_ops_cell_matrange.h"
#include "lang/cubalc_lang_ops_cell_matword.h"
#include "lang/cubalc_lang_ops_cell_matpath.h"
#include "lang/cubalc_lang_ops_cell_matfill.h"
#include "lang/cubalc_lang_ops_cell_matalgebra.h"
#include "lang/cubalc_lang_ops_cell_matrel.h"
#include "lang/cubalc_lang_ops_cell_matmux.h"
#include "lang/cubalc_lang_ops_cell_matbrain.h"
#include "lang/cubalc_lang_ops_cell_energyflow.h"
#include "lang/cubalc_lang_ops_cell_runtime.h"
#include "lang/cubalc_lang_ops_cell_include_query.h"
#include "lang/cubalc_lang_ops_cell_include.h"
#include "lang/cubalc_lang_ops_cell_libcat.h"
#include "lang/cubalc_lang_ops_cell_libpick.h"
#include "lang/cubalc_lang_ops_cell_libread.h"
#include "lang/cubalc_lang_ops_cell_libmeta.h"
#include "lang/cubalc_lang_ops_cell_libage.h"
#include "lang/cubalc_lang_ops_cell_preload.h"
#include "lang/cubalc_lang_ops_cell_matlogic.h"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int cubalc_lang_ops_cell(VM *vm, Lex *L){
  /* nibble4 peel (AND4CELL…CLIPS4CELL) */
  {
    int n4 = cubalc_lang_ops_cell_try_nibble4(vm, L);
    if (n4 != 0) return n4;
  }
  /* width8 peel (SHL8CELL…CLIP8CELL) */
  {
    int w8 = cubalc_lang_ops_cell_try_width8(vm, L);
    if (w8 != 0) return w8;
  }
  /* bitwise_w peel (AND8CELL…NOT32CELL) */
  {
    int bw = cubalc_lang_ops_cell_try_bitwise_w(vm, L);
    if (bw != 0) return bw;
  }
  /* w16rot peel (BSWAP64…SANY/SFIND/SSCAN + fold/logic plane) */
  {
    int w16 = cubalc_lang_ops_cell_try_w16rot(vm, L);
    if (w16 != 0) return w16;
  }
  /* shift32 peel (SHL32CELL…SAR32CELL) */
  {
    int s32 = cubalc_lang_ops_cell_try_shift32(vm, L);
    if (s32 != 0) return s32;
  }
  /* rot32 peel (ROL32CELL…BITREV32CELL) */
  {
    int r32 = cubalc_lang_ops_cell_try_rot32(vm, L);
    if (r32 != 0) return r32;
  }
  /* metrics peel (PARITYCELL…BSWAP16CELL) */
  {
    int met = cubalc_lang_ops_cell_try_metrics(vm, L);
    if (met != 0) return met;
  }
  /* rng peel (RAND·SEED·SRAND·SHUFFLE·RANDBITS) */
  {
    int rng = cubalc_lang_ops_cell_try_rng(vm, L);
    if (rng != 0) return rng;
  }
  /* shift32/rot32/metrics/w16rot monolit bodies removed — try_* handles first (G_cell_margin) */

  /* energy peel (ENERGYGET·SET·ADD·SUB·XFER·CLAMP·SWAP·SHARE·fleet·SEFLOW) */
  {
    int en = cubalc_lang_ops_cell_try_energy(vm, L);
    if (en != 0) return en;
  }
  /* matbits peel (ROTBITS·SHL/SHR·SETBIT·COUNTBITS·GETBIT) */
  {
    int mb = cubalc_lang_ops_cell_try_matbits(vm, L);
    if (mb != 0) return mb;
  }
  /* matscan peel (FINDONE·REVBITS·SWAPBIT·DIFFBITS) */
  {
    int ms = cubalc_lang_ops_cell_try_matscan(vm, L);
    if (ms != 0) return ms;
  }
  /* matmetric peel (DOTBITS·JACCARD·MAJ·GRAY·CLZ/CTZ·ORPOP) */
  {
    int mm = cubalc_lang_ops_cell_try_matmetric(vm, L);
    if (mm != 0) return mm;
  }
  /* matruns peel (TRANSBITS·RUNSBITS·MASKPOP) */
  {
    int mr = cubalc_lang_ops_cell_try_matruns(vm, L);
    if (mr != 0) return mr;
  }
  /* matfind peel (MAXRUN·ZRUNS·FINDZERO) */
  {
    int mf = cubalc_lang_ops_cell_try_matfind(vm, L);
    if (mf != 0) return mf;
  }
  /* matrange peel (PARITYBITS·COPYRANGE·SWAPRANGE·ROTRANGE) */
  {
    int mrg = cubalc_lang_ops_cell_try_matrange(vm, L);
    if (mrg != 0) return mrg;
  }
  /* matword peel (WORDFROM·WORDTO·EXTRACTBITS·DEPOSITBITS) */
  {
    int mw = cubalc_lang_ops_cell_try_matword(vm, L);
    if (mw != 0) return mw;
  }
  /* matpath peel (PEXTBITS·PDEPBITS·ZIPBITS·UNZIPBITS) */
  {
    int mp = cubalc_lang_ops_cell_try_matpath(vm, L);
    if (mp != 0) return mp;
  }
  /* matfill peel (FILLRANGE·CLEARRANGE·FLIPRANGE·COUNTRANGE) */
  {
    int mf = cubalc_lang_ops_cell_try_matfill(vm, L);
    if (mf != 0) return mf;
  }
  /* matalgebra peel (ANDRANGE·ORRANGE·XORRANGE) */
  {
    int ma = cubalc_lang_ops_cell_try_matalgebra(vm, L);
    if (ma != 0) return ma;
  }
  /* matrel peel (EQBITS·NEBITS·SUBSETBITS·SUPERSETBITS·DISJOINTBITS) */
  {
    int mr = cubalc_lang_ops_cell_try_matrel(vm, L);
    if (mr != 0) return mr;
  }
  /* matmux peel (MUXBITS·BLENDBITS·MATCHBITS) */
  {
    int mm = cubalc_lang_ops_cell_try_matmux(vm, L);
    if (mm != 0) return mm;
  }
  /* matbrain peel (SETDIGIT·FOLDBITS·EEG*) */
  {
    int mb = cubalc_lang_ops_cell_try_matbrain(vm, L);
    if (mb != 0) return mb;
  }
  /* energyflow peel (COMPARE·HARMONY·RESOLVE·ENERGYFLOW·FLOWI) */
  {
    int ef = cubalc_lang_ops_cell_try_energyflow(vm, L);
    if (ef != 0) return ef;
  }
  /* runtime peel (DECIDE·VIZ·SPIN·WAIT) */
  {
    int rt = cubalc_lang_ops_cell_try_runtime(vm, L);
    if (rt != 0) return rt;
  }
  /* include query peel (INCLUDESTEMS·HASINCLUDE·NEEDINCLUDE) */
  {
    int iq = cubalc_lang_ops_cell_try_include_query(vm, L);
    if (iq != 0) return iq;
  }
  /* preload query peel (LISTPRELOAD·PRELOADMISS·HASPRELOAD) */
  {
    int pl = cubalc_lang_ops_cell_try_preload(vm, L);
    if (pl != 0) return pl;
  }
  /* matlogic peel (CLEARBITS·NOTBITS·ANDBITS·ORBITS·XORBITS·…) */
  {
    int ml = cubalc_lang_ops_cell_try_matlogic(vm, L);
    if (ml != 0) return ml;
  }
  /* INCLUDE monolit → lang_ops_cell_include.c (G_cell_margin peel) */
  {
    int inc = cubalc_lang_ops_cell_try_include(vm, L);
    if (inc != 0) return inc;
  }
  /* lib catalog LISTINCLUDES·LISTLIBS·MATCH* → lang_ops_cell_libcat.c (G_cell_margin) */
  {
    int lc = cubalc_lang_ops_cell_try_libcat(vm, L);
    if (lc != 0) return lc;
  }


  /* lib pick PICKLIB·NTHLIB·NEWEST/OLDEST·SORTLIBS → lang_ops_cell_libpick.c (G_cell_libpick) */
  {
    int lp = cubalc_lang_ops_cell_try_libpick(vm, L);
    if (lp != 0) return lp;
  }

  /* lib age FRESH/STALE·LIBAGE·HASFRESH·NEEDFRESH → lang_ops_cell_libage.c (G_cell_libage) */
  {
    int la = cubalc_lang_ops_cell_try_libage(vm, L);
    if (la != 0) return la;
  }

  /* lib read CATLIB·GREPLIB·HEAD/TAILLIB → lang_ops_cell_libread.c (G_cell_libread) */
  {
    int lr = cubalc_lang_ops_cell_try_libread(vm, L);
    if (lr != 0) return lr;
  }

  /* lib meta LIBDEPS·INFO·TREE·CHECK·DEFAULTS·RECIPE → lang_ops_cell_libmeta.c (G_cell_libmeta) */
  {
    int lm = cubalc_lang_ops_cell_try_libmeta(vm, L);
    if (lm != 0) return lm;
  }

  return 0;
}
