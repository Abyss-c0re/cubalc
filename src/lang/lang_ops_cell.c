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
#include "lang/cubalc_lang_ops_cell_mat.h"
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
  /* mat* family matbits…matbrain+matlogic → cell_mat meta-try (B_cell_peel) */
  {
    int mat = cubalc_lang_ops_cell_try_mat(vm, L);
    if (mat != 0) return mat;
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
