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
#include "lang/cubalc_lang_ops_cell_inc.h"
#include "lang/cubalc_lang_ops_cell_lib.h"

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
  /* runtime·include_query·preload·include → cell_inc meta-try (B_cell_peel) */
  {
    int inc = cubalc_lang_ops_cell_try_inc(vm, L);
    if (inc != 0) return inc;
  }
  /* lib* family libcat·libpick·libage·libread·libmeta → cell_lib meta-try (B_cell_peel) */
  {
    int lib = cubalc_lang_ops_cell_try_lib(vm, L);
    if (lib != 0) return lib;
  }

  return 0;
}
