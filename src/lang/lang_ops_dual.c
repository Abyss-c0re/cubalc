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
#include "lang/cubalc_lang_ops_dual_imm_hbit.h"
#include "lang/cubalc_lang_ops_dual_imm_harith.h"
#include "lang/cubalc_lang_ops_dual_imm_hdiv.h"
#include "lang/cubalc_lang_ops_dual_imm_hcmp.h"
#include "lang/cubalc_lang_ops_dual_imm_hord.h"
#include "lang/cubalc_lang_ops_dual_imm_hsord.h"
#include "lang/cubalc_lang_ops_dual_imm_hsmm.h"
#include "lang/cubalc_lang_ops_dual_imm_hsclamp.h"
#include "lang/cubalc_lang_ops_dual_imm_hpred.h"
#include "lang/cubalc_lang_ops_dual_imm_h0ord.h"
#include "lang/cubalc_lang_ops_dual_imm_barith.h"
#include "lang/cubalc_lang_ops_dual_imm_bdiv.h"
#include "lang/cubalc_lang_ops_dual_imm_bcmp.h"
#include "lang/cubalc_lang_ops_dual_imm_bord.h"
#include "lang/cubalc_lang_ops_dual_imm_bsord.h"
#include "lang/cubalc_lang_ops_dual_imm_bsmm.h"
#include "lang/cubalc_lang_ops_dual_imm_bsclamp.h"
#include "lang/cubalc_lang_ops_dual_imm_bpred.h"
#include "lang/cubalc_lang_ops_dual_imm_b0ord.h"
#include "lang/cubalc_lang_ops_dual_imm_bbit.h"
#include "lang/cubalc_lang_ops_dual_imm_narith.h"
#include "lang/cubalc_lang_ops_dual_imm_ndiv.h"
#include "lang/cubalc_lang_ops_dual_imm_ncmp.h"
#include "lang/cubalc_lang_ops_dual_imm_nord.h"
#include "lang/cubalc_lang_ops_dual_imm_nsord.h"
#include "lang/cubalc_lang_ops_dual_imm_nsmm.h"
#include "lang/cubalc_lang_ops_dual_imm_nsclamp.h"
#include "lang/cubalc_lang_ops_dual_imm_npred.h"
#include "lang/cubalc_lang_ops_dual_imm_nbit.h"

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
  {
    int ih = cubalc_lang_ops_dual_try_imm_hbit(vm, L);
    if (ih != 0) return ih;
  }
  {
    int ihar = cubalc_lang_ops_dual_try_imm_harith(vm, L);
    if (ihar != 0) return ihar;
  }
  {
    int ihd = cubalc_lang_ops_dual_try_imm_hdiv(vm, L);
    if (ihd != 0) return ihd;
  }
  {
    int ihc = cubalc_lang_ops_dual_try_imm_hcmp(vm, L);
    if (ihc != 0) return ihc;
  }
  {
    int iho = cubalc_lang_ops_dual_try_imm_hord(vm, L);
    if (iho != 0) return iho;
  }
  {
    int ihs = cubalc_lang_ops_dual_try_imm_hsord(vm, L);
    if (ihs != 0) return ihs;
  }
  {
    int ihm = cubalc_lang_ops_dual_try_imm_hsmm(vm, L);
    if (ihm != 0) return ihm;
  }
  {
    int ihc2 = cubalc_lang_ops_dual_try_imm_hsclamp(vm, L);
    if (ihc2 != 0) return ihc2;
  }
  {
    int ihp = cubalc_lang_ops_dual_try_imm_hpred(vm, L);
    if (ihp != 0) return ihp;
  }
  {
    int ih0 = cubalc_lang_ops_dual_try_imm_h0ord(vm, L);
    if (ih0 != 0) return ih0;
  }
  {
    int iba = cubalc_lang_ops_dual_try_imm_barith(vm, L);
    if (iba != 0) return iba;
  }
  {
    int ibd = cubalc_lang_ops_dual_try_imm_bdiv(vm, L);
    if (ibd != 0) return ibd;
  }
  {
    int ibc = cubalc_lang_ops_dual_try_imm_bcmp(vm, L);
    if (ibc != 0) return ibc;
  }
  {
    int ibo = cubalc_lang_ops_dual_try_imm_bord(vm, L);
    if (ibo != 0) return ibo;
  }
  {
    int ibs = cubalc_lang_ops_dual_try_imm_bsord(vm, L);
    if (ibs != 0) return ibs;
  }
  {
    int ibm = cubalc_lang_ops_dual_try_imm_bsmm(vm, L);
    if (ibm != 0) return ibm;
  }
  {
    int ibc = cubalc_lang_ops_dual_try_imm_bsclamp(vm, L);
    if (ibc != 0) return ibc;
  }
  {
    int ibp = cubalc_lang_ops_dual_try_imm_bpred(vm, L);
    if (ibp != 0) return ibp;
  }
  {
    int ib0 = cubalc_lang_ops_dual_try_imm_b0ord(vm, L);
    if (ib0 != 0) return ib0;
  }
  {
    int ibb = cubalc_lang_ops_dual_try_imm_bbit(vm, L);
    if (ibb != 0) return ibb;
  }
  {
    int ina = cubalc_lang_ops_dual_try_imm_narith(vm, L);
    if (ina != 0) return ina;
  }
  {
    int ind = cubalc_lang_ops_dual_try_imm_ndiv(vm, L);
    if (ind != 0) return ind;
  }
  {
    int inc = cubalc_lang_ops_dual_try_imm_ncmp(vm, L);
    if (inc != 0) return inc;
  }
  {
    int ino = cubalc_lang_ops_dual_try_imm_nord(vm, L);
    if (ino != 0) return ino;
  }
  {
    int ins = cubalc_lang_ops_dual_try_imm_nsord(vm, L);
    if (ins != 0) return ins;
  }
  {
    int inm = cubalc_lang_ops_dual_try_imm_nsmm(vm, L);
    if (inm != 0) return inm;
  }
  {
    int inc = cubalc_lang_ops_dual_try_imm_nsclamp(vm, L);
    if (inc != 0) return inc;
  }
  /* imm 4-bit sign/zero preds DSIGN4N·D0EQ4N·D0NE4N·D0LT4N·D0GT4N·D0LE4N·D0GE4N → dual_imm_npred */
  {
    int inp = cubalc_lang_ops_dual_try_imm_npred(vm, L);
    if (inp != 0) return inp;
  }
  /* imm 4-bit field bitwise DAND4N·DOR4N·DXOR4N·DNAND4N·DNOR4N·DXNOR4N → dual_imm_nbit */
  {
    int inb = cubalc_lang_ops_dual_try_imm_nbit(vm, L);
    if (inb != 0) return inb;
  }
  return 0;
}
