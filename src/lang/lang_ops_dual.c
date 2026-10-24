/* CubalC lang — lang_ops_dual.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_dual_pack.h"
#include "lang/cubalc_lang_ops_dual_clip.h"
#include "lang/cubalc_lang_ops_dual_field32.h"
#include "lang/cubalc_lang_ops_dual_unary32.h"
#include "lang/cubalc_lang_ops_dual_unary16.h"
#include "lang/cubalc_lang_ops_dual_unary8.h"
#include "lang/cubalc_lang_ops_dual_unary4.h"
#include "lang/cubalc_lang_ops_dual_pair.h"
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
  /* pack/unpack peel (DLO4…DUNPACK32) */
  {
    int pr = cubalc_lang_ops_dual_try_pack(vm, L);
    if (pr != 0) return pr;
  }
  /* clip/sext/rotate peel (DCLIP32…DROR32) */
  {
    int cr = cubalc_lang_ops_dual_try_clip(vm, L);
    if (cr != 0) return cr;
  }
  /* imm-32 field peel (DADD32N…D0GE32N) */
  {
    int f32 = cubalc_lang_ops_dual_try_field32(vm, L);
    if (f32 != 0) return f32;
  }
  /* imm-32 unary/shift/metrics peel (DNOT32N…DPARITY32N) */
  {
    int u32 = cubalc_lang_ops_dual_try_unary32(vm, L);
    if (u32 != 0) return u32;
  }
  /* imm-16 unary/shift/metrics peel (DABS16N…DNEG16N) */
  {
    int u16 = cubalc_lang_ops_dual_try_unary16(vm, L);
    if (u16 != 0) return u16;
  }
  /* imm-8 unary/shift/metrics peel (DNOT8N…DZEXT8N) */
  {
    int u8 = cubalc_lang_ops_dual_try_unary8(vm, L);
    if (u8 != 0) return u8;
  }
  /* imm-4 unary/shift/metrics peel (DNOT4N…DZEXT4N) */
  {
    int u4 = cubalc_lang_ops_dual_try_unary4(vm, L);
    if (u4 != 0) return u4;
  }
  {
    int pa = cubalc_lang_ops_dual_try_pair(vm, L);
    if (pa != 0) return pa;
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
