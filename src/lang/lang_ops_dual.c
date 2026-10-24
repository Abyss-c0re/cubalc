/* CubalC lang — lang_ops_dual.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_dual_early.h"
#include "lang/cubalc_lang_ops_dual_fieldrot.h"
#include "lang/cubalc_lang_ops_dual_immshift.h"
#include "lang/cubalc_lang_ops_dual_imm_alu.h"
#include "lang/cubalc_lang_ops_dual_imm_sat.h"
#include "lang/cubalc_lang_ops_dual_imm_div.h"
#include "lang/cubalc_lang_ops_dual_imm_bit.h"
#include "lang/cubalc_lang_ops_dual_imm_range.h"
#include "lang/cubalc_lang_ops_dual_imm_cmp.h"
#include "lang/cubalc_lang_ops_dual_bitpath.h"
#include "lang/cubalc_lang_ops_dual_bitmet.h"
#include "lang/cubalc_lang_ops_dual_datapath.h"
#include "lang/cubalc_lang_ops_dual_imm_field.h"
#include "lang/cubalc_lang_ops_dual_imm_wbit.h"
#include "lang/cubalc_lang_ops_dual_imm_h.h"
#include "lang/cubalc_lang_ops_dual_imm_b.h"
#include "lang/cubalc_lang_ops_dual_imm_n.h"

int cubalc_lang_ops_dual(VM *vm, Lex *L){
  /* early pack·clip·field32·unary*·pair → dual_early meta-try (B_dual_peel) */
  {
    int er = cubalc_lang_ops_dual_try_early(vm, L);
    if (er != 0) return er;
  }
  /* dual pair/ALU/cmp/gate/math/bit plane → lang_ops_dual_clip.c (G_dual_margin; 82 ops catalogued) */
  {
    int fr = cubalc_lang_ops_dual_try_fieldrot(vm, L);
    if (fr != 0) return fr;
  }
  {
    int ish = cubalc_lang_ops_dual_try_immshift(vm, L);
    if (ish != 0) return ish;
  }
  {
    int ia = cubalc_lang_ops_dual_try_imm_alu(vm, L);
    if (ia != 0) return ia;
  }
  {
    int isat = cubalc_lang_ops_dual_try_imm_sat(vm, L);
    if (isat != 0) return isat;
  }
  {
    int idv = cubalc_lang_ops_dual_try_imm_div(vm, L);
    if (idv != 0) return idv;
  }
  {
    int ibit = cubalc_lang_ops_dual_try_imm_bit(vm, L);
    if (ibit != 0) return ibit;
  }
  {
    int ir = cubalc_lang_ops_dual_try_imm_range(vm, L);
    if (ir != 0) return ir;
  }
  {
    int ic = cubalc_lang_ops_dual_try_imm_cmp(vm, L);
    if (ic != 0) return ic;
  }
  {
    int bp = cubalc_lang_ops_dual_try_bitpath(vm, L);
    if (bp != 0) return bp;
  }
  {
    int bm = cubalc_lang_ops_dual_try_bitmet(vm, L);
    if (bm != 0) return bm;
  }
  {
    int dp = cubalc_lang_ops_dual_try_datapath(vm, L);
    if (dp != 0) return dp;
  }
  {
    int ifd = cubalc_lang_ops_dual_try_imm_field(vm, L);
    if (ifd != 0) return ifd;
  }
  {
    int iw = cubalc_lang_ops_dual_try_imm_wbit(vm, L);
    if (iw != 0) return iw;
  }
  /* imm 16-bit h* family hbit…h0ord → dual_imm_h meta-try (B_dual_peel) */
  {
    int ihh = cubalc_lang_ops_dual_try_imm_h(vm, L);
    if (ihh != 0) return ihh;
  }
  /* imm 8-bit b* family barith…bbit → dual_imm_b meta-try (B_dual_peel) */
  {
    int ibb = cubalc_lang_ops_dual_try_imm_b(vm, L);
    if (ibb != 0) return ibb;
  }
  /* imm 4-bit n* family narith…nbit → dual_imm_n meta-try (B_dual_peel) */
  {
    int inn = cubalc_lang_ops_dual_try_imm_n(vm, L);
    if (inn != 0) return inn;
  }
  return 0;
}
